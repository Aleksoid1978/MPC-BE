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
#if defined(MEDIAINFO_DSF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Dsf.h"
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const size_t DSF_FormatID_Size=1;
static const char*  DSF_FormatID[DSF_FormatID_Size]=
{
    "DSD",
};

//---------------------------------------------------------------------------
static const size_t DSF_ChannelType_Size=8;
static const char*  DSF_ChannelType[DSF_ChannelType_Size]=
{
    "",
    "Front: C",
    "Front: L R",
    "Front: L C R",
    "Front: L C R, LFE",
    "Front: L R, Side: L R",
    "Front: L C R, Side: L R",
    "Front: L C R, Side: L R, LFE",
};
static const char*  DSF_ChannelType_ChannelLayout[DSF_ChannelType_Size] =
{
    "",
    "C",
    "L R",
    "L R C",
    "L R C LFE",
    "L R Ls Rs",
    "L R C Ls Rs",
    "L R C Ls Rs LFE",
};

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int64u data = 0x64617461;
    const int64u DSD_ = 0x44534420;
    const int64u fmt_ = 0x666D7420;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dsf::File_Dsf()
:File__Analyze(), File__Tags_Helper()
{
    //File__Tags_Helper
    Base=this;

    //Configuration
    ParserName="Dsf";
    #if MEDIAINFO_EVENTS
        //ParserIDs[0]=MediaInfo_Parser_Dsf;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    MustSynchronize=true;
    DataMustAlwaysBeComplete=false;
}

//---------------------------------------------------------------------------
void File_Dsf::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "DSF");
    File__Tags_Helper::Stream_Prepare(Stream_Audio);
}

//---------------------------------------------------------------------------
void File_Dsf::Streams_Finish()
{
    int64u SamplingRate=Retrieve(Stream_Audio, 0, Audio_SamplingRate).To_int64u();
    for (int64u Multiplier=64; Multiplier<=512; Multiplier*=2)
    {
        int64u BaseFrequency=SamplingRate/Multiplier;
        if (BaseFrequency==48000 || BaseFrequency==44100)
        {
            Fill(Stream_Audio, 0, Audio_Format_Commercial_IfAny, __T("DSD")+Ztring::ToZtring(Multiplier));
            break;
        }
    }

    //Tags
    File__Tags_Helper::Streams_Fill();
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dsf::FileHeader_Begin()
{
    //Must have enough buffer for having header
    if (Buffer_Size<4)
        return false; //Must wait for more data

    if (Buffer[0]!=0x44 //"DSD "
     || Buffer[1]!=0x53
     || Buffer[2]!=0x44
     || Buffer[3]!=0x20)
    {
        File__Tags_Helper::Reject();
        return false;
    }

    //Temp
    Metadata=(int64u)-1;

    File__Tags_Helper::Accept();
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dsf::Synched_Test()
{
    if (File_Offset+Buffer_Offset==Metadata)
    {
        //Tags
        if (!File__Tags_Helper::Synched_Test())
            return false;
    }

    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsf::Header_Parse()
{
    //Parsing
    int64u Size;
    int32u Name;
    Get_C4 (Name,                                               "Name");
    Get_L8 (Size,                                               "Size");

    //Coherency check
    if (File_Offset+Buffer_Offset+Size>File_Size)
    {
        Size=File_Size-(File_Offset+Buffer_Offset);
        if (Element_Level<=2) //Incoherencies info only at the top level chunk
            Fill(Stream_General, 0, "IsTruncated", "Yes");
    }

    Header_Fill_Code(Name, Ztring().From_CC4(Name));
    Header_Fill_Size(Size);
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsf::Data_Parse()
{
    //Parsing
    DATA_BEGIN
    ATOM_PARTIAL(data)
    ATOM(DSD_)
    ATOM(fmt_)
    DATA_END
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dsf::data()
{
    //Parsing
    Skip_XX(Element_TotalSize_Get(),                            "sample data");

    //Filling
    Fill(Stream_Audio, 0, Audio_StreamSize, Element_TotalSize_Get());
}

//---------------------------------------------------------------------------
void File_Dsf::DSD_()
{
    //Parsing
    int64u FileSize;
    Get_L8 (FileSize,                                           "Total file size");
    Get_L8 (Metadata,                                           "Pointer to Metadata chunk");

    if (FileSize!=File_Size)
        Fill(Stream_General, 0, "Truncated", "Yes");
}

//---------------------------------------------------------------------------
void File_Dsf::fmt_()
{
    //Parsing
    int64u Count;
    int32u Version, FormatID, ChannelType, Num, Frequency, Bits;
    Get_L4 (Version,                                            "Format version");
    Get_L4 (FormatID,                                           "Format ID");
    Get_L4 (ChannelType,                                        "Channel Type");
    Get_L4 (Num,                                                "Channel num");
    Get_L4 (Frequency,                                          "Sampling frequency");
    Get_L4 (Bits,                                               "Bits per sample");
    Get_L8 (Count,                                              "Sample count");
    Skip_L4(                                                    "Block size per channel");
    Skip_L4(                                                    "Reserved");

    FILLING_BEGIN();
        Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(Version));
        if (FormatID<DSF_FormatID_Size)
            Fill(Stream_Audio, 0, Audio_Format, DSF_FormatID[FormatID]);
        else
            Fill(Stream_Audio, 0, Audio_Format, FormatID);
        if (FormatID<DSF_ChannelType_Size)
        {
            Fill(Stream_Audio, 0, Audio_ChannelPositions, DSF_ChannelType[ChannelType]);
            Fill(Stream_Audio, 0, Audio_ChannelLayout, DSF_ChannelType_ChannelLayout[ChannelType]);
        }
        else
        {
            Fill(Stream_Audio, 0, Audio_ChannelPositions, ChannelType);
            Fill(Stream_Audio, 0, Audio_ChannelLayout, ChannelType);
        }
        Fill(Stream_Audio, 0, Audio_Channel_s_, Num);
        Fill(Stream_Audio, 0, Audio_SamplingRate, Frequency);
        switch (Bits)
        {
            case 1 : Fill(Stream_Audio, 0, Audio_Format_Settings, "Little"); Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, "Little"); break;
            case 8 : Fill(Stream_Audio, 0, Audio_Format_Settings, "Big"); Fill(Stream_Audio, 0, Audio_Format_Settings_Endianness, "Big"); break;
            default:;
        }
        Fill(Stream_Audio, 0, Audio_SamplingCount, Count);
    FILLING_END();
}

//---------------------------------------------------------------------------
} //NameSpace

#endif //MEDIAINFO_DSF_YES

