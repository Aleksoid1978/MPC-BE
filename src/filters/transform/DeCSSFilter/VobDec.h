/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
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

class CVobDec
{
	int m_lfsr0, m_lfsr1;

	void ClockLfsr0Forward(int& lfsr0);
	void ClockLfsr1Forward(int& lfsr1);
	void ClockBackward(int& lfsr0, int& lfsr1);
	void Salt(const BYTE salt[5], int& lfsr0, int& lfsr1);
	int FindLfsr(const BYTE* crypt, int offset, const BYTE* plain);

public:
	CVobDec();
	virtual ~CVobDec();

	bool m_fFoundKey;

	bool FindKey(BYTE* buff);
	void Decrypt(BYTE* buff);
};
