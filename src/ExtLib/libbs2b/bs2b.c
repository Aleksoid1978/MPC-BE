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

#include <math.h>
#include <stdlib.h>
#include <memory.h>

#include "bs2b.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

static void int16swap( uint16_t *x )
{
	*x = ( *x >> 8 ) | ( *x << 8 );
}

static void int24swap( bs2b_uint24_t *x )
{
	uint8_t x1;

	x1 = x->octet0;
	x->octet0 = x->octet2;
	x->octet2 = x1;
}

static void int32swap( uint32_t *x )
{
	*x = ( *x >> 24 ) | ( ( *x >> 8 ) & 0xff00 ) |
		( ( *x << 8 ) & ( ( uint32_t )0xff << 16 ) ) | ( *x << 24 );
}

static void int64swap( uint32_t *x )
{
	uint32_t x1;

	x1 = *x;
	*x = x[ 1 ];
	x[ 1 ] = x1;

	int32swap( x );
	int32swap( x + 1 );
}

static double int242double( bs2b_int24_t *in )
{
	int32_t out =
		#ifdef WORDS_BIGENDIAN
		( ( uint32_t )in->octet2 ) |
		( ( uint32_t )in->octet1 << 8 ) |
		( ( uint32_t )in->octet0 << 16 ) |
		( ( in->octet0 < 0 ? ( uint32_t )-1 : 0 ) << 24 );
		#else
		( ( uint32_t )in->octet0 ) |
		( ( uint32_t )in->octet1 << 8 ) |
		( ( uint32_t )in->octet2 << 16 ) |
		( ( in->octet2 < 0 ? ( uint32_t )-1 : 0 ) << 24 );
		#endif /* WORDS_BIGENDIAN */

	return ( double )out;
} /* int242double() */

static double uint242double( bs2b_uint24_t *in )
{
	uint32_t out =
		#ifdef WORDS_BIGENDIAN
		( ( uint32_t )in->octet2 ) |
		( ( uint32_t )in->octet1 << 8 ) |
		( ( uint32_t )in->octet0 << 16 );
		#else
		( ( uint32_t )in->octet0 ) |
		( ( uint32_t )in->octet1 << 8 ) |
		( ( uint32_t )in->octet2 << 16 );
		#endif /* WORDS_BIGENDIAN */

	return ( double )out;
} /* uint242double() */

static void double2int24( double in, bs2b_int24_t *out )
{
	uint32_t i = ( uint32_t )in;

	#ifdef WORDS_BIGENDIAN
	out->octet2 = i & 0xff;
	out->octet1 = ( i >> 8 ) & 0xff;
	out->octet0 = ( i >> 16 ) & 0xff;
	#else
	out->octet0 = i & 0xff;
	out->octet1 = ( i >> 8 ) & 0xff;
	out->octet2 = ( i >> 16 ) & 0xff;
	#endif /* WORDS_BIGENDIAN */
} /* double2int24() */

static void double2uint24( double in, bs2b_uint24_t *out )
{
	uint32_t i = ( uint32_t )in;

	#ifdef WORDS_BIGENDIAN
	out->octet2 = i & 0xff;
	out->octet1 = ( i >> 8 ) & 0xff;
	out->octet0 = ( i >> 16 ) & 0xff;
	#else
	out->octet0 = i & 0xff;
	out->octet1 = ( i >> 8 ) & 0xff;
	out->octet2 = ( i >> 16 ) & 0xff;
	#endif /* WORDS_BIGENDIAN */
} /* double2uint24() */

/* Set up bs2b data. */
static void init( t_bs2bdp bs2bdp )
{
	double Fc_lo; /* Lowpass filter cut frequency (Hz) */
	double Fc_hi; /* Highboost filter cut frequency (Hz) */
	double G_lo;  /* Lowpass filter gain (multiplier) */
	double G_hi;  /* Highboost filter gain (multiplier) */
	double GB_lo; /* Lowpass filter gain (dB) */
	double GB_hi; /* Highboost filter gain (dB) ( 0 dB is highs ) */
	double level; /* Feeding level (dB) ( level = GB_lo - GB_hi ) */
	double x;

	if( ( bs2bdp->srate > BS2B_MAXSRATE ) || ( bs2bdp->srate < BS2B_MINSRATE ) )
		bs2bdp->srate = BS2B_DEFAULT_SRATE;

	Fc_lo = bs2bdp->level & 0xffff;
	level = ( bs2bdp->level & 0xffff0000 ) >> 16;

	if( ( Fc_lo > BS2B_MAXFCUT ) || ( Fc_lo < BS2B_MINFCUT ) ||
		( level > BS2B_MAXFEED ) || ( level < BS2B_MINFEED ) )
	{
		bs2bdp->level = BS2B_DEFAULT_CLEVEL;
		Fc_lo = bs2bdp->level & 0xffff;
		level = ( bs2bdp->level & 0xffff0000 ) >> 16;
	}

	level /= 10.0;

	GB_lo = level * -5.0 / 6.0 - 3.0;
	GB_hi = level / 6.0 - 3.0;

	G_lo  = pow( 10, GB_lo / 20.0 );
	G_hi  = 1.0 - pow( 10, GB_hi / 20.0 );
	Fc_hi = Fc_lo * pow( 2.0, ( GB_lo - 20.0 * log10( G_hi ) ) / 12.0 );

	/* $fc = $Fc / $s;
	 * $d  = 1 / 2 / pi / $fc;
	 * $x  = exp(-1 / $d);
	 */

	x = exp( -2.0 * M_PI * Fc_lo / bs2bdp->srate );
	bs2bdp->b1_lo = x;
	bs2bdp->a0_lo = G_lo * ( 1.0 - x );
	
	x = exp( -2.0 * M_PI * Fc_hi / bs2bdp->srate );
	bs2bdp->b1_hi = x;
	bs2bdp->a0_hi = 1.0 - G_hi * ( 1.0 - x );
	bs2bdp->a1_hi = -x;

	bs2bdp->gain  = 1.0 / ( 1.0 - G_hi + G_lo );
} /* init() */

/* Single pole IIR filter.
 * O[n] = a0*I[n] + a1*I[n-1] + b1*O[n-1]
 */

/* Lowpass filter */
#define lo_filter( in, out_1 ) \
	( bs2bdp->a0_lo * in + bs2bdp->b1_lo * out_1 )

/* Highboost filter */
#define hi_filter( in, in_1, out_1 ) \
	( bs2bdp->a0_hi * in + bs2bdp->a1_hi * in_1 + bs2bdp->b1_hi * out_1 )

static void cross_feed_d( t_bs2bdp bs2bdp, double *sample )
{
	/* Lowpass filter */
	bs2bdp->lfs.lo[ 0 ] = lo_filter( sample[ 0 ], bs2bdp->lfs.lo[ 0 ] );
	bs2bdp->lfs.lo[ 1 ] = lo_filter( sample[ 1 ], bs2bdp->lfs.lo[ 1 ] );

	/* Highboost filter */
	bs2bdp->lfs.hi[ 0 ] =
		hi_filter( sample[ 0 ], bs2bdp->lfs.asis[ 0 ], bs2bdp->lfs.hi[ 0 ] );
	bs2bdp->lfs.hi[ 1 ] =
		hi_filter( sample[ 1 ], bs2bdp->lfs.asis[ 1 ], bs2bdp->lfs.hi[ 1 ] );
	bs2bdp->lfs.asis[ 0 ] = sample[ 0 ];
	bs2bdp->lfs.asis[ 1 ] = sample[ 1 ];

	/* Crossfeed */
	sample[ 0 ] = bs2bdp->lfs.hi[ 0 ] + bs2bdp->lfs.lo[ 1 ];
	sample[ 1 ] = bs2bdp->lfs.hi[ 1 ] + bs2bdp->lfs.lo[ 0 ];

	/* Bass boost cause allpass attenuation */
	sample[ 0 ] *= bs2bdp->gain;
	sample[ 1 ] *= bs2bdp->gain;
} /* cross_feed_d() */

/* Exported functions.
 * See descriptions in "bs2b.h"
 */

t_bs2bdp bs2b_open( void )
{
	t_bs2bdp bs2bdp = NULL;

	if( NULL != ( bs2bdp = malloc( sizeof( t_bs2bd ) ) ) )
	{
		memset( bs2bdp, 0, sizeof( t_bs2bd ) );
		bs2b_set_srate( bs2bdp, BS2B_DEFAULT_SRATE );
	}

	return bs2bdp;
} /* bs2b_open() */

void bs2b_close( t_bs2bdp bs2bdp )
{
	free( bs2bdp );
} /* bs2b_close() */

void bs2b_set_level( t_bs2bdp bs2bdp, uint32_t level )
{
	if( NULL == bs2bdp ) return;

	if( level == bs2bdp->level ) return;

	bs2bdp->level = level;
	init( bs2bdp );
} /* bs2b_set_level() */

uint32_t bs2b_get_level( t_bs2bdp bs2bdp )
{
	return bs2bdp->level;
} /* bs2b_get_level() */

void bs2b_set_level_fcut( t_bs2bdp bs2bdp, int fcut )
{
	uint32_t level;

	if( NULL == bs2bdp ) return;

	level = bs2bdp->level;
	level &= 0xffff0000;
	level |= ( uint32_t )fcut;
	bs2b_set_level( bs2bdp, level );
} /* bs2b_set_level_fcut() */

int bs2b_get_level_fcut( t_bs2bdp bs2bdp )
{
	return( ( int )( bs2bdp->level & 0xffff ) );
} /* bs2b_get_level_fcut() */

void bs2b_set_level_feed( t_bs2bdp bs2bdp, int feed )
{
	uint32_t level;

	if( NULL == bs2bdp ) return;

	level = bs2bdp->level;
	level &= ( uint32_t )0xffff;
	level |= ( uint32_t )feed << 16;
	bs2b_set_level( bs2bdp, level );
} /* bs2b_set_level_feed() */

int bs2b_get_level_feed( t_bs2bdp bs2bdp )
{
	return( ( int )( ( bs2bdp->level & 0xffff0000 ) >> 16 ) );
} /* bs2b_get_level_feed() */

int bs2b_get_level_delay( t_bs2bdp bs2bdp )
{
	int fcut;
	
	fcut = bs2bdp->level & 0xffff; /* get cut frequency */

	if( ( fcut > BS2B_MAXFCUT ) || ( fcut < BS2B_MINFCUT ) )
		return 0;

	return bs2b_level_delay( fcut );
} /* bs2b_get_level_delay() */

void bs2b_set_srate( t_bs2bdp bs2bdp, uint32_t srate )
{
	if( NULL == bs2bdp ) return;

	if( srate == bs2bdp->srate ) return;

	bs2bdp->srate = srate;
	init( bs2bdp );
	bs2b_clear( bs2bdp );
} /* bs2b_set_srate() */

uint32_t bs2b_get_srate( t_bs2bdp bs2bdp )
{
	return bs2bdp->srate;
} /* bs2b_get_srate() */

void bs2b_clear( t_bs2bdp bs2bdp )
{
	if( NULL == bs2bdp ) return;
	memset( &bs2bdp->lfs, 0, sizeof( bs2bdp->lfs ) );
} /* bs2b_clear() */

int bs2b_is_clear( t_bs2bdp bs2bdp )
{
	int loopv = sizeof( bs2bdp->lfs );

	while( loopv )
	{
		if( ( ( char * )&bs2bdp->lfs )[ --loopv ] != 0 )
			return 0;
	}

	return 1;
} /* bs2b_is_clear() */

char const *bs2b_runtime_version( void )
{
	return BS2B_VERSION_STR;
} /* bs2b_runtime_version() */

uint32_t bs2b_runtime_version_int( void )
{
	return BS2B_VERSION_INT;
} /* bs2b_runtime_version_int() */

void bs2b_cross_feed_d( t_bs2bdp bs2bdp, double *sample, int n )
{
	if( n > 0 )
	{
		while( n-- )
		{
			cross_feed_d( bs2bdp, sample );
			
			/* Clipping of overloaded samples */
			if( sample[ 0 ] >  1.0 ) sample[ 0 ] =  1.0;
			if( sample[ 0 ] < -1.0 ) sample[ 0 ] = -1.0;
			if( sample[ 1 ] >  1.0 ) sample[ 1 ] =  1.0;
			if( sample[ 1 ] < -1.0 ) sample[ 1 ] = -1.0;

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_d() */

void bs2b_cross_feed_dbe( t_bs2bdp bs2bdp, double *sample, int n )
{
	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int64swap( ( uint32_t * )sample );
			int64swap( ( uint32_t * )( sample + 1 ) );
			#endif

			cross_feed_d( bs2bdp, sample );

			/* Clipping of overloaded samples */
			if( sample[ 0 ] >  1.0 ) sample[ 0 ] =  1.0;
			if( sample[ 0 ] < -1.0 ) sample[ 0 ] = -1.0;
			if( sample[ 1 ] >  1.0 ) sample[ 1 ] =  1.0;
			if( sample[ 1 ] < -1.0 ) sample[ 1 ] = -1.0;

			#ifndef WORDS_BIGENDIAN
			int64swap( ( uint32_t * )sample );
			int64swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_dbe() */

void bs2b_cross_feed_dle( t_bs2bdp bs2bdp, double *sample, int n )
{
	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int64swap( ( uint32_t * )sample );
			int64swap( ( uint32_t * )( sample + 1 ) );
			#endif

			cross_feed_d( bs2bdp, sample );

			/* Clipping of overloaded samples */
			if( sample[ 0 ] >  1.0 ) sample[ 0 ] =  1.0;
			if( sample[ 0 ] < -1.0 ) sample[ 0 ] = -1.0;
			if( sample[ 1 ] >  1.0 ) sample[ 1 ] =  1.0;
			if( sample[ 1 ] < -1.0 ) sample[ 1 ] = -1.0;

			#ifdef WORDS_BIGENDIAN
			int64swap( ( uint32_t * )sample );
			int64swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_dle() */

void bs2b_cross_feed_f( t_bs2bdp bs2bdp, float *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] >  1.0 ) sample_d[ 0 ] =  1.0;
			if( sample_d[ 0 ] < -1.0 ) sample_d[ 0 ] = -1.0;
			if( sample_d[ 1 ] >  1.0 ) sample_d[ 1 ] =  1.0;
			if( sample_d[ 1 ] < -1.0 ) sample_d[ 1 ] = -1.0;

			sample[ 0 ] = ( float )sample_d[ 0 ];
			sample[ 1 ] = ( float )sample_d[ 1 ];

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_f() */

void bs2b_cross_feed_fbe( t_bs2bdp bs2bdp, float *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] >  1.0 ) sample_d[ 0 ] =  1.0;
			if( sample_d[ 0 ] < -1.0 ) sample_d[ 0 ] = -1.0;
			if( sample_d[ 1 ] >  1.0 ) sample_d[ 1 ] =  1.0;
			if( sample_d[ 1 ] < -1.0 ) sample_d[ 1 ] = -1.0;

			sample[ 0 ] = ( float )sample_d[ 0 ];
			sample[ 1 ] = ( float )sample_d[ 1 ];

			#ifndef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_fbe() */

void bs2b_cross_feed_fle( t_bs2bdp bs2bdp, float *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] >  1.0 ) sample_d[ 0 ] =  1.0;
			if( sample_d[ 0 ] < -1.0 ) sample_d[ 0 ] = -1.0;
			if( sample_d[ 1 ] >  1.0 ) sample_d[ 1 ] =  1.0;
			if( sample_d[ 1 ] < -1.0 ) sample_d[ 1 ] = -1.0;

			sample[ 0 ] = ( float )sample_d[ 0 ];
			sample[ 1 ] = ( float )sample_d[ 1 ];

			#ifdef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_fle() */

#define MAX_INT32_VALUE  2147483647.0
#define MIN_INT32_VALUE -2147483648.0
#define MAX_INT24_VALUE     8388607.0
#define MIN_INT24_VALUE    -8388608.0
#define MAX_INT16_VALUE       32767.0
#define MIN_INT16_VALUE      -32768.0
#define MAX_INT8_VALUE          127.0
#define MIN_INT8_VALUE         -128.0

void bs2b_cross_feed_s32( t_bs2bdp bs2bdp, int32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( int32_t )sample_d[ 0 ];
			sample[ 1 ] = ( int32_t )sample_d[ 1 ];

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s32() */

void bs2b_cross_feed_u32( t_bs2bdp bs2bdp, uint32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )( ( int32_t )( sample[ 0 ] ^ 0x80000000 ) );
			sample_d[ 1 ] = ( double )( ( int32_t )( sample[ 1 ] ^ 0x80000000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( ( uint32_t )sample_d[ 0 ] ) ^ 0x80000000;
			sample[ 1 ] = ( ( uint32_t )sample_d[ 1 ] ) ^ 0x80000000;

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u32() */

void bs2b_cross_feed_s32be( t_bs2bdp bs2bdp, int32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( int32_t )sample_d[ 0 ];
			sample[ 1 ] = ( int32_t )sample_d[ 1 ];

			#ifndef WORDS_BIGENDIAN
			int32swap( ( uint32_t * )sample );
			int32swap( ( uint32_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s32be() */

void bs2b_cross_feed_u32be( t_bs2bdp bs2bdp, uint32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )( ( int32_t )( sample[ 0 ] ^ 0x80000000 ) );
			sample_d[ 1 ] = ( double )( ( int32_t )( sample[ 1 ] ^ 0x80000000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( ( uint32_t )sample_d[ 0 ] ) ^ 0x80000000;
			sample[ 1 ] = ( ( uint32_t )sample_d[ 1 ] ) ^ 0x80000000;

			#ifndef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u32be() */

void bs2b_cross_feed_s32le( t_bs2bdp bs2bdp, int32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( int32_t )sample_d[ 0 ];
			sample[ 1 ] = ( int32_t )sample_d[ 1 ];

			#ifdef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s32le() */

void bs2b_cross_feed_u32le( t_bs2bdp bs2bdp, uint32_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )( ( int32_t )( sample[ 0 ] ^ 0x80000000 ) );
			sample_d[ 1 ] = ( double )( ( int32_t )( sample[ 1 ] ^ 0x80000000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT32_VALUE ) sample_d[ 0 ] = MAX_INT32_VALUE;
			if( sample_d[ 0 ] < MIN_INT32_VALUE ) sample_d[ 0 ] = MIN_INT32_VALUE;
			if( sample_d[ 1 ] > MAX_INT32_VALUE ) sample_d[ 1 ] = MAX_INT32_VALUE;
			if( sample_d[ 1 ] < MIN_INT32_VALUE ) sample_d[ 1 ] = MIN_INT32_VALUE;

			sample[ 0 ] = ( ( uint32_t )sample_d[ 0 ] ) ^ 0x80000000;
			sample[ 1 ] = ( ( uint32_t )sample_d[ 1 ] ) ^ 0x80000000;

			#ifdef WORDS_BIGENDIAN
			int32swap( sample );
			int32swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u32le() */

void bs2b_cross_feed_s16( t_bs2bdp bs2bdp, int16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( int16_t )sample_d[ 0 ];
			sample[ 1 ] = ( int16_t )sample_d[ 1 ];

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s16() */

void bs2b_cross_feed_u16( t_bs2bdp bs2bdp, uint16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )( ( int16_t )( sample[ 0 ] ^ 0x8000 ) );
			sample_d[ 1 ] = ( double )( ( int16_t )( sample[ 1 ] ^ 0x8000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( ( uint16_t )sample_d[ 0 ] ) ^ 0x8000;
			sample[ 1 ] = ( ( uint16_t )sample_d[ 1 ] ) ^ 0x8000;

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u16() */

void bs2b_cross_feed_s16be( t_bs2bdp bs2bdp, int16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int16swap( ( uint16_t * )sample );
			int16swap( ( uint16_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( int16_t )sample_d[ 0 ];
			sample[ 1 ] = ( int16_t )sample_d[ 1 ];

			#ifndef WORDS_BIGENDIAN
			int16swap( ( uint16_t * )sample );
			int16swap( ( uint16_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s16be() */

void bs2b_cross_feed_u16be( t_bs2bdp bs2bdp, uint16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )( ( int16_t )( sample[ 0 ] ^ 0x8000 ) );
			sample_d[ 1 ] = ( double )( ( int16_t )( sample[ 1 ] ^ 0x8000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( ( uint16_t )sample_d[ 0 ] ) ^ 0x8000;
			sample[ 1 ] = ( ( uint16_t )sample_d[ 1 ] ) ^ 0x8000;

			#ifndef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u16be() */

void bs2b_cross_feed_s16le( t_bs2bdp bs2bdp, int16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( int16_t )sample_d[ 0 ];
			sample[ 1 ] = ( int16_t )sample_d[ 1 ];

			#ifdef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s16le() */

void bs2b_cross_feed_u16le( t_bs2bdp bs2bdp, uint16_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample_d[ 0 ] = ( double )( ( int16_t )( sample[ 0 ] ^ 0x8000 ) );
			sample_d[ 1 ] = ( double )( ( int16_t )( sample[ 1 ] ^ 0x8000 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT16_VALUE ) sample_d[ 0 ] = MAX_INT16_VALUE;
			if( sample_d[ 0 ] < MIN_INT16_VALUE ) sample_d[ 0 ] = MIN_INT16_VALUE;
			if( sample_d[ 1 ] > MAX_INT16_VALUE ) sample_d[ 1 ] = MAX_INT16_VALUE;
			if( sample_d[ 1 ] < MIN_INT16_VALUE ) sample_d[ 1 ] = MIN_INT16_VALUE;

			sample[ 0 ] = ( ( uint16_t )sample_d[ 0 ] ) ^ 0x8000;
			sample[ 1 ] = ( ( uint16_t )sample_d[ 1 ] ) ^ 0x8000;

			#ifdef WORDS_BIGENDIAN
			int16swap( sample );
			int16swap( sample + 1 );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u16le() */

void bs2b_cross_feed_s8( t_bs2bdp bs2bdp, int8_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )sample[ 0 ];
			sample_d[ 1 ] = ( double )sample[ 1 ];

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT8_VALUE ) sample_d[ 0 ] = MAX_INT8_VALUE;
			if( sample_d[ 0 ] < MIN_INT8_VALUE ) sample_d[ 0 ] = MIN_INT8_VALUE;
			if( sample_d[ 1 ] > MAX_INT8_VALUE ) sample_d[ 1 ] = MAX_INT8_VALUE;
			if( sample_d[ 1 ] < MIN_INT8_VALUE ) sample_d[ 1 ] = MIN_INT8_VALUE;

			sample[ 0 ] = ( int8_t )sample_d[ 0 ];
			sample[ 1 ] = ( int8_t )sample_d[ 1 ];

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s8() */

void bs2b_cross_feed_u8( t_bs2bdp bs2bdp, uint8_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = ( double )( ( int8_t )( sample[ 0 ] ^ 0x80 ) );
			sample_d[ 1 ] = ( double )( ( int8_t )( sample[ 1 ] ^ 0x80 ) );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT8_VALUE ) sample_d[ 0 ] = MAX_INT8_VALUE;
			if( sample_d[ 0 ] < MIN_INT8_VALUE ) sample_d[ 0 ] = MIN_INT8_VALUE;
			if( sample_d[ 1 ] > MAX_INT8_VALUE ) sample_d[ 1 ] = MAX_INT8_VALUE;
			if( sample_d[ 1 ] < MIN_INT8_VALUE ) sample_d[ 1 ] = MIN_INT8_VALUE;

			sample[ 0 ] = ( ( uint8_t )sample_d[ 0 ] ) ^ 0x80;
			sample[ 1 ] = ( ( uint8_t )sample_d[ 1 ] ) ^ 0x80;

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u8() */

void bs2b_cross_feed_s24( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = int242double( sample );
			sample_d[ 1 ] = int242double( sample + 1 );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2int24( sample_d[ 0 ], sample );
			double2int24( sample_d[ 1 ], sample + 1 );

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s24() */

void bs2b_cross_feed_u24( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			sample_d[ 0 ] = uint242double( sample )     - MAX_INT24_VALUE - 1.0;
			sample_d[ 1 ] = uint242double( sample + 1 ) - MAX_INT24_VALUE - 1.0;

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2uint24( sample_d[ 0 ] + MAX_INT24_VALUE + 1.0 , sample );
			double2uint24( sample_d[ 1 ] + MAX_INT24_VALUE + 1.0 , sample + 1 );

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u24() */

void bs2b_cross_feed_s24be( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = int242double( sample );
			sample_d[ 1 ] = int242double( sample + 1 );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2int24( sample_d[ 0 ], sample );
			double2int24( sample_d[ 1 ], sample + 1 );

			#ifndef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s24be() */

void bs2b_cross_feed_u24be( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifndef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = uint242double( sample )     - MAX_INT24_VALUE - 1.0;
			sample_d[ 1 ] = uint242double( sample + 1 ) - MAX_INT24_VALUE - 1.0;

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2uint24( sample_d[ 0 ] + MAX_INT24_VALUE + 1.0 , sample );
			double2uint24( sample_d[ 1 ] + MAX_INT24_VALUE + 1.0 , sample + 1 );

			#ifndef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u24be() */

void bs2b_cross_feed_s24le( t_bs2bdp bs2bdp, bs2b_int24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = int242double( sample );
			sample_d[ 1 ] = int242double( sample + 1 );

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2int24( sample_d[ 0 ], sample );
			double2int24( sample_d[ 1 ], sample + 1 );

			#ifdef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_s24le() */

void bs2b_cross_feed_u24le( t_bs2bdp bs2bdp, bs2b_uint24_t *sample, int n )
{
	double sample_d[ 2 ];

	if( n > 0 )
	{
		while( n-- )
		{
			#ifdef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample_d[ 0 ] = uint242double( sample )     - MAX_INT24_VALUE - 1.0;
			sample_d[ 1 ] = uint242double( sample + 1 ) - MAX_INT24_VALUE - 1.0;

			cross_feed_d( bs2bdp, sample_d );

			/* Clipping of overloaded samples */
			if( sample_d[ 0 ] > MAX_INT24_VALUE ) sample_d[ 0 ] = MAX_INT24_VALUE;
			if( sample_d[ 0 ] < MIN_INT24_VALUE ) sample_d[ 0 ] = MIN_INT24_VALUE;
			if( sample_d[ 1 ] > MAX_INT24_VALUE ) sample_d[ 1 ] = MAX_INT24_VALUE;
			if( sample_d[ 1 ] < MIN_INT24_VALUE ) sample_d[ 1 ] = MIN_INT24_VALUE;

			double2uint24( sample_d[ 0 ] + MAX_INT24_VALUE + 1.0 , sample );
			double2uint24( sample_d[ 1 ] + MAX_INT24_VALUE + 1.0 , sample + 1 );

			#ifdef WORDS_BIGENDIAN
			int24swap( ( bs2b_uint24_t * )sample );
			int24swap( ( bs2b_uint24_t * )( sample + 1 ) );
			#endif

			sample += 2;
		} /* while */
	} /* if */
} /* bs2b_cross_feed_u24le() */
