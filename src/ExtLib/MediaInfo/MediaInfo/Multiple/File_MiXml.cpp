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
#if defined(MEDIAINFO_MIXML_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_MiXml.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
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
File_MiXml::File_MiXml()
:File__Analyze()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_MiXml;
    #endif //MEDIAINFO_EVENTS
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MiXml::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    {
        XMLElement* Root=document.FirstChildElement("MediaInfo");
        if (Root)
        {
            const char* Attribute = Root->Attribute("xmlns");
            if (Attribute == NULL || Ztring().From_UTF8(Attribute) != __T("https://mediaarea.net/mediainfo"))
            {
                Reject("MiXml");
                return false;
            }

            Accept("MiXml");

            XMLElement* Media = Root->FirstChildElement();
            while (Media)
            {
                if (string(Media->Value()) == "media")
                {
                    Attribute = Media->Attribute("ref");
                    if (Attribute)
                    {
                        File_Name.From_UTF8(Attribute);
                        Config->File_Names.clear();
                        
                        Fill(Stream_General, 0, General_CompleteName, File_Name, true); //TODO: merge with generic code
                        Fill(Stream_General, 0, General_FolderName, FileName::Path_Get(File_Name), true);
                        Fill(Stream_General, 0, General_FileName, FileName::Name_Get(File_Name), true);
                        Fill(Stream_General, 0, General_FileExtension, FileName::Extension_Get(File_Name), true);
                        if (Retrieve(Stream_General, 0, General_FileExtension).empty())
                            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName), true);
                        else
                            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName)+__T('.')+Retrieve(Stream_General, 0, General_FileExtension), true);
                    }

                    XMLElement* Track = Media->FirstChildElement();
                    while (Track)
                    {
                        if (string(Track->Value()) == "track")
                        {
                            Attribute = Track->Attribute("type");
                            if (Attribute)
                            {
                                string StreamKind(Attribute);

                                StreamKind_Last = Stream_Max;
                                if (StreamKind == "General")
                                    StreamKind_Last = Stream_General;
                                if (StreamKind == "Video")
                                    Stream_Prepare(Stream_Video);
                                if (StreamKind == "Audio")
                                    Stream_Prepare(Stream_Audio);
                                if (StreamKind == "Text")
                                    Stream_Prepare(Stream_Text);
                                if (StreamKind == "Other")
                                    Stream_Prepare(Stream_Other);
                                if (StreamKind == "Image")
                                    Stream_Prepare(Stream_Image);
                                if (StreamKind == "Menu")
                                    Stream_Prepare(Stream_Menu);

                                if (StreamKind_Last != Stream_Max)
                                {
                                    XMLElement* Element = Track->FirstChildElement();
                                    while (Element)
                                    {
                                        string Name(Element->Name());
                                             if (Name == "Format_Version")
                                            Fill(StreamKind_Last, StreamPos_Last, Element->Name(), string("Version ")+Element->GetText(), true, true);
                                        else if (MediaInfoLib::Config.Info_Get(StreamKind_Last).Read(Ztring().From_UTF8(Element->Name()), Info_Measure) == __T(" ms"))
                                        {
                                            //Converting seconds to milliseconds while keeping precision
                                            Ztring N;
                                            N.From_UTF8(Element->GetText());
                                            size_t Dot = N.find('.');
                                            size_t Precision = 0;
                                            if (Dot != string::npos)
                                            {
                                                size_t End = N.find_first_not_of(__T("0123456789"), Dot + 1);
                                                if (End == string::npos)
                                                    End = N.size();
                                                Precision = End - (Dot + 1);
                                                if (Precision <= 3)
                                                    Precision = 0;
                                                else
                                                    Precision -= 3;
                                            }
                                            
                                            Fill(StreamKind_Last, StreamPos_Last, Element->Name(), N.To_float64()*1000, Precision, true);
                                        }
                                        else if (Name != "extra")
                                            Fill(StreamKind_Last, StreamPos_Last, Element->Name(), Element->GetText(), Unlimited, true, true);
                                        else
                                        {
                                            XMLElement* Extra = Element->FirstChildElement();
                                            while (Extra)
                                            {
                                                Fill(StreamKind_Last, StreamPos_Last, Extra->Name(), Extra->GetText(), Unlimited, true, true);

                                                Extra = Extra->NextSiblingElement();
                                            }
                                        }

                                        // Extra filling (duplicated content) //TODO: better handling of all such fields
                                             if (Name == "Format_Settings_Endianness")
                                            Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", Element->GetText());
                                        else if (Name == "Format_Settings_Packing")
                                            Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", Element->GetText());

                                        Element = Element->NextSiblingElement();
                                    }
                                }
                            }
                        }

                        Track = Track->NextSiblingElement();
                    }
                }

                Media = Media->NextSiblingElement();
            }
        }
        else
        {
            Reject("MiXml");
            return false;
        }
    }

    Element_Offset=File_Size;

    //All should be OK...
    return true;
}

} //NameSpace

#endif //MEDIAINFO_MiXml_YES

