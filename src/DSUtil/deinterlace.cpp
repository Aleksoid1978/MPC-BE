//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2007 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"
#include "vd.h"

#include <emmintrin.h>
#include <vd2/system/memory.h>
#include <vd2/system/vdstl.h>

#pragma warning(disable: 4799) // warning C4799: function has no EMMS instruction

///////////////////////////////////////////////////////////////////////////

// Bob

void AvgLines8(BYTE* dst, DWORD h, DWORD pitch)
{
	if(h <= 1) return;

	BYTE* s = dst;
	BYTE* d = dst + (h-2)*pitch;

	for(; s < d; s += pitch*2)
	{
		BYTE* tmp = s;

		{
			for(int i = pitch; i--; tmp++)
			{
				tmp[pitch] = (tmp[0] + tmp[pitch<<1] + 1) >> 1;
			}
		}
	}

	if(!(h&1) && h >= 2)
	{
		dst += (h-2)*pitch;
		memcpy(dst + pitch, dst, pitch);
	}

}

void DeinterlaceBob(BYTE* dst, BYTE* src, DWORD rowbytes, DWORD h, DWORD dstpitch, DWORD srcpitch, bool topfield)
{
	if(topfield)
	{
		BitBltFromRGBToRGB(rowbytes, h/2, dst, dstpitch*2, 8, src, srcpitch*2, 8);
		AvgLines8(dst, h, dstpitch);
	}
	else
	{
		BitBltFromRGBToRGB(rowbytes, h/2, dst + dstpitch, dstpitch*2, 8, src + srcpitch, srcpitch*2, 8);
		AvgLines8(dst + dstpitch, h-1, dstpitch);
	}
}
