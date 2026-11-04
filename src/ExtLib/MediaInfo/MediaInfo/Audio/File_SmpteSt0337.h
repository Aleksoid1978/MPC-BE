/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Non-PCM Audio and Data in an AES3
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_SmpteSt0337H
#define MediaInfo_File_SmpteSt0337H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Pcm.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_SmpteSt0337
//***************************************************************************

class File_SmpteSt0337 : public File_Pcm_Base
{
public :
    // In
    int8u   Endianness;
    bool    Aligned;

    // Constructor/Destructor
    File_SmpteSt0337();
    ~File_SmpteSt0337();

private :
    // Streams management
    void Streams_Accept();
    void Streams_Fill();
    void Streams_Finish();

    // Buffer - Global
    #if MEDIAINFO_SEEK
    void Read_Buffer_Unsynched();
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif // MEDIAINFO_SEEK

    // Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Synched_Init();

    // Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    // Elements
    void Raw();
    void Frame();
    void Frame_WithPadding();
    void Frame_FromMpegPs();

    // Temp
    float64 FrameRate;
    int8u   Stream_Bits;
    int8u   Padding;
    int32u  data_type;
    std::map<int64u, int64u> FrameSizes;
    int64s  GuardBand_Before;
    int64s  GuardBand_After;
    size_t  NullPadding_Size;

    // Parser
    File__Analyze* Parser;
    void Parser_Parse(const int8u* Parser_Buffer, size_t Parser_Buffer_Size);

    // Conformance with SMPTE ST 337 and associated standards (SMPTE ST 338, SMPTE ST 340)
    #if MEDIAINFO_CONFORMANCE
        struct burst_preamble
        {
            int64u  Begin; // Offset of Pa
            int64u  Size; // Size of the burst with its preamble
            int32u  length_code;
            int32u  extended_data_type; // Pe, (int32u)-1 if not present
            int32u  Pf;
            int8u   data_stream_number;
            int8u   data_type_dependent;
            int8u   data_mode;
            int8u   data_type;
            int8u   Pc_Reserved;
            int8u   Pe_Reserved;
        };
        int8u   Conformance_Generation; // Index of the edition to check
        bool    Conformance_DataType_IsChecked; // Data types not defined in the edition are reported, only if an edition is requested
        int8u   Conformance_DataStreamNumbers; // Bit per data_stream_number of bursts other than null data
        int64u  Conformance_Burst_End; // Offset after the previous burst, (int64u)-1 if unknown
        int64u  Conformance_Frames_SinceExtendedSync; // AES3 frames since the last burst preceded by 4 subframes of 0
        int64u  Conformance_Ac3_Burst_Last[7]; // AES3 frame of the last AC-3 or E-AC-3 burst per data_stream_number, (int64u)-1 if unknown
        void    Conformance_Burst(const burst_preamble& Burst);
        void    Conformance_Unsynch();
    #endif //MEDIAINFO_CONFORMANCE

    #if MEDIAINFO_SEEK
        bool                        Duration_Detected;
    #endif // MEDIAINFO_SEEK
};

} // NameSpace

#endif

