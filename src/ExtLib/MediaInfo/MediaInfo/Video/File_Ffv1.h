/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about FFV1 files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Ffv1H
#define MediaInfo_Ffv1H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class RangeCoder
//***************************************************************************

const size_t states_size=32;
const size_t state_transitions_size=256;
typedef int8u states[states_size];
typedef int8u state_transitions[state_transitions_size];

const size_t MAX_PLANES=4;
const size_t MAX_QUANT_TABLES=8;
const size_t MAX_CONTEXT_INPUTS=5;

class RangeCoder
{
public :
    RangeCoder(const int8u* Buffer, size_t Buffer_Size, const state_transitions default_state_transition);

    void AssignStateTransitions (const state_transitions new_state_transition);

    bool    get_rac(int8u States[]);
    int32u  get_symbol_u(states &States);
    int32s  get_symbol_s(states &States);

    int16u Current;
    int16u Mask;
    state_transitions zero_state;
public : //Temp
    state_transitions one_state;
    const int8u* Buffer_Cur;
    const int8u* Buffer_End;

};

//***************************************************************************
// Class File_Ffv1
//***************************************************************************

class File_Ffv1 : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Ffv1();
    ~File_Ffv1();

private :
    //Streams management
    void Streams_Accept();

    //Buffer - Global
    void Read_Buffer_OutOfBand();
    void Read_Buffer_Continue();

    //Elements
    void FrameHeader();
    void slice(states &States);
    void slice_header(states &States);
    void plane(int16s quant_table[MAX_CONTEXT_INPUTS][256]);
    void line(states States[MAX_CONTEXT_INPUTS], int16s quant_table[MAX_CONTEXT_INPUTS][256], int16s *sample[2]);
    void read_quant_tables(int i);
    void read_quant_table(int i, int j, size_t scale);

    //Range coder
    #if MEDIAINFO_TRACE
        void Get_RB (states &States, bool  &Info, const char* Name);
        void Get_RU (states &States, int32u &Info, const char* Name);
        void Get_RS (states &States, int32s &Info, const char* Name);
        void Skip_RC(states &States,              const char* Name);
        void Skip_RU(states &States,              const char* Name);
        void Skip_RS(states &States,              const char* Name);
        #define Info_RC(_STATE, _INFO, _NAME) bool  _INFO; Get_RB (_STATE, _INFO, _NAME)
        #define Info_RU(_STATE, _INFO, _NAME) int32u _INFO; Get_RU (_STATE, _INFO, _NAME)
        #define Info_RS(_STATE, _INFO, _NAME) int32s _INFO; Get_RS (_STATE, _INFO, _NAME)
    #else //MEDIAINFO_TRACE
        void Get_RB_ (states &States, bool  &Info);
        void Get_RU_ (states &States, int32u &Info);
        void Get_RS_ (states &States, int32s &Info);
        #define Get_RB(Bits, Info, Name) Get_RB_(Bits, Info)
        #define Get_RU(Bits, Info, Name) Get_RU_(Bits, Info)
        #define Get_RS(Bits, Info, Name) Get_RS_(Bits, Info)
        void Skip_RC_(states &States);
        void Skip_RU_(states &States);
        void Skip_RS_(states &States);
        #define Skip_RC(Bits, Name) Skip_RC_(Bits)
        #define Skip_RU(Bits, Name) Skip_RU_(Bits)
        #define Skip_RS(Bits, Name) Skip_RS_(Bits)
        #define Info_RC(_STATE, _INFO, _NAME) Skip_RC_(_STATE)
        #define Info_RU(_STATE, _INFO, _NAME) Skip_RU_(_STATE)
        #define Info_RS(_STATE, _INFO, _NAME) Skip_RS_(_STATE)
    #endif //MEDIAINFO_TRACE
    RangeCoder* RC;

    //Temp
    bool    ConfigurationRecordIsPresent;
    int     context_count[MAX_QUANT_TABLES];
    int     len_count[MAX_QUANT_TABLES][MAX_CONTEXT_INPUTS];
    int16s  quant_tables[MAX_QUANT_TABLES][MAX_CONTEXT_INPUTS][256];
    int32u  quant_table_index[MAX_PLANES];
    int32u  version;
    int8u   plane_count;
    state_transitions custom_state_transitions;

    struct slice_struct
    {
        int32u  w;
        int32u  h;
        int16s* sample_buffer;
    };
    slice_struct Slice;
};

} //NameSpace

#endif
