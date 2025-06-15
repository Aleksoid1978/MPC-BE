/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about VP8 files
// http://datatracker.ietf.org/doc/rfc6386/?include_text=1
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_WebpH
#define MediaInfo_WebpH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_WebP
//***************************************************************************

class File_Icc;

class File_WebP : public File__Analyze
{
public:
    //Constructor/Destructor
    File_WebP();
    ~File_WebP();

private:
    //Streams management
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void WEBP() {};
    void WEBP_ALPH();
    void WEBP_ANIM();
    void WEBP_ANMF();
    void WEBP_EXIF();
    void WEBP_ICCP();
    void WEBP_VP8_();
    void WEBP_VP8L();
    void WEBP_VP8X();
    void WEBP_XMP_();

    //Temp
    File_Icc* ICC_Parser = nullptr;
    int32u FrameDuration = 0;
    int64u TotalDuration = 0;
    bool HasAlpha = false;
    bool Alignment_ExtraByte; //Padding from the container
};

} //NameSpace

#endif
