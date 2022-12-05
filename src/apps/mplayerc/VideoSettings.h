/*
 * (C) 2022 see Authors.txt
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

#include "filters/renderer/VideoRenderers/IAllocatorPresenter.h"

enum : int {
	//VIDRNDT_VMR7 = 0, // obsolete
	//VIDRNDT_VMR9_W = 1, // obsolete
	VIDRNDT_EVR = 2,
	VIDRNDT_EVR_CP,
	VIDRNDT_SYNC,
	VIDRNDT_MPCVR,
	VIDRNDT_DXR,
	VIDRNDT_MADVR,
	VIDRNDT_NULL_ANY = 10,
	VIDRNDT_NULL_UNCOMP,
};

class CRenderersSettings
{
public:
	int  iVideoRenderer = VIDRNDT_EVR_CP;

	bool bExclusiveFullscreen = false;

	// Color control
	int  iBrightness = 0;
	int  iContrast   = 0;
	int  iHue        = 0;
	int  iSaturation = 0;

	// subtitles
	SubpicSettings SubpicSets;
	Stereo3DSettings Stereo3DSets;
	ExtraRendererSettings ExtraSets;

	void SetDefault();
	void Load();
	void Save();
};

extern CRenderersSettings& GetRenderersSettings();
