/*
 * (C) 2018 see Authors.txt
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

class __declspec(uuid("5933BB4F-EC4D-454E-8E11-B74DDA92E6F9")) ChaptersSouce : public CSource, public IDSMChapterBagImpl
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

public:
	ChaptersSouce();
	virtual ~ChaptersSouce() {}

	DECLARE_IUNKNOWN;
};
