#include "../../include/core.h"
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

	LookAt(m_breakableOrigin);
	if (pev->origin.z > m_breakableOrigin.z)
		m_duckTime = engine->GetTime() + 1.0f;
	else if (!IsVisible(m_breakableOrigin, m_myself))
		m_duckTime = engine->GetTime() + 1.0f;

	if (!m_isZombieBot && m_currentWeapon != Weapon::Knife)
		FireWeapon((pev->origin - m_breakableOrigin).GetLengthSquared());
	else
	{
		MoveTo(m_breakableOrigin, true);
		SelectBestWeapon();
		m_buttons |= IN_ATTACK;
	}

	IgnoreCollisionShortly();
	m_pauseTime = engine->GetTime() + crandomfloat(2.0f, 7.0f);
}

void Bot::DestroyBreakableEnd(void)
{
	if (!FNullEnt(m_breakableEntity) && m_breakableEntity->v.health > 0.0f)
		m_ignoreEntity = m_breakableEntity;

	m_breakableEntity = nullptr;
}

bool Bot::DestroyBreakableReq(void)
{
	if (FNullEnt(m_breakableEntity))
		return false;

	if (m_breakableEntity->v.health <= 0.0f)
		return false;

	if (m_breakableEntity->v.takedamage == DAMAGE_NO)
		return false;

	if (m_ignoreEntity == m_breakableEntity)
		return false;

	return true;
}