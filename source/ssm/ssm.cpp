#include <core.h>

Process Bot::GetCurrentState(void)
{
	return m_currentProcess;
}

float Bot::GetCurrentStateTime(void)
{
	return m_currentProcessTime - engine->GetTime();
}

bool Bot::SetProcess(const Process& process, const char* debugnote, const bool rememberProcess, const float time)
{
	if (m_currentProcess != process && IsReadyForTheProcess(process))
	{
		extern ConVar ebot_debug;
		if (ebot_debug.GetBool())
			ServerPrint("%s has got a new process from %s to %s | process started -> %s", GetEntityName(GetEntity()), GetProcessName(m_currentProcess), GetProcessName(process), debugnote);

		if (rememberProcess && m_currentProcess > Process::Default && IsReadyForTheProcess(m_currentProcess))
		{
			m_rememberedProcess = m_currentProcess;
			m_rememberedProcessTime = m_currentProcessTime;
		}

		EndProcess(m_currentProcess);
		m_currentProcess = process;
		StartProcess(process);
		m_currentProcessTime = time;
		return true;
	}

	return false;
}

void Bot::StartProcess(const Process& process)
{
	switch (process)
	{
	case Process::Default:
	{
		DefaultStart();
		break;
	}
	case Process::Attack:
	{
		AttackStart();
		break;
	}
	case Process::Defuse:
	{
		DefuseStart();
		break;
	}
	case Process::Plant:
	{
		PlantStart();
		break;
	}
	case Process::Escape:
	{
		EscapeStart();
		break;
	}
	case Process::Pause:
	{
		PauseStart();
		break;
	}
	case Process::DestroyBreakable:
	{
		DestroyBreakableStart();
		break;
	}
	case Process::Pickup:
	{
		PickupStart();
		break;
	}
	case Process::Camp:
	{
		CampStart();
		break;
	}
	case Process::ThrowHE:
	{
		ThrowHEStart();
		break;
	}
	case Process::ThrowFB:
	{
		ThrowFBStart();
		break;
	}
	case Process::ThrowSM:
	{
		ThrowSMStart();
		break;
	}
	case Process::Blind:
	{
		BlindStart();
		break;
	}
	case Process::Jump:
	{
		JumpStart();
		break;
	}
	}
}

void Bot::EndProcess(const Process& process)
{
	switch (process)
	{
	case Process::Default:
	{
		DefaultEnd();
		break;
	}
	case Process::Attack:
	{
		AttackEnd();
		break;
	}
	case Process::Defuse:
	{
		DefuseEnd();
		break;
	}
	case Process::Plant:
	{
		PlantEnd();
		break;
	}
	case Process::Escape:
	{
		EscapeEnd();
		break;
	}
	case Process::Pause:
	{
		PauseEnd();
		break;
	}
	case Process::DestroyBreakable:
	{
		DestroyBreakableEnd();
		break;
	}
	case Process::Pickup:
	{
		PickupEnd();
		break;
	}
	case Process::Camp:
	{
		CampEnd();
		break;
	}
	case Process::ThrowHE:
	{
		ThrowHEEnd();
		break;
	}
	case Process::ThrowFB:
	{
		ThrowFBEnd();
		break;
	}
	case Process::ThrowSM:
	{
		ThrowSMEnd();
		break;
	}
	case Process::Blind:
	{
		BlindEnd();
		break;
	}
	case Process::Jump:
	{
		JumpEnd();
		break;
	}
	}
}

void Bot::UpdateProcess(void)
{
	static float time;
	time = engine->GetTime();

	switch (m_currentProcess)
	{
	case Process::Default:
	{
		DefaultUpdate();
		break;
	}
	case Process::Attack:
	{
		AttackUpdate();
		break;
	}
	case Process::Defuse:
	{
		DefuseUpdate();
		break;
	}
	case Process::Plant:
	{
		PlantUpdate();
		break;
	}
	case Process::Escape:
	{
		EscapeUpdate();
		break;
	}
	case Process::Pause:
	{
		PauseUpdate();
		break;
	}
	case Process::DestroyBreakable:
	{
		DestroyBreakableUpdate();
		break;
	}
	case Process::Pickup:
	{
		PickupUpdate();
		break;
	}
	case Process::Camp:
	{
		CampUpdate();
		break;
	}
	case Process::ThrowHE:
	{
		ThrowHEUpdate();
		break;
	}
	case Process::ThrowFB:
	{
		ThrowFBUpdate();
		break;
	}
	case Process::ThrowSM:
	{
		ThrowSMUpdate();
		break;
	}
	case Process::Blind:
	{
		BlindUpdate();
		break;
	}
	case Process::Jump:
	{
		JumpUpdate();
		break;
	}
	default:
		SetProcess(Process::Default, "unknown process", true, time + 99999999.0f);
	}

	if (m_currentProcess > Process::Default && m_currentProcessTime < time)
	{
		extern ConVar ebot_debug;
		if (ebot_debug.GetBool())
			ServerPrint("%s is cancelled %s process -> timed out.", GetEntityName(GetEntity()), GetProcessName(m_currentProcess));

		EndProcess(m_currentProcess);

		if (m_rememberedProcess != m_currentProcess && m_rememberedProcessTime > time && IsReadyForTheProcess(m_rememberedProcess))
		{
			StartProcess(m_rememberedProcess);
			m_currentProcess = m_rememberedProcess;
			m_currentProcessTime = m_rememberedProcessTime;
			m_rememberedProcessTime = 0.0f;
			m_rememberedProcess = Process::Default;
			return;
		}

		m_currentProcess = Process::Default;
		StartProcess(Process::Default);
	}
}

void Bot::FinishCurrentProcess(const char* debugNote)
{
	if (m_currentProcess > Process::Default)
	{
		extern ConVar ebot_debug;
		if (ebot_debug.GetBool())
			ServerPrint("%s is cancelled %s process -> %s", GetEntityName(GetEntity()), GetProcessName(m_currentProcess), debugNote);

		EndProcess(m_currentProcess);

		if (m_rememberedProcess != m_currentProcess && m_rememberedProcessTime > engine->GetTime() && IsReadyForTheProcess(m_rememberedProcess))
		{
			StartProcess(m_rememberedProcess);
			m_currentProcess = m_rememberedProcess;
			m_currentProcessTime = m_rememberedProcessTime;
			m_rememberedProcessTime = 0.0f;
			m_rememberedProcess = Process::Default;
		}

		StartProcess(Process::Default);
		m_currentProcess = Process::Default;
	}
}

bool Bot::IsReadyForTheProcess(const Process& process)
{
	switch (process)
	{
	case Process::Default:
		return DefaultReq();
	case Process::Attack:
		return AttackReq();
	case Process::Defuse:
		return DefuseReq();
	case Process::Plant:
		return PlantReq();
	case Process::Escape:
		return EscapeReq();
	case Process::Pause:
		return PauseReq();
	case Process::DestroyBreakable:
		return DestroyBreakableReq();
	case Process::Pickup:
		return PickupReq();
	case Process::Camp:
		return CampReq();
	case Process::ThrowHE:
		return ThrowHEReq();
	case Process::ThrowFB:
		return ThrowFBReq();
	case Process::ThrowSM:
		return ThrowSMReq();
	case Process::Blind:
		return BlindReq();
	case Process::Jump:
		return JumpReq();
	}

	return true;
}

char* Bot::GetProcessName(const Process& process)
{
	switch (process)
	{
	case Process::Default:
		return "DEFAULT";
	case Process::Attack:
		return "ATTACK";
	case Process::Plant:
		return "PLANT THE BOMB";
	case Process::Defuse:
		return "DEFUSE THE BOMB";
	case Process::Escape:
		return "ESCAPE FROM THE BOMB";
	case Process::Hide:
		return "HIDE FROM DANGER";
	case Process::Pause:
		return "PAUSE";
	case Process::DestroyBreakable:
		return "DESTROY BREAKABLE";
	case Process::Pickup:
		return "PICKUP ITEM";
	case Process::Camp:
		return "CAMP";
	case Process::ThrowHE:
		return "THROW HE GRENADE";
	case Process::ThrowFB:
		return "THROW FB GRENADE";
	case Process::ThrowSM:
		return "THROW SM GRENADE";
	case Process::Blind:
		return "BLINDED";
	case Process::Jump:
		return "JUMPING";
	default:
		return "UNKNOWN";
	}

	return "UNKNOWN";
}
