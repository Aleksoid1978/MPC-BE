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
#if defined(MEDIAINFO_DCP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_DcpCpl.h"
#include "MediaInfo/Multiple/File_DcpAm.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/XmlUtils.h"
#include "ZenLib/Dir.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "tinyxml2.h"
#include <list>
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DcpCpl::File_DcpCpl()
:File__Analyze()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DcpCpl;
        StreamIDs_Width[0]=sizeof(size_t)*2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX

    //Temp
    ReferenceFiles=NULL;
}

//---------------------------------------------------------------------------
File_DcpCpl::~File_DcpCpl()
{
    delete ReferenceFiles; //ReferenceFiles=NULL;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpCpl::Streams_Finish()
{
    if (ReferenceFiles==NULL)
        return;

    ReferenceFiles->ParseReferences();
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_DcpCpl::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    if (ReferenceFiles==NULL)
        return 0;

    return ReferenceFiles->Seek(Method, Value, ID);
}
#endif //MEDIAINFO_SEEK

//---------------------------------------------------------------------------

static bool IsSmpteSt2067_2(const char* Ns)
{
    return Ns &&
           (!strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-2/2013") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-2/XXXX")); //Some muxers use XXXX instead of year

}

//---------------------------------------------------------------------------

static bool IsSmpteSt2067_3(const char* Ns)
{
    return Ns &&
           (!strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-3/2013") ||
            !strcmp(Ns, "http://www.smpte-ra.org/schemas/2067-3/XXXX")); //Some muxers use XXXX instead of year

}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DcpCpl::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    XMLElement* Root=document.FirstChildElement();
    const char *NameSpace;
    if (!Root || strcmp(LocalName(Root, NameSpace), "CompositionPlaylist"))
    {
        Reject("DcpCpl");
        return false;
    }

    bool IsDcp=false, IsImf=false;
    if (!strcmp(NameSpace, "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#") ||
        !strcmp(NameSpace, "http://www.smpte-ra.org/schemas/429-7/2006/CPL"))
    {
        IsDcp=true;
    }
    else if (IsSmpteSt2067_3(NameSpace))
    {
        IsImf=true;
    }
    else
    {
        Reject("DcpCpl");
        return false;
    }

    Accept("DcpCpl");
    Fill(Stream_General, 0, General_Format, IsDcp?"DCP CPL":"IMF CPL");
    Config->File_ID_OnlyRoot_Set(false);

    ReferenceFiles=new File__ReferenceFilesHelper(this, Config);

    //Parsing main elements
    for (XMLElement* CompositionPlaylist_Item=Root->FirstChildElement(); CompositionPlaylist_Item; CompositionPlaylist_Item=CompositionPlaylist_Item->NextSiblingElement())
    {
        //CompositionTimecode
        if (IsImf && MatchQName(CompositionPlaylist_Item, "CompositionTimecode", NameSpace))
        {
            sequence* Sequence=new sequence;
            Sequence->StreamKind=Stream_Other;
            Sequence->Infos["Type"]=__T("Time code");
            Sequence->Infos["Format"]=__T("CPL TC");
            Sequence->Infos["TimeCode_Striped"]=__T("Yes");
            bool IsDropFrame=false;

            for (XMLElement* CompositionTimecode_Item=CompositionPlaylist_Item->FirstChildElement(); CompositionTimecode_Item; CompositionTimecode_Item=CompositionTimecode_Item->NextSiblingElement())
            {
                const char* Text=CompositionTimecode_Item->GetText();
                if (!Text)
                    continue;
                const char *CtItemNs, *CtItemName = LocalName(CompositionTimecode_Item, CtItemNs);
                if (!CtItemNs || strcmp(CtItemNs, NameSpace))
                    continue; // item has wrong namespace

                //TimecodeDropFrame
                if (!strcmp(CtItemName, "TimecodeDropFrame"))
                {
                    if (strcmp(Text, "") && strcmp(Text, "0"))
                        IsDropFrame=true;
                }

                //TimecodeRate
                if (!strcmp(CtItemName, "TimecodeRate"))
                    Sequence->Infos["FrameRate"].From_UTF8(Text);

                //TimecodeStartAddress
                if (!strcmp(CtItemName, "TimecodeStartAddress"))
                    Sequence->Infos["TimeCode_FirstFrame"].From_UTF8(Text);
            }

            //Adaptation
            if (IsDropFrame)
            {
                std::map<string, Ztring>::iterator Info=Sequence->Infos.find("TimeCode_FirstFrame");
                if (Info!=Sequence->Infos.end() && Info->second.size()>=11 && Info->second[8]!=__T(';'))
                    Info->second[8]=__T(';');
            }

            Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
            ReferenceFiles->AddSequence(Sequence);

            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_ID, Sequence->StreamID);
            for (std::map<string, Ztring>::iterator Info=Sequence->Infos.begin(); Info!=Sequence->Infos.end(); ++Info)
                Fill(Stream_Other, StreamPos_Last, Info->first.c_str(), Info->second);
        }

        //ReelList / SegmentList
        if (MatchQName(CompositionPlaylist_Item, IsDcp?"ReelList":"SegmentList", NameSpace))
        {
            for (XMLElement* ReelList_Item=CompositionPlaylist_Item->FirstChildElement(); ReelList_Item; ReelList_Item=ReelList_Item->NextSiblingElement())
            {
                //Reel
                if (MatchQName(ReelList_Item, IsDcp?"Reel":"Segment", NameSpace))
                {
                    for (XMLElement* Reel_Item=ReelList_Item->FirstChildElement(); Reel_Item; Reel_Item=Reel_Item->NextSiblingElement())
                    {
                        //AssetList
                        if (MatchQName(Reel_Item, IsDcp?"AssetList":"SequenceList", NameSpace))
                        {
                            for (XMLElement* AssetList_Item=Reel_Item->FirstChildElement(); AssetList_Item; AssetList_Item=AssetList_Item->NextSiblingElement())
                            {
                                const char *AlItemNs, *AlItemName = LocalName(AssetList_Item, AlItemNs);
                                if (!AlItemNs)
                                    continue;
                                //File
                                //if ((IsDcp && (!strcmp(AssetList_Item->Value(), "MainPicture") || !strcmp(AssetList_Item->Value(), "MainSound")))
                                // || (IsImf && (!strcmp(AssetList_Item->Value(), "cc:MainImageSequence") || !strcmp(AssetList_Item->Value(), "cc:MainImage"))))
                                if (strcmp(AlItemName, "MarkerSequence")) //Ignoring MarkerSequence for the moment. TODO: check what to do with MarkerSequence
                                {
                                    sequence* Sequence=new sequence;
                                    Ztring Asset_Id;

                                    if (IsDcp && !strcmp(NameSpace, AlItemNs))
                                    {
                                        if (!strcmp(AlItemName, "MainPicture"))
                                            Sequence->StreamKind=Stream_Video;
                                        else if (!strcmp(AlItemName, "MainSound"))
                                            Sequence->StreamKind=Stream_Audio;
                                    }
                                    else if (IsImf && IsSmpteSt2067_2(AlItemNs))
                                    {
                                        if (!strcmp(AlItemName, "MainImageSequence"))
                                            Sequence->StreamKind=Stream_Video;
                                        else if (!strcmp(AlItemName, "MainAudioSequence"))
                                            Sequence->StreamKind=Stream_Audio;
                                    }

                                    for (XMLElement* File_Item=AssetList_Item->FirstChildElement(); File_Item; File_Item=File_Item->NextSiblingElement())
                                    {
                                        //Id
                                        if (MatchQName(File_Item, "Id", NameSpace) && Asset_Id.empty())
                                            Asset_Id.From_UTF8(File_Item->GetText());

                                        //ResourceList
                                        if (IsImf && MatchQName(File_Item, "ResourceList", NameSpace))
                                        {
                                            for (XMLElement* ResourceList_Item=File_Item->FirstChildElement(); ResourceList_Item; ResourceList_Item=ResourceList_Item->NextSiblingElement())
                                            {
                                                //Resource
                                                if (MatchQName(ResourceList_Item, "Resource", NameSpace))
                                                {
                                                    Ztring Resource_Id;

                                                    resource* Resource=new resource;
                                                    for (XMLElement* Resource_Item=ResourceList_Item->FirstChildElement(); Resource_Item; Resource_Item=Resource_Item->NextSiblingElement())
                                                    {
                                                        const char* ResText=Resource_Item->GetText();
                                                        if (!ResText)
                                                            continue;
                                                        const char *ResItemNs, *ResItemName = LocalName(Resource_Item, ResItemNs);
                                                        if (!ResItemNs || strcmp(ResItemNs, NameSpace))
                                                            continue; // item has wrong namespace

                                                        //EditRate
                                                        if (!strcmp(ResItemName, "EditRate"))
                                                        {
                                                            Resource->EditRate=atof(ResText);
                                                            const char* EditRate2=strchr(ResText, ' ');
                                                            if (EditRate2!=NULL)
                                                            {
                                                                float64 EditRate2f=atof(EditRate2);
                                                                if (EditRate2f)
                                                                    Resource->EditRate/=EditRate2f;
                                                            }
                                                        }

                                                        //EntryPoint
                                                        if (!strcmp(ResItemName, "EntryPoint"))
                                                        {
                                                            Resource->IgnoreEditsBefore=atoi(ResText);
                                                            if (Resource->IgnoreEditsAfter!=(int64u)-1)
                                                                Resource->IgnoreEditsAfter+=Resource->IgnoreEditsBefore;
                                                        }

                                                        //Id
                                                        if (!strcmp(ResItemName, "Id") && Resource_Id.empty())
                                                            Resource_Id.From_UTF8(ResText);

                                                        //SourceDuration
                                                        if (!strcmp(ResItemName, "SourceDuration"))
                                                            Resource->IgnoreEditsAfter=Resource->IgnoreEditsBefore+atoi(ResText);

                                                        //TrackFileId
                                                        if (!strcmp(ResItemName, "TrackFileId"))
                                                            Resource->FileNames.push_back(Ztring().From_UTF8(ResText));
                                                    }

                                                    if (Resource->FileNames.empty())
                                                        Resource->FileNames.push_back(Resource_Id);
                                                    Sequence->AddResource(Resource);
                                                }
                                            }
                                        }
                                    }

                                    if (Sequence->Resources.empty())
                                    {
                                        resource* Resource=new resource;
                                        Resource->FileNames.push_back(Asset_Id);
                                        Sequence->AddResource(Resource);
                                    }
                                    Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                                    ReferenceFiles->AddSequence(Sequence);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Element_Offset=File_Size;

    //Getting files names
    FileName Directory(File_Name);
    Ztring DirPath = Directory.Path_Get();
    if (!DirPath.empty())
        DirPath += PathSeparator;
    Ztring Assetmap_FileName=DirPath+__T("ASSETMAP.xml");
    bool IsOk=false;
    if (File::Exists(Assetmap_FileName))
        IsOk=true;
    else
    {
        Assetmap_FileName.resize(Assetmap_FileName.size()-4); //Old fashion, without ".xml"
        if (File::Exists(Assetmap_FileName))
            IsOk=true;
    }
    if (IsOk)
    {
        MediaInfo_Internal MI;
        MI.Option(__T("File_KeepInfo"), __T("1"));
        Ztring ParseSpeed_Save=MI.Option(__T("ParseSpeed_Get"), __T(""));
        Ztring Demux_Save=MI.Option(__T("Demux_Get"), __T(""));
        MI.Option(__T("ParseSpeed"), __T("0"));
        MI.Option(__T("Demux"), Ztring());
        MI.Option(__T("File_IsReferenced"), __T("1"));
        size_t MiOpenResult=MI.Open(Assetmap_FileName);
        MI.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
        MI.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
        if (MiOpenResult
            && (MI.Get(Stream_General, 0, General_Format)==__T("DCP AM")
            || MI.Get(Stream_General, 0, General_Format)==__T("IMF AM")))
        {
            MergeFromAm(((File_DcpAm*)MI.Info)->Streams);
        }
    }

    ReferenceFiles->FilesForStorage=true;

    //All should be OK...
    return true;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpCpl::MergeFromAm (File_DcpPkl::streams &StreamsToMerge)
{
    for (File_DcpPkl::streams::iterator StreamToMerge=StreamsToMerge.begin(); StreamToMerge!=StreamsToMerge.end(); ++StreamToMerge)
        if (!StreamToMerge->ChunkList.empty()) // Note: ChunkLists with more than 1 file are not yet supported)
            ReferenceFiles->UpdateFileName(Ztring().From_UTF8(StreamToMerge->Id), Ztring().From_UTF8(StreamToMerge->ChunkList[0].Path));
}

} //NameSpace

#endif //MEDIAINFO_DCP_YES
