/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about Immersive Audio Model and Formats (IAMF) files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_Iamf
#define MediaInfo_Iamf
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Iamf
//***************************************************************************

class File_Iamf : public File__Analyze
{
public:
    // Constructor/Destructor
    File_Iamf();
    ~File_Iamf();

private:
    // Streams management
    void Streams_Accept();

    // Buffer - Global
    void Read_Buffer_OutOfBand();

    // Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    // Elements
    void ia_codec_config();
    void ia_audio_element();
    void ia_mix_presentation();
    void ia_sequence_header();
    void ParamDefinition(int64u param_definition_type);

    // Helpers
    void Get_leb128(int64u& Info, const char* Name);
};

} //NameSpace

#endif // !MediaInfo_Iamf
