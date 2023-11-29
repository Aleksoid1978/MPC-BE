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
#if defined(MEDIAINFO_PDF_YES) || defined(MEDIAINFO_PNG_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Xmp.h"
#include <cstring>
#include "tinyxml2.h"
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Xmp::FileHeader_Begin()
{
    auto Buffer_Size_Save=Buffer_Size;
    if (Buffer_Size>=32)
    {
        //TinyXML2 seems to not like preprocessor commands at end of file. TODO: remove that after switch to another XML parser
        auto Result=strstr((const char*)Buffer+Buffer_Size-32, "<?xpacket");
        if (Result)
            Buffer_Size=(const int8u*)Result-Buffer;
    }

    XMLDocument document;
    auto Result=FileHeader_Begin_XML(document);
    Buffer_Size=Buffer_Size_Save;
    if (!Result)
       return false;

    std::string NameSpace;
    XMLElement* XmpMeta=document.FirstChildElement("xmpmeta");
    if (XmpMeta==NULL)
    {
        NameSpace="x:";
        XmpMeta=document.FirstChildElement((NameSpace+"xmpmeta").c_str());
    }
    if (!XmpMeta)
    {
        Reject("XMP");
        return false;
    }

    XMLElement* Rdf=XmpMeta->FirstChildElement("RDF");
    if (Rdf==NULL)
    {
        NameSpace="rdf:";
        Rdf=XmpMeta->FirstChildElement((NameSpace+"RDF").c_str());
    }
    if (!Rdf)
    {
        Reject("XMP");
        return false;
    }

    Accept("XMP");

    for (XMLElement* Rdf_Item=Rdf->FirstChildElement(); Rdf_Item; Rdf_Item=Rdf_Item->NextSiblingElement())
    {
        //RDF item
        if (!strcmp(Rdf_Item->Value(), (NameSpace+"Description").c_str()))
        {
            const char* Description=Rdf_Item->Attribute("xmp:Description");
            if (!Description)
                Description=Rdf_Item->Attribute("pdf:Description");
            if (Description && *Description!='\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Description, Description);
            const char* Keywords=Rdf_Item->Attribute("xmp:Keywords");
            if (!Keywords)
                Keywords=Rdf_Item->Attribute("pdf:Keywords");
            if (Keywords && *Keywords !='\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Keywords, Keywords);
            const char* Producer=Rdf_Item->Attribute("xmp:Producer");
            if (!Producer)
                Producer=Rdf_Item->Attribute("pdf:Producer");
            if (Producer && *Producer!='\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Encoded_Library, Producer);
            const char* Attribute=Rdf_Item->Attribute("xmlns:pdfaid");
            if (Attribute)
            {
                string Profile;

                if (strcmp(Attribute, "http://www.aiim.org/pdfa/ns/id/"))
                    Profile=Attribute;
                else
                {
                    Profile+="A";

                    Attribute=Rdf_Item->Attribute("pdfaid:part");
                    if (Attribute)
                    {
                        Profile+='-';
                        Profile+=Attribute;

                        Attribute=Rdf_Item->Attribute("pdfaid:conformance");
                        if (Attribute)
                        {
                            string Conformance(Attribute);
                            if (Conformance.size()==1 && Conformance[0]>='A' && Conformance[0]<='Z')
                                Conformance[0]+=0x20; // From "A" to "a"
                            Profile+=Conformance;
                        }
                    }
                }

                Fill(Stream_General, 0, General_Format_Profile, Profile);
            }
            Ztring ModifyDate, CreateDate;
            for (XMLElement* Description_Item = Rdf_Item->FirstChildElement(); Description_Item; Description_Item=Description_Item->NextSiblingElement())
            {
                if (!strcmp(Description_Item->Value(), "xmp:ModifyDate"))
                {
                    ModifyDate=Ztring().From_UTF8(Description_Item->GetText());
                    if (ModifyDate>CreateDate)
                        Fill(Stream_General, 0, General_Encoded_Date, ModifyDate, true);
                }
                else if (!strcmp(Description_Item->Value(), "xmp:CreatorTool"))
                    Fill(Stream_General, 0, General_Encoded_Application, Description_Item->GetText());
                else if (!strcmp(Description_Item->Value(), "xmp:CreateDate"))
                {
                    CreateDate=Ztring().From_UTF8(Description_Item->GetText());
                    if (CreateDate>ModifyDate)
                        Fill(Stream_General, 0, General_Encoded_Date, CreateDate, true);
                }
            }
        }
    }

    Finish();
    return true;
}

} //NameSpace

#endif //MEDIAINFO_PDF_YES
