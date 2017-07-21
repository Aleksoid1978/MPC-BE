/*
 * (C) 2006-2017 see Authors.txt
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

enum NALU_TYPE {
	NALU_TYPE_UNKNOWN    = -1,
	// AVC
	NALU_TYPE_SLICE      = 1,
	NALU_TYPE_DPA        = 2,
	NALU_TYPE_DPB        = 3,
	NALU_TYPE_DPC        = 4,
	NALU_TYPE_IDR        = 5,
	NALU_TYPE_SEI        = 6,
	NALU_TYPE_SPS        = 7,
	NALU_TYPE_PPS        = 8,
	NALU_TYPE_AUD        = 9,
	NALU_TYPE_EOSEQ      = 10,
	NALU_TYPE_EOSTREAM   = 11,
	NALU_TYPE_FILL       = 12,
	NALU_TYPE_SUBSET_SPS = 15,
	// HEVC
	NALU_TYPE_HEVC_RASL_R     = 9,
	NALU_TYPE_HEVC_BLA_W_LP   = 16,
	NALU_TYPE_HEVC_BLA_W_RADL = 17,
	NALU_TYPE_HEVC_BLA_N_LP   = 18,
	NALU_TYPE_HEVC_IDR_W_RADL = 19,
	NALU_TYPE_HEVC_IDR_N_LP   = 20,
	NALU_TYPE_HEVC_CRA_NUT    = 21,
	NALU_TYPE_HEVC_VPS        = 32,
	NALU_TYPE_HEVC_SPS        = 33,
	NALU_TYPE_HEVC_PPS        = 34,
	NALU_TYPE_HEVC_AUD        = 35,
	NALU_TYPE_HEVC_SEI_PREFIX = 39
};

class CH264Nalu
{
protected :
	int			forbidden_bit;		//! should be always FALSE
	int			nal_reference_idc;	//! NALU_PRIORITY_xxxx
	NALU_TYPE	nal_unit_type;		//! NALU_TYPE_xxxx

	size_t		m_nNALStartPos;		//! NALU start (including startcode / size)
	size_t		m_nNALDataPos;		//! Useful part

	BYTE*		m_pBuffer;
	size_t		m_nCurPos;
	size_t		m_nNextRTP;
	size_t		m_nSize;
	int			m_nNALSize;

	bool		MoveToNextAnnexBStartcode();
	bool		MoveToNextRTPStartcode();

public :
	CH264Nalu() { SetBuffer(NULL, 0, 0); }

	NALU_TYPE GetType() const { return nal_unit_type; };
	bool IsRefFrame() const { return (nal_reference_idc != 0); };

	size_t GetDataLength() const { return m_nCurPos - m_nNALDataPos; };
	BYTE* GetDataBuffer() { return m_pBuffer + m_nNALDataPos; };
	size_t GetRoundedDataLength() const {
		size_t nSize = m_nCurPos - m_nNALDataPos;
		return nSize + 128 - (nSize % 128);
	}

	size_t GetLength() const { return m_nCurPos - m_nNALStartPos; };
	BYTE* GetNALBuffer() { return m_pBuffer + m_nNALStartPos; };
	size_t GetNALPos() { return m_nNALStartPos; }
	bool IsEOF() const { return m_nCurPos >= m_nSize; };

	void SetBuffer(BYTE* pBuffer, size_t nSize, int nNALSize = 0);
	bool ReadNext();
};

class CH265Nalu : public CH264Nalu
{
public:
	CH265Nalu() : CH264Nalu() {};
	bool ReadNext();
};

