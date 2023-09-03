#include <core.h>

void Bot::DestroyBreakableStart(void)
{
	SelectBestWeapon(true, true);
}

void Bot::DestroyBreakableUpdate(void)
{
	if (FNullEnt(m_breakableEntity) || m_breakableEntity->v.health <= 0.0f)
	{
		FinishCurrentProcess("sucsessfully destroyed a breakable");
		return;
	}

	CheckStuck(pev->maxspeed);

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

	if (pev->origin.z < m_breakable.z)
		pev->button |= IN_DUCK;

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
}

bool Bot::DestroyBreakableReq(void)
{
	if (m_breakableEntity->v.health <= 0.0f)
		return false;

	return true;
}