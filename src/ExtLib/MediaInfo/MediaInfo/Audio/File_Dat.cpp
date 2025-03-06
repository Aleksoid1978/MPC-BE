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
#include "MediaInfo/MediaInfo_Events_Internal.h"
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
static const int16u Dat_samples[4] =
{
    1440,
    1323,
    960,
    0,
};

//---------------------------------------------------------------------------
static const int8u Dat_samples_mul[4] =
{
    1,
    2,
    0,
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
static const char* Dat_copy[4] =
{
    "Yes",
    nullptr,
    "No",
    "Once",
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
    item_samples,
    item_numchans,
    item_quantization,
    item_trackpitch,
    item_Max,
};

//---------------------------------------------------------------------------
class file_dat_frame {
public:
    TimeCode TCs[7] = {};
    string RecDate[7] = {};
    int8u TC_IDs[7] = {};
    int16u dtsubid_pno = 0;
};
class file_dat_priv {
public:
    file_dat_frame Frame_First;
    file_dat_frame Frame_Last;
    int8u Items[item_Max][4] = {};
    int8u Frame_Count_Valid = 32; // Mini 31 frames for catching wrong pro time codes
    size_t BuggyFrames = 0;
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
        if (!Max_Value) {
            return -1;
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
                auto Value = Array[Index];
                if (Item == item_trackpitch) {
                    // Samples to frame rate
                    Fill(Stream_Audio, 0, Field, 100.0 / 3 / Value);
                    return;
                }
                Fill(Stream_Audio, 0, Field, Value);
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
                auto Value = Array[Index];
                if (Item == item_samples) {
                    // Samples to sampling rate
                    int32u Value2(Value);
                    Value2 *= 100;
                    Value2 /= 3;
                    Value=(int16u)Value2;
                }
                Fill(Stream_Audio, 0, Field, Value);
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
            Fill(Stream_Other, StreamPos_Last, Other_FrameRate, First.GetFramesMax() >= 32 ? (100.0 / 3.0) : First.GetFrameRate());
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
    Conditional_Int16(Audio_SamplingRate, item_samples, Dat_samples);
    Conditional_Int8(Audio_Channel_s_, item_numchans, Dat_numchans);
    Conditional_Int8(Audio_BitDepth, item_quantization, Dat_quantization);
    Conditional_Int8(Audio_FrameRate, item_trackpitch, Dat_samples_mul);
    for (int i = 0; i < 7; i++) {
        Conditional_TimeCode(i);
    }
    for (int i = 0; i < 7; i++) {
        const auto& RecDate = Priv->Frame_First.RecDate[i];
        if (!RecDate.empty()) {
            Fill(Stream_General, 0, General_Recorded_Date, RecDate);
        }
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
    if (Element_Size < 5822)
    {
        if (Frame_Count && !Priv->BuggyFrames && Frame_Count >= Priv->Frame_Count_Valid / 4)
            Accept();

        IsTruncated(File_Offset + Buffer_Offset + 5822, true, "DAT");
        Element_Name("Partial frame");
        Skip_XX(Element_Size, "Data");
        return;
    }

    Skip_XX(5760,                                               "audio");
    Element_Begin1("dtsubcode");
        BS_Begin();
        size_t numpacks_Min = 0;
        auto Frame = Priv ? Priv->Frame_Last : file_dat_frame();
        for (int i = 0; i < 7; i++) {
            auto ParityCheck_Cur = Buffer + (size_t)(Buffer_Offset + Element_Offset + i * 8);
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
            if (Parity) {
                Element_Info1("Parity error");
                Skip_BS(52,                                     "(Skipped)");
            }
            else {
                switch (id) {
                case 0:
                    Skip_BS(52,                                 "padding");
                    break;
                case 1:
                case 2:
                case 3:
                    Frame.TC_IDs[i] = id;
                    dttimepack(Frame.TCs[i]);
                    break;
                case 5:
                    dtdatepack(Frame.RecDate[i]);
                    break;
                default:
                    Skip_BS(52,                                 "(Unknown)");
                }
            }
            Skip_S1( 8,                                         "parity");
            Element_End0();
        }
        Element_End0();
    bool ipf_l, ipf_r;
    Element_Begin1("dtsubid");
        int8u dataid, pno1, numpacks, pno2, pno3, ipf;
        Skip_SB(                                                "ctrlid - Priority ID");
        Skip_SB(                                                "ctrlid - Start ID");
        Skip_SB(                                                "ctrlid - Shortening ID");
        Skip_SB(                                                "ctrlid - TOC ID");
        Get_S1 ( 4, dataid,                                     "dataid");
        Get_S1 ( 4, pno1,                                       "pno (program number) - 1");
        Get_S1 ( 4, numpacks,                                   "numpacks");
        Get_S1 ( 4, pno2,                                       "pno (program number) - 2");
        Get_S1 ( 4, pno3,                                       "pno (program number) - 3");
        Get_S1 ( 8, ipf,                                        "ipf (interpolation flags)");
        Get_Flags (ipf, 6, ipf_l,                               "left ipf");
        Get_Flags (ipf, 7, ipf_r,                               "right ipf");
        int16u pno = ((int16u)pno1 << 8) | (pno2 << 4) | pno3;
        auto pno_IsSpecial_Check = [](int8u pno) {
            return pno == 0xAA || pno == 0xBB || pno == 0xEE;
        };
        bool pno_CurrentIsSpecial = pno_IsSpecial_Check(pno);
        bool pno_IsValid = false;
        if ((pno1 < 8 && pno2 < 10 && pno3 < 10) || pno_CurrentIsSpecial) { // pno is BCD or 0x0AA meaning unknown or 0x0BB meaning lead-in or 0x0EE meaning lead-out
            bool pno_PreviousIsSpecial = pno_IsSpecial_Check(Frame.dtsubid_pno);
            if (pno_CurrentIsSpecial || pno_PreviousIsSpecial || (pno && (pno == Frame.dtsubid_pno || pno == Frame.dtsubid_pno + 1))) {
                pno_IsValid = true;
            }
            Frame.dtsubid_pno = pno;
        }
        if (dataid || numpacks >= 8 || !pno_IsValid) {
            Element_Info1("Problem");
        }
    Element_End0();
    Element_Begin1("dtmainid");
        int8u fmtid, emphasis, sampfreq, numchans, quantization, trackpitch;
        Get_S1 ( 2, fmtid,                                      "fmtid"); Param_Info1C(Dat_fmtid[fmtid], Dat_fmtid[fmtid]);
        Get_S1 ( 2, emphasis,                                   "emphasis"); Param_Info1C(Dat_emphasis[emphasis], Dat_emphasis[emphasis]);
        Get_S1 ( 2, sampfreq,                                   "sampfreq"); Param_Info2C(Dat_samples[sampfreq], Dat_samples[sampfreq], " samples");
        Get_S1 ( 2, numchans,                                   "numchans"); Param_Info2C(Dat_numchans[numchans], Dat_numchans[numchans], " channels");
        Get_S1 ( 2, quantization,                               "quantization"); Param_Info2C(Dat_quantization[quantization], Dat_quantization[quantization], " bits");
        Get_S1 ( 2, trackpitch,                                 "trackpitch"); Param_Info1C(Dat_samples_mul[trackpitch], Dat_samples_mul[trackpitch]);
        Info_S1( 2, copy,                                       "copy"); Param_Info1C(Dat_copy[copy], Dat_copy[copy]);
        Skip_S1( 2,                                             "pack");
        if (fmtid) {
            Element_Info1("Problem");
        }
    Element_End0();
    BS_End();

    if (!Priv) {
        Priv = new file_dat_priv;
    }
    if (dataid || numpacks >= 8 || !pno_IsValid || fmtid) {
        Priv->BuggyFrames++;
    }
    if (!Frame_Count) {
        Priv->Frame_First = Frame;
    }
    FILLING_BEGIN();
        Element_Info1C(Frame_Count_NotParsedIncluded != (int64u)-1, __T("Frame ") + Ztring::ToZtring(Frame_Count_NotParsedIncluded));
        Element_Info1C((FrameInfo.PTS!=(int64u)-1), __T("PTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PTS)/1000000)));
        if (Frame_Count_NotParsedIncluded != (int64u)-1) {
            Frame_Count_NotParsedIncluded++;
        }

        Priv->Items[item_emphasis][emphasis]++;
        Priv->Items[item_samples][sampfreq]++;
        Priv->Items[item_numchans][numchans]++;
        Priv->Items[item_quantization][quantization]++;
        Priv->Items[item_trackpitch][trackpitch]++;

        Frame_Count++;
        if (!Status[IsAccepted]) {
            if (Frame_Count - Priv->BuggyFrames >= Priv->Frame_Count_Valid) {
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
            else if (Priv->BuggyFrames >= Priv->Frame_Count_Valid) {
                Reject();
            }
        }

        int8u MoreData[0x100];
        #if MEDIAINFO_EVENTS
            EVENT_BEGIN(DvDif, Change, 0)
                Event.StreamOffset = File_Size + Buffer_Offset;
                Event.FrameNumber = Frame_Count_NotParsedIncluded - 1;
                Event.PTS = 30000000 * Event.FrameNumber;
                Event.DUR = 30000000;
                Event.VideoChromaSubsampling = (int32u)-1;
                if (Dat_samples[sampfreq])
                {
                    Event.AudioRate_N = Dat_samples[sampfreq] * 100 / 3;
                    Event.AudioRate_D = 1;
                }
                if (Dat_numchans[numchans])
                    Event.AudioChannels = Dat_numchans[numchans];
                if (Dat_numchans[quantization])
                    Event.AudioBitDepth = Dat_quantization[quantization];
                size_t MoreData_Offset = sizeof(size_t);
                MoreData[MoreData_Offset++] = 1;
                MoreData[MoreData_Offset++] = MediaInfo_Event_Change_MoreData_Emphasis;
                MoreData[MoreData_Offset++] = emphasis;
                if (Frame.dtsubid_pno != (int16u)-1)
                {
                    MoreData[MoreData_Offset++] = 2;
                    MoreData[MoreData_Offset++] = MediaInfo_Event_Change_MoreData_ProgramNumber;
                    *((uint16_t*)(MoreData + MoreData_Offset)) = Frame.dtsubid_pno;
                    MoreData_Offset += 2;
                }
                if (MoreData_Offset > sizeof(size_t))
                {
                    *((size_t*)MoreData) = MoreData_Offset - sizeof(size_t);
                    Event.MoreData = MoreData;
                }
            EVENT_END()

            EVENT_BEGIN(DvDif, Analysis_Frame, 1)
                Event.StreamOffset = File_Size + Buffer_Offset;
                Event.FrameNumber = Frame_Count_NotParsedIncluded - 1;
                Event.PTS = 30000000 * Event.FrameNumber;
                Event.DUR = 30000000;
                Event.TimeCode |= 0x7FFFF << 8;
                Event.TimeCode |= 0x3F;
                Event.RecordedDateTime1 |= 0x1FFFF;
                Event.RecordedDateTime1 |= 0x7F << 17;
                Event.RecordedDateTime2 |= 0x0F << 12;
                Event.RecordedDateTime2 |= 0x1F << 8;
                Event.RecordedDateTime2 |= 0x7F;
                Event.RecordedDateTime2 |= 0x0F << 12;
                Event.RecordedDateTime2 |= 0x1F << 6;
                if (ipf_l || ipf_r) {
                    if (ipf_l && ipf_r) {
                        Event.Coherency_Flags |= 1 << 4;
                    }
                    if (ipf_l) {
                        Event.MoreFlags |= 1 << 4;
                    }
                    if (ipf_r) {
                        Event.MoreFlags |= 1 << 5;
                    }
                }
                size_t MoreData_Offset = sizeof(size_t);
                for (int i = 0; i < 7; i++) {
                    switch (Frame.TC_IDs[i]) {
                    case 1:
                    case 2:
                    case 3:
                    {
                        TimeCode Previous;
                        TimeCode Ref;
                        if (Frame_Count_NotParsedIncluded > 1)
                        {
                            Previous = Priv->Frame_Last.TCs[i];
                            Previous.SetFramesMax(Frame.TCs[i].GetFramesMax());
                            Ref = Previous;
                            ++Ref;
                        }
                        int32u Value = 0;
                        Value |= (int32u)Frame.TCs[i].ToSeconds() << 8;
                        Value |= Frame.TCs[i].IsDropFrame() << 7;
                        Value |= Frame.TCs[i].GetFrames();
                        if (Previous.IsValid())
                        {
                            if (Frame.TCs[i] == Previous)
                                Value |= 1 << 31;
                            else if (Frame.TCs[i] != Ref)
                                Value |= 1 << 30;
                        }
                        MoreData[MoreData_Offset++] = 9;
                        MoreData[MoreData_Offset++] = MediaInfo_Event_Analysis_Frame_TimeCode;
                        *((int32u*)(MoreData + MoreData_Offset)) = Value;
                        MoreData_Offset += 4;
                        *((int32u*)(MoreData + MoreData_Offset)) = 0;
                        MoreData_Offset += 4;
                        MoreData[MoreData_Offset++] = Frame.TC_IDs[i] - 1;
                        break;
                    }
                    default:;
                    }
                }
                if (MoreData_Offset > sizeof(size_t))
                {
                    *((size_t*)MoreData) = MoreData_Offset - sizeof(size_t);
                    Event.MoreData = MoreData;
                }
            EVENT_END()
        #endif //MEDIAINFO_EVENTS

        Demux(Buffer + Buffer_Offset, 5292, ContentType_MainStream);


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
// Helpers
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
        Get_S1 ( 2, pno23,                                      "freq"); Param_Info2C(Dat_samples[pno23], Dat_samples[pno23], " samples");
        Get_S1 ( 3, index,                                      "xrate"); Param_Info2C(pno23 && Dat_xrate[index], Dat_xrate[index], " fps");
        Get_S2 (11, sample,                                     "sample");
    }
    else {
        Get_S1 (3, pno1,                                        "pno (program number) - 1");
        Get_BCD(   pno23,                                       "pno (program number) - 2", "pno (program number) - 3");
        Get_BCD(   index,                                       "index1", "index2");
    }
    Element_Begin1("dttimecode");
    Get_BCD(   h,                                               "h1", "h2");
    Get_BCD(   m,                                               "m1", "m2");
    Get_BCD(   s,                                               "s1", "s2");
    Get_BCD(   f,                                               "f1", "f2");
    if (pro || (h != 0xAA && h != 0xBB)) {
        auto FramesMax = TC.GetFramesMax();
        auto FrameMax_Theory = pro ? (index <= 2 ? 29 : (((int32u)Dat_xrate[index]) - 1)) : (33 + ((s % 3) == 2));
        if (f > FrameMax_Theory && FramesMax <= FrameMax_Theory) {
            Fill_Conformance("dttimepack", "dttimepack is indicated as pro time code but contains frame numbers as if it is non pro time code");
            FramesMax = 33 + ((s % 3) == 2);
        }
        else if (FramesMax < FrameMax_Theory) {
            FramesMax = FrameMax_Theory;
        }
        bool IS1001fps = FramesMax != 32 && pro && (index == 1 || index == 2);
        TC = TimeCode(h, m, s, f, FramesMax, TimeCode::FPS1001(IS1001fps).DropFrame(IS1001fps && index == 2));
        Element_Info1(TC.ToString());
        Element_Level--;
        Element_Info1(TC.ToString());
        Element_Level--;
        Element_Info1(TC.ToString());
        Element_Level--;
        Element_Info1(TC.ToString());
        Element_Level += 3;
        if ((!pro && (pno1 >= 10 || pno23 == (int8u)-1 || index == (int8u)-1)) || (pro && (!Dat_samples[pno23] || (pno23 && !Dat_xrate[index]) || sample >= 1440)) || !TC.IsValid()) {
            Element_Info1("Problem");
        }
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Dat::dtdatepack(string& Date)
{
    // Parsing
    int8u year2, month, day, h, m, s;
    Element_Begin1("dtdatepack");
    Skip_S1(4,                                                  "dayow");
    Get_BCD(year2, "year1", "year2");
    Get_BCD(month, "month1", "month2");
    Get_BCD(day, "day1", "day2");
    Get_BCD(h, "h1", "h2");
    Get_BCD(m, "m1", "m2");
    Get_BCD(s, "s1", "s2");
    Date.clear();
    int16u year=(int16u)year2+(year2<70?2000:1900);
    Date+='0'+((year     )/1000);
    Date+='0'+((year%1000)/100 );
    Date+='0'+((year% 100)/10  );
    Date+='0'+((year%  10)     );
    Date+='-';
    Date+='0'+((month   )/10);
    Date+='0'+((month%10)   );
    Date+='-';
    Date+='0'+((day   )/10);
    Date+='0'+((day%10)   );
    Date+=' ';
    Date+='0'+((h   )/10);
    Date+='0'+((h%10)   );
    Date+=':';
    Date+='0'+((m   )/10);
    Date+='0'+((m%10)   );
    Date+=':';
    Date+='0'+((s   )/10);
    Date+='0'+((s%10)   );
    Element_Info1(Date);
    Element_End0();
    Element_Info1(Date);
    Element_Level -= 2;
    Element_Info1(Date);
    Element_Level += 2;
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

