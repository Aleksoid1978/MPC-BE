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
#if defined(MEDIAINFO_ISM_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Ism.h"
#include <set>
#include "MediaInfo/MediaInfo.h"
#include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#include "ZenLib/FileName.h"
#include "ZenLib/Format/Http/Http_Utils.h"
#include "tinyxml2.h"
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Ism::File_Ism()
:File__Analyze(), File__HasReferences()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Ism;
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
void File_Ism::Streams_Accept()
{
    Fill(Stream_General, 0, General_Format, "ISM");
    ReferenceFiles_Accept(this, Config);
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Ism::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    {
        XMLElement* Root=document.FirstChildElement("smil");
        if (Root)
        {
            #if defined(MEDIAINFO_REFERENCES_YES)
            std::set<Ztring> FileNames;
            #endif //MEDIAINFO_REFERENCES_YES

            XMLElement* Body=Root->FirstChildElement();
            while (Body)
            {
                if (string(Body->Value())=="body")
                {
                    XMLElement* Switch=Body->FirstChildElement();
                    while (Switch)
                    {
                        if (string(Switch->Value())=="switch")
                        {
                            Accept("ISM");

                            #if defined(MEDIAINFO_REFERENCES_YES)
                            XMLElement* Stream=Switch->FirstChildElement();
                            while (Stream)
                            {
                                string Value(Stream->Value());
                                if (Value=="video" || Value=="videostream" || Value=="audio" || Value=="audiostream" || Value=="text" || Value=="textstream")
                                {
                                    sequence* Sequence=new sequence;

                                    if (Value=="video" || Value=="videostream")
                                        Sequence->StreamKind=Stream_Video;
                                    if (Value=="audio" || Value=="audiostream")
                                        Sequence->StreamKind=Stream_Audio;
                                    if (Value=="text"  || Value=="textstream" )
                                        Sequence->StreamKind=Stream_Text;

                                    const char* Attribute=Stream->Attribute("src");
                                    if (Attribute)
                                        Sequence->AddFileName(Ztring().From_UTF8(Attribute));

                                    XMLElement* Param=Stream->FirstChildElement();
                                    while (Param)
                                    {
                                        if (string(Param->Value())=="param")
                                        {
                                            Attribute=Param->Attribute("name");
                                            if (Attribute && Ztring().From_UTF8(Attribute)==__T("trackID"))
                                            {
                                                Attribute=Param->Attribute("value");
                                                if (Attribute)
                                                    Sequence->StreamID=Ztring().From_UTF8(Attribute).To_int64u();
                                            }
                                        }
                                        Param=Param->NextSiblingElement();
                                    }

                                    if (!Sequence->FileNames.empty() && !Sequence->FileNames[0].empty() && FileNames.find(Sequence->FileNames[0])==FileNames.end())
                                    {
                                        ReferenceFiles->AddSequence(Sequence);
                                        FileNames.insert(Sequence->FileNames[0]);
                                    }
                                }

                                Stream=Stream->NextSiblingElement();
                            }
                            #endif //MEDIAINFO_REFERENCES_YES
                        }

                        Switch=Switch->NextSiblingElement();
                    }
                }

                Body=Body->NextSiblingElement();
            }
        }
        else
        {
            Reject("ISM");
            return false;
        }
    }

    Element_Offset=File_Size;

    //All should be OK...
    return true;
}

} //NameSpace

#endif //MEDIAINFO_ISM_YES

