/*
 * (C) 2017 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "SampleFormat.h"
#include "AudioDecoders.h"

std::unique_ptr<uint8_t[]> DecodeDvdLPCM(unsigned& dst_size, SampleFormat& dst_sf, BYTE* src, unsigned& src_size, const unsigned channels, const unsigned bitdepth)
{
	// https://wiki.multimedia.cx/index.php/PCM#DVD_PCM

	const unsigned blocksize = channels * 2 * bitdepth / 8;
	if (!blocksize || blocksize > src_size) {
		return nullptr;
	}
	const unsigned allsamples = (src_size / blocksize) * 2 * channels;

	dst_size = allsamples * (bitdepth <= 16 ? 2 : 4); // convert to 16 and 32-bit
	std::unique_ptr<BYTE[]> dst(new(std::nothrow) BYTE[dst_size]);

	if (!dst) {
		return nullptr;
	}

	if (bitdepth == 16) {
		uint16_t* src16 = (uint16_t*)src;
		uint16_t* dst16 = (uint16_t*)dst.get();

		for (unsigned i = 0; i < allsamples; ++i) {
			*(dst16++) = _byteswap_ushort(*src16++);
		}
		dst_sf = SAMPLE_FMT_S16;
	}
	else if (bitdepth == 20) {
		uint32_t* dst32 = (uint32_t*)dst.get(); // convert to 32-bit

		if (channels == 1) {
			for (unsigned i = 0; i < allsamples; i += 2) {
				*(dst32++) = (src[0] << 24) | (src[1] << 16) | ((src[4] & 0xF0) << 8);
				*(dst32++) = (src[2] << 24) | (src[3] << 16) | ((src[4] & 0x0F) << 12);
				src += 5;
			}
		}
		else {
			for (unsigned i = 0; i < allsamples; i += 4) {
				*(dst32++) = (src[0] << 24) | (src[1] << 16) | ((src[8] & 0xF0) << 8);
				*(dst32++) = (src[2] << 24) | (src[3] << 16) | ((src[8] & 0x0F) << 12);
				*(dst32++) = (src[4] << 24) | (src[5] << 16) | ((src[9] & 0xF0) << 8);
				*(dst32++) = (src[6] << 24) | (src[7] << 16) | ((src[9] & 0x0F) << 12);
				src += 10;
			}
		}
		dst_sf = SAMPLE_FMT_S32;
	}
	else if (bitdepth == 24) {
		uint32_t* dst32 = (uint32_t*)dst.get(); // convert to 32-bit

		if (channels == 1) {
			for (unsigned i = 0; i < allsamples; i += 2) {
				*(dst32++) = (src[0] << 24) | (src[1] << 16) | (src[4] << 8); // Sample 1
				*(dst32++) = (src[2] << 24) | (src[3] << 16) | (src[5] << 8); // Sample 2
				src += 6;
			}
		}
		else {
			for (unsigned i = 0; i < allsamples; i += 4) {
				*(dst32++) = (src[0] << 24) | (src[1] << 16) | (src[8] << 8); // Sample 1
				*(dst32++) = (src[2] << 24) | (src[3] << 16) | (src[9] << 8); // Sample 2
				*(dst32++) = (src[4] << 24) | (src[5] << 16) | (src[10] << 8); // Sample 3
				*(dst32++) = (src[6] << 24) | (src[7] << 16) | (src[11] << 8); // Sample 4
				src += 12;
			}
		}
		dst_sf = SAMPLE_FMT_S32;
	}

	src_size %= blocksize;

	return dst;
}
