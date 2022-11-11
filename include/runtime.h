//
//  Copyright (C) 2009-2010 Dmitry Zhukov. All rights reserved.
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
// $Id$
//

#ifndef __RUNTIME_INCLUDED__
#define __RUNTIME_INCLUDED__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <stdarg.h>

#pragma warning (disable : 4996) // get rid of this

//
// Type: uint32_t
// Unsigned 32bit integer.
//
typedef unsigned int uint32_t;

//
// Type: uint64_t
// Unsigned 64bit integer.
typedef unsigned long long uint64_t;

//
// Type: uint8_t
// Unsigned byte.
//
typedef unsigned char uint8_t;

//
// Type: int16_t
// Signed byte.
//
typedef signed short int16_t;

//
// Type: uint16_t
// Unsigned shot.
//
typedef unsigned short uint16_t;


//
// Macro: nullvec
//
// This macro is a null vector.
//
#define nullvec Vector::GetNull ()

//
// Macro: InternalAssert
//
// Asserts expression.
//
#define Assert(expr)

//
// Function: FormatBuffer
// 
// Formats a buffer using variable arguments.
//
// Parameters:
//   format - String to format.
//   ... - List of format arguments.
//
// Returns:
//   Formatted buffer.
//
inline char* FormatBuffer(char* format, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);

    return &buffer[0];
}

//
// Enum: LogMask
//
// LM_ERROR - Log's errors to a file.
// LM_ASSERT - Log's asserts to a file.
// LM_WARNING - Log's warnings to a file.
// LM_DEFAULT - Log's generic messages.
// LM_NOTICE - Log's notices messages. (off by default).
// LM_CRITICAL - Isn't really a bitmask, just throws critical error.
// LM_CONSOLE - Output the error to console.
//
// See Also:
//   <Logger>
//
enum LogMask
{
    LM_ERROR = (1 << 0),
    LM_ASSERT = (1 << 1),
    LM_WARNING = (1 << 2),
    LM_DEFAULT = (1 << 3),
    LM_NOTICE = (1 << 4),
    LM_CRITICAL = (1 << 5),
    LM_CONSOLE = (1 << 6)
};

//
// Class: Singleton
//  Implements no-copying singleton.
//
template <typename T> class Singleton
{
    //
    // Group: (Con/De)structors
    //
protected:

    //
    // Function: Singleton
    //  Default singleton constructor.
    //
    Singleton(void) { }

    //
    // Function: ~Singleton
    //  Default singleton destructor.
    //
    virtual ~Singleton(void) { }


public:

    //
    // Function: GetObject
    //  Gets the object of singleton.
    //
    // Returns:
    //  Object pointer.
    //  
    //
    static inline T* GetObjectPtr(void)
    {
        static T reference;
        return &reference;
    }

    //
    // Function: GetObject
    //  Gets the object of singleton as reference.
    //
    // Returns:
    //  Object reference.
    //  
    //
    static inline T& GetReference(void)
    {
        static T reference;
        return reference;
    }
};

//
// Namespace: Math
// Utility mathematical functions.
//
namespace Math
{
    const float MATH_ONEPSILON = 0.01f;
    const float MATH_EQEPSILON = 0.001f;
    const float MATH_FLEPSILON = 1.192092896e-07f;

    //
    // Constant: MATH_PI
    // Mathematical PI value.
    //
    const float MATH_PI = 3.1415926f;

    const float MATH_D2R = MATH_PI / 180.0f;
    const float MATH_R2D = 180.0f / MATH_PI;

    //
    // Function: FltZero
    // 
    // Checks whether input entry float is zero.
    //
    // Parameters:
    //   entry - Input float.
    //
    // Returns:
    //   True if float is zero, false otherwise.
    //
    // See Also:
    //   <FltEqual>
    //
    // Remarks:
    //   This eliminates Intel C++ Compiler's warning about float equality/inquality.
    //
    inline bool FltZero(float entry)
    {
        return fabsf(entry) < MATH_ONEPSILON;
    }

    //
    // Function: FltEqual
    // 
    // Checks whether input floats are equal.
    //
    // Parameters:
    //   entry1 - First entry float.
    //   entry2 - Second entry float.
    //
    // Returns:
    //   True if floats are equal, false otherwise.
    //
    // See Also:
    //   <FltZero>
    //
    // Remarks:
    //   This eliminates Intel C++ Compiler's warning about float equality/inquality.
    //
    inline bool FltEqual(float entry1, float entry2)
    {
        return fabsf(entry1 - entry2) < MATH_EQEPSILON;
    }

    //
    // Function: RadianToDegree
    // 
    //  Converts radians to degrees.
    //
    // Parameters:
    //   radian - Input radian.
    //
    // Returns:
    //   Degree converted from radian.
    //
    // See Also:
    //   <DegreeToRadian>
    //
    inline float RadianToDegree(float radian)
    {
        return radian * MATH_R2D;
    }

    //
    // Function: DegreeToRadian
    // 
    // Converts degrees to radians.
    //
    // Parameters:
    //   degree - Input degree.
    //
    // Returns:
    //   Radian converted from degree.
    //
    // See Also:
    //   <RadianToDegree>
    //
    inline float DegreeToRadian(float degree)
    {
        return degree * MATH_D2R;
    }

    //
    // Function: AngleMod
    //
    // Adds or subtracts 360.0f enough times need to given angle in order to set it into the range [0.0, 360.0f).
    //
    // Parameters:
    //	  angle - Input angle.
    //
    // Returns:
    //   Resulting angle.
    //
    inline float AngleMod(float angle)
    {
        return 360.0f / 65536.0f * (static_cast <int> (angle * (65536.0f / 360.0f)) & 65535);
    }

    //
    // Function: AngleNormalize
    //
    // Adds or subtracts 360.0f enough times need to given angle in order to set it into the range [-180.0, 180.0f).
    //
    // Parameters:
    //	  angle - Input angle.
    //
    // Returns:
    //   Resulting angle.
    //
    inline float AngleNormalize(float angle)
    {
        return 360.0f / 65536.0f * (static_cast <int> ((angle + 180.0f) * (65536.0f / 360.0f)) & 65535) - 180.0f;
    }

    //
    // Function: SineCosine
    //
    // Very fast platform-dependent sine and cosine calculation routine.
    //
    // Parameters:
    //	  radians - Input degree (angle).
    //	  sine - Output for Sine.
    //	  cosine - Output for Cosine.
    //
    void inline SineCosine(float radians, float& sine, float& cosine)
    {
#if defined (PLATFORM_WIN32)
        __asm
        {
            fld uint32_t ptr[radians]
            fsincos

            mov edx, uint32_t ptr[cosine]
            mov eax, uint32_t ptr[sine]

            fstp uint32_t ptr[edx]
            fstp uint32_t ptr[eax]
        }
#else
        sine = sinf(radians);
        cosine = cosf(radians);
#endif
    }
}

//
// Class: Vector
// Standard 3-dimensional vector.
//
class Vector
{
    //
    // Group: Variables.
    //
public:
    //
    // Variable: x,y,z
    // X, Y and Z axis members.
    //
    float x, y, z;

    //
    // Group: (Con/De)structors.
    //
public:
    //
    // Function: Vector
    //
    // Constructs Vector from float, and assign it's value to all axises.
    //
    // Parameters:
    //	  scaler - Value for axises.
    //
    inline Vector(float scaler = 0.0f) : x(scaler), y(scaler), z(scaler)
    {
    }

    //
    // Function: Vector
    //
    // Standard Vector Constructor.
    //
    // Parameters:
    //	  inputX - Input X axis.
    //	  inputY - Input Y axis.
    //	  inputZ - Input Z axis.
    //
    inline Vector(float inputX, float inputY, float inputZ) : x(inputX), y(inputY), z(inputZ)
    {
    }

    //
    // Function: Vector
    //
    // Constructs Vector from float pointer.
    //
    // Parameters:
    //	  other - Float pointer.
    //
    inline Vector(float* other) : x(other[0]), y(other[1]), z(other[2])
    {
    }

    //
    // Function: Vector
    //
    // Constructs Vector from another Vector.
    //
    // Parameters:
    //   right - Other Vector, that should be assigned.
    //
    inline Vector(const Vector& right) : x(right.x), y(right.y), z(right.z)
    {
    }
    //
    // Group: Operators.
    //
public:
    inline operator float* (void)
    {
        return &x;
    }

    inline operator const float* (void) const
    {
        return &x;
    }

    inline float& operator [] (int index)
    {
        return (&x)[index];
    }

    inline const float& operator [] (int index) const
    {
        return (&x)[index];
    }

    inline const Vector operator + (const Vector& right) const
    {
        return Vector(x + right.x, y + right.y, z + right.z);
    }

    inline const Vector operator - (const Vector& right) const
    {
        return Vector(x - right.x, y - right.y, z - right.z);
    }

    inline const Vector operator - (void) const
    {
        return Vector(-x, -y, -z);
    }

    friend inline const Vector operator * (const float vec, const Vector& right)
    {
        return Vector(right.x * vec, right.y * vec, right.z * vec);
    }

    inline const Vector operator * (float vec) const
    {
        return Vector(vec * x, vec * y, vec * z);
    }

    inline const Vector operator / (float vec) const
    {
        const float inv = 1 / vec;
        return Vector(inv * x, inv * y, inv * z);
    }

    inline const Vector operator ^ (const Vector& right) const
    {
        return Vector(y * right.z - z * right.y, z * right.x - x * right.z, x * right.y - y * right.x);
    }

    inline float operator | (const Vector& right) const
    {
        return x * right.x + y * right.y + z * right.z;
    }

    inline const Vector& operator += (const Vector& right)
    {
        x += right.x;
        y += right.y;
        z += right.z;

        return *this;
    }

    inline const Vector& operator -= (const Vector& right)
    {
        x -= right.x;
        y -= right.y;
        z -= right.z;

        return *this;
    }

    inline const Vector& operator *= (float vec)
    {
        x *= vec;
        y *= vec;
        z *= vec;

        return *this;
    }

    inline const Vector& operator /= (float vec)
    {
        const float inv = 1 / vec;

        x *= inv;
        y *= inv;
        z *= inv;

        return *this;
    }

    inline bool operator == (const Vector& right) const
    {
        return Math::FltEqual(x, right.x) && Math::FltEqual(y, right.y) && Math::FltEqual(z, right.z);
    }

    inline bool operator != (const Vector& right) const
    {
        return !Math::FltEqual(x, right.x) && !Math::FltEqual(y, right.y) && !Math::FltEqual(z, right.z);
    }

    inline const Vector& operator = (const Vector& right)
    {
        x = right.x;
        y = right.y;
        z = right.z;

        return *this;
    }
    //
    // Group: Functions.
    //
public:

    //
    // Function: GetLength
    //
    // Gets length (magnitude) of 3D vector.
    //
    // Returns:
    //   Length (magnitude) of the 3D vector.
    //
    // See Also:
    //   <GetLengthSquared>
    //
    inline float GetLength(void) const
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

    //
    // Function: GetLength2D
    //
    // Gets length (magnitude) of vector ignoring Z axis.
    //
    // Returns:
    //   2D length (magnitude) of the vector.
    //
    // See Also:
    //   <GetLengthSquared2D>
    //
    inline float GetLength2D(void) const
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

    //
    // Function: GetLengthSquared
    //
    // Gets squared length (magnitude) of 3D vector.
    //
    // Returns:
    //   Squared length (magnitude) of the 3D vector.
    //
    // See Also:
    //   <GetLength>
    //
    inline float GetLengthSquared(void) const
    {
        return x * x + y * y + z * z;
    }

    //
    // Function: GetLengthSquared2D
    //
    // Gets squared length (magnitude) of vector ignoring Z axis.
    //
    // Returns:
    //   2D squared length (magnitude) of the vector.
    //
    // See Also:
    //   <GetLength2D>
    //
    inline float GetLengthSquared2D(void) const
    {
        return x * x + y * y;
    }

    //
    // Function: SkipZ
    //
    // Gets vector without Z axis.
    //
    // Returns:
    //   2D vector from 3D vector.
    //
    inline Vector SkipZ(void) const
    {
        return Vector(x, y, 0.0f);
    }

    //
    // Function: Normalize
    //
    // Normalizes a vector.
    //
    // Returns:
    //   The previous length of the 3D vector.
    //
    inline Vector Normalize(void) const
    {
        float length = GetLength() + static_cast <float> (Math::MATH_FLEPSILON);

        if (Math::FltZero(length))
            return Vector(0, 0, 1.0f);

        length = 1.0f / length;

        return Vector(x * length, y * length, z * length);
    }

    //
    // Function: Normalize
    //
    // Normalizes a 2D vector.
    //
    // Returns:
    //   The previous length of the 2D vector.
    //
    inline Vector Normalize2D(void) const
    {
        float length = GetLength2D() + static_cast <float> (Math::MATH_FLEPSILON);

        if (Math::FltZero(length))
            return Vector(0, 1.0, 0);

        length = 1.0f / length;

        return Vector(x * length, y * length, 0.0f);
    }

    //
    // Function: IsNull
    //
    // Checks whether vector is null.
    //
    // Returns:
    //   True if this vector is (0.0f, 0.0f, 0.0f) within tolerance, false otherwise.
    //
    inline bool IsNull(void) const
    {
        return Math::FltZero(x) && Math::FltZero(y) && Math::FltZero(z);
    }

    //
    // Function: GetNull
    //
    // Gets a nulled vector.
    //
    // Returns:
    //   Nulled vector.
    //
    inline static const Vector& GetNull(void)
    {
        static const Vector& s_null = Vector(0.0, 0.0, 0.0f);
        return s_null;
    }

    //
    // Function: ClampAngles
    //
    // Clamps the angles (ignore Z component).
    //
    // Returns:
    //   3D vector with clamped angles (ignore Z component).
    //
    inline Vector ClampAngles(void)
    {
        x = Math::AngleNormalize(x);
        y = Math::AngleNormalize(y);
        z = 0.0f;

        return *this;
    }

    //
    // Function: ToPitch
    //
    // Converts a spatial location determined by the vector passed into an absolute X angle (pitch) from the origin of the world.
    //
    // Returns:
    //   Pitch angle.
    //
    inline float ToPitch(void) const
    {
        if (Math::FltZero(x) && Math::FltZero(y))
            return 0.0f;

        return Math::RadianToDegree(atan2f(z, GetLength2D()));
    }

    //
    // Function: ToYaw
    //
    // Converts a spatial location determined by the vector passed into an absolute Y angle (yaw) from the origin of the world.
    //
    // Returns:
    //   Yaw angle.
    //
    inline float ToYaw(void) const
    {
        if (Math::FltZero(x) && Math::FltZero(y))
            return 0.0f;

        return Math::RadianToDegree(atan2f(y, x));
    }

    //
    // Function: ToAngles
    //
    // Convert a spatial location determined by the vector passed in into constant absolute angles from the origin of the world.
    //
    // Returns:
    //   Converted from vector, constant angles.
    //
    inline Vector ToAngles(void) const
    {
        // is the input vector absolutely vertical?
        if (Math::FltZero(x) && Math::FltZero(y))
            return Vector(z > 0.0f ? 90.0f : 270.0f, 0.0, 0.0f);

        // else it's another sort of vector compute individually the pitch and yaw corresponding to this vector.
        return Vector(Math::RadianToDegree(atan2f(z, GetLength2D())), Math::RadianToDegree(atan2f(y, x)), 0.0f);
    }

    //
    // Function: BuildVectors
    // 
    //	Builds a 3D referential from a view angle, that is to say, the relative "forward", "right" and "upward" direction 
    // that a player would have if he were facing this view angle. World angles are stored in Vector structs too, the 
    // "x" component corresponding to the X angle (horizontal angle), and the "y" component corresponding to the Y angle 
    // (vertical angle).
    //
    // Parameters:
    //   forward - Forward referential vector.
    //   right - Right referential vector.
    //   upward - Upward referential vector.
    //
    inline void BuildVectors(Vector* forward, Vector* right, Vector* upward) const
    {
        float sinePitch = 0.0f, cosinePitch = 0.0f, sineYaw = 0.0f, cosineYaw = 0.0f, sineRoll = 0.0f, cosineRoll = 0.0f;

        Math::SineCosine(Math::DegreeToRadian(x), sinePitch, cosinePitch); // compute the sine and cosine of the pitch component
        Math::SineCosine(Math::DegreeToRadian(y), sineYaw, cosineYaw); // compute the sine and cosine of the yaw component
        Math::SineCosine(Math::DegreeToRadian(z), sineRoll, cosineRoll); // compute the sine and cosine of the roll component

        if (forward != nullptr)
        {
            forward->x = cosinePitch * cosineYaw;
            forward->y = cosinePitch * sineYaw;
            forward->z = -sinePitch;
        }

        if (right != nullptr)
        {
            right->x = -sineRoll * sinePitch * cosineYaw + cosineRoll * sineYaw;
            right->y = -sineRoll * sinePitch * sineYaw - cosineRoll * cosineYaw;
            right->z = -sineRoll * cosinePitch;
        }

        if (upward != nullptr)
        {
            upward->x = cosineRoll * sinePitch * cosineYaw + sineRoll * sineYaw;
            upward->y = cosineRoll * sinePitch * sineYaw - sineRoll * cosineYaw;
            upward->z = cosineRoll * cosinePitch;
        }
    }
};

namespace Math
{
    inline bool BBoxIntersects(const Vector& min1, const Vector& max1, const Vector& min2, const Vector& max2)
    {
        return min1.x < max2.x&& max1.x > min2.x && min1.y < max2.y&& max1.y > min2.y && min1.z < max2.z&& max1.z > min2.z;
    }
}

//
// Class: Array
//  Universal template array container.
//
template <typename T> class Array
{
private:
    T* m_elements;

    int m_resizeStep;
    int m_itemSize;
    int m_itemCount;

    //
    // Group: (Con/De)structors
    //
public:

    //
    // Function: Array
    //  Default array constructor.
    //
    // Parameters:
    //  resizeStep - Array resize step, when new items added, or old deleted.
    //
    Array(int resizeStep = 0)
    {
        m_elements = nullptr;
        m_itemSize = 0;
        m_itemCount = 0;
        m_resizeStep = resizeStep;
    }

    //
    // Function: Array
    //  Array copying constructor.
    //
    // Parameters:
    //  other - Other array that should be assigned to this one.
    //
    Array(const Array <T>& other)
    {
        m_elements = nullptr;
        m_itemSize = 0;
        m_itemCount = 0;
        m_resizeStep = 0;

        AssignFrom(other);
    }

    //
    // Function: ~Array
    //  Default array destructor.
    //
    virtual ~Array(void)
    {
        Destory();
    }

    //
    // Group: Functions
    //
public:

    //
    // Function: Destory
    //  Destroys array object, and all elements.
    //
    void Destory(void)
    {
        delete[] m_elements;
        m_elements = nullptr;
        m_itemSize = 0;
        m_itemCount = 0;
    }

    //
    // Function: SetSize
    //  Sets the size of the array.
    //
    // Parameters:
    //  newSize - Size to what array should be resized.
    //  keepData - Keep exiting data, while resizing array or not.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool SetSize(int newSize, bool keepData = true)
    {
        if (newSize == 0)
        {
            Destory();
            return true;
        }

        int checkSize = 0;

        if (m_resizeStep != 0)
            checkSize = m_itemCount + m_resizeStep;
        else
        {
            checkSize = m_itemCount / 8;

            if (checkSize < 4)
                checkSize = 4;
            else if (checkSize > 1024)
                checkSize = 1024;

            checkSize += m_itemCount;
        }

        if (newSize > checkSize)
            checkSize = newSize;

        T* buffer = new T[checkSize];

        if (keepData && m_elements != nullptr)
        {
            if (checkSize < m_itemCount)
                m_itemCount = checkSize;

            for (int i = 0; i < m_itemCount; i++)
                buffer[i] = m_elements[i];
        }

        delete[] m_elements;

        m_elements = buffer;
        m_itemSize = checkSize;

        return true;
    }

    //
    // Function: GetSize
    //  Gets allocated size of array.
    //
    // Returns:
    //  Number of allocated items.
    //
    int GetSize(void) const
    {
        return m_itemSize;
    }

    //
    // Function: GetElementNumber
    //  Gets real number currently in array.
    //
    // Returns:
    //  Number of elements.
    //
    int GetElementNumber(void) const
    {
        return m_itemCount;
    }

    //
    // Function: SetEnlargeStep
    //  Sets step, which used while resizing array data.
    //
    // Parameters:
    //  resizeStep - Step that should be set.
    //  
    void SetEnlargeStep(int resizeStep = 0)
    {
        m_resizeStep = resizeStep;
    }

    //
    // Function: GetEnlargeStep
    //  Gets the current enlarge step.
    //
    // Returns:
    //  Current resize step.
    //
    int GetEnlargeStep(void)
    {
        return m_resizeStep;
    }

    //
    // Function: SetAt
    //  Sets element data, at specified index.
    //
    // Parameters:
    //  index - Index where object should be assigned.
    //  object - Object that should be assigned.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool SetAt(int index, T object, bool enlarge = true)
    {
        if (index >= m_itemSize)
        {
            if (!enlarge || !SetSize(index + 1))
                return false;
        }

        m_elements[index] = object;

        if (index >= m_itemCount)
            m_itemCount = index + 1;

        return true;
    }

    //
    // Function: GetAt
    //  Gets element from specified index
    //
    // Parameters:
    //  index - Element index to retrieve.
    //
    // Returns:
    //  Element object.
    //
    T& GetAt(int index)
    {
        return m_elements[index];
    }

    //
    // Function: GetAt
    //  Gets element at specified index, and store it in reference object.
    //
    // Parameters:
    //  index - Element index to retrieve.
    //  object - Holder for element reference.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool GetAt(int index, T& object)
    {
        if (index >= m_itemCount)
            return false;

        object = m_elements[index];
        return true;
    }

    //
    // Function: InsertAt
    //  Inserts new element at specified index.
    //
    // Parameters:
    //  index - Index where element should be inserted.
    //  object - Object that should be inserted.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool InsertAt(int index, T object, bool enlarge = true)
    {
        return InsertAt(index, &object, 1, enlarge);
    }

    //
    // Function: InsertAt
    //  Inserts number of element at specified index.
    //
    // Parameters:
    //  index - Index where element should be inserted.
    //  objects - Pointer to object list.
    //  count - Number of element to insert.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool InsertAt(int index, T* objects, int count = 1, bool enlarge = true)
    {
        if (objects == nullptr || count < 1)
            return false;

        int newSize = 0;

        if (m_itemCount > index)
            newSize = m_itemCount + count;
        else
            newSize = index + count;

        if (newSize >= m_itemSize)
        {
            if (!enlarge || !SetSize(newSize))
                return false;
        }

        if (index >= m_itemCount)
        {
            for (int i = 0; i < count; i++)
                m_elements[i + index] = objects[i];

            m_itemCount = newSize;
        }
        else
        {
            int i = 0;

            for (i = m_itemCount; i > index; i--)
                m_elements[i + count - 1] = m_elements[i - 1];

            for (i = 0; i < count; i++)
                m_elements[i + index] = objects[i];

            m_itemCount += count;
        }
        return true;
    }

    //
    // Function: InsertAt
    //  Inserts other array reference into the our array.
    //
    // Parameters:
    //  index - Index where element should be inserted.
    //  objects - Pointer to object list.
    //  count - Number of element to insert.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool InsertAt(int index, Array <T>& other, bool enlarge = true)
    {
        if (&other == this)
            return false;

        return InsertAt(index, other.m_elements, other.m_itemCount, enlarge);
    }

    //
    // Function: RemoveAt
    //  Removes elements from specified index.
    //
    // Parameters:
    //  index - Index, where element should be removed.
    //  count - Number of elements to remove.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool RemoveAt(int index, int count = 1)
    {
        if (index + count > m_itemCount)
            return false;

        if (count < 1)
            return true;

        m_itemCount -= count;

        for (int i = index; i < m_itemCount; i++)
            m_elements[i] = m_elements[i + count];

        return true;
    }

    //
    // Function: Push
    //  Appends element to the end of array.
    //
    // Parameters:
    //  object - Object to append.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool Push(T object, bool enlarge = true)
    {
        return InsertAt(m_itemCount, &object, 1, enlarge);
    }

    //
    // Function: Push
    //  Appends number of elements to the end of array.
    //
    // Parameters:
    //  objects - Pointer to object list.
    //  count - Number of element to insert.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool Push(T* objects, int count = 1, bool enlarge = true)
    {
        return InsertAt(m_itemCount, objects, count, enlarge);
    }

    //
    // Function: Push
    //  Inserts other array reference into the our array.
    //
    // Parameters:
    //  objects - Pointer to object list.
    //  count - Number of element to insert.
    //  enlarge - Checks whether array must be resized in case, allocated size + enlarge step is exceeded.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool Push(Array <T>& other, bool enlarge = true)
    {
        if (&other == this)
            return false;

        return InsertAt(m_itemCount, other.m_elements, other.m_itemCount, enlarge);
    }

    //
    // Function: GetData
    //  Gets the pointer to all element in array.
    //
    // Returns:
    //  Pointer to object list.
    //
    T* GetData(void)
    {
        return m_elements;
    }

    //
    // Function: RemoveAll
    //  Resets array, and removes all elements out of it.
    // 
    void RemoveAll(void)
    {
        m_itemCount = 0;
        SetSize(m_itemCount);
    }

    //
    // Function: IsEmpty
    //  Checks whether element is empty.
    //
    // Returns:
    //  True if element is empty, false otherwise.
    //
    inline bool IsEmpty(void)
    {
        return m_itemCount <= 0;
    }

    //
    // Function: FreeExtra
    //  Frees unused space.
    //
    void FreeSpace(bool destroyIfEmpty = true)
    {
        if (m_itemCount == 0)
        {
            if (destroyIfEmpty)
                Destory();

            return;
        }

        T* buffer = new T[m_itemCount];

        if (m_elements != nullptr)
        {
            for (int i = 0; i < m_itemCount; i++)
                buffer[i] = m_elements[i];
        }

        delete[] m_elements;

        m_elements = buffer;
        m_itemSize = m_itemCount;
    }

    //
    // Function: Pop
    //  Pops element from array.
    //
    // Returns:
    //  Object popped from the end of array.
    //
    T Pop(void)
    {
        T element = m_elements[m_itemCount - 1];
        RemoveAt(m_itemCount - 1);

        return element;
    }

    //
    // Function: PopNoReturn
    //  Pops element from array without returning last element.
    //
    T PopNoReturn(void)
    {
        T element = m_elements[m_itemCount - 1];
        RemoveAt(m_itemCount - 1);
    }

    T& Last(void)
    {
        return m_elements[m_itemCount - 1];
    }

    bool GetLast(T& item)
    {
        if (m_itemCount <= 0)
            return false;

        item = m_elements[m_itemCount - 1];

        return true;
    }

    //
    // Function: AssignFrom
    //  Reassigns current array with specified one.
    //
    // Parameters:
    //  other - Other array that should be assigned.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool AssignFrom(const Array <T>& other)
    {
        if (&other == this)
            return true;

        if (!SetSize(other.m_itemCount, false))
            return false;

        for (int i = 0; i < other.m_itemCount; i++)
            m_elements[i] = other.m_elements[i];

        m_itemCount = other.m_itemCount;
        m_resizeStep = other.m_resizeStep;

        return true;
    }

    //
    // Function: GetRandomElement
    //  Gets the random element from the array.
    //
    // Returns:
    //  Random element reference.
    //
    T& GetRandomElement(void) const
    {
        return m_elements[(*g_engfuncs.pfnRandomLong) (0, m_itemCount - 1)];
    }

    Array <T>& operator = (const Array <T>& other)
    {
        AssignFrom(other);
        return *this;
    }

    T& operator [] (int index)
    {
        if (index < m_itemSize && index >= m_itemCount)
            m_itemCount = index + 1;

        return GetAt(index);
    }
};

template <typename T1, typename T2> struct Pair
{
public:
    T1 first;
    T2 second;

public:
    Pair <T1, T2>(void) : first(T1()), second(T2())
    {
    }

    Pair(const T1& f, const T2& s) : first(f), second(s)
    {
    }

    template <typename A1, typename A2> Pair(const Pair <A1, A2>& right) : first(right.first), second(right.second)
    {
    }
};

template <class K, class V> class Map
{
    //
    // Typedef: MapEntries
    //
public:
    typedef Array <Pair <K, V> > MapEntries;

    // Group: Private members.
    //
private:
    struct HashItem
    {
        //
        // Group: Members.
        //
    public:
        int index;
        HashItem* next;

        //
        // Group: Functions.
        //
    public:
        inline HashItem(void) : next(nullptr)
        {
        }

        inline HashItem(int index, HashItem* next) : index(index), next(next)
        {
        }
    };

    int m_hashSize;

    HashItem** m_table;
    MapEntries m_mapTable;

private:
    static inline int HashFunc(K tag)
    {
        int key = tag;

        key += ~(key << 16);
        key ^= (key >> 5);

        key += (key << 3);
        key ^= (key >> 13);

        key += ~(key << 9);
        key ^= (key >> 17);

        return key;
    }

    //
    // Group: (Con/De)structors.
    //
public:

    //
    // Function: Map
    //
    // Default constructor for map container.
    //
    // Parameters:
    //	  hashSize - Initial hash size.
    //
    inline Map <K, V>(int hashSize = 36) : m_hashSize(hashSize), m_table(new HashItem* [hashSize])
    {
        if (m_table == nullptr)
            return;

        for (int i = 0; i < hashSize; i++)
            m_table[i] = nullptr;
    }

    //
    // Function: ~Map
    //
    // Default map container destructor.
    //
    // Parameters:
    //	  hashSize - Initial hash size.
    //
    inline ~Map <K, V>(void)
    {
        RemoveAll();

        delete[] m_table;
    }

    //
    // Group: Functions.
    //
public:
    //
    // Function: IsExists
    //
    // Checks whether specified element exists in container.
    //
    // Parameters:
    //	  keyName - Key that should be looked up.
    //
    // Returns:
    //   True if key exists, false otherwise.
    //
    inline bool IsExists(const K& keyName) const
    {
        return GetIndex(keyName) != -1;
    }

    //
    // Function: SetupMap
    //
    // Initializes map, if not initialized automatically.
    //
    // Parameters:
    //	  hashSize - Initial hash size.
    //
    inline void SetupMap(int hashSize)
    {
        m_hashSize = hashSize;
        m_table = new HashItem * [hashSize];

        if (m_table == nullptr)
            return;

        for (int i = 0; i < hashSize; i++)
            m_table[i] = nullptr;
    }

    //
    // Function: IsEmpty
    //
    // Checks whether map container is currently empty.
    //
    // Returns:
    //   True if no elements exists, false otherwise.
    //
    inline bool IsEmpty(void) const
    {
        return m_mapTable.GetAllocatedSize() == 0;
    }

    //
    // Function: GetSize
    //
    // Retrieves size of the map container.
    //
    // Returns:
    //   Number of elements currently in map container.
    //
    inline int GetSize(void) const
    {
        return m_mapTable.GetAllocatedSize();
    }

    //
    // Function: GetKey
    //
    // Gets the key object, by it's index.
    //
    // Parameters:
    //	  index - Index of key.
    //
    // Returns:
    //   Object containing the key.
    //
    inline K& GetKey(int index)
    {
        return m_mapTable[index].first;
    }

    //
    // Function: GetKey
    //
    // Gets the constant key object, by it's index.
    //
    // Parameters:
    //	  index - Index of key.
    //
    // Returns:
    //   Constant object containing the key.
    //
    inline const K& GetKey(int index) const
    {
        return m_mapTable[index].first;
    }

    // Function: GetValue
    //
    // Gets the element object, by it's index.
    //
    // Parameters:
    //	  index - Index of element.
    //
    // Returns:
    //   Object containing the element.
    //
    inline V& GetValue(int index)
    {
        return m_mapTable[index].second;
    }

    //
    // Function: GetValue
    //
    // Gets the constant element object, by it's index.
    //
    // Parameters:
    //	  index - Index of element.
    //
    // Returns:
    //   Constant object containing the element.
    //
    inline const V& GetValue(int index) const
    {
        return m_mapTable[index].second;
    }

    //
    // Function: GetElements
    //
    // Gets the all elements of container.
    //
    // Returns:
    //   Array of elements, containing inside container.
    //
    // See also:
    //   <MapEntries>
    //
    inline MapEntries& GetElements(void)
    {
        return m_mapTable;
    }

    //
    // Function: Find
    //
    //	Finds element by his key name.
    //
    // Parameters:
    //	  keyName - Key name to be searched.
    //	  element - Holder for element object.
    //
    // Returns:
    //   True if element found, false otherwise.
    //
    bool Find(const K& keyName, V& element) const
    {
        int index = GetIndex(keyName);

        if (index == -1)
            return false;

        element = m_mapTable[index].second;

        return true;
    }

    //
    // Function: Find
    //
    // Finds element by his key name.
    //
    // Parameters:
    //	  keyName - Key name to be searched.
    //	  elementPointer - Holder for element pointer.
    //
    // Returns: True if element found, false otherwise.
    //
    bool Find(const K& keyName, V*& elementPointer) const
    {
        int index = GetIndex(keyName);

        if (index == -1)
            return false;

        elementPointer = &m_mapTable[index].second;

        return true;
    }

    //
    // Function: Remove
    //
    // Removes element from container.
    //
    // Parameters:
    //	  keyName - Key name of element, that should be removed.
    //
    // Returns:
    //   True if key was removed successfully, false otherwise.
    //
    bool Remove(const K& keyName)
    {
        int hashID = Map::HashFunc <K>(keyName) % m_hashSize;
        HashItem* hashItem = m_table[hashID], * nextHash = nullptr;

        while (hashItem != nullptr)
        {
            if (m_mapTable[hashItem->index].first == keyName)
            {
                if (nextHash == nullptr)
                    m_table[hashID] = hashItem->next;
                else
                    nextHash->next = hashItem->next;

                m_mapTable.RemoveAt(hashItem->index);
                delete hashItem;

                return true;
            }
            nextHash = hashItem;
            hashItem = hashItem->next;
        }
        return false;
    }

    //
    // Function: RemoveAll
    //
    // Removes all elements from container.
    //
    void RemoveAll(void)
    {
        HashItem* ptr, * next;

        for (int i = m_hashSize - 1; i < -1; i--)
        {
            ptr = m_table[i];

            while (ptr != nullptr)
            {
                next = ptr->next;

                delete ptr;
                ptr = next;
            }

            m_table[i] = nullptr;
        }

        m_mapTable.RemoveAll();
    }

    //
    // Function: GetIndex
    //
    // Gets index of element.
    //
    // Parameters:
    //	  keyName - Key of element.
    //	  create - If true and no element found by a keyName, create new element.
    //
    // Returns:
    //   Either found index, created index, or -1 in case of error.
    //
    int GetIndex(const K& keyName, bool create = false)
    {
        int hashID = Map <K, V>::HashFunc(keyName) % m_hashSize;

        for (HashItem* ptr = m_table[hashID]; ptr != nullptr; ptr = ptr->next)
        {
            if (m_mapTable[ptr->index].first == keyName)
                return ptr->index;
        }

        if (create)
        {
            int item = m_mapTable.GetAllocatedSize();

            if (m_mapTable.SetSize(item + 1))
            {
                m_table[hashID] = new HashItem(item, m_table[hashID]);
                m_mapTable[item].first = keyName;

                return item;
            }
        }

        return -1;
    }

    //
    // Group: Operators.
    //
public:
    inline V& operator [] (const K& keyName)
    {
        return m_mapTable[GetIndex(keyName, true)].second;
    }

    inline const V& operator [] (const K& keyName) const
    {
        return m_mapTable[GetIndex(keyName, true)].second;
    }
};

class String
{
private:
    char* m_bufferPtr;
    int m_allocatedSize;
    int m_stringLength;

    //
    // Group: Private functions
    //
private:

    //
    // Function: UpdateBufferSize
    //  Updates the buffer size.
    //
    // Parameters:
    //  size - New size of buffer.
    //
    void UpdateBufferSize(int size)
    {
        if (size <= m_allocatedSize)
            return;

        m_allocatedSize = size + 16;
        char* tempBuffer = new char[size + 1];

        if (m_bufferPtr != nullptr)
        {
            strcpy(tempBuffer, m_bufferPtr);
            tempBuffer[m_stringLength] = 0;

            delete[] m_bufferPtr;
        }

        m_bufferPtr = tempBuffer;
        m_allocatedSize = size;
    }

    //
    // Function: MoveItems
    //  Moves characters inside buffer pointer.
    //
    // Parameters:
    //  destIndex - Destination index.
    //  sourceIndex - Source index.
    //
    void MoveItems(int destIndex, int sourceIndex)
    {
        memmove(m_bufferPtr + destIndex, m_bufferPtr + sourceIndex, sizeof(char) * (m_stringLength - sourceIndex + 1));
    }

    //
    // Function: Initialize
    //  Initializes string buffer.
    //
    // Parameters:
    //  length - Initial length of string.
    //
    void Initialize(int length)
    {
        int freeSize = m_allocatedSize - m_stringLength - 1;

        if (length <= freeSize)
            return;

        int delta = 4;

        if (m_allocatedSize > 64)
            delta = m_allocatedSize * 0.5;
        else if (m_allocatedSize > 8)
            delta = 16;

        if (freeSize + delta < length)
            delta = length - freeSize;

        UpdateBufferSize(m_allocatedSize + delta);
    }

    //
    // Function: CorrectIndex
    //  Gets the correct string end index.
    //
    // Parameters:
    //  index - Holder for index.
    //
    void CorrectIndex(int& index) const
    {
        if (index > m_stringLength)
            index = m_stringLength;
    }

    //
    // Function: InsertSpace
    //  Inserts space at specified location, with specified length.
    //
    // Parameters:
    //  index - Location to insert space.
    //  size - Size of space insert.
    //
    void InsertSpace(int& index, int size)
    {
        CorrectIndex(index);
        Initialize(size);

        MoveItems(index + size, index);
    }

    //
    // Function: IsTrimChar
    //  Checks whether input is trimming character.
    //
    // Parameters:
    //  input - Input to check for.
    //
    // Returns:
    //  True if it's a trim char, false otherwise.
    //
    bool IsTrimChar(char input)
    {
        return input == ' ' || input == '\t' || input == '\n';
    }

    //
    // Group: (Con/De)structors
    //
public:
    String(void)
    {
        m_bufferPtr = nullptr;
        m_allocatedSize = 0;
        m_stringLength = 0;
    }

    ~String(void)
    {
        delete[] m_bufferPtr;
    }

    String(const char* bufferPtr)
    {
        m_bufferPtr = nullptr;
        m_allocatedSize = 0;
        m_stringLength = 0;

        Assign(bufferPtr);
    }

    String(char input)
    {
        m_bufferPtr = nullptr;
        m_allocatedSize = 0;
        m_stringLength = 0;

        Assign(input);
    }

    String(const String& inputString)
    {
        m_bufferPtr = nullptr;
        m_allocatedSize = 0;
        m_stringLength = 0;

        Assign(inputString.GetBuffer());
    }

    //
    // Group: Functions
    //
public:

    inline char* GetRawData(void)
    {
        return m_bufferPtr;
    }

    inline const char* GetRawData(void) const
    {
        return m_bufferPtr;
    }

    //
    // Function: GetBuffer
    //  Gets the string buffer.
    //
    // Returns:
    //  Pointer to buffer.
    //
    const char* GetBuffer(void)
    {
        if (m_bufferPtr == nullptr || *m_bufferPtr == 0x0)
            return "";

        return &m_bufferPtr[0];
    }

    //
    // Function: GetBuffer
    //  Gets the string buffer (constant).
    //
    // Returns:
    //  Pointer to constant buffer.
    //
    const char* GetBuffer(void) const
    {
        if (m_bufferPtr == nullptr || *m_bufferPtr == 0x0)
            return "";

        return &m_bufferPtr[0];
    }

    //
    // Function: ToFloat
    //  Gets the string as float, if possible.
    //
    // Returns:
    //  Float value of string.
    //
    float ToFloat(void)
    {
        return static_cast <float> (atof(m_bufferPtr));
    }

    //
    // Function: ToInt
    //  Gets the string as integer, if possible.
    //
    // Returns:
    //  Integer value of string.
    //
    int ToInt(void) const
    {
        return atoi(m_bufferPtr);
    }

    //
    // Function: ReleaseBuffer
    //  Terminates the string with null character.
    //
    void ReleaseBuffer(void)
    {
        ReleaseBuffer(strlen(m_bufferPtr));
    }

    //
    // Function: ReleaseBuffer
    //  Terminates the string with null character with specified buffer end.
    //
    // Parameters:
    //  newLength - End of buffer.
    //
    void ReleaseBuffer(int newLength)
    {
        m_bufferPtr[newLength] = 0;
        m_stringLength = newLength;
    }

    //
    // Function: GetBuffer
    //  Gets the buffer with specified length.
    //
    // Parameters:
    //  minLength - Length to retrieve.
    //
    // Returns:
    //  Pointer to string buffer.
    //
    char* GetBuffer(int minLength)
    {
        if (minLength >= m_allocatedSize)
            UpdateBufferSize(minLength + 1);

        return m_bufferPtr;
    }

    //
    // Function: GetBufferSetLength
    //  Gets the buffer with specified length, and terminates string with that length.
    //
    // Parameters:
    //  minLength - Length to retrieve.
    //
    // Returns:
    //  Pointer to string buffer.
    //
    char* GetBufferSetLength(int length)
    {
        char* buffer = GetBuffer(length);

        m_stringLength = length;
        m_bufferPtr[length] = 0;

        return buffer;
    }

    //
    // Function: Append
    //  Appends the string to existing buffer.
    //
    // Parameters:
    //  bufferPtr - String buffer to append.
    //
    void Append(const char* bufferPtr)
    {
        UpdateBufferSize(m_stringLength + strlen(bufferPtr) + 1);
        strcat(m_bufferPtr, bufferPtr);

        m_stringLength = strlen(m_bufferPtr);
    }

    //
    // Function: Append
    //  Appends the character to existing buffer.
    //
    // Parameters:
    //  input - Character to append.
    //
    void Append(const char input)
    {
        UpdateBufferSize(m_stringLength + 2);

        m_bufferPtr[m_stringLength] = input;
        m_bufferPtr[m_stringLength++] = 0;
    }

    //
    // Function: Append
    //  Appends the string to existing buffer.
    //
    // Parameters:
    //  inputString - String buffer to append.
    //
    void Append(const String& inputString)
    {
        const char* bufferPtr = inputString.GetBuffer();
        UpdateBufferSize(m_stringLength + strlen(bufferPtr));

        strcat(m_bufferPtr, bufferPtr);
        m_stringLength = strlen(m_bufferPtr);
    }

    //
    // Function: AppendFormat
    //  Appends the formatted string to existing buffer.
    //
    // Parameters:
    //  fmt - Formatted, tring buffer to append.
    //
    void AppendFormat(const char* fmt, ...)
    {
        va_list ap;
        char buffer[1024];

        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);

        Append(buffer);
    }

    //
    // Function: Assign
    //  Assigns the string to existing buffer.
    //
    // Parameters:
    //  inputString - String buffer to assign.
    //
    void Assign(const String& inputString)
    {
        Assign(inputString.GetBuffer());
    }

    //
    // Function: Assign
    //  Assigns the character to existing buffer.
    //
    // Parameters:
    //  input - Character to assign.
    //
    void Assign(char input)
    {
        char psz[2] = { input, 0 };
        Assign(psz);
    }

    //
    // Function: Assign
    //  Assigns the string to existing buffer.
    //
    // Parameters:
    //  bufferPtr - String buffer to assign.
    //
    void Assign(const char* bufferPtr)
    {
        if (bufferPtr == nullptr)
        {
            UpdateBufferSize(1);
            m_stringLength = 0;
            return;
        }

        UpdateBufferSize(strlen(bufferPtr));

        if (m_bufferPtr != nullptr)
        {
            strcpy(m_bufferPtr, bufferPtr);
            m_stringLength = strlen(m_bufferPtr);
        }
        else
            m_stringLength = 0;
    }

    //
    // Function: Assign
    //  Assigns the formatted string to existing buffer.
    //
    // Parameters:
    //  fmt - Formatted string buffer to assign.
    //
    void AssignFormat(const char* fmt, ...)
    {
        va_list ap;
        char buffer[1024];

        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);

        Assign(buffer);
    }

    //
    // Function: Empty
    //  Empties the string.
    //
    void SetEmpty(void)
    {
        if (m_bufferPtr != nullptr)
        {
            m_bufferPtr[0] = 0;
            m_stringLength = 0;
        }
    }

    //
    // Function: IsEmpty
    //  Checks whether string is empty.
    //
    // Returns:
    //  True if string is empty, false otherwise.
    //
    bool IsEmpty(void) const
    {
        if (m_bufferPtr == nullptr || m_stringLength == 0)
            return true;

        return false;
    }

    //
    // Function: GetLength
    //  Gets the string length.
    //
    // Returns:
    //  Length of string, 0 in case of error.
    //
    int GetLength(void)
    {
        if (m_bufferPtr == nullptr)
            return 0;

        return m_stringLength;
    }

    operator const char* (void) const
    {
        return GetBuffer();
    }

    operator char* (void)
    {
        return const_cast <char*> (GetBuffer());
    }

    operator int(void)
    {
        return ToInt();
    }

    operator long(void)
    {
        return static_cast <long> (ToInt());
    }

    operator float(void)
    {
        return ToFloat();
    }

    operator double(void)
    {
        return static_cast <double> (ToFloat());
    }

    friend String operator + (const String& s1, const String& s2)
    {
        String result(s1);
        result += s2;

        return result;
    }

    friend String operator + (const String& holder, char ch)
    {
        String result(holder);
        result += ch;

        return result;
    }

    friend String operator + (char ch, const String& holder)
    {
        String result(ch);
        result += holder;

        return result;
    }

    friend String operator + (const String& holder, const char* str)
    {
        String result(holder);
        result += str;

        return result;
    }

    friend String operator + (const char* str, const String& holder)
    {
        String result(const_cast <char*> (str));
        result += holder;

        return result;
    }

    friend bool operator == (const String& s1, const String& s2)
    {
        return s1.Compare(s2) == 0;
    }

    friend bool operator < (const String& s1, const String& s2)
    {
        return s1.Compare(s2) < 0;
    }

    friend bool operator > (const String& s1, const String& s2)
    {
        return s1.Compare(s2) > 0;
    }

    friend bool operator == (const char* s1, const String& s2)
    {
        return s2.Compare(s1) == 0;
    }

    friend bool operator == (const String& s1, const char* s2)
    {
        return s1.Compare(s2) == 0;
    }

    friend bool operator != (const String& s1, const String& s2)
    {
        return s1.Compare(s2) != 0;
    }

    friend bool operator != (const char* s1, const String& s2)
    {
        return s2.Compare(s1) != 0;
    }

    friend bool operator != (const String& s1, const char* s2)
    {
        return s1.Compare(s2) != 0;
    }

    String& operator = (const String& inputString)
    {
        Assign(inputString);
        return *this;
    }

    String& operator = (const char* bufferPtr)
    {
        Assign(bufferPtr);
        return *this;
    }

    String& operator = (char input)
    {
        Assign(input);
        return *this;
    }

    String& operator += (const String& inputString)
    {
        Append(inputString);
        return *this;
    }

    String& operator += (const char* bufferPtr)
    {
        Append(bufferPtr);
        return *this;
    }

    char operator [] (int index)
    {
        if (index > m_stringLength)
            return -1;

        return m_bufferPtr[index];
    }

    //
    // Function: Mid
    //  Gets the substring by specified bounds.
    //
    // Parameters:
    //  startIndex - Start index to get from.
    //  count - Number of characters to get.
    //
    // Returns:
    //  Tokenized string.
    //
    String Mid(int startIndex, int count = -1)
    {
        String result;

        if (startIndex >= m_stringLength || !m_bufferPtr)
            return result;

        if (count == -1)
            count = m_stringLength - startIndex;
        else if (startIndex + count >= m_stringLength)
            count = m_stringLength - startIndex;

        int i = 0, j = 0;
        char* holder = new char[m_stringLength + 1];

        for (i = startIndex; i < startIndex + count; i++)
            holder[j++] = m_bufferPtr[i];

        holder[j] = 0;
        result.Assign(holder);

        delete[] holder;
        return result;
    }

    //
    // Function: Mid
    //  Gets the substring by specified bounds.
    //
    // Parameters:
    //  startIndex - Start index to get from.
    //
    // Returns:
    //  Tokenized string.
    //
    String Mid(int startIndex)
    {
        return Mid(startIndex, m_stringLength - startIndex);
    }

    //
    // Function: Left
    //  Gets the string from left side.
    //
    // Parameters:
    //  count - Number of characters to get.
    //
    // Returns:
    //  Tokenized string.
    //
    String Left(int count)
    {
        return Mid(0, count);
    }

    //
    // Function: Right
    //  Gets the string from right side.
    //
    // Parameters:
    //  count - Number of characters to get.
    //
    // Returns:
    //  Tokenized string.
    //
    String Right(int count)
    {
        if (count > m_stringLength)
            count = m_stringLength;

        return Mid(m_stringLength - count, count);
    }

    //
    // Function: ToUpper
    //  Gets the string in upper case.
    //
    // Returns:
    //  Upped sting.
    //
    String ToUpper(void)
    {
        String result;

        for (int i = 0; i < GetLength(); i++)
            result += toupper(m_bufferPtr[i]);

        return result;
    }

    //
    // Function: ToUpper
    //  Gets the string in upper case.
    //
    // Returns:
    //  Lowered sting.
    //
    String ToLower(void)
    {
        String result;

        for (int i = 0; i < GetLength(); i++)
            result += tolower(m_bufferPtr[i]);

        return result;
    }

    //
    // Function: ToReverse
    //  Reverses the string.
    //
    // Returns:
    //  Reversed string.
    //
    String ToReverse(void)
    {
        char* source = m_bufferPtr + GetLength() - 1;
        char* dest = m_bufferPtr;

        while (source > dest)
        {
            if (*source == *dest)
            {
                source--;
                dest++;
            }
            else
            {
                char ch = *source;

                *source-- = *dest;
                *dest++ = ch;
            }
        }

        return m_bufferPtr;
    }

    //
    // Function: MakeUpper
    //  Converts string to upper case.
    //
    void MakeUpper(void)
    {
        *this = ToUpper();
    }

    //
    // Function: MakeLower
    //  Converts string to lower case.
    //
    void MakeLower(void)
    {
        *this = ToLower();
    }

    //
    // Function: MakeReverse
    //  Converts string into reverse order.
    //
    void MakeReverse(void)
    {
        *this = ToReverse();
    }

    //
    // Function: Compare
    //  Compares string with other string.
    //
    // Parameters:
    //  string - String t compare with.
    //
    // Returns:
    //  Zero if they are equal.
    //
    int Compare(const String& string) const
    {
        return strcmp(m_bufferPtr, string.m_bufferPtr);
    }

    //
    // Function: Compare
    //  Compares string with other string.
    //
    // Parameters:
    //  str - String t compare with.
    //
    // Returns:
    //  Zero if they are equal.
    //
    int Compare(const char* str) const
    {
        return strcmp(m_bufferPtr, str);
    }

    //
    // Function: Collate
    //  Collate the string.
    //
    // Parameters:
    //  string - String to collate.
    //
    // Returns:
    //  One on success.
    //
    int Collate(const String& string) const
    {
        return strcoll(m_bufferPtr, string.m_bufferPtr);
    }

    //
    // Function: Find
    //  Find the character.
    //
    // Parameters:
    //  input - Character to search for.
    //
    // Returns:
    //  Index of character.
    //
    int Find(char input) const
    {
        return Find(input, 0);
    }

    //
    // Function: Find
    //  Find the character.
    //
    // Parameters:
    //  input - Character to search for.
    //  startIndex - Start index to search from.
    //
    // Returns:
    //  Index of character.
    //
    int Find(char input, int startIndex) const
    {
        char* str = m_bufferPtr + startIndex;

        for (;;)
        {
            if (*str == input)
                return str - m_bufferPtr;

            if (*str == 0)
                return -1;

            str++;
        }
    }

    //
    // Function: Find
    //  Tries to find string.
    //
    // Parameters:
    //  string - String to search for.
    //
    // Returns:
    //  Position of found string.
    //
    int Find(const String& string) const
    {
        return Find(string, 0);
    }

    //
    // Function: Find
    //  Tries to find string from specified index.
    //
    // Parameters:
    //  string - String to search for.
    //  startIndex - Index to start search from.
    //
    // Returns:
    //  Position of found string.
    //
    int Find(const String& string, int startIndex) const
    {
        if (string.m_stringLength == 0)
            return startIndex;

        for (; startIndex < m_stringLength; startIndex++)
        {
            int j;

            for (j = 0; j < string.m_stringLength && startIndex + j < m_stringLength; j++)
            {
                if (m_bufferPtr[startIndex + j] != string.m_bufferPtr[j])
                    break;
            }

            if (j == string.m_stringLength)
                return startIndex;
        }
        return -1;
    }

    //
    // Function: ReverseFind
    //  Tries to find character in reverse order.
    //
    // Parameters:
    //  ch - Character to search for.
    //
    // Returns:
    //  Position of found character.
    //
    int ReverseFind(char ch)
    {
        if (m_stringLength == 0)
            return -1;

        char* str = m_bufferPtr + m_stringLength - 1;

        for (;;)
        {
            if (*str == ch)
                return str - m_bufferPtr;

            if (str == m_bufferPtr)
                return -1;
            str--;
        }
    }

    //
    // Function: FindOneOf
    //  Find one of occurrences of string.
    //
    // Parameters:
    //  string - String to search for.
    //
    // Returns:
    //  -1 in case of nothing is found, start of string in buffer otherwise.
    //
    int FindOneOf(const String& string)
    {
        for (int i = 0; i < m_stringLength; i++)
        {
            if (string.Find(m_bufferPtr[i]) >= 0)
                return i;
        }
        return -1;
    }

    //
    // Function: TrimRight
    //  Trims string from right side.
    //
    // Returns:
    //  Trimmed string.
    //
    String& TrimRight(void)
    {
        char* str = m_bufferPtr;
        char* last = nullptr;

        while (*str != 0)
        {
            if (IsTrimChar(*str))
            {
                if (last == nullptr)
                    last = str;
            }
            else
                last = nullptr;
            str++;
        }

        if (last != nullptr)
            Delete(last - m_bufferPtr);

        return *this;
    }

    //
    // Function: TrimLeft
    //  Trims string from left side.
    //
    // Returns:
    //  Trimmed string.
    //
    String& TrimLeft(void)
    {
        char* str = m_bufferPtr;

        while (IsTrimChar(*str))
            str++;

        if (str != m_bufferPtr)
        {
            int first = int(str - GetBuffer());
            char* buffer = GetBuffer(GetLength());

            str = buffer + first;
            int length = GetLength() - first;

            memmove(buffer, str, (length + 1) * sizeof(char));
            ReleaseBuffer(length);
        }
        return *this;
    }

    //
    // Function: Trim
    //  Trims string from both sides.
    //
    // Returns:
    //  Trimmed string.
    //
    String& Trim(void)
    {
        return TrimRight().TrimLeft();
    }

    //
    // Function: TrimRight
    //  Trims specified character at the right of the string.
    //
    // Parameters:
    //  ch - Character to trim.
    //
    void TrimRight(char ch)
    {
        const char* str = m_bufferPtr;
        const char* last = nullptr;

        while (*str != 0)
        {
            if (*str == ch)
            {
                if (last == nullptr)
                    last = str;
            }
            else
                last = nullptr;

            str++;
        }

        if (last != nullptr)
        {
            int i = last - m_bufferPtr;
            Delete(i, m_stringLength - i);
        }
    }

    //
    // Function: TrimLeft
    //  Trims specified character at the left of the string.
    //
    // Parameters:
    //  ch - Character to trim.
    //
    void TrimLeft(char ch)
    {
        char* str = m_bufferPtr;

        while (ch == *str)
            str++;

        Delete(0, str - m_bufferPtr);
    }

    //
    // Function: Insert
    //  Inserts character at specified index.
    //
    // Parameters:
    //  index - Position to insert string.
    //  ch - Character to insert.
    //
    // Returns:
    //  New string length.
    //
    int Insert(int index, char ch)
    {
        InsertSpace(index, 1);

        m_bufferPtr[index] = ch;
        m_stringLength++;

        return m_stringLength;
    }

    //
    // Function: Insert
    //  Inserts string at specified index.
    //
    // Parameters:
    //  index - Position to insert string.
    //  string - Text to insert.
    //
    // Returns:
    //  New string length.
    //
    int Insert(int index, const String& string)
    {
        CorrectIndex(index);

        if (string.m_stringLength == 0)
            return m_stringLength;

        int numInsertChars = string.m_stringLength;
        InsertSpace(index, numInsertChars);

        for (int i = 0; i < numInsertChars; i++)
            m_bufferPtr[index + i] = string[i];
        m_stringLength += numInsertChars;

        return m_stringLength;
    }

    //
    // Function: Replace
    //  Replaces old characters with new one.
    //
    // Parameters:
    //  oldCharacter - Old character to replace.
    //  newCharacter - New character to replace with.
    //
    // Returns:
    //  Number of occurrences replaced.
    //
    int Replace(char oldCharacter, char newCharacter)
    {
        if (oldCharacter == newCharacter)
            return 0;

        static int num = 0;
        int position = 0;

        while (position < GetLength())
        {
            position = Find(oldCharacter, position);

            if (position < 0)
                break;

            m_bufferPtr[position] = newCharacter;

            position++;
            num++;
        }
        return num;
    }

    //
    // Function: Replace
    //  Replaces string in other string.
    //
    // Parameters:
    //  oldString - Old string to replace.
    //  newString - New string to replace with.
    //
    // Returns:
    //  Number of characters replaced.
    //
    int Replace(const String& oldString, const String& newString)
    {
        if (oldString.m_stringLength == 0)
            return 0;

        if (newString.m_stringLength == 0)
            return 0;

        int oldLength = oldString.m_stringLength;
        int newLength = newString.m_stringLength;

        int num = 0;
        int position = 0;

        while (position < m_stringLength)
        {
            position = Find(oldString, position);

            if (position < 0)
                break;

            Delete(position, oldLength);
            Insert(position, newString);

            position += newLength;
            num++;
        }
        return num;
    }

    //
    // Function: Delete
    //  Deletes characters from string.
    //
    // Parameters:
    //  index - Start of characters remove.
    //  count - Number of characters to remove.
    //
    // Returns:
    //  New string length.
    //
    int Delete(int index, int count = 1)
    {
        if (index + count > m_stringLength)
            count = m_stringLength - index;

        if (count > 0)
        {
            MoveItems(index, index + count);
            m_stringLength -= count;
        }
        return m_stringLength;
    }

    //
    // Function: TrimQuotes
    //  Trims trailing quotes.
    //
    // Returns:
    //  Trimmed string.
    //
    String TrimQuotes(void)
    {
        TrimRight('\"');
        TrimRight('\'');

        TrimLeft('\"');
        TrimLeft('\'');

        return *this;
    }

    //
    // Function: Contains
    //  Checks whether string contains something.
    //
    // Parameters:
    //  what - String to check.
    //
    // Returns:
    //  True if string exists, false otherwise.
    //
    bool Contains(const String& what)
    {
        return strstr(m_bufferPtr, what.m_bufferPtr) != nullptr;
    }

    //
    // Function: Hash
    //  Gets the string hash.
    //
    // Returns:
    //  Hash of the string.
    //
    unsigned long Hash(void)
    {
        unsigned long hash = 0;
        const char* ptr = m_bufferPtr;

        while (*ptr)
        {
            hash = (hash << 5) + hash + (*ptr);
            ptr++;
        }
        return hash;
    }

    //
    // Function: Split
    //  Splits string using string separator.
    //
    // Parameters:
    //  separator - Separator to split with.
    //
    // Returns:
    //  Array of slitted strings.
    //
    // See Also:
    //  <Array>
    //
    Array <String> Split(const char* separator)
    {
        Array <String> holder;
        int tokenLength, index = 0;

        do
        {
            index += strspn(&m_bufferPtr[index], separator);
            tokenLength = strcspn(&m_bufferPtr[index], separator);

            if (tokenLength > 0)
                holder.Push(Mid(index, tokenLength));

            index += tokenLength;

        } while (tokenLength > 0);

        return holder;
    }

    //
    // Function: Split
    //  Splits string using character.
    //
    // Parameters:
    //  separator - Separator to split with.
    //
    // Returns:
    //  Array of slitted strings.
    //
    // See Also:
    //  <Array>
    //
    Array <String> Split(char separator)
    {
        char sep[2];

        sep[0] = separator;
        sep[1] = 0x0;

        return Split(sep);
    }
};

//
// Class: File
// A simple wrapper to a stdio FILE.
//
class File
{
    //
    // Group: Private members.
    //
private:

    //
    // Variable: m_handle
    // Pointer to C file stream.
    //
    FILE* m_handle;

    // Variable: m_size
    // Number of bytes in file.
    //
    int m_size;

    //
    // Group: (Con/De)structors.
    //
public:
    //
    // Function: File
    //
    // Default file class constructor.
    //
    inline File(void) : m_handle(nullptr), m_size(0)
    {
    }

    //
    // Function: File
    //
    // Default file class, constructor, with file opening.
    //
    // Parameters:
    //   filePath - String containing file name.
    //   mode - String containing open mode for file.
    //
    inline File(const String& filePath, const String& mode = "rt") : m_handle(nullptr), m_size(0)
    {
        Open(filePath, mode);
    }

    //
    // Function: ~File
    //
    // Default file class, destructor.
    //
    inline ~File(void)
    {
        if (IsValid())
            fclose(m_handle);
    }

    //
    // Group: Functions.
    //
public:
    //
    // Function: Open
    //
    // Opens file and gets it's size.
    //
    // Parameters:
    //	  filePath - String containing file name.
    //	  mode - String containing open mode for file.
    //
    // Returns:
    //   True if operation succeeded, false otherwise.
    //
    inline bool Open(const String& filePath, const String& mode = "rt")
    {
        m_handle = fopen(filePath, mode);

        if (!IsValid())
            return false;

        fseek(m_handle, 0l, SEEK_END);
        m_size = ftell(m_handle); // get the filesize.
        fseek(m_handle, 0l, SEEK_SET);

        return true;
    }

    //
    // Function: Close
    //
    // Closes file, and destroys STDIO file object.
    //
    inline void Close(void)
    {
        if (IsValid())
        {
            fclose(m_handle);
            m_handle = nullptr;
        }

        m_size = 0;
    }

    //
    // Function: IsEndOfFile
    //
    // Checks whether we reached end of file.
    //
    // Returns:
    //   True if reached, false otherwise.
    //
    inline bool IsEndOfFile(void) const
    {
        return feof(m_handle) != 0;
    }

    //
    // Function: Flush
    //
    // Flushes file stream.
    //
    // Returns:
    //   True if operation succeeded, false otherwise.
    //
    inline bool Flush(void) const
    {
        return fflush(m_handle) == 0;
    }

    //
    // Function: GetCharacter
    //
    // Pops one character from the file stream.
    //
    // Returns:
    //   Popped from stream character.
    //
    inline uint8_t GetCharacter(void) const
    {
        return  static_cast <uint8_t> (fgetc(m_handle));
    }

    //
    // Function: GetBuffer
    //
    // Gets the line from file stream, and stores it inside string class.
    //
    // Parameters:
    //	  buffer - String buffer, that should receive line.
    //	  count - Maximum size of buffer.
    //
    // Returns:
    //   True if operation succeeded, false otherwise.
    //
    inline bool GetBuffer(String& buffer, int count = 256) const
    {
        char* tempBuffer = new char[static_cast <uint32_t> (count)];
        buffer.SetEmpty();

        if (tempBuffer == nullptr)
            return false;

        if (fgets(tempBuffer, count, m_handle) != nullptr)
        {
            buffer = tempBuffer;
            delete[] tempBuffer;

            return true;
        }

        delete[] tempBuffer;

        return false;
    }

    //
    // Function: GetBuffer
    //
    // Gets the line from file stream, and stores it inside string class.
    //
    // Parameters:
    //	  buffer - String buffer, that should receive line.
    //	  count - Maximum size of buffer.
    //
    // Returns:
    //   True if operation succeeded, false otherwise.
    //
    inline bool GetBuffer(char* buffer, int count = 256) const
    {
        return fgets(buffer, count, m_handle) != nullptr;
    }

    //
    // Function: Print
    //
    // Puts formatted buffer, into stream.
    //
    // Parameters:
    //	  format - String to write.
    //
    // Returns:
    //   Number of bytes, that was written.
    //
    inline int Print(const char* format, ...) const
    {
        va_list ap;

        va_start(ap, format);
        int written = vfprintf(m_handle, format, ap);
        va_end(ap);

        return written;
    }

    //
    // Function: Print
    //
    // Puts formatted buffer, into stream.
    //
    // Parameters:
    //	  format - String to write.
    //
    // Returns
    //   Number of bytes, that was written.
    //
    inline int Print(const String& message) const
    {
        return fprintf(m_handle, message);
    }

    //
    // Function: PutCharacter
    //
    // Puts character into file stream.
    //
    // Parameters:
    //	  character - Character that should be put into stream.
    //
    // Returns:
    //   Character that was putted into the stream.
    //
    inline bool PutCharacter(uint8_t character) const
    {
        return fputc(static_cast <int> (character), m_handle) != EOF;
    }

    //
    // Function: PutString
    //
    // Puts buffer into the file stream.
    //
    // Parameters:
    //	  string - Buffer that should be put, into stream.
    //
    // Returns:
    //   True if operation succeeded, false otherwise.
    //
    inline bool PutString(const String& string) const
    {
        return fputs(string, m_handle) != EOF;
    }

    //
    // Function: Read
    //
    // Reads buffer from file stream in binary format.
    //
    // Parameters:
    //	  buffer - Holder for read buffer.
    //	  size - Size of the buffer to read.
    //	  count - Number of buffer chunks to read.
    //
    // Returns:
    //   Number of bytes red from file.
    //
    inline bool Read(void* buffer, uint32_t size, uint32_t count = 1) const
    {
        return fread(buffer, size, count, m_handle) == count;
    }

    //
    // Function: Write
    //
    // Writes binary buffer into file stream.
    //
    // Parameters:
    //	  buffer - Buffer holder, that should be written into file stream.
    //	  size - Size of the buffer that should be written.
    //	  count - Number of buffer chunks to write.
    //
    // Returns:
    //   Numbers of bytes written to file.
    //
    inline bool Write(void* buffer, uint32_t size, uint32_t count = 1) const
    {
        return fwrite(buffer, size, count, m_handle) == count;
    }

    //
    // Function: Seek
    //
    // Seeks file stream with specified parameters.
    //
    // Parameters:
    //	  offset - Offset where cursor should be set.
    //	  origin - Type of offset set.
    //
    // Returns:
    //   True if operation success, false otherwise.
    //
    inline bool Seek(long offset, int origin) const
    {
        return fseek(m_handle, offset, origin) == 0;
    }

    //
    // Function: Rewind
    //
    // Rewinds the file stream.
    //
    inline void Rewind(void) const
    {
        rewind(m_handle);
    }

    //
    // Function: GetSize
    //
    // Gets the file size of opened file stream.
    //
    // Returns:
    //   Number of bytes in file.
    //
    inline int GetSize(void) const
    {
        return m_size;
    }

    //
    // Function: IsValid
    //
    // Checks whether file stream is valid.
    //
    // Returns:
    //   True if file stream valid, false otherwise.
    //
    inline bool IsValid(void) const
    {
        return m_handle != nullptr;
    }
};

//
// Interface: ILoggerEngine
// Engine specific information for logger.
//
class ILoggerEngine
{
    //
    // Group: Virtual Methods
    //
public:

    //
    // Function: Echo
    //
    // Echos something to the engine console.
    //
    // Parameters:
    //   format - String to print with varargs.
    //
    virtual void EchoWithTag(const char* format, ...) = 0;

    //
    // Function: GetFlags
    //
    // Get's the flags that are set for logging.
    //
    // Returns:
    //   Bitmask of flags currently set for logging.
    //
    virtual int GetFlags(void) = 0;

    //
    // Function: GetLogFileName
    //
    // Get's the logfile name.
    //
    // Returns:
    //   Name of the logfile.
    //
    virtual const char* GetLogFileName(void) = 0;
};

//
// Class: Logger
// Simple logger class for logging actions.
//
class Logger : public Singleton <Logger>
{
    //
    // Group: Private members.
    //
private:

    //
    // Variable: m_logFile
    // Pointer to log file.
    //
    File m_logFile;

    //
    // Variable: m_logger
    //
    ILoggerEngine* m_logger;

    //
    // Group: (Con/De)structors.
    //
public:
    //
    // Function: Logger
    // 
    // Default logger constructor.
    //
    Logger(void)
    {
        if (m_logFile.IsValid())
            return;

        m_logFile.Open("logfile.log", "at");
    }

    //
    // Function: ~Logger
    // 
    // Default logger destructor.
    //
    ~Logger(void)
    {
        if (m_logFile.IsValid())
            m_logFile.Close();
    }

    //
    // Group: Operators
    //
public:
    Logger* operator -> (void)
    {
        return this;
    }

    //
    // Group: Private utilities.
    //
private:

    //
    // Function: GetTimeFormatString
    // 
    // Get's the formatted time string.
    //
    // Returns:
    //   Formatted time string.
    //
    inline const char* GetTimeFormatString(void) const
    {
        static char timeFormatStr[32];
        memset(timeFormatStr, 0, sizeof(char) * 32);

        time_t tick = time(&tick);
        tm* time = localtime(&tick);

        sprintf(timeFormatStr, "%02i:%02i:%02i", time->tm_hour, time->tm_min, time->tm_sec);

        return &timeFormatStr[0];
    }

public:
    //
    // Function: SetLoggerEngine
    //
    // Sets the logger engine.
    //
    // Parameters:
    //   logger - Pointer to implementation of ILoggerEngine interface.
    //
    // See Also:
    //    <ILoggerEngine>
    //
    inline void SetLoggerEngine(ILoggerEngine* logger)
    {
        m_logger = logger;


        if (m_logger && m_logFile.IsValid())
        {
            m_logFile.Close();
            m_logFile.Open(m_logger->GetLogFileName(), "at");
        }
    }

    //
    // Macro: DEFINE_PRINT_FUNCTION
    // 
    // Defines a generic print function.
    //
#define DEFINE_PRINT_FUNCTION(funcName, logMask, logStr) \
   void funcName (const char *format, ...) \
   { \
      int flags = m_logger->GetFlags (); \
      \
      if ((flags & logMask) != logMask) \
         return; \
      \
      char buffer[1024]; \
      va_list ap; \
      \
      va_start (ap, format); \
      vsprintf (buffer, format, ap); \
      va_end (ap); \
      \
      if (flags & LM_CONSOLE) \
         m_logger->EchoWithTag ("(%s): %s", logStr, buffer); \
      \
      m_logFile.Print ("[%s] (%s): %s\n", GetTimeFormatString (), logStr, buffer); \
      \
   }

public:
    DEFINE_PRINT_FUNCTION(Notice, LM_NOTICE, "NOTICE");
    DEFINE_PRINT_FUNCTION(Error, LM_ERROR, "ERROR");
    DEFINE_PRINT_FUNCTION(Warning, LM_WARNING, "WARNING");
    DEFINE_PRINT_FUNCTION(Critical, LM_CRITICAL, "CRITICAL");
    DEFINE_PRINT_FUNCTION(Message, LM_DEFAULT, "MESSAGE");

    // shouldn't be called directory, only via <Assert> macros.
    DEFINE_PRINT_FUNCTION(FastAssert, LM_ASSERT, "ASSERT");
};

class Color
{
    //
    // Group: Red, green, blue and alpha (controls transparency (0 - transparent, 255 - opaque)) color components (0 - 255).
    //
public:
    int red, green, blue, alpha;

    //
    // Group: (Con/De)structors.
    //
public:
    inline Color(int color = 0) : red(color), green(color), blue(color), alpha(color)
    {

    }

    inline Color(int inputRed, int inputGreen, int inputBlue, int inputAlpha = 0) : red(inputRed), green(inputGreen), blue(inputBlue), alpha(inputAlpha)
    {
    }

    inline Color(const Color& right) : red(right.red), green(right.green), blue(right.blue), alpha(right.alpha)
    {
    }

    //
    // Group: Operators.
    //
public:
    // equality
    inline bool operator == (const Color& right) const
    {
        return red == right.red && green == right.green && blue == right.blue && alpha == right.alpha;
    }

    inline bool operator != (const Color& right) const
    {
        return !operator == (right);
    }

    // array access
    inline int& operator [] (int colourIndex)
    {
        return (&red)[colourIndex];
    }

    inline const int& operator [] (int colourIndex) const
    {
        return (&red)[colourIndex];
    }

    inline const Color operator / (int scaler) const
    {
        return Color(red / scaler, green / scaler, blue / scaler, alpha / scaler);
    }
};

template <typename T1, typename T2> inline Pair <T1, T2> MakePair(T1 first, T2 second)
{
    return Pair <T1, T2>(first, second);
}


// @DEPRECATEME@
#define ITERATE_ARRAY(arrayName, iteratorName) \
   for (int iteratorName = 0; iteratorName != arrayName.GetElementNumber (); iteratorName++)

#endif // RUNTIME_INCLUDED
