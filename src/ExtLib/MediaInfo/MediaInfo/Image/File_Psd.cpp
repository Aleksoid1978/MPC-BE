/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PSD files
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
// From http://www.adobe.com/devnet-apps/photoshop/fileformatashtml/PhotoshopFileFormats.htm
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
#if defined(MEDIAINFO_PSD_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Image/File_Psd.h"
#if defined(MEDIAINFO_IIM_YES)
    #include "MediaInfo/Tag/File_Iim.h"
#endif
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
static const char* Psd_ColorMode(int16u ColorMode)
{
    switch(ColorMode)
    {
        case 0 : return "Bitmap";
        case 1 : return "Grayscale";
        case 2 : return "Indexed";
        case 3 : return "RGB";
        case 4 : return "CMYK";
        case 7 : return "Multichannel";
        case 8 : return "Duotone";
        case 9 : return "Lab";
        default: return "";
    }
}

//---------------------------------------------------------------------------
// Photoshop Tags
//---------------------------------------------------------------------------

#define ELEM(_TAG,_NAME) \
    const int16u _NAME = _TAG; \

struct Psd_tag_desc {
    int16u Tag_ID;
    const char* Description;
};
#define ELEM_TRACE(_NAME,_DESC) \
    {_NAME, _DESC}, \

namespace Elements {
    ELEM(0x03E8, NumberOfChannelsRowsColumnsDepthMode)
    ELEM(0x03E9, MacintoshPrintManagerPrintInfoRecord)
    ELEM(0x03EA, MacintoshPageFormat)
    ELEM(0x03EB, IndexedColorTable)
    ELEM(0x03ED, ResolutionInfo)
    ELEM(0x03EE, NamesOfTheAlphaChannels)
    ELEM(0x03EF, DisplayInfo)
    ELEM(0x03F0, Caption)
    ELEM(0x03F1, Border)
    ELEM(0x03F2, BackgroundColor)
    ELEM(0x03F3, PrintFlags)
    ELEM(0x03F4, GrayscaleAndMultichannelHalftoning)
    ELEM(0x03F5, ColorHalftoning)
    ELEM(0x03F6, DuotoneHalftoning)
    ELEM(0x03F7, GrayscaleAndMultichannelTransferFunction)
    ELEM(0x03F8, ColorTransferFunctions)
    ELEM(0x03F9, DuotoneTransferFunctions)
    ELEM(0x03FA, DuotoneImage)
    ELEM(0x03FB, BlackAndWhiteValuesForTheDotRange)
    ELEM(0x03FD, EPSOptions)
    ELEM(0x03FE, QuickMask)
    ELEM(0x0400, LayerState)
    ELEM(0x0401, WorkingPath)
    ELEM(0x0402, LayersGroup)
    ELEM(0x0404, IPTCNAA)
    ELEM(0x0405, ImageMode)
    ELEM(0x0406, JPEGQuality)
    ELEM(0x0408, GridAndGuides)
    ELEM(0x0409, Thumbnail)
    ELEM(0x040A, CopyrightFlag)
    ELEM(0x040B, URL)
    ELEM(0x040C, Thumbnail_New)
    ELEM(0x040D, GlobalAngle)
    ELEM(0x040E, ColorSamplers)
    ELEM(0x040F, ICCProfile)
    ELEM(0x0410, Watermark)
    ELEM(0x0411, ICCUntaggedProfile)
    ELEM(0x0412, EffectsVisible)
    ELEM(0x0413, SpotHalftone)
    ELEM(0x0414, DocumentSpecificIDsSeedNumber)
    ELEM(0x0415, AlphaNames)
    ELEM(0x0416, IndexedColorTableCount)
    ELEM(0x0417, TransparencyIndex)
    ELEM(0x0419, GlobalAltitude)
    ELEM(0x041A, Slices)
    ELEM(0x041B, WorkflowURL)
    ELEM(0x041C, JumpToXPEP)
    ELEM(0x041D, AlphaIdentifiers)
    ELEM(0x041E, URLList)
    ELEM(0x0421, VersionInfo)
    ELEM(0x0422, EXIFData1)
    ELEM(0x0423, EXIFData3)
    ELEM(0x0424, XMPMetadata)
    ELEM(0x0425, CaptionDigest)
    ELEM(0x0426, PrintScale)
    ELEM(0x0428, PixelAspectRatio)
    ELEM(0x0429, LayerComps)
    ELEM(0x042A, AlternateDuotoneColors)
    ELEM(0x042B, AlternateSpotColors)
    ELEM(0x042D, LayerSelectionIDs)
    ELEM(0x042E, HDRToning)
    ELEM(0x042F, PrintInfo)
    ELEM(0x0430, LayerGroupsEnabledID)
    ELEM(0x0431, ColorSamplers_New)
    ELEM(0x0432, MeasurementScale)
    ELEM(0x0433, Timeline)
    ELEM(0x0434, SheetDisclosure)
    ELEM(0x0435, DisplayInfo_New)
    ELEM(0x0436, OnionSkins)
    ELEM(0x0438, Count)
    ELEM(0x043A, Print)
    ELEM(0x043B, PrintStyle)
    ELEM(0x043C, MacintoshNSPrintInfo)
    ELEM(0x043D, WindowsDEVMODE)
    ELEM(0x043E, AutoSaveFilePath)
    ELEM(0x043F, AutoSaveFormat)
    ELEM(0x0440, PathSelectionState)
    ELEM(0x07D0, Path)
    ELEM(0x0BB7, NameOfClippingPath)
    ELEM(0x0BB8, OriginPathInfo)
    ELEM(0x0FA0, PlugIn)
    ELEM(0x1B58, ImageReadyVariables)
    ELEM(0x1B59, ImageReadyDataSets)
    ELEM(0x1B5A, ImageReadyDefaultSelectedState)
    ELEM(0x1B5B, ImageReady7RolloverExpandedState)
    ELEM(0x1B5C, ImageReadyRolloverExpandedState)
    ELEM(0x1B5D, ImageReadySaveLayerSettings)
    ELEM(0x1B5E, ImageReadyVersion)
    ELEM(0x1F40, LightroomWorkflow)
    ELEM(0x2710, PrintFlags_New)

Psd_tag_desc Desc[] =
{
    ELEM_TRACE(NumberOfChannelsRowsColumnsDepthMode, "Number of channels, rows, columns, depth, and mode")
    ELEM_TRACE(MacintoshPrintManagerPrintInfoRecord, "Macintosh print manager print info record")
    ELEM_TRACE(MacintoshPageFormat, "Macintosh page format")
    ELEM_TRACE(IndexedColorTable, "Indexed color table")
    ELEM_TRACE(ResolutionInfo, "Resolution info")
    ELEM_TRACE(NamesOfTheAlphaChannels, "Names of the alpha channels")
    ELEM_TRACE(DisplayInfo, "Display info")
    ELEM_TRACE(Caption, "Caption")
    ELEM_TRACE(Border, "Border")
    ELEM_TRACE(BackgroundColor, "Background color")
    ELEM_TRACE(PrintFlags, "Print flags")
    ELEM_TRACE(GrayscaleAndMultichannelHalftoning, "Grayscale and multichannel halftoning")
    ELEM_TRACE(ColorHalftoning, "Color halftoning")
    ELEM_TRACE(DuotoneHalftoning, "Duotone halftoning")
    ELEM_TRACE(GrayscaleAndMultichannelTransferFunction, "Grayscale and multichannel transfer function")
    ELEM_TRACE(ColorTransferFunctions, "Color transfer functions")
    ELEM_TRACE(DuotoneTransferFunctions, "Duotone transfer functions")
    ELEM_TRACE(DuotoneImage, "Duotone image")
    ELEM_TRACE(BlackAndWhiteValuesForTheDotRange, "Black and white values for the dot range")
    ELEM_TRACE(EPSOptions, "EPS options")
    ELEM_TRACE(QuickMask, "Quick Mask")
    ELEM_TRACE(LayerState, "Layer state")
    ELEM_TRACE(WorkingPath, "Working path")
    ELEM_TRACE(LayersGroup, "Layers group")
    ELEM_TRACE(IPTCNAA, "IPTC-NAA")
    ELEM_TRACE(ImageMode, "Image mode")
    ELEM_TRACE(JPEGQuality, "JPEG quality")
    ELEM_TRACE(GridAndGuides, "Grid and guides")
    ELEM_TRACE(Thumbnail, "Thumbnail")
    ELEM_TRACE(CopyrightFlag, "Copyright flag")
    ELEM_TRACE(URL, "URL")
    ELEM_TRACE(Thumbnail_New, "Thumbnail (new)")
    ELEM_TRACE(GlobalAngle, "Global Angle")
    ELEM_TRACE(ColorSamplers, "Color samplers")
    ELEM_TRACE(ICCProfile, "ICC Profile")
    ELEM_TRACE(Watermark, "Watermark")
    ELEM_TRACE(ICCUntaggedProfile, "ICC Untagged Profile")
    ELEM_TRACE(EffectsVisible, "Effects visible")
    ELEM_TRACE(SpotHalftone, "Spot Halftone")
    ELEM_TRACE(DocumentSpecificIDsSeedNumber, "Document-specific IDs seed number")
    ELEM_TRACE(AlphaNames, "Alpha Names")
    ELEM_TRACE(IndexedColorTableCount, "Indexed color table count")
    ELEM_TRACE(TransparencyIndex, "Transparency Index")
    ELEM_TRACE(GlobalAltitude, "Global altitude")
    ELEM_TRACE(Slices, "Slices")
    ELEM_TRACE(WorkflowURL, "Workflow URL")
    ELEM_TRACE(JumpToXPEP, "Jump To XPEP")
    ELEM_TRACE(AlphaIdentifiers, "Alpha Identifiers")
    ELEM_TRACE(URLList, "URL List")
    ELEM_TRACE(VersionInfo, "Version Info")
    ELEM_TRACE(EXIFData1, "Exif data 1")
    ELEM_TRACE(EXIFData3, "Exif data 3")
    ELEM_TRACE(XMPMetadata, "XMP metadata")
    ELEM_TRACE(CaptionDigest, "Caption digest")
    ELEM_TRACE(PrintScale, "Print scale")
    ELEM_TRACE(PixelAspectRatio, "Pixel aspect ratio")
    ELEM_TRACE(LayerComps, "Layer comps")
    ELEM_TRACE(AlternateDuotoneColors, "Alternate Duotone colors")
    ELEM_TRACE(AlternateSpotColors, "Alternate Spot colors")
    ELEM_TRACE(LayerSelectionIDs, "Layer selection IDs")
    ELEM_TRACE(HDRToning, "HDR toning")
    ELEM_TRACE(PrintInfo, "Print info")
    ELEM_TRACE(LayerGroupsEnabledID, "Layer groups enabled ID")
    ELEM_TRACE(ColorSamplers_New, "Color samplers (new)")
    ELEM_TRACE(MeasurementScale, "Measurement scale")
    ELEM_TRACE(Timeline, "Timeline")
    ELEM_TRACE(SheetDisclosure, "Sheet disclosure")
    ELEM_TRACE(DisplayInfo_New, "Display info (new)")
    ELEM_TRACE(OnionSkins, "Onion skins")
    ELEM_TRACE(Count, "Count")
    ELEM_TRACE(Print, "Print")
    ELEM_TRACE(PrintStyle, "Print style")
    ELEM_TRACE(MacintoshNSPrintInfo, "Macintosh NSPrintInfo")
    ELEM_TRACE(WindowsDEVMODE, "Windows DEVMODE")
    ELEM_TRACE(AutoSaveFilePath, "Auto save file path")
    ELEM_TRACE(AutoSaveFormat, "Auto save format")
    ELEM_TRACE(PathSelectionState, "Path selection state")
    ELEM_TRACE(Path, "Path")
    ELEM_TRACE(NameOfClippingPath, "Name of clipping path")
    ELEM_TRACE(OriginPathInfo, "Origin path info")
    ELEM_TRACE(PlugIn, "Plug-in")
    ELEM_TRACE(ImageReadyVariables, "Image Ready variables")
    ELEM_TRACE(ImageReadyDataSets, "Image Ready data sets")
    ELEM_TRACE(ImageReadyDefaultSelectedState, "Image Ready default selected state")
    ELEM_TRACE(ImageReady7RolloverExpandedState, "Image Ready 7 rollover expanded state")
    ELEM_TRACE(ImageReadyRolloverExpandedState, "Image Ready rollover expanded state")
    ELEM_TRACE(ImageReadySaveLayerSettings, "Image Ready save layer settings")
    ELEM_TRACE(ImageReadyVersion, "Image Ready version")
    ELEM_TRACE(LightroomWorkflow, "Lightroom workflow")
    ELEM_TRACE(PrintFlags_New, "Print flags (new)")
};
};

const auto Elements_Size = sizeof(Elements::Desc) / sizeof(*Elements::Desc);

static string Psd_Tag_Description(int16u Tag_ID)
{
#ifdef MEDIAINFO_TRACE
    int16u Tag_ID2;
    if (Tag_ID > 0x07D0 && Tag_ID <= 0x0BB6) {
        Tag_ID2 = 0x07D0;
    }
    else if (Tag_ID > 0x0FA0 && Tag_ID <= 0x1387) {
        Tag_ID2 = 0x0FA0;
    }
    else {
        Tag_ID2 = Tag_ID;
    }
    for (size_t Pos = 0; Pos < Elements_Size; ++Pos)
    {
        if (Elements::Desc[Pos].Tag_ID == Tag_ID2)
            return Elements::Desc[Pos].Description;
    }
#endif //MEDIAINFO_TRACE
    return Ztring::ToZtring_From_CC2(Tag_ID).To_UTF8();
}

//***************************************************************************
// File Header Information
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Psd::FileHeader_Begin()
{
    if (Step == Step_ImageResourcesBlock) {
        return true;
    }

    if (Buffer_Size < 6)
        return false; //Must wait for more data

    auto Version = CC2(Buffer + 4);
    if (CC4(Buffer)!=0x38425053  || Version < 1 || Version > 2) //"8BPS"
    {
        Reject();
        return false;
    }

    //All should be OK...
    return true;
}

//---------------------------------------------------------------------------
void File_Psd::FileHeader_Parse()
{
    if (Step == Step_ImageResourcesBlock) {
        return;
    }

    //Parsing
    int32u Width, Height;
    int16u BitsDepth, Version, channels, ColorMode;
    Skip_C4(                                                    "Signature");
    Get_B2 (Version,                                            "Version"); //  1 = PSD, 2 = PSB
    Skip_B6(                                                    "Reserved");
    Get_B2 (channels,                                           "channels"); // 1 to 56, including alpha channel
    Get_B4 (Height,                                             "Height");
    Get_B4 (Width,                                              "Width");
    Get_B2 (BitsDepth,                                          "Depth"); // 1,8,16 or 32
    Get_B2 (ColorMode,                                          "Color Mode"); Param_Info1(Psd_ColorMode(ColorMode));

    FILLING_BEGIN();
        Accept("PSD");
        Stream_Prepare(Stream_Image);
        Fill(Stream_Image, 0, Image_Format, Version==1?"PSD":"PSB");
        Fill(Stream_Image, 0, Image_Format_Version, Version);
        Fill(Stream_Image, 0, Image_ColorSpace, Psd_ColorMode(ColorMode));
        Fill(Stream_Image, 0, Image_Width, Width);
        Fill(Stream_Image, 0, Image_Height, Height);
        Fill(Stream_Image, 0, Image_BitDepth, BitsDepth);
        Step = Step_ColorModeData;
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Psd::Header_Parse()
{
    if (!IsSub && Element_Level == 2 && Step == Step_ImageResourcesBlock) {
        Step = Step_LayerAndMaskInformation;
        DataMustAlwaysBeComplete = false;
    }

    switch (Step) {
    case Step_ColorModeData:
    case Step_ImageResources:
    case Step_LayerAndMaskInformation: {
        int32u Size;
        Get_B4 (Size,                                       "Size");
        Header_Fill_Size(Element_Offset + Size);
        Element_ThisIsAList();
        break;
    }
    case Step_ImageResourcesBlock: {
        int32u Signature;
        Get_C4 (Signature,                                  "Signature");
        switch (Signature) {
        case 0x3842494D: { // 8BIM
            int32u Size;
            int16u ID;
            Get_B2 (ID,                                     "Resource ID");
            Skip_PA(                                        "Name");
            if (Element_Offset % 2) {
                Skip_B1(                                    "Padding");
            }
            Get_B4 (Size,                                   "Size");
            if (Size % 2) {
                Size++;
            }
            Header_Fill_Size(Element_Offset + Size);
            Header_Fill_Code(ID, Psd_Tag_Description(ID).c_str());
            break;
        }
        default: Finish();
        }
        break;
    }
    case Step_ImageData: {
        Header_Fill_Size(File_Size - (File_Offset + Buffer_Offset));
        Element_ThisIsAList();
        break;
    }
    default: Finish();
    }
}

//---------------------------------------------------------------------------
void File_Psd::Data_Parse()
{
    if (Alignment_ExtraByte <= Element_Size)
        Element_Size -= Alignment_ExtraByte;

    switch (Step) {
    case Step_ColorModeData: ColorModeData(); break;
    case Step_ImageResources: ImageResources(); break;
    case Step_ImageResourcesBlock: ImageResourcesBlock(); break;
    case Step_LayerAndMaskInformation: LayerAndMaskInformation(); break;
    case Step_ImageData: ImageData(); break;
    default: Finish();
    }

    if (Alignment_ExtraByte)
    {
        Element_Size += Alignment_ExtraByte;
        if (Element_Offset + Alignment_ExtraByte == Element_Size)
            Skip_XX(Alignment_ExtraByte,                        "Alignment");
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Psd::ColorModeData()
{
    Element_Name("Color mode data");
    Skip_XX(Element_Size,                                       "(Data)");
    Step = Step_ImageResources;
}

//---------------------------------------------------------------------------
void File_Psd::ImageResources()
{
    Element_Name("Image resources");
    Step = Step_ImageResourcesBlock;
    Element_ThisIsAList();
}

//---------------------------------------------------------------------------
void File_Psd::ImageResourcesBlock()
{
    #define ELEMENT_CASE(NAME) \
        case Elements::NAME: NAME(); break;

    switch (Element_Code) {
    ELEMENT_CASE(CaptionDigest);
    ELEMENT_CASE(CopyrightFlag);
    ELEMENT_CASE(IPTCNAA);
    ELEMENT_CASE(JPEGQuality);
    ELEMENT_CASE(Thumbnail);
    ELEMENT_CASE(Thumbnail_New);
    ELEMENT_CASE(URL);
    ELEMENT_CASE(VersionInfo);
    default: Skip_XX(Element_Size,                              "(Data)");
    }

}

//---------------------------------------------------------------------------
void File_Psd::LayerAndMaskInformation()
{
    Element_Name("Layer and mask information");
    Skip_XX(Element_Size,                                       "(Data)");
    Step = Step_ImageData;
}

//---------------------------------------------------------------------------
void File_Psd::ImageData()
{
    Element_Name("Image data");
    Skip_XX(Element_Size,                                       "(Data)");
    Finish();
}

//---------------------------------------------------------------------------
void File_Psd::CopyrightFlag()
{
    Skip_B1(                                                    "Copyright Flag");
}

//---------------------------------------------------------------------------
void File_Psd::CaptionDigest()
{
    Skip_Hexa(16,                                               "Digest");
}

//---------------------------------------------------------------------------
void File_Psd::IPTCNAA()
{
    //Parsing
    #if defined(MEDIAINFO_IIM_YES)
    File_Iim MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0, false);
    Merge(MI, Stream_Image, 0, 0, false);
    size_t Count = MI.Count_Get(Stream_Image);
    for (size_t i = 1; i < Count; ++i) {
        Merge(MI, Stream_Image, i, StreamPos_Last + 1, false);
    }
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "(Data)");
    #endif
}

//---------------------------------------------------------------------------
void File_Psd::JPEGQuality()
{
    int16u quality, format;
    Get_B2(quality,                                             "Quality"); Param_Info1(static_cast<int16s>(quality) + 4);
    Get_B2(format,                                              "Format"); Param_Info1(format == 0x0000 ? "Standard" : format == 0x0001 ? "Optimized" : format == 0x0101 ? "Progressive" : "");
    Skip_XX(Element_Size - Element_Offset,                      "(Unknown)");
}

//---------------------------------------------------------------------------
void File_Psd::Thumbnail()
{
    Skip_B4(                                                    "Format");
    Skip_B4(                                                    "Width");
    Skip_B4(                                                    "Height");
    Skip_B4(                                                    "Widthbytes");
    Skip_B4(                                                    "Total size");
    Skip_B4(                                                    "Size after compression");
    Skip_B2(                                                    "Bits per pixel");
    Skip_B2(                                                    "Number of planes");
    if (!Count_Get(Stream_General)) Stream_Prepare(Stream_General);
    if (!Count_Get(Stream_Image)) Stream_Prepare(Stream_Image);
    Attachment(IsSub?"PSD":nullptr, {}, "Thumbnail");
}

//---------------------------------------------------------------------------
void File_Psd::URL()
{
    string url;
    Get_String(Element_Size, url,                               "URL");
    FILLING_BEGIN_PRECISE()
        Fill(Stream_General, 0, "URL", url);
    FILLING_END()
}

//---------------------------------------------------------------------------
void File_Psd::VersionInfo()
{
    int32u size;
    Skip_B4     (                                               "version");
    Skip_B1     (                                               "hasRealMergedData");
    Get_B4      (size,                                          "writer name length");
    Skip_UTF16B (static_cast<int64u>(size)*2,                   "writer name");
    Get_B4      (size,                                          "reader name length");
    Skip_UTF16B (static_cast<int64u>(size)*2,                   "reader name");
    Skip_B4     (                                               "file version");
}

} //NameSpace

#endif
