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
#include "tinyxml2.h"
#include "MediaInfo/OutputHelpers.h"
#include "MediaInfo/File__Analyse_Automatic.h"
#include <ctime>

using namespace std;
using namespace ZenLib;
using namespace tinyxml2;

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
Ztring XML_Encode (const Ztring& Data)
{
    Ztring Result;
    tstring::size_type Pos;
    for (Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case __T('"'): Result+=__T("&quot;"); break;
            case __T('&'): Result+=__T("&amp;"); break;
            case __T('\''): Result+=__T("&apos;"); break;
            case __T('<'): Result+=__T("&lt;"); break;
            case __T('>'): Result+=__T("&gt;"); break;
            case __T('\n'): Result+=__T("&#xA;"); break;
            case __T('\r'):
                Result+=__T("&#xA;");
                if (Pos+1<Data.size() && Data[Pos+1]==__T('\n')) // translate the #xD #xA sequence to a single #xA character
                    Pos++;
            break;
            default: if (Data[Pos]<0x0 || Data[Pos]>=0x20) Result+=Data[Pos]; // Ignore others control characters
        }
    }
    return Result;
}

//---------------------------------------------------------------------------
string XML_Encode (const string& Data)
{
    string Result;
    for (string::size_type Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case '\'': Result+="&apos;"; break;
            case '"': Result+="&quot;"; break;
            case '&': Result+="&amp;"; break;
            case '<': Result+="&lt;"; break;
            case '>': Result+="&gt;"; break;
            case '\n': Result+="&#xA;"; break;
            case '\r':
                Result+="&#xA;";
                if (Pos+1<Data.size() && Data[Pos+1]=='\n') // translate the #xD #xA sequence to a single #xA character
                    Pos++;
            break;
            default: if ((unsigned char)Data[Pos]>=0x20) Result+=Data[Pos]; // Ignore others control characters
        }
    }
    return Result;
}

//---------------------------------------------------------------------------
string To_XML (Node& Cur_Node, const int& Level, bool Print_Header, bool Indent)
{
    string Result;

    if (!Cur_Node.RawContent.empty())
    {
      //  if (Level)
      //      Result+="\n";
        Result+=Cur_Node.RawContent;
        return Result;
    }

    if (Print_Header)
    {
        Result+="<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        //Current date/time is ISO format
        time_t Time=time(NULL);
        Ztring TimeS; TimeS.Date_From_Seconds_1970((int32u)Time);
        TimeS.FindAndReplace(__T("UTC "), __T(""));
        TimeS.FindAndReplace(__T(" "), __T("T"));
        TimeS+=__T('Z');

        Result+=string("<!-- Generated at "+TimeS.To_UTF8()+" by "+MediaInfoLib::Config.Info_Version_Get().To_UTF8()+" -->\n");
    }

    if (Cur_Node.Name.empty() && Cur_Node.XmlCommentOut.empty())
        return Result;

    if (Level)
        Result+="\n";

    if (!Cur_Node.XmlCommentOut.empty())
    {
        Result+=(Indent?string(Level, '\t'):string())+"<!-- "+Cur_Node.XmlCommentOut;

        // If the node name is empty, just print the comment
        if (Cur_Node.Name.empty())
        {
            Result += " -->";
            return Result;
        }
        Result+="\n";
    }

    Result+=(Indent?string(Level, '\t'):string())+"<"+Cur_Node.Name;

    for (size_t Pos=0; Pos<Cur_Node.Attrs.size(); Pos++)
    {
        if (Cur_Node.Attrs[Pos].first.empty())
            continue;

        Result+=" "+Cur_Node.Attrs[Pos].first+"=\""+XML_Encode(Cur_Node.Attrs[Pos].second)+"\"";
    }
    Cur_Node.Attrs.clear(); //Free memory

    if (Cur_Node.Value.empty() && Cur_Node.Childs.empty())
    {
        Result+=" />";
        if (!Cur_Node.XmlComment.empty() && Cur_Node.XmlCommentOut.empty())
            Result+=" <!-- "+Cur_Node.XmlComment+" -->";
        if (Cur_Node.XmlCommentOut.size())
            Result+="\n"+(Indent?string(Level, '\t'):string())+"-->";
        return Result;
    }

    Result+=">";

    if (Cur_Node.Value.size())
    {
        if (Cur_Node.Childs.size())
            Result+="\n"+(Indent?string(Level+1, '\t'):string());
        Result+=XML_Encode(Cur_Node.Value);
    }

    bool CanDisplayXmlComment;
    if (Cur_Node.Childs.size())
    {
        CanDisplayXmlComment=false;
        if (!Cur_Node.XmlComment.empty() && Cur_Node.XmlCommentOut.empty())
            Result+=" <!-- "+Cur_Node.XmlComment+" -->";

        for (size_t Pos=0; Pos<Cur_Node.Childs.size(); Pos++)
        {
            if (!Cur_Node.Childs[Pos])
                continue;

            Result+=To_XML(*Cur_Node.Childs[Pos], Level+1, false, Indent);
            delete Cur_Node.Childs[Pos];
            Cur_Node.Childs[Pos]=NULL;
        }
        Cur_Node.Childs.clear(); //Free memory
        Result+="\n"+(Indent?string(Level, '\t'):string());
    }
    else
        CanDisplayXmlComment=true;

    Result+="</"+Cur_Node.Name+">";
    if (Cur_Node.XmlCommentOut.size())
        Result+="\n"+(Indent?string(Level, '\t'):string())+"-->";
    else if (!Cur_Node.XmlComment.empty() && CanDisplayXmlComment)
        Result+=" <!-- "+Cur_Node.XmlComment+" -->";
    if (!Level)
        Result+="\n";

    return Result;
}

//---------------------------------------------------------------------------
string JSON_Encode (const string& Data)
{
    string Result;
    for (string::size_type Pos=0; Pos<Data.size(); Pos++)
    {
        switch (Data[Pos])
        {
            case '\b': Result+="\\b"; break;
            case '\f': Result+="\\f"; break;
            case '\n': Result+="\\n"; break;
            case '\r': Result+="\\r"; break;
            case '\t': Result+="\\t"; break;
            case '"': Result+="\\\""; break;
            case '\\': Result+="\\\\"; break;
            default: Result+=Data[Pos];
        }
    }
     return Result;
}

//---------------------------------------------------------------------------
string To_JSON_Attributes(Node& Cur_Node, const int& Level, bool Indent)
{
    string Result;
    for (size_t Pos=0; Pos<Cur_Node.Attrs.size(); Pos++)
    {
        if (Cur_Node.Attrs[Pos].first.empty() || Cur_Node.Attrs[Pos].first.substr(0, 5)=="xmlns" || Cur_Node.Attrs[Pos].first.substr(0, 3)=="xsi")
            continue;

        Result+="\n"+(Indent?string(Level, '\t'):string())+"\"@"+Cur_Node.Attrs[Pos].first+"\": \""
               +JSON_Encode(Cur_Node.Attrs[Pos].second)+"\"";

        if (Pos<Cur_Node.Attrs.size()-1 || Cur_Node.Value.size() || Cur_Node.Childs.size())
            Result+=",";
    }
    Cur_Node.Attrs.clear(); //Free memory

    return Result;
}

//---------------------------------------------------------------------------
string To_JSON_Elements(Node& Cur_Node, const int& Level, bool Indent)
{
    string Result;

    for (size_t Pos=0; Pos<Cur_Node.Childs.size(); Pos++)
    {
        if (!Cur_Node.Childs[Pos])
            continue;

        if (!Cur_Node.Childs[Pos]->RawContent.empty())
        {
            if (Level)
                Result+="\n";
            Result+=Cur_Node.Childs[Pos]->RawContent;
            continue;
        }

        if (Cur_Node.Name.empty())
            continue;

        Result+="\n"+(Indent?string(Level, '\t'):string())+"\""+Cur_Node.Childs[Pos]->Name+"\": ";

        bool Multiple=Cur_Node.Childs[Pos]->Multiple;
        if (Multiple)
            Result+="[\n";

        string Name=Cur_Node.Childs[Pos]->Name;
        for (size_t Pos2=Pos; Pos2<Cur_Node.Childs.size() && Cur_Node.Childs[Pos2]->Name==Name; Pos2++)
        {
            if (!Cur_Node.Childs[Pos2])
                continue;

            if (Cur_Node.Childs[Pos2]->Attrs.empty() && Cur_Node.Childs[Pos2]->Childs.empty() && !Cur_Node.Childs[Pos2]->Multiple)
            {
                if (Cur_Node.Childs[Pos2]->Value.empty())
                    Result+="null";
                else
                    Result+="\""+JSON_Encode(Cur_Node.Childs[Pos2]->Value)+"\"";
            }
            else
            {
                Result+=(Indent?string(Level+1, '\t'):string())+"{";
                Result+=To_JSON_Attributes(*Cur_Node.Childs[Pos2], Level+2, Indent);
                Result+=To_JSON_Elements(*Cur_Node.Childs[Pos2], Level+2, Indent);
                Result+="\n";

                if(!Cur_Node.Childs[Pos2]->Value.empty())
                    Result+=(Indent?string(Level+2, '\t'):string())+"\"#value\": \""+JSON_Encode(Cur_Node.Childs[Pos2]->Value)+"\"\n";
                Result+=(Indent?string(Level+1, '\t'):string())+"}";
            }
            if (Pos2<Cur_Node.Childs.size()-1 && Cur_Node.Childs[Pos2]->Name==Cur_Node.Childs[Pos2+1]->Name)
                Result+=",";

            if (Multiple)
                Result+="\n";

            delete Cur_Node.Childs[Pos2];
            Cur_Node.Childs[Pos2]=NULL;
            Pos=Pos2;
        }
        if (Multiple)
            Result+=(Indent?string(Level, '\t'):string())+"]";

        if (Pos<Cur_Node.Childs.size()-1 || !Cur_Node.Value.empty())
            Result+=",";
    }
    Cur_Node.Childs.clear(); //Free memory

    return Result;
}

//---------------------------------------------------------------------------
string To_JSON (Node& Cur_Node, const int& Level, bool Print_Header, bool Indent)
{
    string Result;

    if (!Cur_Node.RawContent.empty())
    {
        if (Level)
            Result+="\n";
        Result+=Cur_Node.RawContent;
        return Result;
    }

    if (Cur_Node.Name.empty())
        return Result;

    if (Print_Header)
        Result+="{\n";

    Result+=(Indent?string(Level+1, '\t'):string())+"\""+Cur_Node.Name+"\": ";

    if (Cur_Node.Attrs.empty() && Cur_Node.Childs.empty() && !Cur_Node.Multiple)
    {
        if (Cur_Node.Value.empty())
            Result+="null";
        else
            Result+="\""+JSON_Encode(Cur_Node.Value)+"\"";
        if (Print_Header)
            Result+="\n}\n";
        return Result;
    }

    Result+="{";
    Result+=To_JSON_Attributes(Cur_Node, Level+2, Indent);
    Result+=To_JSON_Elements(Cur_Node, Level+2, Indent);
    if (!Cur_Node.Value.empty())
        Result+="\n"+(Indent?string(Level+2, '\t'):string())+"\"#value\": \""+JSON_Encode(Cur_Node.Value)+"\"";

    Result+="\n"+(Indent?string(Level+1, '\t'):string())+"}";

    if (Print_Header)
        Result+="\n}\n";

    return Result;
}

//---------------------------------------------------------------------------
bool Parse_XML(const ZtringList& Parents, const Ztring& PlaceHolder, const XMLNode* _XmlNode, Node* _Node, Node** _MI_Info, const Ztring& FileName, ZtringListList& Values)
{
    bool ToReturn = true;

    if (!_Node)
        return ToReturn;

    Node* Current=NULL;

    const XMLElement* Element = _XmlNode->ToElement();

    if (!Element)
        return ToReturn;

    if (Parents.Find(Ztring(Element->Value()))==string::npos)
    {
        if (PlaceHolder==Ztring(Element->Value()) && Element->FirstChild() == NULL && _MI_Info && *_MI_Info)
        {
            //Replace placeholder by MediaInfo report
            _Node->Childs.push_back(*_MI_Info);
            *_MI_Info=NULL;
        }
        else
        {
            Ztring Value=Ztring(Element->GetText()?Element->GetText():"");
            if (Value.length()>3 && Value.at(0)=='%' && Value.at(1)!='%' && Value.at(Value.length()-1)=='%')
            {
                Value=Values.FindValue(FileName, Values(0).Find(Value.substr(1, Value.length()-2)), 0, 1);
                if (Value.empty())
                    return false;
            }

            Current=new Node(Element->Value(), Value.To_UTF8(), true);

            for (const XMLAttribute* Attribute = Element->FirstAttribute(); Attribute; Attribute = Attribute->Next())
            {
                Ztring Value=Ztring(Attribute->Value());
                if (Value.length()>3 && Value.at(0)=='%' && Value.at(1)!='%' && Value.at(Value.length()-1)=='%')
                {
                    Value=Values.FindValue(FileName, Values(0).Find(Value.substr(1, Value.length()-2)), 0, 1);
                    if (Value.empty())
                        return false;
                }

                Current->Add_Attribute(Attribute->Name(), Value);
            }
        }
    }

    for (const XMLNode* El = Element->FirstChild(); El; El = El->NextSibling())
        ToReturn=Parse_XML(Parents, PlaceHolder, El, Current?Current:_Node, _MI_Info, FileName, Values);

    if (Current)
    {
        if (ToReturn)
            _Node->Childs.push_back(Current);
        else
            delete Current;
  }

    return ToReturn;
}

//---------------------------------------------------------------------------
bool ExternalMetadata(const Ztring& FileName, const Ztring& ExternalMetadata, const Ztring& ExternalMetaDataConfig, const ZtringList& Parents, const Ztring& PlaceHolder, Node* Main, Node* MI_Info)
{
    if (ExternalMetaDataConfig.empty()) // ExternalMetadata is used in export only if there is ExternalMetadata config (= else it is another format)
        return true;

    ZtringListList CSV;
    CSV.Separator_Set(0, EOL);
    CSV.Separator_Set(1, __T(";"));
    CSV.Write(ExternalMetadata);

    //Check if the CSV contains at least the header + one entry
    if (CSV.size()<2)
    {
        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid CSV for external metadata");
        return false;
    }

    //Check if the file is present in the CSV
    if (CSV.FindValue(FileName, 0, 0, 1).empty())
    {
        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "File name not found in external metadata file");
        return false;
    }

    //Parse XML template
    XMLDocument Template;
    if (Template.Parse(ExternalMetaDataConfig.To_UTF8().c_str()) != XML_SUCCESS)
    {
        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid XML template for external metadata");
        return false;
    }

    Parse_XML(Parents, PlaceHolder, Template.RootElement(), Main, &MI_Info, FileName, CSV);

    return true;
}

//---------------------------------------------------------------------------
Ztring VideoCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos) //xxyyzz: xx=main number, yy=sub-number, zz=sub-sub-number
{
    switch (termID/10000)
    {
        case 1 : return __T("MPEG-1 Video");
        case 2 :    switch ((termID%10000)/100)
                    {
                        case 1 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Simple Profile @ Main Level");
                                        default: return __T("MPEG-2 Video Simple Profile");
                                    }
                        case 2 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Main Profile @ Low Level");
                                        case 2 : return __T("MPEG-2 Video Main Profile @ Main Level");
                                        case 3 : return __T("MPEG-2 Video Main Profile @ High 1440 Level");
                                        case 4 : return __T("MPEG-2 Video Main Profile @ High Level");
                                        default: return __T("MPEG-2 Video Main Profile");
                                    }
                        case 3 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video SNR Scalable Profile @ Low Level");
                                        case 2 : return __T("MPEG-2 Video SNR Scalable Profile @ Main Level");
                                        default: return __T("MPEG-2 Video SNR Scalable Profile");
                                    }
                        case 4 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Spatial Scalable Profile @ Main Level");
                                        case 2 : return __T("MPEG-2 Video Spatial Scalable Profile @ High 1440 Level");
                                        case 3 : return __T("MPEG-2 Video Spatial Scalable Profile @ High Level");
                                        default: return __T("MPEG-2 Video Spatial Scalable Profile");
                                    }
                        case 5 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video High Profile @ Main Level");
                                        case 2 : return __T("MPEG-2 Video High Profile @ High 1440 Level");
                                        case 3 : return __T("MPEG-2 Video High Profile @ High Level");
                                        default: return __T("MPEG-2 Video High Profile");
                                    }
                        case 6 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video Multiview Profile @ Main Level");
                                        default: return __T("MPEG-2 Video Multiview Profile");
                                    }
                        case 7 :    switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-2 Video 4:2:2 Profile @ Main Level");
                                        default: return __T("MPEG-2 Video 4:2:2 Profile");
                                    }
                        default: return __T("MPEG-2 Video");
                    }
        case 3 :    switch ((termID%10000)/100)
                    {
                        case  1 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Visual Simple Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Visual Simple Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Visual Simple Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Simple Profile");
                                    }
                        case  2 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Scalable Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Simple Scalable Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Simple Scalable Profile");
                                    }
                        case  3 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 3");
                                        case 5 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 4");
                                        case 6 : return __T("MPEG-4 Advanced Visual Simple Profile @ Level 5");
                                        default: return __T("MPEG-4 Advanced Visual Simple Profile");
                                    }
                        case  4 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Core Profile");
                                    }
                        case  5 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Core-Scalable Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Core-Scalable Profile");
                                    }
                        case  6 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual AdvancedCore Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual AdvancedCore Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual AdvancedCore Profile");
                                    }
                        case  7 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Main Profile @ Level 2");
                                        case 2 : return __T("MPEG-4 Visual Main Profile @ Level 3");
                                        case 3 : return __T("MPEG-4 Visual Main Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Main Profile");
                                    }
                        case  8 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual N-bit Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Main Profile");
                                    }
                        case  9 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Advanced Real Time Simple Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Advanced Real Time Simple Profile");
                                    }
                        case 10 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Advanced Coding Efficiency Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Advanced Coding Efficiency Profile");
                                    }
                        case 11 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Simple Studio Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Simple Studio Profile");
                                    }
                        case 12 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Core Studio Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Core Studio Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Core Studio Profile @ Level 3");
                                        case 4 : return __T("MPEG-4 Visual Core Studio Profile @ Level 4");
                                        default: return __T("MPEG-4 Visual Core Studio Profile");
                                    }
                        case 13 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 0");
                                        case 2 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 1");
                                        case 3 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 2");
                                        case 4 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 3");
                                        case 5 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 4");
                                        case 6 : return __T("MPEG-4 Visual Fine Granularity Scalable Profile @ Level 5");
                                        default: return __T("MPEG-4 Visual Fine Granularity Scalable Profile");
                                    }
                        case 14 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Simple Face Animation Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Simple Face Animation Profile @ Level 2");
                                        default: return __T("MPEG-4 Simple Face Animation Profile");
                                    }
                        case 15 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Simple FBA Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Simple FBA Profile @ Level 2");
                                        default: return __T("MPEG-4 Simple FBA Profile");
                                    }
                        case 16 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Basic Animated Texture Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Basic Animated Texture Profile @ Level 2");
                                        default: return __T("MPEG-4 Basic Animated Texture Profile");
                                    }
                        case 17 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Advanced Scalable Texture Profile @ Level 1");
                                        default: return __T("MPEG-4 Advanced Scalable Texture Profile");
                                    }
                        case 18 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 2");
                                        case 3 : return __T("MPEG-4 Visual Advanced Scalable Texture Profile @ Level 3");
                                        default: return __T("MPEG-4 Visual Advanced Scalable Texture Profile");
                                    }
                        case 19 :   switch (termID%100)
                                    {
                                        case 1 : return __T("MPEG-4 Visual Hybrid Profile @ Level 1");
                                        case 2 : return __T("MPEG-4 Visual Hybrid Profile @ Level 2");
                                        default: return __T("MPEG-4 Visual Hybrid Profile");
                                    }
                        default: return __T("MPEG-4 Visual");
                    }
        case 4 :    return __T("JPEG");
        case 5 :    return __T("MJPEG");
        case 6 :    return __T("JPEG2000");
        case 7 :    return __T("H261");
        case 8 :    return __T("H263");
        default: return MI.Get(Stream_Video, StreamPos, Video_Format);
    }
}


} //NameSpace
