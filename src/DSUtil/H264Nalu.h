/*
 * (C) 2006-2024 see Authors.txt
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
	NALU_TYPE_UNKNOWN       = -1,
	// AVC
	NALU_TYPE_SLICE         = 1,
	NALU_TYPE_DPA           = 2,
	NALU_TYPE_DPB           = 3,
	NALU_TYPE_DPC           = 4,
	NALU_TYPE_IDR           = 5,
	NALU_TYPE_SEI           = 6,
	NALU_TYPE_SPS           = 7,
	NALU_TYPE_PPS           = 8,
	NALU_TYPE_AUD           = 9,
	NALU_TYPE_EOSEQ         = 10,
	NALU_TYPE_EOSTREAM      = 11,
	NALU_TYPE_FILL          = 12,
	NALU_TYPE_SUBSET_SPS    = 15,
	// HEVC
	NALU_TYPE_HEVC_RASL_R     = 9,
	NALU_TYPE_HEVC_BLA_W_LP   = 16,
	NALU_TYPE_HEVC_BLA_W_RADL = 17,
	NALU_TYPE_HEVC_BLA_N_LP   = 18,
	NALU_TYPE_HEVC_IDR_W_RADL = 19,
	NALU_TYPE_HEVC_IDR_N_LP   = 20,
	NALU_TYPE_HEVC_CRA        = 21,
	NALU_TYPE_HEVC_VPS        = 32,
	NALU_TYPE_HEVC_SPS        = 33,
	NALU_TYPE_HEVC_PPS        = 34,
	NALU_TYPE_HEVC_AUD        = 35,
	NALU_TYPE_HEVC_EOSEQ      = 36,
	NALU_TYPE_HEVC_SEI_PREFIX = 39,
	NALU_TYPE_HEVC_UNSPEC62   = 62,
	// VVC
	NALU_TYPE_VVC_TRAIL       = 0,
	NALU_TYPE_VVC_STSA        = 1,
	NALU_TYPE_VVC_RADL        = 2,
	NALU_TYPE_VVC_RASL        = 3,
	NALU_TYPE_VVC_RSV_VCL_4   = 4,
	NALU_TYPE_VVC_RSV_VCL_5   = 5,
	NALU_TYPE_VVC_RSV_VCL_6   = 6,
	NALU_TYPE_VVC_IDR_W_RADL  = 7,
	NALU_TYPE_VVC_IDR_N_LP    = 8,
	NALU_TYPE_VVC_CRA         = 9,
	NALU_TYPE_VVC_GDR         = 10,
	NALU_TYPE_VVC_RSV_IRAP_11 = 11,
	NALU_TYPE_VVC_OPI         = 12,
	NALU_TYPE_VVC_DCI         = 13,
	NALU_TYPE_VVC_VPS         = 14,
	NALU_TYPE_VVC_SPS         = 15,
	NALU_TYPE_VVC_PPS         = 16,
	NALU_TYPE_VVC_PREFIX_APS  = 17,
	NALU_TYPE_VVC_SUFFIX_APS  = 18,
	NALU_TYPE_VVC_PH          = 19,
	NALU_TYPE_VVC_AUD         = 20,
	NALU_TYPE_VVC_EOS         = 21,
	NALU_TYPE_VVC_EOB         = 22,
	NALU_TYPE_VVC_PREFIX_SEI  = 23,
	NALU_TYPE_VVC_SUFFIX_SEI  = 24,
	NALU_TYPE_VVC_FD          = 25,
	NALU_TYPE_VVC_RSV_NVCL_26 = 26,
	NALU_TYPE_VVC_RSV_NVCL_27 = 27,
	NALU_TYPE_VVC_UNSPEC_28   = 28,
	NALU_TYPE_VVC_UNSPEC_29   = 29,
	NALU_TYPE_VVC_UNSPEC_30   = 30,
	NALU_TYPE_VVC_UNSPEC_31   = 31
};

class CH264Nalu
{
protected :
	int         forbidden_bit     = 0;                 //! should be always FALSE
	int         nal_reference_idc = 0;                 //! NALU_PRIORITY_xxxx
	NALU_TYPE   nal_unit_type     = NALU_TYPE_UNKNOWN; //! NALU_TYPE_xxxx

	size_t      m_nNALStartPos    = 0;                 //! NALU start (including startcode / size)
	size_t      m_nNALDataPos     = 0;                 //! Useful part

	const BYTE* m_pBuffer         = nullptr;
	size_t      m_nCurPos         = 0;
	size_t      m_nNextRTP        = 0;
	size_t      m_nSize           = 0;
	int         m_nNALSize        = 0;

	size_t      m_nNALStartCodeSize = 3;

	bool        MoveToNextAnnexBStartcode();
	bool        MoveToNextRTPStartcode();

public :
	CH264Nalu() = default;

	NALU_TYPE   GetType() const { return nal_unit_type; }
	bool        IsRefFrame() const { return (nal_reference_idc != 0); }

	size_t      GetDataLength() const { return m_nCurPos - m_nNALDataPos; }
	const BYTE* GetDataBuffer() const { return m_pBuffer + m_nNALDataPos; }
	size_t      GetRoundedDataLength() const {
		const size_t nSize = m_nCurPos - m_nNALDataPos;
		return nSize + 128 - (nSize % 128);
	}

	size_t      GetLength() const { return m_nCurPos - m_nNALStartPos; }
	const BYTE* GetNALBuffer() const { return m_pBuffer + m_nNALStartPos; }
	size_t      GetNALPos() const { return m_nNALStartPos; }
	bool        IsEOF() const { return m_nCurPos >= m_nSize; }

	void        SetBuffer(const BYTE* pBuffer, const size_t nSize, const int nNALSize = 0);
	bool        ReadNext();
};

class CH265Nalu : public CH264Nalu
{
public:
	CH265Nalu() : CH264Nalu() {};
	bool ReadNext();
};

class CH266Nalu : public CH264Nalu
{
public:
	CH266Nalu() : CH264Nalu() {};
	bool ReadNext();
};
