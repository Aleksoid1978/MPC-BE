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
typedef int8u **states_context_plane;

const size_t MAX_PLANES=4;
const size_t MAX_QUANT_TABLES=8;
const size_t MAX_CONTEXT_INPUTS=5;

class RangeCoder;

//***************************************************************************
// Class Slice
//***************************************************************************

const int32u RUN_MODE_STOP = 0;
const int32u RUN_MODE_PROCESSING = 1;
const int32u RUN_MODE_INTERRUPTED = 2;

namespace FFV1
{
    typedef int32s pixel_t;
}

class Slice
{
public:

    Slice() : run_index(0), run_mode(RUN_MODE_STOP),
        sample_buffer(NULL)
    {
        for (size_t i = 0; i < MAX_QUANT_TABLES; ++i)
            plane_states[i] = NULL;

        for (size_t i = 0; i < MAX_PLANES; ++i)
            contexts[i] = NULL;
    }

    ~Slice()
    {
        if (sample_buffer)
        {
            delete [] sample_buffer;
            sample_buffer = NULL;
        }
        contexts_clean();
    }

    void    sample_buffer_new(size_t size)
    {
        if (sample_buffer)
        {
            delete [] sample_buffer;
            sample_buffer = NULL;
        }
        sample_buffer = new FFV1::pixel_t[size];
    }

    void    run_index_init() { run_index=0; }
    void    run_mode_init() { run_segment_length = 0; run_mode=RUN_MODE_STOP; }

    //TEMP
    int32u  x;
    int32u  y;
    int32u  w;
    int32u  h;
    int32u  run_index;
    int32u  run_mode;
    int32s  run_segment_length;
    FFV1::pixel_t* sample_buffer;

    class Context
    {
    public:
        static const int32u N0; // Determined threshold to divide N and B
        static const int32s Cmax; // Storage limitation, C never upper than 127
        static const int32s Cmin; // Storage limitation, C never below than -128
        int32s N; // Count where the context was encountred
        int32s B; // Accumulated sum of corrected prediction residuals
        int32s A; // Accumulated sum of the absolute corrected prediction residuals (St + Nt)
        int32s C; // Correction value

        Context() :
            N(1),
            B(0),
            A(4),
            C(0)
        {}

        inline void update_correlation_value_and_shift()
        {
            // Step 11: Resets
            if (N == N0)
            {
                // divide by 2, if >= 0 : ROUND, otherwise, CEIL
                N >>= 1;
                A >>= 1;
                B >>= 1;
            }
            ++N; // context meets

                 // Step 12: Bias computation procedure
                 // Keep B in (-N;0]
            if (B <= -N)
            {
                if (C > Cmin)
                    --C;

                B += N;
                if (B <= -N)
                    B = -N + 1;
            }
            else if (B > 0)
            {
                if (C < Cmax)
                    ++C;

                if (B > N)
                    B = 0;
                else
                    B -= N;
            }
        }
        inline int compute_k()
        {
            int k = 0;
            while ((N << k) < A)
                ++k;
            return k;
        }
    };

    typedef Context* ContextPtr;

    ContextPtr contexts[MAX_PLANES];
    states_context_plane plane_states[MAX_QUANT_TABLES];
    size_t               plane_states_maxsizes[MAX_QUANT_TABLES];

    // HELPER
    void contexts_init(int32u plane_count, int32u quant_table_index[MAX_PLANES], int32u context_count[MAX_QUANT_TABLES]);
    void contexts_clean();
};

//***************************************************************************
// Class File_Ffv1
//***************************************************************************

typedef FFV1::pixel_t quant_table_struct2[256];
typedef quant_table_struct2 quant_table_struct[MAX_CONTEXT_INPUTS];

class File_Ffv1 : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Ffv1();
    ~File_Ffv1();

    //Input
    int32u Width;
    int32u Height;

private :
    //Streams management
    void Streams_Accept();

    //Buffer - Global
    void Read_Buffer_OutOfBand();
    void Read_Buffer_Continue();
    void Skip_Frame();

    //Elements
    void   FrameHeader();
    void   Filling_FrameHeader(int32u intra);
    int    slice(states &States);
    int    slice_header(states &States);
    int32s get_symbol_with_bias_correlation(Slice::ContextPtr context);
    void rgb();
    void plane(int32u pos);
    void line(int pos, FFV1::pixel_t *sample[2]);
    int32s pixel_GR(int32s context);
    int32s pixel_RC(int32s context);
    void read_quant_tables(int i);
    void read_quant_table(int i, int j, size_t scale);
    void copy_plane_states_to_slice(int8u plane_count);

    //Range coder
    #if MEDIAINFO_TRACE
        void Get_RB (states &States, bool  &Info, const char* Name);
        void Get_RU (states &States, int32u &Info, const char* Name);
        void Get_RS (states &States, int32s &Info, const char* Name);
        void Get_RS (int8u* &States, int32s &Info, const char* Name);
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
        void Get_RS_ (int8u* &States, int32s &Info);
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
    Slice *slices;
    Slice *current_slice;

    //Temp
    bool    ConfigurationRecordIsPresent;
    bool    KeyFramePassed;
    int32u  context_count[MAX_QUANT_TABLES];
    int32u  len_count[MAX_QUANT_TABLES][MAX_CONTEXT_INPUTS];
    quant_table_struct quant_tables[MAX_QUANT_TABLES];
    int32u  quant_table_index[MAX_PLANES];
    int32u  quant_table_count;
    int32u  version;
    int32u  micro_version;
    int32u  error_correction;
    int32u  num_h_slices;
    int32u  num_v_slices;
    int32u  chroma_h_shift;
    int32u  chroma_v_shift;
    int32u  picture_structure;
    int32u  sample_aspect_ratio_num;
    int32u  sample_aspect_ratio_den;
    int8u   coder_type;
    int8u   colorspace_type;
    int8u   bits_per_sample;
    bool    keyframe;
    bool    chroma_planes;
    bool    alpha_plane;
    bool    is_overflow_16bit;
    state_transitions state_transitions_table;

    //TEMP
    static const int32u PREFIX_MAX = 12; //limit
    int8u bits_max;
    int32s bits_mask1;
    int32s bits_mask2;
    int32s bits_mask3;
    states_context_plane Context_RC; // Range Coder context
    Slice::ContextPtr    Context_GR; // Rice Golomb context
    size_t x;

    states_context_plane plane_states[MAX_QUANT_TABLES];
    size_t               plane_states_maxsizes[MAX_QUANT_TABLES];

    int32s golomb_rice_decode(int k);
    void plane_states_clean(states_context_plane states[MAX_QUANT_TABLES]);
};

} //NameSpace

#endif
