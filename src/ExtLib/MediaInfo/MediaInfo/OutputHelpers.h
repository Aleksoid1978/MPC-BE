/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Output_Helpers
#define MediaInfo_Output_Helpers
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
#include <ZenLib/Ztring.h>
#include <string>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

ZenLib::Ztring XML_Encode (const ZenLib::Ztring& Data);
std::string XML_Encode (const std::string& Data);
std::string JSON_Encode (const std::string& Data);

typedef std::pair<std::string, std::string> Attribute;
typedef std::vector<Attribute> Attributes;

struct Node
{
    std::string Name;
    std::string Value;
    Attributes Attrs;
    std::vector<Node*> Childs;
    std::string XmlComment; //If set, add comment after element data
    std::string XmlCommentOut; //If set, comment out the whole node in the xml output with the string as comment
    std::string RawContent; //If set, replace the whole node by the string
    bool Multiple;

    //Constructors
    Node()
    {
    }
    Node(const char* _Name) : Name(_Name), Multiple(false)
    {
    }
    Node(const char* _Name, bool _Multiple) : Name(_Name), Multiple(_Multiple)
    {
    }
    Node(const std::string& _Name, const std::string& _Value=std::string()) : Name(_Name), Value(_Value), Multiple(false)
    {
    }
    Node(const std::string& _Name, const std::string& _Value, bool _Multiple) : Name(_Name), Value(_Value), Multiple(_Multiple)
    {
    }
    Node(const std::string& _Name, const std::string& _Value, const std::string& _Atribute_Name, const std::string& _Atribute_Value=std::string()) : Name(_Name), Value(_Value), Multiple(false)
    {
        if (_Atribute_Value.empty())
            return;
        Add_Attribute(_Atribute_Name, _Atribute_Value);
    }
    Node(const std::string& _Name, const std::string& _Value, const std::string& _Atribute_Name, const std::string& _Atribute_Value, bool _Multiple) : Name(_Name), Value(_Value), Multiple(_Multiple)
    {
        if (_Atribute_Value.empty())
            return;
        Add_Attribute(_Atribute_Name, _Atribute_Value);
    }

    //Destructor
    ~Node()
    {
        for (size_t Pos=0; Pos<Childs.size(); Pos++)
            delete Childs[Pos];
    }

    //Add_Child functions
    Node* Add_Child(const std::string& Name, bool Multiple=false)
    {
        Childs.push_back(new Node(Name, std::string(), Multiple));
        return Childs.back();
    }
    Node* Add_Child(const std::string& Name, const std::string& Value, bool Multiple=false)
    {
        Childs.push_back(new Node(Name, Value, Multiple));
        return Childs.back();
    }
    #if defined(UNICODE) || defined (_UNICODE)
    Node* Add_Child(const std::string& Name, const ZenLib::Ztring& Value, bool Multiple=false)
    {
        return Add_Child(Name, Value.To_UTF8(), Multiple);
    }
    #endif //defined(UNICODE) || defined (_UNICODE)
    Node* Add_Child(const std::string& Name, const std::string& Value, const std::string& _Atribute_Name, const char* _Atribute_Value, bool Multiple=false)
    {
        Childs.push_back(new Node(Name, Value, _Atribute_Name, _Atribute_Value, Multiple));
        return Childs.back();
    }
    Node* Add_Child(const std::string& Name, const std::string& Value, const std::string& _Atribute_Name, const std::string& _Atribute_Value, bool Multiple=false)
    {
        Childs.push_back(new Node(Name, Value, _Atribute_Name, _Atribute_Value, Multiple));
        return Childs.back();
    }
    #if defined(UNICODE) || defined (_UNICODE)
    Node* Add_Child(const std::string& Name, const std::string& Value, const std::string& _Atribute_Name, const ZenLib::Ztring& _Atribute_Value, bool Multiple=false)
    {
        return Add_Child(Name, Value, _Atribute_Name, _Atribute_Value.To_UTF8(), Multiple);
    }
    Node* Add_Child(const std::string& Name, const ZenLib::Ztring& Value, const std::string& _Atribute_Name, const std::string& _Atribute_Value, bool Multiple=false)
    {
        return Add_Child(Name, Value.To_UTF8(), _Atribute_Name, _Atribute_Value, Multiple);
    }
    #endif //defined(UNICODE) || defined (_UNICODE)
    //Add_Child_IfNotEmpty functions
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, const std::string& Name, bool Multiple=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, ZenLib::Ztring().From_UTF8(FieldName));
        if (!Value.empty())
            return Add_Child(Name, Value, Multiple);
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, const std::string& Name, bool Multiple=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
        if (!Value.empty())
            return Add_Child(Name, Value, Multiple);
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, const std::string& Name, const std::string& Name2, bool Multiple=false, bool Multiple2=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
        if (!Value.empty())
        {
            Node* Child=Add_Child(Name, std::string(), Multiple);
            Child->Add_Child(Name2, Value.To_UTF8(), Multiple2);
            return Child;
        }
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, const std::string& Name, const std::string& Name2, bool Multiple=false, bool Multiple2=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, ZenLib::Ztring().From_UTF8(FieldName));
        if (!Value.empty())
        {
            Node* Child=Add_Child(Name, std::string(), Multiple);
            return Child->Add_Child(Name2, Value.To_UTF8(), Multiple2);
            return Child;
        }
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, const std::string& Name, const std::string& Atribute_Name, const std::string& Attribute_Value, bool Multiple=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
        if (!Value.empty())
            return Add_Child(Name, Value, Atribute_Name, Attribute_Value, Multiple);
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, const std::string& Name, const std::string& Atribute_Name, const std::string& Attribute_Value, bool Multiple=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, ZenLib::Ztring().From_UTF8(FieldName));
        if (!Value.empty())
            return Add_Child(Name, Value, Atribute_Name, Attribute_Value, Multiple);
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, const std::string& Name, const std::string& Atribute_Name, const std::string& Attribute_Value, const std::string& Name2, bool Multiple=false, bool Multiple2=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
        if (!Value.empty())
        {
            Node* Child=Add_Child(Name, std::string(), Atribute_Name, Attribute_Value, Multiple);
            Child->Add_Child(Name2, Value.To_UTF8(), Multiple2);
            return Child;
        }
        return NULL;
    }
    Node* Add_Child_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, const std::string& Name, const std::string& Atribute_Name, const std::string& Attribute_Value, const std::string& Name2, bool Multiple=false, bool Multiple2=false)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return NULL;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, ZenLib::Ztring().From_UTF8(FieldName));
        if (!Value.empty())
        {
            Node* Child=Add_Child(Name, std::string(), Atribute_Name, Attribute_Value, Multiple);
            Child->Add_Child(Name2, Value.To_UTF8(), Multiple2);
            return Child;
        }
        return NULL;
    }
    //Add_Attribute functions
    void Add_Attribute(const std::string& Name, const char* Value=NULL)
    {
        Attrs.push_back(Attribute(Name, Value?std::string(Value):std::string()));
    }
    void Add_Attribute(const std::string& Name, const std::string& Value)
    {
        Attrs.push_back(Attribute(Name, Value));
    }
    void Add_Attribute(const std::string& Name, const ZenLib::Ztring& Value)
    {
        Attrs.push_back(Attribute(Name, Value.To_UTF8()));
    }
    //Add_Attribute_IfNotEmpty functions
    void Add_Attribute_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, const char* FieldName, const std::string& Name)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, ZenLib::Ztring().From_UTF8(FieldName));
        if (!Value.empty())
            Add_Attribute(Name, Value);
    }
    void Add_Attribute_IfNotEmpty(MediaInfo_Internal &MI, stream_t StreamKind, size_t StreamPos, size_t FieldName, const std::string& Name)
    {
        if (StreamKind==Stream_Max || StreamPos==(size_t)-1)
            return;
        const ZenLib::Ztring& Value=MI.Get(StreamKind, StreamPos, FieldName);
        if (!Value.empty())
            Add_Attribute(Name, Value);
    }
};

std::string To_XML (Node& Cur_Node, const int& Level, bool Print_Header=false, bool Indent=true);
std::string To_JSON (Node& Cur_Node, const int& Level, bool Print_Header=false, bool Indent=true);

bool ExternalMetadata(const ZenLib::Ztring& FileName, const ZenLib::Ztring& ExternalMetadata, const ZenLib::Ztring& ExternalMetaDataConfig, const ZenLib::ZtringList& Parents, const  ZenLib::Ztring& PlaceHolder, Node* Main, Node* MI_Info);

Ztring VideoCompressionCodeCS_Name(int32u termID, MediaInfo_Internal &MI, size_t StreamPos);

} //NameSpace

#endif
