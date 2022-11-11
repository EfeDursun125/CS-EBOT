#ifndef __HALFLIFE1_INTERFACE__
#define __HALFLIFE1_INTERFACE__

#ifdef _WIN32
#pragma once
#endif

// detects the build platform
#if defined (__linux__) || defined (__debian__) || defined (__linux)
#define __linux__ 1

#endif

#if defined __linux__
#define EXTRAOFFSET					5
#else
#define EXTRAOFFSET					0
#endif

#if !defined PLATFORM_LINUX64
#define OFFSET_TEAM					114 + EXTRAOFFSET
#else
#define OFFSET_TEAM					139 + EXTRAOFFSET
#endif

// for when we care about how many bits we use
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;

#ifdef _WIN32
#ifdef _MSC_VER
typedef signed __int64 int64;
#endif
#elif defined __linux__
typedef long long int64;
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

#ifdef _WIN32
#ifdef _MSC_VER
typedef unsigned __int64 uint64;
#endif
#elif defined __linux__
typedef unsigned long long uint64;
#endif

typedef float float32;
typedef double float64;

// for when we don't care about how many bits we use
typedef unsigned int uint;

// This can be used to ensure the size of pointers to members when declaring
// a pointer type for a class that has only been forward declared
#ifdef _MSC_VER
#define SINGLE_INHERITANCE __single_inheritance
#define MULTIPLE_INHERITANCE __multiple_inheritance
#else
#define SINGLE_INHERITANCE
#define MULTIPLE_INHERITANCE
#endif

// Maximum and minimum representable values
#define  INT8_MAX    SCHAR_MAX
#define  INT16_MAX   SHRT_MAX
#define  INT32_MAX   LONG_MAX
#define  INT64_MAX  (((int64)~0) >> 1)

#define  INT8_MIN    SCHAR_MIN
#define  INT16_MIN   SHRT_MIN
#define  INT32_MIN   LONG_MIN
#define  INT64_MIN  (((int64)1) << 63)

#define  UINT8_MAX  ((uint8)~0)
#define  UINT16_MAX ((uint16)~0)
#define  UINT32_MAX ((uint32)~0)
#define  UINT64_MAX ((uint64)~0)

#define  UINT8_MIN   0
#define  UINT16_MIN  0
#define  UINT32_MIN  0
#define  UINT64_MIN  0

#ifndef  UINT_MIN
#define  UINT_MIN    UINT32_MIN
#endif

#define  FLOAT32_MAX FLT_MAX
#define  FLOAT64_MAX DBL_MAX

#define  FLOAT32_MIN FLT_MIN
#define  FLOAT64_MIN DBL_MIN

// portability / compiler settings
#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__   1
#endif

#elif __linux__
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef void* HINSTANCE;

#define _MAX_PATH PATH_MAX
#endif // defined(_WIN32) && !defined(WINDED)

// Defines MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH  260
#endif

// Used to step into the debugger
#define  DebuggerBreak()  __asm { int 3 }

// C functions for external declarations that call the appropriate C++ methods
#ifndef EXPORT
#ifdef _WIN32
#define EXPORT   __declspec( dllexport )
#else
#define EXPORT                  /* */
#endif
#endif

#if defined __i386__ && !defined __linux__
#define id386   1
#else
#define id386   0
#endif // __i386__

#ifdef _WIN32
// Used for dll exporting and importing
#define  DLL_EXPORT   extern "C" __declspec( dllexport )
#define  DLL_IMPORT   extern "C" __declspec( dllimport )

// Can't use extern "C" when DLL exporting a class
#define  DLL_CLASS_EXPORT   __declspec( dllexport )
#define  DLL_CLASS_IMPORT   __declspec( dllimport )

// Can't use extern "C" when DLL exporting a global
#define  DLL_GLOBAL_EXPORT   extern __declspec( dllexport )
#define  DLL_GLOBAL_IMPORT   extern __declspec( dllimport )
#elif defined __linux__

// Used for dll exporting and importing
#define  DLL_EXPORT   extern "C"
#define  DLL_IMPORT   extern "C"

// Can't use extern "C" when DLL exporting a class
#define  DLL_CLASS_EXPORT
#define  DLL_CLASS_IMPORT

// Can't use extern "C" when DLL exporting a global
#define  DLL_GLOBAL_EXPORT   extern
#define  DLL_GLOBAL_IMPORT   extern

#else
#error "Unsupported Platform."
#endif

#ifndef _WIN32
#define FAKEFUNC (void *) 0
#else
#define FAKEFUNC __noop
#endif

// Used for standard calling conventions
#ifdef _WIN32
#define  STDCALL            __stdcall
#define  FASTCALL            __fastcall
#ifndef FORCEINLINE
#define  FORCEINLINE         __forceinline
#endif
#else
#define  STDCALL
#define  FASTCALL
#define  FORCEINLINE         inline
#endif

// Force a function call site -not- to inlined. (useful for profiling)
#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))

// Pass hints to the compiler to prevent it from generating unnessecary / stupid code
// in certain situations.  Several compilers other than MSVC also have an equivilent 
// construct.
//
// Essentially the 'Hint' is that the condition specified is assumed to be true at 
// that point in the compilation.  If '0' is passed, then the compiler assumes that
// any subsequent code in the same 'basic block' is unreachable, and thus usually 
// removed.
#ifdef _MSC_VER
#define HINT(THE_HINT)   __assume((THE_HINT))
#else
#define HINT(THE_HINT)   0
#endif

// Marks the codepath from here until the next branch entry point as unreachable,
// and asserts if any attempt is made to execute it.
#define UNREACHABLE() { ASSERT(0); HINT(0); }

// In cases where no default is present or appropriate, this causes MSVC to generate 
// as little code as possible, and throw an assertion in debug.
#define NO_DEFAULT default: UNREACHABLE();

#ifdef _WIN32
// Alloca defined for this platform
#define  stackalloc( _size ) _alloca( _size )
#define  stackfree( _p )   0
#elif __linux__
// Alloca defined for this platform
#define  stackalloc( _size ) alloca( _size )
#define  stackfree( _p )   0
#endif

#include <math.h>

// 2DVector - used for many pathfinding and many other 
// operations that are treated as planar rather than 3d.
class Vector2D
{
public:
    inline Vector2D(void)
    {
    }
    inline Vector2D(float X, float Y)
    {
        x = X;
        y = Y;
    }
    inline Vector2D operator+ (const Vector2D& v) const
    {
        return Vector2D(x + v.x, y + v.y);
    }
    inline Vector2D operator- (const Vector2D& v) const
    {
        return Vector2D(x - v.x, y - v.y);
    }
    inline Vector2D operator* (float fl) const
    {
        return Vector2D(x * fl, y * fl);
    }
    inline Vector2D operator/ (float fl) const
    {
        return Vector2D(x / fl, y / fl);
    }
    inline int operator== (const Vector2D& v) const
    {
        return x == v.x && y == v.y;
    }

    inline float Length(void) const
    {
#ifdef __SSE2__
        float number = _mm_cvtss_f32(_mm_add_ss(_mm_mul_ss(_mm_load_ss(&x), _mm_load_ss(&x)), _mm_mul_ss(_mm_load_ss(&y), _mm_load_ss(&y))));
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&number)));
#else
        float number = x * x + y * y;
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;
        i = 0x5f3759df - (i >> 1);
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));
        return y * number;
#endif
    }

    inline Vector2D Normalize(void) const
    {
        Vector2D vec2;

        float flLen = Length();
        if (flLen == 0)
            return Vector2D(0, 0);
        else
        {
            flLen = 1 / flLen;
#ifdef __SSE2__
            return Vector2D(_mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&x), _mm_load_ss(&flLen))), _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&y), _mm_load_ss(&flLen))));
#else
            return Vector2D(x * flLen, y * flLen);
#endif
        }
    }

    float x, y;
};

inline float DotProduct(const Vector2D& a, const Vector2D& b)
{
    return (a.x * b.x + a.y * b.y);
}

inline Vector2D operator* (float fl, const Vector2D& v)
{
    return v * fl;
}
#if 0
// 3D Vector
class Vector                    // same data-layout as engine's vec3_t,
{                               // which is a vec_t[3]
public:
    // Construction/destruction
    inline Vector(void)
    {
    }
    inline Vector(float X, float Y, float Z)
    {
        x = X;
        y = Y;
        z = Z;
    }
    inline Vector(const Vector& v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
    }
    inline Vector(float rgfl[3])
    {
        x = rgfl[0];
        y = rgfl[1];
        z = rgfl[2];
    }

    // Operators
    inline Vector operator- (void) const
    {
        return Vector(-x, -y, -z);
    }
    inline int operator== (const Vector& v) const
    {
        return x == v.x && y == v.y && z == v.z;
    }
    inline int operator!= (const Vector& v) const
    {
        return !(*this == v);
    }
    inline Vector operator+ (const Vector& v) const
    {
        return Vector(x + v.x, y + v.y, z + v.z);
    }
    inline Vector operator- (const Vector& v) const
    {
        return Vector(x - v.x, y - v.y, z - v.z);
    }
    inline Vector operator* (float fl) const
    {
        return Vector(x * fl, y * fl, z * fl);
    }
    inline Vector operator/ (float fl) const
    {
        return Vector(x / fl, y / fl, z / fl);
    }

    // Methods
    inline void CopyToArray(float* rgfl) const
    {
        rgfl[0] = x, rgfl[1] = y, rgfl[2] = z;
    }
    inline float Length(void) const
    {
        float number = x * x + y * y + z * z;
#ifdef __SSE2__
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&number)));
#else
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;
        i = 0x5f3759df - (i >> 1);
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));
        return y * number;
#endif
    }
    operator float* ()
    {
        return &x;
    }                            // Vectors will now automatically convert to float * when needed
    operator   const float* () const
    {
        return &x;
    }                            // Vectors will now automatically convert to float *when needed


    inline Vector Normalize(void) const
    {
        float flLen = Length();

        if (flLen == 0)
            return Vector(0, 0, 1);       // ????

        flLen = 1 / flLen;
#ifdef __SSE2__
        return Vector(_mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&x), _mm_load_ss(&flLen))), _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&y), _mm_load_ss(&flLen))), _mm_cvtss_f32(_mm_mul_ss(_mm_load_ss(&z), _mm_load_ss(&flLen))));
#else
        return Vector(x * flLen, y * flLen, z * flLen);
#endif
    }

    inline Vector IgnoreZComponent(void) const
    {
        return Vector(x, y, 0);
    }

    inline Vector2D Make2D(void) const
    {
        Vector2D Vec2;

        Vec2.x = x;
        Vec2.y = y;

        return Vec2;
    }
    inline float Length2D(void) const
    {
#ifdef __SSE2__
        float number = _mm_cvtss_f32(_mm_add_ss(_mm_mul_ss(_mm_load_ss(&x), _mm_load_ss(&x)), _mm_mul_ss(_mm_load_ss(&y), _mm_load_ss(&y))));
        return _mm_cvtss_f32(_mm_sqrt_ss(_mm_load_ss(&number)));
#else
        float number = x * x + y * y;
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;
        i = 0x5f3759df - (i >> 1);
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));
        return y * number;
#endif
    }

    // Members
    float x, y, z;
};
inline Vector operator* (float fl, const Vector& v)
{
    return v * fl;
}

inline float DotProduct(const Vector& a, const Vector& b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline Vector CrossProduct(const Vector& a, const Vector& b)
{
    return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
#else
#include "runtime.h"
#endif
//
// Constants shared by the engine and dlls
// This header file included by engine files and DLL files.
// Most came from server.h

// edict->flags
#define FL_FLY               (1 << 0)   // Changes the SV_Movestep() behavior to not need to be on ground
#define FL_SWIM              (1 << 1)   // Changes the SV_Movestep() behavior to not need to be on ground (but stay in water)
#define FL_CONVEYOR          (1 << 2)
#define FL_CLIENT            (1 << 3)
#define FL_INWATER           (1 << 4)
#define FL_MONSTER           (1 << 5)
#define FL_GODMODE           (1 << 6)
#define FL_NOTARGET          (1 << 7)
#define FL_SKIPLOCALHOST     (1 << 8)   // Don't send entity to local host, it's predicting this entity itself
#define FL_ONGROUND          (1 << 9)   // At rest / on the ground
#define FL_PARTIALGROUND     (1 << 10)  // not all corners are valid
#define FL_WATERJUMP         (1 << 11)  // player jumping out of water
#define FL_FROZEN            (1 << 12)  // Player is frozen for 3rd person camera
#define FL_FAKECLIENT        (1 << 13)  // JAC: fake client, simulated server side; don't send network messages to them
#define FL_DUCKING           (1 << 14)  // Player flag -- Player is fully crouched
#define FL_FLOAT             (1 << 15)  // Apply floating force to this entity when in water
#define FL_GRAPHED           (1 << 16)  // worldgraph has this ent listed as something that blocks a connection

// UNDONE: Do we need these?
#define FL_IMMUNE_WATER      (1 << 17)
#define FL_IMMUNE_SLIME      (1 << 18)
#define FL_IMMUNE_LAVA       (1 << 19)

#define FL_PROXY             (1 << 20)  // This is a spectator proxy
#define FL_ALWAYSTHINK       (1 << 21)  // Brush model flag -- call think every frame regardless of nextthink - ltime (for constantly changing velocity/path)
#define FL_BASEVELOCITY      (1 << 22)  // Base velocity has been applied this frame (used to convert base velocity into momentum)
#define FL_MONSTERCLIP       (1 << 23)  // Only collide in with monsters who have FL_MONSTERCLIP set
#define FL_ONTRAIN           (1 << 24)  // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
#define FL_WORLDBRUSH        (1 << 25)  // Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something)
#define FL_SPECTATOR         (1 << 26)  // This client is a spectator, don't run touch functions, etc.
#define FL_CUSTOMENTITY      (1 << 29)  // This is a custom entity
#define FL_KILLME            (1 << 30)  // This entity is marked for death -- This allows the engine to kill ents at the appropriate time
#define FL_DORMANT           (1 << 31)  // Entity is dormant, no updates to client

// Goes into globalvars_t.trace_flags
#define FTRACE_SIMPLEBOX     (1 << 0)   // Traceline with a simple box

// walkmove modes
#define WALKMOVE_NORMAL       0 // normal walkmove
#define WALKMOVE_WORLDONLY    1 // doesn't hit ANY entities, no matter what the solid type
#define WALKMOVE_CHECKONLY    2 // move, but don't touch triggers

// edict->movetype values
#define MOVETYPE_NONE           0       // never moves
#define MOVETYPE_WALK           3       // Player only - moving on the ground
#define MOVETYPE_STEP           4       // gravity, special edge handling -- monsters use this
#define MOVETYPE_FLY            5       // No gravity, but still collides with stuff
#define MOVETYPE_TOSS           6       // gravity/collisions
#define MOVETYPE_PUSH           7       // no clip to world, push and crush
#define MOVETYPE_NOCLIP         8       // No gravity, no collisions, still do velocity/avelocity
#define MOVETYPE_FLYMISSILE     9       // extra size to monsters
#define MOVETYPE_BOUNCE         10      // Just like Toss, but reflect velocity when contacting surfaces
#define MOVETYPE_BOUNCEMISSILE  11      // bounce w/o gravity
#define MOVETYPE_FOLLOW         12      // track movement of aiment
#define MOVETYPE_PUSHSTEP       13      // BSP model that needs physics/world collisions (uses nearest hull for world collision)

// edict->solid values
// NOTE: Some movetypes will cause collisions independent of SOLID_NOT/SOLID_TRIGGER when the entity moves
// SOLID only effects OTHER entities colliding with this one when they move - UGH!
#define    SOLID_NOT                0   // no interaction with other objects
#define    SOLID_TRIGGER            1   // touch on edge, but not blocking
#define    SOLID_BBOX               2   // touch on edge, block
#define    SOLID_SLIDEBOX           3   // touch on edge, but not an onground
#define    SOLID_BSP                4   // bsp clip, touch on edge, block

// edict->deadflag values
#define    DEAD_NO              0       // alive
#define    DEAD_DYING           1       // playing death animation or still falling off of a ledge waiting to hit ground
#define    DEAD_DEAD            2       // dead. lying still.
#define DEAD_RESPAWNABLE        3
#define DEAD_DISCARDBODY        4

#define    DAMAGE_NO            0
#define    DAMAGE_YES           1
#define    DAMAGE_AIM           2

// entity effects
#define    EF_BRIGHTFIELD       1       // swirling cloud of particles
#define    EF_MUZZLEFLASH       2       // single frame ELIGHT on entity attachment 0
#define    EF_BRIGHTLIGHT       4       // DLIGHT centered at entity origin
#define    EF_DIMLIGHT          8       // player flashlight
#define EF_INVLIGHT             16      // get lighting from ceiling
#define EF_NOINTERP             32      // don't interpolate the next frame
#define EF_LIGHT                64      // rocket flare glow sprite
#define EF_NODRAW               128     // don't draw entity

// entity flags
#define EFLAG_SLERP             1       // do studio interpolation of this entity

//
// temp entity events
//
#define    TE_BEAMPOINTS        0       // beam effect between two points
// coord coord coord (start position) 
// coord coord coord (end position) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define    TE_BEAMENTPOINT        1     // beam effect between point and entity
// short (start entity) 
// coord coord coord (end position) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define    TE_GUNSHOT          2        // particle effect plus ricochet sound
// coord coord coord (position) 

#define    TE_EXPLOSION        3        // additive sprite, 2 dynamic lights, flickering particles, explosion sound, move vertically 8 pps
// coord coord coord (position) 
// short (sprite index)
// byte (scale in 0.1's)
// byte (framerate)
// byte (flags)
//
// The Explosion effect has some flags to control performance/aesthetic features:
#define TE_EXPLFLAG_NONE        0       // all flags clear makes default Half-Life explosion
#define TE_EXPLFLAG_NOADDITIVE  1       // sprite will be drawn opaque (ensure that the sprite you send is a non-additive sprite)
#define TE_EXPLFLAG_NODLIGHTS   2       // do not render dynamic lights
#define TE_EXPLFLAG_NOSOUND     4       // do not play client explosion sound
#define TE_EXPLFLAG_NOPARTICLES 8       // do not draw particles


#define    TE_TAREXPLOSION        4     // Quake1 "tarbaby" explosion with sound
// coord coord coord (position) 

#define    TE_SMOKE            5        // alphablend sprite, move vertically 30 pps
// coord coord coord (position) 
// short (sprite index)
// byte (scale in 0.1's)
// byte (framerate)

#define    TE_TRACER            6       // tracer effect from point to point
// coord, coord, coord (start) 
// coord, coord, coord (end)

#define    TE_LIGHTNING        7        // TE_BEAMPOINTS with simplified parameters
// coord, coord, coord (start) 
// coord, coord, coord (end) 
// byte (life in 0.1's) 
// byte (width in 0.1's) 
// byte (amplitude in 0.01's)
// short (sprite model index)

#define    TE_BEAMENTS            8
// short (start entity) 
// short (end entity) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define    TE_SPARKS            9       // 8 random tracers with gravity, ricochet sprite
// coord coord coord (position) 

#define    TE_LAVASPLASH        10      // Quake1 lava splash
// coord coord coord (position) 

#define    TE_TELEPORT            11    // Quake1 teleport splash
// coord coord coord (position) 

#define TE_EXPLOSION2        12 // Quake1 colormaped (base palette) particle explosion with sound
// coord coord coord (position) 
// byte (starting color)
// byte (num colors)

#define TE_BSPDECAL            13       // Decal from the .BSP file
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// short (texture index of precached decal texture name)
// short (entity index)
// [optional - only included if previous short is non-zero (not the world)] short (index of model of above entity)

#define TE_IMPLOSION        14  // tracers moving toward a point
// coord, coord, coord (position)
// byte (radius)
// byte (count)
// byte (life in 0.1's) 

#define TE_SPRITETRAIL        15        // line of moving glow sprites with gravity, fadeout, and collisions
// coord, coord, coord (start) 
// coord, coord, coord (end) 
// short (sprite index)
// byte (count)
// byte (life in 0.1's) 
// byte (scale in 0.1's) 
// byte (velocity along Vector in 10's)
// byte (randomness of velocity in 10's)

#define TE_BEAM                16       // obsolete

#define TE_SPRITE            17 // additive sprite, plays 1 cycle
// coord, coord, coord (position) 
// short (sprite index) 
// byte (scale in 0.1's) 
// byte (brightness)

#define TE_BEAMSPRITE        18 // A beam with a sprite at the end
// coord, coord, coord (start position) 
// coord, coord, coord (end position) 
// short (beam sprite index) 
// short (end sprite index) 

#define TE_BEAMTORUS        19  // screen aligned beam ring, expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMDISK            20       // disk that expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMCYLINDER        21       // cylinder that expands to max radius over lifetime
// coord coord coord (center position) 
// coord coord coord (axis and radius) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_BEAMFOLLOW        22 // create a line of decaying beam segments until entity stops moving
// short (entity:attachment to follow)
// short (sprite index)
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte,byte,byte (color)
// byte (brightness)

#define TE_GLOWSPRITE        23
// coord, coord, coord (pos) short (model index) byte (scale / 10)

#define TE_BEAMRING            24       // connect a beam ring to two entities
// short (start entity) 
// short (end entity) 
// short (sprite index) 
// byte (starting frame) 
// byte (frame rate in 0.1's) 
// byte (life in 0.1's) 
// byte (line width in 0.1's) 
// byte (noise amplitude in 0.01's) 
// byte,byte,byte (color)
// byte (brightness)
// byte (scroll speed in 0.1's)

#define TE_STREAK_SPLASH    25  // oriented shower of tracers
// coord coord coord (start position) 
// coord coord coord (direction Vector) 
// byte (color)
// short (count)
// short (base speed)
// short (ramdon velocity)

#define TE_BEAMHOSE            26       // obsolete

#define TE_DLIGHT            27 // dynamic light, effect world, minor entity effect
// coord, coord, coord (pos) 
// byte (radius in 10's) 
// byte byte byte (color)
// byte (brightness)
// byte (life in 10's)
// byte (decay rate in 10's)

#define TE_ELIGHT            28 // point entity light, no world effect
// short (entity:attachment to follow)
// coord coord coord (initial position) 
// coord (radius)
// byte byte byte (color)
// byte (life in 0.1's)
// coord (decay rate)

#define TE_TEXTMESSAGE        29
// short 1.2.13 x (-1 = center)
// short 1.2.13 y (-1 = center)
// byte Effect 0 = fade in/fade out
// 1 is flickery credits
// 2 is write out (training room)

// 4 bytes r,g,b,a color1   (text color)
// 4 bytes r,g,b,a color2   (effect color)
// ushort 8.8 fadein time
// ushort 8.8  fadeout time
// ushort 8.8 hold time
// optional ushort 8.8 fxtime   (time the highlight lags behing the leading text in effect 2)
// string text message       (512 chars max sz string)
#define TE_LINE                30
// coord, coord, coord        startpos
// coord, coord, coord        endpos
// short life in 0.1 s
// 3 bytes r, g, b

#define TE_BOX                31
// coord, coord, coord        boxmins
// coord, coord, coord        boxmaxs
// short life in 0.1 s
// 3 bytes r, g, b

#define TE_KILLBEAM            99       // kill all beams attached to entity
// short (entity)

#define TE_LARGEFUNNEL        100
// coord coord coord (funnel position)
// short (sprite index) 
// short (flags) 

#define    TE_BLOODSTREAM        101    // particle spray
// coord coord coord (start position)
// coord coord coord (spray Vector)
// byte (color)
// byte (speed)

#define    TE_SHOWLINE            102   // line of particles every 5 units, dies in 30 seconds
// coord coord coord (start position)
// coord coord coord (end position)

#define TE_BLOOD            103 // particle spray
// coord coord coord (start position)
// coord coord coord (spray Vector)
// byte (color)
// byte (speed)

#define TE_DECAL            104 // Decal applied to a brush entity (not the world)
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name)
// short (entity index)

#define TE_FIZZ                105      // create alpha sprites inside of entity, float upwards
// short (entity)
// short (sprite index)
// byte (density)

#define TE_MODEL            106 // create a moving model that bounces and makes a sound when it hits
// coord, coord, coord (position) 
// coord, coord, coord (velocity)
// angle (initial yaw)
// short (model index)
// byte (bounce sound type)
// byte (life in 0.1's)

#define TE_EXPLODEMODEL        107      // spherical shower of models, picks from set
// coord, coord, coord (origin)
// coord (velocity)
// short (model index)
// short (count)
// byte (life in 0.1's)

#define TE_BREAKMODEL        108        // box of models or sprites
// coord, coord, coord (position)
// coord, coord, coord (size)
// coord, coord, coord (velocity)
// byte (random velocity in 10's)
// short (sprite or model index)
// byte (count)
// byte (life in 0.1 secs)
// byte (flags)

#define TE_GUNSHOTDECAL        109      // decal and ricochet sound
// coord, coord, coord (position)
// short (entity index)
// byte (decal)

#define TE_SPRITE_SPRAY        110      // spay of alpha sprites
// coord, coord, coord (position)
// coord, coord, coord (velocity)
// short (sprite index)
// byte (count)
// byte (speed)
// byte (noise)

#define TE_ARMOR_RICOCHET    111        // quick spark sprite, client ricochet sound.
// coord, coord, coord (position)
// byte (scale in 0.1's)

#define TE_PLAYERDECAL        112       // ???
// byte (playerindex)
// coord, coord, coord (position)
// short (entity)
// byte (decal number)
// [optional] short (model index)

#define TE_BUBBLES            113       // create alpha sprites inside of box, float upwards
// coord, coord, coord (min start position)
// coord, coord, coord (max start position)
// coord (float height)
// short (model index)
// byte (count)
// coord (speed)

#define TE_BUBBLETRAIL        114       // create alpha sprites along a line, float upwards
// coord, coord, coord (min start position)
// coord, coord, coord (max start position)
// coord (float height)
// short (model index)
// byte (count)
// coord (speed)

#define TE_BLOODSPRITE        115       // spray of opaque sprite1's that fall, single sprite2 for 1..2 secs (this is a high-priority tent)
// coord, coord, coord (position)
// short (sprite1 index)
// short (sprite2 index)
// byte (color)
// byte (scale)

#define TE_WORLDDECAL        116        // Decal applied to the world brush
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name)

#define TE_WORLDDECALHIGH    117        // Decal (with texture index > 256) applied to world brush
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name - 256)

#define TE_DECALHIGH        118 // Same as TE_DECAL, but the texture index was greater than 256
// coord, coord, coord (x,y,z), decal position (center of texture in world)
// byte (texture index of precached decal texture name - 256)
// short (entity index)

#define TE_PROJECTILE        119        // Makes a projectile (like a nail) (this is a high-priority tent)
// coord, coord, coord (position)
// coord, coord, coord (velocity)
// short (modelindex)
// byte (life)
// byte (owner)  projectile won't collide with owner (if owner == 0, projectile will hit any client).

#define TE_SPRAY            120 // Throws a shower of sprites or models
// coord, coord, coord (position)
// coord, coord, coord (direction)
// short (modelindex)
// byte (count)
// byte (speed)
// byte (noise)
// byte (rendermode)

#define TE_PLAYERSPRITES    121 // sprites emit from a player's bounding box (ONLY use for players!)
// byte (playernum)
// short (sprite modelindex)
// byte (count)
// byte (variance) (0 = no variance in size) (10 = 10% variance in size)

#define TE_PARTICLEBURST    122 // very similar to lavasplash.
// coord (origin)
// short (radius)
// byte (particle color)
// byte (duration * 10) (will be randomized a bit)

#define TE_FIREFIELD            123     // makes a field of fire.
// coord (origin)
// short (radius) (fire is made in a square around origin. -radius, -radius to radius, radius)
// short (modelindex)
// byte (count)
// byte (flags)
// byte (duration (in seconds) * 10) (will be randomized a bit)
//
// to keep network traffic low, this message has associated flags that fit into a byte:
#define TEFIRE_FLAG_ALLFLOAT    1       // all sprites will drift upwards as they animate
#define TEFIRE_FLAG_SOMEFLOAT   2       // some of the sprites will drift upwards. (50% chance)
#define TEFIRE_FLAG_LOOP        4       // if set, sprite plays at 15 fps, otherwise plays at whatever rate stretches the animation over the sprite's duration.
#define TEFIRE_FLAG_ALPHA       8       // if set, sprite is rendered alpha blended at 50% else, opaque
#define TEFIRE_FLAG_PLANAR     16       // if set, all fire sprites have same initial Z instead of randomly filling a cube.

#define TE_PLAYERATTACHMENT            124      // attaches a TENT to a player (this is a high-priority tent)
// byte (entity index of player)
// coord (vertical offset) ( attachment origin.z = player origin.z + vertical offset )
// short (model index)
// short (life * 10 );

#define TE_KILLPLAYERATTACHMENTS    125 // will expire all TENTS attached to a player.
// byte (entity index of player)

#define TE_MULTIGUNSHOT                126      // much more compact shotgun message
// This message is used to make a client approximate a 'spray' of gunfire.
// Any weapon that fires more than one bullet per frame and fires in a bit of a spread is
// a good candidate for MULTIGUNSHOT use. (shotguns)
//
// NOTE: This effect makes the client do traces for each bullet, these client traces ignore
//         entities that have studio models.Traces are 4096 long.
//
// coord (origin)
// coord (origin)
// coord (origin)
// coord (direction)
// coord (direction)
// coord (direction)
// coord (x noise * 100)
// coord (y noise * 100)
// byte (count)
// byte (bullethole decal texture index)

#define TE_USERTRACER                127        // larger message than the standard tracer, but allows some customization.
// coord (origin)
// coord (origin)
// coord (origin)
// coord (velocity)
// coord (velocity)
// coord (velocity)
// byte ( life * 10 )
// byte ( color ) this is an index into an array of color vectors in the engine. (0 - )
// byte ( length * 10 )



#define MSG_BROADCAST       0   // unreliable to all
#define MSG_ONE             1   // reliable to one (msg_entity)
#define MSG_ALL             2   // reliable to all
#define MSG_INIT            3   // write to the init string
#define MSG_PVS             4   // Ents in PVS of org
#define MSG_PAS             5   // Ents in PAS of org
#define MSG_PVS_R           6   // Reliable to PVS
#define MSG_PAS_R           7   // Reliable to PAS
#define MSG_ONE_UNRELIABLE  8   // Send to one client, but don't put in reliable stream, put in unreliable datagram ( could be dropped )
#define MSG_SPEC            9   // Sends to all spectator proxies

// contents of a spot in the world
#define    CONTENTS_EMPTY      -1
#define    CONTENTS_SOLID      -2
#define    CONTENTS_WATER      -3
#define    CONTENTS_SLIME      -4
#define    CONTENTS_LAVA       -5
#define    CONTENTS_SKY        -6

#define    CONTENTS_LADDER     -16

#define    CONTENT_FLYFIELD            -17
#define    CONTENT_GRAVITY_FLYFIELD    -18
#define    CONTENT_FOG                 -19

#define CONTENT_EMPTY    -1
#define CONTENT_SOLID    -2
#define CONTENT_WATER    -3
#define CONTENT_SLIME    -4
#define CONTENT_LAVA     -5
#define CONTENT_SKY      -6

// channels
#define CHAN_AUTO              0
#define CHAN_WEAPON            1
#define CHAN_VOICE             2
#define CHAN_ITEM              3
#define CHAN_BODY              4
#define CHAN_STREAM            5        // allocate stream channel from the static or dynamic area
#define CHAN_STATIC            6        // allocate channel from the static area
#define CHAN_NETWORKVOICE_BASE 7        // voice data coming across the network
#define CHAN_NETWORKVOICE_END  500      // network voice data reserves slots (CHAN_NETWORKVOICE_BASE through CHAN_NETWORKVOICE_END).

// attenuation values
#define ATTN_NONE        0
#define ATTN_NORM       (float)0.8
#define ATTN_IDLE       (float)2
#define ATTN_STATIC     (float)1.25

// pitch values
#define PITCH_NORM       100    // non-pitch shifted
#define PITCH_LOW        95     // other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH       120

// volume values
#define VOL_NORM        1.0

// plats
#define PLAT_LOW_TRIGGER    1

// Trains
#define SF_TRAIN_WAIT_RETRIGGER  1
#define SF_TRAIN_START_ON        4      // Train is initially moving
#define SF_TRAIN_PASSABLE        8      // Train is not solid -- used to make water trains

// buttons
#define IN_ATTACK   (1 << 0)
#define IN_JUMP     (1 << 1)
#define IN_DUCK     (1 << 2)
#define IN_FORWARD  (1 << 3)
#define IN_BACK     (1 << 4)
#define IN_USE      (1 << 5)
#define IN_CANCEL   (1 << 6)
#define IN_LEFT     (1 << 7)
#define IN_RIGHT    (1 << 8)
#define IN_MOVELEFT (1 << 9)
#define IN_MOVERIGHT (1 << 10)
#define IN_ATTACK2  (1 << 11)
#define IN_RUN      (1 << 12)
#define IN_RELOAD   (1 << 13)
#define IN_ALT1     (1 << 14)
#define IN_SCORE    (1 << 15)

// Break Model Defines

#define BREAK_TYPEMASK    0x4F
#define BREAK_GLASS       0x01
#define BREAK_METAL       0x02
#define BREAK_FLESH       0x04
#define BREAK_WOOD        0x08

#define BREAK_SMOKE       0x10
#define BREAK_TRANS       0x20
#define BREAK_CONCRETE    0x40
#define BREAK_2           0x80

// Colliding temp entity sounds

#define BOUNCE_GLASS    BREAK_GLASS
#define BOUNCE_METAL    BREAK_METAL
#define BOUNCE_FLESH    BREAK_FLESH
#define BOUNCE_WOOD     BREAK_WOOD
#define BOUNCE_SHRAP    0x10
#define BOUNCE_SHELL    0x20
#define BOUNCE_CONCRETE BREAK_CONCRETE
#define BOUNCE_SHOTSHELL 0x80

// Temp entity bounce sound types
#define TE_BOUNCE_NULL       0
#define TE_BOUNCE_SHELL      1
#define TE_BOUNCE_SHOTSHELL  2

// Rendering constants
enum
{
    kRenderNormal,               // src
    kRenderTransColor,           // c*a+dest*(1-a)
    kRenderTransTexture,         // src*a+dest*(1-a)
    kRenderGlow,                 // src*a+dest -- No Z buffer checks
    kRenderTransAlpha,           // src*srca+dest*(1-srca)
    kRenderTransAdd,             // src*a+dest
};

enum
{
    kRenderFxNone = 0,
    kRenderFxPulseSlow,
    kRenderFxPulseFast,
    kRenderFxPulseSlowWide,
    kRenderFxPulseFastWide,
    kRenderFxFadeSlow,
    kRenderFxFadeFast,
    kRenderFxSolidSlow,
    kRenderFxSolidFast,
    kRenderFxStrobeSlow,
    kRenderFxStrobeFast,
    kRenderFxStrobeFaster,
    kRenderFxFlickerSlow,
    kRenderFxFlickerFast,
    kRenderFxNoDissipation,
    kRenderFxDistort,            // Distort/scale/translate flicker
    kRenderFxHologram,           // kRenderFxDistort + distance fade
    kRenderFxDeadPlayer,         // kRenderAmt is the player index
    kRenderFxExplode,            // Scale up really big!
    kRenderFxGlowShell,          // Glowing Shell
    kRenderFxClampMinScale,      // Keep this sprite from getting very small (SPRITES only!)
};


typedef int func_t;

typedef unsigned char uint8_t;
typedef unsigned short word;

#define _DEF_BYTE_

#undef true
#undef false

#ifndef __cplusplus
typedef enum
{
    false, true
} qboolean;
#else
typedef int qboolean;
#endif

typedef struct
{
    uint8_t r, g, b;
} color24;

typedef struct
{
    unsigned r, g, b, a;
} colorVec;

#ifdef _WIN32
#pragma pack(push,2)
#endif

typedef struct
{
    unsigned short r, g, b, a;
} PackedColorVec;

#ifdef _WIN32
#pragma pack(pop)
#endif
typedef struct link_s
{
    struct link_s* prev, * next;
} link_t;

typedef struct edict_s edict_t;

typedef struct
{
    Vector normal;
    float dist;
} plane_t;

typedef struct
{
    qboolean allsolid;           // if true, plane is not valid
    qboolean startsolid;         // if true, the initial point was in a solid area
    qboolean inopen, inwater;
    float fraction;              // time completed, 1.0f = didn't hit anything
    Vector endpos;               // final position
    plane_t plane;               // surface normal at impact
    edict_t* ent;                // entity the surface is on
    int hitgroup;                // 0 == generic, non zero is specific body part
} trace_t;


#define FCVAR_ARCHIVE      (1 << 0)     // set to cause it to be saved to vars.rc
#define FCVAR_USERINFO     (1 << 1)     // changes the client's info string
#define FCVAR_SERVER       (1 << 2)     // notifies players when changed
#define FCVAR_EXTDLL       (1 << 3)     // defined by external DLL
#define FCVAR_CLIENTDLL    (1 << 4)     // defined by the client dll
#define FCVAR_PROTECTED    (1 << 5)     // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY       (1 << 6)     // This cvar cannot be changed by clients connected to a multiplayer server.
#define FCVAR_PRINTABLEONLY (1 << 7)    // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED     (1 << 8)     // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log


typedef enum
{
    at_notice,
    at_console,                  // same as at_notice, but forces a ConPrintf, not a message box
    at_aiconsole,                // same as at_console, but only shown if developer level is 2!
    at_warning,
    at_error,
    at_logged                    // Server print to console ( only in multiplayer games ).
} ALERT_TYPE;

// 4-22-98  JOHN: added for use in pfnClientPrintf
typedef enum
{
    print_console,
    print_center,
    print_chat,
} PRINT_TYPE;

typedef enum
{
    print_withtag = print_console | 0x3ff,
} PRINT_TYPE_EX;                // (dz): added for bots needs

// For integrity checking of content on clients
typedef enum
{
    force_exactfile,             // File on client must exactly match server's file
    force_model_samebounds,      // For model files only, the geometry must fit in the same bbox
    force_model_specifybounds,   // For model files only, the geometry must fit in the specified bbox
} FORCE_TYPE;


typedef enum
{
    PNL_NULL = 0,
    PNL_INI_DELETED,
    PNL_FILE_NEWER,
    PNL_COMMAND,
    PNL_CMD_FORCED,
    PNL_DELAYED,
    PNL_PLUGIN,
    PNL_PLG_FORCED,
    PNL_RELOAD,
} PL_UNLOAD_REASON;

typedef enum
{
    MRES_UNSET = 0,
    MRES_IGNORED,
    MRES_HANDLED,
    MRES_OVERRIDE,
    MRES_SUPERCEDE,
} META_RES;

typedef enum
{
    PT_NEVER = 0,
    PT_STARTUP,
    PT_CHANGELEVEL,
    PT_ANYTIME,
    PT_ANYPAUSE,
} PLUG_LOADTIME;

// for getgameinfo:
typedef enum
{
    GINFO_NAME = 0,
    GINFO_DESC,
    GINFO_GAMEDIR,
    GINFO_DLL_FULLPATH,
    GINFO_DLL_FILENAME,
    GINFO_REALDLL_FULLPATH,
} ginfo_t;




typedef struct cvar_s
{
    char* name;
    char* string;                // (dz): changed since genclass.h library #define
    int flags;
    float value;
    struct cvar_s* next;
} cvar_t;

//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//        include progdefs.h
// This is conveniently done for them in extdll.h
//

#ifdef _WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT               /* */
#endif

// Returned by TraceLine
typedef struct
{
    int fAllSolid;               // if true, plane is not valid
    int fStartSolid;             // if true, the initial point was in a solid area
    int fInOpen;
    int fInWater;
    float flFraction;            // time completed, 1.0f = didn't hit anything
    Vector vecEndPos;            // final position
    float flPlaneDist;
    Vector vecPlaneNormal;       // surface normal at impact
    edict_t* pHit;               // entity the surface is on
    int iHitgroup;               // 0 == generic, non zero is specific body part
} TraceResult;

// CD audio status
typedef struct
{
    int fPlaying;                // is sound playing right now?
    int fWasPlaying;             // if not, CD is paused if WasPlaying is true.
    int fInitialized;
    int fEnabled;
    int fPlayLooping;
    float cdvolume;
    //BYTE     remap[100];
    int fCDRom;
    int fPlayTrack;
} CDStatus;

typedef uint32 CRC32_t;

// Engine hands this to DLLs for functionality callbacks
typedef struct enginefuncs_s
{
    int (*pfnPrecacheModel) (char* s);
    int (*pfnPrecacheSound) (char* s);
    void (*pfnSetModel) (edict_t* e, const char* m);
    int (*pfnModelIndex) (const char* m);
    int (*pfnModelFrames) (int modelIndex);
    void (*pfnSetSize) (edict_t* e, const float* rgflMin, const float* rgflMax);
    void (*pfnChangeLevel) (char* s1, char* s2);
    void (*pfnGetSpawnParms) (edict_t* ent);
    void (*pfnSaveSpawnParms) (edict_t* ent);
    float (*pfnVecToYaw) (const float* rgflVector);
    void (*pfnVecToAngles) (const float* rgflVectorIn, float* rgflVectorOut);
    void (*pfnMoveToOrigin) (edict_t* ent, const float* pflGoal, float dist, int iMoveType);
    void (*pfnChangeYaw) (edict_t* ent);
    void (*pfnChangePitch) (edict_t* ent);
    edict_t* (*pfnFindEntityByString) (edict_t* pentEdictStartSearchAfter, const char* pszField, const char* pszValue);
    int (*pfnGetEntityIllum) (edict_t* pEnt);
    edict_t* (*pfnFindEntityInSphere) (edict_t* pentEdictStartSearchAfter, const float* org, float rad);
    edict_t* (*pfnFindClientInPVS) (edict_t* pentEdict);
    edict_t* (*pfnEntitiesInPVS) (edict_t* pplayer);
    void (*pfnMakeVectors) (const float* rgflVector);
    void (*pfnAngleVectors) (const float* rgflVector, float* forward, float* right, float* up);
    edict_t* (*pfnCreateEntity) (void);
    void (*pfnRemoveEntity) (edict_t* e);
    edict_t* (*pfnCreateNamedEntity) (int className);
    void (*pfnMakeStatic) (edict_t* ent);
    int (*pfnEntIsOnFloor) (edict_t* e);
    int (*pfnDropToFloor) (edict_t* e);
    int (*pfnWalkMove) (edict_t* ent, float yaw, float dist, int iMode);
    void (*pfnSetOrigin) (edict_t* e, const float* rgflOrigin);
    void (*pfnEmitSound) (edict_t* entity, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);
    void (*pfnEmitAmbientSound) (edict_t* entity, float* pos, const char* samp, float vol, float attenuation, int fFlags, int pitch);
    void (*pfnTraceLine) (const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr);
    void (*pfnTraceToss) (edict_t* pent, edict_t* pentToIgnore, TraceResult* ptr);
    int (*pfnTraceMonsterHull) (edict_t* pentEdict, const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr);
    void (*pfnTraceHull) (const float* v1, const float* v2, int fNoMonsters, int hullNumber, edict_t* pentToSkip, TraceResult* ptr);
    void (*pfnTraceModel) (const float* v1, const float* v2, int hullNumber, edict_t* pent, TraceResult* ptr);
    const char* (*pfnTraceTexture) (edict_t* pTextureEntity, const float* v1, const float* v2);
    void (*pfnTraceSphere) (const float* v1, const float* v2, int fNoMonsters, float radius, edict_t* pentToSkip, TraceResult* ptr);
    void (*pfnGetAimVector) (edict_t* ent, float speed, float* rgflReturn);
    void (*pfnServerCommand) (char* str);
    void (*pfnServerExecute) (void);
    void (*pfnClientCommand) (edict_t* pentEdict, char* szFmt, ...);
    void (*pfnParticleEffect) (const float* org, const float* dir, float color, float count);
    void (*pfnLightStyle) (int style, char* val);
    int (*pfnDecalIndex) (const char* name);
    int (*pfnPointContents) (const float* rgflVector);
    void (*pfnMessageBegin) (int msg_dest, int msg_type, const float* pOrigin, edict_t* ed);
    void (*pfnMessageEnd) (void);
    void (*pfnWriteByte) (int iValue);
    void (*pfnWriteChar) (int iValue);
    void (*pfnWriteShort) (int iValue);
    void (*pfnWriteLong) (int iValue);
    void (*pfnWriteAngle) (float flValue);
    void (*pfnWriteCoord) (float flValue);
    void (*pfnWriteString) (const char* sz);
    void (*pfnWriteEntity) (int iValue);
    void (*pfnCVarRegister) (cvar_t* pCvar);
    float (*pfnCVarGetFloat) (const char* szVarName);
    const char* (*pfnCVarGetString) (const char* szVarName);
    void (*pfnCVarSetFloat) (const char* szVarName, float flValue);
    void (*pfnCVarSetString) (const char* szVarName, const char* szValue);
    void (*pfnAlertMessage) (ALERT_TYPE atype, char* szFmt, ...);
    void (*pfnEngineFprintf) (void* pfile, char* szFmt, ...);
    void* (*pfnPvAllocEntPrivateData) (edict_t* pentEdict, int32 cb);
    void* (*pfnPvEntPrivateData) (edict_t* pentEdict);
    void (*pfnFreeEntPrivateData) (edict_t* pentEdict);
    const char* (*pfnSzFromIndex) (int iString);
    int (*pfnAllostring) (const char* szValue);
    struct entvars_s* (*pfnGetVarsOfEnt) (edict_t* pentEdict);
    edict_t* (*pfnPEntityOfEntOffset) (int iEntOffset);
    int (*pfnEntOffsetOfPEntity) (const edict_t* pentEdict);
    int (*pfnIndexOfEdict) (const edict_t* pentEdict);
    edict_t* (*pfnPEntityOfEntIndex) (int iEntIndex);
    edict_t* (*pfnFindEntityByVars) (struct entvars_s* pvars);
    void* (*pfnGetModelPtr) (edict_t* pentEdict);
    int (*pfnRegUserMsg) (const char* pszName, int iSize);
    void (*pfnAnimationAutomove) (const edict_t* pentEdict, float flTime);
    void (*pfnGetBonePosition) (const edict_t* pentEdict, int iBone, float* rgflOrigin, float* rgflAngles);
    uint32(*pfnFunctionFromName) (const char* pName);
    const char* (*pfnNameForFunction) (uint32 function);
    void (*pfnClientPrintf) (edict_t* pentEdict, PRINT_TYPE ptype, const char* szMsg);  // JOHN: engine callbacks so game DLL can print messages to individual clients
    void (*pfnServerPrint) (const char* szMsg);
    const char* (*pfnCmd_Args) (void);   // these 3 added 
    const char* (*pfnCmd_Argv) (int argc);       // so game DLL can easily 
    int (*pfnCmd_Argc) (void);   // access client 'cmd' strings
    void (*pfnGetAttachment) (const edict_t* pentEdict, int iAttachment, float* rgflOrigin, float* rgflAngles);
    void (*pfnCRC32_Init) (CRC32_t* pulCRC);
    void (*pfnCRC32_ProcessBuffer) (CRC32_t* pulCRC, void* p, int len);
    void (*pfnCRC32_ProcessByte) (CRC32_t* pulCRC, uint8_t ch);
    CRC32_t(*pfnCRC32_Final) (CRC32_t pulCRC);
    int32(*pfnRandomLong) (int32 lLow, int32 lHigh);
    float (*pfnRandomFloat) (float flLow, float flHigh);
    void (*pfnSetView) (const edict_t* pClient, const edict_t* pViewent);
    float (*pfnTime) (void);
    void (*pfnCrosshairAngle) (const edict_t* pClient, float pitch, float yaw);
    uint8_t* (*pfnLoadFileForMe) (char* filename, int* pLength);
    void (*pfnFreeFile) (void* buffer);
    void (*pfnEndSection) (const char* pszSectionName);  // trigger_endsection
    int (*pfnCompareFileTime) (char* filename1, char* filename2, int* iCompare);
    void (*pfnGetGameDir) (char* szGetGameDir);
    void (*pfnCvar_RegisterVariable) (cvar_t* variable);
    void (*pfnFadeClientVolume) (const edict_t* pentEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);
    void (*pfnSetClientMaxspeed) (const edict_t* pentEdict, float fNewMaxspeed);
    edict_t* (*pfnCreateFakeClient) (const char* netname);       // returns null if fake client can't be created
    //
    void (*pfnRunPlayerMove) (edict_t* fakeclient, const float* viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, uint8_t impulse, uint8_t msec);
    //
    int (*pfnNumberOfEntities) (void);
    char* (*pfnGetInfoKeyBuffer) (edict_t* e);  // passing in null gets the serverinfo
    char* (*pfnInfoKeyValue) (char* infobuffer, char* key);
    void (*pfnSetKeyValue) (char* infobuffer, char* key, char* value);
    void (*pfnSetClientKeyValue) (int clientIndex, char* infobuffer, char* key, char* value);
    int (*pfnIsMapValid) (char* filename);
    void (*pfnStaticDecal) (const float* origin, int decalIndex, int entityIndex, int modelIndex);
    int (*pfnPrecacheGeneric) (char* s);
    int (*pfnGetPlayerUserId) (edict_t* e);     // returns the server assigned userid for this player.  useful for logging frags, etc.  returns -1 if the edict couldn't be found in the list of clients
    void (*pfnBuildSoundMsg) (edict_t* entity, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float* pOrigin, edict_t* ed);
    int (*pfnIsDedicatedServer) (void);  // is this a dedicated server?
    cvar_t* (*pfnCVarGetPointer) (const char* szVarName);
    unsigned int (*pfnGetPlayerWONId) (edict_t* e);     // returns the server assigned WONid for this player.  useful for logging frags, etc.  returns -1 if the edict couldn't be found in the list of clients

    void (*pfnInfo_RemoveKey) (char* s, const char* key);
    const char* (*pfnGetPhysicsKeyValue) (const edict_t* pClient, const char* key);
    void (*pfnSetPhysicsKeyValue) (const edict_t* pClient, const char* key, const char* value);
    const char* (*pfnGetPhysicsInfoString) (const edict_t* pClient);
    unsigned short (*pfnPrecacheEvent) (int type, const char* psz);
    void (*pfnPlaybackEvent) (int flags, const edict_t* pInvoker, unsigned short eventindex, float delay, float* origin, float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
    uint8_t* (*pfnSetFatPVS) (float* org);
    uint8_t* (*pfnSetFatPAS) (float* org);
    int (*pfnCheckVisibility) (const edict_t* entity, uint8_t* pset);
    void (*pfnDeltaSetField) (struct delta_s* pFields, const char* fieldname);
    void (*pfnDeltaUnsetField) (struct delta_s* pFields, const char* fieldname);
    void (*pfnDeltaAddEncoder) (char* name, void (*conditionalencode) (struct delta_s* pFields, const uint8_t* from, const uint8_t* to));
    int (*pfnGetCurrentPlayer) (void);
    int (*pfnCanSkipPlayer) (const edict_t* player);
    int (*pfnDeltaFindField) (struct delta_s* pFields, const char* fieldname);
    void (*pfnDeltaSetFieldByIndex) (struct delta_s* pFields, int fieldNumber);
    void (*pfnDeltaUnsetFieldByIndex) (struct delta_s* pFields, int fieldNumber);
    void (*pfnSetGroupMask) (int mask, int op);
    int (*pfnCreateInstancedBaseline) (int classname, struct entity_state_s* baseline);
    void (*pfnCvar_DirectSet) (struct cvar_s* var, char* value);
    void (*pfnForceUnmodified) (FORCE_TYPE type, float* mins, float* maxs, const char* filename);
    void (*pfnGetPlayerStats) (const edict_t* pClient, int* ping, int* packet_loss);
    void (*pfnAddServerCommand) (char* cmd_name, void (*function) (void));

    qboolean(*pfnVoice_GetClientListening) (int iReceiver, int iSender);
    qboolean(*pfnVoice_SetClientListening) (int iReceiver, int iSender, qboolean bListen);

    const char* (*pfnGetPlayerAuthId) (edict_t* e);

    struct sequenceEntry_s* (*pfnSequenceGet) (const char* fileName, const char* entryName);
    struct sentenceEntry_s* (*pfnSequencePickSentence) (const char* groupName, int pickMethod, int* picked);

    int (*pfnGetFileSize) (char* filename);
    unsigned int (*pfnGetApproxWavePlayLen) (const char* filepath);

    int (*pfnIsCareerMatch) (void);
    int (*pfnGetLocalizedStringLength) (const char* label);
    void (*pfnRegisterTutorMessageShown) (int mid);
    int (*pfnGetTimesTutorMessageShown) (int mid);
    void (*pfnProcessTutorMessageDecayBuffer) (int* buffer, int bufferLength);
    void (*pfnConstructTutorMessageDecayBuffer) (int* buffer, int bufferLength);
    void (*pfnResetTutorMessageDecayData) (void);

    void (*pfnQueryClientCvarValue) (const edict_t* player, const char* cvarName);
    void (*pfnQueryClientCvarValue2) (const edict_t* player, const char* cvarName, int requestID);
} enginefuncs_t;

// Passed to pfnKeyValue
typedef struct KeyValueData_s
{
    char* szClassName;           // in: entity classname
    char* szKeyName;             // in: name of key
    char* szValue;               // in: value of key
    int32 fHandled;              // out: DLL sets to true if key-value pair was understood
} KeyValueData;


#define ARRAYSIZE_HLSDK(p)       (sizeof(p)/sizeof(p[0]))
typedef struct customization_s customization_t;

typedef struct
{
    // Initialize/shutdown the game (one-time call after loading of game .dll )
    void (*pfnGameInit) (void);
    int (*pfnSpawn) (edict_t* pent);
    void (*pfnThink) (edict_t* pent);
    void (*pfnUse) (edict_t* pentUsed, edict_t* pentOther);
    void (*pfnTouch) (edict_t* pentTouched, edict_t* pentOther);
    void (*pfnBlocked) (edict_t* pentBlocked, edict_t* pentOther);
    void (*pfnKeyValue) (edict_t* pentKeyvalue, KeyValueData* pkvd);
    void (*pfnSave) (edict_t* pent, struct SAVERESTOREDATA* pSaveData);
    int (*pfnRestore) (edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
    void (*pfnSetAbsBox) (edict_t* pent);

    void (*pfnSaveWriteFields) (SAVERESTOREDATA*, const char*, void*, struct TYPEDESCRIPTION*, int);
    void (*pfnSaveReadFields) (SAVERESTOREDATA*, const char*, void*, TYPEDESCRIPTION*, int);

    void (*pfnSaveGlobalState) (SAVERESTOREDATA*);
    void (*pfnRestoreGlobalState) (SAVERESTOREDATA*);
    void (*pfnResetGlobalState) (void);

    qboolean(*pfnClientConnect) (edict_t* pentEdict, const char* pszName, const char* pszAddress, char szRejectReason[128]);

    void (*pfnClientDisconnect) (edict_t* pentEdict);
    void (*pfnClientKill) (edict_t* pentEdict);
    void (*pfnClientPutInServer) (edict_t* pentEdict);
    void (*pfnClientCommand) (edict_t* pentEdict);
    void (*pfnClientUserInfoChanged) (edict_t* pentEdict, char* infobuffer);

    void (*pfnServerActivate) (edict_t* pentEdictList, int edictCount, int clientMax);
    void (*pfnServerDeactivate) (void);

    void (*pfnPlayerPreThink) (edict_t* pentEdict);
    void (*pfnPlayerPostThink) (edict_t* pentEdict);

    void (*pfnStartFrame) (void);
    void (*pfnParmsNewLevel) (void);
    void (*pfnParmsChangeLevel) (void);

    // Returns string describing current .dll.  E.g., TeamFotrress 2, Half-Life
    const char* (*pfnGetGameDescription) (void);

    // Notify dll about a player customization.
    void (*pfnPlayerCustomization) (edict_t* pentEdict, struct customization_s* pCustom);

    // Spectator funcs
    void (*pfnSpectatorConnect) (edict_t* pentEdict);
    void (*pfnSpectatorDisconnect) (edict_t* pentEdict);
    void (*pfnSpectatorThink) (edict_t* pentEdict);

    // Notify game .dll that engine is going to shut down.  Allows mod authors to set a breakpoint.
    void (*pfnSys_Error) (const char* error_string);

    void (*pfnPM_Move) (struct playermove_s* ppmove, qboolean server);
    void (*pfnPM_Init) (struct playermove_s* ppmove);
    char (*pfnPM_FindTextureType) (char* name);
    void (*pfnSetupVisibility) (struct edict_s* pViewEntity, struct edict_s* pClient, uint8_t** pvs, uint8_t** ucPAS);
    void (*pfnUpdateClientData) (const struct edict_s* ent, int sendweapons, struct clientdata_s* cd);
    int (*pfnAddToFullPack) (struct entity_state_s* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, uint8_t* pSet);
    void (*pfnCreateBaseline) (int player, int eindex, struct entity_state_s* baseline, struct edict_s* entity, int playermodelindex, Vector player_mins, Vector player_maxs);
    void (*pfnRegisterEncoders) (void);
    int (*pfnGetWeaponData) (struct edict_s* player, struct weapon_data_s* info);

    void (*pfnCmdStart) (const edict_t* player, const struct c* cmd, unsigned int random_seed);
    void (*pfnCmdEnd) (const edict_t* player);

    // Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
    //  size of the response_buffer, so you must zero it out if you choose not to respond.
    int (*pfnConnectionlessPacket) (const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size);

    // Enumerates player hulls.  Returns 0 if the hull number doesn't exist, 1 otherwise
    int (*pfnGetHullBounds) (int hullnumber, float* mins, float* maxs);

    // Create baselines for certain "unplaced" items.
    void (*pfnCreateInstancedBaselines) (void);

    // One of the pfnForceUnmodified files failed the consistency check for the specified player
    // Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
    int (*pfnInconsistentFile) (const struct edict_s* player, const char* filename, char* disconnect_message);

    // The game .dll should return 1 if lag compensation should be allowed ( could also just set
    //  the sv_unlag cvar.
    // Most games right now should return 0, until client-side weapon prediction code is written
    //  and tested for them.
    int (*pfnAllowLagCompensation) (void);
} DLL_FUNCTIONS;

// Current version.
#define NEW_DLL_FUNCTIONS_VERSION    1

typedef struct
{
    // Called right before the object's memory is freed. 
    // Calls its destructor.
    void (*pfnOnFreeEntPrivateData) (edict_t* pEnt);
    void (*pfnGameShutdown) (void);
    int (*pfnShouldCollide) (edict_t* pentTouched, edict_t* pentOther);

    void (*pfnCvarValue) (const edict_t* pEnt, const char* value);
    void (*pfnCvarValue2) (const edict_t* pEnt, int requestID, const char* cvarName, const char* value);
} NEW_DLL_FUNCTIONS;

// Pointer operators
#define PTR_TO_BYTE(in) *(uint8_t *) (in)
#define PTR_TO_FLT(in) *(float *) (in)
#define PTR_TO_INT(in) *(int *) (in)
#define PTR_TO_STR(in) (char *) (in)


// Must be provided by user of this code
extern enginefuncs_t g_engfuncs;

// The actual engine callbacks
#define GETPLAYERUSERID (*g_engfuncs.pfnGetPlayerUserId)
#define PRECACHE_MODEL (*g_engfuncs.pfnPrecacheModel)
#define PRECACHE_SOUND (*g_engfuncs.pfnPrecacheSound)
#define PRECACHE_GENERIC (*g_engfuncs.pfnPrecacheGeneric)
#define SET_MODEL       (*g_engfuncs.pfnSetModel)
#define MODEL_INDEX     (*g_engfuncs.pfnModelIndex)
#define MODEL_FRAMES   (*g_engfuncs.pfnModelFrames)
#define SET_SIZE       (*g_engfuncs.pfnSetSize)
#define CHANGE_LEVEL   (*g_engfuncs.pfnChangeLevel)
#define GET_SPAWN_PARMS (*g_engfuncs.pfnGetSpawnParms)
#define SAVE_SPAWN_PARMS (*g_engfuncs.pfnSaveSpawnParms)
#define VEC_TO_YAW      (*g_engfuncs.pfnVecToYaw)
#define VEC_TO_ANGLES   (*g_engfuncs.pfnVecToAngles)
#define MOVE_TO_ORIGIN (*g_engfuncs.pfnMoveToOrigin)
#define oldCHANGE_YAW  (*g_engfuncs.pfnChangeYaw)
#define CHANGE_PITCH   (*g_engfuncs.pfnChangePitch)
#define MAKE_VECTORS   (*g_engfuncs.pfnMakeVectors)
#define CREATE_ENTITY  (*g_engfuncs.pfnCreateEntity)
#define REMOVE_ENTITY  (*g_engfuncs.pfnRemoveEntity)
#define CREATE_NAMED_ENTITY (*g_engfuncs.pfnCreateNamedEntity)
#define MAKE_STATIC    (*g_engfuncs.pfnMakeStatic)
#define ENT_IS_ON_FLOOR (*g_engfuncs.pfnEntIsOnFloor)
#define DROP_TO_FLOOR  (*g_engfuncs.pfnDropToFloor)
#define WALK_MOVE      (*g_engfuncs.pfnWalkMove)
#define SET_ORIGIN     (*g_engfuncs.pfnSetOrigin)
#define EMIT_SOUND_DYN2 (*g_engfuncs.pfnEmitSound)
#define BUILD_SOUND_MSG (*g_engfuncs.pfnBuildSoundMsg)
#define TRACE_LINE       (*g_engfuncs.pfnTraceLine)
#define TRACE_TOSS       (*g_engfuncs.pfnTraceToss)
#define TRACE_MONSTER_HULL (*g_engfuncs.pfnTraceMonsterHull)
#define TRACE_HULL       (*g_engfuncs.pfnTraceHull)
#define GET_AIM_VECTOR   (*g_engfuncs.pfnGetAimVector)
#define SERVER_COMMAND   (*g_engfuncs.pfnServerCommand)
#define SERVER_EXECUTE   (*g_engfuncs.pfnServerExecute)
#define CLIENT_COMMAND   (*g_engfuncs.pfnClientCommand)
#define PARTICLE_EFFECT  (*g_engfuncs.pfnParticleEffect)
#define LIGHT_STYLE      (*g_engfuncs.pfnLightStyle)
#define DECAL_INDEX      (*g_engfuncs.pfnDecalIndex)
#define POINT_CONTENTS   (*g_engfuncs.pfnPointContents)
#define CRC32_INIT          (*g_engfuncs.pfnCRC32_Init)
#define CRC32_PROCESS_BUFFER (*g_engfuncs.pfnCRC32_ProcessBuffer)
#define CRC32_PROCESS_BYTE  (*g_engfuncs.pfnCRC32_ProcessByte)
#define CRC32_FINAL         (*g_engfuncs.pfnCRC32_Final)
#define RANDOM_LONG       (*g_engfuncs.pfnRandomLong)
#define RANDOM_FLOAT      (*g_engfuncs.pfnRandomFloat)
#define GETPLAYERAUTHID   (*g_engfuncs.pfnGetPlayerAuthId)
inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float* pOrigin = nullptr, edict_t* ed = nullptr)
{
    (*g_engfuncs.pfnMessageBegin) (msg_dest, msg_type, pOrigin, ed);
}

#define MESSAGE_END      (*g_engfuncs.pfnMessageEnd)
#define WRITE_BYTE       (*g_engfuncs.pfnWriteByte)
#define WRITE_CHAR       (*g_engfuncs.pfnWriteChar)
#define WRITE_SHORT      (*g_engfuncs.pfnWriteShort)
#define WRITE_LONG       (*g_engfuncs.pfnWriteLong)
#define WRITE_ANGLE      (*g_engfuncs.pfnWriteAngle)
#define WRITE_COORD      (*g_engfuncs.pfnWriteCoord)
#define WRITE_STRING   (*g_engfuncs.pfnWriteString)
#define WRITE_ENTITY   (*g_engfuncs.pfnWriteEntity)
#define CVAR_REGISTER  (*g_engfuncs.pfnCVarRegister)
#define CVAR_GET_FLOAT (*g_engfuncs.pfnCVarGetFloat)
#define CVAR_GET_STRING (*g_engfuncs.pfnCVarGetString)
#define CVAR_SET_FLOAT (*g_engfuncs.pfnCVarSetFloat)
#define CVAR_SET_STRING (*g_engfuncs.pfnCVarSetString)
#define CVAR_GET_POINTER (*g_engfuncs.pfnCVarGetPointer)
#define ALERT           (*g_engfuncs.pfnAlertMessage)
#define ENGINE_FPRINTF  (*g_engfuncs.pfnEngineFprintf)
#define ALLOC_PRIVATE   (*g_engfuncs.pfnPvAllocEntPrivateData)
#define GET_PRIVATE(pent)  (pent ? (pent->pvPrivateData) : nullptr);
#define FREE_PRIVATE   (*g_engfuncs.pfnFreeEntPrivateData)
#define ALLOC_STRING   (*g_engfuncs.pfnAllostring)
#define FIND_ENTITY_BY_STRING   (*g_engfuncs.pfnFindEntityByString)
#define GETENTITYILLUM   (*g_engfuncs.pfnGetEntityIllum)
#define FIND_ENTITY_IN_SPHERE (*g_engfuncs.pfnFindEntityInSphere)
#define FIND_CLIENT_IN_PVS   (*g_engfuncs.pfnFindClientInPVS)
#define EMIT_AMBIENT_SOUND   (*g_engfuncs.pfnEmitAmbientSound)
#define GET_MODEL_PTR        (*g_engfuncs.pfnGetModelPtr)
#define REG_USER_MSG         (*g_engfuncs.pfnRegUserMsg)
#define GET_BONE_POSITION    (*g_engfuncs.pfnGetBonePosition)
#define FUNCTION_FROM_NAME   (*g_engfuncs.pfnFunctionFromName)
#define NAME_FOR_FUNCTION    (*g_engfuncs.pfnNameForFunction)
#define TRACE_TEXTURE        (*g_engfuncs.pfnTraceTexture)
#define CLIENT_PRINTF        (*g_engfuncs.pfnClientPrintf)
#define CMD_ARGS             (*g_engfuncs.pfnCmd_Args)
#define CMD_ARGC             (*g_engfuncs.pfnCmd_Argc)
#define CMD_ARGV             (*g_engfuncs.pfnCmd_Argv)
#define GET_ATTACHMENT       (*g_engfuncs.pfnGetAttachment)
#define SET_VIEW             (*g_engfuncs.pfnSetView)
#define SET_CROSSHAIRANGLE   (*g_engfuncs.pfnCrosshairAngle)
#define LOAD_FILE_FOR_ME     (*g_engfuncs.pfnLoadFileForMe)
#define FREE_FILE            (*g_engfuncs.pfnFreeFile)
#define COMPARE_FILE_TIME    (*g_engfuncs.pfnCompareFileTime)
#define GET_GAME_DIR         (*g_engfuncs.pfnGetGameDir)
#define IS_MAP_VALID         (*g_engfuncs.pfnIsMapValid)
#define NUMBER_OF_ENTITIES   (*g_engfuncs.pfnNumberOfEntities)
#define IS_DEDICATED_SERVER  (*g_engfuncs.pfnIsDedicatedServer)
#define PRECACHE_EVENT       (*g_engfuncs.pfnPrecacheEvent)
#define PLAYBACK_EVENT_FULL  (*g_engfuncs.pfnPlaybackEvent)
#define ENGINE_SET_PVS       (*g_engfuncs.pfnSetFatPVS)
#define ENGINE_SET_PAS       (*g_engfuncs.pfnSetFatPAS)
#define ENGINE_CHECK_VISIBILITY (*g_engfuncs.pfnCheckVisibility)
#define DELTA_SET              (*g_engfuncs.pfnDeltaSetField)
#define DELTA_UNSET            (*g_engfuncs.pfnDeltaUnsetField)
#define DELTA_ADDENCODER       (*g_engfuncs.pfnDeltaAddEncoder)
#define ENGINE_CURRENT_PLAYER  (*g_engfuncs.pfnGetCurrentPlayer)
#define ENGINE_CANSKIP         (*g_engfuncs.pfnCanSkipPlayer)
#define DELTA_FINDFIELD        (*g_engfuncs.pfnDeltaFindField)
#define DELTA_SETBYINDEX       (*g_engfuncs.pfnDeltaSetFieldByIndex)
#define DELTA_UNSETBYINDEX     (*g_engfuncs.pfnDeltaUnsetFieldByIndex)
#define ENGINE_GETPHYSINFO     (*g_engfuncs.pfnGetPhysicsInfoString)
#define ENGINE_SETGROUPMASK    (*g_engfuncs.pfnSetGroupMask)
#define ENGINE_INSTANCE_BASELINE (*g_engfuncs.pfnCreateInstancedBaseline)
#define ENGINE_FORCE_UNMODIFIED (*g_engfuncs.pfnForceUnmodified)
#define PLAYER_CNX_STATS       (*g_engfuncs.pfnGetPlayerStats)


#ifdef DLL_DEBUG
#define DEBUG 1
#endif

#ifdef _WIN32
/*
#pragma warning(disable : 4244) // int or float down-conversion
#pragma warning(disable : 4305) // int or float data truncation
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4514) // unreferenced inline function removed
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4715) // not all control paths return a value
#pragma warning(disable : 4996) // function was declared deprecated
*/
/* (dz): disable deprecation warnings concerning unsafe CRT functions */
#if !defined _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
//#include "windows.h"
#else // _WIN32
#define FALSE 0
#define TRUE (!FALSE)
typedef unsigned long ULONG;
typedef uint8_t BYTE;
typedef int BOOL;

#define MAX_PATH PATH_MAX
#include <limits.h>
#include <stdarg.h>
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#endif
#endif //_WIN32


typedef int func_t;             //
typedef float vec_t;            // needed before including progdefs.h

#define vec3_t Vector

#define    MAX_ENT_LEAFS    48

typedef struct entvars_s
{
    int classname;
    int globalname;

    vec3_t origin;
    vec3_t oldorigin;
    vec3_t velocity;
    vec3_t basevelocity;
    vec3_t clbasevelocity;       // Base velocity that was passed in to server physics so 
    // client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
    vec3_t movedir;

    vec3_t angles;               // Model angles
    vec3_t avelocity;            // angle velocity (degrees per second)
    vec3_t punchangle;           // auto-decaying view angle adjustment
    vec3_t v_angle;              // Viewing angle (player only)

    // For parametric entities
    vec3_t endpos;
    vec3_t startpos;
    float impacttime;
    float starttime;

    int fixangle;                // 0:nothing, 1:force view angles, 2:add avelocity
    float idealpitch;
    float pitch_speed;
    float ideal_yaw;
    float yaw_speed;

    int modelindex;
    int model;

    int viewmodel;               // player's viewmodel
    int weaponmodel;             // what other players see

    vec3_t absmin;               // BB max translated to world coord
    vec3_t absmax;               // BB max translated to world coord
    vec3_t mins;                 // local BB min
    vec3_t maxs;                 // local BB max
    vec3_t size;                 // maxs - mins

    float ltime;
    float nextthink;

    int movetype;
    int solid;

    int skin;
    int body;                    // sub-model selection for studiomodels
    int effects;

    float gravity;               // % of "normal" gravity
    float friction;              // inverse elasticity of MOVETYPE_BOUNCE

    int light_level;

    int sequence;                // animation sequence
    int gaitsequence;            // movement animation sequence for player (0 for none)
    float frame;                 // % playback position in animation sequences (0..255)
    float animtime;              // world time when frame was set
    float framerate;             // animation playback rate (-8x to 8x)
    uint8_t controller[4];          // bone controller setting (0..255)
    uint8_t blending[2];            // blending amount between sub-sequences (0..255)

    float scale;                 // sprite rendering scale (0..255)

    int rendermode;
    float renderamt;
    vec3_t rendercolor;
    int renderfx;

    float health;
    float frags;
    int weapons;                 // bit mask for available weapons
    float takedamage;

    int deadflag;
    vec3_t view_ofs;             // eye position

    int button;
    int impulse;

    edict_t* chain;              // Entity pointer when linked into a linked list
    edict_t* dmg_inflictor;
    edict_t* enemy;
    edict_t* aiment;             // entity pointer when MOVETYPE_FOLLOW
    edict_t* owner;
    edict_t* groundentity;

    int spawnflags;
    int flags;

    int colormap;                // lowbyte topcolor, highbyte bottomcolor
    int team;

    float max_health;
    float teleport_time;
    float armortype;
    float armorvalue;
    int waterlevel;
    int watertype;

    int target;
    int targetname;
    int netname;
    int message;

    float dmg_take;
    float dmg_save;
    float dmg;
    float dmgtime;

    int noise;
    int noise1;
    int noise2;
    int noise3;

    float speed;
    float air_finished;
    float pain_finished;
    float radsuit_finished;

    edict_t* pContainingEntity;

    int playerclass;
    float maxspeed;

    float fov;
    int weaponanim;

    int pushmsec;

    int bInDuck;
    int flTimeStepSound;
    int flSwimTime;
    int flDuckTime;
    int iStepLeft;
    float flFallVelocity;

    int gamestate;

    int oldbuttons;

    int groupinfo;

    // For mods
    int iuser1;
    int iuser2;
    int iuser3;
    int iuser4;
    float fuser1;
    float fuser2;
    float fuser3;
    float fuser4;
    vec3_t vuser1;
    vec3_t vuser2;
    vec3_t vuser3;
    vec3_t vuser4;
    edict_t* euser1;
    edict_t* euser2;
    edict_t* euser3;
    edict_t* euser4;
} entvars_t;


struct edict_s
{
    qboolean free;
    int serialnumber;
    link_t area;                 // linked to a division node or leaf
    int headnode;                // -1 to use normal leaf check
    int num_leafs;
    short leafnums[MAX_ENT_LEAFS];
    float freetime;              // sv.time when the object was freed
    void* pvPrivateData;         // Alloced and freed by engine, used by DLLs
    entvars_t v;                 // C exported fields from progs
};


#define MAX_WEAPON_SLOTS   5    // hud item selection slots
#define MAX_ITEM_TYPES     6    // hud item selection slots

#define MAX_ITEMS          5    // hard coded item types

#define HIDEHUD_WEAPONS      ( 1 << 0 )
#define HIDEHUD_FLASHLIGHT   ( 1 << 1 )
#define HIDEHUD_ALL          ( 1 << 2 )
#define HIDEHUD_HEALTH       ( 1 << 3 )

#define MAX_AMMO_TYPES  32      // ???
#define MAX_AMMO_SLOTS  32      // not really slots

#define HUD_PRINTNOTIFY     1
#define HUD_PRINTCONSOLE    2
#define HUD_PRINTTALK       3
#define HUD_PRINTCENTER     4

#define WEAPON_SUIT         31
#define __USE_GNU           1


#undef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT   __declspec(dllexport)
#elif defined(linux)
#define DLLEXPORT               /* */
#define WINAPI                  /* */
#endif

#define C_DLLEXPORT      extern "C" DLLEXPORT

#define META_INTERFACE_VERSION "5:13"

typedef struct meta_globals_s
{
    META_RES mres;
    META_RES prev_mres;
    META_RES status;
    void* orig_ret;
    void* override_ret;
} meta_globals_t;

extern meta_globals_t* gpMetaGlobals;

#define SET_META_RESULT(result) gpMetaGlobals->mres=result
#define RETURN_META(result) { gpMetaGlobals->mres=result; return; }
#define RETURN_META_VALUE(result, value) { gpMetaGlobals->mres=result; return(value); }
#define META_RESULT_STATUS   gpMetaGlobals->status
#define META_RESULT_PREVIOUS gpMetaGlobals->prev_mres
#define META_RESULT_ORIG_RET(type) *(type *)gpMetaGlobals->orig_ret
#define META_RESULT_OVERRIDE_RET(type)   *(type *)gpMetaGlobals->override_ret

typedef int (*GETENTITYAPI_FN) (DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
typedef int (*GETENTITYAPI2_FN) (DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
typedef int (*GETNEWDLLFUNCTIONS_FN) (NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
typedef int (*GET_ENGINE_FUNCTIONS_FN) (enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion);


typedef struct
{
    GETENTITYAPI_FN pfnGetEntityAPI;
    GETENTITYAPI_FN pfnGetEntityAPI_Post;
    GETENTITYAPI2_FN pfnGetEntityAPI2;
    GETENTITYAPI2_FN pfnGetEntityAPI2_Post;
    GETNEWDLLFUNCTIONS_FN pfnGetNewDLLFunctions;
    GETNEWDLLFUNCTIONS_FN pfnGetNewDLLFunctions_Post;
    GET_ENGINE_FUNCTIONS_FN pfnGetEngineFunctions;
    GET_ENGINE_FUNCTIONS_FN pfnGetEngineFunctions_Post;
} metamod_funcs_t;

typedef struct
{
    DLL_FUNCTIONS* dllapi_table;
    NEW_DLL_FUNCTIONS* newapi_table;
} gamedll_funcs_t;



typedef struct
{
    char* ifvers;
    char* name;
    char* version;
    char* date;
    char* author;
    char* url;
    char* logtag;
    PLUG_LOADTIME loadable;
    PLUG_LOADTIME unloadable;
} plugin_info_t;
extern plugin_info_t Plugin_info;

typedef plugin_info_t* plid_t;

#define PLID   &Plugin_info
typedef struct hudtextparms_s
{
    float x;
    float y;
    int effect;
    uint8_t r1, g1, b1, a1;
    uint8_t r2, g2, b2, a2;
    float fadeinTime;
    float fadeoutTime;
    float holdTime;
    float fxTime;
    int channel;
} hudtextparms_t;


// Meta Utility Function table type.
typedef struct meta_util_funcs_s
{
    void (*pfnLogConsole) (plid_t plid, const char* fmt, ...);
    void (*pfnLogMessage) (plid_t plid, const char* fmt, ...);
    void (*pfnLogError) (plid_t plid, const char* fmt, ...);
    void (*pfnLogDeveloper) (plid_t plid, const char* fmt, ...);
    void (*pfnCenterSay) (plid_t plid, const char* fmt, ...);
    void (*pfnCenterSayParms) (plid_t plid, hudtextparms_t tparms, const char* fmt, ...);
    void (*pfnCenterSayVarargs) (plid_t plid, hudtextparms_t tparms, const char* fmt, va_list ap);
    qboolean(*pfnCallGameEntity) (plid_t plid, const char* entStr, entvars_t* pev);
    int (*pfnGetUserMsgID) (plid_t plid, const char* msgname, int* size);
    const char* (*pfnGetUserMsgName) (plid_t plid, int msgid, int* size);
    const char* (*pfnGetPluginPath) (plid_t plid);
    const char* (*pfnGetGameInfo) (plid_t plid, ginfo_t tag);
    int (*pfnLoadPlugin) (plid_t plid, const char* cmdline, PLUG_LOADTIME now, void** plugin_handle);
    int (*pfnUnloadPlugin) (plid_t plid, const char* cmdline, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
    int (*pfnUnloadPluginByHandle) (plid_t plid, void* plugin_handle, PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
    const char* (*pfnIsQueryingClientCvar) (plid_t plid, const edict_t* player);
    int (*pfnMakeRequestID) (plid_t plid);
    void (*pfnGetHookTables) (plid_t plid, enginefuncs_t** peng, DLL_FUNCTIONS** pdll, NEW_DLL_FUNCTIONS** pnewdll);
} mutil_funcs_t;


extern gamedll_funcs_t* gpGamedllFuncs;
extern mutil_funcs_t* gpMetaUtilFuncs;
extern meta_globals_t* gpMetaGlobals;
extern metamod_funcs_t gMetaFunctionTable;

C_DLLEXPORT void Meta_Init(void);
typedef void (*META_INIT_FN) (void);

C_DLLEXPORT int Meta_Query(char* interfaceVersion, plugin_info_t** plinfo, mutil_funcs_t* pMetaUtilFuncs);
typedef int (*META_QUERY_FN) (char* interfaceVersion, plugin_info_t** plinfo, mutil_funcs_t* pMetaUtilFuncs);

C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now, metamod_funcs_t* pFunctionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs);
typedef int (*META_ATTACH_FN) (PLUG_LOADTIME now, metamod_funcs_t* pFunctionTable, meta_globals_t* pMGlobals, gamedll_funcs_t* pGamedllFuncs);

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason);
typedef int (*META_DETACH_FN) (PLUG_LOADTIME now, PL_UNLOAD_REASON reason);

C_DLLEXPORT int GetEntityAPI_Post(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
C_DLLEXPORT int GetEntityAPI2_Post(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);

C_DLLEXPORT int GetNewDLLFunctions_Post(NEW_DLL_FUNCTIONS* pNewFunctionTable, int* interfaceVersion);
C_DLLEXPORT int GetEngineFunctions(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion);
C_DLLEXPORT int GetEngineFunctions_Post(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion);

C_DLLEXPORT bool Amxx_Runebot(void);
typedef bool(*AMXX_RUN_ebot) (void);

C_DLLEXPORT float Amxx_APIVersion(void);
typedef float(*AMXX_API_VERSION) (void);

C_DLLEXPORT void Amxx_Check_amxxdllversion(float version, int bu1, int bu2, int bu3, int bu4);
typedef void(*AMXX_CHECK_APIDLL_VERSION)(float version, int bu1, int bu2, int bu3, int bu4);

C_DLLEXPORT int Amxx_Isebot(int index);
typedef int(*AMXX_IS_ebot) (int index);

C_DLLEXPORT int Amxx_CheckEnemy(int index);
typedef int(*AMXX_CHECK_ENEMY) (int index);

C_DLLEXPORT int Amxx_CheckMoveTarget(int index);
typedef int(*AMXX_CHECK_MOVETARGET) (int index);

C_DLLEXPORT int Amxx_SetEnemy(int index, int target, float blockCheckTime);
typedef int(*AMXX_SET_ENEMY)(int index, int target, float blockCheckTime);

C_DLLEXPORT int Amxx_SetBotMove(int index, int pluginSet);
typedef int(*AMXX_SET_BOT_MOVE) (int index, int pluginSet);

C_DLLEXPORT int Amxx_SetBotLookAt(int index, Vector lookAt);
typedef int(*AMXX_SET_BOT_LOOKAT)(int index, Vector lookAt);

C_DLLEXPORT int Amxx_SetWeaponClip(int index, int weaponClip);
typedef int(*AMXX_SET_WEAPON_CLIP)(int index, int weaponClip);

C_DLLEXPORT int Amxx_BlockWeaponReload(int index, int blockWeaponReload);
typedef int(*AMXX_BLOCK_WEAPON_RELOAD)(int index, int blockWeaponReload);

//C_DLLEXPORT void Amxx_Addebot(const char *name, int skill, int team);
//typedef void(*AMXX_ADD_ebot)(const char *name, int skill, int team);

C_DLLEXPORT int Amxx_SetKADistance(int index, int k1d, int k2d);
typedef int(*AMXX_SET_KA_DISTANCE)(int index, int k1d, int k2d);

C_DLLEXPORT int Amxx_Addebot(const char* name, int skill, int team);
typedef int(*AMXX_ADD_ebot)(const char* name, int skill, int team);

C_DLLEXPORT int Amxx_SetGunADistance(int index, int minDistance, int maxDistance);
typedef int(*AMXX_SET_GUNA_DISTANCE) (int index, int minDistance, int maxDistance);

C_DLLEXPORT int Amxx_IsZombieBot(int index);
typedef int(*AMXX_IS_ZOMBIE_BOT) (int index);

C_DLLEXPORT int Amxx_SetZombieBot(int index, int zombieBot);
typedef int(*_AMXX_SET_ZOMBIE_BOT) (int index, int zombieBot);

C_DLLEXPORT int Amxx_GetOriginPoint(Vector origin);
typedef int(*AMXX_GET_ORIGIN_POINT) (Vector origin);

C_DLLEXPORT int Amxx_GetBotPoint(int index, int mod);
typedef int(*AMXX_GET_BOT_POINT) (int index, int mod);

C_DLLEXPORT int Amxx_GetBotNavNum(int index);
typedef int(*AMXX_GET_BOT_NAV_NUM) (int index);

C_DLLEXPORT int Amxx_GetBotNavPointId(int index, int pointNum);
typedef int(*AMXX_GET_BOT_NAV_POINTID) (int index, int pointNum);

C_DLLEXPORT int Amxx_SetEntityAction(int index, int team, int action);
typedef int(*AMXX_SET_ENTITY_ACTION) (int index, int team, int action);

C_DLLEXPORT void Amxx_AddLog(char* logText);
typedef void(*AMXX_ADD_LOG)(char* logText);

C_DLLEXPORT int Amxx_SetBotGoal(int index, int goal);
typedef int(*_AMXX_SET_BOT_GOAL) (int index, int goal);

C_DLLEXPORT int Amxx_BlockWeaponPick(int index, int blockWeaponPick);
typedef int(*_AMXX_BLOCK_WEAPON_PICK) (int index, int blockWeaponPick);

C_DLLEXPORT int Amxx_GetEntityWaypointId(int index);
typedef int(*_AMXX_GET_ENTITY_WAYPOINT_ID) (int index);

#define MDLL_FUNC   gpGamedllFuncs->dllapi_table

#define MDLL_GameDLLInit            MDLL_FUNC->pfnGameInit
#define MDLL_Spawn                  MDLL_FUNC->pfnSpawn
#define MDLL_Think                  MDLL_FUNC->pfnThink
#define MDLL_Use                  MDLL_FUNC->pfnUse
#define MDLL_Touch                  MDLL_FUNC->pfnTouch
#define MDLL_Blocked               MDLL_FUNC->pfnBlocked
#define MDLL_KeyValue               MDLL_FUNC->pfnKeyValue
#define MDLL_Save                  MDLL_FUNC->pfnSave
#define MDLL_Restore               MDLL_FUNC->pfnRestore
#define MDLL_ObjectCollsionBox         MDLL_FUNC->pfnAbsBox
#define MDLL_SaveWriteFields         MDLL_FUNC->pfnSaveWriteFields
#define MDLL_SaveReadFields            MDLL_FUNC->pfnSaveReadFields
#define MDLL_SaveGlobalState         MDLL_FUNC->pfnSaveGlobalState
#define MDLL_RestoreGlobalState         MDLL_FUNC->pfnRestoreGlobalState
#define MDLL_ResetGlobalState         MDLL_FUNC->pfnResetGlobalState
#define MDLL_ClientConnect            MDLL_FUNC->pfnClientConnect
#define MDLL_ClientDisconnect         MDLL_FUNC->pfnClientDisconnect
#define MDLL_ClientKill               MDLL_FUNC->pfnClientKill
#define MDLL_ClientPutInServer         MDLL_FUNC->pfnClientPutInServer
#define MDLL_ClientCommand            MDLL_FUNC->pfnClientCommand
#define MDLL_ClientUserInfoChanged      MDLL_FUNC->pfnClientUserInfoChanged
#define MDLL_ServerActivate            MDLL_FUNC->pfnServerActivate
#define MDLL_ServerDeactivate         MDLL_FUNC->pfnServerDeactivate
#define MDLL_PlayerPreThink            MDLL_FUNC->pfnPlayerPreThink
#define MDLL_PlayerPostThink         MDLL_FUNC->pfnPlayerPostThink
#define MDLL_StartFrame               MDLL_FUNC->pfnStartFrame
#define MDLL_ParmsNewLevel            MDLL_FUNC->pfnParmsNewLevel
#define MDLL_ParmsChangeLevel         MDLL_FUNC->pfnParmsChangeLevel
#define MDLL_GetGameDescription         MDLL_FUNC->pfnGetGameDescription
#define MDLL_PlayerCustomization      MDLL_FUNC->pfnPlayerCustomization
#define MDLL_SpectatorConnect         MDLL_FUNC->pfnSpectatorConnect
#define MDLL_SpectatorDisconnect      MDLL_FUNC->pfnSpectatorDisconnect
#define MDLL_SpectatorThink            MDLL_FUNC->pfnSpectatorThink
#define MDLL_Sys_Error               MDLL_FUNC->pfnSys_Error
#define MDLL_PM_Move               MDLL_FUNC->pfnPM_Move
#define MDLL_PM_Init               MDLL_FUNC->pfnPM_Init
#define MDLL_PM_FindTextureType         MDLL_FUNC->pfnPM_FindTextureType
#define MDLL_SetupVisibility         MDLL_FUNC->pfnSetupVisibility
#define MDLL_UpdateClientData         MDLL_FUNC->pfnUpdateClientData
#define MDLL_AddToFullPack            MDLL_FUNC->pfnAddToFullPack
#define MDLL_CreateBaseline            MDLL_FUNC->pfnCreateBaseline
#define MDLL_RegisterEncoders         MDLL_FUNC->pfnRegisterEncoders
#define MDLL_GetWeaponData            MDLL_FUNC->pfnGetWeaponData
#define MDLL_CmdStart               MDLL_FUNC->pfnCmdStart
#define MDLL_CmdEnd                  MDLL_FUNC->pfnCmdEnd
#define MDLL_ConnectionlessPacket      MDLL_FUNC->pfnConnectionlessPacket
#define MDLL_GetHullBounds            MDLL_FUNC->pfnGetHullBounds
#define MDLL_CreateInstancedBaselines   MDLL_FUNC->pfnCreateInstancedBaselines
#define MDLL_InconsistentFile         MDLL_FUNC->pfnInconsistentFile
#define MDLL_AllowLagCompensation      MDLL_FUNC->pfnAllowLagCompensation

#define MNEW_FUNC   gpGamedllFuncs->newapi_table

#define MNEW_OnFreeEntPrivateData      MNEW_FUNC->pfnOnFreeEntPrivateData
#define MNEW_GameShutdown            MNEW_FUNC->pfnGameShutdown
#define MNEW_ShouldCollide            MNEW_FUNC->pfnShouldCollide
#define MNEW_CvarValue              MNEW_FUNC->pfnCvarValue
#define MNEW_CvarValue2             MNEW_FUNC->pfnCvarValue2

// convenience macros for metautil functions
#define LOG_CONSOLE         (*gpMetaUtilFuncs->pfnLogConsole)
#define LOG_MESSAGE         (*gpMetaUtilFuncs->pfnLogMessage)
#define LOG_MMERROR         (*gpMetaUtilFuncs->pfnLogError)
#define LOG_DEVELOPER      (*gpMetaUtilFuncs->pfnLogDeveloper)
#define CENTER_SAY         (*gpMetaUtilFuncs->pfnCenterSay)
#define CENTER_SAY_PARMS   (*gpMetaUtilFuncs->pfnCenterSayParms)
#define CENTER_SAY_VARARGS   (*gpMetaUtilFuncs->pfnCenterSayVarargs)
#define CALL_GAME_ENTITY   (*gpMetaUtilFuncs->pfnCallGameEntity)
#define GET_USER_MSG_ID      (*gpMetaUtilFuncs->pfnGetUserMsgID)
#define GET_USER_MSG_NAME   (*gpMetaUtilFuncs->pfnGetUserMsgName)
#define GET_PLUGIN_PATH      (*gpMetaUtilFuncs->pfnGetPluginPath)
#define GET_GAME_INFO      (*gpMetaUtilFuncs->pfnGetGameInfo)
#define LOAD_PLUGIN      (*gpMetaUtilFuncs->pfnLoadPlugin)
#define UNLOAD_PLUGIN      (*gpMetaUtilFuncs->pfnUnloadPlugin)
#define UNLOAD_PLUGIN_BY_HANDLE   (*gpMetaUtilFuncs->pfnUnloadPluginByHandle)
#define IS_QUERYING_CLIENT_CVAR (*gpMetaUtilFuncs->pfnIsQueryingClientCvar)
#define MAKE_REQUESTID      (*gpMetaUtilFuncs->pfnMakeRequestID)
#define GET_HOOK_TABLES    (*gpMetaUtilFuncs->pfnGetHookTables)


// max buffer size for printed messages
//#define MAX_LOGMSG_LEN  1024


void inline UTIL_TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, bool ignoreGlass, edict_t* ignoreEntity, TraceResult* ptr)
{

    (*g_engfuncs.pfnTraceLine) (start, end, (ignoreMonsters ? 1 : 0) | (ignoreGlass ? 0x100 : 0), ignoreEntity, ptr);
}

void inline UTIL_TraceLine(const Vector& start, const Vector& end, bool ignoreMonsters, edict_t* ignoreEntity, TraceResult* ptr)
{

    (*g_engfuncs.pfnTraceLine) (start, end, ignoreMonsters ? 1 : 0, ignoreEntity, ptr);
}

void inline UTIL_TraceHull(const Vector& start, const Vector& end, bool ignoreMonsters, int hullNumber, edict_t* ignoreEntity, TraceResult* ptr)
{
    (*g_engfuncs.pfnTraceHull) (start, end, ignoreMonsters ? 1 : 0, hullNumber, ignoreEntity, ptr);
}

Vector UTIL_VecToAngles(const Vector& vec);

#ifdef _WIN32
#pragma once
#endif

typedef struct
{
    float time;
    float frametime;
    float force_retouch;
    char* mapname;
    char* startspot;
    float deathmatch;
    float coop;
    float teamplay;
    float serverflags;
    float found_secrets;
    vec3_t v_forward;
    vec3_t v_up;
    vec3_t v_right;
    float trace_allsolid;
    float trace_startsolid;
    float trace_fraction;
    vec3_t trace_endpos;
    vec3_t trace_plane_normal;
    float trace_plane_dist;
    edict_t* trace_ent;
    float trace_inopen;
    float trace_inwater;
    int trace_hitgroup;
    int trace_flags;
    int msg_entity;
    int cdAudioTrack;
    int maxClients;
    int maxEntities;
    const char* pStringBase;
    void* pSaveData;
    vec3_t vecLandmarkOffset;
} globalvars_t;



#ifdef DEBUG
#undef DEBUG
#endif

inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float* pOrigin, entvars_t* ent);  // implementation later in this file

extern globalvars_t* g_pGlobals;

#define DLL_GLOBAL

extern DLL_GLOBAL const Vector g_vecZero;

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)       (const char *)(g_pGlobals->pStringBase + (int)offset)
#define MAKE_STRING(str)   ((int)str - (int)STRING(0))

inline edict_t* FIND_ENTITY_BY_CLASSNAME(edict_t* entStart, const char* pszName)
{
    return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}

inline edict_t* FIND_ENTITY_BY_TARGETNAME(edict_t* entStart, const char* pszName)
{
    return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}

// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t* FIND_ENTITY_BY_TARGET(edict_t* entStart, const char* pszName)
{
    return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// Keeps clutter down a bit, when using a float as a bit-Vector
#define SetBits(flBitVector, bits)     ((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits)   ((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit)      ((int)(flBitVector) & (bit))

   // Makes these more explicit, and easier to find
#define FILE_GLOBAL static

   // Until we figure out why "const" gives the compiler problems, we'll just have to use
   // this bogus "empty" define to mark things as constant.
#define CONSTANT

   // More explicit than "int"
typedef int EOFFSET;

// In case it's not alread defined
typedef int BOOL;

// In case this ever changes
#define M_PI 3.1415926

   //
   // Conversion among the three types of "entity", including identity-conversions.
   //
inline edict_t* ENT(const entvars_t* pev)
{
    return pev->pContainingEntity;
}
inline edict_t* ENT(edict_t* pent)
{
    return pent;
}
inline edict_t* ENT(EOFFSET eoffset)
{
    return (*g_engfuncs.pfnPEntityOfEntOffset) (eoffset);
}
inline EOFFSET OFFSET(EOFFSET eoffset)
{
    return eoffset;
}
inline EOFFSET OFFSET(const edict_t* pent)
{
    return (*g_engfuncs.pfnEntOffsetOfPEntity) (pent);
}
inline EOFFSET OFFSET(entvars_t* pev)
{
    return OFFSET(ENT(pev));
}
inline entvars_t* VARS(entvars_t* pev)
{
    return pev;
}

inline entvars_t* VARS(edict_t* pent)
{
    if (!pent)
        return nullptr;

    return &pent->v;
}

inline entvars_t* VARS(EOFFSET eoffset)
{
    return VARS(ENT(eoffset));
}
inline int ENTINDEX(edict_t* pentEdict)
{
    return (*g_engfuncs.pfnIndexOfEdict) (pentEdict);
}
inline edict_t* INDEXENT(int iEdictNum)
{
    return (*g_engfuncs.pfnPEntityOfEntIndex) (iEdictNum);
}
inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float* pOrigin, entvars_t* ent)
{
    (*g_engfuncs.pfnMessageBegin) (msg_dest, msg_type, pOrigin, ENT(ent));
}

// Testing the three types of "entity" for nullity
#define eoNullEntity 0
inline BOOL FNullEnt(EOFFSET eoffset)
{
    return eoffset == 0;
}
inline BOOL FNullEnt(entvars_t* pev)
{
    return pev == nullptr || FNullEnt(OFFSET(pev));
}
inline int FNullEnt(const edict_t* pent)
{
    return !pent || !(*g_engfuncs.pfnEntOffsetOfPEntity) (pent);
}

// Testing strings for nullity
#define iStringNull 0
inline BOOL FStringNull(int iString)
{
    return iString == iStringNull;
}

#define cchMapNameMost 32

#define SAFE_FUNCTION_CALL(pfn,args) try { pfn args; } catch (...)  { }

// Dot products for view cone checking
#define VIEW_FIELD_FULL         (float)-1.0f     // +-180 degrees
#define VIEW_FIELD_WIDE         (float)-0.7     // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define VIEW_FIELD_NARROW       (float)0.7      // +-45 degrees, more narrow check used to set up ranged attacks
#define VIEW_FIELD_ULTRA_NARROW (float)0.9      // +-25 degrees, more narrow check used to set up ranged attacks

   // from ebot.h
extern inline bool IsNullString(const char*);

// Misc useful
inline BOOL FStrEq(const char* sz1, const char* sz2)
{
    if (!sz1 || !sz2)
        return 0;             // safety check

    return (strcmp(sz1, sz2) == 0);
}
inline BOOL FClassnameIs(edict_t* pent, const char* szClassname)
{
    return FStrEq(STRING(VARS(pent)->classname), szClassname);
}
inline BOOL FClassnameIs(entvars_t* pev, const char* szClassname)
{
    return FStrEq(STRING(pev->classname), szClassname);
}

typedef enum
{
    ignore_monsters = 1, dont_ignore_monsters = 0, missile = 2
} IGNORE_MONSTERS;
typedef enum
{
    ignore_glass = 1, dont_ignore_glass = 0
} IGNORE_GLASS;
typedef enum
{
    point_hull = 0, human_hull = 1, large_hull = 2, head_hull = 3
} HULL;



extern Vector GetEntityOrigin(entvars_t* pevBModel);

#define AMBIENT_SOUND_STATIC            0       // medium radius attenuation
#define AMBIENT_SOUND_EVERYWHERE        1
#define AMBIENT_SOUND_SMALLRADIUS       2
#define AMBIENT_SOUND_MEDIUMRADIUS      4
#define AMBIENT_SOUND_LARGERADIUS       8
#define AMBIENT_SOUND_START_SILENT      16
#define AMBIENT_SOUND_NOT_LOOPING       32

#define SPEAKER_START_SILENT            1       // wait for trigger 'on' to start announcements

#define SND_SPAWNING       (1 << 8)     // duplicated in protocol.h we're spawing, used in some cases for ambients
#define SND_STOP           (1 << 5)     // duplicated in protocol.h stop sound
#define SND_CHANGE_VOL     (1 << 6)     // duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH   (1 << 7)     // duplicated in protocol.h change sound pitch

#define LFO_SQUARE         1
#define LFO_TRIANGLE       2
#define LFO_RANDOM         3

// func_rotating
#define SF_BRUSH_ROTATE_Y_AXIS      0
#define SF_BRUSH_ROTATE_INSTANT     1
#define SF_BRUSH_ROTATE_BACKWARDS   2
#define SF_BRUSH_ROTATE_Z_AXIS      4
#define SF_BRUSH_ROTATE_X_AXIS      8
#define SF_PENDULUM_AUTO_RETURN     16
#define SF_PENDULUM_PASSABLE        32

#define SF_BRUSH_ROTATE_SMALLRADIUS  128
#define SF_BRUSH_ROTATE_MEDIUMRADIUS 256
#define SF_BRUSH_ROTATE_LARGERADIUS  512

#define PUSH_BLOCK_ONLY_X    1
#define PUSH_BLOCK_ONLY_Y    2

#define VEC_HULL_MIN         Vector(-16.0f, -16.0f, -36.0f)
#define VEC_HULL_MAX         Vector( 16.0f,  16.0f,  36.0f)
#define VEC_HUMAN_HULL_MIN   Vector( -16.0f, -16.0f, 0.0f )
#define VEC_HUMAN_HULL_MAX   Vector( 16.0f, 16.0f, 72.0f )
#define VEC_HUMAN_HULL_DUCK  Vector( 16.0f, 16.0f, 36.0f )

#define VEC_VIEW             Vector( 0.0f, 0.0f, 2.0f8 )

#define VEC_DUCK_HULL_MIN    Vector(-16.0f, -16.0f, -18.0f )
#define VEC_DUCK_HULL_MAX    Vector( 16.0f,  16.0f,  18.0f )
#define VEC_DUCK_VIEW        Vector( 0.0f, 0.0f, 12.0f )

#define SVC_TEMPENTITY      23
#define SVC_INTERMISSION    30
#define SVC_CDTRACK         32
#define SVC_WEAPONANIM      35
#define SVC_ROOMTYPE        37
#define SVC_DIRECTOR        51

   // triggers
#define SF_TRIGGER_ALLOWMONSTERS    1   // monsters allowed to fire this trigger
#define SF_TRIGGER_NOCLIENTS        2   // players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES        4   // only pushables can fire this trigger

   // func breakable
#define SF_BREAK_TRIGGER_ONLY   1       // may only be broken by trigger
#define SF_BREAK_TOUCH          2       // can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE       4       // can be broken by a player standing on it
#define SF_BREAK_CROWBAR        256     // instant break if hit with crowbar

   // func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE       128

#define SF_LIGHT_START_OFF     1

#define SPAWNFLAG_NOMESSAGE    1
#define SPAWNFLAG_NOTOUCH      1
#define SPAWNFLAG_DROIDONLY    4

#define SPAWNFLAG_USEONLY   1   // can't be touched, must be used (buttons)

#define TELE_PLAYER_ONLY    1
#define TELE_SILENT         2

#define SF_TRIG_PUSH_ONCE   1

   // NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
   // is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
   // down to 1 is a lower pitch.   150 to 70 is the realistic range.
   // EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
   // fast as EMIT_SOUND (the pitchshift mixer is not native coded).

void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch);


inline void EMIT_SOUND(edict_t* entity, int channel, const char* sample, float volume, float attenuation)
{
    EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t* entity, int channel, const char* sample)
{
    EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

// removes linker warning when using msvcrt library
#if defined ( _MSC_VER )
#define stricmp _stricmp
#define unlink _unlink
#define mkdir _mkdir
#endif

   // macro to handle memory allocation fails
#define TerminateOnMalloc() AddLogEntry (LOG_FATAL, "Memory Allocation Fail!\nFile: %s (Line: %d)", __FILE__, __LINE__)
#define InternalAssert(expr) if(!(expr)) { AddLogEntry (LOG_ERROR, "Assertion Fail! (Expression: %s, File: %s, Line: %d)", #expr, __FILE__, __LINE__); }


#define GET_INFOKEYBUFFER   (*g_engfuncs.pfnGetInfoKeyBuffer)
#define INFOKEY_VALUE      (*g_engfuncs.pfnInfoKeyValue)
#define SET_CLIENT_KEYVALUE   (*g_engfuncs.pfnSetClientKeyValue)
#define REG_SVR_COMMAND      (*g_engfuncs.pfnAddServerCommand)
#define SERVER_PRINT      (*g_engfuncs.pfnServerPrint)
#define SET_SERVER_KEYVALUE   (*g_engfuncs.pfnSetKeyValue)

inline char* ENTITY_KEYVALUE(edict_t* entity, char* key)
{
    char* ifbuf = GET_INFOKEYBUFFER(entity);

    return (INFOKEY_VALUE(ifbuf, key));
}

inline void ENTITY_SET_KEYVALUE(edict_t* entity, char* key, char* value)
{
    char* ifbuf = GET_INFOKEYBUFFER(entity);

    SET_CLIENT_KEYVALUE(ENTINDEX(entity), ifbuf, key, value);
}

inline char* SERVERINFO(char* key)
{
    edict_t* server = INDEXENT(0);

    return (ENTITY_KEYVALUE(server, key));
}

inline void SET_SERVERINFO(char* key, char* value)
{
    edict_t* server = INDEXENT(0);
    char* ifbuf = GET_INFOKEYBUFFER(server);

    SET_SERVER_KEYVALUE(ifbuf, key, value);
}

inline char* LOCALINFO(char* key)
{
    edict_t* server = nullptr;
    return (ENTITY_KEYVALUE(server, key));
}

inline void SET_LOCALINFO(char* key, char* value)
{
    edict_t* server = nullptr;
    char* ifbuf = GET_INFOKEYBUFFER(server);
    SET_SERVER_KEYVALUE(ifbuf, key, value);
}

short FixedSigned16(float value, float scale);
unsigned short FixedUnsigned16(float value, float scale);

#undef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT   __declspec(dllexport)
#elif defined(linux)
#define DLLEXPORT               /* */
#define WINAPI                  /* */
#endif /* linux */

#define C_DLLEXPORT      extern "C" DLLEXPORT

#include "stdarg.h"
#include "runtime.h"


#undef GetObject

static inline void MakeVectors(const Vector& in)
{
    in.BuildVectors(&g_pGlobals->v_forward, &g_pGlobals->v_right, &g_pGlobals->v_up);
}

namespace SDK_Utils
{
    inline const char* GetStringFromOffset(int offset)
    {
        return static_cast <const char*> (g_pGlobals->pStringBase + offset);
    }

    inline int MakeStringByOffset(const char* str)
    {
        return reinterpret_cast <int> (str) - reinterpret_cast <int> (GetStringFromOffset(0));
    }
};

//////////////////////////////////////////////////////////////////////////
// evething above that lines, should go away, since it's hl1dsk-only-stuff
// and we targeting to be compatilbe to css, with minimal changes, in future
//////////////////////////////////////////////////////////////////////////

enum VarType
{
    VARTYPE_NORMAL = 0,
    VARTYPE_READONLY,
    VARTYPE_PASSWORD
};

enum GlobalVector
{
    GLOBALVECTOR_FORWARD = 0,
    GLOBALVECTOR_RIGHT,
    GLOBALVECTOR_UP
};

enum TraceIgnore
{
    NO_MONSTERS = (1 << 0),
    NO_GLASS = (1 << 1),
    NO_NOTHING = 0,
    NO_BOTH = NO_MONSTERS | NO_GLASS
};

enum LineType
{
    LINE_SIMPLE,
    LINE_ARROW
};

enum TraceTarget
{
    HULL_HEAD = 3,
    HULL_POINT = 0,
    HULL_IGNORE = -1
};

enum PrintType
{
    PRINT_CONSOLE,
    PRINT_CHAT,
    PRINT_CENTER
};

enum ClientFlag
{
    CLIENT_ALIVE,
    CLIENT_VALID
};

enum ClientTeam
{
    CLIENT_UNASSIGNED = -1,
    CLIENT_TERRORIST = 0,
    CLIENT_COUNTER
};

class ConVar
{
public:
    cvar_t* m_eptr;

public:
    ConVar(const char* name, const char* initval, VarType type = VARTYPE_NORMAL);

    inline bool GetBool(void)
    {
        return m_eptr->value > 0.0f;
    }

    inline int GetInt(void)
    {
        return static_cast <int> (m_eptr->value);
    }

    inline int GetFlags(void)
    {
        return m_eptr->flags;
    }

    inline float GetFloat(void)
    {
        return m_eptr->value;
    }

    inline const char* GetString(void)
    {
        return m_eptr->string;
    }

    inline const char* GetName(void)
    {
        return m_eptr->name;
    }

    inline void SetFloat(float val)
    {
        g_engfuncs.pfnCVarSetFloat(m_eptr->name, val);
    }

    inline void SetInt(int val)
    {
        SetFloat(static_cast <float> (val));
    }

    inline void SetString(const char* val)
    {
        g_engfuncs.pfnCVarSetString(m_eptr->name, val);
    }
};

//
// RNG is taken for JKBotti

class Entity
{
protected:
    edict_t* m_ent;

public:
    inline Entity(void) : m_ent(nullptr)
    {
        // nothing todo
    }

    inline Entity(edict_t* ent) : m_ent(ent)
    {
        // nothing todo
    }

public:
    Entity* operator -> (void)
    {
        return this;
    }

    inline operator edict_t* (void) const
    {
        return m_ent;
    }

    inline Entity& operator = (const Entity& other)
    {
        m_ent = other.m_ent;
        return *this;
    }

    inline Entity& operator = (edict_t* other)
    {
        m_ent = other;
        return *this;
    }

    inline bool operator == (const Entity& other) const
    {
        return IsValid() && m_ent == other.m_ent;
    }

    inline bool operator != (const Entity& other) const
    {
        return m_ent != other.m_ent;
    }

public:
    inline bool IsPlayer(void) const
    {
        return IsValid() && !!(m_ent->v.flags & (FL_FAKECLIENT | FL_CLIENT));
    }

    virtual inline bool IsBot(void) const
    {
        return !!(m_ent->v.flags & FL_FAKECLIENT);
    }

    inline bool IsValid(void) const
    {
        if (m_ent == nullptr || g_engfuncs.pfnEntOffsetOfPEntity(m_ent) == 0 || m_ent->free || (m_ent->v.flags & FL_KILLME))
            return false;

        return true;
    }

    inline virtual bool IsAlive(void) const
    {
        if (!IsValid())
            return false;

        return m_ent->v.deadflag == DEAD_NO && m_ent->v.health > 0 && m_ent->v.movetype != MOVETYPE_NOCLIP;
    }

    inline bool IsRendered(void) const
    {
        return !!(m_ent->v.effects & EF_NODRAW);
    }

    inline String GetClassname(void) const
    {
        return SDK_Utils::GetStringFromOffset(m_ent->v.classname);
    }

    inline String GetModel(void) const
    {
        return IsPlayer() ? g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(m_ent), "model") : SDK_Utils::GetStringFromOffset(m_ent->v.model);
    }

    inline String GetViewModel(void) const
    {
        return SDK_Utils::GetStringFromOffset(m_ent->v.viewmodel);
    }

    inline void SetModel(const String& model) const
    {
        g_engfuncs.pfnSetModel(m_ent, model.GetRawData());
    }

    inline String GetName(void) const
    {
        return SDK_Utils::GetStringFromOffset(m_ent->v.netname);
    }

    inline void SetName(const String& name) const
    {
        m_ent->v.netname = SDK_Utils::MakeStringByOffset(name.GetRawData());
    }

    inline Vector GetHeadOrigin(void) const
    {
        return GetOrigin() + m_ent->v.view_ofs;
    }

    inline void SetOrigin(const Vector& origin) const
    {
        g_engfuncs.pfnSetOrigin(m_ent, origin);
    }

    inline const Vector& GetVelocity(void) const
    {
        return m_ent->v.velocity;
    }

    inline void SetVelocity(const Vector& velocity) const
    {
        m_ent->v.velocity = velocity;
    }

    inline const Vector& GetBodyAngles(void) const
    {
        return m_ent->v.angles;
    }

    inline void SetBodyAngles(const Vector& angles) const
    {
        m_ent->v.angles = angles;
    }

    inline const Vector& GetViewAngles(void) const
    {
        return m_ent->v.v_angle;
    }

    inline void SetViewAngles(const Vector& viewAngles) const
    {
        m_ent->v.v_angle = viewAngles;
    }

    inline const Vector& GetAbsMin(void) const
    {
        return m_ent->v.absmin;
    }

    inline const Vector& GetAbsMax(void) const
    {
        return m_ent->v.absmax;
    }

    inline const Vector& GetMins(void) const
    {
        return m_ent->v.mins;
    }

    inline const Vector& GetMaxs(void) const
    {
        return m_ent->v.maxs;
    }

    inline float GetMaximumSpeed(void) const
    {
        return m_ent->v.maxspeed;
    }

    inline void SetMaximumSpeed(float maxSpeed) const
    {
        m_ent->v.maxspeed = maxSpeed;
    }

    inline Entity GetOwner(void) const
    {
        return m_ent->v.owner;
    }

    inline Entity GetDamageInflictor(void) const
    {
        return m_ent->v.dmg_inflictor;
    }

    inline float GetHealth(void) const
    {
        return m_ent->v.health;
    }

    inline void SetHealth(float health) const
    {
        m_ent->v.health = health;
    }

    inline String GetTarget(void) const
    {
        return SDK_Utils::GetStringFromOffset(m_ent->v.target);
    }

    inline String GetTargetName(void) const
    {
        return SDK_Utils::GetStringFromOffset(m_ent->v.targetname);
    }

    inline bool IsOnLadder(void) const
    {
        return m_ent->v.movetype == MOVETYPE_FLY;
    }

    inline bool IsOnFloor(void) const
    {
        return IsPlayer() ? (!!(m_ent->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))) : g_engfuncs.pfnEntIsOnFloor(m_ent) != 0;
    }

    inline bool IsInWater(void) const
    {
        return m_ent->v.waterlevel >= 2;
    }

    inline Vector GetCenter(void) const
    {
        return (m_ent->v.absmin + m_ent->v.absmax) * 0.5f;
    }

    inline virtual Vector GetOrigin(void) const
    {
        return HasBoundingBox() ? GetCenter() : m_ent->v.rendercolor == nullvec ? m_ent->v.absmin + (m_ent->v.size * 0.5) : m_ent->v.origin;
    }

    inline bool HasBoundingBox(void) const
    {
        return m_ent->v.absmin != nullvec && m_ent->v.absmax != nullvec;
    }

    inline float GetSpeed(void) const
    {
        return m_ent->v.velocity.GetLength();
    }

    inline float GetSpeed2D(void) const
    {
        return m_ent->v.velocity.GetLength2D();
    }

    inline float GetFOV(void) const
    {
        return m_ent->v.fov;
    }

    inline void SetFOV(float fov) const
    {
        m_ent->v.fov = fov;
    }

    inline int GetIndex(void) const
    {
        return g_engfuncs.pfnIndexOfEdict(m_ent);
    }
};

class Client : public Entity
{
private:
    int m_team;
    int m_flags;
    Vector m_safeOrigin;

public:
    Client(void)
    {
        m_flags = CLIENT_UNASSIGNED;
        m_team = CLIENT_UNASSIGNED;

        m_ent = nullptr;
    }

    Client(edict_t* ent)
    {
        m_flags = CLIENT_UNASSIGNED;
        m_team = CLIENT_UNASSIGNED;

        m_ent = ent;
    }

public:
    Entity* operator -> (void)
    {
        return this;
    }

    inline Client& operator = (edict_t* other)
    {
        m_ent = other;
        return *this;
    }


    inline Client& operator = (const Client& other)
    {
        m_ent = other.m_ent;
        return *this;
    }

    inline Client& operator = (const Entity& other)
    {
        m_ent = other;
        return *this;
    }

    inline bool operator == (const Client& other) const
    {
        return IsValid() && m_ent == other.m_ent;
    }

    inline bool operator != (const Client& other) const
    {
        return m_ent != other.m_ent;
    }

    inline bool operator == (const Entity& other) const
    {
        return IsValid(); //&& m_ent == other;
    }

    inline bool operator != (const Entity& other) const
    {
        return true;// m_ent != other;
    }

public:
    inline float GetShootingConeDeviation(const Vector& pos) const;

    inline bool IsInViewCone(const Vector& pos) const;

    inline bool IsVisible(const Vector& pos) const;

    void Print(PrintType printType, const char* format, ...) const
    {
        if (IsBot())
            return;

        char buffer[2048];
        va_list ap;

        va_start(ap, format);
        vsprintf(buffer, format, ap);
        va_end(ap);

        int enginePrintType = 0;

        switch (printType)
        {
        case PRINT_CENTER:
            enginePrintType = 1;
            break;

        case PRINT_CHAT:
            enginePrintType = 2;
            break;

        case PRINT_CONSOLE:
            enginePrintType = 0;
            break;
        }
        strcat(buffer, "\n");

        // print to client
        g_engfuncs.pfnClientPrintf(m_ent, static_cast <PRINT_TYPE> (enginePrintType), buffer);
    }

    inline bool HasFlag(int clientFlags);

    virtual Vector GetOrigin(void) const;
    virtual bool IsAlive(void) const;

    void Maintain(const Entity& ent);
};

// netmessage functions
enum NetMsg
{
    NETMSG_UNDEFINED = -1,
    NETMSG_VGUI = 1,
    NETMSG_SHOWMENU = 2,
    NETMSG_WLIST = 3,
    NETMSG_CURWEAPON = 4,
    NETMSG_AMMOX = 5,
    NETMSG_AMMOPICK = 6,
    NETMSG_DAMAGE = 7,
    NETMSG_MONEY = 8,
    NETMSG_STATUSICON = 9,
    NETMSG_DEATH = 10,
    NETMSG_SCREENFADE = 11,
    NETMSG_HLTV = 12,
    NETMSG_TEXTMSG = 13,
    NETMSG_SCOREINFO = 14,
    NETMSG_BARTIME = 15,
    NETMSG_SENDAUDIO = 17,
    NETMSG_SAYTEXT = 18,
    NETMSG_BOTVOICE = 19,
    NETMSG_NUM = 21
};

struct MessageBlock
{
    int bot;
    int state;
    int msg;
    int regMsgs[NETMSG_NUM];
};

class Engine : public Singleton <Engine>
{
    friend class Client;

    // bot client command
    bool m_isBotCommand;
    char m_arguments[256];
    int m_argumentCount;
    MessageBlock m_msgBlock;

private:
    Client m_clients[32];

private:
    uint32_t m_rnd[2];
    double m_divider;

private:
    enum GameVars
    {
        GVAR_C4TIMER = 0,
        GVAR_BUYTIME,
        GVAR_FRIENDLYFIRE,
        GVAR_ROUNDTIME,
        GVAR_FREEZETIME,
        GVAR_FOOTSTEPS,
        GVAR_GRAVITY,
        GVAR_DEVELOPER,

        GVAR_NUM
    };

private:
    const static int MAX_BOTVARS = 100;

    struct VarPair
    {
        cvar_t reg;
        class ConVar* self;
    } m_regVars[MAX_BOTVARS];

    int m_regCount;
    cvar_t* m_gameVars[GVAR_NUM];

public:
    Engine* operator -> (void)
    {
        return this;
    }

public:
    float ApproachAngle(float target, float value, float speed);

    float AngleDiff(float destAngle, float srcAngle);

    float DoClamp(float a, float b, float c);

    // sends bot command
    void IssueBotCommand(edict_t* ent, const char* fmt, ...);

    // generates random float based on low and high value
    float RandomFloat(float low, float high);

    // generates random integer based on low and high value
    int RandomInt(int low, int high);

    // pushes global convar to list that will be registered by engine
    void RegisterVariable(const char* variable, const char* value, VarType varType, ConVar* self);

    // register previously pushed convars to the engine registration
    void PushRegisteredConVarsToEngine(void);

    // get the pointers of game cvars
    void GetGameConVarsPointers(void);

    const Vector& GetGlobalVector(GlobalVector id);

    void SetGlobalVector(GlobalVector id, const Vector& newVector);

    void BuildGlobalVectors(const Vector& on);

    bool IsFriendlyFireOn(void);

    bool IsFootstepsOn(void);

    float GetC4TimerTime(void);

    float GetBuyTime(void);

    float GetRoundTime(void);

    float GetFreezeTime(void);

    int GetGravity(void);

    int GetDeveloperLevel(void);

    void PrintServer(const char* format, ...);

    float GetTime(void);

    int GetMaxClients(void);

    void PrintAllClients(PrintType printType, const char* format, ...);

    const Entity& GetEntityByIndex(int index);

    const Client& GetClientByIndex(int index);

    void MaintainClients(void);

    void DrawLine(const Client& client, const Vector& start, const Vector& end, const Color& color, int width, int noise, int speed, int life, int lineType = LINE_SIMPLE);

    // find registered message id
    FORCEINLINE int FindMessageId(int type)
    {
        return m_msgBlock.regMsgs[type];
    }
};

#define engine Engine::GetReference ()

class Tracer
{
private:
    Vector m_hitEndPos;
    Vector m_planeNormal;

    float m_fraction;
    Entity m_hit;

    Vector m_start;
    Vector m_end;

    bool m_monsters;
    bool m_glass;
    bool m_solid;
    bool m_allSolid;

    Entity m_ignore;
    int m_hullNumber;

public:
    Tracer(const Vector& start, const Vector& end, int ignoreFlags, const Entity& ignore, int hull = -1, bool run = false)
    {
        // @DEPRECATEME@
        SetParameters(start, end, ignoreFlags, ignore, hull);

        if (run)
            Fire();
    }

    Tracer(void)
    {
    }

    inline float Fire(void)
    {
        TraceResult tr;

        if (m_hullNumber != -1)
            g_engfuncs.pfnTraceHull(m_start, m_end, m_monsters ? 1 : 0, m_hullNumber, m_ignore ? m_ignore : nullptr, &tr);
        else
            g_engfuncs.pfnTraceLine(m_start, m_end, m_monsters ? 1 : 0 || m_glass ? 0x100 : 0, m_ignore ? m_ignore : nullptr, &tr);

        m_fraction = tr.flFraction;
        m_planeNormal = tr.vecPlaneNormal;
        m_end = tr.vecEndPos;
        m_hit = tr.pHit;
        m_solid = tr.fStartSolid > 0;
        m_allSolid = tr.fAllSolid > 0;

        return m_fraction;
    }

    inline Tracer* SetParameters(const Vector& start, const Vector& end, int ignoreFlags, const Entity& ignore, int hull = -1)
    {
        m_start = start;
        m_end = end;
        m_ignore = ignore;
        m_hullNumber = hull;
        m_glass = false;
        m_monsters = false;
        m_solid = false;
        m_allSolid = false;

        if (ignoreFlags & NO_GLASS)
            m_glass = true;

        if (ignoreFlags & NO_MONSTERS)
            m_monsters = true;

        return this;
    }

    inline bool IsStartSolid(void)
    {
        return m_solid;
    }

    inline bool IsAllSolid(void)
    {
        return m_allSolid;
    }

    inline const Vector& GetHitEndPos(void)
    {
        return m_hitEndPos;
    }

    inline const Vector& GetPlaneNormal(void)
    {
        return m_planeNormal;
    }

    inline const Entity& GetHit(void)
    {
        return m_hit;
    }

    inline bool HasHitEntity(void)
    {
        return m_hit.IsValid();
    }

    inline bool CheckHitClassname(const String& other)
    {
        if (m_hit.GetClassname().Find(other) != -1)
            return true;

        return false;
    }

    inline String GetClassname(void)
    {
        return m_hit.GetClassname();
    }

    inline float GetFraction(void)
    {
        return m_fraction;
    }
};

#endif
