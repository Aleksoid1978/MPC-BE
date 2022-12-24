/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about USAC payload of various files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_UsacH
#define MediaInfo_File_UsacH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef MEDIAINFO_MPEG4_YES
    #include "MediaInfo/Multiple/File_Mpeg4_Descriptors.h"
#endif
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Usac
//***************************************************************************


class File_Usac : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Usac();
    ~File_Usac();

    #if MEDIAINFO_CONFORMANCE
    enum conformance_level
    {
        Error,
        Warning,
        Info,
        ConformanceLevel_Max
    };
    struct field_value
    {
        string Field;
        string Value;
        bitset8 Flags;
        vector<int64u> FramePoss;

        field_value(string&& Field, string&& Value, bitset8 Flags, int64u FramePos)
            : Field(Field)
            , Value(Value)
            , Flags(Flags)
        {
            FramePoss.push_back(FramePos);
        }

        friend bool operator==(const field_value& l, const field_value& r)
        {
            return l.Field==r.Field && l.Value==r.Value && l.Flags.to_int8u()==r.Flags.to_int8u();
        }
    };
    #endif

    //Bookmark
    struct bs_bookmark
    {
        int64u                      Element_Offset;
        size_t                      Trusted;
        size_t                      NewSize;
        size_t                      End;
        int8u                       BitsNotIncluded;
        bool                        UnTrusted;
        #if MEDIAINFO_CONFORMANCE
            vector<field_value>     ConformanceErrors[ConformanceLevel_Max];
        #endif
    };
    bs_bookmark                     BS_Bookmark(size_t NewSize);
    #if MEDIAINFO_CONFORMANCE
    bool                            BS_Bookmark(bs_bookmark& B, const string& ConformanceFieldName);
    #else
    bool                            BS_Bookmark(bs_bookmark& B);
    inline bool                     BS_Bookmark(bs_bookmark& B, const string& ConformanceFieldName) {return BS_Bookmark(B);}
    #endif

    //Fill
    void Fill_DRC(const char* Prefix=NULL);
    void Fill_Loudness(const char* Prefix=NULL, bool NoConCh=false);

    //Elements - USAC - Config
    void UsacConfig                         (size_t BitsNotIncluded=(size_t)-1);
    void UsacDecoderConfig                  ();
    void UsacSingleChannelElementConfig     ();
    void UsacChannelPairElementConfig       ();
    void UsacLfeElementConfig               ();
    void UsacExtElementConfig               ();
    void UsacCoreConfig                     ();
    void SbrConfig                          ();
    void SbrDlftHeader                      ();
    void Mps212Config                       (int8u StereoConfigindex);
    void uniDrcConfig                       ();
    void uniDrcConfigExtension              ();
    void downmixInstructions                (bool V1=false);
    void drcCoefficientsBasic               ();
    void drcCoefficientsUniDrc              (bool V1=false);
    void drcInstructionsBasic               ();
    bool drcInstructionsUniDrc              (bool V1=false, bool NoV0=false);
    void channelLayout                      ();
    void UsacConfigExtension                ();
    void loudnessInfoSet                    (bool V1=false);
    bool loudnessInfo                       (bool FromAlbum, bool V1=false);
    void loudnessInfoSetExtension           ();
    void streamId                           ();

    //Elements - USAC - Frame
    #if MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE
    void UsacFrame                          (size_t BitsNotIncluded=(size_t)-1);
    void UsacSingleChannelElement           ();
    void UsacChannelPairElement             ();
    void UsacLfeElement                     ();
    void UsacExtElement                     (size_t elemIdx, bool usacIndependencyFlag);
    void AudioPreRoll                       ();
    #endif //MEDIAINFO_TRACE || MEDIAINFO_CONFORMANCE

    //Utils
    void escapedValue                       (int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);

    //***********************************************************************
    // Temp AAC
    //***********************************************************************

    int8u   channelConfiguration;
    int8u   sampling_frequency_index;
    int8u   extension_sampling_frequency_index;

    //***********************************************************************
    // Conformance
    //***********************************************************************

    #if MEDIAINFO_CONFORMANCE
    enum conformance_flags
    {
        None,
        Usac,
        BaselineUsac,
        xHEAAC,
        MpegH,
        Conformance_Max
    };
    bitset8                         ConformanceFlags;
    vector<field_value>             ConformanceErrors_Total[ConformanceLevel_Max];
    vector<field_value>             ConformanceErrors[ConformanceLevel_Max];
    audio_profile                   Format_Profile;
    bool CheckIf(const bitset8 Flags) { return !ConformanceFlags || !Flags || (ConformanceFlags & Flags); }
    void Fill_Conformance(const char* Field, const char* Value, bitset8 Flags={}, conformance_level Level=Error);
    void Fill_Conformance(const char* Field, const string Value, bitset8 Flags={}, conformance_level Level=Error) { Fill_Conformance(Field, Value.c_str(), Flags, Level); }
    void Fill_Conformance(const char* Field, const char* Value, conformance_flags Flag, conformance_level Level=Error) { Fill_Conformance(Field, Value, bitset8().set(Flag)); }
    void Fill_Conformance(const char* Field, const string Value, conformance_flags Flag, conformance_level Level=Error) { Fill_Conformance(Field, Value.c_str(), Flag, Level); }
    void Clear_Conformance();
    void Merge_Conformance(bool FromConfig=false);
    void Streams_Finish_Conformance();
    #else
    inline void Streams_Finish_Conformance() {}
    #endif

    //***********************************************************************
    // Others
    //***********************************************************************

    struct usac_element
    {
        int32u                      usacElementType;
        int32u                      usacExtElementDefaultLength;
        bool                        usacExtElementPayloadFrag;
    };
    struct downmix_instruction
    {
        int8u                       targetChannelCount;
    };
    typedef std::map<int8u, downmix_instruction> downmix_instructions;
    struct drc_id
    {
        int8u   drcSetId;
        int8u   downmixId;
        int8u   eqSetId;

        drc_id(int8u drcSetId, int8u downmixId=0, int8u eqSetId=0)
          : drcSetId(drcSetId),
            downmixId(downmixId),
            eqSetId(eqSetId)
        {}

        bool empty() const
        {
            return !drcSetId && !downmixId && !eqSetId;
        }

        string to_string() const
        {
            if (empty())
                return string();
            string Id=std::to_string(drcSetId);
            Id+='-';
            Id+= std::to_string(downmixId);
            //if (V1)
            //{
            //    Id+='-';
            //    Id+=std::to_string(eqSetId);
            //}
            return Id;
        }
        friend bool operator<(const drc_id& l, const drc_id& r)
        {
            //return memcmp(&l, &r, sizeof(drc_id));
            if (l.drcSetId<r.drcSetId)
                return true;
            if (l.drcSetId!=r.drcSetId)
                return false;
            if (l.downmixId<r.downmixId)
                return true;
            if (l.downmixId!=r.downmixId)
                return false;
            if (l.eqSetId<r.eqSetId)
                return true;
            return false;
        }
    };
    struct loudness_info
    {
        struct measurements
        {
            Ztring                  Values[16];
        };
        Ztring                      SamplePeakLevel;
        Ztring                      TruePeakLevel;
        measurements                Measurements;
    };
    typedef std::map<drc_id, loudness_info> loudness_infos;
    struct drc_info
    {
        string                      drcSetEffectTotal;
    };
    typedef std::map<drc_id, drc_info> drc_infos;
    struct gain_set
    {
        int8u                       bandCount;
    };
    typedef std::vector<gain_set> gain_sets;
    struct usac_config
    {
        vector<usac_element>        usacElements;
        downmix_instructions        downmixInstructions_Data;
        loudness_infos              loudnessInfo_Data[2]; // By non-album/album
        drc_infos                   drcInstructionsUniDrc_Data;
        gain_sets                   gainSets;
        #if MEDIAINFO_CONFORMANCE
        size_t                      loudnessInfoSet_Present[2];
        #endif
        int32u                      numOutChannels;
        int32u                      sampling_frequency;
        int8u                       channelConfiguration;
        int8u                       sampling_frequency_index;
        int8u                       coreSbrFrameLengthIndex;
        int8u                       baseChannelCount;
        bool                        IsNotValid;
        bool                        loudnessInfoSet_IsNotValid;
        bool                        harmonicSBR;
    };
    struct usac_frame
    {
        int32u                      numPreRollFrames;
    };
    usac_config                     Conf; //Main conf
    usac_config                     C; //Current conf
    usac_frame                      F; //Current frame
    int8u                           IsParsingRaw;
};

} //NameSpace

#endif

