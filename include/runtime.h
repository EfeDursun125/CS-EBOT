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
#include <list>
#include <cstring>
#include <cstdarg>
#include "clib.h"

#pragma warning (disable : 4996) // get rid of this

//
// Type: uint32_t
// Unsigned 32bit integer.
//
typedef unsigned int uint32_t;

//
// Type: uint64_t
// Unsigned 64bit integer.
// typedef unsigned long long uint64_t;

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
#define nullvec Vector::GetNull()

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
inline void FormatBuffer(char buffer[], const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
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

#define MATH_ONEPSILON 0.01f
#define MATH_EQEPSILON 0.001f
#define MATH_FLEPSILON 1.192092896e-07f
#define MATH_D2R 0.017453292519943295f
#define MATH_R2D 57.295779513082320876f

//
// Namespace: Math
// Utility mathematical functions.
//
namespace Math
{
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
    inline bool FltZero(const float entry)
    {
        return cabsf(entry) < MATH_ONEPSILON;
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
    inline bool FltEqual(const float entry1, const float entry2)
    {
        return cabsf(entry1 - entry2) < MATH_EQEPSILON;
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
    inline float RadianToDegree(const float radian)
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
    inline float DegreeToRadian(const float degree)
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
    inline float AngleMod(const float angle)
    {
        return 182.044444444f * (static_cast<int>(angle * 182.044444444f) & 65535);
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
        if (angle > 180.0f)
            angle -= 360.0f * croundf(angle * 0.00277777777778f);
        else if (angle < -180.0f)
            angle += 360.0f * croundf(-angle * 0.00277777777778f);
        return angle;
    }
}

//
// Class: Vector
// Standard 3-dimensional vector.
//
class Vector
{
public:
    float x;
    float y;
    float z;
    inline Vector(const float scaler = 0.0f) : x(scaler), y(scaler), z(scaler) {}
    inline Vector(const float inputX, const float inputY, const float inputZ) : x(inputX), y(inputY), z(inputZ) {}
    inline Vector(float* other) : x(other[0]), y(other[1]), z(other[2]) {}
    inline Vector(const Vector& right) : x(right.x), y(right.y), z(right.z) {}
public:
    inline operator float* (void) { return &x; }
    inline operator const float* (void) const { return &x; }
    inline const Vector operator+ (const Vector& right) const { return Vector(x + right.x, y + right.y, z + right.z); }
    inline const Vector operator- (const Vector& right) const { return Vector(x - right.x, y - right.y, z - right.z); }
    inline const Vector operator- (void) const { return Vector(-x, -y, -z); }
    friend inline const Vector operator* (const float vec, const Vector& right) { return Vector(right.x * vec, right.y * vec, right.z * vec); }
    inline const Vector operator* (const float vec) const { return Vector(vec * x, vec * y, vec * z); }
    inline const Vector operator/ (const float vec) const { const float inv = 1 / vec; return Vector(inv * x, inv * y, inv * z); }
    inline const Vector operator^ (const Vector& right) const { return Vector(y * right.z - z * right.y, z * right.x - x * right.z, x * right.y - y * right.x); }
    inline float operator| (const Vector& right) const { return x * right.x + y * right.y + z * right.z; }

    inline const Vector& operator+= (const Vector& right)
    {
        x += right.x;
        y += right.y;
        z += right.z;
        return *this;
    }

    inline const Vector& operator-= (const Vector& right)
    {
        x -= right.x;
        y -= right.y;
        z -= right.z;
        return *this;
    }

    inline const Vector& operator*= (const float vec)
    {
        x *= vec;
        y *= vec;
        z *= vec;
        return *this;
    }

    inline const Vector& operator/= (const float vec)
    {
        const float inv = 1.0f / vec;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    inline bool operator== (const Vector& right) const { return Math::FltEqual(x, right.x) && Math::FltEqual(y, right.y) && Math::FltEqual(z, right.z); }
    inline bool operator!= (const Vector& right) const { return !Math::FltEqual(x, right.x) && !Math::FltEqual(y, right.y) && !Math::FltEqual(z, right.z); }

    inline const Vector& operator= (const Vector& right)
    {
        x = right.x;
        y = right.y;
        z = right.z;
        return *this;
    }
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
        return csqrtf(x * x + y * y + z * z);
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
        return csqrtf(x * x + y * y);
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
        const float length = crsqrtf(x * x + y * y + z * z);
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
        const float length = crsqrtf(x * x + y * y);
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
        return Math::RadianToDegree(catan2f(z, GetLength2D()));
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
        return Math::RadianToDegree(catan2f(y, x));
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
        // it's another sort of vector compute individually the pitch and yaw corresponding to this vector.
        return Vector(Math::RadianToDegree(catan2f(z, GetLength2D())), Math::RadianToDegree(catan2f(y, x)), 0.0f);
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

        csincosf(Math::DegreeToRadian(x), sinePitch, cosinePitch); // compute the sine and cosine of the pitch component
        csincosf(Math::DegreeToRadian(y), sineYaw, cosineYaw); // compute the sine and cosine of the yaw component
        csincosf(Math::DegreeToRadian(z), sineRoll, cosineRoll); // compute the sine and cosine of the roll component

        if (forward)
        {
            forward->x = cosinePitch * cosineYaw;
            forward->y = cosinePitch * sineYaw;
            forward->z = -sinePitch;
        }

        if (right)
        {
            right->x = -sineRoll * sinePitch * cosineYaw + cosineRoll * sineYaw;
            right->y = -sineRoll * sinePitch * sineYaw - cosineRoll * cosineYaw;
            right->z = -sineRoll * cosinePitch;
        }

        if (upward)
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
        return min1.x < max2.x && max1.x > min2.x && min1.y < max2.y && max1.y > min2.y && min1.z < max2.z && max1.z > min2.z;
    }
}

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
    Array(const int resizeStep = 0)
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
        Destroy();
    }

    //
    // Group: Functions
    //
public:

    //
    // Function: Destroy
    //  Destroys array object, and all elements.
    //
    inline void Destroy(void)
    {
        if (m_elements)
        {
            delete[] m_elements;
            m_elements = nullptr;
        }

        m_itemSize = 0;
        m_itemCount = 0;
        m_resizeStep = 0;
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
    inline bool SetSize(const int newSize, const bool keepData = true)
    {
        if (!newSize)
        {
            Destroy();
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

            if (checkSize > 1024)
                checkSize = 1024;

            checkSize += m_itemCount;
        }

        if (newSize > checkSize)
            checkSize = newSize;

        T* buffer = new(std::nothrow) T[checkSize];
        if (!buffer)
            return false;

        if (keepData && m_elements)
        {
            if (checkSize < m_itemCount)
                m_itemCount = checkSize;

            int i;
            for (i = 0; i < m_itemCount; i++)
                buffer[i] = m_elements[i];

            delete[] m_elements;
        }

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
    inline int GetSize(void) const
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
    inline int GetElementNumber(void) const
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
    inline void SetEnlargeStep(const int resizeStep = 0)
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
    inline int GetEnlargeStep(void)
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
    inline bool SetAt(const int index, const T object, const bool enlarge = true)
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
    inline T& GetAt(const int index)
    {
        if (index < 0 || index >= m_itemCount)
            return m_elements[crandomint(0, m_itemCount - 1)];

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
    inline bool GetAt(const int index, T& object)
    {
        if (index < 0 || index >= m_itemCount)
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
    inline bool InsertAt(const int index, const T object, const bool enlarge = true)
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
    inline bool InsertAt(const int index, const T* objects, const int count = 1, const bool enlarge = true)
    {
        if (!objects || count < 1)
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
            int i;
            for (i = 0; i < count; i++)
                m_elements[i + index] = objects[i];

            m_itemCount = newSize;
        }
        else
        {
            int i;
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
    inline bool InsertAt(const int index, const Array <T>& other, const bool enlarge = true)
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
    inline bool RemoveAt(const int index, const int count = 1)
    {
        if (index + count > m_itemCount)
            return false;

        if (count < 1)
            return true;

        m_itemCount -= count;

        int i;
        for (i = index; i < m_itemCount; i++)
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
    inline bool Push(const T object, const bool enlarge = true)
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
    inline bool Push(const T* objects, const int count = 1, const bool enlarge = true)
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
    inline bool Push(const Array <T>& other, const bool enlarge = true)
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
    inline T* GetData(void)
    {
        return m_elements;
    }

    //
    // Function: RemoveAll
    //  Resets array, and removes all elements out of it.
    // 
    inline void RemoveAll(void)
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
        return !m_itemCount;
    }

    //
    // Function: FreeExtra
    //  Frees unused space.
    //
    inline void FreeSpace(const bool destroyIfEmpty = true)
    {
        if (!m_itemCount)
        {
            if (destroyIfEmpty)
                Destroy();

            return;
        }

        T* buffer = new(std::nothrow) T[m_itemCount];
        if (!buffer)
            return;

        if (m_elements)
        {
            int i;
            for (i = 0; i < m_itemCount; i++)
                buffer[i] = m_elements[i];

            delete[] m_elements;
        }

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
    inline T Pop(void)
    {
        const T element = m_elements[m_itemCount - 1];
        RemoveAt(m_itemCount - 1);
        return element;
    }

    //
    // Function: PopNoReturn
    //  Pops element from array.
    //
    inline T PopNoReturn(void)
    {
        RemoveAt(m_itemCount - 1);
    }

    inline T& Last(void)
    {
        return m_elements[m_itemCount - 1];
    }

    inline bool GetLast(const T& item)
    {
        if (!m_itemCount)
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
    inline bool AssignFrom(const Array <T>& other)
    {
        if (&other == this)
            return true;

        if (!SetSize(other.m_itemCount, false))
            return false;

        if (!other.m_elements)
            return false;

        int i;
        for (i = 0; i < other.m_itemCount; i++)
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
    inline T& GetRandomElement(void) const
    {
        return m_elements[crandomint(0, m_itemCount - 1)];
    }

    inline Array <T>& operator = (const Array <T>& other)
    {
        AssignFrom(other);
        return *this;
    }

    inline T& operator [] (const int index)
    {
        if (index < m_itemSize && index >= m_itemCount)
            m_itemCount = index + 1;

        return GetAt(index);
    }
};

//
// Class: String
//  Reference counted string class.
//
class String
{
protected:
    char* m_buffer;
    int m_used;
    int m_allocated;
private:
    inline bool IsTrimmingCharacter(char chr)
    {
        return chr == ' ' || chr == '\n' || chr == '\t' || chr == '\r' || chr == '\v' || chr == '\f';
    }

    inline void MoveItems(const int destIndex, const int srcIndex)
    {
        if (m_buffer)
            memmove(m_buffer + destIndex, m_buffer + srcIndex, sizeof(char) * (m_used - srcIndex + 1));
    }

    inline void InsertSpace(int& index, const int size)
    {
        CorrectIndex(index);
        GrowLength(size);
        MoveItems(index + size, index);
    }

    inline void SetCapacity(const int newCapacity)
    {
        int realCapacity = newCapacity + 1;
        if (realCapacity == m_allocated)
            return;

        char* newBuffer = new(std::nothrow) char[realCapacity];
        if (!newBuffer)
            return;

        if (m_allocated && m_buffer)
        {
            int i;
            for (i = 0; i < m_used; i++)
                newBuffer[i] = m_buffer[i];

            delete[] m_buffer;
        }

        m_buffer = newBuffer;
        m_buffer[m_used] = 0;
        m_allocated = realCapacity;
    }

    inline void GrowLength(const int howMany)
    {
        const int freeSize = m_allocated - m_used - 1;
        if (howMany <= freeSize)
            return;

        int delta = 4;
        if (m_allocated > 64)
            delta = m_allocated / 2;
        else if (m_allocated > 8)
            delta = 16;

        if (freeSize + delta < howMany)
            delta = howMany - freeSize;

        SetCapacity(m_allocated + delta);
    }

    inline void CorrectIndex(int& index) const
    {
        if (index > m_used)
            index = m_used;
    }

public:
    inline String(void) : m_buffer(nullptr), m_used(0), m_allocated(0)
    {
        SetCapacity(3);
    }

    inline String(char chr) : m_buffer(nullptr), m_used(0), m_allocated(0)
    {
        SetCapacity(1);
        if (m_buffer)
        {
            m_buffer[0] = chr;
            m_buffer[1] = 0;
        }
        m_used = 1;
    }

    inline String(char* str) : m_buffer(nullptr), m_used(0), m_allocated(0)
    {
        if (str)
        {
            const int length = strlen(str);
            SetCapacity(length);
            if (m_buffer)
            {
                strcpy(m_buffer, str);
                m_used = length;
            }
        }
    }

    inline String(const char* str) : m_buffer(nullptr), m_used(0), m_allocated(0)
    {
        if (str)
        {
            const int length = strlen(str);
            SetCapacity(length);
            if (m_buffer)
            {
                strcpy(m_buffer, str);
                m_used = length;
            }
        }
    }

    inline String(const String& other) : m_buffer(nullptr), m_used(0), m_allocated(0)
    {
        SetCapacity(other.m_used);
        if (m_buffer && other.m_buffer)
        {
            strcpy(m_buffer, other.m_buffer);
            m_used = other.m_used;
        }
    }

    inline ~String(void)
    {
        Destroy();
    }

    inline operator const char* (void) const
    {
        return m_buffer;
    }

    inline operator char* (void)
    {
        return m_buffer;
    }

    inline operator const double(void) const
    {
        if (m_buffer)
            return static_cast<double>(atof(m_buffer));
        
        return 0.0;
    }

    inline operator double(void)
    {
        if (m_buffer)
            return static_cast<double>(atof(m_buffer));

        return 0.0;
    }

    inline operator const float(void) const
    {
        if (m_buffer)
            return atof(m_buffer);

        return 0.0f;
    }

    inline operator float(void)
    {
        if (m_buffer)
            return atof(m_buffer);

        return 0.0f;
    }

    inline operator const int(void) const
    {
        if (m_buffer)
            return atoi(m_buffer);

        return 0;
    }

    inline operator int(void)
    {
        if (m_buffer)
            return atoi(m_buffer);

        return 0;
    }

    inline operator const long(void) const
    {
        if (m_buffer)
            return static_cast<long>(atoi(m_buffer));

        return 0;
    }

    inline operator long(void)
    {
        if (m_buffer)
            return static_cast<long>(atoi(m_buffer));

        return 0;
    }

    inline void Destroy(void)
    {
        if (m_buffer)
        {
            delete[] m_buffer;
            m_buffer = nullptr;
        }
    }

    inline char* GetRawData(void)
    {
        return m_buffer;
    }

    inline const char* GetRawData(void) const
    {
        return m_buffer;
    }

    inline const char* GetBuffer(const int minBufLength)
    {
        if (minBufLength >= m_allocated)
            SetCapacity(minBufLength);

        return m_buffer;
    }

    inline void ReleaseBuffer(void)
    {
        if (m_buffer)
            ReleaseBuffer(strlen(m_buffer));
    }

    inline void ReleaseBuffer(const int newLength)
    {
        if (m_buffer)
        {
            m_buffer[newLength] = 0;
            m_used = newLength;
        }
    }

    inline String& operator = (char chr)
    {
        SetEmpty();
        SetCapacity(1);

        if (m_buffer)
        {
            m_buffer[0] = chr;
            m_buffer[1] = 0;
        }

        m_used = 1;
        return *this;
    }

    inline String& operator = (const char* str)
    {
        if (!str)
            return *this;

        SetEmpty();
        const int length = strlen(str);
        SetCapacity(length);

        if (m_buffer)
        {
            strcpy(m_buffer, str);
            m_used = length;
        }
        
        return *this;
    }

    inline String& operator = (const String& other)
    {
        if (&other == this)
            return *this;

        SetEmpty();
        SetCapacity(other.m_used);

        if (m_buffer && other.m_buffer)
        {
            strcpy(m_buffer, other.m_buffer);
            m_used = other.m_used;
        }

        return *this;
    }

    inline String& operator += (char chr)
    {
        GrowLength(1);

        if (m_buffer)
        {
            m_buffer[m_used] = chr;
            m_buffer[++m_used] = 0;
        }

        return *this;
    }

    inline String& operator += (const char* str)
    {
        if (!str)
            return *this;

        const int length = strlen(str);
        GrowLength(length);

        if (m_buffer)
        {
            strcpy(m_buffer + m_used, str);
            m_used += length;
        }
        
        return *this;
    }

    inline String& operator += (const String& other)
    {
        GrowLength(other.m_used);

        if (m_buffer && other.m_buffer)
        {
            strcpy(m_buffer + m_used, other.m_buffer);
            m_used += other.m_used;
        }
        
        return *this;
    }

    friend inline String operator + (const String& str1, const String& str2)
    {
        String result(str1);
        result += str2;
        return result;
    }

    friend inline String operator + (const String& str, char chr)
    {
        String result(str);
        result += chr;
        return result;
    }

    friend inline String operator + (char chr, const String& str)
    {
        String result(chr);
        result += str;
        return result;
    }

    friend inline String operator + (const String& str1, const char* str2)
    {
        String result(str1);
        result += str2;
        return result;
    }

    friend inline String operator + (const char* str1, const String& str2)
    {
        String result(str1);
        result += str2;
        return result;
    }

    friend inline bool operator == (const String& str1, const String& str2)
    {
        return str1.Compare(str2) == 0;
    }

    friend inline bool operator < (const String& str1, const String& str2)
    {
        return str1.Compare(str2) < 0;
    }

    friend inline bool operator == (const char* str1, const String& str2)
    {
        return str2.Compare(str1) == 0;
    }

    friend inline bool operator == (const String& str1, const char* str2)
    {
        return str1.Compare(str2) == 0;
    }

    friend inline bool operator != (const String& str1, const String& str2)
    {
        return str1.Compare(str2) != 0;
    }

    friend inline bool operator != (const char* str1, const String& str2)
    {
        return str2.Compare(str1) != 0;
    }

    friend inline bool operator != (const String& str1, const char* str2)
    {
        return str1.Compare(str2) != 0;
    }

    inline void SetEmpty(void)
    {
        m_used = 0;
        if (m_buffer)
            m_buffer[0] = 0;
    }

    inline int GetLength(void) const
    {
        return m_used;
    }

    inline bool IsEmpty(void) const
    {
        return !m_used;
    }

    inline String Mid(const int startIndex) const
    {
        return Mid(startIndex, m_used - startIndex);
    }

    inline String Mid(const int startIndex, int count) const
    {
        if (startIndex + count > m_used)
            count = m_used - startIndex;

        if (startIndex == 0 && startIndex + count == m_used)
            return *this;

        String result;
        result.SetCapacity(count);

        if (m_buffer && result.m_buffer)
        {
            int i;
            for (i = 0; i < count; i++)
                result.m_buffer[i] = m_buffer[startIndex + i];

            result.m_buffer[count] = 0;
            result.m_used = count;
        }

        return result;
    }

    inline String Left(const int count) const
    {
        return Mid(0, count);
    }

    inline String Right(int count) const
    {
        if (count > m_used)
            count = m_used;

        return Mid(m_used - count, count);
    }

    inline String& MakeUpper(void)
    {
        if (!m_buffer)
            return *this;

        char* ptr = m_buffer;
        while (*ptr)
        {
            *ptr = static_cast<char>(toupper(*ptr));
            ptr++;
        }

        return *this;
    }

    inline String& MakeLower(void)
    {
        if (!m_buffer)
            return *this;

        char* ptr = m_buffer;
        while (*ptr)
        {
            *ptr = static_cast<char>(tolower(*ptr));
            ptr++;
        }

        return *this;
    }

    inline int Compare(const String& str) const
    {
        if (m_buffer && str.m_buffer)
            return strcmp(m_buffer, str.m_buffer);

        return 0;
    }

    inline int Compare(const char* str) const
    {
        if (m_buffer && str)
            return strcmp(m_buffer, str);

        return 0;
    }
    
    inline bool Has(const String& other) const
    {
        if (m_buffer && other.GetRawData())
            return strstr(m_buffer, const_cast<char*>(other.GetRawData()));

        return false;
    }

    inline int Find(const char chr) const
    {
        return Find(chr, 0);
    }

    inline int Find(const char chr, const int startIndex) const
    {
        if (!m_buffer)
            return -1;

        char* ptr = m_buffer + startIndex;
        for (; ;)
        {
            if (*ptr == chr)
                return static_cast<int>(ptr - m_buffer);

            if (!*ptr)
                return -1;

            ptr++;
        }
    }

    inline int Find(const String& str) const
    {
        return Find(str, 0);
    }

    inline int Find(const String& str, int startIndex) const
    {
        if (str.IsEmpty())
            return startIndex;

        if (m_buffer && str.m_buffer)
        {
            for (; startIndex < m_used; startIndex++)
            {
                int j;
                for (j = 0; j < str.m_used && startIndex + j < m_used; j++)
                {
                    if (m_buffer[startIndex + j] != str.m_buffer[j])
                        break;
                }

                if (j == str.m_used)
                    return startIndex;
            }
        }
        
        return -1;
    }

    inline int ReverseFind(char chr) const
    {
        if (!m_buffer)
            return -1;

        char* ptr = m_buffer + m_used - 1;
        for (; ;)
        {
            if (*ptr == chr)
                return static_cast<int>(ptr - m_buffer);

            if (ptr == m_buffer)
                return -1;

            ptr--;
        }

        return -1;
    }

    inline int FindOneOf(const String& str) const
    {
        if (m_buffer)
        {
            int i;
            for (i = 0; i < m_used; i++)
            {
                if (str.Find(m_buffer[i]) >= 0)
                    return i;
            }
        }
        
        return -1;
    }

    inline String& TrimLeft(char chr)
    {
        if (!m_buffer)
            return *this;

        char* ptr = m_buffer;
        while (chr == *ptr)
            ptr++;

        Delete(0, ptr - m_buffer);
        return *this;
    }

    inline String& TrimRight(char chr)
    {
        if (!m_buffer)
            return *this;

        char* ptr = m_buffer;
        char* last = nullptr;

        while (*ptr)
        {
            if (*ptr == chr)
            {
                if (!last)
                    last = ptr;
            }
            else
                last = nullptr;

            ptr++;
        }

        if (last)
        {
            const int diff = static_cast<int>(last - m_buffer);
            Delete(diff, m_used - diff);
        }

        return *this;
    }


    inline String& TrimLeft(void)
    {
        if (!m_buffer)
            return *this;

        int index = 0;
        for (; index < m_used && IsTrimmingCharacter(m_buffer[index]); index++)
            Delete(0, index);

        return *this;
    }

    inline String& TrimRight(void)
    {
        if (!m_buffer)
            return *this;

        int srcIndex = m_used - 1, destIndex = 0;
        for (; srcIndex < m_used && IsTrimmingCharacter(m_buffer[srcIndex]); srcIndex--, destIndex++)
            Delete(srcIndex + 1, destIndex);

        return *this;
    }

    inline String& Trim(void)
    {
        return TrimRight().TrimLeft();
    }

    inline String& TrimQuotes(void)
    {
        return TrimRight('\'').TrimRight('\"').TrimLeft('\'').TrimLeft('\"');
    }

    inline int Insert(int index, const char chr)
    {
        InsertSpace(index, 1);

        if (m_buffer)
        {
            m_buffer[index] = chr;
            m_used++;
        }

        return m_used;
    }

    inline int Insert(int index, const String& str)
    {
        CorrectIndex(index);

        if (str.IsEmpty())
            return m_used;

        int numInsertChars = str.GetLength();
        InsertSpace(index, numInsertChars);

        if (m_buffer)
        {
            int i;
            for (i = 0; i < numInsertChars; i++)
                m_buffer[index + i] = str[i];

            m_used += numInsertChars;
        }
        
        return m_used;
    }

    inline int Replace(const char oldChar, const char newChar)
    {
        if (!m_buffer || oldChar == newChar)
            return 0;

        int number = 0;
        int pos = 0;
        while (pos < GetLength())
        {
            pos = Find(oldChar, pos);
            if (pos < 0)
                break;

            m_buffer[pos] = newChar;
            pos++;
            number++;
        }

        return number;
    }

    inline int Replace(const String& oldString, const String& newString)
    {
        if (oldString.IsEmpty() || oldString == newString)
            return 0;

        const int oldStringLength = oldString.GetLength();
        const int newStringLength = newString.GetLength();

        int number = 0;
        int pos = 0;

        while (pos < m_used)
        {
            pos = Find(oldString, pos);

            if (pos < 0)
                break;

            Delete(pos, oldStringLength);
            Insert(pos, newString);

            pos += newStringLength;
            number++;
        }

        return number;
    }

    inline int Delete(const int index, int count = 1)
    {
        if (index + count > m_used)
            count = m_used - index;

        if (count)
        {
            MoveItems(index, index + count);
            m_used -= count;
        }

        return m_used;
    }

    Array <String> Split(char* separator)
    {
        Array <String> holder;
        int tokenLength, index = 0;
        do
        {
            index += strspn(&m_buffer[index], separator);
            tokenLength = strcspn(&m_buffer[index], separator);
            if (tokenLength > 0)
                holder.Push(Mid(index, tokenLength));
            index += tokenLength;
        } while (tokenLength > 0);
        return holder;
    }

    Array <String> Split(const char separator)
    {
        char sep[2] = {separator, 0x0};
        return Split(sep);
    }
};

//
// Class: File
//  Simple STDIO file wrapper class.
//
class File
{
protected:
    FILE* m_handle;
    int m_fileSize;

    //
    // Group: (Con/De)structors
    //
public:

    //
    // Function: File
    //  Default file class, constructor.
    //
    File(void)
    {
        m_handle = nullptr;
        m_fileSize = 0;
    }

    //
    // Function: File
    //  Default file class, constructor, with file opening.
    //
    File(String fileName, String mode = "rt")
    {
        Open(fileName, mode);
    }

    //
    // Function: ~File
    //  Default file class, destructor.
    //
    ~File(void)
    {
        Close();
    }

    //
    // Function: Open
    //  Opens file and gets it's size.
    //
    // Parameters:
    //  fileName - String containing file name.
    //  mode - String containing open mode for file.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool Open(const String& fileName, const String& mode)
    {
        if (!(m_handle = fopen(fileName, mode)))
            return false;

        fseek(m_handle, 0L, SEEK_END);
        m_fileSize = ftell(m_handle);
        fseek(m_handle, 0L, SEEK_SET);

        return true;
    }

    //
    // Function: Close
    //  Closes file, and destroys STDIO file object.
    //
    void Close(void)
    {
        if (m_handle)
        {
            fclose(m_handle);
            m_handle = nullptr;
        }

        m_fileSize = 0;
    }

    //
    // Function: Eof
    //  Checks whether we reached end of file.
    //
    // Returns:
    //  True if reached, false otherwise.
    //
    bool Eof(void)
    {
        return feof(m_handle) ? true : false;
    }

    //
    // Function: Flush
    //  Flushes file stream.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool Flush(void)
    {
        return fflush(m_handle) ? false : true;
    }

    //
    // Function: GetChar
    //  Pops one character from the file stream.
    //
    // Returns:
    //  Popped from stream character
    //
    int GetCharacter(void)
    {
        return fgetc(m_handle);
    }

    //
    // Function: GetBuffer
    //  Gets the single line, from the non-binary stream.
    //
    // Parameters:
    //  buffer - Pointer to buffer, that should receive string.
    //  count - Max. size of buffer.
    //
    // Returns:
    //  Pointer to string containing popped line.
    //
    char* GetBuffer(char* buffer, const int count)
    {
        return fgets(buffer, count, m_handle);
    }

    //
    // Function: GetBuffer
    //  Gets the line from file stream, and stores it inside string class.
    //
    // Parameters:
    //  buffer - String buffer, that should receive line.
    //  count - Max. size of buffer.
    //
    // Returns:
    //  True if operation succeeded, false otherwise.
    //
    bool GetBuffer(String& buffer, const int count)
    {
        return !String(fgets(buffer, count, m_handle)).IsEmpty();
    }

    //
    // Function: Print
    //  Puts formatted buffer, into stream.
    //
    // Parameters:
    //  format - 
    //
    // Returns:
    //  Number of bytes, that was written.
    //
    int Printf(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        const int written = vfprintf(m_handle, format, ap);
        va_end(ap);

        if (written < 0)
            return 0;

        return written;
    }

    //
    // Function: PutCharacter
    //  Puts character into file stream.
    //
    // Parameters:
    //  ch - Character that should be put into stream.
    //
    // Returns:
    //  Character that was putted into the stream.
    //
    char PutCharacter(const char ch)
    {
        return static_cast<char>(fputc(ch, m_handle));
    }

    //
    // Function: PutString
    //  Puts buffer into the file stream.
    //
    // Parameters:
    //  buffer - Buffer that should be put, into stream.
    //
    // Returns:
    //  True, if operation succeeded, false otherwise.
    //
    bool PutString(const String buffer)
    {
        if (fputs(buffer, m_handle) < 0)
            return false;

        return true;
    }

    //
    // Function: Read
    //  Reads buffer from file stream in binary format.
    //
    // Parameters:
    //  buffer - Holder for read buffer.
    //  size - Size of the buffer to read.
    //  count - Number of buffer chunks to read.
    //
    // Returns:
    //  Number of bytes red from file.
    //
    int Read(void* buffer, const int size, const int count = 1)
    {
        return fread(buffer, size, count, m_handle);
    }

    //
    // Function: Write
    //  Writes binary buffer into file stream.
    //
    // Parameters:
    //  buffer - Buffer holder, that should be written into file stream.
    //  size - Size of the buffer that should be written.
    //  count - Number of buffer chunks to write.
    //
    // Returns:
    //  Numbers of bytes written to file.
    //
    int Write(void* buffer, const int size, const int count = 1)
    {
        return fwrite(buffer, size, count, m_handle);
    }

    //
    // Function: Seek
    //  Seeks file stream with specified parameters.
    //
    // Parameters:
    //  offset - Offset where cursor should be set.
    //  origin - Type of offset set.
    //
    // Returns:
    //  True if operation success, false otherwise.
    //
    bool Seek(const long offset, const int origin)
    {
        if (fseek(m_handle, offset, origin) != 0)
            return false;

        return true;
    }

    //
    // Function: Rewind
    //  Rewinds the file stream.
    //
    void Rewind(void)
    {
        rewind(m_handle);
    }

    //
    // Function: GetSize
    //  Gets the file size of opened file stream.
    //
    // Returns:
    //  Number of bytes in file.
    //
    int GetSize(void)
    {
        return m_fileSize;
    }

    //
    // Function: IsValid
    //  Checks whether file stream is valid.
    //
    // Returns:
    //  True if file stream valid, false otherwise.
    //
    bool IsValid(void)
    {
        return m_handle;
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
        const tm* time = localtime(&tick);
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
      const int flags = m_logger->GetFlags (); \
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
      m_logFile.Printf ("[%s] (%s): %s\n", GetTimeFormatString (), logStr, buffer); \
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
public:
    uint8_t red, green, blue, alpha;
public:
    inline Color(const uint8_t color = 0) : red(color), green(color), blue(color), alpha(color) {}
    inline Color(uint8_t inputRed, uint8_t inputGreen, uint8_t inputBlue, uint8_t inputAlpha = static_cast<uint8_t>(0)) : red(inputRed), green(inputGreen), blue(inputBlue), alpha(inputAlpha) {}
    inline Color(const Color& right) : red(right.red), green(right.green), blue(right.blue), alpha(right.alpha) {}
public:
    inline bool operator == (const Color& right) const { return red == right.red && green == right.green && blue == right.blue && alpha == right.alpha; }
    inline bool operator != (const Color& right) const { return !operator == (right); }
    inline uint8_t& operator [] (uint8_t colourIndex) { return (&red)[colourIndex]; }
    inline const uint8_t& operator [] (uint8_t colourIndex) const { return (&red)[colourIndex]; }
    inline const Color operator / (uint8_t scaler) const { return Color(red / scaler, green / scaler, blue / scaler, alpha / scaler); }
};

#endif // RUNTIME_INCLUDED
