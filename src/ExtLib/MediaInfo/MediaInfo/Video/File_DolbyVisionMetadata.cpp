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
#if defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Video/File_DolbyVisionMetadata.h"
#include "tinyxml2.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <vector>
using namespace tinyxml2;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
// Element names without the namespace prefix (e.g. "fw:Shot" is "Shot")
static const char* Local(const char* Name)
{
    const char* Colon = strrchr(Name, ':');
    return Colon ? Colon + 1 : Name;
}
static bool Is(const XMLElement* Element, const char* Name)
{
    return !strcmp(Local(Element->Name()), Name);
}
static XMLElement* Child(const XMLElement* Element, const char* Name)
{
    for (auto Item = Element->FirstChildElement(); Item; Item = Item->NextSiblingElement())
        if (Is(Item, Name))
            return const_cast<XMLElement*>(Item);
    return nullptr;
}
static XMLElement* Next(const XMLElement* Element, const char* Name)
{
    for (auto Item = Element->NextSiblingElement(); Item; Item = Item->NextSiblingElement())
        if (Is(Item, Name))
            return const_cast<XMLElement*>(Item);
    return nullptr;
}

//---------------------------------------------------------------------------
static void TID_Collect(XMLElement* Element, std::set<string>& TIDs)
{
    for (XMLElement* Item = Element->FirstChildElement(); Item; Item = Item->NextSiblingElement())
    {
        if (Is(Item, "TID"))
        {
            if (const char* Text = Item->GetText())
                TIDs.insert(Text);
        }
        else
            TID_Collect(Item, TIDs);
    }
}

//---------------------------------------------------------------------------
// Values separated by spaces (version 4.x or more) or commas (version 2.x)
static std::vector<float64> Values_Get(const char* Text)
{
    std::vector<float64> Values;
    for (;;)
    {
        while (*Text == ' ' || *Text == ',')
            Text++;
        if (!*Text)
            break;
        const char* End = Text;
        while (*End && *End != ' ' && *End != ',')
            End++;
        Values.push_back(Ztring().From_UTF8(Text, End - Text).To_float64());
        Text = End;
    }
    return Values;
}

//---------------------------------------------------------------------------
// Root element of a malformed XML document, after the XML declaration and the comments
static bool Root_IsMDF(const int8u* Buffer, size_t Buffer_Size)
{
    static const char Root_Name[] = "DolbyLabsMDF";
    const char* Text = (const char*)Buffer;
    const char* Text_End = Text + Buffer_Size;
    if (Text_End - Text >= 3 && !memcmp(Text, "\xEF\xBB\xBF", 3))
        Text += 3;
    for (;;)
    {
        while (Text < Text_End && (*Text == ' ' || *Text == '\t' || *Text == '\r' || *Text == '\n'))
            Text++;
        if (Text_End - Text >= 2 && (!memcmp(Text, "<?", 2) || !memcmp(Text, "<!", 2)))
        {
            Text = (const char*)memchr(Text, '>', Text_End - Text);
            if (!Text)
                return false;
            Text++;
            continue;
        }
        break;
    }
    if (Text >= Text_End || *Text != '<')
        return false;
    const char* Name_Begin = ++Text;
    while (Text < Text_End && *Text != ' ' && *Text != '\t' && *Text != '\r' && *Text != '\n' && *Text != '>' && *Text != '/')
        Text++;
    string Name(Name_Begin, Text - Name_Begin);
    return !strcmp(Local(Name.c_str()), Root_Name);
}

//---------------------------------------------------------------------------
// Versions with a known structure: 2.x (CM v2.9), 4.x and 5.x (CM v4.0), 6.x
static bool Version_IsSupported(const string& Version)
{
    auto Version_Major = atoi(Version.c_str());
    return Version_Major == 2 || (Version_Major >= 4 && Version_Major <= 6);
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_DolbyVisionMetadata::File_DolbyVisionMetadata()
{
    //Configuration
    StreamSource=IsContainerExtra;
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_DolbyVisionMetadata::FileHeader_Begin()
{
    //Configuration
    if (!IsSub)
        StreamSource=IsContainer; // Standalone file, not extra metadata of a container

    XMLDocument document;
    if (!FileHeader_Begin_XML(document, false))
    {
        if (!document.Error())
            return false; // Rejected or waiting for more data

        // Malformed XML, the root element is enough for detecting a Dolby Vision MDF and the issue is reported
        if (!Root_IsMDF(Buffer, Buffer_Size))
        {
            Reject("DolbyVisionMetadata");
            return false;
        }
        Accept("DolbyVisionMetadata");
        Fill(Stream_General, 0, General_Format, "Dolby Vision Metadata");
        Stream_Prepare(Stream_Video);
        Fill(Stream_Video, 0, Video_HDR_Format, "Dolby Vision Metadata");
        Stream_Prepare(Stream_Other);
        Fill(Stream_Other, 0, Other_Format, "Dolby Vision MDF");
        Fill_Conformance("DolbyVision XML", "XML is malformed (line " + std::to_string(document.ErrorLineNum()) + ')', {}, Conformance_Error, Stream_Video, 0);
        Element_Offset=File_Size;
        return true;
    }

    string Version;
    float32 AspectRatio=0;
    XMLElement* Root = document.FirstChildElement();
    if (!Root)
    {
        Reject("DolbyVisionMetadata");
        return false;
    }

    // Version 2.x to 5.x: global data, track or frame data, frame data of integrated data
    XMLElement* MasterElements[3] = {};
    std::vector<XMLElement*> Tracks; // Version 6.x
    const auto Version_Get = [&](XMLElement* Parent)
    {
        if (XMLElement* Item = Child(Parent, "Version"))
            if (const char* Text = Item->GetText())
                Version = Text;
    };

    const char* Format = nullptr;
    if (Is(Root, "DolbyVisionGlobalDataGSP"))
    {
        Version_Get(Root);
        MasterElements[1] = Child(Root, "Track");
        Format = "Dolby Vision Global Data GSP";
    }
    else if (Is(Root, "DolbyVisionIntegratedData"))
    {
        Version_Get(Root);
        if (XMLElement* GlobalData = Child(Root, "DolbyVisionGlobalData"))
        {
            if (XMLElement* Item = Child(GlobalData, "CanvasAspectRatio"))
                if (const char* Text = Item->GetText())
                    if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                        AspectRatio = TextF;
            MasterElements[1] = Child(GlobalData, "Track");
        }
        MasterElements[2] = Child(Root, "DolbyVisionFrameData");
        Format = "Dolby Vision Integrated Data";
    }
    else if (Is(Root, "DolbyVisionIntegratedWrapper"))
    {
        MasterElements[0] = Child(Root, "DolbyVisionGlobalData");
        MasterElements[1] = Child(Root, "DolbyVisionFrameData"); // Frame Data has priority over Global Data
        Format = MasterElements[1] ? "Dolby Vision Frame Data" : (MasterElements[0] ? "Dolby Vision Global Data" : "Dolby Vision Integrated Wrapper");
    }
    else if (Is(Root, "DolbyVisionGlobalData"))
    {
        MasterElements[0] = Root;
        Format = "Dolby Vision Global Data";
    }
    else if (Is(Root, "DolbyVisionFrameData"))
    {
        Version_Get(Root);
        MasterElements[1] = Root;
        Format = "Dolby Vision Frame Data";
    }
    else if (Is(Root, "DolbyLabsMDF"))
    {
        Format = "Dolby Vision MDF";
    }
    else
    {
        Reject("DolbyVisionMetadata");
        return false;
    }
    if (const char* Text = Root->Attribute("version"))
        Version=Text;
    for (auto MasterElement : MasterElements)
        if (MasterElement)
            if (const char* Text = MasterElement->Attribute("version"))
                Version=Text;

    // Dolby Vision XML metadata, root element DolbyLabsMDF (CM v2.9 is version 2.x, CM v4.0 is version 4.x or more)
    XMLElement* MDF_Output = nullptr;
    XMLElement* MDF_Revision_Last = nullptr;
    if (Is(Root, "DolbyLabsMDF"))
    {
        Version_Get(Root);
        for (XMLElement* MDF_Item = Root->FirstChildElement(); MDF_Item; MDF_Item = MDF_Item->NextSiblingElement())
        {
            if (Is(MDF_Item, "RevisionHistory"))
            {
                for (XMLElement* Revision = Child(MDF_Item, "Revision"); Revision; Revision = Next(Revision, "Revision"))
                    MDF_Revision_Last = Revision;
            }
            if (Is(MDF_Item, "Outputs"))
            {
                if (XMLElement* Output = Child(MDF_Item, "Output"))
                {
                    MDF_Output = Output;
                    if (XMLElement* Video = Child(Output, "Video"))
                        MasterElements[1] = Child(Video, "Track");
                }
            }
        }
    }

    // Version 6.x: tracks instead of the previous structure
    // Dolby Vision MDF without the expected structure (e.g. unsupported version) is not rejected, the root element is enough for detecting it and the issue is reported
    const auto IsV6 = !Version.empty() && atoi(Version.c_str()) >= 6;
    const char* Structure_Missing = nullptr;
    if (IsV6)
    {
        for (auto& MasterElement : MasterElements)
            MasterElement = nullptr;
        MDF_Output = nullptr;
        for (XMLElement* Track = Child(Root, "Track"); Track; Track = Next(Track, "Track"))
            Tracks.push_back(Track);
        if (Tracks.empty())
        {
            if (!Is(Root, "DolbyLabsMDF"))
            {
                Reject("DolbyVisionMetadata");
                return false;
            }
            Structure_Missing = "Track";
        }
    }
    else if (Is(Root, "DolbyLabsMDF") ? !MDF_Output : (!MasterElements[0] && !MasterElements[1] && !MasterElements[2]))
    {
        if (!Is(Root, "DolbyLabsMDF"))
        {
            Reject("DolbyVisionMetadata");
            return false;
        }
        Structure_Missing = "Outputs/Output";
    }

    const auto IsWholeStream = Is(Root, "DolbyLabsMDF"); // Not only the data of a frame

    Accept("DolbyVisionMetadata");
    Fill(Stream_General, 0, General_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Video);
    Fill(Stream_Video, 0, Video_HDR_Format, "Dolby Vision Metadata");
    Stream_Prepare(Stream_Other);
    if (Format)
        Fill(Stream_Other, 0, Other_Format, Format);
    if (!Version.empty())
        Fill(Stream_Video, 0, Video_HDR_Format_Version, Version);
    if (AspectRatio)
        Fill(Stream_Video, 0, Video_DisplayAspectRatio, AspectRatio);

    // Dolby Vision metadata, displayed the same way as the metadata from the RPU
    DV_RPU DV;
    int8u TargetDisplay_WithoutID = 255;
    auto TargetDisplay_Fill = [&](XMLElement* TargetDisplay)
    {
        const char* Name = TargetDisplay->Attribute("name");
        const char* ID = TargetDisplay->Attribute("id");
        float64 PeakBrightness = 0, MinimumBrightness = 0;
        for (XMLElement* TargetDisplay_Item = TargetDisplay->FirstChildElement(); TargetDisplay_Item; TargetDisplay_Item = TargetDisplay_Item->NextSiblingElement())
        {
            const char* Text = TargetDisplay_Item->GetText();
            if (!Text)
                continue;
            if (Is(TargetDisplay_Item, "Name"))
                Name = Text;
            if (Is(TargetDisplay_Item, "ID"))
                ID = Text;
            if (Is(TargetDisplay_Item, "PeakBrightness"))
                PeakBrightness = atof(Text);
            if (Is(TargetDisplay_Item, "MinimumBrightness"))
                MinimumBrightness = atof(Text);
        }
        int8u Index = ID ? (int8u)atoi(ID) : TargetDisplay_WithoutID--;
        if (find(DV.L8_target_display_index.begin(), DV.L8_target_display_index.end(), Index) != DV.L8_target_display_index.end())
            return;
        DV.L8_target_display_index.push_back(Index);
        if (Name)
            DV.TargetDisplay_Names.emplace_back(Index, Ztring().From_UTF8(Name));
        if (PeakBrightness)
            DV.L10.push_back({ Index, 0, DolbyVision_Nits_ToPQ12(PeakBrightness), DolbyVision_Nits_ToPQ12(MinimumBrightness) });
    };
    auto MasteringDisplay_Fill = [&](XMLElement* MasteringDisplay)
    {
        const char* Name = MasteringDisplay->Attribute("name");
        const char* ID = MasteringDisplay->Attribute("id");
        for (XMLElement* MasteringDisplay_Item = MasteringDisplay->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item = MasteringDisplay_Item->NextSiblingElement())
        {
            const char* Text = MasteringDisplay_Item->GetText();
            if (!Text)
                continue;
            if (Is(MasteringDisplay_Item, "Name"))
                Name = Text;
            if (Is(MasteringDisplay_Item, "ID"))
                ID = Text;
            if (Is(MasteringDisplay_Item, "DiagonalSize"))
                DV.source_diagonal = (int16u)atoi(Text);
        }
        if (Name)
            DV.MasteringDisplay_Name.From_UTF8(Name);
        if (ID)
            DV.MasteringDisplay_ID.From_UTF8(ID);
    };
    auto MasteringDisplay_Values = [&](const mastering_metadata_2086& Mastering)
    {
        if (Mastering.Luminance[0] != (int32u)-1 && Mastering.Luminance[1] != (int32u)-1)
        {
            int32u Source = (((int32u)DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[0]) / 10000)) << 16) | DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[1]) / 10000);
            if (find(DV.Sources.begin(), DV.Sources.end(), Source) == DV.Sources.end())
                DV.Sources.push_back(Source);
        }
        DV_RPU::l9 L9{};
        static const int8u Order[8] = { 4, 5, 0, 1, 2, 3, 6, 7 }; // Green, blue, red, white to red, green, blue, white
        for (int8u i = 0; i < 8; i++)
        {
            int16u Value = Mastering.Primaries[Order[i]];
            if (Value == (int16u)-1)
                return;
            L9.Primaries[i] = (int16u)float64_int64s(((float64)Value) * 32767 / 50000);
        }
        L9.source_primary_index = 255;
        L9.Explicit = true;
        if (find(DV.L9.begin(), DV.L9.end(), L9) == DV.L9.end())
            DV.L9.push_back(L9);
    };
    bool Shot_HasL5 = false, Shot_WithoutL5 = false; // The default active aspect ratio is used only by the shots without a specific one
    float64 ImageAspectRatio_Default = 0;
    auto Level_Parse = [&](XMLElement* Level_Item)
    {
        int Level = Level_Item->IntAttribute("level", -1);
        if (Level < 0)
            return;
        DV_RPU::l9 L9{};
        DV_RPU::l11 L11{ 0, 0, (int8u)-1 };
        bool L9_IsValid = false, L11_IsValid = false;
        for (XMLElement* Item = Level_Item->FirstChildElement(); Item; Item = Item->NextSiblingElement())
        {
            const char* Text = Item->GetText();
            if (!Text)
                continue;
            if (Level == 1 && IsWholeStream && Is(Item, "ImageCharacter"))
            {
                auto Values = Values_Get(Text); // Minimum, average, maximum
                if (Values.size() == 3 && Values[1] >= 0 && Values[2] >= 0 && Values[1] <= 1 && Values[2] <= 1)
                {
                    auto avg_PQ = (int16u)float64_int64s(Values[1] * 4095);
                    auto max_PQ = (int16u)float64_int64s(Values[2] * 4095);
                    if (DV.L1_avg_PQ < avg_PQ)
                        DV.L1_avg_PQ = avg_PQ;
                    if (DV.L1_max_PQ < max_PQ)
                        DV.L1_max_PQ = max_PQ;
                }
            }
            if (Level == 5 && Is(Item, "AspectRatios"))
            {
                Shot_HasL5 = true;
                auto Values = Values_Get(Text); // Canvas, image
                if (Values.size() == 2 && Values[1] > 0 && find(DV.Active_DisplayAspectRatios.begin(), DV.Active_DisplayAspectRatios.end(), Values[1]) == DV.Active_DisplayAspectRatios.end())
                    DV.Active_DisplayAspectRatios.push_back(Values[1]);
            }
            if (Level == 9 && Is(Item, "SourceColorModel"))
            {
                L9.source_primary_index = (int8u)atoi(Text);
                L9_IsValid = true;
            }
            if (Level == 9 && Is(Item, "SourceColorPrimary"))
            {
                auto Values = Values_Get(Text);
                if (Values.size() == 8)
                {
                    for (int8u i = 0; i < 8; i++)
                        L9.Primaries[i] = (int16u)(int16s)float64_int64s(Values[i] * 32767);
                    L9.Explicit = true;
                }
            }
            if (Level == 11 && Is(Item, "ContentType"))
            {
                L11.content_type = (int8u)atoi(Text);
                L11_IsValid = true;
            }
            if (Level == 11 && Is(Item, "IntendedWhitePoint"))
                L11.white_point = (int8u)atoi(Text);
        }
        if (L9_IsValid && (L9.Explicit || L9.source_primary_index != 255) && find(DV.L9.begin(), DV.L9.end(), L9) == DV.L9.end())
            DV.L9.push_back(L9);
        if (L11_IsValid && find(DV.L11.begin(), DV.L11.end(), L11) == DV.L11.end())
            DV.L11.push_back(L11);
    };
    auto DynamicData_Parse = [&](XMLElement* PluginNode)
    {
        // Version 2.x: DolbyEDR elements with a level attribute, version 4.x or more: LevelN elements in DVDynamicData
        for (XMLElement* PluginNode_Item = PluginNode->FirstChildElement(); PluginNode_Item; PluginNode_Item = PluginNode_Item->NextSiblingElement())
        {
            if (Is(PluginNode_Item, "DVDynamicData"))
            {
                for (XMLElement* Level_Item = PluginNode_Item->FirstChildElement(); Level_Item; Level_Item = Level_Item->NextSiblingElement())
                    Level_Parse(Level_Item);
            }
            else
                Level_Parse(PluginNode_Item);
        }
    };

    if (IsV6 && Version_IsSupported(Version) && Is(Root, "DolbyLabsMDF"))
        DV.CMVersion.From_UTF8("4.0");
    if (MDF_Output)
    {
        if (!Version.empty())
            DV.CMVersion.From_UTF8(Version[0] == '2' ? "2.9" : "4.0");
        if (MDF_Revision_Last)
        {
            const char* CompanyName = nullptr;
            const char* Name = nullptr;
            const char* SoftwareVersion = nullptr;
            const char* DateTime = nullptr;
            for (XMLElement* Revision_Item = MDF_Revision_Last->FirstChildElement(); Revision_Item; Revision_Item = Revision_Item->NextSiblingElement())
            {
                if (Is(Revision_Item, "Author"))
                    CompanyName = Revision_Item->GetText();
                if (Is(Revision_Item, "Software"))
                    Name = Revision_Item->GetText();
                if (Is(Revision_Item, "SoftwareVersion"))
                    SoftwareVersion = Revision_Item->GetText();
                if (Is(Revision_Item, "DateTime"))
                    DateTime = Revision_Item->GetText();
            }
            if (CompanyName)
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, CompanyName);
            if (Name)
                Fill(Stream_General, 0, General_Encoded_Application_Name, Name);
            if (SoftwareVersion)
                Fill(Stream_General, 0, General_Encoded_Application_Version, SoftwareVersion);
            if (DateTime)
                Fill(Stream_General, 0, General_Encoded_Date, DateTime);
        }
        if (const char* Text = MDF_Output->Attribute("name"))
            Fill(Stream_General, 0, General_Title, Text);
        for (XMLElement* Output_Item = MDF_Output->FirstChildElement(); Output_Item; Output_Item = Output_Item->NextSiblingElement())
        {
            if (Is(Output_Item, "CompositionName"))
            {
                if (const char* Text = Output_Item->GetText())
                    Fill(Stream_General, 0, General_Title, Text, Unlimited, true, true);
            }
            if (Is(Output_Item, "UniqueID"))
            {
                if (const char* Text = Output_Item->GetText())
                    Fill(Stream_General, 0, General_UniqueID, Text);
            }
            if (Is(Output_Item, "CanvasAspectRatio"))
            {
                if (const char* Text = Output_Item->GetText())
                    if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                        Fill(Stream_Video, 0, Video_DisplayAspectRatio, TextF);
            }
            if (Is(Output_Item, "ImageAspectRatio"))
            {
                if (const char* Text = Output_Item->GetText())
                    if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                    {
                        Fill(Stream_Video, 0, Video_Active_DisplayAspectRatio, TextF);
                        ImageAspectRatio_Default = TextF;
                    }
            }
        }
        if (MasterElements[1])
            if (const char* Text = MasterElements[1]->Attribute("name"))
                Fill(Stream_Other, 0, Other_Title, Text);
    }

    int64u Shot_In = (int64u)-1, Shot_Out = 0;
    std::set<int64u> Shot_Ins; // Unique shots, the shot is repeated in each frame of integrated data
    const auto Shot_Add = [&](int64u In_Value, int64u Duration_Value)
    {
        Shot_Ins.insert(In_Value);
        if (Shot_In > In_Value)
            Shot_In = In_Value;
        if (Shot_Out < In_Value + Duration_Value)
            Shot_Out = In_Value + Duration_Value;
    };
    const auto Shot_Parse = [&](XMLElement* Shot)
    {
        if (XMLElement* Record = Child(Shot, "Record"))
        {
            XMLElement* In = Child(Record, "In");
            XMLElement* Duration = Child(Record, "Duration");
            const char* In_Text = In ? In->GetText() : nullptr;
            const char* Duration_Text = Duration ? Duration->GetText() : nullptr;
            if (In_Text && Duration_Text)
                Shot_Add(Ztring().From_UTF8(In_Text).To_int64u(), Ztring().From_UTF8(Duration_Text).To_int64u());
        }
        Shot_HasL5 = false;
        for (XMLElement* Shot_Item = Shot->FirstChildElement(); Shot_Item; Shot_Item = Shot_Item->NextSiblingElement())
        {
            if (Is(Shot_Item, "PluginNode"))
                DynamicData_Parse(Shot_Item);
            if (Is(Shot_Item, "Frame"))
                if (XMLElement* PluginNode = Child(Shot_Item, "PluginNode"))
                    DynamicData_Parse(PluginNode);
        }
        if (!Shot_HasL5)
            Shot_WithoutL5 = true;
    };
    // Version 6.x, tracks with channels
    const auto Gamut_Get = [&](XMLElement* Gamut, mastering_metadata_2086& Mastering)
    {
        const char* Name = Gamut ? Gamut->Attribute("name") : nullptr;
        if (!Name)
            return;
        float64 Values[8]; // Red, green, blue, white
        if (!strcmp(Name, "custom"))
        {
            const char* Text = Gamut->GetText();
            auto List = Values_Get(Text ? Text : "");
            if (List.size() != 8)
                return;
            for (int8u i = 0; i < 8; i++)
                Values[i] = List[i];
        }
        else
        {
            static const struct
            {
                const char* Name;
                float64 Values[8];
            } Gamuts[] =
            {
                { "bt709" , { 0.640, 0.330, 0.300, 0.600, 0.150, 0.060, 0.3127 , 0.3290  } },
                { "p3d65" , { 0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127 , 0.3290  } },
                { "p3d60" , { 0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.32168, 0.33767 } },
                { "p3dci" , { 0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3140 , 0.3510  } },
                { "bt2020", { 0.708, 0.292, 0.170, 0.797, 0.131, 0.046, 0.3127 , 0.3290  } },
            };
            size_t i = 0;
            while (i < sizeof(Gamuts) / sizeof(*Gamuts) && strcmp(Gamuts[i].Name, Name))
                i++;
            if (i >= sizeof(Gamuts) / sizeof(*Gamuts))
                return;
            memcpy(Values, Gamuts[i].Values, sizeof(Values));
        }
        static const int8u Order[8] = { 4, 5, 0, 1, 2, 3, 6, 7 }; // Red, green, blue, white to green, blue, red, white
        for (int8u i = 0; i < 8; i++)
            Mastering.Primaries[Order[i]] = (int16u)float64_int64s(Values[i] * 50000);
    };
    const auto Luminance_Get = [&](XMLElement* Luminance, mastering_metadata_2086& Mastering)
    {
        const char* Text = Luminance ? Luminance->GetText() : nullptr;
        auto List = Values_Get(Text ? Text : ""); // Minimum, maximum
        if (List.size() != 2)
            return;
        Mastering.Luminance[0] = (int32u)float64_int64s(List[0] * 10000);
        Mastering.Luminance[1] = (int32u)float64_int64s(List[1] * 10000);
    };
    const auto Tracks_Parse = [&]()
    {
        float64 MaxCLL = 0, MaxFALL = 0;
        for (auto Track : Tracks)
        {
            if (XMLElement* UniversalData = Child(Track, "UniversalData"))
            {
                XMLElement* ColorEncoding = Child(UniversalData, "ColorEncoding");
                if (ColorEncoding)
                {
                    mastering_metadata_2086 Mastering;
                    memset(&Mastering, 0xFF, sizeof(Mastering));
                    Gamut_Get(Child(ColorEncoding, "Gamut"), Mastering);
                    Ztring colour_primaries;
                    Get_MasteringDisplayColorVolume(colour_primaries, colour_primaries, Mastering); // Second part is not used
                    if (!colour_primaries.empty())
                        Fill(Stream_Video, 0, Video_colour_primaries, colour_primaries);
                    if (XMLElement* Transfer = Child(ColorEncoding, "Transfer"))
                        if (const char* Name = Transfer->Attribute("name"))
                            Fill(Stream_Video, 0, Video_transfer_characteristics, !strcmp(Name, "pq") ? "PQ" : (!strcmp(Name, "hlg") ? "HLG" : Name));
                    if (XMLElement* ColorRange = Child(ColorEncoding, "ColorRange"))
                        if (const char* Name = ColorRange->Attribute("name"))
                            Fill(Stream_Video, 0, Video_colour_range, !strcmp(Name, "full") ? "Full" : (!strcmp(Name, "legal") ? "Limited" : Name));
                }
                XMLElement* FrameRate = Child(UniversalData, "FrameRate");
                if (FrameRate)
                {
                    const char* Text = FrameRate->GetText();
                    auto List = Values_Get(Text ? Text : ""); // Numerator, denominator
                    if (List.size() == 2 && List[0] && List[1])
                        Fill(Stream_Video, 0, Video_FrameRate, List[0] / List[1]);
                }
                if (XMLElement* ShotList = Child(UniversalData, "ShotList"))
                {
                    for (XMLElement* Shot = Child(ShotList, "Shot"); Shot; Shot = Next(Shot, "Shot"))
                    {
                        const char* Text = Shot->GetText();
                        auto List = Values_Get(Text ? Text : ""); // First frame, frame count
                        if (List.size() == 2)
                            Shot_Add((int64u)List[0], (int64u)List[1]);
                    }
                }
            }
            for (XMLElement* Channel = Child(Track, "Channel"); Channel; Channel = Next(Channel, "Channel"))
            {
                for (XMLElement* Item = Channel->FirstChildElement(); Item; Item = Item->NextSiblingElement())
                {
                    const char* Text = Item->GetText();
                    auto Values = Values_Get(Text ? Text : "");
                    if (Is(Item, "MasteringDisplay") || Is(Item, "L0"))
                    {
                        mastering_metadata_2086 Mastering;
                        memset(&Mastering, 0xFF, sizeof(Mastering));
                        Gamut_Get(Child(Item, "Gamut"), Mastering);
                        Luminance_Get(Child(Item, "Luminance"), Mastering);
                        MasteringDisplay_Values(Mastering);
                        if (Is(Item, "MasteringDisplay"))
                        {
                            if (XMLElement* Name = Child(Item, "Name"))
                                if (const char* Name_Text = Name->GetText())
                                    DV.MasteringDisplay_Name.From_UTF8(Name_Text);
                            Ztring colour_primaries, luminance;
                            Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                            if (!colour_primaries.empty() && Retrieve_Const(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries).empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                            if (!luminance.empty() && Retrieve_Const(Stream_Video, 0, Video_MasteringDisplay_Luminance).empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                        }
                    }
                    if (Is(Item, "History"))
                    {
                        XMLElement* Revision_Last = nullptr;
                        for (XMLElement* Revision = Child(Item, "Revision"); Revision; Revision = Next(Revision, "Revision"))
                            Revision_Last = Revision;
                        if (Revision_Last)
                        {
                            if (XMLElement* Author = Child(Revision_Last, "Author"))
                                if (const char* Value = Author->GetText())
                                    Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Value, Unlimited, true, true);
                            if (XMLElement* Software = Child(Revision_Last, "Software"))
                                if (const char* Value = Software->GetText())
                                    Fill(Stream_General, 0, General_Encoded_Application_Name, Value, Unlimited, true, true);
                            if (XMLElement* Software_Version = Child(Revision_Last, "Version"))
                                if (const char* Value = Software_Version->GetText())
                                    Fill(Stream_General, 0, General_Encoded_Application_Version, Value, Unlimited, true, true);
                            if (XMLElement* DateTime = Child(Revision_Last, "DateTime"))
                                if (const char* Value = DateTime->GetText())
                                    Fill(Stream_General, 0, General_Encoded_Date, Value, Unlimited, true, true);
                        }
                    }
                    if (IsWholeStream && Is(Item, "L1") && Values.size() == 3 && Values[1] >= 0 && Values[2] >= 0 && Values[1] <= 1 && Values[2] <= 1)
                    {
                        // Minimum, average, maximum
                        auto avg_PQ = (int16u)float64_int64s(Values[1] * 4095);
                        auto max_PQ = (int16u)float64_int64s(Values[2] * 4095);
                        if (DV.L1_avg_PQ < avg_PQ)
                            DV.L1_avg_PQ = avg_PQ;
                        if (DV.L1_max_PQ < max_PQ)
                            DV.L1_max_PQ = max_PQ;
                    }
                    if (Is(Item, "L5") && Values.size() == 6 && Values[2] + Values[3] < Values[0] && Values[4] + Values[5] < Values[1])
                    {
                        // Canvas width and height (e.g. 1920 1080 or 1777780 1000000), then left, right, top, bottom offsets in the same unit
                        auto AspectRatio = (Values[0] - Values[2] - Values[3]) / (Values[1] - Values[4] - Values[5]);
                        AspectRatio = float64_int64s(AspectRatio * 100000) / 100000.0;
                        if (AspectRatio > 0 && find(DV.Active_DisplayAspectRatios.begin(), DV.Active_DisplayAspectRatios.end(), AspectRatio) == DV.Active_DisplayAspectRatios.end())
                            DV.Active_DisplayAspectRatios.push_back(AspectRatio);
                    }
                    if (Is(Item, "L6") && Values.size() == 2)
                    {
                        // MaxFALL, MaxCLL
                        DV_RPU::l6 L6{};
                        L6.max_content_light_level = Values[1];
                        L6.max_frame_average_light_level = Values[0];
                        if ((L6.max_content_light_level || L6.max_frame_average_light_level) && find(DV.L6.begin(), DV.L6.end(), L6) == DV.L6.end())
                            DV.L6.push_back(L6);
                        if (MaxCLL < Values[1])
                            MaxCLL = Values[1];
                        if (MaxFALL < Values[0])
                            MaxFALL = Values[0];
                    }
                    if (Is(Item, "L9"))
                    {
                        mastering_metadata_2086 Mastering;
                        memset(&Mastering, 0xFF, sizeof(Mastering));
                        Gamut_Get(Child(Item, "Gamut"), Mastering);
                        MasteringDisplay_Values(Mastering);
                    }
                    if (Is(Item, "L10"))
                    {
                        for (XMLElement* Target = Child(Item, "Target"); Target; Target = Next(Target, "Target"))
                        {
                            XMLElement* ID = Child(Target, "ID");
                            const char* ID_Text = ID ? ID->GetText() : nullptr;
                            if (!ID_Text)
                                continue;
                            auto Index = (int8u)atoi(ID_Text);
                            if (find(DV.L8_target_display_index.begin(), DV.L8_target_display_index.end(), Index) != DV.L8_target_display_index.end())
                                continue;
                            DV.L8_target_display_index.push_back(Index);
                            mastering_metadata_2086 Mastering;
                            memset(&Mastering, 0xFF, sizeof(Mastering));
                            Luminance_Get(Child(Target, "Luminance"), Mastering);
                            if (Mastering.Luminance[1] != (int32u)-1)
                                DV.L10.push_back({ Index, 0, DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[1]) / 10000), DolbyVision_Nits_ToPQ12(((float64)Mastering.Luminance[0]) / 10000) });
                        }
                    }
                    if (Is(Item, "L11") && Values.size() == 2)
                    {
                        // Content type, intended white point
                        DV_RPU::l11 L11{ (int8u)Values[0], (int8u)Values[1], (int8u)-1 };
                        if (const char* UseInReferenceMode = Item->Attribute("useInReferenceMode"))
                            L11.reference_mode = (!strcmp(UseInReferenceMode, "true") || !strcmp(UseInReferenceMode, "1")) ? 1 : 0;
                        if (find(DV.L11.begin(), DV.L11.end(), L11) == DV.L11.end())
                            DV.L11.push_back(L11);
                    }
                    if (Is(Item, "L254") && Values.size() == 4)
                    {
                        // DM mode, DM version, CM version
                        DV.L254_Present = true;
                        DV.dm_mode = (int8u)Values[0];
                        DV.dm_version_index = (int8u)Values[1];
                        DV.CMVersion = Ztring::ToZtring((int64u)Values[2]) + __T('.') + Ztring::ToZtring((int64u)Values[3]);
                    }
                }
            }
        }
        if (MaxCLL)
            Fill(Stream_Video, 0, Video_MaxCLL, MaxCLL, 0);
        if (MaxFALL)
            Fill(Stream_Video, 0, Video_MaxFALL, MaxFALL, 0);
    };
    for (auto MasterElement : MasterElements)
    {
    if (!MasterElement)
        continue;
    if (IsWholeStream && Is(MasterElement, "DolbyVisionFrameData") && Child(MasterElement, "Record"))
        Shot_Parse(MasterElement); // Version 2.x, the shot data is directly in the frame data
    for (XMLElement* DolbyVisionGlobalData_Item=MasterElement->FirstChildElement(); DolbyVisionGlobalData_Item; DolbyVisionGlobalData_Item=DolbyVisionGlobalData_Item->NextSiblingElement())
    {
        if (Is(DolbyVisionGlobalData_Item, "Shot"))
        {
            if (IsWholeStream) // Not the shot of a frame only
                Shot_Parse(DolbyVisionGlobalData_Item);
            continue;
        }
        if (Is(DolbyVisionGlobalData_Item, "ColorEncoding"))
        {
            mastering_metadata_2086 Mastering;
            memset(&Mastering, 0xFF, sizeof(Mastering));
            for (XMLElement* ColorEncoding_Item=DolbyVisionGlobalData_Item->FirstChildElement(); ColorEncoding_Item; ColorEncoding_Item=ColorEncoding_Item->NextSiblingElement())
            {
                if (Is(ColorEncoding_Item, "BitDepth"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        Fill(Stream_Video, 0, Video_BitDepth, Text);
                    }
                }
                if (Is(ColorEncoding_Item, "ChromaFormat"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (strlen(Text) == 3 && Text[0] >= '0' && Text[0] <= '9' && Text[1] >= '0' && Text[1] <= '9' && Text[2] >= '0' && Text[2] <= '9')
                        {
                            const char ChromaSubsampling[] = { Text[0], ':', Text[1], ':', Text[2], '\0' };
                            Fill(Stream_Video, 0, Video_ChromaSubsampling, ChromaSubsampling);
                        }
                        else
                            Fill(Stream_Video, 0, Video_ChromaSubsampling, Text);
                    }
                }
                if (Is(ColorEncoding_Item, "CanvasAspectRatio"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                        if (float32 TextF = Ztring().From_UTF8(Text).To_float32())
                            Fill(Stream_Video, 0, Video_DisplayAspectRatio, TextF);
                }
                if (false) // Is(ColorEncoding_Item, "ColorSpace"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "rgb"))
                            Text = "RGB";
                        if (!strcmp(Text, "yuv"))
                            Text = "YUV";
                        Fill(Stream_Video, 0, Video_ColorSpace, Text);
                    }
                }
                if (Is(ColorEncoding_Item, "Encoding"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "pq"))
                            Text="PQ";
                        Fill(Stream_Video, 0, Video_transfer_characteristics, Text);
                    }
                }
                if (Is(ColorEncoding_Item, "Primaries"))
                {
                    for (XMLElement* Primaries_Item=ColorEncoding_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                    {
                        int8u i=(int8u)-1;
                        if (Is(Primaries_Item, "Green"))
                            i=0;
                        if (Is(Primaries_Item, "Blue"))
                            i=2;
                        if (Is(Primaries_Item, "Red"))
                            i=4;
                        if (i!=(int8u)-1)
                        {
                            if (const char* Text=Primaries_Item->GetText())
                            {
                                ZtringList List;
                                List.Separator_Set(0, __T(" "));
                                List.Write(Ztring(Text));
                                if (List.size()==1)
                                {
                                    //Trying with space
                                    List.Separator_Set(0, __T(","));
                                    List.Write(Ztring(Text));
                                }
                                if (List.size()==2)
                                {
                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                }
                            }
                        }
                    }
                }
                if (Is(ColorEncoding_Item, "SignalRange"))
                {
                    if (const char* Text = ColorEncoding_Item->GetText())
                    {
                        if (!strcmp(Text, "computer"))
                            Text = "Full";
                        Fill(Stream_Video, 0, Video_colour_range, Text);
                    }
                }
                if (Is(ColorEncoding_Item, "WhitePoint"))
                {
                    if (const char* Text=ColorEncoding_Item->GetText())
                    {
                        ZtringList List;
                        List.Separator_Set(0, __T(" "));
                        List.Write(Ztring(Text));
                        if (List.size()==1)
                        {
                            //Trying with space
                            List.Separator_Set(0, __T(","));
                            List.Write(Ztring(Text));
                        }
                        if (List.size()==2)
                        {
                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                        }
                    }
                }
            }
            Ztring colour_primaries;
            Get_MasteringDisplayColorVolume(colour_primaries, colour_primaries, Mastering); // Second part is not used
            if (!colour_primaries.empty())
                Fill(Stream_Video, 0, Video_colour_primaries, colour_primaries);
        }
        if (Is(DolbyVisionGlobalData_Item, "EditRate"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
            {
                float32 FrameRate = Ztring().From_UTF8(Text).To_float32();
                if (FrameRate)
                {
                    Text = strchr(Text, ' ');
                    if (Text)
                    {
                        Text++;
                        float32 FrameRateD = Ztring().From_UTF8(Text).To_float32();
                        if (FrameRateD)
                            FrameRate /= FrameRateD;
                    }
                }
                Fill(Stream_Video, 0, Video_FrameRate, FrameRate);
            }
        }
        if (Is(DolbyVisionGlobalData_Item, "Level6"))
        {
            DV_RPU::l6 L6{};
            for (XMLElement* Level6_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Level6_Item; Level6_Item = Level6_Item->NextSiblingElement())
            {
                if (Is(Level6_Item, "MaxCLL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if ((L6.max_content_light_level = Ztring().From_UTF8(Text).To_float64()))
                            Fill(Stream_Video, 0, Video_MaxCLL, Ztring(Text));
                }
                if (Is(Level6_Item, "MaxFALL"))
                {
                    if (const char* Text = Level6_Item->GetText())
                        if ((L6.max_frame_average_light_level = Ztring().From_UTF8(Text).To_float64()))
                            Fill(Stream_Video, 0, Video_MaxFALL, Ztring(Text));
                }
            }
            if (L6.max_content_light_level || L6.max_frame_average_light_level)
                DV.L6.push_back(L6);
        }
        if (Is(DolbyVisionGlobalData_Item, "PluginNode"))
        {
            for (XMLElement* PluginNode_Item=DolbyVisionGlobalData_Item->FirstChildElement(); PluginNode_Item; PluginNode_Item=PluginNode_Item->NextSiblingElement())
            {
                if (Is(PluginNode_Item, "DolbyEDR") || Is(PluginNode_Item, "DVGlobalData"))
                {
                    for (XMLElement* DolbyEDR_Item=PluginNode_Item->FirstChildElement(); DolbyEDR_Item; DolbyEDR_Item=DolbyEDR_Item->NextSiblingElement())
                    {
                        if (Is(DolbyEDR_Item, "Characteristics"))
                        {
                            for (XMLElement* Characteristics_Item=DolbyEDR_Item->FirstChildElement(); Characteristics_Item; Characteristics_Item=Characteristics_Item->NextSiblingElement())
                            {
                                if (Is(Characteristics_Item, "MasteringDisplay"))
                                {
                                    MasteringDisplay_Fill(Characteristics_Item);
                                    mastering_metadata_2086 Mastering;
                                    memset(&Mastering, 0xFF, sizeof(Mastering));
                                    for (XMLElement* MasteringDisplay_Item=Characteristics_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                                    {
                                        if (Is(MasteringDisplay_Item, "MinimumBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (Is(MasteringDisplay_Item, "PeakBrightness"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                                Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                        }
                                        if (Is(MasteringDisplay_Item, "Primaries"))
                                        {
                                            for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                            {
                                                int8u i=(int8u)-1;
                                                if (Is(Primaries_Item, "Green"))
                                                    i=0;
                                                if (Is(Primaries_Item, "Blue"))
                                                    i=2;
                                                if (Is(Primaries_Item, "Red"))
                                                    i=4;
                                                if (i!=(int8u)-1)
                                                {
                                                    if (const char* Text=Primaries_Item->GetText())
                                                    {
                                                        ZtringList List;
                                                        List.Separator_Set(0, __T(" "));
                                                        List.Write(Ztring(Text));
                                                        if (List.size()==1)
                                                        {
                                                            //Trying with space
                                                            List.Separator_Set(0, __T(","));
                                                            List.Write(Ztring(Text));
                                                        }
                                                        if (List.size()==2)
                                                        {
                                                            Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                            Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (Is(MasteringDisplay_Item, "WhitePoint"))
                                        {
                                            if (const char* Text=MasteringDisplay_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                    Ztring colour_primaries, luminance;
                                    MasteringDisplay_Values(Mastering);
                                    Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                                    if (!colour_primaries.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                                    if (!luminance.empty())
                                        Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                                }
                                if (Is(Characteristics_Item, "TargetDisplay"))
                                    TargetDisplay_Fill(Characteristics_Item);
                            }
                        }
                        if (Is(DolbyEDR_Item, "TargetDisplay"))
                            TargetDisplay_Fill(DolbyEDR_Item);
                        if (Is(DolbyEDR_Item, "MasteringDisplay"))
                        {
                            MasteringDisplay_Fill(DolbyEDR_Item);
                            mastering_metadata_2086 Mastering;
                            memset(&Mastering, 0xFF, sizeof(Mastering));
                            for (XMLElement* MasteringDisplay_Item=DolbyEDR_Item->FirstChildElement(); MasteringDisplay_Item; MasteringDisplay_Item=MasteringDisplay_Item->NextSiblingElement())
                            {
                                if (Is(MasteringDisplay_Item, "MinimumBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[0]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (Is(MasteringDisplay_Item, "PeakBrightness"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                        Mastering.Luminance[1]=float64_int64s(Ztring(Text).To_float64()*10000);
                                }
                                if (Is(MasteringDisplay_Item, "Primaries"))
                                {
                                    for (XMLElement* Primaries_Item=MasteringDisplay_Item->FirstChildElement(); Primaries_Item; Primaries_Item=Primaries_Item->NextSiblingElement())
                                    {
                                        int8u i=(int8u)-1;
                                        if (Is(Primaries_Item, "Green"))
                                            i=0;
                                        if (Is(Primaries_Item, "Blue"))
                                            i=2;
                                        if (Is(Primaries_Item, "Red"))
                                            i=4;
                                        if (i!=(int8u)-1)
                                        {
                                            if (const char* Text=Primaries_Item->GetText())
                                            {
                                                ZtringList List;
                                                List.Separator_Set(0, __T(" "));
                                                List.Write(Ztring(Text));
                                                if (List.size()==1)
                                                {
                                                    //Trying with space
                                                    List.Separator_Set(0, __T(","));
                                                    List.Write(Ztring(Text));
                                                }
                                                if (List.size()==2)
                                                {
                                                    Mastering.Primaries[i]=float64_int64s(List[0].To_float64()*50000);
                                                    Mastering.Primaries[i+1]=float64_int64s(List[1].To_float64()*50000);
                                                }
                                            }
                                        }
                                    }
                                }
                                if (Is(MasteringDisplay_Item, "WhitePoint"))
                                {
                                    if (const char* Text=MasteringDisplay_Item->GetText())
                                    {
                                        ZtringList List;
                                        List.Separator_Set(0, __T(" "));
                                        List.Write(Ztring(Text));
                                        if (List.size()==1)
                                        {
                                            //Trying with space
                                            List.Separator_Set(0, __T(","));
                                            List.Write(Ztring(Text));
                                        }
                                        if (List.size()==2)
                                        {
                                            Mastering.Primaries[6]=float64_int64s(List[0].To_float64()*50000);
                                            Mastering.Primaries[7]=float64_int64s(List[1].To_float64()*50000);
                                        }
                                    }
                                }
                            }
                            Ztring colour_primaries, luminance;
                            MasteringDisplay_Values(Mastering);
                            Get_MasteringDisplayColorVolume(colour_primaries, luminance, Mastering);
                            if (!colour_primaries.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_ColorPrimaries, colour_primaries);
                            if (!luminance.empty())
                                Fill(Stream_Video, 0, Video_MasteringDisplay_Luminance, luminance);
                        }
                    }
                }
                if (Is(PluginNode_Item, "Level254"))
                {
                    for (XMLElement* Level254_Item = PluginNode_Item->FirstChildElement(); Level254_Item; Level254_Item = Level254_Item->NextSiblingElement())
                    {
                        const char* Text = Level254_Item->GetText();
                        if (!Text)
                            continue;
                        if (Is(Level254_Item, "CMVersion"))
                        {
                            // "4 1" is displayed as "4.1"
                            Ztring CMVersion;
                            CMVersion.From_UTF8(Text);
                            CMVersion.FindAndReplace(__T(" "), __T("."), 0, Ztring_Recursive);
                            CMVersion.FindAndReplace(__T(","), __T("."), 0, Ztring_Recursive);
                            DV.CMVersion = CMVersion;
                        }
                        if (Is(Level254_Item, "DMVersion"))
                        {
                            DV.L254_Present = true;
                            DV.dm_version_index = (int8u)atoi(Text);
                        }
                        if (Is(Level254_Item, "DMMode"))
                        {
                            DV.L254_Present = true;
                            DV.dm_mode = (int8u)atoi(Text);
                        }
                    }
                }
            }
        }
        if (Is(DolbyVisionGlobalData_Item, "Rate"))
        {
            int64u n = 0, d = 0;
            for (XMLElement* Rate_Item = DolbyVisionGlobalData_Item->FirstChildElement(); Rate_Item; Rate_Item = Rate_Item->NextSiblingElement())
            {
                if (Is(Rate_Item, "d"))
                    d = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
                if (Is(Rate_Item, "n"))
                    n = Ztring().From_UTF8(Rate_Item->GetText()).To_float32();
            }
            if (n && d)
                Fill(Stream_Video, 0, Video_FrameRate, ((float32)n) / d);
        }
        if (Is(DolbyVisionGlobalData_Item, "TrackName"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_Title, Text, Unlimited, true, true);
        }
        if (Is(DolbyVisionGlobalData_Item, "UniqueID"))
        {
            if (const char* Text = DolbyVisionGlobalData_Item->GetText())
                Fill(Stream_Other, 0, Other_UniqueID, Text);
        }
    }
    }
    if (IsV6)
        Tracks_Parse();
    if (!Shot_Ins.empty())
    {
        DV.Count_Shot = (int32u)Shot_Ins.size();
        if (Shot_Out > Shot_In)
        {
            Fill(Stream_Video, 0, Video_FrameCount, Shot_Out - Shot_In);
            DV.FrameRange_Start = Shot_In;
            DV.FrameRange_End = Shot_Out - 1;
        }
    }
    if (Shot_WithoutL5 && ImageAspectRatio_Default > 0 && find(DV.Active_DisplayAspectRatios.begin(), DV.Active_DisplayAspectRatios.end(), ImageAspectRatio_Default) == DV.Active_DisplayAspectRatios.end())
        DV.Active_DisplayAspectRatios.push_back(ImageAspectRatio_Default);
    if (!Structure_Missing)
        DolbyVision_Streams_Fill(DV, DV_Source_XML);

    auto HasSchema = Conformance_Schema(Root, Version);
    if (MDF_Output || Structure_Missing)
        Conformance_MDF(Root, MasterElements[1], Version, HasSchema);
    if (Structure_Missing && Version_IsSupported(Version))
        Fill_Conformance("DolbyVision Structure", string(Structure_Missing) + " element is missing", {}, Conformance_Error, Stream_Video, 0);
    if (IsV6)
        Conformance_V6(Root);

    //All should be OK...
    Element_Offset=File_Size;
    return true;
}

//***************************************************************************
// Schema, basic checks based on the XSD (elements, attributes, counts, order, values)
//***************************************************************************

//---------------------------------------------------------------------------
namespace
{

enum xsd_type : int8u
{
    XSD_Complex,            // Child elements only
    XSD_Empty,              // No content
    XSD_String,
    XSD_NonNegativeInteger,
    XSD_Integer,
    XSD_Boolean,
    XSD_Decimal,
    XSD_Float,
    XSD_DateTime,
    XSD_Uuid,
    XSD_Enum,
    XSD_Gamut,              // Version 6.x, content depends on the name attribute
    XSD_Transfer,           // Version 6.x, content depends on the name attribute
};

struct xsd_value
{
    xsd_type            Type;
    float64             Min;
    float64             Max;
    bool                Min_Exclusive;
    int8u               List_Min;       // 0 if not a list
    int8u               List_Max;
    const char* const*  Enums;          // Null terminated
};

static constexpr float64 Inf = std::numeric_limits<float64>::infinity();
static constexpr xsd_value XSD_Value(xsd_type Type, float64 Min = -Inf, float64 Max = Inf, int8u List_Min = 0, int8u List_Max = 0, const char* const* Enums = nullptr, bool Min_Exclusive = false)
{
    return { Type, Min, Max, Min_Exclusive, List_Min, List_Max, Enums };
}

struct xsd_attribute
{
    const char*         Name;
    bool                Required;
    const xsd_value*    Value;
    const char*         Fixed;          // Fixed value
};

enum xsd_flags : int8u
{
    XSD_Any         = 1 << 0,           // Any content (extension properties)
    XSD_Unordered   = 1 << 1,           // Repeated sequence, the order is not checked
    XSD_Choice      = 1 << 2,           // Only one kind of child element
};

enum xsd_version : int8u
{
    V4 = 1 << 0,                        // 4.0.2
    V5 = 1 << 1,                        // 5.1.0
};

struct xsd_element
{
    const char*             Name;
    int8u                   Min;
    int8u                   Max;        // 0 means unbounded
    int8u                   Versions;   // 0 means all versions
    const xsd_value*        Value;
    const xsd_element*      Children;   // Terminated by a null name
    const xsd_attribute*    Attributes; // Terminated by a null name
    int8u                   Flags;
};

//---------------------------------------------------------------------------
// Values
static const char* const Encoding_V4[] = { "pq", "linear", "gamma_bt1886", "gamma_dci", "gamma_22", "gamma_24", nullptr };
static const char* const Encoding_V5[] = { "pq", "linear", "gamma_bt1886", "gamma_dci", "gamma_22", "gamma_24", "hlg", nullptr };
static const char* const ColorSpace_V4[] = { "rgb", "xyz", "ycbcr_bt709", "ycbcr_bt2020", "ycbcr_native", nullptr };
static const char* const SignalRange_V4[] = { "computer", "video", nullptr };
static const char* const ApplicationType_V5[] = { "ALL", "HOME", "CINEMA", nullptr };
static const char* const ColorSpace_V6[] = { "rgb", "ycbcr_bt709", "ycbcr_bt2020", "ycbcr_native", "xyz", nullptr };
static const char* const ColorRange_V6[] = { "legal", "full", nullptr };
static const char* const Gamut_V6[] = { "bt709", "p3d65", "p3d60", "p3dci", "bt2020", "dolbywcg", "xyz", "custom", nullptr };
static const char* const Transfer_V6[] = { "linear", "gamma", "bt1886", "dcigamma", "pq", "hlg", nullptr };
static const char* const TrackType_V6[] = { "GlobalDolbyVision", "DynamicDolbyVision", nullptr };
static const char* const ChannelType_V6[] = { "History", "MasteringDisplay", "CreationProfile", "SDRDefinition", "AnchorTarget", "CreativeEnvironment", "L0", "L1", "L2", "L3", "L4", "L5", "L6", "L8", "L9", "L10", "L11", "L11x", "L15", "L16", "L18", "L254", nullptr };

static constexpr xsd_value V_Complex = XSD_Value(XSD_Complex);
static constexpr xsd_value V_Empty = XSD_Value(XSD_Empty);
static constexpr xsd_value V_String = XSD_Value(XSD_String);
static constexpr xsd_value V_NonNegativeInteger = XSD_Value(XSD_NonNegativeInteger, 0);
static constexpr xsd_value V_Integer = XSD_Value(XSD_Integer);
static constexpr xsd_value V_Boolean = XSD_Value(XSD_Boolean);
static constexpr xsd_value V_Decimal = XSD_Value(XSD_Decimal);
static constexpr xsd_value V_DateTime = XSD_Value(XSD_DateTime);
static constexpr xsd_value V_Uuid = XSD_Value(XSD_Uuid);
static constexpr xsd_value V_xy = XSD_Value(XSD_Decimal, 0, 1, 2, 2);
static constexpr xsd_value V_PeakBrightness = XSD_Value(XSD_Decimal, 0, 10000);
static constexpr xsd_value V_MinimumBrightness = XSD_Value(XSD_Decimal, 0, 9.9999);
static constexpr xsd_value V_Encoding_V4 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, Encoding_V4);
static constexpr xsd_value V_Encoding_V5 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, Encoding_V5);
static constexpr xsd_value V_ColorSpace_V4 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, ColorSpace_V4);
static constexpr xsd_value V_SignalRange_V4 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, SignalRange_V4);
static constexpr xsd_value V_ApplicationType_V5 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, ApplicationType_V5);
static constexpr xsd_value V_FloatPositive = XSD_Value(XSD_Float, 0);
static constexpr xsd_value V_PQ_3 = XSD_Value(XSD_Float, 0, 1, 3, 3);
static constexpr xsd_value V_Param = XSD_Value(XSD_Float, -1, 1);
static constexpr xsd_value V_Param_3 = XSD_Value(XSD_Float, -1, 1, 3, 3);
static constexpr xsd_value V_Param_6 = XSD_Value(XSD_Float, -1, 1, 6, 6);
static constexpr xsd_value V_Param_9 = XSD_Value(XSD_Float, -1, 1, 9, 9);
static constexpr xsd_value V_AspectRatios = XSD_Value(XSD_Decimal, 0, Inf, 2, 2, nullptr, true);
static constexpr xsd_value V_SourceColorModel = XSD_Value(XSD_NonNegativeInteger, 255, 255);
static constexpr xsd_value V_SourceColorPrimary = XSD_Value(XSD_Decimal, -1, 1, 8, 8);
static constexpr xsd_value V_DMMode = XSD_Value(XSD_NonNegativeInteger, 0, 1);
static constexpr xsd_value V_DMVersion = XSD_Value(XSD_NonNegativeInteger, 0, 255);
static constexpr xsd_value V_CMVersion = XSD_Value(XSD_NonNegativeInteger, 0, Inf, 2, 2);
static constexpr xsd_value V_ColorSpace_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, ColorSpace_V6);
static constexpr xsd_value V_ColorRange_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, ColorRange_V6);
static constexpr xsd_value V_GamutName_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, Gamut_V6);
static constexpr xsd_value V_TransferName_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, Transfer_V6);
static constexpr xsd_value V_TrackType_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, TrackType_V6);
static constexpr xsd_value V_ChannelType_V6 = XSD_Value(XSD_Enum, -Inf, Inf, 0, 0, ChannelType_V6);
static constexpr xsd_value V_Gamut = XSD_Value(XSD_Gamut);
static constexpr xsd_value V_Transfer = XSD_Value(XSD_Transfer);
static constexpr xsd_value V_UnsignedInt = XSD_Value(XSD_NonNegativeInteger, 0, 4294967295.0);
static constexpr xsd_value V_UnsignedInt_2 = XSD_Value(XSD_NonNegativeInteger, 0, 4294967295.0, 2, 2);
static constexpr xsd_value V_UnsignedInt_4 = XSD_Value(XSD_NonNegativeInteger, 0, 4294967295.0, 4, 4);
static constexpr xsd_value V_UnsignedInt_6 = XSD_Value(XSD_NonNegativeInteger, 0, 4294967295.0, 6, 6);
static constexpr xsd_value V_FrameRange = XSD_Value(XSD_NonNegativeInteger, 0, 4294967295.0, 1, 2);
static constexpr xsd_value V_NormalFloat = XSD_Value(XSD_Float, 0, 1);
static constexpr xsd_value V_NormalFloat_2 = XSD_Value(XSD_Float, 0, 1, 2, 2);
static constexpr xsd_value V_NormalFloat_3 = XSD_Value(XSD_Float, 0, 1, 3, 3);
static constexpr xsd_value V_NormalFloat_4 = XSD_Value(XSD_Float, 0, 1, 4, 4);
static constexpr xsd_value V_NormalFloat_1to7 = XSD_Value(XSD_Float, 0, 1, 1, 7);
static constexpr xsd_value V_FinitePositiveFloat_2 = XSD_Value(XSD_Float, 0, std::numeric_limits<float64>::max(), 2, 2);
static constexpr xsd_value V_InfiniteFloat_3 = XSD_Value(XSD_Float, -Inf, std::numeric_limits<float64>::max(), 3, 3);
static constexpr xsd_value V_MinusOneToOne_3 = XSD_Value(XSD_Float, -1, 1, 3, 3);
static constexpr xsd_value V_MinusOneToOne_4 = XSD_Value(XSD_Float, -1, 1, 4, 4);
static constexpr xsd_value V_MinusOneToOne_6 = XSD_Value(XSD_Float, -1, 1, 6, 6);
static constexpr xsd_value V_MinusOneToOne_8 = XSD_Value(XSD_Float, -1, 1, 8, 8);
static constexpr xsd_value V_MinusOneToOne_10 = XSD_Value(XSD_Float, -1, 1, 10, 10);
static constexpr xsd_value V_Count15 = XSD_Value(XSD_Integer, 0, 15);
static constexpr xsd_value V_Count15_2 = XSD_Value(XSD_Integer, 0, 15, 2, 2);
static constexpr xsd_value V_Count255 = XSD_Value(XSD_Integer, 0, 255);

//---------------------------------------------------------------------------
// Attributes
static const xsd_attribute A_Level0[] = { { "level", true, &V_Integer, "0" }, {} };
static const xsd_attribute A_Level1[] = { { "level", true, &V_Integer, "1" }, {} };
static const xsd_attribute A_Level2[] = { { "level", true, &V_Integer, "2" }, {} };
static const xsd_attribute A_Level3[] = { { "level", true, &V_Integer, "3" }, {} };
static const xsd_attribute A_Level5[] = { { "level", true, &V_Integer, "5" }, {} };
static const xsd_attribute A_Level6[] = { { "level", true, &V_Integer, "6" }, {} };
static const xsd_attribute A_Level8[] = { { "level", true, &V_Integer, "8" }, {} };
static const xsd_attribute A_Level9[] = { { "level", true, &V_Integer, "9" }, {} };
static const xsd_attribute A_Level11[] = { { "level", true, &V_Integer, "11" }, {} };
static const xsd_attribute A_Level254[] = { { "level", true, &V_Integer, "254" }, {} };
static const xsd_attribute A6_f[] = { { "f", false, &V_FrameRange }, {} };
static const xsd_attribute A6_ColorSpace[] = { { "name", true, &V_ColorSpace_V6 }, {} };
static const xsd_attribute A6_ColorRange[] = { { "name", true, &V_ColorRange_V6 }, {} };
static const xsd_attribute A6_Gamut[] = { { "name", true, &V_GamutName_V6 }, {} };
static const xsd_attribute A6_Transfer[] = { { "name", true, &V_TransferName_V6 }, {} };
static const xsd_attribute A6_Trim[] = { { "t", true, &V_UnsignedInt }, {} };
static const xsd_attribute A6_L8_Item[] = { { "t", true, &V_Integer }, {} };
static const xsd_attribute A6_L16_Params[] = { { "t", false, &V_Count255 }, {} };
static const xsd_attribute A6_L11[] = { { "f", false, &V_FrameRange }, { "useInReferenceMode", false, &V_Boolean }, {} };
static const xsd_attribute A6_L11x[] = { { "f", false, &V_FrameRange }, { "frc", false, &V_Boolean }, {} };
static const xsd_attribute A6_Revision[] = { { "f", false, &V_FrameRange }, { "revision", true, &V_Count15 }, {} };
static const xsd_attribute A6_Track[] = { { "type", true, &V_TrackType_V6 }, { "name", false, &V_String }, { "uuid", true, &V_Uuid }, {} };
static const xsd_attribute A6_Channel[] = { { "type", true, &V_ChannelType_V6 }, { "name", true, &V_String }, {} };

//---------------------------------------------------------------------------
// Version 4.0.2 and 5.1.0 elements
static const xsd_element E_Primaries[] =
{
    { "Red"                     , 1, 1, 0 , &V_xy },
    { "Green"                   , 1, 1, 0 , &V_xy },
    { "Blue"                    , 1, 1, 0 , &V_xy },
    {}
};
static const xsd_element E_ColorEncoding[] =
{
    { "Primaries"               , 1, 1, 0 , &V_Complex, E_Primaries },
    { "WhitePoint"              , 1, 1, 0 , &V_xy },
    { "PeakBrightness"          , 1, 1, 0 , &V_NonNegativeInteger },
    { "MinimumBrightness"       , 1, 1, 0 , &V_NonNegativeInteger },
    { "Encoding"                , 1, 1, V4, &V_Encoding_V4 },
    { "Encoding"                , 1, 1, V5, &V_Encoding_V5 },
    { "ColorSpace"              , 1, 1, 0 , &V_ColorSpace_V4 },
    { "SignalRange"             , 1, 1, 0 , &V_SignalRange_V4 },
    {}
};
static const xsd_element E_Display[] =
{
    { "ID"                      , 1, 1, 0 , &V_String },
    { "Name"                    , 1, 1, 0 , &V_String },
    { "Primaries"               , 1, 1, 0 , &V_Complex, E_Primaries },
    { "WhitePoint"              , 1, 1, 0 , &V_xy },
    { "PeakBrightness"          , 1, 1, 0 , &V_PeakBrightness },
    { "MinimumBrightness"       , 1, 1, 0 , &V_MinimumBrightness },
    { "EOTF"                    , 0, 1, V4, &V_Encoding_V4 },
    { "EOTF"                    , 0, 1, V5, &V_Encoding_V5 },
    { "DiagonalSize"            , 1, 1, 0 , &V_NonNegativeInteger },
    { "ApplicationType"         , 1, 1, V5, &V_ApplicationType_V5 },
    {}
};
static const xsd_element E_DVGlobalData[] =
{
    { "MasteringDisplay"        , 1, 1, 0 , &V_Complex, E_Display },
    { "TargetDisplay"           , 0, 0, 0 , &V_Complex, E_Display },
    {}
};
static const xsd_element E_Level1[] =
{
    { "ImageCharacter"          , 1, 1, 0 , &V_PQ_3 },
    {}
};
static const xsd_element E_Level2[] =
{
    { "TID"                     , 1, 1, 0 , &V_NonNegativeInteger },
    { "Trim"                    , 1, 1, 0 , &V_Param_9 },
    {}
};
static const xsd_element E_Level3[] =
{
    { "L1Offset"                , 1, 1, 0 , &V_Param_3 },
    {}
};
static const xsd_element E_Level5[] =
{
    { "AspectRatios"            , 1, 1, 0 , &V_AspectRatios },
    {}
};
static const xsd_element E_Level6[] =
{
    { "MaxCLL"                  , 1, 1, 0 , &V_FloatPositive },
    { "MaxFALL"                 , 1, 1, 0 , &V_FloatPositive },
    {}
};
static const xsd_element E_Level8[] =
{
    { "TID"                     , 1, 1, 0 , &V_NonNegativeInteger },
    { "L8Trim"                  , 1, 1, 0 , &V_Param_6 },
    { "MidContrastBias"         , 1, 1, 0 , &V_Param },
    { "HighlightClipping"       , 1, 1, 0 , &V_Param },
    { "SaturationVectorField"   , 1, 1, 0 , &V_Param_6 },
    { "HueVectorField"          , 1, 1, 0 , &V_Param_6 },
    {}
};
static const xsd_element E_Level9[] =
{
    { "SourceColorModel"        , 1, 1, 0 , &V_SourceColorModel },
    { "SourceColorPrimary"      , 1, 1, 0 , &V_SourceColorPrimary },
    {}
};
static const xsd_element E_Level11[] =
{
    { "ContentType"             , 1, 1, 0 , &V_NonNegativeInteger },
    { "IntendedWhitePoint"      , 1, 1, 0 , &V_NonNegativeInteger },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E_Level254[] =
{
    { "DMMode"                  , 1, 1, 0 , &V_DMMode },
    { "DMVersion"               , 1, 1, 0 , &V_DMVersion },
    { "CMVersion"               , 1, 1, 0 , &V_CMVersion },
    {}
};
static const xsd_element E_DVDynamicData[] =
{
    { "Level1"                  , 1, 1, 0 , &V_Complex, E_Level1, A_Level1 },
    { "Level2"                  , 0, 0, 0 , &V_Complex, E_Level2, A_Level2 },
    { "Level3"                  , 0, 1, 0 , &V_Complex, E_Level3, A_Level3 },
    { "Level5"                  , 0, 1, 0 , &V_Complex, E_Level5, A_Level5 },
    { "Level8"                  , 0, 0, 0 , &V_Complex, E_Level8, A_Level8 },
    { "Level9"                  , 0, 1, 0 , &V_Complex, E_Level9, A_Level9 },
    {}
};
static const xsd_element E_Shot_PluginNode[] =
{
    { "DVDynamicData"           , 1, 1, 0 , &V_Complex, E_DVDynamicData },
    { "Level11"                 , 0, 1, V5, &V_Complex, E_Level11, A_Level11 },
    {}
};
static const xsd_element E_Frame_PluginNode[] =
{
    { "DVDynamicData"           , 1, 1, 0 , &V_Complex, E_DVDynamicData },
    {}
};
static const xsd_element E_Frame[] =
{
    { "EditOffset"              , 1, 1, 0 , &V_NonNegativeInteger },
    { "PluginNode"              , 1, 1, 0 , &V_Complex, E_Frame_PluginNode },
    {}
};
static const xsd_element E_Record[] =
{
    { "In"                      , 1, 1, 0 , &V_NonNegativeInteger },
    { "Duration"                , 1, 1, 0 , &V_NonNegativeInteger },
    {}
};
static const xsd_element E_Shot[] =
{
    { "UniqueID"                , 1, 1, 0 , &V_String },
    { "Record"                  , 1, 1, 0 , &V_Complex, E_Record },
    { "PluginNode"              , 1, 1, 0 , &V_Complex, E_Shot_PluginNode },
    { "Frame"                   , 0, 0, 0 , &V_Complex, E_Frame },
    {}
};
static const xsd_element E_Track_PluginNode[] =
{
    { "DVGlobalData"            , 1, 1, 0 , &V_Complex, E_DVGlobalData, A_Level0 },
    { "Level11"                 , 1, 1, V5, &V_Complex, E_Level11, A_Level11 },
    { "Level254"                , 1, 1, 0 , &V_Complex, E_Level254, A_Level254 },
    {}
};
static const xsd_element E_Track[] =
{
    { "TrackName"               , 1, 1, 0 , &V_String },
    { "UniqueID"                , 1, 1, 0 , &V_String },
    { "EditRate"                , 1, 1, 0 , &V_String },
    { "ColorEncoding"           , 1, 1, 0 , &V_Complex, E_ColorEncoding },
    { "Level6"                  , 0, 1, 0 , &V_Complex, E_Level6, A_Level6 },
    { "PluginNode"              , 1, 1, 0 , &V_Complex, E_Track_PluginNode },
    { "Shot"                    , 0, 0, 0 , &V_Complex, E_Shot },
    {}
};
static const xsd_element E_Video[] =
{
    { "Track"                   , 1, 0, 0 , &V_Complex, E_Track },
    {}
};
static const xsd_element E_Output[] =
{
    { "CompositionName"         , 1, 1, 0 , &V_String },
    { "UniqueID"                , 1, 1, 0 , &V_String },
    { "NumberVideoTracks"       , 1, 1, 0 , &V_NonNegativeInteger },
    { "CanvasAspectRatio"       , 1, 1, 0 , &V_Decimal },
    { "ImageAspectRatio"        , 1, 1, 0 , &V_Decimal },
    { "Video"                   , 1, 1, 0 , &V_Complex, E_Video },
    {}
};
static const xsd_element E_Outputs[] =
{
    { "Output"                  , 1, 0, 0 , &V_Complex, E_Output },
    {}
};
static const xsd_element E_Revision[] =
{
    { "DateTime"                , 1, 1, 0 , &V_DateTime },
    { "Author"                  , 1, 1, 0 , &V_String },
    { "Software"                , 1, 1, 0 , &V_String },
    { "SoftwareVersion"         , 1, 1, 0 , &V_String },
    { "Comment"                 , 0, 1, 0 , &V_String },
    {}
};
static const xsd_element E_RevisionHistory[] =
{
    { "Revision"                , 0, 0, 0 , &V_Complex, E_Revision },
    {}
};
static const xsd_element E_MDF[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "RevisionHistory"         , 0, 1, 0 , &V_Complex, E_RevisionHistory },
    { "Outputs"                 , 1, 1, 0 , &V_Complex, E_Outputs },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E_GSP[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "RevisionHistory"         , 0, 1, 0 , &V_Complex, E_RevisionHistory },
    { "CompositionName"         , 1, 1, 0 , &V_String },
    { "CanvasAspectRatio"       , 1, 1, 0 , &V_Decimal },
    { "ImageAspectRatio"        , 1, 1, 0 , &V_Decimal },
    { "Track"                   , 1, 1, 0 , &V_Complex, E_Track },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E_FW[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Shot"                    , 1, 1, 0 , &V_Complex, E_Shot },
    { "FrameNumber"             , 1, 1, 0 , &V_Integer },
    {}
};
static const xsd_element E_Integrated_GlobalData[] =
{
    { "RevisionHistory"         , 0, 1, 0 , &V_Complex, E_RevisionHistory },
    { "CompositionName"         , 1, 1, 0 , &V_String },
    { "CanvasAspectRatio"       , 1, 1, 0 , &V_Decimal },
    { "ImageAspectRatio"        , 1, 1, 0 , &V_Decimal },
    { "Track"                   , 1, 1, 0 , &V_Complex, E_Track },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E_Integrated_FrameData[] =
{
    { "Shot"                    , 1, 1, 0 , &V_Complex, E_Shot },
    { "FrameNumber"             , 1, 1, 0 , &V_Integer },
    {}
};
static const xsd_element E_Integrated[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "DolbyVisionGlobalData"   , 1, 1, 0 , &V_Complex, E_Integrated_GlobalData },
    { "DolbyVisionFrameData"    , 1, 1, 0 , &V_Complex, E_Integrated_FrameData },
    {}
};

//---------------------------------------------------------------------------
// Version 6.0.0 elements
static const xsd_element E6_ColorEncoding[] =
{
    { "ColorSpace"              , 1, 1, 0 , &V_Empty, nullptr, A6_ColorSpace },
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Transfer"                , 1, 1, 0 , &V_Transfer, nullptr, A6_Transfer },
    { "ColorRange"              , 1, 1, 0 , &V_Empty, nullptr, A6_ColorRange },
    {}
};
static const xsd_element E6_ShotList[] =
{
    { "Shot"                    , 1, 0, 0 , &V_UnsignedInt_2 },
    {}
};
static const xsd_element E6_SceneList[] =
{
    { "Scene"                   , 1, 0, 0 , &V_UnsignedInt_2 },
    {}
};
static const xsd_element E6_UniversalData[] =
{
    { "ColorEncoding"           , 0, 1, 0 , &V_Complex, E6_ColorEncoding },
    { "FrameRate"               , 0, 1, 0 , &V_UnsignedInt_2 },
    { "DefaultEncodingTrack"    , 0, 1, 0 , &V_Uuid },
    { "DefaultContentMappingTrack", 0, 1, 0 , &V_Uuid },
    { "EncoderCompatible"       , 0, 1, 0 , &V_String },
    { "CMOfflineCompatible"     , 0, 1, 0 , &V_String },
    { "ShotList"                , 0, 1, 0 , &V_Complex, E6_ShotList },
    { "SceneList"               , 0, 1, 0 , &V_Complex, E6_SceneList },
    {}
};
static const xsd_element E6_Revision[] =
{
    { "DateTime"                , 1, 1, 0 , &V_DateTime },
    { "Author"                  , 1, 1, 0 , &V_String },
    { "Software"                , 1, 1, 0 , &V_String },
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Comment"                 , 0, 1, 0 , &V_String },
    {}
};
static const xsd_element E6_History[] =
{
    { "Revision"                , 1, 0, 0 , &V_Complex, E6_Revision },
    {}
};
static const xsd_element E6_CreationProfile[] =
{
    { "Name"                    , 1, 1, 0 , &V_String },
    { "VariantName"             , 1, 1, 0 , &V_String },
    { "BackwardCompatible"      , 0, 1, 0 , &V_Empty },
    { "RestrictLift"            , 0, 1, 0 , &V_Empty },
    { "ConstrainLiftGain"       , 0, 1, 0 , &V_Empty },
    { "ConstrainMidtoneOffset"  , 0, 1, 0 , &V_Empty },
    { "PermitToneDetail"        , 0, 1, 0 , &V_Empty },
    { "PermitCM4Secondaries"    , 0, 1, 0 , &V_Empty },
    { "FilterCM4Secondaries"    , 0, 1, 0 , &V_Empty },
    { "PermitCM29Preview"       , 0, 1, 0 , &V_Empty },
    { "HDRGamutMatch"           , 0, 1, 0 , &V_Empty },
    {}
};
static const xsd_element E6_SDRDefinition[] =
{
    // Choice between the definition and SDRDerivationNotAllowed, checked separately
    { "ID"                      , 0, 1, 0 , &V_UnsignedInt },
    { "Gamut"                   , 0, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Luminance"               , 0, 1, 0 , &V_FinitePositiveFloat_2 },
    { "Transfer"                , 0, 1, 0 , &V_Transfer, nullptr, A6_Transfer },
    { "SDRDerivationNotAllowed" , 0, 1, 0 , &V_Empty },
    {}
};
static const xsd_element E6_AnchorTarget[] =
{
    { "ID"                      , 1, 1, 0 , &V_UnsignedInt },
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Luminance"               , 1, 1, 0 , &V_FinitePositiveFloat_2 },
    { "AllowSDROutput"          , 0, 1, 0 , &V_Empty },
    { "AnchorTrimsOnSDRDisplay" , 0, 1, 0 , &V_Empty },
    {}
};
static const xsd_element E6_CreativeEnvironment[] =
{
    { "SurroundLuminancePQ"     , 0, 1, 0 , &V_NormalFloat },
    { "StepOutOfBlackPQ"        , 0, 1, 0 , &V_NormalFloat },
    {}
};
static const xsd_element E6_MasteringDisplay[] =
{
    { "Name"                    , 1, 1, 0 , &V_String },
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Luminance"               , 1, 1, 0 , &V_FinitePositiveFloat_2 },
    {}
};
static const xsd_element E6_L0[] =
{
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Luminance"               , 1, 1, 0 , &V_FinitePositiveFloat_2 },
    {}
};
static const xsd_element E6_L2[] =
{
    { "Trim"                    , 1, 0, 0 , &V_MinusOneToOne_6, nullptr, A6_Trim },
    {}
};
static const xsd_element E6_L8[] =
{
    { "Trim"                    , 1, 0, 0 , &V_MinusOneToOne_8, nullptr, A6_Trim },
    { "SaturationGain"          , 0, 0, 0 , &V_MinusOneToOne_6, nullptr, A6_L8_Item },
    { "HueShift"                , 0, 0, 0 , &V_MinusOneToOne_6, nullptr, A6_L8_Item },
    {}
};
static const xsd_element E6_L9[] =
{
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    {}
};
static const xsd_element E6_Target[] =
{
    { "ID"                      , 1, 1, 0 , &V_UnsignedInt },
    { "Gamut"                   , 1, 1, 0 , &V_Gamut, nullptr, A6_Gamut },
    { "Luminance"               , 1, 1, 0 , &V_FinitePositiveFloat_2 },
    { "Transfer"                , 0, 1, 0 , &V_Transfer, nullptr, A6_Transfer },
    {}
};
static const xsd_element E6_L10[] =
{
    { "Target"                  , 1, 0, 0 , &V_Complex, E6_Target },
    {}
};
static const xsd_element E6_L15[] =
{
    { "Params"                  , 1, 1, 0 , &V_MinusOneToOne_10 },
    { "Pyramid"                 , 0, 1, 0 , &V_NormalFloat_1to7 },
    {}
};
static const xsd_element E6_L16[] =
{
    { "Params"                  , 1, 0, 0 , &V_MinusOneToOne_4, nullptr, A6_L16_Params },
    { "Pyramid"                 , 0, 1, 0 , &V_NormalFloat_1to7 },
    {}
};
static const xsd_element E6_Channel[] =
{
    { "History"                 , 0, 0, 0 , &V_Complex, E6_History, A6_f },
    { "MasteringDisplay"        , 0, 0, 0 , &V_Complex, E6_MasteringDisplay, A6_f },
    { "CreationProfile"         , 0, 0, 0 , &V_Complex, E6_CreationProfile, A6_f },
    { "SDRDefinition"           , 0, 0, 0 , &V_Complex, E6_SDRDefinition, A6_f },
    { "AnchorTarget"            , 0, 0, 0 , &V_Complex, E6_AnchorTarget, A6_f },
    { "CreativeEnvironment"     , 0, 0, 0 , &V_Complex, E6_CreativeEnvironment, A6_f },
    { "L0"                      , 0, 0, 0 , &V_Complex, E6_L0, A6_f },
    { "L1"                      , 0, 0, 0 , &V_NormalFloat_3, nullptr, A6_f },
    { "L2"                      , 0, 0, 0 , &V_Complex, E6_L2, A6_f },
    { "L3"                      , 0, 0, 0 , &V_MinusOneToOne_3, nullptr, A6_f },
    { "L4"                      , 0, 0, 0 , &V_NormalFloat_2, nullptr, A6_f },
    { "L5"                      , 0, 0, 0 , &V_UnsignedInt_6, nullptr, A6_f },
    { "L6"                      , 0, 0, 0 , &V_FinitePositiveFloat_2, nullptr, A6_f },
    { "L8"                      , 0, 0, 0 , &V_Complex, E6_L8, A6_f, XSD_Unordered },
    { "L9"                      , 0, 0, 0 , &V_Complex, E6_L9, A6_f },
    { "L10"                     , 0, 0, 0 , &V_Complex, E6_L10, A6_f },
    { "L11"                     , 0, 0, 0 , &V_Count15_2, nullptr, A6_L11 },
    { "L11x"                    , 0, 0, 0 , &V_InfiniteFloat_3, nullptr, A6_L11x },
    { "L15"                     , 0, 0, 0 , &V_Complex, E6_L15, A6_Revision },
    { "L16"                     , 0, 0, 0 , &V_Complex, E6_L16, A6_Revision },
    { "L18"                     , 0, 0, 0 , &V_NormalFloat_4, nullptr, A6_Revision },
    { "L254"                    , 0, 0, 0 , &V_UnsignedInt_4, nullptr, A6_f },
    {}
};
static const xsd_element E6_Track[] =
{
    { "UniversalData"           , 1, 1, 0 , &V_Complex, E6_UniversalData },
    { "Channel"                 , 1, 0, 0 , &V_Complex, E6_Channel, A6_Channel, XSD_Choice },
    {}
};
static const xsd_element E6_FwTrack[] =
{
    { "Channel"                 , 1, 0, 0 , &V_Complex, E6_Channel, A6_Channel, XSD_Choice },
    {}
};
static const xsd_element E6_MDF[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Track"                   , 1, 0, 0 , &V_Complex, E6_Track, A6_Track },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E6_Integrated[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Track"                   , 1, 0, 0 , &V_Complex, E6_Track, A6_Track },
    { "FrameNumber"             , 1, 1, 0 , &V_NonNegativeInteger },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E6_FW[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Track"                   , 1, 0, 0 , &V_Complex, E6_FwTrack, A6_Track },
    { "FrameNumber"             , 1, 1, 0 , &V_NonNegativeInteger },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};
static const xsd_element E6_GSP[] =
{
    { "Version"                 , 1, 1, 0 , &V_String },
    { "Track"                   , 1, 0, 0 , &V_Complex, E6_Track, A6_Track },
    { "ExtensionProperties"     , 0, 1, 0 , &V_Complex, nullptr, nullptr, XSD_Any },
    {}
};

//---------------------------------------------------------------------------
// Messages, one per kind of issue and per element path, with the count of occurrences
struct xsd_messages
{
    struct item
    {
        string      Message;
        size_t      Count;
    };
    std::vector<string> Keys;               // Order of appearance
    std::map<string, item> Items;

    void Add(const string& Key, const string& Message)
    {
        auto Item = Items.find(Key);
        if (Item != Items.end())
        {
            Item->second.Count++;
            return;
        }
        Keys.push_back(Key);
        Items[Key] = { Message, 1 };
    }
};

//---------------------------------------------------------------------------
static string Trim(const char* Text)
{
    if (!Text)
        return {};
    const char* End = Text + strlen(Text);
    while (*Text == ' ' || *Text == '\t' || *Text == '\r' || *Text == '\n')
        Text++;
    while (End > Text && (End[-1] == ' ' || End[-1] == '\t' || End[-1] == '\r' || End[-1] == '\n'))
        End--;
    return string(Text, End);
}

//---------------------------------------------------------------------------
static std::vector<string> Split(const string& Text)
{
    std::vector<string> Items;
    size_t Begin = 0;
    for (;;)
    {
        Begin = Text.find_first_not_of(" \t\r\n", Begin);
        if (Begin == string::npos)
            break;
        auto End = Text.find_first_of(" \t\r\n", Begin);
        Items.push_back(Text.substr(Begin, End == string::npos ? string::npos : End - Begin));
        if (End == string::npos)
            break;
        Begin = End;
    }
    return Items;
}

//---------------------------------------------------------------------------
static bool Number_Get(const string& Item, xsd_type Type, float64& Value)
{
    // Syntax check, then conversion independent from the locale
    if (Type == XSD_Float && (Item == "INF" || Item == "+INF" || Item == "-INF"))
    {
        Value = Item[0] == '-' ? -Inf : Inf;
        return true;
    }
    const char* Current = Item.c_str();
    if (*Current == '+' || (*Current == '-' && Type != XSD_NonNegativeInteger))
        Current++;
    size_t Digits = 0;
    while (*Current >= '0' && *Current <= '9')
        Current++, Digits++;
    if (*Current == '.' && (Type == XSD_Decimal || Type == XSD_Float))
    {
        Current++;
        while (*Current >= '0' && *Current <= '9')
            Current++, Digits++;
    }
    if (!Digits)
        return false;
    if ((*Current == 'e' || *Current == 'E') && Type == XSD_Float)
    {
        Current++;
        if (*Current == '+' || *Current == '-')
            Current++;
        size_t Exponent_Digits = 0;
        while (*Current >= '0' && *Current <= '9')
            Current++, Exponent_Digits++;
        if (!Exponent_Digits)
            return false;
    }
    if (*Current)
        return false;
    std::istringstream Stream(Item);
    Stream.imbue(std::locale::classic());
    Stream >> Value;
    return true;
}

//---------------------------------------------------------------------------
static bool DateTime_IsValid(const string& Item)
{
    // YYYY-MM-DDThh:mm:ss[.s+][Z|(+|-)hh:mm]
    const char* Text = Item.c_str();
    const auto Digits = [&](size_t Count) {
        for (size_t i = 0; i < Count; i++)
            if (*Text < '0' || *Text > '9')
                return false;
            else
                Text++;
        return true;
    };
    const auto Char = [&](char Value) {
        if (*Text != Value)
            return false;
        Text++;
        return true;
    };
    if (*Text == '-')
        Text++;
    if (!Digits(4) || !Char('-') || !Digits(2) || !Char('-') || !Digits(2) || !Char('T') || !Digits(2) || !Char(':') || !Digits(2) || !Char(':') || !Digits(2))
        return false;
    if (*Text == '.')
    {
        Text++;
        if (!Digits(1))
            return false;
        while (*Text >= '0' && *Text <= '9')
            Text++;
    }
    if (*Text == 'Z')
        Text++;
    else if (*Text == '+' || *Text == '-')
    {
        Text++;
        if (!Digits(2) || !Char(':') || !Digits(2))
            return false;
    }
    return !*Text;
}

//---------------------------------------------------------------------------
static bool Uuid_IsValid(const string& Item)
{
    if (Item.size() != 36)
        return false;
    for (size_t i = 0; i < 36; i++)
    {
        auto Value = Item[i];
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (Value != '-')
                return false;
        }
        else if (!((Value >= '0' && Value <= '9') || (Value >= 'a' && Value <= 'f') || (Value >= 'A' && Value <= 'F')))
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
static string Value_Expected(const xsd_value& Value)
{
    string Expected;
    if (Value.List_Max)
    {
        Expected = "list of ";
        Expected += std::to_string(Value.List_Min);
        if (Value.List_Max != Value.List_Min)
            Expected += " to " + std::to_string(Value.List_Max);
        Expected += ' ';
    }
    switch (Value.Type)
    {
    case XSD_NonNegativeInteger: Expected += "non negative integer"; break;
    case XSD_Integer: Expected += "integer"; break;
    case XSD_Boolean: Expected += "boolean"; break;
    case XSD_Decimal: Expected += "decimal"; break;
    case XSD_Float: Expected += "float"; break;
    case XSD_DateTime: Expected += "date and time"; break;
    case XSD_Uuid: Expected += "UUID"; break;
    case XSD_Enum:
        Expected += "one of ";
        for (auto Enum = Value.Enums; *Enum; Enum++)
        {
            if (Enum != Value.Enums)
                Expected += ", ";
            Expected += *Enum;
        }
        return Expected;
    default:;
    }
    if (Value.List_Max > 1)
        Expected += 's';
    const auto IsMin = Value.Min != -Inf && !(Value.Type == XSD_NonNegativeInteger && !Value.Min);
    const auto IsMax = Value.Max != Inf && Value.Max != std::numeric_limits<float64>::max() && Value.Max != 4294967295.0;
    const auto Number = [](float64 Value) {
        auto Text = Ztring::ToZtring(Value, 4).To_UTF8();
        if (Text.find('.') != string::npos)
        {
            Text.erase(Text.find_last_not_of('0') + 1);
            if (Text.back() == '.')
                Text.pop_back();
        }
        return Text;
    };
    if (IsMin && IsMax)
        Expected += " from " + Number(Value.Min) + " to " + Number(Value.Max);
    else if (IsMin)
        Expected += string(Value.Min_Exclusive ? " > " : " >= ") + Number(Value.Min);
    else if (IsMax)
        Expected += " <= " + Number(Value.Max);
    return Expected;
}

//---------------------------------------------------------------------------
static void Value_Check(const char* Text, const xsd_value& Value, const XMLElement* Element, const string& Path, xsd_messages& Messages)
{
    auto Content = Trim(Text);
    auto Value_Type = Value.Type;
    const xsd_value* Value_Ptr = &Value;
    xsd_value Value_Specific;
    switch (Value_Type)
    {
    case XSD_Complex:
    case XSD_String:
        return;
    case XSD_Empty:
        if (!Content.empty())
            Messages.Add(Path + " content", Path + " content \"" + Content + "\" is not permitted, no content is expected");
        return;
    case XSD_Gamut:
    case XSD_Transfer:
    {
        // Content depends on the name attribute
        const char* Name = Element->Attribute("name");
        if (!Name)
            return;
        int8u Count = 0;
        if (Value_Type == XSD_Gamut)
            Count = !strcmp(Name, "custom") ? 8 : 0;
        else if (!strcmp(Name, "linear"))
            Count = 1;
        else if (!strcmp(Name, "gamma") || !strcmp(Name, "dcigamma"))
            Count = 2;
        else if (!strcmp(Name, "bt1886") || !strcmp(Name, "hlg"))
            Count = 3;
        if (!Count)
        {
            if (!Content.empty())
                Messages.Add(Path + " content", Path + " content \"" + Content + "\" is not permitted with name \"" + Name + "\", no content is expected");
            return;
        }
        Value_Specific = Value_Type == XSD_Gamut ? XSD_Value(XSD_Decimal, -10, 10, 8, 8) : XSD_Value(XSD_Float, 0, std::numeric_limits<float64>::max(), Count, Count);
        Value_Ptr = &Value_Specific;
        break;
    }
    default:;
    }

    const auto& Value_Final = *Value_Ptr;
    std::vector<string> Items;
    if (Value_Final.List_Max)
    {
        Items = Split(Content);
        if (Items.size() < Value_Final.List_Min || Items.size() > Value_Final.List_Max)
        {
            Messages.Add(Path + " count", Path + " value \"" + Content + "\" is not permitted, " + Value_Expected(Value_Final) + " is expected");
            return;
        }
    }
    else
        Items.push_back(Content);
    for (const auto& Item : Items)
    {
        bool IsValid;
        switch (Value_Final.Type)
        {
        case XSD_Boolean:
            IsValid = Item == "true" || Item == "false" || Item == "1" || Item == "0";
            break;
        case XSD_DateTime:
            IsValid = DateTime_IsValid(Item);
            break;
        case XSD_Uuid:
            IsValid = Uuid_IsValid(Item);
            break;
        case XSD_Enum:
        {
            IsValid = false;
            for (auto Enum = Value_Final.Enums; *Enum; Enum++)
                if (Item == *Enum)
                    IsValid = true;
            break;
        }
        default:
        {
            float64 Number;
            IsValid = Number_Get(Item, Value_Final.Type, Number)
                && (Value_Final.Min_Exclusive ? Number > Value_Final.Min : Number >= Value_Final.Min)
                && Number <= Value_Final.Max;
        }
        }
        if (!IsValid)
        {
            Messages.Add(Path + " value", Path + " value \"" + Content + "\" is not permitted, " + Value_Expected(Value_Final) + " is expected");
            return;
        }
    }
}

//---------------------------------------------------------------------------
static void Schema_Check(const XMLElement* Element, const xsd_element& Info, int8u Version, const string& Path, xsd_messages& Messages)
{
    // Attributes
    for (auto Attribute = Element->FirstAttribute(); Attribute; Attribute = Attribute->Next())
    {
        const char* Name = Attribute->Name();
        if (!strncmp(Name, "xmlns", 5) && (!Name[5] || Name[5] == ':'))
            continue; // Namespace declarations
        if (!strncmp(Name, "xsi:", 4))
            continue;
        const xsd_attribute* Attribute_Info = nullptr;
        for (auto Item = Info.Attributes; Item && Item->Name; Item++)
            if (!strcmp(Item->Name, Name))
                Attribute_Info = Item;
        auto Attribute_Path = Path + '@' + Name;
        if (!Attribute_Info)
        {
            Messages.Add(Attribute_Path + " unknown", Attribute_Path + " attribute is not part of the specs");
            continue;
        }
        if (Attribute_Info->Fixed && strcmp(Attribute->Value(), Attribute_Info->Fixed))
            Messages.Add(Attribute_Path + " fixed", Attribute_Path + " value \"" + Attribute->Value() + "\" is not permitted, \"" + Attribute_Info->Fixed + "\" is expected");
        else
            Value_Check(Attribute->Value(), *Attribute_Info->Value, Element, Attribute_Path, Messages);
    }
    for (auto Item = Info.Attributes; Item && Item->Name; Item++)
        if (Item->Required && !Element->Attribute(Item->Name))
            Messages.Add(Path + '@' + Item->Name + " missing", Path + '@' + Item->Name + " attribute is missing");

    // Content
    if (Info.Value->Type != XSD_Complex)
    {
        Value_Check(Element->GetText(), *Info.Value, Element, Path, Messages);
        if (Element->FirstChildElement())
            Messages.Add(Path + " children", Path + " child elements are not permitted");
        return;
    }
    if (Info.Flags & XSD_Any)
        return;

    // Child elements
    size_t Children_Count = 0;
    while (Info.Children && Info.Children[Children_Count].Name)
        Children_Count++;
    std::vector<size_t> Counts(Children_Count);
    size_t Order_Last = 0;
    const char* Choice_Name = nullptr;
    for (auto Child_Element = Element->FirstChildElement(); Child_Element; Child_Element = Child_Element->NextSiblingElement())
    {
        const char* Name = Local(Child_Element->Name());
        auto Child_Path = Path + '/' + Name;
        size_t i = 0;
        while (i < Children_Count && (strcmp(Info.Children[i].Name, Name) || (Info.Children[i].Versions && !(Info.Children[i].Versions & Version))))
            i++;
        if (i >= Children_Count)
        {
            Messages.Add(Child_Path + " unknown", Child_Path + " element is not part of the specs");
            continue;
        }
        Counts[i]++;
        if (Info.Flags & XSD_Choice)
        {
            if (Choice_Name && strcmp(Choice_Name, Name))
                Messages.Add(Path + " choice", Path + " element contains " + Choice_Name + " and " + Name + " elements, only one kind is permitted");
            Choice_Name = Name;
        }
        else if (!(Info.Flags & XSD_Unordered))
        {
            if (i < Order_Last)
                Messages.Add(Child_Path + " order", Child_Path + " element is not at the expected position");
            else
                Order_Last = i;
        }
        Schema_Check(Child_Element, Info.Children[i], Version, Child_Path, Messages);
    }
    for (size_t i = 0; i < Children_Count; i++)
    {
        const auto& Child_Info = Info.Children[i];
        if (Child_Info.Versions && !(Child_Info.Versions & Version))
            continue;
        auto Child_Path = Path + '/' + Child_Info.Name;
        if (Counts[i] < Child_Info.Min)
            Messages.Add(Child_Path + " missing", Child_Path + " element is missing");
        else if (Child_Info.Max && Counts[i] > Child_Info.Max)
            Messages.Add(Child_Path + " count", Child_Path + " element count " + std::to_string(Counts[i]) + " is not permitted, max is " + std::to_string(Child_Info.Max));
    }
    if ((Info.Flags & XSD_Choice) && !Choice_Name)
        Messages.Add(Path + " empty", Path + " element is empty, one child element is expected");
}

//---------------------------------------------------------------------------
// Version 6.x, constraints from the XSD assertions
static void Schema_Check_V6(const XMLElement* Root, const string& Root_Name, xsd_messages& Messages)
{
    const auto IsGSP = Root_Name == "DolbyVisionGlobalDataGSP";
    const auto IsFW = Root_Name == "DolbyVisionFrameData";
    const auto IsMDF = Root_Name == "DolbyLabsMDF";
    for (auto Track = Child(Root, "Track"); Track; Track = Next(Track, "Track"))
    {
        auto Track_Path = Root_Name + "/Track";
        const char* Type = Track->Attribute("type");
        const auto IsGlobal = Type && !strcmp(Type, "GlobalDolbyVision");
        const auto IsDynamic = Type && !strcmp(Type, "DynamicDolbyVision");
        if (IsFW && !IsDynamic)
            Messages.Add(Track_Path + "@type fw", Track_Path + "@type value \"" + (Type ? Type : "") + "\" is not permitted, \"DynamicDolbyVision\" is expected");
        if (auto UniversalData = Child(Track, "UniversalData"))
        {
            static const char* const Global_Only[] = { "ColorEncoding", "FrameRate", "DefaultEncodingTrack", "DefaultContentMappingTrack", nullptr };
            static const char* const Dynamic_Only[] = { "EncoderCompatible", "CMOfflineCompatible", "ShotList", "SceneList", nullptr };
            for (auto Item = UniversalData->FirstChildElement(); Item; Item = Item->NextSiblingElement())
            {
                const char* Name = Local(Item->Name());
                for (auto List = IsGlobal ? Dynamic_Only : (IsDynamic ? Global_Only : nullptr); List && *List; List++)
                    if (!strcmp(Name, *List))
                        Messages.Add(Track_Path + "/UniversalData/" + Name + " type", Track_Path + "/UniversalData/" + Name + " element is not permitted in a " + Type + " track");
            }
        }
        for (auto Channel = Child(Track, "Channel"); Channel; Channel = Next(Channel, "Channel"))
        {
            for (auto Item = Channel->FirstChildElement(); Item; Item = Item->NextSiblingElement())
            {
                const string Name = Local(Item->Name());
                const auto Item_Path = Track_Path + "/Channel/" + Name;
                const auto IsGlobalChannel = Name == "History" || Name == "MasteringDisplay";
                if ((IsGlobal && !IsGlobalChannel) || (IsDynamic && IsGlobalChannel))
                    Messages.Add(Item_Path + " type", Item_Path + " element is not permitted in a " + Type + " track");
                static const char* const GSP_NotPermitted[] = { "L1", "L2", "L3", "L4", "L5", "L8", "L9", "L11x", "L15", "L16", "L18", nullptr };
                static const char* const FW_NotPermitted[] = { "History", "MasteringDisplay", "CreationProfile", "SDRDefinition", "AnchorTarget", "L0", "L6", "L10", "L11", "L254", nullptr };
                for (auto List = IsGSP ? GSP_NotPermitted : (IsFW ? FW_NotPermitted : nullptr); List && *List; List++)
                    if (Name == *List)
                        Messages.Add(Item_Path + " root", Item_Path + " element is not permitted in " + Root_Name);
                if ((IsMDF || IsGSP) && !Item->Attribute("f"))
                    Messages.Add(Item_Path + "@f missing", Item_Path + "@f attribute is missing");
                if (Name == "SDRDefinition")
                {
                    // Either the SDR definition or SDRDerivationNotAllowed
                    size_t Count = 0;
                    for (auto Definition_Item : { "ID", "Gamut", "Luminance", "Transfer" })
                        if (Child(Item, Definition_Item))
                            Count++;
                    if ((Count && Count != 4) || (Count && Child(Item, "SDRDerivationNotAllowed")))
                        Messages.Add(Item_Path + " choice", Item_Path + " element is not permitted, ID, Gamut, Luminance and Transfer elements or only SDRDerivationNotAllowed element are expected");
                }
            }
        }
    }
}

} //NameSpace

//---------------------------------------------------------------------------
bool File_DolbyVisionMetadata::Conformance_Schema(XMLElement* Root, const string& Version)
{
    // Schema depending on the root element and the version
    const string Root_Name = Local(Root->Name());
    const xsd_element* Elements = nullptr;
    int8u Version_Flag = 0;
    if (Version == "4.0.2" || Version == "5.1.0")
    {
        Version_Flag = Version[0] == '4' ? V4 : V5;
        if (Root_Name == "DolbyLabsMDF")
            Elements = E_MDF;
        else if (Root_Name == "DolbyVisionGlobalDataGSP")
            Elements = E_GSP;
        else if (Root_Name == "DolbyVisionFrameData")
            Elements = E_FW;
        else if (Root_Name == "DolbyVisionIntegratedData")
            Elements = E_Integrated;
    }
    else if (Version == "6.0.0")
    {
        if (Root_Name == "DolbyLabsMDF")
            Elements = E6_MDF;
        else if (Root_Name == "DolbyVisionGlobalDataGSP")
            Elements = E6_GSP;
        else if (Root_Name == "DolbyVisionFrameData")
            Elements = E6_FW;
        else if (Root_Name == "DolbyVisionIntegratedData")
            Elements = E6_Integrated;
    }
    if (!Elements)
        return false;

    xsd_messages Messages;
    const xsd_element Root_Info = { Root_Name.c_str(), 1, 1, 0, &V_Complex, Elements };
    Schema_Check(Root, Root_Info, Version_Flag, Root_Name, Messages);
    if (Version[0] == '6')
        Schema_Check_V6(Root, Root_Name, Messages);
    for (const auto& Key : Messages.Keys)
    {
        const auto& Item = Messages.Items[Key];
        Fill_Conformance("DolbyVision Schema", Item.Message + (Item.Count > 1 ? " (" + std::to_string(Item.Count) + " times)" : string()), {}, Conformance_Error, Stream_Video, 0);
    }
    return true;
}

//***************************************************************************
// Conformance
//***************************************************************************

//---------------------------------------------------------------------------
void File_DolbyVisionMetadata::Conformance_MDF(XMLElement* Root, XMLElement* Track, const string& Version, bool HasSchema)
{
    // Version, attribute in version 2.x, element in version 4.x or more
    if (Version.empty())
    {
        Fill_Conformance("DolbyVision Version", "Version is missing", {}, Conformance_Error, Stream_Video, 0);
        static const char Namespace_Prefix[] = "http://www.dolby.com/schemas/dvmd/";
        if (const char* Namespace = Root->Attribute("xmlns"))
            if (!strncmp(Namespace, Namespace_Prefix, sizeof(Namespace_Prefix) - 1))
            {
                string Namespace_Version(Namespace + sizeof(Namespace_Prefix) - 1);
                if (!Version_IsSupported(Namespace_Version))
                    Fill_Conformance("DolbyVision Namespace", "Namespace " + string(Namespace) + " is not supported", {}, Conformance_Warning, Stream_Video, 0);
            }
        return;
    }
    if (!Version_IsSupported(Version))
    {
        Fill_Conformance("DolbyVision Version", "Version " + Version + " is not supported", {}, Conformance_Warning, Stream_Video, 0);
        return;
    }
    const int Version_Major = atoi(Version.c_str());
    const bool IsV2 = Version_Major < 4;
    const char* Version_Attribute = Root->Attribute("version");
    const char* Version_Element = nullptr;
    if (XMLElement* Version_Item = Child(Root, "Version"))
        Version_Element = Version_Item->GetText();
    if (!HasSchema && Version_Attribute && Version_Element && strcmp(Version_Attribute, Version_Element))
        Fill_Conformance("DolbyVision Version", "version attribute (" + string(Version_Attribute) + ") and Version element (" + Version_Element + ") differ", {}, Conformance_Error, Stream_Video, 0);
    if (!HasSchema && (IsV2 ? Version_Element != nullptr : Version_Attribute != nullptr))
        Fill_Conformance("DolbyVision Version", IsV2 ? "Version element is not expected in version 2.x, version attribute is expected" : "version attribute is not expected in version 4.x or more, Version element is expected", {}, Conformance_Warning, Stream_Video, 0);

    // Namespace
    if (const char* Namespace = Root->Attribute("xmlns"))
    {
        static const char Namespace_Prefix[] = "http://www.dolby.com/schemas/dvmd/";
        if (!strncmp(Namespace, Namespace_Prefix, sizeof(Namespace_Prefix) - 1))
        {
            string Namespace_Expected(Version);
            for (auto& Character : Namespace_Expected)
                if (Character == '.')
                    Character = '_';
            if (Namespace_Expected != Namespace + sizeof(Namespace_Prefix) - 1)
                Fill_Conformance("DolbyVision Namespace", "Namespace " + string(Namespace) + " does not match version " + Version, {}, Conformance_Error, Stream_Video, 0);
        }
    }

    if (!Track)
        return;

    // Structure, element names and value separator depend on version (checked with the schema if available)
    const char* GlobalData_Unexpected = IsV2 ? "DVGlobalData" : "DolbyEDR";
    const char* DynamicData_Unexpected = IsV2 ? "DVDynamicData" : "DolbyEDR";
    auto Separator_Check = [&](XMLElement* Element)
    {
        const char* Text = Element ? Element->GetText() : nullptr;
        if (!Text || HasSchema)
            return;
        bool HasComma = strchr(Text, ',') != nullptr;
        if (IsV2 ? (!HasComma && strchr(Text, ' ')) : HasComma)
            Fill_Conformance("DolbyVision Structure", string(Local(Element->Name())) + " values are separated by " + (IsV2 ? "space" : "comma") + ", " + (IsV2 ? "comma" : "space") + " is expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
    };
    if (XMLElement* ColorEncoding = Child(Track, "ColorEncoding"))
    {
        Separator_Check(Child(ColorEncoding, "WhitePoint"));
        if (XMLElement* Primaries = Child(ColorEncoding, "Primaries"))
            Separator_Check(Child(Primaries, "Red"));
    }

    // Global data
    std::set<string> TargetDisplay_IDs;
    const char* MasteringDisplay_Peak = nullptr;
    XMLElement* Level254 = nullptr;
    auto Display_Parse = [&](XMLElement* Parent)
    {
        for (XMLElement* Item = Parent->FirstChildElement(); Item; Item = Item->NextSiblingElement())
        {
            if (Is(Item, "TargetDisplay"))
            {
                const char* ID = Item->Attribute("id");
                if (XMLElement* ID_Item = Child(Item, "ID"))
                    if (const char* Text = ID_Item->GetText())
                        ID = Text;
                if (ID)
                    TargetDisplay_IDs.insert(ID);
            }
            if (Is(Item, "MasteringDisplay"))
            {
                if (XMLElement* PeakBrightness = Child(Item, "PeakBrightness"))
                    MasteringDisplay_Peak = PeakBrightness->GetText();
            }
        }
    };
    if (XMLElement* PluginNode = Child(Track, "PluginNode"))
    {
        for (XMLElement* PluginNode_Item = PluginNode->FirstChildElement(); PluginNode_Item; PluginNode_Item = PluginNode_Item->NextSiblingElement())
        {
            if (!HasSchema && Is(PluginNode_Item, GlobalData_Unexpected))
                Fill_Conformance("DolbyVision Structure", string(GlobalData_Unexpected) + " element is not expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
            if (Is(PluginNode_Item, "DVGlobalData") || Is(PluginNode_Item, "DolbyEDR"))
            {
                // Same places as for the display of the displays, also if the element name does not match the version
                Display_Parse(PluginNode_Item);
                if (XMLElement* Characteristics = Child(PluginNode_Item, "Characteristics"))
                    Display_Parse(Characteristics);
            }
            if (Is(PluginNode_Item, "Level254"))
                Level254 = PluginNode_Item;
        }
    }

    // Level 254
    if (IsV2 && Level254)
        Fill_Conformance("DolbyVision Level254", "Level254 is not expected in version " + Version, {}, Conformance_Warning, Stream_Video, 0);
    if (!IsV2 && !Level254 && !HasSchema)
        Fill_Conformance("DolbyVision Level254", "Level254 is missing", {}, Conformance_Warning, Stream_Video, 0);
    if (!IsV2 && Level254)
    {
        if (XMLElement* CMVersion = Child(Level254, "CMVersion"))
        {
            if (const char* Text = CMVersion->GetText())
                if (atoi(Text) != 4)
                    Fill_Conformance("DolbyVision Level254", "CMVersion " + string(Text) + " does not match version " + Version + " (actual " + std::to_string(atoi(Text)) + ", expected 4)", {}, Conformance_Warning, Stream_Video, 0);
        }
        else if (!HasSchema)
            Fill_Conformance("DolbyVision Level254", "CMVersion is missing", {}, Conformance_Warning, Stream_Video, 0);
    }

    // Level 6
    if (MasteringDisplay_Peak)
        if (XMLElement* Level6 = Child(Track, "Level6"))
            if (XMLElement* MaxCLL = Child(Level6, "MaxCLL"))
                if (const char* Text = MaxCLL->GetText())
                    if (Ztring().From_UTF8(Text).To_float64() > Ztring().From_UTF8(MasteringDisplay_Peak).To_float64())
                        Fill_Conformance("DolbyVision Level6", "MaxCLL is more than mastering display peak brightness (actual " + string(Text) + ", expected <= " + MasteringDisplay_Peak + ")", {}, Conformance_Warning, Stream_Video, 0);

    // Shots
    std::set<string> TIDs;
    std::vector<std::pair<int64u, int64u>> Shots;
    bool DynamicData_Unexpected_Found = HasSchema;
    for (XMLElement* Shot = Child(Track, "Shot"); Shot; Shot = Next(Shot, "Shot"))
    {
        TID_Collect(Shot, TIDs);
        if (!DynamicData_Unexpected_Found)
            if (XMLElement* PluginNode = Child(Shot, "PluginNode"))
                if (Child(PluginNode, DynamicData_Unexpected))
                {
                    Fill_Conformance("DolbyVision Structure", string(DynamicData_Unexpected) + " element in Shot is not expected in version " + Version, {}, Conformance_Error, Stream_Video, 0);
                    DynamicData_Unexpected_Found = true;
                }

        XMLElement* Record = Child(Shot, "Record");
        XMLElement* In = Record ? Child(Record, "In") : nullptr;
        XMLElement* Duration = Record ? Child(Record, "Duration") : nullptr;
        const char* In_Text = In ? In->GetText() : nullptr;
        const char* Duration_Text = Duration ? Duration->GetText() : nullptr;
        if (!In_Text || !Duration_Text)
        {
            Fill_Conformance("DolbyVision Shot", "Record In or Duration is missing", {}, Conformance_Error, Stream_Video, 0);
            continue;
        }
        Shots.emplace_back(Ztring().From_UTF8(In_Text).To_int64u(), Ztring().From_UTF8(Duration_Text).To_int64u());
    }
    Conformance_Shots(Shots);

    // Trims must reference a declared target display
    for (const auto& TID : TIDs)
        if (TargetDisplay_IDs.find(TID) == TargetDisplay_IDs.end())
            Fill_Conformance("DolbyVision TID", "TID " + TID + " does not match any TargetDisplay ID", {}, Conformance_Error, Stream_Video, 0);
}


//---------------------------------------------------------------------------
void File_DolbyVisionMetadata::Conformance_Shots(const std::vector<std::pair<int64u, int64u>>& Shots)
{
    // Shots are expected to be contiguous, first frame and frame count of each shot
    int64u Previous_Out = (int64u)-1;
    int64u Gap_Count = 0, Gap_Frames = 0, Gap_First = 0;
    int64u Overlap_Count = 0, Overlap_Frames = 0, Overlap_First = 0;
    int64u Empty_Count = 0, Empty_First = 0;
    for (const auto& Shot : Shots)
    {
        auto In_Value = Shot.first;
        auto Duration_Value = Shot.second;
        if (!Duration_Value)
        {
            if (!Empty_Count)
                Empty_First = In_Value;
            Empty_Count++;
        }
        if (Previous_Out != (int64u)-1)
        {
            if (In_Value > Previous_Out)
            {
                if (!Gap_Count)
                    Gap_First = Previous_Out;
                Gap_Count++;
                Gap_Frames += In_Value - Previous_Out;
            }
            if (In_Value < Previous_Out)
            {
                if (!Overlap_Count)
                    Overlap_First = In_Value;
                Overlap_Count++;
                Overlap_Frames += Previous_Out - In_Value;
            }
        }
        Previous_Out = In_Value + Duration_Value;
    }
    if (Gap_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Gap_Count) + " gap(s) between shots, " + std::to_string(Gap_Frames) + " frame(s) not covered, first gap at frame " + std::to_string(Gap_First), {}, Conformance_Error, Stream_Video, 0);
    if (Overlap_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Overlap_Count) + " overlap(s) between shots, " + std::to_string(Overlap_Frames) + " frame(s) covered more than once, first overlap at frame " + std::to_string(Overlap_First), {}, Conformance_Error, Stream_Video, 0);
    if (Empty_Count)
        Fill_Conformance("DolbyVision Shot", std::to_string(Empty_Count) + " shot(s) with a duration of 0, first at frame " + std::to_string(Empty_First), {}, Conformance_Error, Stream_Video, 0);
}

//---------------------------------------------------------------------------
void File_DolbyVisionMetadata::Conformance_V6(XMLElement* Root)
{
    // Same checks as with version 4.x, with the version 6.x structure
    float64 MasteringDisplay_Peak = 0;
    string MasteringDisplay_Peak_Text;
    std::set<string> Target_IDs;
    std::set<string> TIDs;
    std::vector<std::pair<int64u, int64u>> Shots;
    std::vector<string> MaxCLLs;
    for (XMLElement* Track = Child(Root, "Track"); Track; Track = Next(Track, "Track"))
    {
        const char* Type = Track->Attribute("type");
        if (Type && !strcmp(Type, "DynamicDolbyVision"))
        if (XMLElement* UniversalData = Child(Track, "UniversalData"))
            if (XMLElement* ShotList = Child(UniversalData, "ShotList"))
                for (XMLElement* Shot = Child(ShotList, "Shot"); Shot; Shot = Next(Shot, "Shot"))
                {
                    auto Values = Values_Get(Shot->GetText() ? Shot->GetText() : "");
                    if (Values.size() == 2)
                        Shots.emplace_back((int64u)Values[0], (int64u)Values[1]);
                }
        for (XMLElement* Channel = Child(Track, "Channel"); Channel; Channel = Next(Channel, "Channel"))
            for (XMLElement* Item = Channel->FirstChildElement(); Item; Item = Item->NextSiblingElement())
            {
                if (Is(Item, "MasteringDisplay"))
                    if (XMLElement* Luminance = Child(Item, "Luminance"))
                    {
                        auto Values = Values_Get(Luminance->GetText() ? Luminance->GetText() : "");
                        auto Texts = Split(Trim(Luminance->GetText()));
                        if (Values.size() == 2 && Texts.size() == 2 && MasteringDisplay_Peak < Values[1])
                        {
                            MasteringDisplay_Peak = Values[1];
                            MasteringDisplay_Peak_Text = Texts[1];
                        }
                    }
                if (Is(Item, "L6"))
                {
                    auto Texts = Split(Trim(Item->GetText())); // MaxFALL, MaxCLL
                    if (Texts.size() == 2)
                        MaxCLLs.push_back(Texts[1]);
                }
                if (Is(Item, "L10"))
                    for (XMLElement* Target = Child(Item, "Target"); Target; Target = Next(Target, "Target"))
                        if (XMLElement* ID = Child(Target, "ID"))
                            if (const char* Text = ID->GetText())
                                Target_IDs.insert(Trim(Text));
                if (Is(Item, "SDRDefinition") || Is(Item, "AnchorTarget"))
                    if (XMLElement* ID = Child(Item, "ID"))
                        if (const char* Text = ID->GetText())
                            Target_IDs.insert(Trim(Text));
                if (Is(Item, "L2") || Is(Item, "L8"))
                    for (XMLElement* Trim_Item = Child(Item, "Trim"); Trim_Item; Trim_Item = Next(Trim_Item, "Trim"))
                        if (const char* Text = Trim_Item->Attribute("t"))
                            TIDs.insert(Text);
            }
    }

    // Level 6
    if (MasteringDisplay_Peak)
        for (const auto& MaxCLL : MaxCLLs)
            if (Ztring().From_UTF8(MaxCLL).To_float64() > MasteringDisplay_Peak)
                Fill_Conformance("DolbyVision Level6", "MaxCLL is more than mastering display peak brightness (actual " + MaxCLL + ", expected <= " + MasteringDisplay_Peak_Text + ")", {}, Conformance_Warning, Stream_Video, 0);

    // Shots
    Conformance_Shots(Shots);

    // Trims must reference a declared target
    if (!Target_IDs.empty())
        for (const auto& TID : TIDs)
            if (Target_IDs.find(TID) == Target_IDs.end())
                Fill_Conformance("DolbyVision TID", "Trim t " + TID + " does not match any Target ID", {}, Conformance_Error, Stream_Video, 0);
}

} //NameSpace

#endif //defined(MEDIAINFO_MPEG4_YES) || defined(MEDIAINFO_MXF_YES)

