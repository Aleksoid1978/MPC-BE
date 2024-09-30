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
#if defined(MEDIAINFO_SMPTEST0337_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_ChannelSplitting.h"
#if defined(MEDIAINFO_SMPTEST0337_YES)
    #include "MediaInfo/Audio/File_SmpteSt0337.h"
#endif
#if defined(MEDIAINFO_PCM_YES)
    #include "MediaInfo/Audio/File_Pcm.h"
#endif
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

File_ChannelSplitting::File_ChannelSplitting()
:File_Pcm_Base()
{
    //Configuration
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=0; // MediaInfo_Parser_ChannelSplitting;
        StreamIDs_Width[0]=1;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_TRACE
        Trace_Layers_Update(0); //Container1
    #endif //MEDIAINFO_TRACE
    StreamSource=IsStream;

    //In
    BitDepth=0;
    Sign='\0';
    SamplingRate=0;
    Aligned=false;
    Common=NULL;
    Channel_Total=1;
}

File_ChannelSplitting::~File_ChannelSplitting()
{
    delete Common; //Common=NULL;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_ChannelSplitting::Streams_Fill()
{
    size_t TotalChannelsCount=0;
    size_t PcmChannelsCount=0;
    size_t AdmChannelsCount=0;
    for (int c=0; c<2; c++)
    {
        TotalChannelsCount+=Common->SplittedChannels[c].size();
        PcmChannelsCount+=Common->SplittedChannels[c].size();
        for (size_t i=0; i<Common->SplittedChannels[c].size(); i++)
        {
            std::vector<File__Analyze*>& Parsers=Common->SplittedChannels[c][i]->Parsers;

            if (Parsers.size()!=1)
                continue;
            Fill(Parsers[0]);
            if (Parsers[0]->Count_Get(Stream_Audio))
            {
                PcmChannelsCount--;
                auto Metadata_Format=Parsers[0]->Get(Stream_Audio, 0, "Metadata_Format");
                if (Metadata_Format.rfind(__T("ADM"), 0)==0 || Metadata_Format.rfind(__T("S-ADM"), 0)==0)
                    AdmChannelsCount++;
            }
        }
    }
    if (PcmChannelsCount+AdmChannelsCount==TotalChannelsCount)
    {
        File_Pcm Parser;
        Parser.Codec=Codec;
        Parser.Sign=Sign;
        Parser.BitDepth=BitDepth;
        Parser.Channels=Channel_Total;
        Parser.SamplingRate=SamplingRate;
        Parser.Endianness=Endianness;
        Open_Buffer_Init(&Parser);
        Parser.Accept();
        Fill(&Parser);
        size_t Pos=Count_Get(Stream_Audio);
        Merge(Parser);

        for (size_t i=0; i<Common->SplittedChannels[0].size(); i++)
        {
            std::vector<File__Analyze*>& Parsers=Common->SplittedChannels[0][i]->Parsers;

            Merge(*Parsers[0], Stream_Audio, 0, 0);
        }
        return;
    }

    Fill(Stream_General, 0, General_Format, "ChannelSplitting");

    for (size_t i=0; i<Common->SplittedChannels[1].size(); i++)
    {
        std::vector<File__Analyze*>& Parsers=Common->SplittedChannels[1][i]->Parsers;

        if (Parsers.size()!=1)
            continue;
        Fill(Parsers[0]);

        if (!Parsers[0]->Status[IsAccepted])
        {
            for (int j=0; j<2; j++)
            {
                File_Pcm Parser;
                Parser.Codec=Codec;
                Parser.Sign=Sign;
                Parser.BitDepth=BitDepth;
                Parser.Channels=1;
                Parser.SamplingRate=SamplingRate;
                Parser.Endianness=Endianness;
                Open_Buffer_Init(&Parser);
                Parser.Accept();
                Fill(&Parser);
                size_t Pos=Count_Get(Stream_Audio);
                Merge(Parser);
                Fill(Stream_Audio, Pos, Audio_ID, i*2+1+j, true);
                Fill(Stream_Audio, Pos, Audio_MuxingMode, "Multiple");
            }
            continue;
        }

        size_t Pos=Count_Get(Stream_Audio);
        Merge(*Parsers[0]);
        for (size_t j=0; j<Parsers[0]->Count_Get(Stream_Audio); j++)
        {
            Ztring ID=Ztring().From_Number(i*2+1)+__T(" / ") + Ztring().From_Number(i*2+2);
            const Ztring& ID_Parser=Parsers[0]->Retrieve_Const(Stream_Audio, j, Audio_ID);
            if (!ID_Parser.empty())
            {
                ID+=__T('-');
                ID+=ID_Parser;
            }
            Fill(Stream_Audio, Pos+j, Audio_ID, ID, true);
            Ztring MuxingMode=__T("Multiple");
            const Ztring& MuxingMode_Parser=Parsers[0]->Retrieve(Stream_Audio, j, Audio_MuxingMode);
            if (!MuxingMode_Parser.empty())
            {
                MuxingMode+=__T(" / ");
                MuxingMode+=MuxingMode_Parser;
            }
            Fill(Stream_Audio, Pos+j, Audio_MuxingMode, MuxingMode, true);
        }
    }
}

//---------------------------------------------------------------------------
void File_ChannelSplitting::Streams_Finish()
{
    for (int c=0; c<2; c++)
        for (size_t i = 0; i<Common->SplittedChannels[c].size(); i++)
        {
            std::vector<File__Analyze*>& Parsers=Common->SplittedChannels[c][i]->Parsers;
        
            if (Parsers.size()!=1)
                continue;

            Finish(Parsers[0]);
        }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_ChannelSplitting::Read_Buffer_Init()
{
    if (Common==NULL)
    {
        if ((Channel_Total%2 && BitDepth==20) || (BitDepth!=16 && BitDepth!=20 && BitDepth!=24 && BitDepth!=32))
        {
            Reject();
            return; //Currently supporting only pairs in 20-bit mode
        }

        //Common
        Common=new common;
        for (int c=0; c<2; c++)
        {
            Common->SplittedChannels[c].resize(Channel_Total/(1+c));

            for (size_t i=0; i<Common->SplittedChannels[c].size(); i++)
            {
                Common->SplittedChannels[c][i]=new common::channel;
                std::vector<File__Analyze*>& Parsers=Common->SplittedChannels[c][i]->Parsers;

                //SMPTE ST 337
                {
                    File_SmpteSt0337* Parser=new File_SmpteSt0337;
                    Parser->BitDepth=BitDepth;
                    Parser->Endianness=Endianness;
                    Parser->Aligned=Aligned;
                    Parsers.push_back(Parser);
                }

                // PCM // TODO (currently no demux)

                //for all parsers
                for (size_t Pos=0; Pos<Parsers.size(); Pos++)
                {
                    #if MEDIAINFO_DEMUX
                        if (Config->Demux_Unpacketize_Get())
                        {
                            Parsers[Pos]->Demux_UnpacketizeContainer=true;
                            Parsers[Pos]->Demux_Level=2; //Container
                            Demux_Level=4; //Intermediate
                        }
                    #endif //MEDIAINFO_DEMUX
                    Element_Code=i+1;
                    Open_Buffer_Init(Parsers[Pos]);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
void File_ChannelSplitting::Read_Buffer_Continue()
{
    //Handling of multiple frames in one block
    if (Buffer_Size==0)
    {
        Read_Buffer_Continue_Parse();
        return;
    }

    if (!Common)
        return;

    //Size of buffer
    for (int c=0; c<2; c++)
        for (size_t i=0; i<Common->SplittedChannels[c].size(); i++)
        {
            common::channel* SplittedChannel=Common->SplittedChannels[c][i];

            if (Buffer_Size/Common->SplittedChannels[c].size()>SplittedChannel->Buffer_Size_Max)
                SplittedChannel->resize(Buffer_Size/Common->SplittedChannels[c].size());
            SplittedChannel->Buffer_Size=0;
        }

    //Copying to splitted channels
    Buffer_Offset=0;
    bool OddChannelCount=Channel_Total&1;
    size_t PairCount=Channel_Total/2;
    size_t BytesToCopy=Channel_Total*BitDepth/8;
    vector<common::channel*>& SplittedChannels0=Common->SplittedChannels[0];
    vector<common::channel*>& SplittedChannels1=Common->SplittedChannels[1];
    while (BytesToCopy<=Buffer_Size-Buffer_Offset)
    {
        for (size_t i=0; i<PairCount; i++)
        {
            common::channel* SplittedChannel00=SplittedChannels0[i*2];
            common::channel* SplittedChannel01=SplittedChannels0[i*2+1];
            common::channel* SplittedChannel1= SplittedChannels1[i];

            switch (BitDepth)
            {
                case 32:
                    // Copy 8 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
                case 24:
                    // Copy 6 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
                case 20:
                    // Copy 5 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    break;
                case 16:
                    // Copy 4 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel01->Buffer[SplittedChannel01->Buffer_Size++]=Buffer[Buffer_Offset];
                    SplittedChannel1->Buffer[SplittedChannel1->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
            }
        }
        if (OddChannelCount)
        {
            common::channel* SplittedChannel00=SplittedChannels0[Channel_Total-1];
            switch (BitDepth)
            {
                case 32:
                    // Copy 4 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
                case 24:
                    // Copy 3 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
                case 16:
                    // Copy 2 bytes
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    SplittedChannel00->Buffer[SplittedChannel00->Buffer_Size++]=Buffer[Buffer_Offset++];
                    break;
            }
        }
    }

    for (int c=0; c<2; c++)
        for (size_t i=0; i<Common->SplittedChannels[c].size(); i++)
        {
            common::channel* SplittedChannel=Common->SplittedChannels[c][i];

            for (size_t Pos=0; Pos<SplittedChannel->Parsers.size(); Pos++)
            {
                if (FrameInfo_Next.DTS!=(int64u)-1)
                    SplittedChannel->Parsers[Pos]->FrameInfo = FrameInfo_Next; //AES3 parse has its own buffer management
                else if (FrameInfo.DTS!=(int64u)-1)
                    SplittedChannel->Parsers[Pos]->FrameInfo = FrameInfo;
            }
        }
    FrameInfo=frame_info();
    AllFilled=true;
    AllFinished=true;
    SplittedChannels_c=0;
    SplittedChannels_i=0;
    size_t ContentSize=Buffer_Offset;
    Buffer_Offset=0;
    Skip_XX(ContentSize,                                        "Channel grouping data");
    Element_Offset=0;
    Read_Buffer_Continue_Parse();
    Buffer_Offset=ContentSize;
    Element_WaitForMoreData();
}

void File_ChannelSplitting::Read_Buffer_Continue_Parse()
{
    for (; SplittedChannels_c<2; SplittedChannels_c++)
    {
        for (; SplittedChannels_i<Common->SplittedChannels[SplittedChannels_c].size(); SplittedChannels_i++)
        {
            common::channel* SplittedChannel=Common->SplittedChannels[SplittedChannels_c][SplittedChannels_i];

            for (size_t Pos=0; Pos<SplittedChannel->Parsers.size(); Pos++)
            {
                Element_Code=SplittedChannels_i*2+1;
                #if MEDIAINFO_DEMUX
                    Demux(Buffer+Buffer_Offset, Buffer_Size-Buffer_Offset, ContentType_MainStream);
                #endif //MEDIAINFO_EVENTS
                Open_Buffer_Continue(SplittedChannel->Parsers[Pos], SplittedChannel->Buffer, SplittedChannel->Buffer_Size, false);

                //Multiple parsers
                if (SplittedChannel->Parsers.size()>1)
                {
                    //Test if valid
                    if (!Status[IsAccepted] && SplittedChannel->Parsers[SplittedChannel->Parsers.size()-1]->Frame_Count+1 >= ((File_Pcm*)SplittedChannel->Parsers[SplittedChannel->Parsers.size()-1])->Frame_Count_Valid)
                    {
                        //Rejecting here, else the parser may emit a frame before we detect that we want to reject the stream because there are no PCM stream (not handled here)
                        Reject();
                        return;
                    }
                    if (!SplittedChannel->Parsers[Pos]->Status[IsAccepted] && SplittedChannel->Parsers[Pos]->Status[IsFinished])
                    {
                        delete *(SplittedChannel->Parsers.begin()+Pos);
                        SplittedChannel->Parsers.erase(SplittedChannel->Parsers.begin()+Pos);
                        Pos--;
                    }
                    else if (SplittedChannel->Parsers.size()>1 && SplittedChannel->Parsers[Pos]->Status[IsAccepted])
                    {
                        if (Pos==SplittedChannel->Parsers.size()-1)
                            SplittedChannel->IsPcm=true; //Last parser is PCM
                        
                        File__Analyze* Parser=SplittedChannel->Parsers[Pos];
                        for (size_t Pos2=0; Pos2<SplittedChannel->Parsers.size(); Pos2++)
                        {
                            if (Pos2!=Pos)
                                delete *(SplittedChannel->Parsers.begin()+Pos2);
                        }
                        SplittedChannel->Parsers.clear();
                        SplittedChannel->Parsers.push_back(Parser);
                    }
                }
            }
            if (!Status[IsAccepted] && !SplittedChannel->IsPcm && SplittedChannel->Parsers.size()==1 && SplittedChannel->Parsers[0]->Status[IsAccepted])
                Accept();
            if (SplittedChannel->IsPcm || SplittedChannel->Parsers.size()!=1 || (!SplittedChannel->Parsers[0]->Status[IsFinished] && !SplittedChannel->Parsers[0]->Status[IsFilled]))
                AllFilled=false;
            if (SplittedChannel->IsPcm || SplittedChannel->Parsers.size()!=1 || !SplittedChannel->Parsers[0]->Status[IsFinished])
                AllFinished=false;

            #if MEDIAINFO_DEMUX
                if (Config->Demux_EventWasSent)
                {
                    SplittedChannels_i++;
                    return;
                }
            #endif //MEDIAINFO_DEMUX
        }
        SplittedChannels_i=0;
    }
    Frame_Count++;
    if (!Status[IsFilled] && AllFilled)
        Fill();
    if (!Status[IsFinished] && AllFinished)
        Finish();
}

//---------------------------------------------------------------------------
void File_ChannelSplitting::Read_Buffer_Unsynched()
{
    if (!Common)
        return;

    for (int c=0; c<2; c++)
        for (size_t i=0; i<Common->SplittedChannels[c].size(); i++)
        {
            common::channel* SplittedChannel=Common->SplittedChannels[c][i];

            for (size_t Pos=0; Pos<SplittedChannel->Parsers.size(); Pos++)
                if (SplittedChannel->Parsers[Pos])
                    SplittedChannel->Parsers[Pos]->Open_Buffer_Unsynch();
        }
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_SMPTEST0337_YES
