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
#if defined(MEDIAINFO_DPX_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Dpx.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* DPX_Orientation[]=
{
    "Left to right, Top to bottom",
    "Right to left, Top to bottom",
    "Left to right, Bottom to top",
    "Right to left, Bottom to top",
    "Top to bottom, Left to right",
    "Top to bottom, Right to left",
    "Bottom to top, Left to right",
    "Bottom to top, Right to left",
    "Reserved for future use"
};

//---------------------------------------------------------------------------
static const char* DPX_Descriptors0[]=
{
    "User defined (or unspecified single component)",
    "Red (R)",
    "Green (G)",
    "Blue (B)",
    "Alpha (matte)",
    "", //No info
    "Luma (Y)",
    "Color Difference (CB, CR, subsampled by two)",
    "Depth (Z)",
    "Composite video"
};

static const char* DPX_Descriptors50[]=
{
    "R,G,B",
    "R,G,B, Alpha (A)",
    "A, B, G, R"
};

static const char* DPX_Descriptors100[]=
{
    "CB, Y, CR, Y (4:2:2) ---- based on SMPTE 125M",
    "CB, Y, A, CR, Y, A (4:2:2:4)",
    "CB, Y, CR (4:4:4)",
    "CB, Y, CR, A (4:4:4:4)"
};

const char* DPX_Descriptors150[]=
{
    "User-defined 2-component element",
    "User-defined 3-component element",
    "User-defined 4-component element",
    "User-defined 5-component element",
    "User-defined 6-component element",
    "User-defined 7-component element",
    "User-defined 8-component element"
};

static const char* DPX_Descriptors(int8u i)
{
    if(i<10)
        return DPX_Descriptors0[i];
    if(i<50)
        return "Reserved for future single components";
    if(i<53)
        return DPX_Descriptors50[i-50];
    if(i<100)
        return "Reserved for future RGB ++ formats";
    if(i<104)
        return DPX_Descriptors100[i-100];
    if(i<150)
        return "Reserved for future CBYCR ++ formats";
    if(i<157)
        return "Reserved for future single components";
    return "Reserved for future formats";
}

//---------------------------------------------------------------------------
static const char* DPX_Descriptors_ColorSpace(int8u i)
{
    switch (i)
    {
        case   1: return "R";
        case   2: return "G";
        case   3: return "B";
        case   4: return "A";
        case   6: return "Y";
        case   7: return "UV";
        case   8: return "Z";
        case  50: return "RGB";
        case  51:
        case  52: return "RGBA";
        case 100:
        case 102:
        case 103: return "YUV";
        case 101: return "YUVA";
        default : return "";
    }
}

//---------------------------------------------------------------------------
static const char* DPX_Descriptors_ChromaSubsampling(int8u i)
{
    switch (i)
    {
        case 100:
        case 101: return "4:2:2";
        default : return "";
    }
}

//---------------------------------------------------------------------------
static const char* DPX_TransferCharacteristic(int8u TransferCharacteristic)
{
    switch (TransferCharacteristic)
    {
        case  1 : return "Printing density";
        case  2 : return "Linear";
        case  3 : return "Logarithmic";                                 //Value not specified in specs
        case  5 : return "SMPTE 274M";                                  //Same as BT.709
        case  6 : return "BT.709";
        case  7 : return "BT.601 PAL";                                  //BT.470 System B, BT.470 System G, ISO does a difference between B and M
        case  8 : return "BT.601 NTSC";                                 //BT.470 System M, ISO does a difference between B and M
        case  9 : return "Composite NTSC";
        case 10 : return "Composite PAL";
        case 11 : return "Z (depth) - linear";
        case 12 : return "Z (depth) - homogeneous";
        case 13 : return "ADX";                                         //SMPTE ST 2065-3 Academy Density Exchange Encoding
        default : return "";
    }
};

//---------------------------------------------------------------------------
static const char* DPX_ColorimetricSpecification(int8u ColorimetricSpecification)
{
    switch (ColorimetricSpecification)
    {
        case  1 : return "Printing density";
        case  5 : return "SMPTE 274M";                                  //SMPTE 274M, for HDTV, mapped to BT.709
        case  6 : return "BT.709";
        case  7 : return "BT.601 PAL";                                  //BT.470 System B, BT.470 System G
        case  8 : return "BT.601 NTSC";                                 //BT.470 System M
        case  9 : return "Composite NTSC";
        case 10 : return "Composite PAL";
        case 13 : return "ADX";                                         //SMPTE ST 2065-3 Academy Density Exchange Encoding
        default : return "";
    }
}

//---------------------------------------------------------------------------
static const char* DPX_ValidBitDephs(int8u i)
{
    switch(i)
    {
        case 1  :
        case 8  :
        case 10 :
        case 12 :
        case 16 :
                    return "integer";
        case 32 :
                    return "IEEE floating point (R32)";
        case 64 :
                    return "IEEE floating point (R64)";
        default :
                    return "invalid";
    }
}

//---------------------------------------------------------------------------
static const char* DPX_ComponentDataPackingMethod[]=
{
    "Packed",
    "Filled A",
    "Filled B",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
static const char* DPX_ComponentDataEncodingMethod[]=
{
    "Raw",
    "RLE",
    "",
    "",
    "",
    "",
    "",
    "",
};

//---------------------------------------------------------------------------
static const char* DPX_VideoSignalStandard0[]=
{
    "Undefined",
    "NTSC",
    "PAL",
    "PAL-M",
    "SECAM"
};

static const char* DPX_VideoSignalStandard50[]=
{
    "YCBCR ITU-R 601-5 525-line, 2:1 interlace, 4:3 aspect ratio",
    "YCBCR ITU-R 601-5 625-line, 2:1 interlace, 4:3 aspect ratio"
};

static const char* DPX_VideoSignalStandard100[]=
{
    "YCBCR ITU-R 601-5 525-line, 2:1 interlace, 16:9 aspect ratio",
    "YCBCR ITU-R 601-5 625-line, 2:1 interlace, 16:9 aspect ratio"
};

static const char* DPX_VideoSignalStandard150[]=
{
    "YCBCR 1050-line, 2:1 interlace, 16:9 aspect ratio",
    "YCBCR 1125-line, 2:1 interlace, 16:9 aspect ratio (SMPTE 274M)",
    "YCBCR 1250-line, 2:1 interlace, 16:9 aspect ratio",
    "YCBCR 1125-line, 2:1 interlace, 16:9 aspect ratio (SMPTE 240M)"
};

static const char* DPX_VideoSignalStandard200[]=
{
    "YCBCR 525-line, 1:1 progressive, 16:9 aspect ratio",
    "YCBCR 625-line, 1:1 progressive, 16:9 aspect ratio",
    "YCBCR 750-line, 1:1 progressive, 16:9 aspect ratio (SMPTE 296M)",
    "YCBCR 1125-line, 1:1 progressive, 16:9 aspect ratio (SMPTE 274M)"
};

static const char* DPX_VideoSignalStandard(int8u i)
{
    if(i<5)
        return DPX_VideoSignalStandard0[i];
    if(i<50)
        return "Reserved for other composite video";
    if(i<52)
        return DPX_VideoSignalStandard50[i-50];
    if(i<100)
        return "Reserved for future component video";
    if(i<102)
        return DPX_VideoSignalStandard100[i-100];
    if(i<150)
        return "Reserved for future widescreen";
    if(i<154)
        return DPX_VideoSignalStandard150[i-150];
    if(i<200)
        return "Reserved for future high-definition interlace";
    if(i<204)
        return DPX_VideoSignalStandard200[i-200];
    return "Reserved for future high-definition progressive";
}

//***************************************************************************
// Const
//***************************************************************************

enum Elements
{
    Pos_GenericSection,
    Pos_IndustrySpecific,
    Pos_UserDefined,
    Pos_Padding,
    Pos_ImageData,
};

//***************************************************************************
// Helpers
//***************************************************************************

//
bool DPX_DateTime_Valid(const string &FromDpx)
{
    if (FromDpx.size()<19
     || FromDpx[ 0]<'0' || FromDpx[ 0]>'9'
     || FromDpx[ 1]<'0' || FromDpx[ 1]>'9'
     || FromDpx[ 2]<'0' || FromDpx[ 2]>'9'
     || FromDpx[ 3]<'0' || FromDpx[ 3]>'9'
     || FromDpx[ 4] != ':'
     || FromDpx[ 5]<'0' || FromDpx[ 5]>'1'
     || FromDpx[ 6]<'0' || FromDpx[ 6]>'9'
     || FromDpx[ 7] != ':'
     || FromDpx[ 8]<'0' || FromDpx[ 8]>'3'
     || FromDpx[ 9]<'0' || FromDpx[ 9]>'9'
     || FromDpx[10] != ':'
     || FromDpx[11]<'0' || FromDpx[11]>'2'
     || FromDpx[12]<'0' || FromDpx[12]>'9'
     || FromDpx[13] != ':'
     || FromDpx[14]<'0' || FromDpx[14]>'5'
     || FromDpx[15]<'0' || FromDpx[15]>'9'
     || FromDpx[16] != ':'
     || FromDpx[17]<'0' || FromDpx[17]>'5'
     || FromDpx[18]<'0' || FromDpx[18]>'9'
     )
        return false;

    return true;
}

string DPX_DateTime2Iso(const string &FromDpx)
{
    if (!DPX_DateTime_Valid(FromDpx))
        return string();

    // Date/Time
    string ToReturn(FromDpx.substr(0, 19));
    ToReturn[ 4]='-';
    ToReturn[ 7]='-';
    ToReturn[10]='T'; // Note: should be 'T' (ISO 8601), TODO: change to 'T' during next API change

    // Time zone
    if (FromDpx.size()>20)
    {
        size_t Offset;
        if (FromDpx[19]==':') // Note: Cineon and DPX v1 header date/time format is with ":LTZ", DPX v2 date/time format is "LTZ", but both are atually seen in any version. Always testing both.
            Offset=20;
        else
            Offset=19;

        size_t Max=FromDpx.find('\0');
        if (Max==(size_t)-1)
            Max=FromDpx.size();

        ToReturn+=FromDpx.substr(Offset, Max-Offset);

        if (ToReturn.size()>22)
            ToReturn.insert(ToReturn.begin(), ':'); //Hours/Minutes offset sepearator added
        else if (ToReturn.size()==22 && (ToReturn[19]=='+' || ToReturn[19]=='-'))
            ToReturn+=":00"; // Minutes offset, default
    }

    return ToReturn;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Dpx::File_Dpx()
:File__Analyze()
{
    //Configuration
    ParserName="DPX";
    StreamSource=IsStream;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dpx::Streams_Accept()
{
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

    //Filling
    Fill(StreamKind_Last, StreamPos_Last, "Format_Settings_Endianness" /*Fill_Parameter(StreamKind_Last, Generic_Format)*/, LittleEndian?"Little":"Big");
    Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", LittleEndian? "Little":"Big");
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Dpx::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<4)
        return false; //Must wait for more data

    int32u Magic=CC4(Buffer);
    switch (Magic)
    {
        case 0x802A5FD7 :   //       (Cineon Big)
        case 0xD75F2A80 :   //       (Cineon Little)
        case 0x53445058 :   //"SPDX" (Dpx Big)
        case 0x58504453 :   //"XDPS" (Dpx Little)
                            break;
        default         :
                            Reject();
                            return false;
    }

    //Generic Section size
    if (Buffer_Size<28)
        return false; //Must wait for more data
    Sizes_Pos=Pos_GenericSection;
    switch (Magic)
    {
        case 0x802A5FD7 :   //       (Cineon Big)
        case 0xD75F2A80 :   //       (Cineon Little)
                            IsDpx=false;
                            break;
        case 0x58504453 :   //"XDPS" (Dpx Little)
        case 0x53445058 :   //"SPDX" (Dpx Big)
                            IsDpx=true;
                            break;
        default         :   ;
    }
    switch (Magic)
    {
        case 0xD75F2A80 :   //       (Cineon Little)
        case 0x58504453 :   //"XDPS" (Dpx Little)
                            LittleEndian=true;
                            break;
        case 0x802A5FD7 :   //       (Cineon Big)
        case 0x53445058 :   //"SPDX" (Dpx Big)
                            LittleEndian=false;
                            break;
        default         :   ;
    }

    //All should be OK...
    Accept();

    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dpx::Header_Parse()
{
    if (Sizes_Pos==Pos_ImageData)
        DataMustAlwaysBeComplete=false; //We actually don't need to load the whole ImageData, as we don't parse it

    //Filling
    Header_Fill_Code(Sizes_Pos); //We use Sizes_Pos as the unique key
    if (Sizes.empty())
    {
        if (Element_Size<28)
        {
            Element_WaitForMoreData();
            return;
        }
        int32u Size=LittleEndian?LittleEndian2int32u(Buffer+Buffer_Offset+24):BigEndian2int32u(Buffer+Buffer_Offset+24);
        if (Size==(int32u)-1)
            Size=LittleEndian?LittleEndian2int32u(Buffer+Buffer_Offset+4):BigEndian2int32u(Buffer+Buffer_Offset+4);
        Header_Fill_Size(Size);
    }
    else
        Header_Fill_Size(Sizes[Sizes_Pos]);
}

//---------------------------------------------------------------------------
void File_Dpx::Data_Parse()
{
    if (!IsDpx) // Is Cineon
    {
        switch (Element_Code)
        {
            case Pos_GenericSection   : GenericSectionHeader_Cineon(); break;
            case Pos_IndustrySpecific : IndustrySpecificHeader_Cineon(); break;
            case Pos_UserDefined      : UserDefinedHeader_Cineon(); break;
            case Pos_Padding          : Padding(); break;
            case Pos_ImageData        : ImageData(); break;
            default                   : ;
        }
    }
    else
    {
        switch (Element_Code)
        {
            case Pos_GenericSection   : GenericSectionHeader_Dpx(); break;
            case Pos_IndustrySpecific : IndustrySpecificHeader_Dpx(); break;
            case Pos_UserDefined      : UserDefinedHeader_Dpx(); break;
            case Pos_Padding          : Padding(); break;
            case Pos_ImageData        : ImageData(); break;
            default                   : ;
        }
    }

    do
        Sizes_Pos++; //We go automaticly to the next block
    while (Sizes_Pos<Sizes.size() && Sizes[Sizes_Pos]==0);
    if (Sizes_Pos>=Sizes.size())
    {
        Sizes.clear();
        Sizes_Pos=0;

        if (!Status[IsFilled])
            Fill();
        if (File_Offset+Buffer_Offset+Element_Size<Config->File_Current_Size)
            GoTo(Config->File_Current_Size);
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dpx::GenericSectionHeader_Cineon()
{
    Element_Name("Generic section header");

    //Parsing
    Element_Begin1("File information");
    string CreationDate, CreationTime;
    string Version;
    int32u Size_Header, Size_Total, Size_Generic, Size_Industry, Size_User;
    Skip_B4(                                                    "Magic number");
    Get_X4 (Size_Header,                                        "Offset to image data");
    Get_X4 (Size_Generic,                                       "Generic section header length");
    Get_X4 (Size_Industry,                                      "Industry specific header length");
    Get_X4 (Size_User,                                          "User-defined header length");
    Get_X4 (Size_Total,                                         "Total image file size");
    Get_ASCII  (8, Version,                                     "Version number of header format");
    Skip_UTF8  (100,                                            "FileName");
    Get_ASCII  (12,  CreationDate,                              "Creation Date");
    Get_ASCII  (12,  CreationTime,                              "Creation Time");
    Skip_XX(36,                                                 "Reserved for future use");
    Element_End0();

    Element_Begin1("Image information");
    int8u ImageElements;
    Info_B1(ImageOrientation,                                   "Image orientation"); Param_Info1(DPX_Orientation[ImageOrientation>8?8:ImageOrientation]);
    Get_B1 (ImageElements,                                      "Number of image elements");
    Skip_B2(                                                    "Unused");
    if (ImageElements>8)
        ImageElements=8;
    for(int8u ImageElement=0; ImageElement<ImageElements; ImageElement++)
        GenericSectionHeader_Cineon_ImageElement();
    if (ImageElements!=8)
        Skip_XX((8-ImageElements)*28,                           "Padding");
    Skip_BF4(                                                   "White point - x");
    Skip_BF4(                                                   "White point - y");
    Skip_BF4(                                                   "Red primary chromaticity - x");
    Skip_BF4(                                                   "Red primary chromaticity - u");
    Skip_BF4(                                                   "Green primary chromaticity - x");
    Skip_BF4(                                                   "Green primary chromaticity - y");
    Skip_BF4(                                                   "Blue primary chromaticity - x");
    Skip_BF4(                                                   "Blue primary chromaticity - y");
    Skip_UTF8(200,                                              "Label text");
    Skip_XX(28,                                                 "Reserved for future use");
    Element_End0();

    Element_Begin1("Image Data Format Information");
    Skip_B1(                                                    "Data interleave");
    Skip_B1(                                                    "Packing");
    Skip_B1(                                                    "Data signed or unsigned");
    Skip_B1(                                                    "Image sense");
    Skip_B4(                                                    "End of line padding");
    Skip_B4(                                                    "End of channel padding");
    Skip_XX(20,                                                 "Reserved for future use");

    Element_Begin1("Image Origination Information");
    Skip_B4(                                                    "X offset");
    Skip_B4(                                                    "Y offset");
    Skip_UTF8  (100,                                            "FileName");
    Get_ASCII  (12,  CreationDate,                              "Creation Date");
    Get_ASCII  (12,  CreationTime,                              "Creation Time");
    Skip_UTF8(64,                                               "Input device");
    Skip_UTF8(32,                                               "Input device model number");
    Skip_UTF8(32,                                               "Input device serial number");
    Skip_BF4(                                                   "X input device pitch");
    Skip_BF4(                                                   "Y input device pitch");
    Skip_BF4(                                                   "Image gamma of capture device");
    Skip_XX(40,                                                 "Reserved for future use");
    Element_End0();

    FILLING_BEGIN();
        //Coherency tests
        if (File_Offset+Buffer_Offset+Size_Total>=Config->File_Current_Size)
            Size_Total=(int32u)(Config->File_Current_Size-(File_Offset+Buffer_Offset)); //The total size is bigger than the real size
        if (Size_Generic+Size_Industry+Size_User>Size_Header || Size_Header>Size_Total)
        {
            Reject();
            return;
        }

        //Filling sizes
        Sizes.push_back(Size_Header);
        Sizes.push_back(Size_Industry);
        Sizes.push_back(Size_User);
        Sizes.push_back(Size_Header-(Size_Generic+Size_Industry+Size_User)); //Size of padding
        Sizes.push_back(Size_Total-Size_Header); //Size of image

        //Filling meta
        if (Frame_Count==0)
        {
            Fill(Stream_General, 0, General_Encoded_Date, DPX_DateTime2Iso(CreationDate+':'+CreationTime));
            Fill(StreamKind_Last, StreamPos_Last, "Encoded_Date", DPX_DateTime2Iso(CreationDate+':'+CreationTime));
            Fill(StreamKind_Last, StreamPos_Last, "Format", "Cineom");
            if (Version.size()>2 && Version[0]=='V' && Version[1]>='0' && Version[2]<='9')
                Version.insert(1, "ersion ");
            Fill(StreamKind_Last, StreamPos_Last, "Format_Version", Version);
            Fill(Stream_General, 0, General_Format_Version, Version);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dpx::GenericSectionHeader_Cineon_ImageElement()
{
    Element_Begin1("image element");
    int32u Width, Height;
    Skip_B1(                                                    "Designator - Byte 0");
    Skip_B1(                                                    "Designator - Byte 1");
    Skip_B1(                                                    "Bits per pixel");
    Skip_B1(                                                    "Unused");
    Get_X4 (Width,                                              "Pixels per line");
    Get_X4 (Height,                                             "Lines per image element");
    Skip_BF4(                                                   "Minimum data value");
    Skip_BF4(                                                   "Minimum quantity represented");
    Skip_BF4(                                                   "Maximum data value");
    Skip_BF4(                                                   "Maximum quantity represented");
    Element_End0();

    FILLING_BEGIN();
        if (Frame_Count==0)
        {
            Fill(StreamKind_Last, StreamPos_Last, "Width", Width);
            Fill(StreamKind_Last, StreamPos_Last, "Height", Height);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dpx::GenericSectionHeader_Dpx()
{
    Element_Name("Generic section header");

    //Parsing
    Element_Begin1("File information");
    std::string Version, CreationDate, Creator, Project, Copyright;
    int32u Size_Header, Size_Total, Size_Generic, Size_Industry, Size_User;
    Skip_String(4,                                              "Magic number");
    Get_X4 (Size_Header,                                        "Offset to image data");
    Get_ASCII (8, Version,                                      "Version number of header format");
    Get_X4 (Size_Total,                                         "Total image file size");
    Skip_B4(                                                    "Ditto Key");
    Get_X4 (Size_Generic,                                       "Generic section header length");
    Get_X4 (Size_Industry,                                      "Industry specific header length");
    Get_X4 (Size_User,                                          "User-defined header length");
    Skip_UTF8  (100,                                            "FileName");
    Get_ASCII  (24,  CreationDate,                              "Creation Date");
    Get_ASCII  (100, Creator,                                   "Creator");
    Get_ASCII  (200, Project,                                   "Project");
    Get_ASCII  (200, Copyright,                                 "Right to use or copyright statement");
    Skip_B4(                                                    "Encryption key");
    Skip_XX(104,                                                "Reserved for future use");
    Element_End0();

    Element_Begin1("Image information");
    int32u Width, Height, PAR_H, PAR_V;
    int16u ImageElements;
    Info_X2(ImageOrientation,                                   "Image orientation");Param_Info1(DPX_Orientation[ImageOrientation>8?8:ImageOrientation]);
    Get_X2 (ImageElements,                                      "Number of image elements");
    if (ImageElements>8)
        ImageElements=8;
    Get_X4 (Width,                                              "Pixels per line");
    Get_X4 (Height,                                             "Lines per image element");
    for(int16u ImageElement=0; ImageElement<ImageElements; ImageElement++)
        GenericSectionHeader_Dpx_ImageElement();
    if (ImageElements!=8)
        Skip_XX((8-ImageElements)*72,                           "Padding");
    Skip_XX(52,                                                 "Reserved for future use");
    Element_End0();

    Element_Begin1("Image source information");
    Skip_B4(                                                    "X Offset");
    Skip_B4(                                                    "Y Offset");
    Skip_BF4(                                                   "X center");
    Skip_BF4(                                                   "Y center");
    Skip_B4(                                                    "X original size");
    Skip_B4(                                                    "Y original size");
    Skip_UTF8(100,                                              "Source image filename");
    Skip_UTF8(24,                                               "Source image date/time");
    Skip_UTF8(32,                                               "Input device name");
    Skip_UTF8(32,                                               "Input device serial number");
    Element_Begin1("Border validity");
    Skip_B2(                                                    "XL border");
    Skip_B2(                                                    "XR border");
    Skip_B2(                                                    "YT border");
    Skip_B2(                                                    "YB border");
    Element_End0();
    Get_X4 (PAR_H,                                              "Pixel ratio : horizontal");
    Get_X4 (PAR_V,                                              "Pixel ratio : vertical");

    Element_Begin1("Additional source image information");
    Skip_BF4(                                                   "X scanned size");
    Skip_BF4(                                                   "Y scanned size");
    Skip_XX(20,                                                 "Reserved for future use");
    Element_End0();

    FILLING_BEGIN();
        //Coherency tests
        if (File_Offset+Buffer_Offset+Size_Total!=Config->File_Current_Size)
            Size_Total=(int32u)(Config->File_Current_Size-(File_Offset+Buffer_Offset)); //The total size is bigger than the real size
        if (Size_Generic==(int32u)-1)
            Size_Generic=(int32u)Element_Size;
        if (Size_Industry==(int32u)-1)
            Size_Industry=0;
        if (Size_User==(int32u)-1)
            Size_User=0;
        if (Size_Generic+Size_Industry+Size_User>Size_Header || Size_Header>Size_Total)
        {
            Reject();
            return;
        }

        //Filling sizes
        Sizes.push_back(Size_Header);
        Sizes.push_back(Size_Industry);
        Sizes.push_back(Size_User);
        Sizes.push_back(Size_Header-(Size_Generic+Size_Industry+Size_User)); //Size of padding
        Sizes.push_back(Size_Total-Size_Header); //Size of image

        //Filling meta
        if (Frame_Count==0)
        {
            Fill(Stream_General, 0, General_Encoded_Date, DPX_DateTime2Iso(CreationDate));
            Fill(StreamKind_Last, StreamPos_Last, "Encoded_Date", DPX_DateTime2Iso(CreationDate));
            Fill(Stream_General, 0, General_Encoded_Library, Creator);
            Fill(StreamKind_Last, StreamPos_Last, "Encoded_Library", Creator);
            Fill(Stream_General, 0, "Project", Project); //ToDo: map to a MediaInfo field (which one?)
            Fill(Stream_General, 0, General_Copyright, Copyright);
            Fill(StreamKind_Last, StreamPos_Last, "Format", "DPX");
            if (Version.size()>2 && Version[0]=='V' && Version[1]>='0' && Version[2]<='9')
                Version.insert(1, "ersion ");
            Fill(StreamKind_Last, StreamPos_Last, "Format_Version", Version);
            Fill(Stream_General, 0, General_Format_Version, Version);

            Fill(StreamKind_Last, StreamPos_Last, "Width", Width);
            Fill(StreamKind_Last, StreamPos_Last, "Height", Height);
            if (PAR_V && PAR_H!=(int32u)-1 && PAR_V!=(int32u)-1)
                Fill(StreamKind_Last, StreamPos_Last, "PixelAspectRatio", ((float)PAR_H)/PAR_V);
            else
                Fill(StreamKind_Last, StreamPos_Last, "PixelAspectRatio", (float)1, 3);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dpx::GenericSectionHeader_Dpx_ImageElement()
{
    Element_Begin1("image element");
    int16u ComponentDataPackingMethod, ComponentDataEncodingMethod;
    int8u Descriptor, TransferCharacteristic, ColorimetricSpecification, BitDephs;
    Info_X4(DataSign,                                           "Data sign");Param_Info1((DataSign==0?"unsigned":"signed"));
    Skip_B4(                                                    "Reference low data code value");
    Skip_BF4(                                                   "Reference low quantity represented");
    Skip_B4(                                                    "Reference high data code value");
    Skip_BF4(                                                   "Reference high quantity represented");
    Get_B1 (Descriptor,                                         "Descriptor");Param_Info1(DPX_Descriptors(Descriptor));
    Get_B1 (TransferCharacteristic,                             "Transfer characteristic");Param_Info1(DPX_TransferCharacteristic(TransferCharacteristic));
    Get_B1 (ColorimetricSpecification,                          "Colorimetric specification");Param_Info1(DPX_ColorimetricSpecification(ColorimetricSpecification));
    Get_B1 (BitDephs,                                           "Bit depth");Param_Info1(DPX_ValidBitDephs(BitDephs));
    Get_X2 (ComponentDataPackingMethod,                         "Packing");Param_Info1((ComponentDataPackingMethod<8?DPX_ComponentDataPackingMethod[ComponentDataPackingMethod]:"invalid"));
    Get_X2 (ComponentDataEncodingMethod,                        "Encoding");Param_Info1((ComponentDataEncodingMethod<8?DPX_ComponentDataEncodingMethod[ComponentDataEncodingMethod]:"invalid"));
    Skip_X4(                                                    "Offset to data");
    Skip_X4(                                                    "End-of-line padding");
    Skip_X4(                                                    "End-of-image padding");
    Skip_UTF8(32,                                               "Description of image element");
    Element_End0();

    FILLING_BEGIN();
        if (Frame_Count==0)
        {
            Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", DPX_Descriptors_ColorSpace(Descriptor));
            Fill(StreamKind_Last, StreamPos_Last, "ChromaSubsampling", DPX_Descriptors_ChromaSubsampling(Descriptor));
            Fill(StreamKind_Last, StreamPos_Last, "BitDepth", BitDephs);
            Fill(StreamKind_Last, StreamPos_Last, "colour_description_present", "Yes");
            Fill(StreamKind_Last, StreamPos_Last, "colour_primaries", DPX_ColorimetricSpecification(ColorimetricSpecification));
            Fill(StreamKind_Last, StreamPos_Last, "transfer_characteristics", DPX_TransferCharacteristic(TransferCharacteristic));
            if (ComponentDataPackingMethod<8)
            {
                Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", DPX_ComponentDataPackingMethod[ComponentDataPackingMethod]);
                Fill(StreamKind_Last, StreamPos_Last, "Format_Settings_Packing", DPX_ComponentDataPackingMethod[ComponentDataPackingMethod]);
            }
            if (ComponentDataEncodingMethod<8)
            {
                Fill(StreamKind_Last, StreamPos_Last, "Format_Compression", DPX_ComponentDataEncodingMethod[ComponentDataEncodingMethod]);
            }
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dpx::IndustrySpecificHeader_Cineon()
{
    Element_Name("Motion picture industry specific header");

    //Parsing
    Element_Begin1("Motion-picture film information");
    Skip_B1(                                                    "?");
    Skip_B1(                                                    "?");
    Skip_B1(                                                    "?");
    Skip_B1(                                                    "?");
    Skip_B4(                                                    "?");
    Skip_B4(                                                    "?");
    Skip_UTF8(32,                                               "?");
    Skip_B4(                                                    "?");
    Skip_B4(                                                    "?");
    Skip_UTF8(32,                                               "?");
    Skip_UTF8(200,                                              "?");
    Skip_XX(740,                                                "Reserved for future use");
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Dpx::IndustrySpecificHeader_Dpx()
{
    Element_Name("Industry specific header");

    //Parsing
    float32 FrameRate;
    Element_Begin1("Motion-picture film information");
    Skip_String(2,                                              "Film mfg. ID code");
    Skip_String(2,                                              "Film type");
    Skip_String(2,                                              "Offset in perfs");
    Skip_String(6,                                              "Prefix");
    Skip_String(4,                                              "Count");
    Skip_String(32,                                             "Format - e.g. Academy");
    Skip_B4(                                                    "Frame position in sequence");
    Skip_B4(                                                    "Sequence length (frames)");
    Skip_B4(                                                    "Held count (1 = default)");
    Get_XF4 (FrameRate,                                         "Frame rate of original (frames/s)");
    Skip_BF4(                                                   "Shutter angle of camera in degrees");
    Skip_UTF8(32,                                               "Frame identification - e.g. keyframe");
    Skip_UTF8(100,                                              "Slate information");
    Skip_XX(56,                                                 "Reserved for future use");
    Element_End0();

    Element_Begin1("Television information");
    Skip_B4(                                                    "SMPTE time code");
    Skip_B4(                                                    "SMPTE user bits");
    Info_B1(Interlace,                                          "Interlace");Param_Info1((Interlace==0?"noninterlaced":"2:1 interlace"));
    Skip_B1(                                                    "Field number");
    Info_B1(VideoSignalStandard,                                "Video signal standard");Param_Info1(DPX_VideoSignalStandard(VideoSignalStandard));
    Skip_B1(                                                    "Zero");
    Skip_BF4(                                                   "Horizontal sampling rate (Hz)");
    Skip_BF4(                                                   "Vertical sampling rate (Hz)");
    Skip_BF4(                                                   "Temporal sampling rate or frame rate (Hz)");
    Skip_BF4(                                                   "Time offset from sync to first pixel (ms)");
    Skip_BF4(                                                   "Gamma");
    Skip_BF4(                                                   "Black level code value");
    Skip_BF4(                                                   "Black gain");
    Skip_BF4(                                                   "Breakpoint");
    Skip_BF4(                                                   "Reference white level code value");
    Skip_BF4(                                                   "Integration time (s)");
    Skip_XX(76,                                                 "Reserved for future use");
    Element_End0();

    FILLING_BEGIN();
        if (FrameRate)
            Fill(StreamKind_Last, StreamPos_Last, "FrameRate", FrameRate);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Dpx::UserDefinedHeader_Cineon()
{
    Element_Name("User defined header");

    //Parsing
    Skip_XX(Sizes[Pos_UserDefined],                             "Unknown");
}

//---------------------------------------------------------------------------
void File_Dpx::UserDefinedHeader_Dpx()
{
    Element_Name("User defined header");

    //Parsing
    if (Sizes[Pos_UserDefined]<32)
    {
        //Not in spec
        Skip_XX(Sizes[Pos_UserDefined],                         "Unknown");
        return;
    }
    Skip_UTF8(32,                                               "User identification");
    Skip_XX(Sizes[Pos_UserDefined]-32,                          "User defined");
}

//---------------------------------------------------------------------------
void File_Dpx::Padding()
{
    Element_Name("Padding");

    //Parsing
    Skip_XX(Sizes[Pos_Padding],                                 "Padding");
}

//---------------------------------------------------------------------------
void File_Dpx::ImageData()
{
    Element_Name("Image Data");

    //Parsing
    Skip_XX(Sizes[Pos_ImageData],                               "Data");

    Frame_Count++;
    if (Frame_Count_NotParsedIncluded!=(int64u)-1)
        Frame_Count_NotParsedIncluded++;

    if (Config->ParseSpeed<1.0)
        Finish("DPX"); //No need of more
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Dpx::Get_X2(int16u &Info, const char* Name)
{
    if (LittleEndian)
        Get_L2 (Info,                                           Name);
    else
        Get_B2 (Info,                                           Name);
}

//---------------------------------------------------------------------------
void File_Dpx::Get_X4(int32u &Info, const char* Name)
{
    if (LittleEndian)
        Get_L4 (Info,                                           Name);
    else
        Get_B4 (Info,                                           Name);
}

//---------------------------------------------------------------------------
void File_Dpx::Get_XF4(float32 &Info, const char* Name)
{
    if (LittleEndian)
        Get_LF4 (Info,                                           Name);
    else
        Get_BF4 (Info,                                           Name);
}

//---------------------------------------------------------------------------
void File_Dpx::Get_ASCII(size_t Size, string &Info, const char* Name)
{
    //Looking for end of string (null byte)
    auto ActualSize=SizeUpTo0(Size);
    auto Element_Offset_Max=Element_Offset+Size;
    Get_String (ActualSize, Info,                               Name);
    auto Buffer_Begin=Buffer+Buffer_Offset;
    while (Element_Offset<Element_Offset_Max && !Buffer_Begin[Element_Offset])
        ++Element_Offset;
    if (Element_Offset_Max>Element_Offset)
        Skip_XX(Element_Offset_Max-Element_Offset,              "(Unknown)");
}

} //NameSpace

#endif //MEDIAINFO_DPX_YES
