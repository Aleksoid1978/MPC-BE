/*-
 * Copyright (c) 2005 Boris Mikhaylov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BS2BTYPES_H
#define BS2BTYPES_H

#include <sys/types.h>
#include <limits.h>

#ifndef _INT8_T_DECLARED
typedef signed   char    int8_t;
#endif

#ifndef _UINT8_T_DECLARED
typedef unsigned char    uint8_t;
#endif

#ifndef _INT16_T_DECLARED
typedef signed   short   int16_t;
#endif

#ifndef _UINT16_T_DECLARED
typedef unsigned short   uint16_t;
#endif

#ifndef _INT32_T_DECLARED
 #if UINT_MAX == 0xffff /* 16 bit compiler */
 typedef signed   long    int32_t;
 #else /* UINT_MAX != 0xffff */ /* 32/64 bit compiler */
 typedef signed   int     int32_t;
 #endif
#endif /* !_INT32_T_DECLARED */

#ifndef _UINT32_T_DECLARED
 #if UINT_MAX == 0xffff /* 16 bit compiler */
 typedef unsigned long    uint32_t;
 #else /* UINT_MAX != 0xffff */ /* 32/64 bit compiler */
 typedef unsigned int     uint32_t;
 #endif
#endif /* !_UINT32_T_DECLARED */

typedef struct
{
	uint8_t octet0;
	uint8_t octet1;
	int8_t  octet2;
} bs2b_int24_t;

typedef struct
{
	uint8_t octet0;
	uint8_t octet1;
	uint8_t octet2;
} bs2b_uint24_t;

#endif	/* BS2BTYPES_H */
