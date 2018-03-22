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

#ifndef BS2BCLASS_H
#define BS2BCLASS_H

#include "bs2b.h"

class bs2b_base
{
private:
	t_bs2bdp bs2bdp;

public:
	bs2b_base();
	~bs2b_base();

	void     set_level( uint32_t level );
	uint32_t get_level();
	void     set_level_fcut( int fcut );
	int      get_level_fcut();
	void     set_level_feed( int feed );
	int      get_level_feed();
	int      get_level_delay();
	void     set_srate( uint32_t srate );
	uint32_t get_srate();
	void     clear();
	bool     is_clear();

	char const *runtime_version( void );
	uint32_t    runtime_version_int( void );

	inline void cross_feed( double *sample, int n = 1 )
	{
		bs2b_cross_feed_d( bs2bdp, sample, n );
	}

	inline void cross_feed_be( double *sample, int n = 1 )
	{
		bs2b_cross_feed_dbe( bs2bdp, sample, n );
	}

	inline void cross_feed_le( double *sample, int n = 1 )
	{
		bs2b_cross_feed_dle( bs2bdp, sample, n );
	}

	inline void cross_feed( float *sample, int n = 1 )
	{
		bs2b_cross_feed_f( bs2bdp, sample, n );
	}

	inline void cross_feed_be( float *sample, int n = 1 )
	{
		bs2b_cross_feed_fbe( bs2bdp, sample, n );
	}

	inline void cross_feed_le( float *sample, int n = 1 )
	{
		bs2b_cross_feed_fle( bs2bdp, sample, n );
	}

	inline void cross_feed( int32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s32( bs2bdp, sample, n );
	}

	inline void cross_feed( uint32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u32( bs2bdp, sample, n );
	}

	inline void cross_feed_be( int32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s32be( bs2bdp, sample, n );
	}

	inline void cross_feed_be( uint32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u32be( bs2bdp, sample, n );
	}

	inline void cross_feed_le( int32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s32le( bs2bdp, sample, n );
	}

	inline void cross_feed_le( uint32_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u32le( bs2bdp, sample, n );
	}

	inline void cross_feed( int16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s16( bs2bdp, sample, n );
	}

	inline void cross_feed( uint16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u16( bs2bdp, sample, n );
	}

	inline void cross_feed_be( int16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s16be( bs2bdp, sample, n );
	}

	inline void cross_feed_be( uint16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u16be( bs2bdp, sample, n );
	}

	inline void cross_feed_le( int16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s16le( bs2bdp, sample, n );
	}

	inline void cross_feed_le( uint16_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u16le( bs2bdp, sample, n );
	}

	inline void cross_feed( int8_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s8( bs2bdp, sample, n );
	}

	inline void cross_feed( uint8_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u8( bs2bdp, sample, n );
	}

	inline void cross_feed( bs2b_int24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s24( bs2bdp, sample, n );
	}

	inline void cross_feed( bs2b_uint24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u24( bs2bdp, sample, n );
	}

	inline void cross_feed_be( bs2b_int24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s24be( bs2bdp, sample, n );
	}

	inline void cross_feed_be( bs2b_uint24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u24be( bs2bdp, sample, n );
	}

	inline void cross_feed_le( bs2b_int24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_s24le( bs2bdp, sample, n );
	}

	inline void cross_feed_le( bs2b_uint24_t *sample, int n = 1 )
	{
		bs2b_cross_feed_u24le( bs2bdp, sample, n );
	}
}; // class bs2b_base

#endif // BS2BCLASS_H
