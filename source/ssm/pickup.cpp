#include <core.h>

void Bot::PickupStart(void)
{

}

void Bot::PickupUpdate(void)
{
	if (!m_buyingFinished && !PickupReq())
	{
		m_pickupItem = nullptr;
		m_pickupType = PickupType::None;
		FinishCurrentProcess("i can't pickup this item...");
		return;
	}

	if (!m_isSlowThink)
		FindItem();

	const Vector destination = GetEntityOrigin(m_pickupItem);

	// where are you???
	if (destination == nullvec)
	{
		m_pickupItem = nullptr;
		m_pickupType = PickupType::None;
		FinishCurrentProcess("pickup item is disappeared...");
		return;
	}

	FindEnemyEntities();
	FindFriendsAndEnemiens();
	UpdateLooking();
	
	m_destOrigin = destination;

	MoveTo(destination);
	CheckStuck(pev->maxspeed);

	switch (m_pickupType)
	{
	case PickupType::GetEntity:
	{
		if (FNullEnt(m_pickupItem) || (GetTeam(m_pickupItem) != Team(-1) && m_team != GetTeam(m_pickupItem)))
		{
			m_pickupItem = nullptr;
			m_pickupType = PickupType::None;
		}
		break;
	}
	case PickupType::Weapon:
	{
		// near to weapon?
		const float itemDistance = (destination - pev->origin).GetLengthSquared();
		if (itemDistance < squaredf(64.0f))
		{
			int i;
			for (i = 0; i < 7; i++)
			{
				if (cstrcmp(g_weaponSelect[i].modelName, STRING(m_pickupItem->v.model) + 9) == 0)
					break;
			}

			if (i < 7)
			{
				// secondary weapon. i.e., pistol
				int weaponID = 0;
				for (i = 0; i < 7; i++)
				{
					if (pev->weapons & (1 << g_weaponSelect[i].id))
						weaponID = i;
				}

				if (weaponID > 0)
				{
					SelectWeaponbyNumber(weaponID);
					FakeClientCommand(GetEntity(), "drop");
					if (HasShield()) // If we have the shield...
						FakeClientCommand(GetEntity(), "drop"); // discard both shield and pistol
				}
			}
			else
			{
				// primary weapon
				const int weaponID = GetHighestWeapon();
				if (weaponID > 6 || HasShield())
				{
					SelectWeaponbyNumber(weaponID);
					FakeClientCommand(GetEntity(), "drop");
				}
			}

			CheckSilencer(); // check the silencer
			if (IsValidWaypoint(m_currentWaypointIndex))
			{
				if (itemDistance > squaredi(m_waypoint.radius))
					FindWaypoint();
			}
		}
		break;
	}
	case PickupType::Shield:
	{
		if (HasShield())
		{
			m_pickupItem = nullptr;
			break;
		}

		const float itemDistance = (destination - pev->origin).GetLengthSquared();
		if (itemDistance < squaredf(64.0f)) // near to shield?
		{
			// get current best weapon to check if it's a primary in need to be dropped
			const int weaponID = GetHighestWeapon();
			if (weaponID > 6)
			{
				SelectWeaponbyNumber(weaponID);
				FakeClientCommand(GetEntity(), "drop");

				if (IsValidWaypoint(m_currentWaypointIndex))
				{
					if (itemDistance > squaredi(m_waypoint.radius))
						FindWaypoint();
				}
			}
		}
		break;
	}
	case PickupType::PlantedC4:
	{
		if (m_team == Team::Counter && (destination - pev->origin).GetLengthSquared() < squaredf(64.0f))
		{
			if (!SetProcess(Process::Defuse, "trying to defusing the bomb", false, engine->GetTime() + m_hasDefuser ? 6.0f : 12.0f))
				FinishCurrentProcess("cannot start to defuse bomb");
		}
		break;
	}
	case PickupType::Hostage:
	{
		if (m_team != Team::Counter || !IsAlive(m_pickupItem))
		{
			// don't pickup dead hostages
			m_pickupItem = nullptr;
			FinishCurrentProcess("hostage is dead");
			break;
		}

		LookAt(destination);

		// let A* find better path for hostages
		m_navNode.Clear();

		if ((destination - pev->origin).GetLengthSquared() < squaredf(64.0f))
		{
			if (g_gameVersion & Game::Xash)
				pev->buttons |= IN_USE;
			else // use game dll function to make sure the hostage is correctly 'used'
				MDLL_Use(m_pickupItem, GetEntity());

			for (auto& hostage : m_hostages)
			{
				if (FNullEnt(hostage)) // store pointer to hostage so other bots don't steal from this one or bot tries to reuse it
				{
					hostage = m_pickupItem;
					m_pickupItem = nullptr;
					break;
				}
			}
		}
		break;
	}
	case PickupType::DefuseKit:
	{
		if (m_hasDefuser || m_team != Team::Counter)
		{
			m_pickupItem = nullptr;
			m_pickupType = PickupType::None;
		}
		break;
	}
	case PickupType::Button:
	{
		if (FNullEnt(m_pickupItem) || m_buttonPushTime < engine->GetTime()) // it's safer...
		{
			FinishCurrentProcess("button is gone...");
			m_pickupType = PickupType::None;
			break;
		}

		LookAt(destination);

		// find angles from bot origin to entity...
		if ((destination - pev->origin).GetLengthSquared() < squaredf(90.0f)) // near to the button?
		{
			if (InFieldOfView(destination - EyePosition()) < 15) // facing it directly?
			{
				if (g_gameVersion & Game::Xash)
					pev->buttons |= IN_USE;
				else
					MDLL_Use(m_pickupItem, GetEntity());

				m_pickupItem = nullptr;
				m_pickupType = PickupType::None;
				m_buttonPushTime = engine->GetTime();
				FinishCurrentProcess("i have pushed the button");
			}
		}
		break;
	}
	}
}

void Bot::PickupEnd(void)
{
	m_navNode.Clear();
	FindWaypoint();
}

bool Bot::PickupReq(void)
{
	if (!m_buyingFinished)
		return false;

	return AllowPickupItem();
}