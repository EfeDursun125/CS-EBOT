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
// $Id: engine.cpp 35 2009-06-24 16:43:26Z jeefo $
//

#include "../include/core.h"

ConVar::ConVar(const char* name, const char* initval, const VarType type)
{
    engine->RegisterVariable(name, initval, type, this);
}

void Engine::RegisterVariable(const char* variable, const char* value, const VarType varType, ConVar* self)
{
    VarPair newVariable;
    newVariable.reg.name = const_cast<char*>(variable);
    newVariable.reg.string = const_cast<char*>(value);

    int engineFlags = FCVAR_EXTDLL;
    if (varType == VARTYPE_NORMAL)
        engineFlags |= FCVAR_SERVER;
    else if (varType == VARTYPE_READONLY)
        engineFlags |= FCVAR_SERVER | FCVAR_SPONLY | FCVAR_PRINTABLEONLY;
    else if (varType == VARTYPE_PASSWORD)
        engineFlags |= FCVAR_PROTECTED;

    newVariable.reg.flags = engineFlags;
    newVariable.self = self;
    m_regVars.Push(&newVariable);
}

void Engine::PushRegisteredConVarsToEngine(void)
{
    int16_t i;
    VarPair* ptr;
    for (i = 0; i < m_regVars.Size(); i++)
    {
        ptr = &m_regVars[i];
        if (!ptr)
            break;

        g_engfuncs.pfnCVarRegister(&ptr->reg);
        ptr->self->m_eptr = g_engfuncs.pfnCVarGetPointer(ptr->reg.name);
    }
}

void Engine::GetGameConVarsPointers(void)
{
    m_gameVars[GVAR_GRAVITY] = g_engfuncs.pfnCVarGetPointer("sv_gravity");
    m_gameVars[GVAR_DEVELOPER] = g_engfuncs.pfnCVarGetPointer("developer");
}

const Vector& Engine::GetGlobalVector(const GlobalVector id)
{
    if (!g_pGlobals)
        return nullvec;

    switch (id)
    {
    case GLOBALVECTOR_FORWARD:
        return g_pGlobals->v_forward;
    case GLOBALVECTOR_RIGHT:
        return g_pGlobals->v_right;
    case GLOBALVECTOR_UP:
        return g_pGlobals->v_up;
    }
    return nullvec;
}

void Engine::SetGlobalVector(const GlobalVector id, const Vector& newVector)
{
    if (!g_pGlobals)
        return;

    switch (id)
    {
    case GLOBALVECTOR_FORWARD:
    {
        g_pGlobals->v_forward = newVector;
        break;
    }
    case GLOBALVECTOR_RIGHT:
    {
        g_pGlobals->v_right = newVector;
        break;
    }
    case GLOBALVECTOR_UP:
    {
        g_pGlobals->v_up = newVector;
        break;
    }
    }
}

void Engine::BuildGlobalVectors(const Vector& on)
{
    if (g_pGlobals)
        on.BuildVectors(&g_pGlobals->v_forward, &g_pGlobals->v_right, &g_pGlobals->v_up);
}

int Engine::GetGravity(void)
{
    if (!m_gameVars[GVAR_GRAVITY])
        return 800;

    return static_cast<int>(m_gameVars[GVAR_GRAVITY]->value);
}

int Engine::GetDeveloperLevel(void)
{
    if (!m_gameVars[GVAR_DEVELOPER])
        return 0;

    return static_cast<int>(m_gameVars[GVAR_DEVELOPER]->value);
}

void Engine::PrintServer(const char* format, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
    cstrcat(buffer, "\n");
    g_engfuncs.pfnServerPrint(buffer);
}

int Engine::GetMaxClients(void)
{
    if (g_pGlobals)
        return g_pGlobals->maxClients;

    return 32;
}

float Engine::GetTime(void)
{
    if (g_pGlobals)
        return g_pGlobals->time;

    return 1.0f;
}

#pragma warning (disable : 4172)
const Entity& Engine::GetEntityByIndex(const int index)
{
    return g_engfuncs.pfnPEntityOfEntIndex(index);
}
#pragma warning (default : 4172)

const Client& Engine::GetClientByIndex(const int index)
{
    return m_clients[index];
}

void Engine::MaintainClients(void)
{
    for (const auto& client : g_clients)
    {
        if (client.index < 0)
            continue;

        if (FNullEnt(client.ent))
            continue;

        m_clients[client.index].Maintain(client.ent);
    }
}

void Engine::DrawLine(edict_t* client, const Vector& start, const Vector& end, const Color& color, const int width, const int noise, const int speed, const int life, const int lineType)
{
    if (!IsValidPlayer(client) || IsValidBot(client))
        return;

    MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, client);
    WRITE_BYTE(TE_BEAMPOINTS);
    WRITE_COORD(start.x);
    WRITE_COORD(start.y);
    WRITE_COORD(start.z);
    WRITE_COORD(end.x);
    WRITE_COORD(end.y);
    WRITE_COORD(end.z);
    WRITE_SHORT(lineType == LINE_ARROW ? g_modelIndexArrow : g_modelIndexLaser);
    WRITE_BYTE(0);
    WRITE_BYTE(10);
    WRITE_BYTE(life);
    WRITE_BYTE(width);
    WRITE_BYTE(noise);
    WRITE_BYTE(color.red);
    WRITE_BYTE(color.green);
    WRITE_BYTE(color.blue);
    WRITE_BYTE(color.alpha);
    WRITE_BYTE(speed);
    MESSAGE_END();
}

void Engine::DrawLineToAll(const Vector& start, const Vector& end, const Color& color, const int width, const int noise, const int speed, const int life, const int lineType)
{
    for (const Clients& client : g_clients)
    {
        if (IsValidBot(client.ent))
            continue;

        if (!IsValidPlayer(client.ent))
            continue;

        MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, client.ent);
        WRITE_BYTE(TE_BEAMPOINTS);
        WRITE_COORD(start.x);
        WRITE_COORD(start.y);
        WRITE_COORD(start.z);
        WRITE_COORD(end.x);
        WRITE_COORD(end.y);
        WRITE_COORD(end.z);
        WRITE_SHORT(lineType == LINE_ARROW ? g_modelIndexArrow : g_modelIndexLaser);
        WRITE_BYTE(0);
        WRITE_BYTE(10);
        WRITE_BYTE(life);
        WRITE_BYTE(width);
        WRITE_BYTE(noise);
        WRITE_BYTE(color.red);
        WRITE_BYTE(color.green);
        WRITE_BYTE(color.blue);
        WRITE_BYTE(color.alpha);
        WRITE_BYTE(speed);
        MESSAGE_END();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CLIENT
//////////////////////////////////////////////////////////////////////////
bool Client::IsInViewCone(const Vector& pos) const
{
    engine->BuildGlobalVectors(GetViewAngles());
    return ((pos - GetHeadOrigin()).Normalize() | g_pGlobals->v_forward) > ccosf(Math::DegreeToRadian((GetFOV() > 0.0f ? GetFOV() : 91.0f) * 0.51f));
}

bool Client::IsVisible(const Vector& pos) const
{
    TraceResult tr;
    TraceLine(GetHeadOrigin(), pos, TraceIgnore::Everything, m_ent, &tr);
    return tr.flFraction >= 1.0f;
}

bool Client::HasFlag(const int clientFlags)
{
    return (m_flags & clientFlags) == clientFlags;
}

Vector Client::GetOrigin(void) const
{
    return m_safeOrigin;
}

bool Client::IsAlive(void) const
{
    return !!(m_flags & CLIENT_ALIVE | CLIENT_VALID);
}

void Client::Maintain(const Entity& ent)
{
    if (ent.IsPlayer())
    {
        m_ent = ent;
        m_safeOrigin = ent.GetOrigin();
        m_flags |= ent.IsAlive() ? CLIENT_VALID | CLIENT_ALIVE : CLIENT_VALID;
    }
    else
    {
        m_safeOrigin = nullvec;
        m_flags = ~(CLIENT_VALID | CLIENT_ALIVE);
    }
}