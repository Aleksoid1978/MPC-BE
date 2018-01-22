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
#include "MediaInfo/Multiple/File_DcpPkl.h"
#include "MediaInfo/Multiple/File_DcpAm.h"
#include "MediaInfo/Multiple/File_DcpCpl.h"
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/XmlUtils.h"
#if defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_REFERENCES_YES)
#include "ZenLib/FileName.h"
#include "tinyxml2.h"
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DcpPkl::File_DcpPkl()
:File__Analyze(), File__HasReferences()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_DcpPkl;
        StreamIDs_Width[0]=sizeof(size_t)*2;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_EventWasSent_Accept_Specific=true;
    #endif //MEDIAINFO_DEMUX
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpPkl::Streams_Finish()
{
    if (Config->File_IsReferenced_Get())
        return;

    ReferenceFiles_Finish();

    // Detection of IMF CPL
    bool IsImf=false;
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            if (Retrieve((stream_t)StreamKind, StreamPos, "MuxingMode").find(__T("IMF CPL"))==0)
                IsImf=true;
    if (IsImf)
    {
        Fill(Stream_General, 0, General_Format, "IMF PKL", Unlimited, true, true);
        Clear(Stream_General, 0, General_Format_Version);
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_DcpPkl::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    if (Config->File_IsReferenced_Get())
        return 0;

    return ReferenceFiles_Seek(Method, Value, ID);
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DcpPkl::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    XMLElement* PackingList=document.FirstChildElement();
    const char *NameSpace;
    if (!PackingList || strcmp(LocalName(PackingList, NameSpace), "PackingList") || !NameSpace)
    {
        Reject("DcpPkl");
        return false;
    }
    if (strcmp(NameSpace, "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#") &&
        strcmp(NameSpace, "http://www.smpte-ra.org/schemas/429-8/2007/PKL") &&
        strcmp(NameSpace, "http://www.smpte-ra.org/schemas/2067-2/2016/PKL"))
    {
        Reject("DcpPkl");
        return false;
    }

    Accept("DcpPkl");
    Fill(Stream_General, 0, General_Format, "DCP PKL");
    #if defined(MEDIAINFO_REFERENCES_YES)
    Config->File_ID_OnlyRoot_Set(false);

    //Parsing main elements
    for (XMLElement* PackingList_Item=PackingList->FirstChildElement(); PackingList_Item; PackingList_Item=PackingList_Item->NextSiblingElement())
    {
        //AssetList
        if (MatchQName(PackingList_Item, "AssetList", NameSpace))
        {
            for (XMLElement* AssetList_Item=PackingList_Item->FirstChildElement(); AssetList_Item; AssetList_Item=AssetList_Item->NextSiblingElement())
            {
                //Asset
                if (MatchQName(AssetList_Item, "Asset", NameSpace))
                {
                    stream Stream;

                    for (XMLElement* File_Item=AssetList_Item->FirstChildElement(); File_Item; File_Item=File_Item->NextSiblingElement())
                    {
                        const char* Text=File_Item->GetText();
                        const char *FileItemNs, *FileItemName = LocalName(File_Item, FileItemNs);
                        if (!FileItemNs || strcmp(FileItemNs, NameSpace))
                            continue; // item has wrong namespace
                        //AnnotationText
                        if (Text && !strcmp(FileItemName, "AnnotationText"))
                            Stream.AnnotationText=Text;

                        //Id
                        if (Text && !strcmp(FileItemName, "Id"))
                            Stream.Id=Text;

                        //OriginalFileName
                        if (Text && !strcmp(FileItemName, "OriginalFileName"))
                            Stream.OriginalFileName=Text;

                        //Type
                        if (!strcmp(FileItemName, "Type"))
                        {
                            if (!Text)
                                Stream.StreamKind=Stream_Other;
                            else if (!strcmp(Text, "application/x-smpte-mxf;asdcpKind=Picture"))
                                Stream.StreamKind=Stream_Video;
                            else if (!strcmp(Text, "application/x-smpte-mxf;asdcpKind=Sound"))
                                Stream.StreamKind=Stream_Audio;
                            else if (!strcmp(Text, "text/xml") || !strcmp(Text, "text/xml;asdcpKind=CPL"))
                                Stream.StreamKind=(stream_t)(Stream_Max+1); // Means CPL
                            else
                                Stream.StreamKind=Stream_Other;
                        }
                    }

                    Streams.push_back(Stream);
                }
            }
        }
    }

    //Merging with Assetmap
    if (!Config->File_IsReferenced_Get())
    {
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
                MergeFromAm(((File_DcpPkl*)MI.Info)->Streams);
            }
        }
    }

    //Creating the playlist
    if (!Config->File_IsReferenced_Get())
    {
        ReferenceFiles_Accept(this, Config);

        for (File_DcpPkl::streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
            if (Stream->StreamKind==(stream_t)(Stream_Max+1) && Stream->ChunkList.size()==1) // Means CPL
            {
                sequence* Sequence=new sequence;
                Sequence->FileNames.push_back(Ztring().From_UTF8(Stream->ChunkList[0].Path));

                Sequence->StreamID=ReferenceFiles->Sequences_Size()+1;
                ReferenceFiles->AddSequence(Sequence);
            }

        ReferenceFiles->FilesForStorage=true;
    }
    #endif //MEDIAINFO_REFERENCES_YES

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
void File_DcpPkl::MergeFromAm (File_DcpPkl::streams &StreamsToMerge)
{
    for (File_DcpPkl::streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
    {
        for (File_DcpPkl::streams::iterator StreamToMerge=StreamsToMerge.begin(); StreamToMerge!=StreamsToMerge.end(); ++StreamToMerge)
            if (StreamToMerge->Id==Stream->Id)
            {
                stream_t StreamKind=Stream->StreamKind; //Keeping StreamKind from PKL
                *Stream=*StreamToMerge;
                Stream->StreamKind=StreamKind; //Keeping StreamKind from PKL
            }
    }
}

} //NameSpace

#endif //MEDIAINFO_DCP_YES

