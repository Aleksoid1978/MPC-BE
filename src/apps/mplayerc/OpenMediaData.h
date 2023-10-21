/*
 * (C) 2023 see Authors.txt
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

class OpenMediaData
{
public:
	//OpenMediaData() {}
	virtual ~OpenMediaData() {} // one virtual funct is needed to enable rtti
	CStringW title;
	CSubtitleItemList subs;

	BOOL bAddRecent = TRUE;
};

class OpenFileData : public OpenMediaData
{
public:
	//OpenFileData() {}
	CFileItem fi;
	CAudioItemList auds;
	REFERENCE_TIME rtStart = INVALID_TIME;

	void Clear() {
		title.Empty();
		subs.clear();
		bAddRecent = TRUE;
		fi.Clear();
		auds.clear();
		rtStart = INVALID_TIME;
	}
};

class OpenDVDData : public OpenMediaData
{
public:
	//OpenDVDData() {}
	CStringW path;
	CComPtr<IDvdState> pDvdState;
};

class OpenDeviceData : public OpenMediaData
{
public:
	//OpenDeviceData() {}
	CStringW DisplayName[2];
	int vinput   = -1;
	int vchannel = -1;
	int ainput   = -1;
};
