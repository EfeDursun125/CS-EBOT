#include <core.h>

ConVar::ConVar(const char* name, const char* initval, VarType type)
{
    engine->RegisterVariable(name, initval, type, this);
}

void Engine::RegisterVariable(const char* variable, const char* value, VarType varType, ConVar* self)
{
    VarPair newVariable;

    newVariable.reg.name = const_cast <char*> (variable);
    newVariable.reg.string = const_cast <char*> (value);

    int engineFlags = FCVAR_EXTDLL;

    if (varType == VARTYPE_NORMAL)
        engineFlags |= FCVAR_SERVER;
    else if (varType == VARTYPE_READONLY)
        engineFlags |= FCVAR_SERVER | FCVAR_SPONLY | FCVAR_PRINTABLEONLY;
    else if (varType == VARTYPE_PASSWORD)
        engineFlags |= FCVAR_PROTECTED;

    newVariable.reg.flags = engineFlags;
    newVariable.self = self;

    cmemcpy(&m_regVars[m_regCount], &newVariable, sizeof(VarPair));
    m_regCount++;
}

void Engine::PushRegisteredConVarsToEngine(void)
{
    for (int i = 0; i < m_regCount; i++)
    {
        VarPair* ptr = &m_regVars[i];
        if (ptr == nullptr)
            break;

        g_engfuncs.pfnCVarRegister(&ptr->reg);
        ptr->self->m_eptr = g_engfuncs.pfnCVarGetPointer(ptr->reg.name);
    }
}

void Engine::GetGameConVarsPointers(void)
{
    m_gameVars[GVAR_C4TIMER] = g_engfuncs.pfnCVarGetPointer("mp_c4timer");
    m_gameVars[GVAR_BUYTIME] = g_engfuncs.pfnCVarGetPointer("mp_buytime");
    m_gameVars[GVAR_FRIENDLYFIRE] = g_engfuncs.pfnCVarGetPointer("mp_friendlyfire");
    m_gameVars[GVAR_ROUNDTIME] = g_engfuncs.pfnCVarGetPointer("mp_roundtime");
    m_gameVars[GVAR_FREEZETIME] = g_engfuncs.pfnCVarGetPointer("mp_freezetime");
    m_gameVars[GVAR_FOOTSTEPS] = g_engfuncs.pfnCVarGetPointer("mp_footsteps");
    m_gameVars[GVAR_GRAVITY] = g_engfuncs.pfnCVarGetPointer("sv_gravity");
    m_gameVars[GVAR_DEVELOPER] = g_engfuncs.pfnCVarGetPointer("developer");

    // if buytime is null, just set it to round time
    if (m_gameVars[GVAR_BUYTIME] == nullptr)
        m_gameVars[GVAR_BUYTIME] = m_gameVars[3];
}

const Vector& Engine::GetGlobalVector(GlobalVector id)
{
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

void Engine::SetGlobalVector(GlobalVector id, const Vector& newVector)
{
    switch (id)
    {
    case GLOBALVECTOR_FORWARD:
        g_pGlobals->v_forward = newVector;
        break;

    case GLOBALVECTOR_RIGHT:
        g_pGlobals->v_right = newVector;
        break;

    case GLOBALVECTOR_UP:
        g_pGlobals->v_up = newVector;
        break;
    }
}

void Engine::BuildGlobalVectors(const Vector& on)
{
    on.BuildVectors(&g_pGlobals->v_forward, &g_pGlobals->v_right, &g_pGlobals->v_up);
}

bool Engine::IsFootstepsOn(void)
{
    if (m_gameVars[GVAR_FOOTSTEPS] == nullptr)
        return true;

    return m_gameVars[GVAR_FOOTSTEPS]->value > 0;
}

float Engine::GetC4TimerTime(void)
{
    if (m_gameVars[GVAR_C4TIMER] == nullptr)
        return 35.0f;

    return m_gameVars[GVAR_C4TIMER]->value;
}

float Engine::GetBuyTime(void)
{
    if (m_gameVars[GVAR_BUYTIME] == nullptr)
        return 15.0f;

    return m_gameVars[GVAR_BUYTIME]->value;
}

float Engine::GetRoundTime(void)
{
    // we have no idea
    if (m_gameVars[GVAR_ROUNDTIME] == nullptr)
        return CRandomFloat(120.0f, 300.0f);

    return m_gameVars[GVAR_ROUNDTIME]->value;
}

float Engine::GetFreezeTime(void)
{
    if (m_gameVars[GVAR_FREEZETIME] == nullptr)
        return 1.0f;

    return m_gameVars[GVAR_FREEZETIME]->value;
}

int Engine::GetGravity(void)
{
    if (m_gameVars[GVAR_GRAVITY] == nullptr)
        return 800;

    return static_cast<int>(m_gameVars[GVAR_GRAVITY]->value);
}

int Engine::GetDeveloperLevel(void)
{
    if (m_gameVars[GVAR_DEVELOPER] == nullptr)
        return 0;

    return static_cast<int>(m_gameVars[GVAR_DEVELOPER]->value);
}

bool Engine::IsFriendlyFireOn(void)
{
    if (m_gameVars[GVAR_FRIENDLYFIRE] == nullptr)
        return false;

    return m_gameVars[GVAR_FRIENDLYFIRE]->value > 0;
}

void Engine::PrintServer(const char* format, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);

    cstrcat(buffer, "\n");

    g_engfuncs.pfnServerPrint(buffer);
}

int Engine::GetMaxClients(void)
{
    return g_pGlobals->maxClients;
}

float Engine::GetTime(void)
{
    return g_pGlobals->time;
}

#pragma warning (disable : 4172)
const Entity& Engine::GetEntityByIndex(int index)
{
    return g_engfuncs.pfnPEntityOfEntIndex(index);
}
#pragma warning (default : 4172)

const Client& Engine::GetClientByIndex(int index)
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

void Engine::DrawLine(edict_t* client, const Vector& start, const Vector& end, const Color& color, int width, int noise, int speed, int life, int lineType)
{
    if (!IsValidPlayer(client))
        return;

    MessageSender(MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, g_hostEntity)
        .WriteByte(TE_BEAMPOINTS)
        .WriteCoord(start.x)
        .WriteCoord(start.y)
        .WriteCoord(start.z)
        .WriteCoord(end.x)
        .WriteCoord(end.y)
        .WriteCoord(end.z)
        .WriteShort(lineType == LINE_SIMPLE ? g_modelIndexLaser : g_modelIndexArrow)
        .WriteByte(0)
        .WriteByte(10)
        .WriteByte(life)
        .WriteByte(width)
        .WriteByte(noise)
        .WriteByte(color.red)
        .WriteByte(color.green)
        .WriteByte(color.blue)
        .WriteByte(color.alpha)
        .WriteByte(speed);
}

void Engine::IssueBotCommand(edict_t* ent, const char* fmt, ...)
{
    // the purpose of this function is to provide fakeclients (bots) with the same client
    // command-scripting advantages (putting multiple commands in one line between semicolons)
    // as real players. It is an improved version of botman's FakeClientCommand, in which you
    // supply directly the whole string as if you were typing it in the bot's "console". It
    // is supposed to work exactly like the pfnClientCommand (server-sided client command).

    if (FNullEnt(ent))
        return;

    va_list ap;
    static char string[256];

    va_start(ap, fmt);
    vsnprintf(string, 256, fmt, ap);
    va_end(ap);

    if (IsNullString(string))
        return;

    m_arguments[0] = 0x0;
    m_argumentCount = 0;

    m_isBotCommand = true;

    int i, pos = 0;
    const int length = cstrlen(string);

    while (pos < length)
    {
        const int start = pos;
        int stop = pos;

        while (pos < length && string[pos] != ';')
            pos++;

        if (string[pos - 1] == '\n')
            stop = pos - 2;
        else
            stop = pos - 1;

        for (i = start; i <= stop; i++)
            m_arguments[i - start] = string[i];

        m_arguments[i - start] = 0;
        pos++;

        int index = 0;
        m_argumentCount = 0;

        while (index < i - start)
        {
            while (index < i - start && m_arguments[index] == ' ')
                index++;

            if (m_arguments[index] == '"')
            {
                index++;

                while (index < i - start && m_arguments[index] != '"')
                    index++;
                index++;
            }
            else
            {
                while (index < i - start && m_arguments[index] != ' ')
                    index++;
            }

            m_argumentCount++;
        }

        MDLL_ClientCommand(ent);
    }

    m_isBotCommand = false;

    m_arguments[0] = 0x0;
    m_argumentCount = 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// CLIENT
//////////////////////////////////////////////////////////////////////////
float Client::GetShootingConeDeviation(const Vector& pos) const
{
    engine->BuildGlobalVectors(GetViewAngles());
    return g_pGlobals->v_forward | (pos - GetHeadOrigin()).Normalize();
}

bool Client::IsInViewCone(const Vector& pos) const
{
    engine->BuildGlobalVectors(GetViewAngles());
    return ((pos - GetHeadOrigin()).Normalize() | g_pGlobals->v_forward) >= cosf(Math::DegreeToRadian((GetFOV() > 0.0f ? GetFOV() : 90.0f) * 0.5f));
}

bool Client::IsVisible(const Vector& pos) const
{
    Tracer trace(GetHeadOrigin(), pos, NO_BOTH, m_ent);
    return !(trace.Fire() != 1.0);
}

bool Client::HasFlag(int clientFlags)
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