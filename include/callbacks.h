#ifndef CALLBACKS_INCLUDED
#define CALLBACKS_INCLUDED

//
// Enum: ClientStatusIcon
//
// CSTATUS_BOMB - Client is in bomb zone.
// CSTATUS_DEFUSER - Client has a defuser.
// CSTATUS_VIPSAFE - Client is in vip safety zone.
// CSTATUS_BUY - Client is in buy zone.
//
enum ClientStatusIcon
{
    CSTATUS_BOMB = (1 << 0),
    CSTATUS_DEFUSER = (1 << 1),
    CSTATUS_VIPSAFE = (1 << 2),
    CSTATUS_BUY = (1 << 3)
};

//
// Class: BotCallbacks
//
class BotCallbacks : public Singleton <BotCallbacks>
{
    //
    // Group: Public Operators
    //
public:
    BotCallbacks* operator -> (void)
    {
        return this;
    }

    //
    // Group: Public Callbacks
    //
public:
    void OnEngineFrame(void);

    void OnGameInitialize(void);

    void OnGameShutdown(void);

    void OnRoundStateChanged(bool started, Team winner);

    void OnChangeLevel(void);

    void OnServerActivate(void);

    void OnServerDeactivate(void);

    //
    // Function: OnEmitSound
    // 
    // Called when the engine emits a sound. Used for creating bot-"hearing" system.
    //
    // Parameters:
    //   client - Client who is issuing the sound.
    //   soundName - Name of the sound that was issued.
    //   volume -  Volume of the sound.
    //   attenuation - Attenuation of the sound.
    //
    // See Also:
    //   <Entity>
    //
    void OnEmitSound(const Entity& client, const String& soundName, float volume, float attenuation);

    void OnClientConnect(const Client& client, const String& name, const String& address);

    void OnClientDisconnect(const Client& client);

    void OnClientEntersServer(const Entity& client);

    void OnClientProgressBarChanged(const Entity& client, bool visible);

    void OnClientTeamUpdated(const Entity& client, Team newTeam);

    void OnClientBlinded(const Entity& client, const Color& color);

    void OnClientDied(const Entity& killer, const Entity& victim);

    void OnClientStatusIconChanged(const Entity& client, ClientStatusIcon icons);

    void OnEntitySpawned(const Entity& ent);

    void OnRunCommand(const Entity& client, const String& command, const Array <const String&>& args);

    void OnBombPlanted(const Entity& bombEntity, const Vector& bombPos);

    void OnWeaponSecondaryActionChanged(bool silencer, bool enabled);
};

#define callbacks BotCallbacks::GetReference ()


#endif
