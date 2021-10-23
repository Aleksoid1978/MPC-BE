/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Some containers use 4-channel stream for 2 AES3 (Stereo) transport
// We need to split the 4-channel streams in two before sending
// data to AES parser
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_ChannelSplittingH
#define MediaInfo_File_ChannelSplittingH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_Pcm.h"
#include <cstring>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_ChannelSplitting
//***************************************************************************
#ifdef MEDIAINFO_SMPTEST0337_YES
class File_ChannelSplitting : public File_Pcm_Base
{
public :
    //In
    Ztring  Codec;
    int8u   Sign;
    int8u   BitDepth;
    int16u  SamplingRate;
    bool    Aligned;

    struct common
    {
        struct channel
        {
            int8u*          Buffer;
            size_t          Buffer_Size;
            size_t          Buffer_Size_Max;
            std::vector<File__Analyze*> Parsers;
            bool            IsPcm;

            channel()
            {
                Buffer=NULL;
                Buffer_Size=0;
                Buffer_Size_Max=0;
                IsPcm=false;
            }

            ~channel()
            {
                delete[] Buffer; //Buffer=NULL;
                for (size_t Pos=0; Pos<Parsers.size(); Pos++)
                    delete Parsers[Pos];
            }

            void resize(size_t NewSize)
            {
                delete[] Buffer;
                Buffer_Size_Max=NewSize;
                Buffer=new int8u[Buffer_Size_Max];
            }
        };
        vector<channel*>    SplittedChannels[2]; //0 = Subframe, 1=Frame

        common()
        {
        }

        ~common()
        {
            for (int c=0; c<2; c++)
                for (size_t Pos=0; Pos<SplittedChannels[c].size(); Pos++)
                    delete SplittedChannels[c][Pos]; //Channels[c][Pos]=NULL;
        }
    };
    int64u  StreamID;
    common* Common;
    int8u   Channel_Total;

    //Constructor/Destructor
    File_ChannelSplitting();
    ~File_ChannelSplitting();

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Global
    void Read_Buffer_Init ();
    void Read_Buffer_Continue ();
    void Read_Buffer_Continue_Parse ();
    void Read_Buffer_Unsynched();

    //Temp
    bool AllFilled;
    bool AllFinished;
    size_t SplittedChannels_c;
    size_t SplittedChannels_i;
};

#endif // MEDIAINFO_SMPTEST0337_YES
} //NameSpace

#endif

