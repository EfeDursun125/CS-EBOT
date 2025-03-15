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
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
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
    inline const Vector operator/ (const float vec) const
    {
        if (Math::FltZero(vec))
            return Vector(x, y, z);

        const float inv = 1.0f / vec;
        return Vector(inv * x, inv * y, inv * z);
    }
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
        if (!Math::FltZero(vec))
        {
            const float inv = 1.0f / vec;
            x *= inv;
            y *= inv;
            z *= inv;
        }
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
        const float length = crsqrtf(x * x + y * y + z * z) + MATH_FLEPSILON;
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
        const float length = crsqrtf(x * x + y * y) + MATH_FLEPSILON;
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
        if (Math::FltZero(x) && Math::FltZero(y))
            return 0.0f;

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
        // is the input vector absolutely vertical?
        if (Math::FltZero(x) && Math::FltZero(y))
            return Vector(z > 0.0f ? 90.0f : 270.0f, 0.0, 0.0f);

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

#include <xmmintrin.h>
inline float GetVectorDistanceSSE(const Vector vec1, const Vector vec2)
{
    const __m128 v1 = _mm_set_ps(0.0f, vec1.z, vec1.y, vec1.x);
    const __m128 v2 = _mm_set_ps(0.0f, vec2.z, vec2.y, vec2.x);
    const __m128 diff = _mm_sub_ps(v1, v2);
    const __m128 squared = _mm_mul_ps(diff, diff);
    __m128 sum = _mm_add_ps(squared, squared);
    sum = _mm_add_ps(sum, _mm_movehl_ps(sum, sum));
    sum = _mm_add_ss(sum, _mm_shuffle_ps(sum, sum, 0x55));
    return _mm_cvtss_f32(_mm_sqrt_ss(sum));
}

namespace Math
{
    inline bool BBoxIntersects(const Vector& min1, const Vector& max1, const Vector& min2, const Vector& max2)
    {
        return min1.x < max2.x && max1.x > min2.x && min1.y < max2.y && max1.y > min2.y && min1.z < max2.z && max1.z > min2.z;
    }
}

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
    File(const char* fileName, const char* mode)
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
    bool Open(const char* fileName, const char* mode)
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
    bool PutString(const char* buffer)
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
        if (fseek(m_handle, offset, origin))
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
        cmemset(timeFormatStr, 0, sizeof(char) * 32);
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
