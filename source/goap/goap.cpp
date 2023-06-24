#include <core.h>
extern ConVar ebot_debug;

Process Bot::GetProcess(void)
{
	return m_currentProcess;
}

bool Bot::SetProcess(const Process process, const char* debugnote, const bool rememberProcess, const float time)
{
	if (m_currentProcess != process && IsReadyForTheProcess(process))
	{
		if (ebot_debug.GetInt() > 0)
			ServerPrint("%s has got a new process from %s to %s | process started -> %s", GetEntityName(GetEntity()), GetProcessName(m_currentProcess), GetProcessName(process), debugnote);

		if (rememberProcess && m_currentProcess > Process::Default)
		{
			m_rememberedProcess = m_currentProcess;
			m_rememberedProcessTime = m_currentProcessTime;
		}

		EndProcess(m_currentProcess);
		m_currentProcess = process;
		StartProcess(process);
		m_currentProcessTime = AddTime(time);
		return true;
	}

	return false;
}

void Bot::StartProcess(const Process process)
{
	switch (process)
	{
	case Process::Default:
		DefaultStart();
		break;
	case Process::Attack:
		AttackStart();
		break;
	case Process::Defuse:
		DefuseStart();
		break;
	case Process::Plant:
		PlantStart();
		break;
	case Process::Escape:
		EscapeStart();
		break;
	case Process::Pause:
		PauseStart();
		break;
	case Process::DestroyBreakable:
		DestroyBreakableStart();
		break;
	case Process::Pickup:
		PickupStart();
		break;
	case Process::Camp:
		CampStart();
		break;
	}
}

void Bot::EndProcess(const Process process)
{
	switch (process)
	{
	case Process::Default:
		DefaultEnd();
		break;
	case Process::Attack:
		AttackEnd();
		break;
	case Process::Defuse:
		DefuseEnd();
		break;
	case Process::Plant:
		PlantEnd();
		break;
	case Process::Escape:
		EscapeEnd();
		break;
	case Process::Pause:
		PauseEnd();
	case Process::DestroyBreakable:
		DestroyBreakableEnd();
		break;
	case Process::Pickup:
		PickupEnd();
		break;
	case Process::Camp:
		CampEnd();
		break;
	}
}

void Bot::UpdateProcess(void)
{
	switch (m_currentProcess)
	{
	case Process::Default:
		DefaultUpdate();
		break;
	case Process::Attack:
		AttackUpdate();
		break;
	case Process::Defuse:
		DefuseUpdate();
		break;
	case Process::Plant:
		PlantUpdate();
		break;
	case Process::Escape:
		EscapeUpdate();
		break;
	case Process::Pause:
		PauseUpdate();
		break;
	case Process::DestroyBreakable:
		DestroyBreakableUpdate();
		break;
	case Process::Pickup:
		PickupUpdate();
		break;
	case Process::Camp:
		CampUpdate();
		break;
	default:
		SetProcess(Process::Default, "unknown process");
		break;
	}

	if (m_currentProcess > Process::Default && m_currentProcessTime < engine->GetTime())
	{
		if (ebot_debug.GetInt() > 0)
			ServerPrint("%s is cancelled %s process -> timed out.", GetEntityName(GetEntity()), GetProcessName(m_currentProcess));

		EndProcess(m_currentProcess);

		if (m_rememberedProcess != m_currentProcess && m_rememberedProcessTime > engine->GetTime() && IsReadyForTheProcess(m_rememberedProcess))
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
		if (ebot_debug.GetInt() > 0)
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

bool Bot::IsReadyForTheProcess(const Process process)
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
	}

	return true;
}

char* Bot::GetProcessName(const Process process)
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
	}

	return "UNKNOWN";
}