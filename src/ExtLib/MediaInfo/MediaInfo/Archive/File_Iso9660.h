/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Iso9660 files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Iso9660H
#define MediaInfo_File_Iso9660H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iso9660
//***************************************************************************

class File_Iso9660 : public File__Analyze
{
public :
    //Constructor/Destructor
    ~File_Iso9660();

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Primary_Volume_Descriptor();
    void Primary_Volume_Descriptor2();
    void Path_Table();
    void Directory();
    void File();

    //Helpers
    void Directory_Record(int32u Size=0, const char* Name=nullptr);

    //Temp
    int16u Logical_Block_Size;
    struct record
    {
        int32u Location;
        int32u Length;
        Ztring Name;
        int8u  Flags;
    };
    typedef std::map<int32u, std::vector<record> > records; // Key is location of extent
    records Records;
    set<int32u> Parsed;
    set<int32u> NotParsed;
    struct file_info
    {
        record* Record_Current;
        Ztring Name;
    };
    typedef std::map<Ztring, record*> file_infos; // Key is complete file name
    file_infos MI_MasterFileInfos;
    file_infos MI_DataFileInfos;
    std::map<Ztring, MediaInfo_Internal*> MI_MasterFiles; // Key is complete file name
    std::map<Ztring, MediaInfo_Internal*> MI_DataFiles; // Key is complete file name
    MediaInfo_Internal* MI_Current;
    int64u MI_Current_StartOffset;
    int64u MI_Current_EndOffset;
    int32u Root_Location;
    bool HasTag;
    #if MEDIAINFO_TRACE
        bool Trace_Activated_Save;
    #endif //MEDIAINFO_TRACE

    //Helpers
    void Manage_Files();
    bool Manage_File(file_infos& MI_Files);
    void Manage_MasterFiles();
    void Manage_DataFiles();
    void Skip_DateTime(const char* Info) { Get_DateTime(nullptr, Info); }
    void Get_DateTime(Ztring* Value, const char* Info);
};

} //NameSpace

#endif
