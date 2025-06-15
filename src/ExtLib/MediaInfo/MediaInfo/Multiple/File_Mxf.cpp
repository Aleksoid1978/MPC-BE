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
#if defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mxf.h"
#include "MediaInfo/Multiple/File_Mxf_Automated.h"
#include "MediaInfo/Video/File_DolbyVisionMetadata.h"
#include "MediaInfo/Video/File_HdrVividMetadata.h"
#include "MediaInfo/Audio/File_DolbyAudioMetadata.h"
#if defined(MEDIAINFO_DVDIF_YES)
    #include "MediaInfo/Multiple/File_DvDif.h"
#endif
#if defined(MEDIAINFO_DVDIF_YES)
    #include "MediaInfo/Multiple/File_DvDif.h"
#endif
#if defined(MEDIAINFO_VBI_YES)
    #include "MediaInfo/Multiple/File_Vbi.h"
#endif
#if defined(MEDIAINFO_AVC_YES)
    #include "MediaInfo/Video/File_Avc.h"
#endif
#if defined(MEDIAINFO_FFV1_YES)
    #include "MediaInfo/Video/File_Ffv1.h"
#endif
#if defined(MEDIAINFO_HEVC_YES)
    #include "MediaInfo/Video/File_Hevc.h"
#endif
#if defined(MEDIAINFO_MPEG4V_YES)
    #include "MediaInfo/Video/File_Mpeg4v.h"
#endif
#if defined(MEDIAINFO_MPEGV_YES)
    #include "MediaInfo/Video/File_Mpegv.h"
#endif
#if defined(MEDIAINFO_PRORES_YES)
    #include "MediaInfo/Video/File_ProRes.h"
#endif
#if defined(MEDIAINFO_VC1_YES)
    #include "MediaInfo/Video/File_Vc1.h"
#endif
#if defined(MEDIAINFO_VC3_YES)
    #include "MediaInfo/Video/File_Vc3.h"
#endif
#if defined(MEDIAINFO_AAC_YES)
    #include "MediaInfo/Audio/File_Aac.h"
#endif
#if defined(MEDIAINFO_AC3_YES)
    #include "MediaInfo/Audio/File_Ac3.h"
#endif
#if defined(MEDIAINFO_ADM_YES)
    #include "MediaInfo/Audio/File_Adm.h"
#endif
#if defined(MEDIAINFO_IAB_YES)
    #include "MediaInfo/Audio/File_Iab.h"
#endif
#if defined(MEDIAINFO_MGA_YES)
    #include "MediaInfo/Audio/File_Mga.h"
#endif
#if defined(MEDIAINFO_SMPTEST0337_YES)
    #include "MediaInfo/Audio/File_ChannelGrouping.h"
#endif
#if defined(MEDIAINFO_SMPTEST0337_YES)
    #include "MediaInfo/Audio/File_ChannelSplitting.h"
#endif
#if defined(MEDIAINFO_MPEGA_YES)
    #include "MediaInfo/Audio/File_Mpega.h"
#endif
#if defined(MEDIAINFO_PCM_YES)
    #include "MediaInfo/Audio/File_Pcm.h"
#endif
#if defined(MEDIAINFO_SMPTEST0331_YES)
    #include "MediaInfo/Audio/File_SmpteSt0331.h"
#endif
#if defined(MEDIAINFO_SMPTEST0337_YES)
    #include "MediaInfo/Audio/File_SmpteSt0337.h"
#endif
#if defined(MEDIAINFO_JPEG_YES)
    #include "MediaInfo/Image/File_Jpeg.h"
#endif
#if defined(MEDIAINFO_TTML_YES)
    #include "MediaInfo/Text/File_Ttml.h"
#endif
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/File_Unknown.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/FileName.h"
#include "MediaInfo/MediaInfo_Internal.h"
#if defined(MEDIAINFO_REFERENCES_YES)
    #include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/Format/Http/Http_Utils.h"
#include <cfloat>
#include <cmath>
#if MEDIAINFO_ADVANCED
    #include <limits>
#endif //MEDIAINFO_ADVANCED
#if MEDIAINFO_SEEK
    #include <algorithm>
#endif //MEDIAINFO_SEEK
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

#if defined(MEDIAINFO_IAB_YES)
    Ztring ChannelLayout_2018_Rename(const Ztring& Channels, const Ztring& Format);
#endif

//***************************************************************************
//
//  PartitionPack
//  Primer
//  Preface
//      --> ContentStorage
//              --> Packages --> Package (Material, Source)
//                      --> Tracks --> Track
//                              --> Sequence
//                                      --> StructuralComponents --> StructuralComponent (Timecode, SourceClip)
//                      --> Descriptors --> Descriptor (Multiple, Essence)
//                              --> Descriptors --> Descriptor (Essence)
//              --> EssenceContainerData
//              --> Identifications --> Identification
//
//***************************************************************************

//***************************************************************************
// Constants
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_TRACE
    static const size_t MaxCountSameElementInTrace=10;
#endif // MEDIAINFO_TRACE

//---------------------------------------------------------------------------
#define VECTOR(LENGTH) \
    auto Count=Vector(LENGTH); \
    if (Count==(int32u)-1) \
        return; \

//---------------------------------------------------------------------------
extern const char* Mpegv_profile_and_level_indication_profile[];
extern const char* Mpegv_profile_and_level_indication_level[];
extern const char* Mpeg4v_Profile_Level(int32u Profile_Level);
extern string Avc_profile_level_string(int8u profile_idc, int8u level_idc=0, int8u constraint_set_flags=0);
extern string Jpeg2000_Rsiz(int16u Rsiz);

//---------------------------------------------------------------------------
extern const char* AfdBarData_active_format[];
extern const char* AfdBarData_active_format_4_3[];
extern const char* AfdBarData_active_format_16_9[];

//---------------------------------------------------------------------------
static const char* Mxf_Category(int8u Category)
{
    switch(Category)
    {
        case 0x01 : return "Dictionary";
        case 0x02 : return "Group";
        case 0x03 : return "Wrapper";
        case 0x04 : return "Label";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_Registry(int16u CategoryRegistry) // SMPTE ST 336
{
    switch (CategoryRegistry)
    {
        // Dictionary
        case 0x0101 : return "Metadata";
        case 0x0102 : return "Essence";
        // Group
        case 0x0204 : return "Variable-Length Packs";
        case 0x0205 : return "Defined-Length Packs";
        case 0x0243 : return "2-byte length, 1-byte tag";
        case 0x0253 : return "2-byte length, 2-byte tag";
        case 0x0263 : return "4-byte length, 1-byte tag";
        case 0x0273 : return "4-byte length, 2-byte tag";
        // Label
        case 0x0401 : return "Fixed";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_MPEG2_CodedContentType(int8u CodedContentType)
{
    switch(CodedContentType)
    {
        case 0x01 : return "Progressive";
        case 0x02 : return "Interlaced";
        case 0x03 : return ""; //Mixed
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_AVC_SequenceParameterSetFlag_Constancy(bool Constancy)
{
    if (Constancy)
        return "";
    else
        return "Constant";
}

//---------------------------------------------------------------------------
static const char* Mxf_OperationalPattern(const int128u OperationalPattern)
{
    //Item and Package Complexity
    int32u Code_Compare4=(int32u)OperationalPattern.lo;
    switch ((int8u)(Code_Compare4>>24))
    {
        case 0x01 : switch ((int8u)(Code_Compare4>>16))
                    {
                        case 0x01 : return "OP-1a";
                        case 0x02 : return "OP-1b";
                        case 0x03 : return "OP-1c";
                        default   : return "";
                    }
        case 0x02 : switch ((int8u)(Code_Compare4>>16))
                    {
                        case 0x01 : return "OP-2a";
                        case 0x02 : return "OP-2b";
                        case 0x03 : return "OP-2c";
                        default   : return "";
                    }
        case 0x03 : switch ((int8u)(Code_Compare4>>16))
                    {
                        case 0x01 : return "OP-3a";
                        case 0x02 : return "OP-3b";
                        case 0x03 : return "OP-3c";
                        default   : return "";
                    }
        case 0x10 : return "OP-Atom";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_EssenceContainer(const int128u EssenceContainer)
{
    if ((EssenceContainer.hi&0xFFFFFFFFFFFFFF00LL)!=0x060E2B3404010100LL)
        return "";

    int8u Code1=(int8u)((EssenceContainer.lo&0xFF00000000000000LL)>>56);
    int8u Code2=(int8u)((EssenceContainer.lo&0x00FF000000000000LL)>>48);
    int8u Code3=(int8u)((EssenceContainer.lo&0x0000FF0000000000LL)>>40);
    int8u Code4=(int8u)((EssenceContainer.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((EssenceContainer.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((EssenceContainer.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((EssenceContainer.lo&0x000000000000FF00LL)>> 8);

    switch (Code1)
    {
        case 0x0D : //Public Use
                    switch (Code2)
                    {
                        case 0x01 : //AAF
                                    switch (Code3)
                                    {
                                        case 0x03 : //Essence Container Application
                                                    switch (Code4)
                                                    {
                                                        case 0x01 : //MXF EC Structure version
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x02 : //Essence container kind
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x01 : return "D-10"; // Video and Audio
                                                                                        case 0x02 : return "DV";
                                                                                        case 0x05 : return "Uncompressed pictures";
                                                                                        case 0x06 : return "PCM";
                                                                                        case 0x04 : return "MPEG ES mappings with Stream ID";
                                                                                        case 0x0A : return "A-law";
                                                                                        case 0x0C : return "JPEG 2000";
                                                                                        case 0x10 : return "AVC";
                                                                                        case 0x11 : return "VC-3";
                                                                                        case 0x13 : return "Timed Text";
                                                                                        case 0x16 : return "AAC (ADIF)";
                                                                                        case 0x17 : return "AAC (ADTS)";
                                                                                        case 0x18 : return "AAC (LATM/LOAS)";
                                                                                        case 0x1C : return "ProRes";
                                                                                        case 0x1D : return "IAB";
                                                                                        case 0x23 : return "FFV1";
                                                                                        case 0x25 : return "MGA";
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        default   : return "";
                    }
        case 0x0E : //Private Use
                    switch (Code2)
                    {
                        case 0x04 : //Avid
                                    switch (Code3)
                                    {
                                        case 0x03 : //Essence Container Application
                                                    switch (Code4)
                                                    {
                                                        case 0x01 : //MXF EC Structure version
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x02 : //Essence container kind
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x06 : return "VC-3";
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        case 0x06 : //Sony
                                    switch (Code3)
                                    {
                                        case 0x0D :
                                                    switch (Code4)
                                                    {
                                                        case 0x03 :
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x02 :
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x01 :
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "Sony RAW?";
                                                                                                        default   : return "";
                                                                                                    }
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        default   : return "";
                    }
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_EssenceContainer_Mapping(int64u Code_lo)
{
    switch (Code_lo & 0xFFFFFFFFFFFF0000) {
    case Labels::MXFGCSMPTED10Mappings:
        return "Frame (D-10)";
    case Labels::MXFGCSMPTED11Mappings:
    case Labels::MXFGCGenericVBIDataMappingUndefinedPayload:
    case Labels::MXFGCGenericANCDataMappingUndefinedPayload:
        return "Frame";
    case Labels::MXFGCDVDIFMappings:
        switch ((int8u)Code_lo) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "?";
        }
        break;
    case Labels::MXFGCMPEGES:
    case Labels::MXFGCMPEGPES:
    case Labels::MXFGCMPEGPS:
    case Labels::MXFGCMPEGTS:
    case Labels::MXFGCAVCNALUnitStream:
    case Labels::MXFGCAVCByteStream:
    case Labels::MXFGCHEVCNALUnitStream:
    case Labels::MXFGCHEVCByteStream:
        switch ((int8u)Code_lo) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "Custom: Stripe";
        case 0x04: return "Custom: PES";
        case 0x05: return "Custom: Fixed Audio Size";
        case 0x06: return "Custom: Splice";
        case 0x07: return "Custom: Closed GOP";
        case 0x08: return "Custom: Slave";
        }
        break;
    case Labels::MXFGCUncompressedPictures:
        switch ((int8u)Code_lo) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "Line";
        }
        break;
    case Labels::MXFGCAESBWFAudio:
        switch ((int8u)(Code_lo >> 8)) {
        case 0x01: return "Frame (BWF)";
        case 0x02: return "Clip (BWF)";
        case 0x03: return "Frame (AES)";
        case 0x04: return "Clip (AES)";
        case 0x08: return "Custom (BWF)";
        case 0x09: return "Custom (AES)";
        }
        break;
    case Labels::MXFGCALawAudioMappings:
    case Labels::MXFGCEncryptedDataMappings:
    case Labels::MXFGCJPEG2000PictureMappings:
    case Labels::MXFGCVC3Pictures:
    case Labels::MXFGCVC1Pictures:
    case Labels::MXFGCTIFFEP:
    case Labels::MXFGCVC2Pictures:
    case Labels::MXFGCACESPictures:
    case Labels::MXFGCEssenceContainerDNxPacked:
    case Labels::MXFGCFFV1Pictures:
    case Labels::AvidTechnologyIncVC3Pictures:
    case Labels::SonyCorporationRAWSQ:
        switch ((int8u)(Code_lo >> 8)) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "?";
        }
        break;
    case Labels::MXF_GC_AAC_ADIF:
    case Labels::MXF_GC_AAC_ADTS:
    case Labels::MXF_GC_AAC_LATM_LOAS:
    case Labels::MXFGCVC5EssenceContainerLabel:
        switch ((int8u)(Code_lo >> 8)) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "Custom";
        }
        break;
    case Labels::MXFGCDMCVTData:
    case Labels::MXFGCEssenceContainerProResPicture:
    case Labels::MXFGCMGAAudioMappings:
        switch ((int8u)(Code_lo >> 8)) {
        case 0x01: return "Frame";
        }
        break;
    case Labels::MXFGCGenericData:
    case Labels::MXFGCImmersiveAudio:
        switch ((int16u)Code_lo) {
        case 0x0101: return "Clip";
        case 0x0102: return "Frame";
        }
        break;
    case Labels::MXFGCJPEGXSPictures:
        switch ((int8u)(Code_lo >> 8)) {
        case 0x01: return "Frame";
        case 0x02: return "Clip";
        case 0x03: return "Custom";
        }
        break;
    case 0x0e09060701010000:
        switch ((int16u)Code_lo) {
        case 0x0103: return "Frame";
        }
        break;
    }

    return "";
}

//---------------------------------------------------------------------------
static const char* Mxf_EssenceCompression(const int128u EssenceCompression)
{
    if ((EssenceCompression.hi&0xFFFFFFFFFFFFFF00LL)!=0x060E2B3404010100LL || !((EssenceCompression.lo&0xFF00000000000000LL)==0x0400000000000000LL || (EssenceCompression.lo&0xFF00000000000000LL)==0x0E00000000000000LL))
        return "";

    int8u Code1=(int8u)((EssenceCompression.lo&0xFF00000000000000LL)>>56);
    int8u Code2=(int8u)((EssenceCompression.lo&0x00FF000000000000LL)>>48);
    int8u Code3=(int8u)((EssenceCompression.lo&0x0000FF0000000000LL)>>40);
    int8u Code4=(int8u)((EssenceCompression.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((EssenceCompression.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((EssenceCompression.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((EssenceCompression.lo&0x000000000000FF00LL)>> 8);

    switch (Code1)
    {
        case 0x04 : //
                    switch (Code2)
                    {
                        case 0x01 : //Picture
                                    switch (Code3)
                                    {
                                        case 0x02 : //Coding characteristics
                                                    switch (Code4)
                                                    {
                                                        case 0x01 : //Uncompressed coding
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x01 : //Uncompressed picture coding
                                                                                    return "YUV";
                                                                        default   : return "";
                                                                    }
                                                        case 0x02 : //Compressed coding
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x01 : //MPEG Compression
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x00 : return "MPEG Video";
                                                                                        case 0x01 : return "MPEG Video"; //Version 2
                                                                                        case 0x02 : return "MPEG Video"; //Version 2
                                                                                        case 0x03 : return "MPEG Video"; //Version 2
                                                                                        case 0x04 : return "MPEG Video"; //Version 2
                                                                                        case 0x11 : return "MPEG Video"; //Version 1
                                                                                        case 0x20 : return "MPEG-4 Visual";
                                                                                        case 0x30 :
                                                                                        case 0x31 :
                                                                                        case 0x32 :
                                                                                        case 0x33 :
                                                                                        case 0x34 :
                                                                                        case 0x35 :
                                                                                        case 0x36 :
                                                                                        case 0x37 :
                                                                                        case 0x38 :
                                                                                        case 0x39 :
                                                                                        case 0x3A :
                                                                                        case 0x3B :
                                                                                        case 0x3C :
                                                                                        case 0x3D :
                                                                                        case 0x3E :
                                                                                        case 0x3F : return "AVC";
                                                                                        default   : return "";
                                                                                    }
                                                                        case 0x02 : return "DV";
                                                                        case 0x03 : //Individual Picture Coding Schemes
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x01 : return "JPEG 2000";
                                                                                        case 0x06 : return "ProRes";
                                                                                        case 0x09 : return "FFV1";
                                                                                        default   : return "";
                                                                                    }
                                                                        case 0x71 : return "VC-3";
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        case 0x02 : //Sound
                                    switch (Code3)
                                    {
                                        case 0x02 : //Coding characteristics
                                                    switch (Code4)
                                                    {
                                                        case 0x01 : //Uncompressed Sound Coding
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x00 : return "PCM";
                                                                        case 0x01 : return "PCM";
                                                                        case 0x7E : return "PCM"; //AIFF
                                                                        case 0x7F : return "PCM"; // TODO: Undefined
                                                                        default   : return "";
                                                                    }
                                                        case 0x02 : //Compressed coding
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x03 : //Compressed Audio Coding
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x01 : //Compandeded Audio Coding
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "A-law";
                                                                                                        case 0x10 : return "DV Audio"; //DV 12-bit
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        case 0x02 : //SMPTE 338M Audio Coding
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "AC-3";
                                                                                                        case 0x04 : return "MPEG-1 Audio Layer 1";
                                                                                                        case 0x05 : return "MPEG-1 Audio Layer 2 or 3";
                                                                                                        case 0x06 : return "MPEG-2 Audio Layer 1";
                                                                                                        case 0x1C : return "Dolby E";
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        case 0x03 : //MPEG-2 Coding (not defined in SMPTE 338M)
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "AAC version 2";
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        case 0x04 : //MPEG-4 Audio Coding
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "MPEG-4 Speech Profile";
                                                                                                        case 0x02 : return "MPEG-4 Synthesis Profile";
                                                                                                        case 0x03 : return "MPEG-4 Scalable Profile";
                                                                                                        case 0x04 : return "MPEG-4 Main Profile";
                                                                                                        case 0x05 : return "MPEG-4 High Quality Audio Profile";
                                                                                                        case 0x06 : return "MPEG-4 Low Delay Audio Profile";
                                                                                                        case 0x07 : return "MPEG-4 Natural Audio Profile";
                                                                                                        case 0x08 : return "MPEG-4 Mobile Audio Internetworking Profile";
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        default   : return "";
                                                                                    }
                                                                        case 0x04 : //MPEG Audio Compression (SMPTE ST 381-4)
                                                                                    switch (Code6) {
                                                                                        case 0x03 : //MPEG-2 AAC
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "Low Complexity Profile MPEG-2 AAC";
                                                                                                        case 0x02 : return "Low Complexity Profile MPEG-2 AAC LC+SBR";
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        case 0x04 : //MPEG-4 AAC
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x01 : return "MPEG-4 AAC Profile";
                                                                                                        case 0x02 : return "MPEG-4 High Efficiency AAC Profile";
                                                                                                        case 0x03 : return "MPEG-4 High Efficiency AAC v2 Profile";
                                                                                                        default   : return ""; //Unknown
                                                                                                    }
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        default   : return "";
                    }
        case 0x0E : //Private Use
                    switch (Code2)
                    {
                        case 0x04 : //Avid
                                    switch (Code3)
                                    {
                                        case 0x02 : //Essence Compression ?
                                                    switch (Code4)
                                                    {
                                                        case 0x01 : //?
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x02 : //?
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x04 : return "VC-3";
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        case 0x06 : //Sony
                                    switch (Code3)
                                    {
                                        case 0x04 :
                                                    switch (Code4)
                                                    {
                                                        case 0x01 :
                                                                    switch (Code5)
                                                                    {
                                                                        case 0x02 :
                                                                                    switch (Code6)
                                                                                    {
                                                                                        case 0x04 :
                                                                                                    switch (Code7)
                                                                                                    {
                                                                                                        case 0x02 : return "Sony RAW SQ";
                                                                                                        default   : return "";
                                                                                                    }
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                         default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        case 0x09 : //Dolby
                                    switch (Code3)
                                    {
                                        case 0x06 :
                                                    switch (Code4)
                                                    {
                                                        case 0x04 : return "IAB";
                                                        default   : return "";
                                                    }
                                         default   : return "";
                                    }
                        default   : return "";
                    }
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_EssenceCompression_Profile(const int128u& EssenceCompression)
{
    int8u Code2=(int8u)((EssenceCompression.lo&0x00FF000000000000LL)>>48);
    int8u Code3=(int8u)((EssenceCompression.lo&0x0000FF0000000000LL)>>40);
    int8u Code4=(int8u)((EssenceCompression.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((EssenceCompression.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((EssenceCompression.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((EssenceCompression.lo&0x000000000000FF00LL)>> 8);
    int8u Code8=(int8u)((EssenceCompression.lo&0x00000000000000FFLL)    );

    switch (Code2)
    {
        case 0x01 : //Picture
                    switch (Code3)
                    {
                        case 0x02 : //Coding characteristics
                                    switch (Code4)
                                    {
                                        case 0x02 : //Compressed coding
                                                    switch (Code5)
                                                    {
                                                        case 0x01 : //MPEG Compression
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x20 : //MPEG-4 Visual
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x10 : //
                                                                                                    switch (Code8)
                                                                                                    {
                                                                                                        case 0x01 :
                                                                                                        case 0x02 :
                                                                                                        case 0x03 :
                                                                                                        case 0x04 :
                                                                                                                    return Mpeg4v_Profile_Level(B8(11100000)+Code8);
                                                                                                        case 0x05 :
                                                                                                        case 0x06 :
                                                                                                                    return Mpeg4v_Profile_Level(B8(11101011)-5+Code8);
                                                                                                        default   : return "";
                                                                                                    }
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                        case 0x03 : //Individual Picure Coding Schemes
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x06 : //ProRes
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : return "422 Proxy";
                                                                                        case 0x02 : return "422 LT";
                                                                                        case 0x03 : return "422";
                                                                                        case 0x04 : return "422 HQ";
                                                                                        case 0x05 : return "4444";
                                                                                        case 0x06 : return "4444 XQ";
                                                                                        default   : return "";
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                        default   : return "";
                                                    }
                                         default   : return "";
                                    }
                         default   : return "";
                    }
        default   : return "";
    }
}
//---------------------------------------------------------------------------
static const char* Mxf_EssenceCompression_Version(const int128u& EssenceCompression)
{
    int8u Code2=(int8u)((EssenceCompression.lo&0x00FF000000000000LL)>>48);
    int8u Code3=(int8u)((EssenceCompression.lo&0x0000FF0000000000LL)>>40);
    int8u Code4=(int8u)((EssenceCompression.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((EssenceCompression.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((EssenceCompression.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((EssenceCompression.lo&0x000000000000FF00LL)>> 8);

    switch (Code2)
    {
        case 0x01 : //Picture
                    switch (Code3)
                    {
                        case 0x02 : //Coding characteristics
                                    switch (Code4)
                                    {
                                        case 0x02 : //Compressed coding
                                                    switch (Code5)
                                                    {
                                                        case 0x01 : //MPEG Compression
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x01 : return "Version 2";
                                                                        case 0x02 : return "Version 2";
                                                                        case 0x03 : return "Version 2";
                                                                        case 0x04 : return "Version 2";
                                                                        case 0x11 : return "Version 1";
                                                                        default   : return "";
                                                                    }
                                                        default   : return "";
                                                    }
                                         default   : return "";
                                    }
                         default   : return "";
                    }
        case 0x02 : //Sound
                    switch (Code3)
                    {
                        case 0x02 : //Coding characteristics
                                    switch (Code4)
                                    {
                                        case 0x02 : //Compressed coding
                                                    switch (Code5)
                                                    {
                                                        case 0x03 : //Compressed Audio Coding
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x02 : //SMPTE 338M Audio Coding
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x04 : return "Version 1"; //Layer 1
                                                                                        case 0x05 : return "Version 1"; //Layer 2 or 3
                                                                                        case 0x06 : return "Version 2"; //Layer 1
                                                                                        default   : return ""; //Unknown
                                                                                    }
                                                                        default   : return "";
                                                                    }

                                                        case 0x04 : //MPEG Audio Compression (SMPTE ST 381-4)
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x03 : //MPEG-2 AAC
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : return "Version 4";
                                                                                        case 0x02 : return "Version 4";
                                                                                        default   : return ""; //Unknown
                                                                                    }
                                                                        case 0x04 : //MPEG-4 AAC
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : return "Version 4";
                                                                                        case 0x02 : return "Version 4";
                                                                                        case 0x03 : return "Version 4";
                                                                                        default   : return ""; //Unknown
                                                                                    }
                                                                        default   : return "";
                                                                    }
                                                            default   : return "";
                                                    }
                                            default   : return "";
                                    }
                            default   : return "";
                    }
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_Sequence_DataDefinition(const int128u DataDefinition)
{
    int8u Code4=(int8u)((DataDefinition.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((DataDefinition.lo&0x00000000FF000000LL)>>24);

    switch (Code4)
    {
        case 0x01 :
                    switch (Code5)
                    {
                        case 0x01 :
                        case 0x02 :
                        case 0x03 : return "Time";
                        case 0x10 : return "Descriptive Metadata";
                        default   : return "";
                    }
        case 0x02 :
                    switch (Code5)
                    {
                        case 0x01 : return "Picture";
                        case 0x02 : return "Sound";
                        case 0x03 : return "Data";
                        default   : return "";
                    }
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_FrameLayout(int8u FrameLayout)
{
    switch (FrameLayout)
    {
        case 0x00 : return "Full frame";
        case 0x01 : return "Separated fields";
        case 0x02 : return "Single field";
        case 0x03 : return "Mixed fields";
        case 0x04 : return "Segmented frame";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_FrameLayout_ScanType(int8u FrameLayout)
{
    switch (FrameLayout)
    {
        case 0x01 :
        case 0x04 :
        case 0xFF : //Seen in one file
                    return "Interlaced";
        default   :
                    return "Progressive";
    }
}

//---------------------------------------------------------------------------
static int8u Mxf_FrameLayout_Multiplier(int8u FrameLayout)
{
    switch (FrameLayout)
    {
        case 0x01 :
        case 0x04 :
        case 0xFF : //Seen in one file
                    return 2;
        default   :
                    return 1;
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_ColorPrimaries(const int128u ColorPrimaries)
{
    int32u Code_Compare4=(int32u)ColorPrimaries.lo;
    switch ((int8u)(Code_Compare4>>16))
    {
        case 0x01 : return "BT.601 NTSC";
        case 0x02 : return "BT.601 PAL";
        case 0x03 : return "BT.709";
        case 0x04 : return "BT.2020";
        case 0x05 : return "XYZ";
        case 0x06 : return "Display P3";
        case 0x07 : return "ACES"; //  SMPTE ST 2065-1
        case 0x08 : return "XYZ"; //  SMPTE ST 2067-40 / ISO 11664-3
        default   : return "";
    }
}

//---------------------------------------------------------------------------
const char* Mxf_TransferCharacteristic_MXF[]=
{
    "BT.601",
    "BT.709",
    "SMPTE 240M",
    "SMPTE 274M",
    "BT.1361",
    "Linear",
    "SMPTE 428M",
    "xvYCC",
    "BT.2020", // ISO does a difference of value between 10 and 12 bit
    "PQ",
    "HLG",
    "Gamma 2.6", // SMPTE ST 2067-50
    "sRGB/sYCC", // IEC 61966-2-1
};
const char* Mxf_TransferCharacteristic_Sony_1[]= // 0x0E06x Sony values are read from SR Viewer program
{
    "DVW-709 Like",                      
    "E10/E30STD for J EK",
    "E10/E30STD for UC",
    "",
    "",
    "BBC Initial50",
    "SD CamCorder STD",
    "BVW-400 Like",
    "Ikegami",
};
const char* Mxf_TransferCharacteristic_Sony_2[]= // 0x0E06x Sony values are read from SR Viewer program
{
    "HG3250G36",
    "HG4600G30",
    "HG3259G40",
    "HG4609G33",
    "HG8000G36",
    "HG8000G30",
    "HG8009G40",
    "HG8009G33",
};
const char* Mxf_TransferCharacteristic_Sony_3[]= // 0x0E06x Sony values are read from SR Viewer program
{
    "CINE1 of EX1/EX3",
    "CINE2 of EX1/EX3",
    "CINE3 of EX1/EX3",
    "CINE4 of EX1/EX3",
    "Kodak 5248 film like",
    "Kodak 5245 film like",
    "Kodak 5293 film like",
    "Kodak 5296 film like",
    "Average of Film of MSW-900",
};
const char* Mxf_TransferCharacteristic_Sony_4[]= // 0x0E06x Sony values are read from SR Viewer program
{
    "User defined curve1",
    "User defined curve2",
    "User defined curve3",
    "User defined curve4",
    "User defined curve5",
    "User defined curve6",
    "User defined curve7",
    "User defined curve8",
};
const char* Mxf_TransferCharacteristic_Sony_5[]= // 0x0E06x Sony values are read from SR Viewer program
{
    "S-Log",
    "FS-Log",
    "R709 180%",
    "R709 800%",
    "",
    "Cine-Log",
    "ASC-CDL",
};
const char** Mxf_TransferCharacteristic_Sony[]=
{
    Mxf_TransferCharacteristic_Sony_1,
    Mxf_TransferCharacteristic_Sony_2,
    Mxf_TransferCharacteristic_Sony_3,
    Mxf_TransferCharacteristic_Sony_4,
    Mxf_TransferCharacteristic_Sony_5,
};
const int8u Mxf_TransferCharacteristic_Sony_Size[]=
{
    (int8u)(sizeof(Mxf_TransferCharacteristic_Sony_1)/sizeof(Mxf_TransferCharacteristic_Sony_1[0])),
    (int8u)(sizeof(Mxf_TransferCharacteristic_Sony_2)/sizeof(Mxf_TransferCharacteristic_Sony_2[0])),
    (int8u)(sizeof(Mxf_TransferCharacteristic_Sony_3)/sizeof(Mxf_TransferCharacteristic_Sony_3[0])),
    (int8u)(sizeof(Mxf_TransferCharacteristic_Sony_4)/sizeof(Mxf_TransferCharacteristic_Sony_4[0])),
    (int8u)(sizeof(Mxf_TransferCharacteristic_Sony_5)/sizeof(Mxf_TransferCharacteristic_Sony_5[0])),
};
static string Mxf_TransferCharacteristic(const int128u Value)
{
    if ((Value.lo>>24)==0x0401010101000000LL>>24) // MXF
    {
        int8u Value2=(int8u)((Value.lo>>16)-1);
        if (Value2<sizeof(Mxf_TransferCharacteristic_MXF)/sizeof(Mxf_TransferCharacteristic_MXF[0]))
            return Mxf_TransferCharacteristic_MXF[Value2];
    }
    if ((Value.lo>>16)==0x0E06040101010000LL>>16) // Sony
    {
        int8u Value2=(int8u)((Value.lo>>8)-1);
        if (Value2<sizeof(Mxf_TransferCharacteristic_Sony_Size)/sizeof(Mxf_TransferCharacteristic_Sony_Size[0]))
        {
            int8u Value3=(int8u)(size_t(Value.lo)-1);
            if (Value3<Mxf_TransferCharacteristic_Sony_Size[Value2])
                return Mxf_TransferCharacteristic_Sony[Value2][Value3];
        }
    }

    Ztring ValueS;
    ValueS.From_Number(Value.lo, 16);
    if (ValueS.size()<16)
        ValueS.insert(0, 16-ValueS.size(), __T('0'));
    return ValueS.To_UTF8();
}

//---------------------------------------------------------------------------
static const char* Mxf_CodingEquations(const int128u CodingEquations)
{
    int32u Code_Compare4=(int32u)CodingEquations.lo;
    switch ((int8u)(Code_Compare4>>16))
    {
        case 0x01 : return "BT.601";
        case 0x02 : return "BT.709";
        case 0x03 : return "SMPTE 240M";
        case 0x04 : return "YCgCo";
        case 0x05 : return "Identity";
        case 0x06 : return "BT.2020 non-constant";
        default   : return "";
    }
}

//---------------------------------------------------------------------------
static const char* Mxf_ChannelAssignment_ChannelLayout(const int128u& ChannelLayout, int32u ChannelsCount=(int32u)-1)
{
    //Sound Channel Labeling
    if ((ChannelLayout.hi&0xFFFFFFFFFFFFFF00LL)!=0x060E2B3404010100LL && (ChannelLayout.lo&0xFFFFFFFF00000000LL)!=0x0402021000000000LL)
        return "";

    int8u Code5=(int8u)((ChannelLayout.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((ChannelLayout.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((ChannelLayout.lo&0x000000000000FF00LL)>> 8);

    switch (Code5)
    {
        case 0x03 : //SMPTE 429-2
                    switch (Code6)
                    {
                        case 0x01 : //Sets
                                    switch (Code7)
                                    {
                                        case 0x01 : //Config 1
                                                    switch (ChannelsCount)
                                                    {
                                                        case  6 : return "L R C LFE Ls Rs";
                                                        default : return "L R C LFE Ls Rs HI VI-N";
                                                    }
                                        case 0x02 : //Config 2
                                                    switch (ChannelsCount)
                                                    {
                                                        case  6 : return "L R C LFE Ls Rs";
                                                        case  8 : return "L R C LFE Ls Rs Cs X";
                                                        default : return "L R C LFE Ls Rs Cs X HI VI-N";
                                                    }
                                        case 0x03 : //Config 3
                                                    switch (ChannelsCount)
                                                    {
                                                        case  6 : return "L R C LFE Ls Rs";
                                                        case  8 : return "L R C LFE Ls Rs Lrs Rrs";
                                                        default : return "L R C LFE Ls Rs Lrs Rrs HI VI-N";
                                                    }
                                        default   : return "";
                                    }
                        default   : return "";
                    }
        default   : return "";
    }
}
static const char* Mxf_ChannelAssignment_ChannelLayout(const int128u ChannelLayout)
{
    return Mxf_ChannelAssignment_ChannelLayout(ChannelLayout, (int32u)-1);
}

static string MXF_MCALabelDictionaryID_ChannelPositions(const std::vector<int128u> &MCALabelDictionaryIDs)
{
    string ToReturn;
    std::bitset<8> Front, Side, Back, Lfe;
    bool IsOk=true;

    for (size_t Pos=0; Pos<MCALabelDictionaryIDs.size(); Pos++)
    {
        if ((MCALabelDictionaryIDs[Pos].hi&0xFFFFFFFFFFFFFF00LL)==0x060E2B3404010100LL && (MCALabelDictionaryIDs[Pos].lo&0xFF00000000000000LL)==0x0300000000000000LL)
        {
            switch ((MCALabelDictionaryIDs[Pos].lo>>48)&0xFF)
            {
            case 0x01 :
                            switch ((MCALabelDictionaryIDs[Pos].lo>>40)&0xFF)
                            {
                                case 0x01 :
                                            switch ((MCALabelDictionaryIDs[Pos].lo>>32)&0xFF)
                                            {
                                                case 0x01 : Front[0]=true; break; //L
                                                case 0x02 : Front[1]=true; break; //R
                                                case 0x03 : Front[2]=true; break; //C
                                                case 0x04 : Lfe[0]=true; break; //LFE
                                                case 0x05 : Side[0]=true; break; //Ls
                                                case 0x06 : Side[1]=true; break; //Rs
                                                case 0x20 : Front[6]=true; break; //M1 //TODO: split M1 and M2 in 2 services
                                                case 0x21 : Front[7]=true; break; //M2 //TODO: split M1 and M2 in 2 services
                                                case 0x22 : Front[4]=true; break; //Lt
                                                case 0x23 : Front[5]=true; break; //Rt
                                                default   : IsOk=false;
                                            }
                                            break;
                                default  : IsOk=false;
                            }
                            break;
                case 0x02 :
                            switch ((MCALabelDictionaryIDs[Pos].lo>>40)&0xFF)
                            {
                                case 0x01 :
                                            switch ((MCALabelDictionaryIDs[Pos].lo>>32)&0xFF)
                                            {
                                                case 0x01 : Front[0]=true; break; //L
                                                case 0x02 : Front[1]=true; break; //R
                                                case 0x03 : Front[2]=true; break; //C
                                                case 0x04 : Lfe[0]=true; break; //LFE
                                                case 0x05 : Side[0]=true; break; //Ls
                                                case 0x06 : Side[1]=true; break; //Rs
                                                case 0x20 :
                                                            switch ((MCALabelDictionaryIDs[Pos].lo>>24)&0xFF)
                                                            {
                                                                case 0x03 : Front[4]=true; break; //Lt
                                                                case 0x04 : Front[5]=true; break; //Rt
                                                                default   : IsOk=false;
                                                            }
                                                            break;
                                                default   : IsOk=false;
                                            }
                                            break;
                                default  : IsOk=false;
                            }
                            break;
                default  : IsOk=false;
            }
        }
        else
            IsOk=false;
    }

    if (IsOk)
    {
        string FrontS, SideS, BackS, LfeS;
        if (Front.any())
        {
            FrontS+="Front: ";
            if (Front[0])
                FrontS+="L ";
            if (Front[2])
                FrontS+="C ";
            if (Front[1])
                FrontS+="R ";
            if (Front[4])
                FrontS+="Lt ";
            if (Front[5])
                FrontS+="Rt ";
            if (Front[6])
                FrontS+="M ";
            if (Front[7])
                FrontS+="M ";
            FrontS.resize(FrontS.size()-1);
        }
        if (Side.any())
        {
            SideS+="Side: ";
            if (Side[0])
                SideS+="L ";
            if (Side[1])
                SideS+="R ";
            SideS.resize(SideS.size()-1);
        }
        if (Back.any())
        {
            BackS+="Back: ";
            if (Back[0])
                BackS+="L ";
            if (Back[2])
                BackS+="C ";
            if (Back[1])
                BackS+="R ";
            BackS.resize(BackS.size()-1);
        }
        if (Lfe.any())
        {
            if (Lfe[0])
                LfeS+="LFE";
        }
        if (!FrontS.empty())
            ToReturn+=FrontS;
        if (!SideS.empty())
        {
            if (!ToReturn.empty())
                ToReturn+=", ";
            ToReturn+=SideS;
        }
        if (!BackS.empty())
        {
            if (!ToReturn.empty())
                ToReturn+=", ";
            ToReturn+=BackS;
        }
        if (!LfeS.empty())
        {
            if (!ToReturn.empty())
                ToReturn+=", ";
            ToReturn+=LfeS;
        }
    }

    return ToReturn;
}

static string MXF_MCALabelDictionaryID_ChannelLayout(const std::vector<int128u> &MCALabelDictionaryIDs)
{
    string ToReturn;

    for (size_t Pos=0; Pos<MCALabelDictionaryIDs.size(); Pos++)
    {
        const char* Value="";
        if ((MCALabelDictionaryIDs[Pos].hi&0xFFFFFFFFFFFFFF00LL)==0x060E2B3404010100LL && (MCALabelDictionaryIDs[Pos].lo&0xFF00000000000000LL)==0x0300000000000000LL)
        {
            switch ((MCALabelDictionaryIDs[Pos].lo>>48)&0xFF)
            {
            case 0x01 :
                            switch ((MCALabelDictionaryIDs[Pos].lo>>40)&0xFF)
                            {
                                case 0x01 :
                                            switch ((MCALabelDictionaryIDs[Pos].lo>>32)&0xFF)
                                            {
                                                case 0x01 : Value="L"; break;
                                                case 0x02 : Value="R"; break;
                                                case 0x03 : Value="C"; break;
                                                case 0x04 : Value="LFE"; break;
                                                case 0x05 : Value="Ls"; break;
                                                case 0x06 : Value="Rs"; break;
                                                case 0x20 : Value="M"; break;
                                                case 0x21 : Value="M"; break;
                                                case 0x22 : Value="Lt"; break;
                                                case 0x23 : Value="Rt"; break;
                                                default   : Value="";
                                            }
                                            break;
                                default  : Value="";
                            }
                            break;
                case 0x02 :
                            switch ((MCALabelDictionaryIDs[Pos].lo>>40)&0xFF)
                            {
                                case 0x01 :
                                            switch ((MCALabelDictionaryIDs[Pos].lo>>32)&0xFF)
                                            {
                                                case 0x01 : Value="L"; break;
                                                case 0x02 : Value="R"; break;
                                                case 0x03 : Value="C"; break;
                                                case 0x04 : Value="LFE"; break;
                                                case 0x05 : Value="Ls"; break;
                                                case 0x06 : Value="Rs"; break;
                                                case 0x20 :
                                                            switch ((MCALabelDictionaryIDs[Pos].lo>>24)&0xFF)
                                                            {
                                                                case 0x03 : Value="Lt"; break;
                                                                case 0x04 : Value="Rt"; break;
                                                                default   : Value="";
                                                            }
                                                            break;
                                                case 0x22 : Value="Lt"; break;
                                                case 0x23 : Value="Rt"; break;
                                                default   : Value="";
                                            }
                                            break;
                                default  : Value="";
                            }
                            break;
                default  : Value="";
            }
        }
        if (!ToReturn.empty())
            ToReturn+=' ';
        if (strlen(Value))
            ToReturn+=Value;
        else
        {
            Ztring Value; Value.From_Number(MCALabelDictionaryIDs[Pos].lo>>32, 16);
            if (Value.size()<8)
                Value.insert(0, 8-Value.size(), __T('0'));
            ToReturn+=Value.To_UTF8();
        }
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
const size_t Mxf_AS11_ClosedCaptionType_Count=2;
static const char* Mxf_AS11_ClosedCaptionType[Mxf_AS11_ClosedCaptionType_Count]=
{
    "Hard of Hearing",
    "Translation",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_AudioTrackLayout_Count=0x35;
static const char* Mxf_AS11_AudioTrackLayout[Mxf_AS11_AudioTrackLayout_Count]=
{
    "EBU R 48: 1a",
    "EBU R 48: 1b",
    "EBU R 48: 1c",
    "EBU R 48: 2a",
    "EBU R 48: 2b",
    "EBU R 48: 2c",
    "EBU R 48: 3a",
    "EBU R 48: 3b",
    "EBU R 48: 4a",
    "EBU R 48: 4b",
    "EBU R 48: 4c",
    "EBU R 48: 5a",
    "EBU R 48: 5b",
    "EBU R 48: 6a",
    "EBU R 48: 6b",
    "EBU R 48: 7a",
    "EBU R 48: 7b",
    "EBU R 48: 8a",
    "EBU R 48: 8b",
    "EBU R 48: 8c",
    "EBU R 48: 9a",
    "EBU R 48: 9b",
    "EBU R 48: 10a",
    "EBU R 48: 11a",
    "EBU R 48: 11b",
    "EBU R 48: 11c",
    "EBU R 123: 2a",
    "EBU R 123: 4a",
    "EBU R 123: 4b",
    "EBU R 123: 4c",
    "EBU R 123: 8a",
    "EBU R 123: 8b",
    "EBU R 123: 8c",
    "EBU R 123: 8d",
    "EBU R 123: 8e",
    "EBU R 123: 8f",
    "EBU R 123: 8g",
    "EBU R 123: 8h",
    "EBU R 123: 8i",
    "EBU R 123: 12a",
    "EBU R 123: 12b",
    "EBU R 123: 12c",
    "EBU R 123: 12d",
    "EBU R 123: 12e",
    "EBU R 123: 12f",
    "EBU R 123: 12g",
    "EBU R 123: 12h",
    "EBU R 123: 16a",
    "EBU R 123: 16b",
    "EBU R 123: 16c",
    "EBU R 123: 16d",
    "EBU R 123: 16e",
    "EBU R 123: 16f",
};
struct mxf_as11_audiotracklayout_assignment
{
    const int8u Count;
    const char* Assign[16];
};
static const mxf_as11_audiotracklayout_assignment Mxf_AS11_AudioTrackLayout_ChannelPositions[Mxf_AS11_AudioTrackLayout_Count]=
{
    {  2, "Front: C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1a
    {  4, "Front: C", NULL, "Front: C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1b
    {  8, "Front: C", NULL, "Front: C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1c
    {  2, "Front: L", "Front: R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2a
    {  4, "Front: L", "Front: R", "Front: L", "Front: R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2b
    {  8, "Front: L", "Front: R", "Front: L", "Front: R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2c
    {  4, "Front: L", "Front: R", "Front: L", "Front: R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 3a
    {  8, "Front: L", "Front: R", "Front: L", "Front: R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 3b
    {  2, "EBU R 48: 4a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 4b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 4c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 5a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 5b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 6a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 6b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 7a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 7b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 48: 8a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 8b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 8c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 9a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 9b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 48: 10a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 11a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 11b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 11c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 123: 2a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8d", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8e", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8f", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8g", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8h", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8i", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12d", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12e", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12f", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12g", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12h", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 16, "Front: L", "Front: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", }, //123 16a
    { 16, "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", }, //123 16b
    { 16, "Front: L", "Front: R", "Front: L", "Front: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", }, //123 16c
    { 16, "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", }, //123 16d
    { 16, "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Front: L", "Front: R", "Front: C", "LFE", "Side: L", "Side: R", "Side: L", "Side: R", "Side: L", "Side: R", }, //123 16e
    { 16, "Front: L", "Front: R", NULL, NULL, "Front: L", "Front: R", NULL, NULL, "Front: L", "Front: R", NULL, NULL, "Front: L", "Front: R", "Front: L", "Front: R", }, //123 16f
};
static const mxf_as11_audiotracklayout_assignment Mxf_AS11_AudioTrackLayout_ChannelLayout[Mxf_AS11_AudioTrackLayout_Count]=
{
    {  2, "C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1a
    {  4, "C", NULL, "C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1b
    {  8, "C", NULL, "C", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 1c
    {  2, "L", "R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2a
    {  4, "L", "R", "L", "R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2b
    {  8, "L", "R", "L", "R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 2c
    {  4, "L", "R", "L", "R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 3a
    {  8, "L", "R", "L", "R", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, }, //48 3b
    {  2, "EBU R 48: 4a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 4b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 4c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 5a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 5b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 6a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 6b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 7a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 7b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 48: 8a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 8b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 8c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 9a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 9b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 48: 10a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 48: 11a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 11b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 48: 11c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  2, "EBU R 123: 2a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  4, "EBU R 123: 4c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8d", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8e", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8f", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8g", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8h", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    {  8, "EBU R 123: 8i", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12a", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12b", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12c", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12d", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12e", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12f", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12g", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 12, "EBU R 123: 12h", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
    { 16, "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", }, //123 16a
    { 16, "L", "R", "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "L", "R", "C", "LFE", "Ls", "Rs", }, //123 16b
    { 16, "L", "R", "L", "R", "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "C", "LFE", "Ls", "Rs", }, //123 16c
    { 16, "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", }, //123 16d
    { 16, "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "C", "LFE", "Ls", "Rs", "L", "R", "L", "R", }, //123 16e
    { 16, "L", "R", NULL, NULL, "L", "R", NULL, NULL, "L", "R", NULL, NULL, "L", "R", "L", "R", }, //123 16f
};


//---------------------------------------------------------------------------
const size_t Mxf_AS11_FpaPass_Count=3;
static const char* Mxf_AS11_FpaPass[Mxf_AS11_FpaPass_Count]=
{
    "Yes",
    "No",
    "", // Not tested
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_SigningPresent_Count=3;
static const char* Mxf_AS11_SigningPresent[Mxf_AS11_SigningPresent_Count]=
{
    "Yes",
    "No",
    "Signer only",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_3D_Type_Count=4;
static const char* Mxf_AS11_3D_Type[Mxf_AS11_3D_Type_Count]=
{
    "Side by side",
    "Dual",
    "Left eye only",
    "Right eye only",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_AudioLoudnessStandard_Count=2;
static const char* Mxf_AS11_AudioLoudnessStandard[Mxf_AS11_AudioLoudnessStandard_Count]=
{
    "",
    "EBU R 128",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_AudioDescriptionType_Count=2;
static const char* Mxf_AS11_AudioDescriptionType[Mxf_AS11_AudioDescriptionType_Count]=
{
    "Control data / Narration",
    "AD Mix",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_OpenCaptionsType_Count=2;
static const char* Mxf_AS11_OpenCaptionsType[Mxf_AS11_OpenCaptionsType_Count]=
{
    "Hard of Hearing",
    "Translation",
};

//---------------------------------------------------------------------------
const size_t Mxf_AS11_SignLanguage_Count=2;
static const char* Mxf_AS11_SignLanguage[Mxf_AS11_SignLanguage_Count]=
{
    "BSL (British Sign Language)",
    "BSL (Makaton)",
};


//---------------------------------------------------------------------------
// EBU Tech 3349
static string Mxf_CameraUnitAcquisitionMetadata_GammaforCDL(int8u Value)
{
    switch(Value)
    {
        case 0x00 : return "Same as Capture Gamma";
        case 0x01 : return "Linear";
        case 0x02 : return "S-Log";
        case 0x03 : return "Cine-Log";
        case 0xFF : return "";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
};

//---------------------------------------------------------------------------
// EBU Tech 3349
static string Mxf_CameraUnitAcquisitionMetadata_NeutralDensityFilterWheelSetting(int16u Value)
{
    switch(Value)
    {
        case 0x01 : return "Clear";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
};

//---------------------------------------------------------------------------
// EBU Tech 3349
static string Mxf_CameraUnitAcquisitionMetadata_ImageSensorReadoutMode(int8u Value)
{
    switch(Value)
    {
        case 0x00 : return "Interlaced field";
        case 0x01 : return "Interlaced frame";
        case 0x02 : return "Progressive frame";
        case 0xFF : return "Undefined";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
};

//---------------------------------------------------------------------------
// RDD 18
static string Mxf_CameraUnitAcquisitionMetadata_AutoExposureMode(int128u Value)
{
    switch(Value.lo)
    {
        case 0x0510010101010000LL : return "Manual";
        case 0x0510010101020000LL : return "Full Auto";
        case 0x0510010101030000LL : return "Gain Priority Auto";
        case 0x0510010101040000LL : return "Iris Priority Auto";
        case 0x0510010101050000LL : return "Shutter Priority Auto";
        default   :
                    {
                    Ztring ValueS;
                    ValueS.From_Number(Value.lo, 16);
                    if (ValueS.size()<16)
                        ValueS.insert(0, 16-ValueS.size(), __T('0'));
                    return ValueS.To_UTF8();
                    }
    }
}

//---------------------------------------------------------------------------
// RDD 18
static string Mxf_CameraUnitAcquisitionMetadata_AutoFocusSensingAreaSetting(int8u Value)
{
    switch(Value)
    {
        case 0x00 : return "Manual";
        case 0x01 : return "Center Sensitive Auto";
        case 0x02 : return "Full Screen Sensing Auto";
        case 0x03 : return "Multi Spot Sensing Auto";
        case 0x04 : return "Single Spot Sensing Auto";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// RDD 18
static string Mxf_CameraUnitAcquisitionMetadata_ColorCorrectionFilterWheelSetting(int8u Value)
{
    switch(Value)
    {
        case 0x00 : return "Cross effect";
        case 0x01 : return "Color Compensation 3200 K";
        case 0x02 : return "Color Compensation 4300 K";
        case 0x03 : return "Color Compensation 6300 K";
        case 0x04 : return "Color Compensation 5600 K";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// RDD 18
static string Mxf_CameraUnitAcquisitionMetadata_AutoWhiteBalanceMode(int8u Value)
{
    switch(Value)
    {
        case 0x00 : return "Preset";
        case 0x01 : return "Automatic";
        case 0x02 : return "Hold";
        case 0x03 : return "One Push";
        default   : return Ztring::ToZtring(Value).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// CameraUnitAcquisitionMetadata
static string Mxf_AcquisitionMetadata_ElementName(int16u Value, bool IsSony=false)
{
    if (IsSony)
        switch (Value)
        {
            case 0xE101: return "EffectiveMarkerCoverage";
            case 0xE102: return "EffectiveMarkerAspectRatio";
            case 0xE103: return "CameraProcessDiscriminationCode";
            case 0xE104: return "RotaryShutterMode";
            case 0xE105: return "RawBlackCodeValue";
            case 0xE106: return "RawGrayCodeValue";
            case 0xE107: return "RawWhiteCodeValue";
            case 0xE109: return "MonitoringDescriptions";
            case 0xE10B: return "MonitoringBaseCurve";
          //case 0xE201: return "CookeProtocol_BinaryMetadata"; //Not used
            case 0xE202: return "CookeProtocol_UserMetadata";
            case 0xE203: return "CookeProtocol_CalibrationType";
            default:     ;
        }

    switch (Value) // From EBU Tech 3349
    {
        case 0x3210: return "TransferCharacteristics"; // a.k.a. Capture Gamma Equation
        case 0x3219: return "ColorPrimaries";
        case 0x321A: return "MatrixCoefficients"; // a.k.a. Coding Equations
        case 0x8000: return "IrisFNumber";
        case 0x8001: return "FocusPositionFromImagePlane";
        case 0x8002: return "FocusPositionFromFrontLensVertex";
        case 0x8003: return "MacroSetting";
        case 0x8004: return "LensZoom35mmStillCameraEquivalent";
        case 0x8005: return "LensZoomActualFocalLength";
        case 0x8006: return "OpticalExtenderMagnification";
        case 0x8007: return "LensAttributes";
        case 0x8008: return "IrisTNumber";
        case 0x8009: return "IrisRingPosition";
        case 0x800A: return "FocusRingPosition";
        case 0x800B: return "ZoomRingPosition";
        case 0x8100: return "AutoExposureMode";
        case 0x8101: return "AutoFocusSensingAreaSetting";
        case 0x8102: return "ColorCorrectionFilterWheelSetting";
        case 0x8103: return "NeutralDensityFilterWheelSetting";
        case 0x8104: return "ImageSensorDimensionEffectiveWidth";
        case 0x8105: return "ImageSensorDimensionEffectiveHeight";
        case 0x8106: return "CaptureFrameRate";
        case 0x8107: return "ImageSensorReadoutMode";
        case 0x8108: return "ShutterSpeed_Angle";
        case 0x8109: return "ShutterSpeed_Time";
        case 0x810A: return "CameraMasterGainAdjustment";
        case 0x810B: return "ISOSensitivity";
        case 0x810C: return "ElectricalExtenderMagnification";
        case 0x810D: return "AutoWhiteBalanceMode";
        case 0x810E: return "WhiteBalance";
        case 0x810F: return "CameraMasterBlackLevel";
        case 0x8110: return "CameraKneePoint";
        case 0x8111: return "CameraKneeSlope";
        case 0x8112: return "CameraLuminanceDynamicRange";
        case 0x8113: return "CameraSettingFileURI";
        case 0x8114: return "CameraAttributes";
        case 0x8115: return "ExposureIndexofPhotoMeter";
        case 0x8116: return "GammaForCDL";
        case 0x8117: return "ASC_CDL_V12";
        case 0x8118: return "ColorMatrix";
        default:     return Ztring(Ztring::ToZtring(Value, 16)).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// CameraUnitAcquisitionMetadata - Sony E201 (Values are internal MI)
const size_t Mxf_AcquisitionMetadata_Sony_E201_ElementCount = 11;
static const char* Mxf_AcquisitionMetadata_Sony_E201_ElementName[Mxf_AcquisitionMetadata_Sony_E201_ElementCount] =
{
    "FocusDistance",
    "ApertureValue",
    "ApertureScale",
    "EffectiveFocaleLength",
    "HyperfocalDistance",
    "NearFocusDistance",
    "FarFocusDistance",
    "HorizontalFieldOfView",
    "EntrancePupilPosition",
    "NormalizedZoomValue",
    "LensSerialNumber",
};

//---------------------------------------------------------------------------
// Read from SR Viewer program
static string Mxf_AcquisitionMetadata_Sony_CameraProcessDiscriminationCode(int16u Value)
{
    switch (Value)
    {
        case 0x0101: return "F65 RAW Mode released in December 2011";
        case 0x0102: return "F65 HD Mode released in April 2012";
        case 0x0103: return "F65 RAW High Frame Rate Mode released in July 2012";
        default:     return Ztring(Ztring::ToZtring(Value, 16)).To_UTF8();
    }
};

//---------------------------------------------------------------------------
// Read from SR Viewer program
static string Mxf_AcquisitionMetadata_Sony_MonitoringBaseCurve(int128u Value)
{
    switch(Value.lo)
    {
        case 0x0E06040101010508LL : return "S-Log2";
        default   :
                    {
                    Ztring ValueS;
                    ValueS.From_Number(Value.lo, 16);
                    if (ValueS.size()<16)
                        ValueS.insert(0, 16-ValueS.size(), __T('0'));
                    return ValueS.To_UTF8();
                    }
    }
}

//***************************************************************************
// Config
//***************************************************************************

//---------------------------------------------------------------------------
static const char* ShowSource_List[] =
{
    "colour_description",
    "colour_range",
    "colour_primaries",
    "matrix_coefficients",
    "transfer_characteristics",
    "MasteringDisplay_ColorPrimaries",
    "MasteringDisplay_Luminance",
    "MaxCLL",
    "MaxFALL",
    NULL
};
static bool ShowSource_IsInList(const string &Value)
{
    for (size_t i = 0; ShowSource_List[i]; i++)
        if (ShowSource_List[i] == Value)
            return true;
    return false;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mxf::File_Mxf()
:File__Analyze(), File__HasReferences()
{
    //Configuration
    ParserName="MXF";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Mxf;
        StreamIDs_Width[0]=8;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    MustSynchronize=true;
    DataMustAlwaysBeComplete=false;
    Buffer_TotalBytes_Fill_Max=(int64u)-1; //Disabling this feature for this format, this is done in the parser
    FrameInfo.DTS=0;
    Frame_Count_NotParsedIncluded=0;
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX

    //In
    IsRtmd=false;

    //Hints
    File_Buffer_Size_Hint_Pointer=NULL;
    Synched_Count=0;

    //Temp
    RandomIndexPacks_AlreadyParsed=false;
    Streams_Count=(size_t)-1;
    OperationalPattern=0;
    Buffer_Begin=(int64u)-1;
    Buffer_End=0;
    Buffer_End_Unlimited=false;
    Buffer_Header_Size=0;
    Preface_Current.hi=0;
    Preface_Current.lo=0;
    IsParsingMiddle_MaxOffset=(int64u)-1;
    Track_Number_IsAvailable=false;
    IsParsingEnd=false;
    IsCheckingRandomAccessTable=false;
    IsCheckingFooterPartitionAddress=false;
    IsSearchingFooterPartitionAddress=false;
    FooterPartitionAddress_Jumped=false;
    PartitionPack_Parsed=false;
    HeaderPartition_IsOpen=false;
    Is1001=false;
    IdIsAlwaysSame_Offset=0;
    PartitionMetadata_PreviousPartition=(int64u)-1;
    PartitionMetadata_FooterPartition=(int64u)-1;
    RandomIndexPacks_MaxOffset=(int64u)-1;
    DTS_Delay=0;
    SDTI_TimeCode_RepetitionCount=0;
    SDTI_SizePerFrame=0;
    SDTI_IsPresent=false;
    SDTI_IsInIndexStreamOffset=true;
    #if MEDIAINFO_TRACE
        SDTI_SystemMetadataPack_Trace_Count=0;
        SDTI_PackageMetadataSet_Trace_Count=0;
        Padding_Trace_Count=0;
    #endif // MEDIAINFO_TRACE
    Essences_FirstEssence_Parsed=false;
    MayHaveCaptionsInStream=false;
    StereoscopicPictureSubDescriptor_IsPresent=false;
    UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony=false;
    Essences_UsedForFrameCount=(int32u)-1;
    #if MEDIAINFO_ADVANCED
        Footer_Position=(int64u)-1;
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_NEXTPACKET
        ReferenceFiles_IsParsing=false;
    #endif //MEDIAINFO_NEXTPACKET
    #if defined(MEDIAINFO_ANCILLARY_YES)
        Ancillary=NULL;
    #endif //defined(MEDIAINFO_ANCILLARY_YES)

    ExtraMetadata_Offset=(int64u)-1;
    DolbyAudioMetadata=NULL;
    #if defined(MEDIAINFO_ADM_YES)
        Adm=NULL;
        Adm_ForLaterMerge=NULL;
    #endif

    #if MEDIAINFO_DEMUX
        Demux_HeaderParsed=false;
    #endif //MEDIAINFO_DEMUX

    Partitions_Pos=0;
    Partitions_IsCalculatingHeaderByteCount=false;
    Partitions_IsCalculatingSdtiByteCount=false;
    Partitions_IsFooter=false;

    #if MEDIAINFO_SEEK
        IndexTables_Pos=0;
        Clip_Header_Size=0;
        Clip_Begin=(int64u)-1;
        Clip_End=0;
        OverallBitrate_IsCbrForSure=0;
        Duration_Detected=false;
    #endif //MEDIAINFO_SEEK
    #if MEDIAINFO_DEMUX
        DemuxedSampleCount_Total=(int64u)-1;
        DemuxedSampleCount_Current=(int64u)-1;
        DemuxedSampleCount_AddedToFirstFrame=0;
        DemuxedElementSize_AddedToFirstFrame=0;
    #endif //MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
File_Mxf::~File_Mxf()
{
    #if defined(MEDIAINFO_ANCILLARY_YES)
        if (Ancillary)
        {
            for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
                for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
                    if (*Parser == Ancillary)
                        *Parser = nullptr;
            delete Ancillary;
        }
    #endif //defined(MEDIAINFO_ANCILLARY_YES)
	
    for (size_t i = 0; i < AcquisitionMetadataLists.size(); i++)
        delete AcquisitionMetadataLists[ i ];
	
    AcquisitionMetadataLists.clear();
	
    for (size_t i = 0; i < AcquisitionMetadata_Sony_E201_Lists.size(); i++)
        delete AcquisitionMetadata_Sony_E201_Lists[ i ];
	
    AcquisitionMetadata_Sony_E201_Lists.clear();
    delete DolbyAudioMetadata;
    #if defined(MEDIAINFO_ADM_YES)
        delete Adm;
    #endif
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mxf::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "MXF");

    //Configuration
    Buffer_MaximumSize=64*1024*1024; //Some big frames are possible (e.g YUV 4:2:2 10 bits 1080p, 4K)
    File_Buffer_Size_Hint_Pointer=Config->File_Buffer_Size_Hint_Pointer_Get();
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Fill()
{
    for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
        for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
            Fill(*Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish()
{
    Frame_Count=Frame_Count_NotParsedIncluded=FrameInfo.PTS=FrameInfo.DTS=(int64u)-1;

    #if MEDIAINFO_NEXTPACKET && defined(MEDIAINFO_REFERENCES_YES)
        //Locators only
        if (ReferenceFiles_IsParsing)
        {
            ReferenceFiles->ParseReferences();
            #if MEDIAINFO_DEMUX
                if (Config->Demux_EventWasSent)
                    return;
            #endif //MEDIAINFO_DEMUX

            Streams_Finish_CommercialNames();
            return;
        }
    #endif //MEDIAINFO_NEXTPACKET && defined(MEDIAINFO_REFERENCES_YES)

    //Per stream
    for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
    {
        if (Essence->second.Parsers.size()>1 && Essence->second.StreamKind==Stream_Audio) // Last parser is PCM, impossible to detect with another method if there is only one block
        {
            for (size_t Pos=0; Pos<Essence->second.Parsers.size()-1; Pos++)
                delete Essence->second.Parsers[Pos];
            Essence->second.Parsers.erase(Essence->second.Parsers.begin(), Essence->second.Parsers.begin()+Essence->second.Parsers.size()-1);
            Essence->second.Parsers[0]->Accept();
            Essence->second.Parsers[0]->Fill();
        }
        for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
        {
            if (!(*Parser)->Status[IsFinished])
            {
                if (Config->ParseSpeed>=1)
                {
                    int64u File_Size_Temp=File_Size;
                    File_Size=File_Offset+Buffer_Offset+Element_Offset;
                    Open_Buffer_Continue(*Parser, Buffer, 0);
                    File_Size=File_Size_Temp;
                }
                Finish(*Parser);
                #if MEDIAINFO_DEMUX
                    if (Config->Demux_EventWasSent)
                        return;
                #endif //MEDIAINFO_DEMUX
            }
        }
    }

    if (!Track_Number_IsAvailable)
    {
        if (Tracks.empty())
        {
            //Clear
            for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
                for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
                {
                    Merge(*(*Parser));

                    Ztring LawRating=(*Parser)->Retrieve(Stream_General, 0, General_LawRating);
                    if (!LawRating.empty())
                        Fill(Stream_General, 0, General_LawRating, LawRating, true);
                    Ztring Title=(*Parser)->Retrieve(Stream_General, 0, General_Title);
                    if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                        Fill(Stream_General, 0, General_Title, Title);

                    if (IsSub && StreamKind_Last!=Stream_Max && Retrieve(StreamKind_Last, StreamPos_Last, "MuxingMode").empty())
                        Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", "MXF");
                }
        }
        else
            for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
            {
                //Searching the corresponding Descriptor
                stream_t StreamKind=Stream_Max;
                for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                    if (Descriptor->second.LinkedTrackID==Track->second.TrackID)
                    {
                        StreamKind=Descriptor->second.StreamKind;
                        break;
                    }
                if (StreamKind!=Stream_Max)
                {
                    for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
                        if (Essence->second.StreamKind==StreamKind && !Essence->second.Track_Number_IsMappedToTrack)
                        {
                            Track->second.TrackNumber=Essence->first;
                            Essence->second.Track_Number_IsMappedToTrack=true;
                            break;
                        }
                }
        }
    }

    File_Size_Total=File_Size;
    StreamKind_Last=Stream_Max;
    StreamPos_Last=(size_t)-1;

    Streams_Finish_Preface(Preface_Current);

    //OperationalPattern
    Fill(Stream_General, 0, General_Format_Profile, Mxf_OperationalPattern(OperationalPattern));

    //Time codes
    if (SDTI_TimeCode_StartTimecode.IsSet())
    {
        bool IsDuplicate=false;
        for (size_t Pos2=0; Pos2<Count_Get(Stream_Other); Pos2++)
            if (Retrieve(Stream_Other, Pos2, "TimeCode_Source")==__T("SDTI"))
                IsDuplicate=true;
        if (!IsDuplicate)
        {
            Fill_Flush();
            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "SMPTE TC");
            Fill(Stream_Other, StreamPos_Last, Other_MuxingMode, "SDTI");
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_FirstFrame, SDTI_TimeCode_StartTimecode.ToString());
            Fill(Stream_Other, StreamPos_Last, Other_FrameRate, SDTI_TimeCode_StartTimecode.GetFrameRate());
        }
    }

    //Parsing locators
    Locators_Test();
    #if defined(MEDIAINFO_REFERENCES_YES) && MEDIAINFO_NEXTPACKET
        if (Config->NextPacket_Get() && ReferenceFiles)
        {
            ReferenceFiles_IsParsing=true;
            return;
        }
    #endif //MEDIAINFO_NEXTPACKET

    //Sizes
    #if MEDIAINFO_ADVANCED
        if (Footer_Position!=(int64u)-1)
            Fill(Stream_General, 0, General_FooterSize, File_Size-Footer_Position);
        else if (Config->ParseSpeed>-1 || (!Partitions.empty() && Partitions[0].FooterPartition && Partitions[0].FooterPartition>=File_Size))
            IsTruncated((!Partitions.empty() && Partitions[0].FooterPartition && Partitions[0].FooterPartition>=File_Size)?Partitions[0].FooterPartition:(int64u)-1, false, "MXF");
    #endif //MEDIAINFO_ADVANCED

    //Handling separate streams
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).empty() && !Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Encoded)).empty() && !Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).empty())
            {
                float64 BitRate_Encoded=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Encoded)).To_float64();
                float64 Duration=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).To_float64();
                if (Duration)
                    Fill((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded), BitRate_Encoded/8*(Duration/1000), 0);
            }

    //File size in case of partial file analysis
    if (Config->File_IgnoreEditsBefore || Config->File_IgnoreEditsAfter!=(int64u)-1)
    {
        int64u FrameCount_FromComponent=(int64u)-1;
        for (components::iterator Component=Components.begin(); Component!=Components.end(); ++Component)
            if (FrameCount_FromComponent>Component->second.Duration)
                FrameCount_FromComponent=Component->second.Duration;
        float64 EditRate_FromTrack=DBL_MAX;
        for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
            if (EditRate_FromTrack>Track->second.EditRate)
                EditRate_FromTrack=Track->second.EditRate;
        if (FrameCount_FromComponent!=(int64u)-1 && FrameCount_FromComponent && EditRate_FromTrack!=DBL_MAX && EditRate_FromTrack)
        {
            int64u FrameCount=FrameCount_FromComponent;
            int64u File_IgnoreEditsBefore=Config->File_IgnoreEditsBefore;
            if (File_IgnoreEditsBefore && Config->File_EditRate && (EditRate_FromTrack<Config->File_EditRate*0.9 || EditRate_FromTrack>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                File_IgnoreEditsBefore=float64_int64s(((float64)File_IgnoreEditsBefore)/Config->File_EditRate*EditRate_FromTrack);
            int64u File_IgnoreEditsAfter=Config->File_IgnoreEditsAfter;
            if (File_IgnoreEditsAfter!=(int64u)-1 && Config->File_EditRate && (EditRate_FromTrack<Config->File_EditRate*0.9 || EditRate_FromTrack>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                File_IgnoreEditsAfter=float64_int64s(((float64)File_IgnoreEditsAfter)/Config->File_EditRate*EditRate_FromTrack);
            if (File_IgnoreEditsAfter<FrameCount)
                FrameCount=File_IgnoreEditsAfter;
            if (FrameCount<File_IgnoreEditsBefore)
                FrameCount=File_IgnoreEditsBefore;
            FrameCount-=File_IgnoreEditsBefore;

            float64 File_Size_Temp=(float64)File_Size;
            File_Size_Temp/=FrameCount_FromComponent;
            File_Size_Temp*=FrameCount;
            Fill(Stream_General, 0, General_FileSize, File_Size_Temp, 0, true);
        }
    }

    //System scheme 1
    for (const auto& SystemScheme : SystemScheme1s)
    {
        for (size_t i=0; i<SystemScheme.second.TimeCodeArray_StartTimecodes.size(); i++)
        {
            Fill_Flush(); //TODO: remove it there is a refactoring
            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_ID, "SystemScheme1-"+to_string(SystemScheme.first&0xFF)+'-'+to_string(i));
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "SMPTE TC");
            Fill(Stream_Other, StreamPos_Last, Other_MuxingMode, "System scheme 1");
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_FirstFrame, SystemScheme.second.TimeCodeArray_StartTimecodes[i].ToString());
        }
    }

    //Primary package info
    bool PrimaryPackageIsSourcePackage=false;
    bool PrimaryPackageIsMaterialPackage=false;
    for (prefaces::iterator Preface=Prefaces.begin(); Preface!=Prefaces.end(); ++Preface)
    {
        for (packages::iterator Package=Packages.begin(); Package!=Packages.end(); ++Package)
            if (Package->first==Preface->second.PrimaryPackage)
            {
                if (Package->second.IsSourcePackage)
                    PrimaryPackageIsSourcePackage=true;
                else
                    PrimaryPackageIsMaterialPackage=true;
            }
    }
    if (PrimaryPackageIsSourcePackage && !PrimaryPackageIsMaterialPackage)
    {
        Fill(Stream_General, 0, "PrimaryPackage", "Source Package");
        Fill_SetOptions(Stream_General, 0, "PrimaryPackage", "N NT");
    }
    if (!PrimaryPackageIsSourcePackage && PrimaryPackageIsMaterialPackage)
    {
        Fill(Stream_General, 0, "PrimaryPackage", "Material Package");
        Fill_SetOptions(Stream_General, 0, "PrimaryPackage", "N NT");
    }

    //CameraUnitAcquisitionMetadata
    if (!AcquisitionMetadataLists.empty())
    {
        if (!Count_Get(Stream_Other))
            Stream_Prepare(Stream_Other);

        for (size_t Pos = 0; Pos < AcquisitionMetadataLists.size(); Pos++)
        {
            if (UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony && Pos==0xE201 && !AcquisitionMetadata_Sony_E201_Lists.empty())
            {
                for (size_t i=0; i<Mxf_AcquisitionMetadata_Sony_E201_ElementCount; ++i)
                    if (AcquisitionMetadata_Sony_E201_Lists[i] && !AcquisitionMetadata_Sony_E201_Lists[i]->empty())
                    {
                        string ElementName(Mxf_AcquisitionMetadata_Sony_E201_ElementName[i]);
                        string ElementName_FirstFrame(ElementName+"_FirstFrame");
                        string ElementName_Values(ElementName+"_Values");
                        string ElementName_FrameCounts(ElementName+"_FrameCounts");
                        Fill(Stream_Other, 0, ElementName_FirstFrame.c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[0].Value);
                        switch (i)
                        {
                            case  0 : //FocusDistance
                            case  4 : //HyperfocalDistance
                            case  5 : //NearFocusDistance
                            case  6 : //FarFocusDistance
                            case  8 : //EntrancePupilPosition
                                Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                                if (AcquisitionMetadataLists[0xE203] && !AcquisitionMetadataLists[0xE203]->empty()) //Calibration Type
                                    Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[0].Value+' '+(*AcquisitionMetadataLists[0xE203])[0].Value);
                                break;
                            case  3 : //EffectiveFocaleLength
                                Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                                Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[0].Value+" mm");
                                break;
                            case  7 : //HorizontalFieldOfView
                                Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                                Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[0].Value+"\xc2\xB0");
                                break;
                            default : ;
                        }
                        for (size_t List_Pos=0; List_Pos<AcquisitionMetadata_Sony_E201_Lists[i]->size(); List_Pos++)
                        {
                            Fill(Stream_Other, 0, ElementName_Values.c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[List_Pos].Value);
                            Fill(Stream_Other, 0, ElementName_FrameCounts.c_str(), (*AcquisitionMetadata_Sony_E201_Lists[i])[List_Pos].FrameCount);
                        }
                        Fill_SetOptions(Stream_Other, 0, ElementName_Values.c_str(), "N NT");
                        Fill_SetOptions(Stream_Other, 0, ElementName_FrameCounts.c_str(), "N NT");
                    }
            }
            else if (AcquisitionMetadataLists[Pos] && !AcquisitionMetadataLists[Pos]->empty())
            {
                string ElementName(Mxf_AcquisitionMetadata_ElementName((int16u)Pos, UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony));
                string ElementName_FirstFrame(ElementName+"_FirstFrame");
                string ElementName_Values(ElementName+"_Values");
                string ElementName_FrameCounts(ElementName+"_FrameCounts");
                Fill(Stream_Other, 0, ElementName_FirstFrame.c_str(), (*AcquisitionMetadataLists[Pos])[0].Value);
                switch (Pos)
                {
                    case 0x8001 : //FocusPosition_ImagePlane
                    case 0x8002 : //FocusPosition_FrontLensVertex
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" m");
                        break;
                    case 0x8004 : //LensZoom35mmStillCameraEquivalent
                    case 0x8005 : //LensZoomActualFocalLength
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" mm");
                        break;
                    case 0x8006 : //OpticalExtenderMagnification
                    case 0x8009 : //IrisRingPosition
                    case 0x800A : //FocusRingPosition
                    case 0x800B : //ZoomRingPosition
                    case 0x810C : //ElectricalExtenderMagnification
                    case 0x810F : //CameraMasterBlackLevel
                    case 0x8110 : //CameraKneePoint
                    case 0x8112 : //CameraLuminanceDynamicRange
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+"%");
                        break;
                    case 0x8104 : //ImagerDimension_EffectiveWidth
                    case 0x8105 : //ImagerDimension_EffectiveHeight
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" mm");
                        break;
                    case 0x8106 : //CaptureFrameRate
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" fps");
                        break;
                    case 0x8108 : //ShutterSpeed_Angle
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+"\xc2\xB0");
                        break;
                    case 0x8109 : //ShutterSpeed_Time
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" s");
                        break;
                    case 0x810A : //CameraMasterGainAdjustment
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" dB");
                        break;
                    case 0x810E : //WhiteBalance
                        Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                        Fill(Stream_Other, 0, (ElementName_FirstFrame+"/String").c_str(), (*AcquisitionMetadataLists[Pos])[0].Value+" K");
                        break;
                    default : ;
                }
                if (UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony && Pos==0xE203) // Calibration Type, not for display
                    Fill_SetOptions(Stream_Other, 0, ElementName_FirstFrame.c_str(), "N NT");
                for (size_t List_Pos=0; List_Pos<AcquisitionMetadataLists[Pos]->size(); List_Pos++)
                {
                    Fill(Stream_Other, 0, ElementName_Values.c_str(), (*AcquisitionMetadataLists[Pos])[List_Pos].Value);
                    Fill(Stream_Other, 0, ElementName_FrameCounts.c_str(), (*AcquisitionMetadataLists[Pos])[List_Pos].FrameCount);
                }
                Fill_SetOptions(Stream_Other, 0, ElementName_Values.c_str(), "N NT");
                Fill_SetOptions(Stream_Other, 0, ElementName_FrameCounts.c_str(), "N NT");
            }
        }
    }

    for (const auto& Parser : ToMergeLater)
    {
        if (Parser->Count_Get(Stream_Video))
        {
            for (const auto& Elem : MXFGenericStreamDataElementKey)
            {
                if (Elem.second.Parser->Count_Get(Stream_Video) && Elem.second.Parser->Retrieve_Const(Stream_General, 0, General_Format)==Parser->Retrieve_Const(Stream_General, 0, General_Format))
                    Parser->Merge(*Elem.second.Parser, Stream_Video, 0, 0);
            }
            if (!Count_Get(Stream_Video))
                Stream_Prepare(Stream_Video);
            Merge(*Parser, Stream_Video, 0, 0);
        }
        delete Parser;
    }
    ToMergeLater.clear();

    //Metadata
    for (const auto& Elem : MXFGenericStreamDataElementKey)
    {
        if (!Elem.second.Parser || !Count_Get(Stream_Other))
            continue;
        Stream_Prepare(Stream_Other);
        Fill(Stream_Other, StreamPos_Last, Other_MuxingMode, "Generic Stream Data");
        Fill(Stream_Other, StreamPos_Last, "MuxingMode_MoreInfo", "Contains additional metadata for other tracks");
        Merge(*Elem.second.Parser, Stream_Other, 0, StreamPos_Last);

        auto SID = Elem.second.SID;
        if (SID) {
            for (const auto& Descriptor : Descriptors) {
                if (Descriptor.second.SID == SID) {
                    for (const auto& Info : Descriptor.second.Infos) {
                        Fill(Stream_Other, StreamPos_Last, Info.first.c_str(), Info.second);
                    }
                }
            }
        }
    }
    if (DolbyAudioMetadata) //Before ADM for having content before all ADM stuff
        Merge(*DolbyAudioMetadata, Stream_Audio, 0, 0);
    if (Adm)
    {
        Finish(Adm);
        Merge(*Adm, Stream_Audio, 0, 0);
    }
    if (Adm_ForLaterMerge)
    {
        auto& S=*Adm_ForLaterMerge->Stream_More;
        size_t Start=(Stream_Audio<S.size() && !S[Stream_Audio].empty())?S[Stream_Audio][0].size():0;
        ((File_Iab*)Adm_ForLaterMerge)->Streams_Fill_ForAdm();
        if (Adm)
        {
            //Check if compatible
            size_t End=(Stream_Audio<S.size() && !S[Stream_Audio].empty())?S[Stream_Audio][0].size():0;
            for (size_t i=Start; i<End; i++)
            {
                const auto& Field_Name=S[Stream_Audio][0][i][Info_Name];
                const auto& Field_Text_Iab=S[Stream_Audio][0][i][Info_Text];
                const auto& Field_Text_Adm=Retrieve_Const(Stream_Audio, 0, Field_Name.To_UTF8().c_str());
                if ((Field_Text_Iab==__T("Yes") && Field_Text_Adm.empty())
                 || (Field_Text_Iab!=__T("Yes") && Field_Text_Iab!=Field_Text_Adm && (Field_Name.find(__T(" ChannelLayout"))==string::npos
                                                                                   || ChannelLayout_2018_Rename(Field_Text_Iab, __T("IAB"))!=ChannelLayout_2018_Rename(Field_Text_Adm, __T("ADM")))))
                {
                    Fill(Stream_Audio, 0, "Warnings", "Potential IAB/ADM coherency issues detected");
                }

            }
        }
        else
        {
            //Merge
            Merge(*Adm_ForLaterMerge, Stream_Audio, 0, 0);
        }
    }
    if (DolbyAudioMetadata) //After ADM for having content inside ADM stuff
        ((File_DolbyAudioMetadata*)DolbyAudioMetadata)->Merge(*this, 0);
    if (Adm && (!DolbyAudioMetadata || !((File_DolbyAudioMetadata*)DolbyAudioMetadata)->HasSegment9) && Retrieve_Const(Stream_Audio, 0, "AdmProfile_Format")==__T("Dolby Atmos Master"))
    {
        Clear(Stream_Audio, 0, "AdmProfile");
        Clear(Stream_Audio, 0, "AdmProfile_Format");
        Clear(Stream_Audio, 0, "AdmProfile_Version");
    }

    //Commercial names
    Streams_Finish_CommercialNames();

    Merge_Conformance();
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Preface (const int128u PrefaceUID)
{
    prefaces::iterator Preface=Prefaces.find(PrefaceUID);
    if (Preface==Prefaces.end())
        return;

    //ContentStorage
    Streams_Finish_ContentStorage(Preface->second.ContentStorage);

    //ContenStorage, for AS11
    Streams_Finish_ContentStorage_ForAS11(Preface->second.ContentStorage);

    //Identifications
    for (size_t Pos=0; Pos<Preface->second.Identifications.size(); Pos++)
        Streams_Finish_Identification(Preface->second.Identifications[Pos]);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Preface_ForTimeCode (const int128u PrefaceUID)
{
    prefaces::iterator Preface=Prefaces.find(PrefaceUID);
    if (Preface==Prefaces.end())
        return;

    //ContentStorage
    Streams_Finish_ContentStorage_ForTimeCode(Preface->second.ContentStorage);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_ContentStorage (const int128u ContentStorageUID)
{
    contentstorages::iterator ContentStorage=ContentStorages.find(ContentStorageUID);
    if (ContentStorage==ContentStorages.end())
        return;

    for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
        Streams_Finish_Package(ContentStorage->second.Packages[Pos]);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_ContentStorage_ForTimeCode (const int128u ContentStorageUID)
{
    contentstorages::iterator ContentStorage=ContentStorages.find(ContentStorageUID);
    if (ContentStorage==ContentStorages.end())
        return;

    //Searching the right Time code track first TODO: this is an hack in order to get material or source time code, we need to have something more conform in the future
    // Material Package then Source Package
    for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
    {
        packages::iterator Package=Packages.find(ContentStorage->second.Packages[Pos]);
        if (Package!=Packages.end() && !Package->second.IsSourcePackage)
            Streams_Finish_Package_ForTimeCode(ContentStorage->second.Packages[Pos]);
    }
    for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
    {
        packages::iterator Package=Packages.find(ContentStorage->second.Packages[Pos]);
        if (Package!=Packages.end() && Package->second.IsSourcePackage)
            Streams_Finish_Package_ForTimeCode(ContentStorage->second.Packages[Pos]);
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_ContentStorage_ForAS11 (const int128u ContentStorageUID)
{
    contentstorages::iterator ContentStorage=ContentStorages.find(ContentStorageUID);
    if (ContentStorage==ContentStorages.end())
        return;

    for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
        Streams_Finish_Package_ForAS11(ContentStorage->second.Packages[Pos]);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Package (const int128u PackageUID)
{
    packages::iterator Package=Packages.find(PackageUID);
    if (Package==Packages.end() || !Package->second.IsSourcePackage)
        return;

    for (size_t Pos=0; Pos<Package->second.Tracks.size(); Pos++)
        Streams_Finish_Track(Package->second.Tracks[Pos]);

    Streams_Finish_Descriptor(Package->second.Descriptor, PackageUID);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Package_ForTimeCode (const int128u PackageUID)
{
    packages::iterator Package=Packages.find(PackageUID);
    if (Package==Packages.end())
        return;

    for (size_t Pos=0; Pos<Package->second.Tracks.size(); Pos++)
        Streams_Finish_Track_ForTimeCode(Package->second.Tracks[Pos], Package->second.IsSourcePackage);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Package_ForAS11 (const int128u PackageUID)
{
    packages::iterator Package=Packages.find(PackageUID);
    if (Package==Packages.end() || Package->second.IsSourcePackage)
        return;

    for (size_t Pos=0; Pos<Package->second.Tracks.size(); Pos++)
        Streams_Finish_Track_ForAS11(Package->second.Tracks[Pos]);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Track(const int128u TrackUID)
{
    tracks::iterator Track=Tracks.find(TrackUID);
    if (Track==Tracks.end() || Track->second.Stream_Finish_Done)
        return;

    StreamKind_Last=Stream_Max;
    StreamPos_Last=(size_t)-1;

    Streams_Finish_Essence(Track->second.TrackNumber, TrackUID);

    //Sequence
    Streams_Finish_Component(Track->second.Sequence, Track->second.EditRate_Real?Track->second.EditRate_Real:Track->second.EditRate, Track->second.TrackID, Track->second.Origin);

    //Done
    Track->second.Stream_Finish_Done=true;
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Track_ForTimeCode(const int128u TrackUID, bool IsSourcePackage)
{
    tracks::iterator Track=Tracks.find(TrackUID);
    if (Track==Tracks.end() || Track->second.Stream_Finish_Done)
        return;

    StreamKind_Last=Stream_Max;
    StreamPos_Last=(size_t)-1;

    //Sequence
    Streams_Finish_Component_ForTimeCode(Track->second.Sequence, Track->second.EditRate_Real?Track->second.EditRate_Real:Track->second.EditRate, Track->second.TrackID, Track->second.Origin, IsSourcePackage, Track->second.TrackName);
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Track_ForAS11(const int128u TrackUID)
{
    tracks::iterator Track=Tracks.find(TrackUID);
    if (Track==Tracks.end() || Track->second.Stream_Finish_Done)
        return;

    StreamKind_Last=Stream_Max;
    StreamPos_Last=(size_t)-1;

    //Sequence
    Streams_Finish_Component_ForAS11(Track->second.Sequence, Track->second.EditRate_Real?Track->second.EditRate_Real:Track->second.EditRate, Track->second.TrackID, Track->second.Origin);

    //TrackName
    if (StreamKind_Last!=Stream_Max && !Track->second.TrackName.empty())
        Fill(StreamKind_Last, StreamPos_Last, "Title", Track->second.TrackName);

    //Done
    Track->second.Stream_Finish_Done=true;
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Essence(int32u EssenceUID, int128u TrackUID)
{
    essences::iterator Essence=Essences.find(EssenceUID);
    if (Essence==Essences.end() || Essence->second.Stream_Finish_Done)
        return;

    if (Essence->second.Parsers.size()!=1)
        return;
    parsers::iterator Parser=Essence->second.Parsers.begin();

    //Descriptive Metadata
    std::vector<int128u> ProductionFrameworks_List;
    int32u TrackID=(int32u)-1;
    tracks::iterator Track=Tracks.find(TrackUID);
    if (Track!=Tracks.end())
        TrackID=Track->second.TrackID;

    for (dmsegments::iterator DescriptiveMarker=DescriptiveMarkers.begin(); DescriptiveMarker!=DescriptiveMarkers.end(); ++DescriptiveMarker)
        for (size_t Pos=0; Pos<DescriptiveMarker->second.TrackIDs.size(); Pos++)
            if (DescriptiveMarker->second.TrackIDs[Pos]==TrackID)
                ProductionFrameworks_List.push_back(DescriptiveMarker->second.Framework);

    if (Config->ParseSpeed<1.0 && !(*Parser)->Status[IsFinished])
    {
        Fill(*Parser);
        (*Parser)->Open_Buffer_Unsynch();
    }
    Finish(*Parser);
    StreamKind_Last=Stream_Max;
    if ((*Parser)->Count_Get(Stream_Video) && (*Parser)->Get(Stream_General, 0, General_Format)!=__T("Dolby Vision Metadata") && (*Parser)->Get(Stream_General, 0, General_Format) != __T("HDR Vivid Metadata"))
    {
        Stream_Prepare(Stream_Video);
        if (IsSub)
            Fill(Stream_Video, StreamPos_Last, Video_MuxingMode, "MXF");
    }
    else if ((*Parser)->Count_Get(Stream_Audio))
        Stream_Prepare(Stream_Audio);
    else if ((*Parser)->Count_Get(Stream_Text))
        Stream_Prepare(Stream_Text);
    else if ((*Parser)->Count_Get(Stream_Other))
        Stream_Prepare(Stream_Other);
    else if (Essence->second.StreamKind!=Stream_Max)
        Stream_Prepare(Essence->second.StreamKind);
    else
    {
        for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
            if (Descriptor->second.LinkedTrackID==Essence->second.TrackID)
            {
                if (Descriptor->second.StreamKind!=Stream_Max)
                {
                    Stream_Prepare(Descriptor->second.StreamKind);
                    Descriptor->second.StreamPos=StreamPos_Last;
                }
                break;
            }
        if (StreamKind_Last==Stream_Max)
            return; //Not found
    }
    if (IsSub)
        Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", "MXF");

    for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
        if (Descriptor->second.LinkedTrackID==Essence->second.TrackID)
        {
            if (Descriptor->second.StreamKind!=Stream_Max)
                Descriptor->second.StreamPos=StreamPos_Last;
            break;
        }

    for (std::map<std::string, Ztring>::iterator Info=Essence->second.Infos.begin(); Info!=Essence->second.Infos.end(); ++Info)
        Fill(StreamKind_Last, StreamPos_Last, Info->first.c_str(), Info->second, true);
    if (MxfTimeCodeForDelay.IsInit())
    {
        bool MxfTimeCodeForDelay_Is1001 = false;
        if (Demux_Rate) {
            MxfTimeCodeForDelay_Is1001 = Demux_Rate != (int)Demux_Rate;
        }
        else if (MxfTimeCodeForDelay.InstanceUID != 0) {
            for (const auto& Component : Components) {
                for (const auto& StructuralComponent : Component.second.StructuralComponents) {
                    if (StructuralComponent == MxfTimeCodeForDelay.InstanceUID) {
                        for (const auto& Track : Tracks) {
                            if (Track.second.Sequence == Component.first) {
                                if (Track.second.EditRate && Track.second.EditRate != (int)Track.second.EditRate) {
                                    MxfTimeCodeForDelay_Is1001 = true; // TODO: if audio with timecode with audio sampling rate, it does not work, is there a way to know that it is 1/1.001 based?
                                }
                            }
                        }
                    }
                }
            }
        }

        const float64 TimeCode_StartTimecode_Temp = MxfTimeCodeForDelay.Get_TimeCode_StartTimecode_Temp(Config->File_IgnoreEditsBefore, MxfTimeCodeForDelay_Is1001);
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay), TimeCode_StartTimecode_Temp*1000, 0, true);
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source), "Container");
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_DropFrame), MxfTimeCodeForDelay.DropFrame?"Yes":"No");

        //TimeCode TC(MxfTimeCodeForDelay.StartTimecode, MxfTimeCodeForDelay.RoundedTimecodeBase, MxfTimeCodeForDelay.DropFrame);
        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_FirstFrame), TC.ToString().c_str());
        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_Source), "Time code track (stripped)");
    }
    size_t SDTI_TimeCode_StartTimecode_StreamPos_Last{};
    if (SDTI_TimeCode_StartTimecode.IsSet())
    {
        SDTI_TimeCode_StartTimecode_StreamPos_Last=StreamPos_Last;
        Fill(StreamKind_Last, StreamPos_Last, "Delay_SDTI", (int64s)SDTI_TimeCode_StartTimecode.ToMilliseconds());
        if (StreamKind_Last!=Stream_Max)
            Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Delay_SDTI", "N NT");

        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_FirstFrame), SDTI_TimeCode_StartTimecode.c_str());
        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_Source), "SDTI");
    }
    size_t SystemScheme1_TimeCodeArray_StartTimecode_StreamPos_Last{};
    if (!SystemScheme1s.empty() && !SystemScheme1s.begin()->second.TimeCodeArray_StartTimecodes.empty())
    {
        SystemScheme1_TimeCodeArray_StartTimecode_StreamPos_Last=StreamPos_Last;
        Fill(StreamKind_Last, StreamPos_Last, "Delay_SystemScheme1", (int64s)SystemScheme1s.begin()->second.TimeCodeArray_StartTimecodes.front().ToMilliseconds());
        if (StreamKind_Last!=Stream_Max)
            Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Delay_SystemScheme1", "N NT");

        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_FirstFrame), SystemScheme1_TimeCodeArray_StartTimecode.c_str());
        //Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_TimeCode_Source), "System scheme 1");
    }

    //Special case - Multiple sub-streams in a stream
    if (((*Parser)->Retrieve(Stream_General, 0, General_Format)==__T("ChannelGrouping") || (*Parser)->Count_Get(StreamKind_Last)>1) && (*Parser)->Count_Get(Stream_Audio))
    {
        //Before
        if (StreamKind_Last==Stream_Audio)
        {
            Clear(Stream_Audio, StreamPos_Last, Audio_Format_Settings_Sign);
        }
        ZtringList StreamSave; StreamSave.Write((*File__Analyze::Stream)[StreamKind_Last][StreamPos_Last].Read());
        ZtringListList StreamMoreSave; StreamMoreSave.Write((*Stream_More)[StreamKind_Last][StreamPos_Last].Read());

        //Erasing former streams data
        stream_t NewKind=StreamKind_Last;
        size_t NewPos1;
        Ztring ID;
        if ((*Parser)->Retrieve(Stream_General, 0, General_Format)==__T("ChannelGrouping"))
        {
            //Searching second stream
            size_t StreamPos_Difference=Essence->second.StreamPos-Essence->second.StreamPos_Initial;
            essences::iterator Essence1=Essence;
            --Essence1;
            Essence->second.StreamPos=Essence1->second.StreamPos;
            for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
            {
                if (Descriptor->second.LinkedTrackID==Essence1->second.TrackID)
                    Descriptor->second.StreamPos=Essence1->second.StreamPos;
                if (Descriptor->second.LinkedTrackID==Essence->second.TrackID)
                    Descriptor->second.StreamPos=Essence->second.StreamPos;
            }

            //Removing the 2 corresponding streams
            NewPos1=Essence->second.StreamPos_Initial-1+StreamPos_Difference;
            ID=Ztring::ToZtring(Essence1->second.TrackID)+__T(" / ")+Ztring::ToZtring(Essence->second.TrackID);

            Stream_Erase(NewKind, NewPos1+1);
            Stream_Erase(NewKind, NewPos1);

            essences::iterator NextStream=Essence1;
            ++NextStream;
            size_t NewAudio_Count=Essence->second.Parsers[0]->Count_Get(Stream_Audio);
            while (NextStream!=Essences.end())
            {
                if (NextStream->second.StreamKind==Stream_Audio)
                {
                    NextStream->second.StreamPos-=2;
                    NextStream->second.StreamPos+=NewAudio_Count;
                    NextStream->second.StreamPos_Initial-=2;
                    NextStream->second.StreamPos_Initial+=NewAudio_Count;
                }
                ++NextStream;
            }
        }
        else
        {
            NewPos1=StreamPos_Last;
            ID=Ztring::ToZtring(Essence->second.TrackID);
            Stream_Erase(NewKind, NewPos1);
        }

        //After
        for (size_t StreamPos=0; StreamPos<(*Parser)->Count_Get(NewKind); StreamPos++)
        {
            Stream_Prepare(NewKind, NewPos1+StreamPos);
            Merge(*(*Parser), StreamKind_Last, StreamPos, StreamPos_Last);
            Ztring Parser_ID=Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
            Fill(StreamKind_Last, StreamPos_Last, General_ID, ID+(Parser_ID.empty()?Ztring():(__T("-")+Parser_ID)), true);
            for (size_t Pos=0; Pos<StreamSave.size(); Pos++)
            {
                if (Pos==Fill_Parameter(StreamKind_Last, Generic_BitRate) && (*Parser)->Count_Get(NewKind)>1 && (!StreamSave[Pos].empty() || StreamPos))
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Encoded), StreamPos?0:(StreamSave[Pos].To_int64u()*2), 10, true);
                else if (Pos==Fill_Parameter(StreamKind_Last, Generic_StreamSize) && (*Parser)->Count_Get(NewKind)>1 && (!StreamSave[Pos].empty() || StreamPos))
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize_Encoded), StreamPos?0:(StreamSave[Pos].To_int64u()*2), 10, true);
                else if (Retrieve(StreamKind_Last, StreamPos_Last, Pos).empty())
                    Fill(StreamKind_Last, StreamPos_Last, Pos, StreamSave[Pos]);
            }
            for (size_t Pos=0; Pos<StreamMoreSave.size(); Pos++)
            {
                Fill(StreamKind_Last, StreamPos_Last, StreamMoreSave(Pos, 0).To_UTF8().c_str(), StreamMoreSave(Pos, 1));
                if (StreamMoreSave(Pos, Info_Name)==__T("Delay_SDTI"))
                    Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Delay_SDTI", "N NT");
                if (StreamMoreSave(Pos, Info_Name)==__T("Delay_SystemScheme1"))
                    Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Delay_SystemScheme1", "N NT");
            }

            for (size_t Pos=0; Pos<ProductionFrameworks_List.size(); Pos++)
            {
                dmscheme1s::iterator ProductionFramework=ProductionFrameworks.find(ProductionFrameworks_List[Pos]);
                if (ProductionFramework!=ProductionFrameworks.end())
                {
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Language), ProductionFramework->second.PrimaryExtendedSpokenLanguage, true);
                }
            }
        }
    }
    else //Normal
    {
        //From descriptor
        if ((*Parser)->Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("PCM")) // MXF handles channel count only with PCM, not with compressed data
            for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                if (Descriptor->second.LinkedTrackID==Essence->second.TrackID && Descriptor->second.StreamKind==Stream_Audio && StreamKind_Last==Stream_Audio && Descriptor->second.ChannelCount!=(int32u)-1)
                {
                    Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, Descriptor->second.ChannelCount);
                    break;
                }
        if (!Adm_ForLaterMerge && (*Parser)->Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("IAB"))
            Adm_ForLaterMerge=(File_Iab*)*Parser; // Will be used later for ADM

        Merge(*(*Parser), StreamKind_Last, 0, StreamPos_Last);

        Ztring LawRating=(*Parser)->Retrieve(Stream_General, 0, General_LawRating);
        if (!LawRating.empty())
            Fill(Stream_General, 0, General_LawRating, LawRating, true);
        Ztring Title=(*Parser)->Retrieve(Stream_General, 0, General_Title);
        if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
            Fill(Stream_General, 0, General_Title, Title);

        for (size_t Pos=0; Pos<ProductionFrameworks_List.size(); Pos++)
        {
            dmscheme1s::iterator ProductionFramework=ProductionFrameworks.find(ProductionFrameworks_List[Pos]);
            if (ProductionFramework!=ProductionFrameworks.end())
            {
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Language), ProductionFramework->second.PrimaryExtendedSpokenLanguage, true);
            }
        }

        for (size_t StreamPos=1; StreamPos<(*Parser)->Count_Get(StreamKind_Last); StreamPos++) //If more than 1 stream, TODO: better way to do this
        {
            Stream_Prepare(StreamKind_Last);
            Fill(StreamKind_Last, StreamPos_Last, "Title", Retrieve_Const(StreamKind_Last, StreamPos_Last-StreamPos, "Title"));
            Merge(*(*Parser), StreamKind_Last, StreamPos, StreamPos_Last);
        }

        if (StreamKind_Last!=Stream_Other && (*Parser)->Count_Get(Stream_Other))
        {
            stream_t StreamKind_Last_Main=StreamKind_Last;
            size_t StreamPos_Last_Main=StreamPos_Last;
            for (size_t StreamPos=0; StreamPos<(*Parser)->Count_Get(Stream_Other); StreamPos++)
            {
                Stream_Prepare(Stream_Other);
                Merge(*(*Parser), Stream_Other, StreamPos, StreamPos_Last);
            }
            Streams_Finish_Essence_FillID(EssenceUID, TrackUID);
            StreamKind_Last=StreamKind_Last_Main;
            StreamPos_Last=StreamPos_Last_Main;
        }
    }

    Streams_Finish_Essence_FillID(EssenceUID, TrackUID);

    //Special case - DV
    #if defined(MEDIAINFO_DVDIF_YES)
        if (StreamKind_Last==Stream_Video && Retrieve(Stream_Video, StreamPos_Last, Video_Format)==__T("DV"))
        {
            if (Retrieve(Stream_General, 0, General_Recorded_Date).empty())
                Fill(Stream_General, 0, General_Recorded_Date, (*Parser)->Retrieve(Stream_General, 0, General_Recorded_Date));

            //Video and Audio are together
            size_t Audio_Count=(*Parser)->Count_Get(Stream_Audio);
            for (size_t Audio_Pos=0; Audio_Pos<Audio_Count; Audio_Pos++)
            {
                Fill_Flush();
                Stream_Prepare(Stream_Audio);
                size_t Pos=Count_Get(Stream_Audio)-1;
                (*Parser)->Finish();
                if (MxfTimeCodeForDelay.IsInit())
                {
                    bool MxfTimeCodeForDelay_Is1001 = false;
                    if (Demux_Rate) {
                        MxfTimeCodeForDelay_Is1001 = Demux_Rate != (int)Demux_Rate;
                    }
                    else if (MxfTimeCodeForDelay.InstanceUID != 0) {
                        for (const auto& Component : Components) {
                            for (const auto& StructuralComponent : Component.second.StructuralComponents) {
                                if (StructuralComponent == MxfTimeCodeForDelay.InstanceUID) {
                                    for (const auto& Track : Tracks) {
                                        if (Track.second.Sequence == Component.first) {
                                            if (Track.second.EditRate && Track.second.EditRate != (int)Track.second.EditRate) {
                                                MxfTimeCodeForDelay_Is1001 = true; // TODO: if audio with timecode with audio sampling rate, it does not work, is there a way to know that it is 1/1.001 based?
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    const float64 TimeCode_StartTimecode_Temp = MxfTimeCodeForDelay.Get_TimeCode_StartTimecode_Temp(Config->File_IgnoreEditsBefore, MxfTimeCodeForDelay_Is1001);
                    Fill(Stream_Audio, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay), TimeCode_StartTimecode_Temp*1000, 0, true);
                    Fill(Stream_Audio, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source), "Container");
                }
                Merge(*(*Parser), Stream_Audio, Audio_Pos, StreamPos_Last);
                if (Retrieve(Stream_Audio, Pos, Audio_MuxingMode).empty())
                    Fill(Stream_Audio, Pos, Audio_MuxingMode, Retrieve(Stream_Video, Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1), Video_Format), true);
                else
                    Fill(Stream_Audio, Pos, Audio_MuxingMode, Retrieve(Stream_Video, Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1), Video_Format)+__T(" / ")+Retrieve(Stream_Audio, Pos, Audio_MuxingMode), true);
                Fill(Stream_Audio, Pos, Audio_Duration, Retrieve(Stream_Video, Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1), Video_Duration));
                Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, 0); //Included in the DV stream size
                Ztring ID=Retrieve(Stream_Audio, Pos, Audio_ID);
                Fill(Stream_Audio, Pos, Audio_ID, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_ID)+__T("-")+ID, true);
                Fill(Stream_Audio, Pos, Audio_ID_String, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_ID_String)+__T("-")+ID, true);
                Fill(Stream_Audio, Pos, Audio_Title, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_Title), true);
            }

            StreamKind_Last=Stream_Video;
            StreamPos_Last=Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1);
        }
    #endif

    //Special case - MPEG Video + Captions
    if (StreamKind_Last==Stream_Video && (*Parser)->Count_Get(Stream_Text))
    {
        //Video and Text are together
        size_t Parser_Text_Count=(*Parser)->Count_Get(Stream_Text);
        for (size_t Parser_Text_Pos=0; Parser_Text_Pos<Parser_Text_Count; Parser_Text_Pos++)
        {
            size_t StreamPos_Video=StreamPos_Last;
            Fill_Flush();
            Stream_Prepare(Stream_Text);
            (*Parser)->Finish();
            if (MxfTimeCodeForDelay.IsInit())
            {
                bool MxfTimeCodeForDelay_Is1001 = false;
                if (Demux_Rate) {
                    MxfTimeCodeForDelay_Is1001 = Demux_Rate != (int)Demux_Rate;
                }
                else if (MxfTimeCodeForDelay.InstanceUID != 0) {
                    for (const auto& Component : Components) {
                        for (const auto& StructuralComponent : Component.second.StructuralComponents) {
                            if (StructuralComponent == MxfTimeCodeForDelay.InstanceUID) {
                                for (const auto& Track : Tracks) {
                                    if (Track.second.Sequence == Component.first) {
                                        if (Track.second.EditRate && Track.second.EditRate != (int)Track.second.EditRate) {
                                            MxfTimeCodeForDelay_Is1001 = true; // TODO: if audio with timecode with audio sampling rate, it does not work, is there a way to know that it is 1/1.001 based?
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                const float64 TimeCode_StartTimecode_Temp = MxfTimeCodeForDelay.Get_TimeCode_StartTimecode_Temp(Config->File_IgnoreEditsBefore, MxfTimeCodeForDelay_Is1001);
                Fill(Stream_Text, Parser_Text_Pos, Fill_Parameter(StreamKind_Last, Generic_Delay), TimeCode_StartTimecode_Temp*1000, 0, true);
                Fill(Stream_Text, Parser_Text_Pos, Fill_Parameter(StreamKind_Last, Generic_Delay_Source), "Container");
            }
            Merge(*(*Parser), Stream_Text, Parser_Text_Pos, StreamPos_Last);
            Fill(Stream_Text, StreamPos_Last, Text_Duration, Retrieve(Stream_Video, StreamPos_Video, Video_Duration));
            Ztring ID=Retrieve(Stream_Text, StreamPos_Last, Text_ID);
            if (Retrieve(Stream_Text, StreamPos_Last, Text_MuxingMode).find(__T("Ancillary"))!=string::npos)
            {
                for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                    if (Descriptor->second.Type==descriptor::Type_AncPackets)
                    {
                        Fill(Stream_Text, StreamPos_Last, Text_ID, Ztring::ToZtring(Descriptor->second.LinkedTrackID)+__T("-")+ID, true);
                        Fill(Stream_Text, StreamPos_Last, Text_ID_String, Ztring::ToZtring(Descriptor->second.LinkedTrackID)+__T("-")+ID, true);
                        Fill(Stream_Text, StreamPos_Last, Text_Title, Tracks[TrackUID].TrackName, true);
                        break;
                    }
            }
            else
            {
                Fill(Stream_Text, StreamPos_Last, Text_ID, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_ID)+__T("-")+ID, true);
                Fill(Stream_Text, StreamPos_Last, Text_ID_String, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_ID_String)+__T("-")+ID, true);
                Fill(Stream_Text, StreamPos_Last, Text_Title, Retrieve(Stream_Video, Count_Get(Stream_Video)-1, Video_Title), true);
            }

            for (size_t Pos=0; Pos<ProductionFrameworks_List.size(); Pos++)
            {
                dmscheme1s::iterator ProductionFramework=ProductionFrameworks.find(ProductionFrameworks_List[Pos]);
                if (ProductionFramework!=ProductionFrameworks.end())
                {
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Language), ProductionFramework->second.PrimaryExtendedSpokenLanguage, true);
                }
            }
        }

        Ztring LawRating=(*Parser)->Retrieve(Stream_General, 0, General_LawRating);
        if (!LawRating.empty())
            Fill(Stream_General, 0, General_LawRating, LawRating, true);
        Ztring Title=(*Parser)->Retrieve(Stream_General, 0, General_Title);
        if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
            Fill(Stream_General, 0, General_Title, Title);

        StreamKind_Last=Stream_Video;
        StreamPos_Last=Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1);
    }

    //Stream size
    if (StreamKind_Last!=Stream_Max && Count_Get(Stream_Video)+Count_Get(Stream_Audio)==1 && Essence->second.Stream_Size!=(int64u)-1)
    {
        //TODO: Stream_Size is present only if there is one stream, so it works in most cases. We should find a better way.
        int64u Stream_Size=Essence->second.Stream_Size;
        if (Config->File_IgnoreEditsBefore || Config->File_IgnoreEditsAfter!=(int64u)-1)
        {
            int64u FrameCount_FromComponent=(int64u)-1;
            for (components::iterator Component=Components.begin(); Component!=Components.end(); ++Component)
                if (FrameCount_FromComponent>Component->second.Duration)
                    FrameCount_FromComponent=Component->second.Duration;
            float64 EditRate_FromTrack=DBL_MAX;
            for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                if (EditRate_FromTrack>Track->second.EditRate)
                    EditRate_FromTrack=Track->second.EditRate;
            if (FrameCount_FromComponent!=(int64u)-1 && FrameCount_FromComponent && EditRate_FromTrack!=DBL_MAX && EditRate_FromTrack)
            {
                int64u FrameCount=FrameCount_FromComponent;
                int64u File_IgnoreEditsBefore=Config->File_IgnoreEditsBefore;
                if (File_IgnoreEditsBefore && Config->File_EditRate && (EditRate_FromTrack<Config->File_EditRate*0.9 || EditRate_FromTrack>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                    File_IgnoreEditsBefore=float64_int64s(((float64)File_IgnoreEditsBefore)/Config->File_EditRate*EditRate_FromTrack);
                int64u File_IgnoreEditsAfter=Config->File_IgnoreEditsAfter;
                if (File_IgnoreEditsAfter!=(int64u)-1 && Config->File_EditRate && (EditRate_FromTrack<Config->File_EditRate*0.9 || EditRate_FromTrack>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                    File_IgnoreEditsAfter=float64_int64s(((float64)File_IgnoreEditsAfter)/Config->File_EditRate*EditRate_FromTrack);
                if (File_IgnoreEditsAfter<FrameCount)
                    FrameCount=File_IgnoreEditsAfter;
                if (FrameCount<File_IgnoreEditsBefore)
                    FrameCount=File_IgnoreEditsBefore;
                FrameCount-=File_IgnoreEditsBefore;

                float64 Stream_Size_Temp=(float64)Stream_Size;
                Stream_Size_Temp/=FrameCount_FromComponent;
                Stream_Size_Temp*=FrameCount;
                Stream_Size=float64_int64s(Stream_Size_Temp);
            }
        }

        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), Stream_Size);
    }

    if (SDTI_TimeCode_StartTimecode.IsSet())
    {
        TimeCode TC=SDTI_TimeCode_StartTimecode;
        int32u FrameRate=float32_int32s(Retrieve_Const(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameRate)).To_float32());
        if (FrameRate)
            TC.SetFramesMax((int16u)(FrameRate-1));
        Fill(StreamKind_Last, SDTI_TimeCode_StartTimecode_StreamPos_Last, "Delay_SDTI", (int64s)TC.ToMilliseconds(), true, true);
    }
    if (!SystemScheme1s.empty() && !SystemScheme1s.begin()->second.TimeCodeArray_StartTimecodes.empty())
    {
        TimeCode TC=SystemScheme1s.begin()->second.TimeCodeArray_StartTimecodes.front();
        int32u FrameRate=float32_int32s(Retrieve_Const(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameRate)).To_float32());
        if (FrameRate)
            TC.SetFramesMax((int16u)(FrameRate-1));
        Fill(StreamKind_Last, SystemScheme1_TimeCodeArray_StartTimecode_StreamPos_Last, "Delay_SystemScheme1", (int64s)TC.ToMilliseconds(), true, true);
    }

    //Done
    Essence->second.Stream_Finish_Done=true;

    if ((*Parser))
    {
        // TODO: avoid this hack
        const auto& Format = (*Parser)->Retrieve_Const(Stream_General, 0, General_Format);
        if (Format == __T("Dolby Vision Metadata")
         || Format == __T("HDR Vivid Metadata"))
        {
            ToMergeLater.push_back(*Parser);
            *Parser = nullptr;
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Essence_FillID(int32u EssenceUID, int128u TrackUID)
{
    essences::iterator Essence=Essences.find(EssenceUID);
    if (Essence==Essences.end() || Essence->second.Stream_Finish_Done)
        return;

   parsers::iterator Parser=Essence->second.Parsers.begin();

   if (Retrieve(StreamKind_Last, StreamPos_Last, General_ID).empty() || StreamKind_Last==Stream_Text || StreamKind_Last==Stream_Other) //TODO: better way to do detect subID
    {
        //Looking for Material package TrackID
        int32u TrackID=(int32u)-1;
        for (packages::iterator SourcePackage=Packages.begin(); SourcePackage!=Packages.end(); ++SourcePackage)
            if (SourcePackage->second.PackageUID.hi.hi) //Looking fo a SourcePackage with PackageUID only
            {
                //Testing if the Track is in this SourcePackage
                for (size_t Tracks_Pos=0; Tracks_Pos<SourcePackage->second.Tracks.size(); Tracks_Pos++)
                    if (SourcePackage->second.Tracks[Tracks_Pos]==TrackUID)
                    {
                        tracks::iterator Track=Tracks.find(SourcePackage->second.Tracks[Tracks_Pos]);
                        if (Track!=Tracks.end())
                            TrackID=Track->second.TrackID;
                    }
            }

        Ztring ID;
        Ztring ID_String;
        if (TrackID!=(int32u)-1)
            ID=Ztring::ToZtring(TrackID);
        else if (Tracks[TrackUID].TrackID!=(int32u)-1)
            ID=Ztring::ToZtring(Tracks[TrackUID].TrackID);
        else
        {
            ID=Ztring::ToZtring(Essence->first);
            ID_String=Ztring::ToZtring(Essence->first, 16);
        }
        if (!ID.empty())
        {
            for (size_t StreamPos=StreamPos_Last-((*Parser)->Count_Get(StreamKind_Last)?((*Parser)->Count_Get(StreamKind_Last)-1):0); StreamPos<=StreamPos_Last; StreamPos++) //If more than 1 stream
            {
                Ztring ID_Temp(ID);
                if (!Retrieve(StreamKind_Last, StreamPos, General_ID).empty())
                {
                    ID_Temp+=__T("-");
                    ID_Temp+=Retrieve(StreamKind_Last, StreamPos, General_ID);
                }
                Fill(StreamKind_Last, StreamPos, General_ID, ID_Temp, true);
                if (!ID_String.empty())
                    Fill(StreamKind_Last, StreamPos, General_ID_String, ID_String, true);
            }
        }
        if (!Tracks[TrackUID].TrackName.empty())
        {
            for (size_t StreamPos=StreamPos_Last-((*Parser)->Count_Get(StreamKind_Last)?((*Parser)->Count_Get(StreamKind_Last)-1):0); StreamPos<=StreamPos_Last; StreamPos++) //If more than 1 stream
            {
                Ztring Title_Temp=Retrieve(StreamKind_Last, StreamPos, "Title");
                Fill(StreamKind_Last, StreamPos, "Title", Title_Temp.empty()?Tracks[TrackUID].TrackName:(Tracks[TrackUID].TrackName+__T(" - ")+Title_Temp), true);
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Descriptor(const int128u DescriptorUID, const int128u PackageUID)
{
    descriptors::iterator Descriptor=Descriptors.find(DescriptorUID);
    if (Descriptor==Descriptors.end())
        return;

    //Subs
    if (Descriptor->second.Type==descriptor::type_Mutiple)
    {
        for (size_t Pos=0; Pos<Descriptor->second.SubDescriptors.size(); Pos++)
            Streams_Finish_Descriptor(Descriptor->second.SubDescriptors[Pos], PackageUID);
        return; //Is not a real descriptor
    }

    StreamKind_Last=Descriptor->second.StreamKind;
    StreamPos_Last=Descriptor->second.StreamPos;
    if (StreamPos_Last==(size_t)-1)
    {
        for (size_t Pos=0; Pos<Count_Get(StreamKind_Last); Pos++)
        {
            Ztring ID=Retrieve(StreamKind_Last, Pos, General_ID);
            size_t ID_Dash_Pos=ID.find(__T('-'));
            if (ID_Dash_Pos!=string::npos)
                ID.resize(ID_Dash_Pos);
            if (Ztring::ToZtring(Descriptor->second.LinkedTrackID)==ID)
            {
                StreamPos_Last=Pos;
                break;
            }
        }
    }
    if (StreamPos_Last==(size_t)-1)
    {
        auto NotSubDescriptorCount=Descriptors.size();
        auto NotSubDescriptorID=Descriptor->first;
        for (const auto& SearchedDescriptor : Descriptors)
        {
            if (Locators.empty() && Descriptor->second.StreamKind!=Stream_Max && SearchedDescriptor.second.StreamKind!=Descriptor->second.StreamKind)
            {
                NotSubDescriptorCount--;
                continue;
            }
            auto IsSubDescriptor=false;
            for (const auto& SearchingDescriptor : Descriptors)
                for (const auto& SearchingDescriptor_SubID : SearchingDescriptor.second.SubDescriptors)
                    if (SearchedDescriptor.first==SearchingDescriptor_SubID)
                        IsSubDescriptor=true;
            NotSubDescriptorCount-=IsSubDescriptor;
            if (!IsSubDescriptor)
                NotSubDescriptorID=SearchedDescriptor.first;
        }
        if (NotSubDescriptorCount==1 && NotSubDescriptorID==Descriptor->first)
        {
            if (Count_Get(Descriptor->second.StreamKind)==0)
            {
                Stream_Prepare(Descriptor->second.StreamKind);
                if (Descriptor->second.LinkedTrackID!=(int32u)-1)
                    Fill(StreamKind_Last, StreamPos_Last, General_ID, Descriptor->second.LinkedTrackID);
            }
            else
                StreamPos_Last=0;
        }
        else if (Descriptor->second.LinkedTrackID!=(int32u)-1)
        {
            //Workaround for a specific file with same ID
            if (!Locators.empty())
                for (descriptors::iterator Descriptor1=Descriptors.begin(); Descriptor1!=Descriptor; ++Descriptor1)
                    if (Descriptor1->second.LinkedTrackID==Descriptor->second.LinkedTrackID)
                    {
                        IdIsAlwaysSame_Offset++;
                        break;
                    }

            Stream_Prepare(Descriptor->second.StreamKind);
            Fill(StreamKind_Last, StreamPos_Last, General_ID, Descriptor->second.LinkedTrackID+IdIsAlwaysSame_Offset);
        }
        else
        {
            //Looking for Material package TrackID
            packages::iterator SourcePackage=Packages.find(PackageUID);
            //We have the the right PackageUID, looking for SourceClip from Sequence from Track from MaterialPackage
            for (components::iterator SourceClip=Components.begin(); SourceClip!=Components.end(); ++SourceClip)
                if (SourceClip->second.SourcePackageID.lo==SourcePackage->second.PackageUID.lo) //int256u doesn't support yet ==
                {
                    //We have the right SourceClip, looking for the Sequence from Track from MaterialPackage
                    for (components::iterator Sequence=Components.begin(); Sequence!=Components.end(); ++Sequence)
                        for (size_t StructuralComponents_Pos=0; StructuralComponents_Pos<Sequence->second.StructuralComponents.size(); StructuralComponents_Pos++)
                            if (Sequence->second.StructuralComponents[StructuralComponents_Pos]==SourceClip->first)
                            {
                                //We have the right Sequence, looking for Track from MaterialPackage
                                for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                                {
                                    if (Track->second.Sequence==Sequence->first)
                                    {
                                        Ztring ID=Ztring::ToZtring(Track->second.TrackID);
                                        StreamKind_Last=Stream_Max;
                                        StreamPos_Last=(size_t)-1;
                                        for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                                            for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                                                if (ID==Retrieve((stream_t)StreamKind, StreamPos, General_ID))
                                                {
                                                    StreamKind_Last=(stream_t)StreamKind;
                                                    StreamPos_Last=(stream_t)StreamPos;
                                                }
                                        if (StreamPos_Last==(size_t)-1 && !Descriptor->second.Locators.empty()) //TODO: 1 file has a TimeCode stream linked to a video stream, and it is displayed if Locator test is removed. Why? AS02 files streams are not filled if I remove completely this block, why?
                                        {
                                            if (Descriptor->second.StreamKind!=Stream_Max)
                                                Stream_Prepare(Descriptor->second.StreamKind);
                                            if (Track->second.TrackID!=(int32u)-1)
                                            {
                                                if (Descriptor->second.LinkedTrackID==(int32u)-1)
                                                    Descriptor->second.LinkedTrackID=Track->second.TrackID;
                                                if (Descriptor->second.StreamKind!=Stream_Max)
                                                {
                                                    Fill(StreamKind_Last, StreamPos_Last, General_ID, ID);
                                                    Fill(StreamKind_Last, StreamPos_Last, "Title", Track->second.TrackName);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                }
        }
    }

    //Locators
    size_t Before_Count[Stream_Max];
    for (size_t Pos=0; Pos<Stream_Max; Pos++)
        Before_Count[Pos]=(size_t)-1;
    Before_Count[Stream_Video]=Count_Get(Stream_Video);
    Before_Count[Stream_Audio]=Count_Get(Stream_Audio);
    Before_Count[Stream_Text]=Count_Get(Stream_Text);
    for (size_t Locator_Pos=0; Locator_Pos<Descriptor->second.Locators.size(); Locator_Pos++)
    {
        //Locator
        Streams_Finish_Locator(DescriptorUID, Descriptor->second.Locators[Locator_Pos]);
    }

    if (StreamPos_Last==(size_t)-1 && Essences.size()==1)
    {
        //Only one essence, there is sometimes no LinkedTrackID
        if (Count_Get(Stream_Video)==1)
        {
            StreamKind_Last=Stream_Video;
            StreamPos_Last=0;
        }
        else if (Count_Get(Stream_Audio)==1)
        {
            StreamKind_Last=Stream_Audio;
            StreamPos_Last=0;
        }
    }

    Streams_Finish_Descriptor(Descriptor);
    auto StreamKind_Last_Sav=StreamKind_Last;
    auto StreamPos_Last_Sav=StreamPos_Last;
    auto ID_Sav=Ztring::ToZtring(Descriptor->second.LinkedTrackID);
    for (size_t StreamKind=Stream_Text; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
        {
            if (StreamKind==StreamKind_Last_Sav && StreamPos==StreamPos_Last_Sav)
                continue;
            auto ID=Retrieve_Const((stream_t)StreamKind, StreamPos, General_ID);
            size_t ID_SubStreamInfo_Pos=ID.find(__T('-'));
            if (ID_SubStreamInfo_Pos!=string::npos)
            {
                ID.resize(ID_SubStreamInfo_Pos);
            }
            if (ID==ID_Sav)
            {
                if (Descriptor->second.StreamKind==Stream_Video)
                {
                    Fill((stream_t)StreamKind, StreamPos, General_StreamOrder, Retrieve_Const(Stream_Video, 0, General_StreamOrder));
                    continue;
                }
                if (Descriptor->second.StreamKind==Stream_Audio) //TODO: handle correctly when ID is same in different tracks
                {
                    continue;
                }
                StreamKind_Last=(stream_t)StreamKind;
                StreamPos_Last=StreamPos;
                Streams_Finish_Descriptor(Descriptor);
            }
        }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Descriptor(descriptors::iterator Descriptor)
{
    size_t Before_Count[Stream_Max];
    for (size_t Pos=0; Pos<Stream_Max; Pos++)
        Before_Count[Pos]=(size_t)-1;
    Before_Count[Stream_Video]=Count_Get(Stream_Video);
    Before_Count[Stream_Audio]=Count_Get(Stream_Audio);
    Before_Count[Stream_Text]=Count_Get(Stream_Text);

    if (StreamKind_Last!=Stream_Max && StreamPos_Last!=(size_t)-1)
    {
        //Handling buggy files
        if (Descriptor->second.Is_Interlaced() && Descriptor->second.Height==1152 && Descriptor->second.Height_Display==1152 && Descriptor->second.Width==720) //Height value is height of the frame instead of the field
            Descriptor->second.Height_Display/=2;

        //ID
        if (Descriptor->second.LinkedTrackID!=(int32u)-1 && Retrieve(StreamKind_Last, StreamPos_Last, General_ID).empty())
        {
            for (size_t StreamKind=0; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=Before_Count[StreamKind]; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring ID=Retrieve((stream_t)StreamKind, StreamPos, General_ID);
                    if (ID.empty() || Config->File_ID_OnlyRoot_Get())
                        Fill((stream_t)StreamKind, StreamPos, General_ID, Descriptor->second.LinkedTrackID, 10, true);
                    else
                        Fill((stream_t)StreamKind, StreamPos, General_ID, Ztring::ToZtring(Descriptor->second.LinkedTrackID)+ID, true);
                }
        }

        if (Descriptor->second.Width!=(int32u)-1 && Retrieve(Stream_Video, StreamPos_Last, Video_Width).empty())
            Fill(Stream_Video, StreamPos_Last, Video_Width, Descriptor->second.Width, 10, true);
        if (Descriptor->second.Width_Display!=(int32u)-1 && Descriptor->second.Width_Display!=Retrieve(Stream_Video, StreamPos_Last, Video_Width).To_int32u()
         && !(Retrieve(Stream_Video, StreamPos_Last, Video_Format) == __T("DV") && Descriptor->second.Width_Display == 1920 && (Retrieve(Stream_Video, StreamPos_Last, Video_Width) == __T("1280") || Retrieve(Stream_Video, StreamPos_Last, Video_Width) == __T("1440")))) // Exception: DVCPRO HD is really 1440 but lot of containers fill the width value with the marketing width 1920, we ignore it
        {
            Fill(Stream_Video, StreamPos_Last, Video_Width_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Width), true);
            if (Retrieve(Stream_Video, StreamPos_Last, Video_PixelAspectRatio_Original).empty())
                Fill(Stream_Video, StreamPos_Last, Video_PixelAspectRatio_Original, Retrieve(Stream_Video, StreamPos_Last, Video_PixelAspectRatio), true);
            Clear(Stream_Video, StreamPos_Last, Video_PixelAspectRatio);
            Fill(Stream_Video, StreamPos_Last, Video_Width, Descriptor->second.Width_Display, 10, true);
            if (Descriptor->second.Width_Display_Offset!=(int32u)-1)
                Fill(Stream_Video, StreamPos_Last, Video_Width_Offset, Descriptor->second.Width_Display_Offset, 10, true);
        }
        if (Descriptor->second.Height!=(int32u)-1 && Retrieve(Stream_Video, StreamPos_Last, Video_Height).empty())
            Fill(Stream_Video, StreamPos_Last, Video_Height, Descriptor->second.Height, 10, true);
        if (Descriptor->second.Height_Display!=(int32u)-1 && Descriptor->second.Height_Display!=Retrieve(Stream_Video, StreamPos_Last, Video_Height).To_int32u())
        {
            Fill(Stream_Video, StreamPos_Last, Video_Height_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Height), true);
            if (Retrieve(Stream_Video, StreamPos_Last, Video_PixelAspectRatio_Original).empty())
                Fill(Stream_Video, StreamPos_Last, Video_PixelAspectRatio_Original, Retrieve(Stream_Video, StreamPos_Last, Video_PixelAspectRatio), true);
            Clear(Stream_Video, StreamPos_Last, Video_PixelAspectRatio);
            Fill(Stream_Video, StreamPos_Last, Video_Height, Descriptor->second.Height_Display, 10, true);
            if (Descriptor->second.Height_Display_Offset!=(int32u)-1)
                Fill(Stream_Video, StreamPos_Last, Video_Height_Offset, Descriptor->second.Height_Display_Offset, 10, true);
        }

        //Info
        const Ztring &ID=Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
        size_t ID_Dash_Pos=ID.find(__T('-'));
        size_t StreamWithSameID=1;
        if (ID_Dash_Pos!=(size_t)-1)
        {
            Ztring RealID=ID.substr(0, ID_Dash_Pos+1);
            while (StreamPos_Last+StreamWithSameID<Count_Get(StreamKind_Last) && Retrieve(StreamKind_Last, StreamPos_Last+StreamWithSameID, General_ID).find(RealID)==0)
                StreamWithSameID++;
        }
        if (Descriptor->second.SampleRate && StreamKind_Last==Stream_Video)
        {
            float64 SampleRate=Descriptor->second.SampleRate;
            if (StereoscopicPictureSubDescriptor_IsPresent)
            {
                SampleRate/=2;
                Fill(Stream_Video, StreamPos_Last, Video_MultiView_Count, 2, 10, true);
            }
            for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
                if (Essence->second.StreamKind==Stream_Video && Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1)==StreamPos_Last)
                {
                    if (Essence->second.Field_Count_InThisBlock_1 && !Essence->second.Field_Count_InThisBlock_2)
                        SampleRate/=2;
                    break;
                }
            Ztring SampleRate_Container; SampleRate_Container.From_Number(SampleRate); //TODO: fill frame rate before the merge with raw stream information
            const Ztring &SampleRate_RawStream=Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate);
            if (!SampleRate_RawStream.empty() && SampleRate_Container!=SampleRate_RawStream)
                Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Original, SampleRate_RawStream);
            Fill(Stream_Video, StreamPos_Last, Video_FrameRate, SampleRate, 3, true);
        }
        if (StreamKind_Last==Stream_Video)
            ColorLevels_Compute(Descriptor, true, Retrieve(Stream_Video, StreamPos_Last, Video_BitDepth).To_int32u());

        //MasteringDisplay specific info
        std::map<std::string, Ztring>::iterator Info_MasteringDisplay_Primaries = Descriptor->second.Infos.find("MasteringDisplay_Primaries");
        std::map<std::string, Ztring>::iterator Info_MasteringDisplay_WhitePointChromaticity = Descriptor->second.Infos.find("MasteringDisplay_WhitePointChromaticity");
        std::map<std::string, Ztring>::iterator MasteringDisplay_Luminance_Max = Descriptor->second.Infos.find("MasteringDisplay_Luminance_Max");
        std::map<std::string, Ztring>::iterator MasteringDisplay_Luminance_Min = Descriptor->second.Infos.find("MasteringDisplay_Luminance_Min");
        mastering_metadata_2086 MasteringMeta;
        memset(&MasteringMeta, 0xFF, sizeof(MasteringMeta));
        if (Info_MasteringDisplay_Primaries!= Descriptor->second.Infos.end())
        {
            ZtringList Primaries (Info_MasteringDisplay_Primaries->second);
            if (Primaries.size() == 6)
            {
                MasteringMeta.Primaries[0] = Primaries[0].To_int16u();
                MasteringMeta.Primaries[1] = Primaries[1].To_int16u();
                MasteringMeta.Primaries[2] = Primaries[2].To_int16u();
                MasteringMeta.Primaries[3] = Primaries[3].To_int16u();
                MasteringMeta.Primaries[4] = Primaries[4].To_int16u();
                MasteringMeta.Primaries[5] = Primaries[5].To_int16u();
            }
        }
        if (Info_MasteringDisplay_WhitePointChromaticity!= Descriptor->second.Infos.end())
        {
            ZtringList WhitePoint (Info_MasteringDisplay_WhitePointChromaticity->second);
            if (WhitePoint.size() == 2)
            {
                MasteringMeta.Primaries[6] = WhitePoint[0].To_int16u();
                MasteringMeta.Primaries[7] = WhitePoint[1].To_int16u();
            }
        }
        if (MasteringDisplay_Luminance_Min!=Descriptor->second.Infos.end())
        {
            MasteringMeta.Luminance[0] = MasteringDisplay_Luminance_Min->second.To_int32u();
        }
        if (MasteringDisplay_Luminance_Max!=Descriptor->second.Infos.end())
        {
            MasteringMeta.Luminance[1] = MasteringDisplay_Luminance_Max->second.To_int32u();
        }
        Ztring MasteringDisplay_ColorPrimaries;
        Ztring MasteringDisplay_Luminance;
        Get_MasteringDisplayColorVolume(MasteringDisplay_ColorPrimaries, MasteringDisplay_Luminance, MasteringMeta);
        if (!MasteringDisplay_ColorPrimaries.empty())
        {
            Descriptor->second.Infos["HDR_Format"]="SMPTE ST 2086";
            Descriptor->second.Infos["HDR_Format_Compatibility"]="HDR10";
            Descriptor->second.Infos["MasteringDisplay_ColorPrimaries"]=MasteringDisplay_ColorPrimaries;
            Descriptor->second.Infos["MasteringDisplay_Luminance"]=MasteringDisplay_Luminance;
        }

        for (std::map<std::string, Ztring>::iterator Info=Descriptor->second.Infos.begin(); Info!=Descriptor->second.Infos.end(); ++Info)
            if (Info!=Info_MasteringDisplay_Primaries
             && Info!=Info_MasteringDisplay_WhitePointChromaticity
             && Info!=MasteringDisplay_Luminance_Max
             && Info!=MasteringDisplay_Luminance_Min)
            {
                //Special case
                if (Info->first=="BitRate" && Retrieve(StreamKind_Last, StreamPos_Last, General_ID).find(__T(" / "))!=string::npos)
                {
                    if (!Retrieve(StreamKind_Last, StreamPos_Last, Info->first.c_str()).empty())
                        continue; // Not always valid e.g. Dolby E or AC-3 in AES3. TODO: check in case of normal check
                    if (Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate)).empty() || Retrieve(StreamKind_Last, StreamPos_Last, General_ID).find(__T('-'))!=string::npos)
                        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Encoded), Info->second.To_int64u()*2, 10, true);
                    else
                        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate), Info->second.To_int64u()*2, 10, true);
                }
                else if (!Info->second.empty())
                {
                    const Ztring FromEssence=Retrieve_Const(StreamKind_Last, StreamPos_Last, Info->first.c_str());
                    for (size_t Pos=0; Pos<StreamWithSameID; Pos++)
                    {
                        if (FromEssence.empty())
                        {
                            Fill(StreamKind_Last, StreamPos_Last+Pos, Info->first.c_str(), Info->second);
                        }
                        else if (FromEssence!=Info->second)
                        {
                            // Special cases
                            if (Info->first=="ColorSpace" && ((Info->second==__T("RGB") && FromEssence==__T("XYZ")) || (Info->second==__T("RGBA") && FromEssence==__T("XYZA"))))
                                continue; // "RGB" is used by MXF for "XYZ" too
                            if (Info->first=="Format")
                                continue; // Too much important to show the essence format, ignoring for the moment. TODO: display something in such case
                            if (Info->first=="Duration")
                                continue; // Found 1 file with descriptor with wrong descriptor SampleRate. TODO: better display in such case (should not take precedence other other durations, how to display the issue between component duration and descriptor duration, both container metadata?). Must also take care about StereoscopicPictureSubDescriptor (SampleRate divided by 2 in that case)
                            if (Info->first=="DisplayAspectRatio")
                                continue; // Handled separately
                            if (Info->first=="Channel(s)")
                                continue; // Not always valid e.g. Dolby E. TODO: check in case of normal check
                            if (Info->first=="BitDepth")
                                continue; // Not always valid e.g. Dolby E. TODO: check in case of normal check
                            if (Info->first=="BitRate")
                                continue; // Not always valid e.g. Dolby E. TODO: check in case of normal check
                            if (Info->first=="StreamOrder")
                            {
                                Fill(StreamKind_Last, StreamPos_Last+Pos, Info->first.c_str(), Info->second);
                                continue;
                            }

                            // Filling both values
                            Fill(StreamKind_Last, StreamPos_Last+Pos, (Info->first+"_Original").c_str(), FromEssence); //TODO: use the generic engine by filling descriptor info before merging essence info
                            Fill(StreamKind_Last, StreamPos_Last+Pos, Info->first.c_str(), Info->second, true); //TODO: use the generic engine by filling descriptor info before merging essence info
                            if (ShowSource_IsInList(Info->first))
                            {
                                Fill(StreamKind_Last, StreamPos_Last+Pos, (Info->first+"_Source").c_str(), "Container", Unlimited, true, true);
                                Fill(StreamKind_Last, StreamPos_Last+Pos, (Info->first+"_Original_Source").c_str(), "Stream");
                            }
                        }
                        else
                        {
                            if (ShowSource_IsInList(Info->first))
                                Fill(StreamKind_Last, StreamPos_Last+Pos, (Info->first+"_Source").c_str(), "Container / Stream", Unlimited, true, true);
                        }
                    }
                }
            }
        Ztring Format, CodecID;
        if (Descriptor->second.EssenceContainer.hi!=(int64u)-1)
        {
            CodecID.From_Number(Descriptor->second.EssenceContainer.lo, 16);
            if (CodecID.size()<16)
                CodecID.insert(0, 16-CodecID.size(), __T('0'));
            Format.From_UTF8(Mxf_EssenceContainer(Descriptor->second.EssenceContainer));
        }
        if (Descriptor->second.EssenceCompression.hi!=(int64u)-1)
        {
            if (!CodecID.empty())
                CodecID+=__T('-');
            Ztring EssenceCompression;
            EssenceCompression.From_Number(Descriptor->second.EssenceCompression.lo, 16);
            if (EssenceCompression.size()<16)
                EssenceCompression.insert(0, 16-EssenceCompression.size(), __T('0'));
            CodecID+=EssenceCompression;
            Ztring Format_FromCompression; Format_FromCompression.From_UTF8(Mxf_EssenceCompression(Descriptor->second.EssenceCompression));
            if (!Format_FromCompression.empty())
                Format=Format_FromCompression; //EssenceCompression has priority
        }
        if (!CodecID.empty())
            for (size_t Pos=0; Pos<StreamWithSameID; Pos++)
                Fill(StreamKind_Last, StreamPos_Last+Pos, Fill_Parameter(StreamKind_Last, Generic_CodecID), CodecID, true);
        if (!Format.empty())
            for (size_t Pos=0; Pos<StreamWithSameID; Pos++)
                if (Retrieve(StreamKind_Last, StreamPos_Last+Pos, Fill_Parameter(StreamKind_Last, Generic_Format)).empty())
                    Fill(StreamKind_Last, StreamPos_Last+Pos, Fill_Parameter(StreamKind_Last, Generic_Format), Format);

        //Bitrate (PCM)
        if (StreamKind_Last==Stream_Audio && Retrieve(Stream_Audio, StreamPos_Last, Audio_BitRate).empty() && Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("PCM") && Retrieve(Stream_Audio, StreamPos_Last, Audio_Format_Settings_Wrapping).find(__T("D-10"))!=string::npos)
        {
            int64u SamplingRate=Retrieve(Stream_Audio, StreamPos_Last, Audio_SamplingRate).To_int64u();
            if (SamplingRate)
               Fill(Stream_Audio, StreamPos_Last, Audio_BitRate, 8*SamplingRate*32);
        }
        if (StreamKind_Last==Stream_Audio && Retrieve(Stream_Audio, StreamPos_Last, Audio_BitRate).empty() && Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("PCM"))
        {
            int64u Channels=Retrieve(Stream_Audio, StreamPos_Last, Audio_Channel_s_).To_int64u();
            int64u SamplingRate=Retrieve(Stream_Audio, StreamPos_Last, Audio_SamplingRate).To_int64u();
            int64u Resolution=Retrieve(Stream_Audio, StreamPos_Last, Audio_BitDepth).To_int64u();
            if (Channels && SamplingRate && Resolution)
               Fill(Stream_Audio, StreamPos_Last, Audio_BitRate, Channels*SamplingRate*Resolution);
        }

        //Bitrate (Video)
        if (StreamKind_Last==Stream_Video && Retrieve(Stream_Video, StreamPos_Last, Video_BitRate).empty())
        {
            //Until now, I only found CBR files
            Fill(Stream_Video, StreamPos_Last, Video_BitRate, Retrieve(Stream_Video, StreamPos_Last, Video_BitRate_Nominal));
        }

        //Display Aspect Ratio
        if (StreamKind_Last==Stream_Video && !Descriptor->second.Infos["DisplayAspectRatio"].empty() && Retrieve(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio)!=Descriptor->second.Infos["DisplayAspectRatio"])
        {
            Ztring DAR=Retrieve(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio);
            Clear(Stream_Video, StreamPos_Last, Video_PixelAspectRatio);
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, Descriptor->second.Infos["DisplayAspectRatio"], true);
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio_Original, DAR);
            float32 Width =Retrieve(Stream_Video, StreamPos_Last, Video_Width             ).To_float32();
            float32 Height=Retrieve(Stream_Video, StreamPos_Last, Video_Height            ).To_float32();
            float32 DAR_F =DAR.To_float32();
            if (Width && Height && DAR_F)
            {
                float32 PAR   =1/(Width/Height/DAR_F);
                if (PAR>(float32)12/(float32)11*0.99 && PAR<(float32)12/(float32)11*1.01)
                    PAR=(float32)12/(float32)11;
                if (PAR>(float32)10/(float32)11*0.99 && PAR<(float32)10/(float32)11*1.01)
                    PAR=(float32)10/(float32)11;
                if (PAR>(float32)16/(float32)11*0.99 && PAR<(float32)16/(float32)11*1.01)
                    PAR=(float32)16/(float32)11;
                if (PAR>(float32)40/(float32)33*0.99 && PAR<(float32)40/(float32)33*1.01)
                    PAR=(float32)40/(float32)33;
                if (PAR>(float32)24/(float32)11*0.99 && PAR<(float32)24/(float32)11*1.01)
                    PAR=(float32)24/(float32)11;
                if (PAR>(float32)20/(float32)11*0.99 && PAR<(float32)20/(float32)11*1.01)
                    PAR=(float32)20/(float32)11;
                if (PAR>(float32)32/(float32)11*0.99 && PAR<(float32)32/(float32)11*1.01)
                    PAR=(float32)32/(float32)11;
                if (PAR>(float32)80/(float32)33*0.99 && PAR<(float32)80/(float32)33*1.01)
                    PAR=(float32)80/(float32)33;
                if (PAR>(float32)18/(float32)11*0.99 && PAR<(float32)18/(float32)11*1.01)
                    PAR=(float32)18/(float32)11;
                if (PAR>(float32)15/(float32)11*0.99 && PAR<(float32)15/(float32)11*1.01)
                    PAR=(float32)15/(float32)11;
                if (PAR>(float32)64/(float32)33*0.99 && PAR<(float32)64/(float32)33*1.01)
                    PAR=(float32)64/(float32)33;
                if (PAR>(float32)160/(float32)99*0.99 && PAR<(float32)160/(float32)99*1.01)
                    PAR=(float32)160/(float32)99;
                if (PAR>(float32)4/(float32)3*0.99 && PAR<(float32)4/(float32)3*1.01)
                    PAR=(float32)4/(float32)3;
                if (PAR>(float32)3/(float32)2*0.99 && PAR<(float32)3/(float32)2*1.01)
                    PAR=(float32)3/(float32)2;
                if (PAR>(float32)2/(float32)1*0.99 && PAR<(float32)2/(float32)1*1.01)
                    PAR=(float32)2;
                if (PAR>(float32)59/(float32)54*0.99 && PAR<(float32)59/(float32)54*1.01)
                    PAR=(float32)59/(float32)54;
            }
        }

        //ActiveFormatDescriptor
        if (StreamKind_Last==Stream_Video && Descriptor->second.ActiveFormat!=(int8u)-1 && Retrieve(Stream_Video, StreamPos_Last, Video_ActiveFormatDescription).empty())
        {
            Fill(Stream_Video, 0, Video_ActiveFormatDescription, Descriptor->second.ActiveFormat);
            if (Descriptor->second.ActiveFormat<16)
            {
                float32 DAR=Retrieve(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio).To_float32();
                if (DAR>(float32)4/(float32)3*0.99 && DAR<(float32)4/(float32)3*1.01)
                    Fill(Stream_Video, 0, Video_ActiveFormatDescription_String, AfdBarData_active_format_4_3[Descriptor->second.ActiveFormat]);
                if (DAR>(float32)16/(float32)9*0.99 && DAR<(float32)16/(float32)9*1.01)
                    Fill(Stream_Video, 0, Video_ActiveFormatDescription_String, AfdBarData_active_format_16_9[Descriptor->second.ActiveFormat]);
            }
            if (Retrieve(Stream_Video, 0, Video_ActiveFormatDescription_String).empty())
                Fill(Stream_Video, 0, Video_ActiveFormatDescription_String, Descriptor->second.ActiveFormat);
        }

        //ScanType / ScanOrder
        if (StreamKind_Last==Stream_Video && Retrieve(Stream_Video, StreamPos_Last, Video_ScanType_Original).empty())
        {
            //ScanType
            auto ScanType=Descriptor->second.Jp2kContentKind==4?__T("Interlaced"):Descriptor->second.ScanType;
            if (Retrieve(Stream_Video, StreamPos_Last, Video_ScanType).empty())
                Fill(Stream_Video, StreamPos_Last, Video_ScanType, ScanType);
            else if (Descriptor->second.Jp2kContentKind<=1)
            {
                // Do not trust frame layout info for JP2k 0 a 1, the jp2k parser needs to decide because all is possible (frame layout interlaced of frame, 1 or 2 codestreams per frame)
            }
            else if (!ScanType.empty() && (ScanType!=Retrieve(Stream_Video, StreamPos_Last, Video_ScanType) && !(Descriptor->second.Is_Interlaced() && Retrieve(Stream_Video, StreamPos_Last, Video_ScanType)==__T("MBAFF"))))
            {
                Fill(Stream_Video, StreamPos_Last, Video_ScanType_Original, Retrieve(Stream_Video, StreamPos_Last, Video_ScanType));
                Fill(Stream_Video, StreamPos_Last, Video_ScanType, ScanType, true);
            }

            //ScanOrder
            Ztring ScanOrder_Temp;
            if ((Descriptor->second.FieldDominance==1 && Descriptor->second.FieldTopness==1) || (Descriptor->second.FieldDominance!=1 && Descriptor->second.FieldTopness==2))
                ScanOrder_Temp.From_UTF8("TFF");
            if ((Descriptor->second.FieldDominance==1 && Descriptor->second.FieldTopness==2) || (Descriptor->second.FieldDominance!=1 && Descriptor->second.FieldTopness==1))
                    ScanOrder_Temp.From_UTF8("BFF");
            if ((!ScanOrder_Temp.empty() && ScanOrder_Temp!=Retrieve(Stream_Video, StreamPos_Last, Video_ScanOrder)) || !Retrieve(Stream_Video, StreamPos_Last, Video_ScanType_Original).empty())
            {
                Fill(Stream_Video, StreamPos_Last, Video_ScanOrder_Original, Retrieve(Stream_Video, StreamPos_Last, Video_ScanOrder), true);
                if (ScanOrder_Temp.empty())
                {
                    Clear(Stream_Video, StreamPos_Last, Video_ScanOrder);
                    Clear(Stream_Video, StreamPos_Last, Video_ScanOrder_String);
                }
                else
                    Fill(Stream_Video, StreamPos_Last, Video_ScanOrder, ScanOrder_Temp, true);
            }
        }

        //BlockAlignment
        if (StreamKind_Last==Stream_Audio && Descriptor->second.BlockAlign!=(int16u)-1)
        {
            if (Retrieve(Stream_Audio, StreamPos_Last, "BlockAlignment").empty()) //TODO: check the reason it is sometimes call several times.
            {
                Fill(Stream_Audio, StreamPos_Last, "BlockAlignment", Descriptor->second.BlockAlign);
                Fill_SetOptions(Stream_Audio, StreamPos_Last, "BlockAlignment", "N NT");
            }
        }

        //Subs and ChannelAssignment
        Ztring ChannelAssignment;
        if (Descriptor->second.ChannelAssignment.lo!=(int64u)-1)
        {
            ChannelAssignment.From_Number(Descriptor->second.ChannelAssignment.lo, 16);
            if (ChannelAssignment.size()<16)
                ChannelAssignment.insert(0, 16-ChannelAssignment.size(), __T('0'));
        }
        if (!Descriptor->second.SubDescriptors.empty())
        {
            std::vector<int128u> AudioChannelLabels_MCALabelDictionaryID;

            for (size_t Pos=0; Pos<Descriptor->second.SubDescriptors.size(); Pos++)
            {
                descriptors::iterator SubDescriptor=Descriptors.find(Descriptor->second.SubDescriptors[Pos]);
                if (SubDescriptor!=Descriptors.end())
                {
                    if (SubDescriptor->second.LinkedTrackID!=(int32u)-1)
                    {
                        //This is not a real subdescriptor, rather a descriptor for another track. Seen in a PHDR track
                        if (SubDescriptor->second.LinkedTrackID!=Descriptor->second.LinkedTrackID) // TODO: better method for avoiding loop
                            Streams_Finish_Descriptor(SubDescriptor->first, int128u());
                        continue;
                    }

                    switch (SubDescriptor->second.Type)
                    {
                        case descriptor::Type_AudioChannelLabelSubDescriptor:
                                                AudioChannelLabels_MCALabelDictionaryID.push_back(SubDescriptor->second.MCALabelDictionaryID);
                                                break;
                        case descriptor::Type_SoundfieldGroupLabelSubDescriptor:
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Partition kind", SubDescriptor->second.MCAPartitionKind);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Partition Number", SubDescriptor->second.MCAPartitionNumber);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Title", SubDescriptor->second.MCATitle);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Title Version", SubDescriptor->second.MCATitleVersion);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Title Sub-Version", SubDescriptor->second.MCATitleSubVersion);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Episode", SubDescriptor->second.MCAEpisode);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Audio Content Kind", SubDescriptor->second.MCAAudioContentKind);
                                                Fill(Stream_Audio, StreamPos_Last, "MCA Audio Element Kind", SubDescriptor->second.MCAAudioElementKind);
                                                //if (SubDescriptor->second.MCALabelDictionaryID.lo!=(int64u)-1)
                                                //{
                                                //    Ztring ChannelAssignment2;
                                                //    ChannelAssignment2.From_Number(SubDescriptor->second.MCALabelDictionaryID.lo, 16);
                                                //    if (ChannelAssignment2.size()<16)
                                                //        ChannelAssignment2.insert(0, 16-ChannelAssignment2.size(), __T('0'));
                                                //    if (!ChannelAssignment.empty())
                                                //        ChannelAssignment+=__T('-');
                                                //    ChannelAssignment+=ChannelAssignment2;
                                                //}
                                                for (std::map<std::string, Ztring>::iterator Info=SubDescriptor->second.Infos.begin(); Info!=SubDescriptor->second.Infos.end(); ++Info)
                                                    Fill(Stream_Audio, StreamPos_Last, Info->first.c_str(), Info->second, true);
                                                break;
                        default:                ;
                                                #if MEDIAINFO_ADVANCED
                                                    if (SubDescriptor->second.Jpeg2000_Rsiz!=(int16u)-1 && !Retrieve(StreamKind_Last, StreamPos_Last, "Format_Profile").empty() && Jpeg2000_Rsiz(SubDescriptor->second.Jpeg2000_Rsiz)!=Retrieve(StreamKind_Last, StreamPos_Last, "Format_Profile").To_UTF8())
                                                    {
                                                        Fill(StreamKind_Last, StreamPos_Last, "Format_Profile_FromStream", Retrieve(StreamKind_Last, StreamPos_Last, "Format_Profile"));
                                                        Fill(StreamKind_Last, StreamPos_Last, "Format_Profile_FromContainer", Jpeg2000_Rsiz(SubDescriptor->second.Jpeg2000_Rsiz));
                                                    }
                                                #endif //MEDIAINFO_ADVANCED
                    }

                    //Special cases
                    std::map<std::string, Ztring>::iterator Info_AvcProfile=SubDescriptor->second.Infos.find("Temp_AVC_Profile");
                    std::map<std::string, Ztring>::iterator Info_AvcLevel=SubDescriptor->second.Infos.find("Temp_AVC_Level");
                    std::map<std::string, Ztring>::iterator Info_AvcConstraintSet=SubDescriptor->second.Infos.find("Temp_AVC_constraint_set");
                    if (Info_AvcProfile!=SubDescriptor->second.Infos.end() || Info_AvcLevel!=SubDescriptor->second.Infos.end() || Info_AvcConstraintSet!=SubDescriptor->second.Infos.end())
                    {
                        //AVC Descriptor creates that, adapting
                        int8u profile_idc;
                        if (Info_AvcProfile!=SubDescriptor->second.Infos.end())
                        {
                            profile_idc=Info_AvcProfile->second.To_int8u();
                            SubDescriptor->second.Infos.erase(Info_AvcProfile);
                        }
                        else
                            profile_idc=0;
                        int8u level_idc;
                        if (Info_AvcLevel!=SubDescriptor->second.Infos.end())
                        {
                            level_idc=Info_AvcLevel->second.To_int8u();
                            SubDescriptor->second.Infos.erase(Info_AvcLevel);
                        }
                        else
                            level_idc=0;
                        int8u constraint_set_flags;
                        if (Info_AvcConstraintSet!=SubDescriptor->second.Infos.end())
                        {
                            constraint_set_flags=Info_AvcConstraintSet->second.To_int8u();
                            SubDescriptor->second.Infos.erase(Info_AvcConstraintSet);
                        }
                        else
                            constraint_set_flags=0;
                        SubDescriptor->second.Infos["Format_Profile"].From_UTF8(Avc_profile_level_string(profile_idc, level_idc, constraint_set_flags));
                    }
                    
                    for (std::map<std::string, Ztring>::iterator Info=SubDescriptor->second.Infos.begin(); Info!=SubDescriptor->second.Infos.end(); ++Info)
                        if (Info->first=="ComponentCount")
                        {
                            if (Info->second==__T("4") && !Retrieve(StreamKind_Last, StreamPos_Last, "ColorSpace").empty())
                                Fill(StreamKind_Last, StreamPos_Last, "ColorSpace", Retrieve(StreamKind_Last, StreamPos_Last, "ColorSpace")+__T('A'), true); // Descriptor name is "RGBA"...
                        }
                        else if (Retrieve(StreamKind_Last, StreamPos_Last, Info->first.c_str()).empty())
                            Fill(StreamKind_Last, StreamPos_Last, Info->first.c_str(), Info->second);
                        else if (Retrieve(StreamKind_Last, StreamPos_Last, Info->first.c_str()) != Info->second)
                        {
                            Fill(StreamKind_Last, StreamPos_Last, (Info->first+"_Original").c_str(), Retrieve(StreamKind_Last, StreamPos_Last, Info->first.c_str()));
                            Fill(StreamKind_Last, StreamPos_Last, Info->first.c_str(), Info->second, true);
                        }
                }
            }

            if (!AudioChannelLabels_MCALabelDictionaryID.empty())
            {
                Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions, MXF_MCALabelDictionaryID_ChannelPositions(AudioChannelLabels_MCALabelDictionaryID));
                Fill(Stream_Audio, StreamPos_Last, Audio_ChannelLayout, MXF_MCALabelDictionaryID_ChannelLayout(AudioChannelLabels_MCALabelDictionaryID));
            }
        }
        if (!ChannelAssignment.empty())
            Fill(Stream_Audio, StreamPos_Last, Audio_ChannelLayoutID, ChannelAssignment);

        auto CodecID_Addition = Descriptor->second.Infos.find("CodecID");
        if (CodecID_Addition != Descriptor->second.Infos.end()) {
            Ztring CodecID = Retrieve_Const(StreamKind_Last, StreamPos_Last, CodecID_Addition->first.c_str());
            if (CodecID != CodecID_Addition->second) {
                Fill(StreamKind_Last, StreamPos_Last, CodecID_Addition->first.c_str(), CodecID+__T('-')+CodecID_Addition->second, true);
            }
        }
    }

    //Fallback on partition data if classic methods failed
    if (StreamKind_Last!=Stream_Max && StreamPos_Last!=(size_t)-1 && Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format)).empty() && Descriptors.size()==1 && Count_Get(StreamKind_Last)==1)
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), Mxf_EssenceContainer(EssenceContainer_FromPartitionMetadata));
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Locator(const int128u DescriptorUID, const int128u LocatorUID)
{
    descriptors::iterator Descriptor=Descriptors.find(DescriptorUID);
    if (Descriptor==Descriptors.end())
        return;

    locators::iterator Locator=Locators.find(LocatorUID);
    if (Locator==Locators.end())
        return;

    //External file name specific
    if (!Locator->second.IsTextLocator && !Locator->second.EssenceLocator.empty())
    {
        //Preparing
        Locator->second.StreamKind=StreamKind_Last;
        Locator->second.StreamPos=StreamPos_Last;
        Locator->second.LinkedTrackID=Descriptor->second.LinkedTrackID;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_CommercialNames ()
{
    //Commercial names
    if (Count_Get(Stream_Video)==1)
    {
        Streams_Finish_StreamOnly();
             if (!Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny).empty())
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny));
            Fill(Stream_General, 0, General_Format_Commercial, __T("MXF ")+Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny));
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("DV"))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "DV");
            Fill(Stream_General, 0, General_Format_Commercial, "MXF DV");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("AVC") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && Retrieve(Stream_Video, 0, Video_Format_Settings_SliceCount)==__T("10") && Retrieve(Stream_Video, 0, Video_BitRate)==__T("56064000"))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "AVC-Intra 50");
            Fill(Stream_General, 0, General_Format_Commercial, "MXF AVC-Intra 50");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "AVC-Intra 50");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("AVC") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && Retrieve(Stream_Video, 0, Video_Format_Settings_SliceCount)==__T("10") && Retrieve(Stream_Video, 0, Video_BitRate)==__T("113664000"))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "AVC-Intra 100");
            Fill(Stream_General, 0, General_Format_Commercial, "MXF AVC-Intra 100");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "AVC-Intra 100");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("30000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("30000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("30000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "IMX 30");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "IMX 30");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("40000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("40000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("40000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "IMX 40");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "IMX 40");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)==__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("50000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "IMX 50");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "IMX 50");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && !Retrieve(Stream_Video, 0, Video_Format_Settings_GOP).empty() && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("18000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("18000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("18000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM HD 18");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM HD 18");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && !Retrieve(Stream_Video, 0, Video_Format_Settings_GOP).empty() && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("25000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("25000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("25000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM HD 25");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM HD 25");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && !Retrieve(Stream_Video, 0, Video_Format_Settings_GOP).empty() && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("35000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("35000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("35000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM HD 35");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM HD 35");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && !Retrieve(Stream_Video, 0, Video_Format_Settings_GOP).empty() && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("50000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM HD422");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM HD422");
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Component(const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin)
{
    components::iterator Component=Components.find(ComponentUID);
    if (Component==Components.end())
        return;

    //Duration
    if (EditRate && StreamKind_Last!=Stream_Max && Component->second.Duration!=(int64u)-1)
    {
        int64u FrameCount=Component->second.Duration;
        if (StreamKind_Last==Stream_Video || Config->File_EditRate)
        {
            int64u File_IgnoreEditsBefore=Config->File_IgnoreEditsBefore;
            if (File_IgnoreEditsBefore && Config->File_EditRate && (EditRate<Config->File_EditRate*0.9 || EditRate>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                File_IgnoreEditsBefore=float64_int64s(((float64)File_IgnoreEditsBefore)/Config->File_EditRate*EditRate);
            int64u File_IgnoreEditsAfter=Config->File_IgnoreEditsAfter;
            if (File_IgnoreEditsAfter!=(int64u)-1 && Config->File_EditRate && (EditRate<Config->File_EditRate*0.9 || EditRate>Config->File_EditRate*1.1)) //In case of problem or EditRate being sampling rate
                File_IgnoreEditsAfter=float64_int64s(((float64)File_IgnoreEditsAfter)/Config->File_EditRate*EditRate);
            if (File_IgnoreEditsAfter<FrameCount)
                FrameCount=File_IgnoreEditsAfter;
            if (FrameCount<File_IgnoreEditsBefore)
                FrameCount=File_IgnoreEditsBefore;
            FrameCount-=File_IgnoreEditsBefore;
        }
        if (StreamPos_Last==(size_t)-1 && Count_Get(StreamKind_Last)==1)
            StreamPos_Last=0; // TODO: remove this hack
        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), FrameCount*1000/EditRate, 0, true);
        size_t ID_SubStreamInfo_Pos=Retrieve(StreamKind_Last, StreamPos_Last, General_ID).find(__T('-'));
        if (ID_SubStreamInfo_Pos!=string::npos)
        {
            Ztring ID=Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
            ID.resize(ID_SubStreamInfo_Pos+1);
            size_t StreamPos_Last_Temp=StreamPos_Last;
            while (StreamPos_Last_Temp)
            {
                StreamPos_Last_Temp--;
                if (Retrieve(StreamKind_Last, StreamPos_Last_Temp, General_ID).find(ID)!=0)
                    break;
                Fill(StreamKind_Last, StreamPos_Last_Temp, Fill_Parameter(StreamKind_Last, Generic_Duration), FrameCount*1000/EditRate, 0, true);
            }
        }

        // Hack, TODO: find a correct method for detecting fiel/frame differene
        if (StreamKind_Last==Stream_Video)
            for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
                if (Essence->second.StreamKind==Stream_Video && Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1)==StreamPos_Last)
                {
                    if (Essence->second.Field_Count_InThisBlock_1 && !Essence->second.Field_Count_InThisBlock_2)
                    {
                        FrameCount/=2;
                        EditRate/=2;
                    }
                    break;
                }

        FillAllMergedStreams=true;

        if (Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount)).empty())
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount), FrameCount);

        if (Retrieve(StreamKind_Last, StreamPos_Last, "FrameRate").empty())
            Fill(StreamKind_Last, StreamPos_Last, "FrameRate", EditRate);

        FillAllMergedStreams=false;

        const Ztring& FrameRate_FromStream=Retrieve(StreamKind_Last, StreamPos_Last, "FrameRate");
        if (FrameRate_FromStream.empty())
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameRate), EditRate);
        else if (StreamKind_Last!=Stream_Audio || (Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("Dolby E") && EditRate<1000)) //Arbitrary number for detecting frame rate vs sample rate (both are possible with Clip-wrapped MXF PCM e.g. 29.97 or 48000)
        {
            Ztring FrameRate_FromContainer; FrameRate_FromContainer.From_Number(EditRate);
            if (FrameRate_FromStream!=FrameRate_FromContainer)
            {
                size_t ID_SubStreamInfo_Pos=Retrieve(StreamKind_Last, StreamPos_Last, General_ID).find(__T('-')); //Filling all tracks with same ID (e.g. Dolby E). TODO: merge code
                size_t StreamPos_Last_Temp=StreamPos_Last;
                Ztring ID;
                if (ID_SubStreamInfo_Pos!=string::npos)
                {
                    ID=Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
                    ID.resize(ID_SubStreamInfo_Pos+1);
                }
                for (;;)
                {
                    //Merge was already done, we need to do here Container/Stream coherency test. TODO: merge of code
                    Fill(StreamKind_Last, StreamPos_Last_Temp, "FrameRate_Original", FrameRate_FromStream);
                    Fill(StreamKind_Last, StreamPos_Last_Temp, Fill_Parameter(StreamKind_Last, Generic_FrameRate), FrameRate_FromContainer, true);
                    if (ID.empty() || !StreamPos_Last_Temp)
                        break;
                    StreamPos_Last_Temp--;
                    if (Retrieve(StreamKind_Last, StreamPos_Last_Temp, General_ID).find(ID)!=0)
                        break;
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Component_ForTimeCode(const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin, bool IsSourcePackage, const Ztring& TrackName)
{
    components::iterator Component=Components.find(ComponentUID);
    if (Component==Components.end())
        return;

    //For the sequence, searching Structural componenents
    for (size_t Pos=0; Pos<Component->second.StructuralComponents.size(); Pos++)
    {
        components::iterator Component2=Components.find(Component->second.StructuralComponents[Pos]);
        if (Component2!=Components.end() && Component2->second.MxfTimeCode.StartTimecode!=(int64u)-1 && !Config->File_IsReferenced_Get())
        {
            //Note: Origin is not part of the StartTimecode for the first frame in the source package. From specs: "For a Timecode Track with a single Timecode Component and with origin N, where N greater than 0, the timecode value at the Zero Point of the Track equals the start timecode of the Timecode Component incremented by N units."
            TimeCode TC=Component2->second.MxfTimeCode.RoundedTimecodeBase<0x8000?TimeCode((int64_t)(Component2->second.MxfTimeCode.StartTimecode+Config->File_IgnoreEditsBefore), Component2->second.MxfTimeCode.RoundedTimecodeBase-1, TimeCode::DropFrame(Component2->second.MxfTimeCode.DropFrame).FPS1001(Component2->second.MxfTimeCode.DropFrame)):TimeCode();
            bool IsHybridTimeCode=false;
            if (Component->second.StructuralComponents.size()==2 && !Pos)
            {
                components::iterator Component_TC2=Components.find(Component->second.StructuralComponents[1]);
                if (Component_TC2!=Components.end() && Component_TC2->second.MxfTimeCode.StartTimecode!=(int64u)-1)
                {
                    TimeCode TC2=Component_TC2->second.MxfTimeCode.RoundedTimecodeBase<0x8000?TimeCode((int64_t)(Component_TC2->second.MxfTimeCode.StartTimecode+Config->File_IgnoreEditsBefore), Component_TC2->second.MxfTimeCode.RoundedTimecodeBase-1, TimeCode::DropFrame(Component2->second.MxfTimeCode.DropFrame).FPS1001(Component2->second.MxfTimeCode.DropFrame)):TimeCode();
                    if (TC2.ToFrames()-TC.ToFrames()==2)
                    {
                        ++TC;
                        IsHybridTimeCode=true;
                    }
                }
            }
            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_ID, Ztring::ToZtring(TrackID)+(IsSourcePackage?__T("-Source"):__T("-Material")));
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "MXF TC");

            #if MEDIAINFO_ADVANCED
                if (Config->TimeCode_Dumps)
                {
                    auto id=Ztring(Ztring::ToZtring(TrackID)+(IsSourcePackage?__T("-Source"):__T("-Material"))).To_UTF8();
                    auto& TimeCode_Dump=(*Config->TimeCode_Dumps)[id];
                    if (TimeCode_Dump.Attributes_First.empty())
                    {
                        TimeCode_Dump.Attributes_First+=" id=\""+id+"\" format=\"smpte-st377\" frame_rate=\""+to_string(Component2->second.MxfTimeCode.RoundedTimecodeBase);
                        if (Component2->second.MxfTimeCode.DropFrame)
                            TimeCode_Dump.Attributes_First+=+"000/1001";
                        TimeCode_Dump.Attributes_First+='\"';
                        TimeCode_Dump.FrameCount=Component2->second.Duration;
                        TimeCode_Dump.Attributes_Last+=" start_tc=\""+TimeCode((int64_t)Component2->second.MxfTimeCode.StartTimecode, Component2->second.MxfTimeCode.RoundedTimecodeBase-1, TimeCode::DropFrame(Component2->second.MxfTimeCode.DropFrame).FPS1001(Component2->second.MxfTimeCode.DropFrame)).ToString() + '\"';
                    }
                }
            #endif //MEDIAINFO_ADVANCED

            if (Component2->second.MxfTimeCode.RoundedTimecodeBase<=(int8u)-1) // Found files with RoundedTimecodeBase of 0x8000
                Fill(Stream_Other, StreamPos_Last, Other_FrameRate, Component2->second.MxfTimeCode.RoundedTimecodeBase/(Is1001?1.001:1.000));
            TC.Set1001fps(Is1001);
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_FirstFrame, TC.ToString().c_str());
            if (Component2->second.Duration && Component2->second.Duration!=(int64u)-1)
            {
                Fill(Stream_Other, StreamPos_Last, Other_FrameCount, Component2->second.Duration);
                if (TC.GetFramesMax())
                {
                    TC+=Component2->second.Duration-1;
                    Fill(Stream_Other, StreamPos_Last, Other_TimeCode_LastFrame, TC.ToString().c_str());
                }
            }
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_Settings, IsSourcePackage?__T("Source Package"):__T("Material Package"));
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_Stripped, "Yes");
            Fill(Stream_Other, StreamPos_Last, Other_Title, TrackName);

            if ((!TimeCodeFromMaterialPackage && IsSourcePackage) || (TimeCodeFromMaterialPackage && !IsSourcePackage))
            {
                MxfTimeCodeForDelay=Component2->second.MxfTimeCode;

                DTS_Delay=((float64)MxfTimeCodeForDelay.StartTimecode)/MxfTimeCodeForDelay.RoundedTimecodeBase;
                if (MxfTimeCodeForDelay.DropFrame)
                {
                    DTS_Delay*=1001;
                    DTS_Delay/=1000;
                }
                FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
                #if MEDIAINFO_DEMUX
                    Config->Demux_Offset_DTS_FromStream=FrameInfo.DTS;
                #endif //MEDIAINFO_DEMUX
            }

            if (!IsSourcePackage)
            {
                MxfTimeCodeMaterial=Component2->second.MxfTimeCode;
            }

            if (IsHybridTimeCode)
                break;
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Component_ForAS11(const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin)
{
    components::iterator Component=Components.find(ComponentUID);
    if (Component==Components.end())
        return;

    //Computing frame rate
    int64u TC_Temp=0;
    int8u FrameRate_TempI;
    bool DropFrame_Temp;
    if (MxfTimeCodeMaterial.IsInit())
    {
        TC_Temp=MxfTimeCodeMaterial.StartTimecode;
        FrameRate_TempI=(int8u)MxfTimeCodeMaterial.RoundedTimecodeBase;
        DropFrame_Temp=MxfTimeCodeMaterial.DropFrame;
    }
    else
    {
        TC_Temp=0;
        Ztring FrameRateS=Retrieve(Stream_Video, 0, Video_FrameRate);
        int32u FrameRate_TempI32=float32_int32s(FrameRateS.To_float32());
        if (FrameRate_TempI32 && FrameRate_TempI32<256)
        {
            FrameRate_TempI=(int8u)FrameRate_TempI32;
            float32 FrameRateF=FrameRateS.To_float32();
            float FrameRateF_Min=((float32)FrameRate_TempI)/((float32)1.002);
            float FrameRateF_Max=(float32)FrameRate_TempI;
            if (FrameRateF>=FrameRateF_Min && FrameRateF<FrameRateF_Max)
                DropFrame_Temp=true;
            else
                DropFrame_Temp=false;
        }
        else
        {
            FrameRate_TempI=25;
            DropFrame_Temp=false;
        }
    }

    //For the sequence, searching Structural componenents
    int64u Duration_CurrentPos=0;
    int64u Duration_Programme=0;
    for (size_t Pos=0; Pos<Component->second.StructuralComponents.size(); Pos++)
    {
        // AS-11
        dmsegments::iterator DescriptiveMarker=DescriptiveMarkers.find(Component->second.StructuralComponents[Pos]);
        if (DescriptiveMarker!=DescriptiveMarkers.end())
        {
            as11s::iterator AS11=AS11s.find(DescriptiveMarker->second.Framework);
            if (AS11!=AS11s.end())
            {
                if (StreamKind_Last==Stream_Max)
                {
                    Stream_Prepare(Stream_Other);
                    Fill(Stream_Other, StreamPos_Last, Other_ID, TrackID);
                    Fill(Stream_Other, StreamPos_Last, Other_Type, "Metadata");
                    if (AS11->second.Type==as11::Type_Segmentation)
                    {
                        if (AS11->second.PartTotal!=(int16u)-1)
                            Fill(Stream_Other, StreamPos_Last, "PartTotal", AS11->second.PartTotal);
                    }
                }

                switch (AS11->second.Type)
                {
                    case as11::Type_Core:
                                                    Fill(Stream_Other, StreamPos_Last, "Format", "AS-11 Core");
                                                    Fill(Stream_Other, StreamPos_Last, "SeriesTitle", AS11->second.SeriesTitle);
                                                    Fill(Stream_Other, StreamPos_Last, "ProgrammeTitle", AS11->second.ProgrammeTitle);
                                                    Fill(Stream_Other, StreamPos_Last, "EpisodeTitleNumber", AS11->second.EpisodeTitleNumber);
                                                    Fill(Stream_Other, StreamPos_Last, "ShimName", AS11->second.ShimName);
                                                    if (AS11->second.ShimVersion_Major!=(int8u)-1)
                                                    {
                                                       Ztring Version=Ztring::ToZtring(AS11->second.ShimVersion_Major);
                                                       if (AS11->second.ShimVersion_Minor!=(int8u)-1)
                                                       {
                                                           Version+=__T('.');
                                                           Version+=Ztring::ToZtring(AS11->second.ShimVersion_Minor);
                                                       }
                                                       Fill(Stream_Other, StreamPos_Last, "ShimVersion", Version);
                                                    }
                                                    if (AS11->second.AudioTrackLayout<Mxf_AS11_AudioTrackLayout_Count)
                                                    {
                                                        Fill(Stream_Other, StreamPos_Last, "AudioTrackLayout", Mxf_AS11_AudioTrackLayout[AS11->second.AudioTrackLayout]);

                                                        //Per track
                                                        const mxf_as11_audiotracklayout_assignment &ChP=Mxf_AS11_AudioTrackLayout_ChannelPositions[AS11->second.AudioTrackLayout];
                                                        const mxf_as11_audiotracklayout_assignment &ChL=Mxf_AS11_AudioTrackLayout_ChannelLayout[AS11->second.AudioTrackLayout];
                                                        if (Count_Get(Stream_Audio)>=ChP.Count)
                                                            for (int8u Pos=0; Pos<ChP.Count; ++Pos)
                                                            {
                                                                if (ChP.Assign[Pos])
                                                                    Fill(Stream_Audio, Pos, Audio_ChannelPositions, ChP.Assign[Pos]);
                                                                if (ChL.Assign[Pos])
                                                                    Fill(Stream_Audio, Pos, Audio_ChannelLayout, ChL.Assign[Pos]);
                                                                Fill(Stream_Audio, Pos, Audio_ChannelLayoutID, Mxf_AS11_AudioTrackLayout[AS11->second.AudioTrackLayout]);
                                                            }
                                                    }
                                                    Fill(Stream_Other, StreamPos_Last, "PrimaryAudioLanguage", AS11->second.PrimaryAudioLanguage);
                                                    //Fill_SetOptions(Stream_Other][StreamPos_Last](Ztring().From_UTF8("PrimaryAudioLanguage", "N NT");
                                                    //if (MediaInfoLib::Config.Iso639_Find(AS11->second.PrimaryAudioLanguage).empty())
                                                    //    Fill(Stream_Other, StreamPos_Last, "PrimaryAudioLanguage/String", MediaInfoLib::Config.Iso639_Translate(AS11->second.PrimaryAudioLanguage));
                                                    if (AS11->second.ClosedCaptionsPresent<2)
                                                        Fill(Stream_Other, StreamPos_Last, "ClosedCaptionsPresent", AS11->second.ClosedCaptionsPresent?"Yes":"No");
                                                    if (AS11->second.ClosedCaptionsType<Mxf_AS11_ClosedCaptionType_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "ClosedCaptionType", Mxf_AS11_ClosedCaptionType[AS11->second.ClosedCaptionsType]);
                                                    Fill(Stream_Other, StreamPos_Last, "ClosedCaptionsLanguage", AS11->second.ClosedCaptionsLanguage);
                                                    break;
                    case as11::Type_Segmentation:
                                                    Fill(Stream_Other, StreamPos_Last, "Format", "AS-11 Segmentation", Unlimited, true, true);
                                                    if (AS11->second.PartNumber!=(int16u)-1 && AS11->second.PartTotal!=(int16u)-1)
                                                    {
                                                        string S;
                                                        S+=TimeCode((int64_t)(TC_Temp+Duration_CurrentPos), FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString();
                                                        if (DescriptiveMarker->second.Duration!=(int64u)-1)
                                                        {
                                                            S+=" + ";
                                                            S+=TimeCode((int64_t)DescriptiveMarker->second.Duration, FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString();
                                                            S+=" = ";
                                                            Duration_CurrentPos+=DescriptiveMarker->second.Duration;
                                                            S+=TimeCode((int64_t)(TC_Temp+Duration_CurrentPos), FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString();
                                                            Duration_Programme+=DescriptiveMarker->second.Duration;
                                                        }
                                                        Fill(Stream_Other, StreamPos_Last, Ztring::ToZtring(AS11->second.PartNumber).To_UTF8().c_str(), S);
                                                    }
                                                    break;
                    case as11::Type_UKDPP:
                                                    Fill(Stream_Other, StreamPos_Last, "Format", "AS-11 UKDPP");
                                                    Fill(Stream_Other, StreamPos_Last, "ProductionNumber", AS11->second.ProductionNumber);
                                                    Fill(Stream_Other, StreamPos_Last, "Synopsis", AS11->second.Synopsis);
                                                    Fill(Stream_Other, StreamPos_Last, "Originator", AS11->second.Originator);
                                                    if (AS11->second.CopyrightYear!=(int16u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "CopyrightYear", AS11->second.CopyrightYear);
                                                    Fill(Stream_Other, StreamPos_Last, "OtherIdentifier", AS11->second.OtherIdentifier);
                                                    Fill(Stream_Other, StreamPos_Last, "OtherIdentifierType", AS11->second.OtherIdentifierType);
                                                    Fill(Stream_Other, StreamPos_Last, "Genre", AS11->second.Genre);
                                                    Fill(Stream_Other, StreamPos_Last, "Distributor", AS11->second.Distributor);
                                                    if (AS11->second.PictureRatio_D!=(int32u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "PictureRatio", Ztring::ToZtring(AS11->second.PictureRatio_N)+__T(':')+Ztring::ToZtring(AS11->second.PictureRatio_D));
                                                    if (AS11->second.ThreeD!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "3D", AS11->second.ThreeD?__T("Yes"):__T("No"));
                                                    if (AS11->second.ThreeDType<Mxf_AS11_3D_Type_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "3DType", Mxf_AS11_3D_Type[AS11->second.ThreeDType]);
                                                    if (AS11->second.ProductPlacement!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "ProductPlacement", AS11->second.ProductPlacement?__T("Yes"):__T("No"));
                                                    if (AS11->second.FpaPass<Mxf_AS11_FpaPass_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "FpaPass", Mxf_AS11_FpaPass[AS11->second.FpaPass]);
                                                    Fill(Stream_Other, StreamPos_Last, "FpaManufacturer", AS11->second.FpaManufacturer);
                                                    Fill(Stream_Other, StreamPos_Last, "FpaVersion", AS11->second.FpaVersion);
                                                    Fill(Stream_Other, StreamPos_Last, "VideoComments", AS11->second.VideoComments);
                                                    if (AS11->second.SecondaryAudioLanguage!=__T("zxx"))
                                                        Fill(Stream_Other, StreamPos_Last, "SecondaryAudioLanguage", AS11->second.SecondaryAudioLanguage);
                                                    if (AS11->second.TertiaryAudioLanguage!=__T("zxx"))
                                                        Fill(Stream_Other, StreamPos_Last, "TertiaryAudioLanguage", AS11->second.TertiaryAudioLanguage);
                                                    if (AS11->second.AudioLoudnessStandard<Mxf_AS11_AudioLoudnessStandard_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "AudioLoudnessStandard", Mxf_AS11_AudioLoudnessStandard[AS11->second.AudioLoudnessStandard]);
                                                    Fill(Stream_Other, StreamPos_Last, "AudioComments", AS11->second.AudioComments);
                                                    if (AS11->second.LineUpStart!=(int64u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "LineUpStart", Ztring().From_UTF8(TimeCode((int64_t)(TC_Temp+AS11->second.LineUpStart), FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString()));
                                                    if (AS11->second.IdentClockStart!=(int64u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "IdentClockStart", Ztring().From_UTF8(TimeCode((int64_t)(TC_Temp+AS11->second.IdentClockStart), FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString()));
                                                    if (AS11->second.TotalNumberOfParts!=(int16u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "TotalNumberOfParts", AS11->second.TotalNumberOfParts);
                                                    if (AS11->second.TotalProgrammeDuration!=(int64u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "TotalProgrammeDuration", Ztring().From_UTF8(TimeCode((int64_t)AS11->second.TotalProgrammeDuration, FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString()));
                                                    if (AS11->second.AudioDescriptionPresent!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "AudioDescriptionPresent", AS11->second.AudioDescriptionPresent?__T("Yes"):__T("No"));
                                                    if (AS11->second.AudioDescriptionType<Mxf_AS11_AudioDescriptionType_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "AudioDescriptionType", Mxf_AS11_AudioLoudnessStandard[AS11->second.AudioDescriptionType]);
                                                    if (AS11->second.OpenCaptionsPresent!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "OpenCaptionsPresent", AS11->second.OpenCaptionsPresent?__T("Yes"):__T("No"));
                                                    if (AS11->second.OpenCaptionsType<Mxf_AS11_OpenCaptionsType_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "OpenCaptionsType", Mxf_AS11_OpenCaptionsType[AS11->second.OpenCaptionsType]);
                                                    Fill(Stream_Other, StreamPos_Last, "OpenCaptionsLanguage", AS11->second.OpenCaptionsLanguage);
                                                    if (AS11->second.SigningPresent<Mxf_AS11_SigningPresent_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "SigningPresent", Mxf_AS11_SigningPresent[AS11->second.SigningPresent]);
                                                    if (AS11->second.SignLanguage<Mxf_AS11_SignLanguage_Count)
                                                        Fill(Stream_Other, StreamPos_Last, "SignLanguage", Mxf_AS11_SignLanguage[AS11->second.SignLanguage]);
                                                    //if (AS11->second.CompletionDate!=(int64u)-1)
                                                    //    Fill(Stream_Other, StreamPos_Last, "CompletionDate", Ztring::ToZtring(AS11->second.CompletionDate)+__T(" (TODO: Timestamp translation)")); //TODO: Timestamp
                                                    if (AS11->second.TextlessElementsExist!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "TextlessElementsExist", AS11->second.TextlessElementsExist?__T("Yes"):__T("No"));
                                                    if (AS11->second.ProgrammeHasText!=(int8u)-1)
                                                        Fill(Stream_Other, StreamPos_Last, "ProgrammeHasText", AS11->second.ProgrammeHasText?__T("Yes"):__T("No"));
                                                    Fill(Stream_Other, StreamPos_Last, "ProgrammeTextLanguage", AS11->second.ProgrammeTextLanguage);
                                                    Fill(Stream_Other, StreamPos_Last, "ContactEmail", AS11->second.ContactEmail);
                                                    Fill(Stream_Other, StreamPos_Last, "ContactTelephoneNumber", AS11->second.ContactTelephoneNumber);
                                                    break;
                    default: ;
                }
            }
            else if (DescriptiveMarker->second.IsAs11SegmentFiller && DescriptiveMarker->second.Duration!=(int64u)-1)
                Duration_CurrentPos+=DescriptiveMarker->second.Duration;
        }
    }
    if (Duration_Programme)
        Fill(Stream_Other, StreamPos_Last, "TotalProgrammeDuration", TimeCode((int64_t)Duration_Programme, FrameRate_TempI-1, TimeCode::DropFrame(DropFrame_Temp).FPS1001(DropFrame_Temp)).ToString());
}

//---------------------------------------------------------------------------
void File_Mxf::Streams_Finish_Identification (const int128u IdentificationUID)
{
    identifications::iterator Identification=Identifications.find(IdentificationUID);
    if (Identification==Identifications.end())
        return;

    //Product part
    Ztring Encoded_Application_Version=Identification->second.ProductVersion.empty()?Identification->second.VersionString:Identification->second.ProductVersion;
    Ztring Encoded_Application_ProductName(Identification->second.ProductName);
    if (!Identification->second.CompanyName.empty() && Identification->second.CompanyName.size()<Encoded_Application_ProductName.size())
    {
        Ztring ProductName_Begin(Encoded_Application_ProductName.c_str(), Identification->second.CompanyName.size());
        if (Identification->second.CompanyName.Compare(ProductName_Begin) && Encoded_Application_ProductName[Identification->second.CompanyName.size()]==__T(' '))
            Encoded_Application_ProductName.erase(0, Identification->second.CompanyName.size()+1);
    }
    size_t Encoded_Application_ProductName_Pos = Encoded_Application_ProductName.find_last_of(__T(' '));
    if (Encoded_Application_ProductName_Pos!=string::npos)
    {
        Ztring Encoded_Application_ProductName_End(Encoded_Application_ProductName.c_str()+Encoded_Application_ProductName_Pos+1);
        if (Encoded_Application_Version.find(Encoded_Application_ProductName_End)==0)
            Encoded_Application_ProductName.resize(Encoded_Application_ProductName_Pos); //Removing version number from the name (format not conform)
    }
    Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Identification->second.CompanyName, true);
    Fill(Stream_General, 0, General_Encoded_Application_Name, Encoded_Application_ProductName, true);
    Fill(Stream_General, 0, General_Encoded_Application_Version, Encoded_Application_Version, true);

    //Platform part
    Ztring Library_Name(Identification->second.Platform);
    size_t Library_Name_Pos = Library_Name.find_last_of(__T(' '));
    if (Library_Name_Pos!=string::npos)
    {
        Ztring Library_Name_End(Library_Name.c_str()+Library_Name_Pos+1);
        if (Identification->second.ToolkitVersion.find(Library_Name_End)==0)
            Library_Name.resize(Library_Name_Pos); //Removing version number from the name (format not conform)
    }
    Fill(Stream_General, 0, General_Encoded_Library_Name, Library_Name, true);
    Fill(Stream_General, 0, General_Encoded_Library_Version, Identification->second.ToolkitVersion, true);

    for (std::map<std::string, Ztring>::iterator Info=Identification->second.Infos.begin(); Info!=Identification->second.Infos.end(); ++Info)
        Fill(Stream_General, 0, Info->first.c_str(), Info->second, true);
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mxf::Read_Buffer_Init()
{
    EssenceContainer_FromPartitionMetadata=0;
    #if MEDIAINFO_DEMUX
         Demux_UnpacketizeContainer=Config->Demux_Unpacketize_Get();
         Demux_Rate=Config->Demux_Rate_Get();
    #endif //MEDIAINFO_DEMUX

    //Config
    TimeCodeFromMaterialPackage=Config->File_Mxf_TimeCodeFromMaterialPackage_Get();
}

//---------------------------------------------------------------------------
void File_Mxf::Read_Buffer_Continue()
{
    #if MEDIAINFO_DEMUX
        if (Demux_CurrentParser)
        {
            if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded--;
            Open_Buffer_Continue(Demux_CurrentParser, Buffer+Buffer_Offset, 0, false);
            if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                Frame_Count_NotParsedIncluded++;
            if (Config->Demux_EventWasSent)
                return;
            switch (Demux_CurrentParser->Field_Count_InThisBlock)
            {
                case 1 : Demux_CurrentEssence->second.Field_Count_InThisBlock_1++; break;
                case 2 : Demux_CurrentEssence->second.Field_Count_InThisBlock_2++; break;
                default: ;
            }
            if (Demux_CurrentParser->Buffer_Size)
                Demux_CurrentParser=NULL; //No more need of it
        }
    #endif //MEDIAINFO_DEMUX

    Read_Buffer_CheckFileModifications();

    if (IsRtmd)
    {
        Synched=false;
        if (Buffer_Size>=18)
        {
            Skip_XX(13,                                         "Unknown");
            Element_Begin1("Time code");
                int8u HH, MM, SS, Drop, FF;
                Get_B1(HH,                                      "HH");
                Get_B1(MM,                                      "MM");
                Get_B1(SS,                                      "SS");
                Get_B1(Drop,                                    "Drop");
                Get_B1(FF,                                      "SS");
                TimeCode TC(HH, MM, SS, FF, FF<=99?99:255, TimeCode::flags().DropFrame(Drop));
                Element_Info1(TC.ToString());
                if (!Frame_Count_NotParsedIncluded)
                {
                    Accept();
                    Stream_Prepare(Stream_Other);
                    Fill(Stream_Other, 0, Other_Format, "Sony Real Time Metadata");
                    Fill(Stream_Other, 0, Other_TimeCode_FirstFrame, TC.ToString());
                }
            Element_End0();
        }
    }

    if (IsSearchingFooterPartitionAddress)
    {
        if (File_Offset+Buffer_Size<File_Size)
        {
            Element_WaitForMoreData();
            return;
        }

        IsSearchingFooterPartitionAddress=false;
        Buffer_Offset=Buffer_Size; //Default is end of file (not found)

        const int8u* B_Cur06=Buffer+Buffer_Size-16;
        while (B_Cur06>=Buffer)
        {
            while (B_Cur06>=Buffer)
            {
                if (*B_Cur06==0x06)
                    break;
                B_Cur06--;
            }
            if (B_Cur06<Buffer)
                break;

            const int8u* B_Cur=B_Cur06;
            if (*(B_Cur++)==0x06
             && *(B_Cur++)==0x0E
             && *(B_Cur++)==0x2B
             && *(B_Cur++)==0x34
             && *(B_Cur++)==0x02
             && *(B_Cur++)==0x05
             && *(B_Cur++)==0x01
             && *(B_Cur++)==0x01
             && *(B_Cur++)==0x0D
             && *(B_Cur++)==0x01
             && *(B_Cur++)==0x02
             && *(B_Cur++)==0x01
             && *(B_Cur++)==0x01
             && *(B_Cur++)==0x04)
            {
                IsCheckingFooterPartitionAddress=true;
                Buffer_Offset=B_Cur06-Buffer;
                break;
            }

            B_Cur06--;
        }

        if (B_Cur06<Buffer)
        {
            GoToFromEnd(0);
            return;
        }
    }

    if (IsCheckingFooterPartitionAddress)
    {
        if (Buffer_Offset+17>Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }

        IsCheckingFooterPartitionAddress=false;

        const int8u* B_Cur=Buffer+Buffer_Offset;
        if (*(B_Cur++)==0x06
         && *(B_Cur++)==0x0E
         && *(B_Cur++)==0x2B
         && *(B_Cur++)==0x34
         && *(B_Cur++)==0x02
         && *(B_Cur++)==0x05
         && *(B_Cur++)==0x01
         && *(B_Cur++)==0x01
         && *(B_Cur++)==0x0D
         && *(B_Cur++)==0x01
         && *(B_Cur++)==0x02
         && *(B_Cur++)==0x01
         && *(B_Cur++)==0x01
         && *(B_Cur++)==0x04)
        {
            int64u Size=*(B_Cur++);
            if (Size >= 0x80)
            {
                Size&=0x7F;
                if (17+Size>Buffer_Size)
                {
                    if (File_Offset+17+Size<File_Size)
                    {
                        Element_WaitForMoreData();
                        return;
                    }

                    IsTruncated(File_Offset+17+Size, true, "MXF");
                }
            }
        }
        else
        {
            GoToFromEnd(4); //For random access table because it is invalid footer
            return;
        }
    }

    if (IsCheckingRandomAccessTable)
    {
        if (17>Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }
        IsCheckingRandomAccessTable=false;
        if (CC4(Buffer+Buffer_Offset)!=0x060E2B34 || CC3(Buffer+Buffer_Offset+4)!=0x020501 || CC3(Buffer+Buffer_Offset+8)!=0x0D0102 || CC1(Buffer+Buffer_Offset+12)!=0x01) // Checker if it is a Partition
        {
            if (File_Size>=64*1024)
            {
                IsSearchingFooterPartitionAddress=true;
                GoToFromEnd(64*1024); // Wrong offset, searching from end of file
            }
            else
                GoToFromEnd(0);
            return;
        }
    }

    if (Config->ParseSpeed<1.0 && File_Offset+Buffer_Offset+4==File_Size)
    {
        int32u Length;
        Get_B4 (Length,                                         "Length (Random Index)");
        if (Length>=16+4 && Length<File_Size/2)
        {
            GoToFromEnd(Length); //For random access table
            IsCheckingRandomAccessTable=true;
            Open_Buffer_Unsynch();
        }
        else
        {
            if (File_Size>=64*1024)
            {
                IsSearchingFooterPartitionAddress=true;
                GoToFromEnd(64*1024); // Wrong offset, searching from end of file
            }
            else
                GoToFromEnd(0);
            return;
        }
    }
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
void File_Mxf::Read_Buffer_CheckFileModifications()
{
    if (!IsSub)
    {
        if (Config->ParseSpeed>=1.0)
        {
            bool Buffer_End_IsUpdated=false;
            if (HeaderPartition_IsOpen && !Config->File_IsNotGrowingAnymore)
            {
                File F;
                F.Open(File_Name);
                std::vector<int8u> SearchingPartitionPack(65536);
                size_t SearchingPartitionPack_Size=F.Read(&SearchingPartitionPack[0], SearchingPartitionPack.size());
                for (size_t Pos=0; Pos+16<SearchingPartitionPack_Size; Pos++)
                    if (SearchingPartitionPack[Pos   ]==0x06
                     && SearchingPartitionPack[Pos+ 1]==0x0E
                     && SearchingPartitionPack[Pos+ 2]==0x2B
                     && SearchingPartitionPack[Pos+ 3]==0x34
                     && SearchingPartitionPack[Pos+ 4]==0x02
                     && SearchingPartitionPack[Pos+ 5]==0x05
                     && SearchingPartitionPack[Pos+ 6]==0x01
                     && SearchingPartitionPack[Pos+ 7]==0x01
                     && SearchingPartitionPack[Pos+ 8]==0x0D
                     && SearchingPartitionPack[Pos+ 9]==0x01
                     && SearchingPartitionPack[Pos+10]==0x02
                     && SearchingPartitionPack[Pos+11]==0x01
                     && SearchingPartitionPack[Pos+12]==0x01
                     && SearchingPartitionPack[Pos+13]==0x02) //Header Partition Pack
                    {
                        switch (SearchingPartitionPack[Pos+14])
                        {
                            case 0x02 :
                            case 0x04 :
                                        {
                                        //Filling duration
                                        F.Close();
                                        Config->File_IsNotGrowingAnymore=true;
                                        MediaInfo_Internal MI;
                                        Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
                                        Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
                                        MI.Option(__T("ParseSpeed"), __T("0"));
                                        MI.Option(__T("Demux"), Ztring());
                                        size_t MiOpenResult=MI.Open(File_Name);
                                        MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
                                        MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
                                        if (MiOpenResult)
                                        {
                                            Fill(Stream_General, 0, General_Format_Settings, MI.Get(Stream_General, 0, General_Format_Settings), true);
                                            Fill(Stream_General, 0, General_Duration, MI.Get(Stream_General, 0, General_Duration), true);
                                            Fill(Stream_General, 0, General_FileSize, MI.Get(Stream_General, 0, General_FileSize), true);
                                            Fill(Stream_General, 0, General_StreamSize, MI.Get(Stream_General, 0, General_StreamSize), true);
                                            if (Buffer_End_Unlimited)
                                            {
                                                Buffer_End=MI.Get(Stream_General, 0, General_FileSize).To_int64u()-MI.Get(Stream_General, 0, General_FooterSize).To_int64u();
                                                Buffer_End_IsUpdated=true;
                                            }
                                            #if defined(MEDIAINFO_REFERENCES_YES)
                                            if (!Config->File_IsReferenced_Get() && ReferenceFiles && Retrieve(Stream_General, 0, General_StreamSize).To_int64u())
                                            {
                                                //Playlist file size is not correctly modified
                                                Config->File_Size-=File_Size;
                                                File_Size=Retrieve(Stream_General, 0, General_StreamSize).To_int64u();
                                                Config->File_Size+=File_Size;
                                            }
                                            #endif //MEDIAINFO_REFERENCES_YES
                                        }
                                        }
                                        break;
                            default   : ;
                        }
                    }

                if (Buffer_End && Buffer_End_Unlimited && !Buffer_End_IsUpdated)
                    Buffer_End=Config->File_Size; //Updating Clip end in case the
            }

            Config->State_Set(((float)Buffer_TotalBytes)/Config->File_Size);
        }
    }
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
void File_Mxf::Read_Buffer_AfterParsing()
{
    if (File_GoTo==(int64u)-1 && File_Offset+Buffer_Offset>=IsParsingMiddle_MaxOffset)
    {
        Fill();
        Open_Buffer_Unsynch();
        Finish();
        return;
    }

    if (Config->IsFinishing)
    {
        if (Partitions_IsCalculatingHeaderByteCount)
        {
            Partitions_IsCalculatingHeaderByteCount=false;
            if (Partitions_Pos<Partitions.size())
                Partitions[Partitions_Pos].PartitionPackByteCount=File_Offset+Buffer_Offset-Partitions[Partitions_Pos].StreamOffset;
        }

        if (IsParsingEnd)
        {
            if (PartitionMetadata_PreviousPartition && RandomIndexPacks.empty() && !RandomIndexPacks_AlreadyParsed)
            {
                Partitions_Pos=0;
                while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset!=PartitionMetadata_PreviousPartition)
                    Partitions_Pos++;
                if (Partitions_Pos==Partitions.size())
                {
                    GoTo(PartitionMetadata_PreviousPartition);
                    Open_Buffer_Unsynch();
                    return;
                }
            }
        }

        //Checking if we want to seek again
        if (File_GoTo==(int64u)-1)
            GoToFromEnd(0);
    }

    if (IsSub)
    {
        Frame_Count++;
        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
            Frame_Count_NotParsedIncluded++;
        if (!Status[IsFilled] && (Config->ParseSpeed<=0 || IsRtmd))
            Fill();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Read_Buffer_Unsynched()
{
    //Adapting DataSizeToParse
    if (Buffer_End)
    {
        if (File_GoTo>=Buffer_End //Too much late
        || File_GoTo<=Buffer_Begin) //Too much early
        {
            Buffer_Begin=(int64u)-1;
            Buffer_End=0;
            Buffer_End_Unlimited=false;
            Buffer_Header_Size=0;
            MustSynchronize=true;
            Synched=false;
            UnSynched_IsNotJunk=true;
        }
        else
            Synched=true; //Always in clip data
    }

    FrameInfo=frame_info();
    FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
    Frame_Count_NotParsedIncluded=(int64u)-1;
    #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
        if (!Tracks.empty() && Tracks.begin()->second.EditRate) //TODO: use the corresponding track instead of the first one
            FrameInfo.DUR=float64_int64s(1000000000/Tracks.begin()->second.EditRate);
        else if (!IndexTables.empty() && IndexTables[0].IndexEditRate)
            FrameInfo.DUR=float64_int64s(1000000000/IndexTables[0].IndexEditRate);

        //Calculating the byte count not included in seek information (partition, index...)
        int64u FutureFileOffset=File_GoTo==(int64u)-1?(File_Offset+Buffer_Offset):File_GoTo;
        int64u StreamOffset_Offset=0;
        Partitions_Pos=0;
        while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<=FutureFileOffset)
        {
            StreamOffset_Offset+=Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
            Partitions_Pos++;
        }

        if (Partitions_Pos==2 && Partitions[1].StreamOffset==FutureFileOffset && Descriptors.size()==1 && Descriptors.begin()->second.StreamKind==Stream_Text)
            Frame_Count_NotParsedIncluded=0;

        if (Descriptors.size()==1 && Descriptors.begin()->second.ByteRate!=(int32u)-1 && Descriptors.begin()->second.SampleRate)
        {
            float64 BytePerFrame=Descriptors.begin()->second.ByteRate/Descriptors.begin()->second.SampleRate;
            float64 Frame_Count_NotParsedIncluded_Precise;
            if (FutureFileOffset>(StreamOffset_Offset+Buffer_Header_Size))
                Frame_Count_NotParsedIncluded_Precise=(FutureFileOffset-(StreamOffset_Offset+Buffer_Header_Size))/BytePerFrame; //In case of audio at frame rate not an integer
            else
                Frame_Count_NotParsedIncluded_Precise=0;
            Frame_Count_NotParsedIncluded=float64_int64s(Frame_Count_NotParsedIncluded_Precise);
            FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000+((float64)Frame_Count_NotParsedIncluded_Precise)*1000000000/Descriptors.begin()->second.SampleRate);
            FrameInfo.PTS=FrameInfo.DTS;
            if (!Tracks.empty() && Tracks.begin()->second.EditRate) //TODO: use the corresponding track instead of the first one
                FrameInfo.DUR=float64_int64s(1000000000/Tracks.begin()->second.EditRate);
            else if (!IndexTables.empty() && IndexTables[0].IndexEditRate)
                FrameInfo.DUR=float64_int64s(1000000000/IndexTables[0].IndexEditRate);
            else
                FrameInfo.DUR=float64_int64s(1000000000/Descriptors.begin()->second.SampleRate);
            #if MEDIAINFO_DEMUX
                Demux_random_access=true;
            #endif //MEDIAINFO_DEMUX
        }
        else if (!IndexTables.empty() && IndexTables[0].EditUnitByteCount)
        {
            int64u Position=0;
            Frame_Count_NotParsedIncluded=0;
            for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
            {
                if (IndexTables[0].IndexDuration && FutureFileOffset>=((Buffer_End?Buffer_Begin:(StreamOffset_Offset+Buffer_Header_Size))+Position)+IndexTables[Pos].IndexDuration*IndexTables[Pos].EditUnitByteCount) //Considering IndexDuration=0 as unlimited
                {
                    Position+=SDTI_SizePerFrame+IndexTables[Pos].EditUnitByteCount*IndexTables[Pos].IndexDuration;
                    Frame_Count_NotParsedIncluded+=IndexTables[Pos].IndexDuration;
                }
                else
                {
                    int64u FramesToAdd;
                    if (FutureFileOffset>((Buffer_End?Buffer_Begin:(StreamOffset_Offset+Buffer_Header_Size))+Position))
                        FramesToAdd=(FutureFileOffset-((Buffer_End?Buffer_Begin:(StreamOffset_Offset+Buffer_Header_Size))+Position))/IndexTables[Pos].EditUnitByteCount;
                    else
                        FramesToAdd=0;
                    Position+=(SDTI_SizePerFrame+IndexTables[Pos].EditUnitByteCount)*FramesToAdd;
                    if (IndexTables[Pos].IndexEditRate)
                    {
                        if (Descriptors.size()==1 && Descriptors.begin()->second.SampleRate!=IndexTables[Pos].IndexEditRate)
                        {
                            float64 Frame_Count_NotParsedIncluded_Precise=((float64)FramesToAdd)/IndexTables[Pos].IndexEditRate*Descriptors.begin()->second.SampleRate;
                            Frame_Count_NotParsedIncluded+=float64_int64s(((float64)FramesToAdd)/IndexTables[Pos].IndexEditRate*Descriptors.begin()->second.SampleRate);
                            FrameInfo.PTS=FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000+((float64)Frame_Count_NotParsedIncluded_Precise)*1000000000/Descriptors.begin()->second.SampleRate);
                        }
                        else
                        {
                            Frame_Count_NotParsedIncluded+=FramesToAdd;
                            FrameInfo.PTS=FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000+((float64)Frame_Count_NotParsedIncluded)*1000000000/IndexTables[Pos].IndexEditRate);
                        }
                    }
                    else
                        FrameInfo.PTS=FrameInfo.DTS=(int64u)-1;
                    #if MEDIAINFO_DEMUX
                        Demux_random_access=true;
                    #endif //MEDIAINFO_DEMUX

                    break;
                }
            }
        }
        else if (!IndexTables.empty() && !IndexTables[0].Entries.empty())
        {
            int64u StreamOffset;
            if (StreamOffset_Offset<FutureFileOffset)
                StreamOffset=FutureFileOffset-StreamOffset_Offset;
            else
                StreamOffset=0;
            for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
            {
                //Searching the right index
                if (!IndexTables[Pos].Entries.empty() && StreamOffset>=IndexTables[Pos].Entries[0].StreamOffset+(IndexTables[Pos].IndexStartPosition)*SDTI_SizePerFrame && (Pos+1>=IndexTables.size() || IndexTables[Pos+1].Entries.empty() || StreamOffset<IndexTables[Pos+1].Entries[0].StreamOffset+(IndexTables[Pos+1].IndexStartPosition)*SDTI_SizePerFrame))
                {
                    //Searching the frame pos
                    for (size_t EntryPos=0; EntryPos<IndexTables[Pos].Entries.size(); EntryPos++)
                    {
                        //Testing coherency
                        int64u Entry0_StreamOffset=0; //For coherency checking
                        int64u Entry_StreamOffset=IndexTables[Pos].Entries[EntryPos].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos)*SDTI_SizePerFrame;
                        int64u Entry1_StreamOffset=File_Size; //For coherency checking
                        if (EntryPos==0 && Pos && !IndexTables[Pos-1].Entries.empty())
                            Entry0_StreamOffset=IndexTables[Pos-1].Entries[IndexTables[Pos-1].Entries.size()-1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos-1)*SDTI_SizePerFrame;
                        else if (EntryPos)
                            Entry0_StreamOffset=IndexTables[Pos].Entries[EntryPos-1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos-1)*SDTI_SizePerFrame;
                        if (EntryPos+1<IndexTables[Pos].Entries.size())
                            Entry1_StreamOffset=IndexTables[Pos].Entries[EntryPos+1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos+1)*SDTI_SizePerFrame;
                        else if (Pos+1<IndexTables.size() && !IndexTables[Pos+1].Entries.empty())
                            Entry1_StreamOffset=IndexTables[Pos+1].Entries[0].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos+1)*SDTI_SizePerFrame;

                        if (Entry0_StreamOffset>Entry_StreamOffset || Entry_StreamOffset>Entry1_StreamOffset)
                            break; //Problem

                        if (StreamOffset>=Entry_StreamOffset && StreamOffset<Entry1_StreamOffset)
                        {
                            //Special case: we are not sure the last index is the last frame, doing nothing
                            if (Pos+1==IndexTables.size() && EntryPos+1==IndexTables[Pos].Entries.size())
                                break;

                            Frame_Count_NotParsedIncluded=IndexTables[Pos].IndexStartPosition+EntryPos;
                            if (IndexTables[Pos].IndexEditRate)
                                FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000+((float64)Frame_Count_NotParsedIncluded)/IndexTables[Pos].IndexEditRate*1000000000);


                            #if MEDIAINFO_DEMUX
                                Demux_random_access=IndexTables[Pos].Entries[EntryPos].Type?false:true;
                            #endif //MEDIAINFO_DEMUX
                            break;
                        }
                    }
                }
            }
        }
        else if (OverallBitrate_IsCbrForSure)
        {
            int64u Begin=Partitions[0].StreamOffset+Partitions[0].PartitionPackByteCount+Partitions[0].HeaderByteCount+Partitions[0].IndexByteCount;
            Frame_Count_NotParsedIncluded=(FutureFileOffset-Begin)/OverallBitrate_IsCbrForSure;
            if (!Descriptors.empty() && Descriptors.begin()->second.SampleRate)
                FrameInfo.PTS=FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000+((float64)Frame_Count_NotParsedIncluded)*1000000000/Descriptors.begin()->second.SampleRate);
        }
        else if (Frame_Count_NotParsedIncluded==0)
        {
            FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
        }

    #endif //if MEDIAINFO_DEMUX || MEDIAINFO_SEEK

    if (!Tracks.empty() && Tracks.begin()->second.EditRate) //TODO: use the corresponding track instead of the first one
        FrameInfo.DUR=float64_int64s(1000000000/Tracks.begin()->second.EditRate);
    #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
        else if (!IndexTables.empty() && IndexTables[0].IndexEditRate)
            FrameInfo.DUR=float64_int64s(1000000000/IndexTables[0].IndexEditRate);
    #endif //if MEDIAINFO_DEMUX || MEDIAINFO_SEEK

    for (essences::iterator Essence=Essences.begin(); Essence!=Essences.end(); ++Essence)
        for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
        {
            (*Parser)->Open_Buffer_Unsynch();
            Essence->second.FrameInfo=FrameInfo;
            Essence->second.Frame_Count_NotParsedIncluded=Frame_Count_NotParsedIncluded;
        }

    Partitions_Pos=0;
    if (Partitions_IsCalculatingHeaderByteCount)
    {
        Partitions.erase(Partitions.end()-1);
        Partitions_IsCalculatingHeaderByteCount=false;
    }
    if (Partitions_IsCalculatingSdtiByteCount)
        Partitions_IsCalculatingSdtiByteCount=false;

    #if MEDIAINFO_SEEK
        IndexTables_Pos=0;
    #endif //MEDIAINFO_SEEK
}

//---------------------------------------------------------------------------
#if (MEDIAINFO_DEMUX || MEDIAINFO_SEEK) && defined(MEDIAINFO_FILE_YES)
bool File_Mxf::DetectDuration ()
{
    if (Config->ParseSpeed<=0 || Duration_Detected)
        return false;

    MediaInfo_Internal MI;
    MI.Option(__T("File_IsDetectingDuration"), __T("1"));
    MI.Option(__T("File_KeepInfo"), __T("1"));
    Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
    Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
    MI.Option(__T("ParseSpeed"), __T("0"));
    MI.Option(__T("Demux"), Ztring());
    MI.Option(__T("File_Mxf_ParseIndex"), __T("1"));
    size_t MiOpenResult=MI.Open(File_Name);
    MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
    MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
    if (!MiOpenResult || MI.Get(Stream_General, 0, General_Format)!=__T("MXF"))
        return false;
    Partitions=((File_Mxf*)MI.Info)->Partitions;
    std::sort(Partitions.begin(), Partitions.end());
    IndexTables=((File_Mxf*)MI.Info)->IndexTables;
    std::sort(IndexTables.begin(), IndexTables.end());
    SDTI_SizePerFrame=((File_Mxf*)MI.Info)->SDTI_SizePerFrame;
    Clip_Begin=((File_Mxf*)MI.Info)->Clip_Begin;
    Clip_End=((File_Mxf*)MI.Info)->Clip_End;
    Clip_Header_Size=((File_Mxf*)MI.Info)->Clip_Header_Size;
    Clip_Code=((File_Mxf*)MI.Info)->Clip_Code;
    Tracks=((File_Mxf*)MI.Info)->Tracks; //In one file (*-009.mxf), the TrackNumber is known only at the end of the file (Open and incomplete header/footer)
    for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
        Track->second.Stream_Finish_Done=false; //Reseting the value, it is not done in this instance
    if (MI.Get(Stream_General, 0, General_OverallBitRate_Mode)==__T("CBR") && Partitions.size()==2 && Partitions[0].FooterPartition==Partitions[1].StreamOffset && !Descriptors.empty())
    {
        //Searching duration
        int64u Duration=0;
        for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
            if (Descriptor->second.Duration!=(int64u)-1 && Descriptor->second.Duration)
            {
                if (Duration && Duration!=Descriptor->second.Duration)
                {
                    Duration=0;
                    break; //Not supported
                }
                Duration=Descriptor->second.Duration;
            }

        //Computing the count of bytes per frame
        if (Duration)
        {
            int64u Begin=Partitions[0].StreamOffset+Partitions[0].PartitionPackByteCount+Partitions[0].HeaderByteCount+Partitions[0].IndexByteCount;
            float64 BytesPerFrameF=((float64)(Partitions[0].FooterPartition-Begin)/Duration);
            OverallBitrate_IsCbrForSure=float64_int64s(BytesPerFrameF);
            if (OverallBitrate_IsCbrForSure!=BytesPerFrameF) //Testing integrity of the computing
                OverallBitrate_IsCbrForSure=0;
        }
    }
    Duration_Detected=true;

    return true;
}
#endif //(MEDIAINFO_DEMUX || MEDIAINFO_SEEK) && defined(MEDIAINFO_FILE_YES)

#if MEDIAINFO_SEEK
size_t File_Mxf::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    #if defined(MEDIAINFO_REFERENCES_YES)
        if (ReferenceFiles)
            return ReferenceFiles->Seek(Method, Value, ID);
    #endif //defined(MEDIAINFO_REFERENCES_YES)

    //Init
    if (!Duration_Detected)
    {
        if (!DetectDuration())
            return 0;
    }

    //Config - TODO: merge with the one in Data_Parse()
    if (!Essences_FirstEssence_Parsed)
    {
        //Searching single descriptor if it is the only valid descriptor
        descriptors::iterator SingleDescriptor=Descriptors.end();
        for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
            if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max)
            {
                if (SingleDescriptor!=Descriptors.end())
                {
                    SingleDescriptor=Descriptors.end();
                    break; // 2 or more descriptors, can not be used
                }
                SingleDescriptor=SingleDescriptor_Temp;
            }

        if (SingleDescriptor!=Descriptors.end() && SingleDescriptor->second.StreamKind==Stream_Audio)
        {
            //Configuring bitrate is not available in descriptor
            if (Descriptors.begin()->second.ByteRate==(int32u)-1)
            {
                std::map<std::string, Ztring>::const_iterator i=Descriptors.begin()->second.Infos.find("SamplingRate");
                if (i!=Descriptors.begin()->second.Infos.end())
                {
                    int32u SamplingRate=i->second.To_int32u();

                    if (Descriptors.begin()->second.BlockAlign!=(int16u)-1)
                        Descriptors.begin()->second.ByteRate=SamplingRate*Descriptors.begin()->second.BlockAlign;
                    else if (Descriptors.begin()->second.QuantizationBits!=(int8u)-1)
                        Descriptors.begin()->second.ByteRate=SamplingRate*Descriptors.begin()->second.QuantizationBits / 8;
                }
            }
        }

        for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
        {
            //Configuring EditRate if needed (e.g. audio at 48000 Hz)
            if (Descriptor->second.SampleRate>1000)
            {
                float64 EditRate_FromTrack=DBL_MAX;
                for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                    if (Track->second.EditRate && EditRate_FromTrack>Track->second.EditRate)
                        EditRate_FromTrack=Track->second.EditRate;
                if (EditRate_FromTrack>1000)
                    EditRate_FromTrack=Demux_Rate; //Default value;
                Descriptor->second.SampleRate=EditRate_FromTrack;
                for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                    if (Track->second.EditRate>EditRate_FromTrack)
                    {
                        Track->second.EditRate_Real=Track->second.EditRate;
                        Track->second.EditRate=EditRate_FromTrack;
                    }
            }
        }

        Essences_FirstEssence_Parsed=true;
    }
    //Parsing
    switch (Method)
    {
        case 0  :
                    {
                    if (Config->File_IgnoreEditsBefore && Config->File_EditRate)
                    {
                        Read_Buffer_Seek(3, 0, (int64u)-1);
                        if (File_GoTo!=(int64u)-1)
                            Value+=File_GoTo;
                    }

                    //Calculating the byte count not included in seek information (partition, index...)
                    Partitions_Pos=0;
                    while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<Value)
                        Partitions_Pos++;
                    if (Partitions_Pos && (Partitions_Pos==Partitions.size() || Partitions[Partitions_Pos].StreamOffset!=Value))
                        Partitions_Pos--; //This is the previous item
                    if (Partitions_Pos>=Partitions.size())
                    {
                        GoTo(0);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    int64u StreamOffset_Offset=Partitions[Partitions_Pos].StreamOffset-Partitions[Partitions_Pos].BodyOffset+Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;

                    //If in header
                    if ((Clip_Begin!=(int64u)-1 && Value<Clip_Begin) || Value<StreamOffset_Offset)
                    {
                        GoTo(StreamOffset_Offset);
                        Open_Buffer_Unsynch();
                        return 1;
                    }

                    if (Buffer_End
                     && Descriptors.size()==1 && Descriptors.begin()->second.ByteRate!=(int32u)-1 && Descriptors.begin()->second.BlockAlign && Descriptors.begin()->second.BlockAlign!=(int16u)-1  && Descriptors.begin()->second.SampleRate)
                    {
                        if (Value>StreamOffset_Offset)
                        {
                            float64 BytesPerFrame=Descriptors.begin()->second.ByteRate/Descriptors.begin()->second.SampleRate;
                            int64u FrameCount=(int64u)((Value-Buffer_Begin)/BytesPerFrame);
                            int64u SizeBlockAligned=float64_int64s(FrameCount*BytesPerFrame);
                            SizeBlockAligned/=Descriptors.begin()->second.BlockAlign;
                            SizeBlockAligned*=Descriptors.begin()->second.BlockAlign;

                            GoTo(Buffer_Begin+SizeBlockAligned);
                            Open_Buffer_Unsynch();
                            return 1;
                        }
                    }
                    else if (Buffer_End
                     && !IndexTables.empty() && IndexTables[0].EditUnitByteCount)
                    {
                        int64u Stream_Offset=0;
                        for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                        {
                            if (IndexTables[Pos].IndexDuration==0 || Value<StreamOffset_Offset+Stream_Offset+IndexTables[Pos].IndexDuration*IndexTables[Pos].EditUnitByteCount)
                            {
                                int64u FrameToAdd=(Value-(StreamOffset_Offset+Stream_Offset))/IndexTables[Pos].EditUnitByteCount;
                                Stream_Offset+=FrameToAdd*IndexTables[Pos].EditUnitByteCount;

                                GoTo(Buffer_Begin+Stream_Offset);
                                Open_Buffer_Unsynch();
                                return 1;
                            }
                            else
                                Stream_Offset+=IndexTables[Pos].IndexDuration*IndexTables[Pos].EditUnitByteCount;
                        }
                        return 2; //Invalid value
                    }

                    GoTo(Value);
                    Open_Buffer_Unsynch();
                    return 1;
                    }
        case 1  :
                    return Read_Buffer_Seek(0, File_Size*Value/10000, ID);
        case 2  :   //Timestamp
                    {
                        //We transform TimeStamp to a frame number
                        descriptors::iterator Descriptor;
                        for (Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                            if (Descriptor->second.SampleRate)
                                break;
                        if (Descriptor==Descriptors.end())
                            return (size_t)-1; //Not supported

                        else if (MxfTimeCodeForDelay.StartTimecode!=(int64u)-1)
                        {
                            int64u Delay=float64_int64s(DTS_Delay*1000000000);
                            if (Value<Delay)
                                return 2; //Invalid value
                            Value-=Delay;
                        }
                        Value=float64_int64s(((float64)Value)/1000000000*Descriptor->second.SampleRate);
                        }
                    [[fallthrough]];
        case 3  :   //FrameNumber
                    Value+=Config->File_IgnoreEditsBefore;

                    if (Descriptors.size()==1 && Descriptors.begin()->second.ByteRate!=(int32u)-1 && Descriptors.begin()->second.BlockAlign && Descriptors.begin()->second.BlockAlign!=(int16u)-1  && Descriptors.begin()->second.SampleRate)
                    {
                        if (Descriptors.begin()->second.SampleRate!=Config->File_EditRate && Config->File_IgnoreEditsBefore)
                        {
                            //Edit rate and Demux rate are different, not well supported for the moment
                            Value-=Config->File_IgnoreEditsBefore;
                            Value+=float64_int64s(((float64)Config->File_IgnoreEditsBefore)/Config->File_EditRate*Descriptors.begin()->second.SampleRate);
                        }

                        float64 BytesPerFrame=Descriptors.begin()->second.ByteRate/Descriptors.begin()->second.SampleRate;
                        int64u StreamOffset=(int64u)(Value*BytesPerFrame);
                        StreamOffset/=Descriptors.begin()->second.BlockAlign;
                        StreamOffset*=Descriptors.begin()->second.BlockAlign;

                        //Calculating the byte count not included in seek information (partition, index...)
                        int64u StreamOffset_Offset=0;
                        Partitions_Pos=0;
                        while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<=StreamOffset_Offset+StreamOffset+Value*SDTI_SizePerFrame)
                        {
                            StreamOffset_Offset+=Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
                            Partitions_Pos++;
                        }

                        if (Clip_Begin!=(int64u)-1)
                        {
                            Buffer_Begin=Clip_Begin;
                            Buffer_End=Clip_End;
                            Buffer_Header_Size=Clip_Header_Size;
                            Code=Clip_Code;
                            MustSynchronize=false;
                            #if MEDIAINFO_DEMUX
                                if (Buffer_End && Demux_UnpacketizeContainer && Essences.size()==1 && Essences.begin()->second.Parsers.size()==1 && (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer)
                                {
                                    (*(Essences.begin()->second.Parsers.begin()))->Demux_Level=2; //Container
                                    (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer=true;
                                }
                            #endif //MEDIAINFO_DEMUX
                        }

                        GoTo(StreamOffset_Offset+Buffer_Header_Size+StreamOffset+Value*SDTI_SizePerFrame);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    else if (!IndexTables.empty() && IndexTables[0].EditUnitByteCount)
                    {
                        if (Descriptors.size()==1 && Descriptors.begin()->second.SampleRate!=IndexTables[0].IndexEditRate)
                        {
                            float64 ValueF=(float64)Value;
                            ValueF/=Descriptors.begin()->second.SampleRate;
                            ValueF*=IndexTables[0].IndexEditRate;
                            Value=float64_int64s(ValueF);
                        }

                        if (IndexTables[IndexTables.size()-1].IndexDuration && IndexTables[IndexTables.size()-1].IndexStartPosition!=(int64u)-1 && Value>=IndexTables[IndexTables.size()-1].IndexStartPosition+IndexTables[IndexTables.size()-1].IndexDuration) //Considering IndexDuration=0 as unlimited
                            return 2; //Invalid value

                        int64u StreamOffset=0;
                        for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                        {
                            if (IndexTables[Pos].IndexDuration && Value>IndexTables[Pos].IndexStartPosition+IndexTables[Pos].IndexDuration) //Considering IndexDuration=0 as unlimited
                                StreamOffset+=IndexTables[Pos].EditUnitByteCount*IndexTables[Pos].IndexDuration;
                            else
                            {
                                StreamOffset+=IndexTables[Pos].EditUnitByteCount*(Value-IndexTables[Pos].IndexStartPosition);
                                break;
                            }
                        }

                        //Calculating the byte count not included in seek information (partition, index...)
                        int64u StreamOffset_Offset=0;
                        Partitions_Pos=0;
                        while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<=StreamOffset_Offset+StreamOffset+Value*SDTI_SizePerFrame)
                        {
                            StreamOffset_Offset+=Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
                            Partitions_Pos++;
                        }

                        if (Clip_Begin!=(int64u)-1)
                        {
                            Buffer_Begin=Clip_Begin;
                            Buffer_End=Clip_End;
                            Buffer_Header_Size=Clip_Header_Size;
                            Code=Clip_Code;
                            MustSynchronize=false;
                            #if MEDIAINFO_DEMUX
                                if (Buffer_End && Demux_UnpacketizeContainer && Essences.size()==1 && Essences.begin()->second.Parsers.size()==1 && (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer)
                                {
                                    (*(Essences.begin()->second.Parsers.begin()))->Demux_Level=2; //Container
                                    (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer=true;
                                }
                            #endif //MEDIAINFO_DEMUX
                        }

                        GoTo(StreamOffset_Offset+Buffer_Header_Size+StreamOffset+Value*SDTI_SizePerFrame);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    else if (!IndexTables.empty() && !IndexTables[0].Entries.empty())
                    {
                        for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                        {
                            if (Value>=IndexTables[Pos].IndexStartPosition && Value<IndexTables[Pos].IndexStartPosition+IndexTables[Pos].IndexDuration)
                            {
                                while (Value>=IndexTables[Pos].IndexStartPosition && IndexTables[Pos].Entries[(size_t)(Value-IndexTables[Pos].IndexStartPosition)].Type)
                                {
                                    Value--;
                                    if (Value<IndexTables[Pos].IndexStartPosition)
                                    {
                                        if (Pos==0)
                                            break; //There is a problem
                                        Pos--; //In previous index
                                    }
                                }

                                int64u StreamOffset=IndexTables[Pos].Entries[(size_t)(Value-IndexTables[Pos].IndexStartPosition)].StreamOffset;

                                //Calculating the byte count not included in seek information (partition, index...)
                                int64u StreamOffset_Offset=0;
                                Partitions_Pos=0;
                                while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<=StreamOffset_Offset+StreamOffset+Value*SDTI_SizePerFrame)
                                {
                                    StreamOffset_Offset+=Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
                                    Partitions_Pos++;
                                }

                                if (Clip_Begin!=(int64u)-1)
                                {
                                    Buffer_Begin=Clip_Begin;
                                    Buffer_End=Clip_End;
                                    Buffer_Header_Size=Clip_Header_Size;
                                    Code=Clip_Code;
                                    MustSynchronize=false;
                                    #if MEDIAINFO_DEMUX
                                        if (Buffer_End && Demux_UnpacketizeContainer && Essences.size()==1 && Essences.begin()->second.Parsers.size()==1 && (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer)
                                        {
                                            (*(Essences.begin()->second.Parsers.begin()))->Demux_Level=2; //Container
                                            (*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer=true;
                                        }
                                    #endif //MEDIAINFO_DEMUX
                                }

                                GoTo(StreamOffset_Offset+Buffer_Header_Size+StreamOffset+Value*SDTI_SizePerFrame);
                                Open_Buffer_Unsynch();
                                return 1;
                            }
                        }
                        return 2; //Invalid value
                    }
                    else if (OverallBitrate_IsCbrForSure)
                    {
                        int64u Begin=Partitions[0].StreamOffset+Partitions[0].PartitionPackByteCount+Partitions[0].HeaderByteCount+Partitions[0].IndexByteCount;
                        GoTo(Begin+Value*OverallBitrate_IsCbrForSure);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    else
                        return (size_t)-1; //Not supported
        default :   return (size_t)-1; //Not supported
    }
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mxf::FileHeader_Begin()
{
    //Element_Size
    if (Buffer_Size<0x18)
        return false; //Must wait for more data

    //AAF has some MXF start codes
    if (Buffer[ 0x0]==0xD0
     && Buffer[ 0x1]==0xCF
     && Buffer[ 0x2]==0x11
     && Buffer[ 0x3]==0xE0
     && Buffer[ 0x4]==0xA1
     && Buffer[ 0x5]==0xB1
     && Buffer[ 0x6]==0x1A
     && Buffer[ 0x7]==0xE1
     && Buffer[ 0x8]==0x41
     && Buffer[ 0x9]==0x41
     && Buffer[ 0xA]==0x46
     && Buffer[ 0xB]==0x42
     && Buffer[ 0xC]==0x0D
     && Buffer[ 0xD]==0x00
     && Buffer[ 0xE]==0x4F
     && Buffer[ 0xF]==0x4D
     && Buffer[0x10]==0x06
     && Buffer[0x11]==0x0E
     && Buffer[0x12]==0x2B
     && Buffer[0x13]==0x34
     && Buffer[0x14]==0x01
     && Buffer[0x15]==0x01
     && Buffer[0x16]==0x01
     && Buffer[0x17]==0xFF)
    {
        Reject("Mxf");
        return false;
    }

    return true;
}

//***************************************************************************
// Buffer - Synchro
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mxf::Synchronize()
{
    //Synchronizing
    while (Buffer_Offset+4<=Buffer_Size && (Buffer[Buffer_Offset  ]!=0x06
                                         || Buffer[Buffer_Offset+1]!=0x0E
                                         || Buffer[Buffer_Offset+2]!=0x2B
                                         || Buffer[Buffer_Offset+3]!=0x34))
    {
        Buffer_Offset++;
        while (Buffer_Offset<Buffer_Size && Buffer[Buffer_Offset]!=0x06)
            Buffer_Offset++;
    }


    while (Buffer_Offset+4<=Buffer_Size
        && CC4(Buffer+Buffer_Offset)!=0x060E2B34)
        Buffer_Offset++;

    //Parsing last bytes if needed
    if (Buffer_Offset+4>Buffer_Size)
    {
        if (Buffer_Offset+3==Buffer_Size && CC3(Buffer+Buffer_Offset)!=0x060E2B)
            Buffer_Offset++;
        if (Buffer_Offset+2==Buffer_Size && CC2(Buffer+Buffer_Offset)!=0x060E)
            Buffer_Offset++;
        if (Buffer_Offset+1==Buffer_Size && CC1(Buffer+Buffer_Offset)!=0x06)
            Buffer_Offset++;
        return false;
    }

    if (IsSub && !Status[IsAccepted])
        Accept();

    //Synched is OK
    return true;
}

//---------------------------------------------------------------------------
bool File_Mxf::Synched_Test()
{
    //Must have enough buffer for having header
    if (Buffer_Offset+16>Buffer_Size)
        return false;

    //Quick test of synchro
    if (CC4(Buffer+Buffer_Offset)!=0x060E2B34)
    {
        Synched=false;
        if (!Status[IsAccepted])
            Trusted_IsNot("Sync"); //If there is an unsynch before the parser accepts the stream, very high risk the that the file is not MXF
    }
    else if (!Status[IsAccepted])
    {
        if (Synched_Count>=8)
            Accept();
        else
            Synched_Count++;
    }

    //Trace config
    #if MEDIAINFO_TRACE
        if (Synched)
        {
            int64u Compare=CC8(Buffer+Buffer_Offset+ 4);
            if (Compare==0x010201010D010301LL //Raw stream
             || (Compare==0x0101010203010210LL && CC1(Buffer+Buffer_Offset+12)==0x01) //Filler
             || (Compare==0x020501010D010301LL && CC3(Buffer+Buffer_Offset+12)==0x040101) //SDTI Package Metadata Pack
             || (Compare==0x024301010D010301LL && CC3(Buffer+Buffer_Offset+12)==0x040102) //SDTI Package Metadata Set
             || (Compare==0x025301010D010301LL && CC3(Buffer+Buffer_Offset+12)==0x140201)) //System Scheme 1
            {
                Trace_Layers_Update(8); //Stream
            }
            else
            {
                Trace_Layers_Update(0); //Container1
            }
        }
    #endif //MEDIAINFO_TRACE

    //We continue
    return true;
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mxf::Header_Begin()
{
    while (Buffer_End)
    {
        #if MEDIAINFO_DEMUX
            //Searching single descriptor if it is the only valid descriptor
            descriptors::iterator SingleDescriptor=Descriptors.end();
            for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
                if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max)
                {
                    if (SingleDescriptor!=Descriptors.end())
                    {
                        SingleDescriptor=Descriptors.end();
                        break; // 2 or more descriptors, can not be used
                    }
                    SingleDescriptor=SingleDescriptor_Temp;
                }

            if (Demux_UnpacketizeContainer && SingleDescriptor!=Descriptors.end() && SingleDescriptor->second.ByteRate!=(int32u)-1 && SingleDescriptor->second.BlockAlign && SingleDescriptor->second.BlockAlign!=(int16u)-1  && SingleDescriptor->second.SampleRate)
            {
                float64 BytesPerFrame=((float64)SingleDescriptor->second.ByteRate)/SingleDescriptor->second.SampleRate;
                int64u FramesAlreadyParsed=float64_int64s(((float64)(File_Offset+Buffer_Offset-Buffer_Begin))/BytesPerFrame);
                Element_Size=float64_int64s(SingleDescriptor->second.ByteRate/SingleDescriptor->second.SampleRate*(FramesAlreadyParsed+1));
                #if MEDIAINFO_DEMUX
                    Element_Size+=DemuxedElementSize_AddedToFirstFrame;
                #endif //MEDIAINFO_DEMUX
                Element_Size/=SingleDescriptor->second.BlockAlign;
                Element_Size*=SingleDescriptor->second.BlockAlign;
                Element_Size-=File_Offset+Buffer_Offset-Buffer_Begin;
                if (Config->File_IsGrowing && Element_Size && File_Offset+Buffer_Offset+Element_Size>Buffer_End)
                    return false; //Waiting for more data
                while (Element_Size && File_Offset+Buffer_Offset+Element_Size>Buffer_End)
                    Element_Size-=SingleDescriptor->second.BlockAlign;
                if (Element_Size==0)
                    Element_Size=Buffer_End-(File_Offset+Buffer_Offset);
                if (Buffer_Offset+Element_Size>Buffer_Size)
                    return false;

                #if MEDIAINFO_DEMUX
                    if (!DemuxedSampleCount_Total && Config->Demux_Offset_DTS!=(int64u)-1 && Config->File_EditRate)
                    {
                        //Need to sync to a rounded value compared to the whole stream (including previous files)
                        float64 TimeStamp=((float64)Config->Demux_Offset_DTS)/1000000000;
                        int64u FramesBeForeThisFileMinusOne=(int64u)(TimeStamp*SingleDescriptor->second.SampleRate);
                        if ((((float64)FramesBeForeThisFileMinusOne)/SingleDescriptor->second.SampleRate)!=TimeStamp)
                        {
                            float64 Delta=(((float64)FramesBeForeThisFileMinusOne+1)/SingleDescriptor->second.SampleRate)-TimeStamp;
                            DemuxedSampleCount_AddedToFirstFrame=float64_int64s(Delta*Config->File_EditRate);
                            DemuxedElementSize_AddedToFirstFrame=DemuxedSampleCount_AddedToFirstFrame*SingleDescriptor->second.BlockAlign;
                            Element_Size+=DemuxedElementSize_AddedToFirstFrame;
                        }
                    }
                    if (DemuxedSampleCount_Total!=(int64u)-1 && Config->File_IgnoreEditsAfter!=(int64u)-1)
                    {
                        DemuxedSampleCount_Current=Element_Size/SingleDescriptor->second.BlockAlign;
                        int64u RealSampleRate=SingleDescriptor->second.Infos["SamplingRate"].To_int64u();
                        int64u IgnoreSamplesAfter;
                        if (RealSampleRate==Config->File_EditRate)
                            IgnoreSamplesAfter=Config->File_IgnoreEditsAfter;
                        else
                            IgnoreSamplesAfter=float64_int64s(((float64)Config->File_IgnoreEditsAfter)/Config->File_EditRate*RealSampleRate);
                        if (DemuxedSampleCount_Total+DemuxedSampleCount_Current>IgnoreSamplesAfter)
                        {
                            DemuxedSampleCount_Current=Config->File_IgnoreEditsAfter-DemuxedSampleCount_Total;
                            Element_Size=DemuxedSampleCount_Current*SingleDescriptor->second.BlockAlign;
                        }
                        if (DemuxedSampleCount_Total+DemuxedSampleCount_Current+1==IgnoreSamplesAfter)
                            DemuxedSampleCount_Current++; //Avoid rounding issues (sometimes it remains only 1 sample)
                    }
                #endif //MEDIAINFO_DEMUX
            }
            else if (Demux_UnpacketizeContainer && !IndexTables.empty() && IndexTables[0].EditUnitByteCount)
            {
                //Calculating the byte count not included in seek information (partition, index...)
                int64u StreamOffset_Offset;
                if (!Partitions.empty())
                {
                    while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<File_Offset+Buffer_Offset-Header_Size)
                        Partitions_Pos++;
                    if (Partitions_Pos && (Partitions_Pos==Partitions.size() || Partitions[Partitions_Pos].StreamOffset!=File_Offset+Buffer_Offset-Header_Size))
                        Partitions_Pos--; //This is the previous item
                    StreamOffset_Offset=Partitions[Partitions_Pos].StreamOffset-Partitions[Partitions_Pos].BodyOffset+Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
                }
                else
                    StreamOffset_Offset=0;

                int64u Position=0;
                for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                {
                    if (IndexTables[Pos].IndexDuration && File_Offset+Buffer_Offset>=StreamOffset_Offset+Buffer_Header_Size+Position+IndexTables[Pos].IndexDuration*IndexTables[Pos].EditUnitByteCount) //Considering IndexDuration==0 as unlimited
                        Position+=IndexTables[Pos].EditUnitByteCount*IndexTables[Pos].IndexDuration;
                    else
                    {
                        Element_Size=IndexTables[Pos].EditUnitByteCount;
                        if (File_Offset+Buffer_Offset+Element_Size>Buffer_End)
                        {
                            Element_Size=Buffer_End-(File_Offset+Buffer_Offset);
                            break; //There is a problem
                        }

                        if (Buffer_Offset+Element_Size>Buffer_Size)
                        {
                            //Hints
                            if (File_Buffer_Size_Hint_Pointer)
                            {
                                size_t Buffer_Size_Target=(size_t)(Buffer_Offset+Element_Size-Buffer_Size+24); //+24 for next packet header
                                if (Buffer_Size_Target<128*1024)
                                    Buffer_Size_Target=128*1024;
                                //if ((*File_Buffer_Size_Hint_Pointer)<Buffer_Size_Target)
                                    (*File_Buffer_Size_Hint_Pointer)=Buffer_Size_Target;
                            }

                            return false;
                        }
                        break;
                    }
                }

                if (Buffer_Offset+(size_t)Element_Size>Buffer_Size)
                    Element_Size=Buffer_Size-Buffer_Offset; //There is a problem
            }
            else if (Demux_UnpacketizeContainer && !IndexTables.empty() && !IndexTables[0].Entries.empty())
            {
                //Calculating the byte count not included in seek information (partition, index...)
                int64u StreamOffset_Offset;
                if (!Partitions.empty())
                {
                    while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<File_Offset+Buffer_Offset-Header_Size)
                        Partitions_Pos++;
                    if (Partitions_Pos && (Partitions_Pos==Partitions.size() || Partitions[Partitions_Pos].StreamOffset!=File_Offset+Buffer_Offset-Header_Size))
                        Partitions_Pos--; //This is the previous item
                    StreamOffset_Offset=Partitions[Partitions_Pos].StreamOffset-Partitions[Partitions_Pos].BodyOffset+Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
                }
                else
                    StreamOffset_Offset=0;

                int64u StreamOffset=File_Offset+Buffer_Offset-StreamOffset_Offset;
                for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                {
                    //Searching the right index
                    if (!IndexTables[Pos].Entries.empty() && StreamOffset>=IndexTables[Pos].Entries[0].StreamOffset+(IndexTables[Pos].IndexStartPosition)*SDTI_SizePerFrame && (Pos+1>=IndexTables.size() || StreamOffset<IndexTables[Pos+1].Entries[0].StreamOffset+(IndexTables[Pos+1].IndexStartPosition)*SDTI_SizePerFrame))
                    {
                        //Searching the frame pos
                        for (size_t EntryPos=0; EntryPos<IndexTables[Pos].Entries.size(); EntryPos++)
                        {
                            //Testing coherency
                            int64u Entry0_StreamOffset=0; //For coherency checking
                            int64u Entry_StreamOffset=IndexTables[Pos].Entries[EntryPos].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos)*SDTI_SizePerFrame;
                            int64u Entry1_StreamOffset=File_Size; //For coherency checking
                            if (EntryPos==0 && Pos && !IndexTables[Pos-1].Entries.empty())
                                Entry0_StreamOffset=IndexTables[Pos-1].Entries[IndexTables[Pos-1].Entries.size()-1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos-1)*SDTI_SizePerFrame;
                            else if (EntryPos)
                                Entry0_StreamOffset=IndexTables[Pos].Entries[EntryPos-1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos-1)*SDTI_SizePerFrame;
                            if (EntryPos+1<IndexTables[Pos].Entries.size())
                                Entry1_StreamOffset=IndexTables[Pos].Entries[EntryPos+1].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos+1)*SDTI_SizePerFrame;
                            else if (Pos+1<IndexTables.size() && !IndexTables[Pos+1].Entries.empty())
                                Entry1_StreamOffset=IndexTables[Pos+1].Entries[0].StreamOffset+(IndexTables[Pos].IndexStartPosition+EntryPos+1)*SDTI_SizePerFrame;

                            if (Entry0_StreamOffset>Entry_StreamOffset || Entry_StreamOffset>Entry1_StreamOffset)
                                break; //Problem

                            if (StreamOffset>=Entry_StreamOffset && StreamOffset<Entry1_StreamOffset)
                            {
                                Element_Size=StreamOffset_Offset+Buffer_Header_Size+Entry1_StreamOffset-(File_Offset+Buffer_Offset);
                                if (File_Offset+Buffer_Offset+Element_Size>Buffer_End)
                                {
                                    Element_Size=Buffer_End-(File_Offset+Buffer_Offset);
                                    break; //There is a problem
                                }

                                if (Buffer_Offset+Element_Size>Buffer_Size)
                                {
                                    //Hints
                                    if (File_Buffer_Size_Hint_Pointer)
                                    {
                                        size_t Buffer_Size_Target=(size_t)(Buffer_Offset+Element_Size-Buffer_Size+24); //+24 for next packet header
                                        if (Buffer_Size_Target<128*1024)
                                            Buffer_Size_Target=128*1024;
                                        //if ((*File_Buffer_Size_Hint_Pointer)<Buffer_Size_Target)
                                            (*File_Buffer_Size_Hint_Pointer)=Buffer_Size_Target;
                                    }

                                    return false;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else
        #endif //MEDIAINFO_DEMUX
            if (File_Offset+Buffer_Size<=Buffer_End)
                Element_Size=Buffer_Size-Buffer_Offset; //All the buffer is used
            else
                Element_Size=Buffer_End-(File_Offset+Buffer_Offset);

        Element_Begin0();
        Data_Parse();
        Buffer_Offset+=(size_t)Element_Size;
        Element_Size-=Element_Offset;
        Element_Offset=0;
        Element_End0();

        if (Buffer_End && (File_Offset+Buffer_Offset+Element_Size>=Buffer_End || File_GoTo!=(int64u)-1) )
        {
            Buffer_Begin=(int64u)-1;
            Buffer_End=0;
            Buffer_End_Unlimited=false;
            Buffer_Header_Size=0;
            MustSynchronize=true;
        }

        if (Buffer_Offset>=Buffer_Size)
            return false;

        #if MEDIAINFO_DEMUX
            if (Config->Demux_EventWasSent)
                return false;
        #endif //MEDIAINFO_DEMUX
    }

    return true;
}

//---------------------------------------------------------------------------
void File_Mxf::Header_Parse()
{
    //Parsing
    int64u Length;
    Get_UL(Code,                                                "Code", NULL);
    Get_BER(Length,                                             "Length");
    if (Element_IsWaitingForMoreData())
        return;

    if (Length==0
     && (((int32u)Code.hi)>>8)==0x010201
     && Retrieve(Stream_General, 0, General_Format_Settings).find(__T(" / Incomplete"))!=string::npos
     )
    {
        if (Buffer_Offset+Element_Offset+4>Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }

        if (BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset)!=0x060E2B34)
        {
            Buffer_End_Unlimited=true;
            Length=File_Size-(File_Offset+Buffer_Offset+Element_Offset);
        }
    }

    if (Config->File_IsGrowing && File_Offset+Buffer_Offset+Element_Offset+Length>File_Size)
    {
        Element_WaitForMoreData();
        return;
    }

    if (Length==0 && Essences.empty() && Retrieve(Stream_General, 0, General_Format_Settings).find(__T(" / Incomplete"))!=string::npos)
    {
        if (Buffer_Offset+Element_Offset+4>Buffer_Size)
        {
            Element_WaitForMoreData();
            return;
        }

        if (BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset)!=0x060E2B34)
        {
            Buffer_End_Unlimited=true;
            Length=File_Size-(File_Offset+Buffer_Offset+Element_Offset);
        }
    }

    if (Config->File_IsGrowing && File_Offset+Buffer_Offset+Element_Offset+Length>File_Size)
    {
        Element_WaitForMoreData();
        return;
    }

    //Filling
    int32u Code_Compare3=Code.lo>>32;
    bool IsFiller=(Code.hi>>24)==0x060E2B3401
              && (Code.lo==Elements::FillerData || Code.lo==Elements::TerminatingFillerData);
    if (IsFiller)
        DataMustAlwaysBeComplete=false;
    if (Config->ParseSpeed<0
     && !IsParsingEnd
     && ((!Partitions_Pos
     && !Partitions.empty()
     && !Partitions_IsCalculatingHeaderByteCount
     && File_Offset+Buffer_Offset+(IsFiller?(Element_Offset+Length):0)>=Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount)
     || ((Code.hi>> 8)==0x060E2B34010201LL)
     || ((Code.hi>>24)==0x060E2B34020501LL
      && (Code.lo>>16)==(GroupsPacks::BodyPartitionOpenIncomplete>>16))))
    {
        if (Config->ParseSpeed<=-1)
        {
            Finish();
            return;
        }
        IsParsingEnd=true;
        if (PartitionMetadata_FooterPartition!=(int64u)-1 && PartitionMetadata_FooterPartition>File_Offset+Buffer_Offset+(size_t)Element_Size)
        {
            if (PartitionMetadata_FooterPartition+17<=File_Size)
            {
                GoTo(PartitionMetadata_FooterPartition);
                IsCheckingFooterPartitionAddress=true;
            }
            else
            {
                GoToFromEnd(4); //For random access table
                FooterPartitionAddress_Jumped=true;
            }
        }
        else
        {
            GoToFromEnd(4); //For random access table
            FooterPartitionAddress_Jumped=true;
        }
        Open_Buffer_Unsynch();
        return;
    }
    if (Partitions_IsCalculatingHeaderByteCount)
    {
        if (!IsFiller)
        {
            Partitions_IsCalculatingHeaderByteCount=false;
            if (Partitions_Pos<Partitions.size())
                Partitions[Partitions_Pos].PartitionPackByteCount=File_Offset+Buffer_Offset-Partitions[Partitions_Pos].StreamOffset;
        }
    }
    if (Partitions_IsCalculatingSdtiByteCount)
    {
        if (!(((Code.hi>>24)==0x060E2B3402LL //Independent of Category
            && (Code.lo>>16)==(Groups::SDTISystemMetadataPack>>16))
          || (((Code.hi>>24)==0x060E2B3401LL
            && (Code.lo==Elements::FillerData || Code.lo==Elements::TerminatingFillerData)))))
        {
            if (Partitions_Pos<Partitions.size() && !SDTI_IsInIndexStreamOffset)
                SDTI_SizePerFrame=File_Offset+Buffer_Offset-(Partitions[Partitions_Pos].StreamOffset+Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount);
            Partitions_IsCalculatingSdtiByteCount=false;
        }
    }

    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
        if (!Demux_HeaderParsed && !Partitions.empty() && Partitions[Partitions.size()-1].StreamOffset+Partitions[Partitions.size()-1].PartitionPackByteCount+Partitions[Partitions.size()-1].HeaderByteCount+Partitions[Partitions.size()-1].IndexByteCount==File_Offset+Buffer_Offset)
        {
            Demux_HeaderParsed=true;

            //Testing locators
            Locators_CleanUp();

            if (Config->File_IgnoreEditsBefore && !Config->File_IsDetectingDuration_Get() && Config->Event_CallBackFunction_IsSet()) //Only if demux packet may be requested
                Open_Buffer_Seek(3, 0, (int64u)-1); //Forcing seek to Config->File_IgnoreEditsBefore
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
            {
                if (Locators.empty())
                {
                    Config->Demux_EventWasSent=true; //First set is to indicate the user that header is parsed
                    return;
                }
            }
        }
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX

    if (Buffer_Offset+Element_Offset+Length>(size_t)-1 || Buffer_Offset+(size_t)(Element_Offset+Length)>Buffer_Size) //Not complete
    {
        if (Length>File_Size/2) //Divided by 2 for testing if this is a big chunk = Clip based and not frames.
        {
            //Calculating the byte count not included in seek information (partition, index...)
            int64u StreamOffset_Offset;
            if (!Partitions.empty())
            {
                while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<File_Offset+Buffer_Offset-Header_Size)
                    Partitions_Pos++;
                if (Partitions_Pos && (Partitions_Pos==Partitions.size() || Partitions[Partitions_Pos].StreamOffset!=File_Offset+Buffer_Offset-Header_Size))
                    Partitions_Pos--; //This is the previous item
                StreamOffset_Offset=Partitions[Partitions_Pos].StreamOffset-Partitions[Partitions_Pos].BodyOffset+Partitions[Partitions_Pos].PartitionPackByteCount+Partitions[Partitions_Pos].HeaderByteCount+Partitions[Partitions_Pos].IndexByteCount;
            }
            else
                StreamOffset_Offset=0;

            if (StreamOffset_Offset<=File_Offset+Buffer_Offset
             && !Partitions_IsFooter
             && !((Code.hi>>24)==0x060E2B3402
               && Code.lo==GroupsPacks::HeaderPartitionOpenIncomplete)
             && !((Code.hi>>24)==0x060E2B3402
               && Code.lo==Groups::IndexTableSegment))
            {
                Buffer_Begin=File_Offset+Buffer_Offset+Element_Offset;
                Buffer_End=Buffer_Begin+Length;
                Buffer_Header_Size=Element_Offset;
                MustSynchronize=false;
                Length=0;
                #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
                Clip_Begin=Buffer_Begin;
                Clip_End=Buffer_End;
                Clip_Header_Size=Buffer_Header_Size;
                Clip_Code=Code;
                #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK
            }
        }

        if (Buffer_Begin==(int64u)-1)
        {
            if (Length<=File_Size/2) //Divided by 2 for testing if this is a big chunk = Clip based and not frames.))
            {
                if (File_Buffer_Size_Hint_Pointer)
                {
                    int64u Buffer_Size_Target=(size_t)(Buffer_Offset+Element_Offset+Length-Buffer_Size+24); //+24 for next packet header

                    if (Buffer_Size_Target<128*1024)
                        Buffer_Size_Target=128*1024;
                    //if ((*File_Buffer_Size_Hint_Pointer)<Buffer_Size_Target)
                        (*File_Buffer_Size_Hint_Pointer)=(size_t)Buffer_Size_Target;
                }


                Element_WaitForMoreData();
                return;
            }
        }
    }

    #if MEDIAINFO_TRACE
        Header_Fill_Code(0, Ztring::ToZtring(Code.hi, 16)+Ztring::ToZtring(Code.lo, 16));
    #else //MEDIAINFO_TRACE
        Header_Fill_Code(0);
    #endif //MEDIAINFO_TRACE
    Header_Fill_Size(Element_Offset+Length);
}

//---------------------------------------------------------------------------
namespace Type
{
    int32u Elements = 1;
    int32u Groups = 2;
}

void File_Mxf::Data_Parse()
{
    //Clearing
    InstanceUID=0;
    if (!Essences_FirstEssence_Parsed)
    {
        Frame_Count=(int64u)-1;
        Frame_Count_NotParsedIncluded=(int64u)-1;
        FrameInfo=frame_info();
    }

    if (!Element_IsComplete_Get() && !Buffer_End) {
        Element_WaitForMoreData();
        return;
    }

    #if MEDIAINFO_TRACE
    if (Trace_Activated) {
        auto Name = Mxf_Param_Info((int32u)Code.hi, Code.lo);
        Element_Name(Name ? Name : Ztring().From_UUID(Code).To_UTF8().c_str());
    }
    #endif //MEDIAINFO_TRACE

    //Parsing
    int32u Code_Compare3=Code.lo>>32;
    int32u Code_Compare4=(int32u)Code.lo;
    typedef void (File_Mxf::* method_name)();

    auto ManageElement = [&](method_name MethodName) {
        if (!Element_IsComplete_Get() && Buffer_End) {
            Skip_XX(Element_Size - Element_Offset, "Data");
            return;
        }
        (this->*MethodName)();
    };
    #undef ELEMENT
    #define ELEMENT(_ELEMENT) \
    else if (Code.lo==Elements::_ELEMENT) \
    { \
        ManageElement(&File_Mxf::_ELEMENT); \
    } \

    #undef GROUPSTATIC
    #define GROUPSTATIC(_ELEMENT) \
    else if (Code.lo==GroupsPacks::_ELEMENT) \
    { \
        ManageGroup(&File_Mxf::_ELEMENT); \
    } \

    auto ManageGroup = [&](method_name MethodName) {
        switch ((int8u)(Code.hi>>16))         {
        case 0x05: (this->*MethodName)(); break;
        case 0x43:
        case 0x53:
        case 0x63:
            while (Element_Offset < Element_Size)
            {
                Element_Begin0();
                Element_Begin1(                                 "Header");
                switch (((int8u)(Code.hi>>16)) & 0xF0) {
                case 0x40:
                {
                    int8u Code1;
                    Get_B1(Code1,                               "Code");
                    Code2 = Code1;
                }
                break;
                default:
                    Get_B2(Code2,                               "Code");
                }
                switch (((int8u)(Code.hi>>16)) & 0xF0) {
                case 0x60:
                {
                    int32u Code4;
                    Get_B4(Code4,                               "Length");
                    if (Code4 >> 16) {
                        Skip_XX(Element_Size - Element_Offset,  "(Unsupported)");
                    }
                    Code2 = (int16u)Code4;
                }
                break;
                default:
                    Get_B2(Length2,                             "Length");
                }
                Element_End0();
                #if MEDIAINFO_TRACE
                if (Trace_Activated) {
                    std::map<int16u, int128u>::iterator Primer_Value = Primer_Values.find(Code2);
                    if (Primer_Value != Primer_Values.end()) {
                        auto Name = Mxf_Param_Info((int32u)Primer_Value->second.hi, Primer_Value->second.lo);
                        Element_Name(Name ? Name : Ztring().From_UUID(Primer_Value->second).To_UTF8().c_str());
                    }
                    else {
                        Element_Name(Ztring().From_CC2(Code2).To_UTF8().c_str());
                    }
                }
                #endif //MEDIAINFO_TRACE
                auto Length2_Max = Element_Size - Element_Offset;
                if (Length2 > Length2_Max) {
                    Length2 = Length2_Max;
                }
                auto Element_Size_Save = Element_Size;
                Element_Size = Element_Offset + Length2;
                (this->*MethodName)();
                if (Element_Offset < Element_Size) {
                    Skip_XX(Element_Size - Element_Offset,      "Unknown");
                }
                Element_Size = Element_Size_Save;
                Element_End0();
            }
            break;
        default: Skip_XX(Element_Size - Element_Offset,         "Unknown");
        }
        };
    #undef GROUP
    #define GROUP(_ELEMENT) \
    else if (Code.lo==Groups::_ELEMENT) \
    { \
        ManageGroup(&File_Mxf::_ELEMENT); \
    } \

    /*
    int8u Registry = (int8u)(Code.hi >> 16);
    if ((Code.hi>>8)==0x060E2B34010201LL && Code.lo==Elements::Dolby_0502010001013) { //Exception, TODO: to be confirmed
        Code.hi=0x060E2B340101010CLL; // MXFGenericStreamDataElementKey hi
        Code.lo=Elements::MXFGenericStreamDataElementKey|0x0901000000LL; // With Big-Endian or is a byte-streamm
        int8u Registry=0x01;
    }
    */

    //Parsing
    switch (Code.hi >> 8) {
    case 0x060E2B34010201LL: {
    {
        auto Name = Mxf_Param_Info((int32u)Code.hi, Code.lo & 0xFFFFFFFFFF00FF00);
        Element_Name(Name ? Name : "Unknown stream");

        //Config
        if (!Essences_FirstEssence_Parsed)
        {
            Streams_Finish_Preface_ForTimeCode(Preface_Current); //Configuring DTS_Delay

            #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
            //Searching single descriptor if it is the only valid descriptor
            descriptors::iterator SingleDescriptor=Descriptors.end();
            for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
                if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max)
                {
                    if (SingleDescriptor!=Descriptors.end())
                    {
                        SingleDescriptor=Descriptors.end();
                        break; // 2 or more descriptors, can not be used
                    }
                    SingleDescriptor=SingleDescriptor_Temp;
                }

            if (SingleDescriptor!=Descriptors.end() && SingleDescriptor->second.StreamKind==Stream_Audio)
            {
                //Configuring bitrate is not available in descriptor
                if (SingleDescriptor->second.ByteRate==(int32u)-1)
                {
                    std::map<std::string, Ztring>::const_iterator i=Descriptors.begin()->second.Infos.find("SamplingRate");
                    if (i != Descriptors.begin()->second.Infos.end())
                    {
                        int32u SamplingRate=i->second.To_int32u();

                        if (SingleDescriptor->second.BlockAlign != (int16u)-1)
                            SingleDescriptor->second.ByteRate = SamplingRate*SingleDescriptor->second.BlockAlign;
                        else if (SingleDescriptor->second.QuantizationBits != (int8u)-1)
                            SingleDescriptor->second.ByteRate = SamplingRate*SingleDescriptor->second.QuantizationBits / 8;
                    }
                }
            }

            for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
            {
                //Configuring EditRate if needed (e.g. audio at 48000 Hz)
                if (Descriptor->second.SampleRate>1000)
                {
                    float64 EditRate_FromTrack=DBL_MAX;
                    for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                        if (Track->second.EditRate && EditRate_FromTrack>Track->second.EditRate)
                            EditRate_FromTrack=Track->second.EditRate;
                    if (EditRate_FromTrack>1000)
                        EditRate_FromTrack=Demux_Rate; //Default value;
                    Descriptor->second.SampleRate=EditRate_FromTrack;
                    DemuxedSampleCount_Total=Config->File_IgnoreEditsBefore;
                    for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                        if (Track->second.EditRate>EditRate_FromTrack)
                        {
                            Track->second.EditRate_Real=Track->second.EditRate;
                            Track->second.EditRate=EditRate_FromTrack;
                        }
                }
            }
            #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK

            Frame_Count=0;
            Frame_Count_NotParsedIncluded=0;
            FrameInfo=frame_info();
            FrameInfo.DTS=0;
            Essences_FirstEssence_Parsed=true;
        }

        if (IsParsingEnd)
        {
            NextRandomIndexPack();
            return;
        }

        essences::iterator Essence=Essences.find(Code_Compare4);
        if (Essence==Essences.end())
        {
            Essences[Code_Compare4];
            Essence=Essences.find(Code_Compare4);
        }

        #if MEDIAINFO_TRACE
            if (Trace_Activated)
            {
                if (Essence->second.Trace_Count<MaxCountSameElementInTrace)
                    Essence->second.Trace_Count++;
                else
                    Element_Set_Remove_Children_IfNoErrors();
            }
        #endif // MEDIAINFO_TRACE

        if (Essence->second.Parsers.empty())
        {
            //Searching single descriptor if it is the only valid descriptor
            descriptors::iterator SingleDescriptor=Descriptors.end();
            for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
                if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max || SingleDescriptor_Temp->second.LinkedTrackID!=(int32u)-1)
                {
                    if (SingleDescriptor!=Descriptors.end())
                    {
                        SingleDescriptor=Descriptors.end();
                        break; // 2 or more descriptors, can not be used
                    }
                    SingleDescriptor=SingleDescriptor_Temp;
                }

            //Format_Settings_Wrapping
            if (SingleDescriptor!=Descriptors.end())
            {
                std::map<std::string, Ztring>::iterator i=SingleDescriptor->second.Infos.find("Format_Settings_Wrapping");
                if ((i==SingleDescriptor->second.Infos.end() || i->second.empty()) && (Buffer_End?(Buffer_End-Buffer_Begin):Element_Size)>File_Size/2) //Divided by 2 for testing if this is a big chunk = Clip based and not frames.
                {
                    if (i==SingleDescriptor->second.Infos.end())
                        SingleDescriptor->second.Infos["Format_Settings_Wrapping"]=__T("Clip");
                    else
                        i->second=__T("Clip"); //By default, not sure about it, should be from descriptor
                }
            }

            //Searching the corresponding Track (for TrackID)
            if (!Essence->second.TrackID_WasLookedFor)
            {
                for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                    if (Track->second.TrackNumber==Code_Compare4)
                        Essence->second.TrackID=Track->second.TrackID;
                #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
                    if (Essence->second.TrackID==(int32u)-1 && !Duration_Detected && !Config->File_IsDetectingDuration_Get())
                    {
                        DetectDuration(); //In one file (*-009.mxf), the TrackNumber is known only at the end of the file (Open and incomplete header/footer)
                        for (tracks::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
                            if (Track->second.TrackNumber==Code_Compare4)
                                Essence->second.TrackID=Track->second.TrackID;
                    }
                #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK

                // Fallback in case TrackID is not detected, forcing TrackID and TrackNumber
                if (Essence->second.TrackID==(int32u)-1 && SingleDescriptor!=Descriptors.end())
                {
                    Essence->second.TrackID=SingleDescriptor->second.LinkedTrackID;

                    prefaces::iterator Preface=Prefaces.find(Preface_Current);
                    if (Preface!=Prefaces.end())
                    {
                        contentstorages::iterator ContentStorage=ContentStorages.find(Preface->second.ContentStorage);
                        if (ContentStorage!=ContentStorages.end())
                        {
                            for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
                            {
                                packages::iterator Package=Packages.find(ContentStorage->second.Packages[Pos]);
                                if (Package!=Packages.end() && Package->second.IsSourcePackage)
                                {
                                    for (size_t Pos=0; Pos<Package->second.Tracks.size(); Pos++)
                                    {
                                        tracks::iterator Track=Tracks.find(Package->second.Tracks[Pos]);
                                        if (Track!=Tracks.end())
                                        {
                                            if (Track->second.TrackNumber==0 && Track->second.TrackID==Essence->second.TrackID)
                                            {
                                                Track->second.TrackNumber=Essence->first;
                                                Essence->second.Track_Number_IsMappedToTrack=true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Essence->second.TrackID_WasLookedFor=true;
            }

            //Searching the corresponding Descriptor
            bool DescriptorFound=false;
            for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                if (Descriptor==SingleDescriptor || (Descriptor->second.LinkedTrackID==Essence->second.TrackID && Descriptor->second.LinkedTrackID!=(int32u)-1))
                {
                    DescriptorFound=true;

                    Essence->second.StreamPos_Initial=Essence->second.StreamPos=Code_Compare4&0x000000FF;

                    if (Descriptor->second.StreamKind==Stream_Audio)
                    {
                        std::map<std::string, Ztring>::iterator i=Descriptor->second.Infos.find("Format_Settings_Endianness");
                        if (i==Descriptor->second.Infos.end())
                        {
                            Ztring Format;
                            Format.From_UTF8(Mxf_EssenceCompression(Descriptor->second.EssenceCompression));
                            if (Format.empty())
                                Format.From_UTF8(Mxf_EssenceContainer(Descriptor->second.EssenceContainer));
                            if (Format.find(__T("PCM"))==0)
                                Descriptor->second.Infos["Format_Settings_Endianness"]=__T("Little");
                        }
                    }

                    ChooseParser__FromCodingScheme(Essence, Descriptor); //Searching by the descriptor
                    if (Essence->second.Parsers.empty())
                        ChooseParser__FromEssence(Essence, Descriptor); //Searching by the track identifier
                    if (Essence->second.Parsers.size()==1 && Descriptor->second.Infos["Format_Settings_Wrapping"].rfind(__T("Frame"), 0)==0 || Essence->second.Infos["Format_Settings_Wrapping"].rfind(__T("Frame"), 0)==0) //TODO: Parsers.size()==1 is for avoiding Dolby E issues, it should be fixed instead
                        Essence->second.Parsers[0]->FrameIsAlwaysComplete=true;

                    #ifdef MEDIAINFO_DEMUX
                        if (Ztring().From_UTF8(Mxf_EssenceContainer(Descriptor->second.EssenceContainer))==__T("AVC"))
                            Essence->second.ShouldCheckAvcHeaders=File_Avc::AVC_Intra_CodecID_FromMeta(Descriptor->second.Width, Descriptor->second.Height, Descriptor->second.Is_Interlaced()?2:1, 1, (int32u)(float64_int64s(Descriptor->second.SampleRate)), 0);
                    #endif //MEDIAINFO_DEMUX
                    break;
                }
            if (!DescriptorFound)
                Streams_Count++; //This stream was not yet counted

            //Searching by the track identifier
            if (Essence->second.Parsers.empty())
            {
                ChooseParser__FromEssence(Essence, Descriptors.end());
                if (Essence->second.Parsers.size()==1 && Essence->second.Infos["Format_Settings_Wrapping"].rfind(__T("Frame"), 0)==0) //TODO: Parsers.size()==1 is for avoiding Dolby E issues, it should be fixed instead
                    Essence->second.Parsers[0]->FrameIsAlwaysComplete=true;
            }

            //Check of Essence used as a reference for frame count
            if (Essences_UsedForFrameCount==(int32u)-1)
                Essences_UsedForFrameCount=Essence->first;

            //Demux
            #if MEDIAINFO_DEMUX
                //Configuration
                if (!IsSub) //Updating for MXF only if MXF is not embedded in another container
                {
                    Essence->second.Frame_Count_NotParsedIncluded=Frame_Count_NotParsedIncluded;
                    if (Essence->second.Frame_Count_NotParsedIncluded!=(int64u)-1 && Essence->second.Frame_Count_NotParsedIncluded && Essence->first!=Essences_UsedForFrameCount)
                        Essence->second.Frame_Count_NotParsedIncluded--; //Info is from the first essence parsed, and 1 frame is already parsed
                    Essence->second.FrameInfo.DTS=FrameInfo.DTS;
                    if (Essence->second.FrameInfo.DTS!=(int64u)-1 && FrameInfo.DUR!=(int64u)-1 && Frame_Count_NotParsedIncluded && Essence->first!=Essences_UsedForFrameCount)
                        Essence->second.FrameInfo.DTS-=FrameInfo.DUR; //Info is from the first essence parsed, and 1 frame is already parsed
                    if (!Tracks.empty() && Tracks.begin()->second.EditRate) //TODO: use the corresponding track instead of the first one
                        Essence->second.FrameInfo.DUR=float64_int64s(1000000000/Tracks.begin()->second.EditRate);
                    else if (!IndexTables.empty() && IndexTables[0].IndexEditRate)
                        Essence->second.FrameInfo.DUR=float64_int64s(1000000000/IndexTables[0].IndexEditRate);
                    #if MEDIAINFO_DEMUX
                        if (Buffer_End && Demux_UnpacketizeContainer && Essences.size()==1 && !Essences.begin()->second.Parsers.empty() && !(*(Essences.begin()->second.Parsers.begin()))->Demux_UnpacketizeContainer)
                            for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
                            {
                                (*Parser)->Demux_Level=2; //Container
                                (*Parser)->Demux_UnpacketizeContainer=true;
                            }
                    #endif //MEDIAINFO_DEMUX
                }
                if (Essence->second.TrackID!=(int32u)-1)
                    Element_Code=Essence->second.TrackID;
                else
                    Element_Code=Code.lo;
            #endif //MEDIAINFO_DEMUX

            if (Essence->second.Parsers.empty())
            {
                if (Streams_Count>0)
                    Streams_Count--;
            }
            else
            {
                Element_Code=Essence->second.TrackID;
                for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
                {
                    Open_Buffer_Init(*Parser);
                    if ((*Parser)->Status[IsFinished])
                        if (Streams_Count>0)
                            Streams_Count--;
                }
            }

            if ((Code_Compare4&0x000000FF)==0x00000000)
                StreamPos_StartAtZero.set(Essence->second.StreamKind);

            //Stream size is sometime easy to find
            if ((Buffer_End?(Buffer_End-Buffer_Begin):Element_TotalSize_Get())>=File_Size*0.98) //let imagine: if element size is 98% of file size, this is the only one element in the file
            {
                Essence->second.Stream_Size=Buffer_End?(Buffer_End-Buffer_Begin):Element_TotalSize_Get();
            }

            //Compute stream bit rate if there is only one stream
            int64u Stream_Size;
            if (Essence->second.Stream_Size!=(int64u)-1)
                Stream_Size=Essence->second.Stream_Size;
            else
                Stream_Size=File_Size; //TODO: find a way to remove header/footer correctly
            if (Stream_Size!=(int64u)-1)
            {
                //Searching single descriptor if it is the only valid descriptor
                descriptors::iterator SingleDescriptor=Descriptors.end();
                for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
                    if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max)
                    {
                        if (SingleDescriptor!=Descriptors.end())
                        {
                            SingleDescriptor=Descriptors.end();
                            break; // 2 or more descriptors, can not be used
                        }
                        SingleDescriptor=SingleDescriptor_Temp;
                    }

                if (SingleDescriptor!=Descriptors.end())
                {
                    if (SingleDescriptor->second.ByteRate!=(int32u)-1)
                        for (parsers::iterator Parser=Essence->second.Parsers.begin(); Parser!=Essence->second.Parsers.end(); ++Parser)
                            (*Parser)->Stream_BitRateFromContainer=SingleDescriptor->second.ByteRate*8;
                    else if (SingleDescriptor->second.Infos["Duration"].To_float64())
                        for (parsers::iterator Parser=Essences.begin()->second.Parsers.begin(); Parser!=Essences.begin()->second.Parsers.end(); ++Parser)
                            (*Parser)->Stream_BitRateFromContainer=((float64)Stream_Size)*8/(SingleDescriptor->second.Infos["Duration"].To_float64()/1000);
                }
            }
        }

        //Frame info is specific to the container, and it is not updated
        const frame_info FrameInfo_Temp=FrameInfo;
        int64u Frame_Count_NotParsedIncluded_Temp=Frame_Count_NotParsedIncluded;
        if (!IsSub) //Updating for MXF only if MXF is not embedded in another container
        {
            FrameInfo=frame_info();
            Frame_Count_NotParsedIncluded=(int64u)-1;
        }

        //Demux
        #if MEDIAINFO_DEMUX
            if (Essence->second.TrackID!=(int32u)-1)
                Element_Code=Essence->second.TrackID;
            else
                Element_Code=Code.lo;
            Demux_Level=(!Essence->second.Parsers.empty() && ((*(Essence->second.Parsers.begin()))->Demux_UnpacketizeContainer || (*(Essence->second.Parsers.begin()))->Demux_Level==2))?4:2; //Intermediate (D-10 Audio) / Container
            if (!IsSub) //Updating for MXF only if MXF is not embedded in another container
            {
                FrameInfo=Essence->second.FrameInfo;
                Frame_Count_NotParsedIncluded=Essence->second.Frame_Count_NotParsedIncluded;
            }
            Demux_random_access=true;

            bool ShouldDemux=true;
            if (!Essence->second.Parsers.empty() && Essence->second.ShouldCheckAvcHeaders)
            {
                if (Essence->second.Parsers[0]->Status[IsAccepted])
                    ShouldDemux=false;
                else
                {
                    //Checking if we need to add SPS/PPS
                    size_t CheckMax=0x10; //SPS uses to be in the first bytes only
                    if (CheckMax>Element_Size-4)
                        CheckMax=Element_Size-4;
                    ShouldDemux=false;
                    const int8u* Buffer_Temp=Buffer+(size_t)(Buffer_Offset+Element_Offset);
                    for (size_t i=0; i<CheckMax; i++)
                        if (Buffer_Temp[i]==0x00 && Buffer_Temp[i+1]==0x00 && Buffer_Temp[i+2]==0x01 && Buffer_Temp[i+3]==0x67)
                        {
                            //Stream_Temp.Demux_Level&=~((1<<7)|(1<<6)); //Remove the flag, SPS/PPS detected
                            ShouldDemux=true;
                            break;
                        }
                }

                if (!ShouldDemux)
                {
                    //Stream_Temp.Demux_Level|= (1<<6); //In case of seek, we need to send again SPS/PPS //Deactivated because Hydra does not decode after a seek + 1 SPS/PPS only.
                    //Stream_Temp.Demux_Level&=~(1<<7); //Remove the flag, SPS/PPS sent
                    File_Avc::avcintra_header AvcIntraHeader=File_Avc::AVC_Intra_Headers_Data(Essence->second.ShouldCheckAvcHeaders);
                    size_t Buffer_Temp_Size=AvcIntraHeader.Size+(size_t)(Element_Size);
                    int8u* Buffer_Temp_Data=new int8u[Buffer_Temp_Size];
                    if (AvcIntraHeader.Data)
                        memcpy(Buffer_Temp_Data, AvcIntraHeader.Data, AvcIntraHeader.Size);
                    memcpy(Buffer_Temp_Data+AvcIntraHeader.Size, Buffer+Buffer_Offset, (size_t)(Element_Size));
                    Demux(Buffer_Temp_Data, Buffer_Temp_Size, ContentType_MainStream);
                    Open_Buffer_Continue(Essence->second.Parsers[0], AvcIntraHeader.Data, AvcIntraHeader.Size);
                }

                Essence->second.ShouldCheckAvcHeaders=0;
            }
            if (ShouldDemux)
            {
                Demux(Buffer+Buffer_Offset, (size_t)Element_Size, ContentType_MainStream);
            }

        #endif //MEDIAINFO_DEMUX

        if (!Essence->second.Parsers.empty() && !(*(Essence->second.Parsers.begin()))->Status[IsFinished])
        {
            if (Code_Compare3!=0x0E090502 && ((Code_Compare4&0xFF00FF00)==0x17000100 || (Code_Compare4&0xFF00FF00)==0x17000200))
            {
                if (Element_Size)
                {
                    parsers::iterator Parser=Essence->second.Parsers.begin();

                    //Ancillary, SMPTE ST 436
                    int16u Count;
                    Get_B2 (Count,                                  "Number of Lines");
                    if (Count*14>Element_Size)
                    {
                        (*Parser)->Finish();
                        Skip_XX(Element_Size-2,                     "Unknown");
                        Count=0;
                    }
                    for (int16u Pos=0; Pos<Count; Pos++)
                    {
                        Element_Begin1("Line");
                        int32u ArrayCount, ArrayLength;
                        int16u LineNumber, SampleCount;
                        int8u WrappingType, SampleCoding;
                        Get_B2 (LineNumber,                         "Line Number"); Element_Info1(LineNumber);
                        Get_B1 (WrappingType,                       "Wrapping Type");
                        Get_B1 (SampleCoding,                       "Payload Sample Coding");
                        Get_B2 (SampleCount,                        "Payload Sample Count");
                        Get_B4 (ArrayCount,                         "Payload Array Count");
                        Get_B4 (ArrayLength,                        "Payload Array Length");

                        if (Essence->second.Frame_Count_NotParsedIncluded!=(int64u)-1)
                            (*Parser)->Frame_Count_NotParsedIncluded=Essence->second.Frame_Count_NotParsedIncluded;
                        if (Essence->second.FrameInfo.DTS!=(int64u)-1)
                            (*Parser)->FrameInfo.DTS=Essence->second.FrameInfo.DTS;
                        if (Essence->second.FrameInfo.PTS!=(int64u)-1)
                            (*Parser)->FrameInfo.PTS=Essence->second.FrameInfo.PTS;
                        if (Essence->second.FrameInfo.DUR!=(int64u)-1)
                            (*Parser)->FrameInfo.DUR=Essence->second.FrameInfo.DUR;
                        #if defined(MEDIAINFO_VBI_YES)
                            if ((*Parser)->ParserName=="Vbi")
                            {
                                ((File_Vbi*)(*Parser))->WrappingType=WrappingType;
                                ((File_Vbi*)(*Parser))->SampleCoding=SampleCoding;
                                ((File_Vbi*)(*Parser))->LineNumber=LineNumber;
                                if (Pos+1==Count)
                                    ((File_Vbi*)(*Parser))->IsLast=true;
                            }
                        #endif //defined(MEDIAINFO_VBI_YES)
                        #if defined(MEDIAINFO_ANCILLARY_YES)
                            if ((*Parser)->ParserName=="Ancillary")
                                ((File_Ancillary*)(*Parser))->LineNumber=LineNumber;
                            if ((*Parser)->ParserName=="Ancillary" && (((File_Ancillary*)(*Parser))->FrameRate==0 || ((File_Ancillary*)(*Parser))->AspectRatio==0))
                            {
                                //Configuring with video info
                                for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                                    if (Descriptor->second.StreamKind==Stream_Video)
                                    {
                                        ((File_Ancillary*)(*Parser))->HasBFrames=Descriptor->second.HasBFrames;
                                        ((File_Ancillary*)(*Parser))->AspectRatio=Descriptor->second.DisplayAspectRatio;
                                        ((File_Ancillary*)(*Parser))->FrameRate=Descriptor->second.SampleRate;
                                        break;
                                    }
                            }
                        #endif //defined(MEDIAINFO_ANCILLARY_YES)
                        int64u Parsing_Size=SampleCount;
                        int64u Array_Size=ArrayCount*ArrayLength;
                        if (Element_Offset+Parsing_Size>Element_Size)
                            Parsing_Size=Element_Size-Element_Offset; // There is a problem
                        if (Parsing_Size>Array_Size)
                            Parsing_Size=Array_Size; // There is a problem
                        (*Parser)->Frame_Count_NotParsedIncluded=Frame_Count_NotParsedIncluded;
                        Open_Buffer_Continue((*Parser), Buffer+Buffer_Offset+(size_t)(Element_Offset), Parsing_Size);
                        Element_Offset+=Parsing_Size;
                        if (Parsing_Size<Array_Size)
                            Skip_XX(Array_Size-Parsing_Size,    "Padding");
                        Element_End0();
                    }
                    if (!Essence->second.IsFilled && (!Count || (Essence->second.Parsers.size()==1 && Essence->second.Parsers[0]->Status[IsFilled])))
                    {
                        if (Streams_Count>0)
                            Streams_Count--;
                        Essence->second.IsFilled=true;
                        if (Config->ParseSpeed<1.0 && IsSub)
                        {
                            Fill();
                            Open_Buffer_Unsynch();
                            Finish();
                        }
                    }
                }
            }
            else
            {
                for (size_t Pos=0; Pos<Essence->second.Parsers.size(); Pos++)
                {
                    //Parsing
                    if (IsSub)
                    {
                        if (Frame_Count_NotParsedIncluded!=(int64u)-1)
                            Essence->second.Parsers[Pos]->Frame_Count_NotParsedIncluded=Frame_Count_NotParsedIncluded;
                        if (FrameInfo.DTS!=(int64u)-1)
                            Essence->second.Parsers[Pos]->FrameInfo.DTS=FrameInfo.DTS;
                        if (FrameInfo.PTS!=(int64u)-1)
                           Essence->second.Parsers[Pos]->FrameInfo.PTS=FrameInfo.PTS;
                        if (FrameInfo.DUR!=(int64u)-1)
                            Essence->second.Parsers[Pos]->FrameInfo.DUR=FrameInfo.DUR;
                    }
                    else
                    {
                        if (Essence->second.Frame_Count_NotParsedIncluded!=(int64u)-1)
                            Essence->second.Parsers[Pos]->Frame_Count_NotParsedIncluded=Essence->second.Frame_Count_NotParsedIncluded;
                        if (Essence->second.FrameInfo.DTS!=(int64u)-1)
                            Essence->second.Parsers[Pos]->FrameInfo.DTS=Essence->second.FrameInfo.DTS;
                        if (Essence->second.FrameInfo.PTS!=(int64u)-1)
                            Essence->second.Parsers[Pos]->FrameInfo.PTS=Essence->second.FrameInfo.PTS;
                        if (Essence->second.FrameInfo.DUR!=(int64u)-1)
                            Essence->second.Parsers[Pos]->FrameInfo.DUR=Essence->second.FrameInfo.DUR;
                    }
                    Open_Buffer_Continue(Essence->second.Parsers[Pos], Buffer+Buffer_Offset, (size_t)Element_Size);
                    #if MEDIAINFO_DEMUX
                        if (Demux_Level==4 && Config->Demux_EventWasSent && Essence->second.StreamKind==Stream_Video && Essence->second.Parsers[Pos]->ParserIDs[StreamIDs_Size]==MediaInfo_Parser_Jpeg) // Only File_Jpeg. TODO: limit to File_Jpeg instead of video streams
                        {
                            Demux_CurrentParser=Essence->second.Parsers[Pos];
                            Demux_CurrentEssence=Essence;
                        }
                    #endif //MEDIAINFO_DEMUX
                    switch (Essence->second.Parsers[Pos]->Field_Count_InThisBlock)
                    {
                        case 1 : Essence->second.Field_Count_InThisBlock_1++; break;
                        case 2 : Essence->second.Field_Count_InThisBlock_2++; break;
                        default: ;
                    }

                    //Multiple parsers
                    if (Essence->second.Parsers.size()>1)
                    {
                        if (!Essence->second.Parsers[Pos]->Status[IsAccepted] && Essence->second.Parsers[Pos]->Status[IsFinished])
                        {
                            delete static_cast<MediaInfoLib::File__Analyze*>(*(Essence->second.Parsers.begin()+Pos));
                            Essence->second.Parsers.erase(Essence->second.Parsers.begin()+Pos);
                            Pos--;
                        }
                        else if (Essence->second.Parsers.size()>1 && Essence->second.Parsers[Pos]->Status[IsAccepted])
                        {
                            File__Analyze* Parser=Essence->second.Parsers[Pos];
                            for (size_t Pos2=0; Pos2<Essence->second.Parsers.size(); Pos2++)
                            {
                                if (Pos2!=Pos)
                                    delete static_cast<MediaInfoLib::File__Analyze*>(*(Essence->second.Parsers.begin()+Pos2));
                            }
                            Essence->second.Parsers.clear();
                            Essence->second.Parsers.push_back(Parser);
                        }
                    }

                    if (!Status[IsAccepted] && !Essence->second.Parsers.empty() && Essence->second.Parsers[0]->Status[IsAccepted])
                        Accept();
                }

                Element_Offset=Element_Size;
            }

            if (Essence->second.Parsers.size()==1 && Essence->second.Parsers[0]->Status[IsAccepted] && Essence->second.Frame_Count_NotParsedIncluded==(int64u)-1)
            {
                Essence->second.FrameInfo.DTS=Essence->second.Parsers[0]->FrameInfo.DTS;
                Essence->second.FrameInfo.PTS=Essence->second.Parsers[0]->FrameInfo.PTS;
                Essence->second.FrameInfo.DUR=Essence->second.Parsers[0]->FrameInfo.DUR;
            }
            else if (Buffer_End)
            {
                Essence->second.Frame_Count_NotParsedIncluded=(int64u)-1;
                Essence->second.FrameInfo=frame_info();
            }
            else
            {
                if (Essence->second.Frame_Count_NotParsedIncluded!=(int64u)-1)
                    Essence->second.Frame_Count_NotParsedIncluded++;
                if (Essence->second.FrameInfo.DTS!=(int64u)-1 && Essence->second.FrameInfo.DUR!=(int64u)-1)
                    Essence->second.FrameInfo.DTS+=Essence->second.FrameInfo.DUR;
                if (Essence->second.FrameInfo.PTS!=(int64u)-1 && Essence->second.FrameInfo.DUR!=(int64u)-1)
                    Essence->second.FrameInfo.PTS+=Essence->second.FrameInfo.DUR;
            }

            //Disabling this Streams
            if (!Essence->second.IsFilled && Essence->second.Parsers.size()==1 && Essence->second.Parsers[0]->Status[IsFilled])
            {
                if (Streams_Count>0)
                    Streams_Count--;
                Essence->second.IsFilled=true;
                if (Config->ParseSpeed<1.0 && IsSub)
                {
                    Fill();
                    Open_Buffer_Unsynch();
                }
            }
        }
        else
            Skip_XX(Element_Size,                               "Data");

        //Frame info is specific to the container, and it is not updated
        if (Essence->first==Essences_UsedForFrameCount)
        {
            FrameInfo=Essence->second.FrameInfo;
            Frame_Count_NotParsedIncluded=Essence->second.Frame_Count_NotParsedIncluded;
        }
        else
        {
            FrameInfo=FrameInfo_Temp;
            Frame_Count_NotParsedIncluded=Frame_Count_NotParsedIncluded_Temp;
        }

        //Ignore tail
        #if MEDIAINFO_DEMUX
            if (DemuxedSampleCount_Total!=(int64u)-1 && DemuxedSampleCount_Current!=(int64u)-1)
            {
                DemuxedSampleCount_Total+=DemuxedSampleCount_Current;
                Frame_Count_NotParsedIncluded=DemuxedSampleCount_Total;
            }
        #endif //MEDIAINFO_DEMUX
        if (Config->ParseSpeed>=1.0 && Frame_Count_NotParsedIncluded!=(int64u)-1 && Config->File_IgnoreEditsAfter!=(int64u)-1)
        {
            descriptors::iterator SingleDescriptor=Descriptors.end();
            for (descriptors::iterator SingleDescriptor_Temp=Descriptors.begin(); SingleDescriptor_Temp!=Descriptors.end(); ++SingleDescriptor_Temp)
                if (SingleDescriptor_Temp->second.StreamKind!=Stream_Max)
                {
                    if (SingleDescriptor!=Descriptors.end())
                    {
                        SingleDescriptor=Descriptors.end();
                        break; // 2 or more descriptors, can not be used
                    }
                    SingleDescriptor=SingleDescriptor_Temp;
                }

            int64u RealSampleRate=(SingleDescriptor==Descriptors.end() || SingleDescriptor->second.StreamKind!=Stream_Audio)?((int64u)Config->File_EditRate):SingleDescriptor->second.Infos["SamplingRate"].To_int64u();
            int64u IgnoreSamplesAfter;
            if (!RealSampleRate || RealSampleRate==Config->File_EditRate)
                IgnoreSamplesAfter=Config->File_IgnoreEditsAfter;
            else
                IgnoreSamplesAfter=float64_int64s(((float64)Config->File_IgnoreEditsAfter)/Config->File_EditRate*RealSampleRate);
            if (Frame_Count_NotParsedIncluded>=IgnoreSamplesAfter)
            {
                if (PartitionMetadata_FooterPartition!=(int64u)-1 && PartitionMetadata_FooterPartition>=File_Offset+Buffer_Offset+Element_Size)
                    GoTo(PartitionMetadata_FooterPartition);
                else
                    GoToFromEnd(0);
                }
        }
        #if MEDIAINFO_DEMUX
            if (DemuxedSampleCount_Total!=(int64u)-1)
            {
                Frame_Count_NotParsedIncluded=(int64u)-1;
            }
        #endif //MEDIAINFO_DEMUX
    }
    break;
    case 0x060E2B34010101LL: {
    if (!Element_IsComplete_Get())
    {
        Element_WaitForMoreData();
        return;
    }
    if ((Code.lo>>40)==Elements::MXFGenericStreamDataElementKey>>40)
    {
        Element_Name(Mxf_Param_Info_Elements(Elements::MXFGenericStreamDataElementKey));
        MXFGenericStreamDataElementKey_09_01();
    }
    ELEMENT(FillerData)
    ELEMENT(TerminatingFillerData)
    ELEMENT(XMLDocumentText_Indirect)
    ELEMENT(SubDescriptors)
    ELEMENT(PackageMarkerObject)
    }
    break;
    }
    case 0x060E2B34020401LL:
    case 0x060E2B34020501LL:
    case 0x060E2B34025301LL: {
    if (!Element_IsComplete_Get())
    {
        Element_WaitForMoreData();
        return;
    }
    if (false) {}
    GROUP(LensUnitAcquisitionMetadata)
    GROUP(CameraUnitAcquisitionMetadata)
    GROUP(UserDefinedAcquisitionMetadata)
    GROUP(FillerGroup)
    GROUP(Sequence)
    GROUP(SourceClip)
    GROUP(TimecodeGroup)
    GROUP(ContentStorage)
    GROUP(EssenceData)
    GROUP(PictureDescriptor)
    GROUP(CDCIDescriptor)
    GROUP(RGBADescriptor)
    GROUP(Preface)
    GROUP(Identification)
    GROUP(NetworkLocator)
    GROUP(TextLocator)
    GROUP(StereoscopicPictureSubDescriptor)
    GROUP(MaterialPackage)
    GROUP(SourcePackage)
    GROUP(EventTrack)
    GROUP(StaticTrack)
    GROUP(TimelineTrack)
    GROUP(DescriptiveMarker)
    GROUP(SoundDescriptor)
    GROUP(DataEssenceDescriptor)
    GROUP(MultipleDescriptor)
    GROUP(DescriptiveClip)
    GROUP(AES3PCMDescriptor)
    GROUP(WAVEPCMDescriptor)
    GROUP(MPEGVideoDescriptor)
    GROUP(JPEG2000SubDescriptor)
    GROUP(VBIDataDescriptor)
    GROUP(ANCDataDescriptor)
    GROUP(MPEGAudioDescriptor)
    GROUP(ApplicationPlugInObject)
    GROUP(ApplicationReferencedObject)
    GROUP(MCALabelSubDescriptor)
    GROUP(DCTimedTextDescriptor)
    GROUP(DCTimedTextResourceSubDescriptor)
    GROUP(ContainerConstraintsSubDescriptor)
    GROUP(MPEG4VisualSubDescriptor)
    GROUP(AudioChannelLabelSubDescriptor)
    GROUP(SoundfieldGroupLabelSubDescriptor)
    GROUP(GroupOfSoundfieldGroupsLabelSubDescriptor)
    GROUP(AVCSubDescriptor)
    GROUP(IABEssenceDescriptor)
    GROUP(IABSoundfieldLabelSubDescriptor)
    GROUPSTATIC(HeaderPartitionOpenIncomplete)
    GROUPSTATIC(HeaderPartitionClosedIncomplete)
    GROUPSTATIC(HeaderPartitionOpenComplete)
    GROUPSTATIC(HeaderPartitionClosedComplete)
    GROUPSTATIC(BodyPartitionOpenIncomplete)
    GROUPSTATIC(BodyPartitionClosedIncomplete)
    GROUPSTATIC(BodyPartitionOpenComplete)
    GROUPSTATIC(BodyPartitionClosedComplete)
    GROUPSTATIC(GenericStreamPartition)
    GROUPSTATIC(FooterPartitionClosedIncomplete)
    GROUPSTATIC(FooterPartitionClosedComplete)
    GROUPSTATIC(PrimerPack)
    GROUP(IndexTableSegment)
    GROUPSTATIC(RandomIndexPack)
    GROUP(SDTISystemMetadataPack)
    else if ((Code.lo>>16)==(Groups::SDTISystemMetadataPack>>16))
    {
        Code_Compare4&=0xFFFFFF00; //Remove MetaData Block Count
        if (0) {}
        GROUP(SDTIPackageMetadataSet)
        GROUP(SDTIPictureMetadataSet)
        GROUP(SDTISoundMetadataSet)
        GROUP(SDTIDataMetadataSet)
        GROUP(SDTIControlMetadataSet)
    }
    else if ((Code.lo>>16)==(Groups::SystemScheme1FirstElement>>16))
    {
        Element_Code=Code.lo;
        Code_Compare4&=0xFFFF0000; //Remove Metadata or Control Element Identifier + Element Number
        if (0) {}
        GROUP(SystemScheme1FirstElement)
    }
    GROUP(DM_AS_11_Core_Framework)
    GROUP(DM_AS_11_Segmentation_Framework)
    GROUP(DM_AS_11_UKDPP_Framework)
    GROUP(ProductionFramework)
    GROUP(TextBasedFramework)
    GROUP(GenericStreamTextBasedSet)
    GROUP(ISXDDataEssenceDescriptor)
    GROUP(PHDRMetadataTrackSubDescriptor)
    GROUP(OmneonVideoNetworksDescriptiveMetadataLinks)
    GROUP(OmneonVideoNetworksDescriptiveMetadataData)
    GROUP(HdrVividMetadataTrackSubDescriptor)
    GROUP(FFV1PictureSubDescriptor)
    GROUP(MGASoundEssenceDescriptor)
    GROUP(MGAAudioMetadataSubDescriptor)
    GROUP(MGASoundfieldGroupLabelSubDescriptor)
    GROUP(SADMAudioMetadataSubDescriptor)
    GROUP(RIFFChunkDefinitionSubDescriptor)
    GROUP(ADM_CHNASubDescriptor)
    GROUP(ADMChannelMapping)
    GROUP(RIFFChunkReferencesSubDescriptor)
    GROUP(ADMAudioMetadataSubDescriptor)
    GROUP(ADMSoundfieldGroupLabelSubDescriptor)
    else {
        ManageGroup((int8u)(Code.hi>>16)==0x53?&File_Mxf::UnknownGroupItem:&File_Mxf::UnknownElement);
    }
    break;
    }
    default:
        Skip_XX(Element_Size,                                   "Unknown");
    }

    if (Buffer_End && (File_Offset+Buffer_Offset+Element_Size>=Buffer_End || File_GoTo!=(int64u)-1) )
    {
        Buffer_Begin=(int64u)-1;
        Buffer_End=0;
        Buffer_End_Unlimited=false;
        Buffer_Header_Size=0;
        MustSynchronize=true;
    }

    if ((!IsParsingEnd && IsParsingMiddle_MaxOffset==(int64u)-1 && Config->ParseSpeed<1.0)
     && ((!IsSub && File_Offset>=Buffer_PaddingBytes+(Config->ParseSpeed<=0?0x1000000:0x4000000)) //TODO: 64 MB by default (security), should be changed
      || (Streams_Count==0 && !Descriptors.empty())))
    {
        Fill();

        IsParsingEnd=true;
        if (PartitionMetadata_FooterPartition!=(int64u)-1 && PartitionMetadata_FooterPartition>File_Offset+Buffer_Offset+(size_t)Element_Size)
        {
            if (PartitionMetadata_FooterPartition+17<=File_Size)
            {
                GoTo(PartitionMetadata_FooterPartition);
                IsCheckingFooterPartitionAddress=true;
            }
            else
            {
                GoToFromEnd(4); //For random access table
                FooterPartitionAddress_Jumped=true;
            }
        }
        else
        {
            GoToFromEnd(4); //For random access table
            FooterPartitionAddress_Jumped=true;
        }
        Open_Buffer_Unsynch();
    }

    if (File_Offset+Buffer_Offset+Element_Size>=RandomIndexPacks_MaxOffset)
        NextRandomIndexPack();
}

//***************************************************************************
// Elements
//***************************************************************************

#define ELEMENT_BEGIN() \
    { \
        switch (Code2) \
        { \

#define ELEMENT_MIDDLE() \
    default: \
    { \
        std::map<int16u, int128u>::iterator Primer_Value = Primer_Values.find(Code2); \
        if (Primer_Value != Primer_Values.end()) \
        { \
            if (false); \

#define ELEMENT_END() \
        } \
    } \
    } \
    } \

#undef ELEMENT
#define ELEMENT(_CODE, _CALL) \
    case 0x##_CODE :    { \
                        std::map<int16u, int128u>::iterator Primer_Value=Primer_Values.find(0x##_CODE); \
                        const char* Temp; \
                        if (Primer_Value!=Primer_Values.end()) { \
                            auto Temp = Mxf_Param_Info((int32u)Primer_Value->second.hi, Primer_Value->second.lo); \
                            Element_Name(Temp ? Temp : Ztring().From_UUID(Code).To_UTF8().c_str()); \
                        } \
                        else { \
                            Element_Name(Ztring().From_CC2(0x##_CODE).To_UTF8().c_str()); \
                        } \
                        _CALL(); \
                        } \
                        break; \

#define ELEM____UUID_(_ELEMENT) \
else if ((Primer_Value->second.hi>>24)==0x060E2B3401LL \
      && Primer_Value->second.lo==Elements::_ELEMENT) \
{ \
    auto Temp = Mxf_Param_Info((int32u)Primer_Value->second.hi, Primer_Value->second.lo); \
    Element_Name(Temp ? Temp : Ztring().From_UUID(Code).To_UTF8().c_str()); \
    int64u Element_Size_Save=Element_Size; \
    Element_Size=Element_Offset+Length2; \
    _ELEMENT(); \
    Element_Offset=Element_Size; \
    Element_Size=Element_Size_Save; \
}

//---------------------------------------------------------------------------
void File_Mxf::UnknownElement()
{
    Skip_XX(Element_Size-Element_Offset,                        "Unknown");
}

//---------------------------------------------------------------------------
void File_Mxf::UnknownGroupItem()
{
    std::map<int16u, int128u>::iterator Primer_Value = Primer_Values.find(Code2);
    if (Primer_Value != Primer_Values.end()) {
        int8u Category = (int8u)(((int32u)Primer_Value->second.hi) >> 24);
        if (Category == 0x01) {
            ItemType = Mxf_Param_Type_Elements(Primer_Value->second.lo);
        }
        else {
            ItemType = {};
        }
    }
    else {
        ItemType = {};
    }
    static const char* Name = "Value";
    switch (ItemType) {
    case Type_UInt:             {
                                    switch (Length2) {
                                    case 1: 
                                        {
                                        int8u Value;
                                        Get_B1 (Value,          Name); Element_Info1(Value);
                                        Value_UInt = Value;
                                        }
                                    break;
                                    case 2:
                                        {
                                        int16u Value;
                                        Get_B2 (Value,          Name); Element_Info1(Value);
                                        Value_UInt = Value;
                                        }
                                    break;
                                    case 3:
                                        {
                                        Get_B3 (Value_UInt,     Name); Element_Info1(Value_UInt);
                                        }
                                    break;
                                    case 4:
                                        {
                                        Get_B4 (Value_UInt,     Name); Element_Info1(Value_UInt);
                                        }
                                    break;
                                    default:
                                        Skip_XX(Length2,        "(Unsupported)");
                                    }
                                } break;
    case Type_AUID:             { Get_UUID(Value_UUID,          Name); Element_Info1(Ztring().From_UUID(Value_UUID)); } break; // TODO: implement 16-bit Little Endian UL
    case Type_UUID:             { Get_UL(Value_UUID,            Name, nullptr); Element_Info1(int32u(Value_UUID.hi>>32)==0x060E2B34?Mxf_Param_Info((int32u)Value_UUID.hi, Value_UUID.lo):Ztring().From_UUID(Value_UUID).To_UTF8().c_str()); } break;
    case Type_UTF16:            { Get_UTF16B(Length2, Value_String, Name); Element_Info1(Value_String); } break;
    case Type_ISO7:             { Get_UTF8(Length2, Value_String, Name); Element_Info1(Value_String); } break;
    case Type_Ref:              { Get_UUID(Value_UUID,          Name); Element_Info1(Ztring().From_UUID(Value_UUID)); } break;
    case Type_Refs: { VECTOR(16); Value_UUIDVector.resize(Count); for (int32u i = 0; i < Count; i++) { Get_UUID(Value_UUIDVector[i], Name); Element_Info1(Ztring().From_UUID(Value_UUIDVector[i])); } } break;
    default: Skip_XX(Length2,                                   Name);
    }
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor()
{
    Descriptors[InstanceUID].IsAes3Descriptor=true;

    ELEMENT_BEGIN()
    ELEMENT(3D08, AES3PCMDescriptor_AuxBitsMode)
    ELEMENT(3D0D, AES3PCMDescriptor_Emphasis)
    ELEMENT(3D0F, AES3PCMDescriptor_BlockStartOffset)
    ELEMENT(3D10, AES3PCMDescriptor_ChannelStatusMode)
    ELEMENT(3D11, AES3PCMDescriptor_FixedChannelStatusData)
    ELEMENT(3D12, AES3PCMDescriptor_UserDataMode)
    ELEMENT(3D13, AES3PCMDescriptor_FixedUserData)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    WAVEPCMDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3301, CDCIDescriptor_ComponentDepth)
    ELEMENT(3302, CDCIDescriptor_HorizontalSubsampling)
    ELEMENT(3303, CDCIDescriptor_ColorSiting)
    ELEMENT(3304, CDCIDescriptor_BlackRefLevel)
    ELEMENT(3305, CDCIDescriptor_WhiteReflevel)
    ELEMENT(3306, CDCIDescriptor_ColorRange)
    ELEMENT(3307, CDCIDescriptor_PaddingBits)
    ELEMENT(3308, CDCIDescriptor_VerticalSubsampling)
    ELEMENT(3309, CDCIDescriptor_AlphaSampleDepth)
    ELEMENT(330B, CDCIDescriptor_ReversedByteOrder)
    ELEMENT_MIDDLE()
    ELEM____UUID_(SubDescriptors)
    ELEMENT_END()
    PictureDescriptor();

    if (Descriptors[InstanceUID].Infos.find("ColorSpace")==Descriptors[InstanceUID].Infos.end())
        Descriptor_Fill("ColorSpace", "YUV");
}

//---------------------------------------------------------------------------
void File_Mxf::HeaderPartitionOpenIncomplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::HeaderPartitionClosedIncomplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::HeaderPartitionOpenComplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::HeaderPartitionClosedComplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::BodyPartitionOpenIncomplete()
{
    //Parsing
    PartitionMetadata();

    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
        if (!Demux_HeaderParsed)
        {
            Demux_HeaderParsed=true;

            //Testing locators
            Locators_CleanUp();

            if (Config->File_IgnoreEditsBefore && !Config->File_IsDetectingDuration_Get() && Config->Event_CallBackFunction_IsSet()) //Only if demux packet may be requested
                Open_Buffer_Seek(3, 0, (int64u)-1); //Forcing seek to Config->File_IgnoreEditsBefore
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
            {
                if (Locators.empty())
                {
                    Config->Demux_EventWasSent=true; //First set is to indicate the user that header is parsed
                    return;
                }
            }
        }
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
void File_Mxf::BodyPartitionClosedIncomplete()
{
    //Parsing
    PartitionMetadata();

    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
        if (!Demux_HeaderParsed)
        {
            Demux_HeaderParsed=true;

            //Testing locators
            Locators_CleanUp();

            if (Config->File_IgnoreEditsBefore && !Config->File_IsDetectingDuration_Get() && Config->Event_CallBackFunction_IsSet()) //Only if demux packet may be requested
                Open_Buffer_Seek(3, 0, (int64u)-1); //Forcing seek to Config->File_IgnoreEditsBefore
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
            {
                if (Locators.empty())
                {
                    Config->Demux_EventWasSent=true; //First set is to indicate the user that header is parsed
                    return;
                }
            }
        }
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
void File_Mxf::BodyPartitionOpenComplete()
{
    //Parsing
    PartitionMetadata();

    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
        if (!Demux_HeaderParsed)
        {
            Demux_HeaderParsed=true;

            //Testing locators
            Locators_CleanUp();

            if (Config->File_IgnoreEditsBefore && !Config->File_IsDetectingDuration_Get() && Config->Event_CallBackFunction_IsSet()) //Only if demux packet may be requested
                Open_Buffer_Seek(3, 0, (int64u)-1); //Forcing seek to Config->File_IgnoreEditsBefore
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
            {
                if (Locators.empty())
                {
                    Config->Demux_EventWasSent=true; //First set is to indicate the user that header is parsed
                    return;
                }
            }
        }
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
void File_Mxf::BodyPartitionClosedComplete()
{
    //Parsing
    PartitionMetadata();

    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
        if (!Demux_HeaderParsed)
        {
            Demux_HeaderParsed=true;

            //Testing locators
            Locators_CleanUp();

            if (Config->File_IgnoreEditsBefore && !Config->File_IsDetectingDuration_Get() && Config->Event_CallBackFunction_IsSet()) //Only if demux packet may be requested
                Open_Buffer_Seek(3, 0, (int64u)-1); //Forcing seek to Config->File_IgnoreEditsBefore
            if (Config->NextPacket_Get() && Config->Event_CallBackFunction_IsSet())
            {
                if (Locators.empty())
                {
                    Config->Demux_EventWasSent=true; //First set is to indicate the user that header is parsed
                    return;
                }
            }
        }
    #endif //MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
}

//---------------------------------------------------------------------------
void File_Mxf::GenericStreamPartition()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::FooterPartitionOpenIncomplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::FooterPartitionClosedIncomplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::FooterPartitionOpenComplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::FooterPartitionClosedComplete()
{
    //Parsing
    PartitionMetadata();
}

//---------------------------------------------------------------------------
void File_Mxf::ContentStorage()
{
    ELEMENT_BEGIN()
    ELEMENT(1901, ContentStorage_Packages)
    ELEMENT(1902, ContentStorage_EssenceData)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();

    if (Code2==0x3C0A && InstanceUID==Prefaces[Preface_Current].ContentStorage) //InstanceIUD
    {
        Element_Level--;
        Element_Info1("Valid from Preface");
        Element_Level++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::DescriptiveMarker()
{
    ELEMENT_BEGIN()
    ELEMENT(0202, DescriptiveMarker_Duration)
    ELEMENT(6101, DescriptiveMarker_DMFramework)
    ELEMENT(6102, DescriptiveMarker_TrackIDs)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    if (Code2!=0x0202) //TODO: better way to handle that
    StructuralComponent();
}

//---------------------------------------------------------------------------
void File_Mxf::EssenceData()
{
    ELEMENT_BEGIN()
    ELEMENT(2701, EssenceData_LinkedPackageUID)
    ELEMENT(3F06, EssenceData_IndexSID)
    ELEMENT(3F07, EssenceData_BodySID)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::EventTrack()
{
    ELEMENT_BEGIN()
    ELEMENT(4901, EventTrack_EventEditRate)
    ELEMENT(4902, EventTrack_EventOrigin)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenericTrack();
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3001, FileDescriptor_SampleRate)
    ELEMENT(3002, FileDescriptor_ContainerDuration)
    ELEMENT(3004, FileDescriptor_EssenceContainer)
    ELEMENT(3005, FileDescriptor_Codec)
    ELEMENT(3006, FileDescriptor_LinkedTrackID)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenericDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification()
{
    ELEMENT_BEGIN()
    ELEMENT(3C01, Identification_CompanyName)
    ELEMENT(3C02, Identification_ProductName)
    ELEMENT(3C03, Identification_ProductVersion)
    ELEMENT(3C04, Identification_VersionString)
    ELEMENT(3C05, Identification_ProductUID)
    ELEMENT(3C06, Identification_ModificationDate)
    ELEMENT(3C07, Identification_ToolkitVersion)
    ELEMENT(3C08, Identification_Platform)
    ELEMENT(3C09, Identification_ThisGenerationUID)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment()
{
    if (Element_Offset==4)
    {
        #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
            //Testing if already parsed
            for (size_t Pos=0; Pos<IndexTables.size(); Pos++)
                if (File_Offset+Buffer_Offset-Header_Size==IndexTables[Pos].StreamOffset)
                {
                    Element_Offset=Element_Size;
                    return;
                }

            IndexTables.push_back(indextable());
            IndexTables[IndexTables.size()-1].StreamOffset=File_Offset+Buffer_Offset-Header_Size;
        #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK
    }

    ELEMENT_BEGIN()
    ELEMENT(3F05, IndexTableSegment_EditUnitByteCount)
    ELEMENT(3F06, IndexTableSegment_IndexSID)
    ELEMENT(3F07, IndexTableSegment_BodySID)
    ELEMENT(3F08, IndexTableSegment_SliceCount)
    ELEMENT(3F09, IndexTableSegment_DeltaEntryArray)
    ELEMENT(3F0A, IndexTableSegment_IndexEntryArray)
    ELEMENT(3F0B, IndexTableSegment_IndexEditRate)
    ELEMENT(3F0C, IndexTableSegment_IndexStartPosition)
    ELEMENT(3F0D, IndexTableSegment_IndexDuration)
    ELEMENT(3F0E, IndexTableSegment_PosTableCount)
    ELEMENT(8002, IndexTableSegment_8002)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    InterchangeObject();

    if (Code2==0x3C0A) //InstanceIUD
    {
        IndexTable_NSL=0;
        IndexTable_NPE=0;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::GenericDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(2F01, GenericDescriptor_Locators)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::JPEG2000SubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(Rsiz)
    ELEM____UUID_(Xsiz)
    ELEM____UUID_(Ysiz)
    ELEM____UUID_(XOsiz)
    ELEM____UUID_(YOsiz)
    ELEM____UUID_(XTsiz)
    ELEM____UUID_(YTsiz)
    ELEM____UUID_(XTOsiz)
    ELEM____UUID_(YTOsiz)
    ELEM____UUID_(Csiz)
    ELEM____UUID_(PictureComponentSizing)
    ELEM____UUID_(CodingStyleDefault)
    ELEM____UUID_(QuantizationDefault)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1PictureSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(FFV1InitializationMetadata)
    ELEM____UUID_(FFV1IdenticalGOP)
    ELEM____UUID_(FFV1MaxGOP)
    ELEM____UUID_(FFV1MaximumBitRate)
    ELEM____UUID_(FFV1Version)
    ELEM____UUID_(FFV1MicroVersion)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::MGASoundEssenceDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(SubDescriptors)
    ELEM____UUID_(MGASoundEssenceBlockAlign)
    ELEM____UUID_(MGASoundEssenceAverageBytesPerSecond)
    ELEM____UUID_(MGASoundEssenceSequenceOffset)
    ELEMENT_END()
    SoundDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::MGAAudioMetadataSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(MGALinkID)
    ELEM____UUID_(MGAAudioMetadataIndex)
    ELEM____UUID_(MGAAudioMetadataIdentifier)
    ELEM____UUID_(MGAAudioMetadataPayloadULArray)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::MGASoundfieldGroupLabelSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(MGAMetadataSectionLinkID)
    ELEMENT_END()
    MCALabelSubDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::SADMAudioMetadataSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(SADMMetadataSectionLinkID)
    ELEM____UUID_(SADMProfileLevelULBatch)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkDefinitionSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(RIFFChunkStreamID)
    ELEM____UUID_(RIFFChunkID)
    ELEM____UUID_(RIFFChunkUUID)
    ELEM____UUID_(RIFFChunkHashSHA1)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ADM_CHNASubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(NumLocalChannels)
    ELEM____UUID_(NumADMAudioTrackUIDs)
    ELEM____UUID_(ADMChannelMappingsArray)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ADMChannelMapping()
{
    //Clear if first sub-element
    #if defined(MEDIAINFO_ADM_YES)
        if (Element_Offset<=4)
            ADMChannelMapping_Presence.reset();
    #endif //defined(MEDIAINFO_ADM_YES)

    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(LocalChannelID)
    ELEM____UUID_(ADMAudioTrackUID)
    ELEM____UUID_(ADMAudioTrackChannelFormatID)
    ELEM____UUID_(ADMAudioPackFormatID)
    ELEMENT_END()
    GenerationInterchangeObject();

    #if defined(MEDIAINFO_ADM_YES)
        if ((ADMChannelMapping_Presence.to_ulong()&3)==3)
        {
            if (!Adm)
            {
                Adm=new File_Adm;
                Open_Buffer_Init(Adm);
            }
            ((File_Adm*)Adm)->chna_Add(ADMChannelMapping_LocalChannelID, ADMChannelMapping_ADMAudioTrackUID);
            ADMChannelMapping_Presence.reset();
        }
    #endif //defined(MEDIAINFO_ADM_YES)
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkReferencesSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(RIFFChunkStreamIDsArray)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioMetadataSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(RIFFChunkStreamID_link1)
    ELEM____UUID_(ADMProfileLevelULBatch)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ADMSoundfieldGroupLabelSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(RIFFChunkStreamID_link2)
    ELEM____UUID_(ADMAudioProgrammeID_ST2131)
    ELEM____UUID_(ADMAudioContentID_ST2131)
    ELEM____UUID_(ADMAudioObjectID_ST2131)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::GenerationInterchangeObject()
{
    ELEMENT_BEGIN()
    ELEMENT(0102, GenerationInterchangeObject_GenerationUID)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage()
{
    ELEMENT_BEGIN()
    ELEMENT(4401, GenericPackage_PackageUID)
    ELEMENT(4402, GenericPackage_Name)
    ELEMENT(4403, GenericPackage_Tracks)
    ELEMENT(4404, GenericPackage_PackageModifiedDate)
    ELEMENT(4405, GenericPackage_PackageCreationDate)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3201, PictureDescriptor_PictureEssenceCoding)
    ELEMENT(3202, PictureDescriptor_StoredHeight)
    ELEMENT(3203, PictureDescriptor_StoredWidth)
    ELEMENT(3204, PictureDescriptor_SampledHeight)
    ELEMENT(3205, PictureDescriptor_SampledWidth)
    ELEMENT(3206, PictureDescriptor_SampledXOffset)
    ELEMENT(3207, PictureDescriptor_SampledYOffset)
    ELEMENT(3208, PictureDescriptor_DisplayHeight)
    ELEMENT(3209, PictureDescriptor_DisplayWidth)
    ELEMENT(320A, PictureDescriptor_DisplayXOffset)
    ELEMENT(320B, PictureDescriptor_DisplayYOffset)
    ELEMENT(320C, PictureDescriptor_FrameLayout)
    ELEMENT(320D, PictureDescriptor_VideoLineMap)
    ELEMENT(320E, PictureDescriptor_AspectRatio)
    ELEMENT(320F, PictureDescriptor_AlphaTransparency)
    ELEMENT(3210, PictureDescriptor_TransferCharacteristic)
    ELEMENT(3211, PictureDescriptor_ImageAlignmentOffset)
    ELEMENT(3212, PictureDescriptor_FieldDominance)
    ELEMENT(3213, PictureDescriptor_ImageStartOffset)
    ELEMENT(3214, PictureDescriptor_ImageEndOffset)
    ELEMENT(3215, PictureDescriptor_SignalStandard)
    ELEMENT(3216, PictureDescriptor_StoredF2Offset)
    ELEMENT(3217, PictureDescriptor_DisplayF2Offset)
    ELEMENT(3218, PictureDescriptor_ActiveFormatDescriptor)
    ELEMENT(3219, PictureDescriptor_ColorPrimaries)
    ELEMENT(321A, PictureDescriptor_CodingEquations)
    ELEMENT_MIDDLE()
    ELEM____UUID_(MasteringDisplayPrimaries)
    ELEM____UUID_(MasteringDisplayWhitePointChromaticity)
    ELEM____UUID_(MasteringDisplayMaximumLuminance)
    ELEM____UUID_(MasteringDisplayMinimumLuminance)
    ELEMENT_END()
    FileDescriptor();

    if (Descriptors[InstanceUID].StreamKind==Stream_Max)
    {
        Descriptors[InstanceUID].StreamKind=Stream_Video;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3D01, SoundDescriptor_QuantizationBits)
    ELEMENT(3D02, SoundDescriptor_Locked)
    ELEMENT(3D03, SoundDescriptor_AudioSamplingRate)
    ELEMENT(3D04, SoundDescriptor_AudioRefLevel)
    ELEMENT(3D05, SoundDescriptor_ElectroSpatialFormulation)
    ELEMENT(3D06, SoundDescriptor_SoundEssenceCompression)
    ELEMENT(3D07, SoundDescriptor_ChannelCount)
    ELEMENT(3D0C, SoundDescriptor_DialNorm)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    FileDescriptor();

    if (Descriptors[InstanceUID].StreamKind==Stream_Max)
    {
        Descriptors[InstanceUID].StreamKind=Stream_Audio;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::DataEssenceDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3E01, DataEssenceDescriptor_DataEssenceCoding)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    FileDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericTrack()
{
    ELEMENT_BEGIN()
    ELEMENT(4801, GenericTrack_TrackID)
    ELEMENT(4802, GenericTrack_TrackName)
    ELEMENT(4803, GenericTrack_Sequence)
    ELEMENT(4804, GenericTrack_TrackNumber)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::InterchangeObject()
{
    ELEMENT_BEGIN()
    ELEMENT(3C0A, InterchangeObject_InstanceUID)
    ELEMENT_MIDDLE()
    ELEMENT_END()
}

//---------------------------------------------------------------------------
void File_Mxf::MaterialPackage()
{
    GenericPackage();

    if (Code2==0x3C0A)
    {
        if (InstanceUID==Prefaces[Preface_Current].PrimaryPackage) //InstanceIUD
        {
            Element_Level--;
            Element_Info1("Primary package");
            Element_Level++;
        }
        for (contentstorages::iterator ContentStorage=ContentStorages.begin(); ContentStorage!=ContentStorages.end(); ++ContentStorage)
        {
            for (size_t Pos=0; Pos<ContentStorage->second.Packages.size(); Pos++)
                if (InstanceUID==ContentStorage->second.Packages[Pos])
                {
                    Element_Level--;
                    Element_Info1("Valid from Content storage");
                    Element_Level++;
                }
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::MPEGVideoDescriptor()
{
    Descriptors[InstanceUID].HasMPEGVideoDescriptor=true;

    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(SingleSequence)
    ELEM____UUID_(ConstantBPictureCount)
    ELEM____UUID_(CodedContentScanning)
    ELEM____UUID_(LowDelay)
    ELEM____UUID_(ClosedGOP)
    ELEM____UUID_(IdenticalGOP)
    ELEM____UUID_(MaxGOP)
    ELEM____UUID_(MaxBPictureCount)
    ELEM____UUID_(ProfileAndLevel)
    ELEM____UUID_(BitRate)
    ELEMENT_END()
    CDCIDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::MultipleDescriptor()
{
    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
        Descriptors[InstanceUID].Type=descriptor::type_Mutiple;

    ELEMENT_BEGIN()
    ELEMENT(3F01, MultipleDescriptor_FileDescriptors)
    ELEMENT_MIDDLE()
    //ELEM____UUID_(SubDescriptors) //TODO: check when MPEG-4 Visual subdescriptor, it disabled stream info merge
    ELEMENT_END()
    FileDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::DescriptiveClip()
{
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::NetworkLocator()
{
    ELEMENT_BEGIN()
    ELEMENT(4001, NetworkLocator_URLString)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();

    if (Code2==0x3C0A)
    {
        for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
        {
            for (size_t Pos=0; Pos<Descriptor->second.Locators.size(); Pos++)
                if (InstanceUID==Descriptor->second.Locators[Pos])
                {
                    Element_Level--;
                    Element_Info1("Valid from Descriptor");
                    Element_Level++;
                }
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Preface()
{
    ELEMENT_BEGIN()
    ELEMENT(3B02, Preface_LastModifiedDate)
    ELEMENT(3B03, Preface_ContentStorage)
    ELEMENT(3B05, Preface_Version)
    ELEMENT(3B06, Preface_Identifications)
    ELEMENT(3B07, Preface_ObjectModelVersion)
    ELEMENT(3B08, Preface_PrimaryPackage)
    ELEMENT(3B09, Preface_OperationalPattern)
    ELEMENT(3B0A, Preface_EssenceContainers)
    ELEMENT(3B0B, Preface_DMSchemes)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();

    if (Code2==0x3C0A) //InstanceIUD
    {
        Preface_Current=InstanceUID;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::PrimerPack()
{
    //Parsing
    VECTOR(2+16);
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("LocalTagEntryBatch");
        int16u LocalTag;
        int128u UID;
        Get_B2 (LocalTag,                                       "LocalTag"); Element_Info1(Ztring().From_CC2(LocalTag));
        Get_UL (UID,                                            "UID", NULL); { auto Temp = Mxf_Param_Info((int32u)UID.hi, UID.lo); Element_Info1(Temp ? Temp : Ztring().From_UUID(UID).To_UTF8().c_str()); };
        Element_End0();

        FILLING_BEGIN();
            Primer_Values[LocalTag]=UID;
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor()
{
    Descriptors[InstanceUID].Type=descriptor::Type_RGBA;

    ELEMENT_BEGIN()
    ELEMENT(3401, RGBADescriptor_PixelLayout)
    ELEMENT(3403, RGBADescriptor_Palette)
    ELEMENT(3404, RGBADescriptor_PaletteLayout)
    ELEMENT(3405, RGBADescriptor_ScanningDirection)
    ELEMENT(3406, RGBADescriptor_ComponentMaxRef)
    ELEMENT(3407, RGBADescriptor_ComponentMinRef)
    ELEMENT(3408, RGBADescriptor_AlphaMaxRef)
    ELEMENT(3409, RGBADescriptor_AlphaMinRef)
    ELEMENT_MIDDLE()
    ELEM____UUID_(SubDescriptors)
    ELEMENT_END()
    PictureDescriptor(); 
    
    if (Descriptors[InstanceUID].Infos.find("ColorSpace")==Descriptors[InstanceUID].Infos.end())
        Descriptor_Fill("ColorSpace", "RGB");
}

//---------------------------------------------------------------------------
void File_Mxf::RandomIndexPack()
{
    //Parsing
    while (Element_Offset+4<Element_Size)
    {
        Element_Begin1("PartitionArray");
        randomindexpack RandomIndexPack;
        Get_B4 (RandomIndexPack.BodySID,                        "BodySID"); Element_Info1(RandomIndexPack.BodySID);
        Get_B8 (RandomIndexPack.ByteOffset,                     "ByteOffset"); Element_Info1(Ztring::ToZtring(RandomIndexPack.ByteOffset, 16));
        Element_End0();

        FILLING_BEGIN();
            if (!RandomIndexPacks_AlreadyParsed && PartitionPack_AlreadyParsed.find(RandomIndexPack.ByteOffset)==PartitionPack_AlreadyParsed.end())
                RandomIndexPacks.push_back(RandomIndexPack);
            if (!RandomIndexPacks_AlreadyParsed && ExtraMetadata_SID.find(RandomIndexPack.BodySID)!=ExtraMetadata_SID.end() && RandomIndexPack.ByteOffset<ExtraMetadata_Offset)
                ExtraMetadata_Offset=RandomIndexPack.ByteOffset;
        FILLING_END();
    }
    Skip_B4(                                                    "Length");

    FILLING_BEGIN();
        if (Config->ParseSpeed<1.0 && !RandomIndexPacks_AlreadyParsed && !RandomIndexPacks.empty() && Config->File_Mxf_ParseIndex_Get())
        {
            IsParsingEnd=true;
            GoTo(RandomIndexPacks[0].ByteOffset);
            RandomIndexPacks.erase(RandomIndexPacks.begin());
            Open_Buffer_Unsynch();

            //Hints
            if (File_Buffer_Size_Hint_Pointer)
                (*File_Buffer_Size_Hint_Pointer)=64*1024;
        }
        else if (!RandomIndexPacks_AlreadyParsed && !Partitions_IsFooter && !RandomIndexPacks.empty() && (!RandomIndexPacks[RandomIndexPacks.size()-1].BodySID || File_Offset+Buffer_Offset-Header_Size-RandomIndexPacks[RandomIndexPacks.size()-1].ByteOffset<16*1024*1024)) // If footer was not parsed but is available
        {
            GoTo(RandomIndexPacks[RandomIndexPacks.size()-1].ByteOffset);
        }
        RandomIndexPacks_AlreadyParsed=true;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::FillerGroup()
{
    ELEMENT_BEGIN()
    ELEMENT(0202, DescriptiveMarker_Duration)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    if (Code2!=0x0202) //TODO: better way to handle that
    StructuralComponent();

    DescriptiveMarkers[InstanceUID].IsAs11SegmentFiller=true;
}

//---------------------------------------------------------------------------
void File_Mxf::Sequence()
{
    ELEMENT_BEGIN()
    ELEMENT(1001, Sequence_StructuralComponents)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    StructuralComponent();

    if (Code2==0x3C0A)
    {
        for (std::map<int128u, track>::iterator Track=Tracks.begin(); Track!=Tracks.end(); ++Track)
        {
            if (InstanceUID==Track->second.Sequence)
            {
                Element_Level--;
                Element_Info1("Valid from track");
                Element_Level++;
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SourceClip()
{
    ELEMENT_BEGIN()
    ELEMENT(1101, SourceClip_SourcePackageID)
    ELEMENT(1102, SourceClip_SourceTrackID)
    ELEMENT(1201, SourceClip_StartPosition)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    StructuralComponent();
}

//---------------------------------------------------------------------------
void File_Mxf::SourcePackage()
{
    ELEMENT_BEGIN()
    ELEMENT(4701, SourcePackage_Descriptor)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenericPackage();

    Packages[InstanceUID].IsSourcePackage=true;
}

//---------------------------------------------------------------------------
void File_Mxf::StaticTrack() 
{
    GenericTrack();
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1FirstElement()
{
    ELEMENT_BEGIN()
    #if MEDIAINFO_TRACE
    ELEMENT(0101, SystemScheme1_FrameCount)
    #endif //MEDIAINFO_TRACE
    ELEMENT(0102, SystemScheme1_TimeCodeArray)
    #if MEDIAINFO_TRACE
    ELEMENT(0103, SystemScheme1_ClipIDArray)
    ELEMENT(0104, SystemScheme1_ExtendedClipIDArray)
    ELEMENT(0105, SystemScheme1_VideoIndexArray)
    ELEMENT(0106, SystemScheme1_KLVMetadataSequence)
    ELEMENT(3001, SystemScheme1_SampleRate)
    ELEMENT(4804, SystemScheme1_EssenceTrackNumber)
    ELEMENT(6801, SystemScheme1_EssenceTrackNumberBatch)
    ELEMENT(6803, SystemScheme1_ContentPackageIndexArray)
    #endif //MEDIAINFO_TRACE
    ELEMENT_MIDDLE()
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::DM_AS_11_Core_Framework()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(AS_11_Series_Title)
    ELEM____UUID_(AS_11_Programme_Title)
    ELEM____UUID_(AS_11_Episode_Title_Number)
    ELEM____UUID_(AS_11_Shim_Name)
    ELEM____UUID_(AS_11_Audio_Track_Layout)
    ELEM____UUID_(AS_11_Primary_Audio_Language)
    ELEM____UUID_(AS_11_Closed_Captions_Present)
    ELEM____UUID_(AS_11_Closed_Captions_Type)
    ELEM____UUID_(AS_11_Caption_Language)
    ELEM____UUID_(AS_11_Shim_Version)
    ELEMENT_END()
    StructuralComponent();

    if (Code2==0x3C0A) //InstanceIUD
        AS11s[InstanceUID].Type=as11::Type_Core;
}

//---------------------------------------------------------------------------
void File_Mxf::DM_AS_11_Segmentation_Framework()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(AS_11_Part_Number)
    ELEM____UUID_(AS_11_Part_Total)
    ELEMENT_END()
    StructuralComponent();

    if (Code2==0x3C0A) //InstanceIUD
        AS11s[InstanceUID].Type=as11::Type_Segmentation;
}

//---------------------------------------------------------------------------
void File_Mxf::DM_AS_11_UKDPP_Framework()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(UKDPP_Production_Number)
    ELEM____UUID_(UKDPP_Synopsis)
    ELEM____UUID_(UKDPP_Originator)
    ELEM____UUID_(UKDPP_Copyright_Year)
    ELEM____UUID_(UKDPP_Other_Identifier)
    ELEM____UUID_(UKDPP_Other_Identifier_Type)
    ELEM____UUID_(UKDPP_Genre)
    ELEM____UUID_(UKDPP_Distributor)
    ELEM____UUID_(UKDPP_Picture_Ratio)
    ELEM____UUID_(UKDPP_3D)
    ELEM____UUID_(UKDPP_3D_Type)
    ELEM____UUID_(UKDPP_Product_Placement)
    ELEM____UUID_(UKDPP_PSE_Pass)
    ELEM____UUID_(UKDPP_PSE_Manufacturer)
    ELEM____UUID_(UKDPP_PSE_Version)
    ELEM____UUID_(UKDPP_Video_Comments)
    ELEM____UUID_(UKDPP_Secondary_Audio_Language)
    ELEM____UUID_(UKDPP_Tertiary_Audio_Language)
    ELEM____UUID_(UKDPP_Audio_Loudness_Standard)
    ELEM____UUID_(UKDPP_Audio_Comments)
    ELEM____UUID_(UKDPP_Line_Up_Start)
    ELEM____UUID_(UKDPP_Ident_Clock_Start)
    ELEM____UUID_(UKDPP_Total_Number_Of_Parts)
    ELEM____UUID_(UKDPP_Total_Programme_Duration)
    ELEM____UUID_(UKDPP_Audio_Description_Present)
    ELEM____UUID_(UKDPP_Audio_Description_Type)
    ELEM____UUID_(UKDPP_Open_Captions_Present)
    ELEM____UUID_(UKDPP_Open_Captions_Type)
    ELEM____UUID_(UKDPP_Open_Captions_Language)
    ELEM____UUID_(UKDPP_Signing_Present)
    ELEM____UUID_(UKDPP_Sign_Language)
    ELEM____UUID_(UKDPP_Completion_Date)
    ELEM____UUID_(UKDPP_Textless_Elements_Exist)
    ELEM____UUID_(UKDPP_Programme_Has_Text)
    ELEM____UUID_(UKDPP_Programme_Text_Language)
    ELEM____UUID_(UKDPP_Contact_Email)
    ELEM____UUID_(UKDPP_Contact_Telephone_Number)
    ELEMENT_END()
    StructuralComponent();

    if (Code2==0x3C0A) //InstanceIUD
        AS11s[InstanceUID].Type=as11::Type_UKDPP;
}

//---------------------------------------------------------------------------
void File_Mxf::ProductionFramework()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(PrimaryExtendedSpokenLanguageCode)
    ELEM____UUID_(SecondaryExtendedSpokenLanguageCode)
    ELEM____UUID_(OriginalExtendedSpokenLanguageCode)
    ELEM____UUID_(SecondaryOriginalExtendedSpokenLanguageCode)
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::TextBasedFramework()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(TextBasedObject)
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericStreamTextBasedSet()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(TextDataDescription)
    ELEM____UUID_(TextMIMEMediaType)
    ELEM____UUID_(RFC5646TextLanguageCode)
    ELEM____UUID_(GenericStreamID)
    ELEMENT_END()
    InterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericStreamID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        ExtraMetadata_SID.insert(Data);
        Descriptors[InstanceUID].SID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MXFGenericStreamDataElementKey_09_01()
{
    // Check if already parsed
    auto Offset=File_Offset+Buffer_Offset+Element_Offset;
    auto Found=MXFGenericStreamDataElementKey.find(Offset);
    if (Found!=MXFGenericStreamDataElementKey.end())
    {
        Skip_XX(Element_Size,                                   "(Already parsed)");
        return;
    }
    auto& Elem=MXFGenericStreamDataElementKey[Offset];
    if (Partitions_Pos<Partitions.size())
        Elem.SID=Partitions[Partitions_Pos].BodySID;

    //Preparing
    vector<File__Analyze*> Parsers;
    #if 1
        Parsers.push_back(new File_DolbyVisionMetadata);
    #endif
    #if 1
        Parsers.push_back(new File_HdrVividMetadata);
    #endif
    #if defined(MEDIAINFO_ADM_YES)
        Parsers.push_back(new File_Adm);
    #endif
    #if 1
        Parsers.push_back(new File_DolbyAudioMetadata(true));
    #endif

    //Parsing
    for (auto& Parser : Parsers)
    {
        Open_Buffer_Init(Parser);
        Open_Buffer_Continue(Parser);
        if (!Elem.Parser && Parser->Status[IsAccepted])
            Elem.Parser=Parser;
        else
            delete Parser;
        Element_Offset=0;
    }
    if (!Elem.Parser)
    {
        File__Analyze* Parser = new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Elem.Parser = Parser;
    }

    //Exception - ADM
    #if defined(MEDIAINFO_ADM_YES)
        if (Elem.Parser && Elem.Parser->Retrieve_Const(Stream_General, 0, General_Format)==__T("ADM"))
        {
            ((File_Adm*)Elem.Parser)->chna_Move((File_Adm*)Adm);
            delete Adm;
            Adm=Elem.Parser;
            Elem.Parser=nullptr;
        }
    #endif

    //Exception - Dolby Audio Metadata
    #if 1
        if (Elem.Parser && Elem.Parser->Retrieve_Const(Stream_General, 0, General_Format)==__T("Dolby Audio Metadata"))
        {
            delete DolbyAudioMetadata;
            DolbyAudioMetadata=Elem.Parser;
            Elem.Parser=nullptr;
        }
    #endif

    Skip_String(Element_Size,                                   "Data");
    Element_Show();
}

//---------------------------------------------------------------------------
void File_Mxf::StructuralComponent()
{
    ELEMENT_BEGIN()
    ELEMENT(0201, StructuralComponent_DataDefinition)
    ELEMENT(0202, StructuralComponent_Duration)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::TextLocator()
{
    ELEMENT_BEGIN()
    ELEMENT(4101, TextLocator_LocatorName)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::StereoscopicPictureSubDescriptor()
{
    StereoscopicPictureSubDescriptor_IsPresent=true;

    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::TimecodeGroup()
{
    if (Element_Offset==4)
    {
        MxfTimeCodeForDelay=mxftimecode();
        DTS_Delay=0;
        FrameInfo.DTS=0;
    }

    ELEMENT_BEGIN()
    ELEMENT(1501, TimecodeGroup_StartTimecode)
    ELEMENT(1502, TimecodeGroup_RoundedTimecodeBase)
    ELEMENT(1503, TimecodeGroup_DropFrame)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    StructuralComponent();
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT(3D09, WAVEPCMDescriptor_AvgBps)
    ELEMENT(3D0A, WAVEPCMDescriptor_BlockAlign)
    ELEMENT(3D0B, WAVEPCMDescriptor_SequenceOffset)
    ELEMENT(3D29, WAVEPCMDescriptor_PeakEnvelopeVersion)
    ELEMENT(3D2A, WAVEPCMDescriptor_PeakEnvelopeFormat)
    ELEMENT(3D2B, WAVEPCMDescriptor_PointsPerPeakValue)
    ELEMENT(3D2C, WAVEPCMDescriptor_PeakEnvelopeBlockSize)
    ELEMENT(3D2D, WAVEPCMDescriptor_PeakChannels)
    ELEMENT(3D2E, WAVEPCMDescriptor_PeakFrames)
    ELEMENT(3D2F, WAVEPCMDescriptor_PeakOfPeaksPosition)
    ELEMENT(3D30, WAVEPCMDescriptor_PeakEnvelopeTimestamp)
    ELEMENT(3D31, WAVEPCMDescriptor_PeakEnvelopeData)
    ELEMENT(3D32, WAVEPCMDescriptor_ChannelAssignment)
    ELEMENT_MIDDLE()
    ELEM____UUID_(SubDescriptors)
    ELEMENT_END()
    SoundDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::VBIDataDescriptor()
{
    DataEssenceDescriptor();

    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
    {
        Descriptors[InstanceUID].Type=descriptor::Type_AncPackets;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::ANCDataDescriptor()
{
    DataEssenceDescriptor();

    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
    {
        Descriptors[InstanceUID].Type=descriptor::Type_AncPackets;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::MPEGAudioDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(MPEGAudioBitRate)
    ELEMENT_END()
    SoundDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::PackageMarkerObject()
{
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ApplicationPlugInObject()
{
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ApplicationReferencedObject()
{
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::MCALabelSubDescriptor()
{
    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
        Descriptors[InstanceUID].Type=descriptor::Type_MCALabelSubDescriptor;

    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(MCAChannelID)
    ELEM____UUID_(MCALabelDictionaryID)
    ELEM____UUID_(MCATagSymbol)
    ELEM____UUID_(MCATagName)
    ELEM____UUID_(GroupOfSoundfieldGroupsLinkID)
    ELEM____UUID_(MCALinkID)
    ELEM____UUID_(SoundfieldGroupLinkID)
    ELEM____UUID_(MCAPartitionKind)
    ELEM____UUID_(MCAPartitionNumber)
    ELEM____UUID_(MCATitle)
    ELEM____UUID_(MCATitleVersion)
    ELEM____UUID_(MCATitleSubVersion)
    ELEM____UUID_(MCAEpisode)
    ELEM____UUID_(MCAAudioContentKind)
    ELEM____UUID_(MCAAudioElementKind)
    ELEM____UUID_(RFC5646SpokenLanguage)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::DCTimedTextDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(ResourceID)
    ELEM____UUID_(NamespaceURI)
    ELEM____UUID_(UCSEncoding)
    ELEMENT_END()
    DataEssenceDescriptor();

    if (Descriptors[InstanceUID].StreamKind==Stream_Max)
    {
        Descriptors[InstanceUID].StreamKind=Stream_Text;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::DCTimedTextResourceSubDescriptor()
{
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ContainerConstraintsSubDescriptor()
{
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::MPEG4VisualSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(MPEG4VisualSingleSequence)
    ELEM____UUID_(MPEG4VisualConstantBVOPs)
    ELEM____UUID_(MPEG4VisualCodedContentType)
    ELEM____UUID_(MPEG4VisualLowDelay)
    ELEM____UUID_(MPEG4VisualClosedGOP)
    ELEM____UUID_(MPEG4VisualIdenticalGOP)
    ELEM____UUID_(MPEG4VisualMaxGOP)
    ELEM____UUID_(MPEG4VisualBVOPCount)
    ELEM____UUID_(MPEG4VisualProfileAndLevel)
    ELEM____UUID_(MPEG4VisualBitRate)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::ResourceID()
{
    //Parsing
    Info_UUID(Data,                                             "UUID"); Element_Info1(Ztring().From_UUID(Data));
}

//---------------------------------------------------------------------------
void File_Mxf::NamespaceURI()
{
    //Parsing
    Info_UTF16B (Length2, Value,                                "Value"); Element_Info1(Value);
}

//---------------------------------------------------------------------------
void File_Mxf::TextMIMEMediaType()
{
    //Parsing
    Info_UTF16B (Length2, Value,                                "Value"); Element_Info1(Value);
}

//---------------------------------------------------------------------------
void File_Mxf::UCSEncoding()
{
    //Parsing
    Info_UTF16B (Length2, Value,                                "Value"); Element_Info1(Value);
}

//---------------------------------------------------------------------------
void File_Mxf::AudioChannelLabelSubDescriptor()
{
    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
        Descriptors[InstanceUID].Type=descriptor::Type_AudioChannelLabelSubDescriptor;

    MCALabelSubDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundfieldGroupLabelSubDescriptor()
{
    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
        Descriptors[InstanceUID].Type=descriptor::Type_SoundfieldGroupLabelSubDescriptor;

    MCALabelSubDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::GroupOfSoundfieldGroupsLabelSubDescriptor()
{
    if (Descriptors[InstanceUID].Type==descriptor::Type_Unknown)
        Descriptors[InstanceUID].Type=descriptor::Type_GroupOfSoundfieldGroupsLabelSubDescriptor;

    MCALabelSubDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(AVCConstantBPictureFlag)
    ELEM____UUID_(AVCCodedContentKind)
    ELEM____UUID_(AVCClosedGOPIndicator)
    ELEM____UUID_(AVCIdenticalGOPIndicator)
    ELEM____UUID_(AVCMaximumGOPSize)
    ELEM____UUID_(AVCMaximumBPictureCount)
    ELEM____UUID_(AVCProfile)
    ELEM____UUID_(AVCMaximumBitRate)
    ELEM____UUID_(AVCProfileConstraint)
    ELEM____UUID_(AVCLevel)
    ELEM____UUID_(AVCDecodingDelay)
    ELEM____UUID_(AVCMaximumRefFrames)
    ELEM____UUID_(AVCSequenceParameterSetFlag)
    ELEM____UUID_(AVCPictureParameterSetFlag)
    ELEM____UUID_(AVCAverageBitRate)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::IABEssenceDescriptor()
{
    SoundDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::IABSoundfieldLabelSubDescriptor()
{
    MCALabelSubDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAChannelID()
{
    if (Length2==4)
    {
        Info_B4(Value,                                          "Value"); Element_Info1(Value);
    }
    else
        Skip_XX(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::MCALabelDictionaryID()
{
    //Parsing
    int128u Value;
    Get_UL   (Value,                                            "Value", NULL); Element_Info1(Ztring().From_UUID(Value));

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCALabelDictionaryID=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCATagSymbol()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCATagSymbol=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCATagName()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCATagName=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GroupOfSoundfieldGroupsLinkID()
{
    if (Length2==0)
        return;

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID(Data,                                          "Value");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::MCALinkID()
{
    //Parsing
    int128u Value;
    Get_UUID (Value,                                            "Value"); Element_Info1(Ztring().From_UUID(Value));

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCALinkID=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundfieldGroupLinkID()
{
    //Parsing
    int128u Value;
    Get_UUID (Value,                                            "Value"); Element_Info1(Ztring().From_UUID(Value));

    FILLING_BEGIN();
        Descriptors[InstanceUID].SoundfieldGroupLinkID=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAPartitionKind()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCAPartitionKind=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAPartitionNumber()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCAPartitionNumber=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCATitle()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCATitle=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCATitleVersion()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCATitleVersion=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCATitleSubVersion()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCATitleSubVersion=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAEpisode()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCAEpisode=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAAudioContentKind()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCAAudioContentKind=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MCAAudioElementKind()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptors[InstanceUID].MCAAudioElementKind=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::TextDataDescription()
{
    //Parsing
    Ztring Value;
    Get_UTF16B (Length2, Value,                                 "Value"); Element_Info1(Value);

    if (Value.find(__T(".dolby.com/"))!=string::npos && Value.find(__T(' '))==string::npos) // found in some Dolby Vision files
        Descriptors[InstanceUID].Infos["CodecID"]=Value;
}

//---------------------------------------------------------------------------
void File_Mxf::Filler()
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (Padding_Trace_Count<MaxCountSameElementInTrace || (IsParsingMiddle_MaxOffset==(int64u)-1 && Partitions_IsFooter))
            {
                if (!Essences.empty()) //Only after first essence data or in footer
                    Padding_Trace_Count++;
            }
            else
            {
                Element_Set_Remove_Children_IfNoErrors();
                Element_Begin0(); //TODO: Element_Set_Remove_Children_IfNoErrors does not work if there is not sub-element
                Element_End0();
            }
        }
    #endif // MEDIAINFO_TRACE

    Skip_XX(Element_Size,                                       "Junk");

    Buffer_PaddingBytes+=Element_Size;
    DataMustAlwaysBeComplete=true;
}

//---------------------------------------------------------------------------
void File_Mxf::TerminatingFillerData()
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (Padding_Trace_Count<MaxCountSameElementInTrace || Partitions_IsFooter)
            {
                if (!Essences.empty()) //Only after first essence data or in footer
                    Padding_Trace_Count++;
            }
            else
            {
                Element_Set_Remove_Children_IfNoErrors();
                Element_Begin0(); //TODO: Element_Set_Remove_Children_IfNoErrors does not work if there is not sub-element
                Element_End0();
            }
        }
    #endif // MEDIAINFO_TRACE

    Skip_XX(Element_Size,                                       "Junk");

    Buffer_PaddingBytes+=Element_Size;
}

//---------------------------------------------------------------------------
void File_Mxf::XMLDocumentText_Indirect()
{
    Skip_XX(Element_Size,                                       "XML data");
}

//---------------------------------------------------------------------------
void File_Mxf::SubDescriptors()
{
    Descriptors[InstanceUID].SubDescriptors.clear();

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID(Data,                                          "Sub Descriptor");

        FILLING_BEGIN();
            Descriptors[InstanceUID].SubDescriptors.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata()
{
    if (AcquisitionMetadataLists.empty())
        AcquisitionMetadataLists.resize(0x10000);

    ELEMENT_BEGIN()
    ELEMENT(8000, LensUnitAcquisitionMetadata_IrisFNumber)
    ELEMENT(8001, LensUnitAcquisitionMetadata_FocusPositionFromImagePlane)
    ELEMENT(8002, LensUnitAcquisitionMetadata_FocusPositionFromFrontLensVertex)
    ELEMENT(8003, LensUnitAcquisitionMetadata_MacroSetting)
    ELEMENT(8004, LensUnitAcquisitionMetadata_LensZoom35mmStillCameraEquivalent)
    ELEMENT(8005, LensUnitAcquisitionMetadata_LensZoomActualFocalLength)
    ELEMENT(8006, LensUnitAcquisitionMetadata_OpticalExtenderMagnification)
    ELEMENT(8007, LensUnitAcquisitionMetadata_LensAttributes)
    ELEMENT(8008, LensUnitAcquisitionMetadata_IrisTNumber)
    ELEMENT(8009, LensUnitAcquisitionMetadata_IrisRingPosition)
    ELEMENT(800A, LensUnitAcquisitionMetadata_FocusRingPosition)
    ELEMENT(800B, LensUnitAcquisitionMetadata_ZoomRingPosition)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata()
{
    if (AcquisitionMetadataLists.empty())
        AcquisitionMetadataLists.resize(0x10000);

    ELEMENT_BEGIN()
    ELEMENT(3210, CameraUnitAcquisitionMetadata_TransferCharacteristic)
    ELEMENT(3219, CameraUnitAcquisitionMetadata_ColorPrimaries)
    ELEMENT(321A, CameraUnitAcquisitionMetadata_CodingEquations)
    ELEMENT(8100, CameraUnitAcquisitionMetadata_AutoExposureMode)
    ELEMENT(8101, CameraUnitAcquisitionMetadata_AutoFocusSensingAreaSetting)
    ELEMENT(8102, CameraUnitAcquisitionMetadata_ColorCorrectionFilterWheelSetting)
    ELEMENT(8103, CameraUnitAcquisitionMetadata_NeutralDensityFilterWheelSetting)
    ELEMENT(8104, CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveWidth)
    ELEMENT(8105, CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveHeight)
    ELEMENT(8106, CameraUnitAcquisitionMetadata_CaptureFrameRate)
    ELEMENT(8107, CameraUnitAcquisitionMetadata_ImageSensorReadoutMode)
    ELEMENT(8108, CameraUnitAcquisitionMetadata_ShutterSpeed_Angle)
    ELEMENT(8109, CameraUnitAcquisitionMetadata_ShutterSpeed_Time)
    ELEMENT(810A, CameraUnitAcquisitionMetadata_CameraMasterGainAdjustment)
    ELEMENT(810B, CameraUnitAcquisitionMetadata_ISOSensitivity)
    ELEMENT(810C, CameraUnitAcquisitionMetadata_ElectricalExtenderMagnification)
    ELEMENT(810D, CameraUnitAcquisitionMetadata_AutoWhiteBalanceMode)
    ELEMENT(810E, CameraUnitAcquisitionMetadata_WhiteBalance)
    ELEMENT(810F, CameraUnitAcquisitionMetadata_CameraMasterBlackLevel)
    ELEMENT(8110, CameraUnitAcquisitionMetadata_CameraKneePoint)
    ELEMENT(8111, CameraUnitAcquisitionMetadata_CameraKneeSlope)
    ELEMENT(8112, CameraUnitAcquisitionMetadata_CameraLuminanceDynamicRange)
    ELEMENT(8113, CameraUnitAcquisitionMetadata_CameraSettingFileURI)
    ELEMENT(8114, CameraUnitAcquisitionMetadata_CameraAttributes)
    ELEMENT(8115, CameraUnitAcquisitionMetadata_ExposureIndexofPhotoMeter)
    ELEMENT(8116, CameraUnitAcquisitionMetadata_GammaForCDL)
    ELEMENT(8117, CameraUnitAcquisitionMetadata_ASC_CDL_V12)
    ELEMENT(8118, CameraUnitAcquisitionMetadata_ColorMatrix)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata()
{
    if (AcquisitionMetadataLists.empty())
    {
        AcquisitionMetadataLists.resize(0x10000);
        AcquisitionMetadata_Sony_CalibrationType = (int8u)-1;
    }

    ELEMENT_BEGIN()
    ELEMENT(E000, UserDefinedAcquisitionMetadata_UdamSetIdentifier)
    default:
    {
    if (UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony)
        switch(Code2)
        {
    ELEMENT(8007, LensUnitAcquisitionMetadata_LensAttributes)
    ELEMENT(E101, UserDefinedAcquisitionMetadata_Sony_E101) //Effective Marker Coverage
    ELEMENT(E102, UserDefinedAcquisitionMetadata_Sony_E102) //Effective Marker Aspect Ratio
    ELEMENT(E103, UserDefinedAcquisitionMetadata_Sony_E103) //Camera Process Discrimination Code
    ELEMENT(E104, UserDefinedAcquisitionMetadata_Sony_E104) //Rotary Shutter Mode
    ELEMENT(E105, UserDefinedAcquisitionMetadata_Sony_E105) //Raw Black Code Value
    ELEMENT(E106, UserDefinedAcquisitionMetadata_Sony_E106) //Raw Gray Code Value
    ELEMENT(E107, UserDefinedAcquisitionMetadata_Sony_E107) //Raw White Code Value
    ELEMENT(E109, UserDefinedAcquisitionMetadata_Sony_E109) //Monitoring Descriptions
    ELEMENT(E10B, UserDefinedAcquisitionMetadata_Sony_E10B) //Monitoring Base Curve
    ELEMENT(E201, UserDefinedAcquisitionMetadata_Sony_E201) //Cooke Protocol Binary Metadata
    ELEMENT(E202, UserDefinedAcquisitionMetadata_Sony_E202) //Cooke Protocol User Metadata
    ELEMENT(E203, UserDefinedAcquisitionMetadata_Sony_E203) //Cooke Protocol Calibration Type
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::SDTISystemMetadataPack() //SMPTE 385M + 326M
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (SDTI_SystemMetadataPack_Trace_Count<MaxCountSameElementInTrace)
                SDTI_SystemMetadataPack_Trace_Count++;
            else
                Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE

    //Info for SDTI in Index StreamOffset
    if (!SDTI_IsPresent)
    {
        if (!Partitions.empty() && File_Offset+Buffer_Offset<Partitions[Partitions_Pos].StreamOffset+Partitions[Partitions_Pos].BodyOffset)
            SDTI_IsInIndexStreamOffset=false;

        SDTI_IsPresent=true;
    }

    //Parsing
    int8u SMB, CPR_Rate, Format;
    bool SMB_UL_Present, SMB_CreationTimeStamp, SMB_UserTimeStamp, CPR_1_1001;
    Get_B1 (SMB,                                                "System Metadata Bitmap");
        Skip_Flags(SMB, 7,                                      "FEC Active");
        Get_Flags (SMB, 6, SMB_UL_Present,                      "SMPTE Label");
        Get_Flags (SMB, 5, SMB_CreationTimeStamp,               "Creation Date/Time");
        Get_Flags (SMB, 4, SMB_UserTimeStamp,                   "User Date/Time");
        Skip_Flags(SMB, 3,                                      "Picture item");
        Skip_Flags(SMB, 2,                                      "Sound item");
        Skip_Flags(SMB, 1,                                      "Data item");
        Skip_Flags(SMB, 0,                                      "Control item");
    BS_Begin();
    Element_Begin1("Content Package Rate");
    Skip_S1(2,                                                  "Reserved");
    Get_S1 (5, CPR_Rate,                                        "Package Rate"); //See SMPTE 326M
    Get_SB (   CPR_1_1001,                                      "1.001 Flag");
    Element_End0();
    Element_Begin1("Content Package Type");
    Skip_S1(3,                                                  "Stream Status");
    Skip_SB(                                                    "Sub-package flag");
    Skip_SB(                                                    "Transfer Mode");
    Skip_S1(3,                                                  "Timing Mode");
    Element_End0();
    BS_End();
    Skip_B2(                                                    "channel handle");
    Skip_B2(                                                    "continuity count");

    //Some computing
    static int8u FrameRates_List[3] = { 24, 25, 30 };
    int8u  FrameRate;
    int8u  RepetitionMaxCount;
    if (CPR_Rate && CPR_Rate<=0x0C) //See SMPTE 326M)
    {
        CPR_Rate--;
        RepetitionMaxCount=CPR_Rate/3;
        FrameRate=FrameRates_List[CPR_Rate%3]*(RepetitionMaxCount+1);
    }
    else
    {
        FrameRate=0;
        RepetitionMaxCount=0;
    }

    //Parsing
    if (SMB_UL_Present)
        Skip_UL(                                                "SMPTE Universal label");
    if (SMB_CreationTimeStamp)
    {
        Get_B1 (Format,                                         "Format"); //0x81=timecode, 0x82=date-timecode
        Skip_B8(                                                "Time stamp");
        Skip_B8(                                                "Zero");
    }
    else
        Skip_XX(17,                                             "Junk");
    if (SMB_UserTimeStamp)
    {
        Get_B1 (Format,                                         "Format"); //0x81=timecode, 0x82=date-timecode, SMPTE 331M
        Element_Begin1("TimeCode");
        int8u Frames_Units, Frames_Tens, Seconds_Units, Seconds_Tens, Minutes_Units, Minutes_Tens, Hours_Units, Hours_Tens;
        bool  DropFrame;
        BS_Begin();

        #if MEDIAINFO_ADVANCED
            int32u BG;
            bool ColorFrame, FieldPhaseBgf0, Bgf0Bgf2, Bgf1, Bgf2FieldPhase;
            #define Get_TCB(_1,_2)      Get_SB(_1,_2)
            #define Get_TC4(_1,_2)      Get_L4(_1,_2)
        #else
            #define Get_TCB(_1,_2)      Skip_SB(_2)
            #define Get_TC4(_1,_2)      Skip_L4(_2)
        #endif

        Get_TCB(   ColorFrame,                                  "CF - Color fame");
        Get_SB (   DropFrame,                                   "DP - Drop frame");
        Get_S1 (2, Frames_Tens,                                 "Frames (Tens)");
        Get_S1 (4, Frames_Units,                                "Frames (Units)");

        Get_TCB(   FieldPhaseBgf0,                              "FP - Field Phase / BGF0");
        Get_S1 (3, Seconds_Tens,                                "Seconds (Tens)");
        Get_S1 (4, Seconds_Units,                               "Seconds (Units)");

        Get_TCB(   Bgf0Bgf2,                                    "BGF0 / BGF2");
        Get_S1 (3, Minutes_Tens,                                "Minutes (Tens)");
        Get_S1 (4, Minutes_Units,                               "Minutes (Units)");

        Get_TCB(   Bgf2FieldPhase,                              "BGF2 / Field Phase");
        Get_TCB(   Bgf1,                                        "BGF1");
        Get_S1 (2, Hours_Tens,                                  "Hours (Tens)");
        Get_S1 (4, Hours_Units,                                 "Hours (Units)");

        BS_End();

        Get_TC4(   BG,                                          "BG");

        #undef Get_TCB
        #undef Get_TC4

        //TimeCode
        TimeCode TimeCode_Current(  Hours_Tens  *10+Hours_Units,
                                    Minutes_Tens*10+Minutes_Units,
                                    Seconds_Tens*10+Seconds_Units,
                                    (Frames_Tens *10+Frames_Units)*(RepetitionMaxCount+1),
                                    FrameRate-1,
                                    TimeCode::DropFrame(DropFrame).FPS1001(CPR_1_1001));
        TimeCode_Current.Set1001fps(CPR_1_1001);
        if (RepetitionMaxCount)
        {
            if (SDTI_TimeCode_Previous.IsSet() && TimeCode_Current==SDTI_TimeCode_Previous)
            {
                SDTI_TimeCode_RepetitionCount++;
                ++TimeCode_Current;
                if (!SDTI_TimeCode_StartTimecode.IsSet() && SDTI_TimeCode_RepetitionCount>=RepetitionMaxCount)
                    SDTI_TimeCode_StartTimecode=SDTI_TimeCode_Previous; //The first time code was the first one of the repetition sequence
            }
            else
            {
                if (!SDTI_TimeCode_StartTimecode.IsSet() && SDTI_TimeCode_Previous.IsSet())
                {
                    SDTI_TimeCode_StartTimecode=SDTI_TimeCode_Previous;
                    while(SDTI_TimeCode_RepetitionCount<RepetitionMaxCount)
                    {
                        ++SDTI_TimeCode_StartTimecode;
                        SDTI_TimeCode_RepetitionCount++;
                    }
                }
                SDTI_TimeCode_RepetitionCount=0;
                SDTI_TimeCode_Previous=TimeCode_Current;
            }
        }
        else if (!SDTI_TimeCode_StartTimecode.IsSet())
            SDTI_TimeCode_StartTimecode=TimeCode_Current;

        Element_Info1(Ztring().From_UTF8(TimeCode_Current.ToString().c_str()));
        Element_Level--;
        Element_Info1(Ztring().From_UTF8(TimeCode_Current.ToString().c_str()));
        Element_Level++;

        #if MEDIAINFO_ADVANCED
            if (Config->TimeCode_Dumps)
            {
                auto id=string("SDTI");
                auto& TimeCode_Dump=(*Config->TimeCode_Dumps)[id];
                if (TimeCode_Dump.List.empty())
                {
                    TimeCode_Dump.Attributes_First+=" id=\""+id+"\" format=\"smpte-st326\"";
                    if (FrameRate)
                    {
                        TimeCode_Dump.Attributes_First+=" frame_rate=\""+to_string(FrameRate);
                        if (CPR_1_1001)
                            TimeCode_Dump.Attributes_First+="000/1001";
                        TimeCode_Dump.Attributes_First+='\"';
                    }
                    TimeCode_Dump.Attributes_Last+=" fp=\"0\" bgf=\"0\" bg=\"0\"";
                }
                TimeCode CurrentTC(Hours_Tens*10+Hours_Units, Minutes_Tens*10+Minutes_Units, Seconds_Tens*10+Seconds_Units, Frames_Tens*10+Frames_Units, TimeCode_Dump.FramesMax, TimeCode::DropFrame(DropFrame).FPS1001(CPR_1_1001));
                TimeCode_Dump.List+="    <tc v=\""+CurrentTC.ToString()+'\"';
                if (FieldPhaseBgf0)
                    TimeCode_Dump.List+=" fp=\"1\"";
                if (Bgf0Bgf2 || Bgf1 || Bgf2FieldPhase)
                    TimeCode_Dump.List+=" bgf=\""+to_string((Bgf2FieldPhase<<2)|(Bgf1<<1)|(Bgf2FieldPhase<<0))+"\"";
                if (BG)
                    TimeCode_Dump.List+=" bgf=\""+to_string(BG)+"\"";
                if (TimeCode_Dump.LastTC.IsValid() && TimeCode_Dump.LastTC.GetFramesMax() && CurrentTC!=TimeCode_Dump.LastTC+1)
                    TimeCode_Dump.List+=" nc=\"1\"";
                TimeCode_Dump.LastTC=CurrentTC;
                TimeCode_Dump.List+="/>\n";
                TimeCode_Dump.FrameCount++;
            }
        #endif //MEDIAINFO_ADVANCED

        Element_End0();

        Skip_B8(                                            "Zero");
    }
    else
        Skip_XX(17,                                             "Junk");

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;

    FILLING_BEGIN_PRECISE();
        if (!Status[IsAccepted])
            Accept();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SDTIPackageMetadataSet()
{
    #if MEDIAINFO_TRACE
        if (Trace_Activated)
        {
            if (SDTI_PackageMetadataSet_Trace_Count<MaxCountSameElementInTrace)
                SDTI_PackageMetadataSet_Trace_Count++;
            else
                Element_Set_Remove_Children_IfNoErrors();
        }
    #endif // MEDIAINFO_TRACE

    while (Element_Offset<Element_Size)
    {
        //Parsing
        Element_Begin1("Item");
        int128u Tag;
        int16u Length;
        int8u Type;
        Get_B1 (Type,                                            "Type");
        Get_B2 (Length,                                         "Length");
        int64u End=Element_Offset+Length;
        Get_UL (Tag,                                            "Tag", NULL); //TODO: check 10-byte UL with out_imx.mxf
        switch (Type)
        {
            case 0x83 : //UMID
                        {
                            Skip_UMID(                          );
                            if (Element_Offset<End)
                                Skip_UL  (                      "Zeroes");
                        }
                        break;
            case 0x88 : //KLV Metadata
                        {
                            while (Element_Offset<End)
                            {
                                int64u Length;
                                Get_BER(Length,                 "Length");
                                switch ((Tag.lo>>16)&0xFF)
                                {
                                    case 0x00 : Skip_UTF8(Length,"Data"); break;
                                    case 0x01 : Skip_UTF16L(Length,"Data"); break;
                                    default   : Skip_XX(Length, "Data");
                                }
                            }
                        }
                        break;
            default   : Skip_XX(Length,                         "Unknown");
        }
        Element_End0();
    }

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;

    FILLING_BEGIN_PRECISE();
        if (!Status[IsAccepted])
            Accept();
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SDTIPictureMetadataSet()
{
    Skip_XX(Element_Size,                                       "Data");

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;
}

//---------------------------------------------------------------------------
void File_Mxf::SDTISoundMetadataSet()
{
    Skip_XX(Element_Size,                                       "Data");

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;
}

//---------------------------------------------------------------------------
void File_Mxf::SDTIDataMetadataSet()
{
    Skip_XX(Element_Size,                                       "Data");

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;
}

//---------------------------------------------------------------------------
void File_Mxf::SDTIControlMetadataSet()
{
    Skip_XX(Element_Size,                                       "Data");

    //Filling
    if (SDTI_SizePerFrame==0)
        Partitions_IsCalculatingSdtiByteCount=true;
}

//---------------------------------------------------------------------------
void File_Mxf::PHDRImageMetadataItem()
{
    //Parsing
    Skip_String(Element_Size,                                   "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ISXDDataEssenceDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(DolbyNamespaceURI)
    ELEMENT_END()

    DataEssenceDescriptor();
}

//---------------------------------------------------------------------------
void File_Mxf::PHDRMetadataTrackSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(PHDRDataDefinition)
    ELEM____UUID_(PHDRSourceTrackID)
    ELEM____UUID_(PHDRSimplePayloadSID)
    ELEMENT_END()
    GenerationInterchangeObject();

    if (Descriptors[InstanceUID].StreamKind==Stream_Max)
    {
        Descriptors[InstanceUID].StreamKind=Stream_Other;
        if (Streams_Count==(size_t)-1)
            Streams_Count=0;
        Streams_Count++;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::OmneonVideoNetworksDescriptiveMetadataLinks()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(OmneonVideoNetworksDescriptiveMetadataItems)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::OmneonVideoNetworksDescriptiveMetadataData()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(ItemName_ISO7)
    ELEM____UUID_(ItemName)
    ELEM____UUID_(ItemValue_ISO7)
    ELEM____UUID_(ItemValue)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::OmneonVideoNetworksDescriptiveMetadataItems()
{
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID(Data,                                          "Package");

        //Filling
        DMOmneonLinks[InstanceUID].Links.push_back(Data);
    }
}

//---------------------------------------------------------------------------
void File_Mxf::HdrVividMetadataTrackSubDescriptor()
{
    ELEMENT_BEGIN()
    ELEMENT_MIDDLE()
    ELEM____UUID_(HdrVividDataDefinition)
    ELEM____UUID_(HdrVividSourceTrackID)
    ELEM____UUID_(HdrVividSimplePayloadSID)
    ELEMENT_END()
    GenerationInterchangeObject();
}

//---------------------------------------------------------------------------
void File_Mxf::HdrVividDataDefinition()
{
    Info_UL(Data,                                               "Data", nullptr); Element_Info1(Ztring().From_UUID(Data));
}

//---------------------------------------------------------------------------
void File_Mxf::HdrVividSourceTrackID()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::HdrVividSimplePayloadSID()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::HdrVividMetadataItem()
{
    Skip_XX(Element_Size,                                       "HDR Vivid data");
}

//---------------------------------------------------------------------------
void File_Mxf::TimelineTrack()
{
    ELEMENT_BEGIN()
    ELEMENT(4B01, Track_EditRate)
    ELEMENT(4B02, Track_Origin)
    ELEMENT_MIDDLE()
    ELEMENT_END()
    GenericTrack();

    if (Code2==0x3C0A)
    {
        for (packages::iterator Package=Packages.begin(); Package!=Packages.end(); ++Package)
        {
            if (Package->first==Prefaces[Preface_Current].PrimaryPackage) //InstanceIUD
            {
                Element_Level--;
                Element_Info1("Primary package");
                Element_Level++;
            }
            for (size_t Pos=0; Pos<Package->second.Tracks.size(); Pos++)
                if (InstanceUID==Package->second.Tracks[Pos])
                {
                    Element_Level--;
                    Element_Info1("Valid from Package");
                    Element_Level++;
                }
        }
    }
}

//***************************************************************************
// Base
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_AuxBitsMode()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_Emphasis()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_BlockStartOffset()
{
    //Parsing
    Info_B2(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_ChannelStatusMode()
{
    //Parsing
    Skip_XX(Length2,                                            "Batch");
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_FixedChannelStatusData()
{
    //Parsing
    Skip_XX(Length2,                                           "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_UserDataMode()
{
    //Parsing
    Skip_XX(Length2,                                           "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::AES3PCMDescriptor_FixedUserData()
{
    //Parsing
    Skip_XX(Length2,                                           "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_ComponentDepth()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data)
            Descriptor_Fill("BitDepth", Ztring().From_Number(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_HorizontalSubsampling()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].SubSampling_Horizontal=Data;
        Subsampling_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_ColorSiting()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_BlackRefLevel()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].MinRefLevel==(int32u)-1)
            Descriptors[InstanceUID].MinRefLevel=Data;
        ColorLevels_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_WhiteReflevel()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].MaxRefLevel==(int32u)-1)
            Descriptors[InstanceUID].MaxRefLevel=Data;
        ColorLevels_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_ColorRange()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].ColorRange==(int32u)-1)
            Descriptors[InstanceUID].ColorRange=Data;
        ColorLevels_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_PaddingBits()
{
    //Parsing
    Info_B2(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_VerticalSubsampling()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].SubSampling_Vertical=Data;
        Subsampling_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_AlphaSampleDepth()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::CDCIDescriptor_ReversedByteOrder()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::ContentStorage_Packages()
{
    ContentStorages[InstanceUID].Packages.clear();

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID(Data,                                          "Package");

        FILLING_BEGIN();
            Element_Info1C((Data==Prefaces[Preface_Current].PrimaryPackage), "Primary package");
            ContentStorages[InstanceUID].Packages.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::ContentStorage_EssenceData()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        Skip_UUID(                                              "EssenceContainer");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::DescriptiveMarker_Duration()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                               "Data"); Element_Info1(Data); //units of edit rate

    DescriptiveMarkers[InstanceUID].Duration=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::DescriptiveMarker_DMFramework()
{
    //Parsing
    int128u Data;
    Get_UUID(Data,                                             "DM Framework"); Element_Info1(Ztring().From_UUID(Data));

    DescriptiveMarkers[InstanceUID].Framework=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::DescriptiveMarker_TrackIDs()
{
    //Parsing
    VECTOR(4);
    for (int32u i=0; i<Count; i++)
    {
        int32u Data;
        Get_B4 (Data,                                           "Track ID");

        FILLING_BEGIN();
            DescriptiveMarkers[InstanceUID].TrackIDs.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::EssenceData_LinkedPackageUID()
{
    //Parsing
    Skip_UMID();
}

//---------------------------------------------------------------------------
void File_Mxf::EssenceData_IndexSID()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::EssenceData_BodySID()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::EventTrack_EventEditRate()
{
    //Parsing
    Info_Rational();
}

//---------------------------------------------------------------------------
void File_Mxf::EventTrack_EventOrigin()
{
    //Parsing
    Info_B8(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor_SampleRate()
{
    Get_Rational(Descriptors[InstanceUID].SampleRate); Element_Info1(Descriptors[InstanceUID].SampleRate);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].SampleRate && Descriptors[InstanceUID].Duration!=(int64u)-1)
            Descriptor_Fill("Duration", Ztring().From_Number(Descriptors[InstanceUID].Duration/Descriptors[InstanceUID].SampleRate*1000, 0));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor_ContainerDuration()
{
    int64u Data;
    Get_B8 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data)
        {
            Descriptors[InstanceUID].Duration=Data;
            if (Descriptors[InstanceUID].SampleRate && Descriptors[InstanceUID].Duration!=(int64u)-1)
                Descriptors[InstanceUID].Infos["Duration"].From_Number(Descriptors[InstanceUID].Duration/Descriptors[InstanceUID].SampleRate*1000, 0);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor_EssenceContainer()
{
    //Parsing
    int128u EssenceContainer;
    Get_UL (EssenceContainer,                                   "EssenceContainer", Mxf_EssenceContainer); Element_Info1(Mxf_EssenceContainer(EssenceContainer));

    FILLING_BEGIN();
        int8u Code6=(int8u)((EssenceContainer.lo&0x0000000000FF0000LL)>>16);
        int8u Code7=(int8u)((EssenceContainer.lo&0x000000000000FF00LL)>> 8);
        int8u Code8=(int8u)((EssenceContainer.lo&0x00000000000000FFLL)    );

        Descriptors[InstanceUID].EssenceContainer=EssenceContainer;
        Descriptor_Fill("Format_Settings_Wrapping", Mxf_EssenceContainer_Mapping(EssenceContainer.lo));

        if (!DataMustAlwaysBeComplete && Descriptors[InstanceUID].Infos["Format_Settings_Wrapping"].find(__T("Frame"))!=string::npos)
            DataMustAlwaysBeComplete=true;

        if (Code6==0x0C)
            Descriptors[InstanceUID].Jp2kContentKind=Code7;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor_Codec()
{
    //Parsing
    Skip_UL(                                                    "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::FileDescriptor_LinkedTrackID()
{
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].LinkedTrackID==(int32u)-1)
            Descriptors[InstanceUID].LinkedTrackID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::InterchangeObject_InstanceUID()
{
    //Parsing
    Get_UUID(InstanceUID,                                       "UUID"); Element_Info1(Ztring().From_UUID(InstanceUID));

    FILLING_BEGIN();
        //Putting the right UID for already parsed items
        prefaces::iterator Preface=Prefaces.find(0);
        if (Preface!=Prefaces.end())
        {
            Prefaces[InstanceUID]=Preface->second;
            Prefaces.erase(Preface);
        }
        identifications::iterator Identification=Identifications.find(0);
        if (Identification!=Identifications.end())
        {
            Identifications[InstanceUID]=Identification->second;
            Identifications.erase(Identification);
        }
        contentstorages::iterator ContentStorage=ContentStorages.find(0);
        if (ContentStorage!=ContentStorages.end())
        {
            ContentStorages[InstanceUID]=ContentStorage->second;
            ContentStorages.erase(ContentStorage);
        }
        packages::iterator Package=Packages.find(0);
        if (Package!=Packages.end())
        {
            Packages[InstanceUID]=Package->second;
            Packages.erase(Package);
        }
        tracks::iterator Track=Tracks.find(0);
        if (Track!=Tracks.end())
        {
            Tracks[InstanceUID]=Track->second;
            Tracks.erase(Track);
        }
        descriptors::iterator Descriptor=Descriptors.find(0);
        if (Descriptor!=Descriptors.end())
        {
            descriptors::iterator Descriptor_Previous=Descriptors.find(InstanceUID);
            if (Descriptor_Previous!=Descriptors.end())
            {
                //Merging
                Descriptor->second.Infos.insert(Descriptor_Previous->second.Infos.begin(), Descriptor_Previous->second.Infos.end()); //TODO: better implementation
            }
            for (std::map<std::string, Ztring>::iterator Info = Descriptor->second.Infos.begin(); Info != Descriptor->second.Infos.end(); ++Info) //Note: can not be mapped directly because there are some tests done in Descriptor_Fill
                Descriptor_Fill(Info->first.c_str(), Info->second);
            std::map<std::string, Ztring> Infos_Temp=Descriptors[InstanceUID].Infos; //Quick method for copying the whole descriptor without erasing the modifications made by Descriptor_Fill(). TODO: a better method in order to be more generic
            Descriptors[InstanceUID]=Descriptor->second;
            Descriptors[InstanceUID].Infos=Infos_Temp;
            Descriptor->second.Parser=nullptr; //TODO: safer move from one key to another
            Descriptors.erase(Descriptor);
        }
        locators::iterator Locator=Locators.find(0);
        if (Locator!=Locators.end())
        {
            Locators[InstanceUID]=Locator->second;
            Locators.erase(Locator);
        }
        components::iterator Component=Components.find(0);
        if (Component!=Components.end())
        {
            Components[InstanceUID].Update(Component->second);
            Components.erase(Component);
        }
        dmsegments::iterator DescriptiveMarker=DescriptiveMarkers.find(0);
        if (DescriptiveMarker!=DescriptiveMarkers.end())
        {
            DescriptiveMarkers[InstanceUID]=DescriptiveMarker->second;
            DescriptiveMarkers.erase(DescriptiveMarker);
        }
        as11s::iterator AS11=AS11s.find(0);
        if (AS11!=AS11s.end())
        {
            AS11s[InstanceUID]=AS11->second;
            AS11s.erase(AS11);
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenerationInterchangeObject_GenerationUID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::GenericDescriptor_Locators()
{
    Descriptors[InstanceUID].Locators.clear();

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("Locator");
        int128u UUID;
        Get_UUID(UUID,                                          "UUID");

        FILLING_BEGIN();
            Descriptors[InstanceUID].Locators.push_back(UUID);
        FILLING_END();

        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage_PackageUID()
{
    //Parsing
    int256u Data;
    Get_UMID (Data,                                             "PackageUID");

    FILLING_BEGIN();
        Packages[InstanceUID].PackageUID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage_Name()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                   "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Essences.empty() && Data!=Retrieve(Stream_General, 0, General_PackageName))
            Fill(Stream_General, 0, General_PackageName, Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage_Tracks()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID(Data,                                          "Track");

        FILLING_BEGIN();
            Packages[InstanceUID].Tracks.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage_PackageModifiedDate()
{
    //Parsing
    Info_Timestamp();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericPackage_PackageCreationDate()
{
    //Parsing
    Info_Timestamp();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_PictureEssenceCoding()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_EssenceCompression); Element_Info1(Mxf_EssenceCompression(Data));

    FILLING_BEGIN();
        Descriptors[InstanceUID].EssenceCompression=Data;
        Descriptors[InstanceUID].StreamKind=Stream_Video;
        Descriptor_Fill("Format", Mxf_EssenceCompression(Data));
        Descriptor_Fill("Format_Version", Mxf_EssenceCompression_Version(Data));
        Descriptor_Fill("Format_Profile", Mxf_EssenceCompression_Profile(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_StoredHeight()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].Height==(int32u)-1)
        {
            if (Descriptors[InstanceUID].Is_Interlaced())
                Data*=2; //This is per field
            if (Descriptors[InstanceUID].Height==(int32u)-1)
                Descriptors[InstanceUID].Height=Data;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_StoredWidth()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].Width==(int32u)-1)
        {
                Descriptors[InstanceUID].Width=Data;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_SampledHeight()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].Is_Interlaced())
            Data*=2; //This is per field
        Descriptors[InstanceUID].Height=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_SampledWidth()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].Width=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_SampledXOffset()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_SampledYOffset()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_DisplayHeight()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].Is_Interlaced())
            Data*=2; //This is per field
        Descriptors[InstanceUID].Height_Display=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_DisplayWidth()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].Width_Display=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_DisplayXOffset()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].Width_Display_Offset=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_DisplayYOffset()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].Is_Interlaced())
            Data*=2; //This is per field
        Descriptors[InstanceUID].Height_Display_Offset=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_FrameLayout()
{
    //Parsing
    int8u Data;
    Get_B1 (Data,                                               "Data"); Element_Info1(Data); Param_Info1(Mxf_FrameLayout(Data)); Element_Info1(Mxf_FrameLayout(Data));

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].ScanType.empty())
        {
            if (Descriptors[InstanceUID].Height!=(int32u)-1) Descriptors[InstanceUID].Height*=Mxf_FrameLayout_Multiplier(Data);
            if (Descriptors[InstanceUID].Height_Display!=(int32u)-1) Descriptors[InstanceUID].Height_Display*=Mxf_FrameLayout_Multiplier(Data);
            if (Descriptors[InstanceUID].Height_Display_Offset!=(int32u)-1) Descriptors[InstanceUID].Height_Display_Offset*=Mxf_FrameLayout_Multiplier(Data);
        }
        if (Descriptors[InstanceUID].ScanType.empty() || !Partitions_IsFooter)
            Descriptors[InstanceUID].ScanType.From_UTF8(Mxf_FrameLayout_ScanType(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_VideoLineMap()
{
    int64u VideoLineMapEntries_Total=0;
    bool   VideoLineMapEntry_IsZero=false;

    //Parsing
    VECTOR(4);
    for (int32u i=0; i<Count; i++)
    {
        int32u VideoLineMapEntry;
        Get_B4 (VideoLineMapEntry,                              "VideoLineMapEntry");

        if (VideoLineMapEntry)
            VideoLineMapEntries_Total+=VideoLineMapEntry;
        else
            VideoLineMapEntry_IsZero=true;
    }

    FILLING_BEGIN();
        // Cryptic formula:
        //    odd odd field 2 upper
        //    odd even field 1 upper
        //    even odd field 1 upper
        //    even even field 2 upper
        if (Count==2 && !VideoLineMapEntry_IsZero) //2 values
            Descriptors[InstanceUID].FieldTopness=(VideoLineMapEntries_Total%2)?1:2;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_AspectRatio()
{
    //Parsing
    float64 Data;
    Get_Rational(Data);

    FILLING_BEGIN();
        if (Data)
        {
            Descriptors[InstanceUID].DisplayAspectRatio=Data;
            Descriptor_Fill("DisplayAspectRatio", Ztring().From_Number(Data, 3));
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_AlphaTransparency()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_TransferCharacteristic()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", NULL); Element_Info1(Mxf_TransferCharacteristic(Data));

    FILLING_BEGIN();
        Descriptor_Fill("transfer_characteristics", Ztring().From_UTF8(Mxf_TransferCharacteristic(Data)));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_ImageAlignmentOffset()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_FieldDominance()
{
    //Parsing
    int8u Data;
    Get_B1 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].FieldDominance=Data;
    FILLING_END();
    //Parsing
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_ImageStartOffset()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_ImageEndOffset()
{
    //Parsing
    Info_B4(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_SignalStandard()
{
    //Parsing
    Info_B1(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_StoredF2Offset()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_DisplayF2Offset()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_ActiveFormatDescriptor()
{
    //Parsing
    int8u Data;
    bool Is1dot3=Retrieve(Stream_General, 0, General_Format_Version).To_float32()>=1.3?true:false;
    if (!Is1dot3 && Element_Size && Buffer[(size_t)(Buffer_Offset+Element_Offset)]&0x60)
        Is1dot3=true;

    BS_Begin();
    if (Is1dot3)
    {
        Skip_SB(                                                    "Reserved");
        Get_S1 (4, Data,                                            "Data"); Element_Info1C((Data<16), AfdBarData_active_format[Data]);
        Skip_SB(                                                    "AR");
        Skip_S1(2,                                                  "Reserved");
    }
    else
    {
        Skip_S1(3,                                                  "Reserved");
        Get_S1 (4, Data,                                            "Data"); Element_Info1C((Data<16), AfdBarData_active_format[Data]);
        Skip_SB(                                                    "AR");
    }
    BS_End();

    FILLING_BEGIN();
        Descriptors[InstanceUID].ActiveFormat=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_ColorPrimaries()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_ColorPrimaries);  Element_Info1(Mxf_ColorPrimaries(Data));

    FILLING_BEGIN();
        Descriptor_Fill("colour_primaries", Mxf_ColorPrimaries(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PictureDescriptor_CodingEquations()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_CodingEquations);  Element_Info1(Mxf_CodingEquations(Data));

    FILLING_BEGIN();
        Descriptor_Fill("matrix_coefficients", Mxf_CodingEquations(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MasteringDisplayPrimaries()
{
    //Parsing
    int16u x[3];
    int16u y[3];
    for (size_t c = 0; c < 3; c++)
    {
        Get_B2(x[c],                                            "display_primaries_x");
        Get_B2(y[c],                                            "display_primaries_y");
    }

    FILLING_BEGIN();
        ZtringList MasteringDisplay_ColorPrimaries;
        for (size_t c = 0; c < 3; c++)
        {
            MasteringDisplay_ColorPrimaries.push_back(Ztring::ToZtring(x[c]));
            MasteringDisplay_ColorPrimaries.push_back(Ztring::ToZtring(y[c]));
        }
        Descriptor_Fill("MasteringDisplay_Primaries", MasteringDisplay_ColorPrimaries.Read());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MasteringDisplayWhitePointChromaticity()
{
    int16u x;
    int16u y;
    Get_B2(x,                                                   "white_point_x");
    Get_B2(y,                                                   "white_point_y");

    FILLING_BEGIN();
        ZtringList MasteringDisplay_WhitePointChromaticity;
        MasteringDisplay_WhitePointChromaticity.push_back(Ztring::ToZtring(x));
        MasteringDisplay_WhitePointChromaticity.push_back(Ztring::ToZtring(y));
        Descriptor_Fill("MasteringDisplay_WhitePointChromaticity", MasteringDisplay_WhitePointChromaticity.Read());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MasteringDisplayMaximumLuminance()
{
    //Parsing
    int32u max;
    Get_B4 (max,                                                "Data");

    FILLING_BEGIN();
        Descriptor_Fill("MasteringDisplay_Luminance_Max", Ztring::ToZtring(max));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MasteringDisplayMinimumLuminance()
{
    //Parsing
    int32u min;
    Get_B4 (min,                                               "Data");

    FILLING_BEGIN();
        Descriptor_Fill("MasteringDisplay_Luminance_Min", Ztring::ToZtring(min));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::TextBasedObject()
{
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_QuantizationBits()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data)
        {
            Descriptor_Fill("BitDepth", Ztring().From_Number(Data));
            Descriptors[InstanceUID].QuantizationBits=Data;
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_Locked()
{
    //Parsing
    int8u Data;
    Get_B1 (Data,                                               "Data"); Element_Info1(Data?"Yes":"No");

    FILLING_BEGIN();
        Descriptor_Fill("Locked", Data?"Yes":"No");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_AudioSamplingRate()
{
    //Parsing
    float64 Data;
    Get_Rational(Data); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptor_Fill("SamplingRate", Ztring().From_Number(Data, 0));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_AudioRefLevel()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info2(Data, " dB");
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_ElectroSpatialFormulation()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data); //Enum
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_SoundEssenceCompression()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_EssenceCompression); Element_Info1(Mxf_EssenceCompression(Data));

    FILLING_BEGIN();
        Descriptors[InstanceUID].EssenceCompression=Data;
        Descriptors[InstanceUID].StreamKind=Stream_Audio;
        Descriptor_Fill("Format", Mxf_EssenceCompression(Data));
        Descriptor_Fill("Format_Version", Mxf_EssenceCompression_Version(Data));
        if ((Data.lo&0xFFFFFFFFFF000000LL)==0x040202017e000000LL)
            Descriptor_Fill("Format_Settings_Endianness", "Big");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_ChannelCount()
{
    //Parsing
    int32u Value;
    Get_B4 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        if (Value)
        {
            Descriptors[InstanceUID].ChannelCount=Value;
            Descriptor_Fill("Channel(s)", Ztring().From_Number(Value));
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SoundDescriptor_DialNorm()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info2(Data, " dB");
}

//---------------------------------------------------------------------------
void File_Mxf::DataEssenceDescriptor_DataEssenceCoding()
{
    //Parsing
    Skip_UL(                                                    "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::GenericTrack_TrackID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Tracks[InstanceUID].TrackID==(int32u)-1)
            Tracks[InstanceUID].TrackID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericTrack_TrackName()
{
    //Parsing
    Ztring Data;
    Get_UTF16B (Length2, Data,                                  "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Tracks[InstanceUID].TrackName=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericTrack_Sequence()
{
    //Parsing
    int128u Data;
    Get_UUID(Data,                                              "Data"); Element_Info1(uint128toString(Data, 16));

    FILLING_BEGIN();
        Tracks[InstanceUID].Sequence=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::GenericTrack_TrackNumber()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Ztring::ToZtring(Data, 16));

    FILLING_BEGIN();
        if (Tracks[InstanceUID].TrackNumber==(int32u)-1 || Data) // In some cases, TrackNumber is 0 for all track and we have replaced with the right value during the parsing
            Tracks[InstanceUID].TrackNumber=Data;
        Track_Number_IsAvailable=true;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_CompanyName()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                  "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Identifications[InstanceUID].CompanyName=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ProductName()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                  "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Identifications[InstanceUID].ProductName=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ProductVersion()
{
    //Parsing
    int16u Major, Minor, Patch, Build, Release;
    Get_B2 (Major,                                              "Major");
    Get_B2 (Minor,                                              "Minor");
    Get_B2 (Patch,                                              "Patch");
    Get_B2 (Build,                                              "Build");
    Get_B2 (Release,                                            "Release");
    Ztring Version=Ztring::ToZtring(Major)+__T('.')
                  +Ztring::ToZtring(Minor)+__T('.')
                  +Ztring::ToZtring(Patch)+__T('.')
                  +Ztring::ToZtring(Build)+__T('.')
                  +Ztring::ToZtring(Release)      ;
    Element_Info1(Version);

    FILLING_BEGIN();
        if (Major || Minor || Patch || Build || Release)
            Identifications[InstanceUID].ProductVersion=Version;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_VersionString()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                  "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Identifications[InstanceUID].VersionString=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ProductUID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ModificationDate()
{
    //Parsing
    Info_Timestamp();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ToolkitVersion()
{
    //Parsing
    int16u Major, Minor, Patch, Build, Release;
    Get_B2 (Major,                                              "Major");
    Get_B2 (Minor,                                              "Minor");
    Get_B2 (Patch,                                              "Patch");
    Get_B2 (Build,                                              "Build");
    if (Element_Size-Element_Offset==1)
    {
        int8u t;
        Get_B1 (t,                                              "Release"); Param_Error("Identification ToolkitVersion is 9 byte long (should be 10)");
        Release=t;
    }
    else
        Get_B2 (Release,                                        "Release");
    Ztring Version=Ztring::ToZtring(Major)+__T('.')
                  +Ztring::ToZtring(Minor)+__T('.')
                  +Ztring::ToZtring(Patch)+__T('.')
                  +Ztring::ToZtring(Build)+__T('.')
                  +Ztring::ToZtring(Release)      ;
    Element_Info1(Version);

    FILLING_BEGIN();
        if (Major || Minor || Patch || Build || Release)
            Identifications[InstanceUID].ToolkitVersion=Version;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_Platform()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                  "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data!=__T("Unknown"))
            Identifications[InstanceUID].Platform=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Identification_ThisGenerationUID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_EditUnitByteCount()
{
    //Parsing
    int32u Data;
    Get_B4(Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        #if MEDIAINFO_SEEK
            IndexTables[IndexTables.size()-1].EditUnitByteCount=Data;
        #endif //MEDIAINFO_SEEK
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_IndexSID()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_BodySID()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_SliceCount()
{
    //Parsing
    int8u Data;
    Get_B1(Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        IndexTable_NSL=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_DeltaEntryArray()
{
    //Parsing
    int32u NDE, Length;
    Get_B4(NDE,                                                 "NDE");
    Get_B4(Length,                                              "Length");
    for (int32u Pos=0; Pos<NDE; Pos++)
    {
        Element_Begin1("Delta Entry");
        Skip_B1(                                                "PosTableIndex");
        Skip_B1(                                                "Slice");
        Skip_B4(                                                "Element Delta");
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_IndexEntryArray()
{
    //Parsing
    int32u NIE, Length;
    Get_B4(NIE,                                                 "NIE");
    Get_B4(Length,                                              "Length");
    for (int32u Pos=0; Pos<NIE; Pos++)
    {
        #if MEDIAINFO_SEEK
            indextable::entry Entry;
            int64u Stream_Offset;
            bool   forward_rediction_flag, backward_prediction_flag;
        #endif //MEDIAINFO_SEEK
        int8u Flags;
        Element_Begin1("Index Entry");
        Skip_B1(                                                "Temporal Offset");
        Skip_B1(                                                "Key-Frame Offset");
        Get_B1 (Flags,                                          "Flags");
            Skip_Flags(Flags, 7,                                "Random Access");
            Skip_Flags(Flags, 6,                                "Sequence Header");
            #if MEDIAINFO_SEEK
                Get_Flags (Flags, 5, forward_rediction_flag,    "forward prediction flag");
                Get_Flags (Flags, 4, backward_prediction_flag,  "backward prediction flag");
            #else //MEDIAINFO_SEEK
                Skip_Flags(Flags, 5,                            "forward prediction flag");
                Skip_Flags(Flags, 4,                            "backward prediction flag");
            #endif //MEDIAINFO_SEEK
        #if MEDIAINFO_SEEK
            Get_B8 (Stream_Offset,                              "Stream Offset");
            Entry.StreamOffset=Stream_Offset;
            Entry.Type=(forward_rediction_flag?1:0)*2+(backward_prediction_flag?1:0);
            IndexTables[IndexTables.size()-1].Entries.push_back(Entry);
        #else //MEDIAINFO_SEEK
            Skip_B8(                                            "Stream Offset");
        #endif //MEDIAINFO_SEEK
        for (int32u NSL_Pos=0; NSL_Pos<IndexTable_NSL; NSL_Pos++)
            Skip_B4(                                            "SliceOffset");
        for (int32u NPE_Pos=0; NPE_Pos<IndexTable_NPE; NPE_Pos++)
            Skip_B4(                                            "PosTable");
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_IndexEditRate()
{
    //Parsing
    float64 Data;
    Get_Rational(Data);

    FILLING_BEGIN();
        #if MEDIAINFO_SEEK
            IndexTables[IndexTables.size()-1].IndexEditRate=Data;
        #endif //MEDIAINFO_SEEK
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_IndexStartPosition()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        #if MEDIAINFO_SEEK
            IndexTables[IndexTables.size()-1].IndexStartPosition=Data;

            //Integrity test (in one file, I have 2 indexes with IndexStartPosition=0, the first  one is weird (only one frame), or the same index is repeated
            //Integrity test (in one file, I have 2 indexes with IndexStartPosition=0, the second one is weird (only one frame), or the same index is repeated
            for (size_t Pos=0; Pos<IndexTables.size()-1; Pos++)
                if (IndexTables[Pos].IndexStartPosition==Data)
                {
                    if (IndexTables[Pos].IndexDuration==1 && Pos!=IndexTables.size()-1)
                        IndexTables.erase(IndexTables.begin()+Pos);
                    else
                    {
                        //Removed in order to get all indexes, even the duplicated ones (in order to check duplicated index in the footer)
                        //IndexTables.erase(IndexTables.begin()+IndexTables.size()-1);
                        //Element_Offset=Element_Size;
                    }

                    return;
                }
        #endif //MEDIAINFO_SEEK
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_IndexDuration()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        #if MEDIAINFO_SEEK
            IndexTables[IndexTables.size()-1].IndexDuration=Data;
        #endif //MEDIAINFO_SEEK
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_PosTableCount()
{
    //Parsing
    int8u Data;
    Get_B1(Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        IndexTable_NPE=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::IndexTableSegment_8002()
{
    //Parsing
    Info_B8(Data,                                               "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::Rsiz()
{
    #if MEDIAINFO_ADVANCED
        //Parsing
        int16u Data;
        Get_B2 (Data,                                            "Data"); Element_Info1(Data);

        FILLING_BEGIN();
            Descriptors[InstanceUID].Jpeg2000_Rsiz=Data;
        FILLING_END();
    #else //MEDIAINFO_ADVANCED
        Info_B2(Data,                                            "Data"); Element_Info1(Data);
    #endif //MEDIAINFO_ADVANCED
}

//---------------------------------------------------------------------------
void File_Mxf::Xsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::Ysiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::XOsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::YOsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::XTsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::YTsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::XTOsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::YTOsiz()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::Csiz()
{
    //Parsing
    int16u Data;
    Get_B2 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN()
        Descriptor_Fill("ComponentCount", Ztring::ToZtring(Data));
    FILLING_END()

}

//---------------------------------------------------------------------------
void File_Mxf::PictureComponentSizing()
{
    //Parsing
    VECTOR(3);
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("PictureComponentSize");
        Info_B1(Ssiz,                                           "Component sample precision"); Element_Info1(Ssiz);
        Info_B1(XRsiz,                                          "Horizontal separation of a sample"); Element_Info1(XRsiz);
        Info_B1(YRsiz,                                          "Vertical separation of a sample"); Element_Info1(YRsiz);
        Element_End0();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::CodingStyleDefault()
{
    Skip_XX(Length2,                                            "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::QuantizationDefault()
{
    Skip_XX(Length2,                                            "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1InitializationMetadata()
{
    #if defined(MEDIAINFO_FFV1_YES)
        File_Ffv1* Parser=new File_Ffv1;
        Open_Buffer_Init(Parser);
        Open_Buffer_OutOfBand(Parser);
        
        descriptor& Descriptor=Descriptors[InstanceUID];
        delete Descriptor.Parser;
        Descriptor.Parser=Parser;
    #else
        Skip_XX(Element_Size,                                   "Data");
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1IdenticalGOP()
{
    Skip_B1(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1MaxGOP()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1MaximumBitRate()
{
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1Version()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::FFV1MicroVersion()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkStreamID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
         ExtraMetadata_SID.insert(Data);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkID()
{
    Skip_C4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkUUID()
{
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkHashSHA1()
{
    Skip_B4(                                                    "Data");
    Skip_B4(                                                    "Data");
    Skip_B4(                                                    "Data");
    Skip_B4(                                                    "Data");
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkStreamIDsArray()
{
    VECTOR(4);
    for (int32u i=0; i<Count; i++)
        Skip_B4(                                                "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::NumLocalChannels()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::NumADMAudioTrackUIDs()
{
    Skip_B2(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMChannelMappingsArray()
{
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
        Skip_UUID(                                              "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::LocalChannelID()
{
    int32u Data;
    Get_B4(Data,                                                "Data");

    FILLING_BEGIN();
        #if defined(MEDIAINFO_ADM_YES)
            ADMChannelMapping_LocalChannelID=Data;
            ADMChannelMapping_Presence.set(0);
        #endif //defined(MEDIAINFO_ADM_YES)
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioTrackUID()
{
    Ztring Data;
    Get_UTF16B (Length2, Data,                                  "Data");

    FILLING_BEGIN();
        #if defined(MEDIAINFO_ADM_YES)
            ADMChannelMapping_ADMAudioTrackUID= Data.To_UTF8();
            ADMChannelMapping_Presence.set(1);
        #endif //defined(MEDIAINFO_ADM_YES)
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioTrackChannelFormatID()
{
    Skip_UTF16B(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioPackFormatID()
{
    Skip_UTF16B(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkStreamID_link1()
{
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMProfileLevelULBatch()
{
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
        Skip_UUID(                                              "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::RIFFChunkStreamID_link2()
{
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioProgrammeID_ST2131()
{
    Skip_UTF16B(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioContentID_ST2131()
{
    Skip_UTF16B(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::ADMAudioObjectID_ST2131()
{
    Skip_UTF16B(Length2,                                        "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::MPEGAudioBitRate()
{
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_LastModifiedDate()
{
    //Parsing
    Ztring Value;
    Get_Timestamp(Value); Element_Info1(Value);

    FILLING_BEGIN();
        Fill(Stream_General, 0, General_Encoded_Date, Value, true);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PrimaryExtendedSpokenLanguageCode()
{
    //Parsing
    Ztring Data;
    Get_UTF8 (Length2, Data,                                    "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        ProductionFrameworks[InstanceUID].PrimaryExtendedSpokenLanguage=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SecondaryExtendedSpokenLanguageCode()
{
    //Parsing
    Info_UTF8(Length2, Data,                                    "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::OriginalExtendedSpokenLanguageCode()
{
    //Parsing
    Info_UTF8(Length2, Data,                                    "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::SecondaryOriginalExtendedSpokenLanguageCode()
{
    //Parsing
    Info_UTF8(Length2, Data,                                    "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RFC5646SpokenLanguage()
{
    bool SizeIsPresent=false;
    if (Length2>=4)
    {
        int32u Size;
        Peek_B4(Size);
        if (Size==((int32u)Length2)-4)
        {
            SizeIsPresent=true;
            Skip_B4(                                            "Value size");
        }
    }

    //Parsing
    Ztring Value;
    Get_UTF8 (Length2-(SizeIsPresent?4:0), Value,               "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        Descriptor_Fill("Language", Value);
    FILLING_END();
}


//---------------------------------------------------------------------------
void File_Mxf::ItemName_ISO7()
{
    //Parsing
    Ztring Data;
    Get_UTF8 (Length2, Data,                                    "Data"); Element_Info1(Data);

    //Filling
    DMOmneons[InstanceUID].Name=Data;
    Name=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::ItemName()
{
    //Parsing
    Ztring Data;
    Get_UTF16B (Length2, Data,                                  "Data"); Element_Info1(Data);

    //Filling
    DMOmneons[InstanceUID].Name=Data;
    Name=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::ItemValue_ISO7()
{
    //Parsing
    Ztring Data;
    Get_UTF8 (Length2, Data,                                    "Data"); Element_Info1(Data);

    //Filling
    DMOmneons[InstanceUID].Name=Data;
    Fill(Stream_General, 0, Name.To_UTF8().c_str(), Data);
    Name.clear();
}

//---------------------------------------------------------------------------
void File_Mxf::ItemValue()
{
    //Parsing
    Ztring Data;
    Get_UTF16B (Length2, Data,                                  "Data"); Element_Info1(Data);

    //Filling
    DMOmneons[InstanceUID].Name=Data;
    Fill(Stream_General, 0, Name.To_UTF8().c_str(), Data);
    Name.clear();
}

//---------------------------------------------------------------------------
void File_Mxf::SingleSequence()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data?"Yes":"No");
}

//---------------------------------------------------------------------------
void File_Mxf::ConstantBPictureCount()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data?"Yes":"No");
}

//---------------------------------------------------------------------------
void File_Mxf::CodedContentScanning()
{
    //Parsing
    int8u Data;
    Get_B1 (Data,                                               "Data"); Element_Info1(Mxf_MPEG2_CodedContentType(Data));

    FILLING_BEGIN();
        descriptor& desc_item = Descriptors[InstanceUID];
        if (desc_item.ScanType.empty())
        {
            if (Data==2)
            {
                if (desc_item.Height!=(int32u)-1) desc_item.Height*=2;
                if (desc_item.Height_Display!=(int32u)-1) desc_item.Height_Display*=2;
                if (desc_item.Height_Display_Offset!=(int32u)-1) desc_item.Height_Display_Offset*=2;
            }
            desc_item.ScanType.From_UTF8(Mxf_MPEG2_CodedContentType(Data));
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LowDelay()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data?"Yes":"No");
}

//---------------------------------------------------------------------------
void File_Mxf::ClosedGOP()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data?"Yes":"No");
}

//---------------------------------------------------------------------------
void File_Mxf::IdenticalGOP()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data?"Yes":"No");
}

//---------------------------------------------------------------------------
void File_Mxf::MaxGOP()
{
    //Parsing
    int16u Data;
    Get_B2 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data==1)
            Descriptors[InstanceUID].Infos["Format_Settings_GOP"]=__T("N=1");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MaxBPictureCount()
{
    //Parsing
    int16u Data;
    Get_B2 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptors[InstanceUID].HasBFrames=Data?true:false;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::ProfileAndLevel()
{
    //Parsing
    int8u profile_and_level_indication_profile, profile_and_level_indication_level;
    BS_Begin();
    Skip_SB(                                                    "profile_and_level_indication_escape");
    Get_S1 ( 3, profile_and_level_indication_profile,           "profile_and_level_indication_profile"); Param_Info1(Mpegv_profile_and_level_indication_profile[profile_and_level_indication_profile]);
    Get_S1 ( 4, profile_and_level_indication_level,             "profile_and_level_indication_level"); Param_Info1(Mpegv_profile_and_level_indication_level[profile_and_level_indication_level]);
    BS_End();

    FILLING_BEGIN();
        if (profile_and_level_indication_profile && profile_and_level_indication_level)
            Descriptor_Fill("Format_Profile", Ztring().From_UTF8(Mpegv_profile_and_level_indication_profile[profile_and_level_indication_profile])+__T("@")+Ztring().From_UTF8(Mpegv_profile_and_level_indication_level[profile_and_level_indication_level]));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MPEG4VisualProfileAndLevel()
{
    //Parsing
    int8u profile_and_level_indication;
    Get_B1 (profile_and_level_indication,                       "profile_and_level_indication"); Param_Info1(Mpeg4v_Profile_Level(profile_and_level_indication)); Element_Info1(Mpeg4v_Profile_Level(profile_and_level_indication));

    FILLING_BEGIN();
        if (profile_and_level_indication)
            Descriptor_Fill("Format_Profile", Mpeg4v_Profile_Level(profile_and_level_indication));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCProfile()
{
    //Parsing
    int8u profile_idc;
    Get_B1 (profile_idc,                                        "profile_idc"); Element_Info1(Avc_profile_level_string(profile_idc));

    FILLING_BEGIN();
        if (profile_idc)
            Descriptor_Fill("Temp_AVC_Profile", Ztring().From_Number(profile_idc));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::BitRate()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptor_Fill("BitRate", Ztring().From_Number(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCMaximumBitRate()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        Descriptor_Fill("BitRate_Maximum", Ztring().From_Number(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCProfileConstraint()
{
    //Parsing
    int8u constraint_set_flags;
    Get_B1 (constraint_set_flags,                               "constraint_sett_flags");
        Skip_Flags(constraint_set_flags, 7,                     "constraint_sett0_flag");
        Skip_Flags(constraint_set_flags, 6,                     "constraint_sett1_flag");
        Skip_Flags(constraint_set_flags, 5,                     "constraint_sett2_flag");
        Skip_Flags(constraint_set_flags, 4,                     "constraint_sett3_flag");
        Skip_Flags(constraint_set_flags, 3,                     "constraint_sett4_flag");
        Skip_Flags(constraint_set_flags, 2,                     "constraint_sett5_flag");
        Skip_Flags(constraint_set_flags, 1,                     "constraint_sett6_flag");
        Skip_Flags(constraint_set_flags, 0,                     "constraint_sett7_flag");

    FILLING_BEGIN();
        if (constraint_set_flags)
            Descriptor_Fill("Temp_AVC_constraint_set", Ztring::ToZtring(constraint_set_flags));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCLevel()
{
    //Parsing
    int8u level_idc;
    Get_B1 (level_idc,                                          "level_idc");  Element_Info1(Avc_profile_level_string(0, level_idc));

    FILLING_BEGIN();
        if (level_idc)
            Descriptor_Fill("Temp_AVC_Level", Ztring().From_Number(level_idc));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCDecodingDelay()
{
    //Parsing
    Info_B1(Data,                                               "Data"); Element_Info1(Data==0xFF?"":(Data?"Yes":"No"));
}

//---------------------------------------------------------------------------
void File_Mxf::AVCMaximumRefFrames()
{
    //Parsing
    Info_B1(Data,                                               "max_num_ref_frames"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::AVCSequenceParameterSetFlag()
{
    //Parsing
    BS_Begin();
    Info_SB(   Constancy,                                       "Constancy");
    Info_BS(3, Location,                                        "In-band location"); Element_Info1(Mxf_AVC_SequenceParameterSetFlag_Constancy(Constancy));
    Skip_BS(4,                                                  "reserved"); Element_Info1(Mxf_AVC_SequenceParameterSetFlag_Constancy(Location));
    BS_End();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCPictureParameterSetFlag()
{
    //Parsing
    BS_Begin();
    Info_SB(   Constancy,                                       "Constancy");
    Info_BS(3, Location,                                        "In-band location"); Element_Info1(Mxf_AVC_SequenceParameterSetFlag_Constancy(Constancy));
    Skip_BS(4,                                                  "reserved"); Element_Info1(Mxf_AVC_SequenceParameterSetFlag_Constancy(Location));
    BS_End();
}

//---------------------------------------------------------------------------
void File_Mxf::AVCAverageBitRate()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data");

    FILLING_BEGIN();
        Descriptor_Fill("BitRate", Ztring().From_Number(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::NetworkLocator_URLString()
{
    //Parsing
    Ztring Data;
    Get_UTF16B(Length2, Data,                                   "Essence Locator"); Element_Info1(Data);

    FILLING_BEGIN();
        Locators[InstanceUID].EssenceLocator=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::MGALinkID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::MGAAudioMetadataIndex()
{
    //Parsing
    Skip_B1(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::MGAAudioMetadataIdentifier()
{
    //Parsing
    Skip_B1(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::MGAAudioMetadataPayloadULArray()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        //Parsing
        Skip_UUID(                                              "UUID");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::MGAMetadataSectionLinkID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::SADMMetadataSectionLinkID()
{
    //Parsing
    Skip_UUID(                                                  "UUID");
}

//---------------------------------------------------------------------------
void File_Mxf::SADMProfileLevelULBatch()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        //Parsing
        Skip_UUID(                                              "UUID");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::MultipleDescriptor_FileDescriptors()
{
    Descriptors[InstanceUID].SubDescriptors.clear();

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        //Parsing
        int128u Data;
        Get_UUID(Data,                                          "UUID");

        FILLING_BEGIN();
            Descriptors[InstanceUID].SubDescriptors.push_back(Data);
            Descriptors[Data].Infos["StreamOrder"].From_Number(i);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::RFC5646TextLanguageCode()
{
    //Parsing
    Skip_UTF16B(Length2,                                        "Content");
}

//---------------------------------------------------------------------------
void File_Mxf::PartitionMetadata()
{
    //Parsing
    int64u PreviousPartition, FooterPartition, HeaderByteCount, IndexByteCount, BodyOffset;
    int32u IndexSID, BodySID;
    int32u KAGSize;
    int16u MajorVersion, MinorVersion;
    Get_B2 (MajorVersion,                                       "MajorVersion");
    Get_B2 (MinorVersion,                                       "MinorVersion");
    Get_B4 (KAGSize,                                            "KAGSize");
    Skip_B8(                                                    "ThisPartition");
    Get_B8 (PreviousPartition,                                  "PreviousPartition");
    Get_B8 (FooterPartition,                                    "FooterPartition");
    Get_B8 (HeaderByteCount,                                    "HeaderByteCount");
    Get_B8 (IndexByteCount,                                     "IndexByteCount");
    Get_B4 (IndexSID,                                           "IndexSID");
    Get_B8 (BodyOffset,                                         "BodyOffset");
    Get_B4 (BodySID,                                            "BodySID");
    Get_UL (OperationalPattern,                                 "OperationalPattern", Mxf_OperationalPattern);

    Element_Begin1("EssenceContainers"); //Vector
        auto Count=Vector(16);
        if (Count==(int32u)-1)
        {
            Element_End0();
            return;
        }
        for (int32u i=0; i<Count; i++)
        {
            int128u EssenceContainer;
            Get_UL (EssenceContainer,                           "EssenceContainer", Mxf_EssenceContainer);
            if (Count==1)
                EssenceContainer_FromPartitionMetadata=EssenceContainer;
        }
    Element_End0();

    FILLING_BEGIN_PRECISE();
        if (!Status[IsAccepted])
            Accept();
    FILLING_END();

    PartitionPack_Parsed=true;
    Partitions_IsFooter=(Code.lo&0x00FF0000)==0x00040000;
    if (PreviousPartition!=File_Offset+Buffer_Offset-Header_Size)
        PartitionMetadata_PreviousPartition=PreviousPartition;
    if (FooterPartition)
        PartitionMetadata_FooterPartition=FooterPartition;
    bool AlreadyParsed=false;
    for (size_t Pos=0; Pos<Partitions.size(); Pos++)
        if (Partitions[Pos].StreamOffset==File_Offset+Buffer_Offset-Header_Size)
            AlreadyParsed=true;
    if (!AlreadyParsed)
    {
        partition Partition;
        Partition.StreamOffset=File_Offset+Buffer_Offset-Header_Size;
        Partition.FooterPartition=FooterPartition;
        Partition.HeaderByteCount=HeaderByteCount;
        Partition.IndexByteCount=IndexByteCount;
        Partition.BodyOffset=BodyOffset;
        Partition.BodySID=BodySID;
        Partitions_Pos=0;
        while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset<Partition.StreamOffset)
            Partitions_Pos++;
        Partitions.insert(Partitions.begin()+Partitions_Pos, Partition);
        Partitions_IsCalculatingHeaderByteCount=true;
    }

    Fill(Stream_General, 0, General_Format_Version, Ztring::ToZtring(MajorVersion)+__T('.')+Ztring::ToZtring(MinorVersion), true);

    if ((Code.lo&0xFF0000)==0x020000) //If Header Partition Pack
        switch ((Code.lo>>8)&0xFF)
        {
            case 0x01 : Fill(Stream_General, 0, General_Format_Settings, "Open / Incomplete"  , Unlimited, true, true);
                        if (Config->ParseSpeed>=1.0)
                        {
                            Config->File_IsGrowing=true;
                            HeaderPartition_IsOpen=true;
                            #if MEDIAINFO_HASH
                                delete Hash; Hash=NULL;
                            #endif //MEDIAINFO_HASH
                        }
                        break;
            case 0x02 : Fill(Stream_General, 0, General_Format_Settings, "Closed / Incomplete", Unlimited, true, true);
                        break;
            case 0x03 : Fill(Stream_General, 0, General_Format_Settings, "Open / Complete"    , Unlimited, true, true);
                        if (Config->ParseSpeed>=1.0)
                        {
                            Config->File_IsGrowing=true;
                            HeaderPartition_IsOpen=true;
                            #if MEDIAINFO_HASH
                                delete Hash; Hash=NULL;
                            #endif //MEDIAINFO_HASH
                        }
                        break;
            case 0x04 : Fill(Stream_General, 0, General_Format_Settings, "Closed / Complete"  , Unlimited, true, true);
                        break;
            default   : ;
        }

    if ((Code.lo&0xFF0000)==0x030000 && (Code.lo&0x00FF00)<=0x000400) //If Body Partition Pack
    {
        if (IsParsingEnd)
        {
            //Parsing only index
            RandomIndexPacks_MaxOffset=File_Offset+Buffer_Offset+Element_Size+HeaderByteCount+IndexByteCount;

            //Hints
            if (File_Buffer_Size_Hint_Pointer && Buffer_Offset+Element_Size+HeaderByteCount+IndexByteCount>=Buffer_Size)
            {
                size_t Buffer_Size_Target=(size_t)(Buffer_Offset+Element_Size+HeaderByteCount+IndexByteCount-Buffer_Size);
                if (Buffer_Size_Target<128*1024)
                    Buffer_Size_Target=128*1024;
                //if ((*File_Buffer_Size_Hint_Pointer)<Buffer_Size_Target)
                    (*File_Buffer_Size_Hint_Pointer)=Buffer_Size_Target;
            }
        }
    }

    if ((Code.lo&0xFF0000)==0x040000) //If Footer Partition Pack
    {
        switch ((Code.lo>>8)&0xFF)
        {
            case 0x02 :
            case 0x04 :
                        Config->File_IsGrowing=false;
                        break;
            default   : ;
        }

        #if MEDIAINFO_ADVANCED
            if (Footer_Position==(int64u)-1)
                Footer_Position=File_Offset+Buffer_Offset-Header_Size;
        #endif //MEDIAINFO_ADVANCED

        #if MEDIAINFO_ADVANCED
            //IsTruncated
            if (!Trusted_Get())
                IsTruncated();
            else
            {
                int32u KAGSize_Corrected=KAGSize;
                if (!KAGSize || KAGSize>=File_Size) // Checking incoherent behaviors
                    KAGSize_Corrected=1;
                int64u Element_Size_WithPadding=Element_Offset;
                if (Element_Size_WithPadding%KAGSize_Corrected)
                {
                    Element_Size_WithPadding-=Element_Size_WithPadding%KAGSize_Corrected;
                    Element_Size_WithPadding+=KAGSize_Corrected;
                }

                auto ExpectedSize=File_Offset+Buffer_Offset-Header_Size+Element_Size_WithPadding;
                if ((Code.lo&0xFF0000)==0x020000) //If Header Partition Pack
                    ExpectedSize+=HeaderByteCount+IndexByteCount;
                if (ExpectedSize>File_Size)
                    IsTruncated(ExpectedSize, true);
            }
        #endif //MEDIAINFO_ADVANCED
    }

    PartitionPack_AlreadyParsed.insert(File_Offset+Buffer_Offset-Header_Size);
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_ContentStorage()
{
    //Parsing
    int128u Data;
    Get_UUID(Data,                                              "Data"); Element_Info1(Ztring().From_UUID(Data));

    FILLING_BEGIN();
        Prefaces[Preface_Current].ContentStorage=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_Version()
{
    //Parsing
    Info_B1(Major,                                              "Major"); //1
    Info_B1(Minor,                                              "Minor"); //2
    Element_Info1(Ztring::ToZtring(Major)+__T('.')+Ztring::ToZtring(Minor));
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_Identifications()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("Identification");
        int128u Data;
        Get_UUID(Data,                                          "UUID"); Element_Info1(Ztring().From_UUID(Data));
        Element_End0();

        FILLING_BEGIN();
            Prefaces[Preface_Current].Identifications.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_ObjectModelVersion()
{
    //Parsing
    Skip_B4(                                                    "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_PrimaryPackage()
{
    //Parsing
    int128u Data;
    Get_UUID(Data,                                              "Data");

    FILLING_BEGIN();
        Prefaces[Preface_Current].PrimaryPackage=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_OperationalPattern()
{
    //Parsing
    Get_UL (OperationalPattern,                                 "UUID", Mxf_OperationalPattern); Element_Info1(Mxf_OperationalPattern(OperationalPattern));
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_EssenceContainers()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        Info_UL(EssenceContainer,                               "EssenceContainer", Mxf_EssenceContainer);
    }
}

//---------------------------------------------------------------------------
void File_Mxf::Preface_DMSchemes()
{
    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        Info_UL(Data,                                           "DMScheme", NULL); Element_Info1(Ztring().From_UUID(Data));
    }
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_PixelLayout()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_Palette()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_PaletteLayout()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_ScanningDirection()
{
    //Parsing
    Info_B1(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_ComponentMaxRef()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].MaxRefLevel==(int32u)-1)
            Descriptors[InstanceUID].MaxRefLevel=Data;
        ColorLevels_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_ComponentMinRef()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].MinRefLevel==(int32u)-1)
            Descriptors[InstanceUID].MinRefLevel=Data;
        ColorLevels_Compute(Descriptors.find(InstanceUID));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_AlphaMaxRef()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::RGBADescriptor_AlphaMinRef()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::Sequence_StructuralComponents()
{
    Components[InstanceUID].StructuralComponents.clear();

    //Parsing
    VECTOR(16);
    for (int32u i=0; i<Count; i++)
    {
        int128u Data;
        Get_UUID (Data,                                         "StructuralComponent");

        FILLING_BEGIN();
            Components[InstanceUID].StructuralComponents.push_back(Data);
        FILLING_END();
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SourceClip_SourcePackageID()
{
    //Parsing
    int256u Data;
    Get_UMID(Data,                                              "SourcePackageID");

    FILLING_BEGIN();
        Components[InstanceUID].SourcePackageID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SourceClip_SourceTrackID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                                "SourceTrackID"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Components[InstanceUID].SourceTrackID==(int32u)-1)
            Components[InstanceUID].SourceTrackID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SourceClip_StartPosition()
{
    //Parsing
    Info_B8(Data,                                               "StartPosition"); Element_Info1(Data); //units of edit rate
}

//---------------------------------------------------------------------------
void File_Mxf::SourcePackage_Descriptor()
{
    //Parsing
    int128u Data;
    Get_UUID(Data,                                              "Data"); Element_Info1(Ztring().From_UUID(Data));

    FILLING_BEGIN();
        Packages[InstanceUID].Descriptor=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::StructuralComponent_DataDefinition()
{
    //Parsing
    Info_UUID(Data,                                             "Data"); Element_Info1(Mxf_Sequence_DataDefinition(Data));
}

//---------------------------------------------------------------------------
void File_Mxf::StructuralComponent_Duration()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                               "Data"); Element_Info1(Data); //units of edit rate

    FILLING_BEGIN();
        if (Data!=0xFFFFFFFFFFFFFFFFLL)
            Components[InstanceUID].Duration=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_FrameCount()
{
    //Parsing
    Skip_B4(                                                    "Value");
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_TimeCodeArray()
{
    //Parsing
    VECTOR(8);
    auto& SystemScheme=SystemScheme1s[Element_Code&0xFFFF];
    bool IsStart=SystemScheme.TimeCodeArray_StartTimecodes.empty();
    for (int32u i=0; i<Count; i++)
    {
        Element_Begin1("TimeCode");
        int8u Frames_Units, Frames_Tens, Seconds_Units, Seconds_Tens, Minutes_Units, Minutes_Tens, Hours_Units, Hours_Tens;
        bool  DropFrame;
        BS_Begin();

        #if MEDIAINFO_ADVANCED
            int32u BG;
            bool ColorFrame, FieldPhaseBgf0, Bgf0Bgf2, Bgf1, Bgf2FieldPhase;
            #define Get_TCB(_1,_2)      Get_SB(_1,_2)
            #define Get_TC4(_1,_2)      Get_L4(_1,_2)
        #else
            #define Get_TCB(_1,_2)      Skip_SB(_2)
            #define Get_TC4(_1,_2)      Skip_L4(_2)
        #endif

        Get_TCB(   ColorFrame,                                  "CF - Color fame");
        Get_SB (   DropFrame,                                   "DP - Drop frame");
        Get_S1 (2, Frames_Tens,                                 "Frames (Tens)");
        Get_S1 (4, Frames_Units,                                "Frames (Units)");

        Get_TCB(   FieldPhaseBgf0,                              "FP - Field Phase / BGF0");
        Get_S1 (3, Seconds_Tens,                                "Seconds (Tens)");
        Get_S1 (4, Seconds_Units,                               "Seconds (Units)");

        Get_TCB(   Bgf0Bgf2,                                    "BGF0 / BGF2");
        Get_S1 (3, Minutes_Tens,                                "Minutes (Tens)");
        Get_S1 (4, Minutes_Units,                               "Minutes (Units)");

        Get_TCB(   Bgf2FieldPhase,                              "BGF2 / Field Phase");
        Get_TCB(   Bgf1,                                        "BGF1");
        Get_S1 (2, Hours_Tens,                                  "Hours (Tens)");
        Get_S1 (4, Hours_Units,                                 "Hours (Units)");

        BS_End();

        Get_TC4(   BG,                                          "BG");

        #undef Get_TCB
        #undef Get_TC4

        auto FramesMax=MxfTimeCodeForDelay.IsInit()?(MxfTimeCodeForDelay.RoundedTimecodeBase-1):0;
        Element_Info1(TimeCode(Hours_Tens*10+Hours_Units, Minutes_Tens*10+Minutes_Units, Seconds_Tens*10+Seconds_Units, Frames_Tens*10+Frames_Units, FramesMax, TimeCode::DropFrame(DropFrame).FPS1001(Is1001)).ToString());
        Element_End0();

        //TimeCode
        if (IsStart && !IsParsingEnd && IsParsingMiddle_MaxOffset==(int64u)-1)
        {
            SystemScheme.TimeCodeArray_StartTimecodes.emplace_back(Hours_Tens*10+Hours_Units, Minutes_Tens*10+Minutes_Units, Seconds_Tens*10+Seconds_Units, Frames_Tens*10+Frames_Units, FramesMax, TimeCode::DropFrame(DropFrame).FPS1001(Is1001));
        }

        #if MEDIAINFO_ADVANCED
            if (Config->TimeCode_Dumps)
            {
                auto id="SystemScheme1-"+to_string(Element_Code&0xFF)+'-'+to_string(i);
                auto& TimeCode_Dump=(*Config->TimeCode_Dumps)[id];
                if (TimeCode_Dump.List.empty())
                {
                    TimeCode_Dump.Attributes_First+=" id=\""+id+"\" format=\"smpte-st331\"";

                    //No clear frame rate, looking for the common frame rate
                    float64 FrameRate=0;
                    for (const auto& Track : Tracks)
                    {
                        if (Track.second.EditRate)
                        {
                            auto NewFrameRate=Track.second.EditRate;
                            if (!FrameRate)
                                FrameRate=NewFrameRate;
                            else if (NewFrameRate!=FrameRate)
                            {
                                FrameRate=0;
                                break;
                            }
                        }
                    }
                    if (FrameRate>0)
                    {
                        TimeCode_Dump.Attributes_First+=" frame_rate=\"";
                        auto FrameRateInt=(int64u)ceil(FrameRate);
                        if (FrameRateInt==FrameRate)
                            TimeCode_Dump.Attributes_First+=to_string(FrameRateInt);
                        else
                        {
                            auto Test_1_1001=FrameRateInt/FrameRate;
                            if (Test_1_1001>=1.00005 && Test_1_1001<1.001005)
                                TimeCode_Dump.Attributes_First+=to_string(FrameRateInt)+"000/1001";
                            else
                                TimeCode_Dump.Attributes_First+=to_string(FrameRate);
                        }
                        TimeCode_Dump.Attributes_First+='\"';
                        FrameRateInt--;
                        if (FrameRateInt<=numeric_limits<int32u>::max())
                            (*Config->TimeCode_Dumps)[id].FramesMax=(int32u)(FrameRateInt);
                    }
                    TimeCode_Dump.Attributes_Last+=" fp=\"0\" bgf=\"0\" bg=\"0\"";
                }
                TimeCode CurrentTC(Hours_Tens*10+Hours_Units, Minutes_Tens*10+Minutes_Units, Seconds_Tens*10+Seconds_Units, Frames_Tens*10+Frames_Units, TimeCode_Dump.FramesMax, TimeCode::DropFrame(DropFrame).FPS1001(Is1001));
                TimeCode_Dump.List+="    <tc v=\""+CurrentTC.ToString()+'\"';
                if (FieldPhaseBgf0)
                    TimeCode_Dump.List+=" fp=\"1\"";
                if (Bgf0Bgf2 || Bgf1 || Bgf2FieldPhase)
                    TimeCode_Dump.List+=" bgf=\""+to_string((Bgf2FieldPhase<<2)|(Bgf1<<1)|(Bgf2FieldPhase<<0))+"\"";
                if (BG)
                    TimeCode_Dump.List+=" bgf=\""+to_string(BG)+"\"";
                if (TimeCode_Dump.LastTC.IsValid() && TimeCode_Dump.LastTC.GetFramesMax() && CurrentTC!=TimeCode_Dump.LastTC+1)
                    TimeCode_Dump.List+=" nc=\"1\"";
                TimeCode_Dump.LastTC=CurrentTC;
                TimeCode_Dump.List+="/>\n";
                TimeCode_Dump.FrameCount++;
            }
        #endif //MEDIAINFO_ADVANCED
    }

    if (IsStart && SystemScheme.TimeCodeArray_StartTimecodes.size()>1)
    {
        bool IsNotContinuous=false;
        auto Current=SystemScheme.TimeCodeArray_StartTimecodes.front();
        for (size_t i=1; i<SystemScheme.TimeCodeArray_StartTimecodes.size(); i++)
        {
            const auto& TC=SystemScheme.TimeCodeArray_StartTimecodes[i];
            if (TC==Current)
                continue;
            Current++;
            if (TC!=Current)
            {
                IsNotContinuous=true;
                break;
            }
        }
        if (!IsNotContinuous)
        {
            SystemScheme.TimeCodeArray_StartTimecodes.resize(1);
        }
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_ClipIDArray()
{
    //Parsing
    VECTOR(32);
    for (int32u i=0; i<Count; i++)
    {
        Skip_UMID(                                              );
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_ExtendedClipIDArray()
{
    //Parsing
    VECTOR(64);
    while (Element_Offset<Element_Size)
    {
        Skip_UMID(                                              ); //ExtUMID (ToDo: merge)
        Skip_UMID(                                              );
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_VideoIndexArray()
{
    //Parsing
    auto Count=Vector();
    if (Count==(int32u)-1)
        return;
    int32u Length=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset-4); //Not provided by Vector()
    for (int32u i=0; i<Count; i++)
    {
        Skip_XX(Length,                                         "Video Index");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_KLVMetadataSequence()
{
    Skip_XX(Element_Size,                                        "KLV");
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_SampleRate()
{
    //Parsing
    Skip_B8(                                                    "Value");
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_EssenceTrackNumber()
{
    //Parsing
    Skip_B4(                                                    "Value");
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_EssenceTrackNumberBatch()
{
    //Parsing
    VECTOR(4);
    for (int32u i=0; i<Count; i++)
    {
        Skip_B4(                                                "Track Number");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::SystemScheme1_ContentPackageIndexArray()
{
    //Parsing
    auto Count=Vector();
    if (Count==(int32u)-1)
        return;
    int32u Length=BigEndian2int32u(Buffer+Buffer_Offset+(size_t)Element_Offset-4); //Not provided by Vector()
    for (int32u i=0; i<Count; i++)
    {
        Skip_XX(Length,                                         "Index Entry");
    }
}

//---------------------------------------------------------------------------
void File_Mxf::TextLocator_LocatorName()
{
    //Parsing
    Ztring Data;
    Get_UTF16B (Length2, Data,                                  "Data"); Element_Info1(Data);

    Locators[InstanceUID].EssenceLocator=Data;
    Locators[InstanceUID].IsTextLocator=true;
}

//---------------------------------------------------------------------------
void File_Mxf::TimecodeGroup_StartTimecode()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                                "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Data!=(int64u)-1)
        {
            MxfTimeCodeForDelay.InstanceUID=InstanceUID;
            MxfTimeCodeForDelay.StartTimecode=Data;
            if (MxfTimeCodeForDelay.RoundedTimecodeBase)
            {
                DTS_Delay=((float64)MxfTimeCodeForDelay.StartTimecode)/MxfTimeCodeForDelay.RoundedTimecodeBase;
                if (MxfTimeCodeForDelay.DropFrame)
                {
                    DTS_Delay*=1001;
                    DTS_Delay/=1000;
                }
                FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
                #if MEDIAINFO_DEMUX
                    Config->Demux_Offset_DTS_FromStream=FrameInfo.DTS;
                #endif //MEDIAINFO_DEMUX
            }
        }

        Components[InstanceUID].MxfTimeCode.InstanceUID=InstanceUID;
        Components[InstanceUID].MxfTimeCode.StartTimecode=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::TimecodeGroup_RoundedTimecodeBase()
{
    //Parsing
    int16u Data;
    Get_B2 (Data,                                                "Data"); Element_Info1(Data);

    if (Data && Data!=(int16u)-1)
    {
        MxfTimeCodeForDelay.RoundedTimecodeBase=Data;
        if (MxfTimeCodeForDelay.StartTimecode!=(int64u)-1)
        {
            DTS_Delay=((float64)MxfTimeCodeForDelay.StartTimecode)/MxfTimeCodeForDelay.RoundedTimecodeBase;
            if (MxfTimeCodeForDelay.DropFrame)
            {
                DTS_Delay*=1001;
                DTS_Delay/=1000;
            }
            FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
            #if MEDIAINFO_DEMUX
                Config->Demux_Offset_DTS_FromStream=FrameInfo.DTS;
            #endif //MEDIAINFO_DEMUX
        }
    }

    Components[InstanceUID].MxfTimeCode.RoundedTimecodeBase=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::TimecodeGroup_DropFrame()
{
    //Parsing
    int8u Data;
    Get_B1 (Data,                                                "Data"); Element_Info1(Data);

    if (Data!=(int8u)-1 && Data)
    {
        MxfTimeCodeForDelay.DropFrame=true;
        if (DTS_Delay)
        {
            DTS_Delay*=1001;
            DTS_Delay/=1000;
        }
        FrameInfo.DTS=float64_int64s(DTS_Delay*1000000000);
        #if MEDIAINFO_DEMUX
            Config->Demux_Offset_DTS_FromStream=FrameInfo.DTS;
        #endif //MEDIAINFO_DEMUX
    }

    Components[InstanceUID].MxfTimeCode.DropFrame=Data?true:false;
}

//---------------------------------------------------------------------------
void File_Mxf::Track_EditRate()
{
    //Parsing
    float64 Data;
    Get_Rational(Data); Element_Info1(Data);

    Tracks[InstanceUID].EditRate=Data;
    if (Data!=(int)Data)
        Is1001=true;
}

//---------------------------------------------------------------------------
void File_Mxf::Track_Origin()
{
    //Parsing
    int64u Data;
    Get_B8 (Data,                                                "Data"); Element_Info1(Data); //Note: Origin is signed but there is no signed Get_* in MediaInfo

    Tracks[InstanceUID].Origin=(int64s)Data; //Origin is signed
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_AvgBps()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    Descriptor_Fill("BitRate", Ztring().From_Number(Data*8));
    Descriptors[InstanceUID].ByteRate=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_BlockAlign()
{
    //Parsing
    int16u Data;
    Get_B2 (Data,                                               "Data"); Element_Info1(Data);

    Descriptors[InstanceUID].BlockAlign=Data;
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_SequenceOffset()
{
    //Parsing
    Info_B1(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakEnvelopeVersion()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakEnvelopeFormat()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PointsPerPeakValue()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakEnvelopeBlockSize()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakChannels()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakFrames()
{
    //Parsing
    Info_B4(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakOfPeaksPosition()
{
    //Parsing
    Info_B8(Data,                                                "Data"); Element_Info1(Data);
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakEnvelopeTimestamp()
{
    //Parsing
    Info_Timestamp();
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_PeakEnvelopeData()
{
    //Parsing
    Skip_XX(Length2,                                            "Data");
}

//---------------------------------------------------------------------------
void File_Mxf::WAVEPCMDescriptor_ChannelAssignment()
{
    //Parsing
    int128u Value;
    Get_UL (Value,                                              "Value", Mxf_ChannelAssignment_ChannelLayout); Element_Info1(Mxf_ChannelAssignment_ChannelLayout(Value, Descriptors[InstanceUID].ChannelCount));

    FILLING_BEGIN();
        Descriptors[InstanceUID].ChannelAssignment=Value;

        //Descriptors[InstanceUID].Infos["ChannelLayout"]=Mxf_ChannelAssignment_ChannelLayout(Value, Descriptors[InstanceUID].ChannelCount);
        //Ztring ChannelLayoutID;
        //ChannelLayoutID.From_Number(Value.lo, 16);
        //if (ChannelLayoutID.size()<16)
        //    ChannelLayoutID.insert(0, 16-ChannelLayoutID.size(), __T('0'));
        //Descriptors[InstanceUID].Infos["ChannelLayoutID"]=ChannelLayoutID;
        //Descriptors[InstanceUID].Infos["ChannelPositions"]=Mxf_ChannelAssignment_ChannelPositions(Value, Descriptors[InstanceUID].ChannelCount);
        //if (Descriptors[InstanceUID].Infos["ChannelPositions"].empty())
        //    Descriptors[InstanceUID].Infos["ChannelPositions"]=ChannelLayoutID;
        //Descriptors[InstanceUID].Infos["ChannelPositions/String2"]=Mxf_ChannelAssignment_ChannelPositions2(Value, Descriptors[InstanceUID].ChannelCount);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_IrisFNumber()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(std::pow(2, 8*(1-(float(Value)/0x10000))), 6).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
// Big Endian - float 16 bits
// TODO: remove it when Linux version of ZenLib is updated
float32 BigEndian2float16lens(const char* Liste)
{
    //exponent      4 bit
    //significand  12 bit

    //Retrieving data
    int16u Integer=BigEndian2int16s(Liste);

    //Retrieving elements
    int Exponent=(Integer>>12)&0x0F;
    if (Exponent>=8)
        Exponent=-(((~Exponent)&0x7)+1);
    int32u Mantissa= Integer&0x0FFF;

    //Some computing
    float64 Answer=((float64)Mantissa)*std::pow((float64)10, Exponent);

    return (float32)Answer;
}
inline float32 BigEndian2float16lens(const int8u* List) {return BigEndian2float16lens((const char*)List);}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_FocusPositionFromImagePlane()
{
    //Parsing
    float32 Value;
    Value=BigEndian2float16lens(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Skip_B2(                                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_FocusPositionFromFrontLensVertex()
{
    //Parsing
    float32 Value;
    Value=BigEndian2float16lens(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Skip_B2(                                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_MacroSetting()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value?"On":"Off");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_LensZoom35mmStillCameraEquivalent()
{
    //Parsing
    float32 Value;
    Value=BigEndian2float16lens(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Skip_B2(                                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value*1000, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_LensZoomActualFocalLength()
{
    //Parsing
    float32 Value;
    Value=BigEndian2float16lens(Buffer+Buffer_Offset+(size_t)Element_Offset);
    Skip_B2(                                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value * 1000, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_OpticalExtenderMagnification()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_LensAttributes()
{
    //Parsing
    Ztring Value;
    Get_UTF8(Length2, Value,                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_IrisTNumber()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(std::pow(2, 8*(1-(float(Value)/0x10000))), 6).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_IrisRingPosition()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float)Value)/65536*100, 4).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_FocusRingPosition()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float)Value)/65536*100, 4).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::LensUnitAcquisitionMetadata_ZoomRingPosition()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float)Value)/65536*100, 4).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_TransferCharacteristic()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", NULL); Element_Info1(Mxf_TransferCharacteristic(Data));

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_TransferCharacteristic(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ColorPrimaries()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_ColorPrimaries); Element_Info1(Mxf_ColorPrimaries(Data));

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_ColorPrimaries(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CodingEquations()
{
    //Parsing
    int128u Data;
    Get_UL(Data,                                                "Data", Mxf_CodingEquations); Element_Info1(Mxf_CodingEquations(Data));

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CodingEquations(Data));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_AutoExposureMode()
{
    //Parsing
    int128u Value;
    Get_UUID(Value,                                             "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_AutoExposureMode(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_AutoFocusSensingAreaSetting()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_AutoFocusSensingAreaSetting(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ColorCorrectionFilterWheelSetting()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_ColorCorrectionFilterWheelSetting(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_NeutralDensityFilterWheelSetting()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value"); Element_Info1(Mxf_CameraUnitAcquisitionMetadata_NeutralDensityFilterWheelSetting(Value).c_str());

    FILLING_BEGIN();
        if (Value==1)
            AcquisitionMetadata_Add(Code2, "Clear");
        else
            AcquisitionMetadata_Add(Code2, "1/"+Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveWidth()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/1000, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveHeight()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/1000, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CaptureFrameRate()
{
    //Parsing
    float64 Value;
    Get_Rational(Value);

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ImageSensorReadoutMode()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_ImageSensorReadoutMode(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ShutterSpeed_Angle()
{
    //Parsing
    int32u Value;
    Get_B4(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float)Value) / 60, 1).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ShutterSpeed_Time()
{
    //Parsing
    int32u Num, Den;
    Get_B4(Num,                                                 "Num");
    Get_B4(Den,                                                 "Den");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Num).To_UTF8()+'/'+Ztring::ToZtring(Den).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraMasterGainAdjustment()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/100, 2).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ISOSensitivity()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ElectricalExtenderMagnification()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_AutoWhiteBalanceMode()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_AutoWhiteBalanceMode(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_WhiteBalance()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraMasterBlackLevel()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/10, 1).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraKneePoint()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/10, 1).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraKneeSlope()
{
    //Parsing
    float64 Value;
    Get_Rational(Value);

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value, 3).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraLuminanceDynamicRange()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(((float64)Value)/10, 1).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraSettingFileURI()
{
    //Parsing
    Ztring Value;
    Get_UTF8(Length2, Value,                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_CameraAttributes()
{
    //Parsing
    Ztring Value;
    Get_UTF8(Length2, Value,                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ExposureIndexofPhotoMeter()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_GammaForCDL()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_CameraUnitAcquisitionMetadata_GammaforCDL(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ASC_CDL_V12()
{
    //Parsing
    VECTOR(2);
    if (Count!=10)
    {
        Skip_XX (Length2-8,                                     "Data");
        return;
    }
    float32 sR, sG, sB, oR, oG, oB, pR, pG, pB, sat;
    Get_BF2(sR,                                                 "sR");
    Get_BF2(sG,                                                 "sG");
    Get_BF2(sB,                                                 "sB");
    Get_BF2(oR,                                                 "oR");
    Get_BF2(oG,                                                 "oG");
    Get_BF2(oB,                                                 "oB");
    Get_BF2(pR,                                                 "pR");
    Get_BF2(pG,                                                 "pG");
    Get_BF2(pB,                                                 "pB");
    Get_BF2(sat,                                                "sat");

    FILLING_BEGIN();
        Ztring ValueS=__T("sR=")+Ztring::ToZtring(sR, 1)+__T(" sG=")+Ztring::ToZtring(sG, 1)+__T(" sB=")+Ztring::ToZtring(sB, 1)+__T(" oR=")+Ztring::ToZtring(oR, 1)+__T(" oG=")+Ztring::ToZtring(oG, 1)+__T(" oB=")+Ztring::ToZtring(oB, 1)+__T(" pR=")+Ztring::ToZtring(pR, 1)+__T(" pG=")+Ztring::ToZtring(pG, 1)+__T(" pB=")+Ztring::ToZtring(pB, 1)+__T(" sat=")+Ztring::ToZtring(sat, 1);
        AcquisitionMetadata_Add(Code2, ValueS.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::CameraUnitAcquisitionMetadata_ColorMatrix()
{
    //Parsing
    VECTOR(8);
    for (int32u i=0; i<Count; i++)
    if (Count!=9)
    {
        Skip_XX (Length2-8,                                     "Data");
        return;
    }
    int32u RR_N, RR_D, GR_N, GR_D, BR_N, BR_D, RG_N, RG_D, GG_N, GG_D, BG_N, BG_D, RB_N, RB_D, GB_N, GB_D, BB_N, BB_D;
    Get_B4 (RR_N,                                               "RR Num");
    Get_B4 (RR_D,                                               "RR Den");
    Get_B4 (GR_N,                                               "GR Num");
    Get_B4 (GR_D,                                               "GR Den");
    Get_B4 (BR_N,                                               "BR Num");
    Get_B4 (BR_D,                                               "BR Den");
    Get_B4 (RG_N,                                               "RG Num");
    Get_B4 (RG_D,                                               "RG Den");
    Get_B4 (GG_N,                                               "GG Num");
    Get_B4 (GG_D,                                               "GG Den");
    Get_B4 (BG_N,                                               "BG Num");
    Get_B4 (BG_D,                                               "BG Den");
    Get_B4 (RB_N,                                               "RB Num");
    Get_B4 (RB_D,                                               "RB Den");
    Get_B4 (GB_N,                                               "GB Num");
    Get_B4 (GB_D,                                               "GB Den");
    Get_B4 (BB_N,                                               "BB Num");
    Get_B4 (BB_D,                                               "BB Den");

    FILLING_BEGIN();
        Ztring ValueS=__T("RR=")+Ztring::ToZtring(((float)RR_N)/RR_D, 3)+__T(" GR=")+Ztring::ToZtring(((float)GR_N)/GR_D, 3)+__T(" BR=")+Ztring::ToZtring(((float)BR_N)/BR_D, 3)
                   + __T(" RG=")+Ztring::ToZtring(((float)RG_N)/RG_D, 3)+__T(" GG=")+Ztring::ToZtring(((float)GG_N)/GG_D, 3)+__T(" BG=")+Ztring::ToZtring(((float)BG_N)/BG_D, 3)
                   + __T(" RB=")+Ztring::ToZtring(((float)RB_N)/RB_D, 3)+__T(" GB=")+Ztring::ToZtring(((float)GB_N)/GB_D, 3)+__T(" BB=")+Ztring::ToZtring(((float)BB_N)/BB_D, 3);
        AcquisitionMetadata_Add(Code2, ValueS.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_UdamSetIdentifier()
{
    //Parsing
    int128u Value;
    Get_UUID(Value,                                             "Value");

    FILLING_BEGIN();
        if ((Value.hi==0x966908004678031CLL && Value.lo==0x20500000F0C01181LL)
         || (Value.hi==0x966908004678031CLL && Value.lo==0x20500002F0C01181LL)) //Found Sony metadata with 00 or 02, what is the difference?
            UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony=true;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E101()
{
    //Parsing
    int32u Width, Height;
    Get_B4 (Width,                                              "Width");
    Get_B4 (Height,                                             "Height");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring(Ztring::ToZtring(Width)+__T("x")+Ztring::ToZtring(Height)).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E102()
{
    //Parsing
    int32u Width, Height;
    Get_B4 (Width,                                              "Width");
    Get_B4 (Height,                                             "Height");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring(Ztring::ToZtring(Width)+__T("x")+Ztring::ToZtring(Height)).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E103()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Mxf_AcquisitionMetadata_Sony_CameraProcessDiscriminationCode(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E104()
{
    //Parsing
    int8u Value;
    Get_B1(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value?"On":"Off");
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E105()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E106()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E107()
{
    //Parsing
    int16u Value;
    Get_B2(Value,                                               "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Ztring::ToZtring(Value).To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E109()
{
    //Parsing
    Ztring Value;
    Get_UTF8(Length2, Value,                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E10B()
{
    //Parsing
    int128u Value;
    Get_UUID(Value,                                             "Value");

    FILLING_BEGIN();
        Ztring ValueS;
        ValueS.From_Number(Value.lo, 16);
        if (ValueS.size()<16)
            ValueS.insert(0, 16-ValueS.size(), __T('0'));
        AcquisitionMetadata_Add(Code2, Mxf_AcquisitionMetadata_Sony_MonitoringBaseCurve(Value));
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E201()
{
    if (AcquisitionMetadata_Sony_E201_Lists.empty())
        AcquisitionMetadata_Sony_E201_Lists.resize(Mxf_AcquisitionMetadata_Sony_E201_ElementCount);

    //Parsing
    Ztring  FocusDistance, ApertureValue, ApertureScale, HyperfocalDistance, NearFocusDistance, FarFocusDistance, EntrancePupilPosition;
    string  LensSerialNumber;
    int16u  EffectiveFocaleLength, HorizontalFieldOfView, NormalizedZoomValue;
    if (Length2<27)
    {
        Skip_XX(Length2,                                        "Unknown");
        return;
    }
    int64u End=Element_Offset+Length2;
    Skip_C1(                                                    "Tag");
    BS_Begin();
    Element_Begin1("Focus Distance");
    {
        int8u B1, B2, B3, B4;
        Mark_0();
        Mark_1();
        Get_S1 (6, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        Mark_0();
        Mark_1();
        Get_S1 (6, B3,                                          "3");
        Mark_0();
        Mark_1();
        Get_S1 (6, B4,                                          "4");
        int32u Value=(B1<<18)|(B2<<12)|(B3<<6)|B4;
        if (Value==0xFFFFFF)
            FocusDistance=__T("Infinite");
        else
            switch (AcquisitionMetadata_Sony_CalibrationType)
            {
                case 1 : FocusDistance=Ztring::ToZtring(((float)Value)/10, 1); break;
                default: FocusDistance=Ztring::ToZtring(Value); break;
            }
        Element_Info1(FocusDistance);
    }
    Element_End0();
    Element_Begin1("Aperture Value");
    {
        int8u B1, B2;
        Mark_0();
        Mark_1();
        Get_S1 (6, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        int16u Value=(B1<<6)|B2;
        ApertureValue.From_Number(((float)Value)/100, 2);
        Element_Info1(ApertureValue);
    }
    Element_End0();
    Element_Begin1("Aperture Scale");
    {
        int8u B1, B2, Fraction;
        Mark_1();
        Get_S1 (7, B2,                                          "Integer 2");
        Mark_1();
        Get_S1 (1, B1,                                          "Integer 1");
        Mark_0();
        Mark_0();
        Get_S1 (4, Fraction,                                    "Fraction");
        int8u Value=((B1<<7)|B2);
        ApertureScale=__T("T ")+Ztring::ToZtring(((float)Value)/10, 2)+__T(" + ")+Ztring::ToZtring(Fraction)+__T("/10");
        Element_Info1(ApertureScale);
    }
    Element_End0();
    Element_Begin1("Effective Focale Length");
    {
        int8u B1, B2;
        Mark_0();
        Mark_1();
        Mark_0();
        Mark_0();
        Get_S1 (4, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        EffectiveFocaleLength=(B1<<6)|B2;
        Element_Info2(EffectiveFocaleLength, "mm");
    }
    Element_End0();
    Element_Begin1("Hyperfocal Distance");
    {
        int8u B1, B2, B3, B4;
        Mark_0();
        Mark_1();
        Get_S1 (6, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        Mark_0();
        Mark_1();
        Get_S1 (6, B3,                                          "3");
        Mark_0();
        Mark_1();
        Get_S1 (6, B4,                                          "4");
        int32u Value=(B1<<18)|(B2<<12)|(B3<<6)|B4;
        if (Value==0xFFFFFF)
            HyperfocalDistance=__T("Infinite");
        else
            switch (AcquisitionMetadata_Sony_CalibrationType)
            {
                case 1 : HyperfocalDistance=Ztring::ToZtring(((float)Value)/10, 1); break;
                default: HyperfocalDistance=Ztring::ToZtring(Value);
            }
        Element_Info1(HyperfocalDistance);
    }
    Element_End0();
    Element_Begin1("Near Focus Distance");
    {
        int8u B1, B2, B3, B4;
        Mark_0();
        Mark_1();
        Get_S1 (6, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        Mark_0();
        Mark_1();
        Get_S1 (6, B3,                                          "3");
        Mark_0();
        Mark_1();
        Get_S1 (6, B4,                                          "4");
        int32u Value=(B1<<18)|(B2<<12)|(B3<<6)|B4;
        if (Value==0xFFFFFF)
            NearFocusDistance=__T("Infinite");
        else
            switch (AcquisitionMetadata_Sony_CalibrationType)
            {
                case 1 : NearFocusDistance=Ztring::ToZtring(((float)Value)/10, 1); break;
                default: NearFocusDistance=Ztring::ToZtring(Value);
            }
        Element_Info1(NearFocusDistance);
    }
    Element_End0();
    Element_Begin1("Far Focus Distance");
    {
        int8u B1, B2, B3, B4;
        Mark_0();
        Mark_1();
        Get_S1 (6, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        Mark_0();
        Mark_1();
        Get_S1 (6, B3,                                          "3");
        Mark_0();
        Mark_1();
        Get_S1 (6, B4,                                          "4");
        int32u Value=(B1<<18)|(B2<<12)|(B3<<6)|B4;
        if (Value==0xFFFFFF)
            FarFocusDistance=__T("Infinite");
        else
            switch (AcquisitionMetadata_Sony_CalibrationType)
            {
                case 1 : FarFocusDistance=Ztring::ToZtring(((float)Value)/10, 1); break;
                default: FarFocusDistance=Ztring::ToZtring(Value);
            }
        Element_Info1(FarFocusDistance);
    }
    Element_End0();
    Element_Begin1("Horizontal Field of View");
    {
        int8u B1, B2;
        Mark_0();
        Mark_1();
        Mark_0();
        Get_S1 (5, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        HorizontalFieldOfView=(B1<<6)|B2;
        Element_Info1(Ztring::ToZtring(((float)HorizontalFieldOfView)/10, 1));
    }
    Element_End0();
    Element_Begin1("Entrance Pupil Position");
    {
        int8u   B1, B2;
        bool    Minus;
        Mark_0();
        Mark_1();
        Get_SB (Minus,                                          "Minus");
        Mark_0();
        Get_S1 (4, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        int32u Value=(B1<<6)|B2;
        switch (AcquisitionMetadata_Sony_CalibrationType)
        {
            case 1 : EntrancePupilPosition=Ztring::ToZtring(((float)Value)/10, 1); break;
            default: EntrancePupilPosition=Ztring::ToZtring(Value);
        }
        Element_Info1(EntrancePupilPosition);
    }
    Element_End0();
    Element_Begin1("Normalized Zoom Value");
    {
        int8u B1, B2;
        Mark_0();
        Mark_1();
        Mark_0();
        Mark_0();
        Get_S1 (4, B1,                                          "1");
        Mark_0();
        Mark_1();
        Get_S1 (6, B2,                                          "2");
        NormalizedZoomValue=(B1<<6)|B2;
        Element_Info1(Ztring::ToZtring(((float)NormalizedZoomValue)/1000, 3));
    }
    Element_End0();
    BS_End();
    Skip_C1(                                                    "S");
    Get_String(9, LensSerialNumber,                             "Lens Serial Number");
    if (Element_Offset+2<End)
        Skip_XX(End-2-Element_Offset,                           "Unknown");
    Skip_C2(                                                    "Termination");

    FILLING_BEGIN();
        // Values are internal MI
        AcquisitionMetadata_Sony_E201_Add(0, FocusDistance.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(1, ApertureValue.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(2, ApertureScale.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(3, Ztring::ToZtring(EffectiveFocaleLength).To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(4, HyperfocalDistance.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(5, NearFocusDistance.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(6, FarFocusDistance.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(7, Ztring::ToZtring(((float)HorizontalFieldOfView)/10, 1).To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(8, EntrancePupilPosition.To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(9, Ztring::ToZtring(((float)NormalizedZoomValue)/1000, 3).To_UTF8());
        AcquisitionMetadata_Sony_E201_Add(10, LensSerialNumber);
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E202()
{
    //Parsing
    Ztring Value;
    Get_UTF8(Length2, Value,                                    "Value");

    FILLING_BEGIN();
        AcquisitionMetadata_Add(Code2, Value.To_UTF8());
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UserDefinedAcquisitionMetadata_Sony_E203()
{
    //Parsing
    Get_B1(AcquisitionMetadata_Sony_CalibrationType,            "Value");

    FILLING_BEGIN();
        switch(AcquisitionMetadata_Sony_CalibrationType)
        {
            case 0 : AcquisitionMetadata_Add(Code2, "mm"); break;
            case 1 : AcquisitionMetadata_Add(Code2, "in"); break;
            default : AcquisitionMetadata_Add(Code2, Ztring::ToZtring(AcquisitionMetadata_Sony_CalibrationType).To_UTF8());
        }
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Series_Title()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].SeriesTitle=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Programme_Title()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ProgrammeTitle=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Episode_Title_Number()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].EpisodeTitleNumber=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Shim_Name()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ShimName=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Audio_Track_Layout()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                               "Value"); Element_Info1C(Value<Mxf_AS11_AudioTrackLayout_Count, Mxf_AS11_AudioTrackLayout[Value]);

    FILLING_BEGIN();
        AS11s[InstanceUID].AudioTrackLayout=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Primary_Audio_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].PrimaryAudioLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Closed_Captions_Present()
{
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].ClosedCaptionsPresent=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Closed_Captions_Type()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_ClosedCaptionType_Count, Mxf_AS11_ClosedCaptionType[Value]);

    FILLING_BEGIN();
        AS11s[InstanceUID].ClosedCaptionsType=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Caption_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ClosedCaptionsLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Shim_Version()
{
    //Parsing
    int8u Major, Minor;
    Get_B1 (Major,                                              "Major"); Element_Info1(Major);
    Get_B1 (Minor,                                              "Minor"); Element_Info1(Minor);

    FILLING_BEGIN();
        AS11s[InstanceUID].ShimVersion_Major=Major;
        AS11s[InstanceUID].ShimVersion_Minor=Minor;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Part_Number()
{
    //Parsing
    int16u Value;
    Get_B2 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
         AS11s[InstanceUID].PartNumber=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// AAF
void File_Mxf::AS_11_Part_Total()
{
    //Parsing
    int16u Value;
    Get_B2 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
         AS11s[InstanceUID].PartTotal=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010100
void File_Mxf::UKDPP_Production_Number()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ProductionNumber=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010200
void File_Mxf::UKDPP_Synopsis()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].Synopsis=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010300
void File_Mxf::UKDPP_Originator()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].Originator=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010400
void File_Mxf::UKDPP_Copyright_Year()
{
    //Parsing
    int16u Value;
    Get_B2 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].CopyrightYear=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010500
void File_Mxf::UKDPP_Other_Identifier()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].OtherIdentifier=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010600
void File_Mxf::UKDPP_Other_Identifier_Type()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].OtherIdentifierType=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010700
void File_Mxf::UKDPP_Genre()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].Genre=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010800
void File_Mxf::UKDPP_Distributor()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].Distributor=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010900
void File_Mxf::UKDPP_Picture_Ratio()
{
    //Parsing
    int32u Numerator, Denominator;
    Get_B4 (Numerator,                                          "Numerator");
    Get_B4 (Denominator,                                        "Denominator");
    Element_Info1(Ztring::ToZtring(Numerator)+__T(':')+Ztring::ToZtring(Denominator));

    FILLING_BEGIN();
        AS11s[InstanceUID].PictureRatio_N=Numerator;
        AS11s[InstanceUID].PictureRatio_D=Denominator;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010A00
void File_Mxf::UKDPP_3D()
{
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].ThreeD=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010B00
void File_Mxf::UKDPP_3D_Type()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_3D_Type_Count, Mxf_AS11_3D_Type[Value]);

    FILLING_BEGIN();
        if (Value<Mxf_AS11_3D_Type_Count)
            AS11s[InstanceUID].ThreeDType=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010C00
void File_Mxf::UKDPP_Product_Placement()
{
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].ProductPlacement=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010D00
void File_Mxf::UKDPP_PSE_Pass()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_FpaPass_Count, Mxf_AS11_FpaPass[Value]);

    FILLING_BEGIN();
        AS11s[InstanceUID].FpaPass=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010E00
void File_Mxf::UKDPP_PSE_Manufacturer()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].FpaManufacturer=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101010F00
void File_Mxf::UKDPP_PSE_Version()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].FpaVersion=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011000
void File_Mxf::UKDPP_Video_Comments()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].VideoComments=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011100
void File_Mxf::UKDPP_Secondary_Audio_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
         AS11s[InstanceUID].SecondaryAudioLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::UKDPP_Tertiary_Audio_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
         AS11s[InstanceUID].TertiaryAudioLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011300
void File_Mxf::UKDPP_Audio_Loudness_Standard()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_AudioLoudnessStandard_Count, Mxf_AS11_AudioLoudnessStandard[Value]);

    FILLING_BEGIN();
         AS11s[InstanceUID].AudioLoudnessStandard=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011400
void File_Mxf::UKDPP_Audio_Comments()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].AudioComments=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011500
void File_Mxf::UKDPP_Line_Up_Start()
{
    //Parsing
    int64u Value;
    Get_B8 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].LineUpStart=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011600
void File_Mxf::UKDPP_Ident_Clock_Start()
{
    //Parsing
    int64u Value;
    Get_B8 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].IdentClockStart=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011700
void File_Mxf::UKDPP_Total_Number_Of_Parts()
{
    //Parsing
    int16u Value;
    Get_B2 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].TotalNumberOfParts=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011800
void File_Mxf::UKDPP_Total_Programme_Duration()
{
    //Parsing
    int64u Value;
    Get_B8 (Value,                                              "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].TotalProgrammeDuration=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011900
void File_Mxf::UKDPP_Audio_Description_Present()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].AudioDescriptionPresent=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011A00
void File_Mxf::UKDPP_Audio_Description_Type()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_AudioDescriptionType_Count, Mxf_AS11_AudioDescriptionType[Value]);

    FILLING_BEGIN();
        AS11s[InstanceUID].AudioDescriptionType=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011B00
void File_Mxf::UKDPP_Open_Captions_Present()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].OpenCaptionsPresent=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011C00
void File_Mxf::UKDPP_Open_Captions_Type()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_OpenCaptionsType_Count, Mxf_AS11_OpenCaptionsType[Value]);

    FILLING_BEGIN();
         AS11s[InstanceUID].OpenCaptionsType=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011D00
void File_Mxf::UKDPP_Open_Captions_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].OpenCaptionsLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011E00
void File_Mxf::UKDPP_Signing_Present()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_SigningPresent_Count, Mxf_AS11_SigningPresent[Value]);

    FILLING_BEGIN();
         AS11s[InstanceUID].SigningPresent=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101011F00
void File_Mxf::UKDPP_Sign_Language()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1C(Value<Mxf_AS11_SignLanguage_Count, Mxf_AS11_SignLanguage[Value]);

    FILLING_BEGIN();
         AS11s[InstanceUID].SignLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012000
void File_Mxf::UKDPP_Completion_Date()
{
    //Parsing
    int64u Value;
    Get_B8 (Value,                                              "Value"); Element_Info1(Value); //TODO: Timestamp

    FILLING_BEGIN();
        AS11s[InstanceUID].CompletionDate=Value;
   FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012100
void File_Mxf::UKDPP_Textless_Elements_Exist()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].TextlessElementsExist=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012200
void File_Mxf::UKDPP_Programme_Has_Text()
{
    //Parsing
    int8u Value;
    Get_B1 (Value,                                              "Value"); Element_Info1(Value?"Yes":"No");

    FILLING_BEGIN();
        AS11s[InstanceUID].ProgrammeHasText=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012300
void File_Mxf::UKDPP_Programme_Text_Language()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ProgrammeTextLanguage=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012400
void File_Mxf::UKDPP_Contact_Email()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ContactEmail=Value;
    FILLING_END();
}

//---------------------------------------------------------------------------
// DPP .010101012500
void File_Mxf::UKDPP_Contact_Telephone_Number()
{
    //Parsing
    Ztring Value;
    Get_UTF16B(Length2, Value,                                  "Value"); Element_Info1(Value);

    FILLING_BEGIN();
        AS11s[InstanceUID].ContactTelephoneNumber=Value;
    FILLING_END();
}
//---------------------------------------------------------------------------
void File_Mxf::DolbyNamespaceURI()
{
    //Parsing
    Ztring Data;
    Get_UTF8 (Length2, Data,                                    "Data"); Element_Info1(Data);

    Descriptor_Fill("CodecID", Data);
}

//---------------------------------------------------------------------------
void File_Mxf::PHDRDataDefinition()
{
    //Parsing
    int128u Value;
    Get_UUID (Value,                                            "Value"); Element_Info1(Ztring().From_UUID(Value));

    Ztring CodecID;
    CodecID.From_Number(Value.lo, 16);
    if (CodecID.size()<16) {
        CodecID.insert(0, 16-CodecID.size(), __T('0'));
    }
    Descriptor_Fill("CodecID", CodecID);
}

//---------------------------------------------------------------------------
void File_Mxf::PHDRSourceTrackID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
        if (Descriptors[InstanceUID].LinkedTrackID==(int32u)-1)
            Descriptors[InstanceUID].LinkedTrackID=Data;
    FILLING_END();
}

//---------------------------------------------------------------------------
void File_Mxf::PHDRSimplePayloadSID()
{
    //Parsing
    int32u Data;
    Get_B4 (Data,                                               "Data"); Element_Info1(Data);

    FILLING_BEGIN();
         ExtraMetadata_SID.insert(Data);
    FILLING_END();
}

//***************************************************************************
// Basic types
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mxf::Get_Rational(float64 &Value)
{
    //Parsing
    int32u N, D;
    Get_B4 (N,                                                  "Numerator");
    Get_B4 (D,                                                  "Denominator");
    if (D)
        Value=((float64)N)/D;
    else
        Value=0; //Error
}

//---------------------------------------------------------------------------
void File_Mxf::Skip_Rational()
{
    //Parsing
    Skip_B4(                                                    "Numerator");
    Skip_B4(                                                    "Denominator");
}

//---------------------------------------------------------------------------
void File_Mxf::Info_Rational()
{
    //Parsing
    Info_B4(N,                                                  "Numerator");
    Info_B4(D,                                                  "Denominator");
    Element_Info1C(D, ((float32)N)/D);
}

//---------------------------------------------------------------------------
void File_Mxf::Get_UL(int128u &Value, const char* Name, const char* (*Param) (int128u))
{
    //Parsing
    Element_Begin1(Name);
    int128u V;
    int64u Temp;
    Peek_B8(Temp);
    V.hi = Temp;
    bool IsSmpte = (V.hi >> 32) == 0x060E2B34;
    #if MEDIAINFO_TRACE
    if (Trace_Activated && IsSmpte) {
    Skip_B1(                                                    "Identifier"); Param_Info1("Object Identifier");
    Skip_B1(                                                    "Length");
    Skip_B1(                                                    "Hierarchy"); Param_Info1("ISO");
    Skip_B1(                                                    "Organization"); Param_Info1("SMPTE");
    Skip_B1(                                                    "Category"); Param_Info1(Mxf_Category((int8u)(V.hi >> 24)));
    Skip_B1(                                                    "Registry"); Param_Info1(Mxf_Registry((int16u)(V.hi >> 16)));
    Skip_B1(                                                    "Structure");
    Skip_B1(                                                    "Version");
    }
    else
    #endif //MEDIAINFO_TRACE
    if (Element_Size-Element_Offset>=8)
        Element_Offset+=8;
    else
        Element_Offset=Element_Size;
    Peek_B8(Temp);
    V.lo = Temp;
    #if MEDIAINFO_TRACE
    if (Trace_Activated && IsSmpte) {
        vector<string> Param_Extra;
        int Offset = 0;
        while (Offset < 64) {
            if (!(V.lo & ((int64u)-1) >> Offset) && (((int32u)(V.hi)) >> 8) != 0x010201 && (V.lo >> 16) != (0x0D0102010000LL >> 16)) { // Essence, operation patterns may have trailing zero
                break;
            }
            const char* FieldName = nullptr;
            switch ((((int32u)(V.hi)) >> 8)) {
            case 0x010201: // Dictionary Essence 01
                switch (V.lo >> 24) {
                case 0x0D01030101:
                case 0x0D01030102:
                    switch (Offset) {
                    case 40: FieldName = "Mapping Kind"; break;
                    }
                    break;
                case 0x0D01030104:
                case 0x0D01030114:
                    switch (Offset) {
                    case 40: FieldName = "System Scheme Identifier"; break;
                    case 48: FieldName = "Metadata or Control Element Identifier"; break;
                    case 56: FieldName = "Reserved for use by Metadata Element"; break;
                    }
                    break;
                case 0x0D01030105:
                case 0x0D01030106:
                case 0x0D01030107:
                case 0x0D01030115:
                case 0x0D01030116:
                case 0x0D01030117:
                case 0x0D01030118:
                case 0x0E09060701:
                    switch (Offset) {
                    case 40: FieldName = "Essence Element Count"; break;
                    case 48: FieldName = "Essence Element Type"; break;
                    case 56: FieldName = "Essence Element Number"; break;
                    }
                    break;
                case 0x0E09050201:
                    switch (Offset) {
                    case 40: FieldName = "Essence Element Number"; break;
                    case 48: FieldName = "Essence Element Type"; break;
                    case 56: FieldName = "Essence Element Count"; break;
                    }
                    break;
                }
                break;
            case 0x040101: // Label Fixed 01
                switch (V.lo >> 16) {
                case 0x0D0103010201LL:
                    switch (Offset) {
                    case 48: FieldName = "MPEG Constraints"; break;
                    case 56: FieldName = "Template Extension"; break;
                    }
                    break;
                case 0x0D0103010202LL:
                    switch (Offset) {
                    case 48: FieldName = "Source Coding"; break;
                    case 56: FieldName = "Wrapping Kind"; break;
                    }
                    break;
                case 0x0D0103010203LL:
                    switch (Offset) {
                    case 48: FieldName = "Source Coding"; break;
                    case 56: FieldName = "Template Extension"; break;
                    }
                    break;
                case 0x0D0103010204LL:
                case 0x0D0103010207LL:
                case 0x0D0103010208LL:
                case 0x0D0103010209LL:
                case 0x0D010301020FLL:
                case 0x0D010301021FLL:
                case 0x0D0103010020LL:
                    switch (Offset) {
                    case 48: FieldName = "ISO 13818-1 stream_id bits 6..0"; break;
                    case 56: FieldName = "Wrapping Kind"; break;
                    }
                    break;
                case 0x0D0103010206LL:
                case 0x0D010301020ALL:
                case 0x0D010301020BLL: 
                case 0x0D0103010210LL:
                case 0x0D0103010211LL:
                case 0x0D0103010214LL:
                case 0x0D0103010215LL:
                case 0x0D0103010216LL:
                case 0x0D0103010217LL:
                case 0x0D0103010218LL:
                case 0x0D0103010219LL:
                case 0x0D010301021ALL:
                case 0x0D010301021BLL:
                case 0x0D010301021CLL:
                case 0x0D010301021DLL:
                case 0x0D010301021ELL:
                case 0x0D0103010221LL:
                case 0x0D0103010222LL:
                case 0x0D0103010223LL:
                case 0x0D0103010225LL:
                    switch (Offset) {
                    case 48: FieldName = "Wrapping Kind"; break;
                    }
                    break;
                case 0x0D0102010202LL:
                    switch (Offset) {
                    case 56: FieldName = "OP2a qualifiers"; break;
                    }
                    break;
                }
                break;
            }
            if (!FieldName) {
                FieldName = Mxf_Param_Name((int32u)V.hi, Offset ? (V.lo & ((int64u)-1) << (64 - Offset)) : 0);
            }
            Skip_B1(FieldName);
            Offset += 8;
            const char* MoreInfo = nullptr;
            switch ((((int32u)(V.hi)) >> 8)) {
            case 0x010201: // Dictionary Essence 01
                switch (V.lo >> 24) {
                case 0x0D01030105:
                case 0x0D01030106:
                case 0x0D01030107:
                case 0x0D01030115:
                case 0x0D01030116:
                case 0x0D01030117:
                case 0x0D01030118:
                case 0x0E09050201:
                case 0x0E09060701:
                    switch (Offset) {
                    case 56: MoreInfo = Mxf_Param_Info((int32u)V.hi, V.lo & 0xFFFFFFFFFF00FF00); break;
                    }
                    break;
                }
                break;
            }
            if (!MoreInfo) {
                MoreInfo = Mxf_Param_Info((int32u)V.hi, Offset ? (V.lo & ((int64u)-1) << (64 - Offset)) : 0);
            }
            if (MoreInfo) {
                Param_Info1(MoreInfo);
            }

            // Special cases
            switch ((((int32u)(V.hi)) >> 8)) {
            case 0x040101: // Value Fixed 01
                switch ((int32u)(V.lo >> 32)) {
                case 0x0D010201: // MXF OP Structure Version 1
                    switch (Offset) {
                    case 56: {
                        auto Code7 = (int8u)(V.lo >> 8);
                        Skip_Flags(Code7, 3,                    "MultiTrack");
                        Skip_Flags(Code7, 2,                    "NonStream");
                        Skip_Flags(Code7, 1,                    "External");
                    }
                    }
                    break;
                }
                switch (V.lo >> 24) {
                case 0x0D01020102LL: // MXF OP Structure Version 1 MXF OP2
                case 0x0D01020103LL: // MXF OP Structure Version 1 MXF OP3
                    switch (Offset) {
                    case 64: {
                        auto Code8 = (int8u)V.lo;
                        Skip_Flags(Code8, 4,                    "MayProcess");
                    }
                    }
                    break;
                }
                switch (V.lo >> 16) {
                case 0x0D0103010206LL:
                case 0x0D010301020ALL:
                case 0x0D010301020BLL:
                case 0x0D0103010210LL:
                case 0x0D0103010211LL:
                case 0x0D0103010214LL:
                case 0x0D0103010215LL:
                case 0x0D0103010216LL:
                case 0x0D0103010217LL:
                case 0x0D0103010218LL:
                case 0x0D0103010219LL:
                case 0x0D010301021ALL:
                case 0x0D010301021BLL:
                case 0x0D010301021CLL:
                case 0x0D010301021DLL:
                case 0x0D010301021ELL:
                case 0x0D0103010221LL:
                case 0x0D0103010222LL:
                case 0x0D0103010223LL:
                case 0x0D0103010225LL:
                    switch (Offset) {
                    case 56:
                        Param_Info1(Mxf_EssenceContainer_Mapping(V.lo));
                    }
                    break;
                case 0x0D0103010202LL:
                case 0x0D0103010204LL:
                case 0x0D0103010207LL:
                case 0x0D0103010208LL:
                case 0x0D0103010209LL:
                case 0x0D010301020FLL:
                case 0x0D010301021FLL:
                case 0x0D0103010020LL:
                    switch (Offset) {
                    case 64:
                        Param_Info1(Mxf_EssenceContainer_Mapping(V.lo));
                    }
                    break;
                }
                break;
            }
        }

        static const char* Reserved = "Reserved";
        switch (Offset) {
        case  0: Skip_B8(                                       Reserved); break;
        case  8: Skip_B7(                                       Reserved); break;
        case 16: Skip_B6(                                       Reserved); break;
        case 24: Skip_B5(                                       Reserved); break;
        case 32: Skip_B4(                                       Reserved); break;
        case 40: Skip_B3(                                       Reserved); break;
        case 48: Skip_B2(                                       Reserved); break;
        case 56: Skip_B1(                                       Reserved); break;
        default: ;
        }

        const char* Temp = nullptr;
        if (Param) {
            Temp = Param(V);
        }
        if (!Temp) {
            switch ((((int32u)(V.hi)) >> 8)) {
            case 0x024301:
                switch (V.lo >> 8) {
                case 0x0D010301040102: // SDTI System Metadata Pack
                    V.lo -= V.lo & 0xFF;
                }
            }
            Temp = Mxf_Param_Info((int32u)V.hi, V.lo);
        }
        if (Temp) {
            string Info(Temp);
            for (const auto& Extra : Param_Extra) {
                Info += ' ';
                Info += Extra;
            }
            Element_Info1(Info);
        }
    }
    else
    #endif //MEDIAINFO_TRACE
    if (Element_Size-Element_Offset>=8)
        Element_Offset+=8;
    else
        Element_Offset=Element_Size;
    Element_End0();
    Value=V;
}

//---------------------------------------------------------------------------
void File_Mxf::Skip_UL(const char* Name)
{
    #ifdef MEDIAINFO_MINIMIZE_SIZE
        Skip_UUID();
    #else
        int128u Value;
        Get_UL(Value, Name, NULL);
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::Get_UMID(int256u &Value, const char* Name)
{
    Element_Name(Name);

    //Parsing
    Get_UUID (Value.hi,                                         "Fixed");
    Get_UUID (Value.lo,                                         "UUID"); Element_Info1(Ztring().From_UUID(Value.lo));
}

//---------------------------------------------------------------------------
void File_Mxf::Skip_UMID()
{
    //Parsing
    Skip_UUID(                                                  "Fixed");
    Info_UUID(Data,                                             "UUID"); Element_Info1(Ztring().From_UUID(Data));
}

//---------------------------------------------------------------------------
void File_Mxf::Get_Timestamp(Ztring &Value)
{
    //Parsing
    int16u  Year;
    int8u   Month, Day, Hours, Minutes, Seconds, Milliseconds;
    Get_B2 (Year,                                               "Year");
    Get_B1 (Month,                                              "Month");
    Get_B1 (Day,                                                "Day");
    Get_B1 (Hours,                                              "Hours");
    Get_B1 (Minutes,                                            "Minutes");
    Get_B1 (Seconds,                                            "Seconds");
    Get_B1 (Milliseconds,                                       "Milliseconds/4"); Param_Info2(Milliseconds*4, " ms");
    Value.From_Number(Year);
    Value+=__T('-');
    Ztring Temp;
    Temp.From_Number(Month);
    if (Temp.size()<2)
        Temp.insert(0, 1, __T('0'));
    Value+=Temp;
    Value+=__T('-');
    Temp.From_Number(Day);
    if (Temp.size()<2)
        Temp.insert(0, 1, __T('0'));
    Value+=Temp;
    Value+=__T(' ');
    Temp.From_Number(Hours);
    if (Temp.size()<2)
        Temp.insert(0, 1, __T('0'));
    Value+=Temp;
    Value+=__T(':');
    Temp.From_Number(Minutes);
    if (Temp.size()<2)
        Temp.insert(0, 1, __T('0'));
    Value+=Temp;
    Value+=__T(':');
    Temp.From_Number(Seconds);
    if (Temp.size()<2)
        Temp.insert(0, 1, __T('0'));
    Value+=Temp;
    Value+=__T('.');
    Temp.From_Number(Milliseconds*4);
    if (Temp.size()<3)
        Temp.insert(0, 3-Temp.size(), __T('0'));
    Value+=Temp;
}

//---------------------------------------------------------------------------
void File_Mxf::Skip_Timestamp()
{
    //Parsing
    Skip_B2(                                                    "Year");
    Skip_B1(                                                    "Month");
    Skip_B1(                                                    "Day");
    Skip_B1(                                                    "Hours");
    Skip_B1(                                                    "Minutes");
    Skip_B1(                                                    "Seconds");
    Info_B1(Milliseconds,                                       "Milliseconds/4"); Param_Info2(Milliseconds*4, " ms");
}

//---------------------------------------------------------------------------
void File_Mxf::Info_Timestamp()
{
    //Parsing
    Info_B2(Year,                                               "Year");
    Info_B1(Month,                                              "Month");
    Info_B1(Day,                                                "Day");
    Info_B1(Hours,                                              "Hours");
    Info_B1(Minutes,                                            "Minutes");
    Info_B1(Seconds,                                            "Seconds");
    Info_B1(Milliseconds,                                       "Milliseconds/4"); Param_Info2(Milliseconds*4, " ms");
    Element_Info1(Ztring::ToZtring(Year          )+__T('-')+
                 Ztring::ToZtring(Month         )+__T('-')+
                 Ztring::ToZtring(Day           )+__T(' ')+
                 Ztring::ToZtring(Hours         )+__T(':')+
                 Ztring::ToZtring(Minutes       )+__T(':')+
                 Ztring::ToZtring(Seconds       )+__T('.')+
                 Ztring::ToZtring(Milliseconds*4)         );
}

//---------------------------------------------------------------------------
void File_Mxf::Get_BER(int64u &Value, const char* Name)
{
    int8u Length;
    Get_B1(Length,                                              Name);
    if (Length<0x80)
    {
        Value=Length; //1-byte
        return;
    }

    Length&=0x7F;
    switch (Length)
    {
        case 1 :
                {
                int8u  Length1;
                Get_B1(Length1,                                 Name);
                Value=Length1;
                break;
                }
        case 2 :
                {
                int16u Length2;
                Get_B2(Length2,                                 Name);
                Value=Length2;
                break;
                }
        case 3 :
                {
                int32u Length3;
                Get_B3(Length3,                                 Name);
                Value=Length3;
                break;
                }
        case 4 :
                {
                int32u Length4;
                Get_B4(Length4,                                 Name);
                Value=Length4;
                break;
                }
        case 5 :
                {
                int64u Length5;
                Get_B5(Length5,                                 Name);
                Value=Length5;
                break;
                }
        case 6 :
                {
                int64u Length6;
                Get_B6(Length6,                                 Name);
                Value=Length6;
                break;
                }
        case 7 :
                {
                int64u Length7;
                Get_B7(Length7,                                 Name);
                Value=Length7;
                break;
                }
        case 8 :
                {
                int64u Length8;
                Get_B8(Length8,                                 Name);
                Value=Length8;
                break;
                }
        default:Value=(int64u)-1; //Problem
    }
}

//***************************************************************************
// Parsers
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser__FromCodingScheme(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    if (Config->ParseSpeed<0)
        return;

    if ((Descriptor->second.EssenceCompression.hi&0xFFFFFFFFFFFFFF00LL)!=0x060E2B3404010100LL || (Descriptor->second.EssenceCompression.lo&0xFF00000000000000LL)!=0x0400000000000000LL)
        return ChooseParser__FromEssenceContainer (Essence, Descriptor);

    int8u Code2=(int8u)((Descriptor->second.EssenceCompression.lo&0x00FF000000000000LL)>>48);
    int8u Code3=(int8u)((Descriptor->second.EssenceCompression.lo&0x0000FF0000000000LL)>>40);
    int8u Code4=(int8u)((Descriptor->second.EssenceCompression.lo&0x000000FF00000000LL)>>32);
    int8u Code5=(int8u)((Descriptor->second.EssenceCompression.lo&0x00000000FF000000LL)>>24);
    int8u Code6=(int8u)((Descriptor->second.EssenceCompression.lo&0x0000000000FF0000LL)>>16);
    int8u Code7=(int8u)((Descriptor->second.EssenceCompression.lo&0x000000000000FF00LL)>> 8);

    switch (Code2)
    {
        case 0x01 : //Picture
                    switch (Code3)
                    {
                        case 0x02 : //Coding characteristics
                                    switch (Code4)
                                    {
                                        case 0x01 : //Uncompressed Picture Coding
                                                    switch (Code5)
                                                    {
                                                        case 0x01 : return ChooseParser_Raw(Essence, Descriptor);
                                                        case 0x7F : return ChooseParser_RV24(Essence, Descriptor);
                                                        default   : return;
                                                    }
                                        case 0x02 : //Compressed coding
                                                    switch (Code5)
                                                    {
                                                        case 0x01 : //MPEG Compression
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x01 :
                                                                        case 0x02 :
                                                                        case 0x03 :
                                                                        case 0x04 :
                                                                        case 0x11 : return ChooseParser_Mpegv(Essence, Descriptor);
                                                                        case 0x20 : return ChooseParser_Mpeg4v(Essence, Descriptor);
                                                                        case 0x30 :
                                                                        case 0x31 :
                                                                        case 0x32 :
                                                                        case 0x33 :
                                                                        case 0x34 :
                                                                        case 0x35 :
                                                                        case 0x36 :
                                                                        case 0x37 :
                                                                        case 0x38 :
                                                                        case 0x39 :
                                                                        case 0x3A :
                                                                        case 0x3B :
                                                                        case 0x3C :
                                                                        case 0x3D :
                                                                        case 0x3E :
                                                                        case 0x3F : return ChooseParser_Avc(Essence, Descriptor);
                                                                        default   : return;
                                                                    }
                                                        case 0x02 : return ChooseParser_DV(Essence, Descriptor);
                                                        case 0x03 : //Individual Picture Coding Schemes
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x01 : return ChooseParser_Jpeg2000(Essence, Descriptor);
                                                                        case 0x06 : return ChooseParser_ProRes(Essence, Descriptor);
                                                                        default   : return;
                                                                    }
                                                        case 0x71 : return ChooseParser_Vc3(Essence, Descriptor);
                                                        default   : return;
                                                    }
                                         default   : return;
                                    }
                         default   : return;
                    }
        case 0x02 : //Sound
                    switch (Code3)
                    {
                        case 0x02 : //Coding characteristics
                                    switch (Code4)
                                    {
                                        case 0x01 : //Uncompressed Sound Coding
                                                    ChooseParser__FromEssenceContainer (Essence, Descriptor); //e.g. for D-10 Audio
                                                    if (!Essence->second.Parsers.empty())
                                                        return;
                                                    switch (Code5)
                                                    {
                                                        case 0x02 :
                                                                    return ChooseParser_Mga(Essence, Descriptor);
                                                        case 0x01 :
                                                        case 0x7E :
                                                        case 0x7F : if (Descriptor->second.ChannelCount==1) //PCM, but one file is found with Dolby E in it
                                                                        ChooseParser_ChannelGrouping(Essence, Descriptor);
                                                                    if (Descriptor->second.ChannelCount==2) //PCM, but one file is found with Dolby E in it
                                                                        ChooseParser_SmpteSt0337(Essence, Descriptor);
                                                                    if (Descriptor->second.ChannelCount>=2 && Descriptor->second.ChannelCount!=(int32u)-1) //PCM, but one file is found with Dolby E in it
                                                                        ChooseParser_ChannelSplitting(Essence, Descriptor);
                                                                    [[fallthrough]];
                                                        default   : return ChooseParser_Pcm(Essence, Descriptor);
                                                    }
                                        case 0x02 : //Compressed coding
                                                    switch (Code5)
                                                    {
                                                        case 0x03 : //Compressed Audio Coding
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x01 : //Compandeded Audio Coding
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : if ((Descriptor->second.EssenceContainer.lo&0xFFFF0000)==0x02060000) //Test coherency between container and compression
                                                                                                        return ChooseParser_Pcm(Essence, Descriptor); //Compression is A-Law but Container is PCM, not logic, prioritizing PCM
                                                                                                     else
                                                                                                        return ChooseParser_Alaw(Essence, Descriptor);
                                                                                        case 0x10 : return ChooseParser_Pcm(Essence, Descriptor); //DV 12-bit
                                                                                        default   : return;
                                                                                    }
                                                                        case 0x02 : //SMPTE 338M Audio Coding
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : if (Descriptor->second.IsAes3Descriptor)
                                                                                                        return ChooseParser_SmpteSt0337(Essence, Descriptor);
                                                                                                    else
                                                                                                        return ChooseParser_Ac3(Essence, Descriptor);
                                                                                        case 0x04 :
                                                                                        case 0x05 :
                                                                                        case 0x06 : if (Descriptor->second.IsAes3Descriptor)
                                                                                                        return ChooseParser_SmpteSt0337(Essence, Descriptor);
                                                                                                    else
                                                                                                        return ChooseParser_Mpega(Essence, Descriptor);
                                                                                        case 0x0A : return ChooseParser_Iab(Essence, Descriptor);
                                                                                        case 0x1C : if (Descriptor->second.ChannelCount==1)
                                                                                                        return ChooseParser_ChannelGrouping(Essence, Descriptor); //Dolby E (in 2 mono streams)
                                                                                                    else
                                                                                                        return ChooseParser_SmpteSt0337(Essence, Descriptor); //Dolby E (in 1 stereo streams)
                                                                                        default   : return;
                                                                                    }
                                                                        case 0x03 : //MPEG-2 Coding (not defined in SMPTE 338M)
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 : return ChooseParser_Aac(Essence, Descriptor);
                                                                                        default   : return;
                                                                                    }
                                                                        case 0x04 : //MPEG-4 Audio Coding
                                                                                    switch (Code7)
                                                                                    {
                                                                                        case 0x01 :
                                                                                        case 0x02 :
                                                                                        case 0x03 :
                                                                                        case 0x04 :
                                                                                        case 0x05 :
                                                                                        case 0x06 :
                                                                                        case 0x07 :
                                                                                        case 0x08 : return ChooseParser_Aac(Essence, Descriptor);
                                                                                        default   : return;
                                                                                    }
                                                                        default   : return;
                                                                    }

                                                        case 0x04 : //MPEG Compressed Audio (SMPTE ST 381-4)
                                                                    switch (Code6)
                                                                    {
                                                                        case 0x03 :
                                                                        case 0x04 : return ChooseParser_Aac(Essence, Descriptor);
                                                                        default   : return;
                                                                    }

                                                         default   : return;
                                                    }
                                         default   : return;
                                    }
                         default   : return;
                    }
        default   : return;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser__FromEssenceContainer(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    switch (Descriptor->second.EssenceContainer.lo & 0xFFFFFFFFFFFF0000) {
    case Labels::MXFGCSMPTED10Mappings:
        switch (Descriptor->second.StreamKind)
        {
        case Stream_Video: ChooseParser_Mpegv(Essence, Descriptor); break;
        case Stream_Audio: ChooseParser_SmpteSt0331(Essence, Descriptor); break;
        default:;
        }
        break;
    case Labels::MXFGCDVDIFMappings: ChooseParser_DV(Essence, Descriptor); break;
    //case Labels::MXFGCSMPTED11Mappings:
    //case Labels::MXFGCMPEGES:
    case Labels::MXFGCUncompressedPictures: ChooseParser_Raw(Essence, Descriptor); break;
    case Labels::MXFGCAESBWFAudio:
        if (Descriptor->second.ChannelCount == 1) { //PCM, but one file is found with Dolby E in it
            ChooseParser_ChannelGrouping(Essence, Descriptor);
        }
        if (Descriptor->second.ChannelCount == 2) { //PCM, but one file is found with Dolby E in it
            ChooseParser_SmpteSt0337(Essence, Descriptor);
        }
        if (Descriptor->second.ChannelCount > 2 && Descriptor->second.ChannelCount != (int32u)-1) { //PCM, but one file is found with Dolby E in it
            ChooseParser_ChannelSplitting(Essence, Descriptor);
        }
        ChooseParser_Pcm(Essence, Descriptor);
        break;
    //case Labels::MXFGCMPEGPES:
    //case Labels::MXFGCMPEGPS:
    //case Labels::MXFGCMPEGTS:
    case Labels::MXFGCALawAudioMappings: ChooseParser_Alaw(Essence, Descriptor); break;
    //case Labels::MXFGCEncryptedDataMappings:
    case Labels::MXFGCJPEG2000PictureMappings: ChooseParser_Jpeg2000(Essence, Descriptor); break;
    //case Labels::MXFGCGenericVBIDataMappingUndefinedPayload:
    //case Labels::MXFGCGenericANCDataMappingUndefinedPayload:
    //case Labels::MXFGCAVCNALUnitStream:
    case Labels::MXFGCAVCByteStream: ChooseParser_Avc(Essence, Descriptor); break;
    case Labels::AvidTechnologyIncVC3Pictures:
    case Labels::MXFGCVC3Pictures: ChooseParser_Vc3(Essence, Descriptor); break;
    case Labels::MXFGCVC1Pictures: ChooseParser_Vc1(Essence, Descriptor); break;
    case Labels::MXFGCGenericData: ChooseParser_TimedText(Essence, Descriptor); break;
    //case Labels::MXFGCTIFFEP:
    //case Labels::MXFGCVC2Pictures:
    case Labels::MXF_GC_AAC_ADIF: ChooseParser_Adif(Essence, Descriptor); break;
    case Labels::MXF_GC_AAC_ADTS: ChooseParser_Adts(Essence, Descriptor); break;
    case Labels::MXF_GC_AAC_LATM_LOAS: ChooseParser_Latm(Essence, Descriptor); break;
    //case Labels::MXFGCACESPictures:
    //case Labels::MXFGCDMCVTData:
    //case Labels::MXFGCVC5EssenceContainerLabel:
    case Labels::MXFGCEssenceContainerProResPicture: ChooseParser_ProRes(Essence, Descriptor); break;
    case Labels::MXFGCImmersiveAudio: ChooseParser_Iab(Essence, Descriptor); break;
    case Labels::MXFGCEssenceContainerDNxPacked: ChooseParser_Vc3(Essence, Descriptor); break;
    //case Labels::MXFGCHEVCNALUnitStream: (Essence, Descriptor); break;
    case Labels::MXFGCHEVCByteStream: ChooseParser_Hevc(Essence, Descriptor); break;
    case Labels::MXFGCJPEGXSPictures: ChooseParser_JpegXs(Essence, Descriptor); break;
    case Labels::MXFGCFFV1Pictures: ChooseParser_Ffv1(Essence, Descriptor); break;
    case Labels::MXFGCMGAAudioMappings: ChooseParser_Mga(Essence, Descriptor); break;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser__FromEssence(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    if (Config->ParseSpeed<0)
        return;

    switch (Code.lo & 0xFFFFFFFFFF00FF00) {
    case Essences::D10Video: ChooseParser_Mpegv(Essence, Descriptor); break;
    case Essences::D10Audio: ChooseParser_SmpteSt0331(Essence, Descriptor); break;
    case Essences::UncompressedSystem_Line: ChooseParser_Mxf(Essence, Descriptor); break;
    case Essences::D11Video: ChooseParser_Hdcam(Essence, Descriptor); break;
    case Essences::Uncompressed_Frame:
    case Essences::Uncompressed_Clip:
    case Essences::Uncompressed_Line: ChooseParser_Raw(Essence, Descriptor); break;
    case Essences::MPEG_Frame:
    case Essences::MPEG_Clip:
    case Essences::MPEG_Custom: ChooseParser_Mpegv(Essence, Descriptor); ChooseParser_Avc(Essence, Descriptor); ChooseParser_Hevc(Essence, Descriptor); break;
    case Essences::JPEG2000: ChooseParser_Jpeg2000(Essence, Descriptor); break;
    case Essences::VC1_Frame:
    case Essences::VC1_Clip: ChooseParser_Vc1(Essence, Descriptor); break;
    case Essences::AvidTechnologyInc_VC3_Frame:
    case Essences::AvidTechnologyInc_VC3_Clip:
    case Essences::AvidTechnologyInc_VC3_Custom:
    case Essences::VC3_Frame:
    case Essences::VC3_Clip: ChooseParser_Vc3(Essence, Descriptor); break;
    case Essences::ProRes: ChooseParser_ProRes(Essence, Descriptor); break;
    case Essences::FFV1_Frame:
    case Essences::FFV1_Clip: ChooseParser_Ffv1(Essence, Descriptor); break;
    case Essences::PCM1:
    case Essences::PCM2:
    case Essences::DVAudio:
    case Essences::PCM_P2: ChooseParser_Pcm(Essence, Descriptor); break;
    case Essences::MPEGA_Frame:
    case Essences::MPEGA_Clip:
    case Essences::MPEGA_Custom: ChooseParser_Aac(Essence, Descriptor); ChooseParser_Mpega(Essence, Descriptor); break;
    case Essences::ALaw_Frame:
    case Essences::ALaw_Clip:
    case Essences::ALaw_Custom: ChooseParser_Alaw(Essence, Descriptor); break;
    case Essences::IAB_Temp:
    case Essences::IAB_Clip:
    case Essences::IAB_Frame: ChooseParser_Iab(Essence, Descriptor); break;
    case Essences::MGA_Frame:
    case Essences::MGA_Clip: ChooseParser_Mga(Essence, Descriptor); break;
    case Essences::VBI_Frame: ChooseParser_Vbi(Essence, Descriptor); break;
    case Essences::ANC_Frame: ChooseParser_Ancillary(Essence, Descriptor); break;
    case Essences::xANC_Line:
    case Essences::VANC_Line:
    case Essences::HANC_Line: ChooseParser_xAnc(Essence, Descriptor); break;
    case Essences::TimedText: ChooseParser_TimedText(Essence, Descriptor); break;
    case Essences::DVDIF_Frame:
    case Essences::DVDIF_Clip: ChooseParser_DV(Essence, Descriptor); break;
    case Essences::FrameWrappedISXDData: ChooseParser_Isxd(Essence, Descriptor); break;
    case Essences::FrameWrappedISXDData2: ChooseParser_Isxd(Essence, Descriptor); break;
    case Essences::PHDRImageMetadataItem: ChooseParser_Phdr(Essence, Descriptor); break;
    case Essences::HdrVividMetadataItem: ChooseParser_HdrVivid(Essence, Descriptor); break;
    }
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Avc(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_AVC_YES)
        File_Avc* Parser=new File_Avc;
        MayHaveCaptionsInStream=true;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "AVC");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_DV(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_DVDIF_YES)
        File_DvDif* Parser=new File_DvDif;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "PCM");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Mpeg4v(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_MPEG4V_YES)
        File_Mpeg4v* Parser=new File_Mpeg4v;
        Open_Buffer_Init(Parser);
        Parser->OnlyVOP();
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "MPEG-4 Visual");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Mpegv(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_MPEGV_YES)
        File_Mpegv* Parser=new File_Mpegv();
        Parser->Ancillary=&Ancillary;
        MayHaveCaptionsInStream=true;
        #if MEDIAINFO_ADVANCED
            Parser->InitDataNotRepeated_Optional=true;
        #endif // MEDIAINFO_ADVANCED
        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX
    #else
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "MPEG Video");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Raw(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    File__Analyze* Parser=new File_Unknown();
    Open_Buffer_Init(Parser);
    Parser->Stream_Prepare(Stream_Video);
    Parser->Fill(Stream_Video, 0, Video_Format, "YUV");
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_RV24(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    File__Analyze* Parser=new File_Unknown();
    Open_Buffer_Init(Parser);
    Parser->Stream_Prepare(Stream_Video);
    Parser->Fill(Stream_Video, 0, Video_Format, "RV24");
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Vc3(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_VC3_YES)
        File_Vc3* Parser=new File_Vc3;
        if (Descriptor!=Descriptors.end())
            Parser->FrameRate=Descriptor->second.SampleRate;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "VC-3");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_TimedText(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Text;

    //Filling
    #if defined(MEDIAINFO_TTML_YES)
        File_Ttml* Parser=new File_Ttml;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Text);
        Parser->Fill(Stream_Text, 0, Text_Format, "Timed Text");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Aac(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_AAC_YES)
        File_Aac* Parser=new File_Aac;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Adif(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_AAC_YES)
        File_Aac* Parser=new File_Aac;
        Parser->Mode=File_Aac::Mode_ADIF;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
        Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "ADIF");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Adts(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_AAC_YES)
        File_Aac* Parser=new File_Aac;
        Parser->Mode=File_Aac::Mode_ADTS;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
        Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "ADTS");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Latm(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_AAC_YES)
        File_Aac* Parser=new File_Aac;
        Parser->Mode=File_Aac::Mode_LATM;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "AAC");
        Parser->Fill(Stream_Audio, 0, Audio_MuxingMode, "LATM");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Ac3(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_AC3_YES)
        File_Ac3* Parser=new File_Ac3;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "AC-3");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Alaw(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    File__Analyze* Parser=new File_Unknown();
    Open_Buffer_Init(Parser);
    Parser->Stream_Prepare(Stream_Audio);
    Parser->Fill(Stream_Audio, 0, Audio_Format, "Alaw");
    Essence->second.Parsers.push_back(Parser);
}


//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_ChannelGrouping(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;
    if ((Essence->first&0x000000FF)==0x00000000)
        StreamPos_StartAtZero.set(Essence->second.StreamKind); // Need to do it here because we use StreamPos_StartAtZero immediately

    #if defined(MEDIAINFO_SMPTEST0337_YES)

    //Creating the parser
    if (!((Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1))%2 && Essences[Essence->first-1].Parsers.size()<=1))
    {
        File_ChannelGrouping* Parser;
        if ((Essence->second.StreamPos-(StreamPos_StartAtZero[Essence->second.StreamKind]?0:1))%2) //If the first half-stream was already rejected, don't try this one
        {
            essences::iterator FirstChannel=Essences.find(Essence->first-1);
            if (FirstChannel==Essences.end() || !FirstChannel->second.IsChannelGrouping)
                return ChooseParser_Pcm(Essence, Descriptor); //Not a channel grouping

            Parser=new File_ChannelGrouping;
            Parser->Channel_Pos=1;
            Parser->Common=((File_ChannelGrouping*)Essences[Essence->first-1].Parsers[0])->Common;
            Parser->StreamID=Essence->second.TrackID-1;
        }
        else
        {
            Parser=new File_ChannelGrouping;
            Parser->Channel_Pos=0;
            if (Descriptor!=Descriptors.end())
            {
                std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("SamplingRate");
                if (i!=Descriptor->second.Infos.end())
                    Parser->SamplingRate=i->second.To_int16u();
            }
            Essence->second.IsChannelGrouping=true;
        }
        Parser->Channel_Total=2;
        if (Descriptor!=Descriptors.end())
        {
            Parser->BitDepth=(int8u)(Descriptor->second.BlockAlign<=4?(Descriptor->second.BlockAlign*8):(Descriptor->second.BlockAlign*4)); //In one file, BlockAlign is size of the aggregated channelgroup
            std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("Format_Settings_Endianness");
            if (i!=Descriptor->second.Infos.end())
            {
                if (i->second==__T("Big"))
                    Parser->Endianness='B';
                else
                    Parser->Endianness='L';
            }
            else
                Parser->Endianness='L';
        }
        else
            Parser->Endianness='L';

        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX

        Essence->second.Parsers.push_back(Parser);
    }
    #endif //defined(MEDIAINFO_SMPTEST0337_YES)

    //Adding PCM
    ChooseParser_Pcm(Essence, Descriptor);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_ChannelSplitting(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_SMPTEST0337_YES)
        File_ChannelSplitting* Parser=new File_ChannelSplitting;
        if (Descriptor!=Descriptors.end())
        {
            Parser->Channel_Total=Descriptor->second.ChannelCount;
            if (Descriptor->second.BlockAlign<64)
                Parser->BitDepth=(int8u)(Descriptor->second.BlockAlign*8/Descriptor->second.ChannelCount);
            else if (Descriptor->second.QuantizationBits!=(int32u)-1)
                Parser->BitDepth=(int8u)Descriptor->second.QuantizationBits;
            std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("SamplingRate");
            if (i!=Descriptor->second.Infos.end())
                Parser->SamplingRate=i->second.To_int16u();
            i=Descriptor->second.Infos.find("Format_Settings_Endianness");
            if (i!=Descriptor->second.Infos.end())
            {
                if (i->second==__T("Big"))
                    Parser->Endianness='B';
                else
                    Parser->Endianness='L';
            }
            else
                Parser->Endianness='L';
        }
        else
            Parser->Endianness='L';
        Parser->Aligned=true;

        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX

        Essence->second.Parsers.push_back(Parser);
    #endif //defined(MEDIAINFO_SMPTEST0337_YES)

    //Adding PCM
    ChooseParser_Pcm(Essence, Descriptor);
}
//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Mpega(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_MPEGA_YES)
        File_Mpega* Parser=new File_Mpega;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "MPEG Audio");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Pcm(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    int8u Channels=0;
    if (Descriptor!=Descriptors.end())
    {
        std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("Channel(s)");
        if (i!=Descriptor->second.Infos.end())
            Channels=i->second.To_int8u();

        //Handling some buggy cases
        if (Channels>1 && Descriptor->second.BlockAlign!=(int16u)-1 && Descriptor->second.QuantizationBits!=(int32u)-1)
        {
            if (((int32u)Descriptor->second.BlockAlign)*8==Descriptor->second.QuantizationBits)
                Descriptor->second.BlockAlign*=Channels; //BlockAlign is by channel, it should be by block.
        }
    }

    //Creating the parser
    #if defined(MEDIAINFO_PCM_YES)
        File_Pcm* Parser=new File_Pcm;
        if (Descriptor!=Descriptors.end())
        {
            if (Channels)
                Parser->Channels=Channels;
            std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("SamplingRate");
            if (i!=Descriptor->second.Infos.end())
                Parser->SamplingRate=i->second.To_int16u();
            if (Parser->Channels && Descriptor->second.BlockAlign!=(int16u)-1)
                Parser->BitDepth=(int8u)(Descriptor->second.BlockAlign*8/Parser->Channels);
            else if (Descriptor->second.QuantizationBits<256)
                Parser->BitDepth=(int8u)Descriptor->second.QuantizationBits;
            else
            {
                i=Descriptor->second.Infos.find("BitDepth");
                if (i!=Descriptor->second.Infos.end())
                    Parser->BitDepth=i->second.To_int8u();
            }
            //Handling of quantization bits not being same as BlockAlign/Channels
            if (Channels && Descriptor->second.BlockAlign!=(int16u)-1 && Descriptor->second.QuantizationBits!=(int32u)-1)
            {
                if (Channels*Descriptor->second.QuantizationBits!=((int32u)Descriptor->second.BlockAlign)*8)
                {
                    //Moving Bit depth info to the "Significant" piece of etadata
                    if (Descriptor->second.QuantizationBits<256)
                        Parser->BitDepth_Significant=(int8u)Descriptor->second.QuantizationBits;
                    else
                        Parser->BitDepth_Significant=Parser->BitDepth;
                    Parser->BitDepth=((int8u)Descriptor->second.BlockAlign)*8/Channels;
                }
            }
            i = Descriptor->second.Infos.find("Format_Settings_Endianness");
            if (i!=Descriptor->second.Infos.end())
            {
                if (i->second==__T("Big"))
                    Parser->Endianness='B';
                else
                    Parser->Endianness='L';
            }
            else
                Parser->Endianness='L';
        }
        else
            Parser->Endianness='L';

        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX

        if (Essence->second.Parsers.empty())
            Parser->Frame_Count_Valid=1;
        Essence->second.Parsers.push_back(Parser);
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_SmpteSt0331(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_SMPTEST0331_YES)
        File_SmpteSt0331* Parser=new File_SmpteSt0331;
        if (Descriptor!=Descriptors.end() && Descriptor->second.QuantizationBits!=(int32u)-1)
            Parser->QuantizationBits=Descriptor->second.QuantizationBits;

        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX

        Essence->second.Parsers.push_back(Parser);
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_SmpteSt0337(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_SMPTEST0337_YES)
        File_SmpteSt0337* Parser=new File_SmpteSt0337;
        if (Descriptor!=Descriptors.end())
        {
            if (Descriptor->second.BlockAlign<64)
                Parser->BitDepth=(int8u)(Descriptor->second.BlockAlign*4);
            else if (Descriptor->second.QuantizationBits!=(int32u)-1)
                Parser->BitDepth=(int8u)Descriptor->second.QuantizationBits;
            std::map<std::string, Ztring>::const_iterator i=Descriptor->second.Infos.find("Format_Settings_Endianness");
            if (i!=Descriptor->second.Infos.end())
            {
                if (i->second==__T("Big"))
                    Parser->Endianness='B';
                else
                    Parser->Endianness='L';
            }
            else
                Parser->Endianness='L';
        }
        else
            Parser->Endianness='L';
        Parser->Aligned=true;

        #if MEDIAINFO_DEMUX
            if (Demux_UnpacketizeContainer)
            {
                Parser->Demux_Level=2; //Container
                Parser->Demux_UnpacketizeContainer=true;
            }
        #endif //MEDIAINFO_DEMUX

        Essence->second.Parsers.push_back(Parser);
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Jpeg2000(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_JPEG_YES)
        File_Jpeg* Parser=new File_Jpeg;
        Parser->StreamKind=Stream_Video;
        if (Descriptor!=Descriptors.end())
        {
            Parser->Interlaced=Descriptor->second.Is_Interlaced() || Descriptor->second.Jp2kContentKind==4;
            Parser->MxfContentKind=Descriptor->second.Jp2kContentKind;
            #if MEDIAINFO_DEMUX
                if (Parser->Interlaced)
                {
                    Parser->Demux_Level=2; //Container
                    Parser->Demux_UnpacketizeContainer=true;
                    Parser->FrameRate=Descriptor->second.SampleRate;
                }
            #endif //MEDIAINFO_DEMUX
        }
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "JPEG 2000");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_ProRes(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_PRORES_YES)
        File_ProRes* Parser=new File_ProRes;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "ProRes");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Ffv1(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_FFV1_YES)
        File__Analyze* Parser=NULL;
        if (Descriptor!=Descriptors.end())
        {
            if (Descriptor->second.Parser)
            {
                Essence->second.Parsers.push_back(Descriptor->second.Parser);
                Descriptor->second.Parser=NULL;
            }
            else
            {
                for (size_t i=0; i<Descriptor->second.SubDescriptors.size(); i++)
                {
                    descriptors::iterator Sub=Descriptors.find(Descriptor->second.SubDescriptors[i]);
                    if (Sub!=Descriptors.end() && Sub->second.Parser)
                    {
                        Essence->second.Parsers.push_back(Sub->second.Parser);
                        Sub->second.Parser=NULL;
                    }
                }
            }
        }
        if (Essence->second.Parsers.empty())
            Essence->second.Parsers.push_back(new File_Ffv1);
        for (size_t i=0; i<Essence->second.Parsers.size(); i++)
        {
            File_Ffv1* Parser=(File_Ffv1*)Essence->second.Parsers[i]; // TODO
            if (Descriptor!=Descriptors.end())
            {
                Parser->Width=Descriptor->second.Width;
                Parser->Height=Descriptor->second.Height;
            }
            else
            {
                Parser->Width=0;
                Parser->Height=0;
            }
        }
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "FFV1");
        Essence->second.Parsers.push_back(Parser);
    #endif
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Isxd(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Other;
    Essence->second.Infos["MuxingMode"]="ISXD";
    Essence->second.Infos["MuxingMode_MoreInfo"]="Contains additional metadata for other tracks";

    //Filling
    ChooseParser_DolbyVisionFrameData(Essence, Descriptor);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Phdr(const essences::iterator& Essence, const descriptors::iterator& Descriptor)
{
    Essence->second.StreamKind = Stream_Other;
    Essence->second.Infos["MuxingMode"] = "PHDR";
    Essence->second.Infos["MuxingMode_MoreInfo"] = "Contains additional metadata for other tracks";

    //Filling
    ChooseParser_DolbyVisionFrameData(Essence, Descriptor);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_HdrVivid(const essences::iterator& Essence, const descriptors::iterator& Descriptor)
{
    Essence->second.StreamKind = Stream_Other;
    Essence->second.Infos["MuxingMode_MoreInfo"] = "Contains additional metadata for other tracks";

    //Filling
    #if 1 // TODO
        File_HdrVividMetadata* Parser=new File_HdrVividMetadata;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Parser->Fill(Stream_Other, 0, Other_Format, "HDR Vivid Metadata");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_DolbyVisionFrameData(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    //Filling
    #if 1 // TODO
        File_DolbyVisionMetadata* Parser=new File_DolbyVisionMetadata;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Parser->Fill(Stream_Other, 0, Other_Format, "Dolby Vision Metadata");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Iab(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_IAB_YES)
        File_Iab* Parser=new File_Iab;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "IAB");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Mga(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Audio;

    //Filling
    #if defined(MEDIAINFO_MGA_YES)
        File_Mga* Parser=new File_Mga;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Audio);
        Parser->Fill(Stream_Audio, 0, Audio_Format, "MGA");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Mxf(const essences::iterator& Essence, const descriptors::iterator& Descriptor)
{
    if (!IsSub)
        Essence->second.Parsers.push_back(new File_Mxf());
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Vc1(const essences::iterator& Essence, const descriptors::iterator& Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_VC1_YES)
        File_Vc1* Parser=new File_Vc1;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "VC-1");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Vbi(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    //Filling
    #if defined(MEDIAINFO_VBI_YES)
        File_Vbi* Parser=new File_Vbi;
        MayHaveCaptionsInStream=true;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Parser->Fill(Stream_Video, 0, Other_MuxingMode, "VBI");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Ancillary(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    //Filling
    #if defined(MEDIAINFO_ANCILLARY_YES)
        if (!Ancillary)
            Ancillary=new File_Ancillary;
        File__Analyze* Parser=Ancillary;
        MayHaveCaptionsInStream=true;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Parser->Fill(Stream_Video, 0, Other_MuxingMode, "Ancillary data");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_xAnc(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    //Filling
    #if 0
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Other);
        Parser->Fill(Stream_Video, 0, Other_MuxingMode, "xANC");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Hdcam(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if 0
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "HDCAM");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_Hevc(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if defined(MEDIAINFO_HEVC_YES)
        File_Hevc* Parser=new File_Hevc;
        MayHaveCaptionsInStream=true;
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "HEVC");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//---------------------------------------------------------------------------
void File_Mxf::ChooseParser_JpegXs(const essences::iterator &Essence, const descriptors::iterator &Descriptor)
{
    Essence->second.StreamKind=Stream_Video;

    //Filling
    #if 0
    #else
        //Filling
        File__Analyze* Parser=new File_Unknown();
        Open_Buffer_Init(Parser);
        Parser->Stream_Prepare(Stream_Video);
        Parser->Fill(Stream_Video, 0, Video_Format, "JPEG XS");
    #endif
    Essence->second.Parsers.push_back(Parser);
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
int32u File_Mxf::Vector(int32u ExpectedLength)
{
    if (Element_Size-Element_Offset<8)
    {
        Element_Error("Incoherent element size");
        return (int32u)-1;
    }

    int32u Count, Length;
    Get_B4 (Count,                                              "Count");
    Get_B4 (Length,                                             "Length");

    if (Count*Length>Element_Size-Element_Offset)
    {
        Param_Error("Incoherent Count*Length");
        return (int32u)-1;
    }

    if (Count && ExpectedLength!=(int32u)-1 && Length!=ExpectedLength)
    {
        Param_Error("Unexpected item length");
        return (int32u)-1;
    }

    return Count;
}

//---------------------------------------------------------------------------
void File_Mxf::Subsampling_Compute(descriptors::iterator Descriptor)
{
    if (Descriptor==Descriptors.end() || (Descriptor->second.SubSampling_Horizontal==(int32u)-1 || Descriptor->second.SubSampling_Vertical==(int32u)-1))
        return;

    switch (Descriptor->second.SubSampling_Horizontal)
    {
        case 1 :    switch (Descriptor->second.SubSampling_Vertical)
                    {
                        case 1 : Descriptor->second.Infos["ChromaSubsampling"]=__T("4:4:4"); return;
                        default: Descriptor->second.Infos["ChromaSubsampling"].clear(); return;
                    }
        case 2 :    switch (Descriptor->second.SubSampling_Vertical)
                    {
                        case 1 : Descriptor->second.Infos["ChromaSubsampling"]=__T("4:2:2"); return;
                        case 2 : Descriptor->second.Infos["ChromaSubsampling"]=__T("4:2:0"); return;
                        default: Descriptor->second.Infos["ChromaSubsampling"].clear(); return;
                    }
        case 4 :    switch (Descriptor->second.SubSampling_Vertical)
                    {
                        case 1 : Descriptor->second.Infos["ChromaSubsampling"]=__T("4:1:1"); return;
                        default: Descriptor->second.Infos["ChromaSubsampling"].clear(); return;
                    }
        default:    return;
    }
}


//---------------------------------------------------------------------------
void File_Mxf::ColorLevels_Compute(descriptors::iterator Descriptor, bool Force, int32u BitDepth)
{
    if (Descriptor == Descriptors.end())
        return;
    // BitDepth check
    std::map<std::string, Ztring>::iterator Info=Descriptor->second.Infos.find("BitDepth");
    if (Info!=Descriptor->second.Infos.end())
    {
        if (BitDepth==0 || BitDepth==(int32u)-1)
            BitDepth=Info->second.To_int32u();
        else if (Force && BitDepth!=Info->second.To_int32u())
            Fill(StreamKind_Last, StreamPos_Last, "BitDepth_Container", Info->second);
    }

    // Known values
    if (BitDepth>=8 && BitDepth<=16)
    {
        int32u Multiplier=1<<(BitDepth-8);
        if (Descriptor->second.MinRefLevel==16*Multiplier && Descriptor->second.MaxRefLevel==235*Multiplier && (Descriptor->second.Type==descriptor::Type_RGBA || Descriptor->second.ColorRange==1+224*Multiplier))
        {
            Descriptor->second.Infos["colour_range"]=__T("Limited");
            return;
        }
        if (Descriptor->second.MinRefLevel==0*Multiplier && Descriptor->second.MaxRefLevel==256*Multiplier-1 && (Descriptor->second.Type==descriptor::Type_RGBA || Descriptor->second.ColorRange==256*Multiplier))
        {
            Descriptor->second.Infos["colour_range"]=__T("Full");
            return;
        }
    }

    if (!Force && (Descriptor->second.MinRefLevel==(int32u)-1 || Descriptor->second.MaxRefLevel==(int32u)-1) || (Descriptor->second.Type!=descriptor::Type_RGBA && Descriptor->second.ColorRange==(int32u)-1))
        return;

    // Listing values
    ZtringList List;
    if (Descriptor->second.MinRefLevel!=(int32u)-1)
        List.push_back(__T("Min: ")+Ztring::ToZtring(Descriptor->second.MinRefLevel));
    if (Descriptor->second.MaxRefLevel!=(int32u)-1)
        List.push_back(__T("Max: ")+Ztring::ToZtring(Descriptor->second.MaxRefLevel));
    if (Descriptor->second.ColorRange!=(int32u)-1)
        List.push_back(__T("Chroma range: ")+Ztring::ToZtring(Descriptor->second.ColorRange));
    if (!List.empty())
    {
        List.Separator_Set(0, __T(", "));
        Descriptor->second.Infos["colour_range"]=List.Read();
    }
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
void File_Mxf::Locators_CleanUp()
{
    //Testing locators (TODO: check if this is still useful after refactoring, with MXF having essence and locators)
    if (Locators.size()==1 && !Essences.empty())
    {
        Locators.clear();
        return;
    }

    locators::iterator Locator=Locators.begin();
    while (Locator!=Locators.end())
    {
        bool IsReferenced=false;
        for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
            for (size_t Pos=0; Pos<Descriptor->second.Locators.size(); Pos++)
                if (Locator->first==Descriptor->second.Locators[Pos])
                    IsReferenced=true;
        if (!IsReferenced)
        {
            //Deleting current locator
            locators::iterator LocatorToDelete=Locator;
            ++Locator;
            Locators.erase(LocatorToDelete);
        }
        else
            ++Locator;
    }

}
#endif //defined(MEDIAINFO_REFERENCES_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
void File_Mxf::Locators_Test()
{
    Locators_CleanUp();

    if (!Locators.empty() && ReferenceFiles==NULL)
    {
        ReferenceFiles_Accept(this, Config);

        for (locators::iterator Locator=Locators.begin(); Locator!=Locators.end(); ++Locator)
            if (!Locator->second.IsTextLocator && !Locator->second.EssenceLocator.empty())
            {
                sequence* Sequence=new sequence;
                Sequence->AddFileName(Locator->second.EssenceLocator);
                Sequence->StreamKind=Locator->second.StreamKind;
                Sequence->StreamPos=Locator->second.StreamPos;
                if (Locator->second.LinkedTrackID!=(int32u)-1)
                    Sequence->StreamID=Locator->second.LinkedTrackID;
                else if (!Retrieve(Locator->second.StreamKind, Locator->second.StreamPos, General_ID).empty())
                    Sequence->StreamID=Retrieve(Locator->second.StreamKind, Locator->second.StreamPos, General_ID).To_int64u();
                Sequence->Delay=float64_int64s(DTS_Delay*1000000000);

                //Special cases
                if (Locator->second.StreamKind==Stream_Video)
                {
                    //Searching the corresponding frame rate
                    for (descriptors::iterator Descriptor=Descriptors.begin(); Descriptor!=Descriptors.end(); ++Descriptor)
                        for (size_t LocatorPos=0; LocatorPos<Descriptor->second.Locators.size(); LocatorPos++)
                            if (Descriptor->second.Locators[LocatorPos]==Locator->first)
                                Sequence->FrameRate_Set(Descriptor->second.SampleRate);
                }

                if (Sequence->StreamID!=(int32u)-1)
                {
                    //Descriptive Metadata
                    std::vector<int128u> ProductionFrameworks_List;

                    for (dmsegments::iterator DescriptiveMarker=DescriptiveMarkers.begin(); DescriptiveMarker!=DescriptiveMarkers.end(); ++DescriptiveMarker)
                        for (size_t Pos=0; Pos<DescriptiveMarker->second.TrackIDs.size(); Pos++)
                            if (DescriptiveMarker->second.TrackIDs[Pos]==Sequence->StreamID)
                                ProductionFrameworks_List.push_back(DescriptiveMarker->second.Framework);

                    for (size_t Pos=0; Pos<ProductionFrameworks_List.size(); Pos++)
                    {
                        dmscheme1s::iterator ProductionFramework=ProductionFrameworks.find(ProductionFrameworks_List[Pos]);
                        if (ProductionFramework!=ProductionFrameworks.end())
                        {
                            Sequence->Infos["Language"]=ProductionFramework->second.PrimaryExtendedSpokenLanguage;
                        }
                    }
                }

                ReferenceFiles->AddSequence(Sequence);
            }
            else
            {
                Fill(Stream_General, 0, "UnsupportedSources", Locator->second.EssenceLocator);
                Fill_SetOptions(Stream_General, 0, "UnsupportedSources", "N NT");
            }

        ReferenceFiles->ParseReferences();
    }
}
#endif //defined(MEDIAINFO_REFERENCES_YES)

//---------------------------------------------------------------------------
void File_Mxf::NextRandomIndexPack()
{
    //We have the necessary for indexes, jumping to next index
    Skip_XX(Element_Size-Element_Offset,                        "Data");
    if (RandomIndexPacks.empty())
    {
        if (!RandomIndexPacks_AlreadyParsed)
        {
            Partitions_Pos=0;
            while (Partitions_Pos<Partitions.size() && Partitions[Partitions_Pos].StreamOffset!=PartitionMetadata_PreviousPartition)
                Partitions_Pos++;
            if (Partitions_Pos==Partitions.size())
            {
                GoTo(PartitionMetadata_PreviousPartition);
                Open_Buffer_Unsynch();
            }
            else
                GoToFromEnd(0);
        }
        else
            GoToFromEnd(0);
    }
    else
    {
        GoTo(RandomIndexPacks[0].ByteOffset);
        RandomIndexPacks.erase(RandomIndexPacks.begin());
        Open_Buffer_Unsynch();
    }

    RandomIndexPacks_MaxOffset=(int64u)-1;
}

//---------------------------------------------------------------------------
bool File_Mxf::BookMark_Needed()
{
    Frame_Count_NotParsedIncluded=(int64u)-1;

    if (MayHaveCaptionsInStream && !IsSub && IsParsingEnd && IsParsingMiddle_MaxOffset==(int64u)-1)
    {
        int64u ProbeCaptionBytePos=(int64u)-1;
        int64u ProbeCaptionByteDur=(int64u)-1;
        float32 Duration=0;
        for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        {
            auto Count=Count_Get((stream_t)StreamKind);
            for (size_t StreamPos=0; StreamPos<Count; StreamPos++)
            {
                Duration=Retrieve_Const((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).To_int64u()/1000.0;
                if (Duration)
                    break;
            }
            if (Duration)
                break;
        }
        if (!Duration)
        {
            for (const auto& Track : Tracks)
            {
                if (!Track.second.EditRate)
                    continue;
                const auto& Component=Components.find(Track.second.Sequence);
                if (Component!=Components.end())
                {
                    if (Component->second.Duration && Component->second.Duration!=(int64u)-1)
                        Duration=Component->second.Duration/Track.second.EditRate;
                }
            }
        }
        auto Probe=Config->File_ProbeCaption_Get(ParserName);
        int64u HeaderSize=0;
        int64u ContentSize=File_Size;
        if (!Partitions.empty())
        {
            const auto& FirstPartition=Partitions.front();
            HeaderSize=FirstPartition.PartitionPackByteCount+FirstPartition.HeaderByteCount+FirstPartition.IndexByteCount;
            ContentSize-=HeaderSize;
            const auto& LastPartition=Partitions.back();
            if (LastPartition.StreamOffset==FirstPartition.FooterPartition)
            {
                 int64u FooterSize=File_Size-LastPartition.StreamOffset;
                 ContentSize-=FooterSize;
            }
        }
        switch (Probe.Start_Type)
        {
            case config_probe_none:
                ProbeCaptionBytePos=0;
                break;
            case config_probe_size:
                ProbeCaptionBytePos=Probe.Start;
                break;
            case config_probe_dur:
                if (Duration)
                {
                    ProbeCaptionBytePos=HeaderSize+(float32)Probe.Start/Duration*ContentSize; //TODO: real timestamp
                }
                else
                    ProbeCaptionBytePos=HeaderSize+ContentSize/2;
                break;
            case config_probe_percent:
                ProbeCaptionBytePos=HeaderSize+ContentSize/100*Probe.Start;
                break;
            default:;
        }
        switch (Probe.Duration_Type) {
            case config_probe_none:
                ProbeCaptionByteDur=ContentSize;
                break;
            case config_probe_size:
                ProbeCaptionByteDur=Probe.Duration;
                break;
            case config_probe_dur:
                if (Duration)
                {
                    ProbeCaptionByteDur=(float32)Probe.Duration/Duration*ContentSize; //TODO: real timestamp
                }
                else
                    ProbeCaptionByteDur=ContentSize/100;
                break;
            case config_probe_percent:
                ProbeCaptionByteDur=ContentSize/100*Probe.Duration;
                break;
        }
        auto MaxOffset=ProbeCaptionBytePos+ProbeCaptionByteDur;
        auto CurrentEnd=File_Offset+Buffer_Offset;
        if (ProbeCaptionBytePos!=(int64u)-1 && ProbeCaptionByteDur!=(int64u)-1 && File_Size/2>ProbeCaptionByteDur)
        {
            IsParsingMiddle_MaxOffset=MaxOffset;
            GoTo(ProbeCaptionBytePos);
            Open_Buffer_Unsynch();
            IsParsingEnd=false;
            IsCheckingRandomAccessTable=false;
            Streams_Count=(size_t)-1;
        }
    }

    if (ExtraMetadata_Offset!=(int64u)-1)
    {
        GoTo(ExtraMetadata_Offset);
        ExtraMetadata_Offset=(int64u)-1;
    }

    return false;
}

//---------------------------------------------------------------------------
void File_Mxf::Descriptor_Fill(const char* Name, const Ztring& Value)
{
    descriptor& Descriptor = Descriptors[InstanceUID];
    std::map<std::string, Ztring>::iterator Info = Descriptor.Infos.find(Name);

    //Ignore value if header partition has aleady a value
    if (Partitions_IsFooter && InstanceUID != int128u() && Info != Descriptor.Infos.end())
    {
        //Test
        if (Value != Info->second)
            Descriptor.Infos[string(Name)+"_Footer"] = Value;

        return;
    }

    if (Info == Descriptor.Infos.end())
        Descriptor.Infos[Name] = Value;
    else
        Info->second = Value;
}

} //NameSpace

#endif //MEDIAINFO_MXF_*
