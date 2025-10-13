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
#if defined(MEDIAINFO_EXIF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Exif.h"
#include "MediaInfo/File__MultipleParsing.h"
#if defined(MEDIAINFO_JPEG_YES)
    #include "MediaInfo/Image/File_Jpeg.h"
#endif
#if defined(MEDIAINFO_ICC_YES)
    #include "MediaInfo/Tag/File_Icc.h"
#endif
#if defined(MEDIAINFO_XMP_YES)
    #include "MediaInfo/Tag/File_Xmp.h"
#endif
#if defined(MEDIAINFO_PSD_YES)
    #include "MediaInfo/Image/File_Psd.h"
#endif
#if defined(MEDIAINFO_IIM_YES)
    #include "MediaInfo/Tag/File_Iim.h"
#endif
#include <cmath>
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

const char* Tiff_Compression_Name(int16u compression);

//---------------------------------------------------------------------------
// EXIF Data Types
// 
// All data types are from TIFF 6.0 specification except IFD and UTF-8 which
// are defined in TIFF Specification Supplement 1 and independently by
// EXIF 3.0 specification respectively.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace Exif_Type {
    const int16u BYTE       = 1;
    const int16u ASCII      = 2;
    const int16u SHORT      = 3;
    const int16u LONG       = 4;
    const int16u RATIONAL   = 5;
    const int16u SBYTE      = 6;
    const int16u UNDEFINED  = 7;
    const int16u SSHORT     = 8;
    const int16u SLONG      = 9;
    const int16u SRATIONAL  = 10;
    const int16u FLOAT      = 11;
    const int16u DOUBLE     = 12;
    const int16u IFD        = 13;
    const int16u UTF8       = 129;
}

//---------------------------------------------------------------------------
static const char* Exif_Type_Name(int16u Type)
{
    switch (Type)
    {
    case Exif_Type::BYTE        : return "BYTE";
    case Exif_Type::ASCII       : return "ASCII";
    case Exif_Type::SHORT       : return "SHORT";
    case Exif_Type::LONG        : return "LONG";
    case Exif_Type::RATIONAL    : return "RATIONAL";
    case Exif_Type::SBYTE       : return "SBYTE";
    case Exif_Type::UNDEFINED   : return "UNDEFINED";
    case Exif_Type::SSHORT      : return "SSHORT";
    case Exif_Type::SLONG       : return "SLONG";
    case Exif_Type::SRATIONAL   : return "SRATIONAL";
    case Exif_Type::FLOAT       : return "FLOAT";
    case Exif_Type::DOUBLE      : return "DOUBLE";
    case Exif_Type::IFD         : return "IFD";
    case Exif_Type::UTF8        : return "UTF-8";
    default                     : return "";
    }
}

//---------------------------------------------------------------------------
static const int8u Exif_Type_Size(int16u Type)
{
    switch (Type)
    {
    case Exif_Type::BYTE        : return 1;
    case Exif_Type::ASCII       : return 1;
    case Exif_Type::SHORT       : return 2;
    case Exif_Type::LONG        : return 4;
    case Exif_Type::RATIONAL    : return 8;
    case Exif_Type::SBYTE       : return 1;
    case Exif_Type::UNDEFINED   : return 1;
    case Exif_Type::SSHORT      : return 2;
    case Exif_Type::SLONG       : return 4;
    case Exif_Type::SRATIONAL   : return 8;
    case Exif_Type::FLOAT       : return 4;
    case Exif_Type::DOUBLE      : return 8;
    case Exif_Type::IFD         : return 4;
    case Exif_Type::UTF8        : return 1;
    default                     : return 0;
    }
}

//---------------------------------------------------------------------------
// Tags
//---------------------------------------------------------------------------

#define ELEM(_TAG,_NAME) \
    const int16u _NAME = _TAG; \

struct exif_tag_desc {
    int16u Tag_ID;
    const char* Description;
};
#define ELEM_TRACE(_NAME,_DESC) \
    {_NAME, _DESC}, \

//---------------------------------------------------------------------------
// EXIF Tags
//---------------------------------------------------------------------------

namespace IFD0 {
    ELEM(0x000B, ProcessingSoftware)
    ELEM(0x00FE, SubfileType)
    ELEM(0x00FF, OldSubfileType)
    ELEM(0x0100, ImageWidth)
    ELEM(0x0101, ImageHeight)
    ELEM(0x0102, BitsPerSample)
    ELEM(0x0103, Compression)
    ELEM(0x0106, PhotometricInterpretation)
    ELEM(0x0107, Thresholding)
    ELEM(0x0108, CellWidth)
    ELEM(0x0109, CellLength)
    ELEM(0x010A, FillOrder)
    ELEM(0x010D, DocumentName)
    ELEM(0x010E, ImageDescription)
    ELEM(0x010F, Make)
    ELEM(0x0110, Model)
    ELEM(0x0111, StripOffsets)
    ELEM(0x0112, Orientation)
    ELEM(0x0115, SamplesPerPixel)
    ELEM(0x0116, RowsPerStrip)
    ELEM(0x0117, StripByteCounts)
    ELEM(0x0118, MinSampleValue)
    ELEM(0x0119, MaxSampleValue)
    ELEM(0x011A, XResolution)
    ELEM(0x011B, YResolution)
    ELEM(0x011C, PlanarConfiguration)
    ELEM(0x011D, PageName)
    ELEM(0x011E, XPosition)
    ELEM(0x011F, YPosition)
    ELEM(0x0122, GrayResponseUnit)
    ELEM(0x0128, ResolutionUnit)
    ELEM(0x0129, PageNumber)
    ELEM(0x012D, TransferFunction)
    ELEM(0x0131, Software)
    ELEM(0x0132, DateTime)
    ELEM(0x013B, Artist)
    ELEM(0x013C, HostComputer)
    ELEM(0x013D, Predictor)
    ELEM(0x013E, WhitePoint)
    ELEM(0x013F, PrimaryChromaticities)
    ELEM(0x0141, HalftoneHints)
    ELEM(0x0142, TileWidth)
    ELEM(0x0143, TileLength)
    ELEM(0x014A, SubIFDs) // TIFF/EP
    ELEM(0x014C, InkSet)
    ELEM(0x0151, TargetPrinter)
    ELEM(0x0152, ExtraSamples)
    ELEM(0x0153, SampleFormat)
    ELEM(0x015B, JPEGTables) // TIFF/EP / Adobe Photoshop TIFF Technical Notes
    ELEM(0x0200, JPEGProc) // 0x0200 - 0x0209 Defined TIFF 6.0 Section 22
    ELEM(0x0201, JPEGInterchangeFormat)
    ELEM(0x0202, JPEGInterchangeFormatLength)
    ELEM(0x0203, JPEGRestartInterval)
    ELEM(0x0205, JPEGLosslessPredictors)
    ELEM(0x0206, JPEGPointTransforms)
    ELEM(0x0207, JPEGQTables)
    ELEM(0x0208, JPEGDCTables)
    ELEM(0x0209, JPEGACTables)
    ELEM(0x0211, YCbCrCoefficients)
    ELEM(0x0212, YCbCrSubSampling)
    ELEM(0x0213, YCbCrPositioning)
    ELEM(0x0214, ReferenceBlackWhite)
    ELEM(0x02BC, XMP) // XMP Part 3
    ELEM(0x4746, Rating)
    ELEM(0x4749, RatingPercent)
    ELEM(0x5001, ResolutionXUnit) // 5001-5113 Defined Microsoft
    ELEM(0x5002, ResolutionYUnit)
    ELEM(0x5003, ResolutionXLengthUnit)
    ELEM(0x5004, ResolutionYLengthUnit)
    ELEM(0x5005, PrintFlags)
    ELEM(0x5006, PrintFlagsVersion)
    ELEM(0x5007, PrintFlagsCrop)
    ELEM(0x5008, PrintFlagsBleedWidth)
    ELEM(0x5009, PrintFlagsBleedWidthScale)
    ELEM(0x500A, HalftoneLPI)
    ELEM(0x500B, HalftoneLPIUnit)
    ELEM(0x500C, HalftoneDegree)
    ELEM(0x500D, HalftoneShape)
    ELEM(0x500E, HalftoneMisc)
    ELEM(0x500F, HalftoneScreen)
    ELEM(0x5010, JPEGQuality)
    ELEM(0x5011, GridSize)
    ELEM(0x5012, ThumbnailFormat)
    ELEM(0x5013, ThumbnailWidth)
    ELEM(0x5014, ThumbnailHeight)
    ELEM(0x5015, ThumbnailColorDepth)
    ELEM(0x5016, ThumbnailPlanes)
    ELEM(0x5017, ThumbnailRawBytes)
    ELEM(0x5018, ThumbnailLength)
    ELEM(0x5019, ThumbnailCompressedSize)
    ELEM(0x501a, ColorTransferFunction)
    ELEM(0x501b, ThumbnailData)
    ELEM(0x5020, ThumbnailImageWidth)
    ELEM(0x5021, ThumbnailImageHeight)
    ELEM(0x5022, ThumbnailBitsPerSample)
    ELEM(0x5023, ThumbnailCompression)
    ELEM(0x5024, ThumbnailPhotometricInterp)
    ELEM(0x5025, ThumbnailDescription)
    ELEM(0x5026, ThumbnailEquipMake)
    ELEM(0x5027, ThumbnailEquipModel)
    ELEM(0x5028, ThumbnailStripOffsets)
    ELEM(0x5029, ThumbnailOrientation)
    ELEM(0x502A, ThumbnailSamplesPerPixel)
    ELEM(0x502B, ThumbnailRowsPerStrip)
    ELEM(0x502C, ThumbnailStripByteCounts)
    ELEM(0x502D, ThumbnailResolutionX)
    ELEM(0x502E, ThumbnailResolutionY)
    ELEM(0x502F, ThumbnailPlanarConfig)
    ELEM(0x5030, ThumbnailResolutionUnit)
    ELEM(0x5031, ThumbnailTransferFunction)
    ELEM(0x5032, ThumbnailSoftware)
    ELEM(0x5033, ThumbnailDateTime)
    ELEM(0x5034, ThumbnailArtist)
    ELEM(0x5035, ThumbnailWhitePoint)
    ELEM(0x5036, ThumbnailPrimaryChromaticities)
    ELEM(0x5037, ThumbnailYCbCrCoefficients)
    ELEM(0x5038, ThumbnailYCbCrSubsampling)
    ELEM(0x5039, ThumbnailYCbCrPositioning)
    ELEM(0x503A, ThumbnailRefBlackWhite)
    ELEM(0x503B, ThumbnailCopyright)
    ELEM(0x5090, LuminanceTable)
    ELEM(0x5091, ChrominanceTable)
    ELEM(0x5100, FrameDelay)
    ELEM(0x5101, LoopCount)
    ELEM(0x5102, GlobalPalette)
    ELEM(0x5103, IndexBackground)
    ELEM(0x5104, IndexTransparent)
    ELEM(0x5110, PixelUnits)
    ELEM(0x5111, PixelsPerUnitX)
    ELEM(0x5112, PixelsPerUnitY)
    ELEM(0x5113, PaletteHistogram)
    ELEM(0x828D, CFARepeatPatternDim) // TIFF/EP
    ELEM(0x828E, CFAPattern) // TIFF/EP
    ELEM(0x828F, BatteryLevel) // TIFF/EP
    ELEM(0x8298, Copyright)
    ELEM(0x830E, PixelScale)
    ELEM(0x83BB, IPTC_NAA) // TIFF/EP / Adobe Photoshop File Formats Specification
    ELEM(0x8480, IntergraphMatrix)
    ELEM(0x8482, ModelTiePoint)
    ELEM(0x8546, SEMInfo)
    ELEM(0x85D8, ModelTransform)
    ELEM(0x8649, PhotoshopImageResources) // Adobe Photoshop File Formats Specification
    ELEM(0x8769, IFDExif)
    ELEM(0x8773, ICC_Profile) // TIFF/EP / Adobe Photoshop File Formats Specification
    ELEM(0x87AF, GeoTiffDirectory)
    ELEM(0x87B0, GeoTiffDoubleParams)
    ELEM(0x87B1, GeoTiffAsciiParams)
    ELEM(0x8825, GPSInfoIFD)
    ELEM(0x8829, Interlace) // TIFF/EP
    ELEM(0x882A, TimeZoneOffset) // TIFF/EP
    ELEM(0x882B, SelfTimerMode) // TIFF/EP
    ELEM(0x9003, DateTimeOriginal) // TIFF/EP
    ELEM(0x920B, FlashEnergy) // TIFF/EP
    ELEM(0x920C, SpatialFrequencyResponse) // TIFF/EP
    ELEM(0x920D, Noise) // TIFF/EP
    ELEM(0x920E, FocalPlaneXResolution) // TIFF/EP
    ELEM(0x920F, FocalPlaneYResolution) // TIFF/EP
    ELEM(0x9210, FocalPlaneResolutionUnit) // TIFF/EP
    ELEM(0x9211, ImageNumber) // TIFF/EP
    ELEM(0x9212, SecurityClassification) // TIFF/EP
    ELEM(0x9213, ImageHistory) // TIFF/EP
    ELEM(0x9214, SubjectLocation) // TIFF/EP
    ELEM(0x9215, ExposureIndex) // TIFF/EP
    ELEM(0x9216, TIFFEPStandardID) // TIFF/EP
    ELEM(0x9217, SensingMethod) // TIFF/EP
    ELEM(0x935C, ImageSourceData) // Adobe Photoshop File Formats Specification
    ELEM(0x9C9B, WinExpTitle)
    ELEM(0x9C9C, WinExpComment)
    ELEM(0x9C9D, WinExpAuthor)
    ELEM(0x9C9E, WinExpKeywords)
    ELEM(0x9C9F, WinExpSubject)
    ELEM(0xA480, GDALMetadata)
    ELEM(0xA481, GDALNoData)
    ELEM(0xC44F, Annotations) // Adobe Photoshop File Formats Specification
    ELEM(0xC4A5, PrintIM)
    ELEM(0xC612, DNGVersion) // 0xC612 - 0xC61E Defined DNG
    ELEM(0xC613, DNGBackwardVersion)
    ELEM(0xC614, UniqueCameraModel)
    ELEM(0xC615, LocalizedCameraModel)
    ELEM(0xC616, CFAPlaneColor)
    ELEM(0xC617, CFALayout)
    ELEM(0xC618, LinearizationTable)
    ELEM(0xC619, BlackLevelRepeatDim)
    ELEM(0xC61A, BlackLevel)
    ELEM(0xC61B, BlackLevelDeltaH)
    ELEM(0xC61C, BlackLevelDeltaV)
    ELEM(0xC61D, WhiteLevel)
    ELEM(0xC61E, DefaultScale)
    ELEM(0xC634, MakerNote)
    ELEM(0xC635, MakerNoteSafety)
    ELEM(0xC65A, CalibrationIlluminant1)
    ELEM(0xC65B, CalibrationIlluminant2)
    ELEM(0xC691, CurrentICCProfile)
    ELEM(0xCD49, JXLDistance)
    ELEM(0xCD4A, JXLEffort)
    ELEM(0xCD4B, JXLDecodeSpeed)
    ELEM(0xCEA1, SEAL)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(ProcessingSoftware, "Processing software")
    ELEM_TRACE(SubfileType, "Subfile type")
    ELEM_TRACE(OldSubfileType, "Old subfile type")
    ELEM_TRACE(ImageWidth, "Image width")
    ELEM_TRACE(ImageHeight, "Image height")
    ELEM_TRACE(BitsPerSample, "Bits per sample")
    ELEM_TRACE(Compression, "Compression")
    ELEM_TRACE(PhotometricInterpretation, "Photometric interpretation")
    ELEM_TRACE(Thresholding, "Thresholding")
    ELEM_TRACE(CellWidth, "Cell width")
    ELEM_TRACE(CellLength, "Cell length")
    ELEM_TRACE(FillOrder, "Fill order")
    ELEM_TRACE(DocumentName, "Document name")
    ELEM_TRACE(ImageDescription, "Description")
    ELEM_TRACE(Make, "Manufacturer of image input equipment")
    ELEM_TRACE(Model, "Model of image input equipment")
    ELEM_TRACE(StripOffsets, "Strip offsets")
    ELEM_TRACE(Orientation, "Orientation")
    ELEM_TRACE(SamplesPerPixel, "Samples per pixel")
    ELEM_TRACE(RowsPerStrip, "Rows per strip")
    ELEM_TRACE(StripByteCounts, "Strip byte counts")
    ELEM_TRACE(MinSampleValue, "Min sample value")
    ELEM_TRACE(MaxSampleValue, "Max sample value")
    ELEM_TRACE(XResolution, "X resolution")
    ELEM_TRACE(YResolution, "Y resolution")
    ELEM_TRACE(PlanarConfiguration, "Planar configuration")
    ELEM_TRACE(PageName, "Page name")
    ELEM_TRACE(XPosition, "X position")
    ELEM_TRACE(YPosition, "Y position")
    ELEM_TRACE(GrayResponseUnit, "Gray response unit")
    ELEM_TRACE(ResolutionUnit, "Resolution unit")
    ELEM_TRACE(PageNumber, "Page number")
    ELEM_TRACE(TransferFunction, "Transfer function")
    ELEM_TRACE(Software, "Software used")
    ELEM_TRACE(DateTime, "Date and time of file")
    ELEM_TRACE(Artist, "Artist")
    ELEM_TRACE(HostComputer, "Host computer")
    ELEM_TRACE(Predictor, "Predictor")
    ELEM_TRACE(WhitePoint, "White point")
    ELEM_TRACE(PrimaryChromaticities, "Primary chromaticities")
    ELEM_TRACE(HalftoneHints, "Halftone hints")
    ELEM_TRACE(TileWidth, "Tile width")
    ELEM_TRACE(TileLength, "Tile length")
    ELEM_TRACE(SubIFDs, "Sub IFDs")
    ELEM_TRACE(InkSet, "Ink set")
    ELEM_TRACE(TargetPrinter, "Target printer")
    ELEM_TRACE(ExtraSamples, "Extra samples")
    ELEM_TRACE(SampleFormat, "Sample format")
    ELEM_TRACE(JPEGTables, "JPEG quantization and/or Huffman tables")
    ELEM_TRACE(JPEGProc, "JPEG process used")
    ELEM_TRACE(JPEGInterchangeFormat, "JPEG interchange format bitstream offset")
    ELEM_TRACE(JPEGInterchangeFormatLength, "JPEG interchange format bitstream length")
    ELEM_TRACE(JPEGRestartInterval, "JPEG restart interval length")
    ELEM_TRACE(JPEGLosslessPredictors, "JPEG lossless predictor-selection values list")
    ELEM_TRACE(JPEGPointTransforms, "JPEG point transform values")
    ELEM_TRACE(JPEGQTables, "JPEG quantization tables offsets list")
    ELEM_TRACE(JPEGDCTables, "JPEG DC Huffman tables offsets list")
    ELEM_TRACE(JPEGACTables, "JPEG Huffman AC tables offsets list")
    ELEM_TRACE(YCbCrCoefficients, "Transformation matrix")
    ELEM_TRACE(YCbCrSubSampling, "Chroma subsampling")
    ELEM_TRACE(YCbCrPositioning, "Chroma positioning")
    ELEM_TRACE(ReferenceBlackWhite, "Reference black and white")
    ELEM_TRACE(XMP, "XMP")
    ELEM_TRACE(Rating, "Rating")
    ELEM_TRACE(RatingPercent, "Rating (percent)")
    ELEM_TRACE(ResolutionXUnit, "ResolutionXUnit")
    ELEM_TRACE(ResolutionYUnit, "ResolutionYUnit")
    ELEM_TRACE(ResolutionXLengthUnit, "ResolutionXLengthUnit")
    ELEM_TRACE(ResolutionYLengthUnit, "ResolutionYLengthUnit")
    ELEM_TRACE(PrintFlags, "PrintFlags")
    ELEM_TRACE(PrintFlagsVersion, "PrintFlagsVersion")
    ELEM_TRACE(PrintFlagsCrop, "PrintFlagsCrop")
    ELEM_TRACE(PrintFlagsBleedWidth, "PrintFlagsBleedWidth")
    ELEM_TRACE(PrintFlagsBleedWidthScale, "PrintFlagsBleedWidthScale")
    ELEM_TRACE(HalftoneLPI, "HalftoneLPI")
    ELEM_TRACE(HalftoneLPIUnit, "HalftoneLPIUnit")
    ELEM_TRACE(HalftoneDegree, "HalftoneDegree")
    ELEM_TRACE(HalftoneShape, "HalftoneShape")
    ELEM_TRACE(HalftoneMisc, "HalftoneMisc")
    ELEM_TRACE(HalftoneScreen, "HalftoneScreen")
    ELEM_TRACE(JPEGQuality, "JPEGQuality")
    ELEM_TRACE(GridSize, "GridSize")
    ELEM_TRACE(ThumbnailFormat, "ThumbnailFormat")
    ELEM_TRACE(ThumbnailWidth, "ThumbnailWidth")
    ELEM_TRACE(ThumbnailHeight, "ThumbnailHeight")
    ELEM_TRACE(ThumbnailColorDepth, "ThumbnailColorDepth")
    ELEM_TRACE(ThumbnailPlanes, "ThumbnailPlanes")
    ELEM_TRACE(ThumbnailRawBytes, "ThumbnailRawBytes")
    ELEM_TRACE(ThumbnailLength, "ThumbnailLength")
    ELEM_TRACE(ThumbnailCompressedSize, "ThumbnailCompressedSize")
    ELEM_TRACE(ColorTransferFunction, "ColorTransferFunction")
    ELEM_TRACE(ThumbnailData, "ThumbnailData")
    ELEM_TRACE(ThumbnailImageWidth, "ThumbnailImageWidth")
    ELEM_TRACE(ThumbnailImageHeight, "ThumbnailImageHeight")
    ELEM_TRACE(ThumbnailBitsPerSample, "ThumbnailBitsPerSample")
    ELEM_TRACE(ThumbnailCompression, "ThumbnailCompression")
    ELEM_TRACE(ThumbnailPhotometricInterp, "ThumbnailPhotometricInterp")
    ELEM_TRACE(ThumbnailDescription, "ThumbnailDescription")
    ELEM_TRACE(ThumbnailEquipMake, "ThumbnailEquipMake")
    ELEM_TRACE(ThumbnailEquipModel, "ThumbnailEquipModel")
    ELEM_TRACE(ThumbnailStripOffsets, "ThumbnailStripOffsets")
    ELEM_TRACE(ThumbnailOrientation, "ThumbnailOrientation")
    ELEM_TRACE(ThumbnailSamplesPerPixel, "ThumbnailSamplesPerPixel")
    ELEM_TRACE(ThumbnailRowsPerStrip, "ThumbnailRowsPerStrip")
    ELEM_TRACE(ThumbnailStripByteCounts, "ThumbnailStripByteCounts")
    ELEM_TRACE(ThumbnailResolutionX, "ThumbnailResolutionX")
    ELEM_TRACE(ThumbnailResolutionY, "ThumbnailResolutionY")
    ELEM_TRACE(ThumbnailPlanarConfig, "ThumbnailPlanarConfig")
    ELEM_TRACE(ThumbnailResolutionUnit, "ThumbnailResolutionUnit")
    ELEM_TRACE(ThumbnailTransferFunction, "ThumbnailTransferFunction")
    ELEM_TRACE(ThumbnailSoftware, "ThumbnailSoftware")
    ELEM_TRACE(ThumbnailDateTime, "ThumbnailDateTime")
    ELEM_TRACE(ThumbnailArtist, "ThumbnailArtist")
    ELEM_TRACE(ThumbnailWhitePoint, "ThumbnailWhitePoint")
    ELEM_TRACE(ThumbnailPrimaryChromaticities, "ThumbnailPrimaryChromaticities")
    ELEM_TRACE(ThumbnailYCbCrCoefficients, "ThumbnailYCbCrCoefficients")
    ELEM_TRACE(ThumbnailYCbCrSubsampling, "ThumbnailYCbCrSubsampling")
    ELEM_TRACE(ThumbnailYCbCrPositioning, "ThumbnailYCbCrPositioning")
    ELEM_TRACE(ThumbnailRefBlackWhite, "ThumbnailRefBlackWhite")
    ELEM_TRACE(ThumbnailCopyright, "ThumbnailCopyright")
    ELEM_TRACE(LuminanceTable, "LuminanceTable")
    ELEM_TRACE(ChrominanceTable, "ChrominanceTable")
    ELEM_TRACE(FrameDelay, "FrameDelay")
    ELEM_TRACE(LoopCount, "LoopCount")
    ELEM_TRACE(GlobalPalette, "GlobalPalette")
    ELEM_TRACE(IndexBackground, "IndexBackground")
    ELEM_TRACE(IndexTransparent, "IndexTransparent")
    ELEM_TRACE(PixelUnits, "PixelUnits")
    ELEM_TRACE(PixelsPerUnitX, "PixelsPerUnitX")
    ELEM_TRACE(PixelsPerUnitY, "PixelsPerUnitY")
    ELEM_TRACE(PaletteHistogram, "PaletteHistogram")
    ELEM_TRACE(CFARepeatPatternDim, "CFA Repeat Pattern Dim")
    ELEM_TRACE(CFAPattern, "CFA pattern")
    ELEM_TRACE(BatteryLevel, "Battery Level")
    ELEM_TRACE(Copyright, "Copyright")
    ELEM_TRACE(PixelScale, "Pixel scale")
    ELEM_TRACE(IPTC_NAA, "IPTC-NAA")
    ELEM_TRACE(IntergraphMatrix, "Intergraph matrix")
    ELEM_TRACE(ModelTiePoint, "Model tie point")
    ELEM_TRACE(SEMInfo, "SEM info")
    ELEM_TRACE(ModelTransform, "Model transform")
    ELEM_TRACE(PhotoshopImageResources, "Photoshop image resources (PSIR)")
    ELEM_TRACE(IFDExif, "Exif IFD")
    ELEM_TRACE(ICC_Profile, "ICC profile")
    ELEM_TRACE(GeoTiffDirectory, "GeoTiff directory")
    ELEM_TRACE(GeoTiffDoubleParams, "GeoTiff double params")
    ELEM_TRACE(GeoTiffAsciiParams, "GeoTiff ASCII params")
    ELEM_TRACE(GPSInfoIFD, "GPS IFD")
    ELEM_TRACE(Interlace, "Interlace")
    ELEM_TRACE(TimeZoneOffset, "Time Zone Offset")
    ELEM_TRACE(SelfTimerMode, "Self Timer Mode")
    ELEM_TRACE(DateTimeOriginal, "Date Time Original")
    ELEM_TRACE(FlashEnergy, "Flash Energy")
    ELEM_TRACE(SpatialFrequencyResponse, "Spatial Frequency Response")
    ELEM_TRACE(Noise, "Noise")
    ELEM_TRACE(FocalPlaneXResolution, "Focal plane X resolution")
    ELEM_TRACE(FocalPlaneYResolution, "Focal plane Y resolution")
    ELEM_TRACE(FocalPlaneResolutionUnit, "Focal plane resolutionUnit")
    ELEM_TRACE(ImageNumber, "Image Number")
    ELEM_TRACE(SecurityClassification, "Security Classification")
    ELEM_TRACE(ImageHistory, "Image History")
    ELEM_TRACE(SubjectLocation, "Subject location")
    ELEM_TRACE(ExposureIndex, "Exposure index")
    ELEM_TRACE(TIFFEPStandardID, "TIFF/EP Standard ID")
    ELEM_TRACE(SensingMethod, "Sensing method")
    ELEM_TRACE(ImageSourceData, "Photoshop image source data")
    ELEM_TRACE(WinExpTitle, "Title (Windows Explorer)")
    ELEM_TRACE(WinExpComment, "Comment (Windows Explorer)")
    ELEM_TRACE(WinExpAuthor, "Author (Windows Explorer)")
    ELEM_TRACE(WinExpKeywords, "Keywords (Windows Explorer)")
    ELEM_TRACE(WinExpSubject, "Subject (Windows Explorer)")
    ELEM_TRACE(GDALMetadata, "GDAL metadata")
    ELEM_TRACE(GDALNoData, "GDAL no data")
    ELEM_TRACE(Annotations, "Annotations")
    ELEM_TRACE(PrintIM, "Print IM")
    ELEM_TRACE(DNGVersion, "DNG Version")
    ELEM_TRACE(DNGBackwardVersion, "DNG Backward Version")
    ELEM_TRACE(UniqueCameraModel, "Unique Camera Model")
    ELEM_TRACE(LocalizedCameraModel, "Localized Camera Model")
    ELEM_TRACE(CFAPlaneColor, "CFA Plane Color")
    ELEM_TRACE(CFALayout, "CFA Layout")
    ELEM_TRACE(LinearizationTable, "Linearization Table")
    ELEM_TRACE(BlackLevelRepeatDim, "Black Level Repeat Dim")
    ELEM_TRACE(BlackLevel, "Black Level")
    ELEM_TRACE(BlackLevelDeltaH, "Black Level Delta H")
    ELEM_TRACE(BlackLevelDeltaV, "Black Level Delta V")
    ELEM_TRACE(WhiteLevel, "White Level")
    ELEM_TRACE(DefaultScale, "Default Scale")
    ELEM_TRACE(MakerNote, "Manufacturer notes")
    ELEM_TRACE(MakerNoteSafety, "Manufacturer notes safety")
    ELEM_TRACE(CalibrationIlluminant1, "Calibration illuminant 1")
    ELEM_TRACE(CalibrationIlluminant2, "Calibration illuminant 2")
    ELEM_TRACE(CurrentICCProfile, "Current ICC profile")
    ELEM_TRACE(JXLDistance, "Distance (JXL)")
    ELEM_TRACE(JXLEffort, "Effort (JXL)")
    ELEM_TRACE(JXLDecodeSpeed, "Decode speed (JXL)")
    ELEM_TRACE(SEAL, "SEAL")
};
};

namespace IFDExif {
    ELEM(0x829A, ExposureTime)
    ELEM(0x829D, FNumber)
    ELEM(0x8822, ExposureProgram)
    ELEM(0x8824, SpectralSensitivity)
    ELEM(0x8827, PhotographicSensitivity)
    ELEM(0x8828, OECF)
    ELEM(0x882A, TimeZoneOffset)
    ELEM(0x882B, SelfTimerMode)
    ELEM(0x8830, SensitivityType)
    ELEM(0x8831, StandardOutputSensitivity)
    ELEM(0x8832, RecommendedExposureIndex)
    ELEM(0x8833, ISOSpeed)
    ELEM(0x8834, ISOSpeedLatitudeyyy)
    ELEM(0x8835, ISOSpeedLatitudezzz)
    ELEM(0x9000, ExifVersion)
    ELEM(0x9003, DateTimeOriginal)
    ELEM(0x9004, DateTimeDigitized)
    ELEM(0x9009, GooglePlusUploadCode)
    ELEM(0x9010, OffsetTime)
    ELEM(0x9011, OffsetTimeOriginal)
    ELEM(0x9012, OffsetTimeDigitized)
    ELEM(0x9101, ComponentsConfiguration)
    ELEM(0x9102, CompressedBitsPerPixel)
    ELEM(0x9201, ShutterSpeedValue)
    ELEM(0x9202, ApertureValue)
    ELEM(0x9203, BrightnessValue)
    ELEM(0x9204, ExposureBiasValue)
    ELEM(0x9205, MaxApertureValue)
    ELEM(0x9206, SubjectDistance)
    ELEM(0x9207, MeteringMode)
    ELEM(0x9208, LightSource)
    ELEM(0x9209, Flash)
    ELEM(0x920A, FocalLength)
    ELEM(0x9211, ImageNumber)
    ELEM(0x9212, SecurityClassification)
    ELEM(0x9213, ImageHistory)
    ELEM(0x9214, SubjectArea)
    ELEM(0x927C, MakerNote)
    ELEM(0x9286, UserComment)
    ELEM(0x9290, SubSecTime)
    ELEM(0x9291, SubSecTimeOriginal)
    ELEM(0x9292, SubSecTimeDigitized)
    ELEM(0x9400, AmbientTemperature)
    ELEM(0x9401, Humidity)
    ELEM(0x9402, Pressure)
    ELEM(0x9403, WaterDepth)
    ELEM(0x9404, Acceleration)
    ELEM(0x9405, CameraElevationAngle)
    ELEM(0x9999, XiaomiSettings)
    ELEM(0x9A00, XiaomiModel)
    ELEM(0xA000, FlashpixVersion)
    ELEM(0xA001, ColorSpace)
    ELEM(0xA002, PixelXDimension)
    ELEM(0xA003, PixelYDimension)
    ELEM(0xA004, RelatedSoundFile)
    ELEM(0xA005, InteroperabilityIFD)
    ELEM(0xA20B, FlashEnergy)
    ELEM(0xA20C, SpatialFrequencyResponse)
    ELEM(0xA20E, FocalPlaneXResolution)
    ELEM(0xA20F, FocalPlaneYResolution)
    ELEM(0xA210, FocalPlaneResolutionUnit)
    ELEM(0xA214, SubjectLocation)
    ELEM(0xA215, ExposureIndex)
    ELEM(0xA217, SensingMethod)
    ELEM(0xA300, FileSource)
    ELEM(0xA301, SceneType)
    ELEM(0xA302, CFAPattern)
    ELEM(0xA401, CustomRendered)
    ELEM(0xA402, ExposureMode)
    ELEM(0xA403, WhiteBalance)
    ELEM(0xA404, DigitalZoomRatio)
    ELEM(0xA405, FocalLengthIn35mmFilm)
    ELEM(0xA406, SceneCaptureType)
    ELEM(0xA407, GainControl)
    ELEM(0xA408, Contrast)
    ELEM(0xA409, Saturation)
    ELEM(0xA40A, Sharpness)
    ELEM(0xA40C, SubjectDistanceRange)
    ELEM(0xA420, ImageUniqueID)
    ELEM(0xA430, CameraOwnerName)
    ELEM(0xA431, BodySerialNumber)
    ELEM(0xA432, LensSpecification)
    ELEM(0xA433, LensMake)
    ELEM(0xA434, LensModel)
    ELEM(0xA435, LensSerialNumber)
    ELEM(0xA436, ImageTitle)
    ELEM(0xA437, Photographer)
    ELEM(0xA438, ImageEditor)
    ELEM(0xA439, CameraFirmware)
    ELEM(0xA43A, RAWDevelopingSoftware)
    ELEM(0xA43B, ImageEditingSoftware)
    ELEM(0xA43C, MetadataEditingSoftware)
    ELEM(0xA460, CompositeImage)
    ELEM(0xA461, CompositeImageCount)
    ELEM(0xA500, Gamma)
    ELEM(0xEA1C, Padding)
    ELEM(0xEA1D, OffsetSchema)
    ELEM(0xFDE8, Adobe_OwnerName)
    ELEM(0xFDE9, Adobe_SerialNumber)
    ELEM(0xFDEA, Adobe_Lens)
    ELEM(0xFE4C, Adobe_RawFile)
    ELEM(0xFE4D, Adobe_Converter)
    ELEM(0xFE4E, Adobe_WhiteBalance)
    ELEM(0xFE51, Adobe_Exposure)
    ELEM(0xFE52, Adobe_Shadows)
    ELEM(0xFE53, Adobe_Brightness)
    ELEM(0xFE54, Adobe_Contrast)
    ELEM(0xFE55, Adobe_Saturation)
    ELEM(0xFE56, Adobe_Sharpness)
    ELEM(0xFE57, Adobe_Smoothness)
    ELEM(0xFE58, Adobe_MoireFilter)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(ExposureTime, "Exposure time")
    ELEM_TRACE(FNumber, "F number")
    ELEM_TRACE(ExposureProgram, "Exposure program")
    ELEM_TRACE(SpectralSensitivity, "Spectral sensitivity")
    ELEM_TRACE(PhotographicSensitivity, "Photographic sensitivity")
    ELEM_TRACE(OECF, "Optoelectric coefficient")
    ELEM_TRACE(TimeZoneOffset, "Time zone offset")
    ELEM_TRACE(SelfTimerMode, "Self timer mode")
    ELEM_TRACE(SensitivityType, "Sensitivity type")
    ELEM_TRACE(StandardOutputSensitivity, "Standard output sensitivity")
    ELEM_TRACE(RecommendedExposureIndex, "Recommended exposure index")
    ELEM_TRACE(ISOSpeed, "ISOSpeed")
    ELEM_TRACE(ISOSpeedLatitudeyyy, "ISOSpeed latitude yyy")
    ELEM_TRACE(ISOSpeedLatitudezzz, "ISOSpeed Latitude zzz")
    ELEM_TRACE(ExifVersion, "Exif Version")
    ELEM_TRACE(DateTimeOriginal, "Date and time original")
    ELEM_TRACE(DateTimeDigitized, "Date and time digitized")
    ELEM_TRACE(GooglePlusUploadCode, "GooglePlusUploadCode")
    ELEM_TRACE(OffsetTime, "Date and time offset file")
    ELEM_TRACE(OffsetTimeOriginal, "Date and time offset original")
    ELEM_TRACE(OffsetTimeDigitized, "Date and time offset digitized")
    ELEM_TRACE(ComponentsConfiguration, "Meaning of each component")
    ELEM_TRACE(CompressedBitsPerPixel, "Image compression mode")
    ELEM_TRACE(ShutterSpeedValue, "Shutter speed")
    ELEM_TRACE(ApertureValue, "Aperture")
    ELEM_TRACE(BrightnessValue, "Brightness")
    ELEM_TRACE(ExposureBiasValue, "Exposure bias")
    ELEM_TRACE(MaxApertureValue, "Maximum lens aperture")
    ELEM_TRACE(SubjectDistance, "Subject distance")
    ELEM_TRACE(MeteringMode, "Metering mode")
    ELEM_TRACE(LightSource, "Light source")
    ELEM_TRACE(Flash, "Flash")
    ELEM_TRACE(FocalLength, "Lens focal length")
    ELEM_TRACE(ImageNumber, "Image number")
    ELEM_TRACE(SecurityClassification, "Security classification")
    ELEM_TRACE(ImageHistory, "Image history")
    ELEM_TRACE(SubjectArea, "Subject area")
    ELEM_TRACE(MakerNote, "Manufacturer notes")
    ELEM_TRACE(UserComment, "User comments")
    ELEM_TRACE(SubSecTime, "Date and time subseconds file")
    ELEM_TRACE(SubSecTimeOriginal, "Date and time subseconds original")
    ELEM_TRACE(SubSecTimeDigitized, "Date and time subseconds digitized")
    ELEM_TRACE(AmbientTemperature, "Ambient temperature")
    ELEM_TRACE(Humidity, "Humidity")
    ELEM_TRACE(Pressure, "Pressure")
    ELEM_TRACE(WaterDepth, "Water depth")
    ELEM_TRACE(Acceleration, "Acceleration")
    ELEM_TRACE(CameraElevationAngle, "Camera elevation angle")
    ELEM_TRACE(XiaomiSettings, "Settings (Xiaomi)")
    ELEM_TRACE(XiaomiModel, "Model (Xiaomi)")
    ELEM_TRACE(FlashpixVersion, "Flashpix version")
    ELEM_TRACE(ColorSpace, "Colorspace")
    ELEM_TRACE(PixelXDimension, "Image width")
    ELEM_TRACE(PixelYDimension, "Image height")
    ELEM_TRACE(RelatedSoundFile, "Related sound file")
    ELEM_TRACE(InteroperabilityIFD, "Interoperability IFD")
    ELEM_TRACE(FlashEnergy, "Flash energy")
    ELEM_TRACE(SpatialFrequencyResponse, "Spatial Frequency Response")
    ELEM_TRACE(FocalPlaneXResolution, "Focal plane X resolution")
    ELEM_TRACE(FocalPlaneYResolution, "Focal plane Y resolution")
    ELEM_TRACE(FocalPlaneResolutionUnit, "Focal plane resolutionUnit")
    ELEM_TRACE(SubjectLocation, "Subject location")
    ELEM_TRACE(ExposureIndex, "Exposure index")
    ELEM_TRACE(SensingMethod, "Sensing method")
    ELEM_TRACE(FileSource, "File source")
    ELEM_TRACE(SceneType, "Scene type")
    ELEM_TRACE(CFAPattern, "CFA pattern")
    ELEM_TRACE(CustomRendered, "Custom image processing")
    ELEM_TRACE(ExposureMode, "Exposure mode")
    ELEM_TRACE(WhiteBalance, "White balance")
    ELEM_TRACE(DigitalZoomRatio, "Digital zoom ratio")
    ELEM_TRACE(FocalLengthIn35mmFilm, "Focal length in 35mm film")
    ELEM_TRACE(SceneCaptureType, "Scene capture type")
    ELEM_TRACE(GainControl, "Gain control")
    ELEM_TRACE(Contrast, "Contrast")
    ELEM_TRACE(Saturation, "Saturation")
    ELEM_TRACE(Sharpness, "Sharpness")
    ELEM_TRACE(SubjectDistanceRange, "Subject distance range")
    ELEM_TRACE(ImageUniqueID, "Unique image ID")
    ELEM_TRACE(CameraOwnerName, "Camera owner name")
    ELEM_TRACE(BodySerialNumber, "Body serial number")
    ELEM_TRACE(LensSpecification, "Lens specification")
    ELEM_TRACE(LensMake, "Lens manufacturer")
    ELEM_TRACE(LensModel, "Lens model")
    ELEM_TRACE(LensSerialNumber, "Lens serial number")
    ELEM_TRACE(ImageTitle, "Title")
    ELEM_TRACE(Photographer, "Photographer")
    ELEM_TRACE(ImageEditor, "Editor")
    ELEM_TRACE(CameraFirmware, "Camera firmware")
    ELEM_TRACE(RAWDevelopingSoftware, "RAW developing software")
    ELEM_TRACE(ImageEditingSoftware, "Editing software")
    ELEM_TRACE(MetadataEditingSoftware, "Metadata editing software")
    ELEM_TRACE(CompositeImage, "Composite image")
    ELEM_TRACE(CompositeImageCount, "Composite image count")
    ELEM_TRACE(Gamma, "Gamma")
    ELEM_TRACE(Padding, "Padding")
    ELEM_TRACE(OffsetSchema, "Offset schema (Microsoft)")
    ELEM_TRACE(Adobe_OwnerName, "Owner name (Adobe)")
    ELEM_TRACE(Adobe_SerialNumber, "Serial number (Adobe)")
    ELEM_TRACE(Adobe_Lens, "Lens (Adobe)")
    ELEM_TRACE(Adobe_RawFile, "Raw file (Adobe)")
    ELEM_TRACE(Adobe_Converter, "Converter (Adobe)")
    ELEM_TRACE(Adobe_WhiteBalance, "White balance 2 (Adobe)")
    ELEM_TRACE(Adobe_Exposure, "Exposure (Adobe)")
    ELEM_TRACE(Adobe_Shadows, "Shadows (Adobe)")
    ELEM_TRACE(Adobe_Brightness, "Brightness (Adobe)")
    ELEM_TRACE(Adobe_Contrast, "Contrast (Adobe)")
    ELEM_TRACE(Adobe_Saturation, "Saturation (Adobe)")
    ELEM_TRACE(Adobe_Sharpness, "Sharpness (Adobe)")
    ELEM_TRACE(Adobe_Smoothness, "Smoothness (Adobe)")
    ELEM_TRACE(Adobe_MoireFilter, "Moire filter (Adobe)")
};
};

namespace IFDGPS {
    ELEM(0x0000, GPSVersionID)
    ELEM(0x0001, GPSLatitudeRef)
    ELEM(0x0002, GPSLatitude)
    ELEM(0x0003, GPSLongitudeRef)
    ELEM(0x0004, GPSLongitude)
    ELEM(0x0005, GPSAltitudeRef)
    ELEM(0x0006, GPSAltitude)
    ELEM(0x0007, GPSTimeStamp)
    ELEM(0x0008, GPSSatellites)
    ELEM(0x0009, GPSStatus)
    ELEM(0x000A, GPSMeasureMode)
    ELEM(0x000B, GPSDOP)
    ELEM(0x000C, GPSSpeedRef)
    ELEM(0x000D, GPSSpeed)
    ELEM(0x000E, GPSTrackRef)
    ELEM(0x000F, GPSTrack)
    ELEM(0x0010, GPSImgDirectionRef)
    ELEM(0x0011, GPSImgDirection)
    ELEM(0x0012, GPSMapDatum)
    ELEM(0x0013, GPSDestLatitudeRef)
    ELEM(0x0014, GPSDestLatitude)
    ELEM(0x0015, GPSDestLongitudeRef)
    ELEM(0x0016, GPSDestLongitude)
    ELEM(0x0017, GPSDestBearingRef)
    ELEM(0x0018, GPSDestBearing)
    ELEM(0x0019, GPSDestDistanceRef)
    ELEM(0x001A, GPSDestDistance)
    ELEM(0x001B, GPSProcessingMethod)
    ELEM(0x001C, GPSAreaInformation)
    ELEM(0x001D, GPSDateStamp)
    ELEM(0x001E, GPSDifferential)
    ELEM(0x001F, GPSHPositioningError)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(GPSVersionID, "Version")
    ELEM_TRACE(GPSLatitudeRef, "North or South Latitude")
    ELEM_TRACE(GPSLatitude, "Latitude")
    ELEM_TRACE(GPSLongitudeRef, "East or West Longitude")
    ELEM_TRACE(GPSLongitude, "Longitude")
    ELEM_TRACE(GPSAltitudeRef, "Altitude reference")
    ELEM_TRACE(GPSAltitude, "Altitude")
    ELEM_TRACE(GPSTimeStamp, "Time (atomic clock)")
    ELEM_TRACE(GPSSatellites, "satellites used for measurement")
    ELEM_TRACE(GPSStatus, "Receiver status")
    ELEM_TRACE(GPSMeasureMode, "Measurement mode")
    ELEM_TRACE(GPSDOP, "Measurement precision")
    ELEM_TRACE(GPSSpeedRef, "Speed unit")
    ELEM_TRACE(GPSSpeed, "Speed of GPS receiver")
    ELEM_TRACE(GPSTrackRef, "Reference for direction of movement")
    ELEM_TRACE(GPSTrack, "Direction of movement")
    ELEM_TRACE(GPSImgDirectionRef, "Reference for direction of image")
    ELEM_TRACE(GPSImgDirection, "Direction of image")
    ELEM_TRACE(GPSMapDatum, "Geodetic survey data used")
    ELEM_TRACE(GPSDestLatitudeRef, "Reference for latitude of destination")
    ELEM_TRACE(GPSDestLatitude, "Latitude of destination")
    ELEM_TRACE(GPSDestLongitudeRef, "Reference for longitude of destination")
    ELEM_TRACE(GPSDestLongitude, "Longitude of destination")
    ELEM_TRACE(GPSDestBearingRef, "Reference for bearing of destination")
    ELEM_TRACE(GPSDestBearing, "Bearing of destination")
    ELEM_TRACE(GPSDestDistanceRef, "Reference for distance to destination")
    ELEM_TRACE(GPSDestDistance, "Distance to destination")
    ELEM_TRACE(GPSProcessingMethod, "Name of processing method")
    ELEM_TRACE(GPSAreaInformation, "Name of area")
    ELEM_TRACE(GPSDateStamp, "Date")
    ELEM_TRACE(GPSDifferential, "Differential correction")
    ELEM_TRACE(GPSHPositioningError, "Horizontal positioning error")
};
};

namespace IFDInterop {
    ELEM(0x0001, InteroperabilityIndex)
    ELEM(0x0002, InteroperabilityVersion)
    ELEM(0x1000, RelatedImageFileFormat)
    ELEM(0x1001, RelatedImageWidth)
    ELEM(0x1002, RelatedImageHeight)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(InteroperabilityIndex, "Interoperability Index")
    ELEM_TRACE(InteroperabilityVersion, "Interoperability Version")
    ELEM_TRACE(RelatedImageFileFormat, "Related Image File Format")
    ELEM_TRACE(RelatedImageWidth, "Related Image Width")
    ELEM_TRACE(RelatedImageHeight, "Related Image Height")
};
};

namespace IFDMakernoteApple {
    ELEM(0x0001, MakerNoteVersion)
    ELEM(0x0003, RunTime)
    ELEM(0x0004, AEStable)
    ELEM(0x0005, AETarget)
    ELEM(0x0006, AEAverage)
    ELEM(0x0007, AFStable)
    ELEM(0x0008, AccelerationVector)
    ELEM(0x000a, HDRImageType)
    ELEM(0x000b, BurstUUID)
    ELEM(0x000c, FocusDistanceRange)
    ELEM(0x000f, OISMode)
    ELEM(0x0011, ContentIdentifier)
    ELEM(0x0014, ImageCaptureType)
    ELEM(0x0015, ImageUniqueID)
    ELEM(0x0017, LivePhotoVideoIndex)
    ELEM(0x001a, QualityHint)
    ELEM(0x001d, LuminanceNoiseAmplitude)
    ELEM(0x001f, PhotosAppFeatureFlags)
    ELEM(0x0021, HDRHeadroom)
    ELEM(0x0023, AFPerformance)
    ELEM(0x0027, SignalToNoiseRatio)
    ELEM(0x002b, PhotoIdentifier)
    ELEM(0x002d, ColorTemperature)
    ELEM(0x002e, CameraType)
    ELEM(0x002f, FocusPosition)
    ELEM(0x0030, HDRGain)
    ELEM(0x0038, AFMeasuredDepth)
    ELEM(0x003d, AFConfidence)
    ELEM(0x0040, SemanticStyle)
    ELEM(0x0041, SemanticStyleRenderingVer)
    ELEM(0x0042, SemanticStylePreset)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(MakerNoteVersion, "Maker Note Version")
    ELEM_TRACE(RunTime, "Run Time")
    ELEM_TRACE(AEStable, "AE Stable")
    ELEM_TRACE(AETarget, "AE Target")
    ELEM_TRACE(AEAverage, "AE Average")
    ELEM_TRACE(AFStable, "AF Stable")
    ELEM_TRACE(AccelerationVector, "Acceleration Vector")
    ELEM_TRACE(HDRImageType, "HDR Image Type")
    ELEM_TRACE(BurstUUID, "Burst UUID")
    ELEM_TRACE(FocusDistanceRange, "Focus Distance Range")
    ELEM_TRACE(OISMode, "OIS Mode")
    ELEM_TRACE(ContentIdentifier, "Content Identifier")
    ELEM_TRACE(ImageCaptureType, "Image Capture Type")
    ELEM_TRACE(ImageUniqueID, "Image Unique ID")
    ELEM_TRACE(LivePhotoVideoIndex, "Live Photo Video Index")
    ELEM_TRACE(QualityHint, "Quality Hint")
    ELEM_TRACE(LuminanceNoiseAmplitude, "Luminance Noise Amplitude")
    ELEM_TRACE(PhotosAppFeatureFlags, "Photos App Feature Flags")
    ELEM_TRACE(HDRHeadroom, "HDR Headroom")
    ELEM_TRACE(AFPerformance, "AF Performance")
    ELEM_TRACE(SignalToNoiseRatio, "Signal to Noise Ratio")
    ELEM_TRACE(PhotoIdentifier, "Photo Identifier")
    ELEM_TRACE(ColorTemperature, "Color Temperature")
    ELEM_TRACE(CameraType, "Camera Type")
    ELEM_TRACE(FocusPosition, "Focus Position")
    ELEM_TRACE(HDRGain, "HDR Gain")
    ELEM_TRACE(AFMeasuredDepth, "AF Measured Depth")
    ELEM_TRACE(AFConfidence, "AF Confidence")
    ELEM_TRACE(SemanticStyle, "Semantic Style")
    ELEM_TRACE(SemanticStyleRenderingVer, "Semantic Style Rendering Version")
    ELEM_TRACE(SemanticStylePreset, "Semantic Style Preset")
};
};

namespace IFDMakernoteNikon {
    ELEM(0x0001, MakerNoteVersion)
    ELEM(0x0002, ISO)
    ELEM(0x0003, ColorMode)
    ELEM(0x0004, Quality)
    ELEM(0x0005, WhiteBalance)
    ELEM(0x0006, Sharpness)
    ELEM(0x0007, FocusMode)
    ELEM(0x0008, FlashSetting)
    ELEM(0x0009, FlashType)
    ELEM(0x000b, WhiteBalanceFineTune)
    ELEM(0x000c, WB_RBLevels)
    ELEM(0x000d, ProgramShift)
    ELEM(0x000e, ExposureDifference)
    ELEM(0x000f, ISOSelection)
    ELEM(0x0010, DataDump)
    ELEM(0x0011, PreviewIFD)
    ELEM(0x0012, FlashExposureComp)
    ELEM(0x0013, ISOSetting)
    ELEM(0x0014, ColorBalanceA)
    ELEM(0x0016, ImageBoundary)
    ELEM(0x0017, ExternalFlashExposureComp)
    ELEM(0x0018, FlashExposureBracketValue)
    ELEM(0x0019, ExposureBracketValue)
    ELEM(0x001a, ImageProcessing)
    ELEM(0x001b, CropHiSpeed)
    ELEM(0x001c, ExposureTuning)
    ELEM(0x001d, SerialNumber)
    ELEM(0x001e, ColorSpace)
    ELEM(0x001f, VRInfo)
    ELEM(0x0020, ImageAuthentication)
    ELEM(0x0021, FaceDetect)
    ELEM(0x0022, ActiveDLighting)
    ELEM(0x0023, PictureControlData)
    ELEM(0x0024, WorldTime)
    ELEM(0x0025, ISOInfo)
    ELEM(0x002a, VignetteControl)
    ELEM(0x002b, DistortInfo)
    ELEM(0x002c, UnknownInfo)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(MakerNoteVersion, "Maker Note Version")
    ELEM_TRACE(ISO, "ISO")
    ELEM_TRACE(ColorMode, "Color Mode")
    ELEM_TRACE(Quality, "Quality")
    ELEM_TRACE(WhiteBalance, "White Balance")
    ELEM_TRACE(Sharpness, "Sharpness")
    ELEM_TRACE(FocusMode, "Focus Mode")
    ELEM_TRACE(FlashSetting, "Flash Setting")
    ELEM_TRACE(FlashType, "Flash Type")
    ELEM_TRACE(WhiteBalanceFineTune, "White Balance Fine Tune")
    ELEM_TRACE(WB_RBLevels, "White Balance Red/Blue Levels")
    ELEM_TRACE(ProgramShift, "Program Shift")
    ELEM_TRACE(ExposureDifference, "Exposure Difference")
    ELEM_TRACE(ISOSelection, "ISO Selection")
    ELEM_TRACE(DataDump, "Data Dump")
    ELEM_TRACE(PreviewIFD, "Preview IFD")
    ELEM_TRACE(FlashExposureComp, "Flash Exposure Compensation")
    ELEM_TRACE(ISOSetting, "ISO Setting")
    ELEM_TRACE(ColorBalanceA, "Color Balance A")
    ELEM_TRACE(ImageBoundary, "Image Boundary")
    ELEM_TRACE(ExternalFlashExposureComp, "External Flash Exposure Compensation")
    ELEM_TRACE(FlashExposureBracketValue, "Flash Exposure Bracket Value")
    ELEM_TRACE(ExposureBracketValue, "Exposure Bracket Value")
    ELEM_TRACE(ImageProcessing, "Image Processing")
    ELEM_TRACE(CropHiSpeed, "Crop HiSpeed")
    ELEM_TRACE(ExposureTuning, "Exposure Tuning")
    ELEM_TRACE(SerialNumber, "Serial Number")
    ELEM_TRACE(ColorSpace, "Color Space")
    ELEM_TRACE(VRInfo, "Vibration Reduction Info")
    ELEM_TRACE(ImageAuthentication, "Image Authentication")
    ELEM_TRACE(FaceDetect, "Face Detect")
    ELEM_TRACE(ActiveDLighting, "Active D-Lighting")
    ELEM_TRACE(PictureControlData, "Picture Control Data")
    ELEM_TRACE(WorldTime, "World Time")
    ELEM_TRACE(ISOInfo, "ISO Info")
    ELEM_TRACE(VignetteControl, "Vignette Control")
    ELEM_TRACE(DistortInfo, "Distortion Info")
    ELEM_TRACE(UnknownInfo, "Unknown Info")
};
};

namespace IFDMakernoteSony {
    ELEM(0x0010, CameraInfo)
    ELEM(0x0020, FocusInfo)
    ELEM(0x0102, Quality)
    ELEM(0x0104, FlashExposureComp)
    ELEM(0x0105, Teleconverter)
    ELEM(0x0112, WhiteBalanceFineTune)
    ELEM(0x0114, CameraSettings)
    ELEM(0x0115, WhiteBalance)
    ELEM(0x0116, ExtraInfo)
    ELEM(0x2001, PreviewImage)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(CameraInfo, "Camera Info")
    ELEM_TRACE(FocusInfo, "Focus Info")
    ELEM_TRACE(Quality, "Quality")
    ELEM_TRACE(FlashExposureComp, "Flash Exposure Comp")
    ELEM_TRACE(Teleconverter, "Teleconverter")
    ELEM_TRACE(WhiteBalanceFineTune, "White Balance Fine Tune")
    ELEM_TRACE(CameraSettings, "Camera Settings")
    ELEM_TRACE(WhiteBalance, "White Balance")
    ELEM_TRACE(ExtraInfo, "Extra Info")
    ELEM_TRACE(PreviewImage, "Preview Image")
};
};

namespace IFDMakernoteCanon {
    ELEM(0x0001, CanonCameraSettings)
    ELEM(0x0002, CanonFocalLength)
    ELEM(0x0004, CanonShotInfo)
    ELEM(0x0005, CanonPanorama)
    ELEM(0x0006, CanonImageType)
    ELEM(0x0007, CanonFirmwareVersion)
    ELEM(0x0008, FileNumber)
    ELEM(0x0009, OwnerName)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(CanonCameraSettings, "Canon Camera Settings")
    ELEM_TRACE(CanonFocalLength, "Canon Focal Length")
    ELEM_TRACE(CanonShotInfo, "Canon Shot Info")
    ELEM_TRACE(CanonPanorama, "Canon Panorama")
    ELEM_TRACE(CanonImageType, "Canon Image Type")
    ELEM_TRACE(CanonFirmwareVersion, "Canon Firmware Version")
    ELEM_TRACE(FileNumber, "File Number")
    ELEM_TRACE(OwnerName, "Owner Name")
};
};

//---------------------------------------------------------------------------
// MPF Tags
//---------------------------------------------------------------------------

namespace IFDMPF {
    ELEM(0xB000, MPFVersion)
    ELEM(0xB001, NumberOfImages)
    ELEM(0xB002, MPEntry)
    ELEM(0xB003, ImageUIDList)
    ELEM(0xB004, TotalFrames)
    ELEM(0xB101, MPIndividualNum)
    ELEM(0xB201, PanOrientation)
    ELEM(0xB202, PanOverlap_H)
    ELEM(0xB203, PanOverlap_V)
    ELEM(0xB204, BaseViewpointNum)
    ELEM(0xB205, ConvergenceAngle)
    ELEM(0xB206, BaselineLength)
    ELEM(0xB207, VerticalDivergence)
    ELEM(0xB208, AxisDistance_X)
    ELEM(0xB209, AxisDistance_Y)
    ELEM(0xB20A, AxisDistance_Z)
    ELEM(0xB20B, YawAngle)
    ELEM(0xB20C, PitchAngle)
    ELEM(0xB20D, RollAngle)

exif_tag_desc Desc[] =
{
    ELEM_TRACE(MPFVersion,          "MP Format Version Number")
    ELEM_TRACE(NumberOfImages,      "Number of Images")
    ELEM_TRACE(MPEntry,             "MP Entry")
    ELEM_TRACE(ImageUIDList,        "Individual Image Unique ID list")
    ELEM_TRACE(TotalFrames,         "Total Number of Captured Frames")
    ELEM_TRACE(MPFVersion,          "MP Format Version")
    ELEM_TRACE(MPIndividualNum,     "MP Individual Image Number")
    ELEM_TRACE(PanOrientation,      "Panorama Scanning Orientation")
    ELEM_TRACE(PanOverlap_H,        "Panorama Horizontal Overlap")
    ELEM_TRACE(PanOverlap_V,        "Panorama Vertical Overlap")
    ELEM_TRACE(BaseViewpointNum,    "Base Viewpoint Number")
    ELEM_TRACE(ConvergenceAngle,    "Convergence Angle")
    ELEM_TRACE(BaselineLength,      "Baseline Length")
    ELEM_TRACE(VerticalDivergence,  "Divergence Angle")
    ELEM_TRACE(AxisDistance_X,      "Horizontal Axis Distance")
    ELEM_TRACE(AxisDistance_Y,      "Vertical Axis Distance")
    ELEM_TRACE(AxisDistance_Z,      "Collimation Axis Distance")
    ELEM_TRACE(YawAngle,            "Yaw Angle")
    ELEM_TRACE(PitchAngle,          "Pitch Angle")
    ELEM_TRACE(RollAngle,           "Roll Angle")
};
};

//---------------------------------------------------------------------------
// Names and Descriptions
//---------------------------------------------------------------------------
struct exif_tag_desc_size
{
    exif_tag_desc* Table;
    size_t Size;
    const char* Description;
};
#define DESC_TABLE(_TABLE,_DESC) { _TABLE::Desc, sizeof(_TABLE::Desc) / sizeof(*_TABLE::Desc), _DESC },
enum kind_of_ifd
{
    // Exif
    Kind_IFD0,
    Kind_SubIFD0,
    Kind_SubIFD1,
    Kind_SubIFD2,
    Kind_IFD2,
    Kind_IFD1,
    Kind_Exif,
    Kind_GPS,
    Kind_Interop,

    // Exif Makernotes
    Kind_MakernoteApple,
    Kind_MakernoteNikon,
    Kind_NikonPreview,
    Kind_MakernoteSony,
    Kind_MakernoteCanon,

    // MPF
    Kind_MPF,

    // Special
    Kind_ParsingThumbnail,
    Kind_ParsingNikonPreview
};
exif_tag_desc_size Exif_Descriptions[] =
{
    DESC_TABLE(IFD0, "IFD0 (primary image)")
    DESC_TABLE(IFD0, "Sub IFD")
    DESC_TABLE(IFD0, "Sub IFD 2")
    DESC_TABLE(IFD0, "Sub IFD 3")
    DESC_TABLE(IFD0, "IFD2")
    DESC_TABLE(IFD0, "IFD1 (thumbnail)")
    DESC_TABLE(IFDExif, "Exif")
    DESC_TABLE(IFDGPS, "GPS")
    DESC_TABLE(IFDInterop, "Interoperability")
    DESC_TABLE(IFDMakernoteApple, "Apple Makernote")
    DESC_TABLE(IFDMakernoteNikon, "Nikon Makernote")
    DESC_TABLE(IFD0, "Nikon Makernote Preview Image")
    DESC_TABLE(IFDMakernoteSony, "Sony Makernote")
    DESC_TABLE(IFDMakernoteCanon, "Canon Makernote")
    DESC_TABLE(IFDMPF, "MPF")
};
static string Exif_Tag_Description(int8u NameSpace, int16u Tag_ID)
{
    #ifdef MEDIAINFO_TRACE
    for (size_t Pos = 0; Pos < Exif_Descriptions[NameSpace].Size; ++Pos)
    {
        if (Exif_Descriptions[NameSpace].Table[Pos].Tag_ID == Tag_ID)
            return Exif_Descriptions[NameSpace].Table[Pos].Description;
    }
    #endif //MEDIAINFO_TRACE
    return Ztring::ToZtring_From_CC2(Tag_ID).To_UTF8();
}

//---------------------------------------------------------------------------
// Exif Names and Descriptions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
namespace Exif_IFD0_Orientation {
    const int16u Horizontal                   = 1;
    const int16u MirrorHorizontal             = 2;
    const int16u Rotate180                    = 3;
    const int16u MirrorVertical               = 4;
    const int16u MirrorHorizontalRotate270CW  = 5;
    const int16u Rotate90CW                   = 6;
    const int16u MirrorHorizontalRotate90CW   = 7;
    const int16u Rotate270CW                  = 8;
}

//---------------------------------------------------------------------------
static const char* Exif_IFD0_Orientation_Name(int16u orientation)
{
    switch (orientation)
    {
    case Exif_IFD0_Orientation::Horizontal: return "Horizontal (normal)";
    case Exif_IFD0_Orientation::MirrorHorizontal: return "Mirror horizontal";
    case Exif_IFD0_Orientation::Rotate180: return "Rotate 180";
    case Exif_IFD0_Orientation::MirrorVertical: return "Mirror vertical";
    case Exif_IFD0_Orientation::MirrorHorizontalRotate270CW: return "Mirror horizontal and rotate 270 CW";
    case Exif_IFD0_Orientation::Rotate90CW: return "Rotate 90 CW";
    case Exif_IFD0_Orientation::MirrorHorizontalRotate90CW: return "Mirror horizontal and rotate 90 CW";
    case Exif_IFD0_Orientation::Rotate270CW: return "Rotate 270 CW";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_ExifIFD_Tag_LightSource_Name(int16u value) {
    switch (value) {
    case 0: return "Unknown";
    case 1: return "Daylight";
    case 2: return "Fluorescent";
    case 3: return "Tungsten (Incandescent)";
    case 4: return "Flash";
    case 9: return "Fine Weather";
    case 10: return "Cloudy";
    case 11: return "Shade";
    case 12: return "Daylight Fluorescent";
    case 13: return "Day White Fluorescent";
    case 14: return "Cool White Fluorescent";
    case 15: return "White Fluorescent";
    case 16: return "Warm White Fluorescent";
    case 17: return "Standard Light A";
    case 18: return "Standard Light B";
    case 19: return "Standard Light C";
    case 20: return "D55";
    case 21: return "D65";
    case 22: return "D75";
    case 23: return "D50";
    case 24: return "ISO Studio Tungsten";
    case 255: return "Other";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static string Exif_IFDExif_Flash_Name(int8u value) {
    string flash;
    switch (value & 0b00011000) {
    case 0b00000000: flash += "Unknown"; break;
    case 0b00001000: flash += "On"; break;
    case 0b00010000: flash += "Off"; break;
    case 0b00011000: flash += "Auto"; break;
    }
    switch (value & 0b00000001) {
    case 0b00000000: flash += ", Did not fire"; break;
    case 0b00000001: flash += ", Fired"; break;
    }
    switch (value & 0b00100000) {
    case 0b00100000: flash += ", No flash function"; break;
    }
    switch (value & 0b01000000) {
    case 0b01000000: flash += ", Red-eye reduction"; break;
    }
    switch (value & 0b00000110) {
    case 0b00000100: flash += ", Return not detected"; break;
    case 0b00000110: flash += ", Return detected"; break;
    }
    return flash;
}

//---------------------------------------------------------------------------
static const char* Exif_IFDExif_ColorSpace_Name(int16u value)
{
    switch (value)
    {
    case 0x1: return "sRGB";
    case 0x2: return "Adobe RGB";
    case 0xfffd: return "Wide Gamut RGB";
    case 0xfffe: return "ICC Profile";
    case 0xffff: return "Uncalibrated";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDExif_ExposureProgram_Name(int16u value)
{
    switch (value) {
    case 0: return "Not Defined";
    case 1: return "Manual";
    case 2: return "Program AE";
    case 3: return "Aperture-priority AE";
    case 4: return "Shutter speed priority AE";
    case 5: return "Creative (Slow speed)";
    case 6: return "Action (High speed)";
    case 7: return "Portrait";
    case 8: return "Landscape";
    case 9: return "Bulb";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDExif_WhiteBalance_Name(int16u value)
{
    switch (value) {
    case 0: return "Auto";
    case 1: return "Manual";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDExif_CompositeImage_Name(int16u value)
{
    switch (value) {
    case 1: return "Not a Composite Image";
    case 2: return "General Composite Image";
    case 3: return "Composite Image Captured While Shooting";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDGPS_GPSAltitudeRef_Name(int8u value)
{
    switch (value) {
    case 0: return "Positive ellipsoidal height (at or above ellipsoidal surface)";
    case 1: return "Negative ellipsoidal height (below ellipsoidal surface)";
    case 2: return "Positive sea level value (at or above sea level reference)";
    case 3: return "Negative sea level value (below sea level reference)";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteApple_ImageCaptureType_Name(int32s value)
{
    switch (value) {
    case 1: return "ProRAW";
    case 2: return "Portrait";
    case 10: return "Photo";
    case 11: return "Manual Focus";
    case 12: return "Scene";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteApple_CameraType_Name(int32s value)
{
    switch (value) {
    case 0: return "Back Wide Angle";
    case 1: return "Back Normal";
    case 6: return "Front";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteNikon_CropHiSpeed_Name(int16u value) {
    switch (value) {
    case 0: return "Off";
    case 1: return "1.3x Crop";
    case 2: return "DX Crop";
    case 3: return "5:4 Crop";
    case 4: return "3:2 Crop";
    case 6: return "16:9 Crop";
    case 8: return "2.7x Crop";
    case 9: return "DX Movie Crop";
    case 10: return "1.3x Movie Crop";
    case 11: return "FX Uncropped";
    case 12: return "DX Uncropped";
    case 13: return "2.8x Movie Crop";
    case 14: return "1.4x Movie Crop";
    case 15: return "1.5x Movie Crop";
    case 17: return "1:1 Crop";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteNikon_ColorSpace_Name(int16u value) {
    switch (value) {
    case 1: return "sRGB";
    case 2: return "Adobe RGB";
    case 4: return "BT.2100";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteNikon_ColorSpace_ColourPrimaries(int16u value) {
    switch (value) {
    case 1: return "BT.709";
    case 2: return "Adobe RGB";
    case 4: return "BT.2020";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteNikon_ColorSpace_TransferCharacteristics(int16u value) {
    switch (value) {
    case 1: return "sRGB/sYCC";
    case 2: return "Adobe RGB";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteNikon_ActiveDLighting_Name(int16u value)
{
    switch (value) {
    case 0: return "Off";
    case 1: return "Low";
    case 3: return "Normal";
    case 5: return "High";
    case 7: return "Extra High";
    case 8: return "Extra High 1";
    case 9: return "Extra High 2";
    case 10: return "Extra High 3";
    case 11: return "Extra High 4";
    case 0xFFFF: return "Auto";
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteSony_Quality_Name(int32u value)
{
    switch (value) {
    case 0: return "RAW";
    case 1: return "Super Fine";
    case 2: return "Fine";
    case 3: return "Standard";
    case 4: return "Economy";
    case 5: return "Extra Fine";
    case 6: return "RAW + JPEG/HEIF";
    case 7: return "Compressed RAW";
    case 8: return "Compressed RAW + JPEG";
    case 9: return "Light";
    case 0xFFFFFFFF: // n/a
    default: return "";
    }
}

//---------------------------------------------------------------------------
static const char* Exif_IFDMakernoteSony_WhiteBalance_Name(int32u value) {
    switch (value) {
    case 0x00: return "Auto";
    case 0x01: return "Color Temperature/Color Filter";
    case 0x10: return "Daylight";
    case 0x20: return "Cloudy";
    case 0x30: return "Shade";
    case 0x40: return "Tungsten";
    case 0x50: return "Flash";
    case 0x60: return "Fluorescent";
    case 0x70: return "Custom";
    case 0x80: return "Underwater";
    default: return "";
    }
}

//---------------------------------------------------------------------------
// MPF Names and Descriptions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static string Mpf_ImageAttribute_Desc(int32u value)
{
    string desc;
    if (value & 1U << 31)
        desc += "Dependent Parent Image, ";
    if (value & 1U << 30)
        desc += "Dependent Child Image, ";
    if (value & 1U << 29)
        desc += "Representative Image, ";
    if (!(value & ((1U << 24) | (1U << 25) | (1U << 26))))
        desc += "JPEG, ";
    switch (value & ((1U << 24) - 1))
    {
    case 0x010001: desc += "Large Thumbnail, Class 1, VGA equivalent"; break;
    case 0x010002: desc += "Large Thumbnail, Class 2, Full HD equivalent"; break;
    case 0x020001: desc += "Multi-Frame Image, Panorama"; break;
    case 0x020002: desc += "Multi-Frame Image, Disparity"; break;
    case 0x020003: desc += "Multi-Frame Image, Multi-Angle"; break;
    case 0x030000: desc += "Baseline MP Primary Image"; break;
    case 0x050000: desc += "Gain map"; break;
    }
    return desc;
}

std::string mp_entry::Type() const
{
    switch (ImgAttribute & ((1U << 24) - 1)) {
    case 0x000000: return {};
    case 0x010001:
    case 0x010002: return "Thumbnail";
    case 0x020001: return "Panorama"; break;
    case 0x020002: return "Disparity"; break;
    case 0x020003: return "Multi-Angle"; break;
    case 0x030000: return "Primary"; break;
    case 0x050000: return "Gain map"; break;
    default: return Ztring::ToZtring_From_CC4(ImgAttribute).To_UTF8();
    }
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Streams_Finish()
{
    const auto Infos_Image_It = Infos.find(Kind_IFD0);
    const auto Infos_Thumbnail_It = Infos.find(Kind_IFD1);
    const auto Infos_Exif_It = Infos.find(Kind_Exif);
    const auto Infos_GPS_It = Infos.find(Kind_GPS);
    const auto Infos_Interop_It = Infos.find(Kind_Interop);

    auto FillMetadata = [&](Ztring& Value, const std::pair<const int16u, ZtringList>& Item, size_t Parameter, const char* ParameterC, const string& Unit) {
        if (Value.empty()) {
            Value = Item.second.size() == 1 ? Item.second.front() : Item.second.Read();
        }
        if (Value.empty()) {
            return;
        }
        if (Parameter) {
            if (Parameter != (size_t)-1) {
                Fill(Stream_General, 0, Parameter, Value);
            }
        }
        else if (ParameterC) {
            if (Retrieve_Const(Stream_General, 0, ParameterC) == Value) {
                return;
            }
            Fill(Stream_General, 0, ParameterC, Value);
            if (!Unit.empty()) {
                Fill_SetOptions(Stream_General, 0, ParameterC, "N NF");
                Fill(Stream_General, 0, string(ParameterC).append("/String").c_str(), (Unit.front() == ' ' ? Value : Ztring()) + Ztring().From_UTF8(Unit));
            }
        }
        else {
            //Fill(Stream_General, 0, Exif_Tag_Description(currentIFD, Item.first).c_str(), Value);
        }
    };

    auto MergeDateTimeSubSecOffset = [&](const ZtringList& DateTime, int16u SubSecTimeTag, int16u OffsetTimeTag) -> Ztring {
        Ztring Value = DateTime.Read();
        if (Infos_Exif_It != Infos.end()) {
            const auto& Infos_Exif = Infos_Exif_It->second;
            const auto SubSecTime = Infos_Exif.find(SubSecTimeTag);
            if (SubSecTime != Infos_Exif.end()) {
                Value += __T('.') + SubSecTime->second.Read();
            }
            const auto OffsetTime = Infos_Exif.find(OffsetTimeTag);
            if (OffsetTime != Infos_Exif.end()) {
                Value += OffsetTime->second.Read();
            }
        }
        return Value;
    };

    if (Infos_Image_It != Infos.end()) {
        currentIFD = Kind_IFD0;
        const auto& Infos_Image = Infos_Image_It->second;
        for (const auto& Item : Infos_Image) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFD0::DocumentName: Parameter = General_Title; break;
            case IFD0::ImageDescription: Parameter = General_Description; break;
            case IFD0::Make: Parameter = General_Encoded_Hardware_CompanyName; break;
            case IFD0::Model: Parameter = General_Encoded_Hardware_Model; break;
            case IFD0::Software: Parameter = General_Encoded_Application; break;
            case IFD0::DateTime: {
                Parameter = General_Encoded_Date;
                Value = MergeDateTimeSubSecOffset(Item.second, IFDExif::SubSecTime, IFDExif::OffsetTime);
                break;
            }
            case IFD0::ResolutionXUnit:
            case IFD0::ResolutionYUnit:
            case IFD0::ResolutionXLengthUnit:
            case IFD0::ResolutionYLengthUnit:
            case IFD0::PrintFlags:
            case IFD0::PrintFlagsVersion:
            case IFD0::PrintFlagsCrop:
            case IFD0::PrintFlagsBleedWidth:
            case IFD0::PrintFlagsBleedWidthScale:
            case IFD0::HalftoneLPI:
            case IFD0::HalftoneLPIUnit:
            case IFD0::HalftoneDegree:
            case IFD0::HalftoneShape:
            case IFD0::HalftoneMisc:
            case IFD0::HalftoneScreen:
            case IFD0::JPEGQuality:
            case IFD0::GridSize:
            case IFD0::ThumbnailFormat:
            case IFD0::ThumbnailWidth:
            case IFD0::ThumbnailHeight:
            case IFD0::ThumbnailColorDepth:
            case IFD0::ThumbnailPlanes:
            case IFD0::ThumbnailRawBytes:
            case IFD0::ThumbnailLength:
            case IFD0::ThumbnailCompressedSize:
            case IFD0::ColorTransferFunction:
            case IFD0::ThumbnailData:
            case IFD0::ThumbnailImageWidth:
            case IFD0::ThumbnailImageHeight:
            case IFD0::ThumbnailBitsPerSample:
            case IFD0::ThumbnailCompression:
            case IFD0::ThumbnailPhotometricInterp:
            case IFD0::ThumbnailDescription:
            case IFD0::ThumbnailEquipMake:
            case IFD0::ThumbnailEquipModel:
            case IFD0::ThumbnailStripOffsets:
            case IFD0::ThumbnailOrientation:
            case IFD0::ThumbnailSamplesPerPixel:
            case IFD0::ThumbnailRowsPerStrip:
            case IFD0::ThumbnailStripByteCounts:
            case IFD0::ThumbnailResolutionX:
            case IFD0::ThumbnailResolutionY:
            case IFD0::ThumbnailPlanarConfig:
            case IFD0::ThumbnailResolutionUnit:
            case IFD0::ThumbnailTransferFunction:
            case IFD0::ThumbnailSoftware:
            case IFD0::ThumbnailDateTime:
            case IFD0::ThumbnailArtist:
            case IFD0::ThumbnailWhitePoint:
            case IFD0::ThumbnailPrimaryChromaticities:
            case IFD0::ThumbnailYCbCrCoefficients:
            case IFD0::ThumbnailYCbCrSubsampling:
            case IFD0::ThumbnailYCbCrPositioning:
            case IFD0::ThumbnailRefBlackWhite:
            case IFD0::ThumbnailCopyright:
            case IFD0::LuminanceTable:
            case IFD0::ChrominanceTable:
            case IFD0::FrameDelay:
            case IFD0::LoopCount:
            case IFD0::GlobalPalette:
            case IFD0::IndexBackground:
            case IFD0::IndexTransparent:
            case IFD0::PixelUnits:
            case IFD0::PixelsPerUnitX:
            case IFD0::PixelsPerUnitY:
            case IFD0::PaletteHistogram:
                Parameter = (size_t)-1;
                break;
            case IFD0::Copyright: Parameter = General_Copyright; break;
            case IFD0::TIFFEPStandardID: {
                ParameterC = "TIFFEPVersion";
                const auto TIFFEPStandardID = Infos_Image.find(IFD0::TIFFEPStandardID);
                if (TIFFEPStandardID->second.size() == 4) {
                    Value = TIFFEPStandardID->second.at(0) + __T(".") + TIFFEPStandardID->second.at(1) + __T(".") + TIFFEPStandardID->second.at(2) + __T(".") + TIFFEPStandardID->second.at(3);
                }
                break;
            }
            case IFD0::WinExpTitle: Parameter = General_Title; break;
            case IFD0::WinExpComment: Parameter = General_Comment; break;
            case IFD0::WinExpAuthor: Parameter = General_Performer; break;
            case IFD0::WinExpKeywords: Parameter = General_Keywords; break;
            case IFD0::WinExpSubject: Parameter = General_Subject; break;
            case IFD0::DNGVersion: {
                ParameterC = "DNGVersion";
                const auto DNGVersion = Infos_Image.find(IFD0::DNGVersion);
                if (DNGVersion->second.size() == 4) {
                    Value = DNGVersion->second.at(0) + __T(".") + DNGVersion->second.at(1) + __T(".") + DNGVersion->second.at(2) + __T(".") + DNGVersion->second.at(3);
                }
                break;
            }
            case IFD0::DNGBackwardVersion: {
                ParameterC = "DNGBackwardVersion";
                const auto DNGBackwardVersion = Infos_Image.find(IFD0::DNGBackwardVersion);
                if (DNGBackwardVersion->second.size() == 4) {
                    Value = DNGBackwardVersion->second.at(0) + __T(".") + DNGBackwardVersion->second.at(1) + __T(".") + DNGBackwardVersion->second.at(2) + __T(".") + DNGBackwardVersion->second.at(3);
                }
                break;
            }
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
            switch (Item.first) {
            case IFD0::TIFFEPStandardID: Fill_SetOptions(Stream_General, 0, ParameterC, "N NT"); break;
            case IFD0::DNGVersion: Fill_SetOptions(Stream_General, 0, ParameterC, "N NT"); break;
            case IFD0::DNGBackwardVersion: Fill_SetOptions(Stream_General, 0, ParameterC, "N NT"); break;
            }
        }
    }

    if (Infos_Exif_It != Infos.end()) {
        currentIFD = Kind_Exif;
        const auto& Infos_Exif = Infos_Exif_It->second;
        for (const auto& Item : Infos_Exif) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDExif::ExposureTime: {
                ParameterC = "ShutterSpeed_Time";
                const auto exposure_time{ Item.second.Read().To_float64() };
                string ShutterSpeed_Time;
                if (exposure_time < 0.25001 && exposure_time > 0) {
                    ShutterSpeed_Time = "1/" + std::to_string(static_cast<int>(round(1 / exposure_time)));
                }
                else {
                    ShutterSpeed_Time = std::to_string(exposure_time);
                    ShutterSpeed_Time.erase(ShutterSpeed_Time.find_last_not_of('0') + 1);
                    if (ShutterSpeed_Time.back() == '.')
                        ShutterSpeed_Time.pop_back();
                }
                Value = Item.second.Read();
                ParameterS = ShutterSpeed_Time + " s";
                break;
            }
            case IFDExif::FNumber: ParameterC = "IrisFNumber"; Value.From_Number(Item.second.Read().To_float64(), 1); break;
            case IFDExif::ExposureProgram: ParameterC = "AutoExposureMode"; Value = Exif_IFDExif_ExposureProgram_Name(Item.second.Read().To_int16u()); break;
            case IFDExif::PhotographicSensitivity: if (Item.second.Read().To_int16u()) ParameterC = "ISOSensitivity"; break;
            case IFDExif::SensitivityType: {
                int16u ISOSensitivityType = 0;
                switch (Item.second.Read().To_int16u()) {
                case 1:
                case 4:
                case 5:
                case 7: ISOSensitivityType = IFDExif::StandardOutputSensitivity; break;
                case 2:
                case 6: ISOSensitivityType = IFDExif::RecommendedExposureIndex; break;
                case 3: ISOSensitivityType = IFDExif::ISOSpeed; break;
                }
                if (ISOSensitivityType) {
                    const auto RealISOSensitivity = Infos_Exif.find(ISOSensitivityType);
                    if (RealISOSensitivity != Infos_Exif.end()) {
                        Value = RealISOSensitivity->second.Read();
                    }
                }
                if (!Value.empty()) {
                    Clear(Stream_General, 0, "ISOSensitivity");
                    ParameterC = "ISOSensitivity";
                }
                break;
            }
            case IFDExif::ExifVersion: {
                ParameterC = "ExifVersion";
                string exif_ver{ Item.second.Read().To_UTF8() };
                if (exif_ver.size() == 4) {
                    exif_ver.insert(2, 1, '.');
                    if (exif_ver.front() == '0')
                        exif_ver.erase(0, 1);
                }
                Value.From_UTF8(exif_ver);
                break;
            }
            case IFDExif::ExposureBiasValue: {
                ParameterC = "ExposureBiasValue";
                if (Item.second.Read().To_float64() > 0) {
                    Value.assign(1, '+');
                }
                Value += Item.second.Read();
                break;
            }
            case IFDExif::Flash: ParameterC = "Flash"; Value = Exif_IFDExif_Flash_Name(Item.second.Read().To_int8u()).c_str(); break;
            case IFDExif::FocalLength: ParameterC = "LensZoomActualFocalLength"; ParameterS = " mm"; break;
            case IFDExif::FocalLengthIn35mmFilm:
                if (Item.second.Read().To_int16u()) {
                    ParameterC = "LensZoom35mmStillCameraEquivalent";
                    ParameterS = " mm";
                }
                else Parameter = (size_t)-1;
                break;
            case IFDExif::UserComment: Parameter = General_Comment; break;
            case IFDExif::LensMake: ParameterC = "LensMake"; break;
            case IFDExif::LensModel: ParameterC = "LensModel"; break;
            case IFDExif::ImageTitle: Parameter = General_Title; break;
            case IFDExif::OffsetTime:
            case IFDExif::OffsetTimeOriginal:
            case IFDExif::OffsetTimeDigitized:
            case IFDExif::SubSecTime:
            case IFDExif::SubSecTimeOriginal:
            case IFDExif::SubSecTimeDigitized:
                Parameter = (size_t)-1;
                break;
            case IFDExif::DateTimeOriginal:
                Parameter = General_Recorded_Date;
                Value = MergeDateTimeSubSecOffset(Item.second, IFDExif::SubSecTimeOriginal, IFDExif::OffsetTimeOriginal);
                break;
            case IFDExif::DateTimeDigitized:
                Parameter = General_Mastered_Date;
                Value = MergeDateTimeSubSecOffset(Item.second, IFDExif::SubSecTimeDigitized, IFDExif::OffsetTimeDigitized);
                break;
            case IFDExif::FlashpixVersion: {
                ParameterC = "FlashpixVersion";
                string flashpix_ver{ Item.second.Read().To_UTF8() };
                if (flashpix_ver.size() == 4) {
                    flashpix_ver.insert(2, 1, '.');
                    if (flashpix_ver.front() == '0')
                        flashpix_ver.erase(0, 1);
                }
                Value.From_UTF8(flashpix_ver);
                break;
            }
            case IFDExif::WhiteBalance: ParameterC = "AutoWhiteBalanceMode"; Value = Exif_IFDExif_WhiteBalance_Name(Item.second.Read().To_int16u()); break;
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
            switch (Item.first) {
            case IFDExif::ExifVersion:
            case IFDExif::FlashpixVersion: Fill_SetOptions(Stream_General, 0, ParameterC, "N NF"); break;
            }
        }
    }

    if (Infos_GPS_It != Infos.end()) {
        currentIFD = Kind_GPS;
        const auto& Infos_GPS = Infos_GPS_It->second;
        for (const auto& Item : Infos_GPS) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDGPS::GPSVersionID: {
                ParameterC = "ExifGPSVersion";
                const auto GPSVersionID = Infos_GPS.find(IFDGPS::GPSVersionID);
                if (GPSVersionID->second.size() == 4) {
                    Value = GPSVersionID->second.at(0) + __T(".") + GPSVersionID->second.at(1) + __T(".") + GPSVersionID->second.at(2) + __T(".") + GPSVersionID->second.at(3);
                }
                break;
            }
            case IFDGPS::GPSLatitudeRef:
            case IFDGPS::GPSLongitudeRef:
            case IFDGPS::GPSLongitude:
            case IFDGPS::GPSAltitudeRef:
            case IFDGPS::GPSAltitude:
                Parameter = (size_t)-1;
                break;
            case IFDGPS::GPSLatitude: {
                Parameter = General_Recorded_Location;
                const auto GPSLatitude = Infos_GPS.find(IFDGPS::GPSLatitude);
                const auto GPSLatitudeRef = Infos_GPS.find(IFDGPS::GPSLatitudeRef);
                const auto GPSLongitude = Infos_GPS.find(IFDGPS::GPSLongitude);
                const auto GPSLongitudeRef = Infos_GPS.find(IFDGPS::GPSLongitudeRef);
                const auto GPSAltitudeRef = Infos_GPS.find(IFDGPS::GPSAltitudeRef);
                const auto GPSAltitude = Infos_GPS.find(IFDGPS::GPSAltitude);
                if (GPSLatitude->second.size() == 3 && GPSLatitudeRef != Infos_GPS.end() && GPSLongitude != Infos_GPS.end() && GPSLongitude->second.size() == 3 && GPSLongitudeRef != Infos_GPS.end()) {
                    Value = GPSLatitude->second.at(0) + Ztring().From_UTF8("\xC2\xB0");
                    if (GPSLatitude->second.at(1) != __T("0") || GPSLatitude->second.at(0).find(__T('.')) == string::npos) {
                        Value += GPSLatitude->second.at(1) + __T('\'');
                    }
                    if (GPSLatitude->second.at(2) != __T("0") || GPSLatitude->second.at(1).find(__T('.')) == string::npos) {
                        Value += GPSLatitude->second.at(2) + __T('\"');
                    }
                    Value += GPSLatitudeRef->second.at(0) + __T(' ');
                    Value += GPSLongitude->second.at(0) + Ztring().From_UTF8("\xC2\xB0");
                    if (GPSLongitude->second.at(1) != __T("0") || GPSLongitude->second.at(0).find(__T('.')) == string::npos) {
                        Value += GPSLongitude->second.at(1) + __T('\'');
                    }
                    if (GPSLongitude->second.at(2) != __T("0") || GPSLongitude->second.at(1).find(__T('.')) == string::npos) {
                        Value += GPSLongitude->second.at(2) + __T('\"');;
                    }
                    Value += GPSLongitudeRef->second.at(0) + __T(' ');
                    if (GPSAltitude != Infos_GPS.end()) {
                        if (GPSAltitudeRef != Infos_GPS.end())
                            if (GPSAltitude->second.Read().To_int8u() == 1 || GPSAltitude->second.Read().To_int8u() == 3)
                                Value += __T("-");
                        Value += GPSAltitude->second.Read() + __T('m');
                    }
                }
                break;
                }
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
            switch (Item.first) {
            case IFDGPS::GPSVersionID: Fill_SetOptions(Stream_General, 0, ParameterC, "N NT"); break;
            }
        }
    }

    if (Infos_Interop_It != Infos.end()) {
        currentIFD = Kind_Interop;
        const auto& Infos_Interop = Infos_Interop_It->second;
        for (const auto& Item : Infos_Interop) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDInterop::InteroperabilityVersion: {
                ParameterC = "InteropVersion";
                string interop_ver{ Item.second.Read().To_UTF8() };
                if (interop_ver.size() == 4) {
                    interop_ver.insert(2, 1, '.');
                    if (interop_ver.front() == '0')
                        interop_ver.erase(0, 1);
                }
                Value.From_UTF8(interop_ver);
                break;
            }
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
            switch (Item.first) {
            case IFDInterop::InteroperabilityVersion: Fill_SetOptions(Stream_General, 0, ParameterC, "N NF"); break;
            }
        }
    }

    const auto Infos_MakernoteApple_It = Infos.find(Kind_MakernoteApple);
    if (Infos_MakernoteApple_It != Infos.end()) {
        currentIFD = Kind_MakernoteApple;
        const auto& Infos_MakernoteApple = Infos_MakernoteApple_It->second;
        for (const auto& Item : Infos_MakernoteApple) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDMakernoteApple::ColorTemperature: ParameterC = "ColorTemperature"; ParameterS = " K"; break;
            case IFDMakernoteApple::CameraType: ParameterC = "CameraType"; Value = Exif_IFDMakernoteApple_CameraType_Name(Item.second.Read().To_int32s()); break;
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
        }
    }

    const auto Infos_MakernoteNikon_It = Infos.find(Kind_MakernoteNikon);
    if (Infos_MakernoteNikon_It != Infos.end()) {
        currentIFD = Kind_MakernoteNikon;
        const auto& Infos_MakernoteNikon = Infos_MakernoteNikon_It->second;
        for (const auto& Item : Infos_MakernoteNikon) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDMakernoteNikon::ActiveDLighting: ParameterC = "Active_D-Lighting"; Value = Exif_IFDMakernoteNikon_ActiveDLighting_Name(Item.second.Read().To_int16u()); break;
            case IFDMakernoteNikon::ColorSpace:
                Fill(Stream_Image, 0, Image_colour_primaries, Exif_IFDMakernoteNikon_ColorSpace_ColourPrimaries(Item.second.Read().To_int16u()));
                Fill(Stream_Image, 0, Image_transfer_characteristics, Exif_IFDMakernoteNikon_ColorSpace_TransferCharacteristics(Item.second.Read().To_int16u()));
                break;
            case IFDMakernoteNikon::Quality:
                ParameterC = "Quality";
                Value = Item.second.Read();
                while (Value.back() == __T(' ')) Value.pop_back();
                break;
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
        }
    }

    const auto Infos_MakernoteSony_It = Infos.find(Kind_MakernoteSony);
    if (Infos_MakernoteSony_It != Infos.end()) {
        currentIFD = Kind_MakernoteSony;
        const auto& Infos_MakernoteSony = Infos_MakernoteSony_It->second;
        for (const auto& Item : Infos_MakernoteSony) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            case IFDMakernoteSony::WhiteBalance: ParameterC = "AutoWhiteBalanceMode"; Value = Exif_IFDMakernoteSony_WhiteBalance_Name(Item.second.Read().To_int32u()); break;
            case IFDMakernoteSony::Quality: ParameterC = "Quality"; Value = Exif_IFDMakernoteSony_Quality_Name(Item.second.Read().To_int32u()); break;
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
        }
    }

    const auto Infos_MPF_It = Infos.find(Kind_MPF);
    if (Infos_MPF_It != Infos.end()) {
        currentIFD = Kind_MPF;
        const auto& Infos_MPF = Infos_MPF_It->second;
        for (const auto& Item : Infos_MPF) {
            size_t Parameter = 0;
            Ztring Value;
            const char* ParameterC = nullptr;
            string ParameterS;
            switch (Item.first) {
            }
            FillMetadata(Value, Item, Parameter, ParameterC, ParameterS);
        }
    }

    //ICC
    if (ICC_Parser)
        Merge(*ICC_Parser, Stream_Image, 0, 0);
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::FileHeader_Parse()
{
    //Parsing
    bool SkipHeader{};
    int32u Alignment;

    //HEIF
    if (FromHeif) {
        int32u Size;
        Get_B4 (Size,                                           "Exif header length");
        string Identifier;
        Get_String(Size, Identifier,                            "Identifier");
        if (!(Size == 0 || (Size == 6 && !strncmp(Identifier.c_str(), "Exif\0", 6)))) {
            Reject();
            return;
        }
        OffsetFromContainer = static_cast<int64s>(4) + Size;
    }

    //Exif Makernotes
    if (IsMakernote) {
        if (Buffer_Size >= 10 && !strncmp(reinterpret_cast<const char*>(Buffer), "Apple iOS", 10)) { // the char* contains a terminating \0
            int16u alignment;
            Skip_String(10,                                     "Identifier");
            Skip_XX(2,                                          "(Unknown)");
            Get_C2(alignment,                                   "Alignment");
            if (alignment == 0x4D4D)
                LittleEndian = false;
            else {
                Reject();
                return;
            }
            currentIFD = Kind_MakernoteApple;
            SkipHeader = true;
            IFD_Offsets[Element_Offset] = currentIFD;
        }
        else if (Buffer_Size >= 6 && !strncmp(reinterpret_cast<const char*>(Buffer), "Nikon", 6)) { // the char* contains a terminating \0
            Skip_String(6,                                      "Identifier");
            Skip_B2(                                            "Version");
            Skip_XX(2,                                          "(Unknown)");
            currentIFD = Kind_MakernoteNikon;
            OffsetFromContainer = 10;
        }
        else if (Buffer_Size >= 12 &&
            (!strncmp(reinterpret_cast<const char*>(Buffer), "SONY DSC \0\0", 12)) || // the char* contains another terminating \0
            (!strncmp(reinterpret_cast<const char*>(Buffer), "SONY CAM \0\0", 12)) || // the char* contains another terminating \0
            (!strncmp(reinterpret_cast<const char*>(Buffer), "SONY MOBILE", 12))      // the char* contains a terminating \0
            ) {
            Skip_String(12,                                     "Identifier");
            currentIFD = Kind_MakernoteSony;
            OffsetFromContainer = -static_cast<int64s>(MakernoteOffset);
            SkipHeader = true;
            IFD_Offsets[MakernoteOffset + Element_Offset] = currentIFD;
        }
        else if (Buffer_Size > 26 && 
            (!strncmp(reinterpret_cast<const char*>(Buffer) + Buffer_Size - 8, "\x49\x49\x2A\x00", 4)) || // Canon Makernote footer
            (!strncmp(reinterpret_cast<const char*>(Buffer) + Buffer_Size - 8, "\x4D\x4D\x00\x2A", 4))
            ) {
            Element_Offset = Buffer_Size - 8;
            int32u makernote_offset;
            Get_C4(Alignment,                                   "Alignment");
            if (Alignment == 0x49492A00)
                LittleEndian = true;
            else
                LittleEndian = false;
            Get_X4(makernote_offset,                            "Makernote offset");
            currentIFD = Kind_MakernoteCanon;
            OffsetFromContainer = -static_cast<int64s>(makernote_offset);
            SkipHeader = true;
            IFD_Offsets[makernote_offset] = currentIFD;
            Element_Offset = 0;
        }
        else {
            --Element_Level;
            Skip_XX(Buffer_Size,                                "Data");
            return;
        }
    }

    //MPF
    if (MPEntries) {
        currentIFD = Kind_MPF;
    }

    if (!SkipHeader) { // Some Makernotes do not have typical header
        Get_C4(Alignment,                                       "Alignment");
        if (Alignment == 0x49492A00)
            LittleEndian = true;
        else if (Alignment == 0x4D4D002A)
            LittleEndian = false;
        else
        {
            Reject();
            return;
        }

        //The only IFD that is known at forehand is the first one, it's offset is placed byte 4-7 in the file.
        Get_IFDOffset(currentIFD);
    }

    FILLING_BEGIN();
        if (IFD_Offsets.empty()) {
            Reject();
            return;
        }
        auto FirstIFDOffset = IFD_Offsets.begin()->first;
        IFD_Offsets.erase(IFD_Offsets.begin());
        if (FirstIFDOffset == (int32u)-1) {
            Reject();
            return;
        }
        Accept();
        Stream_Prepare(Stream_Image);

        //Initial IFD
        GoToOffset(FirstIFDOffset);
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Header_Parse()
{
    //Handling remaining IFD data
    if (!IfdItems.empty())
    {
        auto Offset = Buffer_Offset - OffsetFromContainer;
        if (!IsSub) {
            Offset += File_Offset;
        }
        if (Offset != IfdItems.begin()->first) {
            IfdItems.clear(); //There was a problem during the seek, trashing remaining positions from last IFD
            Finish();
            return;
        }
        #ifdef MEDIAINFO_TRACE
        Header_Fill_Code(IfdItems.begin()->second.Tag, Exif_Tag_Description(currentIFD, IfdItems.begin()->second.Tag).c_str());
        #else //MEDIAINFO_TRACE
        Header_Fill_Code(IfdItems.begin()->second.Tag);
        #endif //MEDIAINFO_TRACE
        auto SizePerBlock = Exif_Type_Size(IfdItems.begin()->second.Type);
        auto Size = static_cast<int64u>(SizePerBlock) * IfdItems.begin()->second.Count;
        if (IfdItems.size() > 1) {
            // Found buggy IFD with 2 items having the right size but the second item has a buggy offset
            auto Next = IfdItems.begin();
            ++Next;
            auto MaxSize = Next->first - IfdItems.begin()->first;
            if (Size > MaxSize) {
                bool IsCurated = false;
                if (IfdItems.size() > 2) {
                    auto Size1 = static_cast<int64u>(Exif_Type_Size(Next->second.Type)) * Next->second.Count;
                    auto Next2 = Next;
                    ++Next2;
                    auto MaxSize2 = Next2->first - IfdItems.begin()->first;
                    if (Size + Size1 == MaxSize2) {
                        IfdItems[IfdItems.begin()->first + Size] = Next->second;
                        IfdItems.erase(Next->first);
                        IsCurated = true;
                    }
                }
                if (!IsCurated) {
                    Size = MaxSize;
                    IfdItems.begin()->second.Count = Size / SizePerBlock;
                }
            }
        }
        Header_Fill_Size(Size);
        return;
    }

    //Thumbnail
    if (currentIFD == Kind_ParsingThumbnail) {
        Header_Fill_Code(0xFFFFFFFF, "Thumbnail");
        Header_Fill_Size(Buffer_Size - Buffer_Offset);
        return;
    }

    //Nikon Preview Image
    if (currentIFD == Kind_ParsingNikonPreview) {
        Header_Fill_Code(0xFFFFFFFF, "Nikon Makernote Preview Image");
        Header_Fill_Size(Buffer_Size - Buffer_Offset);
        return;
    }

    // Canon Makernote footer
    if (currentIFD == Kind_MakernoteCanon) {
        if (!HasFooter)
            HasFooter = true;
        else {
            Element_Name("Footer");
            int32u temp;
            Skip_C4(                                            "Alignment");
            Get_X4(temp,                                        "Makernote offset");
            Finish();
            return;
        }
    }

    //Get number of directories for this IFD
    int16u NrOfDirectories;
    Get_X2 (NrOfDirectories,                                    "NrOfDirectories");
    
    //Filling
    Header_Fill_Code(0xFFFFFFFF, "IFD"); //OxFFFFFFFF can not be a Tag, so using it as a magic value
    auto Size = Element_Offset;
    if (NrOfDirectories <= 0x100) { // 
        Size += 12 * static_cast<int64u>(NrOfDirectories); // 12 bytes per directory
        Size += static_cast<int64u>(currentIFD != Kind_MakernoteSony) << 2; // 4 bytes for next IFD offset, Sony Makernote IFD does not have offset to next IFD
        if (currentIFD != Kind_MakernoteSony && Size < Element_Size) { //TODO: when directory is not in full 
            if (Buffer[Buffer_Offset + (size_t)Size - (LittleEndian ? 1 : 4)]) {
                Size -= 4;
            }
        }
    }
    Header_Fill_Size(Size);
}

//---------------------------------------------------------------------------
void File_Exif::Data_Parse()
{
    if (IfdItems.empty())
    {
        if (currentIFD == Kind_ParsingThumbnail || currentIFD == Kind_ParsingNikonPreview) {
            Thumbnail();
        }
        else {
            #ifdef MEDIAINFO_TRACE
            Element_Info1(Exif_Descriptions[currentIFD].Description);
            #endif //MEDIAINFO_TRACE

            //Default values
            Infos[currentIFD].clear();

            //Parsing new IFD
            int32u IFDOffset{};
            while (Element_Size - Element_Offset >= 12)
                Read_Directory();
            if (Element_Offset < Element_Size) // Some IFD does not have offset to next IFD
                Get_IFDOffset(currentIFD == Kind_IFD0 ? Kind_IFD1 : currentIFD == Kind_IFD1 ? Kind_IFD2 : currentIFD == Kind_MPF ? Kind_MPF : (int8u)-1);
        }
    }
    else
    {
        //Handling remaining IFD data from a previous IFD
        GetValueOffsetu(IfdItems.begin()->second); //Parsing the IFD item
        IfdItems.erase(IfdItems.begin()->first); //Removing IFD item from the list of IFD items to parse
    }

    //Some items are not inside the directory, jumping to the offset
    if (!IfdItems.empty())
        GoToOffset(IfdItems.begin()->first);
    else
    {
        //This IFD is finished, going to the next IFD
        if (!IFD_Offsets.empty()) {
            GoToOffset(IFD_Offsets.begin()->first);
            currentIFD = IFD_Offsets.begin()->second;
            IFD_Offsets.erase(IFD_Offsets.begin());
            return;
        }

        //Thumbnail
        if (currentIFD != Kind_ParsingThumbnail) {
            const auto Infos_Thumbnail_It = Infos.find(Kind_IFD1);
            if (Infos_Thumbnail_It != Infos.end()) {
                const auto& Infos_Thumbnail = Infos_Thumbnail_It->second;
                const auto ImageOffset = Infos_Thumbnail.find(IFD0::JPEGInterchangeFormat);
                if (ImageOffset != Infos_Thumbnail.end() && ImageOffset->second.size() == 1) {
                    int32u IFD_Offset = ImageOffset->second.at(0).To_int32u();
                    GoToOffset(IFD_Offset);
                    currentIFD = Kind_ParsingThumbnail;
                    return;
                }
            }
        }

        //Nikon Preview Image
        if (currentIFD != Kind_ParsingNikonPreview) {
            const auto Infos_Thumbnail_It = Infos.find(Kind_NikonPreview);
            if (Infos_Thumbnail_It != Infos.end()) {
                const auto& Infos_Thumbnail = Infos_Thumbnail_It->second;
                const auto ImageOffset = Infos_Thumbnail.find(IFD0::JPEGInterchangeFormat);
                if (ImageOffset != Infos_Thumbnail.end() && ImageOffset->second.size() == 1) {
                    int32u IFD_Offset = ImageOffset->second.at(0).To_int32u();
                    GoToOffset(IFD_Offset);
                    currentIFD = Kind_ParsingNikonPreview;
                    return;
                }
            }
        }

        if (!HasFooter) Finish(); //No more IFDs
    }
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Read_Directory()
{
    // Each directory consist of 4 fields
    // Get information for this directory
    Element_Begin0();
    ifditem IfdItem{};
    Get_X2 (IfdItem.Tag,                                        "TagID"); Param_Info1(Exif_Tag_Description(currentIFD, IfdItem.Tag));
    Get_X2 (IfdItem.Type,                                       "Data type"); Param_Info1(Exif_Type_Name(IfdItem.Type));
    Get_X4 (IfdItem.Count,                                      "Number of components");
    

    #ifdef MEDIAINFO_TRACE
    Element_Name(Exif_Tag_Description(currentIFD, IfdItem.Tag).c_str());
    #endif //MEDIAINFO_TRACE


    int32u Size = Exif_Type_Size(IfdItem.Type) * IfdItem.Count;

    if (Size <= 4)
    {
        if (currentIFD == Kind_IFD0 && IfdItem.Tag == IFD0::IFDExif) {
            Get_IFDOffset(Kind_Exif);
        }
        else if (currentIFD == Kind_IFD0 && IfdItem.Tag == IFD0::GPSInfoIFD) {
            Get_IFDOffset(Kind_GPS);
        }
        else if (currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::InteroperabilityIFD) {
            Get_IFDOffset(Kind_Interop);
        }
        else if (currentIFD == Kind_MakernoteNikon && IfdItem.Tag == IFDMakernoteNikon::PreviewIFD) {
            Get_IFDOffset(Kind_NikonPreview);
        }
        else {
            GetValueOffsetu(IfdItem);

            //Padding up, skip dummy bytes
            if (Size < 4)
                Skip_XX(static_cast<int64u>(4) - Size,          "Padding");
        }
    }
    else
    {
        int32u IFDOffset;
        Get_X4 (IFDOffset,                                      "IFD offset");
        auto IFDBase = (IsSub ? 0 : File_Offset) + (Buffer_Offset - OffsetFromContainer);
        auto IsInsideDirectory = IFDOffset >= IFDBase && IFDOffset < IFDBase + Element_Size;
        if (IFDOffset // Offset cannot be zero. Zero usually means no data.
            && !IsInsideDirectory)  // Offset can not be inside the directory
            IfdItems[IFDOffset] = IfdItem;
        auto End = IFDOffset + Size;
        if (ExpectedFileSize < End)
            ExpectedFileSize = End;
    }
    Element_End0();
}

//---------------------------------------------------------------------------
void File_Exif::MulticodeString(ZtringList& Info)
{
    //Parsing
    Ztring Value;
    int64u CharacterCode;
    Get_C8(CharacterCode,                                       "Character Code");
    auto Size = Element_Size - Element_Offset;
    switch (CharacterCode) {                                    // [Character Code] Character Encoding (Character Set)
        case 0x0000000000000000LL: {                            // [Undefined Text] - (-)
            //TODO: check if ASCII
            Get_ISO_8859_1(Size, Value,                         "Value");
            break;
        }
        case 0x4153434949000000LL:                              // [ASCII] ANSI INCITS 4 (ANSI INCITS 4)
            Get_ISO_8859_1(Size, Value,                         "Value");
            break;
        case 0x554E49434F444500LL: {                            // [UNICODE] UTF-8 (Unicode)
            Peek_UTF8(Size, Value);
            if (Value.size() >= (Size - 1) / 4) // UTF-8 string may (shall) have a trailing NULL, and max 4 bytes per character
                Skip_UTF8(Size,                                 "Value");
            else if (LittleEndian)
                Get_UTF16L(Size, Value,                         "Value");
            else
                Get_UTF16B(Size, Value,                         "Value");
            break;
        }
        case 0x4A49530000000000LL:                              // [JIS] ISO-2022-JP (JIS X 0208)
            Skip_XX(Size,                                       "(Unsupported Character Code)");
            break;
        default:
            Skip_XX(Size,                                       "(Unknown Character Code)");
    }
    if (!Value.empty()) {
        Info.push_back(Value);
        Element_Info1(Value);
    }
}

//---------------------------------------------------------------------------
void File_Exif::Thumbnail()
{
    Stream_Prepare(Stream_Image);
    Fill(Stream_Image, StreamPos_Last, Image_Type, "Thumbnail");

    std::unique_ptr<File__Analyze> Parser;
    kind_of_ifd IFD;

    if (currentIFD == Kind_ParsingNikonPreview) {
        Fill(Stream_Image, StreamPos_Last, Image_MuxingMode, "Exif / Nikon Makernote");
        IFD = Kind_NikonPreview;
    }
    else {
        Fill(Stream_Image, StreamPos_Last, Image_MuxingMode, "Exif");
        IFD = Kind_IFD1;
    }
    
    const auto Infos_Thumbnail_It = Infos.find(IFD);
    if (Infos_Thumbnail_It != Infos.end()) {
        const auto& Infos_Thumbnail = Infos_Thumbnail_It->second;
        const auto Compression_It = Infos_Thumbnail.find(IFD0::Compression);
        if (Compression_It != Infos_Thumbnail.end() && Compression_It->second.size() == 1) {
            int32u Compression = Compression_It->second.at(0).To_int32u();
            switch (Compression) {
            case 6:
            case 7:
                Parser.reset(new File_Jpeg());
                break;
            }
        }
    }
    if (!Parser) {
        Parser.reset(new File__MultipleParsing);
    }

    Open_Buffer_Init(Parser.get());
    Open_Buffer_Continue(Parser.get());
    Open_Buffer_Finalize(Parser.get());
    Merge(*Parser, Stream_Image, 0, StreamPos_Last, false);
}

//---------------------------------------------------------------------------
void File_Exif::Makernote()
{
    auto Buffer_Offset_Save = Buffer_Offset;
    auto Element_Size_Save = Element_Size;
    if (Buffer_Offset > 12 && !strncmp(reinterpret_cast<const char*>(Buffer) + Buffer_Offset - 12, "SONY DSC \0\0", 12)) {
        Buffer_Offset -= 12;
        Element_Size += 12;
    }

    File_Exif MI{};
    MI.IsMakernote = true;
    MI.LittleEndian = LittleEndian;
    MI.MakernoteOffset = Buffer_Offset;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0, false);
    size_t Count = MI.Count_Get(Stream_Image);
    for (size_t i = 0; i < Count; ++i) {
        Merge(MI, Stream_Image, i, i, false);
    }

    Buffer_Offset = Buffer_Offset_Save;
    Element_Size = Element_Size_Save;
    Element_Offset = Element_Size;
}

//---------------------------------------------------------------------------
void File_Exif::ICC_Profile()
{
    #if defined(MEDIAINFO_ICC_YES)
    ICC_Parser.reset(new File_Icc());
    static_cast<File_Icc*>(ICC_Parser.get())->StreamKind = Stream_Image;
    static_cast<File_Icc*>(ICC_Parser.get())->IsAdditional = true;
    Open_Buffer_Init(ICC_Parser.get());
    Open_Buffer_Continue(ICC_Parser.get());
    Open_Buffer_Finalize(ICC_Parser.get());
    #endif
}

//---------------------------------------------------------------------------
void File_Exif::XMP()
{
    #if defined(MEDIAINFO_XMP_YES)
    File_Xmp MI{};
    Open_Buffer_Init(&MI);
    auto Element_Offset_Sav = Element_Offset;
    Open_Buffer_Continue(&MI);
    Element_Offset = Element_Offset_Sav;
    Open_Buffer_Finalize(&MI);
    Element_Show(); //TODO: why is it needed?
    Merge(MI, Stream_General, 0, 0, false);
    #endif
    Skip_UTF8(Element_Size - Element_Offset,                    "XMP metadata");
}

//---------------------------------------------------------------------------
void File_Exif::PhotoshopImageResources()
{
    #if defined(MEDIAINFO_PSD_YES)
    File_Psd MI{};
    MI.Step = File_Psd::Step_ImageResourcesBlock;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0, false);
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "Photoshop Tags");
    #endif
}

//---------------------------------------------------------------------------
void File_Exif::IPTC_NAA()
{
    //Parsing
    #if defined(MEDIAINFO_IIM_YES)
    File_Iim MI;
    Open_Buffer_Init(&MI);
    Open_Buffer_Continue(&MI);
    Open_Buffer_Finalize(&MI);
    Merge(MI, Stream_General, 0, 0, false);
    #else
    Skip_UTF8(Element_Size - Element_Offset,                    "IPTC-NAA data");
    #endif
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Exif::Get_X2(int16u& Info, const char* Name)
{
    if (LittleEndian)
        Get_L2(Info, Name);
    else
        Get_B2(Info, Name);
}

//---------------------------------------------------------------------------
void File_Exif::Get_X4(int32u& Info, const char* Name)
{
    if (LittleEndian)
        Get_L4(Info, Name);
    else
        Get_B4(Info, Name);
}

//---------------------------------------------------------------------------
void File_Exif::Get_IFDOffset(int8u KindOfIFD)
{
    int32u IFDOffset;
    Get_X4 (IFDOffset,                                          "IFD offset");
    if (IFDOffset && (IFDOffset < 8 || IFDOffset > File_Size - File_Offset))
        IFDOffset = 0;
    if (KindOfIFD != (int8u)-1 && IFDOffset)
        IFD_Offsets[IFDOffset] = KindOfIFD;
}

//---------------------------------------------------------------------------
void File_Exif::GetValueOffsetu(ifditem &IfdItem)
{
    auto GetDecimalPlaces = [](int64s numerator, int64s denominator) -> int8u {
        numerator = abs(numerator);
        denominator = abs(denominator);
        if (denominator == 1)
            return 0;
        int8u count{ 1 };
        while (denominator > 10) {
            ++count;
            denominator /= 10;
        }
        if (denominator == 10)
            return count;
        if (numerator == 1)
            return count + 3;
        if (numerator == 10)
            return count + 2;
        if (numerator == 100)
            return count + 1;
        if (count < 3)
            return 3;
        return count;
    };

    ZtringList& Info = Infos[currentIFD][IfdItem.Tag]; Info.clear(); Info.Separator_Set(0, __T(" /"));

    if (IfdItem.Type!=Exif_Type::BYTE && IfdItem.Type!=Exif_Type::ASCII && IfdItem.Type!=Exif_Type::UTF8 && IfdItem.Type!=Exif_Type::UNDEFINED && IfdItem.Count>=1000)
    {
        //Too many data, we don't currently need it and we skip it
        Skip_XX(static_cast<int64u>(Exif_Type_Size(IfdItem.Type))*IfdItem.Count, "Data");
        return;
    }

    auto End = Element_Offset + static_cast<int64u>(Exif_Type_Size(IfdItem.Type)) * IfdItem.Count;
    switch (IfdItem.Type)
    {
    case Exif_Type::BYTE:                                       /* 8-bit unsigned integer. */
        switch (currentIFD) {
        case Kind_IFD0:
        case Kind_IFD2:
        case Kind_SubIFD0:
        case Kind_SubIFD1:
        case Kind_SubIFD2:
            switch (IfdItem.Tag) {
            case IFD0::XMP:
                XMP();
                break;
            case IFD0::PhotoshopImageResources:
                PhotoshopImageResources();
                break;
            default:
                if (IfdItem.Tag >= IFD0::WinExpTitle && IfdItem.Tag <= IFD0::WinExpSubject) {
                    // Content is actually UTF16LE
                    Ztring Data;
                    Get_UTF16L(IfdItem.Count, Data,             "Data");
                    Info.push_back(Data);
                }
                break;
            }
            break;
        }
        if (Element_Offset < End) {
            if (IfdItem.Count <= 16) {
            for (int16u Pos=0; Pos<IfdItem.Count; Pos++)
            {
                int8u Ret8;
                #if MEDIAINFO_TRACE
                    Get_B1 (Ret8,                               "Data"); //L1 and B1 are same
                    Element_Info1(Ztring::ToZtring(Ret8));
                #else //MEDIAINFO_TRACE
                    if (Element_Offset+1>Element_Size)
                    {
                        Trusted_IsNot();
                        break;
                    }
                    Ret8=BigEndian2int8u(Buffer+Buffer_Offset+(size_t)Element_Offset); //LittleEndian2int8u and BigEndian2int8u are same
                    Element_Offset++;
                #endif //MEDIAINFO_TRACE
                Param_Info1C(currentIFD == Kind_GPS && IfdItem.Tag == IFDGPS::GPSAltitudeRef, Exif_IFDGPS_GPSAltitudeRef_Name(Ret8));
                Info.push_back(Ztring::ToZtring(Ret8));
            }
            }
        }
        break;
    case Exif_Type::ASCII:                                      /* ASCII */
        {
            string Data;
            Get_String(IfdItem.Count, Data,                     "Data"); Element_Info1(Data.c_str());
            Info.push_back(Ztring().From_UTF8(Data.c_str()));
        }
        break;
    case Exif_Type::UTF8:                                       /* UTF-8 */
        {
            Ztring Data;
            Get_UTF8(IfdItem.Count, Data,                       "Data"); Element_Info1(Data.To_UTF8().c_str());
            Info.push_back(Data);
        }
        break;
    case Exif_Type::SHORT:                                      /* 16-bit (2-byte) unsigned integer. */
        {
        auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
        for (int16u Pos=0; Pos<Count; Pos++)
        {
            int16u Ret16;
            #if MEDIAINFO_TRACE
                Get_X2 (Ret16,                                  "Data");
                switch (IfdItem.Tag)
                {
                            
                    default : Element_Info1(Ztring::ToZtring(Ret16));
                }
            #else //MEDIAINFO_TRACE
                if (Element_Offset+2>Element_Size)
                {
                    Trusted_IsNot();
                    break;
                }
                if (LittleEndian)
                    Ret16=LittleEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                else
                    Ret16=BigEndian2int16u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                Element_Offset+=2;
            #endif //MEDIAINFO_TRACE
            Param_Info1C((currentIFD == Kind_IFD0 || currentIFD == Kind_IFD1) && IfdItem.Tag == IFD0::Orientation, Exif_IFD0_Orientation_Name(Ret16));
            Param_Info1C((currentIFD == Kind_IFD0 || currentIFD == Kind_IFD1 || currentIFD == Kind_IFD2 || currentIFD == Kind_SubIFD0 || currentIFD == Kind_SubIFD1 || currentIFD == Kind_SubIFD2) && IfdItem.Tag == IFD0::Compression, Tiff_Compression_Name(Ret16));
            Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::LightSource, Exif_ExifIFD_Tag_LightSource_Name(Ret16));
            Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::Flash, Exif_IFDExif_Flash_Name(Ret16));
            Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::ColorSpace, Exif_IFDExif_ColorSpace_Name(Ret16));
            Param_Info1C(currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::CompositeImage, Exif_IFDExif_CompositeImage_Name(Ret16));
            Param_Info1C(currentIFD == Kind_MakernoteNikon && IfdItem.Tag == IFDMakernoteNikon::CropHiSpeed, Exif_IFDMakernoteNikon_CropHiSpeed_Name(Ret16));
            Param_Info1C(currentIFD == Kind_MakernoteNikon && IfdItem.Tag == IFDMakernoteNikon::ColorSpace, Exif_IFDMakernoteNikon_ColorSpace_Name(Ret16));
            Param_Info1C(currentIFD == Kind_MakernoteNikon && IfdItem.Tag == IFDMakernoteNikon::ActiveDLighting, Exif_IFDMakernoteNikon_ActiveDLighting_Name(Ret16));
            Info.push_back(Ztring::ToZtring(Ret16));
        }
        if (Count != IfdItem.Count) {
            Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 2, "(Not parsed)");
        }
        }
        break;
    case Exif_Type::IFD:                                        /* 32-bit (4-byte) unsigned integer IFD offset */
    case Exif_Type::LONG:                                       /* 32-bit (4-byte) unsigned integer */
        switch (currentIFD) {
        case Kind_IFD0:
        case Kind_IFD2:
        case Kind_SubIFD0:
        case Kind_SubIFD1:
        case Kind_SubIFD2:
            switch (IfdItem.Tag) {
            case IFD0::IPTC_NAA:
                IPTC_NAA();
                break;
            }
            break;
        }
        if (Element_Offset < End) {
            auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
            for (int16u Pos=0; Pos<Count; Pos++)
            {
                int32u Ret32;
                if (currentIFD == Kind_IFD0 && IfdItem.Tag == IFD0::SubIFDs) {
                    if (Pos < 3)
                        Get_IFDOffset(Kind_SubIFD0 + Pos);
                    else
                        Get_X4(Ret32,                           "IFD Offset");
                    continue;
                }
                #if MEDIAINFO_TRACE
                    Get_X4 (Ret32, IfdItem.Type == Exif_Type::IFD ? "IFD Offset" : "Data");
                    Element_Info1(Ztring::ToZtring(Ret32));
                #else //MEDIAINFO_TRACE
                    if (Element_Offset+4>Element_Size)
                    {
                        Trusted_IsNot();
                        break;
                    }
                    if (LittleEndian)
                        Ret32=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    else
                        Ret32=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    Element_Offset+=4;
                #endif //MEDIAINFO_TRACE
                Param_Info1C(currentIFD == Kind_MakernoteSony && IfdItem.Tag == IFDMakernoteSony::Quality, Exif_IFDMakernoteSony_Quality_Name(Ret32));
                Param_Info1C(currentIFD == Kind_MakernoteSony && IfdItem.Tag == IFDMakernoteSony::WhiteBalance, Exif_IFDMakernoteSony_WhiteBalance_Name(Ret32));
                Info.push_back(Ztring::ToZtring(Ret32));
            }
            if (Count != IfdItem.Count) {
                Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 4, "(Not parsed)");
            }
        }
        break;
    case Exif_Type::RATIONAL:                                   /* 2x32-bit (2x4-byte) unsigned integers */
        {
        auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
        for (int16u Pos=0; Pos<Count; Pos++)
        {
            int32u N, D;
            #if MEDIAINFO_TRACE
                Get_X4 (N,                                      "Numerator");
                Get_X4 (D,                                      "Denominator");
                if (D)
                    Element_Info1(Ztring::ToZtring(static_cast<float64>(N) / D, GetDecimalPlaces(N, D)));
            #else //MEDIAINFO_TRACE
                if (Element_Offset+8>Element_Size)
                {
                    Trusted_IsNot();
                    break;
                }
                if (LittleEndian)
                {
                    N=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    D=LittleEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset+4);
                }
                else
                {
                    N=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    D=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset+4);
                }
                Element_Offset+=8;
            #endif //MEDIAINFO_TRACE
            if (D)
                Info.push_back(Ztring::ToZtring(static_cast<float64>(N) / D, GetDecimalPlaces(N, D)));
            else
                Info.push_back(Ztring()); // Division by zero, undefined
        }
        if (Count != IfdItem.Count) {
            Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 8, "(Not parsed)");
        }
        }
        break;
    case Exif_Type::SSHORT:                                     /* 16-bit (2-byte) signed integer. */
        {
        auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
        for (int16u Pos=0; Pos<Count; Pos++)
        {
            int16u Ret16u;
            int16s Ret16;
            #if MEDIAINFO_TRACE
                Get_X2 (Ret16u,                                 "Data");
                Ret16 = static_cast<int16s>(Ret16u);
                Element_Info1(Ztring::ToZtring(Ret16));
            #else //MEDIAINFO_TRACE
                if (Element_Offset+2>Element_Size)
                {
                    Trusted_IsNot();
                    break;
                }
                if (LittleEndian)
                    Ret16=LittleEndian2int16s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                else
                    Ret16=BigEndian2int16s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                Element_Offset+=2;
            #endif //MEDIAINFO_TRACE
            Info.push_back(Ztring::ToZtring(Ret16));
        }
        if (Count != IfdItem.Count) {
            Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 2, "(Not parsed)");
        }
        }
        break;
    case Exif_Type::SLONG:                                      /* 32-bit (4-byte) signed integer */
        {
        auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
        for (int16u Pos=0; Pos<Count; Pos++)
        {
            int32u Ret32u;
            int32s Ret32;
            #if MEDIAINFO_TRACE
                Get_X4 (Ret32u,                                 "Data");
                Ret32 = static_cast<int32s>(Ret32u);
                Element_Info1(Ztring::ToZtring(Ret32));
            #else //MEDIAINFO_TRACE
                if (Element_Offset+4>Element_Size)
                {
                    Trusted_IsNot();
                    break;
                }
                if (LittleEndian)
                    Ret32=LittleEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                else
                    Ret32=BigEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                Element_Offset+=4;
            #endif //MEDIAINFO_TRACE
            Param_Info1C(currentIFD == Kind_MakernoteApple && IfdItem.Tag == IFDMakernoteApple::ImageCaptureType, Exif_IFDMakernoteApple_ImageCaptureType_Name(Ret32));
            Param_Info1C(currentIFD == Kind_MakernoteApple && IfdItem.Tag == IFDMakernoteApple::CameraType, Exif_IFDMakernoteApple_CameraType_Name(Ret32));
            Info.push_back(Ztring::ToZtring(Ret32));
        }
        if (Count != IfdItem.Count) {
            Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 4, "(Not parsed)");
        }
        }
        break;
    case Exif_Type::SRATIONAL:                                  /* 2x32-bit (2x4-byte) signed integers */
        {
        auto Count = IfdItem.Count > 16 ? 16 : IfdItem.Count;
        for (int16u Pos=0; Pos<Count; Pos++)
        {
            int32u NU, DU;
            int32s N, D;
            #if MEDIAINFO_TRACE
                Get_X4 (NU,                                     "Numerator");
                Get_X4 (DU,                                     "Denominator");
                N = static_cast<int32s>(NU);
                D = static_cast<int32s>(DU);
                if (D)
                    Element_Info1(Ztring::ToZtring(static_cast<float64>(N) / D, GetDecimalPlaces(N, D)));
            #else //MEDIAINFO_TRACE
                if (Element_Offset+8>Element_Size)
                {
                    Trusted_IsNot();
                    break;
                }
                if (LittleEndian)
                {
                    N=LittleEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    D=LittleEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset+4);
                }
                else
                {
                    N=BigEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset);
                    D=BigEndian2int32s(Buffer+Buffer_Offset+(size_t)Element_Offset+4);
                }
                Element_Offset+=8;
            #endif //MEDIAINFO_TRACE
            if (D)
                Info.push_back(Ztring::ToZtring(static_cast<float64>(N) / D, GetDecimalPlaces(N, D)));
            else
                Info.push_back(Ztring()); // Division by zero, undefined
        }
        if (Count != IfdItem.Count) {
            Skip_XX((IfdItem.Count - static_cast<int64u>(Count)) * 8, "(Not parsed)");
        }
        }
        break;
    case Exif_Type::UNDEFINED:                                  /* Undefined */
        if (
            (currentIFD == Kind_Exif && IfdItem.Tag == IFDExif::UserComment) ||
            (currentIFD == Kind_GPS && (IfdItem.Tag == IFDGPS::GPSProcessingMethod || IfdItem.Tag == IFDGPS::GPSAreaInformation))
            ) {
            MulticodeString(Info);
            break;
        }
        if (
            (currentIFD == Kind_Exif && (IfdItem.Tag == IFDExif::ExifVersion || IfdItem.Tag == IFDExif::FlashpixVersion)) ||
            (currentIFD == Kind_Interop && IfdItem.Tag == IFDInterop::InteroperabilityVersion) ||
            (currentIFD == Kind_MPF && IfdItem.Tag == IFDMPF::MPFVersion)
            ) {
            string Data;
            Get_String(IfdItem.Count, Data,                     "Data"); Element_Info1(Data.c_str());
            Info.push_back(Ztring().From_UTF8(Data.c_str()));
            break;
        }
        switch (currentIFD) {
        case Kind_IFD0:
        case Kind_IFD2:
        case Kind_SubIFD0:
        case Kind_SubIFD1:
        case Kind_SubIFD2:
            switch (IfdItem.Tag) {
            case IFD0::XMP:
                XMP();
                break;
            case IFD0::PhotoshopImageResources:
                PhotoshopImageResources();
                break;
            case IFD0::ICC_Profile:
                ICC_Profile();
                break;
            }
            break;
        case Kind_Exif:
            switch (IfdItem.Tag) {
            case IFDExif::MakerNote:
                if (IfdItem.Count > 4)
                    Makernote();
                break;
            }
            break;
        case Kind_MPF:
            switch (IfdItem.Tag) {
            case IFDMPF::MPEntry:
            {
                int32u num_imgs{ Infos.find(Kind_MPF)->second.find(IFDMPF::NumberOfImages)->second.Read().To_int32u() };
                for (int32u i = 0; i < num_imgs; ++i) {
                    Element_Begin1(("MP Entry " + std::to_string(i + 1)).c_str());
                    mp_entry entry{};
                    Get_X4(entry.ImgAttribute,                  "Individual image Attribute"); Param_Info1(Mpf_ImageAttribute_Desc(entry.ImgAttribute));
                    Get_X4(entry.ImgSize,                       "Individual image Size");
                    Get_X4(entry.ImgOffset,                     "Individual image Data Offset");
                    Get_X2(entry.DependentImg1EntryNo,          "Dependent image 1 Entry No.");
                    Get_X2(entry.DependentImg2EntryNo,          "Dependent image 2 Entry No.");
                    MPEntries->push_back(entry);
                    Element_End0();
                }
                break;
            }
            case IFDMPF::ImageUIDList:
            {
                int32u num_imgs{ Infos.find(Kind_MPF)->second.find(IFDMPF::NumberOfImages)->second.Read().To_int32u() };
                for (int32u i = 0; i < num_imgs; ++i) {
                    string Data;
                    Get_String(33, Data,                        "Individual Image Unique ID"); Element_Info1(Data.c_str());
                    Info.push_back(Ztring().From_UTF8(Data.c_str()));
                }
                break;
            }
            }
            break;
        }
        break;
    }
    if (Element_Offset > End) {
        Element_Offset = End; // There was a problem during parsing
    }
    if (Element_Offset < End) {
        Skip_XX(End - Element_Offset,                           "(Unknown)");
    }
}

//---------------------------------------------------------------------------
void File_Exif::GoToOffset(int64u GoTo_)
{
    Element_Offset = GoTo_ - (Buffer_Offset - OffsetFromContainer);
    if (!IsSub) {
        Element_Offset -= File_Offset;
    }
}

} //NameSpace

#endif //MEDIAINFO_EXIF_YES
