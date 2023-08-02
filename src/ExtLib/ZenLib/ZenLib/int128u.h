/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// based on http://Tringi.Mx-3.cz
// Only adapted for ZenLib:
// - .hpp --> .h
// - Namespace
// - int128u alias
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifndef UINT128_HPP
#define UINT128_HPP

/*
  Name: uint128.hpp
  Copyright: Copyright (C) 2005, Jan Ringos
  Author: Jan Ringos, http://Tringi.Mx-3.cz

  Version: 1.1
*/

#include "ZenLib/Conf.h"

// CLASS

namespace ZenLib
{

class uint128 {
    public://private:
        // Binary correct representation of signed 128bit integer
        int64u lo;
        int64u hi;

    protected:
        // Some global operator functions must be friends
        friend bool operator <  (const uint128 &, const uint128 &) noexcept;
        friend bool operator == (const uint128 &, const uint128 &) noexcept;
        friend bool operator || (const uint128 &, const uint128 &) noexcept;
        friend bool operator && (const uint128 &, const uint128 &) noexcept;

    public:
        // Constructors
        inline uint128 () noexcept : lo(0), hi(0) {};
        inline uint128 (const uint128 & a) noexcept : lo (a.lo), hi (a.hi) {};

        inline uint128 (const int & a) noexcept : lo (a), hi (0ull) {};
        inline uint128 (const unsigned int & a) noexcept : lo (a), hi (0ull) {};
        inline uint128 (const int64u & a) noexcept : lo (a), hi (0ull) {};

        uint128 (const float a) noexcept;
        uint128 (const double & a) noexcept;
        uint128 (const long double & a) noexcept;

        uint128 (const char * sz) noexcept;

        // TODO: Consider creation of operator= to eliminate
        //       the need of intermediate objects during assignments.

    private:
        // Special internal constructors
        uint128 (const int64u & a, const int64u & b) noexcept
            : lo (a), hi (b) {};

    public:
        // Operators
        bool operator ! () const noexcept;

        uint128 operator - () const noexcept;
        uint128 operator ~ () const noexcept;

        uint128 & operator ++ ();
        uint128 & operator -- ();
        uint128 operator ++ (int);
        uint128 operator -- (int);

        uint128 & operator += (const uint128 & b) noexcept;
        uint128 & operator *= (const uint128 & b) noexcept;

        uint128 & operator >>= (unsigned int n) noexcept;
        uint128 & operator <<= (unsigned int n) noexcept;

        uint128 & operator |= (const uint128 & b) noexcept;
        uint128 & operator &= (const uint128 & b) noexcept;
        uint128 & operator ^= (const uint128 & b) noexcept;

        // Inline simple operators
        inline const uint128 & operator + () const noexcept { return *this; }

        // Rest of inline operators
        inline uint128 & operator -= (const uint128 & b) noexcept {
            return *this += (-b);
        };
        inline uint128 & operator /= (const uint128 & b) noexcept {
            uint128 dummy;
            *this = this->div (b, dummy);
            return *this;
        };
        inline uint128 & operator %= (const uint128 & b) noexcept {
            this->div (b, *this);
            return *this;
        };

        // Common methods
        unsigned int toUint () const noexcept {
            return (unsigned int) this->lo; }
        int64u toUint64 () const noexcept {
            return (int64u) this->lo; }
        const char * toString (unsigned int radix = 10) const noexcept;
        float toFloat () const noexcept;
        double toDouble () const noexcept;
        long double toLongDouble () const noexcept;

        // Arithmetic methods
        uint128  div (const uint128 &, uint128 &) const noexcept;

        // Bit operations
        bool    bit (unsigned int n) const noexcept;
        void    bit (unsigned int n, bool val) noexcept;
}
#if defined(__GNUC__) && !defined(__ANDROID_API__)
        __attribute__ ((__aligned__ (16), __packed__))
#endif
;


// GLOBAL OPERATORS

bool operator <  (const uint128 & a, const uint128 & b) noexcept;
bool operator == (const uint128 & a, const uint128 & b) noexcept;
bool operator || (const uint128 & a, const uint128 & b) noexcept;
bool operator && (const uint128 & a, const uint128 & b) noexcept;

// GLOBAL OPERATOR INLINES

inline uint128 operator + (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) += b; }
inline uint128 operator - (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) -= b; }
inline uint128 operator * (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) *= b; }
inline uint128 operator / (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) /= b; }
inline uint128 operator % (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) %= b; }

inline uint128 operator >> (const uint128 & a, unsigned int n) noexcept {
    return uint128 (a) >>= n; }
inline uint128 operator << (const uint128 & a, unsigned int n) noexcept {
    return uint128 (a) <<= n; }

inline uint128 operator & (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) &= b; }
inline uint128 operator | (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) |= b; }
inline uint128 operator ^ (const uint128 & a, const uint128 & b) noexcept {
    return uint128 (a) ^= b; }

inline bool operator >  (const uint128 & a, const uint128 & b) noexcept {
    return   b < a; }
inline bool operator <= (const uint128 & a, const uint128 & b) noexcept {
    return !(b < a); }
inline bool operator >= (const uint128 & a, const uint128 & b) noexcept {
    return !(a < b); }
inline bool operator != (const uint128 & a, const uint128 & b) noexcept {
    return !(a == b); }


// MISC

typedef uint128 __uint128;

typedef uint128 int128u;
} //NameSpace

#endif
