/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Mxf files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_MxfH
#define MediaInfo_File_MxfH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__HasReferences.h"
#if defined(MEDIAINFO_ANCILLARY_YES)
    #include <MediaInfo/Multiple/File_Ancillary.h>
#endif //defined(MEDIAINFO_ANCILLARY_YES)
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/TimeCode.h"
#include <vector>
#include <set>
#include <bitset>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class File_DolbyVisionMetadata;
class File_Adm;
class File_Iab;
class File_DolbyAudioMetadata;

//***************************************************************************
// Class File_Mxf
//***************************************************************************

class File_Mxf : public File__Analyze, File__HasReferences
{
public :
    //Constructor/Destructor
    File_Mxf();
    ~File_Mxf();

    //int256u
    class int256u
    {
        public:
            // Binary correct representation of signed 256bit integer
            int128u lo;
            int128u hi;

            int256u()
            {
                lo.lo=0;
                lo.hi=0;
                hi.lo=0;
                hi.hi=0;
            }
    };


protected :
    //Streams management
    void Streams_Accept();
    void Streams_Fill ();
    void Streams_Finish ();
    void Streams_Finish_Preface (const int128u PrefaceUID);
    void Streams_Finish_Preface_ForTimeCode (const int128u PrefaceUID);
    void Streams_Finish_ContentStorage (const int128u ContentStorageUID);
    void Streams_Finish_ContentStorage_ForTimeCode (const int128u ContentStorageUID);
    void Streams_Finish_ContentStorage_ForAS11 (const int128u ContentStorageUID);
    void Streams_Finish_Package (const int128u PackageUID);
    void Streams_Finish_Package_ForTimeCode (const int128u PackageUID);
    void Streams_Finish_Package_ForAS11 (const int128u PackageUID);
    void Streams_Finish_Track (const int128u TrackUID);
    void Streams_Finish_Track_ForTimeCode (const int128u TrackUID, bool IsSourcePackage);
    void Streams_Finish_Track_ForAS11 (const int128u TrackUID);
    void Streams_Finish_Essence (int32u EssenceUID, int128u TrackUID);
    void Streams_Finish_Essence_FillID (int32u EssenceUID, int128u TrackUID);
    void Streams_Finish_Descriptor (const int128u DescriptorUID, const int128u PackageUID);
    void Streams_Finish_Locator (const int128u DescriptorUID, const int128u LocatorUID);
    void Streams_Finish_Component (const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin);
    void Streams_Finish_Component_ForTimeCode (const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin, bool IsSourcePackage, const Ztring& TrackName);
    void Streams_Finish_Component_ForAS11 (const int128u ComponentUID, float64 EditRate, int32u TrackID, int64s Origin);
    void Streams_Finish_Identification (const int128u IdentificationUID);
    void Streams_Finish_CommercialNames ();

    //Buffer - Global
    void Read_Buffer_Init ();
    void Read_Buffer_Continue ();
    #if defined(MEDIAINFO_FILE_YES)
    void Read_Buffer_CheckFileModifications();
    #else //defined(MEDIAINFO_FILE_YES)
    void Read_Buffer_CheckFileModifications() {}
    #endif //defined(MEDIAINFO_FILE_YES)
    void Read_Buffer_AfterParsing ();
    void Read_Buffer_Unsynched();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();

    //Buffer - Per element
    bool Header_Begin();
    void Header_Parse();
    void Data_Parse();

    //Elements
    void MCAChannelID();
    void MCALabelDictionaryID();
    void MCATagSymbol();
    void MCATagName();
    void GroupOfSoundfieldGroupsLinkID();
    void MCALinkID();
    void SoundfieldGroupLinkID();
    void MCAPartitionKind();
    void MCAPartitionNumber();
    void MCATitle();
    void MCATitleVersion();
    void MCATitleSubVersion();
    void MCAEpisode();
    void MCAAudioContentKind();
    void MCAAudioElementKind();
    void ResourceID();
    void NamespaceURI();
    void UCSEncoding();
    void Filler();
    void Filler01() {Filler();}
    void Filler02() {Filler();}
    void TerminatingFiller();
    void XmlDocumentText();
    void SubDescriptors();
    void LensUnitMetadata();
    void CameraUnitMetadata();
    void UserDefinedAcquisitionMetadata();
    void DMFiller();
    void Sequence();
    void SourceClip();
    void TimecodeComponent();
    void ContentStorage();
    void EssenceContainerData();
    void CDCIEssenceDescriptor();
    void RGBAEssenceDescriptor();
    void Preface();
    void Identification();
    void NetworkLocator();
    void TextLocator();
    void StereoscopicPictureSubDescriptor();
    void MaterialPackage();
    void SourcePackage();
    void EventTrack();
    void StaticTrack();
    void TimelineTrack();
    void DMSegment();
    void GenericSoundEssenceDescriptor();
    void GenericDataEssenceDescriptor();
    void MultipleDescriptor();
    void DMSourceClip();
    void AES3PCMDescriptor();
    void WaveAudioDescriptor();
    void MPEG2VideoDescriptor();
    void JPEG2000PictureSubDescriptor();
    void VbiPacketsDescriptor();
    void AncPacketsDescriptor();
    void MpegAudioDescriptor();
    void PackageMarkerObject();
    void ApplicationPlugInObject();
    void ApplicationReferencedObject();
    void MCALabelSubDescriptor();
    void TimedTextDescriptor();
    void TimedTextResourceSubDescriptor();
    void ContainerConstraintsSubDescriptor();
    void Mpeg4VisualSubDescriptor();
    void AudioChannelLabelSubDescriptor();
    void SoundfieldGroupLabelSubDescriptor();
    void GroupOfSoundfieldGroupsLabelSubDescriptor();
    void AVCSubDescriptor();
    void IABEssenceDescriptor();
    void IABSoundfieldLabelSubDescriptor();
    void OpenIncompleteHeaderPartition();
    void ClosedIncompleteHeaderPartition();
    void OpenCompleteHeaderPartition();
    void ClosedCompleteHeaderPartition();
    void OpenIncompleteBodyPartition();
    void ClosedIncompleteBodyPartition();
    void OpenCompleteBodyPartition();
    void ClosedCompleteBodyPartition();
    void GenericStreamPartition();
    void OpenIncompleteFooterPartition();
    void ClosedIncompleteFooterPartition();
    void OpenCompleteFooterPartition();
    void ClosedCompleteFooterPartition();
    void Primer();
    void IndexTableSegment();
    void RandomIndexPack();
    void SDTI_SystemMetadataPack();
    void SDTI_PackageMetadataSet();
    void SDTI_PictureMetadataSet();
    void SDTI_SoundMetadataSet();
    void SDTI_DataMetadataSet();
    void SDTI_ControlMetadataSet();
    void SystemScheme1();
    void DMScheme1();
    void Application04_01_04_01_01();
    void Application04_01_04_02_01();
    void Application05_09_01();
    void Application_08_BodySID();
    void AS11_AAF_Core();
    void AS11_AAF_Segmentation();
    void AS11_AAF_UKDPP();
    void Dolby_PHDRImageMetadataItem();
    void Dolby_PHDRMetadataTrackSubDescriptor();
    void Omneon_010201010100();
    void Omneon_010201020100();
    void FFV1PictureSubDescriptor();

    //Common
    void GenerationInterchangeObject();
    void InterchangeObject();
    void GenericPictureEssenceDescriptor();
    void PartitionMetadata();
    void GenericTrack();
    void GenericPackage();
    void FileDescriptor();
    void StructuralComponent();
    void GenericDescriptor();

    //Complex types
    void AES3PCMDescriptor_AuxBitsMode();                       //3D08
    void AES3PCMDescriptor_Emphasis();                          //3D0D
    void AES3PCMDescriptor_BlockStartOffset();                  //3D0F
    void AES3PCMDescriptor_ChannelStatusMode();                 //3D10
    void AES3PCMDescriptor_FixedChannelStatusData();            //3D11
    void AES3PCMDescriptor_UserDataMode();                      //3D12
    void AES3PCMDescriptor_FixedUserData();                     //3D13
    void CDCIEssenceDescriptor_ComponentDepth();                //3301
    void CDCIEssenceDescriptor_HorizontalSubsampling();         //3302
    void CDCIEssenceDescriptor_ColorSiting();                   //3303
    void CDCIEssenceDescriptor_BlackRefLevel();                 //3304
    void CDCIEssenceDescriptor_WhiteReflevel();                 //3305
    void CDCIEssenceDescriptor_ColorRange();                    //3306
    void CDCIEssenceDescriptor_PaddingBits();                   //3307
    void CDCIEssenceDescriptor_VerticalSubsampling();           //3308
    void CDCIEssenceDescriptor_AlphaSampleDepth();              //3309
    void CDCIEssenceDescriptor_ReversedByteOrder();             //330B
    void ContentStorage_Packages();                             //1901
    void ContentStorage_EssenceContainerData();                 //1902
    void DMSegment_Duration();                                  //0202 (copy of StructuralComponent_Duration) //TODO: merge with StructuralComponent_Duration
    void DMSegment_DMFramework();                               //6101
    void DMSegment_TrackIDs();                                  //6102
    void EssenceContainerData_LinkedPackageUID();               //2701
    void EssenceContainerData_IndexSID();                       //3F06
    void EssenceContainerData_BodySID();                        //3F07
    void EventTrack_EventEditRate();                            //4901
    void EventTrack_EventOrigin();                              //4902
    void FileDescriptor_SampleRate();                           //3001
    void FileDescriptor_ContainerDuration();                    //3002
    void FileDescriptor_EssenceContainer();                     //3004
    void FileDescriptor_Codec();                                //3005
    void FileDescriptor_LinkedTrackID();                        //3006
    void InterchangeObject_InstanceUID();                       //3C0A
    void GenerationInterchangeObject_GenerationUID();           //0102
    void GenericDescriptor_Locators();                          //2F01
    void GenericPackage_PackageUID();                           //4401
    void GenericPackage_Name();                                 //4402
    void GenericPackage_Tracks();                               //4403
    void GenericPackage_PackageModifiedDate();                  //4404
    void GenericPackage_PackageCreationDate();                  //4405
    void GenericPictureEssenceDescriptor_PictureEssenceCoding();//3201
    void GenericPictureEssenceDescriptor_StoredHeight();        //3202
    void GenericPictureEssenceDescriptor_StoredWidth();         //3203
    void GenericPictureEssenceDescriptor_SampledHeight();       //3204
    void GenericPictureEssenceDescriptor_SampledWidth();        //3205
    void GenericPictureEssenceDescriptor_SampledXOffset();      //3206
    void GenericPictureEssenceDescriptor_SampledYOffset();      //3207
    void GenericPictureEssenceDescriptor_DisplayHeight();       //3208
    void GenericPictureEssenceDescriptor_DisplayWidth();        //3209
    void GenericPictureEssenceDescriptor_DisplayXOffset();      //320A
    void GenericPictureEssenceDescriptor_DisplayYOffset();      //320B
    void GenericPictureEssenceDescriptor_FrameLayout();         //320C
    void GenericPictureEssenceDescriptor_VideoLineMap();        //320D
    void GenericPictureEssenceDescriptor_AspectRatio();         //320E
    void GenericPictureEssenceDescriptor_AlphaTransparency();   //320F
    void GenericPictureEssenceDescriptor_TransferCharacteristic(); //3210
    void GenericPictureEssenceDescriptor_ImageAlignmentOffset();//3211
    void GenericPictureEssenceDescriptor_FieldDominance();      //3212
    void GenericPictureEssenceDescriptor_ImageStartOffset();    //3213
    void GenericPictureEssenceDescriptor_ImageEndOffset();      //3214
    void GenericPictureEssenceDescriptor_SignalStandard();      //3215
    void GenericPictureEssenceDescriptor_StoredF2Offset();      //3216
    void GenericPictureEssenceDescriptor_DisplayF2Offset();     //3217
    void GenericPictureEssenceDescriptor_ActiveFormatDescriptor();//3218
    void GenericPictureEssenceDescriptor_ColorPrimaries();      //3219
    void GenericPictureEssenceDescriptor_CodingEquations();     //321A
    void MasteringDisplayPrimaries();
    void MasteringDisplayWhitePointChromaticity();
    void MasteringDisplayMaximumLuminance();
    void MasteringDisplayMinimumLuminance();
    void GenericSoundEssenceDescriptor_QuantizationBits();      //3D01
    void GenericSoundEssenceDescriptor_Locked();                //3D02
    void GenericSoundEssenceDescriptor_AudioSamplingRate();     //3D03
    void GenericSoundEssenceDescriptor_AudioRefLevel();         //3D04
    void GenericSoundEssenceDescriptor_ElectroSpatialFormulation(); //3D05
    void GenericSoundEssenceDescriptor_SoundEssenceCompression(); //3D06
    void GenericSoundEssenceDescriptor_ChannelCount();          //3D07
    void GenericSoundEssenceDescriptor_DialNorm();              //3D0C
    void GenericDataEssenceDescriptor_DataEssenceCoding();      //3E01
    void GenericTrack_TrackID();                                //4801
    void GenericTrack_TrackName();                              //4802
    void GenericTrack_Sequence();                               //4803
    void GenericTrack_TrackNumber();                            //4804
    void Identification_CompanyName();                          //3C01
    void Identification_ProductName();                          //3C02
    void Identification_ProductVersion();                       //3C03
    void Identification_VersionString();                        //3C04
    void Identification_ProductUID();                           //3C05
    void Identification_ModificationDate();                     //3C06
    void Identification_ToolkitVersion();                       //3C07
    void Identification_Platform();                             //3C08
    void Identification_ThisGenerationUID();                    //3C09
    void IndexTableSegment_EditUnitByteCount();                 //3F05
    void IndexTableSegment_IndexSID();                          //3F06
    void IndexTableSegment_BodySID();                           //3F07
    void IndexTableSegment_SliceCount();                        //3F08
    void IndexTableSegment_DeltaEntryArray();                   //3F09
    void IndexTableSegment_IndexEntryArray();                   //3F0A
    void IndexTableSegment_IndexEditRate();                     //3F0B
    void IndexTableSegment_IndexStartPosition();                //3F0C
    void IndexTableSegment_IndexDuration();                     //3F0D
    void IndexTableSegment_PosTableCount();                     //3F0E
    void IndexTableSegment_8002();                              //8002
    void JPEG2000PictureSubDescriptor_Rsiz();                   //8001
    void JPEG2000PictureSubDescriptor_Xsiz();                   //8002
    void JPEG2000PictureSubDescriptor_Ysiz();                   //8003
    void JPEG2000PictureSubDescriptor_XOsiz();                  //8004
    void JPEG2000PictureSubDescriptor_YOsiz();                  //8005
    void JPEG2000PictureSubDescriptor_XTsiz();                  //8006
    void JPEG2000PictureSubDescriptor_YTsiz();                  //8007
    void JPEG2000PictureSubDescriptor_XTOsiz();                 //8008
    void JPEG2000PictureSubDescriptor_YTOsiz();                 //8009
    void JPEG2000PictureSubDescriptor_Csiz();                   //800A
    void JPEG2000PictureSubDescriptor_PictureComponentSizing(); //800B
    void JPEG2000PictureSubDescriptor_CodingStyleDefault();     //
    void JPEG2000PictureSubDescriptor_QuantizationDefault();    //
    void FFV1PictureSubDescriptor_InitializationMetadata();     //
    void FFV1PictureSubDescriptor_IdenticalGOP();               //
    void FFV1PictureSubDescriptor_MaxGOP();                     //
    void FFV1PictureSubDescriptor_MaximumBitRate();             //
    void FFV1PictureSubDescriptor_Version();                    //
    void FFV1PictureSubDescriptor_MicroVersion();               //
    void MpegAudioDescriptor_BitRate();                         //
    void MultipleDescriptor_FileDescriptors();                  //3F01
    void PrimaryExtendedSpokenLanguage();                       //
    void SecondaryExtendedSpokenLanguage();                     //
    void OriginalExtendedSpokenLanguage();                      //
    void SecondaryOriginalExtendedSpokenLanguage();             //
    void RFC5646AudioLanguageCode();                            //
    void MPEG2VideoDescriptor_SingleSequence();                 //
    void MPEG2VideoDescriptor_ConstantBFrames();                //
    void MPEG2VideoDescriptor_CodedContentType();               //
    void MPEG2VideoDescriptor_LowDelay();                       //
    void MPEG2VideoDescriptor_ClosedGOP();                      //
    void MPEG2VideoDescriptor_IdenticalGOP();                   //
    void MPEG2VideoDescriptor_MaxGOP();                         //
    void MPEG2VideoDescriptor_BPictureCount();                  //
    void MPEG2VideoDescriptor_ProfileAndLevel();                //
    void MPEG2VideoDescriptor_BitRate();                        //
    void Mpeg4VisualDescriptor_SingleSequence() {MPEG2VideoDescriptor_SingleSequence();}
    void Mpeg4VisualDescriptor_ConstantBFrames() {MPEG2VideoDescriptor_ConstantBFrames();}
    void Mpeg4VisualDescriptor_CodedContentType() {MPEG2VideoDescriptor_CodedContentType();}
    void Mpeg4VisualDescriptor_LowDelay() {MPEG2VideoDescriptor_LowDelay();}
    void Mpeg4VisualDescriptor_ClosedGOP() {MPEG2VideoDescriptor_ClosedGOP();}
    void Mpeg4VisualDescriptor_IdenticalGOP() {MPEG2VideoDescriptor_IdenticalGOP();}
    void Mpeg4VisualDescriptor_MaxGOP() {MPEG2VideoDescriptor_MaxGOP();}
    void Mpeg4VisualDescriptor_BPictureCount() {MPEG2VideoDescriptor_BPictureCount();}
    void Mpeg4VisualDescriptor_ProfileAndLevel();               //
    void Mpeg4VisualDescriptor_BitRate() {MPEG2VideoDescriptor_BitRate();}
    void AVCDescriptor_ConstantBFrames() {MPEG2VideoDescriptor_ConstantBFrames();}
    void AVCDescriptor_CodedContentType() {MPEG2VideoDescriptor_CodedContentType();}
    void AVCDescriptor_ClosedGOP() {MPEG2VideoDescriptor_ClosedGOP();}
    void AVCDescriptor_IdenticalGOP() {MPEG2VideoDescriptor_IdenticalGOP();}
    void AVCDescriptor_MaxGOP() {MPEG2VideoDescriptor_MaxGOP();}
    void AVCDescriptor_BPictureCount() {MPEG2VideoDescriptor_BPictureCount();}
    void AVCDescriptor_Profile();
    void AVCDescriptor_MaximumBitRate();
    void AVCDescriptor_ProfileConstraint();
    void AVCDescriptor_Level();
    void AVCDescriptor_DecodingDelay();
    void AVCDescriptor_MaximumRefFrames();
    void AVCDescriptor_SequenceParameterSetFlag();
    void AVCDescriptor_PictureParameterSetFlag();
    void AVCDescriptor_AverageBitRate();
    void NetworkLocator_URLString();                            //4001
    void Preface_LastModifiedDate();                            //3B02
    void Preface_ContentStorage();                              //3B03
    void Preface_Version();                                     //3B05
    void Preface_Identifications();                             //3B06
    void Preface_ObjectModelVersion();                          //3B07
    void Preface_PrimaryPackage();                              //3B08
    void Preface_OperationalPattern();                          //3B09
    void Preface_EssenceContainers();                           //3B0A
    void Preface_DMSchemes();                                   //3B0B
    void RGBAEssenceDescriptor_PixelLayout();                   //3401
    void RGBAEssenceDescriptor_Palette();                       //3403
    void RGBAEssenceDescriptor_PaletteLayout();                 //3404
    void RGBAEssenceDescriptor_ScanningDirection();             //3405
    void RGBAEssenceDescriptor_ComponentMaxRef();               //3406
    void RGBAEssenceDescriptor_ComponentMinRef();               //3407
    void RGBAEssenceDescriptor_AlphaMaxRef();                   //3408
    void RGBAEssenceDescriptor_AlphaMinRef();                   //3409
    void Sequence_StructuralComponents();                       //1001
    void SourceClip_SourcePackageID();                          //1101
    void SourceClip_SourceTrackID();                            //1102
    void SourceClip_StartPosition();                            //1201
    void SourcePackage_Descriptor();                            //4701
    void StructuralComponent_DataDefinition();                  //0201
    void StructuralComponent_Duration();                        //0202
    void SystemScheme1_FrameCount();                            //0101
    void SystemScheme1_TimeCodeArray();                         //0102
    void SystemScheme1_ClipIDArray();                           //0103
    void SystemScheme1_ExtendedClipIDArray();                   //0104
    void SystemScheme1_VideoIndexArray();                       //0105
    void SystemScheme1_KLVMetadataSequence();                   //0106
    void SystemScheme1_SampleRate();                            //3001
    void SystemScheme1_EssenceTrackNumber();                    //4804
    void SystemScheme1_EssenceTrackNumberBatch();               //6801
    void SystemScheme1_ContentPackageIndexArray();              //6803
    void TextLocator_LocatorName();                             //4101
    void TimecodeComponent_StartTimecode();                     //1501
    void TimecodeComponent_RoundedTimecodeBase();               //1502
    void TimecodeComponent_DropFrame();                         //1503
    void Track_EditRate();                                      //4B01
    void Track_Origin();                                        //4B02
    void WaveAudioDescriptor_AvgBps();                          //3D09
    void WaveAudioDescriptor_BlockAlign();                      //3D0A
    void WaveAudioDescriptor_SequenceOffset();                  //3D0B
    void WaveAudioDescriptor_PeakEnvelopeVersion();             //3D29
    void WaveAudioDescriptor_PeakEnvelopeFormat();              //3D2A
    void WaveAudioDescriptor_PointsPerPeakValue();              //3D2B
    void WaveAudioDescriptor_PeakEnvelopeBlockSize();           //3D2C
    void WaveAudioDescriptor_PeakChannels();                    //3D2D
    void WaveAudioDescriptor_PeakFrames();                      //3D2E
    void WaveAudioDescriptor_PeakOfPeaksPosition();             //3D2F
    void WaveAudioDescriptor_PeakEnvelopeTimestamp();           //3D30
    void WaveAudioDescriptor_PeakEnvelopeData();                //3D31
    void WaveAudioDescriptor_ChannelAssignment();               //3D31
    void LensUnitMetadata_IrisFNumber();                        //8000
    void LensUnitMetadata_FocusPositionFromImagePlane();        //8001
    void LensUnitMetadata_FocusPositionFromFrontLensVertex();   //8002
    void LensUnitMetadata_MacroSetting();                       //8003
    void LensUnitMetadata_LensZoom35mmStillCameraEquivalent();  //8004
    void LensUnitMetadata_LensZoomActualFocalLength();          //8005
    void LensUnitMetadata_OpticalExtenderMagnification();       //8006
    void LensUnitMetadata_LensAttributes();                     //8007
    void LensUnitMetadata_IrisTNumber();                        //8008
    void LensUnitMetadata_IrisRingPosition();                   //8009
    void LensUnitMetadata_FocusRingPosition();                  //800A
    void LensUnitMetadata_ZoomRingPosition();                   //800B
    void CameraUnitMetadata_CaptureGammaEquation();             //3210
    void CameraUnitMetadata_AutoExposureMode();                 //8100
    void CameraUnitMetadata_AutoFocusSensingAreaSetting();      //8101
    void CameraUnitMetadata_ColorCorrectionFilterWheelSetting();//8102
    void CameraUnitMetadata_NeutralDensityFilterWheelSetting(); //8103
    void CameraUnitMetadata_ImageSensorDimensionEffectiveWidth();//8104
    void CameraUnitMetadata_ImageSensorDimensionEffectiveHeight();//8105
    void CameraUnitMetadata_CaptureFrameRate();                 //8106
    void CameraUnitMetadata_ImageSensorReadoutMode();           //8107
    void CameraUnitMetadata_ShutterSpeed_Angle();               //8108
    void CameraUnitMetadata_ShutterSpeed_Time();                //8109
    void CameraUnitMetadata_CameraMasterGainAdjustment();       //810A
    void CameraUnitMetadata_ISOSensitivity();                   //810B
    void CameraUnitMetadata_ElectricalExtenderMagnification();  //810C
    void CameraUnitMetadata_AutoWhiteBalanceMode();             //810D
    void CameraUnitMetadata_WhiteBalance();                     //800E
    void CameraUnitMetadata_CameraMasterBlackLevel();           //810F
    void CameraUnitMetadata_CameraKneePoint();                  //8110
    void CameraUnitMetadata_CameraKneeSlope();                  //8111
    void CameraUnitMetadata_CameraLuminanceDynamicRange();      //8112
    void CameraUnitMetadata_CameraSettingFileURI();             //8113
    void CameraUnitMetadata_CameraAttributes();                 //8114
    void CameraUnitMetadata_ExposureIndexofPhotoMeter();        //8115
    void CameraUnitMetadata_GammaForCDL();                      //8116
    void CameraUnitMetadata_ASC_CDL_V12();                      //8117
    void CameraUnitMetadata_ColorMatrix();                      //8118
    void UserDefinedAcquisitionMetadata_UdamSetIdentifier();    //E000
    void UserDefinedAcquisitionMetadata_Sony_8007();
    void UserDefinedAcquisitionMetadata_Sony_E101();
    void UserDefinedAcquisitionMetadata_Sony_E102();
    void UserDefinedAcquisitionMetadata_Sony_E103();
    void UserDefinedAcquisitionMetadata_Sony_E104();
    void UserDefinedAcquisitionMetadata_Sony_E105();
    void UserDefinedAcquisitionMetadata_Sony_E106();
    void UserDefinedAcquisitionMetadata_Sony_E107();
    void UserDefinedAcquisitionMetadata_Sony_E109();
    void UserDefinedAcquisitionMetadata_Sony_E10B();
    void UserDefinedAcquisitionMetadata_Sony_E201();
    void UserDefinedAcquisitionMetadata_Sony_E202();
    void UserDefinedAcquisitionMetadata_Sony_E203();
    void AS11_Core_SeriesTitle();
    void AS11_Core_ProgrammeTitle();
    void AS11_Core_EpisodeTitleNumber();
    void AS11_Core_ShimName();
    void AS11_Core_AudioTrackLayout();
    void AS11_Core_PrimaryAudioLanguage();
    void AS11_Core_ClosedCaptionsPresent();
    void AS11_Core_ClosedCaptionsType();
    void AS11_Core_ClosedCaptionsLanguage();
    void AS11_Core_ShimVersion();
    void AS11_Segment_PartNumber();
    void AS11_Segment_PartTotal();
    void AS11_UKDPP_ProductionNumber();
    void AS11_UKDPP_Synopsis();
    void AS11_UKDPP_Originator();
    void AS11_UKDPP_CopyrightYear();
    void AS11_UKDPP_OtherIdentifier();
    void AS11_UKDPP_OtherIdentifierType();
    void AS11_UKDPP_Genre();
    void AS11_UKDPP_Distributor();
    void AS11_UKDPP_PictureRatio();
    void AS11_UKDPP_3D();
    void AS11_UKDPP_3DType();
    void AS11_UKDPP_ProductPlacement();
    void AS11_UKDPP_FpaPass();
    void AS11_UKDPP_FpaManufacturer();
    void AS11_UKDPP_FpaVersion();
    void AS11_UKDPP_VideoComments();
    void AS11_UKDPP_SecondaryAudioLanguage();
    void AS11_UKDPP_TertiaryAudioLanguage();
    void AS11_UKDPP_AudioLoudnessStandard();
    void AS11_UKDPP_AudioComments();
    void AS11_UKDPP_LineUpStart();
    void AS11_UKDPP_IdentClockStart();
    void AS11_UKDPP_TotalNumberOfParts();
    void AS11_UKDPP_TotalProgrammeDuration();
    void AS11_UKDPP_AudioDescriptionPresent();
    void AS11_UKDPP_AudioDescriptionType();
    void AS11_UKDPP_OpenCaptionsPresent();
    void AS11_UKDPP_OpenCaptionsType();
    void AS11_UKDPP_OpenCaptionsLanguage();
    void AS11_UKDPP_SigningPresent();
    void AS11_UKDPP_SignLanguage();
    void AS11_UKDPP_CompletionDate();
    void AS11_UKDPP_TextlessElementsExist();
    void AS11_UKDPP_ProgrammeHasText();
    void AS11_UKDPP_ProgrammeTextLanguage();
    void AS11_UKDPP_ContactEmail();
    void AS11_UKDPP_ContactTelephoneNumber();
    void Dolby_DataDefinition();
    void Dolby_SourceTrackID();
    void Dolby_SimplePayloadSID();
    void Omneon_010201010100_8001();                            //8001
    void Omneon_010201010100_8003();                            //8003
    void Omneon_010201020100_8002();                            //8002
    void Omneon_010201020100_8003();                            //8003
    void Omneon_010201020100_8004();                            //8004
    void Omneon_010201020100_8005();                            //8005
    void Omneon_010201020100_8006();                            //8006

    //Basic types
    void Get_Rational(float64 &Value);
    void Skip_Rational();
    void Info_Rational();
    void Get_Timestamp (Ztring &Value);
    void Skip_Timestamp();
    void Info_Timestamp();
    void Get_UMID       (int256u &Value, const char* Name);
    void Skip_UMID      ();

    void Get_UL (int128u &Value, const char* Name, const char* (*Param) (int128u));
    void Skip_UL(const char* Name);
    void Get_BER(int64u &Value, const char* Name);
    #if MEDIAINFO_TRACE
    void Info_UL_01xx01_Items ();
    void Info_UL_02xx01_Groups ();
    void Info_UL_040101_Values ();
    #define Info_UL(_INFO, _NAME, _PARAM) int128u _INFO; Get_UL(_INFO, _NAME, _PARAM)
    #else //MEDIAINFO_TRACE
    void Info_UL_01xx01_Items () {Element_Offset+=8;};
    void Info_UL_02xx01_Groups () {Element_Offset+=8;};
    void Info_UL_040101_Values () {Element_Offset+=8;};
    #define Info_UL(_INFO, _NAME, _PARAM) Skip_UL(_NAME)
    #endif //MEDIAINFO_TRACE

    //TimeCode
    struct mxftimecode
    {
        int64u  StartTimecode;
        int16u  RoundedTimecodeBase;
        bool    DropFrame;

        mxftimecode(int16u RoundedTimecodeBase_ = 0, int64u StartTimecode_ = (int64u)-1, bool DropFrame_ = false)
            : RoundedTimecodeBase(RoundedTimecodeBase_)
            , StartTimecode(StartTimecode_)
            , DropFrame(DropFrame_)
        {
        }
        bool IsInit() const
        {
            return RoundedTimecodeBase && StartTimecode != (int64u)-1;
        }
        float64 Get_TimeCode_StartTimecode_Temp(const int64u& File_IgnoreEditsBefore) const
        {
            if (RoundedTimecodeBase)
            {
                float64 TimeCode_StartTimecode_Temp = ((float64)(StartTimecode + File_IgnoreEditsBefore)) / RoundedTimecodeBase;
                if (DropFrame)
                {
                    TimeCode_StartTimecode_Temp *= 1001;
                    TimeCode_StartTimecode_Temp /= 1000;
                }
                return TimeCode_StartTimecode_Temp;
            }
            else
                return 0.0;
        }
    };

    // Temp
    struct randomindexpack
    {
        int64u ByteOffset;
        int32u BodySID;
    };
    std::vector<randomindexpack>     RandomIndexPacks;
    bool                             RandomIndexPacks_AlreadyParsed;
    std::set<int64u>                 PartitionPack_AlreadyParsed;
    size_t Streams_Count;
    int128u Code;
    int128u OperationalPattern;
    int128u InstanceUID;
    int64u Buffer_Begin;
    int64u Buffer_End;
    bool   Buffer_End_Unlimited;
    int64u Buffer_Header_Size;
    int16u Code2;
    int16u Length2;
    int64u File_Size_Total; //Used only in Finish()
    int64u IsParsingMiddle_MaxOffset;
    bool   Track_Number_IsAvailable;
    bool   IsParsingEnd;
    bool   IsCheckingRandomAccessTable;
    bool   IsCheckingFooterPartitionAddress;
    bool   IsSearchingFooterPartitionAddress;
    bool   FooterPartitionAddress_Jumped;
    bool   PartitionPack_Parsed;
    bool   HeaderPartition_IsOpen;
    size_t IdIsAlwaysSame_Offset;

    //Primer
    std::map<int16u, int128u> Primer_Values;

    //Preface
    struct preface
    {
        int128u PrimaryPackage;
        std::vector<int128u> Identifications;
        int128u ContentStorage;

        preface()
        {
            PrimaryPackage.hi=(int64u)-1;
            PrimaryPackage.lo=(int64u)-1;
            ContentStorage.hi=(int64u)-1;
            ContentStorage.lo=(int64u)-1;
        }
    };
    typedef std::map<int128u, preface> prefaces; //Key is InstanceUID of preface
    prefaces Prefaces;
    int128u  Preface_Current;

    //Identification
    struct identification
    {
        Ztring CompanyName;
        Ztring ProductName;
        Ztring ProductVersion;
        Ztring VersionString;
        Ztring ToolkitVersion;
        Ztring Platform;
        std::map<std::string, Ztring> Infos;
    };
    typedef std::map<int128u, identification> identifications; //Key is InstanceUID of identification
    identifications Identifications;

    //ContentStorage
    struct contentstorage
    {
        std::vector<int128u> Packages;
    };
    typedef std::map<int128u, contentstorage> contentstorages; //Key is InstanceUID of ContentStorage
    contentstorages ContentStorages;

    //Package
    struct package
    {
        int256u PackageUID;
        int128u Descriptor;
        std::vector<int128u> Tracks;
        bool IsSourcePackage;

        package()
        {
            Descriptor=0;
            IsSourcePackage=false;
        }
    };
    typedef std::map<int128u, package> packages; //Key is InstanceUID of package
    packages Packages;

    //Track
    struct track
    {
        int128u Sequence;
        int32u TrackID;
        Ztring TrackName;
        int32u TrackNumber;
        float64 EditRate_Real; //Before demux adaptation
        float64 EditRate;
        int64s  Origin;
        bool   Stream_Finish_Done;

        track()
        {
            Sequence=0;
            TrackID=(int32u)-1;
            TrackNumber=(int32u)-1;
            EditRate_Real=(float64)0;
            EditRate=(float64)0;
            Origin=0;
            Stream_Finish_Done=false;
        }
    };
    typedef std::map<int128u, track> tracks; //Key is InstanceUID of the track
    tracks Tracks;

    //Essence
    typedef std::vector<File__Analyze*> parsers;
    struct essence
    {
        stream_t StreamKind;
        size_t   StreamPos;
        size_t   StreamPos_Initial;
        parsers Parsers;
        std::map<std::string, Ztring> Infos;
        int64u Stream_Size;
        int32u TrackID;
        bool   TrackID_WasLookedFor;
        bool   Stream_Finish_Done;
        bool   Track_Number_IsMappedToTrack; //if !Track_Number_IsAvailable, is true when it was euristicly mapped
        bool   IsFilled;
        bool   IsChannelGrouping;
        int64u      Field_Count_InThisBlock_1;
        int64u      Field_Count_InThisBlock_2;
        int64u      Frame_Count_NotParsedIncluded;
        #if MEDIAINFO_TRACE
            int64u  Trace_Count;
        #endif // MEDIAINFO_TRACE
        int32u  ShouldCheckAvcHeaders;
        frame_info  FrameInfo;

        essence()
        {
            StreamKind=Stream_Max;
            StreamPos=(size_t)-1;
            StreamPos_Initial=(size_t)-1;
            Stream_Size=(int64u)-1;
            TrackID=(int32u)-1;
            TrackID_WasLookedFor=false;
            Stream_Finish_Done=false;
            Track_Number_IsMappedToTrack=false;
            IsFilled=false;
            IsChannelGrouping=false;
            Field_Count_InThisBlock_1=0;
            Field_Count_InThisBlock_2=0;
            Frame_Count_NotParsedIncluded=(int64u)-1;
            #if MEDIAINFO_TRACE
                Trace_Count=0;
            #endif // MEDIAINFO_TRACE
            ShouldCheckAvcHeaders=0;
            FrameInfo.DTS=(int64u)-1;
        }
        essence(const essence&) = delete;
        essence(essence&&) = delete;

        ~essence()
        {
            for (size_t Pos=0; Pos<Parsers.size(); Pos++)
                delete Parsers[Pos];
        }
    };
    typedef std::map<int32u, essence> essences; //Key is TrackNumber
    essences Essences;
    std::bitset<Stream_Max+1> StreamPos_StartAtZero; //information about the base of StreamPos (0 or 1, 1 is found in 1 file) TODO: per Essence code (last 4 bytes of the Essence header 0xTTXXTTXX)

    //Descriptor
    struct descriptor
    {
        std::vector<int128u> SubDescriptors;
        std::vector<int128u> Locators;

        Ztring  ScanType;
        stream_t StreamKind;
        size_t   StreamPos;
        File__Analyze* Parser;
        float64 SampleRate;
        float64 DisplayAspectRatio;
        int128u InstanceUID;
        int128u EssenceContainer;
        int128u EssenceCompression;
        int32u LinkedTrackID;
        int32u Width;
        int32u Width_Display;
        int32u Width_Display_Offset;
        int32u Height;
        int32u Height_Display;
        int32u Height_Display_Offset;
        int32u SubSampling_Horizontal;
        int32u SubSampling_Vertical;
        int32u ChannelCount;
        int32u MinRefLevel;
        int32u MaxRefLevel;
        int32u ColorRange;
        int128u ChannelAssignment;
        std::map<std::string, Ztring> Infos;
        int16u BlockAlign;
        int32u QuantizationBits;
        int64u Duration;
        int8u  ActiveFormat;
        int8u  FieldTopness;
        int8u  FieldDominance;
        enum type
        {
            Type_Unknown,
            type_Mutiple,
            Type_CDCI,
            Type_RGBA,
            Type_MPEG2Video,
            Type_WaveAudio,
            Type_AES3PCM,
            Type_JPEG2000Picture,
            Type_AncPackets,
            Type_MCALabelSubDescriptor,
            Type_AudioChannelLabelSubDescriptor,
            Type_SoundfieldGroupLabelSubDescriptor,
            Type_GroupOfSoundfieldGroupsLabelSubDescriptor,
        };
        type Type;
        bool HasBFrames;
        bool HasMPEG2VideoDescriptor;
        bool IsAes3Descriptor;
        int32u ByteRate;
        #if MEDIAINFO_ADVANCED
            int16u Jpeg2000_Rsiz;
        #endif //MEDIAINFO_ADVANCED

        //MCALabelSubDescriptor specific (including SoundfieldGroupLabelSubDescriptor...)
        int128u     MCALabelDictionaryID;
        int128u     MCALinkID;
        Ztring      MCATagSymbol;
        Ztring      MCATagName;
        Ztring      MCAPartitionKind;
        Ztring      MCAPartitionNumber;
        Ztring      MCATitle;
        Ztring      MCATitleVersion;
        Ztring      MCATitleSubVersion;
        Ztring      MCAEpisode;
        Ztring      MCAAudioContentKind;
        Ztring      MCAAudioElementKind;

        //AudioChannelLabelSubDescriptor specific
        int128u     SoundfieldGroupLinkID;

        descriptor()
        {
            StreamKind=Stream_Max;
            StreamPos=(size_t)-1;
            Parser=NULL;
            SampleRate=0;
            DisplayAspectRatio=0;
            InstanceUID.hi=(int64u)-1;
            InstanceUID.lo=(int64u)-1;
            EssenceContainer.hi=(int64u)-1;
            EssenceContainer.lo=(int64u)-1;
            EssenceCompression.hi=(int64u)-1;
            EssenceCompression.lo=(int64u)-1;
            LinkedTrackID=(int32u)-1;
            Width=(int32u)-1;
            Width_Display=(int32u)-1;
            Width_Display_Offset=(int32u)-1;
            Height=(int32u)-1;
            Height_Display=(int32u)-1;
            Height_Display_Offset=(int32u)-1;
            SubSampling_Horizontal=(int32u)-1;
            SubSampling_Vertical=(int32u)-1;
            MinRefLevel=(int32u)-1;
            MaxRefLevel=(int32u)-1;
            ColorRange =(int32u)-1;
            ChannelCount=(int32u)-1;
            ChannelAssignment.hi=(int64u)-1;
            ChannelAssignment.lo=(int64u)-1;
            BlockAlign=(int16u)-1;
            QuantizationBits=(int32u)-1;
            Duration=(int64u)-1;
            ActiveFormat=(int8u)-1;
            FieldTopness=(int8u)-1; //Field x is upper field
            FieldDominance=1; //Default is field 1 temporaly first
            Type=Type_Unknown;
            HasBFrames=false;
            HasMPEG2VideoDescriptor=false;
            IsAes3Descriptor=false;
            ByteRate=(int32u)-1;
            #if MEDIAINFO_ADVANCED
                Jpeg2000_Rsiz=(int16u)-1;
            #endif //MEDIAINFO_ADVANCED

            //MCALabelSubDescriptor specific (including SoundfieldGroupLabelSubDescriptor...)
            MCALabelDictionaryID.hi=(int64u)-1;
            MCALabelDictionaryID.lo=(int64u)-1;
            MCALinkID.hi=(int64u)-1;
            MCALinkID.lo=(int64u)-1;

            //AudioChannelLabelSubDescriptor specific
            SoundfieldGroupLinkID.hi=(int64u)-1;
            SoundfieldGroupLinkID.lo=(int64u)-1;
        }
        descriptor(const descriptor&) = delete;
        descriptor(descriptor& b)
        {
            *this = b;
            b.Parser = nullptr;
        }

        ~descriptor()
        {
            delete Parser;
        }
		bool Is_Interlaced() const
        {
            return ScanType==__T("Interlaced");
        }
    };
    typedef std::map<int128u, descriptor> descriptors; //Key is InstanceUID of Descriptor
    descriptors Descriptors;
    void Descriptor_Fill(const char* Name, const Ztring& Value);

    //Locator
    struct locator
    {
        Ztring      EssenceLocator;
        stream_t    StreamKind;
        size_t      StreamPos;
        int32u      LinkedTrackID;
        bool        IsTextLocator;

        locator()
        {
            StreamKind=Stream_Max;
            StreamPos=(size_t)-1;
            LinkedTrackID=(int32u)-1;
            IsTextLocator=false;
        }

        ~locator()
        {
        }
    };
    typedef std::map<int128u, locator> locators; //Key is InstanceUID of the locator
    locators Locators;
    #if MEDIAINFO_NEXTPACKET
        bool                    ReferenceFiles_IsParsing;
    #endif //MEDIAINFO_NEXTPACKET

    //Component (Sequence, TimeCode, Source Clip)
    struct component
    {
        int64u Duration;
        int256u SourcePackageID; //Sequence from SourcePackage only
        int32u  SourceTrackID;
        std::vector<int128u> StructuralComponents; //Sequence from MaterialPackage only
        mxftimecode MxfTimeCode;

        component()
        {
            Duration=(int64u)-1;
            SourceTrackID=(int32u)-1;
        }

        void Update (struct component &New)
        {
            if (New.Duration!=(int64u)-1)
                Duration=New.Duration;
            if (New.SourcePackageID.hi || New.SourcePackageID.lo)
                SourcePackageID=New.SourcePackageID;
            if (New.SourceTrackID!=(int32u)-1)
                SourceTrackID=New.SourceTrackID;
            if (!New.StructuralComponents.empty())
                StructuralComponents=New.StructuralComponents;
            if (New.MxfTimeCode.StartTimecode!=(int64u)-1)
                MxfTimeCode.StartTimecode=New.MxfTimeCode.StartTimecode;
            if (New.MxfTimeCode.RoundedTimecodeBase)
            {
                MxfTimeCode.RoundedTimecodeBase=New.MxfTimeCode.RoundedTimecodeBase;
                MxfTimeCode.DropFrame=New.MxfTimeCode.DropFrame;
            }
        }
    };
    typedef std::map<int128u, component> components; //Key is InstanceUID of the component
    components Components;

    //Descriptive Metadata - DMSegments
    struct dmsegment
    {
        int128u     Framework;
        std::vector<int32u> TrackIDs;
        int64u Duration;
        bool    IsAs11SegmentFiller;

        dmsegment()
        {
            Framework.lo=(int64u)-1;
            Framework.hi=(int64u)-1;
            Duration=(int64u)-1;
            IsAs11SegmentFiller=false;
        }

        ~dmsegment()
        {
        }
    };
    typedef std::map<int128u, dmsegment> dmsegments; //Key is InstanceUID of the DMSegment
    dmsegments DMSegments;

    //Descriptive Metadata - DMScheme1
    struct dmscheme1
    {
        Ztring      PrimaryExtendedSpokenLanguage;

        dmscheme1()
        {
        }

        ~dmscheme1()
        {
        }
    };
    typedef std::map<int128u, dmscheme1> dmscheme1s; //Key is InstanceUID of the DMScheme1
    dmscheme1s DMScheme1s;

    //Descriptive Metadata - AS11
    struct as11
    {
        enum as11_type
        {
            Type_Unknown,
            Type_Core,
            Type_Segmentation,
            Type_UKDPP,
        };
        as11_type Type;
        Ztring SeriesTitle;
        Ztring ProgrammeTitle;
        Ztring EpisodeTitleNumber;
        Ztring ShimName;
        int8u  AudioTrackLayout;
        Ztring PrimaryAudioLanguage;
        int8u  ClosedCaptionsPresent;
        int8u  ClosedCaptionsType;
        Ztring ClosedCaptionsLanguage;
        int8u  ShimVersion_Major;
        int8u  ShimVersion_Minor;
        int16u PartNumber;
        int16u PartTotal;
        Ztring ProductionNumber;
        Ztring Synopsis;
        Ztring Originator;
        int16u CopyrightYear;
        Ztring OtherIdentifier;
        Ztring OtherIdentifierType;
        Ztring Genre;
        Ztring Distributor;
        int32u PictureRatio_N;
        int32u PictureRatio_D;
        int8u  ThreeD;
        int8u  ThreeDType;
        int8u  ProductPlacement;
        int8u  FpaPass;
        Ztring FpaManufacturer;
        Ztring FpaVersion;
        Ztring VideoComments;
        Ztring SecondaryAudioLanguage;
        Ztring TertiaryAudioLanguage;
        int8u  AudioLoudnessStandard;
        Ztring AudioComments;
        int64u LineUpStart;
        int64u IdentClockStart;
        int16u TotalNumberOfParts;
        int64u TotalProgrammeDuration;
        int8u  AudioDescriptionPresent;
        int8u  AudioDescriptionType;
        int8u  OpenCaptionsPresent;
        int8u  OpenCaptionsType;
        Ztring OpenCaptionsLanguage;
        int8u  SigningPresent;
        int8u  SignLanguage;
        int64u CompletionDate;
        int8u  TextlessElementsExist;
        int8u  ProgrammeHasText;
        Ztring ProgrammeTextLanguage;
        Ztring ContactEmail;
        Ztring ContactTelephoneNumber;

        as11()
        {
            Type=Type_Unknown;
            AudioTrackLayout=(int8u)-1;
            ClosedCaptionsPresent=(int8u)-1;
            ClosedCaptionsType=(int8u)-1;
            ShimVersion_Major=(int8u)-1;
            ShimVersion_Minor=(int8u)-1;
            PartNumber=(int16u)-1;
            PartTotal=(int16u)-1;
            CopyrightYear=(int16u)-1;
            PictureRatio_N=(int32u)-1;
            PictureRatio_D=(int32u)-1;
            ThreeD=(int8u)-1;
            ThreeDType=(int8u)-1;
            ProductPlacement=(int8u)-1;
            AudioLoudnessStandard=(int8u)-1;
            LineUpStart=(int64u)-1;
            IdentClockStart=(int64u)-1;
            TotalNumberOfParts=(int16u)-1;
            TotalProgrammeDuration=(int64u)-1;
            AudioDescriptionPresent=(int8u)-1;
            AudioDescriptionType=(int8u)-1;
            OpenCaptionsPresent=(int8u)-1;
            OpenCaptionsType=(int8u)-1;
            SigningPresent=(int8u)-1;
            SignLanguage=(int8u)-1;
            CompletionDate=(int64u)-1;
            TextlessElementsExist=(int8u)-1;
            ProgrammeHasText=(int8u)-1;
            FpaPass=(int8u)-1;
        }

        ~as11()
        {
        }
    };
    typedef std::map<int128u, as11> as11s; //Key is InstanceUID of the DMScheme1
    as11s AS11s;

    //Parsers
    void           ChooseParser__FromEssence(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_CP_Picture(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_CP_Sound(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_CP_Data(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_14(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_GC_Picture(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_GC_Sound(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_GC_Data(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Aaf_GC_Compound(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Avid(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Avid_Picture(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Dolby(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser__Sony(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__Sony_Picture(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser__FromEssenceContainer(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Avc(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_DV(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Mpeg4v(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Mpegv(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Raw(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_RV24(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Vc3(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_TimedText(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Aac(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Adif(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Adts(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Latm(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Ac3(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Alaw(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_ChannelGrouping(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_ChannelSplitting(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Mpega(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Pcm(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_SmpteSt0331(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_SmpteSt0337(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Jpeg2000(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_ProRes(const essences::iterator &Essence, const descriptors::iterator &Descriptor);
    void           ChooseParser_Ffv1(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_DolbyVisionFrameData(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Iab(const essences::iterator& Essence, const descriptors::iterator& Descriptor);

    //Helpers
    int32u Vector(int32u ExpectedLength=(int32u)-1);
    void Subsampling_Compute(descriptors::iterator Descriptor);
    void ColorLevels_Compute(descriptors::iterator Descriptor, bool Force=false, int32u BitDepth=(int32u)-1);
    #if defined(MEDIAINFO_REFERENCES_YES)
        void Locators_CleanUp();
        void Locators_Test();
    #else //defined(MEDIAINFO_REFERENCES_YES)
        inline void Locators_CleanUp() {}
        inline void Locators_Test() {}
    #endif //defined(MEDIAINFO_REFERENCES_YES)
    void NextRandomIndexPack();
    bool BookMark_Needed();

    //Temp
    int128u EssenceContainer_FromPartitionMetadata;
    int64u PartitionMetadata_PreviousPartition;
    int64u PartitionMetadata_FooterPartition;
    int64u RandomIndexPacks_MaxOffset;
    mxftimecode MxfTimeCodeForDelay;
    mxftimecode MxfTimeCodeMaterial;
    float64 DTS_Delay; //In seconds
    TimeCode            SDTI_TimeCode_StartTimecode;
    size_t              SDTI_TimeCode_RepetitionCount;
    TimeCode            SDTI_TimeCode_Previous;
    int64u SDTI_SizePerFrame;
    bool   SDTI_IsPresent; //Used to test if SDTI packet is used for Index StreamOffset calculation
    bool   SDTI_IsInIndexStreamOffset; //Used to test if SDTI packet is used for Index StreamOffset calculation
    #if MEDIAINFO_TRACE
        int64u SDTI_SystemMetadataPack_Trace_Count;
        int64u SDTI_PackageMetadataSet_Trace_Count;
        int64u Padding_Trace_Count;
    #endif // MEDIAINFO_TRACE
    TimeCode SystemScheme1_TimeCodeArray_StartTimecode;
    int64u SystemScheme1_FrameRateFromDescriptor;
    bool   Essences_FirstEssence_Parsed;
    bool   MayHaveCaptionsInStream;
    bool   StereoscopicPictureSubDescriptor_IsPresent;
    bool   UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony;
    int32u Essences_UsedForFrameCount;
    int32u IndexTable_NSL;
    int32u IndexTable_NPE;
    struct systemscheme
    {
        bool IsTimeCode;

        systemscheme()
        {
            IsTimeCode=false;
        }
    };
    typedef std::map<int16u, systemscheme> systemschemes;
    systemschemes SystemSchemes;
    #if MEDIAINFO_ADVANCED
        int64u Footer_Position;
    #endif //MEDIAINFO_ADVANCED
    #if defined(MEDIAINFO_ANCILLARY_YES)
        File_Ancillary* Ancillary;
    #endif //defined(MEDIAINFO_ANCILLARY_YES)

    //Hints
    size_t* File_Buffer_Size_Hint_Pointer;
    size_t  Synched_Count;

    //Partitions
    struct partition
    {
        int64u StreamOffset; //From file, not MXF one
        int64u PartitionPackByteCount; //Fill included
        int64u FooterPartition;
        int64u HeaderByteCount;
        int64u IndexByteCount;
        int64u BodyOffset;

        partition()
        {
            StreamOffset=0;
            PartitionPackByteCount=(int64u)-1;
            FooterPartition=0;
            HeaderByteCount=0;
            IndexByteCount=0;
            BodyOffset=0;
        }

        bool operator < (const partition& lhs) const
        {
            return StreamOffset<lhs.StreamOffset;
        }
    };
    typedef std::vector<partition>  partitions;
    partitions                      Partitions;
    size_t                          Partitions_Pos;
    bool                            Partitions_IsCalculatingHeaderByteCount;
    bool                            Partitions_IsCalculatingSdtiByteCount;
    bool                            Partitions_IsFooter;

    //Config
    bool                            TimeCodeFromMaterialPackage;

    //CameraUnitMetadata
    struct acquisitionmetadata
    {
        string Value;
        size_t FrameCount;

        acquisitionmetadata(const string &Value_)
            : Value(Value_)
            , FrameCount(1)
        {}
        bool Add(const string& Value_)
        {
            if (Value == Value_)
            {
                FrameCount++;
                return true;
            }
            return false;
        }
    };
    typedef std::vector<acquisitionmetadata> acquisitionmetadatalist;
    vector<acquisitionmetadatalist*> AcquisitionMetadataLists;
    void AcquisitionMetadata_Add(size_t Id, const string& Value)
    {
        if (!AcquisitionMetadataLists[Id])
        {
            AcquisitionMetadataLists[Id]=new acquisitionmetadatalist;
            AcquisitionMetadataLists[Id]->push_back(acquisitionmetadata(Value));
            return;
        }
        if ((*AcquisitionMetadataLists[Id])[AcquisitionMetadataLists[Id]->size()-1].Add(Value))
        {
            return;
        }
        AcquisitionMetadataLists[Id]->push_back(acquisitionmetadata(Value));
    }
    vector<acquisitionmetadatalist*> AcquisitionMetadata_Sony_E201_Lists;
    void AcquisitionMetadata_Sony_E201_Add(size_t Id, const string& Value)
    {
        if (!AcquisitionMetadata_Sony_E201_Lists[Id])
        {
            AcquisitionMetadata_Sony_E201_Lists[Id]=new acquisitionmetadatalist;
            AcquisitionMetadata_Sony_E201_Lists[Id]->push_back(acquisitionmetadata(Value));
            return;
        }
        if ((*AcquisitionMetadata_Sony_E201_Lists[Id])[AcquisitionMetadata_Sony_E201_Lists[Id]->size()-1].Add(Value))
        {
            return;
        }
        AcquisitionMetadata_Sony_E201_Lists[Id]->push_back(acquisitionmetadata(Value));
    }
    int8u AcquisitionMetadata_Sony_CalibrationType;

    // Extra metadata
    int64u ExtraMetadata_Offset;
    std::set<int32u> ExtraMetadata_SID;
    File_DolbyVisionMetadata* DolbyVisionMetadata;
    File_DolbyAudioMetadata* DolbyAudioMetadata;
    #if defined(MEDIAINFO_ADM_YES)
    File_Adm* Adm;
    #endif
    #if defined(MEDIAINFO_IAB_YES)
    File_Iab* Adm_ForLaterMerge;
    #endif
        
    //Demux
    #if MEDIAINFO_DEMUX
        bool Demux_HeaderParsed;
        essences::iterator Demux_CurrentEssence;
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
        size_t CountOfLocatorsToParse;
        float64 Demux_Rate;

        //IndexTable
        struct indextable
        {
            int64u  StreamOffset; //From file, not MXF one
            int64u  IndexStartPosition;
            int64u  IndexDuration;
            int32u  EditUnitByteCount;
            float64 IndexEditRate;
            struct entry
            {
                int64u  StreamOffset;
                int8u   Type;
            };
            std::vector<entry> Entries;

            indextable()
            {
                StreamOffset=(int64u)-1;
                IndexStartPosition=0;
                IndexDuration=0;
                EditUnitByteCount=0;
                IndexEditRate=0;
            }

            bool operator < (const indextable& lhs) const
            {
                return IndexStartPosition<lhs.IndexStartPosition;
            }
        };
        typedef std::vector<indextable> indextables;
        indextables                     IndexTables;
        size_t                          IndexTables_Pos;

        //Other
        int64u  Clip_Header_Size;
        int64u  Clip_Begin;
        int64u  Clip_End;
        int128u Clip_Code;
        int64u  OverallBitrate_IsCbrForSure;
        bool    Duration_Detected;
        #if defined(MEDIAINFO_FILE_YES)
        bool    DetectDuration();
        #else //defined(MEDIAINFO_FILE_YES)
        bool    DetectDuration() { return false; }
        #endif //defined(MEDIAINFO_FILE_YES)
        int64u  DemuxedSampleCount_Total;
        int64u  DemuxedSampleCount_Current;
        int64u  DemuxedSampleCount_AddedToFirstFrame;
        int64u  DemuxedElementSize_AddedToFirstFrame;
    #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK
};

} //NameSpace

#endif
