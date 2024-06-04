/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Inform part
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
#include "ZenLib/Utils.h"
#if defined(MEDIAINFO_EBUCORE_YES)
    #include "MediaInfo/Export/Export_EbuCore.h"
#endif //defined(MEDIAINFO_EBUCORE_YES)
#if defined(MEDIAINFO_FIMS_YES)
    #include "MediaInfo/Export/Export_Fims.h"
#endif //defined(MEDIAINFO_EBUCORE_YES)
#if defined(MEDIAINFO_MPEG7_YES)
    #include "MediaInfo/Export/Export_Mpeg7.h"
#endif //defined(MEDIAINFO_MPEG7_YES)
#if defined(MEDIAINFO_REVTMD_YES)
    #include "MediaInfo/Export/Export_reVTMD.h"
#endif //defined(MEDIAINFO_REVTMD_YES)
#if defined(MEDIAINFO_PBCORE_YES)
    #include "MediaInfo/Export/Export_PBCore.h"
    #include "MediaInfo/Export/Export_PBCore2.h"
#endif //defined(MEDIAINFO_PBCORE_YES)
#if defined(MEDIAINFO_NISO_YES)
#include "MediaInfo/Export/Export_Niso.h"
#endif //defined(MEDIAINFO_NISO_YES)
#if defined(MEDIAINFO_GRAPH_YES)
#include "MediaInfo/Export/Export_Graph.h"
#endif //defined(MEDIAINFO_GRAPH_YES)

#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/File__Analyze.h"
#include "ThirdParty/base64/base64.h"
# if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
#include "MediaInfo/OutputHelpers.h"
#endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES

//---------------------------------------------------------------------------
#include <ctime>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
Ztring Xml_Name_Escape_0_7_78 (const Ztring &Name)
{
    Ztring ToReturn(Name);
    ToReturn.FindAndReplace(__T(" "), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("-"), Ztring(), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("/"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("("), Ztring(), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(")"), Ztring(), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("*"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(","), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(":"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("@"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("."), __T("_"), 0, Ztring_Recursive);
    size_t ToReturn_Pos=0;
    while (ToReturn_Pos<ToReturn.size())
    {
        if (!(ToReturn[ToReturn_Pos]>=__T('A') && ToReturn[ToReturn_Pos]<=__T('Z'))
         && !(ToReturn[ToReturn_Pos]>=__T('a') && ToReturn[ToReturn_Pos]<=__T('z'))
         && !(ToReturn[ToReturn_Pos]>=__T('0') && ToReturn[ToReturn_Pos]<=__T('9'))
         && !(ToReturn[ToReturn_Pos]==__T('_')))
            ToReturn.erase(ToReturn_Pos, 1);
        else
            ToReturn_Pos++;
    }

    if ((ToReturn.operator()(0)>='0' && ToReturn.operator()(0)<='9') || ToReturn.operator()(0)=='-')
        ToReturn.insert(0, 1, __T('_'));

    if (ToReturn.empty())
        ToReturn="Unknown";

    return ToReturn;
}
#endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)

//---------------------------------------------------------------------------
std::string URL_Encoded_Encode(const std::string& URL);
extern MediaInfo_Config Config;
extern const Char* MediaInfo_Version;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void MediaInfo_Internal::ConvertRetour(Ztring& Retour)
{
    Retour.FindAndReplace(__T("\\r\\n"), __T("\n"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\r"), __T("\n"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\n"), __T("\n"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\r\n"), __T("\n"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\r"), __T("\n"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\n"), MediaInfoLib::Config.LineSeparator_Get(), 0, Ztring_Recursive);
}

//---------------------------------------------------------------------------
void MediaInfo_Internal::ConvertRetourSCX(Ztring& Retour)
{
    Retour.FindAndReplace(__T("|SC1|"), __T("\\"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC2|"), __T("["), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC3|"), __T("]"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC4|"), __T(","), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC5|"), __T(";"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC6|"), __T("("), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC7|"), __T(")"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC8|"), __T(")"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("|SC9|"), __T("),"), 0, Ztring_Recursive);
}

//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Inform()
{
    {
        CriticalSectionLocker CSL(CS);
    if (Info && Info->Status[File__Analyze::IsUpdated])
        Info->Open_Buffer_Update();
    }

    #if MEDIAINFO_TRACE
        if (MediaInfoLib::Config.Inform_Get()!=__T("MAXML") && (MediaInfoLib::Config.Trace_Level_Get() || MediaInfoLib::Config.Inform_Get()==__T("Details")))
        {
            if (!Details.empty())
                return Ztring().From_UTF8(Details);
            else if (Info)
                return Info->Details_Get();
            else
                return Ztring();
        }
    #endif //MEDIAINFO_TRACE

    #if MEDIAINFO_ADVANCED
        if (Config.TimeCode_Dumps)
        {
            string Result="<media";
            if (Config.ParseSpeed<1.0)
                Result+=" full=\"0\"";
            Ztring Options=Get(Stream_General, 0, General_CompleteName, Info_Options);
            if (InfoOption_ShowInInform<Options.size() && Options[InfoOption_ShowInInform]==__T('Y'))
            {
                Result+=" ref=\"";
                Result+=XML_Encode(Get(Stream_General, 0, General_CompleteName)).To_UTF8();
                Result+='\"';
            }
            Result+=" format=\"";
            Result+=XML_Encode(Get(Stream_General, 0, General_Format)).To_UTF8();
            Result+="\">\n";
            for (const auto& TimeCode_Dump : *Config.TimeCode_Dumps)
            {
                Result+="  <timecode_stream";
                Result+=TimeCode_Dump.second.Attributes_First;
                Result+=" frame_count=\""+std::to_string(TimeCode_Dump.second.FrameCount)+'\"';
                Result+=TimeCode_Dump.second.Attributes_Last;
                if (TimeCode_Dump.second.List.empty())
                    Result+="/>\n";
                else
                {
                    Result+=">\n";
                    Result+=TimeCode_Dump.second.List;
                    Result+="  </timecode_stream>\n";
                }
            }
            Result+="</media>";
            return Ztring().From_UTF8(Result);
        }
    #endif //MEDIAINFO_ADVANCED

    #if defined(MEDIAINFO_EBUCORE_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.5"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_5, Export_EbuCore::AcquisitionDataOutputMode_Default,
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.6"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_6, Export_EbuCore::AcquisitionDataOutputMode_Default,
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, (Export_EbuCore::acquisitiondataoutputmode)MediaInfoLib::Config.AcquisitionDataOutputMode_Get(),
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_parameterSegment") || MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_ps"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, Export_EbuCore::AcquisitionDataOutputMode_parameterSegment,
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_segmentParameter") || MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_sp"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, Export_EbuCore::AcquisitionDataOutputMode_segmentParameter,
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.5_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_5, Export_EbuCore::AcquisitionDataOutputMode_Default,
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.6_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_6, Export_EbuCore::AcquisitionDataOutputMode_Default,
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, (Export_EbuCore::acquisitiondataoutputmode)MediaInfoLib::Config.AcquisitionDataOutputMode_Get(),
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_parameterSegment_JSON") || MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_ps_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, Export_EbuCore::AcquisitionDataOutputMode_parameterSegment,
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_segmentParameter_JSON") || MediaInfoLib::Config.Inform_Get()==__T("EBUCore_1.8_sp_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::Version_1_8, Export_EbuCore::AcquisitionDataOutputMode_segmentParameter,
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::version(Export_EbuCore::Version_Max-1), (Export_EbuCore::acquisitiondataoutputmode)MediaInfoLib::Config.AcquisitionDataOutputMode_Get(),
            Export_EbuCore::Format_XML, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
        if (MediaInfoLib::Config.Inform_Get()==__T("EBUCore_JSON"))
            return Export_EbuCore().Transform(*this, Export_EbuCore::version(Export_EbuCore::Version_Max-1), (Export_EbuCore::acquisitiondataoutputmode)MediaInfoLib::Config.AcquisitionDataOutputMode_Get(),
            Export_EbuCore::Format_JSON, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
    #endif //defined(MEDIAINFO_EBUCORE_YES)
    #if defined(MEDIAINFO_EBUCORE_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("FIMS_1.1"))
            return Export_Fims().Transform(*this, Export_Fims::Version_1_1);
        if (MediaInfoLib::Config.Inform_Get()==__T("FIMS_1.2"))
            return Export_Fims().Transform(*this, Export_Fims::Version_1_2);
        if (MediaInfoLib::Config.Inform_Get()==__T("FIMS_1.3"))
            return Export_Fims().Transform(*this, Export_Fims::Version_1_3);
        if (MediaInfoLib::Config.Inform_Get()==__T("FIMS"))
            return Export_Fims().Transform(*this);
    #endif //defined(MEDIAINFO_FIMS_YES)
    #if defined(MEDIAINFO_MPEG7_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("MPEG-7"))
            return Export_Mpeg7().Transform(*this);
        if (MediaInfoLib::Config.Inform_Get()==__T("MPEG-7_Strict"))
            return Export_Mpeg7().Transform(*this, Export_Mpeg7::Version_Strict);
        if (MediaInfoLib::Config.Inform_Get()==__T("MPEG-7_Relaxed"))
            return Export_Mpeg7().Transform(*this, Export_Mpeg7::Version_BestEffort_Strict);
        if (MediaInfoLib::Config.Inform_Get()==__T("MPEG-7_Extended_If_Needed"))
            return Export_Mpeg7().Transform(*this, Export_Mpeg7::Version_BestEffort_Extended);
        if (MediaInfoLib::Config.Inform_Get()==__T("MPEG-7_Extended"))
            return Export_Mpeg7().Transform(*this, Export_Mpeg7::Version_Extended);
    #endif //defined(MEDIAINFO_MPEG7_YES)
    #if defined(MEDIAINFO_PBCORE_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore_1") || MediaInfoLib::Config.Inform_Get()==__T("PBCore1")) // 1.x
            return Export_PBCore().Transform(*this);
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore_1.2"))
            return Export_PBCore().Transform(*this);
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore_2") ||MediaInfoLib::Config.Inform_Get()==__T("PBCore2")) //2.x
            return Export_PBCore2().Transform(*this);
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore_2.0"))
            return Export_PBCore2().Transform(*this, Export_PBCore2::Version_2_0);
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore_2.1"))
            return Export_PBCore2().Transform(*this, Export_PBCore2::Version_2_1);
        if (MediaInfoLib::Config.Inform_Get()==__T("PBCore")) //x.x
            return Export_PBCore2().Transform(*this);
    #endif //defined(MEDIAINFO_PBCORE_YES)
    #if defined(MEDIAINFO_NISO_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("NISO_Z39.87"))
            return Export_Niso().Transform(*this, MediaInfoLib::Config.ExternalMetadata_Get(), MediaInfoLib::Config.ExternalMetaDataConfig_Get());
    #endif //defined(MEDIAINFO_NISO_YES)
    #if defined(MEDIAINFO_GRAPH_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Dot"))
            return Export_Graph().Transform(*this, Export_Graph::Graph_All, Export_Graph::Format_Dot);
        #if defined(MEDIAINFO_GRAPHVIZ_YES)
            if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Svg"))
                return Export_Graph().Transform(*this, Export_Graph::Graph_All, Export_Graph::Format_Svg);
        #endif //defined(MEDIAINFO_GRAPHVIZ_YES)
    #endif //defined(MEDIAINFO_GRAPH_YES)
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_AC4_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Ac4_Dot"))
            return Export_Graph().Transform(*this, Export_Graph::Graph_Ac4, Export_Graph::Format_Dot);
        #if defined(MEDIAINFO_GRAPHVIZ_YES)
            if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Ac4_Svg"))
                return Export_Graph().Transform(*this, Export_Graph::Graph_Ac4, Export_Graph::Format_Svg);
        #endif //defined(MEDIAINFO_GRAPHVIZ_YES)
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_AC4_YES)
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_DOLBYE_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Ed2_Dot"))
            return Export_Graph().Transform(*this, Export_Graph::Graph_Ed2, Export_Graph::Format_Dot);
        #if defined(MEDIAINFO_GRAPHVIZ_YES)
            if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Ed2_Svg"))
                return Export_Graph().Transform(*this, Export_Graph::Graph_Ed2, Export_Graph::Format_Svg);
        #endif //defined(MEDIAINFO_GRAPHVIZ_YES)
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_DOLBYE_YES)
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Adm_Dot"))
            return Export_Graph().Transform(*this, Export_Graph::Graph_Adm, Export_Graph::Format_Dot);
        #if defined(MEDIAINFO_GRAPHVIZ_YES)
            if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Adm_Svg"))
                return Export_Graph().Transform(*this, Export_Graph::Graph_Adm, Export_Graph::Format_Svg);
        #endif //defined(MEDIAINFO_GRAPHVIZ_YES)
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_ADM_YES)
    #if defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_MPEGH3DA_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Mpegh3da_Dot"))
            return Export_Graph().Transform(*this, Export_Graph::Graph_Mpegh3da, Export_Graph::Format_Dot);
        #if defined(MEDIAINFO_GRAPHVIZ_YES)
            if (MediaInfoLib::Config.Inform_Get()==__T("Graph_Mpegh3da_Svg"))
                return Export_Graph().Transform(*this, Export_Graph::Graph_Mpegh3da, Export_Graph::Format_Svg);
        #endif //defined(MEDIAINFO_GRAPHVIZ_YES)
    #endif //defined(MEDIAINFO_GRAPH_YES) && defined(MEDIAINFO_MPEGH3DA_YES)
    #if defined(MEDIAINFO_REVTMD_YES)
        if (MediaInfoLib::Config.Inform_Get()==__T("reVTMD"))
            return __T("reVTMD is disabled due to its non-free licensing."); //return Export_reVTMD().Transform(*this);
    #endif //defined(MEDIAINFO_REVTMD_YES)

    #if defined(MEDIAINFO_CUSTOM_YES)

    if (!(
        MediaInfoLib::Config.Inform_Get(__T("General")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Video")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Audio")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Text")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Other")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Image")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Menu")).empty()
     ))
    {
        Ztring Retour;
        Retour+=MediaInfoLib::Config.Inform_Get(__T("File_Begin"));
        Retour+=MediaInfoLib::Config.Inform_Get(__T("General_Begin"));
        Retour+=Inform(Stream_General, 0, false);
        Retour+=MediaInfoLib::Config.Inform_Get(__T("General_End"));
        if (Count_Get(Stream_Video))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Video_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Video); I1++)
        {
            Retour+=Inform(Stream_Video, I1, false);
            if (I1!=Count_Get(Stream_Video)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Video_Middle"));
        }
        if (Count_Get(Stream_Video))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Video_End"));
        if (Count_Get(Stream_Audio))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Audio_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Audio); I1++)
        {
            Retour+=Inform(Stream_Audio, I1, false);
            if (I1!=Count_Get(Stream_Audio)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Audio_Middle"));
        }
        if (Count_Get(Stream_Audio))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Audio_End"));
        if (Count_Get(Stream_Text))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Text_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Text); I1++)
        {
            Retour+=Inform(Stream_Text, I1, false);
            if (I1!=Count_Get(Stream_Text)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Text_Middle"));
        }
        if (Count_Get(Stream_Text))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Text_End"));
        if (Count_Get(Stream_Other))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Other_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Other); I1++)
        {
            Retour+=Inform(Stream_Other, I1, false);
            if (I1!=Count_Get(Stream_Other)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Other_Middle"));
        }
        if (Count_Get(Stream_Other))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Other_End"));
        if (Count_Get(Stream_Image))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Image_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Image); I1++)
        {
            Retour+=Inform(Stream_Image, I1, false);
            if (I1!=Count_Get(Stream_Image)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Image_Middle"));
        }
        if (Count_Get(Stream_Image))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Image_End"));
        if (Count_Get(Stream_Menu))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Menu_Begin"));
        for (size_t I1=0; I1<Count_Get(Stream_Menu); I1++)
        {
            Retour+=Inform(Stream_Menu, I1, false);
            if (I1!=Count_Get(Stream_Menu)-1)
                Retour+=MediaInfoLib::Config.Inform_Get(__T("Menu_Middle"));
        }
        if (Count_Get(Stream_Menu))
            Retour+=MediaInfoLib::Config.Inform_Get(__T("Menu_End"));
        Retour+=MediaInfoLib::Config.Inform_Get(__T("File_End"));

        ConvertRetour(Retour);

        //Special characters
        ConvertRetourSCX(Retour);

        return Retour;
    }
    #endif //defined(MEDIAINFO_CUSTOM_YES)

    #if defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES)

    //Informations
    #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
    Node* Node_Main=NULL;
    Node* Node_MI=NULL;
    #endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES
    Ztring Retour;
    bool HTML=false;
    bool XML=false;
    bool XML_0_7_78_MA=false;
    bool XML_0_7_78_MI=false;
    bool JSON=false;
    bool Conformance_JSON=false;
    bool CSV=false;
    #if defined(MEDIAINFO_HTML_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("HTML"))
        HTML=true;
    #endif //defined(MEDIAINFO_HTML_YES)
    #if defined(MEDIAINFO_XML_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("OLDXML"))
        XML=true;
    if (MediaInfoLib::Config.Inform_Get()==__T("MAXML"))
        XML_0_7_78_MA=true;
    if (MediaInfoLib::Config.Inform_Get()==__T("MIXML") || MediaInfoLib::Config.Inform_Get()==__T("XML"))
        XML_0_7_78_MI=true;
    #endif //defined(MEDIAINFO_XML_YES)
     #if defined(MEDIAINFO_JSON_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("JSON") || MediaInfoLib::Config.Inform_Get()==__T("JSON_URL") || MediaInfoLib::Config.Inform_Get()==__T("Conformance_JSON"))
        JSON=true;
    if (MediaInfoLib::Config.Inform_Get()==__T("JSON_URL") || MediaInfoLib::Config.Inform_Get()==__T("Conformance_JSON"))
        Conformance_JSON=true;
    #endif //defined(MEDIAINFO_JSON_YES)
    #if defined(MEDIAINFO_CSV_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("CSV"))
        CSV=true;
    #endif //defined(MEDIAINFO_CSV_YES)

    if (HTML)
        Retour+=__T("<html>\n\n<head>\n<META http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /></head>\n<body>\n");
    #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
    if (XML_0_7_78_MA || XML_0_7_78_MI || JSON)
    {
        Node_Main=new Node("media");
        Ztring Options=Get(Stream_General, 0, General_CompleteName, Info_Options);
        if (InfoOption_ShowInInform<Options.size() && Options[InfoOption_ShowInInform]==__T('Y') && !Conformance_JSON)
            Node_Main->Add_Attribute("ref", Get(Stream_General, 0, General_CompleteName));
        if (Info && !Info->ParserName.empty())
            Node_Main->Add_Attribute("parser", Info->ParserName);
    }
    if (XML_0_7_78_MA)
    {
        Node_MI=new Node("MediaInfo");
        Node_MI->Add_Attribute("xmlns", __T("http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediainfo"));
        Node_MI->Add_Attribute("version", "2.0");
        Node_Main->Childs.push_back(Node_MI);
    }
    if (XML)
    {
        if (Node_MI)
            Node_Main=Node_MI->Add_Child("File");
        else
            Node_Main=new Node("File");
    }
    #endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)

    for (size_t StreamKind=(size_t)Stream_General; StreamKind<Stream_Max; StreamKind++)
    {
        //Pour chaque type de flux
        for (size_t StreamPos=0; StreamPos<(size_t)Count_Get((stream_t)StreamKind); StreamPos++)
        {
            //Pour chaque stream
            if (HTML) Retour+=__T("<table width=\"100%\" border=\"0\" cellpadding=\"1\" cellspacing=\"2\" style=\"border:1px solid Navy\">\n<tr>\n    <td width=\"150\"><h2>");
            #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
            Node* Node_Current=NULL;
            if (XML || XML_0_7_78_MA || XML_0_7_78_MI || JSON) Node_Current=Node_MI?Node_MI->Add_Child("track", true):Node_Main->Add_Child("track", true);
            #endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
            Ztring B=Get((stream_t)StreamKind, StreamPos, __T("StreamKindPos"));
            if (!XML && !XML_0_7_78_MA && !XML_0_7_78_MI && !JSON)
            {
                Ztring A=Get((stream_t)StreamKind, StreamPos, __T("StreamKind/String"));

                if (!B.empty())
                {
                    if (CSV)
                        A+=__T(",");
                    else
                        A+=MediaInfoLib::Config.Language_Get(__T("  Config_Text_NumberTag"));
                    A+=B;
                }
                Retour+=A;
            }
            if (HTML) Retour+=__T("</h2></td>\n  </tr>");
            #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
            if (XML || XML_0_7_78_MA || XML_0_7_78_MI || JSON)
            {
                Ztring A=Get((stream_t)StreamKind, StreamPos, __T("StreamKind"));

                if (!Conformance_JSON)
                {
                    Node_Current->Add_Attribute("type", A);
                    if (!B.empty()) Node_Current->Add_Attribute("typeorder", B);
                }
                Node* Track=new Node();
                Track->RawContent=Inform((stream_t)StreamKind, StreamPos, false).To_UTF8();
                Node_Current->Childs.push_back(Track);
            }
            else
            #endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
            {
                Retour+=MediaInfoLib::Config.LineSeparator_Get();
                Retour+=Inform((stream_t)StreamKind, StreamPos, false);
            }

            if (HTML) Retour+=__T("</table>\n<br />");
            if (!XML && !XML_0_7_78_MA && !XML_0_7_78_MI && !JSON) Retour+=MediaInfoLib::Config.LineSeparator_Get();
        }
    }

    if (HTML) Retour+=__T("\n</body>\n</html>\n");

    if (!CSV && !HTML && !XML && !XML_0_7_78_MA && !XML_0_7_78_MI && !JSON)
    {
        if (MediaInfoLib::Config.Inform_Version_Get())
        {
            Ztring Name=MediaInfoLib::Config.Language_Get(__T("ReportBy"));
            int8u Name_Size=MediaInfoLib::Config.Language_Get(__T("  Config_Text_ColumnSize")).To_int8u();
            if (Name_Size==0)
                Name_Size=32; //Default
            Name.resize(Name_Size, ' ');

            Retour+=Name;
            Retour+=MediaInfoLib::Config.Language_Get(__T("  Config_Text_Separator"));
            Retour+=MediaInfo_Version;
            Retour+=MediaInfoLib::Config.LineSeparator_Get();
        }

        if (MediaInfoLib::Config.Inform_Timestamp_Get())
        {
            Ztring Name=MediaInfoLib::Config.Language_Get(__T("CreatedOn"));

            int8u Name_Size=MediaInfoLib::Config.Language_Get(__T("  Config_Text_ColumnSize")).To_int8u();
            if (Name_Size==0)
                Name_Size=32; //Default
            Name.resize(Name_Size, ' ');

            Ztring Date=Ztring().Date_From_Seconds_1970((int64s)time(NULL));

            Retour+=Name;
            Retour+=MediaInfoLib::Config.Language_Get(__T("  Config_Text_Separator"));
            Retour+=Date;
            Retour+=MediaInfoLib::Config.LineSeparator_Get();
        }
    }

    #if defined(MEDIAINFO_XML_YES)
        if (XML || XML_0_7_78_MA || XML_0_7_78_MI)
        {
            Retour=Ztring().From_UTF8(To_XML(*Node_Main, 0, false, false));
            delete Node_Main;
        }
    #endif //MEDIAINFO_XML_YES
    #if defined(MEDIAINFO_JSON_YES)
        if (JSON)
        {
            Retour+=__T('{');
            if (!Conformance_JSON)
                Retour+=__T('\n');
            Node Node_Version("creatingLibrary");
            Node_Version.Add_Child("name", Ztring("MediaInfoLib"));
            Node_Version.Add_Child("version", Ztring(MediaInfo_Version).SubString(__T(" - v"), Ztring()));
            if (!Conformance_JSON)
                Node_Version.Add_Child("url", Ztring(__T("http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/MediaInfo")));
            Retour+=Ztring().From_UTF8(To_JSON(Node_Version, 0, false, false, false))+__T(',');
            if (!Conformance_JSON)
                Retour+=__T('\n');

            Retour+=Ztring().From_UTF8(To_JSON(*Node_Main, 0, false, false, false));
            if (!Conformance_JSON)
                Retour+=__T('\n');
            Retour+=__T('}');
            delete Node_Main;
        }
    #endif //MEDIAINFO_JSON_YES

    Retour.FindAndReplace(__T("\\"), __T("|SC1|"), 0, Ztring_Recursive);
    ConvertRetour(Retour);

    //Special characters
    ConvertRetourSCX(Retour);

    #if MEDIAINFO_TRACE
        if (XML_0_7_78_MA)
        {
            if (MediaInfoLib::Config.Trace_Level_Get() || MediaInfoLib::Config.Inform_Get()==__T("Details"))
            {
                Retour+=__T("<MediaTrace xmlns=\"http")+(MediaInfoLib::Config.Https_Get()?Ztring(__T("s")):Ztring())+__T("://mediaarea.net/mediatrace\" version=\"0.1\">\n");
                if (!Details.empty())
                    Retour+=Ztring().From_UTF8(Details);
                else if (Info)
                    Retour+=Info->Details_Get();
                Retour+=__T("\n");
                Retour+=__T("</MediaTrace>\n");
            }
        }
    #endif //MEDIAINFO_TRACE

    #if defined(MEDIAINFO_JSON_YES)
    if (MediaInfoLib::Config.Inform_Get()==__T("JSON_URL") || MediaInfoLib::Config.Inform_Get()==__T("Conformance_JSON"))
        Retour.From_UTF8("https://mediaarea.net/MoreInfo?mi="+URL_Encoded_Encode(Retour.To_UTF8()));
    #endif //defined(MEDIAINFO_JSON_YES)

    return Retour;

    #else //defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES)

    return Ztring(); //Disabled

    #endif //defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES)
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES) || defined(MEDIAINFO_CUSTOM_YES)
#if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
namespace
{
struct nested
{
    std::vector<Node*>* Target;
    Ztring Name;
};
}
#endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES
Ztring MediaInfo_Internal::Inform (stream_t StreamKind, size_t StreamPos, bool IsDirect)
{
    //Integrity
    if (StreamKind>=Stream_Max || StreamPos>=Stream[StreamKind].size())
        return Ztring();

    if (MediaInfoLib::Config.Inform_Get(__T("General")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Video")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Audio")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Text")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Other")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Image")).empty()
     && MediaInfoLib::Config.Inform_Get(__T("Menu")).empty())
    {
        Ztring Retour;
        #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
        bool XML=false;
        bool XML_0_7_78=false;
        bool JSON=false;
        bool Conformance_JSON=false;
        #endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES

        #if defined(MEDIAINFO_HTML_YES)
        bool HTML=MediaInfoLib::Config.Inform_Get()==__T("HTML")?true:false;
        #endif //defined(MEDIAINFO_HTML_YES)
        #if defined(MEDIAINFO_XML_YES)
        XML=MediaInfoLib::Config.Inform_Get()==__T("OLDXML")?true:false;
        XML_0_7_78=(MediaInfoLib::Config.Inform_Get()==__T("MAXML") || MediaInfoLib::Config.Inform_Get()==__T("MIXML") || MediaInfoLib::Config.Inform_Get() == __T("XML"))?true:false;
        if (XML_0_7_78)
            XML=true;
        #endif //defined(MEDIAINFO_XML_YES)
        #if defined(MEDIAINFO_JSON_YES)
        JSON=(MediaInfoLib::Config.Inform_Get()==__T("JSON") || MediaInfoLib::Config.Inform_Get()==__T("JSON_URL") || MediaInfoLib::Config.Inform_Get()==__T("Conformance_JSON"))?true:false;
        Conformance_JSON=MediaInfoLib::Config.Inform_Get()==__T("JSON_URL") || MediaInfoLib::Config.Inform_Get()==__T("Conformance_JSON");
        #endif //defined(MEDIAINFO_JSON_YES)
        #if defined(MEDIAINFO_CSV_YES)
        bool CSV=MediaInfoLib::Config.Inform_Get()==__T("CSV")?true:false;
        #endif //defined(MEDIAINFO_CSV_YES)
        #if defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES))
        bool Text=true;
        #if defined(MEDIAINFO_HTML_YES)
         if (HTML)
             Text=false;
        #endif //defined(MEDIAINFO_HTML_YES)
        #if defined(MEDIAINFO_XML_YES)
         if (XML)
             Text=false;
        #endif //defined(MEDIAINFO_XML_YES)
        #if defined(MEDIAINFO_JSON_YES)
         if (JSON)
             Text=false;
        #endif //defined(MEDIAINFO_JSON_YES)
        #if defined(MEDIAINFO_CSV_YES)
         if (CSV)
             Text=false;
        #endif //defined(MEDIAINFO_CSV_YES)
        #endif //defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES))

        #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
        std::vector<Node*> Fields;
        std::vector<Node*>* Fields_Current=&Fields;
        std::vector<Node*>* Extra=NULL;
        std::vector<nested> Nested;
        bool ExpandSub=Config.File_ExpandSubs_Get();
        if ((XML_0_7_78 || JSON) && ExpandSub)
            Config.File_ExpandSubs_Set(false);
        #endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES

        size_t Size = Count_Get(StreamKind, StreamPos);
        for (size_t Champ_Pos=0; Champ_Pos<Size; Champ_Pos++)
        {
            //Pour chaque champ
            //Ztring A=Get((stream_t)4, 2, 0, Info_Measure_Text); // TODO Bug sinon? voir Ztring
            Ztring A=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Measure_Text); // TODO Bug sinon? voir Ztring
            bool Shouldshow=false;
            #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
            if (XML_0_7_78 || JSON)
            {
                Ztring Options=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Options);
                if (Champ_Pos>=Stream[StreamKind][StreamPos].size())
                {
                    if (InfoOption_ShowInXml>=Options.size() || Options[InfoOption_ShowInXml]==__T('Y'))
                        Shouldshow=true;
                }
                else
                {
                    if (InfoOption_ShowInXml<Options.size() && Options[InfoOption_ShowInXml]==__T('Y'))
                        Shouldshow=true;
                }
            }
            else
            #endif //defined(MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES
            if (Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Options)[InfoOption_ShowInInform]==__T('Y'))
                Shouldshow=true;
            if (!Shouldshow && MediaInfoLib::Config.Complete_Get())
                Shouldshow=true;
            #if defined(MEDIAINFO_JSON_YES)
                if (Conformance_JSON)
                {
                    Ztring Name=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Name);
                    if (Name!=__T("StreamKind") && Name!=__T("Format") && Name!=__T("Metadata_Format") && Name.rfind(__T("Conformance"), 11))
                        Shouldshow=false; // Override, it is intended only for conformance checks
                }
            #endif 
            if (Shouldshow && !Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Text).empty())
            {
                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                //Extra
                if ((XML_0_7_78 || JSON) && !Extra && Champ_Pos>=Stream[StreamKind][StreamPos].size())
                {
                    Node* Node_Extra=new Node("extra");
                    Fields.push_back(Node_Extra);
                    Fields_Current=Extra=&Node_Extra->Childs;
                }
                #endif //defined(MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES

                Ztring Nom=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Name_Text);
                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                if (Nom.empty() || XML_0_7_78 || JSON)
                #else
                if (Nom.empty())
                #endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES

                    Nom=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Name); //Texte n'existe pas

                //Subs
                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                if (XML || JSON)
                {
                    const Ztring& NextName=Get((stream_t)StreamKind, StreamPos, Champ_Pos+1, Info_Name);
                    size_t Nom_ToErase=0;

                    if (!Nested.empty())
                    {
                        nested* LastNested=&Nested[Nested.size()-1];
                        while(!Nested.empty()
                         && !(Nom.size()>LastNested->Name.size() && !Nom.rfind(LastNested->Name, LastNested->Name.size()) && Nom[LastNested->Name.size()]==__T(' ')))
                        {
                            Fields_Current=LastNested->Target;
                            Nested.pop_back();
                            if (Nested.empty())
                                break;
                            LastNested=&Nested[Nested.size()-1];
                        }
                        if (Nested.empty())
                        {
                            if (Extra)
                                Fields_Current=Extra;
                            else
                                Fields_Current=&Fields;
                        }
                        else
                            Nom_ToErase=LastNested->Name.size()+1;
                    }

                    if (NextName.size()>Nom.size() && !NextName.rfind(Nom, Nom.size()) && NextName[Nom.size()]==__T(' '))
                    {
                        string SubName=Nom.To_UTF8();
                        size_t Space=SubName.rfind(' ');
                        if (Space!=(size_t)-1)
                            SubName.erase(0, Space+1);
                        size_t NumbersPos=SubName.find_last_not_of("0123456789");
                        if (NumbersPos!=(size_t)-1)
                            SubName.resize(NumbersPos+1);
                        if (XML_0_7_78 || JSON)
                            SubName=Xml_Name_Escape_0_7_78(Ztring().From_UTF8(SubName)).To_UTF8();
                        else
                            SubName=Xml_Name_Escape(Ztring().From_UTF8(SubName)).To_UTF8();
                        bool IsArray=NumbersPos!=(size_t)-1;
                        Node* Node_Sub=new Node(SubName.c_str(), IsArray);
                        Fields_Current->push_back(Node_Sub);
                        Nested.resize(Nested.size()+1);
                        nested& LastNested=Nested[Nested.size()-1];
                        LastNested.Name=Nom;
                        LastNested.Target=Fields_Current;
                        Fields_Current=&Node_Sub->Childs;
                        continue;
                    }
                    Nom.erase(0, Nom_ToErase);
                }
                #endif // defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)

                #if defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                if (Text)
                #endif //defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                {
                     int8u Nom_Size=MediaInfoLib::Config.Language_Get(__T("  Config_Text_ColumnSize")).To_int8u();
                     if (Nom_Size==0)
                        Nom_Size=32; //Default
                     Nom.resize(Nom_Size, ' ');
                }
                Ztring Valeur=Get((stream_t)StreamKind, StreamPos, Champ_Pos, Info_Text);

                //Handling values with \r\n inside
                #if defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                if (Text)
                #endif //defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                {
                    if (Valeur.find(__T('\r'))!=string::npos || Valeur.find(__T('\n'))!=string::npos)
                    {
                        Valeur.FindAndReplace(__T("\r\n"), __T(" / "), 0, Ztring_Recursive);
                        Valeur.FindAndReplace(__T("\r"), __T(" / "), 0, Ztring_Recursive);
                        Valeur.FindAndReplace(__T("\n"), __T(" / "), 0, Ztring_Recursive);
                        if (Valeur.size()>=3 && Valeur.rfind(__T(" / "))== Valeur.size()-3)
                            Valeur.resize(Valeur.size()-3);
                    }
                }

                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                    if ((XML_0_7_78 || JSON) && MediaInfoLib::Config.Info_Get(StreamKind).Read(Champ_Pos, Info_Measure)==__T(" ms"))
                    {
                        size_t Decimal = Valeur.find(__T('.'));
                        size_t Precision=3;
                        if (Decimal != (size_t)-1)
                            Precision+=Valeur.size()-Decimal-1;
                        float64 Ms=Valeur.To_float64();
                        Valeur.From_Number(Ms/1000, Precision);
                    }
                #endif // defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                Valeur.FindAndReplace(__T("\\"), __T("|SC1|"), 0, Ztring_Recursive);
                #if defined(MEDIAINFO_HTML_YES)
                if (HTML)
                {
                    size_t Level=Nom.find_first_not_of(__T(" "));
                    Ztring Prefix;
                    for (size_t Pos=0; Pos<Level; Pos++)
                        Prefix+=__T("&nbsp;");

                    Retour+=__T("  <tr>\n    <td><i>");
                    Retour+=Prefix+Nom.TrimLeft();
                    Retour+=__T(" :</i></td>\n    <td colspan=\"3\">");
                    Retour+=Valeur;
                    Retour+=__T("</td>\n  </tr>");
                }
                #endif //defined(MEDIAINFO_HTML_YES)
                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                if (XML || JSON)
                {
                    if (XML_0_7_78 || JSON)
                        Nom=Xml_Name_Escape_0_7_78(Nom);
                    else
                        Nom=Xml_Name_Escape(Nom);
                    size_t Modified;
                    Content_Encode_Modifying(Valeur, Modified);
                    if ((XML_0_7_78 || JSON) && Nom.size()>8 && Nom.rfind(__T("_Version"))==Nom.size()-8 && Valeur.size()>8 && Valeur.rfind(__T("Version "), 0)==0)
                    {
                        Valeur.erase(0, 8); // Remove useless "Version "
                        size_t SlashPos = Valeur.find(__T(" / "));
                        if (SlashPos!=string::npos)
                            Valeur.erase(SlashPos);
                    }
                    Ztring Format_Profile_More;
                    if ((XML_0_7_78 || JSON) && Nom==__T("Format_Profile"))
                    {
                        size_t SeparatorPos=Valeur.find(__T('@'));
                        if (SeparatorPos!=string::npos && Valeur.find(__T(" / "))==string::npos) //TODO: better support of compatibility modes (e.g. "Multiview") and sequences (e.g. different profiles in different files "BCS@L3 / BCS@L2 / BCS@L3")
                        {
                            Format_Profile_More=Valeur.substr(SeparatorPos+1);
                            Valeur.erase(SeparatorPos);
                        }
                    }
                    if ((XML_0_7_78 || JSON) && StreamKind==Stream_Audio && (Nom==__T("Format_Profile") || Nom==__T("SamplingRate") || Nom==__T("Channel(s)") || Nom==__T("ChannelPositions") || Nom==__T("ChannelLayout")))
                    {
                        size_t SlashPos = Valeur.find(__T(" / "));
                        if (SlashPos!=string::npos)
                            Valeur.erase(SlashPos);
                    }

                    Node* Node_Current=NULL;
                    Node_Current=new Node(Nom.To_UTF8());
                    if (Modified==1 && !MediaInfoLib::Config.SkipBinaryData_Get()) //Base64
                        Node_Current->Add_Attribute("dt", "binary.base64");

                    if (Modified==1 && MediaInfoLib::Config.SkipBinaryData_Get())
                        Node_Current->Value="(Binary data)";
                    else
                        Node_Current->Value=Valeur.To_UTF8();
                    Fields_Current->push_back(Node_Current);

                    if (!Format_Profile_More.empty())
                    {
                        if (Format_Profile_More.size()>=2 && Format_Profile_More[0]==__T('L') && Format_Profile_More[1]>=__T('0') && Format_Profile_More[1]<=__T('9'))
                            Format_Profile_More.erase(0, 1);
                        size_t SeparatorPos=Format_Profile_More.find(__T('@'));
                        if (SeparatorPos!=string::npos)
                        {
                            Node_Current=new Node("Format_Level", Ztring(Format_Profile_More.substr(0, SeparatorPos)).To_UTF8());
                            Fields_Current->push_back(Node_Current);
                            Node_Current=new Node("Format_Tier", Ztring(Format_Profile_More.substr(SeparatorPos+1)).To_UTF8());
                            Fields_Current->push_back(Node_Current);
                        }
                        else
                        {
                            Node_Current=new Node("Format_Level", Format_Profile_More.To_UTF8());
                            Fields_Current->push_back(Node_Current);
                        }
                    }
                }

                #endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                #if defined(MEDIAINFO_CSV_YES)
                if (CSV)
                {
                    Retour+=Nom;
                    Retour+=__T(",");
                    Retour+=Valeur;
                }
                #endif //defined(MEDIAINFO_CSV_YES)
                #if defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                if (Text)
                #endif //defined(MEDIAINFO_TEXT_YES) && (defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES) || defined(MEDIAINFO_CSV_YES))
                    Retour+=Nom + MediaInfoLib::Config.Language_Get(__T("  Config_Text_Separator")) + Valeur;
                #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
                if (!XML && !JSON)
                #endif //MEDIAINFO_XML_YES || MEDIAINFO_JSON_YES
                    Retour+=MediaInfoLib::Config.LineSeparator_Get();
            }
        }
        #if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
        if (ExpandSub)
            Config.File_ExpandSubs_Set(true);
        for (size_t Field=0; Field < Fields.size(); Field++)
        {
            if (XML)
                Retour+=Ztring().From_UTF8(To_XML(*(Fields[Field]), 1, false, false));
            else if (JSON)
            {
                Retour+=Ztring().From_UTF8(To_JSON(*(Fields[Field]), 1, false, false, false));
                if (Field<Fields.size()-1)
                {
                    Retour+=__T(',');
                    if (!Conformance_JSON)
                        Retour+=__T('\n');
                }
            }
            delete Fields[Field];
        }
        #endif //defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
        ConvertRetour(Retour);

        #if defined(MEDIAINFO_JSON_YES)
            if (JSON)
                Retour.FindAndReplace(__T("|SC1|"), __T("\\\\"), 0, Ztring_Recursive);
            else
                Retour.FindAndReplace(__T("|SC1|"), __T("\\"), 0, Ztring_Recursive);
        #else
            Retour.FindAndReplace(__T("|SC1|"), __T("\\"), 0, Ztring_Recursive);
        #endif

        return Retour;
    }

    Ztring Retour=MediaInfoLib::Config.Inform_Get(Get(StreamKind, 0, __T("StreamKind"), Info_Text));

    //Special characters
    Retour.FindAndReplace(__T("\\\\"), __T("|SC1|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\["), __T("|SC2|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\]"), __T("|SC3|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\,"), __T("|SC4|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\;"), __T("|SC5|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\("), __T("|SC6|"), 0, Ztring_Recursive);
    Retour.FindAndReplace(__T("\\)"), __T("|SC7|"), 0, Ztring_Recursive);

    //Gestion $xx$
    size_t PosX=0;
    while (Retour.find(__T('$'), PosX)!=(size_t)-1)
    {
        PosX=Retour.find(__T('$'), PosX);
        if (Retour.size()>PosX+2 && !(Retour(PosX+1)==__T('i') && Retour(PosX+2)==__T('f') && Retour(PosX+3)==__T('('))) //To keep out "%" without any signification, or "$if(..."
        {
            Ztring ARemplacer=Ztring(__T("$")+Retour.SubString(__T("$"), __T("$"), PosX))+__T("$");
            Ztring RemplacerPar=MediaInfoLib::Config.Language_Get(Retour.SubString(__T("$"), __T("$"), PosX)); //TODO : case sensitive
            Retour.FindAndReplace(ARemplacer, RemplacerPar);
        }
        else if (PosX == (size_t)-1)
            break;
        else
            PosX++;
    }

    //Gestion $if()
    size_t Position=Retour.find(__T("$if("));
    while (Position!=Error && Position>0)
    {
        ZtringList Elements;
        size_t Elements_Index;
        Elements.Separator_Set(0, __T(","));
        Elements.Write(Retour.SubString(__T("$if("), __T(")"), Position));
        Elements(0)=Elements(0).SubString(__T("%"), __T("%"));

        //Test if there is something to replace
        size_t Pos=MediaInfoLib::Config.Info_Get(StreamKind).Find(Elements(0));
        if (Pos!=std::string::npos)
        {
            if (Get(StreamKind, StreamPos, Pos).size()>0)
                Elements_Index=1;
            else
                Elements_Index=2;
        }
        else
        {
            if (!Get(StreamKind, StreamPos, Elements(0)).empty())
                Elements_Index=1;
            else
                Elements_Index=2;
        }

        //Replace
        while (Elements(Elements_Index).SubString(__T("%"), __T("%")).size()>0)
        {
            Ztring ToReplace=Elements(Elements_Index).SubString(__T("%"), __T("%"));
            Ztring ReplacedBy=Get(StreamKind, StreamPos, ToReplace);
            ReplacedBy.FindAndReplace(__T("\\"), __T("|SC1|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T("["), __T("|SC2|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T("]"), __T("|SC3|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T(","), __T("|SC4|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T(";"), __T("|SC5|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T("("), __T("|SC6|"), 0, Ztring_Recursive);
            ReplacedBy.FindAndReplace(__T(")"), __T("|SC7|"), 0, Ztring_Recursive);
            ToReplace=Ztring(__T("%"))+ToReplace+Ztring(__T("%"));
            Elements(Elements_Index).FindAndReplace(ToReplace, ReplacedBy);
        }

        Ztring ToReplace=Ztring(__T("$if("))+Retour.SubString(__T("$if("), __T(")"), Position)+__T(")");
        Retour.FindAndReplace(ToReplace, Elements(Elements_Index));
        Position=Retour.find(__T("$if("));
    }

    //Gestion []
    while (!Retour.SubString(__T("["), __T("]")).empty())
    {
        Ztring Crochets=Retour.SubString(__T("["), __T("]"));
        Ztring ValueToFind=Crochets.SubString(__T("%"), __T("%"));
        Ztring ARemplacer=Ztring(__T("[")+Crochets+__T("]"));
        {
            Ztring RemplacerPar= Get(StreamKind, StreamPos, ValueToFind);
            if (RemplacerPar.empty())
                Retour.FindAndReplace(ARemplacer, Ztring());
            else
            {
                //Formate l'interieur
                Ztring ATraiter=Crochets;
                Ztring Crochets_ARemplacer=__T("%")+ValueToFind+__T("%");
                ATraiter.FindAndReplace(Crochets_ARemplacer, RemplacerPar);
                Retour.FindAndReplace(ARemplacer, ATraiter);
            }
        }
    }

    //Gestion %xxx%
    PosX=0;
    while (Retour.find(__T('%'), PosX)!=(size_t)-1)
    {
        PosX=Retour.find(__T('%'), PosX);
        if (Retour.size() > PosX + 2
         && ((Retour[PosX + 1] >= __T('a') && Retour[PosX + 1] <= __T('z'))
          || (Retour[PosX + 1] >= __T('A') && Retour[PosX + 1] <= __T('Z')))) //To keep out "%" without any signification
        {
            Ztring ARemplacer = Ztring(__T("%") + Retour.SubString(__T("%"), __T("%"), PosX)) + __T("%");
            Ztring RemplacerPar = Get(StreamKind, StreamPos, Retour.SubString(__T("%"), __T("%"), PosX));
            RemplacerPar.FindAndReplace(__T("\\"), __T("|SC1|"), 0, Ztring_Recursive);
            RemplacerPar.FindAndReplace(__T("),"), __T("|SC9|"), 0, Ztring_Recursive);
            RemplacerPar.FindAndReplace(__T(")"), __T("|SC8|"), 0, Ztring_Recursive);
            Retour.FindAndReplace(ARemplacer, RemplacerPar);
        }
        else if (PosX == (size_t)-1)
            break;
        else
            PosX++;
    }

    //Retour=__T("<table width=\"100%\" border=\"0\" cellpadding=\"1\" cellspacing=\"2\" style=\"border:1px solid Navy\">\n<tr>\n    <td width=\"150\">Video #0</td>\n  </tr>\r\n  <tr>\n    <td><i>Codec :</i></td>\n    <td colspan=\"3\">WMV1</td>\n  </tr>\r\n  <tr>\n    <td><i>Codec/Info :</i></td>\n    <td colspan=\"3\">Windows Media Video 7</td>\n  </tr>\r\n  <tr>\n    <td><i>Width :</i></td>\n    <td colspan=\"3\">200 pixels</td>\n  </tr>\r\n  <tr>\n    <td><i>Height :</i></td>\n    <td colspan=\"3\">150 pixels</td>\n  </tr>\r\n  <tr>\n    <td><i>Aspect ratio :</i></td>\n    <td colspan=\"3\">4/3</td>\n  </tr>\r\n  <tr>\n    <td><i>Resolution :</i></td>\n    <td colspan=\"3\">24 bits</td>\n  </tr>\r\n</table>\n");
    ConvertRetour(Retour);

    //Special characters
    if (IsDirect)
    {
        ConvertRetourSCX(Retour);
    }

    return Retour;
}
#endif //defined(MEDIAINFO_TEXT_YES) || defined(MEDIAINFO_HTML_YES) || defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_CSV_YES) || defined(MEDIAINFO_CUSTOM_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_CUSTOM_YES)
void MediaInfo_Internal::Traiter(Ztring &C)
{
    //$if(%a%,zezeze%a%,rrere)
    size_t Position=C.find(__T("$if("));
    while (Position>0)
    {
        //Recuperation de la chaine entiere
        Ztring Total;
        Ztring ARemplacer;
        ZtringList Total1;
        Total1.Separator_Set(0, __T("),"));
        Total=C.SubString(__T("$if("), __T(")"), Position);
        ARemplacer=Ztring(__T("$if(")+Total+__T(")"));
        Total1.Write(Total);
        if (Total1(0).empty()) //mettre champ2
            C.FindAndReplace(ARemplacer, Total1(2), Position);
        else
            C.FindAndReplace(ARemplacer, Total1(1), Position);
        Position=C.find(__T("$if("), Position);
    }

    //reformatage
    C.FindAndReplace(__T("|SC8|"), __T(")"), 0, Ztring_Recursive);
    C.FindAndReplace(__T("|SC9|"), __T("),"), 0, Ztring_Recursive);
    //C.FindAndReplace(__T("\\r\\n"), __T("\n"), 0, Ztring_Recursive);
}
#endif //defined(MEDIAINFO_CUSTOM_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_XML_YES)  || defined(MEDIAINFO_JSON_YES)
Ztring MediaInfo_Internal::Xml_Name_Escape (const Ztring &Name)
{
    Ztring ToReturn(Name);

    if (ToReturn.operator()(0)>='0' && ToReturn.operator()(0)<='9')
        ToReturn.insert(0, 1, __T('_'));
    ToReturn.FindAndReplace(__T(" "), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("/"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("("), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(")"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("*"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(","), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T(":"), __T("_"), 0, Ztring_Recursive);
    ToReturn.FindAndReplace(__T("@"), __T("_"), 0, Ztring_Recursive);
    size_t ToReturn_Pos=0;
    while (ToReturn_Pos<ToReturn.size())
    {
        if (!(ToReturn[ToReturn_Pos]>=__T('A') && ToReturn[ToReturn_Pos]<=__T('Z'))
         && !(ToReturn[ToReturn_Pos]>=__T('a') && ToReturn[ToReturn_Pos]<=__T('z'))
         && !(ToReturn[ToReturn_Pos]>=__T('0') && ToReturn[ToReturn_Pos]<=__T('9'))
         && !(ToReturn[ToReturn_Pos]==__T('_')))
            ToReturn.erase(ToReturn_Pos, 1);
        else
            ToReturn_Pos++;
    }
    if (ToReturn.empty())
        ToReturn="Unknown";

    return ToReturn;
}
#endif //defined(MEDIAINFO_XML_YES)  || defined(MEDIAINFO_JSON_YES)

#if defined(MEDIAINFO_XML_YES)
//---------------------------------------------------------------------------
Ztring MediaInfo_Internal::Xml_Content_Escape (const Ztring &Content, size_t &Modified)
{
    Ztring ToReturn(Content);
    return Xml_Content_Escape_Modifying(ToReturn, Modified);
}

//---------------------------------------------------------------------------
size_t Xml_Content_Escape_MustEscape(const Ztring &Content)
{
    size_t Pos=0;
    size_t Size=Content.size();
    for (; Pos<Size; Pos++)
    {
        switch (Content[Pos])
        {
            case __T('\"'):
            case __T('&') :
            case __T('\''):
            case __T('<') :
            case __T('>') :
                            return Pos;
            default      :
                            if (Content[Pos]>=0x0 && Content[Pos]<0x20)
                                return Pos;
        }
    }

    return Pos;
}

//---------------------------------------------------------------------------
Ztring &MediaInfo_Internal::Xml_Content_Escape_Modifying (Ztring &Content, size_t &Modified)
{
    size_t Pos=Xml_Content_Escape_MustEscape(Content);
    Ztring Content_Save=Content;
    Modified=0;
    if (Pos>=Content.size())
        return Content;

    for (; Pos<Content.size(); Pos++)
    {
        switch (Content[Pos])
        {
            case __T('\"'):
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, __T("quot;"));
                            Pos+=5;
                            break;
            case __T('&'):
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, __T("amp;"));
                            Pos+=4;
                            break;
            case __T('\''):
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, __T("apos;"));
                            Pos+=5;
                            break;
            case __T('<'):
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, __T("lt;"));
                            Pos+=3;
                            break;
            case __T('>'):
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, __T("gt;"));
                            Pos+=3;
                            break;
            case __T('\r'):
            case __T('\n'):
                            break;
            default:
                        if (Content[Pos]>=0x0 && Content[Pos]<0x20)
                        {
                            /* Is still invalid XML
                            Ztring Character=__T("#x")+Ztring::ToZtring(Content[Pos]/16, 16)+Ztring::ToZtring(Content[Pos]%16, 16)+__T(";");
                            Content[Pos]=__T('&');
                            Content.insert(Pos+1, Character);
                            Pos+=5;
                            */
                            string Content_Utf8=Content_Save.To_UTF8(); //TODO: shouldn't we never convert to Unicode?
                            string Content_Base64=Base64::encode(Content_Utf8);
                            Content.From_UTF8(Content_Base64);
                            Modified=1; //Base64
                            Pos=Content.size(); //End
                        }
        }
    }

    return Content;
}
#endif //defined(MEDIAINFO_XML_YES)

#if defined(MEDIAINFO_XML_YES) || defined(MEDIAINFO_JSON_YES)
//---------------------------------------------------------------------------
size_t Content_MustEncode (const Ztring &Content)
{
    size_t Pos=0;
    size_t Size=Content.size();
    for (; Pos<Size; Pos++)
        if (Content[Pos]>=0x0 && Content[Pos]<0x20)
            return Pos;

    return Pos;
}

//---------------------------------------------------------------------------
Ztring &MediaInfo_Internal::Content_Encode_Modifying (Ztring &Content, size_t &Modified)
{
    size_t Pos=Content_MustEncode(Content);
    Ztring Content_Save=Content;
    Modified=0;
    if (Pos>=Content.size())
        return Content;

    for (; Pos<Content.size(); Pos++)
    {
        if (Content[Pos]>=0x0 && Content[Pos]<0x20)
        {
            string Content_Utf8=Content_Save.To_UTF8(); //TODO: shouldn't we never convert to Unicode?
            string Content_Base64=Base64::encode(Content_Utf8);
            Content.From_UTF8(Content_Base64);
            Modified=1; //Base64
            Pos=Content.size(); //End
        }
    }

    return Content;
}
#endif

} //NameSpace
