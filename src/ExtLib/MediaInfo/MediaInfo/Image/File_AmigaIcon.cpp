/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Amiga Icon - Format
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Amiga .info (Workbench icon) files
// Supports Classic, NewIcon, GlowIcon/ColorIcon, and ARGB image layers
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
#if defined(MEDIAINFO_AMIGAICON_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_AmigaIcon.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* AmigaIcon_Type(int8u Type)
{
    switch (Type)
    {
        case  1 : return "Disk";
        case  2 : return "Drawer";
        case  3 : return "Tool";
        case  4 : return "Project";
        case  5 : return "Garbage";
        case  6 : return "Kick";
        case  8 : return "AppIcon";
        default : return "";
    }
}

//***************************************************************************
// Static stuff
//***************************************************************************

//---------------------------------------------------------------------------
bool File_AmigaIcon::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<4)
        return false; //Must wait for more data

    if (CC2(Buffer)!=0xE310)
    {
        Reject("Amiga Icon");
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_AmigaIcon::Read_Buffer_Continue()
{
    //Parsing
    int32u GadgetRender, SelectRender, UserData;
    int32u HasDefaultTool, HasToolTypes, HasDrawerData, HasToolWindow;
    int16u Width, Height;
    int8u  IconType;

    Element_Begin1("DiskObject header");
        Skip_B2(                                                    "Magic");
        Skip_B2(                                                    "Version");

        Element_Begin1("Gadget");
            Skip_B4(                                                "ga_Next");
            Skip_B2(                                                "ga_LeftEdge");
            Skip_B2(                                                "ga_TopEdge");
            Get_B2 (Width,                                          "ga_Width");
            Get_B2 (Height,                                         "ga_Height");
            Skip_B2(                                                "ga_Flags");
            Skip_B2(                                                "ga_Activation");
            Skip_B2(                                                "ga_GadgetType");
            Get_B4 (GadgetRender,                                   "ga_GadgetRender");
            Get_B4 (SelectRender,                                   "ga_SelectRender");
            Skip_B4(                                                "ga_GadgetText");
            Skip_B4(                                                "ga_MutualExclude");
            Skip_B4(                                                "ga_SpecialInfo");
            Skip_B2(                                                "ga_GadgetID");
            Get_B4 (UserData,                                       "ga_UserData");
        Element_End0();

        Get_B1 (IconType,                                           "do_Type");
        Skip_B1(                                                    "do_Pad");
        Get_B4 (HasDefaultTool,                                     "do_DefaultTool");
        Get_B4 (HasToolTypes,                                       "do_ToolTypes");
        Skip_B4(                                                    "do_CurrentX");
        Skip_B4(                                                    "do_CurrentY");
        Get_B4 (HasDrawerData,                                      "do_DrawerData");
        Get_B4 (HasToolWindow,                                      "do_ToolWindow");
        Skip_B4(                                                    "do_StackSize");
    Element_End0();

    FILLING_BEGIN();
        Accept("Amiga Icon");

        Fill(Stream_General, 0, General_Format, "Amiga Icon");
        Fill(Stream_General, 0, General_Format_Profile, AmigaIcon_Type(IconType));
    FILLING_END();

    //DrawerData
    if (HasDrawerData)
    {
        if (56<=Element_Size-Element_Offset)
        {
            Element_Begin1("DrawerData");
                Skip_XX(56,                                         "DrawerData");
            Element_End0();
        }
    }

    //Classic image (normal)
    int16u ClassicWidth=0, ClassicHeight=0, ClassicDepth=0;
    if (GadgetRender && 20<=Element_Size-Element_Offset)
    {
        int16u ImgWidth, ImgHeight, ImgDepth;
        Element_Begin1("Classic image");
            Skip_B2(                                                "im_LeftEdge");
            Skip_B2(                                                "im_TopEdge");
            Get_B2 (ImgWidth,                                       "im_Width");
            Get_B2 (ImgHeight,                                      "im_Height");
            Get_B2 (ImgDepth,                                       "im_Depth");
            Skip_B4(                                                "im_ImageData");
            Skip_B1(                                                "im_PlanePick");
            Skip_B1(                                                "im_PlaneOnOff");
            Skip_B4(                                                "im_Next");

            if (ImgWidth>0 && ImgHeight>0 && ImgDepth>0 && ImgDepth<=8)
            {
                int64u PlaneDataSize=((int64u)((ImgWidth+15)/16)*2)*ImgHeight*ImgDepth;
                if (PlaneDataSize<=Element_Size-Element_Offset)
                    Skip_XX(PlaneDataSize,                          "Plane data");
            }
        Element_End0();

        ClassicWidth=ImgWidth;
        ClassicHeight=ImgHeight;
        ClassicDepth=ImgDepth;
    }

    //Classic image (selected)
    if (SelectRender && 20<=Element_Size-Element_Offset)
    {
        int16u ImgWidth, ImgHeight, ImgDepth;
        Element_Begin1("Classic image (selected)");
            Skip_B2(                                                "im_LeftEdge");
            Skip_B2(                                                "im_TopEdge");
            Get_B2 (ImgWidth,                                       "im_Width");
            Get_B2 (ImgHeight,                                      "im_Height");
            Get_B2 (ImgDepth,                                       "im_Depth");
            Skip_B4(                                                "im_ImageData");
            Skip_B1(                                                "im_PlanePick");
            Skip_B1(                                                "im_PlaneOnOff");
            Skip_B4(                                                "im_Next");

            if (ImgWidth>0 && ImgHeight>0 && ImgDepth>0 && ImgDepth<=8)
            {
                int64u PlaneDataSize=((int64u)((ImgWidth+15)/16)*2)*ImgHeight*ImgDepth;
                if (PlaneDataSize<=Element_Size-Element_Offset)
                    Skip_XX(PlaneDataSize,                          "Plane data");
            }
        Element_End0();
    }

    //DefaultTool
    if (HasDefaultTool && 4<=Element_Size-Element_Offset)
    {
        int32u Length;
        Element_Begin1("DefaultTool");
            Get_B4 (Length,                                         "Length");
            if (Length<=Element_Size-Element_Offset)
                Skip_XX(Length,                                     "Text");
            else
                Skip_XX(Element_Size-Element_Offset,               "(Problem)");
        Element_End0();
    }

    //ToolTypes
    bool HasNewIcon=false;
    int16u NewIconWidth=0, NewIconHeight=0;
    if (HasToolTypes && 4<=Element_Size-Element_Offset)
    {
        int32u CountField;
        Element_Begin1("ToolTypes");
            Get_B4 (CountField,                                     "Count");

            if (CountField>=8)
            {
                int32u NumEntries=CountField/4-1;
                if (NumEntries>(int32u)((Element_Size-Element_Offset)/4))
                    NumEntries=(int32u)((Element_Size-Element_Offset)/4);
                for (int32u i=0; i<NumEntries; i++)
                {
                    if (4>Element_Size-Element_Offset)
                        break;

                    int32u Length;
                    Get_B4 (Length,                                  "Length");

                    //Check for IM1= prefix to detect NewIcon
                    if (!HasNewIcon && Length>=5 && 4<=Element_Size-Element_Offset)
                    {
                        if (Buffer[(size_t)Element_Offset]=='I' && Buffer[(size_t)Element_Offset+1]=='M' && Buffer[(size_t)Element_Offset+2]=='1' && Buffer[(size_t)Element_Offset+3]=='=')
                        {
                            HasNewIcon=true;
                            //Parse NewIcon header: transparency(1) + width(1) + height(1) + colors_hi(1) + colors_lo(1)
                            if (Length>=9 && 9<=Element_Size-Element_Offset
                             && Buffer[(size_t)Element_Offset+5]>=0x21
                             && Buffer[(size_t)Element_Offset+6]>=0x21)
                            {
                                NewIconWidth=Buffer[(size_t)Element_Offset+5]-0x21;
                                NewIconHeight=Buffer[(size_t)Element_Offset+6]-0x21;
                            }
                        }
                    }

                    if (Length>Element_Size-Element_Offset)
                        break;
                    Skip_XX(Length,                                  "ToolType");
                }
            }
        Element_End0();
    }

    //ToolWindow
    if (HasToolWindow && 4<=Element_Size-Element_Offset)
    {
        int32u Length;
        Element_Begin1("ToolWindow");
            Get_B4 (Length,                                         "Length");
            if (Length<=Element_Size-Element_Offset)
                Skip_XX(Length,                                     "Text");
            else
                Skip_XX(Element_Size-Element_Offset,               "(Problem)");
        Element_End0();
    }

    //DrawerData2
    if (HasDrawerData && (UserData&0xFF))
    {
        if (6<=Element_Size-Element_Offset)
        {
            Element_Begin1("DrawerData2");
                Skip_B4(                                            "dd_Flags");
                Skip_B2(                                            "dd_ViewModes");
            Element_End0();
        }
    }

    //Fill Classic image stream
    if (GadgetRender && ClassicWidth && ClassicHeight)
    {
        Stream_Prepare(Stream_Image);
        Fill(Stream_Image, StreamPos_Last, Image_Width, ClassicWidth);
        Fill(Stream_Image, StreamPos_Last, Image_Height, ClassicHeight);
        Fill(Stream_Image, StreamPos_Last, Image_BitDepth, ClassicDepth);
        Fill(Stream_Image, StreamPos_Last, Image_Format, "Raw");
        Fill(Stream_Image, StreamPos_Last, Image_Format_Profile, "Classic");
        Fill(Stream_Image, StreamPos_Last, Image_ColorSpace, "RGB");
    }

    //Fill NewIcon stream
    if (HasNewIcon && NewIconWidth && NewIconHeight)
    {
        Stream_Prepare(Stream_Image);
        Fill(Stream_Image, StreamPos_Last, Image_Width, NewIconWidth);
        Fill(Stream_Image, StreamPos_Last, Image_Height, NewIconHeight);
        Fill(Stream_Image, StreamPos_Last, Image_Format, "Raw");
        Fill(Stream_Image, StreamPos_Last, Image_Format_Profile, "NewIcon");
        Fill(Stream_Image, StreamPos_Last, Image_ColorSpace, "RGB");
    }

    //FORM ICON (GlowIcons / ARGB)
    if (12<=Element_Size-Element_Offset)
    {
        //Search for "FORM" + "ICON"
        int64u SearchPos=Element_Offset;
        while (12<=Element_Size-SearchPos)
        {
            if (Buffer[(size_t)SearchPos]=='F' && Buffer[(size_t)SearchPos+1]=='O' && Buffer[(size_t)SearchPos+2]=='R' && Buffer[(size_t)SearchPos+3]=='M'
             && Buffer[(size_t)SearchPos+8]=='I' && Buffer[(size_t)SearchPos+9]=='C' && Buffer[(size_t)SearchPos+10]=='O' && Buffer[(size_t)SearchPos+11]=='N')
            {
                //Found FORM ICON
                if (SearchPos>Element_Offset)
                    Skip_XX(SearchPos-Element_Offset,               "Padding");

                int32u FormSize;
                Element_Begin1("FORM ICON");
                    Skip_B4(                                        "FORM");
                    Get_B4 (FormSize,                               "Size");
                    Skip_B4(                                        "ICON");

                    int16u FaceWidth=0, FaceHeight=0;
                    bool HasIMAG=false, HasARGB=false;
                    int8u  IMAGDepth=0, IMAGFormat=0;

                    int64u FormEnd=Element_Offset+(FormSize>=4?FormSize-4:0);
                    if (FormEnd>Element_Size)
                        FormEnd=Element_Size;
                    while (Element_Offset<=FormEnd && 8<=FormEnd-Element_Offset && 8<=Element_Size-Element_Offset)
                    {
                        int32u ChunkSize;
                        int32u ChunkName;
                        Peek_B4(ChunkName);
                        Skip_B4(                                    "Chunk name");
                        Get_B4 (ChunkSize,                          "Chunk size");

                        if (ChunkName==0x46414345) //"FACE"
                        {
                            if (ChunkSize>=4 && 4<=Element_Size-Element_Offset)
                            {
                                FaceWidth=Buffer[(size_t)Element_Offset]+1;
                                FaceHeight=Buffer[(size_t)Element_Offset+1]+1;
                            }
                        }
                        else if (ChunkName==0x494D4147) //"IMAG"
                        {
                            if (!HasIMAG && ChunkSize>=10 && 6<=Element_Size-Element_Offset)
                            {
                                IMAGFormat=Buffer[(size_t)Element_Offset+3];
                                IMAGDepth=Buffer[(size_t)Element_Offset+5];
                                HasIMAG=true;
                            }
                        }
                        else if (ChunkName==0x41524742) //"ARGB"
                        {
                            HasARGB=true;
                        }

                        //Skip chunk data (padded to even)
                        if (ChunkSize==0)
                            break;
                        int32u SkipSize=ChunkSize;
                        if (SkipSize%2)
                        {
                            if (SkipSize==0xFFFFFFFF)
                                break;
                            SkipSize++;
                        }
                        if (SkipSize<=Element_Size-Element_Offset)
                            Skip_XX(SkipSize,                       "Chunk data");
                        else
                            break;
                    }

                Element_End0();

                //Fill GlowIcon stream
                if (HasIMAG && FaceWidth && FaceHeight)
                {
                    Stream_Prepare(Stream_Image);
                    Fill(Stream_Image, StreamPos_Last, Image_Width, FaceWidth);
                    Fill(Stream_Image, StreamPos_Last, Image_Height, FaceHeight);
                    Fill(Stream_Image, StreamPos_Last, Image_BitDepth, IMAGDepth);
                    Fill(Stream_Image, StreamPos_Last, Image_Format, IMAGFormat==1?"RLE":"Raw");
                    Fill(Stream_Image, StreamPos_Last, Image_Format_Profile, "GlowIcon");
                    Fill(Stream_Image, StreamPos_Last, Image_ColorSpace, "RGB");
                }

                //Fill ARGB stream
                if (HasARGB && FaceWidth && FaceHeight)
                {
                    Stream_Prepare(Stream_Image);
                    Fill(Stream_Image, StreamPos_Last, Image_Width, FaceWidth);
                    Fill(Stream_Image, StreamPos_Last, Image_Height, FaceHeight);
                    Fill(Stream_Image, StreamPos_Last, Image_BitDepth, 32);
                    Fill(Stream_Image, StreamPos_Last, Image_Format, "Raw");
                    Fill(Stream_Image, StreamPos_Last, Image_Format_Profile, "ARGB");
                    Fill(Stream_Image, StreamPos_Last, Image_ColorSpace, "RGBA");
                }

                break;
            }
            SearchPos++;
        }
    }

    //Skip any remaining data
    if (Element_Offset<Element_Size)
        Skip_XX(Element_Size-Element_Offset,                        "Unknown");

    //No need of more
    Finish("Amiga Icon");
}

//***************************************************************************
//
//***************************************************************************

} //NameSpace

#endif
