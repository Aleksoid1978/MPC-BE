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
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfoList_Internal.h"
#include "MediaInfo/MediaInfo_Config.h"
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_FILE_YES)
#if defined(MEDIAINFO_DIRECTORY_YES)
#include "ZenLib/Dir.h"
#endif //defined(MEDIAINFO_DIRECTORY_YES)
#include "MediaInfo/Reader/Reader_Directory.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include <algorithm>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
//---------------------------------------------------------------------------

//***************************************************************************
// Gestion de la classe
//***************************************************************************

//---------------------------------------------------------------------------
//Constructeurs
MediaInfoList_Internal::MediaInfoList_Internal(size_t Count_Init)
: Thread()
{
    CriticalSectionLocker CSL(CS);

    //Initialisation
    Info.reserve(Count_Init);
    for (size_t Pos=0; Pos<Info.size(); Pos++)
        Info[Pos]=NULL;
    ToParse_AlreadyDone=0;
    ToParse_Total=0;
    CountValid=0;

    //Threading
    BlockMethod=0;
    State=0;
    IsInThread=false;
}

//---------------------------------------------------------------------------
//Destructeur
MediaInfoList_Internal::~MediaInfoList_Internal()
{
    Close();
}

//***************************************************************************
// Fichiers
//***************************************************************************

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Open(const String &File_Name, const fileoptions_t Options)
{
    //Option FileOption_Close
    if (Options & FileOption_CloseAll)
        Close(All);

    //Option Recursive
    //TODO

    //Get all filenames
    ZtringList List;
    size_t Pos=File_Name.find(__T(':'));
    if (Pos!=string::npos && Pos!=1)
        List.push_back(File_Name);
    #if defined(MEDIAINFO_FILE_YES)
    else if (File::Exists(File_Name))
        List.push_back(File_Name);
    #endif //defined(MEDIAINFO_FILE_YES)
    #if defined(MEDIAINFO_DIRECTORY_YES)
    else
    {
        List=Dir::GetAllFileNames(File_Name, (Options&FileOption_NoRecursive)?Dir::Include_Files:((Dir::dirlist_t)(Dir::Include_Files|Dir::Parse_SubDirs)));
        sort(List.begin(), List.end());

        #if MEDIAINFO_ADVANCED
            if (MediaInfoLib::Config.ParseOnlyKnownExtensions_IsSet())
            {
                set<Ztring> ExtensionsList=MediaInfoLib::Config.ParseOnlyKnownExtensions_GetList_Set();
                bool AcceptNoExtension=ExtensionsList.find(Ztring())!=ExtensionsList.end();
                for (size_t i=List.size()-1; i!=(size_t)-1; i--)
                {
                    const Ztring& Name=List[i];
                    size_t Extension_Pos=Name.rfind(__T('.'));
                    if (Extension_Pos!=string::npos && ExtensionsList.find(Name.substr(Extension_Pos+1))==ExtensionsList.end()
                     || Extension_Pos==string::npos && !AcceptNoExtension)
                            List.erase(List.begin()+i);
                }
            }
        #endif //MEDIAINFO_ADVANCED
    }
    #endif //defined(MEDIAINFO_DIRECTORY_YES)

    #if defined(MEDIAINFO_DIRECTORY_YES)
        Reader_Directory().Directory_Cleanup(List);
    #endif //defined(MEDIAINFO_DIRECTORY_YES)

    //Registering files
    {
    CriticalSectionLocker CSL(CS);
    if (ToParse.empty())
        CountValid=0;
    for (ZtringList::iterator L=List.begin(); L!=List.end(); ++L)
        ToParse.push(*L);
    ToParse_Total+=List.size();
    if (ToParse_Total)
        State=ToParse_AlreadyDone*10000/ToParse_Total;
    else
        State=10000;
    }

    //Parsing
    if (BlockMethod==1)
    {
        CriticalSectionLocker CSL(CS);
        if (!IsRunning()) //If already created, the routine will read the new files
        {
            RunAgain();
            IsInThread=true;
        }
        return 0;
    }
    else
    {
        Entry(); //Normal parsing
        return Count_Get();
    }
}

#if defined(MEDIAINFO_FILE_YES)
static size_t RemoveFilesFromList(std::queue<String>& ToParse, Ztring CompleteName_Begin, const Ztring &CompleteName_Last)
{
    size_t Removed=0;
    size_t Pos=0;
    for (; Pos<CompleteName_Begin.size(); Pos++)
    {
        if (Pos>=CompleteName_Last.size())
            break;
        if (CompleteName_Begin[Pos]!=CompleteName_Last[Pos])
            break;
    }
    if (Pos<CompleteName_Begin.size())
    {
        CompleteName_Begin.resize(Pos);
        while (!ToParse.empty() && ToParse.front().find(CompleteName_Begin)==0)
        {
            ToParse.pop();
            Removed++;
        }
    }
    return Removed;
}
#endif //defined(MEDIAINFO_FILE_YES)

void MediaInfoList_Internal::Entry()
{
    if (ToParse_Total==0)
        return;

    for (;;)
    {
        CS.Enter();
        if (!ToParse.empty())
        {
            Ztring FileName=ToParse.front();
            ToParse.pop();
            #if defined(MEDIAINFO_FILE_YES)
                bool Skip=false;
                for (size_t i=0; i<ToParse_ToIgnore.size(); i++)
                {
                    if (ToParse_ToIgnore[i]==FileName)
                    {
                        ToParse_ToIgnore.erase(ToParse_ToIgnore.begin()+i);
                        ToParse_AlreadyDone++;
                        Skip=true;
                        continue;
                    }
                }
                if (Skip)
                    continue;
            #endif //defined(MEDIAINFO_FILE_YES)
            MediaInfo_Internal* MI=new MediaInfo_Internal();
            for (std::map<String, String>::iterator Config_MediaInfo_Item=Config_MediaInfo_Items.begin(); Config_MediaInfo_Item!=Config_MediaInfo_Items.end(); ++Config_MediaInfo_Item)
                MI->Option(Config_MediaInfo_Item->first, Config_MediaInfo_Item->second);
            if (BlockMethod==1)
                MI->Option(__T("Thread"), __T("1"));
            Info.push_back(MI);
            CS.Leave();
            MI->Open(FileName);

            if (BlockMethod==1)
            {
                while (MI->State_Get()<10000)
                {
                    size_t A=MI->State_Get();
                    CS.Enter();
                    State=(ToParse_AlreadyDone*10000+A)/ToParse_Total;
                    CS.Leave();
                    if (IsTerminating())
                    {
                        break;
                    }
                    Yield();
                }
            }
            CS.Enter();
            ToParse_AlreadyDone++;

            #if defined(MEDIAINFO_FILE_YES)
                //Removing sequences of files from the list
                if (!MI->Get(Stream_General, 0, General_CompleteName_Last).empty())
                    ToParse_AlreadyDone+=RemoveFilesFromList(ToParse, MI->Get(Stream_General, 0, General_CompleteName),
                                                                      MI->Get(Stream_General, 0, General_CompleteName_Last));
                if (MI->Config.File_TestDirectory_Get() && MI->Get(Stream_General, 0, General_Format)==__T("Directory"))
                {
                    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                        for (size_t StreamPos=0; StreamPos<MI->Count_Get((stream_t)StreamKind); StreamPos++)
                        {
                            if (!MI->Get((stream_t)StreamKind, StreamPos, __T("Source_Last")).empty())
                                ToParse_AlreadyDone+=RemoveFilesFromList(ToParse, MI->Get(Stream_General, 0, General_CompleteName)+MI->Get((stream_t)StreamKind, StreamPos, __T("Source")),
                                                                                  MI->Get(Stream_General, 0, General_CompleteName)+MI->Get((stream_t)StreamKind, StreamPos, __T("Source_Last")));
                            else
                            {
                                Ztring Source=MI->Get((stream_t)StreamKind, StreamPos, __T("Source"));
                                if (!Source.empty())
                                {
                                    Ztring Dir=MI->Get(Stream_General, 0, General_CompleteName);
                                    if (!Dir.empty() && Dir[Dir.size()-1]!=__T('/') && Dir[Dir.size()-1]!=__T('\\'))
                                    {
                                        size_t Separator_Pos=Dir.find_last_of(__T("\\/"));
                                        if (Separator_Pos!=string::npos)
                                            Dir.resize(Separator_Pos+1);
                                        else
                                            Dir.clear();
                                    }
                                    size_t i;
                                    if (PathSeparator!=__T('/'))
                                        while ((i=Source.find(__T('/')))!=string::npos)
                                            Source[i]=PathSeparator;
                                    if (PathSeparator!=__T('\\'))
                                        while ((i=Source.find(__T('\\')))!=string::npos)
                                            Source[i]=PathSeparator;
                                    Source=Dir+Source;
                                    i=0;
                                    for (; i<Info.size(); i++)
                                        if (Info[i]->Get(Stream_General, 0, General_CompleteName)==Source)
                                        {
                                            delete Info[i];
                                            Info.erase(Info.begin()+i);
                                        }
                                    if (i>=Info.size())
                                        ToParse_ToIgnore.push_back(Source);
                                }
                            }
                        }
                }
            #endif //defined(MEDIAINFO_FILE_YES)
        }

        State=ToParse_AlreadyDone*10000/ToParse_Total;
        //if ((ToParse_AlreadyDone%10)==0)
        //    printf("%f done (%i/%i %s)\n", ((float)State)/100, (int)ToParse_AlreadyDone, (int)ToParse_Total, Ztring(ToParse.front()).To_UTF8().c_str());
        if (IsTerminating() || State==10000)
        {
            CS.Leave();
            break;
        }
        CS.Leave();
        Yield();
    }
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Open_Buffer_Init (int64u File_Size_, int64u File_Offset_)
{
    MediaInfo_Internal* MI=new MediaInfo_Internal();
    MI->Open_Buffer_Init(File_Size_, File_Offset_);

    CriticalSectionLocker CSL(CS);
    size_t Pos=Info.size();
    Info.push_back(MI);
    return Pos;
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Open_Buffer_Continue (size_t FilePos, const int8u* ToAdd, size_t ToAdd_Size)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos>=Info.size() || Info[FilePos]==NULL)
        return 0;

    return Info[FilePos]->Open_Buffer_Continue(ToAdd, ToAdd_Size).to_ulong();
}

//---------------------------------------------------------------------------
int64u MediaInfoList_Internal::Open_Buffer_Continue_GoTo_Get (size_t FilePos)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos>=Info.size() || Info[FilePos]==NULL)
        return (int64u)-1;

    return Info[FilePos]->Open_Buffer_Continue_GoTo_Get();
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Open_Buffer_Finalize (size_t FilePos)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos>=Info.size() || Info[FilePos]==NULL)
        return 0;

    return Info[FilePos]->Open_Buffer_Finalize();
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Save(size_t)
{
    CriticalSectionLocker CSL(CS);
    return 0; //Not yet implemented
}

//---------------------------------------------------------------------------
void MediaInfoList_Internal::Close(size_t FilePos)
{
    if (IsRunning())
    {
        RequestTerminate();
        while(IsExited())
            Yield();
    }

    CriticalSectionLocker CSL(CS);
    if (FilePos==Unlimited)
    {
        for (size_t Pos=0; Pos<Info.size(); Pos++)
        {
            delete Info[Pos]; Info[Pos]=NULL;
        }
        Info.clear();
    }
    else if (FilePos<Info.size())
    {
        delete Info[FilePos]; Info[FilePos]=NULL;
        Info.erase(Info.begin()+FilePos);
    }

    ToParse_AlreadyDone=0;
    ToParse_Total=0;
}

//***************************************************************************
// Get File info
//***************************************************************************

//---------------------------------------------------------------------------
String MediaInfoList_Internal::Inform(size_t FilePos, size_t)
{
    if (FilePos==Error)
    {
        return MediaInfo_Internal::Inform(Info);
    }

    CriticalSectionLocker CSL(CS);

    if (FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return MediaInfoLib::Config.EmptyString_Get();

    return MediaInfo_Internal::Inform(Info[FilePos]);
}

//---------------------------------------------------------------------------
String MediaInfoList_Internal::Get(size_t FilePos, stream_t KindOfStream, size_t StreamNumber, size_t Parameter, info_t KindOfInfo)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos==Error || FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return MediaInfoLib::Config.EmptyString_Get();

    return Info[FilePos]->Get(KindOfStream, StreamNumber, Parameter, KindOfInfo);
}

//---------------------------------------------------------------------------
String MediaInfoList_Internal::Get(size_t FilePos, stream_t KindOfStream, size_t StreamNumber, const String &Parameter, info_t KindOfInfo, info_t KindOfSearch)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos==Error || FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return MediaInfoLib::Config.EmptyString_Get();

    return Info[FilePos]->Get(KindOfStream, StreamNumber, Parameter, KindOfInfo, KindOfSearch);
}

//***************************************************************************
// Set File info
//***************************************************************************

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Set(const String &ToSet, size_t FilePos, stream_t StreamKind, size_t StreamNumber, size_t Parameter, const String &OldValue)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos==(size_t)-1)
        FilePos=0; //TODO : average

    if (FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return 0;

    return Info[FilePos]->Set(ToSet, StreamKind, StreamNumber, Parameter, OldValue);
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Set(const String &ToSet, size_t FilePos, stream_t StreamKind, size_t StreamNumber, const String &Parameter, const String &OldValue)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos==(size_t)-1)
        FilePos=0; //TODO : average

    if (FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return 0;

    return Info[FilePos]->Set(ToSet, StreamKind, StreamNumber, Parameter, OldValue);
}

//***************************************************************************
// Output buffer
//***************************************************************************

/*
//---------------------------------------------------------------------------
char* MediaInfoList_Internal::Output_Buffer_Get (size_t FilePos, size_t &Output_Buffer_Size)
{
    if (FilePos==(size_t)-1)
        FilePos=0; //TODO : average

    if (FilePos>=Info.size() || Info[FilePos]==NULL || Info[FilePos]->Count_Get(Stream_General)==0)
        return 0;

    return Info[FilePos]->Output_Buffer_Get(Output_Buffer_Size);
}
*/

//***************************************************************************
// Information
//***************************************************************************

//---------------------------------------------------------------------------
String MediaInfoList_Internal::Option (const String &Option, const String &Value)
{
    CriticalSectionLocker CSL(CS);
    Ztring OptionLower=Option; OptionLower.MakeLowerCase();
    if (Option.empty())
        return String();
    else if (OptionLower==__T("manguage_update"))
    {
        //Special case : Language_Update must update all MediaInfo classes
        for (unsigned int Pos=0; Pos<Info.size(); Pos++)
            if (Info[Pos])
                Info[Pos]->Option(__T("language_update"), Value);

        return __T("");
    }
    else if (OptionLower==__T("create_dummy"))
    {
        Info.resize(Info.size()+1);
        Info[Info.size()-1]=new MediaInfo_Internal();
        Info[Info.size()-1]->Option(Option, Value);
        return __T("");
    }
    else if (OptionLower==__T("thread"))
    {
        BlockMethod=1;
        return __T("");
    }
    #if MEDIAINFO_ADVANCED
        else if (OptionLower.find(__T("file_inform_stringpointer")) == 0 && Info.size() == 1)
            return Info[0]->Option(Option, Value);
    #endif //MEDIAINFO_ADVANCED
    else if (OptionLower.find(__T("reset"))==0)
    {
        Config_MediaInfo_Items.clear();
        MediaInfoLib::Config.Init(true);
        return Ztring();
    }
    else if (OptionLower.find(__T("file_"))==0)
    {
        Config_MediaInfo_Items[Option]=Value;
        return __T("");
    }
    else
        return MediaInfo::Option_Static(Option, Value);
}

//---------------------------------------------------------------------------
String MediaInfoList_Internal::Option_Static (const String &Option, const String &Value)
{
    return MediaInfo::Option_Static(Option, Value);
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::State_Get()
{
    CriticalSectionLocker CSL(CS);
    if (State==10000)
    {
        //Pause();
        IsInThread=false;
    }

    if (!Info.empty())
    {
        State=0;
        for (size_t Pos=0; Pos<Info.size(); Pos++)
            State+=Info[Pos]->State_Get();
        State/=Info.size()+ToParse.size();
    }

    return State;
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Count_Get (size_t FilePos, stream_t StreamKind, size_t StreamNumber)
{
    CriticalSectionLocker CSL(CS);
    if (FilePos>=Info.size() || Info[FilePos]==NULL)
        return 0;

    return Info[FilePos]->Count_Get(StreamKind, StreamNumber);
}

//---------------------------------------------------------------------------
size_t MediaInfoList_Internal::Count_Get()
{
    CriticalSectionLocker CSL(CS);
    return Info.size();
}

} //NameSpace
