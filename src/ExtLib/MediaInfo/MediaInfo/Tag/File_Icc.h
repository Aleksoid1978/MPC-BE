/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about ICC (International Color Consortium) files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_IccH
#define MediaInfo_File_IccH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class File_Icc_Private;

//***************************************************************************
// Class File_Sami
//***************************************************************************

class File_Icc : public File__Analyze
{
public:
    //In
    stream_t    StreamKind=Stream_Image;
    bool        IsAdditional=false;
    int8u       Frame_Count_Max=0;

    //Constructor/Destructor
    ~File_Icc();

private :
    //Buffer - File header
    void FileHeader_Parse();

    //Buffer - Global
    void Read_Buffer_Continue ();

    //Elements
    void xTRC(int32u Format, int32u Size);
    void xXYZ(int32u Format, int32u Size);
    void cicp(int32u Format, int32u Size);
    void cprt(int32u Format, int32u Size);
    void desc(int32u Format, int32u Size);
    void dmdd(int32u Format, int32u Size);
    void dmnd(int32u Format, int32u Size);
    void view(int32u Format, int32u Size);
    void vued(int32u Format, int32u Size);
    void xxpt(int32u Format, int32u Size);

    //Temp
    File_Icc_Private* P = nullptr;

    //Helpers
    void Skip_curv(int32u Size);
    void Get_desc(int32u Size, Ztring& Value);
    void Get_mluc(int32u Size, Ztring& Value);
    void Skip_text(int32u Size);
    void Skip_XYZ(int32u Size, const char* Name);
    void Skip_s15Fixed16Number(const char* Name);

};

} //NameSpace

#endif
