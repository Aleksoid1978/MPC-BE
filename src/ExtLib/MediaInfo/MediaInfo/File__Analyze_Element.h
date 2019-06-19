/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_File__Analyze_ELEMENTH
#define MediaInfo_File__Analyze_ELEMENTH
//---------------------------------------------------------------------------

#include "MediaInfo/MediaInfo_Config.h"
#include <sstream>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
struct element_details
{
#if MEDIAINFO_TRACE
    class Element_Node_Data
    {
    public:

        enum Value_Output_Format
        {
            Format_Tree,
            Format_Xml,
        };

        enum Value_Type
        {
            ELEMENT_NODE_NONE,
            ELEMENT_NODE_CHAR8,
            ELEMENT_NODE_STR,
            ELEMENT_NODE_BOOL,
            ELEMENT_NODE_INT8U,
            ELEMENT_NODE_INT8S,
            ELEMENT_NODE_INT16U,
            ELEMENT_NODE_INT16S,
            ELEMENT_NODE_INT32U,
            ELEMENT_NODE_INT32S,
            ELEMENT_NODE_INT64U,
            ELEMENT_NODE_INT64S,
            ELEMENT_NODE_INT128U,
            ELEMENT_NODE_FLOAT32,
            ELEMENT_NODE_FLOAT64,
            ELEMENT_NODE_FLOAT80,
        };

        union Value
        {
            char*        Str;
            char         Chars[8];
            bool         b;
            int8u        i8u;
            int8s        i8s;
            int16u       i16u;
            int16s       i16s;
            int32u       i32u;
            int32s       i32s;
            int64u       i64u;
            int64s       i64s;
            float32      f32;
            float64      f64;
            float80     *f80;
            int128u     *i128u;
        };

        Element_Node_Data() : type(ELEMENT_NODE_NONE), format_out(Format_Xml) {}
        ~Element_Node_Data() { clear(); }

        Element_Node_Data& operator=(const Element_Node_Data&);
        void operator=(const std::string& v);
        void operator=(const char* v);
        void operator=(const Ztring& v);
        void operator=(bool v);
        void operator=(int8u v);
        void operator=(int8s v);
        void operator=(int16u v);
        void operator=(int16s v);
        void operator=(int32u v);
        void operator=(int32s v);
        void operator=(int64u v);
        void operator=(int64s v);
        void operator=(int128u v);
        void operator=(float32 v);
        void operator=(float64 v);
        void operator=(float80 v);

        bool operator==(const std::string& str);

        void clear();
        bool empty() {return type == ELEMENT_NODE_NONE;}
        void set_Option(int8u c) {Option = c;}
        void Set_Output_Format(Value_Output_Format v) {format_out = v;}
        friend std::ostream& operator<<(std::ostream& os, const element_details::Element_Node_Data& v);

        static void get_hexa_from_deci_limited_by_bits(std::string& val, int8u bits, int8u default_bits);

    private:
        Value               val;
        int8u               type; //Value_Type
        int8u               format_out; //Value_Output_Format
        int8u               Option; // float: count of valid digits after comma; int: count of valid bits; chars: count of valid chars

        Element_Node_Data(const Element_Node_Data&);
    };

    struct Element_Node_Info
    {
        template<typename T>
        Element_Node_Info(T parameter, const char* _Measure=NULL, int8u Option=(int8u)-1)
        {
            data.set_Option(Option);
            data = parameter;
            if (_Measure)
                Measure = _Measure;
        }

        friend std::ostream& operator<<(std::ostream& os, element_details::Element_Node_Info* v);

        Element_Node_Data data;
        std::string       Measure;

    private:
        Element_Node_Info& operator=(const Element_Node_Info&);
        Element_Node_Info(const Element_Node_Info&);
    };

    class Element_Node
    {
    public:
        Element_Node();
        Element_Node(const Element_Node& node);
        ~Element_Node();

        // Move
        void TakeChilrenFrom(Element_Node& node);

        int64u                           Pos;             // Position of the element in the file
        int64u                           Size;            // Size of the element (including header and sub-elements)
    private:
        std::string                      Name;            // Name planned for this element
    public:
        Element_Node_Data                Value;           // The value (currently used only with Trace XML)
        std::vector<Element_Node_Info*>  Infos;           // More info about the element
        std::vector<Element_Node*>       Children;        // Elements depending on this element
        int32s                           Current_Child;   // Current child selected, used for param
        bool                             NoShow;          // Don't show this element
        bool                             OwnChildren;     // Child is owned by this node
        bool                             IsCat;           // Node is a category
        bool                             HasError;        // Node or sub-nodes has Nok
        bool                             RemoveIfNoErrors;// Remove Children Node if no NOK appears

        void                             Init();          //Initialize with common values
        void Add_Child(Element_Node* node);               //Add a subchild to the current node
        void Set_Name(const string &Name_)
        {
            Name = Name_;
        }
        bool Name_Is_Empty() const {return Name.empty();}

        // Print
        int  Print(MediaInfo_Config::trace_Format Format, std::string& str, const string& eol, int64u File_Size);  //Print the node into str

    private:
        struct print_struc
        {
            std::ostringstream& ss;
            const string eol;
            const size_t offset_size;
            size_t level;

            print_struc(std::ostringstream& ss_, const string& eol_, size_t offset_size_)
                :
                ss(ss_),
                eol(eol_),
                offset_size(offset_size_),
                level(0)
            {
            }
        };
        int  Print_Xml(print_struc& s);                                            //Print the node in XML into ss
        int  Print_Micro_Xml(print_struc& s);                                      //Print the node in micro XML into ss
        int  Print_Tree(print_struc& s);                                           //Print the node into ss
        int  Print_Tree_Cat(print_struc& s);
    };
#endif //MEDIAINFO_TRACE

    int64u       Code;               //Code filled in the file
    int64u       Next;               //
    bool         WaitForMoreData;    //This element is not complete, we need more data
    bool         UnTrusted;          //This element has a problem
    bool         IsComplete;         //This element is fully buffered, no need of more

#if MEDIAINFO_TRACE
    Element_Node TraceNode;
#endif //MEDIAINFO_TRACE
};

} //NameSpace

#endif // !MediaInfo_File__Analyze_ELEMENTH
