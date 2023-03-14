/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Aac_GeneralAudioH
#define MediaInfo_File_Aac_GeneralAudioH
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Huffman tables
//***************************************************************************

//---------------------------------------------------------------------------
// Optimal huffman from:
// "SELECTING AN OPTIMAL HUFFMAN DECODER FOR AAC"
// VLADIMIR Z. MESAROVIC , RAGHUNATH RAO, MIROSLAV V. DOKIC, and SACHIN DEO
// AES Convention Paper 5436

//---------------------------------------------------------------------------
//2-step 1st pass
struct hcb_struct_1
{
    const int8u Offset;
    const int8u Extra;
};

//2-step 2nd pass or binary
typedef int8s hcb_struct[5]; //0=bits to read (2step) or more to read (binary), 1-4: data (1-2 in case of pairs)

//---------------------------------------------------------------------------
// Scalefactor Huffman Codebook
extern const int8u huffman_sf[][2];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 1
extern const hcb_struct_1 huffman_01_1[];
extern const hcb_struct huffman_01[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 2
extern const hcb_struct_1 huffman_02_1[];
extern const hcb_struct huffman_02[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 2
extern const hcb_struct huffman_03[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 4
extern const hcb_struct_1 huffman_04_1[];
extern const hcb_struct huffman_04[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 5
extern const hcb_struct huffman_05[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 6
extern const hcb_struct_1 huffman_06_1[]; 
extern const hcb_struct huffman_06[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 7
extern const hcb_struct huffman_07[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 8
extern const hcb_struct_1 huffman_08_1[];
extern const hcb_struct huffman_08[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 9
extern const hcb_struct huffman_09[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 10
extern const hcb_struct_1 huffman_10_1[];
extern const hcb_struct huffman_10[];

//---------------------------------------------------------------------------
// Spectrum Huffman Codebook 11
extern const hcb_struct_1 huffman_11_1[];
extern const hcb_struct huffman_11[];

//---------------------------------------------------------------------------
extern const int8u hcb_2step_Bytes[];
extern const hcb_struct_1* hcb_2step[];

//---------------------------------------------------------------------------
extern const int16u hcb_table_size[];
extern const hcb_struct* hcb_table[];

//***************************************************************************
// PRED_SFB_MAX
//***************************************************************************

//---------------------------------------------------------------------------
extern const int8u Aac_PRED_SFB_MAX[];

//***************************************************************************
// swb offsets
//***************************************************************************

//---------------------------------------------------------------------------
struct Aac_swb_offset
{
    int8u  num_swb;
    int16u swb_offset[64];
};

//---------------------------------------------------------------------------
extern const Aac_swb_offset Aac_swb_short_window_96[];
extern const Aac_swb_offset Aac_swb_short_window_64[];
extern const Aac_swb_offset Aac_swb_short_window_48[];
extern const Aac_swb_offset Aac_swb_short_window_24[];
extern const Aac_swb_offset Aac_swb_short_window_16[];
extern const Aac_swb_offset Aac_swb_short_window_8[];
extern const  Aac_swb_offset* Aac_swb_offset_short_window[];

//---------------------------------------------------------------------------
extern const Aac_swb_offset Aac_swb_long_window_96[];
extern const Aac_swb_offset Aac_swb_long_window_64[];
extern const Aac_swb_offset Aac_swb_long_window_48[];
extern const Aac_swb_offset Aac_swb_long_window_32[];
extern const Aac_swb_offset Aac_swb_long_window_24[];
extern const Aac_swb_offset Aac_swb_long_window_16[];
extern const Aac_swb_offset Aac_swb_long_window_8[];
extern const Aac_swb_offset* Aac_swb_offset_long_window[];

} //NameSpace

#endif
