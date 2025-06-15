/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about EXIF tags
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_Exif
#define MediaInfo_File_Exif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Exif
//***************************************************************************

class File_Exif : public File__Analyze
{
public:
    //Constructor/Destructor
    File_Exif();

    //In
    bool FromHeif = false;

private :
    //Streams management
    void Streams_Finish();

    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void Read_Directory();
    void MulticodeString(ZtringList& Info);
    void Thumbnail();
    void Makernote();

    //Temp
    struct ifditem
    {
        int16u Tag;
        int16u Type;
        int32u Count;
    };
    typedef std::map<int32u, ifditem> ifditems; //Key is byte offset
    ifditems IfdItems;
    typedef std::map<int16u, ZtringList> infos; //Key is tag value
    std::map<int8u, infos> Infos; // Key is the kind of IFD
    std::map<int32u, int8u> IFD_Offsets; // Value is the kind of IFD
    int64s OffsetFromContainer = 0;
    int8u currentIFD;
    bool LittleEndian;
    bool IsMakernote;
    int32u MakernoteOffset;
    bool HasFooter;

    //Helpers
    void Get_X2(int16u& Info, const char* Name);
    void Get_X4(int32u& Info, const char* Name);
    void Get_IFDOffset(int8u KindOfIFD = (int8u)-1);
    void GetValueOffsetu(ifditem& IfdItem);
    void GoToOffset(int64u GoTo_);
};

} //NameSpace

#endif
