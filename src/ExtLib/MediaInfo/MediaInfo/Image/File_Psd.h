/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PSD files
//
// Contributor: Lionel Duchateau, kurtnoise@free.fr
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_PsdH
#define MediaInfo_File_PsdH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Psd
//***************************************************************************

class File_Psd : public File__Analyze
{
public:
    enum step
    {
        Step_ColorModeData,
        Step_ImageResources,
        Step_ImageResourcesBlock,
        Step_LayerAndMaskInformation,
        Step_ImageData,
    };
    step Step{ Step_ColorModeData };

protected :
    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void ColorModeData();
    void ImageResources();
    void ImageResourcesBlock();
    void LayerAndMaskInformation();
    void ImageData();
    void CopyrightFlag();
    void CaptionDigest();
    void IPTCNAA();
    void JPEGQuality();
    void Thumbnail();
    void URL();
    void VersionInfo();
    void Thumbnail_New() { Thumbnail(); }

    //Temp
    int64u Alignment_ExtraByte = 0; //Padding from the container
};

} //NameSpace

#endif

