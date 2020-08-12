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
#include "MediaInfo/File__Analyze.h"
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

    //Fill
    void Fill_DRC(const char* Prefix=NULL);
    void Fill_Loudness(const char* Prefix=NULL, bool NoConCh=false);

    //Elements - USAC
    void UsacConfig                         ();
    void UsacDecoderConfig                  (int8u coreSbrFrameLengthIndex);
    void UsacSingleChannelElementConfig     (int8u coreSbrFrameLengthIndex);
    void UsacChannelPairElementConfig       (int8u coreSbrFrameLengthIndex);
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
    void escapedValue                       (int32u &Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);

    //***********************************************************************
    // Temp USAC
    //***********************************************************************

    int8u   channelConfiguration;
    int8u   sampling_frequency_index;
    int8u   extension_sampling_frequency_index;

    struct drc_info
    {
        string                      drcSetEffectTotal;
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
    std::map<int16u, drc_info>      drcInstructionsUniDrc_Data; // By id
    std::map<Ztring, loudness_info> loudnessInfo_Data[2]; // By non-album/album then by id
    int8u                           baseChannelCount;
    struct downmix_instruction
    {
        int8u                       targetChannelCount;
    };
    std::map<int8u, downmix_instruction> downmixInstructions_Data;
    struct gain_set
    {
        int8u                       bandCount;
    };
    std::vector<gain_set>           gainSets;
    bool                            loudnessInfoSet_Present;
};

} //NameSpace

#endif

