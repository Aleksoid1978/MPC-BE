/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Mpegh3daH
#define MediaInfo_File_Mpegh3daH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File__Tags.h"
#include "MediaInfo/Audio/File_Aac.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Mpegh3da
//***************************************************************************

class File_Mpegh3da : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Mpegh3da();

private :
    //Info
    struct speaker_info
    {
        bool angularPrecision;
        int16u AzimuthAngle;
        bool AzimuthDirection;
        int16u ElevationAngle;
        bool ElevationDirection;
        bool isLFE;

        speaker_info(bool angularPrecision) :
            angularPrecision(angularPrecision),
            AzimuthAngle(0),
            AzimuthDirection(false),
            ElevationAngle(0),
            ElevationDirection(false),
            isLFE(false)
        {};
    };

    struct speaker_layout
    {
        int32u numSpeakers;
        vector<Aac_OutputChannel> CICPspeakerIdxs;
        vector<speaker_info> SpeakersInfo;

        int8u ChannelLayout;
        speaker_layout() :
            numSpeakers(0),
            ChannelLayout(0)
        {};
    };

    speaker_layout referenceLayout;
    int8u mpegh3daProfileLevelIndication;
    int32u usacSamplingFrequency;
    int8u coreSbrFrameLengthIndex;

    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Sync();
    void mpegh3daConfig();
    void SpeakerConfig3d(speaker_layout& Layout);
    void mpegh3daFlexibleSpeakerConfig(speaker_layout& Layout);
    void mpegh3daSpeakerDescription(speaker_layout& Layout);
    void mpegh3daFrame();

    //Helpers
    void Streams_Fill_ChannelLayout(const string& Prefix, const speaker_layout& Layout, int8u speakerLayoutType=0);
    void escapedValue(int32u& Value, int8u nBits1, int8u nBits2, int8u nBits3, const char* Name);
};

} //NameSpace

#endif
