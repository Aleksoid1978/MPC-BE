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
#if defined(MEDIAINFO_APETAG_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Tag/File_ApeTag.h"
#include <algorithm>
#include <ctime>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
extern std::string Id3v2_PictureType(int8u Type);

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
void File_ApeTag::FileHeader_Parse()
{
    //Parsing
    int64u Signature;
    Peek_B8(Signature);
    if (Signature==0x4150455441474558LL) //"APETAGEX"
        HeaderFooter(); //v2

    FILLING_BEGIN();
        Accept("ApeTag");

        Stream_Prepare(Stream_General);

        Stream_Prepare(Stream_Audio);
    FILLING_END();
}

//***************************************************************************
// Buffer - Per element
//***************************************************************************

//---------------------------------------------------------------------------
bool File_ApeTag::Header_Begin()
{
    if (Buffer_Size<0x20)
        return false; //At least 32 bytes are needed for footer

    return true;
}

//---------------------------------------------------------------------------
void File_ApeTag::Header_Parse()
{
    //Testing if begin or end of tags
    if (CC8(Buffer+Buffer_Offset)==0x4150455441474558LL) //"APETAGEX"
    {
        //Filling
        Header_Fill_Code((int64u)-1, "File Footer");
        Header_Fill_Size(0x20);
        return;
    }

    //Parsing
    Ztring Value;
    int32u Flags, Length;
    Get_L4 (Length,                                         "Length");
    Get_L4 (Flags,                                          "Flags");
        Skip_Flags(Flags,  0,                               "Read Only");
        Get_Flags (Flags,  1, IsBinary,                     "Binary");
        Skip_Flags(Flags,  2,                               "Locator of external stored information");
        Skip_Flags(Flags, 29,                               "Is the header");
        Skip_Flags(Flags, 30,                               "Contains a footer");
        Skip_Flags(Flags, 31,                               "Contains a header");
    size_t Pos=(size_t)Element_Offset;
    for (; Pos<Element_Size; Pos++)
        if (Buffer[Buffer_Offset+Pos]==0x00)
            break;
    if (Pos==Element_Size)
    {
        Element_WaitForMoreData();
        return;
    }
    Get_String(Pos-Element_Offset, Key,                     "Key");
    Skip_L1(                                                "0x00");

    //Filling
    Header_Fill_Code(0, Key.c_str());
    Header_Fill_Size(Element_Offset+Length);
}

//***************************************************************************
// Elements
//***************************************************************************

//---------------------------------------------------------------------------
void File_ApeTag::HeaderFooter()
{
    //Parsing
    int32u Flags;
    Skip_C8(                                                    "Preamble");
    Skip_L4(                                                    "Version");
    Skip_L4(                                                    "Size");
    Skip_L4(                                                    "Count");
    Get_L4 (Flags,                                              "Flags");
        Skip_Flags(Flags,  0,                                   "Read Only");
        Skip_Flags(Flags,  1,                                   "Binary");
        Skip_Flags(Flags,  2,                                   "Locator of external stored information");
        Skip_Flags(Flags, 29,                                   "Is the header");
        Skip_Flags(Flags, 30,                                   "Contains a footer");
        Skip_Flags(Flags, 31,                                   "Contains a header");
    Skip_L8(                                                    "Reserved");
}

//---------------------------------------------------------------------------
void File_ApeTag::Data_Parse()
{
    //If footer
    if (Element_Code==(int64u)-1)
    {
        HeaderFooter();
        Finish("ApeTag");
        return;
    }

    // Mapping APEv2 Cover Art keys to MediaInfo's names matching ID3v2 PictureType
    auto APEKeyToID3v2 = [](std::string& APE_Key) -> std::string {
            struct KeyMapping {
                const char* APE_Key;
                int8u ID3v2_Type;
            };
            static constexpr KeyMapping mappings[] = {
                { "Cover Art (Other)",              0x00 },
                { "Cover Art (Png Icon)",           0x01 },
                { "Cover Art (Icon)",               0x02 },
                { "Cover Art (Front)",              0x03 },
                { "Cover Art (Back)",               0x04 },
                { "Cover Art (Leaflet)",            0x05 },
                { "Cover Art (Media)",              0x06 },
                { "Cover Art (Lead Artist)",        0x07 },
                { "Cover Art (Artist)",             0x08 },
                { "Cover Art (Conductor)",          0x09 },
                { "Cover Art (Band)",               0x0A }, 
                { "Cover Art (Composer)",           0x0B },
                { "Cover Art (Lyricist)",           0x0C },
                { "Cover Art (Recording Location)", 0x0D },
                { "Cover Art (During Recording)",   0x0E },
                { "Cover Art (During Performance)", 0x0F },
                { "Cover Art (Video Capture)",      0x10 },
                { "Cover Art (Fish)",               0x11 },
                { "Cover Art (Illustration)",       0x12 }, 
                { "Cover Art (Band Logotype)",      0x13 },
                { "Cover Art (Publisher Logotype)", 0x14 }
            };
            for (const auto& mapping : mappings) {
                if (APE_Key == mapping.APE_Key) {
                    return Id3v2_PictureType(mapping.ID3v2_Type);
                }
            }
            return APE_Key;
        };

    //Parsing
    Ztring Value;
    if (IsBinary && Key.rfind("Cover Art (", 0) == 0)
    {
        Get_UTF8(SizeUpTo0(), Value,                            "Filename"); Element_Info1(Value);
        Skip_B1(                                                "zero");
        Attachment("APEv2", Value, Ztring(APEKeyToID3v2(Key).c_str()), {}, true);
    }
    else
    {
        Get_UTF8(Element_Size, Value,                           "Value"); Element_Info1(Value);
    }

    //Filling
    transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char c) { return toupper(c); });
         if (Key=="ALBUM")          Fill(Stream_General, 0, General_Album, Value);
    else if (Key=="ALBUM ARTIST")   Fill(Stream_General, 0, General_Album_Performer, Value);
    else if (Key=="ARTIST")         Fill(Stream_General, 0, General_Performer, Value);
    else if (Key=="AUTHOR")         Fill(Stream_General, 0, General_WrittenBy, Value);
    else if (Key=="BAND")           Fill(Stream_General, 0, General_Performer, Value);
    else if (Key=="COMMENT")        Fill(Stream_General, 0, General_Comment, Value);
    else if (Key=="COMMENTS")       Fill(Stream_General, 0, General_Comment, Value);
    else if (Key=="COMPOSER")       Fill(Stream_General, 0, General_Composer, Value);
    else if (Key=="CONTENTGROUP")   Fill(Stream_General, 0, General_Genre, Value);
    else if (Key=="COPYRIGHT")      Fill(Stream_General, 0, General_Copyright, Value);
    else if (Key=="DISK" || Key=="DISC")
    {
                                    if (Value.find(__T('/'))!=Error)
                                    {
                                        Fill(Stream_General, 0, General_Part_Position_Total, Value.SubString(__T("/"), __T("")));
                                        Fill(Stream_General, 0, General_Part_Position, Value.SubString(__T(""), __T("/")));
                                    }
                                    else
                                        Fill(Stream_General, 0, General_Part_Position, Value);
    }
    else if (Key=="DISCSUBTITLE")   Fill(Stream_General, 0, General_Part, Value);
    else if (Key=="ENCODEDBY")      Fill(Stream_General, 0, General_EncodedBy, Value);
    else if (Key=="GENRE")          Fill(Stream_General, 0, General_Genre, Value);
    else if (Key=="LABEL")          Fill(Stream_General, 0, General_Label, Value);
    else if (Key=="LYRICS")         Fill(Stream_General, 0, General_Lyrics, Value);
    else if (Key=="LYRICIST")       Fill(Stream_General, 0, General_Lyricist, Value);
    else if (Key=="ORIGARTIST")     Fill(Stream_General, 0, General_Original_Performer, Value);
    else if (Key=="PUBLISHER")      Fill(Stream_General, 0, General_Publisher, Value);
    else if (Key=="TITLE")          Fill(Stream_General, 0, General_Title, Value);
    else if (Key=="TRACK")
    {
                                    if (Value.find(__T('/'))!=Error)
                                    {
                                        Fill(Stream_General, 0, General_Track_Position_Total, Value.SubString(__T("/"), __T("")));
                                        Fill(Stream_General, 0, General_Track_Position, Value.SubString(__T(""), __T("/")));
                                    }
                                    else
                                        Fill(Stream_General, 0, General_Track_Position, Value);
    }
    else if (Key=="UNSYNCEDLYRICS") Fill(Stream_General, 0, General_Lyrics, Value);
    else if (Key=="WWW")            Fill(Stream_General, 0, General_Title_Url, Value);
    else if (Key=="YEAR")           Fill(Stream_General, 0, General_Recorded_Date, Value);
    else if (Key=="CONTENT GROUP DESCRIPTION") Fill(Stream_General, 0, General_Title, Value);
    else if (Key=="ORIGINAL ALBUM/MOVIE/SHOW TITLE") Fill(Stream_General, 0, General_Original_Album, Value);
    else if (Key=="ORIGINAL ARTIST(S)/PERFORMER(S)") Fill(Stream_General, 0, General_Original_Performer, Value);
    else if (Key=="MP3GAIN_MINMAX") Fill(Stream_Audio, 0, "MP3Gain, Min/Max", Value);
    else if (Key=="MP3GAIN_UNDO") Fill(Stream_Audio, 0, "MP3Gain, Undo", Value);
    else if (Key=="REPLAYGAIN_TRACK_GAIN") Fill(Stream_Audio, 0, Audio_ReplayGain_Gain, Value.To_float64(), 2, true);
    else if (Key=="REPLAYGAIN_TRACK_PEAK") Fill(Stream_Audio, 0, Audio_ReplayGain_Peak, Value.To_float64(), 6, true);
    else if (IsBinary && Key.rfind("COVER ART (", 0) == 0) { /* Already handled by Attachment */ }
    else                            Fill(Stream_General, 0, Key.c_str(), Value);
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_APETAG_YES

