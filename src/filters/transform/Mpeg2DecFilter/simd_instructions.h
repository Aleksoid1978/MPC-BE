/*
 * (C) 2003-2006 Gabest
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

SSE2I_INSTRUCTION(pand,_mm_and_si128)
SSE2I_INSTRUCTION(por,_mm_or_si128)
SSE2I_INSTRUCTION(pxor,_mm_xor_si128)
SSE2I_INSTRUCTION(packuswb,_mm_packus_epi16)
SSE2I_INSTRUCTION(packsswb,_mm_packs_epi16)
SSE2I_INSTRUCTION(packssdw,_mm_packs_epi32)
SSE2I_INSTRUCTION(punpcklbw,_mm_unpacklo_epi8)
SSE2I_INSTRUCTION(punpckhbw,_mm_unpackhi_epi8)
SSE2I_INSTRUCTION(punpcklwd,_mm_unpacklo_epi16)
SSE2I_INSTRUCTION(punpckhwd,_mm_unpackhi_epi16)
SSE2I_INSTRUCTION(punpckldq,_mm_unpacklo_epi32)
SSE2I_INSTRUCTION(pmullw,_mm_mullo_epi16)
SSE2I_INSTRUCTION(pmulhw,_mm_mulhi_epi16)
SSE2I_INSTRUCTION(paddsb,_mm_adds_epi8)
SSE2I_INSTRUCTION(paddb,_mm_add_epi8)
SSE2I_INSTRUCTION(paddw,_mm_add_epi16)
SSE2I_INSTRUCTION(paddsw,_mm_adds_epi16)
SSE2I_INSTRUCTION(paddusw,_mm_adds_epu16)
SSE2I_INSTRUCTION(paddd,_mm_add_epi32)
SSE2I_INSTRUCTION(psubw,_mm_sub_epi16)
SSE2I_INSTRUCTION(psubsw,_mm_subs_epi16)
SSE2I_INSTRUCTION(psubusb,_mm_subs_epu8)
SSE2I_INSTRUCTION(psubd,_mm_sub_epi32)
SSE2I_INSTRUCTION(pmaddwd,_mm_madd_epi16)
SSE2I_INSTRUCTION(pavgb,_mm_avg_epu8)
SSE2I_INSTRUCTION(pcmpeqb,_mm_cmpeq_epi8)
SSE2I_INSTRUCTION(pcmpeqw,_mm_cmpeq_epi16)
SSE2I_INSTRUCTION(pcmpgtb,_mm_cmpgt_epi8)
SSE2I_INSTRUCTION(pcmpgtw,_mm_cmpgt_epi16)
SSE2I_INSTRUCTION(paddusb,_mm_adds_epu8)
