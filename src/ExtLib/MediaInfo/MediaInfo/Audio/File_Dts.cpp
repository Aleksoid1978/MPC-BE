/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

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
#if defined(MEDIAINFO_DTS_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Dts.h"
#include "ZenLib/Utils.h"
#include "ZenLib/BitStream.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#include <algorithm>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const int16u CRC_CCIT_Table[256]=
{
    0x0000, 0x2110, 0x4220, 0x6330, 0x8440, 0xA550, 0xC660, 0xE770,
    0x0881, 0x2991, 0x4AA1, 0x6BB1, 0x8CC1, 0xADD1, 0xCEE1, 0xEFF1,
    0x3112, 0x1002, 0x7332, 0x5222, 0xB552, 0x9442, 0xF772, 0xD662,
    0x3993, 0x1883, 0x7BB3, 0x5AA3, 0xBDD3, 0x9CC3, 0xFFF3, 0xDEE3,
    0x6224, 0x4334, 0x2004, 0x0114, 0xE664, 0xC774, 0xA444, 0x8554,
    0x6AA5, 0x4BB5, 0x2885, 0x0995, 0xEEE5, 0xCFF5, 0xACC5, 0x8DD5,
    0x5336, 0x7226, 0x1116, 0x3006, 0xD776, 0xF666, 0x9556, 0xB446,
    0x5BB7, 0x7AA7, 0x1997, 0x3887, 0xDFF7, 0xFEE7, 0x9DD7, 0xBCC7,
    0xC448, 0xE558, 0x8668, 0xA778, 0x4008, 0x6118, 0x0228, 0x2338,
    0xCCC9, 0xEDD9, 0x8EE9, 0xAFF9, 0x4889, 0x6999, 0x0AA9, 0x2BB9,
    0xF55A, 0xD44A, 0xB77A, 0x966A, 0x711A, 0x500A, 0x333A, 0x122A,
    0xFDDB, 0xDCCB, 0xBFFB, 0x9EEB, 0x799B, 0x588B, 0x3BBB, 0x1AAB,
    0xA66C, 0x877C, 0xE44C, 0xC55C, 0x222C, 0x033C, 0x600C, 0x411C,
    0xAEED, 0x8FFD, 0xECCD, 0xCDDD, 0x2AAD, 0x0BBD, 0x688D, 0x499D,
    0x977E, 0xB66E, 0xD55E, 0xF44E, 0x133E, 0x322E, 0x511E, 0x700E,
    0x9FFF, 0xBEEF, 0xDDDF, 0xFCCF, 0x1BBF, 0x3AAF, 0x599F, 0x788F,
    0x8891, 0xA981, 0xCAB1, 0xEBA1, 0x0CD1, 0x2DC1, 0x4EF1, 0x6FE1,
    0x8010, 0xA100, 0xC230, 0xE320, 0x0450, 0x2540, 0x4670, 0x6760,
    0xB983, 0x9893, 0xFBA3, 0xDAB3, 0x3DC3, 0x1CD3, 0x7FE3, 0x5EF3,
    0xB102, 0x9012, 0xF322, 0xD232, 0x3542, 0x1452, 0x7762, 0x5672,
    0xEAB5, 0xCBA5, 0xA895, 0x8985, 0x6EF5, 0x4FE5, 0x2CD5, 0x0DC5,
    0xE234, 0xC324, 0xA014, 0x8104, 0x6674, 0x4764, 0x2454, 0x0544,
    0xDBA7, 0xFAB7, 0x9987, 0xB897, 0x5FE7, 0x7EF7, 0x1DC7, 0x3CD7,
    0xD326, 0xF236, 0x9106, 0xB016, 0x5766, 0x7676, 0x1546, 0x3456,
    0x4CD9, 0x6DC9, 0x0EF9, 0x2FE9, 0xC899, 0xE989, 0x8AB9, 0xABA9,
    0x4458, 0x6548, 0x0678, 0x2768, 0xC018, 0xE108, 0x8238, 0xA328,
    0x7DCB, 0x5CDB, 0x3FEB, 0x1EFB, 0xF98B, 0xD89B, 0xBBAB, 0x9ABB,
    0x754A, 0x545A, 0x376A, 0x167A, 0xF10A, 0xD01A, 0xB32A, 0x923A,
    0x2EFD, 0x0FED, 0x6CDD, 0x4DCD, 0xAABD, 0x8BAD, 0xE89D, 0xC98D,
    0x267C, 0x076C, 0x645C, 0x454C, 0xA23C, 0x832C, 0xE01C, 0xC10C,
    0x1FEF, 0x3EFF, 0x5DCF, 0x7CDF, 0x9BAF, 0xBABF, 0xD98F, 0xF89F,
    0x176E, 0x367E, 0x554E, 0x745E, 0x932E, 0xB23E, 0xD10E, 0xF01E,
};
uint16_t Dts_CRC_CCIT_Compute(const uint8_t* Buffer, size_t Size)
{
    uint16_t C = 0xFFFF; 
    const uint8_t *End = Buffer + Size;

    while (Buffer < End)
        C = (C >> 8) ^ CRC_CCIT_Table[((uint8_t)C) ^ *Buffer++];

    return C;
}

//---------------------------------------------------------------------------
static const char*  DTS_FrameType[]=
{
    "Termination",
    "Normal",
};

//---------------------------------------------------------------------------
static const int32u DTS_SamplingRate[]=
{
        0,  8000,  16000,  32000,      0,      0,  11025,  22050,
    44100,     0,      0,  12000,  24000,  48000,  96000, 192000,
};

//---------------------------------------------------------------------------
static const int32u DTS_BitRate[]=
{
      32000,   56000,   64000,   96000,  112000,  128000,  192000,  224000,
     256000,  320000,  384000,  448000,  512000,  576000,  640000,  754500,
     960000, 1024000, 1152000, 1280000, 1344000, 1408000, 1411200, 1472000,
    1509750, 1920000, 2048000, 3072000, 3840000,       0,       0,       0,
};

//---------------------------------------------------------------------------
static const int8u DTS_Channels[]=
{
    1, 2, 2, 2, 2, 3, 3, 4,
    4, 5, 6, 6, 6, 7, 8, 8,
};

//---------------------------------------------------------------------------
static const int8u DTS_Resolution[]=
{16, 20, 24, 24};

//---------------------------------------------------------------------------
const char*  DTS_ChannelPositions[16]=
{
    "Front: C",
    "Front: C C",
    "Front: L R",
    "Front: L R",
    "Front: L R",
    "Front: L C R",
    "Front: L R, Back: C",
    "Front: L C R, Back: C",
    "Front: L R, Side: L R",
    "Front: L C R, Side: L R",
    "Front: L C C R, Side: L R",
    "Front: L C R, Side: L R",
    "Front: L R, Side: L R, Back: L R",
    "Front: L C R, Side: L R, Back: L R",
    "Front: L R, Side: L R, Back: L C C R",
    "Front: L C R, Side: L R, Back: L C R",
};

//---------------------------------------------------------------------------
const char*  DTS_ChannelPositions2[16]=
{
    "1/0/0",
    "2/0/0",
    "2/0/0",
    "2/0/0",
    "2/0/0",
    "3/0/0",
    "2/1/0",
    "3/1/0",
    "2/2/0",
    "3/2/0",
    "4/2/0",
    "3/2/0",
    "2/2/2",
    "3/2/2",
    "2/2/4",
    "3/2/3",
};

//---------------------------------------------------------------------------
static const char*  DTS_ChannelLayout[16]=
{
    "M",
    "M M",
    "L R",
    "L R",
    "Lt Rt",
    "C L R",
    "L R Cs",
    "C L R Cs",
    "L R Ls Rs",
    "C L R Ls Rs",
    "Cl Cr L R Ls Rs",
    "C L R Ls Rs",
    "C L R Ls Rs Lrs Rrs",
    "C L R Ls Rs Lrs Rrs",
    "L R Ls Rs Rls Cs Cs Rrs",
    "C L R Ls Rs Rls Cs Rrs",
};

//---------------------------------------------------------------------------
static const char* DTS_ExtensionAudioDescriptor[]=
{
    "Channel Extension",
    "Reserved",
    "Frequency Extension",
    "Channel Extension + Frequency Extension",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
static const char* DTS_HD_RefClockCode[]=
{
    "1/32000",
    "1/44100",
    "1/48000",
    "",
};

//---------------------------------------------------------------------------
std::string DTS_HD_SpeakerActivityMask (int16u SpeakerActivityMask, bool AddCs=false, bool AddLrsRrs=false)
{
    std::string Text;
    if ((SpeakerActivityMask&0x0003)==0x0003)
        Text+="Front: L C R";
    else
    {
        if (SpeakerActivityMask&0x0001)
            Text+="Front: C";
        if (SpeakerActivityMask&0x0002)
            Text+="Front: L R";
    }

    if (SpeakerActivityMask&0x0004)
        Text+=", Side: L R";

    if (SpeakerActivityMask&0x0010 || AddCs)
        Text+=", Back: C";

    if ((SpeakerActivityMask&0x00A0)==0x00A0)
        Text+=", High: L C R";
    else
    {
        if (SpeakerActivityMask&0x0020 || AddLrsRrs)
            Text+=", High: L R";
        if (SpeakerActivityMask&0x0080)
            Text+=", High: C";
    }

    if (SpeakerActivityMask&0x0800)
        Text+=", Side: L R";

    if (SpeakerActivityMask&0x0040)
        Text+=", Back: L R";

    if (SpeakerActivityMask&0x0100)
        Text+=", TopCtrSrrd";
    if (SpeakerActivityMask&0x0200)
        Text+=", Ctr: L R";
    if (SpeakerActivityMask&0x0400)
        Text+=", Wide: L R";
    if (SpeakerActivityMask&0x2000)
        Text+=", HiSide: L R";

    if ((SpeakerActivityMask&0xC000)==0x0C000)
        Text+=", HiRear: L C R";
    else
    {
        if (SpeakerActivityMask&0x4000)
            Text+=", HiRear: C";
        if (SpeakerActivityMask&0x8000)
            Text+=", HiRear: L R";
    }

    if (SpeakerActivityMask&0x0008)
        Text+=", LFE";
    if (SpeakerActivityMask&0x1000)
        Text+=", LFE2";

    return Text;
}

//---------------------------------------------------------------------------
std::string DTS_HD_SpeakerActivityMask2 (int16u SpeakerActivityMask, bool AddCs=false, bool AddLrsRrs=false)
{
    std::string Text;
    if ((SpeakerActivityMask&0x0003)==0x0003)
        Text+="3";
    else
    {
        if (SpeakerActivityMask&0x0001)
            Text+="1";
        else if (SpeakerActivityMask&0x0002)
            Text+="2";
        else
            Text+="0";
    }

    if (SpeakerActivityMask&0x0004)
        Text+="/2";
    else if ((SpeakerActivityMask&0x0840)==0x0000)
        Text+="/0";

    if (SpeakerActivityMask&0x0010 || AddCs)
        Text+="/1";
    else if ((SpeakerActivityMask&0x0840)==0x0000)
        Text+="/0";

    if ((SpeakerActivityMask&0x00A0)==0x00A0)
        Text+=".3";
    else
    {
        if (SpeakerActivityMask&0x0020 || AddLrsRrs)
            Text+=".2";
        if (SpeakerActivityMask&0x0080)
            Text+=".2";
    }

    if (SpeakerActivityMask&0x0800)
        Text+="/2";
    if (SpeakerActivityMask&0x0040)
        Text+="/2";

    if (SpeakerActivityMask&0x0100)
        Text+=".1";
    if (SpeakerActivityMask&0x0200)
        Text+=".2";
    if (SpeakerActivityMask&0x0400)
        Text+=".2";
    if (SpeakerActivityMask&0x2000)
        Text+=".2";

    if ((SpeakerActivityMask&0xC000)==0x0C000)
        Text+=".3";
    else
    {
        if (SpeakerActivityMask&0x4000)
            Text+=".1";
        if (SpeakerActivityMask&0x8000)
            Text+=".2";
    }

    if (SpeakerActivityMask&0x0008)
        Text+=".1";
    if (SpeakerActivityMask&0x1000)
        Text+=".1";

    return Text;
}

//---------------------------------------------------------------------------
std::string DTS_HD_SpeakerActivityMask_ChannelLayout (int16u SpeakerActivityMask, bool AddCs=false, bool AddLrsRrs=false)
{
    std::string Text;
    if (SpeakerActivityMask==1)
        return "M";
    if (SpeakerActivityMask&0x0001)
        Text+=" C";
    if (SpeakerActivityMask&0x0002)
        Text+=" L R";
    if (SpeakerActivityMask&0x0004)
        Text+=" Ls Rs";
    if (SpeakerActivityMask&0x0008)
        Text+=" LFE";
    if (SpeakerActivityMask&0x0010 || AddCs)
        Text+=" Cs";
    if (SpeakerActivityMask&0x0020)
        Text+=" Lh Rh";
    if (SpeakerActivityMask&0x0040 || AddLrsRrs)
        Text+=" Lsr Rsr";
    if (SpeakerActivityMask&0x0080)
        Text+=" Ch";
    if (SpeakerActivityMask&0x0100)
        Text+=" Oh";
    if (SpeakerActivityMask&0x0200)
        Text+=" Lc Rc";
    if (SpeakerActivityMask&0x0400)
        Text+=" Lw Rw";
    if (SpeakerActivityMask&0x0800)
        Text+=" Lss Rss";
    if (SpeakerActivityMask&0x1000)
        Text+=" LFE2";
    if (SpeakerActivityMask&0x2000)
        Text+=" Lhs Rhs";
    if (SpeakerActivityMask&0x4000)
        Text+=" Chr";
    if (SpeakerActivityMask&0x8000)
        Text+=" Lhr";

    if (!Text.empty())
        Text.erase(0, 1);
    return Text;
}

//---------------------------------------------------------------------------
static const char* DTS_HD_TypeDescriptor[]=
{
    "Music",
    "Effects",
    "Dialog",
    "Commentary",
    "Visually Impaired",
    "Hearing  Impaired",
    "Isolated Music Object",
    "Music and Effects",
    "Dialog and Commentary",
    "Effects and Commentary",
    "Isolated Music Object and Commentary",
    "Isolated Music Object and Effects",
    "Karaoke",
    "Music, Effects, Dialog",
    "Complete Audio Presentation",
    "",
};

//---------------------------------------------------------------------------
static int32u DTS_HD_MaximumSampleRate[]=
{
      8000,
     16000,
     32000,
     64000,
    128000,
     22050,
     44100,
     88200,
    176400,
    352800,
     12000,
     24000,
     48000,
     96000,
    192000,
    384000,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dts::File_Dts()
:File__Analyze()
{
    //Configuration
    ParserName="Dts";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Dts;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    Buffer_TotalBytes_FirstSynched_Max=32*1024;
    PTS_DTS_Needed=true;
    StreamSource=IsStream;

    //In
    Frame_Count_Valid=0;

    //Buffer
    Save_Buffer=NULL;

    //Temp
    HD_size=0;
    Primary_Frame_Byte_Size=0;
    HD_SpeakerActivityMask=(int16u)-1;
    channel_arrangement=(int8u)-1;
    channel_arrangement_XCh=(int8u)-1;
    sample_frequency=(int8u)-1;
    sample_frequency_X96k=(int8u)-1;
    bit_rate=(int8u)-1;
    lfe_effects=(int8u)-1;
    bits_per_sample=(int8u)-1;
    ExtensionAudioDescriptor=(int8u)-1;
    HD_BitResolution=(int8u)-1;
    HD_BitResolution_Real=(int8u)-1;
    HD_MaximumSampleRate=(int8u)-1;
    HD_MaximumSampleRate_Real=(int8u)-1;
    HD_TotalNumberChannels=(int8u)-1;
    HD_ExSSFrameDurationCode=(int8u)-1;
    ExtendedCoding=false;
    ES=false;
    Core_Exists=false;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dts::Streams_Fill_Extension()
{
    bool AddCs=false;
    //bool AddLrsRrs=false;
    //bool AddX=false;

    if (HD_TotalNumberChannels!=(int8u)-1)
    {
        int8u i=HD_TotalNumberChannels;
        int8u Core_Core_Channels=DTS_Channels[Core_Core_AMODE];
        if (Presence[presence_Core_Core] && Core_Core_LFF)
            Core_Core_Channels++;

        if (Presence[presence_Core_XXCh] && i<Core_Core_Channels+Core_XXCh_nuNumChSetsInXXCh)
        {
            //switch (Core_XXCh_nuNumChSetsInXXCh)
            //{
            //    case 1: AddCs=true; break;
            //    case 2: AddLrsRrs=true; break;
            //    default: AddX=true;
            //}

            //i=Core_Core_Channels+Core_XXCh_nuNumChSetsInXXCh;
        }
        if (Presence[presence_Core_XCh] && i<Core_Core_Channels+Core_XCh_AMODE)
        {
            //AddCs=true;

            //i=Core_Core_Channels+Core_XCh_AMODE;
        }
        if (!Presence[presence_Core_XCh] && ES && i<Core_Core_Channels+1)
        {
            AddCs=true;

            i=Core_Core_Channels+1; // Must count the Matrixed additional channels
        }

        Data[Channels].push_back(Ztring::ToZtring(i));
    }
    else
    {
        Data[Channels].push_back(Ztring());
    }
    if (HD_SpeakerActivityMask!=(int16u)-1) // && !AddX)
    {
        Data[ChannelPositions].push_back(DTS_HD_SpeakerActivityMask(HD_SpeakerActivityMask, AddCs).c_str()); //AddLrsRrs
        Data[ChannelPositions2].push_back(DTS_HD_SpeakerActivityMask2(HD_SpeakerActivityMask, AddCs).c_str()); //AddLrsRrs
        Data[ChannelLayout].push_back(DTS_HD_SpeakerActivityMask_ChannelLayout(HD_SpeakerActivityMask, AddCs).c_str()); //AddLrsRrs
    }
    else
    {
        Data[ChannelPositions].push_back(Ztring());
        Data[ChannelPositions2].push_back(Ztring());
        Data[ChannelLayout].push_back(Ztring());
    }
    if (HD_BitResolution_Real!=(int8u)-1)
    {
        Data[BitDepth].push_back(Ztring::ToZtring(HD_BitResolution_Real));
    }
    else if (HD_BitResolution!=(int8u)-1)
    {
        Data[BitDepth].push_back(Ztring::ToZtring(HD_BitResolution));
    }
    else
    {
        Data[BitDepth].push_back(Ztring());
    }
    if (HD_MaximumSampleRate_Real!=(int8u)-1)
    {
        Data[SamplingRate].push_back(Ztring::ToZtring(DTS_HD_MaximumSampleRate[HD_MaximumSampleRate_Real]));
    }
    else if (HD_MaximumSampleRate!=(int8u)-1)
    {
        Data[SamplingRate].push_back(Ztring::ToZtring(DTS_HD_MaximumSampleRate[HD_MaximumSampleRate]));
    }
    else
    {
        Data[SamplingRate].push_back(Ztring());
    }
    Data[BitRate].push_back(Ztring::ToZtring(BitRate_Get(true), 0));
    Data[BitRate_Mode].push_back(__T("CBR"));
    Data[Compression_Mode].push_back(__T("Lossy"));
}

//---------------------------------------------------------------------------
void File_Dts::Streams_Fill_Core_ES()
{
    Streams_Fill_Core();

    if (!ES && !Presence[presence_Core_XCh])
        return;

    int8u Channels_Before=Data[Channels][Data[Channels].size()-1].To_int8u();
    Data[Channels].pop_back();
    Data[ChannelPositions].pop_back();
    Data[ChannelPositions2].pop_back();
    Data[ChannelLayout].pop_back();

    Data[Channels].push_back(Ztring::ToZtring(Channels_Before+(Presence[presence_Core_XCh]?Core_XCh_AMODE:1)));
    if (Core_Core_AMODE == 9 && (!Presence[presence_Core_XCh] || Core_XCh_AMODE == 1))
    {
        Data[ChannelPositions].push_back(Ztring(__T("Front: L C R, Side: L R, Back: C"))+(Core_Core_LFF?__T(", LFE"):__T("")));
        Data[ChannelPositions2].push_back(Ztring(__T("3/2/1"))+(Core_Core_LFF?__T(".1"):__T("")));
        Data[ChannelLayout].push_back(Ztring(__T("C L R Ls Rs Cs"))+(Core_Core_LFF?__T(" LFE"):__T("")));
    }
    else if (Core_Core_AMODE == 9 && (Presence[presence_Core_XCh] && Core_XCh_AMODE == 2))
    {
        Data[ChannelPositions].push_back(Ztring(__T("Front: L C R, Side: L R, Back: L R"))+(Core_Core_LFF?__T(", LFE"):__T("")));
        Data[ChannelPositions2].push_back(Ztring(__T("3/2/2"))+(Core_Core_LFF?__T(".1"):__T("")));
        Data[ChannelLayout].push_back(Ztring(__T("C L R Ls Rs Lrs Rrs"))+(Core_Core_LFF?__T(" LFE"):__T("")));
    }
    else
    {
        Ztring Debug=__T("Debug, Core_Core_AMODE=")+Ztring::ToZtring(Core_Core_AMODE);
        if(Presence[presence_Core_XCh])
           Debug +=__T(", Core_XCh_AMODE=")+Ztring::ToZtring(Core_XCh_AMODE);
        Debug +=__T(", Core_Core_LFF=")+Ztring::ToZtring(Core_Core_LFF);
        Data[ChannelPositions].push_back(Debug);
        Data[ChannelPositions2].push_back(Debug);
        Data[ChannelLayout].push_back(Debug);
    }
}

//---------------------------------------------------------------------------
void File_Dts::Streams_Fill_Core()
{
    Ztring Core_BitRate;
    if (bit_rate<25)
        Core_BitRate=Ztring::ToZtring(BitRate_Get(), 0);
    else if (bit_rate==29)
        Core_BitRate=__T("Open");
    else
        Core_BitRate=__T("Unknown");

    if (Core_Core_AMODE<16)
    {
        Data[Channels].push_back(Ztring::ToZtring(DTS_Channels[Core_Core_AMODE]+(Core_Core_LFF?1:0)));
        Data[ChannelPositions].push_back(Ztring().From_UTF8(DTS_ChannelPositions[Core_Core_AMODE])+(Core_Core_LFF?__T(", LFE"):__T("")));
        Data[ChannelPositions2].push_back(Ztring().From_UTF8(DTS_ChannelPositions2[Core_Core_AMODE])+(Core_Core_LFF?__T(".1"):__T(".0")));
        Data[ChannelLayout].push_back(Ztring().From_UTF8(DTS_ChannelLayout[Core_Core_AMODE])+(Core_Core_LFF?__T(" LFE"):__T("")));
    }
    else
    {
        Data[Channels].push_back(__T("User Defined"));
        Data[ChannelPositions].push_back(Ztring(__T("User Defined"))+(Core_Core_LFF?__T(", LFE"):__T("")));
        Data[ChannelPositions2].push_back(Ztring(__T("User Defined"))+(Core_Core_LFF?__T(".1"):__T(".0")));
        Data[ChannelLayout].push_back(Ztring(__T("User Defined"))+(Core_Core_LFF?__T(" LFE"):__T("")));
    }
    Data[BitDepth].push_back(Ztring::ToZtring(DTS_Resolution[bits_per_sample]));
    Data[SamplingRate].push_back(Ztring::ToZtring(DTS_SamplingRate[sample_frequency]));
    Data[BitRate].push_back(Core_BitRate);
    Data[BitRate_Mode].push_back(__T("CBR"));
    Data[Compression_Mode].push_back(__T("Lossy"));
}

//---------------------------------------------------------------------------
void File_Dts::Streams_Fill()
{
    Fill(Stream_General, 0, General_Format, "DTS");

    Stream_Prepare(Stream_Audio);
    Fill(Stream_Audio, 0, Audio_Format, "DTS");

    // IMAX DTS:X
    if (Extension_XLL_IMAXX_Yes>Extension_XLL_IMAXX_No*4)
    {
        Presence.set(presence_Extended_XLL_IMAX);
        Presence.set(presence_Extended_XLL_X);
    }
    if (Presence[presence_Extended_XLL_IMAX])
    {
        Data[Profiles].push_back(__T("IMAX"));

        Streams_Fill_Extension();
        Data[BitRate].back().clear();
        Data[BitRate_Mode].back()=__T("VBR");
        Data[Compression_Mode].back().clear();
    }

    // DTS:X
    if (Extension_XLL_X_Yes>Extension_XLL_X_No*4)
        Presence.set(presence_Extended_XLL_X);
    if (Presence[presence_Extended_XLL_X])
    {
        Data[Profiles].push_back(__T("X"));

        Streams_Fill_Extension();
        Data[BitRate].back().clear();
        Data[BitRate_Mode].back()=__T("VBR");
        Data[Compression_Mode].back().clear();
    }

    // DTS-HD MA
    if (Presence[presence_Extended_XLL])
    {
        Data[Profiles].push_back(__T("MA"));

        Streams_Fill_Extension();
        Data[BitRate].pop_back();
        Data[BitRate_Mode].pop_back();
        Data[Compression_Mode].pop_back();

        Data[BitRate].push_back(__T("Unknown"));
        Data[BitRate_Mode].push_back(__T("VBR"));
        Data[Compression_Mode].push_back(__T("Lossless"));
    }

    // DTS Express
    if (Presence[presence_Extended_LBR])
    {
        Data[Profiles].push_back(__T("Express"));

        Streams_Fill_Extension();
    }

    // DTS-HD HRA
    if (Presence[presence_Extended_XBR]
     || Presence[presence_Extended_X96]
     || Presence[presence_Extended_XXCh]
     || Presence[presence_Core_XXCh])
    {
        Data[Profiles].push_back(__T("HRA"));

        if (Presence[presence_Extended_XBR]
         || Presence[presence_Extended_X96]
         || Presence[presence_Extended_XXCh])
            Streams_Fill_Extension();
        else
        {
            Streams_Fill_Core_ES();

            if (Presence[presence_Extended_X96])
            {
                Data[SamplingRate].pop_back();

                Data[SamplingRate].push_back(Ztring::ToZtring(96000));
            }
        }
    }

    // DTS-HD 96/24
    else
    if (Presence[presence_Core_X96])
    {
        Data[Profiles].push_back(__T("96/24"));

        if (Presence[presence_Extended_X96])
            Streams_Fill_Extension();
        else
        {
            Streams_Fill_Core_ES();
            Data[SamplingRate].pop_back();

            Data[SamplingRate].push_back(Ztring::ToZtring(96000));
        }
    }

    // DTS-ES
    if (ES || Presence[presence_Core_XCh])
    {
        if (!ES)
            Data[Profiles].push_back(__T("ES Discrete without ES Matrix"));
        else if (Presence[presence_Core_XCh])
            Data[Profiles].push_back(__T("ES Discrete"));
        else
            Data[Profiles].push_back(__T("ES Matrix"));
        Streams_Fill_Core_ES();
    }

    // Core
    if (Presence[presence_Core_Core])
    {
        Data[Profiles].push_back(__T("Core"));
        Streams_Fill_Core();
    }

    // Cleanup up
    for (size_t Pos=0; Pos<10; ++Pos)
    {
        Data[Pos].Separator_Set(0, __T(" / "));
        if (Data[Pos].size()>1)
        {
            bool IsNotSame=false;
            for (size_t i=1; i<Data[Pos].size(); ++i)
                if (Data[Pos][i]!=Data[Pos][0])
                    IsNotSame=true;
            if (!IsNotSame)
                Data[Pos].resize(1);
        }
    }
    if (Data[Profiles].size()==1 && Data[Profiles][0]==__T("Core"))
        Data[Profiles].clear(); //Core is the default one

    // Filling
    bool LegacyStreamDisplay=MediaInfoLib::Config.LegacyStreamDisplay_Get();
    Fill(Stream_Audio, 0, Audio_Format_Profile, Data[Profiles].Read());
    Fill(Stream_Audio, 0, Audio_Codec, (Data[Profiles].Find(__T("MA"))!=string::npos || Data[Profiles].Find(__T("HRA"))!=string::npos)?"DTS-HD":"DTS");
    Fill(Stream_General, 0, General_Format_Profile, Retrieve(Stream_Audio, 0, Audio_Format_Profile));
    Fill(Stream_Audio, 0, Audio_Channel_s_, LegacyStreamDisplay?Data[Channels].Read():Data[Channels].Read(0));
    Fill(Stream_Audio, 0, Audio_ChannelPositions, LegacyStreamDisplay?Data[ChannelPositions].Read():Data[ChannelPositions].Read(0));
    Fill(Stream_Audio, 0, Audio_ChannelPositions_String2, LegacyStreamDisplay?Data[ChannelPositions2].Read():Data[ChannelPositions2].Read(0));
    Fill(Stream_Audio, 0, Audio_ChannelLayout, LegacyStreamDisplay?Data[ChannelLayout].Read():Data[ChannelLayout].Read(0));
    Fill(Stream_Audio, 0, Audio_BitDepth, LegacyStreamDisplay?Data[BitDepth].Read():Data[BitDepth].Read(0));
    Fill(Stream_Audio, 0, Audio_SamplingRate, LegacyStreamDisplay?Data[SamplingRate].Read():Data[SamplingRate].Read(0));
    Fill(Stream_Audio, 0, Audio_BitRate, LegacyStreamDisplay?Data[BitRate].Read():(Data[BitRate].Read(0)==__T("Unknown")?Ztring():Data[BitRate].Read(0)));
    Fill(Stream_Audio, 0, Audio_BitRate_Mode, LegacyStreamDisplay?Data[BitRate_Mode].Read():Data[BitRate_Mode].Read(0));
    Fill(Stream_General, 0, General_OverallBitRate_Mode, Retrieve(Stream_Audio, 0, Audio_BitRate_Mode));
    Fill(Stream_Audio, 0, Audio_Compression_Mode, LegacyStreamDisplay?Data[Compression_Mode].Read():Data[Compression_Mode].Read(0), true);

    // Cleanup up
    for (size_t Pos=0; Pos<10; ++Pos)
        Data[Pos].clear();

    //Samples per frame (to be done after SamplingRate filling because it is used during Fill())
    if (Presence[presence_Core_Core])
        Fill(Stream_Audio, 0, Audio_SamplesPerFrame, Number_Of_PCM_Sample_Blocks*32); // Samples per Core frames. TODO: samples per HD frames (interesting?)
}

//---------------------------------------------------------------------------
void File_Dts::Streams_Finish()
{
    Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, BigEndian?"Big":"Little", Unlimited, true, true);
    Fill(Stream_Audio, 0, Audio_Format_Settings_Mode, Word?"16":"14", Unlimited, true, true);

    if (FrameInfo.PTS!=(int64u)-1 && FrameInfo.PTS>PTS_Begin)
    {
        Fill(Stream_Audio, 0, Audio_Duration, ((float64)(FrameInfo.PTS-PTS_Begin))/1000000, 0, true);
        float64 SamplesPerFrame=Retrieve(Stream_Audio, 0, Audio_SamplesPerFrame).To_float64();
        float64 SamplingRate=Retrieve(Stream_Audio, 0, Audio_SamplingRate).To_float64();
        if (SamplesPerFrame && SamplingRate)
            Fill(Stream_Audio, 0, Audio_FrameCount, ((float64)(FrameInfo.PTS-PTS_Begin))/1000000000/(SamplesPerFrame/SamplingRate), 0, true);
    }
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dts::FileHeader_Begin()
{
    //Must have enough buffer for having header
    if (Buffer_Size<4)
        return false; //Must wait for more data

    //False positives detection: Detect WAV files, the parser can't detect it easily, there is only 70 bytes of beginning for saying WAV
    switch (CC4(Buffer))
    {
        case 0x52494646 : //"RIFF"
        case 0x000001FD : //MPEG-PS private
                            Finish("DTS");
                            return false;
        default         :   ;
    }

    //All should be OK...
    if (!Frame_Count_Valid)
        Frame_Count_Valid=Config->ParseSpeed>=0.3?32:(IsSub?1:2);
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dts::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+6<=Buffer_Size)
    {
        if (!FrameSynchPoint_Test())
            return false; //Need more data
        if (Synched)
            break;
        Buffer_Offset++;
    }

    //Parsing last bytes if needed
    if (Buffer_Offset+6>Buffer_Size)
    {
        if (Buffer_Offset+5==Buffer_Size)
        {
            int64u Value=CC5(Buffer+Buffer_Offset);
            if ((Value&0xFFFFFFFFFCLL)!=0x7FFE8001FCLL  //16 bits and big    endian Core
             && (Value&0xFFFFFFFF00LL)!=0xFE7F018000LL  //16 bits and little endian Core
             && (Value&0xFFFFFFFFF7LL)!=0x1FFFE80007LL  //14 bits and big    endian Core
             && (Value&0xFFFFFFFFF0LL)!=0xFF1F00E8F0LL  //14 bits and little endian Core
             && (Value&0xFFFFFFFF00LL)!=0x6458202500LL) //16 bits and big    endian HD
                Buffer_Offset++;
        }
        if (Buffer_Offset+4==Buffer_Size)
        {
            int32u Value=CC4(Buffer+Buffer_Offset);
            if (Value!=0x7FFE8001  //16 bits and big    endian Core
             && Value!=0xFE7F0180  //16 bits and little endian Core
             && Value!=0x1FFFE800  //14 bits and big    endian Core
             && Value!=0xFF1F00E8  //14 bits and little endian Core
             && Value!=0x64582025) //16 bits and big    endian HD
                Buffer_Offset++;
        }
        if (Buffer_Offset+3==Buffer_Size)
        {
            int32u Value=CC3(Buffer+Buffer_Offset);
            if (Value!=0x7FFE80  //16 bits and big    endian Core
             && Value!=0xFE7F01  //16 bits and little endian Core
             && Value!=0x1FFFE8  //14 bits and big    endian Core
             && Value!=0xFF1F00  //14 bits and little endian Core
             && Value!=0x645820) //16 bits and big    endian HD
                Buffer_Offset++;
        }
        if (Buffer_Offset+2==Buffer_Size)
        {
            int16u Value=CC2(Buffer+Buffer_Offset);
            if (Value!=0x7FFE  //16 bits and big    endian Core
             && Value!=0xFE7F  //16 bits and little endian Core
             && Value!=0x1FFF  //14 bits and big    endian Core
             && Value!=0xFF1F  //14 bits and little endian Core
             && Value!=0x6458) //16 bits and big    endian HD
                Buffer_Offset++;
        }
        if (Buffer_Offset+1==Buffer_Size)
        {
            int8u Value=CC1(Buffer+Buffer_Offset);
            if (Value!=0x7F  //16 bits and big    endian Core
             && Value!=0xFE  //16 bits and little endian Core
             && Value!=0x1F  //14 bits and big    endian Core
             && Value!=0xFF  //14 bits and little endian Core
             && Value!=0x64) //16 bits and big    endian HD
                Buffer_Offset++;
        }
        return false;
    }

    //Synched
    return true;
}

//---------------------------------------------------------------------------
bool File_Dts::Synched_Test()
{
    //Padding
    while (Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]==0x00)
        Buffer_Offset++;

    //Must have enough buffer for having header
    if (Buffer_Offset+6>Buffer_Size)
        return false;

    //Quick test of synchro
    if (!FrameSynchPoint_Test())
        return false; //Need more data
    if (!Synched)
        return true;

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_Dts::Read_Buffer_Unsynched()
{
    FrameInfo=frame_info();
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Dts::Demux_UnpacketizeContainer_Test()
{
    int32u Value=CC4(Buffer+Buffer_Offset);
    if (Value==0x7FFE8001)
    {
        int16u Size=((BigEndian2int24u(Buffer+Buffer_Offset+5)>>4)&0x3FFF)+1;
        Demux_Offset=Buffer_Offset+Size;

        if (Buffer_Offset+Size>Buffer_Size && File_Offset+Buffer_Size!=File_Size)
            return false; //No complete frame

        Demux_UnpacketizeContainer_Demux();
    }

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dts::Header_Parse()
{
    Header_Fill_Size(Original_Size);
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dts::Data_Parse()
{
    if (Save_Buffer)
    {
        File_Offset+=Buffer_Offset;
        swap(Buffer, Save_Buffer);
        swap(Buffer_Offset, Save_Buffer_Offset);
        swap(Buffer_Size, Save_Buffer_Size);
        if (!Word)
            Element_Size=Element_Size*14/16;
    }

    //Parsing
    int32u Sync;
    Get_B4 (Sync,                                               "Sync");
    if (Sync==0x64582025)
    {
        //HD
        Element_Name("HD");
        Element_Code=1;

        int16u header_size;
        int8u  SubStreamIndex, NumAssets, NumAudioPresent;
        bool isBlownUpHeader, StaticFieldsPresent;
        Skip_B1(                                                "UserDefinedBits");
        BS_Begin();
        Get_S1 (2, SubStreamIndex,                              "ExtSSIndex");
        Get_SB (isBlownUpHeader,                                "HeaderSizeType");
        int8u ExtraSize=((int8u)isBlownUpHeader)<<2;
        Get_S2 ( 8+ExtraSize, header_size,                      "Bits4Header"); header_size++; Param_Info2(header_size, " bytes");
        Get_S3 (16+ExtraSize, HD_size,                          "Bits4ExSSFsize"); HD_size++; Param_Info2(HD_size, " bytes");
        auto CRC=Dts_CRC_CCIT_Compute(Buffer+Buffer_Offset+Element_Offset, header_size-5);
        if (CRC)
        {
            Element_Info1("CRC NOK");
            return;
        }
        TESTELSE_SB_GET(StaticFieldsPresent,                    "StaticFieldsPresent");
            std::vector<int32u> ActiveExSSMasks;
            Info_S1(2, RefClockCode,                            "RefClockCode"); Param_Info1(DTS_HD_RefClockCode[RefClockCode]);
            Get_S1 (3, HD_ExSSFrameDurationCode,                "ExSSFrameDurationCode"); HD_ExSSFrameDurationCode++; Param_Info1(HD_ExSSFrameDurationCode);
            TEST_SB_SKIP(                                       "TimeStampFlag");
                Skip_S5(36,                                     "TimeStamp");
            TEST_SB_END();
            Get_S1 (3, NumAudioPresent,                         "NumAudioPresent"); NumAudioPresent++; Param_Info2(NumAudioPresent, " channels");
            Get_S1 (3, NumAssets,                               "NumAssets"); NumAssets++; Param_Info2(NumAssets, " assets");
            for (int8u Pos=0; Pos<NumAudioPresent; Pos++)
            {
                int32u ActiveExSSMask;
                Get_S4 (SubStreamIndex+1, ActiveExSSMask,       "ActiveExSSMask");
                ActiveExSSMasks.push_back(ActiveExSSMask);
            }
            for (int8u Pos=0; Pos<NumAudioPresent; Pos++)
                for (int8u Pos2=0; Pos2<SubStreamIndex+1; Pos2+=2)
                    if (ActiveExSSMasks[Pos]%2)
                        Skip_S1(8,                              "ActiveAssetMask");
            TEST_SB_SKIP(                                       "MixMetadataEnbl");
                int8u Bits4MixOutMask, NumMixOutConfigs;
                Skip_S1(2,                                      "MixMetadataAdjLevel");
                Get_S1 (2, Bits4MixOutMask,                     "Bits4MixOutMask"); Bits4MixOutMask=4+Bits4MixOutMask*4; Param_Info2(Bits4MixOutMask, " bits");
                Get_S1 (2, NumMixOutConfigs,                    "NumMixOutConfigs"); NumMixOutConfigs++; Param_Info2(NumMixOutConfigs, " configs");
                for (int8u Pos=0; Pos<NumMixOutConfigs; Pos++)
                    Skip_S1(Bits4MixOutMask,                    "NumMixOutCh");
            TEST_SB_END();
        TESTELSE_SB_ELSE("Static fields present");
            NumAudioPresent=1;
            NumAssets=1;
        TESTELSE_SB_END();
        Asset_Sizes.clear();
        Element_Begin1("Sizes");
        for (int8u Pos=0; Pos<NumAssets; Pos++)
        {
            int32u Size;
            if (isBlownUpHeader)
            {
                Get_S3 (20, Size,                               "AssetFsize"); Size++; Param_Info2(Size, " bytes");
            }
            else
            {
                Get_S3 (16, Size,                               "AssetFsize"); Size++; Param_Info2(Size, " bytes");
            }
            Asset_Sizes.push_back(Size);
        }
        Element_End0();
        for (int8u Asset_Pos=0; Asset_Pos<NumAssets; Asset_Pos++)
        {
            Element_Begin1("AssetDescriptor");
            int16u AssetSize;
            Get_S2 (9, AssetSize,                               "AssetDescriptFsize");
            AssetSize++;  Param_Info2(AssetSize, " bytes?");
            Skip_S1(3,                                          "AssetIndex");
            if (StaticFieldsPresent)
            {
                TEST_SB_SKIP(                                   "AssetTypeDescrPresent");
                    Info_S1( 4, TypeDescriptor,                 "AssetTypeDescriptor"); Param_Info1(DTS_HD_TypeDescriptor[TypeDescriptor]);
                TEST_SB_END();
                TEST_SB_SKIP(                                   "LanguageDescrPresent");
                    Info_S3(24, LanguageDescriptor,             "LanguageDescriptor"); Param_Info1(Ztring().From_CC3(LanguageDescriptor));
                TEST_SB_END();
                TEST_SB_SKIP(                                   "InfoTextPresent");
                    int16u InfoTextByteSize;
                    Get_S2(10, InfoTextByteSize,                "InfoTextByteSize"); InfoTextByteSize++; Param_Info2(InfoTextByteSize, "bytes");
                    for (int16u Pos=0; Pos<InfoTextByteSize; Pos++)
                        Skip_S1(8,                              "InfoTextString");
                TEST_SB_END();
                Get_S1 (5, HD_BitResolution,                    "BitResolution"); HD_BitResolution++; Param_Info2(HD_BitResolution, " bits");
                Get_S1 (4, HD_MaximumSampleRate,                "MaxSampleRate"); Param_Info2(DTS_HD_MaximumSampleRate[HD_MaximumSampleRate], " Hz");
                Get_S1 (8, HD_TotalNumberChannels,              "TotalNumChs"); HD_TotalNumberChannels++; Param_Info2(HD_TotalNumberChannels, " channels");
                TEST_SB_GET(One2OneMapChannels2Speakers,        "One2OneMapChannels2Speakers");
                    int8u SpeakerActivityMaskBits, SpeakerRemapSetsCount;
                    if (HD_TotalNumberChannels>2)
                        Skip_SB(                                "TotalNumChs"); //else is 0
                    if (HD_TotalNumberChannels>6)
                        Skip_SB(                                "EmbeddedSixChFlag"); //else is 0
                    TESTELSE_SB_SKIP(                           "SpkrMaskEnabled");
                        Get_S1 (2, SpeakerActivityMaskBits,     "NumBits4SAMask"); Param_Info2(4+SpeakerActivityMaskBits*4, " bits");
                        SpeakerActivityMaskBits=4+SpeakerActivityMaskBits*4;
                        Get_S2 (SpeakerActivityMaskBits, HD_SpeakerActivityMask, "SpkrActivityMask"); Param_Info1(DTS_HD_SpeakerActivityMask(HD_SpeakerActivityMask).c_str());
                    TESTELSE_SB_ELSE(                           "SpkrMaskEnabled");
                        SpeakerActivityMaskBits=0;
                    TESTELSE_SB_END();
                    Get_S1 (3, SpeakerRemapSetsCount,           "NumSpkrRemapSets");
                    for (int8u Pos=0; Pos<SpeakerRemapSetsCount; Pos++)
                        Skip_S2(SpeakerActivityMaskBits,        "StndrSpkrLayoutMask");
                    for (int8u Pos=0; Pos<SpeakerRemapSetsCount; Pos++)
                    {
                        int8u NumDecCh4Remap;
                        Get_S1(5, NumDecCh4Remap,               "NumDecCh4Remap");
                        //Not finnished!
                    }
                TEST_SB_END();
                Element_End0();
            }
        }
        BS_End();
        Skip_XX(header_size-Element_Offset,                   "Unknown");
    }
    else
    {
        //Frame
        Element_Name("Frame");
        Element_Code=0;

        int8u  EncoderSoftwareRevision;
        bool   crc_present;
        BS_Begin();
        Info_SB(    FrameType,                                      "Frame Type"); Param_Info1(DTS_FrameType[FrameType]);
        Skip_S1( 5,                                                 "Deficit Sample Count");
        Get_SB (    crc_present,                                    "CRC Present");
        Get_S2 ( 7, Number_Of_PCM_Sample_Blocks,                    "Number of PCM Sample Blocks");
        Number_Of_PCM_Sample_Blocks++;
        Get_S2 (14, Primary_Frame_Byte_Size,                        "Primary Frame Byte Size minus 1");
        Primary_Frame_Byte_Size++;
        if (!Word)
            Primary_Frame_Byte_Size=Primary_Frame_Byte_Size*16/14; //Word is on 14 bits
        Param_Info2(Primary_Frame_Byte_Size, " bytes");
        Get_S1 ( 6, channel_arrangement,                            "Audio Channel Arrangement"); Param_Info2C(channel_arrangement<sizeof(DTS_Channels), DTS_Channels[channel_arrangement], " channels");
        Core_Core_AMODE=channel_arrangement;
        Get_S1 ( 4, sample_frequency,                               "Core Audio Sampling Frequency"); Param_Info2(DTS_SamplingRate[sample_frequency], " Hz");
        Get_S1 ( 5, bit_rate,                                       "Transmission Bit Rate"); Param_Info2(DTS_BitRate[bit_rate], " bps");
        Skip_SB(                                                    "Embedded Down Mix Enabled");
        Skip_SB(                                                    "Embedded Dynamic Range");
        Skip_SB(                                                    "Embedded Time Stamp");
        Skip_SB(                                                    "Auxiliary Data");
        Skip_SB(                                                    "HDCD");
        Get_S1 ( 3, ExtensionAudioDescriptor,                       "Extension Audio Descriptor"); Param_Info1(DTS_ExtensionAudioDescriptor[ExtensionAudioDescriptor]);
        Get_SB (    ExtendedCoding,                                 "Extended Coding");
        Skip_SB(                                                    "Audio Sync Word Insertion");
        Get_S1 ( 2, lfe_effects,                                    "Low Frequency Effects");
        Core_Core_LFF=lfe_effects;
        Skip_SB(                                                    "Predictor History");
        if (crc_present)
            Skip_S2(16,                                             "Header CRC Check");
        Skip_SB(                                                    "Multirate Interpolator");
        Get_S1 ( 4, EncoderSoftwareRevision,                        "Encoder Software Revision");
        Skip_S1( 2,                                                 "Copy History");
        Get_S1 ( 2, bits_per_sample,                                "Source PCM Resolution"); Param_Info1(Ztring::ToZtring(DTS_Resolution[bits_per_sample])+__T(" bits"));
        Get_SB (    ES,                                             "ES");
        Skip_SB(                                                    "Front Sum/Difference");
        Skip_SB(                                                    "Surrounds Sum/Difference");
        Skip_S1( 4,                                                 "Dialog Normalisation Parameter");
        switch (EncoderSoftwareRevision)
        {
            case 6 :
                    Skip_S1( 4,                                     "Dialog Normalisation Parameter");
                    break;
            case 7 :
                    Skip_S1( 4,                                     "Dialog Normalisation Parameter");
                    break;
            default :
                    Skip_S1( 4,                                     "Unspecified");
                    break;
        }
        BS_End();
    }

    //Name
    Element_Info1(Ztring::ToZtring(Frame_Count));

    //PTS
    Element_Info1C((FrameInfo.PTS!=(int64u)-1), __T("PTS ")+Ztring().Duration_From_Milliseconds(float64_int64s(((float64)FrameInfo.PTS)/1000000)));

    //Counting
    if (File_Offset+Buffer_Offset+Element_Size==File_Size)
        Frame_Count_Valid=Frame_Count; //Finish frames in case of there are less than Frame_Count_Valid frames
    if (Element_Code==0 || !Core_Exists)
    {
        if (Frame_Count==0)
            PTS_Begin=FrameInfo.PTS;
        Frame_Count++;
        Frame_Count_InThisBlock++;
        float64 BitRate=BitRate_Get();
        if (BitRate)
        {
            FrameInfo.DUR=float64_int64s(((float64)(Element_Size+Header_Size))*8/BitRate*1000000000);
            if (FrameInfo.DTS!=(int64u)-1)
                FrameInfo.DTS+=FrameInfo.DUR;
            if (FrameInfo.PTS!=(int64u)-1)
                FrameInfo.PTS+=FrameInfo.DUR;
        }
    }

    //If filled
    if (Status[IsFilled])
    {
        //Little Endian and 14-bit streams management
        if (Save_Buffer)
        {
            delete[] Buffer;
            Buffer=Save_Buffer; Save_Buffer=NULL;
            Buffer_Offset=Save_Buffer_Offset;
            Buffer_Size=Save_Buffer_Size;
            File_Offset-=Buffer_Offset;
            if (!Word)
                Element_Size=Element_Size*16/14;
        }

        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }

    //Parsing
    switch(Element_Code)
    {
        case 0  : Core(); break;
        case 1  : HD(); break;
        default : Skip_XX(Element_Size,                         "Data");
    }

    //Little Endian and 14-bit streams management
    if (Save_Buffer)
    {
        delete[] Buffer;
        Buffer=Save_Buffer; Save_Buffer=NULL;
        Buffer_Offset=Save_Buffer_Offset;
        Buffer_Size=Save_Buffer_Size;
        File_Offset-=Buffer_Offset;
        if (!Word)
            Element_Size=Element_Size*16/14;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dts::Core()
{
    Element_Name("Frame");

    //It exists (not in XSA streams)
    Core_Exists=true;
    Presence.set(presence_Core_Core);

    //Looking for extensions
    int64u Core_Size=Element_Size, XCh_Sync=Element_Size, XXCh_Sync=Element_Size, X96k_Sync=Element_Size;
    if (ExtendedCoding)
    {
        //XCh
        if (ExtensionAudioDescriptor==0 || ExtensionAudioDescriptor==3)
            for (size_t Pos=0; Pos+4<=Element_Size; Pos++)
                if (CC4(Buffer+Buffer_Offset+Pos)==0x5A5A5A5A)
                    XCh_Sync=Pos;

        //XXCh
        if (ExtensionAudioDescriptor==6)
            for (size_t Pos=0; Pos+4<=Element_Size; Pos++)
                if (CC4(Buffer+Buffer_Offset+Pos)==0x47004A03)
                    XXCh_Sync=Pos;

        //X96k
        if (ExtensionAudioDescriptor==2 || ExtensionAudioDescriptor==3)
            for (size_t Pos=0; Pos+4<=Element_Size; Pos++)
                if (CC4(Buffer+Buffer_Offset+Pos)==0x1D95F262)
                    X96k_Sync=Pos;

        //Finding core size
        if (XCh_Sync<Core_Size)
            Core_Size=XCh_Sync;
        if (XXCh_Sync<Core_Size)
            Core_Size=XXCh_Sync;
        if (X96k_Sync<Core_Size)
            Core_Size=X96k_Sync;
    }

    //Parsing
    Skip_XX(Core_Size-Element_Offset,                           "Core data");
    if (ExtendedCoding && (ExtensionAudioDescriptor==2 || ExtensionAudioDescriptor==3))
    {
        Element_Begin0();
        Skip_B4(                                                "Magic");
        Core_X96k(XCh_Sync-Element_Offset);
        Element_End0();
    }
    if (ExtendedCoding && (ExtensionAudioDescriptor==0 || ExtensionAudioDescriptor==3))
    {
        Element_Begin0();
        Skip_B4(                                                "Magic");
        Core_XCh(Element_Size-Element_Offset);
        Element_End0();
    }
    if (ExtendedCoding && ExtensionAudioDescriptor==6)
    {
        Element_Begin0();
        Skip_B4(                                                "Magic");
        Core_XXCh(Element_Size-Element_Offset);
        Element_End0();
    }

    //Filling
    FILLING_BEGIN();
        if (!Status[IsAccepted] && Frame_Count>=2 && (Frame_Count>=Frame_Count_Valid || (Frame_Count_Valid && (File_Size-Buffer_TotalBytes_FirstSynched)/Frame_Count_Valid<Element_Size))) //2 frames mini in order to catch HD part. TODO: find a better way to accept stream in short files so with only few frames
        {
            Accept("DTS");
            Fill("DTS");

            //No more need data
            if (!IsSub && Config->ParseSpeed<1.0)
                Finish("DTS");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::Core_XCh(int64u Size)
{
    //Parsing
    Element_Name("XCh (6.1 channels)");
    int16u XChFSIZE;
    int8u  AMODE;
    BS_Begin();
    Get_S2 (10, XChFSIZE,                                       "Primary Frame Byte Size");
    Get_S1 ( 4, AMODE,                                          "Extension Channel Arrangement");
    BS_End();
    if (XChFSIZE==Element_Size-(Element_Offset-6))
        XChFSIZE--; //Compatibility reason (from specs)
    if ((int64u)XChFSIZE+1-6<=Size-2)
        Skip_XX(XChFSIZE+1-6,                                   "XCh data");
    else
        Skip_XX(Size-2,                                         "XCh data (with problem)");

    FILLING_BEGIN();
        channel_arrangement_XCh=AMODE;
        Presence.set(presence_Core_XCh);
        Core_XCh_AMODE=AMODE;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::Core_XXCh(int64u Size)
{
    Element_Name("XXCh (6.1 or 7.1 channels)");
    int8u ChannelsAdded;
    BS_Begin();
    Skip_S1 (6,                                                 "nuHeaderSizeXXCh minus 1");
    Skip_S1 (1,                                                 "bCRCPresent4ChSetHeaderXXCh minus 1");
    Skip_S1 (5,                                                 "nuBits4SpkrMaskXXCh minus 1");
    Get_S1  (2, ChannelsAdded,                                  "nuNumChSetsInXXCh minus 1");
    Skip_S1 (2,                                                 "?");
    BS_End();
    Skip_XX(Size-2,                                             "Data");

    FILLING_BEGIN();
        channel_arrangement_XCh=ChannelsAdded+1;
        Core_XXCh_nuNumChSetsInXXCh=ChannelsAdded+1;
        Presence.set(presence_Core_XXCh);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::Core_X96k(int64u Size)
{
    //Parsing
    Element_Name("X96k (96 KHz)");
    int16u FSIZE96;
    int8u  REVNO;
    BS_Begin();
    Get_S2 (12, FSIZE96,                                        "96 kHz Extension Frame Byte Data Size");
    Get_S1 ( 4, REVNO,                                          "Revision Number");
    BS_End();
    Skip_XX(Size-2,                                             "X96k data"); //FSIZE96 is until end, not X96k size

    FILLING_BEGIN();
        sample_frequency_X96k=14; //96KHz
        Profile="96/24";
        Presence.set(presence_Core_X96);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD()
{
    //Parsing
    while (Element_Offset<Element_Size)

    {
        //Looking for size
        int64u Next=Element_Offset+4;
        while (Next+4<=Element_Size)
        {
            int32u CC=CC4(Buffer+Buffer_Offset+(size_t)Next);
            if (CC==0x0A801921
             || CC==0x1D95F262
             || CC==0x41A29547
             || CC==0x47004A03
             || CC==0x5A5A5A5A
             || CC==0x655E315E)
                break;
            Next++;
        }
        if (Next+4>Element_Size)
            Next=Element_Size;

        Element_Begin0();
        int32u Magic;
        Get_B4 (Magic,                                          "Magic");
        switch (Magic)
        {
            case 0x0A801921 : HD_XSA(Next-Element_Offset);  break;
            case 0x1D95F262 : HD_X96k(Next-Element_Offset); break;
            case 0x41A29547 : HD_XLL(Next-Element_Offset);  break;
            case 0x47004A03 : HD_XXCh(Next-Element_Offset); break;
            case 0x5A5A5A5A : HD_XCh(Next-Element_Offset);  break;
            case 0x655E315E : HD_XBR(Next-Element_Offset);  break;
            default :
                        //Magic value is unknown
                        if (Profile.empty())
                            Profile="HD";
                        Skip_XX(Next-Element_Offset,            "Data");
        }
        Element_End0();
    }

    //Filling
    FILLING_BEGIN();
        if (Count_Get(Stream_Audio)==0 && Frame_Count>=Frame_Count_Valid)
        {
            Accept("DTS");
            Fill("DTS");

            //No more need data
            if (!IsSub && Config->ParseSpeed<1.0)
                Finish("DTS");
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_XCh(int64u Size)
{
    //Parsing
    Element_Name("XCh (6.1 channels)");
    Skip_XX(Size,                                               "Data");

    FILLING_BEGIN();
        channel_arrangement_XCh=1;
        if (Profile.empty())
            Profile="HRA";
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_XXCh(int64u Size)
{
    Element_Name("XXCh (6.1 or 7.1 channels)");
    int8u ChannelsAdded;
    BS_Begin();
    Skip_S1 (8,                                                 "?");
    Get_S1  (2, ChannelsAdded,                                  "Channels added?");
    Skip_S1 (6,                                                 "?");
    BS_End();
    Skip_XX(Size-2,                                             "Data");

    FILLING_BEGIN();
        channel_arrangement_XCh=ChannelsAdded;
        if (Profile.empty())
            Profile="HRA";
        Presence.set(presence_Extended_XXCh);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_X96k(int64u Size)
{
    //Parsing
    Element_Name("X96k (96 KHz)");
    Skip_XX(Size,                                               "Data");

    FILLING_BEGIN();
        sample_frequency=14; //96KHz
        Profile="HRA";
        Presence.set(presence_Extended_X96);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_XLL(int64u Size)
{
    Element_Name("XLL (LossLess)");

    // Quick and dirty search of DTS:X pattern
    if (!Presence[presence_Extended_XLL])
    {
        Extension_XLL_X_No=0;
        Extension_XLL_X_Yes=0;
        Extension_XLL_IMAXX_No=0;
        Extension_XLL_IMAXX_Yes=0;
    }

    // Parsing
    int64u End=Element_Offset+Size;
    int64u LLFrameSize;
    int16u ChSetHeaderSize;
    int8u HeaderSize, Bits4FrameFsize, NumChSetsInFrame, SegmentsInFrame, Bits4SSize, Bits4ChMask;
    bool ScalableLSBs;
    Element_Begin1("Common");
    BS_Begin();
    int64u EndBS=Data_BS_Remain();
    Skip_S1(4,                                                  "Version");
    Get_S1 (8, HeaderSize,                                      "HeaderSize"); Param_Info1(HeaderSize-3);
    auto CRC=Dts_CRC_CCIT_Compute(Buffer+Buffer_Offset+Element_Offset, HeaderSize-3);
    Get_S1 (5, Bits4FrameFsize,                                 "Bits4FrameFsize"); Param_Info1(Bits4FrameFsize+1);
    Get_S8 (Bits4FrameFsize+1, LLFrameSize,                     "LLFrameSize"); Param_Info1(LLFrameSize+1);
    Get_S1 (4, NumChSetsInFrame,                                "NumChSetsInFrame");
    NumChSetsInFrame++; Param_Info1(NumChSetsInFrame);
    Get_S1 (4, SegmentsInFrame,                                 "SegmentsInFrame"); Param_Info1(1<<SegmentsInFrame);
    Info_S1(4, SmplInSeg,                                       "SmplInSeg"); Param_Info1(1<<SmplInSeg);
    Get_S1 (5, Bits4SSize,                                      "Bits4SSize");
    Bits4SSize++; Param_Info1(Bits4SSize);
    Skip_S1(2,                                                  "BandDataCRCEn");
    Get_SB (   ScalableLSBs,                                    "ScalableLSBs");
    Get_S1 (5, Bits4ChMask,                                     "Bits4ChMask");
    Bits4ChMask++; Param_Info1(Bits4ChMask);
    if (ScalableLSBs)
        Skip_S1(4,                                              "FixedLSBWidth");
    auto RemainingBitHeader=(HeaderSize-5)*8-(EndBS-Data_BS_Remain());
    if (RemainingBitHeader)
    {
        int8u Padding=(int8u)-1;
        if (RemainingBitHeader<8)
            Peek_S1(RemainingBitHeader, Padding);
        Skip_BS(RemainingBitHeader,                             Padding?"(Unknown)":"Padding");;
    }
    BS_End();
    Skip_B2 (                                                   "CRC16Header"); Param_Info1(CRC?"NOK":"OK");
    Element_End0();
    for (int8u i=0; i<NumChSetsInFrame; i++)
    {
        Element_Begin1("Channel Set");
        BS_Begin();
        int64u EndBS=Data_BS_Remain();
        int8u ChSetLLChannel, BitResolution, FreqIndex, ReplacementSet;
        Get_S2 (10, ChSetHeaderSize,                            "ChSetHeaderSize");
        CRC=Dts_CRC_CCIT_Compute(Buffer+Buffer_Offset+Element_Offset, ChSetHeaderSize+1);
        Get_S1 (4, ChSetLLChannel,                              "ChSetLLChannel");
        ChSetLLChannel+=1;
        Skip_BS(ChSetLLChannel,                                 "ResidualChEncode");
        Get_S1 (5, BitResolution,                               "BitResolution"); BitResolution++;
        if (((int8s)HD_BitResolution_Real)<((int8s)BitResolution) && !CRC)
            HD_BitResolution_Real=BitResolution;
        Skip_S1(5,                                              "BitWidth");
        Get_S1 (4, FreqIndex,                                   "FreqIndex"); Param_Info1(DTS_HD_MaximumSampleRate[FreqIndex]);
        if (((int8s)HD_MaximumSampleRate_Real)<((int8s)FreqIndex) && !CRC)
            HD_MaximumSampleRate_Real=FreqIndex;
        Skip_S1(2,                                              "FsInterpolate");
        Get_S1 (2, ReplacementSet,                              "ReplacementSet");
        if (ReplacementSet)
        {
            Skip_SB(                                            "ActiveReplaceSet");
        }
        if (One2OneMapChannels2Speakers)
        {
            bool PrimaryChSet, DownmixCoeffCodeEmbedded, ChMaskEnabled;
            Get_SB (    PrimaryChSet,                           "PrimaryChSet");
            Get_SB (   DownmixCoeffCodeEmbedded,                "DownmixCoeffCodeEmbedded");
            if (DownmixCoeffCodeEmbedded)
                Skip_SB(                                        "DownmixEmbedded");
            if (DownmixCoeffCodeEmbedded && PrimaryChSet)
                Skip_S1(3,                                      "LLDownmixType");
            Skip_SB(                                            "HierChSet");
            if (DownmixCoeffCodeEmbedded)
            {
                //TODO
            }
            else
            {
            Get_SB (    ChMaskEnabled,                           "ChMaskEnabled");
            if (ChMaskEnabled)
            {
                Info_S4(Bits4ChMask, ChMask,                     "hMask"); Param_Info1(DTS_HD_SpeakerActivityMask(ChMask).c_str());
            }
            }
        }
        Skip_BS((ChSetHeaderSize-1)*8-(EndBS-Data_BS_Remain()), "(Not parsed)");
        BS_End();
        Skip_B2 (                                               "CRC16SubHeader"); Param_Info1(CRC?"NOK":"OK");
        Element_End0();
    }
    size_t Sizes_Total=0;
    size_t Count=(1<<SegmentsInFrame)*NumChSetsInFrame;
    if (DTS_HD_MaximumSampleRate[HD_MaximumSampleRate_Real]>DTS_SamplingRate[sample_frequency])
    {
        Count*=2;
        if (DTS_HD_MaximumSampleRate[HD_MaximumSampleRate_Real]>DTS_SamplingRate[sample_frequency]*2)
            Count*=2;
    }
    size_t NaviByteCount=(Count*Bits4SSize+7)/8+2;
    bool NaviIsfine=false;
    CRC=Dts_CRC_CCIT_Compute(Buffer+Buffer_Offset+Element_Offset, NaviByteCount);
    if (CRC)
    {
        while (CRC)
        {
            auto Buffer_Temp=Buffer+Buffer_Offset+Element_Offset+NaviByteCount;
            auto Buffer_End=Buffer+Buffer_Offset+End;
            while (CRC && Buffer_Temp<Buffer_End)
                CRC=(CRC>>8)^CRC_CCIT_Table[((uint8_t)CRC)^*Buffer_Temp++];
            if (!CRC)
            {
                NaviIsfine=true;
                Count=(Buffer_Temp-(Buffer+Buffer_Offset+Element_Offset)-2)*8/Bits4SSize;
            }
        }
    }
    else
        NaviIsfine=true;
    if (!NaviIsfine)
    {
        return;
    }
    Element_Begin1("NAVI");
    BS_Begin();
    for (size_t j=0; j<Count; j++)
    {
        int32u SegmentSize;
        Get_BS (Bits4SSize, SegmentSize,                        "SegmentSize");
        Sizes_Total+=SegmentSize;
        //TODO: Better management of HD sampling rate > base sampling rate
    }
    BS_End();
    Skip_B2(                                                    "CRC16");
    Element_End0();

    Skip_XX(Sizes_Total,                                        "Segments");
    Skip_XX(Count,                                              "1 byte per set?");
    if (End>Element_Offset)
    {
        auto Remainingbytes=Element_Offset%4;
        if (Remainingbytes)
            Remainingbytes=4-Remainingbytes;
        if (Remainingbytes)
            Skip_XX(Remainingbytes,                             "Padding?");
    }
    if (End>Element_Offset)
    {
        auto Remainingbytes=End-Element_Offset;
        if (Remainingbytes>3 && BigEndian2int32u(Buffer+Buffer_Offset+Element_Offset)==0x02000850)
        {
            Skip_XX(Remainingbytes,                             "DTS:X?");
            Extension_XLL_X_Yes++;
        }
        else
        {
            Extension_XLL_X_No++;
        }
        if (Remainingbytes>3 && (BigEndian2int32u(Buffer+Buffer_Offset+Element_Offset)>>1)==(0xF14000D0>>1))
        {
            Skip_XX(Remainingbytes,                             "IMAX?");
            Extension_XLL_IMAXX_Yes++;
        }
        else
        {
            Extension_XLL_IMAXX_No++;
        }
    }
    if (End>Element_Offset)
        Skip_XX(End-Element_Offset,                             "(Not parsed)");

    FILLING_BEGIN();
        Profile="MA";
        Presence.set(presence_Extended_XLL);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_XBR(int64u Size)
{
    Element_Name("XBR (BitRate extension)");
    Skip_XX(Size,                                               "Data");

    FILLING_BEGIN();
        Profile="HRA";
        Presence.set(presence_Extended_XBR);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dts::HD_XSA(int64u Size)
{
    Element_Name("LBR (low bitrate)");
    Skip_XX(Size,                                               "Data");

    FILLING_BEGIN();
        Profile="Express";
        Presence.set(presence_Extended_LBR);
    FILLING_END();
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
float64 File_Dts::BitRate_Get(bool WithHD)
{
    if (bit_rate<29 || Profile==__T("Express"))
    {
        float64 BitRate;
        if (Profile==__T("Express"))
            BitRate=0; //No core bitrate
        else if (DTS_SamplingRate[sample_frequency])
            BitRate=((float64)Primary_Frame_Byte_Size)*8/(Number_Of_PCM_Sample_Blocks*32)*DTS_SamplingRate[sample_frequency]; //(float64)DTS_BitRate[bit_rate];
        else
            BitRate=0; //Problem
        if (WithHD && HD_ExSSFrameDurationCode!=(int8u)-1)
        {
            int32u SamplePerFrames=HD_ExSSFrameDurationCode;
            switch (HD_MaximumSampleRate)
            {
                case  0 : //  8000
                case 10 : // 12000
                                SamplePerFrames*= 128; break;
                case  1 : // 16000
                case  5 : // 22050
                case 11 : // 24000
                                SamplePerFrames*= 256; break;
                case  2 : // 32000
                case  6 : // 44100
                case 12 : // 48000
                                SamplePerFrames*= 512; break;
                case  3 : // 64000
                case  7 : // 88200
                case 13 : // 96000
                                SamplePerFrames*=1024; break;
                case  4 : //128000
                case  8 : //176400
                case 14 : //192000
                                SamplePerFrames*=2048; break;
                case  9 : //352800
                case 15 : //384000
                                SamplePerFrames*=4096; break;
                default     :   SamplePerFrames=    0; break; //Can never happen (4 bits)
            }
            if (SamplePerFrames)
                BitRate+=((float64)HD_size)*8*DTS_HD_MaximumSampleRate[HD_MaximumSampleRate]/SamplePerFrames;
        }
        //if (Primary_Frame_Byte_Size && Profile==__T("HRA"))
        //    BitRate*=1+((float64)HD_size)/Primary_Frame_Byte_Size; //HD block are not in the nominal bitrate
        return BitRate;
    }
    else
        return 0;
}

//---------------------------------------------------------------------------
bool File_Dts::FrameSynchPoint_Test()
{
    if (Save_Buffer)
        return true; //Test already made by Synchronize()

    int64u Value=CC6(Buffer+Buffer_Offset);
    if ((Value&0xFFFFFFFFFC00LL)==0x7FFE8001FC00LL  //16 bits and big    endian Core
     || (Value&0xFFFFFFFF00FCLL)==0xFE7F018000FCLL  //16 bits and little endian Core
     || (Value&0xFFFFFFFFF7F0LL)==0x1FFFE80007F0LL  //14 bits and big    endian Core
     || (Value&0xFFFFFFFFF0F7LL)==0xFF1F00E8F007LL  //14 bits and little endian Core
     || (Value&0xFFFFFFFF0000LL)==0x645820250000LL) //16 bits and big    endian HD
    {
        //Configuration - 14 bits or Little Endian
        switch (Buffer[Buffer_Offset])
        {
            default   : Word=true;  BigEndian=true;  break; //16 bits and big    endian bitstream
            case 0xFE : Word=true;  BigEndian=false; break; //16 bits and little endian bitstream
            case 0x1F : Word=false; BigEndian=true;  break; //14 bits and big    endian bitstream
            case 0xFF : Word=false; BigEndian=false; break; //14 bits and little endian bitstream
        }

        int32u Size;
        if (Buffer[Buffer_Offset]==0x64) //HD
        {
            if (Buffer_Offset+10>Buffer_Size)
                return false; //Need more data

            bool isBlownUpHeader=(Buffer[Buffer_Offset+5]&0x20)?true:false;
            if (isBlownUpHeader)
                Size=((Buffer[Buffer_Offset+6]&0x01)<<19)
                   | ( Buffer[Buffer_Offset+7]      <<11)
                   | ( Buffer[Buffer_Offset+8]      << 3)
                   | ( Buffer[Buffer_Offset+9]      >> 5);
            else
                Size=((Buffer[Buffer_Offset+6]&0x1F)<<11)
                   | ( Buffer[Buffer_Offset+7]      << 3)
                   | ( Buffer[Buffer_Offset+8]      >> 5);
            Size++;
            Original_Size=Size;
        }
        else
        {
            if (Word)
            {
                if (Buffer_Offset+8>Buffer_Size)
                    return false; //Need more data

                if (BigEndian)
                    Size=((Buffer[Buffer_Offset+5]&0x03)<<12)
                       | ( Buffer[Buffer_Offset+6]      << 4)
                       | ( Buffer[Buffer_Offset+7]      >> 4);
                else
                    Size=((Buffer[Buffer_Offset+4]&0x03)<<12)
                       | ( Buffer[Buffer_Offset+7]      << 4)
                       | ( Buffer[Buffer_Offset+6]      >> 4);
                Size++;
                Original_Size=Size;
            }
            else
            {
                if (Buffer_Offset+10>Buffer_Size)
                    return false; //Need more data

                if (BigEndian)
                    Size=((Buffer[Buffer_Offset+6]&0x03)<<12)
                       | ( Buffer[Buffer_Offset+7]      << 4)
                       | ((Buffer[Buffer_Offset+8]&0x3C)>> 2);
                else
                    Size=((Buffer[Buffer_Offset+7]&0x03)<<12)
                       | ( Buffer[Buffer_Offset+6]      << 4)
                       | ((Buffer[Buffer_Offset+9]&0x3C)>> 2);
                Size++;
                Original_Size=Size*16/14;
            }
        }
        if (Buffer_Offset+Original_Size>Buffer_Size)
            return false; //Need more data

        if (!Word || !BigEndian)
        {
            Save_Buffer=Buffer;
            Save_Buffer_Offset=Buffer_Offset;
            Save_Buffer_Size=Buffer_Size;

            //Preparing new buffer
            if (Word)
            {
                int8u* Buffer_Little=new int8u[Size];
                for (size_t Pos=0; Pos+1<Size; Pos+=2)
                {
                    Buffer_Little[Pos+1]=Save_Buffer[Buffer_Offset+Pos  ];
                    Buffer_Little[Pos  ]=Save_Buffer[Buffer_Offset+Pos+1];
                }
                Save_Buffer=Buffer_Little;
                Save_Buffer_Offset=0;
                Save_Buffer_Size=Size;
            }
            else
            {
                int8u* Dest=new int8u[Size];
                for (size_t Pos=0; Pos+8<=Original_Size; Pos+=8)
                {
                    int64u Value;
                    if (BigEndian)
                        Value =(((int64u)(   BigEndian2int16u(Buffer+Buffer_Offset+Pos  )&0x3FFF))<<42)
                             | (((int64u)(   BigEndian2int16u(Buffer+Buffer_Offset+Pos+2)&0x3FFF))<<28)
                             | (((int64u)(   BigEndian2int16u(Buffer+Buffer_Offset+Pos+4)&0x3FFF))<<14)
                             | (((int64u)(   BigEndian2int16u(Buffer+Buffer_Offset+Pos+6)&0x3FFF))    );
                    else
                        Value =(((int64u)(LittleEndian2int16u(Buffer+Buffer_Offset+Pos  )&0x3FFF))<<42)
                             | (((int64u)(LittleEndian2int16u(Buffer+Buffer_Offset+Pos+2)&0x3FFF))<<28)
                             | (((int64u)(LittleEndian2int16u(Buffer+Buffer_Offset+Pos+4)&0x3FFF))<<14)
                             | (((int64u)(LittleEndian2int16u(Buffer+Buffer_Offset+Pos+6)&0x3FFF))    );
                    int56u2BigEndian(Dest+Pos*14/16, Value);
                }
                Save_Buffer=Dest;
                Save_Buffer_Offset=0;
                Save_Buffer_Size=Size;
            }
        }

        Synched=true;
    }
    else
        Synched=false;

    return true;
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_DTS_YES

