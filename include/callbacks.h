//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id: globals.h 15 2009-06-09 19:08:41Z jeefo $
//

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
