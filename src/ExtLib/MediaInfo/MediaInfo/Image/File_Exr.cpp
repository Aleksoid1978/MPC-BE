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
#if defined(MEDIAINFO_EXR_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Exr.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//***************************************************************************
// Const
//***************************************************************************

enum Elements
{
    Pos_FileInformation,
    Pos_GenericSection,
    Pos_IndustrySpecific,
    Pos_UserDefined,
    Pos_Padding,
    Pos_ImageData,
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Exr::File_Exr()
:File__Analyze()
{
    //Configuration
    ParserName="EXR";
    IsRawStream=true;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exr::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "EXR");

    if (!IsSub)
    {
        TestContinuousFileNames();

        Stream_Prepare((Config->File_Names.size()>1 || Config->File_IsReferenced_Get())?Stream_Video:Stream_Image);
        if (File_Size!=(int64u)-1)
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), File_Size);
        if (StreamKind_Last==Stream_Video)
            Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
    }
    else
        Stream_Prepare(Stream_Image);

    //Configuration
    Buffer_MaximumSize=64*1024*1024; //Some big frames are possible (e.g YUV 4:2:2 10 bits 1080p)
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Exr::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<4)
        return false; //Must wait for more data

    if (CC4(Buffer)!=0x762F3101) //"v/1"+1
    {
        Reject();
        return false;
    }

    //All should be OK...
    Accept();

    return true;
}

//---------------------------------------------------------------------------
void File_Exr::FileHeader_Parse()
{
    //Parsing
    int32u Flags;
    int8u Version;
    bool Deep, Multipart;
    Skip_L4(                                                    "Magic number");
    Get_L1 (Version,                                            "Version field");
    Get_L3 (Flags,                                              "Flags");
        Skip_Flags(Flags, 0,                                    "Single tile");
        Get_Flags (Flags, 1, LongName,                          "Long name");
        Get_Flags (Flags, 2, Deep,                              "Non-image");
        Get_Flags (Flags, 3, Multipart,                         "Multipart");

    //Filling
    if (Frame_Count==0)
    {
        Fill(Stream_General, 0, General_Format_Version, __T("Version ")+Ztring::ToZtring(Version));
        Fill(StreamKind_Last, 0, "Format", "EXR");
        Fill(StreamKind_Last, 0, "Format_Version", __T("Version ")+Ztring::ToZtring(Version));
        Fill(StreamKind_Last, 0, "Format_Profile", (Flags&0x02)?"Tile":"Line");
        if (Deep)
            Fill(Stream_General, 0, "Deep", "Yes");
        if (Deep)
            Fill(Stream_General, 0, "Multipart", "Yes");
    }
    Frame_Count++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;
    ImageData_End=Config->File_Current_Size;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Exr::Header_Begin()
{
    //Name
    name_End=0;
    while (Buffer_Offset+name_End<Buffer_Size)
    {
        if (Buffer[Buffer_Offset+name_End]=='\0')
            break;
        if (name_End>(LongName?255:31))
            break;
        name_End++;
    }
    if (Buffer_Offset+name_End>=Buffer_Size)
        return false;
    if (name_End>(LongName?255:31))
    {
        Reject();
        return false;
    }
    if (name_End==0)
        return true;

    //Type
    type_End=0;
    while (Buffer_Offset+name_End+1+type_End<Buffer_Size)
    {
        if (Buffer[Buffer_Offset+name_End+1+type_End]=='\0')
            break;
        if (type_End>(LongName?255:31))
            break;
        type_End++;
    }

    if (Buffer_Offset+name_End+1+type_End>=Buffer_Size)
        return false;
    if (type_End>(LongName?255:31))
    {
        Reject();
        return false;
    }

    if (Buffer_Offset+name_End+1+type_End+1+4>=Buffer_Size)
        return false;

    return true;
}

//---------------------------------------------------------------------------
void File_Exr::Header_Parse()
{
    //Image data
    if (name_End==0)
    {
        //Filling
        Header_Fill_Code(0, "Image data");
        Header_Fill_Size(ImageData_End-(File_Offset+Buffer_Offset));
        return;
    }

    int32u size;
    Get_String(name_End, name,                                  "name");
    Element_Offset++; //Null byte
    Get_String(type_End, type,                                  "type");
    Element_Offset++; //Null byte
    Get_L4 (size,                                               "size");

    //Filling
    Header_Fill_Code(0, Ztring().From_Local(name.c_str()));
    Header_Fill_Size(name_End+1+type_End+1+4+size);
}

//---------------------------------------------------------------------------
void File_Exr::Data_Parse()
{
         if (name_End==0)
        ImageData();
    else if (name=="channels" && type=="chlist")
        channels();
    else if (name=="comments" && type=="string")
        comments();
    else if (name=="compression" && type=="compression" && Element_Size==1)
        compression();
    else if (name=="dataWindow" && type=="box2i" && Element_Size==16)
        dataWindow();
    else if (name=="displayWindow" && type=="box2i" && Element_Size==16)
        displayWindow();
    else if (name=="pixelAspectRatio" && type=="float" && Element_Size==4)
        pixelAspectRatio();
    else
        Skip_XX(Element_Size,                                   "value");
}

//---------------------------------------------------------------------------
void File_Exr::ImageData()
{
    Skip_XX(Element_Size,                                       "data");

    if (!Status[IsFilled])
        Fill();
    if (Config->ParseSpeed<1.0)
        Finish();
}

//---------------------------------------------------------------------------
struct Exr_channel
{
    string name;
    int32u xSampling;
    int32u ySampling;
};
void File_Exr::channels()
{
    //Parsing
    std::vector<Exr_channel> ChannelList;
    while (Element_Offset+1<Element_Size)
    {
        Element_Begin1("channel");

        //Name
        size_t name_Size=0;
        while (Element_Offset+name_Size<Element_Size)
        {
            if (!Buffer[Buffer_Offset+(size_t)Element_Offset+name_Size])
                break;
            name_Size++;
        }
        name_End++;

        Exr_channel Channel;
        Get_String(name_Size, Channel.name,                 "name"); Element_Info1(Channel.name);
        Element_Offset++; //Null byte
        Skip_L4(                                            "pixel type");
        Skip_L1(                                            "pLinear");
        Skip_B3(                                            "reserved");
        Get_L4 (Channel.xSampling,                          "xSampling");
        Get_L4 (Channel.ySampling,                          "ySampling");
        ChannelList.push_back(Channel);

        Element_End0();
    }

    //Color space
    /* TODO: not finished
    bool HasAlpha=false;
    string ColorSpace, ChromaSubsampling;
    if (!ChannelList.empty() && ChannelList[0].name=="A")
    {
        HasAlpha=true;
        ChannelList.erase(ChannelList.begin());
    }
    if (ChannelList.size()==1 && ChannelList[0].name=="Y")
    {
        ColorSpace="Y";
    }
    else if (ChannelList.size()==3 && ChannelList[0].name=="V" && ChannelList[1].name=="U" && ChannelList[2].name=="Y")
    {
        ColorSpace="YUV";

        //Chroma subsampling
        if (ChannelList[2].xSampling==1 && ChannelList[2].xSampling==1 && ChannelList[0].xSampling==ChannelList[1].xSampling && ChannelList[0].ySampling==ChannelList[1].ySampling)
        {
            switch (ChannelList[0].xSampling)
            {
                case 1 :
                        switch (ChannelList[0].ySampling)
                        {
                            case 1 : ChromaSubsampling="4:4:4"; break;
                            default: ;
                        }
                        break;
                case 2 :
                        switch (ChannelList[0].ySampling)
                        {
                            case 1 : ChromaSubsampling="4:2:2"; break;
                            case 2 : ChromaSubsampling="4:2:0"; break;
                            default: ;
                        }
                        break;
                case 4 :
                        switch (ChannelList[0].ySampling)
                        {
                            case 1 : ChromaSubsampling="4:1:1"; break;
                            case 2 : ChromaSubsampling="4:1:0"; break;
                            default: ;
                        }
                        break;
                default: ;
            }
        }
    }
    else if (ChannelList.size()==3 && ChannelList[0].name=="B" && ChannelList[1].name=="G" && ChannelList[2].name=="R")
    {
        ColorSpace="RGB";
    }
    else
    {
        //TODO
    }
    if (!ColorSpace.empty())
    {
        if (HasAlpha)
            ColorSpace+='A';
        Fill(StreamKind_Last, 0, "ColorSpace", ColorSpace);
    }
    if (!ChromaSubsampling.empty())
        Fill(StreamKind_Last, 0, "ChromaSubsampling", ChromaSubsampling);
    */
}

//---------------------------------------------------------------------------
void File_Exr::comments ()
{
    //Parsing
    Ztring value;
    Get_Local(Element_Size, value,                              "value");

    //Filling
    if (Frame_Count==1)
        Fill(StreamKind_Last, 0, General_Comment, value);
}

//---------------------------------------------------------------------------
void File_Exr::compression ()
{
    //Parsing
    int8u value;
    Get_L1 (value,                                              "value");

    //Filling
    std::string Compression;
    switch (value)
    {
        case 0x00 : Compression="raw"; break;
        case 0x01 : Compression="RLZ"; break;
        case 0x02 : Compression="ZIPS"; break;
        case 0x03 : Compression="ZIP"; break;
        case 0x04 : Compression="PIZ"; break;
        case 0x05 : Compression="PXR24"; break;
        case 0x06 : Compression="B44"; break;
        case 0x07 : Compression="B44A"; break;
        default   : ;
    }

    if (Frame_Count==1)
        Fill(StreamKind_Last, 0, "Format_Compression", Compression.c_str());
}

//---------------------------------------------------------------------------
void File_Exr::dataWindow ()
{
    //Parsing
    int32u xMin, yMin, xMax, yMax;
    Get_L4 (xMin,                                               "xMin");
    Get_L4 (yMin,                                               "yMin");
    Get_L4 (xMax,                                               "xMax");
    Get_L4 (yMax,                                               "yMax");
}

//---------------------------------------------------------------------------
void File_Exr::displayWindow ()
{
    //Parsing
    int32u xMin, yMin, xMax, yMax;
    Get_L4 (xMin,                                               "xMin");
    Get_L4 (yMin,                                               "yMin");
    Get_L4 (xMax,                                               "xMax");
    Get_L4 (yMax,                                               "yMax");

    //Filling
    if (Frame_Count==1)
    {
        Fill(StreamKind_Last, 0, "Width", xMax-xMin+1);
        Fill(StreamKind_Last, 0, "Height", yMax-yMin+1);
    }
}

//---------------------------------------------------------------------------
void File_Exr::pixelAspectRatio ()
{
    //Parsing
    float value;
    Get_LF4(value,                                              "value");

    //Filling
    if (Frame_Count==1)
        Fill(StreamKind_Last, 0, "PixelAspectRatio", value?value:1, 3);
}

} //NameSpace

#endif //MEDIAINFO_EXR_YES
