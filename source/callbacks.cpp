#include <core.h>

#pragma warning (disable : 4100)

void BotCallbacks::OnEngineFrame(void)
{

}

void BotCallbacks::OnGameInitialize(void)
{

}

void BotCallbacks::OnGameShutdown(void)
{

}

void BotCallbacks::OnRoundStateChanged(bool started, Team winner)
{

}

void BotCallbacks::OnChangeLevel(void)
{

}

void BotCallbacks::OnServerActivate(void)
{

}

void BotCallbacks::OnServerDeactivate(void)
{

}

void BotCallbacks::OnEmitSound(const Entity& client, const String& soundName, float volume, float attenuation)
{

}

void BotCallbacks::OnClientConnect(const Client& client, const String& name, const String& address)
{

}

void BotCallbacks::OnClientDisconnect(const Client& client)
{
	//   logger->Warning ("Client disconnected: %s", client.GetName ());
}

void BotCallbacks::OnClientEntersServer(const Entity& client)
{

}

void BotCallbacks::OnClientProgressBarChanged(const Entity& client, bool visible)
{

}

void BotCallbacks::OnClientTeamUpdated(const Entity& client, Team newTeam)
{

}

void BotCallbacks::OnClientBlinded(const Entity& client, const Color& color)
{

}

void BotCallbacks::OnClientDied(const Entity& killer, const Entity& victim)
{

}

void BotCallbacks::OnClientStatusIconChanged(const Entity& client, ClientStatusIcon icons)
{

}

void BotCallbacks::OnEntitySpawned(const Entity& ent)
{

}

void BotCallbacks::OnRunCommand(const Entity& client, const String& command, const Array <const String&>& args)
{

}

void BotCallbacks::OnBombPlanted(const Entity& bombEntity, const Vector& bombPos)
{

}

void BotCallbacks::OnWeaponSecondaryActionChanged(bool silencer, bool enabled)
{

}

#pragma warning (default : 4100)