#include <core.h>

void Bot::DestroyBreakableStart(void)
{
	SelectBestWeapon();
}

void Bot::DestroyBreakableUpdate(void)
{
	CheckStuck(pev->maxspeed);

	if (m_stuckWarn >= 20)
	{
		FinishCurrentProcess("i'm stuck");
		return;
	}

	if (!IsShootableBreakable(m_breakableEntity))
	{
		FinishCurrentProcess("sucsessfully destroyed a breakable");
		return;
	}

	LookAt(m_breakable);
	m_enemyDistance = FLT_MAX;
	m_nearestEnemy = m_breakableEntity;
	m_entityDistance = (pev->origin - m_breakable).GetLengthSquared();
	m_nearestEntity = m_breakableEntity;

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
	if (!IsShootableBreakable(m_breakableEntity))
		return false;

	return true;
}