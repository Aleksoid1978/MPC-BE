/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Links:
//
// http://www.fileformat.info/format/jpeg/
// http://park2.wakwak.com/~tsuruzoh/Computer/Digicams/exif-e.html
// http://www.w3.org/Graphics/JPEG/jfif3.pdf
// http://www.sentex.net/~mwandel/jhead/
// https://github.com/corkami/formats/blob/master/image/jpeg.md
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
#if defined(MEDIAINFO_JPEG_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Jpeg.h"
#include "MediaInfo/Image/File_GainMap.h"
#if defined(MEDIAINFO_PSD_YES)
    #include "MediaInfo/Image/File_Psd.h"
#endif
#if defined(MEDIAINFO_C2PA_YES)
    #include "MediaInfo/Tag/File_C2pa.h"
#endif
#if defined(MEDIAINFO_EXIF_YES)
    #include "MediaInfo/Tag/File_Exif.h"
#endif
#if defined(MEDIAINFO_ICC_YES)
    #include "MediaInfo/Tag/File_Icc.h"
#endif
#if defined(MEDIAINFO_XMP_YES)
    #include "MediaInfo/Tag/File_Xmp.h"
#endif
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/Utils.h"
#include <vector>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
namespace Elements
{
    const int16u TEM =0xFF01;
    const int16u RE30=0xFF30; //JPEG 2000
    const int16u RE31=0xFF31; //JPEG 2000
    const int16u RE32=0xFF32; //JPEG 2000
    const int16u RE33=0xFF33; //JPEG 2000
    const int16u RE34=0xFF34; //JPEG 2000
    const int16u RE35=0xFF35; //JPEG 2000
    const int16u RE36=0xFF36; //JPEG 2000
    const int16u RE37=0xFF37; //JPEG 2000
    const int16u RE38=0xFF38; //JPEG 2000
    const int16u RE39=0xFF39; //JPEG 2000
    const int16u RE3A=0xFF3A; //JPEG 2000
    const int16u RE3B=0xFF3B; //JPEG 2000
    const int16u RE3C=0xFF3C; //JPEG 2000
    const int16u RE3D=0xFF3D; //JPEG 2000
    const int16u RE3E=0xFF3E; //JPEG 2000
    const int16u RE3F=0xFF3F; //JPEG 2000
    const int16u RE44=0xFF44; //JPEG 2000 Found with Kakadu
    const int16u SOC =0xFF4F; //JPEG 2000
    const int16u CAP =0xFF50; //JPEG 2000
    const int16u SIZ =0xFF51; //JPEG 2000
    const int16u COD =0xFF52; //JPEG 2000
    const int16u COC =0xFF53; //JPEG 2000
    const int16u TLM =0xFF55; //JPEG 2000
    const int16u PLM =0xFF57; //JPEG 2000
    const int16u PLT =0xFF58; //JPEG 2000
    const int16u CPF =0xFF59; //JPEG 2000 HT
    const int16u QCD =0xFF5C; //JPEG 2000
    const int16u QCC =0xFF5D; //JPEG 2000
    const int16u RGN =0xFF5E; //JPEG 2000
    const int16u POC =0xFF5F; //JPEG 2000
    const int16u PPM =0xFF60; //JPEG 2000
    const int16u PPT =0xFF61; //JPEG 2000
    const int16u CRG =0xFF63; //JPEG 2000
    const int16u CME =0xFF64; //JPEG 2000
    const int16u SEC =0xFF65; //JPEG 2000 Secure
    const int16u EPB =0xFF66; //JPEG 2000 Wireless
    const int16u ESD =0xFF67; //JPEG 2000 Wireless
    const int16u EPC =0xFF68; //JPEG 2000 Wireless
    const int16u RED =0xFF69; //JPEG 2000 Wireless
    const int16u SOT =0xFF90; //JPEG 2000
    const int16u SOP =0xFF91; //JPEG 2000
    const int16u EPH =0xFF92; //JPEG 2000
    const int16u SOD =0xFF93; //JPEG 2000
    const int16u ISEC=0xFF94; //JPEG 2000 Secure
    const int16u SOF0=0xFFC0;
    const int16u SOF1=0xFFC1;
    const int16u SOF2=0xFFC2;
    const int16u SOF3=0xFFC3;
    const int16u DHT =0xFFC4;
    const int16u SOF5=0xFFC5;
    const int16u SOF6=0xFFC6;
    const int16u SOF7=0xFFC7;
    const int16u JPG =0xFFC8;
    const int16u SOF9=0xFFC9;
    const int16u SOFA=0xFFCA;
    const int16u SOFB=0xFFCB;
    const int16u DAC =0xFFCC;
    const int16u SOFD=0xFFCD;
    const int16u SOFE=0xFFCE;
    const int16u SOFF=0xFFCF;
    const int16u RST0=0xFFD0;
    const int16u RST1=0xFFD1;
    const int16u RST2=0xFFD2;
    const int16u RST3=0xFFD3;
    const int16u RST4=0xFFD4;
    const int16u RST5=0xFFD5;
    const int16u RST6=0xFFD6;
    const int16u RST7=0xFFD7;
    const int16u SOI =0xFFD8;
    const int16u EOI =0xFFD9; //EOC in JPEG 2000
    const int16u SOS =0xFFDA;
    const int16u DQT =0xFFDB;
    const int16u DNL =0xFFDC;
    const int16u DRI =0xFFDD;
    const int16u DHP =0xFFDE;
    const int16u EXP =0xFFDF;
    const int16u APP0=0xFFE0;
    const int16u APP1=0xFFE1;
    const int16u APP2=0xFFE2;
    const int16u APP3=0xFFE3;
    const int16u APP4=0xFFE4;
    const int16u APP5=0xFFE5;
    const int16u APP6=0xFFE6;
    const int16u APP7=0xFFE7;
    const int16u APP8=0xFFE8;
    const int16u APP9=0xFFE9;
    const int16u APPA=0xFFEA;
    const int16u APPB=0xFFEB;
    const int16u APPC=0xFFEC;
    const int16u APPD=0xFFED;
    const int16u APPE=0xFFEE;
    const int16u APPF=0xFFEF;
    const int16u JPG0=0xFFF0;
    const int16u JPG1=0xFFF1;
    const int16u JPG2=0xFFF2;
    const int16u JPG3=0xFFF3;
    const int16u JPG4=0xFFF4;
    const int16u JPG5=0xFFF5;
    const int16u JPG6=0xFFF6;
    const int16u JPG7=0xFFF7;
    const int16u JPG8=0xFFF8;
    const int16u JPG9=0xFFF9;
    const int16u JPGA=0xFFFA;
    const int16u JPGB=0xFFFB;
    const int16u JPGC=0xFFFC;
    const int16u JPGD=0xFFFD;
    const int16u COM =0xFFFE;
}

//---------------------------------------------------------------------------
// Borland C++ does not accept local template
struct Jpeg_samplingfactor
{
    int8u Ci;
    int8u Hi;
    int8u Vi;
};

//---------------------------------------------------------------------------
void Jpeg_AddDec(string& Current, int8u Value)
{
    if (Value < 10)
        Current += '0' + Value;
    else
    {
        Current += '1';
        Current += '0' - 10 + Value;
    }
}

//---------------------------------------------------------------------------
string Jpeg_WithLevel(string Profile, int8u Level, bool HasSubLevel=false)
{
    Profile += '@';
    if (HasSubLevel)
        Profile += 'M'; // Has Mainlevel
    Profile += 'L';
    Jpeg_AddDec(Profile, Level & 0xF);
    if (HasSubLevel)
    {
        Profile += 'S'; // Has Sublevel
        Profile += 'L';
        Jpeg_AddDec(Profile, Level >> 4);
    }
    return Profile;
}

string Jpeg2000_Rsiz(int16u Rsiz)
{
    if (Rsiz&(1<<14))
    {
        string Result="HTJ2K";
        Rsiz^=(1<<14);
        if (Rsiz)
        {
            Result+=" / ";
            Result+=Jpeg2000_Rsiz(Rsiz);
        }
        return Result;
    }

    switch (Rsiz)
    {
        case 0x0000: return "No restrictions";
        case 0x0001: return "Profile-0";
        case 0x0002: return "Profile-1";
        case 0x0003: return "D-Cinema 2k";
        case 0x0004: return "D-Cinema 4k";
        case 0x0005: return "D-Cinema 2k Scalable";
        case 0x0006: return "D-Cinema 4k Scalable";
        case 0x0007: return "Long-term storage";
        case 0x0306: return "BCMR@L6"; //Broadcast Contribution Multi-tile Reversible
        case 0x0307: return "BCMR@L7"; //Broadcast Contribution Multi-tile Reversible
        default:
            switch ((Rsiz & 0xFFF0))
            {
                case 0x0100: return Jpeg_WithLevel("BCS", (int8u)Rsiz); //Broadcast Contribution Single-tile 
                case 0x0200: return Jpeg_WithLevel("BCM", (int8u)Rsiz); //Broadcast Contribution Multi-tile
                default:;
            }
            switch ((Rsiz & 0xFF00))
            {
                case 0x0400: return Jpeg_WithLevel("IMFS2k", (int8u)Rsiz, true); // IMF Single-tile 2k
                case 0x0500: return Jpeg_WithLevel("IMFS4k", (int8u)Rsiz, true); // IMF Single-tile 4k
                case 0x0600: return Jpeg_WithLevel("IMFS8k", (int8u)Rsiz, true); // IMF Single-tile 8k
                case 0x0700: return Jpeg_WithLevel("IMFMR2k", (int8u)Rsiz, true); // IMF Single/Multi-tile 2k
                case 0x0800: return Jpeg_WithLevel("IMFMR4k", (int8u)Rsiz, true); // IMF Single/Multi-tile 4k
                case 0x0900: return Jpeg_WithLevel("IMFMR8k", (int8u)Rsiz, true); // IMF Single/Multi-tile 8k
                default:;
            }
            return Ztring::ToZtring(Rsiz, 16).To_UTF8();
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Jpeg::File_Jpeg()
{
    //Config
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Jpeg;
        StreamIDs_Width[0]=0;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(8); //Stream
    #endif //MEDIAINFO_TRACE
    MustSynchronize=true;
    StreamSource=IsStream;

    //In
    StreamKind=Stream_Image;
    Interlaced=false;
    MxfContentKind=(int8u)-1;
    #if MEDIAINFO_DEMUX
    FrameRate=0;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Jpeg::Streams_Accept()
{
    if (!IsSub)
    {
        TestContinuousFileNames();
        if (Config->File_Names.size() > 1 || Config->File_IsReferenced_Get())
            StreamKind = Stream_Video;
        if (!Count_Get(StreamKind))
            Stream_Prepare(StreamKind);
        if (Config->File_Names.size() > 1)
            Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
    }
    else
        Stream_Prepare(StreamKind);

    //Configuration
    Buffer_MaximumSize=64*1024*1024; //Some big frames are possible (e.g YUV 4:2:2 10 bits 1080p)
}

//---------------------------------------------------------------------------
void File_Jpeg::Streams_Accept_PerImage(const seek_item& Item)
{
    string Format;
    if (Item.Mime.empty() || !Item.Mime.rfind("image/", 0)) {
        Stream_Prepare(StreamKind);
        if (!Item.Mime.empty()) {
            Format = Item.Mime.substr(6);
            if (Format == "jpeg") {
                Format = "JPEG";
            }
            if (Format == "heic") {
                Format = "HEIC";
            }
            if (Format == "avif") {
                Format = "AVIF";
            }
        }
    }
    else if (!Item.Mime.rfind("video/", 0)) {
        Stream_Prepare(Stream_Video); // the content may or may not have audio or other content, but we can not know so let's put that this way for the moment
        Format = Item.Mime.substr(6); // Format of the container, until we parse the content
        {
            if (Format == "mp4") {
                Format = "MPEG-4";
            }
            if (Format == "quicktime") {
                Format = "QuickTime";
            }
        }
    }
    else {
        Stream_Prepare(Stream_Other);
        Format = Item.Mime;
    }
    Fill(StreamKind_Last, StreamPos_Last, "Format", Format);
    Fill(StreamKind_Last, StreamPos_Last, "Type", Item.Type[0]);
    Fill(StreamKind_Last, StreamPos_Last, "Type", Item.Type[1]);
    Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", Item.MuxingMode[0]);
    Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", Item.MuxingMode[1]);
}

//---------------------------------------------------------------------------
void File_Jpeg::Streams_Finish()
{
    for (auto& Item : Seek_Items_WithoutFirstImageOffset) {
        Data_Size -= Item.second.Size;
    }

    Streams_Finish_PerImage();

    for (auto& Item : Seek_Items_WithoutFirstImageOffset) {
        Streams_Accept_PerImage(Item.second);
        Fill(StreamKind_Last, StreamPos_Last, "StreamSize", Item.second.Size);
    }
}

//---------------------------------------------------------------------------
void File_Jpeg::Streams_Finish_PerImage()
{
    if (Data_Size && Data_Size != (int64u)-1) {
        if (StreamKind == Stream_Video && !IsSub && File_Size != (int64u)-1 && !Config->File_Sizes.empty())
            Fill(Stream_Video, 0, Video_StreamSize, File_Size - (File_Size - Data_Size) * Config->File_Sizes.size()); //We guess that the metadata part has a fixed size
        if (StreamKind == Stream_Image && (IsSub || File_Size != (int64u)-1)) {
            Fill(Stream_Image, StreamPos_Last, Image_StreamSize, Data_Size);
            Data_Size = 0;
        }
    }

    auto CurrentMainStream = StreamPos_Last;

    #if defined(MEDIAINFO_EXIF_YES)
    if (Exif_Parser) {
        if (CurrentMainStream == 0)
            Merge(*Exif_Parser, Stream_General, 0, 0, false);
        Merge(*Exif_Parser, StreamKind, 0, CurrentMainStream);
        size_t Count = Exif_Parser->Count_Get(StreamKind);
        for (size_t i = 1; i < Count; ++i) {
            Merge(*Exif_Parser, StreamKind, i, StreamPos_Last + 1, false);
            if (Seek_Items_PrimaryStreamPos) {
                Fill(StreamKind, StreamPos_Last, "MuxingMode_MoreInfo", "Muxed in Image #" + to_string(Seek_Items_PrimaryStreamPos + 1));
            }
        }
        Exif_Parser.reset();
    }
    #endif
    #if defined(MEDIAINFO_PSD_YES)
    if (PSD_Parser) {
        if (CurrentMainStream == 0)
            Merge(*PSD_Parser, Stream_General, 0, 0, false);
        Merge(*PSD_Parser, StreamKind, 0, CurrentMainStream);
        size_t Count = PSD_Parser->Count_Get(StreamKind);
        for (size_t i = 1; i < Count; ++i) {
            Merge(*PSD_Parser, StreamKind, i, StreamPos_Last + 1, false);
        }
        PSD_Parser.reset();
    }
    #endif
    #if defined(MEDIAINFO_ICC_YES)
    if (ICC_Parser) {
        Merge(*ICC_Parser, StreamKind, 0, CurrentMainStream);
        ICC_Parser.reset();
    }
    #endif
    for (const auto& Item : XmpExt_List)
    {
        if (Item.second.Parser) {
            Item.second.Parser->Finish();
            if (CurrentMainStream == 0) {
                Merge(*Item.second.Parser, Stream_General, 0, 0);
                Merge(*Item.second.Parser, false);
            }
        }
    }
    XmpExt_List.clear();
    for (const auto& Item : JpegXtExt_List)
    {
        if (Item.second.Parser) {
            Item.second.Parser->Finish();
            if (CurrentMainStream == 0) {
                Merge(*Item.second.Parser, Stream_General, 0, 0);
                Merge(*Item.second.Parser, false);
            }
        }
    }
    JpegXtExt_List.clear();
}

//***************************************************************************
// Static stuff
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Jpeg::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<3)
        return false; //Must wait for more data

    if (Buffer[2]!=0xFF
     || (CC2(Buffer)!=Elements::SOI
      && CC2(Buffer)!=Elements::SOC))
    {
        Reject("JPEG");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Jpeg::Synchronize()
{
    //Synchronizing
    while(Buffer_Offset+2<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0xFF
                                        || Buffer[Buffer_Offset+1]==0x00))
        Buffer_Offset++;

    if (Buffer_Offset+1==Buffer_Size &&  Buffer[Buffer_Offset  ]!=0xFF)
        Buffer_Offset++;

    if (Buffer_Offset+2>Buffer_Size)
        return false;

    //Synched is OK
    Synched=true;
    return true;
}

//---------------------------------------------------------------------------
bool File_Jpeg::Synched_Test()
{
    if (SOS_SOD_Parsed)
        return true; ///No sync after SOD

    //Must have enough buffer for having header
    if (Buffer_Offset+2>Buffer_Size)
        return false;

    //Quick test of synchro
    if (Buffer[Buffer_Offset]!=0xFF)
    {
        Synched=false;
        return true;
    }

    //We continue
    return true;
}

//---------------------------------------------------------------------------
void File_Jpeg::Synched_Init()
{
    APP0_JFIF_Parsed=false;
    SOS_SOD_Parsed=false;
    CME_Text_Parsed=false;
    Data_Size=0;
    APPE_Adobe0_transform=(int8u)-1;
}

//***************************************************************************
// Buffer - Demux
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
bool File_Jpeg::Demux_UnpacketizeContainer_Test()
{
    if (!IsSub)
    {
        if (!Status[IsAccepted])
        {
            Accept();
            if (Config->Demux_EventWasSent)
                return false;
        }
        if (Config->File_Names.size()>1)
            return Demux_UnpacketizeContainer_Test_OneFramePerFile();
    }

    if (Interlaced && Buffer_Offset==0)
    {
        bool StartIsFound=false;
        while (Demux_Offset+2<=Buffer_Size)
        {
            int16u code=BigEndian2int16u(Buffer+Demux_Offset);
            Demux_Offset+=2;
            switch (code)
            {
                case Elements::SOD  :   //JPEG-2000 start
                                        StartIsFound=true;
                                        break;
                case Elements::TEM  :
                case Elements::RST0 :
                case Elements::RST1 :
                case Elements::RST2 :
                case Elements::RST3 :
                case Elements::RST4 :
                case Elements::RST5 :
                case Elements::RST6 :
                case Elements::RST7 :
                case Elements::SOC  :
                case Elements::SOI  :
                case Elements::EOI  :
                             break;
                default   :
                            if (Demux_Offset+2>Buffer_Size)
                                break;
                            {
                            int16u size=BigEndian2int16u(Buffer+Demux_Offset);
                            if (Demux_Offset+2+size>Buffer_Size)
                                break;
                            Demux_Offset+=size;
                            if (code==Elements::SOS) //JPEG start
                                StartIsFound=true;
                            }
            }
            if (StartIsFound)
                break;
        }

        while (Demux_Offset+2<=Buffer_Size)
        {
            while (Demux_Offset<Buffer_Size && Buffer[Demux_Offset]!=0xFF)
                Demux_Offset++;
            if (Demux_Offset+2<=Buffer_Size && Buffer[Demux_Offset+1]==0xD9) //EOI (JPEG 2000)
                break;
            Demux_Offset++;
        }
        if (Demux_Offset+2<=Buffer_Size)
            Demux_Offset+=2;
    }
    else
        Demux_Offset=Buffer_Size;

    if (Interlaced)
    {
        if (Field_Count==0 && FrameRate && Demux_Offset!=Buffer_Size)
            FrameRate*=2; //Now field rate
        if (FrameRate)
            FrameInfo.DUR=float64_int64s(1000000000/FrameRate); //Actually, field or frame rate
    }

    Demux_UnpacketizeContainer_Demux();

    if (Interlaced)
    {
        if (FrameInfo.DTS!=(int64u)-1 && FrameInfo.DUR!=(int64u)-1)
            FrameInfo.DTS+=FrameInfo.DUR;
    }

    return true;
}
#endif //MEDIAINFO_DEMUX

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Jpeg::Read_Buffer_Unsynched()
{
    SOS_SOD_Parsed=false;
    Data_Size=(int64u)-1;

    Read_Buffer_Unsynched_OneFramePerFile();
}

//---------------------------------------------------------------------------
void File_Jpeg::Read_Buffer_Continue()
{
    if (Config->ParseSpeed>=1.0 && IsSub && Status[IsFilled])
    {
        #if MEDIAINFO_DEMUX
            if (Buffer_TotalBytes<Demux_TotalBytes)
            {
                Skip_XX(Demux_TotalBytes-Buffer_TotalBytes,     "Data"); //We currently don't want to parse data during demux
                Param_Info1(Frame_Count);
                if (Interlaced)
                {
                    Field_Count++;
                    Field_Count_InThisBlock++;
                }
                if (!Interlaced || Field_Count%2==0)
                {
                    Frame_Count++;
                    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                        Frame_Count_NotParsedIncluded++;
                }
                return;
            }
        #endif //MEDIAINFO_DEMUX

        #if MEDIAINFO_DEMUX
        if (!Demux_UnpacketizeContainer)
        #endif //MEDIAINFO_DEMUX
        {
            Skip_XX(Buffer_Size,                                    "Data"); //We currently don't want to parse data during demux
            Param_Info1(Frame_Count);
            if (Interlaced)
                Field_Count+=2;
            Frame_Count++;
            if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
        }
    }
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Jpeg::Header_Parse()
{
    if (SOS_SOD_Parsed)
    {
        Header_Fill_Code(0, "Data");
        if (!Header_Parser_Fill_Size())
        {
            Element_WaitForMoreData();
            return;
        }
        return;
    }

    //Parsing
    int16u code, size;
    Get_B2 (code,                                               "Marker");
    switch (code)
    {
        case Elements::SOI:
            Seek_Items_PrimaryStreamPos = StreamPos_Last == (size_t)-1 ? 0 : StreamPos_Last;
            [[fallthrough]];
        case Elements::TEM :
        case Elements::RE30 :
        case Elements::RE31 :
        case Elements::RE32 :
        case Elements::RE33 :
        case Elements::RE34 :
        case Elements::RE35 :
        case Elements::RE36 :
        case Elements::RE37 :
        case Elements::RE38 :
        case Elements::RE39 :
        case Elements::RE3A :
        case Elements::RE3B :
        case Elements::RE3C :
        case Elements::RE3D :
        case Elements::RE3E :
        case Elements::RE3F :
        case Elements::RST0 :
        case Elements::RST1 :
        case Elements::RST2 :
        case Elements::RST3 :
        case Elements::RST4 :
        case Elements::RST5 :
        case Elements::RST6 :
        case Elements::RST7 :
        case Elements::SOC  :
        case Elements::SOD  :
        case Elements::EOI  :
                    size=0; break;
        default   : Get_B2 (size,                               "Fl - Frame header length");
    }

    //Filling
    Header_Fill_Code(code, Ztring().From_CC2(code));
    Header_Fill_Size(2+size);
}

//---------------------------------------------------------------------------
bool File_Jpeg::Header_Parser_Fill_Size()
{
    //Look for next Sync word
    if (Buffer_Offset_Temp==0) //Buffer_Offset_Temp is not 0 if Header_Parse_Fill_Size() has already parsed first frames
        Buffer_Offset_Temp=Buffer_Offset;

    #if MEDIAINFO_DEMUX
        if (Buffer_TotalBytes+2<Demux_TotalBytes)
            Buffer_Offset_Temp=(size_t)(Demux_TotalBytes-(Buffer_TotalBytes+2));
    #endif //MEDIAINFO_DEMUX

    while (Buffer_Offset_Temp+2<=Buffer_Size)
    {
        while (Buffer_Offset_Temp<Buffer_Size && Buffer[Buffer_Offset_Temp]!=0xFF)
            Buffer_Offset_Temp++;
        if (Buffer_Offset_Temp+2<=Buffer_Size && Buffer[Buffer_Offset_Temp+1]==0xD9) //EOI
            break;
        Buffer_Offset_Temp++;
    }

    //Must wait more data?
    if (Buffer_Offset_Temp+2>Buffer_Size)
    {
        if (/*FrameIsAlwaysComplete ||*/ File_Offset+Buffer_Size>=File_Size)
            Buffer_Offset_Temp=Buffer_Size; //We are sure that the next bytes are a start
        else
            return false;
    }

    //OK, we continue
    Header_Fill_Size(Buffer_Offset_Temp-Buffer_Offset);
    Buffer_Offset_Temp=0;
    return true;
}

//---------------------------------------------------------------------------
void File_Jpeg::Data_Parse()
{
    #define CASE_INFO(_NAME, _DETAIL) \
        case Elements::_NAME : Element_Info1(#_NAME); Element_Info1(_DETAIL); _NAME(); break;

    //Parsing
    if (SOS_SOD_Parsed)
    {
        Skip_XX(Element_Size,                                   "Data");
        SOS_SOD_Parsed=false;
        return;
    }
    switch (Element_Code)
    {
        CASE_INFO(TEM ,                                         "TEM");
        CASE_INFO(RE30,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE31,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE32,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE33,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE34,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE35,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE36,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE37,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE38,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE39,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3A,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3B,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3C,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3D,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3E,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE3F,                                         "Reserved"); //JPEG 2000
        CASE_INFO(RE44,                                         "Reserved"); //JPEG 2000
        CASE_INFO(SOC ,                                         "Start of codestream"); //JPEG 2000
        CASE_INFO(CAP ,                                         "Extended capabilities"); //JPEG 2000
        CASE_INFO(SIZ ,                                         "Image and tile size"); //JPEG 2000
        CASE_INFO(COD ,                                         "Coding style default"); //JPEG 2000
        CASE_INFO(COC ,                                         "Coding style component"); //JPEG 2000
        CASE_INFO(TLM ,                                         "Tile-part lengths, main header"); //JPEG 2000
        CASE_INFO(PLM ,                                         "Packet length, main header"); //JPEG 2000
        CASE_INFO(PLT ,                                         "Packet length, tile-part header"); //JPEG 2000
        CASE_INFO(CPF ,                                         "Corresponding Profile"); //JPEG 2000 HT
        CASE_INFO(QCD ,                                         "Quantization default"); //JPEG 2000
        CASE_INFO(QCC ,                                         "Quantization component "); //JPEG 2000
        CASE_INFO(RGN ,                                         "Region-of-interest"); //JPEG 2000
        CASE_INFO(POC ,                                         "Progression order change"); //JPEG 2000
        CASE_INFO(PPM ,                                         "Packed packet headers, main header"); //JPEG 2000
        CASE_INFO(PPT ,                                         "Packed packet headers, tile-part header"); //JPEG 2000
        CASE_INFO(CRG ,                                         "Component registration"); //JPEG 2000
        CASE_INFO(CME ,                                         "Comment and extension"); //JPEG 2000
        CASE_INFO(SEC ,                                         "Security"); //JPEG 2000 Secure
        CASE_INFO(EPB ,                                         "Error protection block"); //JPEG 2000 Wireless
        CASE_INFO(ESD ,                                         "Error sensitivity descriptor"); //JPEG 2000 Wireless
        CASE_INFO(EPC ,                                         "Error Protection Capability"); //JPEG 2000 Wireless
        CASE_INFO(RED ,                                         "Residual error descriptor"); //JPEG 2000 Wireless
        CASE_INFO(SOT ,                                         "Start of tile-part"); //JPEG 2000
        CASE_INFO(SOP ,                                         "Start of packet"); //JPEG 2000
        CASE_INFO(EPH ,                                         "End of packet header"); //JPEG 2000
        CASE_INFO(SOD ,                                         "Start of data"); //JPEG 2000
        CASE_INFO(ISEC,                                         "In-codestream security"); //JPEG 2000 Secure
        CASE_INFO(SOF0,                                         "Baseline DCT (Huffman)");
        CASE_INFO(SOF1,                                         "Extended sequential DCT (Huffman)");
        CASE_INFO(SOF2,                                         "Progressive DCT (Huffman)");
        CASE_INFO(SOF3,                                         "Lossless (sequential) (Huffman)");
        CASE_INFO(DHT ,                                         "Define Huffman Tables");
        CASE_INFO(SOF5,                                         "Differential sequential DCT (Huffman)");
        CASE_INFO(SOF6,                                         "Differential progressive DCT (Huffman)");
        CASE_INFO(SOF7,                                         "Differential lossless (sequential) (Huffman)");
        CASE_INFO(JPG ,                                         "Reserved for JPEG extensions");
        CASE_INFO(SOF9,                                         "Extended sequential DCT (Arithmetic)");
        CASE_INFO(SOFA,                                         "Progressive DCT (Arithmetic)");
        CASE_INFO(SOFB,                                         "Lossless (sequential) (Arithmetic)");
        CASE_INFO(DAC ,                                         "Define Arithmetic Coding");
        CASE_INFO(SOFD,                                         "Differential sequential DCT (Arithmetic)");
        CASE_INFO(SOFE,                                         "Differential progressive DCT (Arithmetic)");
        CASE_INFO(SOFF,                                         "Differential lossless (sequential) (Arithmetic)");
        CASE_INFO(RST0,                                         "Restart Interval Termination 0");
        CASE_INFO(RST1,                                         "Restart Interval Termination 1");
        CASE_INFO(RST2,                                         "Restart Interval Termination 2");
        CASE_INFO(RST3,                                         "Restart Interval Termination 3");
        CASE_INFO(RST4,                                         "Restart Interval Termination 4");
        CASE_INFO(RST5,                                         "Restart Interval Termination 5");
        CASE_INFO(RST6,                                         "Restart Interval Termination 6");
        CASE_INFO(RST7,                                         "Restart Interval Termination 7");
        CASE_INFO(SOI ,                                         "Start Of Image");
        CASE_INFO(EOI ,                                         "End Of Image"); //Is EOC (End of codestream) in JPEG 2000
        CASE_INFO(SOS ,                                         "Start Of Scan");
        CASE_INFO(DQT ,                                         "Define Quantization Tables");
        CASE_INFO(DNL ,                                         "Define Number of Lines");
        CASE_INFO(DRI ,                                         "Define Restart Interval");
        CASE_INFO(DHP ,                                         "Define Hierarchical Progression");
        CASE_INFO(EXP ,                                         "Expand Reference Components");
        CASE_INFO(APP0,                                         "Application-specific marker 0");
        CASE_INFO(APP1,                                         "Application-specific marker 1");
        CASE_INFO(APP2,                                         "Application-specific marker 2");
        CASE_INFO(APP3,                                         "Application-specific marker 3");
        CASE_INFO(APP4,                                         "Application-specific marker 4");
        CASE_INFO(APP5,                                         "Application-specific marker 5");
        CASE_INFO(APP6,                                         "Application-specific marker 6");
        CASE_INFO(APP7,                                         "Application-specific marker 7");
        CASE_INFO(APP8,                                         "Application-specific marker 8");
        CASE_INFO(APP9,                                         "Application-specific marker 9");
        CASE_INFO(APPA,                                         "Application-specific marker 10");
        CASE_INFO(APPB,                                         "Application-specific marker 11");
        CASE_INFO(APPC,                                         "Application-specific marker 12");
        CASE_INFO(APPD,                                         "Application-specific marker 13");
        CASE_INFO(APPE,                                         "Application-specific marker 14");
        CASE_INFO(APPF,                                         "Application-specific marker 15");
        CASE_INFO(JPG0,                                         "JPG");
        CASE_INFO(JPG1,                                         "JPG");
        CASE_INFO(JPG2,                                         "JPG");
        CASE_INFO(JPG3,                                         "JPG");
        CASE_INFO(JPG4,                                         "JPG");
        CASE_INFO(JPG5,                                         "JPG");
        CASE_INFO(JPG6,                                         "JPG");
        CASE_INFO(JPG7,                                         "JPG");
        CASE_INFO(JPG8,                                         "JPG");
        CASE_INFO(JPG9,                                         "JPG");
        CASE_INFO(JPGA,                                         "JPG");
        CASE_INFO(JPGB,                                         "JPG");
        CASE_INFO(JPGC,                                         "JPG");
        CASE_INFO(JPGD,                                         "JPG");
        CASE_INFO(COM ,                                         "Comment");
        default : Element_Info1("Reserved");
                  Skip_XX(Element_Size,                         "Data");
                  Data_Size = (int64u)-1;
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Jpeg::CAP()
{
    //Parsing
    int32u Pcap;
    Get_B4(Pcap,                                                "Pcap - Parts containing extended capabilities");
    vector<int8u> Ccap_i;
    for (int i=31; i>=0; i--)
    {
        if (Pcap & (1<<i))
            Ccap_i.push_back(32-i);
    }
    for (auto Version : Ccap_i)
    {
        Element_Begin1(("ISO/IEC 15444-" + to_string(Version)).c_str());
        switch (Version)
        {
            case 15:
                {
                int8u MAGB;
                bool HTIRV;
                BS_Begin();
                Skip_S1(2,                                      "HTONLY HTDECLARED MIXED");
                Skip_SB(                                        "MULTIHT");
                Skip_SB(                                        "RGN");
                Skip_SB(                                        "HETEROGENEOUS");
                Skip_S1(5,                                      "Reserved");
                Get_SB (   HTIRV,                               "HTIRV");
                Get_S1 (5, MAGB,                                "MAGB");
                if (!MAGB)
                    MAGB = 8;
                else if (MAGB < 20)
                    MAGB = MAGB + 8;
                else if (MAGB < 31)
                    MAGB = 4 * (MAGB - 19) + 27;
                else
                    MAGB = 74;
                Param_Info1(MAGB);
                Fill(StreamKind_Last, StreamPos_Last, "Compression_Mode", HTIRV?"Lossy":"Lossless", Unlimited, true, true); // TODO: "Lossy" not sure, spec says "can be used with irreversible transforms"
                BS_End();
                }
                break;
            default: Skip_B2(                                   "(Unknown)");
        }
        Element_End0();
    }

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Jpeg::SIZ()
{
    //Parsing
    vector<float> SamplingFactors;
    vector<int8u> BitDepths;
    int8u SamplingFactors_Max=0;
    int32u Xsiz, Ysiz;
    int16u Rsiz, Count;
    Get_B2 (Rsiz,                                               "Rsiz - Capability of the codestream");
    Get_B4 (Xsiz,                                               "Xsiz - Image size X");
    Get_B4 (Ysiz,                                               "Ysiz - Image size Y");
    Skip_B4(                                                    "XOsiz - Image offset X");
    Skip_B4(                                                    "YOsiz - Image offset Y");
    Skip_B4(                                                    "tileW - Size of tile W");
    Skip_B4(                                                    "tileH - Size of tile H");
    Skip_B4(                                                    "XTOsiz - Upper-left tile offset X");
    Skip_B4(                                                    "YTOsiz - Upper-left tile offset Y");
    Get_B2 (Count,                                              "Components and initialize related arrays");
    for (int16u Pos=0; Pos<Count; Pos++)
    {
        Element_Begin1("Initialize related array");
        int8u BitDepth, compSubsX, compSubsY;
        BS_Begin();
        Skip_SB(                                                "Signed");
        Get_S1 (7, BitDepth,                                    "BitDepth"); Param_Info1(1+BitDepth); Element_Info1(1+BitDepth);
        BS_End();
        Get_B1 (   compSubsX,                                   "compSubsX"); Element_Info1(compSubsX);
        Get_B1 (   compSubsY,                                   "compSubsY"); Element_Info1(compSubsY);
        Element_End0();

        //Filling list of HiVi
        if (compSubsX)
        {
            SamplingFactors.push_back(((float)compSubsY)/compSubsX);
            if (((float)compSubsY)/compSubsX>SamplingFactors_Max)
                SamplingFactors_Max=(int8u)((float)compSubsY)/compSubsX;
        }

        if (BitDepths.empty() || BitDepth!=BitDepths[0])
            BitDepths.push_back(BitDepth);
    }

    FILLING_BEGIN_PRECISE();
        if (Frame_Count==0 && Field_Count==0)
        {
            if (IsSub && !Interlaced && MxfContentKind<=1)
            {
                //Checking if a 2nd field is present
                size_t Size=(size_t)(Buffer_Offset+Element_Size);
                if (Size<Buffer_Size)
                {
                    auto End=Buffer+Buffer_Size-Size;
                    for (auto Search=Buffer+1; Search<End; Search++)
                        if (!memcmp(Buffer, Search, Size))
                        {
                            Interlaced=true;
                            break;
                        }
                }
            }

            Accept("JPEG 2000");
            Fill("JPEG 2000");

            Fill(Stream_General, 0, General_Format, "JPEG 2000");
            if (Count_Get(StreamKind_Last)==0)
                Stream_Prepare(StreamKind_Last);
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "JPEG 2000");
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "JPEG 2000");
            Fill(StreamKind_Last, StreamPos_Last, "Format_Profile", Jpeg2000_Rsiz(Rsiz));
            if (StreamKind_Last==Stream_Image)
                Fill(Stream_Image, StreamPos_Last, Image_Codec_String, "JPEG 2000", Unlimited, true, true); //To Avoid automatic filling
            Fill(StreamKind_Last, StreamPos_Last, StreamKind_Last==Stream_Image?(size_t)Image_Width:(size_t)Video_Width, Xsiz);
            Fill(StreamKind_Last, StreamPos_Last, StreamKind_Last==Stream_Image?(size_t)Image_Height:(size_t)Video_Height, Ysiz*(Interlaced?2:1)); //If image is from interlaced content, must multiply height by 2
            if (Interlaced)
                Fill(StreamKind_Last, StreamPos_Last, "ScanType", "Interlaced", Unlimited, true, true);

            if (BitDepths.size()==1)
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitDepth), 1+BitDepths[0]);

            //Chroma subsampling
            if (SamplingFactors_Max)
                while (SamplingFactors_Max<4)
                {
                    for (size_t Pos=0; Pos<SamplingFactors.size(); Pos++)
                        SamplingFactors[Pos]*=2;
                    SamplingFactors_Max*=2;
                }
            while (SamplingFactors.size()<3)
                SamplingFactors.push_back(0);
            Ztring ChromaSubsampling;
            for (size_t Pos=0; Pos<SamplingFactors.size(); Pos++)
                ChromaSubsampling+=Ztring::ToZtring(SamplingFactors[Pos], 0)+__T(':');
            if (!ChromaSubsampling.empty())
            {
                ChromaSubsampling.resize(ChromaSubsampling.size()-1);
                Fill(StreamKind_Last, StreamPos_Last, "ChromaSubsampling", ChromaSubsampling);

                //Not for sure
                if (ChromaSubsampling==__T("4:4:4") && (Retrieve(StreamKind_Last, StreamPos_Last, "Format_Profile")==__T("D-Cinema 2k") || Retrieve(StreamKind_Last, StreamPos_Last, "Format_Profile")==__T("D-Cinema 4k")))
                    Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "XYZ");
                else if (!IsSub)
                {
                    if (ChromaSubsampling==__T("4:2:0") || ChromaSubsampling==__T("4:2:2"))
                        Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YUV");
                    else if (ChromaSubsampling==__T("4:4:4"))
                        Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "RGB");
                }
            }
        }
    FILLING_END();

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Jpeg::COD()
{
    //Parsing
    int8u Style, Style2, Levels, MultipleComponentTransform;
    bool PrecinctUsed;
    Get_B1 (Style,                                              "Scod - Style");
        Get_Flags (Style, 0, PrecinctUsed,                      "Precinct used");
        Skip_Flags(Style, 1,                                    "Use SOP (start of packet)");
        Skip_Flags(Style, 2,                                    "Use EPH (end of packet header)");
    Get_B1 (Levels,                                             "Number of decomposition levels");
    Skip_B1(                                                    "Progression order");
    Skip_B2(                                                    "Number of layers");
    Info_B1(DimX,                                               "Code-blocks dimensions X (2^(n+2))"); Param_Info2(1<<(DimX+2), " pixels");
    Info_B1(DimY,                                               "Code-blocks dimensions Y (2^(n+2))"); Param_Info2(1<<(DimY+2), " pixels");
    Get_B1 (Style2,                                             "Style of the code-block coding passes");
        Skip_Flags(Style2, 0,                                   "Selective arithmetic coding bypass");
        Skip_Flags(Style2, 1,                                   "MQ states for all contexts");
        Skip_Flags(Style2, 2,                                   "Regular termination");
        Skip_Flags(Style2, 3,                                   "Vertically stripe-causal context formation");
        Skip_Flags(Style2, 4,                                   "Error resilience info is embedded on MQ termination");
        Skip_Flags(Style2, 5,                                   "Segmentation marker is to be inserted at the end of each normalization coding pass");
    Skip_B1(                                                    "Transform");
    Get_B1(MultipleComponentTransform,                          "Multiple component transform");
    if (PrecinctUsed)
    {
        BS_Begin();
        Skip_S1(4,                                              "LL sub-band width");
        Skip_S1(4,                                              "LL sub-band height");
        BS_End();
        for (int16u Pos=0; Pos<Levels; Pos++)
        {
            Element_Begin1("Decomposition level");
            BS_Begin();
            Skip_S1(4,                                          "decomposition level width");
            Skip_S1(4,                                          "decomposition level height");
            BS_End();
            Element_End0();
        }
    }

    FILLING_BEGIN();
        if (Frame_Count==0 && Field_Count==0)
        {
            switch (MultipleComponentTransform)
            {
                case 0x01 : Fill(StreamKind_Last, StreamPos_Last, "Compression_Mode", "Lossless", Unlimited, true, true); break;
                case 0x02 : Fill(StreamKind_Last, StreamPos_Last, "Compression_Mode", "Lossy", Unlimited, true, true); break;
                default   : ;
            }
        }
    FILLING_END();

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Jpeg::QCD()
{
    //Parsing
    Skip_B1(                                                    "Sqcd - Style");
    Skip_XX(Element_Size-Element_Offset,                        "QCD data");

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Jpeg::CME()
{
    //Parsing
    int16u Registration;
    Get_B2 (Registration,                                       "Registration");
    if (Registration != 1 && Element_Size >= 16) {
        int64u Probe;
        Peek_B8(Probe);
        if (Probe == 0x4372656174656420) { // "Created "
            Registration = 0x0001;
        }
    }
    switch (Registration) {
    case 0x0000: {
        Skip_XX(Element_Size - Element_Offset,                  "Comment");
        if (!CME_Text_Parsed) {
            Fill(IsSub ? StreamKind_Last : Stream_General, IsSub ? StreamPos_Last : 0, "Comment", "(Binary)");
        }
        break;
        }
    case 0x0001: {
        string Comment;
        Get_String(Element_Size - Element_Offset, Comment,      "Comment");
        Fill(IsSub ? StreamKind_Last : Stream_General, IsSub ? StreamPos_Last : 0, "Comment", Comment, true, Comment.rfind(Retrieve_Const(IsSub ? StreamKind_Last : Stream_General, IsSub ? StreamPos_Last : 0, "Comment").To_UTF8(), 0) == 0);
        break;
    }
    default: {
        Skip_XX(Element_Size - Element_Offset,                  "Comment");
        if (!CME_Text_Parsed) {
            Fill(IsSub ? StreamKind_Last : Stream_General, IsSub ? StreamPos_Last : 0, "Comment", "(Unknown)");
        }
    }
    }
    CME_Text_Parsed = true;
}

//---------------------------------------------------------------------------
void File_Jpeg::SOD()
{
    SOS_SOD_Parsed=true;
    if (Data_Size != (int64u)-1) {
        Data_Size += IsSub ? (Buffer_Size - (Buffer_Offset + Element_Size)) : (File_Size - (File_Offset + Buffer_Offset + Element_Size));
    }
    Data_Common();
    if (Interlaced)
    {
        Field_Count++;
        Field_Count_InThisBlock++;
    }
    if (!Interlaced || Field_Count%2==0)
    {
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (Status[IsFilled])
            Fill();
        if (Config->ParseSpeed<1.0)
            Finish("JPEG 2000"); //No need of more
    }
}

//---------------------------------------------------------------------------
void File_Jpeg::SOF_()
{
    //Parsing
    vector<Jpeg_samplingfactor> SamplingFactors;
    int16u Height, Width;
    int8u  Resolution, Count;
    Get_B1 (Resolution,                                         "P - Sample precision");
    Get_B2 (Height,                                             "Y - Number of lines");
    Get_B2 (Width,                                              "X - Number of samples per line");
    Get_B1 (Count,                                              "Nf - Number of image components in frame");
    for (int8u Pos=0; Pos<Count; Pos++)
    {
        Jpeg_samplingfactor SamplingFactor;
        Element_Begin1("Component");
        Get_B1 (   SamplingFactor.Ci,                           "Ci - Component identifier"); if (SamplingFactor.Ci>Count) Element_Info1(Ztring().append(1, (Char)SamplingFactor.Ci)); else Element_Info1(SamplingFactor.Ci);
        BS_Begin();
        Get_S1 (4, SamplingFactor.Hi,                           "Hi - Horizontal sampling factor"); Element_Info1(SamplingFactor.Hi);
        Get_S1 (4, SamplingFactor.Vi,                           "Vi - Vertical sampling factor"); Element_Info1(SamplingFactor.Vi);
        BS_End();
        Skip_B1(                                                "Tqi - Quantization table destination selector");
        Element_End0();

        //Filling list of HiVi
        SamplingFactors.push_back(SamplingFactor);
    }

    FILLING_BEGIN_PRECISE();
        if (Config->File_Names_Pos <= 1 && Field_Count % 2 == 0)
        {
            Accept("JPEG");
            Fill("JPEG");

            if (Retrieve_Const(Stream_General, 0, General_Format).empty()) {
                Fill(Stream_General, 0, General_Format, "JPEG");
            }
            if (Count_Get(StreamKind_Last)==0)
                Stream_Prepare(StreamKind_Last);
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "JPEG");
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Codec), "JPEG");
            if (StreamKind_Last==Stream_Image)
                Fill(Stream_Image, StreamPos_Last, Image_Codec_String, "JPEG", Unlimited, true, true); //To Avoid automatic filling
            if (StreamKind_Last==Stream_Video)
                Fill(Stream_Video, StreamPos_Last, Video_InternetMediaType, "video/JPEG", Unlimited, true, true);
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitDepth), Resolution);
            Fill(StreamKind_Last, StreamPos_Last, "Height", Height*(Interlaced?2:1));
            Fill(StreamKind_Last, StreamPos_Last, "Width", Width);

            //ColorSpace from http://docs.oracle.com/javase/1.4.2/docs/api/javax/imageio/metadata/doc-files/jpeg_metadata.html
            //TODO: if APPE_Adobe0_transform is present, indicate that K is inverted, see http://halicery.com/Image/jpeg/JPEGCMYK.html
            if (Retrieve_Const(StreamKind_Last, StreamPos_Last, "ColorSpace").empty())
            {
            switch (APPE_Adobe0_transform)
            {
                case 0x01 :
                            if (Count==3)
                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YUV");
                            break;
                case 0x02 :
                            if (Count==4)
                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YUVK");
                            break;
                default   :
                            {
                            int8u Ci[256];
                            memset(Ci, 0, 256);
                            for (int8u Pos=0; Pos<Count; Pos++)
                                Ci[SamplingFactors[Pos].Ci]++;

                            switch (Count)
                            {
                                case 1 :    Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "Y"); break;
                                case 2 :    Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YA"); break;
                                case 3 :
                                                 if (!APP0_JFIF_Parsed && Ci['R']==1 && Ci['G']==1 && Ci['B']==1)                                                       //RGB
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "RGB");
                                            else if ((Ci['Y']==1 && ((Ci['C']==1 && Ci['c']==1)                                                                         //YCc
                                                                  || Ci['C']==2))                                                                                       //YCC
                                                  || APP0_JFIF_Parsed                                                                                                   //APP0 JFIF header present so YCC
                                                  || APPE_Adobe0_transform==0                                                                                           //transform set to YCC
                                                  || (SamplingFactors[0].Ci==0 && SamplingFactors[1].Ci==1 && SamplingFactors[2].Ci==2)                                 //012
                                                  || (SamplingFactors[0].Ci==1 && SamplingFactors[1].Ci==2 && SamplingFactors[2].Ci==3))                                //123
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YUV");
                                            else if (APPE_Adobe0_transform==0 || APPE_Adobe0_transform==(int8u)-1)                                                      //transform set to RGB (it is a guess)
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "RGB");
                                            break;
                                case 4 :
                                                 if (!APP0_JFIF_Parsed && Ci['R']==1 && Ci['G']==1 && Ci['B']==1 && Ci['A']==1)                                         //RGBA
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "RGBA");
                                            else if ((Ci['Y']==1 && Ci['A']==1 && ((Ci['C']==1 && Ci['c']==1)                                                           //YCcA
                                                                                || Ci['C']==2))                                                                         //YCCA
                                                  || APP0_JFIF_Parsed                                                                                                   //APP0 JFIF header present so YCCA
                                                  || (SamplingFactors[0].Ci==0 && SamplingFactors[1].Ci==1 && SamplingFactors[2].Ci==2 && SamplingFactors[3].Ci==3)     //0123
                                                  || (SamplingFactors[0].Ci==1 && SamplingFactors[1].Ci==2 && SamplingFactors[2].Ci==3 && SamplingFactors[3].Ci==4))    //1234
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "YUVA");
                                            else if (Ci['C']==1 && Ci['M']==1 && Ci['Y']==1 && Ci['K']==1)                                                              //CMYK
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "CMYK");
                                            else if (APPE_Adobe0_transform==0 || APPE_Adobe0_transform==(int8u)-1)                                                      //transform set to CMYK (it is a guess)
                                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", "CMYK");
                                            break;
                                default:    ;
                            }
                            }
            }
            }

            //Chroma subsampling
            if ((SamplingFactors.size()==3 || SamplingFactors.size()==4) && SamplingFactors[1].Hi==1 && SamplingFactors[2].Hi==1 && SamplingFactors[1].Vi==1 && SamplingFactors[2].Vi==1)
            {
                string ChromaSubsampling;
                switch (SamplingFactors[0].Hi)
                {
                    case 1 :
                            switch (SamplingFactors[0].Vi)
                            {
                                case 1 : if (Retrieve(StreamKind_Last, StreamPos_Last, "ColorSpace").find(__T("YUV"))==0) ChromaSubsampling="4:4:4"; break;
                                default: ;
                            }
                            break;
                    case 2 :
                            switch (SamplingFactors[0].Vi)
                            {
                                case 1 : ChromaSubsampling="4:2:2"; break;
                                case 2 : ChromaSubsampling="4:2:0"; break;
                                default: ;
                            }
                            break;
                    case 4 :
                            switch (SamplingFactors[0].Vi)
                            {
                                case 1 : ChromaSubsampling="4:1:1"; break;
                                case 2 : ChromaSubsampling="4:1:0"; break;
                                default: ;
                            }
                            break;
                    default: ;
                }
                if (!ChromaSubsampling.empty())
                {
                    if (SamplingFactors.size()>3 && (SamplingFactors[3].Hi!=SamplingFactors[0].Hi || SamplingFactors[3].Vi!=SamplingFactors[0].Vi))
                        ChromaSubsampling+=":?";
                    Fill(StreamKind_Last, StreamPos_Last, "ChromaSubsampling", ChromaSubsampling);
                }
            }
        }
    FILLING_END();

    Data_Common();
}

//---------------------------------------------------------------------------
void File_Jpeg::SOS()
{
    //Parsing
    int8u Count;
    Get_B1 (Count,                                              "Number of image components in scan");
    for (int8u Pos=0; Pos<Count; Pos++)
    {
        Skip_B1(                                                "Scan component selector");
        Skip_B1(                                                "Entropy coding table destination selector");
    }
    Skip_B1(                                                    "Start of spectral or predictor selection");
    Skip_B1(                                                    "End of spectral selection");
    Skip_B1(                                                    "Successive approximation bit position");

    FILLING_BEGIN_PRECISE();
    SOS_SOD_Parsed=true;
    if (Data_Size != (int64u)-1) {
        Data_Size += IsSub ? (Buffer_Size - (Buffer_Offset + Element_Size)) : (File_Size - (File_Offset + Buffer_Offset + Element_Size));
    }
    Data_Common();
    if (Interlaced)
    {
        Field_Count++;
        Field_Count_InThisBlock++;
    }
    if (!Interlaced || Field_Count%2==0)
    {
        Frame_Count++;
        Frame_Count_InThisBlock++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
    }
    if (Status[IsFilled])
        Fill();

    if (GContainerItems_Offset) {
        if (Seek_Items_PrimaryImageType != "Primary") {
            Fill(StreamKind, Seek_Items_PrimaryStreamPos, "Type", Seek_Items_PrimaryImageType);
        }
        for (const auto& Item : Seek_Items_WithoutFirstImageOffset) {
            auto& Seek_Item = Seek_Items[GContainerItems_Offset + Item.first];
            Seek_Item.Type[1] = Item.second.Type[1];
            Seek_Item.MuxingMode[1] = Item.second.MuxingMode[1];
        }
        GContainerItems_Offset = 0;
        Seek_Items_PrimaryImageType.clear();
        Seek_Items_WithoutFirstImageOffset.clear();
    }

    for (auto& Item : Seek_Items) {
        if (Item.second.IsParsed) {
            continue;
        }
        Item.second.IsParsed = true;
        Data_Size -= (IsSub ? Buffer_Size : File_Size) - Item.first;
        Streams_Finish_PerImage();
        Streams_Accept_PerImage(Item.second);
        for (auto& Item2 : Seek_Items) {
            if (Item2.second.DependsOnFileOffset == Item.first) {
                Item2.second.DependsOnStreamPos = StreamPos_Last;
            }
        }
        if (Item.second.DependsOnStreamPos) {
            Fill(StreamKind_Last, StreamPos_Last, "MuxingMode_MoreInfo", "Muxed in Image #" + to_string(Item.second.DependsOnStreamPos + 1));
        }
        if (!Item.second.Mime.empty() && Item.second.Mime != "image/jpeg") {
            continue;
        }
        Seek_Items_PrimaryStreamPos = 0;
        SOS_SOD_Parsed = false;
        Synched = false;
        GoTo(Item.first);
        break;
    }

    if (Config->ParseSpeed < 1.0 && File_GoTo == (int64u)-1) {
        Finish("JPEG"); //No need of more
    }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0()
{
    //Parsing
    int32u Name;
    Get_C4(Name,                                                "Name");
    switch (Name)
    {
        case 0x41564931 : APP0_AVI1(); break; //"AVI1"
        case 0x4A464946 : APP0_JFIF(); break; //"JFIF"
        case 0x4A464646 : APP0_JFFF(); break; //"JFFF"
        default         : Skip_XX(Element_Size-Element_Offset,  "Unknown");
    }
}

//---------------------------------------------------------------------------
// From OpenDML AVI File Format Extensions
void File_Jpeg::APP0_AVI1()
{
    Element_Info1("AVI1");

    //Parsing
    bool UnknownInterlacement_IsDetected=false;
    int8u  FieldOrder=(int8u)-1;
    Get_B1 (FieldOrder,                                         "Polarity");
    if (Element_Size>=14)
    {
        int32u FieldSize, FieldSizeLessPadding;
        Skip_B1(                                                "Reserved");
        Get_B4 (FieldSize,                                      "FieldSize");
        Get_B4 (FieldSizeLessPadding,                           "FieldSizeLessPadding");

        //Coherency
        if (FieldOrder==0 && IsSub && FieldSize && FieldSize!=Buffer_Size)
        {
            if (FieldSizeLessPadding>1 && FieldSizeLessPadding<=Buffer_Size && Buffer[FieldSizeLessPadding-2]==0xFF && Buffer[FieldSizeLessPadding-1]==0xD9  //EOI
             &&                           FieldSize+1         < Buffer_Size && Buffer[FieldSize]             ==0xFF && Buffer[FieldSize+1]           ==0xD8) //SOI
                UnknownInterlacement_IsDetected=true;
        }
    }
    Skip_XX(Element_Size-Element_Offset,                        "Unknown");

    FILLING_BEGIN();
        if (Frame_Count==0 && Field_Count==0)
        {
            Accept();

            if (UnknownInterlacement_IsDetected)
            {
                Fill(Stream_Video, StreamPos_Last, Video_ScanType, "Interlaced");
                Interlaced=true;
            }
            else
            {
            switch (FieldOrder)
            {
                case 0x00 : Fill(Stream_Video, StreamPos_Last, Video_Interlacement, "PPF"); Fill(Stream_Video, StreamPos_Last, Video_ScanType, "Progressive"); break;
                case 0x01 : Fill(Stream_Video, StreamPos_Last, Video_Interlacement, "TFF"); Fill(Stream_Video, StreamPos_Last, Video_ScanType, "Interlaced"); Fill(Stream_Video, StreamPos_Last, Video_ScanOrder, "TFF"); Interlaced=true; break;
                case 0x02 : Fill(Stream_Video, StreamPos_Last, Video_Interlacement, "BFF"); Fill(Stream_Video, StreamPos_Last, Video_ScanType, "Interlaced"); Fill(Stream_Video, StreamPos_Last, Video_ScanOrder, "BFF"); Interlaced=true; break;
                default   : ;
            }
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0_JFIF()
{
    Element_Info1("JFIF");

    //Parsing
    Skip_B1(                                                    "Zero");
    int16u Width, Height;
    int8u  Unit, ThumbailX, ThumbailY;
    Skip_B2(                                                    "Version");
    Get_B1 (Unit,                                               "Unit"); //0=Pixels, 1=dpi, 2=dpcm
    Get_B2 (Width,                                              "Xdensity");
    Get_B2 (Height,                                             "Ydensity");
    Get_B1 (ThumbailX,                                          "Xthumbail");
    Get_B1 (ThumbailY,                                          "Ythumbail");
    Skip_XX(3*ThumbailX*ThumbailY,                              "RGB Thumbail");

    APP0_JFIF_Parsed=true;
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0_JFFF()
{
    Element_Info1("JFFF");

    Skip_B1(                                                    "Zero");
    Skip_B1(                                                    "extension_code"); //0x10 Thumbnail coded using JPEG, 0x11 Thumbnail stored using 1 byte/pixel, 0x13 Thumbnail stored using 3 bytes/pixel
    if (Element_Size>Element_Offset)
        Skip_XX(Element_Size-Element_Offset,                    "extension_data");
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0_JFFF_JPEG()
{
    //Parsing
    Element_Begin1("Thumbail JPEG");
        if (Element_Size>Element_Offset)
            Skip_XX(Element_Size-Element_Offset,                "Data");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0_JFFF_1B()
{
    //Parsing
    Element_Begin1("Thumbail 1 byte per pixel");
        int8u  ThumbailX, ThumbailY;
        Get_B1 (ThumbailX,                                      "Xthumbail");
        Get_B1 (ThumbailY,                                      "Ythumbail");
        Skip_XX(768,                                            "Palette");
        Skip_XX(ThumbailX*ThumbailY,                            "Thumbail");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Jpeg::APP0_JFFF_3B()
{
    //Parsing
    Element_Begin1("Thumbail 3 bytes per pixel");
        int8u  ThumbailX, ThumbailY;
        Get_B1 (ThumbailX,                                      "Xthumbail");
        Get_B1 (ThumbailY,                                      "Ythumbail");
        Skip_XX(3*ThumbailX*ThumbailY,                          "RGB Thumbail");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Jpeg::APP1()
{
    //Parsing
    if (Element_Size >= 29 && !strncmp((const char*)Buffer + Buffer_Offset, "http://ns.adobe.com/xap/1.0/", 29)) { // the char* contains a terminating \0
        Skip_String(29,                                         "Name");
        APP1_XMP();
        return;
    }
    if (Element_Size >= 29 && !strncmp((const char*)Buffer + Buffer_Offset, "http://ns.adobe.com/xmp/extension/", 35)) { // the char* contains a terminating \0
        Skip_String(35,                                         "Name");
        APP1_XMP_Extension();
        return;
    }
    if (Element_Size >= 6 && !strncmp((const char*)Buffer + Buffer_Offset, "Exif\0", 6)) { // the char* contains a second terminating \0
        Skip_String( 6,                                         "Name");
        APP1_EXIF();
        return;
    }
    Skip_XX(Element_Size - Element_Offset,                      "(Unknown)");
}

//---------------------------------------------------------------------------
void File_Jpeg::APP1_EXIF()
{
    Accept();

    Element_Info1("Exif");

    //Parsing
    #if defined(MEDIAINFO_EXIF_YES)
    auto MI = new File_Exif;
    Open_Buffer_Init(MI);
    Open_Buffer_Continue(MI);
    Open_Buffer_Finalize(MI);
    Exif_Parser.reset(MI);
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "EXIF Tags");
    #endif
}

//---------------------------------------------------------------------------
void File_Jpeg::APP1_XMP()
{
    Accept();

    Element_Info1("XMP");

    //Parsing
    #if defined(MEDIAINFO_XMP_YES)
    File_Xmp MI;
    gc_items GContainerItems;
    MI.GContainerItems = &GContainerItems;
    GainMap_metadata_Adobe.reset(new gm_data());
    MI.GainMapData = static_cast<gm_data*>(GainMap_metadata_Adobe.get());
    Open_Buffer_Init(&MI);
    auto Element_Offset_Sav = Element_Offset;
    Open_Buffer_Continue(&MI);
    Element_Offset = Element_Offset_Sav;
    Open_Buffer_Finalize(&MI);
    if (!GContainerItems.empty()) {
        int64u ImgOffset = 0;
        bool IsNotFirst = false;
        for (const auto& Entry : GContainerItems) {
            if (!IsNotFirst) {
                Seek_Items_PrimaryImageType = Entry.Semantic;
                IsNotFirst = true;
                continue;
            }
            auto& Seek_Item = Seek_Items_WithoutFirstImageOffset[ImgOffset];
            Seek_Item.Type[1] = Entry.Semantic;
            Seek_Item.MuxingMode[1] = "GContainer XMP";
            Seek_Item.Mime = Entry.Mime;
            Seek_Item.Size = Entry.Length;
            Seek_Item.Padding = Entry.Padding;
            ImgOffset += Entry.Length;
            ImgOffset += Entry.Padding;
        }
    }
    Element_Show(); //TODO: why is it needed?
    Merge(MI, Stream_General, 0, 0, false);
    #endif
    Skip_UTF8(Element_Size - Element_Offset,                    "XMP metadata");
}

//---------------------------------------------------------------------------
void File_Jpeg::APP1_XMP_Extension()
{
    Accept();

    Element_Info1("Extended XMP");

    //Parsing
    string GUID;
    int32u Size, Offset;
    Get_String(32, GUID,                                        "GUID");
    Get_B4 (Size,                                               "Full length");
    Get_B4 (Offset,                                             "Chunk offset");
    #if defined(MEDIAINFO_XMP_YES)
    auto Item = XmpExt_List.find(GUID);
    if (Item == XmpExt_List.end()) {
        if (Offset) {
            Skip_XX(Element_Size - Element_Offset,              "(Missing start of content)");
            return;
        }
        xmpext XmpExt(new File_Xmp());
        Open_Buffer_Init(XmpExt.Parser.get());
        Item = XmpExt_List.emplace(GUID, std::move(XmpExt)).first;
    }
    if (Offset != Item->second.LastOffset) {
        Skip_XX(Element_Size - Element_Offset,                  "(Missing intermediate content)");
        return;
    }
    Item->second.LastOffset += (int32u)(Element_Size - Element_Offset);
    auto& MI = *(File_Xmp*)Item->second.Parser.get();
    MI.Wait = Item->second.LastOffset < Size;
    auto Element_Offset_Sav = Element_Offset;
    gc_items GContainerItems;
    MI.GContainerItems = &GContainerItems;
    Open_Buffer_Continue(&MI);
    MI.GContainerItems = nullptr;
    if (!GContainerItems.empty()) {
        int64u ImgOffset = 0;
        bool IsNotFirst = false;
        for (const auto& Entry : GContainerItems) {
            if (!IsNotFirst) {
                Seek_Items_PrimaryImageType = Entry.Semantic;
                IsNotFirst = true;
                continue;
            }
            auto& Seek_Item = Seek_Items_WithoutFirstImageOffset[ImgOffset];
            Seek_Item.Type[1] = Entry.Semantic;
            Seek_Item.MuxingMode[1] = "GContainer Extended XMP";
            Seek_Item.Mime = Entry.Mime;
            Seek_Item.Size = Entry.Length;
            Seek_Item.Padding = Entry.Padding;
            ImgOffset += Entry.Length;
            ImgOffset += Entry.Padding;
        }
    }
    Element_Offset = Element_Offset_Sav;
    Element_Show();
    #endif
    Skip_UTF8(Element_Size - Element_Offset,                    "XMP metadata");
    return;
}

//---------------------------------------------------------------------------
void File_Jpeg::APP2()
{
    //Parsing
    auto Begin = Buffer + Buffer_Offset;
    auto Middle = Begin;
    auto End = Begin + (size_t)Element_Size;
    while (Middle < End && *Middle) {
        ++Middle;
    }
    auto Size = Middle - Begin;
    if (Size != Element_Size) {
        Size++;
        Skip_Local(Size,                                        "Signature");
        switch (Size) {
        case 4:
            if (BigEndian2int32u(Buffer + Buffer_Offset) == 0x4D504600) { // "MPF"
                APP2_MPF();
                return;
            }
            break;
        case 12:
            if (!strncmp((const char*)Buffer + Buffer_Offset, "ICC_PROFILE", 12)) {
                APP2_ICC_PROFILE();
                return;
            }
            break;
        case 28:
            if (!strncmp((const char*)Buffer + Buffer_Offset, "urn:iso:std:iso:ts:21496:-1", 28)) {
                APP2_ISO21496_1();
                return;
            }
            break;
        }
        Element_Info1(string((const char*)Buffer + Buffer_Offset, Size - 1));
    }
    Skip_XX(Element_Size - Element_Offset,                      "(Unknown)");
}

//---------------------------------------------------------------------------
void File_Jpeg::APP2_ICC_PROFILE()
{
    Element_Info1("ICC profile");

    //Parsing
    #if defined(MEDIAINFO_ICC_YES)
        int8u Pos, Max;
        Get_B1 (Pos,                                            "Chunk position");
        Get_B1 (Max,                                            "Chunk max");
        Element_Begin1("ICC profile");
        if (Pos == 1) {
            Accept("JPEG");
            ICC_Parser.reset(new File_Icc());
            ((File_Icc*)ICC_Parser.get())->StreamKind = StreamKind;
            Open_Buffer_Init(ICC_Parser.get());
        }
        if (ICC_Parser) {
            ((File_Icc*)ICC_Parser.get())->Frame_Count_Max = Max;
            ((File_Icc*)ICC_Parser.get())->IsAdditional = true;
            Open_Buffer_Continue(ICC_Parser.get());
            if (Pos == Max) {
                Open_Buffer_Finalize(ICC_Parser.get());
            }
        }
        else
        {
            Skip_XX(Element_Size-Element_Offset,                "ICC profile (partial)");
        }
        Element_End0();
    #else
        Skip_XX(Element_Size-Element_Offset,                    "ICC profile");
    #endif
}

//---------------------------------------------------------------------------
void File_Jpeg::APP2_ISO21496_1()
{
    Element_Info1("ISO 21496-1 Gain map metadata for image conversion");

    //Parsing
    File_GainMap MI;
    GainMap_metadata_ISO.reset(new GainMap_metadata());
    MI.output = static_cast<GainMap_metadata*>(GainMap_metadata_ISO.get());
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
}

//---------------------------------------------------------------------------
void File_Jpeg::APP2_MPF()
{
    Element_Info1("Multi-Picture Format");

    //Parsing
    #if defined(MEDIAINFO_EXIF_YES)
    mp_entries MPEntries;
    File_Exif MI;
    MI.MPEntries = &MPEntries;
    const auto Offset = File_Offset + Buffer_Offset + Element_Offset;
    Element_Begin1("Multi-Picture Format");
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Element_End0();
    if (!MPEntries.empty()) {
        vector<int64u> DependsOn;
        DependsOn.resize(MPEntries.size());
        size_t Pos = (size_t)-1;
        for (const auto& Entry : MPEntries) {
            ++Pos;
            if (Entry.DependentImg1EntryNo && Entry.DependentImg1EntryNo <= DependsOn .size() && !DependsOn[Entry.DependentImg1EntryNo - 1]) {
                DependsOn[Entry.DependentImg1EntryNo - 1] = Entry.ImgOffset ? (Offset + Entry.ImgOffset) : 0;
            }
            if (Entry.DependentImg2EntryNo && Entry.DependentImg2EntryNo <= DependsOn.size() && !DependsOn[Entry.DependentImg2EntryNo - 1]) {
                DependsOn[Entry.DependentImg2EntryNo - 1] = Entry.ImgOffset ? (Offset + Entry.ImgOffset) : 0;
            }
            if (!Entry.ImgOffset) {
                string Type = Entry.Type();
                if (!Seek_Items_PrimaryStreamPos) {
                    Fill(StreamKind, 0, "Type", Type);
                }
                continue;
            }
            auto& Seek_Item = Seek_Items[Offset + Entry.ImgOffset];
            Seek_Item.Type[0] = Entry.Type();
            Seek_Item.MuxingMode[0] = "MPF";
            Seek_Item.Size = Entry.ImgSize;
            Seek_Item.DependsOnStreamPos = Seek_Items_PrimaryStreamPos;
            if (DependsOn[Pos]) {
                Seek_Item.DependsOnFileOffset = DependsOn[Pos];
            }
        }
        if (MPEntries.size() > 1) {
            GContainerItems_Offset = Offset + MPEntries[1].ImgOffset;
        }
    }
    Merge(MI, Stream_General, 0, 0, false);
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "Multi-Picture Format");
    #endif
}

//---------------------------------------------------------------------------
void File_Jpeg::APPB()
{
    //Parsing
    int16u Name;
    Get_C2(Name,                                                "Name");
    switch (Name)
    {
    case 0x4A50 : APPB_JPEGXT(); break; //"JP"
    default     : Skip_XX(Element_Size - Element_Offset,        "Unknown");
    }
}

//---------------------------------------------------------------------------
void File_Jpeg::APPB_JPEGXT()
{
    Accept();

    Element_Info1("JPEG XT");

    //Parsing
    int32u SequenceNumber, Name;
    int16u Instance;
    Get_B2 (Instance,                                           "Box Instance");
    Get_B4 (SequenceNumber,                                     "Packet Sequence Number");

    //Probe if likely C2PA
    Element_Offset += 4;
    Peek_B4(Name);
    Element_Offset -= 4;

    switch (Name)
    {
    case 0x6A756D62 : APPB_JPEGXT_JUMB(Instance, SequenceNumber); break; //"jumb"
    default         : Skip_XX(Element_Size - Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
void File_Jpeg::APPB_JPEGXT_JUMB(int16u Instance, int32u SequenceNumber)
{
    #if defined(MEDIAINFO_C2PA_YES)
    auto Item = JpegXtExt_List.find(Instance);
    if (Item == JpegXtExt_List.end()) {
        if (SequenceNumber > 1) {
            Skip_XX(Element_Size - Element_Offset,              "(Missing start of content)");
            return;
        }
        jpegxtext JpegXtExt(new File_C2pa());
        JpegXtExt.LastSequenceNumber = SequenceNumber;
        int32u Size;
        Peek_B4(Size);
        Open_Buffer_Init(JpegXtExt.Parser.get(), Size);
        Item = JpegXtExt_List.emplace(Instance, std::move(JpegXtExt)).first;
    }
    else {
        auto TheoreticalSequenceNumber = Item->second.LastSequenceNumber + 1;
        if (SequenceNumber != TheoreticalSequenceNumber) {
            Skip_XX(Element_Size - Element_Offset,              "(Missing intermediate content)");
            return;
        }
        Item->second.LastSequenceNumber = TheoreticalSequenceNumber;
        Skip_B4(                                                "Total size repeated?");
        Skip_C4(                                                "jumb repeated?");
    }
    auto& MI = *Item->second.Parser.get();
    Open_Buffer_Continue(&MI);
    #endif
    Element_Show();
    return;
}

//---------------------------------------------------------------------------
void File_Jpeg::APPD()
{
    if (Element_Size >= 14 && !strncmp((const char*)Buffer + Buffer_Offset, "Photoshop 3.0", 14)) { // the char* contains a terminating \0
        Element_Info1("Photoshop");
        Skip_String(14,                                         "Name");

        //Parsing
        #if defined(MEDIAINFO_PSD_YES)
        auto MI = new File_Psd();
        MI->Step = File_Psd::Step_ImageResourcesBlock;
        Open_Buffer_Init(MI);
        Open_Buffer_Continue(MI);
        Open_Buffer_Finalize(MI);
        PSD_Parser.reset(MI);
        #else
        Skip_UTF8(Element_Size - Element_Offset,                "Photoshop Tags");
        #endif
    }
    else
        Skip_XX(Element_Size,                                   "(Unknown)");
}

//---------------------------------------------------------------------------
void File_Jpeg::APPE()
{
    //Parsing
    int64u Name;
    Get_C6(Name,                                                "Name");
    switch (Name)
    {
        case 0x41646F626500LL : APPE_Adobe0(); break; //"AVI1"
        default               : Skip_XX(Element_Size-Element_Offset, "Unknown");
    }
}

//---------------------------------------------------------------------------
void File_Jpeg::APPE_Adobe0()
{
    Element_Info1("Adobe");

    //Parsing
    int8u Version;
    Get_B1(Version,                                             "Version");
    if (Version==100)
    {
        int8u transform;
        Skip_B2(                                                "flags0");
        Skip_B2(                                                "flags1");
        Get_B1 (transform,                                      "transform");

        FILLING_BEGIN();
            APPE_Adobe0_transform=transform;
        FILLING_END();
    }
    else
        Skip_XX(Element_Size-Element_Offset,                    "unknown");
}

//---------------------------------------------------------------------------
void File_Jpeg::COM()
{
    //Parsing
    string Comment;
    Get_String(Element_Size - Element_Offset, Comment,          "Comment");
    auto StreamKind = IsSub ? StreamKind_Last : Stream_General;
    if (Comment.rfind("AVID", 0) == 0) {
        Fill(StreamKind, StreamPos_Last, "Encoded_Application_CompanyName", "Avid");
    }
    else {
        Fill(StreamKind, StreamPos_Last, "Comment", Comment);
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Jpeg::Data_Common()
{
    Skip_XX(Element_Size - Element_Offset,                      "Data");
    if (Data_Size != (int64u)-1) {
        Data_Size += Header_Size + Element_Size;
    }
}

} //NameSpace

#endif
