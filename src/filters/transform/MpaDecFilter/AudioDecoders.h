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

#pragma once

static const struct {
	const BYTE ch[8];
	const DWORD layout;
}
s_scmap_hdmv[] = {
	// dshow: left, right, center, LFE, left surround, right surround
	// lets see how we can map these things to dshow (oh the joy!)
	//FL FR FC LFe BL BR FLC FRC
	{{-1,-1,-1,-1,-1,-1,-1,-1 }, 0}, // INVALID
	{{ 0,-1,-1,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_CENTER}, // Mono    M1, 0
	{{-1,-1,-1,-1,-1,-1,-1,-1 }, 0}, // INVALID
	{{ 0, 1,-1,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT}, // Stereo  FL, FR
	{{ 0, 1, 2,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER},                                                           // 3/0      FL, FR, FC
	{{ 0, 1, 2,-1,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_CENTER},                                                            // 2/1      FL, FR, Surround
	{{ 0, 1, 2, 3,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER},                                       // 3/1      FL, FR, FC, Surround
	{{ 0, 1, 2, 3,-1,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},                                           // 2/2      FL, FR, BL, BR
	{{ 0, 1, 2, 3, 4,-1,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},                      // 3/2      FL, FR, FC, BL, BR
	{{ 0, 1, 2, 5, 3, 4,-1,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT},// 3/2+LFe  FL, FR, FC, BL, BR, LFe
	{{ 0, 1, 2, 3, 6, 4, 5,-1 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},// 3/4  FL, FR, FC, BL, Bls, Brs, BR
	{{ 0, 1, 2, 7, 4, 5, 3, 6 }, SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},// 3/4+LFe  FL, FR, FC, BL, Bls, Brs, BR, LFe
};

static const struct {
	const DWORD layout1;
	const DWORD layout2;
} s_scmap_dvda[] = {
	{SPEAKER_FRONT_CENTER,															0},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										0},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_LOW_FREQUENCY},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_LOW_FREQUENCY|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER|SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT,										SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,					SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,					SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,					SPEAKER_LOW_FREQUENCY},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,					SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER,					SPEAKER_LOW_FREQUENCY|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT,	SPEAKER_LOW_FREQUENCY},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT,	SPEAKER_FRONT_CENTER},
	{SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT,	SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY},
};

struct DVDA_INFO {
	unsigned channels1;
	unsigned samplerate1;
	unsigned bitdepth1;
	unsigned channels2;
	unsigned samplerate2;
	unsigned bitdepth2;
	unsigned groupassign;
};

std::unique_ptr<BYTE[]> DecodeDvdLPCM(unsigned& dst_size, SampleFormat& dst_sf, BYTE* src, unsigned& src_size, const unsigned channels, const unsigned bitdepth);
std::unique_ptr<BYTE[]> DecodeDvdaLPCM(unsigned& dst_size, SampleFormat& dst_sf, BYTE* src, unsigned& src_size, const DVDA_INFO& a);
std::unique_ptr<BYTE[]> DecodeHdmvLPCM(unsigned& dst_size, SampleFormat& dst_sf, BYTE* src, unsigned& src_size, const unsigned channels, const unsigned bitdepth, const BYTE channel_conf);
