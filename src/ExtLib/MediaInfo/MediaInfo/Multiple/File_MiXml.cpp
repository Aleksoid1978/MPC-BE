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
#if defined(MEDIAINFO_MIXML_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_MiXml.h"
#include "MediaInfo/MediaInfo_Config.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "ZenLib/FileName.h"
#include "tinyxml2.h"
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
extern const char* AC3_Surround[];
extern const char* AC3_Mode[];
extern const char* AC3_Mode_String[];

//---------------------------------------------------------------------------
// What should be hidden by default

struct extra_string
{
    const char** Names;
    const char* ToAdd;
};
struct extra_array
{
    const char** Names;
    const char* Options;
};

static const char* Xml_Extra_HideString[] =
{
    "GuardBand_Before",
    "GuardBand_After",
    "compr",
    "compr_Average",
    "compr_Minimum",
    "compr_Maximum",
    "dialnorm",
    "dialnorm_Average",
    "dialnorm_Minimum",
    "dialnorm_Maximum",
    "dynrng",
    "dynrng_Average",
    "dynrng_Minimum",
    "dynrng_Maximum",
    NULL,
};

static const char* Xml_Extra_String_channel[] =
{
    "BedChannelCount",
    NULL,
};

static const char* Xml_Extra_String_dB[] =
{
    "compr",
    "compr_Average",
    "compr_Minimum",
    "compr_Maximum",
    "dialnorm",
    "dialnorm_Average",
    "dialnorm_Minimum",
    "dialnorm_Maximum",
    "dynrng",
    "dynrng_Average",
    "dynrng_Minimum",
    "dynrng_Maximum",
    NULL,
};

static const char* Xml_Extra_String_micro[] =
{
    "GuardBand_Before",
    "GuardBand_After",
    NULL,
};

static const extra_string Xml_Extra_String[] =
{
    { Xml_Extra_String_channel,         " channel"},
    { Xml_Extra_String_dB,              " dB" },
    { Xml_Extra_String_micro,           " \xC2xB5s"}, //0xC2 0xB5 = micro sign
    { NULL, NULL},
};

static const char* Xml_Extra_N_NIY[] =
{
    "BedChannelCount",
    "BitRate_Instantaneous",
    "page_id",
    "region_depth",
    "region_height",
    "region_horizontal_address",
    "region_id",
    "region_vertical_address",
    "region_width",
    "subtitle_stream_id",
};

static const char* Xml_Extra_N_NT[] =
{
    "acmod",
    "bext_Present",
    "BlockAlignment",
    "bsid",
    "CaptionServiceContent_IsPresent",
    "CaptionServiceDescriptor_IsPresent",
    "CaptionServiceName",
    "cdp_length_Max",
    "cdp_length_Min",
    "compr",
    "compr_Average",
    "compr_Count",
    "compr_Maximum",
    "compr_Minimum",
    "data_partitioned",
    "Delay_SDTI",
    "Delay_SDTI",
    "Delay_SystemScheme1",
    "Demux_InitBytes",
    "Density_Unit",
    "Density_X",
    "Density_Y",
    "dialnorm",
    "dialnorm_Average",
    "dialnorm_Count",
    "dialnorm_Maximum",
    "dialnorm_Minimum",
    "dsurmod",
    "dynrng",
    "dynrng_Average",
    "dynrng_Count",
    "dynrng_Maximum",
    "dynrng_Minimum",
    "Errors_Stats",
    "Errors_Stats_03",
    "Errors_Stats_05",
    "Errors_Stats_09",
    "Errors_Stats_10",
    "Errors_Stats_Begin",
    "Errors_Stats_End",
    "Errors_Stats_End_03",
    "Errors_Stats_End_05",
    "FrameCount_Speed",
    "GuardBand_After",
    "GuardBand_Before",
    "IBI",
    "intra_dc_precision",
    "lfeon",
    "OverallBitRate_Precision_Max",
    "OverallBitRate_Precision_Min",
    "PCR_Distance_Average",
    "PCR_Distance_Max",
    "PCR_Distance_Min",
    "PCR_Invalid_Count",
    "PrimaryPackage",
    "reversible_vlc",
    "SideCar_FilePos",
    "Source_Delay",
    "Source_Delay_Source",
    "Source_List",
    "UniversalAdID_Registry",
    "UniversalAdID_Value",
    "UnsupportedSources",
    NULL,
};

static const extra_array Xml_Extra_Array[] =
{
    { Xml_Extra_N_NIY,                  "N NIY" },
    { Xml_Extra_N_NT,                   "N NT" },
    { NULL, NULL },
};

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_MiXml::File_MiXml()
:File__Analyze()
{
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_MiXml;
    #endif //MEDIAINFO_EVENTS
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MiXml::FileHeader_Begin()
{
    XMLDocument document;
    if (!FileHeader_Begin_XML(document))
       return false;

    {
        XMLElement* Root=document.FirstChildElement("MediaInfo");
        if (Root)
        {
            const char* Attribute = Root->Attribute("xmlns");
            if (Attribute == NULL || Ztring().From_UTF8(Attribute) != __T("https://mediaarea.net/mediainfo"))
            {
                Reject("MiXml");
                return false;
            }

            Accept("MiXml");

            XMLElement* Media = Root->FirstChildElement();
            while (Media)
            {
                if (string(Media->Value()) == "media")
                {
                    Attribute = Media->Attribute("ref");
                    if (Attribute)
                    {
                        File_Name.From_UTF8(Attribute);
                        Config->File_Names.clear();
                        
                        Fill(Stream_General, 0, General_CompleteName, File_Name, true); //TODO: merge with generic code
                        Fill(Stream_General, 0, General_FolderName, FileName::Path_Get(File_Name), true);
                        Fill(Stream_General, 0, General_FileName, FileName::Name_Get(File_Name), true);
                        Fill(Stream_General, 0, General_FileExtension, FileName::Extension_Get(File_Name), true);
                        if (Retrieve(Stream_General, 0, General_FileExtension).empty())
                            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName), true);
                        else
                            Fill(Stream_General, 0, General_FileNameExtension, Retrieve(Stream_General, 0, General_FileName)+__T('.')+Retrieve(Stream_General, 0, General_FileExtension), true);
                    }

                    XMLElement* Track = Media->FirstChildElement();
                    while (Track)
                    {
                        if (string(Track->Value()) == "track")
                        {
                            Attribute = Track->Attribute("type");
                            if (Attribute)
                            {
                                string StreamKind(Attribute);

                                StreamKind_Last = Stream_Max;
                                if (StreamKind == "General")
                                    StreamKind_Last = Stream_General;
                                if (StreamKind == "Video")
                                    Stream_Prepare(Stream_Video);
                                if (StreamKind == "Audio")
                                    Stream_Prepare(Stream_Audio);
                                if (StreamKind == "Text")
                                    Stream_Prepare(Stream_Text);
                                if (StreamKind == "Other")
                                    Stream_Prepare(Stream_Other);
                                if (StreamKind == "Image")
                                    Stream_Prepare(Stream_Image);
                                if (StreamKind == "Menu")
                                    Stream_Prepare(Stream_Menu);

                                if (StreamKind_Last != Stream_Max)
                                {
                                    XMLElement* Element = Track->FirstChildElement();
                                    while (Element)
                                    {
                                        string Name(Element->Name());
                                             if (Name == "Format_Version")
                                            Fill(StreamKind_Last, StreamPos_Last, Element->Name(), string("Version ")+Element->GetText(), true, true);
                                        else if (MediaInfoLib::Config.Info_Get(StreamKind_Last).Read(Ztring().From_UTF8(Element->Name()), Info_Measure) == __T(" ms"))
                                        {
                                            //Converting seconds to milliseconds while keeping precision
                                            Ztring N;
                                            N.From_UTF8(Element->GetText());
                                            size_t Dot = N.find('.');
                                            size_t Precision = 0;
                                            if (Dot != string::npos)
                                            {
                                                size_t End = N.find_first_not_of(__T("0123456789"), Dot + 1);
                                                if (End == string::npos)
                                                    End = N.size();
                                                Precision = End - (Dot + 1);
                                                if (Precision <= 3)
                                                    Precision = 0;
                                                else
                                                    Precision -= 3;
                                            }
                                            
                                            Fill(StreamKind_Last, StreamPos_Last, Element->Name(), N.To_float64()*1000, Precision, true);
                                        }
                                        else if (Name != "extra")
                                        {
                                            //Special cases
                                            if (Name == "Channels")
                                                Fill(StreamKind_Last, StreamPos_Last, "Channel(s)", Element->GetText());
                                            //Generic filling
                                            else
                                                Fill(StreamKind_Last, StreamPos_Last, Element->Name(), Element->GetText(), Unlimited, true, true);
                                        }
                                        else
                                        {
                                            XMLElement* Extra = Element->FirstChildElement();
                                            while (Extra)
                                            {
                                                if (strstr(Extra->Name(), "_String"))
                                                {
                                                    Extra = Extra->NextSiblingElement();
                                                    continue;
                                                }

                                                Fill(StreamKind_Last, StreamPos_Last, Extra->Name(), Extra->GetText(), Unlimited, true, true);

                                                bool HasString=false;
                                                for (size_t i=0; Xml_Extra_String[i].Names; i++)
                                                    for (size_t j=0; Xml_Extra_String[i].Names[j]; j++)
                                                        if (!strcmp(Extra->Name(), Xml_Extra_String[i].Names[j]))
                                                        {
                                                            Fill(StreamKind_Last, StreamPos_Last, (string(Extra->Name())+"/String").c_str(), MediaInfoLib::Config.Language_Get(Extra->GetText(), Ztring().From_UTF8(Xml_Extra_String[i].ToAdd)), true);
                                                            HasString=true;
                                                            break;
                                                        }

                                                for (size_t i=0; Xml_Extra_Array[i].Names; i++)
                                                    for (size_t j=0; Xml_Extra_Array[i].Names[j]; j++)
                                                        if (!strcmp(Extra->Name(), Xml_Extra_Array[i].Names[j]))
                                                        {
                                                            Fill_SetOptions(StreamKind_Last, StreamPos_Last, Extra->Name(), Xml_Extra_Array[i].Options);
                                                            if (HasString)
                                                            {
                                                                string Options(Xml_Extra_Array[i].Options);
                                                                if (InfoOption_ShowInXml<=Options.size())
                                                                    Options.resize(InfoOption_ShowInXml+1, ' ');
                                                                char Show='Y';
                                                                for (size_t k=0; Xml_Extra_HideString[k]; k++)
                                                                    if (!strcmp(Extra->Name(), Xml_Extra_HideString[k]))
                                                                    {
                                                                        Show='N';
                                                                        break;
                                                                    }
                                                                Options[InfoOption_ShowInInform]=Show;
                                                                Options[InfoOption_ShowInXml]='N';
                                                                Fill_SetOptions(StreamKind_Last, StreamPos_Last, (string(Extra->Name())+"/String").c_str(), Options.c_str());
                                                            }
                                                            break;
                                                        }

                                                if (!strcmp(Extra->Name(), "dsurmod"))
                                                {
                                                    size_t dsurmod=Ztring().From_UTF8(Extra->GetText()).To_int32u();
                                                    if (dsurmod<4)
                                                    {
                                                        Fill(StreamKind_Last, StreamPos_Last, "dsurmod/String", AC3_Surround[dsurmod]);
                                                        Fill_SetOptions(StreamKind_Last, StreamPos_Last, "dsurmod/String", "N NTN");
                                                    }
                                                }

                                                if (!strcmp(Extra->Name(), "ServiceKind"))
                                                {
                                                    const char* ServiceKind=Extra->GetText();
                                                    for (int8u i=0; i<8; i++)
                                                        if (!strcmp(ServiceKind, AC3_Mode[i]))
                                                        {
                                                            Fill(Stream_Audio, 0, Audio_ServiceKind_String, AC3_Mode_String[i]);
                                                            break;
                                                        }
                                                }

                                                Extra = Extra->NextSiblingElement();
                                            }
                                        }

                                        // Extra filling (duplicated content) //TODO: better handling of all such fields
                                             if (Name == "Format_Settings_Endianness")
                                            Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", Element->GetText());
                                        else if (Name == "Format_Settings_Packing")
                                            Fill(StreamKind_Last, StreamPos_Last, "Format_Settings", Element->GetText());

                                        Element = Element->NextSiblingElement();
                                    }
                                }
                            }
                        }

                        Track = Track->NextSiblingElement();
                    }
                }

                Media = Media->NextSiblingElement();
            }
        }
        else
        {
            Reject("MiXml");
            return false;
        }
    }

    //Special cases
    Ztring UniversalAdID_Value=Retrieve(Stream_General, 0, General_UniversalAdID_Value);
    Ztring UniversalAdID_Registry=Retrieve(Stream_General, 0, General_UniversalAdID_Registry);
    if (!UniversalAdID_Value.empty() && !UniversalAdID_Registry.empty())
    {
        Fill(Stream_General, 0, General_UniversalAdID_String, UniversalAdID_Value + __T(" (") + UniversalAdID_Registry + __T(")"), true);
    }
    Ztring MajorBrand=Retrieve(Stream_General, 0, General_CodecID);
    if (!UniversalAdID_Value.empty() && !UniversalAdID_Registry.empty())
    {
        Ztring CodecID_String=MajorBrand;
        Ztring Compatible=Retrieve(Stream_General, 0, General_CodecID);
        if (MajorBrand == __T("qt  "))
        {
            CodecID_String += __T(' ');
            CodecID_String += Retrieve(Stream_General, 0, General_CodecID_Version);
        }
        if (!Compatible.empty())
        {
            CodecID_String += __T(" (");
            CodecID_String += Compatible;
            CodecID_String += __T(')');
        }
        Fill(Stream_General, 0, General_CodecID_String, CodecID_String, true);
    }

    Element_Offset=File_Size;

    //All should be OK...
    return true;
}

} //NameSpace

#endif //MEDIAINFO_MiXml_YES

