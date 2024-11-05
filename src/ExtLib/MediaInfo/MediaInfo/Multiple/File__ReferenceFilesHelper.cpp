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
#if defined(MEDIAINFO_REFERENCES_YES) || defined(MEDIAINFO_JSON_YES)
//---------------------------------------------------------------------------

#include <string>

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static unsigned char Hex2Char (unsigned char Value)
{
         if (Value<=0x9)
        Value+='0';
    else if (Value<=0xF)
        Value+='A'-10;
    else
        Value =0;
    return Value;
}

//---------------------------------------------------------------------------
std::string URL_Encoded_Encode(const std::string& URL)
{
    std::string Result;
    for (std::string::size_type Pos=0; Pos<URL.size(); Pos++)
    {
        auto Char=URL[Pos];
        if (Char<=0x2C
         || Char==0x2E
         || Char==0x2F
         || Char==0x3A
         || Char==0x3B
         || Char==0x3D
         || Char==0x3F
         || Char==0x40
         || Char==0x5B
         || Char==0x5D
//#if defined(__APPLE__) && defined(__MACH__) // URL is rejected by API on macOS, not considered as URL by Thunderbird
         || Char=='{'
         || Char=='}'
         || Char=='\\'
//#endif
        )
        {
            Result+='%';
            Result+=Hex2Char(Char>>4);
            Result+=Hex2Char(Char&0xF);
        }
        else
            Result+=Char;
    }
    return Result;
}

}

#endif

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_REFERENCES_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "ZenLib/File.h"
#include "ZenLib/FileName.h"
#include "ZenLib/Format/Http/Http_Utils.h"
#include <set>
#include <algorithm>
#include <cfloat>
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events_Internal.h"
    #include "MediaInfo/MediaInfo_Config_PerPackage.h"
#endif //MEDIAINFO_EVENTS
#if MEDIAINFO_AES
    #include "ThirdParty/base64/base64.h"
#endif //MEDIAINFO_AES
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{


//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static unsigned char Char2Hex (unsigned char Char)
{
         if (Char<='9' && Char>='0')
        Char-='0';
    else if (Char<='f' && Char>='a')
        Char-='a'-10;
    else if (Char<='F' && Char>='A')
        Char-='A'-10;
    else
        Char =0;
    return Char;
}

//---------------------------------------------------------------------------
std::wstring URL_Encoded_Decode (const std::wstring& URL)
{
    wstring Result;
    wstring::size_type Pos;
    for (Pos=0; Pos<URL.size(); Pos++)
    {
        if (URL[Pos]==L'%' && Pos+2<URL.size()) //At least 3 chars
        {
            int32u Char1 = Char2Hex(URL[Pos+1]);
            int32u Char2 = Char2Hex(URL[Pos+2]);
            int32u Char  = (Char1<<4) | Char2;
            if (Char>=0xC2 && Char<=0xF4)
            {
                //Handle as UTF-8
                auto AdditionalBytes_Real=0;
                auto AdditionalBytes_Theory=Char>=0xF0?3:(Char>=0xE0?2:1);
                Char&=AdditionalBytes_Theory>0xF0?0x1F:0x0F;
                if (Pos+(AdditionalBytes_Theory+1)*3<=URL.size())
                {
                    for (auto i=0; i<AdditionalBytes_Theory; i++)
                        if (URL[Pos+3*i]!=L'%' )
                            AdditionalBytes_Theory=0;
                    for (auto i=0; i<AdditionalBytes_Theory; i++)
                    {
                        auto Base=Pos+(i+1)*3+1;
                        Char1 = Char2Hex(URL[Base]);
                        Char2 = Char2Hex(URL[Base+1]);
                        Char  = (Char<<6) | ((Char1&0x3)<<4) | Char2;
                    }
                    Pos+=3*AdditionalBytes_Theory; //3 additional chars per extra are used
                }
            }
            if (sizeof(wchar_t)==4 || Char<=0xD800)
                Result+=(wchar_t)Char;
            else
            {
                //Output as UTF-16
                Char-=0x10000;
                Result+=0xD800|((wchar_t)(Char>>10));
                Result+=0xDC00|((wchar_t)(Char&((1<<10)-1)));
            }
            Pos+=2; //3 chars are used
        }
        else if (URL[Pos]==L'+')
            Result+=L' ';
        else
            Result+=URL[Pos];
    }
    return Result;
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File__ReferenceFilesHelper::File__ReferenceFilesHelper(File__Analyze* MI_, MediaInfo_Config_MediaInfo* Config_)
{
    //In
    TestContinuousFileNames=false;
    ContainerHasNoId=false;
    ID_Max=0;

    //Private
    Sequences_Current=0;
    MI=MI_;
    Config=Config_;
    Init_Done=false;
    FrameRate=0;
    Duration=0;
    #if MEDIAINFO_DEMUX
        Demux_Interleave=false;
    #if MEDIAINFO_NEXTPACKET
        DTS_Minimal=(int64u)-1;
	#endif
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_EVENTS
        StreamID_Previous=(int64u)-1;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Offset_Video_DTS=0;
    #endif //MEDIAINFO_DEMUX

    //Temp
    FilesForStorage=false;
    HasMainFile=false;
    HasMainFile_Filled=false;
    #if MEDIAINFO_NEXTPACKET
        DTS_Interval=(int64u)-1;
    #endif //MEDIAINFO_NEXTPACKET
}

//---------------------------------------------------------------------------
File__ReferenceFilesHelper::~File__ReferenceFilesHelper()
{
    size_t Sequences_Size=Sequences.size();
    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences_Size; ++Sequences_Pos)
        delete Sequences[Sequences_Pos];
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
bool File__ReferenceFilesHelper_Algo1 (const sequence* Ref1, const sequence* Ref2) { return (Ref1->StreamID<Ref2->StreamID);}
bool File__ReferenceFilesHelper_Algo2 (const sequence* Ref1, const sequence* Ref2) { return (Ref1->StreamPos<Ref2->StreamPos);}
bool File__ReferenceFilesHelper_Algo3 (const sequence* Ref1, const sequence* Ref2) { return (Ref1->StreamKind<Ref2->StreamKind);}
void File__ReferenceFilesHelper_InfoFromFileName (sequences &Sequences)
{
    ZtringListList List;
    vector<size_t> Iterators;

    size_t ItemsValid=0;
    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences.size(); Sequences_Pos++)
    {
        ZtringList List2;
        List2.Separator_Set(0, __T(" "));
        if (Sequences[Sequences_Pos]->StreamKind==Stream_Audio && !Sequences[Sequences_Pos]->FileNames.empty())
        {
            Ztring Name=Sequences[Sequences_Pos]->FileNames[0];
            while (Name.FindAndReplace(__T("51 "), Ztring()));
            while (Name.FindAndReplace(__T("_"), __T(" ")));
            while (Name.FindAndReplace(__T("."), __T(" ")));
            while (Name.FindAndReplace(__T("  "), __T(" ")));
            size_t PathSeparator_Pos=Name.rfind(PathSeparator);
            if (PathSeparator_Pos!=(size_t)-1)
                Name.erase(0, PathSeparator_Pos+1);

            //Removing extension
            if (Name.size()>4 && Name.rfind(__T('.'))==Name.size())
                Name.resize(Name.size()-4);

            List2.Write(Name);
            for (size_t Pos=0; Pos<List2.size(); Pos++)
                List2[Pos].MakeLowerCase();
            List.push_back(List2);
            Iterators.push_back(Sequences_Pos);
            ItemsValid++;
        }
        else
            List.push_back(Ztring());
    }

    if (ItemsValid<2)
        return;

    for (size_t Pos2=0; Pos2<List.size(); Pos2++)
    {
        size_t ChannelLayout_Pos=(size_t)-1;
        size_t Language_Pos=(size_t)-1;
        
        //Pos = index file parts
        for (size_t Pos=0; Pos<List[Pos2].size(); Pos++)
        {
            if (Pos>=List[Pos2].size())
                break; //Maybe begin of title
            const Ztring &Test=List[Pos2][List[Pos2].size()-1-Pos];

            //ChannelLayout
            if (ChannelLayout_Pos==(size_t)-1
             && (Test==__T("l")
             || Test==__T("r")
             || Test==__T("lt")
             || Test==__T("rt")
             || Test==__T("c")
             || Test==__T("lf")
             || Test==__T("lfe")
             || Test==__T("sub")
             || Test==__T("ls")
             || Test==__T("rs")
             || Test==__T("lsr")
             || Test==__T("rsr")             
             || Test==__T("b")
             || Test==__T("mono")))
            {
                ChannelLayout_Pos = Pos;
            }

            //Language
            if (Language_Pos==(size_t)-1
             && (Test==__T("ara")
             || Test==__T("deu")
             || Test==__T("eng")
             || Test==__T("fra")
             || Test==__T("fre")
             || Test==__T("ger")
             || Test==__T("ita")
             || Test==__T("jpn")
             || Test==__T("las") //Latin America Spanish
             || Test==__T("rus")
             || Test==__T("spa")))
            {
                Language_Pos = Pos;
            }

            if (ChannelLayout_Pos!=(size_t)-1 && Language_Pos!=(size_t)-1)
                break;
        }

        //ChannelLayout
        if (ChannelLayout_Pos!=(size_t)-1)
        {
            Ztring ChannelPositions, ChannelPositions2, ChannelLayout;
            if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("l"))
            {
                ChannelPositions=__T("Front: L");
                ChannelPositions2=__T("1/0/0");
                ChannelLayout=__T("L");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("lt"))
            {
                ChannelPositions=__T("Front: Lt");
                ChannelPositions2=__T("1/0/0");
                ChannelLayout=__T("Lt");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("rt"))
            {
                ChannelPositions=__T("Front: Rt");
                ChannelPositions2=__T("1/0/0");
                ChannelLayout=__T("Rt");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("r"))
            {
                ChannelPositions=__T("Front: R");
                ChannelPositions2=__T("1/0/0");
                ChannelLayout=__T("R");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("c") || List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("mono"))
            {
                ChannelPositions=__T("Front: C");
                ChannelPositions2=__T("1/0/0");
                ChannelLayout=__T("C");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("lf") || List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("lfe") || List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("sub"))
            {
                ChannelPositions=__T("LFE");
                ChannelPositions2=__T(".1");
                ChannelLayout=__T("LFE");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("ls"))
            {
                ChannelPositions=__T("Side: L");
                ChannelPositions2=__T("0/1/0");
                ChannelLayout=__T("Ls");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("rs"))
            {
                ChannelPositions=__T("Side: R");
                ChannelPositions2=__T("0/1/0");
                ChannelLayout=__T("Rs");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("lsr"))
            {
                ChannelPositions=__T("Back: L");
                ChannelPositions2=__T("0/0/1");
                ChannelLayout=__T("Lsr");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("rsr"))
            {
                ChannelPositions=__T("Back: R");
                ChannelPositions2=__T("0/0/1");
                ChannelLayout=__T("Rsr");
            }
            else if (List[Pos2][List[Pos2].size()-1-ChannelLayout_Pos]==__T("b"))
            {
                ChannelPositions=__T("Back: C");
                ChannelPositions2=__T("0/0/1");
                ChannelLayout=__T("Cs");
            }

            Sequences[Pos2]->Infos["ChannelPositions"]=ChannelPositions;
            Sequences[Pos2]->Infos["ChannelPositions/String2"]=ChannelPositions2;
            Sequences[Pos2]->Infos["ChannelLayout"]=ChannelLayout;
        }
        
        //Language
        if (Language_Pos!=(size_t)-1)
            if (1+Language_Pos<List[Pos2].size())
            {
                Ztring Language;
                if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("ara"))
                    Language=__T("ar");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("deu") || List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("ger"))
                    Language=__T("de");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("eng"))
                    Language=__T("en");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("fra") || List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("fre"))
                    Language=__T("fr");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("ita"))
                    Language=__T("it");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("jpn"))
                    Language=__T("ja");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("las")) //Latin America Spanish
                    Language=__T("es-419");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("rus"))
                    Language=__T("ru");
                else if (List[Pos2][List[Pos2].size()-1-Language_Pos]==__T("spa"))
                    Language=__T("es");

                Sequences[Pos2]->Infos["Language"]=Language.empty()?List[Pos2][List[Pos2].size()-1-Language_Pos]:Language;
            }
    }
}

//***************************************************************************
// In
//***************************************************************************

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::AddSequence(sequence* NewSequence)
{
    Sequences.push_back(NewSequence);
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::DetectSameReels(vector<size_t> &ReelCount)
{
    if (ReelCount.size()<=1)
        return;

    // Finding the max count of streams per stream kind
    size_t StreamCounts_Max[Stream_Max+1];
    size_t StreamCounts[Stream_Max+1];
    vector<size_t> Sequence_Pos_PerKind[Stream_Max+1];
    memset(StreamCounts_Max, 0x00, (Stream_Max+1)*sizeof(size_t));
    size_t Sequence_Pos=0;
    for (size_t r=0; r<ReelCount.size(); r++)
    {
        memset(StreamCounts, 0x00, (Stream_Max+1)*sizeof(size_t));
        for (size_t i=0; i<ReelCount[r]; i++)
        {
            if (Sequence_Pos_PerKind[Sequences[Sequence_Pos]->StreamKind].size()<=StreamCounts[Sequences[Sequence_Pos]->StreamKind])
                Sequence_Pos_PerKind[Sequences[Sequence_Pos]->StreamKind].push_back(Sequence_Pos);
            StreamCounts[Sequences[Sequence_Pos]->StreamKind]++;
            Sequence_Pos++;
        }
        for (size_t i=0; i<Stream_Max+1; i++)
            if (StreamCounts[i] && StreamCounts[i]!=StreamCounts_Max[i])
            {
                if (StreamCounts_Max[i])
                    return; // incoherent count of streams per stream kind, we do nothing
                StreamCounts_Max[i]=StreamCounts[i];
            }
    }

    //Merge resources from different reels
    Sequence_Pos=ReelCount[0];
    vector<size_t> Sequence_Pos_toDelete;
    for (size_t r=1; r<ReelCount.size(); r++)
    {
        memset(StreamCounts, 0x00, (Stream_Max+1)*sizeof(size_t));
        for (size_t i=0; i<ReelCount[r]; i++)
        {
            if (Sequences[Sequence_Pos]->StreamKind!=Stream_Max)
            {
                size_t Sequence_Pos_First=Sequence_Pos_PerKind[Sequences[Sequence_Pos]->StreamKind][StreamCounts[Sequences[Sequence_Pos]->StreamKind]];
                if (Sequence_Pos!=Sequence_Pos_First)
                {
                    Sequences[Sequence_Pos_First]->Resources.insert(Sequences[Sequence_Pos_First]->Resources.end(), Sequences[Sequence_Pos]->Resources.begin(), Sequences[Sequence_Pos]->Resources.end());
                    Sequence_Pos_toDelete.push_back(Sequence_Pos);
                }
            }
            StreamCounts[Sequences[Sequence_Pos]->StreamKind]++;
            Sequence_Pos++;
        }
    }

    // Remove other reels
    for (size_t i=Sequence_Pos_toDelete.size()-1; i!=(size_t)-1; i--)
    {
        delete Sequences[Sequence_Pos_toDelete[i]];
        Sequences.erase(Sequences.begin()+Sequence_Pos_toDelete[i]);
    }

    // Fake StreamIDs
    for (size_t i=0; i<Sequences.size(); i++)
        Sequences[i]->StreamID=i+1;
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::UpdateFileName(const Ztring& OldFileName, const Ztring& NewFileName)
{
    size_t Sequences_Size=Sequences.size();
    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences_Size; ++Sequences_Pos)
    {
        sequence* Sequence=Sequences[Sequences_Pos];

        Sequence->UpdateFileName(OldFileName, NewFileName);
    }
}

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED
void File__ReferenceFilesHelper::UpdateMetaDataFromSourceEncoding(const string& SourceEncoding, const string& Name, const string& Value)
{
    size_t Sequences_Size=Sequences.size();
    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences_Size; ++Sequences_Pos)
    {
        sequence* Sequence=Sequences[Sequences_Pos];

        Sequence->UpdateMetaDataFromSourceEncoding(SourceEncoding, Name, Value);
    }
}
#endif //MEDIAINFO_ADVANCED

//***************************************************************************
// Streams management
//***************************************************************************

void File__ReferenceFilesHelper::ParseReferences()
{
    if (!Init_Done)
    {
        #if MEDIAINFO_FILTER
            if (MI->Config->File_Filter_Audio_Get())
            {
                for (size_t Pos=0; Pos<Sequences.size(); Pos++)
                    if (Sequences[Pos]->StreamKind!=Stream_Audio)
                    {
                        Sequences.erase(Sequences.begin()+Pos);
                        Pos--;
                    }
            }
        #endif //MEDIAINFO_FILTER

        //Filling Filenames from the more complete version and Edit rates
        float64 EditRate=DBL_MAX;
        size_t  EditRate_Count=0;
        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
            if (Sequences[Sequences_Current]->FileNames.empty())
                for (size_t Pos=0; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
                {
                    for (size_t Resource_FileNames_Pos=0; Resource_FileNames_Pos<Sequences[Sequences_Current]->Resources[Pos]->FileNames.size(); Resource_FileNames_Pos++)
                        Sequences[Sequences_Current]->FileNames.push_back(Sequences[Sequences_Current]->Resources[Pos]->FileNames[Resource_FileNames_Pos]);
                    if (Sequences[Sequences_Current]->Resources[Pos]->EditRate && EditRate!=Sequences[Sequences_Current]->Resources[Pos]->EditRate)
                    {
                        if (EditRate>Sequences[Sequences_Current]->Resources[Pos]->EditRate)
                            EditRate=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
                        EditRate_Count++;
                    }
                }
        /*
        //It was used for demux with seek, not used in practice in the open source version, and it has bad side effects so currently disabled, we should find a better way to do that
        if (EditRate_Count>1)
            //Multiple rates, using only one rate
            for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                for (size_t Pos=0; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
                    if (Sequences[Sequences_Current]->Resources[Pos]->EditRate && EditRate!=Sequences[Sequences_Current]->Resources[Pos]->EditRate)
                    {
                        if (Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore)
                        {
                            float64 Temp=(float64)Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore;
                            Temp/=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
                            Temp*=EditRate;
                            Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore=float64_int64s(Temp);
                        }
                        if (Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter!=(int64u)-1)
                        {
                            float64 Temp=(float64)Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter;
                            Temp/=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
                            Temp*=EditRate;
                            Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter=float64_int64s(Temp);
                        }
                        if (Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration!=(int64u)-1)
                        {
                            float64 Temp=(float64)Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration;
                            Temp/=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
                            Temp*=EditRate;
                            Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration=float64_int64s(Temp);
                        }
                        Sequences[Sequences_Current]->Resources[Pos]->EditRate=EditRate;
                    }
        */

        //Testing IDs
        std::set<int64u> StreamList;
        bool StreamList_DuplicatedIds=false;
        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
            if (StreamList.find(Sequences[Sequences_Current]->StreamID)==StreamList.end())
                StreamList.insert(Sequences[Sequences_Current]->StreamID);
            else
            {
                StreamList_DuplicatedIds=true;
                break;
            }
        if (StreamList_DuplicatedIds)
            for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                Sequences[Sequences_Current]->StreamID=Sequences_Current+1;
        if (Sequences.size()==1 && (*Sequences.begin())->StreamID==(int64u)-1)
        {
            ContainerHasNoId=true;
            #if MEDIAINFO_EVENTS
                MI->StreamIDs_Width[MI->StreamIDs_Size-1]=0;
            #endif //MEDIAINFO_EVENTS
        }
        std::sort(Sequences.begin(), Sequences.end(), File__ReferenceFilesHelper_Algo1);
        std::sort(Sequences.begin(), Sequences.end(), File__ReferenceFilesHelper_Algo2);
        std::sort(Sequences.begin(), Sequences.end(), File__ReferenceFilesHelper_Algo3);

        //InfoFromFileName
        File__ReferenceFilesHelper_InfoFromFileName(Sequences);

        #if MEDIAINFO_EVENTS
            if (MI->Config->Config_PerPackage==NULL)
            {
                MI->Config->Config_PerPackage=new MediaInfo_Config_PerPackage;
                MI->Config->Config_PerPackage->CountOfPackages=Sequences.size();
            }
        #endif //MEDIAINFO_EVENTS

        //Configuring file names
        Sequences_Current=0;
        while (Sequences_Current<Sequences.size())
        {
            ZtringList Names=Sequences[Sequences_Current]->FileNames;
            ZtringList AbsoluteNames; AbsoluteNames.Separator_Set(0, ",");
            bool IsUrlEncoded=false;
            for (size_t Pos=0; Pos<Names.size(); Pos++)
            {
                if (Names[Pos].find(__T("file:///"))==0)
                {
                    Names[Pos].erase(0, 8); //Removing "file:///", this is the default behaviour and this makes comparison easier
                    Names[Pos].From_Unicode(URL_Encoded_Decode(Names[Pos].To_Unicode()));
                    IsUrlEncoded=true;
                }
                if (Names[Pos].find(__T("file://"))==0)
                {
                    Names[Pos].erase(0, 7); //Removing "file://", this is the default behaviour and this makes comparison easier
                    Names[Pos].From_Unicode(URL_Encoded_Decode(Names[Pos].To_Unicode()));
                    IsUrlEncoded=true;
                }
                if (Names[Pos].find(__T("file:"))==0)
                {
                    Names[Pos].erase(0, 5); //Removing "file:", this is the default behaviour and this makes comparison easier
                    Names[Pos].From_Unicode(URL_Encoded_Decode(Names[Pos].To_Unicode()));
                    IsUrlEncoded=true;
                }
                Ztring AbsoluteName;
                if (Names[Pos].find(__T(':'))!=1 && Names[Pos].find(__T("/"))!=0 && Names[Pos].find(__T("\\\\"))!=0) //If absolute patch
                {
                    if (MI->File_Name.find(__T("://"))==string::npos)
                        AbsoluteName=ZenLib::FileName::Path_Get(MI->File_Name);
                    else
                    {
                        size_t Pos_Path=MI->File_Name.find_last_of('/');
                        if (Pos_Path!=Ztring::npos)
                            AbsoluteName=MI->File_Name.substr(0, Pos_Path);
                    }
                    if (!AbsoluteName.empty())
                        AbsoluteName+=ZenLib::PathSeparator;
                }
                AbsoluteName+=Names[Pos];
                #ifdef __WINDOWS__
                    if (AbsoluteName.find(__T("://"))==string::npos)
                        AbsoluteName.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); //Names[Pos] normalization local file
                    else
                        AbsoluteName.FindAndReplace(__T("\\"), __T("/"), 0, Ztring_Recursive); //Names[Pos] normalization with protocol (so "/" in all cases)
                #endif //__WINDOWS__
                AbsoluteNames.push_back(AbsoluteName);
            }
            if (AbsoluteNames.empty() || !(AbsoluteNames[0].find(__T("://"))!=string::npos || File::Exists(AbsoluteNames[0])))
            {
                AbsoluteNames.clear();

                //Configuring file name (this time, we try to force URL decode in all cases)
                for (size_t Pos=0; Pos<Names.size(); Pos++)
                {
                    Names[Pos].From_Unicode(URL_Encoded_Decode(Names[Pos].To_Unicode()));
                    IsUrlEncoded=true;
                    Ztring AbsoluteName;
                    if (Names[Pos].find(__T(':'))!=1 && Names[Pos].find(__T("/"))!=0 && Names[Pos].find(__T("\\\\"))!=0) //If absolute patch
                    {
                        if (MI->File_Name.find(__T("://"))==string::npos)
                            AbsoluteName=ZenLib::FileName::Path_Get(MI->File_Name);
                        else
                        {
                            size_t Pos_Path=MI->File_Name.find_last_of('/');
                            if (Pos_Path!=Ztring::npos)
                                AbsoluteName=MI->File_Name.substr(0, Pos_Path);
                        }
                        if (!AbsoluteName.empty())
                            AbsoluteName+=ZenLib::PathSeparator;
                    }
                    AbsoluteName+=Names[Pos];
                    #ifdef __WINDOWS__
                        AbsoluteName.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); //Names[Pos] normalization
                    #endif //__WINDOWS__
                    AbsoluteNames.push_back(AbsoluteName);
                }

                if (AbsoluteNames.empty() || !File::Exists(AbsoluteNames[0]))
                {
                    AbsoluteNames.clear();
                    Names=Sequences[Sequences_Current]->FileNames;

                    //Configuring file name (this time, we try to test local files)
                    size_t PathSeparator_Pos=Names.empty()?string::npos:Names[0].find_last_of(__T("\\/"));
                    if (PathSeparator_Pos!=string::npos && PathSeparator_Pos)
                    {
                        Ztring PathToRemove=Names[0].substr(0, PathSeparator_Pos);
                        bool IsOk=true;
                        for (size_t Pos=0; Pos<Names.size(); Pos++)
                            if (Names[Pos].find(PathToRemove))
                            {
                                IsOk=false;
                                break;
                            }
                        if (IsOk)
                        {
                            for (size_t Pos=0; Pos<Names.size(); Pos++)
                            {
                                Names[Pos].erase(0, PathSeparator_Pos+1);
                                Ztring AbsoluteName;
                                if (MI->File_Name.find(__T("://"))==string::npos)
                                    AbsoluteName=ZenLib::FileName::Path_Get(MI->File_Name);
                                else
                                {
                                    size_t Pos_Path=MI->File_Name.find_last_of('/');
                                    if (Pos_Path!=Ztring::npos)
                                        AbsoluteName=MI->File_Name.substr(0, Pos_Path);
                                }
                                if (!AbsoluteName.empty())
                                    AbsoluteName+=ZenLib::PathSeparator;
                                AbsoluteName+=Names[Pos];
                                #ifdef __WINDOWS__
                                    if (AbsoluteName.find(__T("://"))==string::npos)
                                        AbsoluteName.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); //Names[Pos] normalization local file
                                    else
                                        AbsoluteName.FindAndReplace(__T("\\"), __T("/"), 0, Ztring_Recursive); //Names[Pos] normalization with protocol (so "/" in all cases)
                                #endif //__WINDOWS__
                                AbsoluteNames.push_back(AbsoluteName);
                            }

                            if (!File::Exists(AbsoluteNames[0]))
                            {
                                AbsoluteNames.clear();
                                Names=Sequences[Sequences_Current]->FileNames;

                                //Configuring file name (this time, we try to test local files)
                                size_t PathSeparator_Pos=Names[0].find_last_of(__T("\\/"));
                                if (PathSeparator_Pos!=string::npos && PathSeparator_Pos)
                                    PathSeparator_Pos=Names[0].find_last_of(__T("\\/"), PathSeparator_Pos-1);
                                if (PathSeparator_Pos!=string::npos && PathSeparator_Pos)
                                {
                                    Ztring PathToRemove=Names[0].substr(0, PathSeparator_Pos);
                                    bool IsOk=true;
                                    for (size_t Pos=0; Pos<Names.size(); Pos++)
                                        if (Names[Pos].find(PathToRemove))
                                        {
                                            IsOk=false;
                                            break;
                                        }
                                    if (IsOk)
                                        for (size_t Pos=0; Pos<Names.size(); Pos++)
                                        {
                                            Names[Pos].erase(0, PathSeparator_Pos+1);
                                            Ztring AbsoluteName;
                                            if (MI->File_Name.find(__T("://"))==string::npos)
                                                AbsoluteName=ZenLib::FileName::Path_Get(MI->File_Name);
                                            else
                                            {
                                                size_t Pos_Path=MI->File_Name.find_last_of('/');
                                                if (Pos_Path!=Ztring::npos)
                                                    AbsoluteName=MI->File_Name.substr(0, Pos_Path);
                                            }
                                            if (!AbsoluteName.empty())
                                                AbsoluteName+=ZenLib::PathSeparator;
                                            AbsoluteName+=Names[Pos];
                                            #ifdef __WINDOWS__
                                                AbsoluteName.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); //Names[Pos] normalization
                                            #endif //__WINDOWS__
                                            AbsoluteNames.push_back(AbsoluteName);
                                        }
                                }

                                if (!AbsoluteNames.empty() && !File::Exists(AbsoluteNames[0]))
                                {
                                    AbsoluteNames.clear();
                                    IsUrlEncoded=false;
                                }
                            }
                        }
                    }
                }
            }
            Sequences[Sequences_Current]->Source=IsUrlEncoded?Ztring().From_Unicode(URL_Encoded_Decode(Sequences[Sequences_Current]->FileNames.Read(0).To_Unicode())).c_str():Sequences[Sequences_Current]->FileNames.Read(0);
            if (Sequences[Sequences_Current]->StreamKind!=Stream_Max && !Sequences[Sequences_Current]->Source.empty())
            {
                if (Sequences[Sequences_Current]->StreamPos==(size_t)-1)
                    Sequences[Sequences_Current]->StreamPos=Stream_Prepare(Sequences[Sequences_Current]->StreamKind);
                MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "Source", Sequences[Sequences_Current]->Source);
            }
            if (!AbsoluteNames.empty())
                Sequences[Sequences_Current]->FileNames=AbsoluteNames;

            if (!AbsoluteNames.empty() && AbsoluteNames[0]==MI->File_Name)
            {
                Sequences[Sequences_Current]->IsCircular=true;
                Sequences[Sequences_Current]->FileNames.clear();
                Sequences[Sequences_Current]->Status.set(File__Analyze::IsFinished);
            }
            else if (!AbsoluteNames.empty())
                Sequences[Sequences_Current]->FileNames=AbsoluteNames;
            else
            {
                Sequences[Sequences_Current]->Status.set(File__Analyze::IsFinished);
                #if MEDIAINFO_EVENTS
                    Config->Event_SubFile_Missing(Sequences[Sequences_Current]->Source);
                #endif //MEDIAINFO_EVENTS
                if (Sequences[Sequences_Current]->StreamKind!=Stream_Max && !Sequences[Sequences_Current]->Source.empty())
                {
                    MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "Source_Info", "Missing");
                    if (MI->Retrieve(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, General_ID).empty() && Sequences[Sequences_Current]->StreamID!=(int64u)-1)
                        MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, General_ID, Sequences[Sequences_Current]->StreamID);
                    for (std::map<string, Ztring>::iterator Info=Sequences[Sequences_Current]->Infos.begin(); Info!=Sequences[Sequences_Current]->Infos.end(); ++Info)
                    {
                        if (Info->first=="CodecID")
                            MI->CodecID_Fill(Info->second, Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, InfoCodecID_Format_Mpeg4);
                        else
                            MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, Info->first.c_str(), Info->second);
                    }
                }
            }

            if (FilesForStorage)
            {
                for (size_t Pos=0; Pos<Sequences[Sequences_Current]->FileNames.size(); Pos++)
                {
                    if (Pos==Sequences[Sequences_Current]->Resources.size())
                        Sequences[Sequences_Current]->Resources.push_back(new resource);
                    Sequences[Sequences_Current]->Resources[Pos]->FileNames.clear();
                    Sequences[Sequences_Current]->Resources[Pos]->FileNames.push_back(Sequences[Sequences_Current]->FileNames[Pos]);
                }
                Sequences[Sequences_Current]->FileNames.resize(1);
            }

            Sequences_Current++;
        }

        CountOfReferencesToParse=Sequences.size();

        #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            if (Config->NextPacket_Get())
            {
                Demux_Interleave=Config->File_Demux_Interleave_Get();
                if (Demux_Interleave)
                {
                    for (sequences::iterator ReferenceSource=Sequences.begin(); ReferenceSource!=Sequences.end(); ++ReferenceSource)
                        if ((*ReferenceSource)->FileNames.empty())
                            CountOfReferencesToParse--;
                    DTS_Interval=250000000LL; // 250 milliseconds
                }
            }
            else
                Demux_Interleave=false;

            //Using the frame rate from the first stream having a frame rate
            if (!FrameRate)
                for (sequences::iterator ReferenceFrameRate=Sequences.begin(); ReferenceFrameRate!=Sequences.end(); ++ReferenceFrameRate)
                    if ((*ReferenceFrameRate)->FrameRate)
                    {
                        FrameRate=(*ReferenceFrameRate)->FrameRate;
                        break;
                    }

            if (Config->NextPacket_Get())
            {
                Sequences_Current=0;
                while (Sequences_Current<Sequences.size())
                {
                    ParseReference(); //Init
                    Sequences_Current++;
                }

                //Cleanup
                for (size_t Pos=0; Pos<Sequences.size(); Pos++)
                    if (Sequences[Pos]->Status[File__Analyze::IsFinished])
                    {
                        Sequences.erase(Sequences.begin()+Pos);
                        Pos--;
                    }
                CountOfReferencesToParse=Sequences.size();
                if (Sequences.empty())
                    return;

                //File size handling
                if (MI->Config->File_Size!=MI->File_Size)
                {
                    MI->Fill(Stream_General, 0, General_FileSize, MI->Config->File_Size, 10, true);
                    MI->Fill(Stream_General, 0, General_StreamSize, MI->File_Size, 10, true);
                }

            }
        #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET

        FileSize_Compute();
        Sequences_Current=0;
        CountOfReferences_ForReadSize=Sequences.size();
        Init_Done=true;

        #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            if (Config->NextPacket_Get() && MI->Demux_EventWasSent_Accept_Specific)
            {
                MI->Config->Demux_EventWasSent=true;
                return;
            }
        #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
    }

    while (Sequences_Current<Sequences.size())
    {
        #if MEDIAINFO_NEXTPACKET
        if (!Sequences[Sequences_Current]->Status[File__Analyze::IsFinished])
        #endif //MEDIAINFO_NEXTPACKET
            ParseReference();

        //State
        int64u FileSize_Parsed=0;
        #if MEDIAINFO_NEXTPACKET
            DTS_Minimal=(int64u)-1;
        #endif //MEDIAINFO_NEXTPACKET
        for (sequences::iterator ReferenceTemp=Sequences.begin(); ReferenceTemp!=Sequences.end(); ++ReferenceTemp)
        {
            if ((*ReferenceTemp)->MI)
            {
                if ((*ReferenceTemp)->State<10000)
                {
                    (*ReferenceTemp)->State=(*ReferenceTemp)->MI->State_Get();
                    if ((*ReferenceTemp)->State && (*ReferenceTemp)->MI->Config.File_Size!=(int64u)-1)
                        FileSize_Parsed+=(int64u)((*ReferenceTemp)->MI->Config.File_Size*(((float)(*ReferenceTemp)->State)/10000));
                }
                else
                    FileSize_Parsed+=(*ReferenceTemp)->MI->Config.File_Size;

                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    //Minimal DTS
                    if (DTS_Interval!=(int64u)-1 && !Sequences[Sequences_Current]->Status[File__Analyze::IsFinished] && ((*ReferenceTemp)->Resources.empty() || (*ReferenceTemp)->Resources_Current<(*ReferenceTemp)->Resources.size()))
                    {
                        int64u DTS_Temp;
                        if (!(*ReferenceTemp)->Resources.empty() && (*ReferenceTemp)->Resources_Current)
                        {
                            if ((*ReferenceTemp)->Resources[(*ReferenceTemp)->Resources_Current]->MI->Info->FrameInfo.DTS!=(int64u)-1)
                                DTS_Temp=(*ReferenceTemp)->Resources[(*ReferenceTemp)->Resources_Current]->MI->Info->FrameInfo.DTS-(*ReferenceTemp)->Resources[(*ReferenceTemp)->Resources_Current]->MI->Info->Config->Demux_Offset_DTS_FromStream;
                            else
                                DTS_Temp=0;
                        }
                        else
                        {
                            if ((*ReferenceTemp)->MI->Info->FrameInfo.DTS!=(int64u)-1)
                                DTS_Temp=(*ReferenceTemp)->MI->Info->FrameInfo.DTS-(*ReferenceTemp)->MI->Info->Config->Demux_Offset_DTS_FromStream;
                            else
                                DTS_Temp=0;
                        }
                        if ((*ReferenceTemp)->Resources_Current<(*ReferenceTemp)->Resources.size())
                            DTS_Temp+=(*ReferenceTemp)->Resources[(*ReferenceTemp)->Resources_Current]->Demux_Offset_DTS;
                        if (DTS_Minimal>DTS_Temp)
                            DTS_Minimal=DTS_Temp;
                    }
                #endif //MEDIAINFO_DEMUX &&  MEDIAINFO_NEXTPACKET
            }
            else
                FileSize_Parsed+=(*ReferenceTemp)->FileSize;
        }
        Config->State_Set(((float)FileSize_Parsed)/MI->Config->File_Size);

        #if MEDIAINFO_EVENTS
            struct MediaInfo_Event_General_SubFile_End_0 Event;
            MI->Event_Prepare((struct MediaInfo_Event_Generic*)&Event, MediaInfo_EventCode_Create(0, MediaInfo_Event_General_SubFile_End, 0), sizeof(struct MediaInfo_Event_General_SubFile_End_0));
            MI->Config->Event_Send(NULL, (const int8u*)&Event, Event.EventSize, MI->File_Name);
        #endif //MEDIAINFO_EVENTS

        #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            if (Demux_Interleave && (Sequences[Sequences_Current]->MI==NULL || Sequences[Sequences_Current]->MI->Info==NULL || Sequences[Sequences_Current]->MI->Info->Demux_CurrentParser==NULL || Sequences[Sequences_Current]->MI->Info->Demux_CurrentParser->Demux_TotalBytes>=Sequences[Sequences_Current]->MI->Info->Demux_CurrentParser->Buffer_TotalBytes+Sequences[Sequences_Current]->MI->Info->Demux_CurrentParser->Buffer_Size))
            {
                size_t Reference_Next=Sequences_Current; ++Reference_Next;

                if (Reference_Next==Sequences.size() && Config->NextPacket_Get() && CountOfReferencesToParse)
                    Sequences_Current=0;
                else
                    Sequences_Current=Reference_Next;

                if (Config->Demux_EventWasSent)
                    return;
            }
            else
            {
                if (Config->Demux_EventWasSent)
                    return;

                Sequences_Current++;

                if (Demux_Interleave && Sequences_Current == Sequences.size() && Config->NextPacket_Get() && CountOfReferencesToParse)
                    Sequences_Current = 0;
            }
        #else //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
            Sequences_Current++;
        #endif //MEDIAINFO_DEMUX
    }

    //File size handling
    FileSize_Compute();
    if (MI->Config->File_Size!=MI->File_Size
        #if MEDIAINFO_ADVANCED
            && !Config->File_IgnoreSequenceFileSize_Get()
        #endif //MEDIAINFO_ADVANCED
            )
    {
        MI->Fill(Stream_General, 0, General_FileSize, MI->Config->File_Size, 10, true);
        MI->Fill(Stream_General, 0, General_StreamSize, MI->File_Size, 10, true);
    }
    #if MEDIAINFO_ADVANCED
        if (Config->File_IgnoreSequenceFileSize_Get())
            MI->Clear(Stream_General, 0, General_FileSize);
    #endif //MEDIAINFO_ADVANCED
}

//---------------------------------------------------------------------------
bool File__ReferenceFilesHelper::ParseReference_Init()
{
    //Configuration
    Sequences[Sequences_Current]->MI=MI_Create();
    if (Config->ParseSpeed>=1)
    {
        for (size_t Pos=0; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
        {
            if (Sequences[Sequences_Current]->Resources[0]->EditRate)
            {
                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    if (Pos==0)
                    {
                        Sequences[Sequences_Current]->Resources[0]->Demux_Offset_DTS=0;
                        Sequences[Sequences_Current]->Resources[0]->Demux_Offset_Frame=0;
                    }
                    if (Pos+1<Sequences[Sequences_Current]->Resources.size())
                    {
                        Sequences[Sequences_Current]->Resources[Pos+1]->Demux_Offset_DTS=float64_int64s(Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_DTS+(Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter-Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore)/Sequences[Sequences_Current]->Resources[0]->EditRate*1000000000);
                        Sequences[Sequences_Current]->Resources[Pos+1]->Demux_Offset_Frame=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_Frame+Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter-Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore;
                    }
                #endif //MEDIAINFO_DEMUX
            }
            else
            {
                MediaInfo_Internal MI2;
                MI2.Option(__T("File_KeepInfo"), __T("1"));
                Ztring ParseSpeed_Save=MI2.Option(__T("ParseSpeed_Get"), __T("0"));
                Ztring Demux_Save=MI2.Option(__T("Demux_Get"), __T(""));
                MI2.Option(__T("ParseSpeed"), __T("0"));
                MI2.Option(__T("Demux"), Ztring());
                Sequences[Sequences_Current]->Resources[Pos]->FileNames.Separator_Set(0, ",");
                size_t MiOpenResult=MI2.Open(Sequences[Sequences_Current]->Resources[Pos]->FileNames.Read());
                MI2.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
                MI2.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
                if (MiOpenResult)
                {
                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        int64u Duration=MI2.Get(Sequences[Sequences_Current]->StreamKind, 0, __T("Duration")).To_int64u()*1000000;
                        int64u FrameCount=MI2.Get(Sequences[Sequences_Current]->StreamKind, 0, __T("FrameCount")).To_int64u();
                        if (Pos==0)
                        {
                            int64u Delay=MI2.Get(Stream_Video, 0, Video_Delay).To_int64u()*1000000;
                            if (Sequences[Sequences_Current]->StreamKind==Stream_Video && Offset_Video_DTS==0)
                                Offset_Video_DTS=Delay;
                            Sequences[Sequences_Current]->Resources[0]->Demux_Offset_DTS=Offset_Video_DTS;
                            Sequences[Sequences_Current]->Resources[0]->Demux_Offset_Frame=0;
                        }
                        if (Pos+1<Sequences[Sequences_Current]->Resources.size())
                        {
                            Sequences[Sequences_Current]->Resources[Pos+1]->Demux_Offset_DTS=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_DTS+Duration;
                            Sequences[Sequences_Current]->Resources[Pos+1]->Demux_Offset_Frame=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_Frame+FrameCount;
                        }
                        else
                            Duration=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_DTS+Duration-Sequences[Sequences_Current]->Resources[0]->Demux_Offset_DTS;
                    #endif //MEDIAINFO_DEMUX
                }
            }

            if (Pos)
            {
                Sequences[Sequences_Current]->Resources[Pos]->MI=MI_Create();
                Sequences[Sequences_Current]->Resources[Pos]->MI->Config.File_IgnoreEditsBefore=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore;
                if (Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter==(int64u)-1 && Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration!=(int64u)-1)
                    Sequences[Sequences_Current]->Resources[Pos]->MI->Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore+Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration;
                else
                    Sequences[Sequences_Current]->Resources[Pos]->MI->Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter;
                Sequences[Sequences_Current]->Resources[Pos]->MI->Config.File_EditRate=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
                #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    Sequences[Sequences_Current]->Resources[Pos]->MI->Config.Demux_Offset_Frame=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_Frame;
                    Sequences[Sequences_Current]->Resources[Pos]->MI->Config.Demux_Offset_DTS=Sequences[Sequences_Current]->Resources[Pos]->Demux_Offset_DTS;
                #endif //MEDIAINFO_DEMUX
            }
        }
        if (!Sequences[Sequences_Current]->Resources.empty())
        {
            Sequences[Sequences_Current]->MI->Config.File_IgnoreEditsBefore=Sequences[Sequences_Current]->Resources[0]->IgnoreEditsBefore;
            if (Sequences[Sequences_Current]->Resources[0]->IgnoreEditsAfter==(int64u)-1 && Sequences[Sequences_Current]->Resources[0]->IgnoreEditsAfterDuration!=(int64u)-1)
                Sequences[Sequences_Current]->MI->Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[0]->IgnoreEditsBefore+Sequences[Sequences_Current]->Resources[0]->IgnoreEditsAfterDuration;
            else
                Sequences[Sequences_Current]->MI->Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[0]->IgnoreEditsAfter;
            Sequences[Sequences_Current]->MI->Config.File_EditRate=Sequences[Sequences_Current]->Resources[0]->EditRate;
            #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                Sequences[Sequences_Current]->MI->Config.Demux_Offset_Frame=Sequences[Sequences_Current]->Resources[0]->Demux_Offset_Frame;
                Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS=Sequences[Sequences_Current]->Resources[0]->Demux_Offset_DTS;
            #endif //MEDIAINFO_DEMUX
        }
    }

    if (Sequences[Sequences_Current]->IsCircular)
    {
        MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "Source_Info", "Circular");
        if (!Config->File_KeepInfo_Get())
        {
            #if MEDIAINFO_DEMUX
                if (CountOfReferencesToParse)
                    CountOfReferencesToParse--;
            #endif //MEDIAINFO_DEMUX
            Sequences[Sequences_Current]->StreamKind=Stream_Max;
            Sequences[Sequences_Current]->StreamPos=(size_t)-1;
            Sequences[Sequences_Current]->FileSize=Sequences[Sequences_Current]->MI->Config.File_Size;
            delete Sequences[Sequences_Current]->MI; Sequences[Sequences_Current]->MI=NULL;
        }
        Sequences[Sequences_Current]->FileNames.clear();
        Sequences[Sequences_Current]->Status.set(File__Analyze::IsFinished);
    }
    else
    {
        //Run
        #if MEDIAINFO_EVENTS
            SubFile_Start();
        #endif //MEDIAINFO_EVENTS
        if (!Sequences[Sequences_Current]->MI->Open(Sequences[Sequences_Current]->FileNames.Read()))
        {
            #if MEDIAINFO_EVENTS
                Config->Event_SubFile_Missing(Sequences[Sequences_Current]->Source);
            #endif //MEDIAINFO_EVENTS
            if (Sequences[Sequences_Current]->StreamKind!=Stream_Max)
                MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "Source_Info", "Missing", Unlimited, true, true);
            if (!Config->File_KeepInfo_Get())
            {
                #if MEDIAINFO_DEMUX
                    if (CountOfReferencesToParse)
                        CountOfReferencesToParse--;
                #endif //MEDIAINFO_DEMUX
                Sequences[Sequences_Current]->StreamKind=Stream_Max;
                Sequences[Sequences_Current]->StreamPos=(size_t)-1;
                Sequences[Sequences_Current]->FileSize=Sequences[Sequences_Current]->MI->Config.File_Size;
                delete Sequences[Sequences_Current]->MI; Sequences[Sequences_Current]->MI=NULL;
            }
            Sequences[Sequences_Current]->Status.set(File__Analyze::IsFinished);
        }

        if (Config->ParseSpeed>=1)
            for (size_t Pos=1; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
            {
                Sequences[Sequences_Current]->Resources[Pos]->FileNames.Separator_Set(0, ",");
                Sequences[Sequences_Current]->Resources[Pos]->MI->Open(Sequences[Sequences_Current]->Resources[Pos]->FileNames.Read());
            }

        #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
            if (Config->NextPacket_Get())
                return false;
        #endif //MEDIAINFO_NEXTPACKET
    }

    return true;
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::ParseReference()
{
    if (Sequences[Sequences_Current]->MI==NULL && !Sequences[Sequences_Current]->FileNames.empty())
    {
        if (!ParseReference_Init())
            return;
    }

    if (Sequences[Sequences_Current]->MI)
    {
        #if MEDIAINFO_EVENTS && MEDIAINFO_NEXTPACKET
            if (DTS_Interval!=(int64u)-1 && !Sequences[Sequences_Current]->Status[File__Analyze::IsFinished] && Sequences[Sequences_Current]->MI->Info->FrameInfo.DTS!=(int64u)-1 && DTS_Minimal!=(int64u)-1 && (Sequences[Sequences_Current]->Resources.empty() || Sequences[Sequences_Current]->Resources_Current<Sequences[Sequences_Current]->Resources.size()))
            {
                int64u DTS_Temp = 0;
                #if MEDIAINFO_DEMUX
                    if (!Sequences[Sequences_Current]->Resources.empty() && Sequences[Sequences_Current]->Resources_Current)
                    {
                        if (Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Info->FrameInfo.DTS!=(int64u)-1)
                            DTS_Temp=Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Info->FrameInfo.DTS-Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Info->Config->Demux_Offset_DTS_FromStream;;
                    }
                    else
                    {
                        if (Sequences[Sequences_Current]->MI->Info->FrameInfo.DTS!=(int64u)-1)
                            DTS_Temp=Sequences[Sequences_Current]->MI->Info->FrameInfo.DTS-Sequences[Sequences_Current]->MI->Info->Config->Demux_Offset_DTS_FromStream;
                    }
                #endif //MEDIAINFO_DEMUX

                DTS_Temp+=Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->Demux_Offset_DTS;
                if (!Sequences[Sequences_Current]->Resources.empty() && Sequences[Sequences_Current]->Resources_Current<Sequences[Sequences_Current]->Resources.size() && Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->EditRate && Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->IgnoreEditsBefore)
                {
                    int64u TimeOffset=float64_int64s(((float64)Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->IgnoreEditsBefore)/Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->EditRate*1000000000);
                    if (DTS_Temp>TimeOffset)
                        DTS_Temp-=TimeOffset;
                    else
                        DTS_Temp=0;
                }
                if (DTS_Temp>DTS_Minimal+DTS_Interval)
                    return;
            }
            if (Config->Event_CallBackFunction_IsSet() && !Sequences[Sequences_Current]->Status[File__Analyze::IsFinished])
            {
                #if MEDIAINFO_DEMUX
                    SubFile_Start();
                    if (Sequences[Sequences_Current]->Resources_Current==0)
                    {
                        while ((Sequences[Sequences_Current]->Status=Sequences[Sequences_Current]->MI->Open_NextPacket())[8])
                        {
                                if (!Sequences[Sequences_Current]->FileSize_IsPresent && Sequences[Sequences_Current]->MI->Config.File_Size!=(int64u)-1)
                                {
                                    Sequences[Sequences_Current]->FileSize_IsPresent=true;
                                    if (CountOfReferences_ForReadSize)
                                    {
                                        CountOfReferences_ForReadSize--;
                                        if (!CountOfReferences_ForReadSize)
                                            CountOfReferences_ForReadSize_Run();
                                    }
                                }

                                if (Config->Event_CallBackFunction_IsSet())
                                {
                                    Config->Demux_EventWasSent=true;
                                    return;
                                }
                        }
                        Sequences[Sequences_Current]->Resources_Current++;
                        if (Sequences[Sequences_Current]->Resources_Current<Sequences[Sequences_Current]->Resources.size() && Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI)
                            Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Open_Buffer_Seek(0, 0, (int64u)-1);
                    }

                    #if MEDIAINFO_NEXTPACKET && MEDIAINFO_DEMUX
                        if (Config->ParseSpeed<1.0)
                            Sequences[Sequences_Current]->Resources_Current=Sequences[Sequences_Current]->Resources.size(); //No need to parse all files
                    #endif //MEDIAINFO_NEXTPACKET

                    while (Sequences[Sequences_Current]->Resources_Current<Sequences[Sequences_Current]->Resources.size())
                    {
                        while ((Sequences[Sequences_Current]->Status=Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Open_NextPacket())[8])
                        {
                                if (!Sequences[Sequences_Current]->FileSize_IsPresent && Sequences[Sequences_Current]->MI->Config.File_Size!=(int64u)-1)
                                {
                                    Sequences[Sequences_Current]->FileSize_IsPresent=true;
                                    if (CountOfReferences_ForReadSize)
                                    {
                                        CountOfReferences_ForReadSize--;
                                        if (!CountOfReferences_ForReadSize)
                                            CountOfReferences_ForReadSize_Run();
                                    }
                                }

                                if (Config->Event_CallBackFunction_IsSet())
                                {
                                    Config->Demux_EventWasSent=true;
                                    return;
                                }
                        }
                        Sequences[Sequences_Current]->Resources_Current++;
                        if (Sequences[Sequences_Current]->Resources_Current<Sequences[Sequences_Current]->Resources.size() && Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI)
                            Sequences[Sequences_Current]->Resources[Sequences[Sequences_Current]->Resources_Current]->MI->Open_Buffer_Seek(0, 0, (int64u)-1);
                    }
                if (CountOfReferencesToParse)
                    CountOfReferencesToParse--;
                #endif //MEDIAINFO_DEMUX
            }
        #endif //MEDIAINFO_EVENTS && MEDIAINFO_NEXTPACKET
        ParseReference_Finalize();
        if (!Config->File_KeepInfo_Get())
        {
            Sequences[Sequences_Current]->StreamKind=Stream_Max;
            Sequences[Sequences_Current]->StreamPos=(size_t)-1;
            Sequences[Sequences_Current]->State=10000;
            if (Sequences[Sequences_Current]->Resources.empty())
                Sequences[Sequences_Current]->FileSize=Sequences[Sequences_Current]->MI->Config.File_Size;
            else if (Sequences[Sequences_Current]->FileSize==(int64u)-1)
            {
                Sequences[Sequences_Current]->FileSize=0;
                for (size_t Pos=0; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
                    for (size_t Resource_FileNames_Pos=0; Resource_FileNames_Pos<Sequences[Sequences_Current]->Resources[Pos]->FileNames.size(); Resource_FileNames_Pos++)
                        Sequences[Sequences_Current]->FileSize+=File::Size_Get(Sequences[Sequences_Current]->Resources[Pos]->FileNames[Resource_FileNames_Pos]);
            }
            delete Sequences[Sequences_Current]->MI; Sequences[Sequences_Current]->MI=NULL;
        }
    }
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::ParseReference_Finalize ()
{
    //Removing wrong initial value
    if (Sequences[Sequences_Current]->MI->Count_Get(Sequences[Sequences_Current]->StreamKind)==0 && Sequences[Sequences_Current]->StreamPos!=(size_t)-1
     && Sequences[Sequences_Current]->MI->Count_Get(Stream_Video)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Audio)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Image)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Text)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Other))
    {
        MI->Stream_Erase(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos);
        for (sequences::iterator ReferenceTemp=Sequences.begin(); ReferenceTemp!=Sequences.end(); ++ReferenceTemp)
            if ((*ReferenceTemp)->StreamKind==Sequences[Sequences_Current]->StreamKind && (*ReferenceTemp)->StreamPos!=(size_t)-1 && (*ReferenceTemp)->StreamPos>Sequences[Sequences_Current]->StreamPos)
                (*ReferenceTemp)->StreamPos--;
        Sequences[Sequences_Current]->StreamPos=(size_t)-1;
    }

    bool StreamFound=false;
    for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
    {
        Ztring Title_Temp;
        for (size_t StreamPos=0; StreamPos<Sequences[Sequences_Current]->MI->Count_Get((stream_t)StreamKind); StreamPos++)
        {
            StreamKind_Last=(stream_t)StreamKind;
            if (Sequences[Sequences_Current]->StreamPos!=(size_t)-1 && StreamKind_Last==Sequences[Sequences_Current]->StreamKind && StreamPos==0)
            {
                StreamPos_To=Sequences[Sequences_Current]->StreamPos;
                StreamFound=true;
                Title_Temp=MI->Retrieve_Const(StreamKind_Last, StreamPos_To-StreamPos, "Title");
            }
            else
            {
                size_t ToInsert=(size_t)-1;
                for (sequences::iterator ReferencePos=Sequences.begin(); ReferencePos!=Sequences.end(); ++ReferencePos)
                    if ((*ReferencePos)->StreamKind==StreamKind_Last && Sequences[Sequences_Current]->StreamID<(*ReferencePos)->StreamID)
                    {
                        ToInsert=(*ReferencePos)->StreamPos;
                        break;
                    }

                StreamPos_To=Stream_Prepare((stream_t)StreamKind, ToInsert);
                if (StreamPos)
                    MI->Fill(StreamKind_Last, StreamPos_To, "Title", Title_Temp);
            }
            StreamPos_From=StreamPos;

            ParseReference_Finalize_PerStream();
        }
    }

    if (!StreamFound && Sequences[Sequences_Current]->StreamKind!=Stream_Max && Sequences[Sequences_Current]->StreamPos!=(size_t)-1 && Sequences[Sequences_Current]->MI->Info)
    {
        Ztring MuxingMode=MI->Retrieve(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "MuxingMode");
        if (!MuxingMode.empty())
            MuxingMode.insert(0, __T(" / "));
        MI->Fill(Sequences[Sequences_Current]->StreamKind, Sequences[Sequences_Current]->StreamPos, "MuxingMode", Sequences[Sequences_Current]->MI->Info->Get(Stream_General, 0, General_Format)+MuxingMode, true);
    }
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::ParseReference_Finalize_PerStream ()
{
    //Hacks - Before
    Ztring CodecID=MI->Retrieve(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_CodecID));
    Ztring ID_Base;
    if (HasMainFile_Filled && !Sequences[Sequences_Current]->IsMain)
    {
        ID_Base=Ztring::ToZtring(ID_Max+Sequences[Sequences_Current]->StreamID-1);
        MI->Fill(StreamKind_Last, StreamPos_To, "SideCar_FilePos", Sequences[Sequences_Current]->StreamID-1);
        MI->Fill_SetOptions(StreamKind_Last, StreamPos_To, "SideCar_FilePos", "N NT");
    }
    else if (Sequences[Sequences_Current]->StreamID!=(int64u)-1)
        ID_Base=Ztring::ToZtring(Sequences[Sequences_Current]->StreamID);
    Ztring ID=ID_Base;
    Ztring ID_String=ID_Base;
    Ztring MenuID;
    Ztring MenuID_String;

    if (!HasMainFile_Filled && Sequences[Sequences_Current]->IsMain)
    {
        MI->Fill(Stream_General, 0, General_Format, Sequences[Sequences_Current]->MI->Get(Stream_General, 0, General_Format) , true);
        MI->Fill(Stream_General, 0, General_CompleteName, Sequences[Sequences_Current]->MI->Get(Stream_General, 0, General_CompleteName) , true);
        MI->Fill(Stream_General, 0, General_FileExtension, Sequences[Sequences_Current]->MI->Get(Stream_General, 0, General_FileExtension) , true);
        HasMainFile=true;
        HasMainFile_Filled=true;
    }
    if (Sequences[Sequences_Current]->IsMain)
    {
        int64u ID_New=Sequences[Sequences_Current]->MI->Get(StreamKind_Last, StreamPos_From, General_ID).To_int64u();
        if (ID_Max<ID_New)
            ID_Max=ID_New;
    }

    MI->Clear(StreamKind_Last, StreamPos_To, General_ID);

    MI->Merge(*Sequences[Sequences_Current]->MI->Info, StreamKind_Last, StreamPos_From, StreamPos_To);

    if (!Sequences[Sequences_Current]->Resources.empty())
    {
        MI->Clear(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_BitRate));
        MI->Clear(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_Duration));
        MI->Clear(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_FrameCount));
        MI->Clear(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_StreamSize));

        float64 BitRate_Before=0;
        int64u Duration_Temp=0;
        int64u FrameCount_Temp=0;
        int64u StreamSize_Temp=0;
        int64u FileSize_Temp=0;
        #if MEDIAINFO_ADVANCED
            std::vector<string> Format_Profiles_FromStream, Format_Profiles_FromContainer, Format_Profiles_FromPlaylist;
        #endif //MEDIAINFO_ADVANCED
        for (size_t Pos=0; Pos<Sequences[Sequences_Current]->Resources.size(); Pos++)
        {
            MediaInfo_Internal MI2;
            MI2.Option(__T("File_KeepInfo"), __T("1"));
            Ztring ParseSpeed_Save=MI2.Option(__T("ParseSpeed_Get"), __T(""));
            Ztring Demux_Save=MI2.Option(__T("Demux_Get"), __T(""));
            MI2.Option(__T("ParseSpeed"), __T("0"));
            MI2.Option(__T("Demux"), Ztring());
            MI2.Config.File_IgnoreEditsBefore=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore;
            if (Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter==(int64u)-1 && Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration!=(int64u)-1)
                MI2.Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsBefore+Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfterDuration;
            else
                MI2.Config.File_IgnoreEditsAfter=Sequences[Sequences_Current]->Resources[Pos]->IgnoreEditsAfter;
            MI2.Config.File_EditRate=Sequences[Sequences_Current]->Resources[Pos]->EditRate;
            Sequences[Sequences_Current]->Resources[Pos]->FileNames.Separator_Set(0, ",");
            if (FrameRate)
                MI2.Option(__T("File_Demux_Rate"), Ztring::ToZtring(FrameRate));
            else if (!Sequences[Sequences_Current]->Resources.empty() && Sequences[Sequences_Current]->Resources[0]->EditRate) //TODO: per Pos
                MI2.Option(__T("File_Demux_Rate"), Ztring::ToZtring(Sequences[Sequences_Current]->Resources[0]->EditRate));
            size_t MiOpenResult=MI2.Open(Sequences[Sequences_Current]->Resources[Pos]->FileNames.Read());
            MI2.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
            MI2.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
            if (MiOpenResult)
            {
                BitRate_Before=MI2.Get(StreamKind_Last, StreamPos_From, MI->Fill_Parameter(StreamKind_Last, Generic_BitRate)).To_float64();
                Ztring Duration_Temp2=MI2.Get(StreamKind_Last, StreamPos_From, MI->Fill_Parameter(StreamKind_Last, Generic_Duration));
                if (!Duration_Temp2.empty() && Duration_Temp!=(int64u)-1)
                    Duration_Temp+=Duration_Temp2.To_int64u();
                else
                    Duration_Temp=(int64u)-1;
                Ztring FrameCount_Temp2=MI2.Get(StreamKind_Last, StreamPos_From, MI->Fill_Parameter(StreamKind_Last, Generic_FrameCount));
                if (!FrameCount_Temp2.empty() && FrameCount_Temp!=(int64u)-1)
                    FrameCount_Temp+=FrameCount_Temp2.To_int64u();
                else
                    FrameCount_Temp=(int64u)-1;
                Ztring StreamSize_Temp2=MI2.Get(StreamKind_Last, StreamPos_From, MI->Fill_Parameter(StreamKind_Last, Generic_StreamSize));
                if (!StreamSize_Temp2.empty() && StreamSize_Temp!=(int64u)-1)
                    StreamSize_Temp+=StreamSize_Temp2.To_int64u();
                else
                    StreamSize_Temp=(int64u)-1;
                Ztring FileSize_Temp2=MI2.Get(Stream_General, 0, General_FileSize);
                if (!FileSize_Temp2.empty() && FileSize_Temp!=(int64u)-1)
                    FileSize_Temp+=FileSize_Temp2.To_int64u();
                else
                    FileSize_Temp=(int64u)-1;
            }
            else
            {
                Duration_Temp=(int64u)-1;
                FrameCount_Temp=(int64u)-1;
                StreamSize_Temp=(int64u)-1;
                FileSize_Temp=(int64u)-1;
                break;
            }

            #if MEDIAINFO_ADVANCED
                //Profile
                map<string, string>::iterator MetadataFromPlaylist_Item=Sequences[Sequences_Current]->Resources[Pos]->MetadataFromPlaylist.find("Format_Profile");
                if (MetadataFromPlaylist_Item!=Sequences[Sequences_Current]->Resources[Pos]->MetadataFromPlaylist.end())
                    Format_Profiles_FromPlaylist.push_back(MetadataFromPlaylist_Item->second);
                else
                    Format_Profiles_FromPlaylist.push_back(string());
                Format_Profiles_FromContainer.push_back(MI2.Get(StreamKind_Last, StreamPos_From, __T("Format_Profile_FromContainer")).To_UTF8());
                Format_Profiles_FromStream.push_back(MI2.Get(StreamKind_Last, StreamPos_From, MI->Fill_Parameter(StreamKind_Last, Generic_Format_Profile)).To_UTF8());
            #endif //MEDIAINFO_ADVANCED
        }
        #if MEDIAINFO_ADVANCED
            //Test of coherency between Playlist and Stream
            //Test of same values for all resources
            bool Format_Profile_NotSame=false;
            bool Format_Profile_CoherencyIssue=false;
            bool Format_Profile_FromContainer_IsPresent=false;
            for (size_t Pos=0; Pos<Format_Profiles_FromStream.size(); Pos++)
            {
                if (Format_Profiles_FromStream[Pos]!=Format_Profiles_FromStream[0])
                    Format_Profile_NotSame=true;
                if (!Format_Profiles_FromPlaylist[Pos].empty() && Format_Profiles_FromStream[Pos]!=Format_Profiles_FromPlaylist[Pos])
                    Format_Profile_CoherencyIssue=true;
                if (!Format_Profiles_FromContainer[Pos].empty())
                    Format_Profile_FromContainer_IsPresent=true;
            }
            if (Format_Profile_NotSame)
            {
                MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile");
                for (size_t Pos=0; Pos<Format_Profiles_FromStream.size(); Pos++)
                    MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile", Format_Profiles_FromStream[Pos]);
                MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_EssencesMismatch", "Yes");
            }
            if (Format_Profile_CoherencyIssue)
            {
                MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile_FromStream");
                if (Format_Profile_FromContainer_IsPresent)
                    MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile_FromContainer");
                MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile_FromPlaylist");
                for (size_t Pos=0; Pos<Format_Profiles_FromStream.size(); Pos++)
                {
                    MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_FromStream", Format_Profiles_FromStream[Pos]);
                    if (Format_Profile_FromContainer_IsPresent)
                        MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_FromContainer", Format_Profiles_FromContainer[Pos].empty()?Format_Profiles_FromStream[Pos]:Format_Profiles_FromContainer[Pos]);
                    MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_FromPlaylist", Format_Profiles_FromPlaylist[Pos]);
                }
            }
            else if (Format_Profile_FromContainer_IsPresent)
            {
                MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile_FromStream");
                MI->Clear(StreamKind_Last, StreamPos_To, "Format_Profile_FromContainer");
                for (size_t Pos=0; Pos<Format_Profiles_FromStream.size(); Pos++)
                {
                    MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_FromStream", Format_Profiles_FromStream[Pos]);
                    MI->Fill(StreamKind_Last, StreamPos_To, "Format_Profile_FromContainer", Format_Profiles_FromContainer[Pos].empty()?Format_Profiles_FromStream[Pos]:Format_Profiles_FromContainer[Pos]);
                }
            }
        #endif //MEDIAINFO_ADVANCED

        if (Duration_Temp!=(int64u)-1)
            MI->Fill(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_Duration), Duration_Temp, 10, true);
        if (FrameCount_Temp!=(int64u)-1)
            MI->Fill(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_FrameCount), FrameCount_Temp, 10, true);
        if (StreamSize_Temp!=(int64u)-1)
            MI->Fill(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_StreamSize), StreamSize_Temp, 10, true);
        if (FileSize_Temp!=(int64u)-1)
            Sequences[Sequences_Current]->FileSize=FileSize_Temp;
        if (BitRate_Before && Duration_Temp)
        {
            float64 BitRate_After=((float64)StreamSize_Temp)*8000/Duration_Temp;
            if (BitRate_Before>BitRate_After*0.999 && BitRate_Before<BitRate_After*1.001)
                MI->Fill(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_BitRate), BitRate_Before, 0, true); //In case of similar bitrate, there is great chance hte compute bit rate is different due to aproximation errors only, using the previous one. TODO: find a better way to detect it
        }
    }

    //Frame rate if available from container
    if (StreamKind_Last==Stream_Video && Sequences[Sequences_Current]->FrameRate)
        MI->Fill(Stream_Video, StreamPos_To, Video_FrameRate, Sequences[Sequences_Current]->FrameRate, 3 , true);

    //Hacks - After
    if (!Sequences[Sequences_Current]->IsMain && CodecID!=MI->Retrieve(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_CodecID)))
    {
        if (!CodecID.empty())
            CodecID+=__T(" / ");
        CodecID+=MI->Retrieve(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_CodecID));
        MI->Fill(StreamKind_Last, StreamPos_To, MI->Fill_Parameter(StreamKind_Last, Generic_CodecID), CodecID, true);
    }
    if (!Sequences[Sequences_Current]->IsMain && Sequences[Sequences_Current]->MI->Count_Get(Stream_Video)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Audio)>1 && Sequences[Sequences_Current]->MI->Get(Stream_Video, 0, Video_Format)!=__T("DV") && Sequences[Sequences_Current]->MI->Get(Stream_Audio, 0, Audio_Format)!=__T("Dolby E"))
    {
        if (StreamKind_Last==Stream_Menu)
        {
            ZtringList List; List.Separator_Set(0, __T(" / ")); List.Write(MI->Retrieve(StreamKind_Last, StreamPos_To, "List"));
            ZtringList List_String; List_String.Separator_Set(0, __T(" / ")); List_String.Write(MI->Retrieve(StreamKind_Last, StreamPos_To, "List/String"));
            if (!ID_Base.empty())
                for (size_t Pos=0; Pos<List.size(); Pos++)
                {
                    List[Pos].insert(0, ID_Base+__T("-"));
                    List_String[Pos].insert(0, ID_Base+__T("-"));
                }
            MI->Fill(Stream_Menu, StreamPos_To, Menu_List, List.Read(), true);
            MI->Fill(Stream_Menu, StreamPos_To, Menu_List_String, List_String.Read(), true);
        }
        else if (Sequences.size()>1 && Sequences[Sequences_Current]->MI->Count_Get(Stream_Menu)==0)
        {
            if (Sequences[Sequences_Current]->MenuPos==(size_t)-1)
            {
                Sequences[Sequences_Current]->MenuPos=MI->Stream_Prepare(Stream_Menu);
                MI->Fill(Stream_Menu, Sequences[Sequences_Current]->MenuPos, General_ID, ID_Base);
                MI->Fill(Stream_Menu, Sequences[Sequences_Current]->MenuPos, "Source", Sequences[Sequences_Current]->Source);
            }
            Ztring List=Sequences[Sequences_Current]->MI->Get(StreamKind_Last, StreamPos_From, General_ID);
            Ztring List_String=Sequences[Sequences_Current]->MI->Get(StreamKind_Last, StreamPos_From, General_ID_String);
            if (!ID_Base.empty())
            {
                List.insert(0, ID_Base+__T("-"));
                List_String.insert(0, ID_Base+__T("-"));
            }
            MI->Fill(Stream_Menu, Sequences[Sequences_Current]->MenuPos, Menu_List, List);
            MI->Fill(Stream_Menu, Sequences[Sequences_Current]->MenuPos, Menu_List_String, List_String);
        }
    }
    if (!Sequences[Sequences_Current]->IsMain && (ContainerHasNoId || !Config->File_ID_OnlyRoot_Get() || Sequences[Sequences_Current]->MI->Get(Stream_General, 0, General_Format)==__T("SCC") || Sequences[Sequences_Current]->MI->Count_Get(Stream_Video)+Sequences[Sequences_Current]->MI->Count_Get(Stream_Audio)>1) && !MI->Retrieve(StreamKind_Last, StreamPos_To, General_ID).empty())
    {
        if (!ID.empty())
            ID+=__T('-');
        ID+=MI->Retrieve(StreamKind_Last, StreamPos_To, General_ID);
        if (!ID_String.empty())
            ID_String+=__T('-');
        ID_String+=MI->Retrieve(StreamKind_Last, StreamPos_To, General_ID_String);
        if (!MI->Retrieve(StreamKind_Last, StreamPos_To, "MenuID").empty())
        {
            if (!ID_Base.empty())
                MenuID=ID_Base+__T('-');
            MenuID+=MI->Retrieve(StreamKind_Last, StreamPos_To, "MenuID");
            if (!ID_Base.empty())
                MenuID_String=ID_Base+__T('-');
            MenuID_String+=MI->Retrieve(StreamKind_Last, StreamPos_To, "MenuID/String");
        }
        else if (Sequences[Sequences_Current]->MenuPos!=(size_t)-1)
        {
            MenuID=ID_Base;
            MenuID_String=ID_Base;
        }
    }
    if (!Sequences[Sequences_Current]->IsMain)
    {
        MI->Fill(StreamKind_Last, StreamPos_To, General_ID, ID, true);
        MI->Fill(StreamKind_Last, StreamPos_To, General_ID_String, ID_String, true);
        MI->Fill(StreamKind_Last, StreamPos_To, "MenuID", MenuID, true);
        MI->Fill(StreamKind_Last, StreamPos_To, "MenuID/String", MenuID_String, true);
        if (!MI->Retrieve(StreamKind_Last, StreamPos_To, "Source").empty())
        {
            if (MI->Retrieve(StreamKind_Last, StreamPos_To, "Source_Original").empty() && Sequences[Sequences_Current]->Source!=MI->Retrieve(StreamKind_Last, StreamPos_To, "Source")) // TODO: better handling
            {
                MI->Fill(StreamKind_Last, StreamPos_To, "Source_Original", MI->Retrieve(StreamKind_Last, StreamPos_To, "Source"));
                MI->Fill(StreamKind_Last, StreamPos_To, "Source_Original_Kind", MI->Retrieve(StreamKind_Last, StreamPos_To, "Source_Kind"));
                MI->Fill(StreamKind_Last, StreamPos_To, "Source_Original_Info", MI->Retrieve(StreamKind_Last, StreamPos_To, "Source_Info"));
            }
            MI->Clear(StreamKind_Last, StreamPos_To, "Source");
            MI->Clear(StreamKind_Last, StreamPos_To, "Source_Kind");
            MI->Clear(StreamKind_Last, StreamPos_To, "Source_Info");
        }
        MI->Fill(StreamKind_Last, StreamPos_To, "Source", Sequences[Sequences_Current]->Source);
    }
    for (std::map<string, Ztring>::iterator Info=Sequences[Sequences_Current]->Infos.begin(); Info!=Sequences[Sequences_Current]->Infos.end(); ++Info)
        if (MI->Retrieve(StreamKind_Last, StreamPos_To, Info->first.c_str()).empty())
            MI->Fill(StreamKind_Last, StreamPos_To, Info->first.c_str(), Info->second);

    //Others
    if (!Sequences[Sequences_Current]->IsMain && Sequences[Sequences_Current]->MI->Info && MI->Retrieve(StreamKind_Last, StreamPos_To, Sequences[Sequences_Current]->MI->Info->Fill_Parameter(StreamKind_Last, Generic_Format))!=Sequences[Sequences_Current]->MI->Info->Get(Stream_General, 0, General_Format))
    {
        Ztring MuxingMode=MI->Retrieve(StreamKind_Last, StreamPos_To, "MuxingMode");
        if (!MuxingMode.empty())
            MuxingMode.insert(0, __T(" / "));
        MI->Fill(StreamKind_Last, StreamPos_To, "MuxingMode", Sequences[Sequences_Current]->MI->Info->Get(Stream_General, 0, General_Format)+MuxingMode, true);
    }

    //Lists
    #if MEDIAINFO_ADVANCED || MEDIAINFO_HASH
        if (!Sequences[Sequences_Current]->List_Compute_Done && (Sequences[Sequences_Current]->MI->Count_Get(Stream_Menu)==0 || StreamKind_Last==Stream_Menu))
        {
            List_Compute();
            Sequences[Sequences_Current]->List_Compute_Done=true;
        }
    #endif //MEDIAINFO_ADVANCED || MEDIAINFO_HASH
}

//---------------------------------------------------------------------------
#if MEDIAINFO_ADVANCED || MEDIAINFO_HASH
void File__ReferenceFilesHelper::List_Compute()
{
    stream_t StreamKind=Sequences.size()>1?StreamKind_Last:Stream_General;
    size_t   StreamPos=Sequences.size()>1?StreamPos_To:0;

    stream_t StreamKind_Target=Sequences[Sequences_Current]->MenuPos==(size_t)-1?StreamKind:Stream_Menu;
    size_t   StreamPos_Target=Sequences[Sequences_Current]->MenuPos==(size_t)-1?StreamPos:Sequences[Sequences_Current]->MenuPos;

    //Hash
    #if MEDIAINFO_HASH
        if (!HasMainFile && Config->File_Hash_Get().to_ulong())
        {
            for (size_t Hash_Pos=0; Hash_Pos<HashWrapper::HashFunction_Max; ++Hash_Pos)
            {
            string Hash_Name(HashWrapper::Name((HashWrapper::HashFunction)Hash_Pos));
            Ztring Hash_NameU; Hash_NameU.From_UTF8(Hash_Name.c_str());
            if (!Sequences[Sequences_Current]->MI->Get(Stream_General, 0, Hash_NameU+__T("_Generated")).empty())
            {
                if (Sequences[Sequences_Current]->MI->Config.File_Names.size()==1)
                {
                    if (MI->Retrieve(StreamKind_Target, StreamPos_Target, "Source").empty())
                    {
                        Ztring SourcePath;
                        Ztring SourceName=MI->Retrieve(Stream_General, 0, General_CompleteName);
                        if (SourceName.find(__T("://"))==string::npos)
                            SourcePath=ZenLib::FileName::Path_Get(SourceName);
                        else
                        {
                            size_t Pos_Path=SourceName.find_last_of('/');
                            if (Pos_Path!=Ztring::npos)
                                SourcePath=SourceName.substr(0, Pos_Path);
                        }
                        size_t SourcePath_Size=SourcePath.size()+1; //Path size + path separator size
                        Ztring Temp=Sequences[Sequences_Current]->MI->Config.File_Names[0];
                        if (!Config->File_IsReferenced_Get())
                            Temp.erase(0, SourcePath_Size);
                        MI->Fill(StreamKind_Target, StreamPos_Target, "Source", Temp);
                    }
                    MI->Fill(StreamKind_Target, StreamPos_Target, ("Source_"+Hash_Name+"_Generated").c_str(), Sequences[Sequences_Current]->MI->Get(Stream_General, 0, Hash_NameU+__T("_Generated")));
                    MI->Fill_SetOptions(StreamKind_Target, StreamPos_Target, ("Source_"+Hash_Name+"_Generated").c_str(), "N NT");
                }
                MI->Fill(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), Sequences[Sequences_Current]->MI->Get(Stream_General, 0, Hash_NameU+__T("_Generated")));
                MI->Fill_SetOptions(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), "N NT");
            }
            if (!Sequences[Sequences_Current]->MI->Get(Stream_General, 0, __T("Source_List_")+Hash_NameU+__T("_Generated")).empty())
            {
                MI->Fill(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), Sequences[Sequences_Current]->MI->Get(Stream_General, 0, __T("Source_List_")+Hash_NameU+__T("_Generated")));
                MI->Fill_SetOptions(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), "N NT");
            }
            else if (!Sequences[Sequences_Current]->MI->Get(StreamKind, StreamPos, __T("Source_List_")+Hash_NameU+__T("_Generated")).empty())
            {
                MI->Fill(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), Sequences[Sequences_Current]->MI->Get(StreamKind, StreamPos, __T("Source_List_")+Hash_NameU+__T("_Generated")));
                MI->Fill_SetOptions(StreamKind_Target, StreamPos_Target, ("Source_List_"+Hash_Name+"_Generated").c_str(), "N NT");
            }
            }
        }
    #endif //MEDIAINFO_HASH

    //Source_List
    #if MEDIAINFO_ADVANCED
        if (!HasMainFile && Config->File_Source_List_Get())
        {
            Ztring SourcePath;
            Ztring SourceName=MI->Retrieve(Stream_General, 0, General_CompleteName);
            if (SourceName.find(__T("://"))==string::npos)
                SourcePath=ZenLib::FileName::Path_Get(SourceName);
            else
            {
                size_t Pos_Path=SourceName.find_last_of('/');
                if (Pos_Path!=Ztring::npos)
                    SourcePath=SourceName.substr(0, Pos_Path);
            }
            size_t SourcePath_Size=SourcePath.size()+1; //Path size + path separator size
            for (size_t Pos=0; Pos<Sequences[Sequences_Current]->FileNames.size(); Pos++)
            {
                Ztring Temp=Sequences[Sequences_Current]->FileNames[Pos];
                if (!Config->File_IsReferenced_Get())
                    Temp.erase(0, SourcePath_Size);
                MI->Fill(StreamKind_Target, StreamPos_Target, "Source_List", Temp);
            }
            if (!Sequences[Sequences_Current]->MI->Get(Stream_General, 0, __T("Source_List")).empty())
            {
                ZtringList List;
                List.Separator_Set(0, __T(" / "));
                List.Write(Sequences[Sequences_Current]->MI->Get(Stream_General, 0, __T("Source_List")));
                for (size_t Pos=0; Pos<List.size(); Pos++)
                {
                    Ztring Temp=List[Pos];
                    if (!Config->File_IsReferenced_Get())
                        Temp.erase(0, SourcePath_Size);
                    MI->Fill(StreamKind_Target, StreamPos_Target, "Source_List", Temp);
                }
            }
            MI->Fill_SetOptions(StreamKind_Target, StreamPos_Target, "Source_List", "N NT");
        }
    #endif //MEDIAINFO_ADVANCED
}
#endif //MEDIAINFO_ADVANCED || MEDIAINFO_HASH

//---------------------------------------------------------------------------
MediaInfo_Internal* File__ReferenceFilesHelper::MI_Create()
{
    //Configuration
    MediaInfo_Internal* MI_Temp=new MediaInfo_Internal();
    for (std::map<string, Ztring>::iterator Config_Item=Sequences[Sequences_Current]->Config.begin(); Config_Item!=Sequences[Sequences_Current]->Config.end(); ++Config_Item)
        MI_Temp->Option(Ztring().From_UTF8(Config_Item->first.c_str()), Config_Item->second);
    MI_Temp->Option(__T("File_IsReferenced"), __T("1"));
    MI_Temp->Option(__T("File_FileNameFormat"), __T("CSV"));
    MI_Temp->Option(__T("File_KeepInfo"), __T("1"));
    MI_Temp->Option(__T("File_ID_OnlyRoot"), Config->File_ID_OnlyRoot_Get()?__T("1"):__T("0"));
    #if defined(MEDIAINFO_DVDIF_YES)
    MI_Temp->Option(__T("File_DvDif_DisableAudioIfIsInContainer"), Config->File_DvDif_DisableAudioIfIsInContainer_Get()?__T("1"):__T("0"));
    #endif
    if ((Sequences.size()>1 || Config->File_MpegTs_ForceMenu_Get()) && !Sequences[Sequences_Current]->IsMain && !HasMainFile)
        MI_Temp->Option(__T("File_MpegTs_ForceMenu"), __T("1"));
    #if MEDIAINFO_AES
        MI_Temp->Option(__T("File_Encryption_Format"), MI->Retrieve(Stream_General, 0, "Encryption_Format"));
        MI_Temp->Option(__T("File_Encryption_Key"), Ztring().From_UTF8(Base64::encode(MI->Config->Encryption_Key_Get())));
        MI_Temp->Option(__T("File_Encryption_Method"), MI->Retrieve(Stream_General, 0, "Encryption_Method"));
        MI_Temp->Option(__T("File_Encryption_Mode"), MI->Retrieve(Stream_General, 0, "Encryption_Mode"));
        MI_Temp->Option(__T("File_Encryption_Padding"), MI->Retrieve(Stream_General, 0, "Encryption_Padding"));
        MI_Temp->Option(__T("File_Encryption_InitializationVector"), MI->Retrieve(Stream_General, 0, "Encryption_InitializationVector"));
    #endif //AES
    #if MEDIAINFO_NEXTPACKET
        if (Config->NextPacket_Get())
            MI_Temp->Option(__T("File_NextPacket"), __T("1"));
        if (Config->File_Demux_Interleave_Get())
            MI_Temp->Option(__T("File_Demux_Interleave"), __T("1"));
    #endif //MEDIAINFO_NEXTPACKET
    #if MEDIAINFO_ADVANCED
        if (Config->File_IgnoreSequenceFileSize_Get())
            MI_Temp->Option(__T("File_IgnoreSequenceFileSize"), __T("1"));
        if (Config->File_IgnoreSequenceFilesCount_Get())
            MI_Temp->Option(__T("File_IgnoreSequenceFilesCount"), __T("1"));
        if (Config->File_Source_List_Get())
            MI_Temp->Option(__T("File_Source_List"), __T("1"));
    #endif //MEDIAINFO_ADVANCED
    #if MEDIAINFO_HASH
        if (Config->File_Hash_Get().to_ulong())
            MI_Temp->Option(__T("File_Hash"), Config->Option(__T("File_Hash_Get"), Ztring()));
    #endif //MEDIAINFO_HASH
    #if MEDIAINFO_EVENTS
        MI_Temp->Config.Config_PerPackage=Config->Config_PerPackage;
        if (Config->Event_CallBackFunction_IsSet())
        {
            MI_Temp->Option(__T("File_Event_CallBackFunction"), Config->Event_CallBackFunction_Get());
            MI_Temp->Config.Config_PerPackage->Event_CallBackFunction_Set(Config->Event_CallBackFunction_Get());
        }
        MI_Temp->Config.File_Names_RootDirectory=FileName(MI->File_Name).Path_Get();
        if (!Config->File_TestContinuousFileNames_Get() || Sequences[Sequences_Current]->FileNames.size()>1)
            MI_Temp->Option(__T("File_TestContinuousFileNames"), __T("0"));
        if (!Config->File_TestDirectory_Get())
            MI_Temp->Option(__T("File_TestDirectory"), __T("0"));
        ZtringListList SubFile_IDs;
        if (Sequences[Sequences_Current]->IsMain)
            HasMainFile=true;
        if (HasMainFile && !Sequences[Sequences_Current]->IsMain)
        {
            ZtringList ID;
            ID.push_back(Ztring::ToZtring((((int64u)MediaInfo_Parser_SideCar)<<56)|(Sequences[Sequences_Current]->StreamID-1)));
            ID.push_back(Ztring::ToZtring(16));
            ID.push_back(Ztring::ToZtring(MediaInfo_Parser_SideCar));
            SubFile_IDs.push_back(ID);
        }
        else if (!Sequences[Sequences_Current]->IsMain)
            for (size_t Pos=0; Pos<MI->StreamIDs_Size; Pos++)
            {
                ZtringList ID;
                if (MI->StreamIDs_Width[Pos]==0)
                    ID.push_back(Ztring::ToZtring(-1));
                else if (Pos+1==MI->StreamIDs_Size)
                    ID.push_back(Ztring::ToZtring(Sequences[Sequences_Current]->StreamID));
                else
                    ID.push_back(Ztring::ToZtring(MI->StreamIDs[Pos]));
                ID.push_back(Ztring::ToZtring(MI->StreamIDs_Width[Pos]));
                ID.push_back(Ztring::ToZtring(MI->ParserIDs[Pos]));
                SubFile_IDs.push_back(ID);
            }
        if (!SubFile_IDs.empty())
        {
            SubFile_IDs.Separator_Set(0, EOL);
            SubFile_IDs.Separator_Set(1, __T(","));
            MI_Temp->Option(__T("File_SubFile_IDs_Set"), SubFile_IDs.Read());
        }
    #endif //MEDIAINFO_EVENTS
    if (FrameRate)
        MI_Temp->Option(__T("File_Demux_Rate"), Ztring::ToZtring(FrameRate));
    else if (!Sequences[Sequences_Current]->Resources.empty() && Sequences[Sequences_Current]->Resources[0]->EditRate) //TODO: per Pos
        MI_Temp->Option(__T("File_Demux_Rate"), Ztring::ToZtring(Sequences[Sequences_Current]->Resources[0]->EditRate));
    #if MEDIAINFO_DEMUX
        if (Config->Demux_Unpacketize_Get())
            MI_Temp->Option(__T("File_Demux_Unpacketize"), __T("1"));
        if (Config->Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10_Get())
            MI_Temp->Option(__T("File_Demux_Avc_Transcode_Iso14496_15_to_Iso14496_10"), __T("1"));
        if (Config->Demux_Hevc_Transcode_Iso14496_15_to_AnnexB_Get())
            MI_Temp->Option(__T("File_Demux_Hevc_Transcode_Iso14496_15_to_AnnexB"), __T("1"));
        switch (Config->Demux_InitData_Get())
        {
            case 0 : MI_Temp->Option(__T("File_Demux_InitData"), __T("Event")); break;
            case 1 : MI_Temp->Option(__T("File_Demux_InitData"), __T("Field")); break;
            default: ;
        }
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_IBIUSAGE
        if (!Sequences[Sequences_Current]->IbiStream.Infos.empty())
        {
            ibi Ibi;
            Ibi.Streams[(int64u)-1]=new ibi::stream(Sequences[Sequences_Current]->IbiStream);

            //IBI Creation
            File_Ibi_Creation IbiCreation(Ibi);
            Ztring IbiText=IbiCreation.Finish();
            if (!IbiText.empty())
                MI_Temp->Option(__T("File_Ibi"), IbiText);
        }
    #endif //MEDIAINFO_IBIUSAGE

    return MI_Temp;
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::Read_Buffer_Unsynched()
{
    MI->Open_Buffer_Unsynch();
    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences.size(); Sequences_Pos++)
        if (Sequences[Sequences_Pos]->MI)
            Sequences[Sequences_Pos]->MI->Open_Buffer_Unsynch();

    #if MEDIAINFO_DEMUX
	#if MEDIAINFO_NEXTPACKET
        DTS_Minimal=(int64u)-1;
	#endif
        Config->Demux_EventWasSent=true; //We want not try to read new data from the file
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_EVENTS
        if (Config->Config_PerPackage)
            Config->Config_PerPackage->Unsynch();
    #endif //MEDIAINFO_EVENTS
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
Ztring Duration_Milliseconds2String(int64u DurationM)
{
    Ztring DurationS;
    DurationS+=L'0'+(Char)(DurationM/(10*60*60*1000)); DurationM%=10*60*60*1000;
    DurationS+=L'0'+(Char)(DurationM/(   60*60*1000)); DurationM%=   60*60*1000;
    DurationS+=L':';
    DurationS+=L'0'+(Char)(DurationM/(   10*60*1000)); DurationM%=   10*60*1000;
    DurationS+=L'0'+(Char)(DurationM/(      60*1000)); DurationM%=      60*1000;
    DurationS+=L':';
    DurationS+=L'0'+(Char)(DurationM/(      10*1000)); DurationM%=      10*1000;
    DurationS+=L'0'+(Char)(DurationM/(         1000)); DurationM%=         1000;
    DurationS+=L'.';
    DurationS+=L'0'+(Char)(DurationM/(          100)); DurationM%=          100;
    DurationS+=L'0'+(Char)(DurationM/(           10)); DurationM%=           10;
    DurationS+=L'0'+(Char)(DurationM);
    return DurationS;
}
size_t File__ReferenceFilesHelper::Seek (size_t Method, int64u Value, int64u ID)
{
    for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
        if (Sequences[Sequences_Current]->MI==NULL && !Sequences[Sequences_Current]->FileNames.empty())
            ParseReference_Init();

    //Parsing
    switch (Method)
    {
        case 0  :
                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        {
                        if (Value)
                        {
                            if (Value>MI->Config->File_Size)
                                return 2; //Invalid value

                            //Init
                            if (!Duration)
                            {
                                Ztring FileName;
                                if (HasMainFile)
                                    for (size_t Sequences_Pos = 0; Sequences_Pos < Sequences.size(); Sequences_Pos++)
                                        if (Sequences[Sequences_Pos]->IsMain && !Sequences[Sequences_Pos]->FileNames.empty())
                                            FileName=Sequences[Sequences_Pos]->FileNames[0];
                                if (FileName.empty())
                                    FileName=MI->File_Name;

                                MediaInfo_Internal MI2;
                                MI2.Option(__T("File_KeepInfo"), __T("1"));
                                Ztring ParseSpeed_Save=MI2.Option(__T("ParseSpeed_Get"), __T(""));
                                Ztring Demux_Save=MI2.Option(__T("Demux_Get"), __T(""));
                                MI2.Option(__T("ParseSpeed"), __T("0"));
                                MI2.Option(__T("Demux"), Ztring());
                                size_t MiOpenResult=MI2.Open(FileName);
                                MI2.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
                                MI2.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
                                if (!MiOpenResult)
                                    return (size_t)-1;
                                Duration=MI2.Get(Stream_General, 0, General_Duration).To_float64()/1000;
                            }

                            //Time percentage
                            float64 DurationF=Duration;
                            DurationF*=Value;
                            DurationF/=MI->Config->File_Size;
                            DurationF*=1000;
                            int64u DurationM=(int64u)DurationF;

                            CountOfReferencesToParse=Sequences.size();
                            bool HasProblem=false;
                            for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                            {
                                if (Sequences[Sequences_Current]->MI)
                                {
                                    Ztring Result;
                                    if (Sequences[Sequences_Current]->Resources.size()<=1 || DurationM<Sequences[Sequences_Current]->Resources[1]->Demux_Offset_DTS)
                                    {
                                        Sequences[Sequences_Current]->Resources_Current=0;
                                        int64u DurationFTemp=DurationF;
                                        if (Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                            DurationFTemp+=Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream/1000000; // From nanoseconds to milliseconds
                                        Ztring DurationS=Duration_Milliseconds2String((int64u)DurationFTemp);
                                        Result=Sequences[Sequences_Current]->MI->Option(__T("File_Seek"), DurationS);
                                    }
                                    else
                                    {
                                        size_t Resources_Current_Temp=1;
                                        while (Resources_Current_Temp<Sequences[Sequences_Current]->Resources.size() && DurationM>=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->Demux_Offset_DTS)
                                            Resources_Current_Temp++;
                                        Resources_Current_Temp--;
                                        int64u DurationFTemp=DurationF;
                                        if (Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                            DurationFTemp+=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream/1000000; // From nanoseconds to milliseconds
                                        Ztring DurationS=Duration_Milliseconds2String((int64u)DurationFTemp);
                                        Result=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Option(__T("File_Seek"), DurationS);
                                        if (Result.empty())
                                            Sequences[Sequences_Current]->Resources_Current=Resources_Current_Temp;
                                    }
                                    if (!Result.empty())
                                        HasProblem=true;
                                }
                                Sequences[Sequences_Current]->Status.reset();
                            }
                            Sequences_Current=0;
                            Open_Buffer_Unsynch();
                            return HasProblem?(size_t)-1:1; //Not supported value if there is a problem (TODO: better info)
                        }

                        CountOfReferencesToParse=Sequences.size();
                        bool HasProblem=false;
                        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                        {
                            if (Sequences[Sequences_Current]->MI)
                            {
                                Sequences[Sequences_Current]->Resources_Current=0;
                                Ztring Result=Sequences[Sequences_Current]->MI->Option(__T("File_Seek"), Ztring::ToZtring(Value));
                                if (!Result.empty())
                                    HasProblem=true;
                            }
                            Sequences[Sequences_Current]->Status.reset();
                        }
                        Sequences_Current=0;
                        Open_Buffer_Unsynch();
                        return HasProblem?(size_t)-1:1; //Not supported value if there is a problem (TODO: better info)
                        }
                    #else //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
        case 1  :
                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    {
                        //Init
                        if (!Duration)
                        {
                            Ztring FileName;
                            if (HasMainFile)
                                for (size_t Sequences_Pos = 0; Sequences_Pos < Sequences.size(); Sequences_Pos++)
                                    if (Sequences[Sequences_Pos]->IsMain && !Sequences[Sequences_Pos]->FileNames.empty())
                                        FileName=Sequences[Sequences_Pos]->FileNames[0];
                            if (FileName.empty())
                                FileName=MI->File_Name;

                            MediaInfo_Internal MI2;
                            MI2.Option(__T("File_KeepInfo"), __T("1"));
                            Ztring ParseSpeed_Save=MI2.Option(__T("ParseSpeed_Get"), __T(""));
                            Ztring Demux_Save=MI2.Option(__T("Demux_Get"), __T(""));
                            MI2.Option(__T("ParseSpeed"), __T("0"));
                            MI2.Option(__T("Demux"), Ztring());
                            size_t MiOpenResult=MI2.Open(FileName);
                            MI2.Option(__T("ParseSpeed"), ParseSpeed_Save); //This is a global value, need to reset it. TODO: local value
                            MI2.Option(__T("Demux"), Demux_Save); //This is a global value, need to reset it. TODO: local value
                            if (!MiOpenResult)
                                return (size_t)-1;
                            Duration=MI2.Get(Stream_General, 0, General_Duration).To_float64()/1000;
                        }

                        //Time percentage
                        float64 DurationF=Duration;
                        DurationF*=Value;
                        DurationF/=10; // divided by 10000 for 0.01 percentage then x1000 for milliseconds

                        CountOfReferencesToParse=Sequences.size();
                        bool HasProblem=false;
                        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                        {
                            if (Sequences[Sequences_Current]->MI)
                            {
                                Ztring Result;
                                if (Sequences[Sequences_Current]->Resources.size()<2 || Duration<Sequences[Sequences_Current]->Resources[1]->Demux_Offset_DTS)
                                {
                                    Sequences[Sequences_Current]->Resources_Current=0;
                                    int64u DurationFTemp=DurationF;
                                    if (Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                        DurationFTemp+=Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream/1000000; // From nanoseconds to milliseconds
                                    Ztring DurationS=Duration_Milliseconds2String((int64u)DurationFTemp);
                                    Result=Sequences[Sequences_Current]->MI->Option(__T("File_Seek"), DurationS);
                                }
                                else
                                {
                                    size_t Resources_Current_Temp=1;
                                    while (Resources_Current_Temp<Sequences[Sequences_Current]->Resources.size() && Duration>=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->Demux_Offset_DTS)
                                        Resources_Current_Temp++;
                                    Resources_Current_Temp--;
                                    int64u DurationFTemp=DurationF;
                                    if (Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                        DurationFTemp+=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream/1000000; // From nanoseconds to milliseconds
                                    Ztring DurationS=Duration_Milliseconds2String((int64u)DurationFTemp);
                                    Result=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Option(__T("File_Seek"), DurationS);
                                    if (Result.empty())
                                        Sequences[Sequences_Current]->Resources_Current=Resources_Current_Temp;
                                }
                                if (!Result.empty())
                                    HasProblem=true;
                            }
                            Sequences[Sequences_Current]->Status.reset();
                        }
                        Sequences_Current=0;
                        Open_Buffer_Unsynch();
                        return HasProblem?2:1; //Invalid value if there is a problem (TODO: better info)
                    }
                    #else //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
        case 2  :   //Timestamp
                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                    {
                        CountOfReferencesToParse=Sequences.size();
                        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                        {
                            if (Sequences[Sequences_Current]->MI)
                            {
                                Ztring Result;
                                if (Sequences[Sequences_Current]->Resources.size()<2 || Value<Sequences[Sequences_Current]->Resources[1]->Demux_Offset_DTS)
                                {
                                    Sequences[Sequences_Current]->Resources_Current=0;
                                    int64u ValueTemp=Value;
                                    if (Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                        ValueTemp+=Sequences[Sequences_Current]->MI->Config.Demux_Offset_DTS_FromStream;
                                    Ztring Time; Time.Duration_From_Milliseconds(ValueTemp/1000000);
                                    Result=Sequences[Sequences_Current]->MI->Option(__T("File_Seek"), Time);
                                }
                                else
                                {
                                    size_t Resources_Current_Temp=1;
                                    while (Resources_Current_Temp<Sequences[Sequences_Current]->Resources.size() && Value>=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->Demux_Offset_DTS)
                                        Resources_Current_Temp++;
                                    Resources_Current_Temp--;
                                    int64u ValueTemp=Value;
                                    if (Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream!=(int64u)-1)
                                        ValueTemp+=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Config.Demux_Offset_DTS_FromStream;
                                    Ztring Time; Time.Duration_From_Milliseconds(ValueTemp/1000000);
                                    Result=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Option(__T("File_Seek"), Time);
                                    if (Result.empty())
                                        Sequences[Sequences_Current]->Resources_Current=Resources_Current_Temp;
                                }
                                if (!Result.empty())
                                    return 2; //Invalid value
                            }
                            else
                            {
                                //There was a problem, removing Sequence
                                Sequences.clear();
                                return Seek(Method, Value, ID);
                            }
                            Sequences[Sequences_Current]->Status.reset();
                        }
                        Sequences_Current=0;
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    #else //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
        case 3  :   //FrameNumber
                    #if MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        CountOfReferencesToParse=Sequences.size();
                        for (Sequences_Current=0; Sequences_Current<Sequences.size(); Sequences_Current++)
                        {
                            if (Sequences[Sequences_Current]->MI)
                            {
                                    Ztring Result;
                                    if (Sequences[Sequences_Current]->Resources.size()<2 || Value<Sequences[Sequences_Current]->Resources[1]->Demux_Offset_Frame)
                                    {
                                        Sequences[Sequences_Current]->Resources_Current=0;
                                        Result=Sequences[Sequences_Current]->MI->Option(__T("File_Seek"), __T("Frame=")+Ztring::ToZtring(Value));
                                    }
                                    else
                                    {
                                        size_t Resources_Current_Temp=1;
                                        while (Resources_Current_Temp<Sequences[Sequences_Current]->Resources.size() && Value>=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->Demux_Offset_Frame)
                                            Resources_Current_Temp++;
                                        Resources_Current_Temp--;
                                        Result=Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->MI->Option(__T("File_Seek"), __T("Frame=")+Ztring::ToZtring(Value-Sequences[Sequences_Current]->Resources[Resources_Current_Temp]->Demux_Offset_Frame));
                                        if (Result.empty())
                                            Sequences[Sequences_Current]->Resources_Current=Resources_Current_Temp;
                                    }
                                if (!Result.empty())
                                    return 2; //Invalid value
                            }
                            Sequences[Sequences_Current]->Status.reset();
                        }
                        Sequences_Current=0;
                        Open_Buffer_Unsynch();
                        return 1;
                    #else //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX && MEDIAINFO_NEXTPACKET
         default :   return 0;
    }
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
size_t File__ReferenceFilesHelper::Stream_Prepare (stream_t StreamKind, size_t StreamPos)
{
    size_t StreamPos_Last=MI->Stream_Prepare(StreamKind, StreamPos);

    for (sequences::iterator ReferencePos=Sequences.begin(); ReferencePos!=Sequences.end(); ++ReferencePos)
        if ((*ReferencePos)->StreamKind==StreamKind && (*ReferencePos)->StreamPos>=StreamPos_Last)
            if ((*ReferencePos)->StreamPos!=(size_t)-1)
                (*ReferencePos)->StreamPos++;

    return StreamPos_Last;
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::FileSize_Compute ()
{
    if (MI->Config==NULL)
        return;

    MI->Config->File_Size=MI->File_Size;

    for (size_t Sequences_Pos=0; Sequences_Pos<Sequences.size(); Sequences_Pos++)
    {
        if (Sequences[Sequences_Pos]->FileSize!=(int64u)-1)
            MI->Config->File_Size+=Sequences[Sequences_Pos]->FileSize;
        else if (Sequences[Sequences_Pos]->MI && Sequences[Sequences_Pos]->MI->Config.File_Size!=(int64u)-1)
        {
            MI->Config->File_Size+=Sequences[Sequences_Pos]->MI->Config.File_Size;
            #if MEDIAINFO_ADVANCED
                if (!Config->File_IgnoreSequenceFileSize_Get())
            #endif //MEDIAINFO_ADVANCED
                {
                    if (!Sequences[Sequences_Pos]->Resources.empty())
                        for (size_t Pos=1; Pos<Sequences[Sequences_Pos]->Resources.size(); Pos++)
                            for (size_t Resource_FileNames_Pos=0; Resource_FileNames_Pos<Sequences[Sequences_Pos]->Resources[Pos]->FileNames.size(); Resource_FileNames_Pos++)
                                MI->Config->File_Size+=File::Size_Get(Sequences[Sequences_Pos]->Resources[Pos]->FileNames[Resource_FileNames_Pos]);
                }
        }
        else
        {
            #if MEDIAINFO_ADVANCED
                if (!Config->File_IgnoreSequenceFileSize_Get())
            #endif //MEDIAINFO_ADVANCED
                {
                    if (Sequences[Sequences_Pos]->Resources.empty())
                        for (size_t Pos=0; Pos<Sequences[Sequences_Pos]->FileNames.size(); Pos++)
                            MI->Config->File_Size+=File::Size_Get(Sequences[Sequences_Pos]->FileNames[Pos]);
                    else
                        for (size_t Pos=0; Pos<Sequences[Sequences_Pos]->Resources.size(); Pos++)
                            for (size_t Resource_FileNames_Pos=0; Resource_FileNames_Pos<Sequences[Sequences_Pos]->Resources[Pos]->FileNames.size(); Resource_FileNames_Pos++)
                                MI->Config->File_Size+=File::Size_Get(Sequences[Sequences_Pos]->Resources[Pos]->FileNames[Resource_FileNames_Pos]);
                }
        }
    }
}

//---------------------------------------------------------------------------
void File__ReferenceFilesHelper::CountOfReferences_ForReadSize_Run ()
{
    //Computing read buffer size
    int64u  File_Size_Total=0;
    int64u  Buffer_Read_Size_Total=MI->Config->File_Buffer_Read_Size_Get();
    for (sequences::iterator Reference_Temp=Sequences.begin(); Reference_Temp!=Sequences.end(); ++Reference_Temp)
        if ((*Reference_Temp)->MI && (*Reference_Temp)->MI->Config.File_Size!=(int64u)-1)
            File_Size_Total+=(*Reference_Temp)->MI->Config.File_Size;
    if (File_Size_Total)
        for (sequences::iterator Reference_Temp=Sequences.begin(); Reference_Temp!=Sequences.end(); ++Reference_Temp)
            if ((*Reference_Temp)->MI)
            {
                int64u  Buffer_Read_Size_Temp=float64_int64s(((float64)(*Reference_Temp)->MI->Config.File_Size)/File_Size_Total*Buffer_Read_Size_Total);
                int64u  Buffer_Read_Size=1;
                while (Buffer_Read_Size<Buffer_Read_Size_Temp)
                    Buffer_Read_Size<<=1;
                (*Reference_Temp)->MI->Config.File_Buffer_Read_Size_Set((size_t)Buffer_Read_Size);
            }
}

//---------------------------------------------------------------------------
#if MEDIAINFO_EVENTS
void File__ReferenceFilesHelper::SubFile_Start()
{
    if (Sequences[Sequences_Current]->StreamID!=StreamID_Previous)
    {
        Ztring FileName_Absolute, FileName_Relative;
        if (Sequences[Sequences_Current]->MI && Sequences[Sequences_Current]->MI->Config.File_Names_Pos && Sequences[Sequences_Current]->MI->Config.File_Names_Pos<Sequences[Sequences_Current]->MI->Config.File_Names.size())
            FileName_Absolute=Sequences[Sequences_Current]->MI->Config.File_Names[Sequences[Sequences_Current]->MI->Config.File_Names_Pos-1];
        else if (!Sequences[Sequences_Current]->FileNames.empty())
            FileName_Absolute=Sequences[Sequences_Current]->FileNames[0];
        else
            FileName_Absolute=Sequences[Sequences_Current]->Source.c_str();

        Sequences[Sequences_Current]->MI->Config.Event_SubFile_Start(FileName_Absolute);
        StreamID_Previous=Sequences[Sequences_Current]->StreamID;
    }
}
#endif //MEDIAINFO_EVENTS

} //NameSpace

#endif //MEDIAINFO_REFERENCES_YES
