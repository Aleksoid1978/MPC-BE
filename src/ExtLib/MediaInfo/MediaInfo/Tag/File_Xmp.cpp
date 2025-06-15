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
#if defined(MEDIAINFO_XMP_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_Xmp.h"
#include "ThirdParty/base64/base64.h"
#include <cstring>
#include "tinyxml2.h"
using namespace tinyxml2;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
static char* strnstr(const char* Str, size_t Size, const char* ToSearch)
{
    size_t ToSearch_Size = strlen(ToSearch);
    if (ToSearch_Size == 0)
        return (char *)Str;

    if (ToSearch_Size > Size)
        return NULL;

    const char* LastPos = (const char *)Str + Size - ToSearch_Size;
    for (const char* Start = Str; Start <= LastPos; Start++)
        if (Start[0] == ToSearch[0] && !memcmp((const void*)&Start[1], (const void*)&ToSearch[1], ToSearch_Size - 1))
            return (char*)Start;

    return NULL;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Xmp::FileHeader_Begin()
{
    if (Wait) {
        Element_WaitForMoreData();
        return false;
    }

    auto Buffer_Size_Save=Buffer_Size;
    if (Buffer_Size>=32)
    {
        //TinyXML2 seems to not like preprocessor commands at end of file. TODO: remove that after switch to another XML parser
        auto Result=strnstr((const char*)Buffer+Buffer_Size-32, 32, "<?xpacket");
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

    auto ParseBase64Image = [this](const char* input_data, const char* muxing_mode, const char* description) -> void {
        if (input_data) {
            std::string Data_Raw(Base64::decode(input_data));
            auto Buffer_Save = Buffer;
            auto Buffer_Offset_Save = Buffer_Offset;
            auto Buffer_Size_Save = Buffer_Size;
            auto Element_Offset_Save = Element_Offset;
            auto Element_Size_Save = Element_Size;
            Buffer = (const int8u*)Data_Raw.c_str();
            Buffer_Offset = 0;
            Buffer_Size = Data_Raw.size();
            Element_Offset = 0;
            Element_Size = Buffer_Size;

            //Filling
            Attachment(muxing_mode, Ztring(), description);

            Buffer = Buffer_Save;
            Buffer_Offset = Buffer_Offset_Save;
            Buffer_Size = Buffer_Size_Save;
            Element_Offset = Element_Offset_Save;
            Element_Size = Element_Size_Save;
        }
        };

    for (XMLElement* Rdf_Item=Rdf->FirstChildElement(); Rdf_Item; Rdf_Item=Rdf_Item->NextSiblingElement())
    {
        //RDF item
        if (!strcmp(Rdf_Item->Value(), (NameSpace+"Description").c_str()))
        {
            const char* RelitInputImageData = Rdf_Item->Attribute("GCamera:RelitInputImageData");
            ParseBase64Image(RelitInputImageData, "Extended XMP / GCamera", "Relit Input Image");
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
            const char* Credit = Rdf_Item->Attribute("photoshop:Credit");
            if (Credit && *Credit != '\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Copyright, Credit);
            const char* DigitalSourceType = Rdf_Item->Attribute("Iptc4xmpExt:DigitalSourceType");
            if (DigitalSourceType && *DigitalSourceType != '\\') { //TODO: support octal and UTF-16 ("\376\377")
                string URI{ DigitalSourceType };
                string::size_type pos = URI.find("https://");
                if (pos != std::string::npos) URI.replace(pos, 5, "http"); // Some Google generated files have https instead of http
                if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/trainedAlgorithmicMedia"))
                    Fill(Stream_General, 0, General_Copyright, "Created using generative AI");
                if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/compositeWithTrainedAlgorithmicMedia"))
                    Fill(Stream_General, 0, General_Copyright, "Edited using generative AI");
            }
            const char* DateTimeOriginal = Rdf_Item->Attribute("exif:DateTimeOriginal");
            if (DateTimeOriginal && *DateTimeOriginal != '\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Recorded_Date, DateTimeOriginal);
            const char* CreatorTool=Rdf_Item->Attribute("xmp:CreatorTool");
            if (CreatorTool && *CreatorTool!='\\') //TODO: support octal and UTF-16 ("\376\377")
                Fill(Stream_General, 0, General_Encoded_Application, CreatorTool);
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
                else if (!strcmp(Description_Item->Value(), "GDepth:Data"))
                {
                    const char* GDepth = Description_Item->GetText();
                    ParseBase64Image(GDepth, "Extended XMP / GDepth Data", "Depth Image");
                }
                else if (!strcmp(Description_Item->Value(), "GDepth:Confidence"))
                {
                    const char* GDepth = Description_Item->GetText();
                    ParseBase64Image(GDepth, "Extended XMP / GDepth Confidence", "Confidence Image");
                }
                else if (!strcmp(Description_Item->Value(), "GImage:Data"))
                {
                    const char* GImage = Description_Item->GetText();
                    ParseBase64Image(GImage, "Extended XMP / GImage Data", "Image");
                }
                else if (!strcmp(Description_Item->Value(), "photoshop:Credit"))
                {
                    Fill(Stream_General, 0, General_Copyright, Description_Item->GetText());
                }
                else if (!strcmp(Description_Item->Value(), "Iptc4xmpExt:DigitalSourceType"))
                {
                    string URI{ Description_Item->GetText() };
                    string::size_type pos = URI.find("https://");
                    if (pos != std::string::npos) URI.replace(pos, 5, "http"); // Some Google generated files have https instead of http
                    if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/trainedAlgorithmicMedia"))
                        Fill(Stream_General, 0, General_Copyright, "Created using generative AI");
                    if (!strcmp(URI.c_str(), "http://cv.iptc.org/newscodes/digitalsourcetype/compositeWithTrainedAlgorithmicMedia"))
                        Fill(Stream_General, 0, General_Copyright, "Edited using generative AI");
                }
                else if (!strcmp(Description_Item->Value(), "dc:title"))
                {
                    XMLElement* rdfAltElement = Description_Item->FirstChildElement("rdf:Alt");
                    if (rdfAltElement)
                        XMLElement* rdfLiElement = rdfAltElement->FirstChildElement("rdf:li");
                        for (XMLElement* rdfLiElement = rdfAltElement->FirstChildElement("rdf:li"); rdfLiElement; rdfLiElement = rdfLiElement->NextSiblingElement("rdf:li"))
                            Fill(Stream_General, 0, General_Title, rdfLiElement->GetText());
                }
                else if (!strcmp(Description_Item->Value(), "dc:description"))
                {
                    XMLElement* rdfAltElement = Description_Item->FirstChildElement("rdf:Alt");
                    if (rdfAltElement)
                        for(XMLElement* rdfLiElement = rdfAltElement->FirstChildElement("rdf:li"); rdfLiElement; rdfLiElement = rdfLiElement->NextSiblingElement("rdf:li"))
                            Fill(Stream_General, 0, General_Description, rdfLiElement->GetText());
                }
                else if (!strcmp(Description_Item->Value(), "dc:subject"))
                {
                    XMLElement* rdfBagElement = Description_Item->FirstChildElement("rdf:Bag");
                    if (rdfBagElement)
                        for (XMLElement* rdfLiElement = rdfBagElement->FirstChildElement("rdf:li"); rdfLiElement; rdfLiElement = rdfLiElement->NextSiblingElement("rdf:li"))
                            Fill(Stream_General, 0, General_Subject, rdfLiElement->GetText());
                }
                else if (!strcmp(Description_Item->Value(), "dc:creator"))
                {
                    XMLElement* rdfSeqElement = Description_Item->FirstChildElement("rdf:Seq");
                    if (rdfSeqElement)
                        for (XMLElement* rdfLiElement = rdfSeqElement->FirstChildElement("rdf:li"); rdfLiElement; rdfLiElement = rdfLiElement->NextSiblingElement("rdf:li"))
                            Fill(Stream_General, 0, General_Producer, rdfLiElement->GetText());
                }
                else if (!strcmp(Description_Item->Value(), "dc:rights"))
                {
                    XMLElement* rdfAltElement = Description_Item->FirstChildElement("rdf:Alt");
                    if (rdfAltElement)
                        for (XMLElement* rdfLiElement = rdfAltElement->FirstChildElement("rdf:li"); rdfLiElement; rdfLiElement = rdfLiElement->NextSiblingElement("rdf:li"))
                            Fill(Stream_General, 0, General_Copyright, rdfLiElement->GetText());
                }
            }
        }
    }

    Finish();
    return true;
}

} //NameSpace

#endif //MEDIAINFO_XMP_YES
