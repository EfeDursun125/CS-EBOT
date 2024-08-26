#include <core.h>
ConVar ebot_kill_breakables("ebot_kill_breakables", "0");

void Bot::DestroyBreakableStart(void)
{
	SelectBestWeapon();
}

void Bot::DestroyBreakableUpdate(void)
{
	if (!DestroyBreakableReq())
	{
		FinishCurrentProcess("sucsessfully destroyed a breakable");
		return;
	}

	if (ebot_kill_breakables.GetBool())
		m_breakableEntity->v.health = -1.0f;

	CheckStuck(pev->maxspeed, m_frameInterval);

	if (m_stuckWarn > 19)
	{
		FinishCurrentProcess("i'm stuck");
		return;
	}

	LookAt(m_breakable);
	m_enemyDistance = FLT_MAX;
	m_nearestEnemy = m_breakableEntity;
	m_entityDistance = (pev->origin - m_breakable).GetLengthSquared();
	m_nearestEntity = m_breakableEntity;

	if (pev->origin.z > m_breakable.z)
		m_duckTime = engine->GetTime() + 1.0f;
	else if (!IsVisible(m_breakable, GetEntity()))
		m_duckTime = engine->GetTime() + 1.0f;

	if (!m_isZombieBot && m_currentWeapon != Weapon::Knife)
		FireWeapon();
	else
	{
		SelectBestWeapon();
		KnifeAttack();
	}
}

void Bot::DestroyBreakableEnd(void)
{
	m_nearestEntity = nullptr;
	m_nearestEnemy = nullptr;
	m_enemyDistance = FLT_MAX;
	m_entityDistance = FLT_MAX;
	m_hasEntitiesNear = false;
	m_hasEnemiesNear = false;
}

bool Bot::DestroyBreakableReq(void)
{
	if (FNullEnt(m_breakableEntity))
		return false;

	if (m_breakableEntity->v.health <= 0.0f)
		return false;

	return true;
}