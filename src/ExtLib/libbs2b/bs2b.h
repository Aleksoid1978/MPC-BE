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

#ifndef BS2B_H
#define BS2B_H

#include "bs2bversion.h"
#include "bs2btypes.h"

/* Minimum/maximum sample rate (Hz) */
#define BS2B_MINSRATE 2000
#define BS2B_MAXSRATE 384000

/* Minimum/maximum cut frequency (Hz) */
/* bs2b_set_level_fcut() */
#define BS2B_MINFCUT 300
#define BS2B_MAXFCUT 2000

/* Minimum/maximum feed level (dB * 10 @ low frequencies) */
/* bs2b_set_level_feed() */
#define BS2B_MINFEED 10   /* 1 dB */
#define BS2B_MAXFEED 150  /* 15 dB */

/* Normal crossfeed levels (Obsolete) */
#define BS2B_HIGH_CLEVEL     ( ( uint32_t )700 | ( ( uint32_t )30 << 16 ) )
#define BS2B_MIDDLE_CLEVEL   ( ( uint32_t )500 | ( ( uint32_t )45 << 16 ) )
#define BS2B_LOW_CLEVEL      ( ( uint32_t )360 | ( ( uint32_t )60 << 16 ) )

/* Easy crossfeed levels (Obsolete) */
#define BS2B_HIGH_ECLEVEL    ( ( uint32_t )700 | ( ( uint32_t )60 << 16 ) )
#define BS2B_MIDDLE_ECLEVEL  ( ( uint32_t )500 | ( ( uint32_t )72 << 16 ) )
#define BS2B_LOW_ECLEVEL     ( ( uint32_t )360 | ( ( uint32_t )84 << 16 ) )

/* Default crossfeed levels */
/* bs2b_set_level() */
#define BS2B_DEFAULT_CLEVEL  ( ( uint32_t )700 | ( ( uint32_t )45 << 16 ) )
#define BS2B_CMOY_CLEVEL     ( ( uint32_t )700 | ( ( uint32_t )60 << 16 ) )
#define BS2B_JMEIER_CLEVEL   ( ( uint32_t )650 | ( ( uint32_t )95 << 16 ) )

/* Default sample rate (Hz) */
#define BS2B_DEFAULT_SRATE   44100

/* A delay at low frequency by microseconds according to cut frequency */
#define bs2b_level_delay( fcut ) ( ( 18700 / fcut ) * 10 )

typedef struct
{
	uint32_t level;              /* Crossfeed level */
	uint32_t srate;              /* Sample rate (Hz) */
	double a0_lo, b1_lo;         /* Lowpass IIR filter coefficients */
	double a0_hi, a1_hi, b1_hi;  /* Highboost IIR filter coefficients */
	double gain;                 /* Global gain against overloading */
	/* Buffer of last filtered sample: [0] 1-st channel, [1] 2-d channel */
	struct { double asis[ 2 ], lo[ 2 ], hi[ 2 ]; } lfs;
} t_bs2bd;

typedef t_bs2bd *t_bs2bdp;

#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */

/* Allocates and sets a data to defaults.
 * Return NULL on error.
 */
t_bs2bdp bs2b_open( void );

/* Close */
void bs2b_close( t_bs2bdp bs2bdp );

/* Sets a new coefficients by new crossfeed value.
 * level = ( ( uint32_t )fcut | ( ( uint32_t )feed << 16 ) )
 * where 'feed' is crossfeeding level at low frequencies (dB * 10)
 * and 'fcut' is cut frecuency (Hz)
 */
void bs2b_set_level( t_bs2bdp bs2bdp, uint32_t level );

/* Return a current crossfeed level value. */
uint32_t bs2b_get_level( t_bs2bdp bs2bdp );

/* Sets a new coefficients by new cut frecuency value (Hz). */
void bs2b_set_level_fcut( t_bs2bdp bs2bdp, int fcut );

/* Return a current cut frecuency value (Hz). */
int bs2b_get_level_fcut( t_bs2bdp bs2bdp );

/* Sets a new coefficients by new crossfeeding level value (dB * 10). */
void bs2b_set_level_feed( t_bs2bdp bs2bdp, int feed );

/* Return a current crossfeeding level value (dB * 10). */
int bs2b_get_level_feed( t_bs2bdp bs2bdp );

/* Return a current delay value at low frequencies (micro seconds). */
int bs2b_get_level_delay( t_bs2bdp bs2bdp );

/* Clear buffers and sets a new coefficients with new sample rate value.
 * srate - sample rate by Hz.
 */
void bs2b_set_srate( t_bs2bdp bs2bdp, uint32_t srate );

/* Return current sample rate value */
uint32_t bs2b_get_srate( t_bs2bdp bs2bdp );

/* Clear buffer */
void bs2b_clear( t_bs2bdp bs2bdp );

/* Return 1 if buffer is clear */
int bs2b_is_clear( t_bs2bdp bs2bdp );

/* Return bs2b version string */
char const *bs2b_runtime_version( void );

/* Return bs2b version integer */
uint32_t bs2b_runtime_version_int( void );

/* 'bs2b_cross_feed_*' crossfeeds buffer of 'n' stereo samples
 * pointed by 'sample'.
 * sample[i]   - first channel,
 * sample[i+1] - second channel.
 * Where 'i' is ( i = 0; i < n * 2; i += 2 )
 */

/* sample poits to double floats native endians */
void bs2b_cross_feed_d( t_bs2bdp bs2bdp, double *sample, int n );

/* sample poits to double floats big endians */
void bs2b_cross_feed_dbe( t_bs2bdp bs2bdp, double *sample, int n );

/* sample poits to double floats little endians */
void bs2b_cross_feed_dle( t_bs2bdp bs2bdp, double *sample, int n );

/* sample poits to floats native endians */
void bs2b_cross_feed_f( t_bs2bdp bs2bdp, float *sample, int n );

/* sample poits to floats big endians */
void bs2b_cross_feed_fbe( t_bs2bdp bs2bdp, float *sample, int n );

/* sample poits to floats little endians */
void bs2b_cross_feed_fle( t_bs2bdp bs2bdp, float *sample, int n );

/* sample poits to 32bit signed integers native endians */
void bs2b_cross_feed_s32( t_bs2bdp bs2bdp, int32_t *sample, int n );

/* sample poits to 32bit unsigned integers native endians */
void bs2b_cross_feed_u32( t_bs2bdp bs2bdp, uint32_t *sample, int n );

/* sample poits to 32bit signed integers big endians */
void bs2b_cross_feed_s32be( t_bs2bdp bs2bdp, int32_t *sample, int n );

/* sample poits to 32bit unsigned integers big endians */
void bs2b_cross_feed_u32be( t_bs2bdp bs2bdp, uint32_t *sample, int n );

/* sample poits to 32bit signed integers little endians */
void bs2b_cross_feed_s32le( t_bs2bdp bs2bdp, int32_t *sample, int n );

/* sample poits to 32bit unsigned integers little endians */
void bs2b_cross_feed_u32le( t_bs2bdp bs2bdp, uint32_t *sample, int n );

/* sample poits to 16bit signed integers native endians */
void bs2b_cross_feed_s16( t_bs2bdp bs2bdp, int16_t *sample, int n );

/* sample poits to 16bit unsigned integers native endians */
void bs2b_cross_feed_u16( t_bs2bdp bs2bdp, uint16_t *sample, int n );

/* sample poits to 16bit signed integers big endians */
void bs2b_cross_feed_s16be( t_bs2bdp bs2bdp, int16_t *sample, int n );

/* sample poits to 16bit unsigned integers big endians */
void bs2b_cross_feed_u16be( t_bs2bdp bs2bdp, uint16_t *sample, int n );

/* sample poits to 16bit signed integers little endians */
void bs2b_cross_feed_s16le( t_bs2bdp bs2bdp, int16_t *sample, int n );

/* sample poits to 16bit unsigned integers little endians */
void bs2b_cross_feed_u16le( t_bs2bdp bs2bdp, uint16_t *sample, int n );

/* sample poits to 8bit signed integers */
void bs2b_cross_feed_s8( t_bs2bdp bs2bdp, int8_t *sample, int n );

/* sample poits to 8bit unsigned integers */
void bs2b_cross_feed_u8( t_bs2bdp bs2bdp, uint8_t *sample, int n );

/* sample poits to 24bit signed integers native endians */
void bs2b_cross_feed_s24( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n );

/* sample poits to 24bit unsigned integers native endians */
void bs2b_cross_feed_u24( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n );

/* sample poits to 24bit signed integers be endians */
void bs2b_cross_feed_s24be( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n );

/* sample poits to 24bit unsigned integers be endians */
void bs2b_cross_feed_u24be( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n );

/* sample poits to 24bit signed integers little endians */
void bs2b_cross_feed_s24le( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n );

/* sample poits to 24bit unsigned integers little endians */
void bs2b_cross_feed_u24le( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n );

#ifdef __cplusplus
}	/* extern "C" */
#endif /* __cplusplus */

#endif	/* BS2B_H */
