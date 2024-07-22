/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Digital Audio Tape
// https://nixdoc.net/man-pages/IRIX/man4/datframe.4.html
//---------------------------------------------------------------------------

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
#if defined(MEDIAINFO_DAT_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Dat.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Dat_pack_id[] =
{
    "Program Time",
    "Absolute Time",
    "Running Time",
    "Table of Contents",
    "Date",
    "Catalog Number",
    "International Standard Recording Code",
    "Pro Binary",
};
const auto Dat_pack_id_size = sizeof(Dat_pack_id) / sizeof(*Dat_pack_id);

//---------------------------------------------------------------------------
static const char* Dat_fmtid[4] =
{
    "for audio use",
    nullptr,
    nullptr,
    nullptr,
};

//---------------------------------------------------------------------------
static const char* Dat_emphasis[4] =
{
    "off",
    "50/15 ms",
    nullptr,
    nullptr,
};

//---------------------------------------------------------------------------
static const int16u Dat_sampfreq[4] =
{
    48000,
    44100,
    32000,
    0,
};

//---------------------------------------------------------------------------
static const int8u Dat_numchans[4] =
{
    2,
    4,
    0,
    0,
};

//---------------------------------------------------------------------------
static const int8u Dat_quantization[4] =
{
    16,
    12,
    0,
    0,
};

//---------------------------------------------------------------------------
static const char* Dat_trackpitch[4] =
{
    "Normal",
    "Wide",
    nullptr,
    nullptr,
};

//---------------------------------------------------------------------------
static const char* Dat_copy[4] =
{
    "Yes",
    nullptr,
    "No",
    nullptr,
};

//---------------------------------------------------------------------------
static const char* Dat_sid[4] =
{
    "SMPTE",
    "Local Pro DIO",
    "Time-of-day Pro DIO",
    nullptr,
};

//---------------------------------------------------------------------------
static const float Dat_xrate[8] =
{
    30,
    30 / 1.001, // NDF
    30 / 1.001, // DF
    25,
    24,
    0,
    0,
    0,
};

//---------------------------------------------------------------------------
enum items {
    item_emphasis,
    item_sampfreq,
    item_numchans,
    item_quantization,
    item_Max,
};

//---------------------------------------------------------------------------
class file_dat_frame {
public:
    TimeCode TCs[7] = {};
    int8u TC_IDs[7] = {};
    int16u dtsubid_pno = (int16u)-1;
};
class file_dat_priv {
public:
    file_dat_frame Frame_First;
    file_dat_frame Frame_Last;
    int8u Items[item_Max][4] = {};
    int8u Frame_Count_Valid = 32; // Mini 31 frames for catching wrong pro time codes
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dat::File_Dat()
: File__Analyze()
{
    //Configuration
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    Frame_Count_NotParsedIncluded=0;
}

//---------------------------------------------------------------------------
File_Dat::~File_Dat()
{
    delete Priv;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dat::Streams_Accept()
{
    auto Conditional = [&](int8u* Values) {
        int Max_Pos = 0;
        int Max_Value = 0;
        for (size_t i = 0; i < 4; i++) {
            if (Max_Value < Values[i]) {
                Max_Value = Values[i];
                Max_Pos = i;
            }
        }
        Max_Value >>= 2;
        for (size_t i = 0; i < 4; i++) {
            if (Max_Value < Values[i] && i != Max_Pos) {
                return -1;
            }
        }
        return Max_Pos;
    };
    auto Conditional_Int8 = [&](audio Field, items Item, const int8u Array[]) {
        auto Index = Conditional(Priv->Items[Item]);
        if (Index >= 0) {
            if (Array[Index]) {
                Fill(Stream_Audio, 0, Field, Array[Index]);
            }
            else {
                Fill(Stream_Audio, 0, Field, "Value" + to_string(Index));
            }
        }
    };
    auto Conditional_Int16 = [&](audio Field, items Item, const int16u Array[]) {
        auto Index = Conditional(Priv->Items[Item]);
        if (Index >= 0) {
            if (Array[Index]) {
                Fill(Stream_Audio, 0, Field, Array[Index]);
            }
            else {
                Fill(Stream_Audio, 0, Field, "Value" + to_string(Index));
            }
        }
        };
    auto Conditional_Char = [&](audio Field, items Item, const char* Array[]) {
        auto Index = Conditional(Priv->Items[Item]);
        if (Index >= 0) {
            if (Array[Index]) {
                Fill(Stream_Audio, 0, Field, Array[Index]);
            }
            else {
                Fill(Stream_Audio, 0, Field, "Value" + to_string(Index));
            }
        }
    };
    auto Conditional_TimeCode = [&](size_t i) {
        if (!Priv->Frame_First.TC_IDs[i]) {
            return;
        }
        Stream_Prepare(Stream_Other);
        TimeCode& First = Priv->Frame_First.TCs[i];
        if (First.IsValid()) {
            TimeCode& Last = Priv->Frame_Last.TCs[i];
            if (Last.IsValid()) {
                First.SetFramesMax(Last.GetFramesMax());
                First.SetDropFrame(Last.IsDropFrame());
                First.Set1001fps(Last.Is1001fps());
            }
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "SMPTE TC");
            Fill(Stream_Other, StreamPos_Last, Other_MuxingMode, Dat_pack_id[Priv->Frame_First.TC_IDs[i] - 1]);
            Fill(Stream_Other, StreamPos_Last, Other_BitRate, 0);
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_FirstFrame, First.ToString());
            Fill(Stream_Other, StreamPos_Last, Other_FrameRate, First.GetFramesMax() == 33 ? (100.0 / 3.0) : First.GetFrameRate());
        }
    };

    Fill(Stream_General, 0, General_Format, "DAT");
    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "PCM");
    Fill(Stream_Audio, 0, Audio_BitRate, float32_int64s(1536000 * 441 / 480));
    if (File_Size != (int64u)-1) {
        Fill(Stream_Audio, 0, Audio_StreamSize, float32_int64s((((float)File_Size / 5822) * 5760) * 441 / 480), 0);
    }
    Conditional_Char(Audio_Format_Settings_Emphasis, item_emphasis, Dat_emphasis);
    Conditional_Int16(Audio_SamplingRate, item_sampfreq, Dat_sampfreq);
    Conditional_Int8(Audio_Channel_s_, item_numchans, Dat_numchans);
    Conditional_Int8(Audio_BitDepth, item_quantization, Dat_quantization);
    for (int i = 0; i < 7; i++) {
        Conditional_TimeCode(i);
    }
}

//---------------------------------------------------------------------------
void File_Dat::Streams_Finish()
{
    auto Conditional_TimeCode = [&](size_t i, size_t& StreamPos) {
        if (!Priv->Frame_First.TC_IDs[i]) {
            return;
        }
        TimeCode& Last = Priv->Frame_Last.TCs[i];
        if (Last.IsValid()) {
            TimeCode& First = Priv->Frame_First.TCs[i];
            if (First.IsValid()) {
                Last.SetFramesMax(First.GetFramesMax());
                Last.SetDropFrame(First.IsDropFrame());
                Last.Set1001fps(First.Is1001fps());
            }
            Fill(Stream_Other, StreamPos, Other_TimeCode_LastFrame, Last.ToString());
        }
        StreamPos++;
    };

    size_t StreamPos = 0;
    for (int i = 0; i < 7; i++) {
        Conditional_TimeCode(i, StreamPos);
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dat::Read_Buffer_Unsynched()
{
    if (Priv) {
        Priv->Frame_Last = {};
    }
    FrameInfo = frame_info();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dat::Header_Parse()
{
    Header_Fill_Size(5822);
    Header_Fill_Code(0, "dtframe");
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dat::Data_Parse()
{
    Skip_XX(5760,                                               "audio");
    Element_Begin1("dtsubcode");
        BS_Begin();
        size_t numpacks_Min = 0;
        auto Frame = Priv ? Priv->Frame_Last : file_dat_frame();
        for (int i = 0; i < 7; i++) {
            auto ParityCheck_Cur = Buffer + (size_t)(Buffer_Offset + Element_Offset);
            auto ParityCheck_End = ParityCheck_Cur + 8;
            int8u Parity = 0;
            for (; ParityCheck_Cur < ParityCheck_End; ParityCheck_Cur++) {
                Parity ^= *ParityCheck_Cur;
            }
            Element_Begin1("dtsubcodepack");
            int8u id;
            Get_S1(4, id,                                       "id"); Element_Info1C(id && id <= Dat_pack_id_size, Dat_pack_id[id - 1]);
            if (id) {
                numpacks_Min++;
            }
            switch (id) {
            case 0:
                Skip_BS(52,                                     "padding");
                break;
            case 2:
            case 3:
                Frame.TC_IDs[i] = id;
                dttimepack(Frame.TCs[i]);
                break;
            default:
                Skip_BS(52,                                     "(Unknown)");
            }
            Skip_S1( 8,                                         "parity");
            if (Parity) {
                Trusted_IsNot("parity");
            }
            Element_End0();
        }
        Element_Begin1("dtsubid");
            int8u dataid, pno1, numpacks, pno2, pno3;
            Skip_SB(                                            "ctrlid - Priority ID");
            Skip_SB(                                            "ctrlid - Start ID");
            Skip_SB(                                            "ctrlid - Shortening ID");
            Skip_SB(                                            "ctrlid - TOC ID");
            Get_S1 ( 4, dataid,                                 "dataid");
            Get_S1 ( 4, pno1,                                   "pno (program number) - 1");
            Get_S1 ( 4, numpacks,                               "numpacks");
            Get_S1 ( 4, pno2,                                   "pno (program number) - 2");
            Get_S1 ( 4, pno3,                                   "pno (program number) - 3");
            Info_S1( 8, ipf,                                    "ipf (interpolation flags)");
            Skip_Flags(ipf, 6,                                  "left ipf");
            Skip_Flags(ipf, 7,                                  "right ipf");
            int16u pno = ((int16u)pno1 << 8) | (pno2 << 4) | pno3;
            auto pno_IsSpecial_Check = [](int8u pno) {
                return pno == 0xAA || pno == 0xBB || pno == 0xEE;
            };
            bool pno_CurrentIsSpecial = pno_IsSpecial_Check(pno);
            bool pno_IsValid = false;
            if ((pno1 < 8 && pno2 < 10 && pno3 < 10) || pno_CurrentIsSpecial) { // pno is BCD or 0x0AA meaning unknown or 0x0BB meaning lead-in or 0x0EE meaning lead-out
                bool pno_PreviousIsSpecial = pno_IsSpecial_Check(Frame.dtsubid_pno);
                if (pno_CurrentIsSpecial || pno_PreviousIsSpecial || pno == Frame.dtsubid_pno + 1) {
                    pno_IsValid = true;
                }
                Frame.dtsubid_pno = pno;
            }
            if (dataid || numpacks >= 8 || numpacks < numpacks_Min || !pno_IsValid) {
                Trusted_IsNot("dtsubid");
            }
        Element_End0();
        Element_Begin1("dtmainid");
            int8u fmtid, emphasis, sampfreq, numchans, quantization;
            Get_S1 ( 2, fmtid,                                  "fmtid"); Param_Info1C(Dat_fmtid[fmtid], Dat_fmtid[fmtid]);
            Get_S1 ( 2, emphasis,                               "emphasis"); Param_Info1C(Dat_emphasis[emphasis], Dat_emphasis[emphasis]);
            Get_S1 ( 2, sampfreq,                               "sampfreq"); Param_Info2C(Dat_sampfreq[sampfreq], Dat_sampfreq[sampfreq], " Hz");
            Get_S1 ( 2, numchans,                               "numchans"); Param_Info2C(Dat_numchans[numchans], Dat_numchans[numchans], " channels");
            Get_S1 ( 2, quantization,                           "quantization"); Param_Info2C(Dat_quantization[quantization], Dat_quantization[quantization], "bits");
            Info_S1( 2, trackpitch,                             "trackpitch"); Param_Info1C(Dat_trackpitch[trackpitch], Dat_trackpitch[trackpitch]);
            Info_S1( 2, copy,                                   "copy"); Param_Info1C(Dat_copy[copy], Dat_copy[copy]);
            Skip_S1( 2,                                         "pack");
            if (fmtid) {
                Trusted_IsNot("dtmainid");
            }
        Element_End0();
        BS_End();
    Element_End0();

    FILLING_BEGIN();
        Element_Info1C(Frame_Count_NotParsedIncluded != (int64u)-1, __T("Frame ") + Ztring::ToZtring(Frame_Count_NotParsedIncluded));
        Element_Info1C((FrameInfo.PTS!=(int64u)-1), __T("PTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PTS)/1000000)));
        if (Frame_Count_NotParsedIncluded != (int64u)-1) {
            Frame_Count_NotParsedIncluded++;
        }

        if (!Status[IsAccepted]) {
            if (!Priv) {
                Priv = new file_dat_priv;
            }
            if (!Frame_Count) {
                Priv->Frame_First = Frame;
            }
            Priv->Items[item_emphasis][emphasis]++;
            Priv->Items[item_sampfreq][sampfreq]++;
            Priv->Items[item_numchans][numchans]++;
            Priv->Items[item_quantization][quantization]++;

            Frame_Count++;
            if (Frame_Count >= Priv->Frame_Count_Valid)
            {
                Accept();
                Fill();
                if (!IsSub && Config->ParseSpeed < 1.0 && File_Offset + Buffer_Offset < File_Size / 2)
                {
                    if (File_Size != (int64u)-1) {
                        Open_Buffer_Unsynch();
                        GoTo((((File_Size) / 5822) - 1) * 5822);
                    }
                    else {
                        Finish();
                    }

                }
            }

        }
        Priv->Frame_Last = Frame;
    FILLING_ELSE();
        if (Priv) {
            if (!Frame_Count) {
                Priv->Frame_First = {};
            }
            Priv->Frame_Last = {};
        }
    FILLING_END();
}

//***************************************************************************
// C++
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dat::dttimepack(TimeCode& TC)
{
    // Parsing
    int16u sample;
    int8u pno1, pno23, index, h, m, s, f;
    bool pro;
    Get_SB (   pro,                                             "pro");
    if (pro) {
        Skip_SB(                                                "fill");
        Get_S1 ( 2, pno1,                                       "sid"); Param_Info1C(Dat_sid[pno1], Dat_sid[pno1]);
        Get_S1 ( 2, pno23,                                      "freq"); Param_Info2C(Dat_sampfreq[pno23], Dat_sampfreq[pno23], " Hz");
        Get_S1 ( 3, index,                                      "xrate"); Param_Info2C(pno23 && Dat_xrate[index], Dat_xrate[index], " fps");
        Get_S2 (11, sample,                                     "sample");
    }
    else {
        Get_S1 (3, pno1,                                        "pno1");
        Get_BCD(   pno23,                                       "pno2", "pno3");
        Get_BCD(   index,                                       "index1", "index2");
    }
    Element_Begin1("dttimecode");
    Get_BCD(   h,                                               "h1", "h2");
    Get_BCD(   m,                                               "m1", "m2");
    Get_BCD(   s,                                               "s1", "s2");
    Get_BCD(   f,                                               "f1", "f2");
    auto FramesMax = TC.GetFramesMax();
    auto FrameMas_Theory = pro ? (index <= 2 ? 29 : (((int32u)Dat_xrate[index]) - 1)) : 33;
    if (f > FrameMas_Theory && FramesMax <= FrameMas_Theory) {
        //Fill_Conformance("dttimepack", "dttimepack is indicated as pro time code but contains frame numbers as if it is non pro time code");
        FramesMax = 33;
    }
    else if (FramesMax < FrameMas_Theory) {
        FramesMax = FrameMas_Theory;
    }
    bool IS1001fps = FramesMax != 33 && pro && (index == 1 || index == 2);
    TC = TimeCode(h, m, s, f, FramesMax, TimeCode::FPS1001(IS1001fps).DropFrame(IS1001fps && index == 2));
    Element_Info1(TC.ToString());
    Element_End0();
    Element_Info1(TC.ToString());
    Element_Level -= 2;
    Element_Info1(TC.ToString());
    Element_Level += 2;
    if ((!pro && (pno1 >= 10 || pno23 == (int8u)-1 || index == (int8u)-1)) || (pro && (!Dat_sampfreq[pno23] || (pno23 && !Dat_xrate[index]) || sample >= 1440)) || !TC.IsValid()) {
        Trusted_IsNot("dtsubcode dttimecode");
    }
}

void File_Dat::Get_BCD(int8u& Value, const char* Name0, const char* Name1)
{
    int8u Value0;
    Get_S1 (4, Value0,                                          Name0);
    Get_S1 (4, Value,                                           Name1);
    int8u ValueHex = (Value0 << 4) | Value;
    if (ValueHex == 0xAA || ValueHex == 0xBB || ValueHex == 0xEE) {
        Value = ValueHex;
    }
    else if (Value0 >= 10 || Value >= 10) {
        Value = (int8u)-1;
    }
    else {
        Value += Value0 * 10;
    }
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_DAT_YES

