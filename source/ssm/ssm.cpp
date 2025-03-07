#include "../../include/core.h"
extern ConVar ebot_debug;

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
		if (ebot_debug.GetBool())
			ServerPrint("%s has got a new process from %s to %s | process started -> %s", GetEntityName(m_myself), GetProcessName(m_currentProcess), GetProcessName(process), debugnote);

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
		case Process::UseButton:
		{
			UseButtonStart();
			break;
		}
		case Process::Override:
			break;
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
		case Process::UseButton:
		{
			UseButtonEnd();
			break;
		}
		case Process::Override:
		{
			OverrideEnd();
			break;
		}
	}
}

void Bot::UpdateProcess(void)
{
	static float time2;
	time2 = engine->GetTime();

	switch (m_currentProcess)
	{
		case Process::Default:
		{
			DefaultUpdate();
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
		case Process::UseButton:
		{
			UseButtonUpdate();
			break;
		}
		case Process::Override:
		{
			OverrideUpdate();
			break;
		}
		default:
			SetProcess(Process::Default, "unknown process", true, time2 + 999999.0f);
	}

	if (m_currentProcess > Process::Default && m_currentProcessTime < time2)
	{
		if (ebot_debug.GetBool())
			ServerPrint("%s is cancelled %s process -> timed out.", GetEntityName(m_myself), GetProcessName(m_currentProcess));

		EndProcess(m_currentProcess);

		if (m_rememberedProcess != m_currentProcess && m_rememberedProcessTime > time2 && IsReadyForTheProcess(m_rememberedProcess))
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
		if (ebot_debug.GetBool())
			ServerPrint("%s is cancelled %s process -> %s", GetEntityName(m_myself), GetProcessName(m_currentProcess), debugNote);

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
		case Process::Pause:
			return PauseReq();
		case Process::DestroyBreakable:
			return DestroyBreakableReq();
		case Process::ThrowHE:
			return ThrowHEReq();
		case Process::ThrowFB:
			return ThrowFBReq();
		case Process::ThrowSM:
			return ThrowSMReq();
		case Process::Blind:
			return BlindReq();
		case Process::UseButton:
			return UseButtonReq();
		case Process::Override:
			return true;
	}

	return true;
}

char* Bot::GetProcessName(const Process& process)
{
	static char pName[128];
	switch (process)
	{
		case Process::Default:
			FormatBuffer(pName, "DEFAULT");
		case Process::Pause:
			FormatBuffer(pName, "PAUSE");
		case Process::DestroyBreakable:
			FormatBuffer(pName, "DESTROY BREAKABLE");
		case Process::ThrowHE:
			FormatBuffer(pName, "THROW HE GRENADE");
		case Process::ThrowFB:
			FormatBuffer(pName, "THROW FB GRENADE");
		case Process::ThrowSM:
			FormatBuffer(pName, "THROW SM GRENADE");
		case Process::Blind:
			FormatBuffer(pName, "BLIND");
		case Process::UseButton:
			FormatBuffer(pName, "USE BUTTON");
		case Process::Override:
			FormatBuffer(pName, "OVERRIDE ID: %i", m_overrideID);
		default:
			FormatBuffer(pName, "UNKNOWN");
	}

	return pName;
}
