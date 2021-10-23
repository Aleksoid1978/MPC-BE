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
#if defined(MEDIAINFO_RIFF_YES) || defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Audio/File_DolbyAudioMetadata.h"
#include "tinyxml2.h"
#include "ThirdParty/base64/base64.h"
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyAudioMetadata::File_DolbyAudioMetadata()
{
    //Configuration
    StreamSource=IsContainerExtra;

    //In
    IsXML=false;

    //Out
    HasSegment9=false;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyAudioMetadata::FileHeader_Begin()
{
    if (!IsXML)
        return true;

    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
    {
        return false;
    }

    XMLElement* Base64DbmdWrapper=document.FirstChildElement();
    if (!Base64DbmdWrapper || strcmp(Base64DbmdWrapper->Name(), "Base64DbmdWrapper"))
    {
        return false;
    }

    if (const char* Text=Base64DbmdWrapper->GetText())
    {
        const int8u* Save_Buffer=Buffer;
        size_t Save_Buffer_Size=Buffer_Size;

        string BufferS = Base64::decode(Text);
        Buffer=(const int8u*)BufferS.c_str();
        Element_Size=Buffer_Size=BufferS.size();

        Element_Begin1("Header");
        int32u Name, Size;
        Get_C4 (Name,                                           "Name");
        Get_L4 (Size,                                           "Size");
        if (Name==0x64626D64 && Size==Element_Size-Element_Offset)
            Read_Buffer_Continue();
        else
            Skip_XX(Element_Size-Element_Offset,                "Unknown");

        Buffer=Save_Buffer;
        Element_Offset=Element_Size=Buffer_Size=Save_Buffer_Size;
    }

    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyAudioMetadata::Read_Buffer_Continue()
{
    Accept("DolbyAudioMetadata");
    Stream_Prepare(Stream_Audio);

    //Parsing
    int32u version;
    Get_L4 (version,                                            "version");
    if ((version>>24)>1)
    {
        Skip_XX(Element_Size-Element_Offset,                    "Data");
        return;
    }
    while(Element_Offset<Element_Size)
    {
        Element_Begin1("metadata_segment");
        int8u metadata_segment_id;
        Get_L1 (metadata_segment_id,                            "metadata_segment_id"); Element_Info1(Ztring::ToZtring(metadata_segment_id));
        if (!metadata_segment_id)
        {
            Element_End0();
            break;
        }
        int16u metadata_segment_size;
        Get_L2 (metadata_segment_size,                          "metadata_segment_size");
        switch (metadata_segment_id)
        {
            case 9:
                HasSegment9=true; // Needed for flagging Dolby Atmos Master ADM profile
                // Fallthrough
            default: Skip_XX(metadata_segment_size,             "metadata_segment_payload");
        }
        Skip_L1(                                                "metadata_segment_checksum");
        Element_End0();
    }

    Finish();
}

} //NameSpace

#endif //defined(MEDIAINFO_RIFF_YES) || defined(MEDIAINFO_MXF_YES)

