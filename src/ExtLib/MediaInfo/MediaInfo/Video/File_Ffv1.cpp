/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// http://www.ffmpeg.org/~michael/ffv1.html
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FFV1_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_Ffv1.h"
#include "ZenLib/BitStream.h"
//---------------------------------------------------------------------------

#include <algorithm>
using namespace std;

//---------------------------------------------------------------------------
namespace MediaInfoLib
{

//***************************************************************************
// Const
//***************************************************************************

extern int32u Psi_CRC_32_Table[256];

//***************************************************************************
// RangeCoder
//***************************************************************************

//---------------------------------------------------------------------------
RangeCoder::RangeCoder (const int8u* Buffer, size_t Buffer_Size, const state_transitions default_state_transition)
{
    //Assign buffer
    Buffer_Cur=Buffer;
    Buffer_End=Buffer+Buffer_Size;

    //Init
    if (Buffer_Size>=2)
    {
        Current=BigEndian2int16u(Buffer_Cur);
        Buffer_Cur+=2;
        Mask=0xFF00;
    }
    else
    {
        Current=0;
        Mask=0;
    }

    AssignStateTransitions(default_state_transition);
}

void RangeCoder::AssignStateTransitions (const state_transitions new_state_transition)
{
    //Assign StateTransitions
    std::memcpy (one_state, new_state_transition, state_transitions_size);
    zero_state[0]=0;
    for (size_t i=1; i<state_transitions_size; i++)
        zero_state[i]=-one_state[state_transitions_size-i];
}

//---------------------------------------------------------------------------
bool RangeCoder::get_rac(int8u States[])
{
    //Here is some black magic... But it works. TODO: better understanding of the algorithm and maybe optimization
    int16u Mask2=(int16u)((((int32u)Mask) * (*States)) >> 8);
    Mask-=Mask2;
    bool Value;
    if (Current<Mask)
    {
        *States=zero_state[*States];
        Value=false;
    }
    else
    {
        Current-=Mask;
        Mask=Mask2;
        *States=one_state[*States];
        Value=true;
    }

    // Next byte
    if (Mask<0x100)
    {
        Mask<<=8;
        Current<<=8;
        if (Buffer_Cur < Buffer_End) // Set to 0 if end of stream. TODO: Find a way to detect buffer underrun
        {
            Current|=*Buffer_Cur;
            Buffer_Cur++;
        }
    }

    return Value;
}

//---------------------------------------------------------------------------
int32u RangeCoder::get_symbol_u(states &States)
{
    if (get_rac(States))
        return 0;

    int8u e=0;
    while (get_rac((States+1+min(e, (int8u)9)))) // 1..10
        e++;

    int8u a=1;
    if (e)
    {
        do
        {
            --e;
            a<<=1;
            if (get_rac((States+22+min(e, (int8u)9))))  // 22..31
                ++a;
        }
        while (e);
    }
    return a;
}

//---------------------------------------------------------------------------
int32s RangeCoder::get_symbol_s(states &States)
{
    if (get_rac(States))
        return 0;

    int8u e=0;
    while (get_rac(States+1+min(e, (int8u)9))) // 1..10
        e++;

    int8u a=1;
    if (e)
    {
        int8u i = e;
        do
        {
            --i;
            a<<=1;
            if (get_rac((States+22+min(i, (int8u)9))))  // 22..31
                ++a;
        }
        while (i);
    }

    if (get_rac((States+11+min(e, (int8u)10)))) // 11..21
        return -((int32s)a);
    else
        return a;
}

//***************************************************************************
// Info
//***************************************************************************

const char* Ffv1_coder_type(int8u coder_type)
{
    switch (coder_type)
    {
        case 0 :
                return "Golomb Rice";
        case 1 :
        case 2 :
                return "Range Coder";
        default:
                return "";
    }
}

const string Ffv1_colorspace_type(int8u colorspace_type, bool chroma_planes, bool alpha_plane)
{
    string ToReturn;
    switch (colorspace_type)
    {
        case 0 :
                    ToReturn=chroma_planes?"YUV":"Y";
                    break;
        case 1 :    ToReturn="RGB"; break;
        default:    return string();
    }

    if (alpha_plane)
        ToReturn+='A';

    return ToReturn;
}

const state_transitions Ffv1_default_state_transition =
{
      0,  0,  0,  0,  0,  0,  0,  0, 20, 21, 22, 23, 24, 25, 26, 27,
     28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 37, 38, 39, 40, 41, 42,
     43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
     74, 75, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
     89, 90, 91, 92, 93, 94, 94, 95, 96, 97, 98, 99,100,101,102,103,
    104,105,106,107,108,109,110,111,112,113,114,114,115,116,117,118,
    119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,133,
    134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,
    150,151,152,152,153,154,155,156,157,158,159,160,161,162,163,164,
    165,166,167,168,169,170,171,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,190,191,192,194,194,
    195,196,197,198,199,200,201,202,202,204,205,206,207,208,209,209,
    210,211,212,213,215,215,216,217,218,219,220,220,222,223,224,225,
    226,227,227,229,229,230,231,232,234,234,235,236,237,238,239,240,
    241,242,243,244,245,246,247,248,248,  0,  0,  0,  0,  0,  0,  0,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ffv1::File_Ffv1()
:File__Analyze()
{
    //Configuration
    ParserName=__T("FFV1");
    IsRawStream=true;

    //Temp
    ConfigurationRecordIsPresent=false;
    RC=NULL;
    version=0;
}

//---------------------------------------------------------------------------
File_Ffv1::~File_Ffv1()
{
    //Temp
    delete RC; //RC=NULL
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::Streams_Accept()
{
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_Format, "FFV1");
}

//***************************************************************************
// RangeCoder
//***************************************************************************

#if MEDIAINFO_TRACE
//---------------------------------------------------------------------------
void File_Ffv1::Get_RB (states &States, bool &Info, const char* Name)
{
    Info=RC->get_rac(States);

    if (Trace_Activated)
    {
        Element_Offset=RC->Buffer_Cur-Buffer;
        Param(Name, Info);
    }
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RU (states &States, int32u &Info, const char* Name)
{
    Info=RC->get_symbol_u(States);

    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS (states &States, int32s &Info, const char* Name)
{
    Info=RC->get_symbol_s(States);

    if (Trace_Activated)
        Param(Name, Info);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RC (states &States, const char* Name)
{
    if (Trace_Activated)
    {
        int8u Info=RC->get_rac(States);
        Element_Offset=RC->Buffer_Cur-Buffer;
        Param(Name, Info);
    }
    else
        RC->get_rac(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RU (states &States, const char* Name)
{
    if (Trace_Activated)
        Param(Name, RC->get_symbol_u(States));
    else
        RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RS (states &States, const char* Name)
{
    if (Trace_Activated)
        Param(Name, RC->get_symbol_s(States));
    else
        RC->get_symbol_s(States);
}

#else //MEDIAINFO_TRACE
//---------------------------------------------------------------------------
void File_Ffv1::Get_RB_ (states &States, bool &Info)
{
    Info=RC->get_rac(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RU_ (states &States, int32u &Info)
{
    Info=RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Get_RS_ (states &States, int32s &Info)
{
    Info=RC->get_symbol_s(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RC_ (states &States)
{
    RC->get_rac(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RU_ (states &States)
{
    RC->get_symbol_u(States);
}

//---------------------------------------------------------------------------
void File_Ffv1::Skip_RS_ (states &States)
{
    RC->get_symbol_s(States);
}

#endif //MEDIAINFO_TRACE

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::Read_Buffer_OutOfBand()
{
    ConfigurationRecordIsPresent=true;
    
    int32u CRC_32=0;
    const int8u* CRC_32_Buffer=Buffer+Buffer_Offset+(size_t)Element_Offset;
    const int8u* CRC_32_Buffer_End=CRC_32_Buffer+(size_t)Element_Size;

    while(CRC_32_Buffer<CRC_32_Buffer_End)
    {
        CRC_32=(CRC_32<<8) ^ Psi_CRC_32_Table[(CRC_32>>24)^(*CRC_32_Buffer)];
        CRC_32_Buffer++;
    }

    if (Buffer_Size < 4 || CRC_32)
    {
        Reject();
        return;
    }

    if (!RC)
        RC = new RangeCoder(Buffer, Buffer_Size-4, Ffv1_default_state_transition);

    FrameHeader();
    if (RC->Buffer_End!=RC->Buffer_Cur)
        Skip_XX(RC->Buffer_Cur - RC->Buffer_End,                "Reserved");
    Skip_B4(                                                    "CRC-32");

    delete RC; RC=NULL;
}

//---------------------------------------------------------------------------
void File_Ffv1::Read_Buffer_Continue()
{
    if (!Status[IsAccepted])
        Accept();

    if (!RC)
        RC = new RangeCoder(Buffer, Buffer_Size, Ffv1_default_state_transition);

    states States;
    memset(States, 128, states_size);

    bool keyframe;
    Get_RB (States, keyframe,                                   "keyframe");

    if (!ConfigurationRecordIsPresent)
        FrameHeader();

    if (version>2)
    {
        int64u Slices_BufferPos=Element_Size;
        vector<int32u> Slices_BufferSizes;
        while (Slices_BufferPos)
        {
            int32u Size = BigEndian2int24u(Buffer + Buffer_Offset + (size_t)Slices_BufferPos - 8);
            Slices_BufferSizes.insert(Slices_BufferSizes.begin(), Size);
            Slices_BufferPos-=8+Size;
        }

        Element_Offset=0;
        for (size_t Pos = 0; Pos < Slices_BufferSizes.size(); Pos++)
        {
            Element_Begin1("Slice");
            int64u End=Element_Offset+Slices_BufferSizes[Pos];
        
            if (Pos)
            {
                delete RC; RC = new RangeCoder(Buffer+Buffer_Offset+(size_t)Element_Offset, Slices_BufferSizes[Pos], custom_state_transitions); //Ffv1_default_state_transition);
            }
            else // ac=2
                RC->AssignStateTransitions(custom_state_transitions);

            //slice(States); // Not yet fully implemented
        
            Skip_XX(End-Element_Offset,                             "Slice data");
            Skip_B3(                                                "slice_size");
            Skip_B1(                                                "error_status");
            Skip_B4(                                                "crc_parity");
            Element_End0();

            break; //TEMP
        }
    }

    Skip_XX(Element_Size-Element_Offset,                            "Other data"); // Not yet fully implemented

    FILLING_BEGIN();
        Frame_Count++;
    FILLING_END();

    Finish();
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Ffv1::FrameHeader()
{
    //Parsing
    states States;
    memset(States, 128, states_size);
    int32u micro_version=0, coder_type, colorspace_type, bits_per_raw_sample=8, chroma_h_shift, chroma_v_shift, num_h_slices_minus1=0, num_v_slices_minus1=0, ec, intra;
    bool chroma_planes, alpha_plane;
    Get_RU (States, version,                                    "version");
    if (( ConfigurationRecordIsPresent && version<=1)
     || (!ConfigurationRecordIsPresent && version> 1))
    {
        Trusted_IsNot("Invalid version in global header");
        return;
    }
    if (version>2)
        Get_RU (States, micro_version,                          "micro_version");
    Get_RU (States, coder_type,                                 "coder_type");
    if (coder_type == 2) //Range coder with custom state transition table
    {
        Element_Begin1("state_transition_deltas");
        for (int16u i = 1; i < state_transitions_size; i++)
        {
            int32s state_transition_delta;
            Get_RS (States, state_transition_delta,             "state_transition_delta");
            custom_state_transitions[i]=state_transition_delta+RC->one_state[i];
            Param_Info1(custom_state_transitions[i]);
        }
        Element_End0();
    }
    Get_RU (States, colorspace_type,                            "colorspace_type");
    if (version)
    {
        Get_RU (States, bits_per_raw_sample,                    "bits_per_raw_sample");
        if (bits_per_raw_sample==0)
            bits_per_raw_sample=8; //I don't know the reason, 8-bit is coded 0 and 10-bit coded 10 (not 2?).
    }
    Get_RB (States, chroma_planes,                              "chroma_planes");
    Get_RU (States, chroma_h_shift,                             "log2(h_chroma_subsample)");
    Get_RU (States, chroma_v_shift,                             "log2(v_chroma_subsample)");
    Get_RB (States, alpha_plane,                                "alpha_plane");
    plane_count=1+(chroma_planes?0:1)+(alpha_plane?0:1); //Warning: chroma is considered as 1 plane
    if (version>1)
    {
        Get_RU (States, num_h_slices_minus1,                    "num_h_slices_minus1");
        Get_RU (States, num_v_slices_minus1,                    "num_v_slices_minus1");
    }
    int32u quant_table_count;
    if (version>1)
        Get_RU (States, quant_table_count,                      "quant_table_count");
    else
        quant_table_count=1;
    for (size_t i = 0; i < quant_table_count; i++)
        read_quant_tables(i);
    memset(quant_tables+quant_table_count, 0x00, (MAX_QUANT_TABLES-quant_table_count)*MAX_CONTEXT_INPUTS*256*sizeof(int16s));

    for (size_t i = 0; i < quant_table_count; i++)
    {
        Element_Begin1("initial_state");
        bool present;
        Get_RB (States, present,                                "present");
        if (present)
            for (size_t j = 0; j < context_count[i]; j++)
            {
                Element_Begin1("initial_state");
                    for (size_t k = 0; k < states_size; k++)
                    {
                        int32s value;
                        Get_RS (States, value,                  "value");
                    }
                Element_End0();
            }
        Element_End0();
    }

    if (version > 2)
    {
        Get_RU (States, ec,                                     "ec");
        if (micro_version)
            Get_RU (States, intra,                              "intra");
    }

    FILLING_BEGIN();
        if (Frame_Count==0)
        {
            Accept();
        
            Ztring Version=__T("Version ")+Ztring::ToZtring(version);
            if (version>2)
            {
                Version+=__T('.');
                Version+=Ztring::ToZtring(micro_version);
            }
            Fill(Stream_Video, 0, "coder_type", Ffv1_coder_type(coder_type));
            Fill(Stream_Video, 0, Video_Format_Version, Version);
            Fill(Stream_Video, 0, Video_BitDepth, bits_per_raw_sample);
            if (version > 1)
            {
                Fill(Stream_Video, 0, "MaxSlicesCount", (num_h_slices_minus1+1)*(num_v_slices_minus1+1));
            }
            if (version > 2)
            {
                if (ec)
                    Fill(Stream_Video, 0, "ErrorDetectionType", "Per slice");
                if (micro_version && intra)
                    Fill(Stream_Video, 0, Video_Format_Settings_GOP, "N=1");
            }
            Fill(Stream_Video, 0, Video_ColorSpace, Ffv1_colorspace_type(colorspace_type, chroma_planes, alpha_plane));
            if (colorspace_type==0 && chroma_planes)
            {
                string ChromaSubsampling;
                switch (chroma_h_shift)
                {
                    case 0 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:4:4"; break;
                                default: ;
                            }
                            break;
                    case 1 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:2:2"; break;
                                case 1 : ChromaSubsampling="4:2:0"; break;
                                default: ;
                            }
                            break;
                    case 2 :
                            switch (chroma_v_shift)
                            {
                                case 0 : ChromaSubsampling="4:1:1"; break;
                                case 1 : ChromaSubsampling="4:1:0"; break;
                                case 2 : ChromaSubsampling="4:1:0 (4x4)"; break;
                                default: ;
                            }
                            break;
                    default: ;
                }
                if (!ChromaSubsampling.empty() && alpha_plane)
                    ChromaSubsampling+=":4";
                Fill(Stream_Video, 0, Video_ChromaSubsampling, ChromaSubsampling);
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Ffv1::slice(states &States)
{
    if (version>2)
    {
        slice_header(States);

    }

    if (!true) //fs->ac
    {
        if (true) //f->version == 3 && f->micro_version > 1 || f->version > 3
        {
            states States;
            memset(States, 129, states_size);
            Skip_RC(States,                                     "?");
        }
    }

    Slice.w=720/2;
    Slice.h=608/2;
    Slice.sample_buffer=new int16s[(Slice.w + 6) * 3 * MAX_PLANES];

    if (true) //colorspace == 0
    {
        if (true) //bits_per_raw_sample >8
        {
            plane(quant_tables[quant_table_index[0]]); //TODO 0
        }
    }
}

//---------------------------------------------------------------------------
void File_Ffv1::slice_header(states &States)
{
    memset(States, 128, states_size);

    int32u slice_x, slice_y, slice_width, slice_height;
    Get_RU (States, slice_x,                                "slice_x");
    Get_RU (States, slice_y,                                "slice_y");
    Get_RU (States, slice_width,                            "slice_width_minus1");
    Get_RU (States, slice_height,                           "slice_height_minus1");
    for (int8u i = 0; i < plane_count; i++)
    {
        Get_RU (States, quant_table_index[i],               "quant_table_index");
    }
    Skip_RU(States,                                         "picture_structure");
    Skip_RU(States,                                         "sample_aspect_ratio num");
    Skip_RU(States,                                         "sample_aspect_ratio den");
    if (version > 3)
    {
        //TODO
    }

    RC->AssignStateTransitions(custom_state_transitions);
}

//---------------------------------------------------------------------------
void File_Ffv1::plane(int16s quant_table[MAX_CONTEXT_INPUTS][256])
{
    Element_Begin1("Plane");

    int x, y;
    int16s *sample[2];
    sample[0] = Slice.sample_buffer + 3;
    sample[1] = Slice.sample_buffer + Slice.w + 6 + 3;

    memset(Slice.sample_buffer, 0, 2 * (Slice.w + 6) * sizeof(*Slice.sample_buffer));

    states States[MAX_CONTEXT_INPUTS];
    memset(States, 128, states_size*MAX_CONTEXT_INPUTS);
 
    for (size_t y = 0; y < 0/*h*/; y++)
    {
        int16s *temp = sample[0]; // FIXME: try a normal buffer

        sample[0] = sample[1];
        sample[1] = temp;

        sample[1][-1] = sample[0][0];
        sample[0][Slice.w]  = sample[0][Slice.w - 1];

        //if (s->avctx->bits_per_raw_sample <= 8)
        {
            /*
            decode_line(s, w, sample, plane_index, 8);
            for (x = 0; x < w; x++)
                src[x + stride * y] = sample[1][x];
                */
        }
        //else
        {
            Element_Begin1("Line");
            Element_Info1(y);

            //decode_line(s, w, sample, plane_index, s->avctx->bits_per_raw_sample);
            line(States, quant_table, sample);
            
            /*
            if (s->packed_at_lsb)
            {
                for (x = 0; x < w; x++) {
                    ((uint16s*)(src + stride*y))[x] = sample[1][x];
                }
            }
            else
            {
                for (x = 0; x < w; x++) {
                    ((uint16s*)(src + stride*y))[x] = sample[1][x] << (16 - s->avctx->bits_per_raw_sample);
                }
            }
            */

            Element_End0();
        }
    }

    Element_End0();
}

static inline int get_context(int16s quant_table[MAX_CONTEXT_INPUTS][256], int16s *src, int16s *last, int16s *last2)
{
    const int LT = last[-1];
    const int T  = last[0];
    const int RT = last[1];
    const int L  = src[-1];

    if (quant_table[3][127])
    {
        const int TT = last2[0];
        const int LL = src[-2];
        return quant_table[0][(L - LT) & 0xFF]
             + quant_table[1][(LT - T) & 0xFF]
             + quant_table[2][(T - RT) & 0xFF]
             + quant_table[3][(LL - L) & 0xFF]
             + quant_table[4][(TT - T) & 0xFF];
    }
    else
        return quant_table[0][(L - LT) & 0xFF]
             + quant_table[1][(LT - T) & 0xFF]
             + quant_table[2][(T - RT) & 0xFF];
}

/* median of 3 */
static inline int mid_pred(int a, int b, int c)
{
#if 0
    int t= (a-b)&((a-b)>>31);
    a-=t;
    b+=t;
    b-= (b-c)&((b-c)>>31);
    b+= (a-b)&((a-b)>>31);

    return b;
#else
    if(a>b){
        if(c>b){
            if(c>a) b=a;
            else    b=c;
        }
    }else{
        if(b>c){
            if(c>a) b=c;
            else    b=a;
        }
    }
    return b;
#endif
}

static inline int predict(int16s *src, int16s *last)
{
    const int LT = last[-1];
    const int T  = last[0];
    const int L  = src[-1];

    return mid_pred(L, L + T - LT, T);
}

        static int A=0;
//---------------------------------------------------------------------------
void File_Ffv1::line(states States[MAX_CONTEXT_INPUTS], int16s quant_table[MAX_CONTEXT_INPUTS][256], int16s *sample[2])
{
    for (size_t x = 0; x < Slice.w; x++)
    {
        int context, sign;

        context = get_context(quant_table, sample[1] + x, sample[0] + x, sample[1] + x);
        if (context < 0)
        {
            context = -context;
            sign    = 1;
        }
        else
            sign = 0;
        if (context>5)
            int b=0;
        
        int32s diff;
        Get_RS(States[context], diff, "diff"); Element_Info1(diff);
        if (sign)
            diff = -diff;

        A++;
        if (A<360)
            sample[1][x] = (predict(sample[1] + x, sample[0] + x) + diff) &
                       ((1 << 8) - 1); //bits
        else
            int b=0;

    }
}

//---------------------------------------------------------------------------
void File_Ffv1::read_quant_tables(int i)
{
    Element_Begin1("quant_table");

    int scale = 1;

    for (int j = 0; j < 5; j++)
    {
        read_quant_table(i, j, scale);
        scale *= 2 * len_count[i][j] - 1;
        if (scale > 32768U)
        {
            Element_End0();
            return;
        }

        context_count[i] = (scale + 1) / 2;
    }

    Element_End0();
}

//---------------------------------------------------------------------------
void File_Ffv1::read_quant_table(int i, int j, size_t scale)
{
    Element_Begin1("per context");

    ;

    int8u States[states_size];
    memset(States, 128, sizeof(States));

    int v = 0;
    for (int k=0; k < 128;)
    {
        int32u len_minus1;
        Get_RU (States, len_minus1,                             "len_minus1");

        if (k+len_minus1 >= 128)
        {
            Element_End0();
            return;
        }

        for (int a=0; a<=len_minus1; a++)
        {
            quant_tables[i][j][k] = scale * v;
            k++;
        }

        v++;
    }

    for (int k = 1; k < 128; k++)
        quant_tables[i][j][256 - k] = -quant_tables[i][j][k];
    quant_tables[i][j][128] = -quant_tables[i][j][127];

    len_count[i][j]=v;

    Element_End0();
}

} //NameSpace

#endif //MEDIAINFO_FFV1_YES
