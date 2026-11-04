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
#include <deque>
#include <vector>
#include <set>
#include <bitset>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mxf
//***************************************************************************

class File_Mxf : public File__Analyze, File__HasReferences
{
public :
    //In
    bool IsRtmd;

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

    enum type {
        Type_Unknown,
        Type_UInt,
        Type_UI16 = Type_UInt,
        Type_Bool = Type_UInt,
        Type_AUID,
        Type_UUID,
        Type_ISO7,
        Type_UTF16,
        Type_Ref,
        Type_Refs,
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
    void Streams_Finish_Arri ();
    void Streams_Finish_Package (const int128u PackageUID);
    void Streams_Finish_Package_ForTimeCode (const int128u PackageUID);
    void Streams_Finish_Package_ForAS11 (const int128u PackageUID);
    #if MEDIAINFO_CONFORMANCE
    void Streams_Finish_ContentStorage_Conformance (const int128u ContentStorageUID);
    void Streams_Finish_Conformance_FileFormat (); //SMPTE ST 377-1, edition depending on the version in the file
    void Streams_Finish_Conformance_HeaderMetadata (); //SMPTE ST 377-1, KLV coding, Primer Pack and Header Metadata, version 1.3 only
    void Streams_Finish_Conformance_IndexTables (); //SMPTE ST 377-1, Index Table Segments of an Index Stream, version 1.3 only
    void Streams_Finish_Conformance_OperationalPattern (); //SMPTE ST 378 (OP1a), and for version 1.3 SMPTE ST 377-1 (generalized patterns) and SMPTE ST 390 (OP-Atom)
    void Streams_Finish_Conformance_MCA (); //SMPTE ST 377-4 and SMPTE ST 377-41, MCA Label Sub-Descriptors
    void Streams_Finish_Conformance_RiffChunks (); //SMPTE ST 2131
    void Streams_Finish_Conformance_References ();
    #endif //MEDIAINFO_CONFORMANCE
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
    void Streams_Finish_CompliantWith ();
    conformance_errors CompliantWith_ArdZdf (size_t Profile_Pos, bool IsCandidateTest=false); //Profile_Pos is the position in the list of ARD/ZDF profiles, candidate test is done before parsing all partitions, without tests needing all partitions or the essence
    bool CompliantWith_ArdZdf_IsCandidate (); //Candidate for an ARD/ZDF profile with Body Partitions
    size_t CompliantWith_ArdZdf_FromContent (std::string& Content); //Position of the ARD/ZDF profile matching the kind of content, (size_t)-1 if none, Content is filled with a description of the content
    #if MEDIAINFO_CONFORMANCE
    conformance_errors CompliantWith_St2067204 (conformance_errors& Warnings); //ADM Audio Track File, to be done before filling the ADM content, Warnings are filled with the recommendations not followed
    conformance_errors CompliantWith_St20675 (conformance_errors& Warnings, bool IsTrackFile); //IMF Essence Component, with the overall constraints of IMF Track Files if IsTrackFile, Warnings are filled with the recommendations not followed
    conformance_errors CompliantWith_St20672 (conformance_errors& Warnings); //IMF Track File, Warnings are filled with the recommendations not followed
    bool CompliantWith_St20672_IsDeclared (); //SMPTE ST 2067-2 declared by the file (Channel Assignment label)
    bool CompliantWith_St20672_IsCandidate (); //Candidate for an IMF Track File (Audio or Timed Text)
    #endif //MEDIAINFO_CONFORMANCE
    bool CompliantWith_St20672_IsTested (bool WithSt2067204=false); //IMF Track File or IMF Essence Component requested (or ADM Audio Track File if WithSt2067204), or candidate for an IMF Track File

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
    type ItemType;
    void UnknownElement();
    void UnknownGroupItem();

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
    void MCAContent();
    void MCAUseClass();
    void TextDataDescription();
    void ResourceID();
    void NamespaceURI();
    void TextMIMEMediaType();
    void UCSEncoding();
    void Filler();
    void FillerData() {Filler();}
    void TerminatingFillerData();
    void XMLDocumentText_Indirect();
    void SubDescriptors();
    void LensUnitAcquisitionMetadata();
    void CameraUnitAcquisitionMetadata();
    void UserDefinedAcquisitionMetadata();
    void FillerGroup();
    void Sequence();
    void SourceClip();
    void TimecodeGroup();
    void ContentStorage();
    void EssenceData();
    void CDCIDescriptor();
    void RGBADescriptor();
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
    void DescriptiveMarker();
    void SoundDescriptor();
    void DataEssenceDescriptor();
    void MultipleDescriptor();
    void DescriptiveClip();
    void AES3PCMDescriptor();
    void WAVEPCMDescriptor();
    void MPEGVideoDescriptor();
    void JPEG2000SubDescriptor();
    void VBIDataDescriptor();
    void ANCDataDescriptor();
    void MPEGAudioDescriptor();
    void PackageMarkerObject();
    void ApplicationPlugInObject();
    void ApplicationReferencedObject();
    void MCALabelSubDescriptor();
    void MCALabelSubDescriptor_Items();
    void DCTimedTextDescriptor();
    void DCTimedTextResourceSubDescriptor();
    void ContainerConstraintsSubDescriptor();
    void MPEG4VisualSubDescriptor();
    void AudioChannelLabelSubDescriptor();
    void SoundfieldGroupLabelSubDescriptor();
    void GroupOfSoundfieldGroupsLabelSubDescriptor();
    void AVCSubDescriptor();
    void IABEssenceDescriptor();
    void IABSoundfieldLabelSubDescriptor();
    void HeaderPartitionOpenIncomplete();
    void HeaderPartitionClosedIncomplete();
    void HeaderPartitionOpenComplete();
    void HeaderPartitionClosedComplete();
    void BodyPartitionOpenIncomplete();
    void BodyPartitionClosedIncomplete();
    void BodyPartitionOpenComplete();
    void BodyPartitionClosedComplete();
    void GenericStreamPartition();
    void FooterPartitionOpenIncomplete();
    void FooterPartitionClosedIncomplete();
    void FooterPartitionOpenComplete();
    void FooterPartitionClosedComplete();
    void PrimerPack();
    void IndexTableSegment();
    void RandomIndexPack();
    void SDTISystemMetadataPack();
    void SDTIPackageMetadataSet();
    void SDTIPictureMetadataSet();
    void SDTISoundMetadataSet();
    void SDTIDataMetadataSet();
    void SDTIControlMetadataSet();
    void SystemScheme1FirstElement();
    void ProductionFramework();
    void TextBasedFramework();
    void GenericStreamTextBasedSet();
    void MXFGenericStreamDataElementKey_09_01();
    void GenericStreamID();
    void DM_AS_11_Core_Framework();
    void DM_AS_11_Segmentation_Framework();
    void DM_AS_11_UKDPP_Framework();
    void Arri_DmFramework();
    void Arri_DmFramework_Sets();
    void Arri_DmSet();
    void Arri_DmSet_Name();
    void Arri_DmSet_Mime();
    void Arri_DmSet_Json();
    void Arri_DmSet_Schema();
    void Arri_ClipSet();
    void Arri_BinaryPack();
    void Arri_BinaryPack_Header();
    void Arri_BinaryPack_VideoParameters();
    void Dolby_050201000101() { MXFGenericStreamDataElementKey_09_01(); }
    void PHDRImageMetadataItem();
    void ISXDDataEssenceDescriptor();
    void PHDRMetadataTrackSubDescriptor();
    void OmneonVideoNetworksDescriptiveMetadataLinks();
    void OmneonVideoNetworksDescriptiveMetadataData();
    void OmneonVideoNetworksDescriptiveMetadataItems();
    void HdrVividMetadataTrackSubDescriptor();
    void HdrVividDataDefinition();
    void HdrVividSourceTrackID();
    void HdrVividSimplePayloadSID();
    void HdrVividMetadataItem();
    void FFV1PictureSubDescriptor();
    void MGASoundEssenceDescriptor();
    void MGAAudioMetadataSubDescriptor();
    void MGASoundfieldGroupLabelSubDescriptor();
    void SADMAudioMetadataSubDescriptor();
    void RIFFChunkDefinitionSubDescriptor();
    void ADM_CHNASubDescriptor();
    void ADMChannelMapping();
    void RIFFChunkReferencesSubDescriptor();
    void ADMAudioMetadataSubDescriptor();
    void ADMSoundfieldGroupLabelSubDescriptor();

    //Common
    void GenerationInterchangeObject();
    void InterchangeObject();
    void PictureDescriptor();
    void PartitionMetadata();
    void BodyPartition();
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
    void CDCIDescriptor_ComponentDepth();                //3301
    void CDCIDescriptor_HorizontalSubsampling();         //3302
    void CDCIDescriptor_ColorSiting();                   //3303
    void CDCIDescriptor_BlackRefLevel();                 //3304
    void CDCIDescriptor_WhiteReflevel();                 //3305
    void CDCIDescriptor_ColorRange();                    //3306
    void CDCIDescriptor_PaddingBits();                   //3307
    void CDCIDescriptor_VerticalSubsampling();           //3308
    void CDCIDescriptor_AlphaSampleDepth();              //3309
    void CDCIDescriptor_ReversedByteOrder();             //330B
    void ContentStorage_Packages();                             //1901
    void ContentStorage_EssenceData();                 //1902
    void DescriptiveMarker_Duration();                                  //0202 (copy of StructuralComponent_Duration) //TODO: merge with StructuralComponent_Duration
    void DescriptiveMarker_DMFramework();                               //6101
    void DescriptiveMarker_TrackIDs();                                  //6102
    void EssenceData_LinkedPackageUID();               //2701
    void EssenceData_IndexSID();                       //3F06
    void EssenceData_BodySID();                        //3F07
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
    void PictureDescriptor_PictureEssenceCoding();//3201
    void PictureDescriptor_StoredHeight();        //3202
    void PictureDescriptor_StoredWidth();         //3203
    void PictureDescriptor_SampledHeight();       //3204
    void PictureDescriptor_SampledWidth();        //3205
    void PictureDescriptor_SampledXOffset();      //3206
    void PictureDescriptor_SampledYOffset();      //3207
    void PictureDescriptor_DisplayHeight();       //3208
    void PictureDescriptor_DisplayWidth();        //3209
    void PictureDescriptor_DisplayXOffset();      //320A
    void PictureDescriptor_DisplayYOffset();      //320B
    void PictureDescriptor_FrameLayout();         //320C
    void PictureDescriptor_VideoLineMap();        //320D
    void PictureDescriptor_AspectRatio();         //320E
    void PictureDescriptor_AlphaTransparency();   //320F
    void PictureDescriptor_TransferCharacteristic(); //3210
    void PictureDescriptor_ImageAlignmentOffset();//3211
    void PictureDescriptor_FieldDominance();      //3212
    void PictureDescriptor_ImageStartOffset();    //3213
    void PictureDescriptor_ImageEndOffset();      //3214
    void PictureDescriptor_SignalStandard();      //3215
    void PictureDescriptor_StoredF2Offset();      //3216
    void PictureDescriptor_DisplayF2Offset();     //3217
    void PictureDescriptor_ActiveFormatDescriptor();//3218
    void PictureDescriptor_ColorPrimaries();      //3219
    void PictureDescriptor_CodingEquations();     //321A
    void MasteringDisplayPrimaries();
    void MasteringDisplayWhitePointChromaticity();
    void MasteringDisplayMaximumLuminance();
    void MasteringDisplayMinimumLuminance();
    void TextBasedObject();
    void SoundDescriptor_QuantizationBits();      //3D01
    void SoundDescriptor_Locked();                //3D02
    void SoundDescriptor_AudioSamplingRate();     //3D03
    void SoundDescriptor_AudioRefLevel();         //3D04
    void SoundDescriptor_ElectroSpatialFormulation(); //3D05
    void SoundDescriptor_SoundEssenceCompression(); //3D06
    void SoundDescriptor_ChannelCount();          //3D07
    void SoundDescriptor_DialNorm();              //3D0C
    void DataEssenceDescriptor_DataEssenceCoding();      //3E01
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
    void Rsiz();                   //8001
    void Xsiz();                   //8002
    void Ysiz();                   //8003
    void XOsiz();                  //8004
    void YOsiz();                  //8005
    void XTsiz();                  //8006
    void YTsiz();                  //8007
    void XTOsiz();                 //8008
    void YTOsiz();                 //8009
    void Csiz();                   //800A
    void PictureComponentSizing(); //800B
    void CodingStyleDefault();     //
    void QuantizationDefault();    //
    void FFV1InitializationMetadata();     //
    void FFV1IdenticalGOP();               //
    void FFV1MaxGOP();                     //
    void FFV1MaximumBitRate();             //
    void FFV1Version();                    //
    void FFV1MicroVersion();               //
    void MGASoundEssenceBlockAlign() {WAVEPCMDescriptor_BlockAlign();};
    void MGASoundEssenceAverageBytesPerSecond() {WAVEPCMDescriptor_AvgBps();}
    void MGASoundEssenceSequenceOffset() {WAVEPCMDescriptor_SequenceOffset();};
    void RIFFChunkStreamID();
    void RIFFChunkID();
    void RIFFChunkUUID();
    void RIFFChunkHashSHA1();
    void RIFFChunkStreamIDsArray();
    void NumLocalChannels();
    void NumADMAudioTrackUIDs();
    void ADMChannelMappingsArray();
    void LocalChannelID();
    void ADMAudioTrackUID();
    void ADMAudioTrackChannelFormatID();
    void ADMAudioPackFormatID();
    void RIFFChunkStreamID_link1();
    void ADMProfileLevelULBatch();
    void RIFFChunkStreamID_link2();
    void ADMAudioProgrammeID_ST2131();
    void ADMAudioContentID_ST2131();
    void ADMAudioObjectID_ST2131();
    void MGALinkID();                                           //
    void MGAAudioMetadataIndex();                               //
    void MGAAudioMetadataIdentifier();                          //
    void MGAAudioMetadataPayloadULArray();                      //
    void MGAMetadataSectionLinkID();                            //
    void SADMMetadataSectionLinkID() { MGAMetadataSectionLinkID(); }
    void SADMProfileLevelULBatch() { ADMProfileLevelULBatch(); }
    void MPEGAudioBitRate();                         //
    void MultipleDescriptor_FileDescriptors();                  //3F01
    void RFC5646SpokenLanguage();
    void PrimaryExtendedSpokenLanguageCode();                   //
    void SecondaryExtendedSpokenLanguageCode();                 //
    void OriginalExtendedSpokenLanguageCode();                  //
    void SecondaryOriginalExtendedSpokenLanguageCode();         //
    void RFC5646TextLanguageCode();                             //
    void ItemName_ISO7();
    void ItemName();
    void ItemValue_ISO7();
    void ItemValue();
    void SingleSequence();                 //
    void ConstantBPictureCount();                //
    void CodedContentScanning();               //
    void LowDelay();                       //
    void ClosedGOP();                      //
    void IdenticalGOP();                   //
    void MaxGOP();                         //
    void MaxBPictureCount();                  //
    void ProfileAndLevel();                //
    void BitRate();                        //
    void MPEG4VisualSingleSequence() {SingleSequence();}
    void MPEG4VisualConstantBVOPs() {ConstantBPictureCount();}
    void MPEG4VisualCodedContentType() {CodedContentScanning();}
    void MPEG4VisualLowDelay() {LowDelay();}
    void MPEG4VisualClosedGOP() {ClosedGOP();}
    void MPEG4VisualIdenticalGOP() {IdenticalGOP();}
    void MPEG4VisualMaxGOP() {MaxGOP();}
    void MPEG4VisualBVOPCount() {MaxBPictureCount();}
    void MPEG4VisualProfileAndLevel();               //
    void MPEG4VisualBitRate() {BitRate();}
    void AVCConstantBPictureFlag() {ConstantBPictureCount();}
    void AVCCodedContentKind() {CodedContentScanning();}
    void AVCClosedGOPIndicator() {ClosedGOP();}
    void AVCIdenticalGOPIndicator() {IdenticalGOP();}
    void AVCMaximumGOPSize() {MaxGOP();}
    void AVCMaximumBPictureCount() {MaxBPictureCount();}
    void AVCProfile();
    void AVCMaximumBitRate();
    void AVCProfileConstraint();
    void AVCLevel();
    void AVCDecodingDelay();
    void AVCMaximumRefFrames();
    void AVCSequenceParameterSetFlag();
    void AVCPictureParameterSetFlag();
    void AVCAverageBitRate();
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
    void RGBADescriptor_PixelLayout();                   //3401
    void RGBADescriptor_Palette();                       //3403
    void RGBADescriptor_PaletteLayout();                 //3404
    void RGBADescriptor_ScanningDirection();             //3405
    void RGBADescriptor_ComponentMaxRef();               //3406
    void RGBADescriptor_ComponentMinRef();               //3407
    void RGBADescriptor_AlphaMaxRef();                   //3408
    void RGBADescriptor_AlphaMinRef();                   //3409
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
    void TimecodeGroup_StartTimecode();                     //1501
    void TimecodeGroup_RoundedTimecodeBase();               //1502
    void TimecodeGroup_DropFrame();                         //1503
    void Track_EditRate();                                      //4B01
    void Track_Origin();                                        //4B02
    void WAVEPCMDescriptor_AvgBps();                          //3D09
    void WAVEPCMDescriptor_BlockAlign();                      //3D0A
    void WAVEPCMDescriptor_SequenceOffset();                  //3D0B
    void WAVEPCMDescriptor_PeakEnvelopeVersion();             //3D29
    void WAVEPCMDescriptor_PeakEnvelopeFormat();              //3D2A
    void WAVEPCMDescriptor_PointsPerPeakValue();              //3D2B
    void WAVEPCMDescriptor_PeakEnvelopeBlockSize();           //3D2C
    void WAVEPCMDescriptor_PeakChannels();                    //3D2D
    void WAVEPCMDescriptor_PeakFrames();                      //3D2E
    void WAVEPCMDescriptor_PeakOfPeaksPosition();             //3D2F
    void WAVEPCMDescriptor_PeakEnvelopeTimestamp();           //3D30
    void WAVEPCMDescriptor_PeakEnvelopeData();                //3D31
    void WAVEPCMDescriptor_ChannelAssignment();               //3D31
    void LensUnitAcquisitionMetadata_IrisFNumber();                        //8000
    void LensUnitAcquisitionMetadata_FocusPositionFromImagePlane();        //8001
    void LensUnitAcquisitionMetadata_FocusPositionFromFrontLensVertex();   //8002
    void LensUnitAcquisitionMetadata_MacroSetting();                       //8003
    void LensUnitAcquisitionMetadata_LensZoom35mmStillCameraEquivalent();  //8004
    void LensUnitAcquisitionMetadata_LensZoomActualFocalLength();          //8005
    void LensUnitAcquisitionMetadata_OpticalExtenderMagnification();       //8006
    void LensUnitAcquisitionMetadata_LensAttributes();                     //8007
    void LensUnitAcquisitionMetadata_IrisTNumber();                        //8008
    void LensUnitAcquisitionMetadata_IrisRingPosition();                   //8009
    void LensUnitAcquisitionMetadata_FocusRingPosition();                  //800A
    void LensUnitAcquisitionMetadata_ZoomRingPosition();                   //800B
    void CameraUnitAcquisitionMetadata_TransferCharacteristic();           //3210
    void CameraUnitAcquisitionMetadata_ColorPrimaries();                   //3219
    void CameraUnitAcquisitionMetadata_CodingEquations();                  //321A
    void CameraUnitAcquisitionMetadata_AutoExposureMode();                 //8100
    void CameraUnitAcquisitionMetadata_AutoFocusSensingAreaSetting();      //8101
    void CameraUnitAcquisitionMetadata_ColorCorrectionFilterWheelSetting();//8102
    void CameraUnitAcquisitionMetadata_NeutralDensityFilterWheelSetting(); //8103
    void CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveWidth();//8104
    void CameraUnitAcquisitionMetadata_ImageSensorDimensionEffectiveHeight();//8105
    void CameraUnitAcquisitionMetadata_CaptureFrameRate();                 //8106
    void CameraUnitAcquisitionMetadata_ImageSensorReadoutMode();           //8107
    void CameraUnitAcquisitionMetadata_ShutterSpeed_Angle();               //8108
    void CameraUnitAcquisitionMetadata_ShutterSpeed_Time();                //8109
    void CameraUnitAcquisitionMetadata_CameraMasterGainAdjustment();       //810A
    void CameraUnitAcquisitionMetadata_ISOSensitivity();                   //810B
    void CameraUnitAcquisitionMetadata_ElectricalExtenderMagnification();  //810C
    void CameraUnitAcquisitionMetadata_AutoWhiteBalanceMode();             //810D
    void CameraUnitAcquisitionMetadata_WhiteBalance();                     //800E
    void CameraUnitAcquisitionMetadata_CameraMasterBlackLevel();           //810F
    void CameraUnitAcquisitionMetadata_CameraKneePoint();                  //8110
    void CameraUnitAcquisitionMetadata_CameraKneeSlope();                  //8111
    void CameraUnitAcquisitionMetadata_CameraLuminanceDynamicRange();      //8112
    void CameraUnitAcquisitionMetadata_CameraSettingFileURI();             //8113
    void CameraUnitAcquisitionMetadata_CameraAttributes();                 //8114
    void CameraUnitAcquisitionMetadata_ExposureIndexofPhotoMeter();        //8115
    void CameraUnitAcquisitionMetadata_GammaForCDL();                      //8116
    void CameraUnitAcquisitionMetadata_ASC_CDL_V12();                      //8117
    void CameraUnitAcquisitionMetadata_ColorMatrix();                      //8118
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
    void AS_11_Series_Title();
    void AS_11_Programme_Title();
    void AS_11_Episode_Title_Number();
    void AS_11_Shim_Name();
    void AS_11_Audio_Track_Layout();
    void AS_11_Primary_Audio_Language();
    void AS_11_Closed_Captions_Present();
    void AS_11_Closed_Captions_Type();
    void AS_11_Caption_Language();
    void AS_11_Shim_Version();
    void AS_11_Part_Number();
    void AS_11_Part_Total();
    void UKDPP_Production_Number();
    void UKDPP_Synopsis();
    void UKDPP_Originator();
    void UKDPP_Copyright_Year();
    void UKDPP_Other_Identifier();
    void UKDPP_Other_Identifier_Type();
    void UKDPP_Genre();
    void UKDPP_Distributor();
    void UKDPP_Picture_Ratio();
    void UKDPP_3D();
    void UKDPP_3D_Type();
    void UKDPP_Product_Placement();
    void UKDPP_PSE_Pass();
    void UKDPP_PSE_Manufacturer();
    void UKDPP_PSE_Version();
    void UKDPP_Video_Comments();
    void UKDPP_Secondary_Audio_Language();
    void UKDPP_Tertiary_Audio_Language();
    void UKDPP_Audio_Loudness_Standard();
    void UKDPP_Audio_Comments();
    void UKDPP_Line_Up_Start();
    void UKDPP_Ident_Clock_Start();
    void UKDPP_Total_Number_Of_Parts();
    void UKDPP_Total_Programme_Duration();
    void UKDPP_Audio_Description_Present();
    void UKDPP_Audio_Description_Type();
    void UKDPP_Open_Captions_Present();
    void UKDPP_Open_Captions_Type();
    void UKDPP_Open_Captions_Language();
    void UKDPP_Signing_Present();
    void UKDPP_Sign_Language();
    void UKDPP_Completion_Date();
    void UKDPP_Textless_Elements_Exist();
    void UKDPP_Programme_Has_Text();
    void UKDPP_Programme_Text_Language();
    void UKDPP_Contact_Email();
    void UKDPP_Contact_Telephone_Number();
    void DolbyNamespaceURI();
    void PHDRDataDefinition();
    void PHDRSourceTrackID();
    void PHDRSimplePayloadSID();

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
    int32u          Value_UInt;
    int128u         Value_UUID;
    Ztring          Value_String;
    vector<int128u> Value_UUIDVector;
    #if MEDIAINFO_TRACE
    #define Info_UL(_INFO, _NAME, _PARAM) int128u _INFO; Get_UL(_INFO, _NAME, _PARAM)
    #else //MEDIAINFO_TRACE
    #define Info_UL(_INFO, _NAME, _PARAM) Skip_UL(_NAME)
    #endif //MEDIAINFO_TRACE

    //TimeCode
    struct mxftimecode
    {
        int128u InstanceUID;
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
        float64 Get_TimeCode_StartTimecode_Temp(const int64u& File_IgnoreEditsBefore, bool Is1001=false) const
        {
            if (RoundedTimecodeBase)
            {
                TimeCode TimeCode_StartTimecode_Temp((uint64_t)(StartTimecode + File_IgnoreEditsBefore), RoundedTimecodeBase - 1, TimeCode::flags().DropFrame(DropFrame).FPS1001(Is1001));
                return TimeCode_StartTimecode_Temp.ToSeconds();
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
    std::deque<randomindexpack>      RandomIndexPacks; //Front is removed when the partition is parsed
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
    bool   ProbeCaption_IsChecked;
    bool   Track_Number_IsAvailable;
    bool   IsParsingEnd;
    bool   IsCheckingRandomAccessTable;
    bool   IsCheckingFooterPartitionAddress;
    bool   IsSearchingFooterPartitionAddress;
    bool   FooterPartitionAddress_Jumped;
    bool   PartitionPack_Parsed;
    bool   HeaderPartition_IsOpen;
    bool   Is1001;
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
    int32u TrackID_FromTrackNumber(int32u TrackNumber);

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
        int64u Element_Size_Constant; //Size of parsed elements if it is always the same, 0 if no element is parsed, (int64u)-1 if not constant
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
            Element_Size_Constant=0;
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
    struct streamsize_fromelements
    {
        size_t  StreamPos;
        Ztring  ID;
        int64u  StreamSize;
        float64 BitRate;
    };
    std::vector<streamsize_fromelements> StreamSizes_FromElements; //Video stream sizes and bit rates from the size of frame wrapped elements, used only if the file is complete
    size_t IndexTable_SegmentCount; //Count of parsed index table segments
    size_t IndexTable_SegmentCount_Cbe; //Count of parsed index table segments with a non-zero Edit Unit Byte Count
    std::bitset<Stream_Max+1> StreamPos_StartAtZero; //information about the base of StreamPos (0 or 1, 1 is found in 1 file) TODO: per Essence code (last 4 bytes of the Essence header 0xTTXXTTXX)

    //Descriptor
    struct descriptor
    {
        std::vector<int128u> SubDescriptors; //File Descriptors for a Multiple Descriptor
        std::vector<int128u> SubDescriptors_Multiple; //Sub-descriptors of a Multiple Descriptor
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
        #if MEDIAINFO_CONFORMANCE
            int64u LinkedTrackID_Offset; //Only if from FileDescriptor
            int32u SourceTrackID; //PHDR or HDR Vivid metadata track
            int64u SourceTrackID_Offset;
            const char* SourceTrackID_Name;
        #endif //MEDIAINFO_CONFORMANCE
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
        int8u  Jp2kContentKind;
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
            Type_ADMSoundfieldGroupLabelSubDescriptor,
            Type_RIFFChunkDefinitionSubDescriptor,
            Type_RIFFChunkReferencesSubDescriptor,
            Type_ADMAudioMetadataSubDescriptor,
            Type_ADM_CHNASubDescriptor,
            Type_ADMChannelMapping,
        };
        type Type;
        bool HasBFrames;
        bool HasMPEGVideoDescriptor;
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

        //RIFFChunkDefinitionSubDescriptor specific
        int32u      RIFFChunkID;

        //ADM_CHNASubDescriptor specific
        std::vector<int128u> ADMChannelMappings; //Entries of ADMChannelMappingsArray

        //DescriptiveMarker, GenericStreamTextBasedSet, RIFFChunkDefinitionSubDescriptor
        int32u      SID;

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
            #if MEDIAINFO_CONFORMANCE
                LinkedTrackID_Offset=(int64u)-1;
                SourceTrackID=(int32u)-1;
                SourceTrackID_Offset=(int64u)-1;
                SourceTrackID_Name=nullptr;
            #endif //MEDIAINFO_CONFORMANCE
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
            Jp2kContentKind=(int8u)-1;
            FieldTopness=(int8u)-1; //Field x is upper field
            FieldDominance=1; //Default is field 1 temporaly first
            Type=Type_Unknown;
            HasBFrames=false;
            HasMPEGVideoDescriptor=false;
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

            //RIFFChunkDefinitionSubDescriptor specific
            RIFFChunkID=0;

            //DescriptiveMarker, GenericStreamTextBasedSet, RIFFChunkDefinitionSubDescriptor
            SID=0;
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
    void Descriptor_Type_Set(descriptor::type Type); //After the parsing of each item, the type is not set for a File Descriptor or a set of another type with the same Instance UID
    descriptor::type Descriptor_Type_Pending; //Type set by Descriptor_Type_Set() before the Instance UID item of the set
    bool SubDescriptor_IsDangling(const descriptors::iterator& Descriptor);
    descriptors::iterator Descriptor_Single(bool IncludeLinkedToTrack=false);
    descriptors::iterator Descriptor_FromTrackID(int32u TrackID);
    #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
    bool Descriptors_ConfigureForDemux();
    #endif //MEDIAINFO_DEMUX || MEDIAINFO_SEEK

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
        #if MEDIAINFO_CONFORMANCE
            int64u SourceTrackID_Offset;
        #endif //MEDIAINFO_CONFORMANCE
        std::vector<int128u> StructuralComponents; //Sequence from MaterialPackage only
        mxftimecode MxfTimeCode;

        component()
        {
            Duration=(int64u)-1;
            SourceTrackID=(int32u)-1;
            #if MEDIAINFO_CONFORMANCE
                SourceTrackID_Offset=(int64u)-1;
            #endif //MEDIAINFO_CONFORMANCE
        }

        void Update (struct component &New)
        {
            if (New.Duration!=(int64u)-1)
                Duration=New.Duration;
            if (New.SourcePackageID.hi || New.SourcePackageID.lo)
                SourcePackageID=New.SourcePackageID;
            if (New.SourceTrackID!=(int32u)-1)
            {
                SourceTrackID=New.SourceTrackID;
                #if MEDIAINFO_CONFORMANCE
                    SourceTrackID_Offset=New.SourceTrackID_Offset;
                #endif //MEDIAINFO_CONFORMANCE
            }
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

    //Timecode comparison
    struct timecodecache
    {
        struct value
        {
            int64u Offset;
            int64u Value;

            value(int64u Offset = 0, int64u Value = 0)
                : Offset(Offset)
                , Value(Value)
            {}
        };
        std::vector<value> Values;
        size_t StreamPos = (size_t)-1;
    };
    typedef std::map<int128u, timecodecache> timecodecaches; //Key is InstanceUID of the timecode
    timecodecaches TimecodeCaches;

    //Track ID references conformance, filled once streams are known
    #if MEDIAINFO_CONFORMANCE
    struct conformance_reference
    {
        int64u      Offset;
        const char* Field;
        string      Value;
        int32u      TrackID;
    };
    std::vector<conformance_reference> Conformance_References;
    #endif //MEDIAINFO_CONFORMANCE

    //Descriptive Metadata - DescriptiveMarkers
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
    typedef std::map<int128u, dmsegment> dmsegments; //Key is InstanceUID of the DescriptiveMarker
    dmsegments DescriptiveMarkers;

    //Descriptive Metadata - ProductionFramework
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
    typedef std::map<int128u, dmscheme1> dmscheme1s; //Key is InstanceUID of the ProductionFramework
    dmscheme1s ProductionFrameworks;

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
    typedef std::map<int128u, as11> as11s; //Key is InstanceUID of the ProductionFramework
    as11s AS11s;

    //Descriptive Metadata - ARRI
    struct arri_dmset
    {
        Ztring      Name;
        Ztring      Mime;
        std::string Json;
    };
    typedef std::map<int128u, arri_dmset> arri_dmsets; //Key is InstanceUID of the set
    arri_dmsets ArriDmSets;
    std::vector<int128u> ArriDmSets_Order; //InstanceUIDs in file order
    std::set<int128u> ArriDmFrameworks; //InstanceUIDs
    std::vector<std::pair<std::string, Ztring> > ArriBinaryFields;
    std::map<std::string, Ztring> ArriClipFields;
    std::map<int128u, std::pair<Ztring, Ztring> > ArriClipParams; //Key: InstanceUID of set, value: name and version
    std::vector<int128u> ArriClipParams_Order; //InstanceUIDs in file order
    std::vector<int128u> ArriClipParams_Refs; //InstanceUIDs in referenced order
    std::map<int128u, std::map<std::string, Ztring> > ArriClipObjects; //Key: InstanceUID of LUT or color space set
    std::map<int128u, std::map<std::string, int128u> > ArriClipObjectRefs; //Key: InstanceUID of LUT set
    std::vector<int128u> ArriClipLuts_Refs; //InstanceUIDs in referenced order
    std::map<int8u, std::vector<std::pair<std::string, Ztring> > > ArriClipLooks; //Key: look index (Mini)

    //Descriptive Metadata - Omneon
    struct dmomneonlink
    {
        std::vector<int128u> Links;
    };
    typedef std::map<int128u, dmomneonlink> dmomneonlinks; //Key is InstanceUID
    dmomneonlinks DMOmneonLinks;
    struct dmomneon
    {
        Ztring      Name;
        Ztring      Value;
    };
    typedef std::map<int128u, dmomneon> dmomneons; //Key is InstanceUID
    dmomneons DMOmneons;
    Ztring      Name;

    //Parsers
    void           ChooseParser__FromCodingScheme(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser__FromEssenceContainer(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser__FromEssence(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
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
    void           ChooseParser_ArriRaw(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Ffv1(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Isxd(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Phdr(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_HdrVivid(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_DolbyVisionFrameData(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Iab(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Mga(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Mxf(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Vc1(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Vbi(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Ancillary(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_xAnc(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Hdcam(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser_Hevc(const essences::iterator& Essence, const descriptors::iterator& Descriptor);
    void           ChooseParser(const essences::iterator& Essence, const descriptors::iterator& Descriptor, stream_t StreamKind, const char* Format = nullptr);

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
    bool   Essences_FirstEssence_Parsed;
    bool   MayHaveCaptionsInStream;
    bool   StereoscopicPictureSubDescriptor_IsPresent;
    bool   UserDefinedAcquisitionMetadata_UdamSetIdentifier_IsSony;
    int32u Essences_UsedForFrameCount;
    int32u IndexTable_NSL;
    int32u IndexTable_NPE;
    struct systemscheme1
    {
        vector<TimeCode> TimeCodeArray_StartTimecodes;
        string ID;
    };
    typedef std::map<int16u, systemscheme1> systemscheme1s;
    systemscheme1s SystemScheme1s;
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
        int64u ThisPartition;
        int64u PreviousPartition;
        int64u FooterPartition;
        int64u HeaderByteCount;
        int64u IndexByteCount;
        int64u BodyOffset;
        int32u BodySID;
        int32u IndexSID;
        int32u KAGSize;
        int128u OperationalPattern;
        int16u MajorVersion;
        int16u MinorVersion;
        int8u  Kind; //Byte 14 of the key: 0x02=Header, 0x03=Body or Generic Stream, 0x04=Footer
        int8u  Status; //Byte 15 of the key: 0x01=Open Incomplete, 0x02=Closed Incomplete, 0x03=Open Complete, 0x04=Closed Complete, 0x11=Generic Stream

        partition()
        {
            StreamOffset=0;
            PartitionPackByteCount=(int64u)-1;
            ThisPartition=0;
            PreviousPartition=0;
            FooterPartition=0;
            HeaderByteCount=0;
            IndexByteCount=0;
            BodyOffset=0;
            BodySID=0;
            IndexSID=0;
            KAGSize=0;
            OperationalPattern=0;
            MajorVersion=0;
            MinorVersion=0;
            Kind=0;
            Status=0;
        }

        bool operator < (const partition& lhs) const
        {
            return StreamOffset<lhs.StreamOffset;
        }
    };
    typedef std::vector<partition>  partitions;
    partitions                      Partitions;

    //Raw values of local set properties, for checking compliance with specifications
    typedef std::map<int64u, std::string> properties; //Key is static local tag or low part of the dynamic property UL, value is the raw content (truncated if long)
    std::map<int128u, properties>   SetProperties; //Key is InstanceUID of the set
    std::set<std::pair<int64u, int64u> > SetProperties_FooterDifferences; //Low part of the set key and property key, for properties having another value in the Footer Partition
    std::set<std::pair<int64u, int64u> > SetProperties_FooterPresenceDifferences; //Low part of the set key and property key, for properties present in only one of the Header Metadata repetitions
    #if MEDIAINFO_CONFORMANCE
        struct instanceuid_set
        {
            int64u                  Partition; //Offset of the partition of the Header Metadata
            int64u                  StreamOffset; //Offset of the set
            int64u                  Key; //Low part of the set key
            int64u                  StreamOffset_Kept; //Offset of the first parsed set with this Instance UID, the one in SetKeys and SetProperties, (int64u)-1 if only dark sets
            int64u                  Key_Kept; //Low part of its set key
            int64u                  Key_FileDescriptor; //Low part of the set key of the first parsed File Descriptor with this Instance UID, 0 if none
        };
        std::map<int128u, instanceuid_set> InstanceUID_Sets; //Key is InstanceUID, value is the first set with this Instance UID in the last Header Metadata parsed containing it
        struct instanceuid_duplicate
        {
            int128u                 InstanceUID;
            int64u                  Key; //Low part of the key of the first set with this Instance UID
            int64u                  Key_Duplicate; //Low part of the key of the set reusing it
        };
        std::map<int64u, instanceuid_duplicate> InstanceUID_Duplicates; //Key is the offset of a set reusing the Instance UID of another set of the same Header Metadata
        std::map<std::pair<int128u, int64u>, properties> SetProperties_Duplicates; //Key is InstanceUID and low part of the set key, for the first parsed set of each kind with an Instance UID reused in its Header Metadata (SetProperties and SetKeys have the first parsed set)
    #endif //MEDIAINFO_CONFORMANCE

    //Helpers for checking compliance with specifications, on raw values of local set properties
    struct property_expected
    {
        int64u          Key; //Static local tag or low part of the dynamic property UL
        const char*     Name;
        std::vector<std::string> Values; //Empty if any value is accepted
        bool            AbsentIsAccepted; //Default value is the expected one
    };
    struct track_info
    {
        tracks::iterator    Track;
        char                Kind; //T=Timecode, V=Picture, A=Sound, D=Data, M=Descriptive Metadata
        int32u              TrackNumber;
        const properties*   Properties;
    };
    static std::string          Raw_UInt(int64u Value, size_t Size); //Big endian
    static std::string          Raw_Rational(int32u Numerator, int32u Denominator);
    static std::string          Raw_UL(int64u Hi, int64u Lo);
    static std::string          Raw_Batch(const std::vector<int32u>& Values); //Batch of 4-byte values
    static bool                 Raw_IsSame(const std::string& Value, const std::string& Expected); //Version byte of UL is ignored
    static std::string          Raw_Hex(const std::string& Value); //Bytes in hexadecimal with a dot every 4 bytes, e.g. "060E2B34.0401010D.04010202.01316001"
    static std::string          Raw_Text(const std::string& Value, bool IsRational=false); //Value for messages, e.g. "10", "50/1", "060E2B34.0401010D.04010202.01316001" or "{21, 584}"
    static bool                 Raw_UInt_Get(const std::string* Value, size_t Size, int64u& Result); //Big endian, returns false if absent or with another size
    static std::string          Raw_UTF16_Get(const std::string* Value); //UTF-16 big endian, characters out of ISO 7 replaced by '?', null characters at the end ignored, empty if absent
    const properties&           SetProperties_Get(const int128u& InstanceUID) const; //Empty if the set is not parsed
    const properties&           SetProperties_Get(const int128u& InstanceUID, int64u Key) const; //Set of this kind (low part of the set key), also if another set of the same Header Metadata reuses its Instance UID
    static const std::string*   Property_Get(const properties& Properties, int64u Key);
    static const std::string*   Property_Get(const properties& Properties, int16u Tag, int64u UL_Lo); //Static tag or dynamic tag
    static bool                 Properties_Test(const properties& Properties, const std::vector<property_expected>& List, conformance_errors& Errors); //Returns true if the test must stop
    static bool                 Properties_TestRange(const properties& Properties, int64u Key, const char* Name, int32u Min, int32u Max, conformance_errors& Errors); //4-byte value, returns true if the test must stop
    static bool                 IsOP1a(int64u Hi, int64u Lo); //OP1a with internal essence
    bool                        Package_Tracks(const package& Package, std::vector<track_info>& List, std::string& Kinds, conformance_errors& Errors); //Kinds has a character per track except Descriptive Metadata, returns true if the test must stop
    static std::string          Tracks_Kinds_Description(const std::string& Kinds); //e.g. "1 Timecode, 1 Video, 16 Audio"
    const randomindexpack*      RandomIndexPack_Find(int64u ByteOffset) const; //Entry of the Random Index Pack with this byte offset, nullptr if not listed
    std::vector<std::string>    HeaderMetadata_FooterDifferences(bool WithPresence=false) const; //Properties of the Header Metadata having another value in the Footer Partition (or present in only one of them if WithPresence), e.g. "Duration of Sequence"
    struct indextablesegment
    {
        int64u      StreamOffset;
        properties  Properties;
        std::string Flags; //Flags of each index entry
    };
    std::vector<indextablesegment>  IndexTableSegments;
    std::string                     IndexTableSegment_Flags; //Flags of each index entry of the index table segment being parsed
    std::vector<int32u>             ContentPackage_Elements; //Track Number of essence elements, in order of first appearance
    bool                            ContentPackage_SystemItemIsFirst;
    bool                            SDTI_PackageMetadataSet_IsPresent;
    bool                            PrivateMetadata_IsPresent;
    std::vector<randomindexpack>    RandomIndexPack_Entries; //All partitions listed in the Random Index Pack
    int64u                          RandomIndexPack_Offset; //Offset of the Random Index Pack, (int64u)-1 if not parsed
    int64u                          RandomIndexPack_Size; //Size of the Random Index Pack
    int32u                          RandomIndexPack_Length; //Value of the Length item of the Random Index Pack
    struct systemitem_checks
    {
        int16u  ContentPackageRate=(int16u)-1;  //Package Rate and 1.001 Flag, (int16u)-1 if not yet parsed, (int16u)-2 if not constant
        bool    TimeCode_IsMissing=false;       //A System Item has no timecode
        bool    TimeCode_IsNotContinuous=false;
        int64u  TimeCode_Count=0;               //Count of timecodes since the last jump
        int32u  TimeCode_First_Seconds=0;       //First timecode since the last jump
        int8u   TimeCode_First_Frames=0;
        int8u   TimeCode_Modes=0;               //Bit 0: frame count at full rate, bits 1+: frame count at full rate divided by pair count, with first frame being the frame of the pair with this index
        int64u  ContentPackage_Offset=(int64u)-1; //Offset of the previous System Item since the last jump
        int64u  ContentPackage_Size=(int64u)-1; //Distance between System Items, (int64u)-1 if not yet parsed, (int64u)-2 if not constant
        std::vector<int32u> ContentPackage_Elements; //Track Number of essence elements since the previous System Item
        std::vector<int32u> ContentPackage_Elements_First; //Track Number of essence elements of the first content package between contiguous System Items
        bool    ContentPackage_Elements_IsNotConstant=false; //Essence elements are not in the same order in all content packages between contiguous System Items
    };
    systemitem_checks               SystemItem_Checks;
    void SystemItem_Checks_TimeCode(int32u Seconds, int8u Frames, int8u FrameRate, int8u PairCount);
    int32u                          ArdZdf_IsRequested; //Bit per ARD/ZDF profile requested by the user, conformance errors are reported
    bool                            ArdZdf_FromContent_IsRequested; //ARD/ZDF profile matching the kind of content requested by the user ("ARD_ZDF"), conformance errors are reported
    #if MEDIAINFO_CONFORMANCE
        bool                        St2067204_IsRequested; //SMPTE ST 2067-204 requested by the user, conformance errors are reported
        bool                        St2067204_IsTested; //Candidate (RIFF Chunks or ADM sets) or requested
        conformance_errors          St2067204_Errors;
        conformance_errors          St2067204_Warnings;
        bool                        St20672_IsRequested; //SMPTE ST 2067-2 requested by the user, conformance errors are reported
        bool                        St20675_IsRequested; //SMPTE ST 2067-5 requested by the user, conformance errors are reported
        size_t                      RiffChunks_ErrorCount; //Count of SMPTE ST 2131 errors
        size_t                      McaLabels_ErrorCount; //Count of SMPTE ST 377-4 errors on Audio Channel, Soundfield Group and Group of Soundfield Groups labels
        void                        CompliantWith_St20672_Audio(const descriptor& Sound, const properties& Sound_Properties, conformance_errors& Errors, conformance_errors& Warnings); //Audio Track File constraints, except the Channel Assignment and the audio labeling
        void                        CompliantWith_St20672_Labels(const properties& Sound_Properties, const std::vector<int128u>& SubDescriptors, conformance_errors& Errors); //Audio labeling of a Sound Track
        std::vector<int128u>        SubDescriptors_Get(const int128u& DescriptorUID); //Sub-descriptors of a File Descriptor, also if the SubDescriptors item is not parsed
        std::set<int128u>           Descriptors_InUse(); //Descriptors and sub-descriptors of the Header Metadata in use, empty if all descriptors are in use
        void                        ContentStorage_Packages_Get(std::vector<int128u>& MaterialPackages, std::vector<int128u>& TopLevelPackages, std::vector<int128u>& EssenceContainerDatas); //Content Storage of the current Preface, once per Package UID
    #endif //MEDIAINFO_CONFORMANCE
    int64u                          Anc_ElementSize; //Size of the ANC Data Elements, (int64u)-1 if not yet parsed, (int64u)-2 if not constant
    void Property_Store(properties& Properties, int64u Size);
    void Properties_Store(properties& Properties);
    void PrivateMetadata_Test(const int128u& UL);
    size_t                          Partitions_Pos;
    bool                            Partitions_IsCalculatingHeaderByteCount;
    bool                            Partitions_IsCalculatingSdtiByteCount;
    bool                            Partitions_IsFooter;
    #if MEDIAINFO_CONFORMANCE
        int64u                      GenericStreamPartition_PackEnd; //Offset after the Generic Stream Partition Pack just parsed, (int64u)-1 if the last Partition Pack is not a Generic Stream one

        //KLV coding and Header Metadata (SMPTE ST 377-1), checked while parsing and reported once streams are known
        struct headermetadata_error
        {
            int64u                  Offset;
            const char*             Field;
            std::string             Value;
        };
        std::vector<headermetadata_error> HeaderMetadata_Errors; //Errors found while parsing
        std::map<int128u, int64u>   SetKeys; //Key is InstanceUID of a Header Metadata Set (including dark sets), value is the low part of the set key
        bool                        SetKey_Is(const int128u& InstanceUID, int64u Key) const; //A set with this Instance UID is of this kind (low part of the set key), also if another set of the same Header Metadata reuses its Instance UID
        int64u                      SetKey_FileDescriptor_Get(const int128u& InstanceUID) const; //Low part of the key of the File Descriptor with this Instance UID, also if another set of the same Header Metadata reuses its Instance UID, else of the set with this Instance UID, 0 if not present
        bool                        InstanceUID_IsReused(const int128u& InstanceUID) const; //Another set of a Header Metadata (also a dark set) reuses this Instance UID
        bool                        Set_IsFileDescriptor; //The set being parsed is a File Descriptor
        std::map<int128u, int64u>   SetPartitions; //Key is InstanceUID of a Header Metadata Set, value is the offset of the partition of its last occurrence
        std::map<int16u, int64u>    HeaderMetadata_TagsNotInPrimer; //Local Tags used in Header Metadata Sets but not in the Primer Pack of their partition, value is the offset of the first set using it
        std::map<int64u, std::set<int128u> > HeaderMetadata_Prefaces; //Key is the offset of the partition, value is the Instance UIDs of its Preface sets
        std::map<int64u, std::set<int128u> > HeaderMetadata_ContentStorages; //Key is the offset of the partition, value is the Instance UIDs of its Content Storage sets
        std::set<int16u>            PrimerPack_Tags; //Local Tags of the Primer Pack of the current partition
        bool                        PrimerPack_IsParsed; //The Primer Pack of the current partition is parsed
        int64u                      HeaderMetadata_Partition; //Offset of the partition of the Header Metadata being parsed
        int64u                      HeaderMetadata_End; //Offset after the Header Metadata being parsed, (int64u)-1 if none
        int8u                       HeaderMetadata_Step; //KLV packet sequence: 0 not tested, 1 after the Partition Pack (Primer Pack expected), 2 after the Primer Pack (Preface expected as first set)
        bool                        HeaderMetadata_FillIsParsed; //A KLV Fill item is between the Partition Pack and the Primer Pack
        int64u                      Klv_Ber80_Offset; //Offset of the first BER length using the 80h token, (int64u)-1 if none
        int64u                      Klv_BerTooLong_Offset; //Offset of the first BER length longer than 9 bytes, (int64u)-1 if none
        int8u                       Klv_BerTooLong_Size;
        void HeaderMetadata_Sequence_Test();
    #endif //MEDIAINFO_CONFORMANCE

    //Config
    bool                            TimeCodeFromMaterialPackage;

    //CameraUnitAcquisitionMetadata
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
    struct generic_stream_data_element_key
    {
        File__Analyze* Parser = {};
        int32u SID = {};
        #if MEDIAINFO_CONFORMANCE
            int64u Key_Lo = {}; //Low part of the key of the Generic Stream Data Element
            bool IsAfterPartitionPack = {}; //Directly after a Generic Stream Partition Pack
            std::string Format; //Format of the content, empty if unknown
            std::string SHA1; //SHA-1 digest of the content, empty if not computed
        #endif //MEDIAINFO_CONFORMANCE

        generic_stream_data_element_key() = default;
        generic_stream_data_element_key(const generic_stream_data_element_key& Value) = delete;

        ~generic_stream_data_element_key()
        {
            delete Parser;
        }
    };
    std::map<int64u, generic_stream_data_element_key> MXFGenericStreamDataElementKey; // Key is file offset
    std::vector<File__Analyze*> ToMergeLater;
    int64u ExtraMetadata_Offset;
    std::set<int32u> ExtraMetadata_SID;
    File__Analyze* DolbyAudioMetadata;
    #if defined(MEDIAINFO_ADM_YES)
    File__Analyze* Adm;
    std::map<int128u, set<int128u> > ADMProfileLevelULBatch_List;
    void CompareAdmProfiles();
    void Streams_Finish_ADM_CHNA();
    #endif
    #if defined(MEDIAINFO_IAB_YES)
    File__Analyze* Adm_ForLaterMerge;
    #endif
    bool IsArriExperimental = false;
        
    //Demux
    #if MEDIAINFO_DEMUX
        bool Demux_HeaderParsed;
        essences::iterator Demux_CurrentEssence;
    #endif //MEDIAINFO_DEMUX

    float64 Demux_Rate;
    #if MEDIAINFO_DEMUX || MEDIAINFO_SEEK
        size_t CountOfLocatorsToParse;

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

    void Streams_Finish_Descriptor (descriptors::iterator Descriptor);
    void Streams_Finish_SoundfieldGroupLabel (const descriptor& Label, size_t StreamPos);
    std::map<size_t, std::vector<Ztring> > ADMSoundfieldGroupLabel_Languages; //Spoken languages of the ADM soundfield groups, per audio stream
};

} //NameSpace

#endif
