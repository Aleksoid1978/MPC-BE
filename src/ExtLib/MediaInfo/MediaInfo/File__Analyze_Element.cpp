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
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/File__Analyze_Element.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include <iomanip>
#include <cstring>
#include "ThirdParty/base64/base64.h"

namespace MediaInfoLib
{
#if MEDIAINFO_TRACE
//***************************************************************************
// Element_Node_Data
//***************************************************************************

//---------------------------------------------------------------------------
element_details::Element_Node_Data& element_details::Element_Node_Data::operator=(const Element_Node_Data& v)
{
    if (this == &v)
        return *this;

    clear();

    type = v.type;
    switch (type)
    {
        case ELEMENT_NODE_STR:
        {
            size_t len = strlen(v.val.Str);
            val.Str = new char[len + 1];
            std::memcpy(val.Str, v.val.Str, len);
            val.Str[len] = '\0';
            break;
        }
        case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT80:
              val.f80 = new float80;
              *val.f80 = *v.val.f80;
              break;
        case ELEMENT_NODE_INT128U:
            val.i128u = new int128u;
            *val.i128u = *v.val.i128u;
            break;
        default:
            val = v.val;
            break;
    }

    format_out = v.format_out;
    Option = v.Option;

    return *this;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const Ztring& v)
{
    clear();

    std::string tmp = v.To_UTF8();
    size_t len = tmp.length();
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, tmp.c_str(), len);
        set_Option(len);
    }
    else
    {
        type = ELEMENT_NODE_STR;
        val.Str = new char[len + 1];
        std::memcpy(val.Str, tmp.c_str(), len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const std::string& v)
{
    clear();

    size_t len =v.length();
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, v.c_str(), len);
        set_Option(len);
    }
    else
    {
        type = ELEMENT_NODE_STR;
        val.Str = new char[len + 1];
        std::memcpy(val.Str, v.c_str(), len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(const char* v)
{
    clear();

    if (!v)
        return;

    type = ELEMENT_NODE_STR;
    int len = strlen(v);
    if (len <= 8)
    {
        type = ELEMENT_NODE_CHAR8;
        std::memcpy(val.Chars, v, len);
        set_Option(len);
    }
    else
    {
        val.Str = new char[len + 1];
        std::memcpy(val.Str, v, len);
        val.Str[len] = '\0';
    }
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(bool v)
{
    clear();

    type = ELEMENT_NODE_BOOL;
    val.b = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8u v)
{
    clear();

    type = ELEMENT_NODE_INT8U;
    val.i8u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int8s v)
{
    clear();

    type = ELEMENT_NODE_INT8S;
    val.i8s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16u v)
{
    clear();

    type = ELEMENT_NODE_INT16U;
    val.i16u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int16s v)
{
    clear();

    type = ELEMENT_NODE_INT16S;
    val.i16s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32u v)
{
    clear();

    type = ELEMENT_NODE_INT32U;
    val.i32u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int32s v)
{
    clear();

    type = ELEMENT_NODE_INT32S;
    val.i32s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64u v)
{
    clear();

    type = ELEMENT_NODE_INT64U;
    val.i64u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int64s v)
{
    clear();

    type = ELEMENT_NODE_INT64S;
    val.i64s = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(int128u v)
{
    clear();

    type = ELEMENT_NODE_INT128U;
    val.i128u = new int128u;
    *val.i128u = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float32 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT32;
    val.f32 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float64 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT64;
    val.f64 = v;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::operator=(float80 v)
{
    clear();

    type = ELEMENT_NODE_FLOAT80;
    val.f80 = new float80;
    *val.f80 = v;
}

//---------------------------------------------------------------------------
bool element_details::Element_Node_Data::operator==(const std::string& v)
{
    if (type == ELEMENT_NODE_CHAR8)
        return v == std::string(val.Chars, Option);
    if (type == ELEMENT_NODE_STR)
        return v == val.Str;

    return false;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::clear()
{
    switch (type)
    {
      case ELEMENT_NODE_STR:
          delete [] val.Str;
          break;
      case ELEMENT_NODE_FLOAT80:
          delete val.f80;
          break;
      case ELEMENT_NODE_INT128U:
          delete val.i128u;
          break;
      default:
          break;
    }

    type = ELEMENT_NODE_NONE;
}

//---------------------------------------------------------------------------
void element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(std::string& val, int8u bits, int8u default_bits)
{
    if (bits == (int8u)-1)
        bits = default_bits;

    std::string zeros;
    bool full = (bits % 4 == 0);
    int nb_zero = (bits / 4) + (full?0:1);
    nb_zero -= val.size();
    if (nb_zero > 0)
        zeros.resize(nb_zero, '0');
    val = zeros + val;
}

//---------------------------------------------------------------------------
static size_t Xml_Name_Escape_MustEscape(const char* Name, size_t Size)
{
    // Case it is empty
    if (!Size)
        return (size_t)-1;

    // Case first char is a digit
    if ((Name[0] >= '0' && Name[0] <= '9'))
        return 0;

    // Cheking all chars
    for (size_t Pos = 0; Pos < Size; Pos++)
    {
        const char C = Name[Pos];
        switch (C)
        {
            case ' ':
            case '/':
            case '(':
            case ')':
            case '*':
            case ',':
            case ':':
            case '@':
                return Pos;
            default:
                if (!(C >= 'A' && C <= 'Z')
                    && !(C >= 'a' && C <= 'z')
                    && !(C >= '0' && C <= '9')
                    && !(C == '_'))
                    return Pos;
        }
     }

    return (size_t)-1;
}

//---------------------------------------------------------------------------
static inline size_t Xml_Name_Escape_MustEscape(const std::string &Name)
{
    return Xml_Name_Escape_MustEscape(Name.c_str(), Name.size());
}

//---------------------------------------------------------------------------
static size_t Xml_Content_Escape_MustEscape(const char* Content, size_t Size)
{
    // Cheking all chars
    for (size_t Pos = 0; Pos < Size; Pos++)
    {
        const unsigned char C = Content[Pos];
        switch (C)
        {
            case '\"':
            case '&':
            case '\'':
            case '<':
            case '>':
                return Pos;
            default:
                if (C<0x20)
                    return Pos;
        }
    }

    return (size_t)-1;
}

//---------------------------------------------------------------------------
static inline size_t Xml_Content_Escape_MustEscape(const std::string &Content)
{
    return Xml_Content_Escape_MustEscape(Content.c_str(), Content.size());
}

//---------------------------------------------------------------------------
static void Xml_Content_Escape(const char* Content, size_t Size, std::string& ToReturn, size_t Pos)
{
    if (Size)
        ToReturn = std::string(Content, Size);
    else
    {
        ToReturn.clear();
        return;
    }

    size_t Content_Size = Size;

    for (; Pos<Size; Pos++)
    {
        const char C = ToReturn[Pos];
        switch (C)
        {
            case '\"':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "quot;");
                Pos += 5;
                Size += 5;
                break;
            case '&':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "amp;");
                Pos += 4;
                Size += 4;
                break;
            case '\'':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "apos;");
                Pos += 5;
                Size += 5;
                break;
            case '<':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "lt;");
                Pos += 3;
                Size += 3;
                break;
            case '>':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "gt;");
                Pos += 3;
                Size += 3;
                break;
            case '\n':
                ToReturn[Pos] = '&';
                ToReturn.insert(Pos + 1, "#xA;");
                Pos += 4;
                Size += 4;
                break;
            case '\r': //Doing like XML rule "(...) translating both the two-character sequence #xD #xA and any #xD that is not followed by #xA to a single #xA character".
                ToReturn[Pos] = '&';
                if (Pos + 1 < Size && ToReturn[Pos + 1] == '\n')
                {
                    Pos++;
                    ToReturn[Pos] = '#';
                    ToReturn.insert(Pos + 1, "xA;");
                    Pos += 3;
                    Size += 3;
                    break;
                }
                ToReturn.insert(Pos + 1, "#xA;");
                Pos += 4;
                Size += 4;
                break;
            default:
                if (C<0x20)
                {
                    ToReturn = Base64::encode(std::string(Content, Content_Size));
                    return; //End
                }
        }
    }
}

//---------------------------------------------------------------------------
static inline void Xml_Content_Escape(const std::string &Content, std::string& ToReturn, size_t Pos)
{
    Xml_Content_Escape(Content.c_str(), Content.size(), ToReturn, Pos);
}

#define VALUE_INTEGER_FORMATED(_val, _type, _length)                                                       \
    do                                                                                                      \
    {                                                                                                       \
        os << std::dec << (_type)_val;                                                                      \
        if (v.format_out == element_details::Element_Node_Data::Format_Tree)                                \
        {                                                                                                   \
            std::stringstream ss;                                                                           \
            ss << std::hex << std::uppercase << (_type)_val;                                                                  \
            std::string str = ss.str();                                                                     \
            element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, _length); \
            os << " (0x" << str << ")";                                                                     \
        }                                                                                                   \
    }                                                                                                       \
    while(0);


//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const element_details::Element_Node_Data& v)
{
    switch (v.type)
    {
      case element_details::Element_Node_Data::ELEMENT_NODE_CHAR8:
      {
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              for (int8u i = 0; i < v.Option; ++i)
                  os.rdbuf()->sputc(v.val.Chars[i]?v.val.Chars[i]:' '); //Text output does not like NULL chars, replacing it by space
              break;
          }

          size_t MustEscape = Xml_Content_Escape_MustEscape(v.val.Chars, v.Option);

          if (MustEscape != (size_t)-1)
          {
              std::string str;
              Xml_Content_Escape(v.val.Chars, v.Option, str, MustEscape);
              os << str;
          }
          else
          {
              for (int8u i = 0; i < v.Option; ++i)
                  os.rdbuf()->sputc(v.val.Chars[i]);
          }
          break;
      }
      case element_details::Element_Node_Data::ELEMENT_NODE_STR:
      {
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              const char* a=v.val.Str;
              for (;;)
              {
                  const char* b=strchr(a, '\n');
                  if (!b)
                      break;
                  std::streamsize c=b-a;
                  if (c && a[c-1]=='\r')
                      c--;
                  os.write(a, c);
                  a=b+1;
                  if (*a) // If it is not the last character
                      os << " / ";
              }
              os << a;
              break;
          }

          size_t MustEscape = Xml_Content_Escape_MustEscape(v.val.Str);
          if (MustEscape != (size_t)-1)
          {
              std::string str;
              Xml_Content_Escape(v.val.Str, str, MustEscape);
              os << str;
          }
          else
              os << v.val.Str;
          break;
      }
      case element_details::Element_Node_Data::ELEMENT_NODE_BOOL:
          if (v.val.b)
              os << "Yes";
          else
              os << "No";
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT8U:
          VALUE_INTEGER_FORMATED(v.val.i8u, unsigned, 8); //use unsigned for stringstream
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT8S:
          VALUE_INTEGER_FORMATED(v.val.i8s, int, 8); //use int for stringstream
          break;

      case element_details::Element_Node_Data::ELEMENT_NODE_INT16U:
          VALUE_INTEGER_FORMATED(v.val.i16u, unsigned, 16); //use unsigned for stringstream
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT16S:
          VALUE_INTEGER_FORMATED(v.val.i16s, int, 16); //use int for stringstream
          break;

      case element_details::Element_Node_Data::ELEMENT_NODE_INT32U:
          VALUE_INTEGER_FORMATED(v.val.i32u, int32u, 32); //use unsigned for stringstream
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT32S:
          VALUE_INTEGER_FORMATED(v.val.i32s, int32s, 32); //use int for stringstream
          break;

      case element_details::Element_Node_Data::ELEMENT_NODE_INT64U:
          VALUE_INTEGER_FORMATED(v.val.i64u, int64u, 64);
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_INT64S:
          VALUE_INTEGER_FORMATED(v.val.i64s, int64s, 64); //use int for stringstream
          break;

      case element_details::Element_Node_Data::ELEMENT_NODE_INT128U:
          os << uint128toString(*v.val.i128u, 10);
          if (v.format_out == element_details::Element_Node_Data::Format_Tree)
          {
              std::string str = uint128toString(*v.val.i128u, 16);
              element_details::Element_Node_Data::get_hexa_from_deci_limited_by_bits(str, v.Option, 128);
              os << " (0x" << str << ")";
          }
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT32:
          os << Ztring::ToZtring(v.val.f32, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT64:
          os << Ztring::ToZtring(v.val.f64, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
      case element_details::Element_Node_Data::ELEMENT_NODE_FLOAT80:
          os << Ztring::ToZtring(*v.val.f80, v.Option == (int8u)-1 ? 3 : v.Option).To_UTF8();
          break;
    }
    return os;
}

#undef VALUE_INTEGER_FORMATED


//***************************************************************************
// Element_Node_Info
//***************************************************************************
std::ostream& operator<<(std::ostream& os, element_details::Element_Node_Info* v)
{
    if (!v)
        return os;

    os << v->data;
    if (!v->Measure.empty())
        os << v->Measure;

    return os;
}

//***************************************************************************
// Element_Node
//***************************************************************************
//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node()
: Pos(0), Size(0),
  Current_Child(-1), NoShow(false), OwnChildren(true), IsCat(false), HasError(false), RemoveIfNoErrors(false)
{
}

//---------------------------------------------------------------------------
element_details::Element_Node::Element_Node(const Element_Node& node)
{
    if (this == &node)
        return;

    Pos = node.Pos;
    Size = node.Size;
    Name = node.Name;
    Value = node.Value;
    Infos = node.Infos;
    Children = node.Children;
    Current_Child = node.Current_Child;
    NoShow = node.NoShow;
    OwnChildren = node.OwnChildren;
    IsCat = node.IsCat;
    HasError = node.HasError;
    RemoveIfNoErrors = node.RemoveIfNoErrors;
}

//---------------------------------------------------------------------------
void element_details::Element_Node::TakeChilrenFrom(Element_Node& node)
{
    if (this == &node || !OwnChildren || !node.OwnChildren)
        return;

    Children.insert(Children.end(), node.Children.begin(), node.Children.end());
    node.Children.clear();
}

//---------------------------------------------------------------------------
element_details::Element_Node::~Element_Node()
{
    if (!OwnChildren)
        return;

    for (size_t i = 0; i < Children.size(); ++i)
        delete Children[i];
    Children.clear();

    for (size_t i = 0; i < Infos.size(); ++i)
        delete Infos[i];
    Infos.clear();
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Init()
{
    Pos = 0;
    Size = 0;
    Name.clear();
    Value.clear();
    if (Children.size() && OwnChildren)
        for (size_t i = 0; i < Children.size(); ++i)
            delete Children[i];
    Children.clear();
    if (OwnChildren && Infos.size())
        for (size_t i = 0; i < Infos.size(); ++i)
            delete Infos[i];
    Infos.clear();
    Current_Child = -1;
    NoShow = false;
    OwnChildren = true;
    IsCat = false;
    HasError = false;
    RemoveIfNoErrors = false;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Micro_Xml(print_struc& s)
{
    if (NoShow)
        return 0;

    if (IsCat || Name_Is_Empty())
        goto print_children;

    if (Value.empty())
        s.ss << "<b";
    else
        s.ss << "<d";

    {
    size_t MustEscape = Xml_Content_Escape_MustEscape(Name);
    if (MustEscape != (size_t)-1)
    {
        std::string str;
        Xml_Content_Escape(Name, str, MustEscape);
        s.ss << " o=\"" << Pos << "\" n=\"" << str << "\"";
    }
    else
        s.ss << " o=\"" << Pos << "\" n=\"" << Name << "\"";
    }

    for (size_t i = 0, info_nb = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = Infos[i];

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
                s.ss << " parser=\"" << Info->data << "\"";
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
                s.ss << " e=\"" << Info->data << "\"";
            continue;
        }

        info_nb++;
        s.ss << " i";
        if (info_nb > 1)
            s.ss << info_nb;
        s.ss << "=\"" << Infos[i] << "\"";
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        s.ss << ">" << Value << "</d>";
    }
    else
        s.ss << " s=\"" << Size << "\">";

    s.level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Micro_Xml(s);

    if (!IsCat && !Name_Is_Empty())
    {
        s.level -= 4;

        //end tag
        if (Value.empty())
        {
            //block
            s.ss << "</b>";
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Xml(print_struc& s)
{
    if (NoShow)
        return 0;

    std::string spaces;

    if (IsCat || Name_Is_Empty())
        goto print_children;

    spaces.resize(s.level, ' ');
    s.ss << spaces;

    if (Value.empty())
        s.ss << "<block";
    else
        s.ss << "<data";

    {
    size_t MustEscape = Xml_Content_Escape_MustEscape(Name);
    if (MustEscape != (size_t)-1)
    {
        std::string str;
        Xml_Content_Escape(Name, str, MustEscape);
        s.ss << " offset=\"" << Pos << "\" name=\"" << str << "\"";
    }
    else
        s.ss << " offset=\"" << Pos << "\" name=\"" << Name << "\"";
    }

    for (size_t i = 0, info_nb = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = Infos[i];

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
                s.ss << " parser=\"" << Info->data << "\"";
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
                s.ss << " error=\"" << Info->data << "\"";
            continue;
        }

        info_nb++;
        s.ss << " info";
        if (info_nb > 1)
            s.ss << info_nb;
        s.ss << "=\"" << Info << "\"";
    }

    if (!Value.empty())
    {
        Value.Set_Output_Format(Element_Node_Data::Format_Xml);
        s.ss << ">" << Value << "</data>";
    }
    else
        s.ss << " size=\"" << Size << "\">";

    s.ss << s.eol;

    s.level += 4;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Xml(s);

    if (!IsCat && !Name_Is_Empty())
    {
        s.level -= 4;

        //end tag
        if (Value.empty())
        {
            //block
            s.ss << spaces << "</block>" << s.eol;
        }
    }

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree_Cat(print_struc& s)
{
    std::ostringstream offset;
    offset << std::setfill('0') << std::setw(s.offset_size) << std::hex << std::uppercase << Pos << std::nouppercase << std::dec;

    std::string spaces;
    spaces.resize(s.level, ' ');

    std::string ToShow;
    ToShow += "---   ";
    ToShow += Name;
    ToShow += "   ---";

    std::string minuses;
    minuses.resize(ToShow.size(), '-');

    s.ss << offset.str() << spaces << minuses << s.eol;
    s.ss << offset.str() << spaces << ToShow << s.eol;
    s.ss << offset.str() << spaces << minuses << s.eol;
    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print_Tree(print_struc& s)
{
    std::string spaces;

    if (NoShow)
        return 0;

    if (IsCat)
        return Print_Tree_Cat(s);
    else if (Name_Is_Empty())
        goto print_children;

    s.ss << std::setfill('0') << std::setw(s.offset_size) << std::hex << std::uppercase << Pos << std::nouppercase << std::dec;
    spaces.resize(s.level, ' ');
    s.ss << spaces;
    s.ss << Name;

    spaces.clear();

#define NB_SPACES 40
    if (!Value.empty())
    {
        s.ss << ":";
        int nb_free = NB_SPACES - s.level - (Name_Is_Empty() ? 0 : Name.length()); // 40 - len(Name) - len(spaces)
        spaces.resize(nb_free > 0 ? nb_free : 1, ' ');
        Value.Set_Output_Format(Element_Node_Data::Format_Tree);
        s.ss << spaces << Value;
        spaces.clear();
    }
#undef NB_SPACES

    for (size_t i = 0; i < Infos.size(); ++i)
    {
        Element_Node_Info* Info = Infos[i];

        if (!Info)
            continue;

        if (Info->Measure == "Parser")
        {
            if (!(Info->data == string()))
                s.ss << " - Parser=" << Info->data;
            continue;
        }
        if (Info->Measure == "Error")
        {
            if (!(Info->data == string()))
                s.ss << " - Error=" << Info->data;
            continue;
        }

        Info->data.Set_Output_Format(Element_Node_Data::Format_Tree);
        s.ss << " - " << Info;
    }

    if (Value.empty())
        s.ss << " (" << Size << " bytes)";

    s.ss << s.eol;

    s.level++;
print_children:
    for (size_t i = 0; i < Children.size(); ++i)
        Children[i]->Print_Tree(s);

    if (!Name_Is_Empty())
        s.level--;

    return 0;
}

//---------------------------------------------------------------------------
int element_details::Element_Node::Print(MediaInfo_Config::trace_Format Format, std::string& Str, const string& eol, int64u File_Size)
{
    //Computing how many characters are needed for displaying maximum file size
    size_t offset_size = sizeof(File_Size)*8-1;
    while (offset_size>1 && (((int64u)1) << offset_size) - 1 >= File_Size)
        offset_size--;
    offset_size++;
    offset_size = (offset_size / 4) + ((offset_size % 4) ? 1 : 0); //4 bits per offset char

    std::ostringstream ss;
    int ret = -1;
    print_struc s(ss, eol, offset_size);
    switch (Format)
    {
        case MediaInfo_Config::Trace_Format_Tree:
            s.level = 1;
            ret = Print_Tree(s);
        break;
        case MediaInfo_Config::Trace_Format_CSV:
            break;
        case MediaInfo_Config::Trace_Format_XML:
            ret = Print_Xml(s);
            break;
        case MediaInfo_Config::Trace_Format_MICRO_XML:
            ret = Print_Micro_Xml(s);
            break;
        default:
            break;
    }
    Str = ss.str();
    return ret;
}

//---------------------------------------------------------------------------
void element_details::Element_Node::Add_Child(Element_Node* node)
{
    if (node->HasError)
    {
        HasError = node->HasError;
        NoShow = false;
    }

    if (RemoveIfNoErrors && !node->HasError)
    {
        if (!HasError)
            NoShow = true;
        return;
    }

    Element_Node *new_node = new Element_Node(*node);
    node->OwnChildren = false;
    Children.push_back(new_node);
}

#endif

}
