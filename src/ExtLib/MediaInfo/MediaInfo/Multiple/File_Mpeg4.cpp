/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Main part
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
#ifdef MEDIAINFO_MPEG4_YES
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mpeg4.h"
#if defined(MEDIAINFO_MPEGPS_YES)
    #include "MediaInfo/Multiple/File_MpegPs.h"
#endif
#if defined(MEDIAINFO_PCM_YES)
    #include "MediaInfo/Audio/File_Pcm.h"
#endif
#include "ZenLib/FileName.h"
#include "MediaInfo/MediaInfo_Internal.h"
#if MEDIAINFO_EVENTS
    #include "MediaInfo/MediaInfo_Events.h"
#endif //MEDIAINFO_EVENTS
#ifdef MEDIAINFO_REFERENCES_YES
    #include "MediaInfo/Multiple/File__ReferenceFilesHelper.h"
#endif //MEDIAINFO_REFERENCES_YES
#include "MediaInfo/Multiple/File_Mpeg4_TimeCode.h"
#include "MediaInfo/TimeCode.h"
#include "ZenLib/Format/Http/Http_Utils.h"
#include <algorithm>    // std::sort
#include <cfloat>       // DBL_MAX
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Const
//***************************************************************************

namespace Elements
{
    const int64u free=0x66726565;
    const int64u mdat=0x6D646174;
    const int64u moov_meta______=0x2D2D2D2D;
    const int64u moov_meta___ART=0xA9415254;
    const int64u moov_meta___alb=0xA9616C62;
    const int64u moov_meta___ard=0xA9617264;
    const int64u moov_meta___arg=0xA9617267;
    const int64u moov_meta___aut=0xA9617574;
    const int64u moov_meta___con=0xA963696E;
    const int64u moov_meta___cmt=0xA9636D74;
    const int64u moov_meta___cpy=0xA9637079;
    const int64u moov_meta___day=0xA9646179;
    const int64u moov_meta___des=0xA9646573;
    const int64u moov_meta___dir=0xA9646972;
    const int64u moov_meta___dis=0xA9646973;
    const int64u moov_meta___edl=0xA965646C;
    const int64u moov_meta___enc=0xA9656E63;
    const int64u moov_meta___fmt=0xA9666D74;
    const int64u moov_meta___gen=0xA967656E;
    const int64u moov_meta___grp=0xA9677270;
    const int64u moov_meta___hos=0xA9686F73;
    const int64u moov_meta___inf=0xA9696E66;
    const int64u moov_meta___key=0xA96B6579;
    const int64u moov_meta___lyr=0xA96C7972;
    const int64u moov_meta___mak=0xA96D616B;
    const int64u moov_meta___mod=0xA96D6F64;
    const int64u moov_meta___nam=0xA96E616D;
    const int64u moov_meta___ope=0xA96F7065;
    const int64u moov_meta___prd=0xA9707264;
    const int64u moov_meta___PRD=0xA9505244;
    const int64u moov_meta___prf=0xA9707266;
    const int64u moov_meta___req=0xA9726571;
    const int64u moov_meta___sne=0xA9736E65;
    const int64u moov_meta___sol=0xA9736F6C;
    const int64u moov_meta___src=0xA9737263;
    const int64u moov_meta___st3=0xA9737403;
    const int64u moov_meta___swr=0xA9737772;
    const int64u moov_meta___too=0xA9746F6F;
    const int64u moov_meta___url=0xA975726C;
    const int64u moov_meta___wrn=0xA977726E;
    const int64u moov_meta___wrt=0xA9777274;
    const int64u moov_meta___xpd=0xA9787064;
    const int64u moov_meta___xyz=0xA978797A;
    const int64u moov_meta__aART=0x61415254;
    const int64u moov_meta__akID=0x616B4944;
    const int64u moov_meta__albm=0x616C626D;
    const int64u moov_meta__apID=0x61704944;
    const int64u moov_meta__atID=0x61744944;
    const int64u moov_meta__auth=0x61757468;
    const int64u moov_meta__catg=0x63617467;
    const int64u moov_meta__cnID=0x636E4944;
    const int64u moov_meta__cpil=0x6370696C;
    const int64u moov_meta__cprt=0x63707274;
    const int64u moov_meta__covr=0x636F7672;
    const int64u moov_meta__desc=0x64657363;
    const int64u moov_meta__disk=0x6469736B;
    const int64u moov_meta__dscp=0x64736370;
    const int64u moov_meta__egid=0x65676964;
    const int64u moov_meta__flvr=0x666C7672;
    const int64u moov_meta__gnre=0x676E7265;
    const int64u moov_meta__geID=0x67654944;
    const int64u moov_meta__grup=0x67727570;
    const int64u moov_meta__hdvd=0x68647664;
    const int64u moov_meta__itnu=0x69746E75;
    const int64u moov_meta__kind=0x6B696E64;
    const int64u moov_meta__keyw=0x6B657977;
    const int64u moov_meta__ldes=0x6C646573;
    const int64u moov_meta__name=0x6E616D65;
    const int64u moov_meta__pcst=0x70637374;
    const int64u moov_meta__perf=0x70657266;
    const int64u moov_meta__pgap=0x70676170;
    const int64u moov_meta__plID=0x706C4944;
    const int64u moov_meta__purd=0x70757264;
    const int64u moov_meta__purl=0x7075726C;
    const int64u moov_meta__rate=0x72617465;
    const int64u moov_meta__rndu=0x726E6475;
    const int64u moov_meta__rpdu=0x72706475;
    const int64u moov_meta__rtng=0x72746E67;
    const int64u moov_meta__sdes=0x73646573;
    const int64u moov_meta__sfID=0x73664944;
    const int64u moov_meta__soaa=0x736F6161;
    const int64u moov_meta__soal=0x736F616C;
    const int64u moov_meta__soar=0x736F6172;
    const int64u moov_meta__soco=0x736F636F;
    const int64u moov_meta__sonm=0x736F6E6D;
    const int64u moov_meta__sosn=0x736F736E;
    const int64u moov_meta__stik=0x7374696B;
    const int64u moov_meta__tagc=0x74616763;
    const int64u moov_meta__titl=0x7469746C;
    const int64u moov_meta__tool=0x746F6F6C;
    const int64u moov_meta__trkn=0x74726B6E;
    const int64u moov_meta__tmpo=0x746D706F;
    const int64u moov_meta__tven=0x7476656E;
    const int64u moov_meta__tves=0x74766573;
    const int64u moov_meta__tvnn=0x74766E6E;
    const int64u moov_meta__tvsh=0x74767368;
    const int64u moov_meta__tvsn=0x7476736E;
    const int64u moov_meta__xid_=0x78696420;
    const int64u moov_meta__year=0x79656172;
    const int64u moov_meta__yyrc=0x79797263;
    const int64u moov_trak_mdia_hdlr_alis=0x616C6973;
    const int64u moov_trak_mdia_hdlr_hint=0x68696E74;
    const int64u skip=0x736B6970;
    const int64u wide=0x77696465;
}

//---------------------------------------------------------------------------
static Ztring Mpeg4_Encoded_Library(int32u Vendor)
{
    switch (Vendor)
    {
        case 0x33495658 : return __T("3ivX");                //3IVX
        case 0x6170706C : return __T("Apple QuickTime");     //appl
        case 0x6E696B6F : return __T("Nikon");               //niko
        case 0x6F6C796D : return __T("Olympus");             //olym
        case 0x6F6D6E65 : return __T("Omneon");              //omne
        default: return Ztring().From_CC4(Vendor);
    }
}

//---------------------------------------------------------------------------
static Ztring Mpeg4_Language_Apple(int16u Language)
{
    static const int16u ListSize=106;
    static const char* Languages[ListSize] = {
        "en", // English
        "fr", // French
        "de", // German
        "it", // Italian
        "nl", // Dutch
        "sv", // Swedish
        "es", // Spanish
        "da", // Danish
        "pt", // Portuguese
        "no", // Norwegian
        "he", // Hebrew
        "ja", // Japanese
        "ar", // Arabic
        "fi", // Finnish
        "el", // Greek
        "is", // Icelandic
        "mt", // Maltese
        "tr", // Turkish
        "hr", // Croatian
        "zh-tw", // Chinese (Taiwan)
        "ur", // Urdu
        "hi", // Hindi
        "th", // Thai
        "ko", // Korean
        "lt", // Lithuanian
        "pl", // Polish
        "hu", // Hungarian
        "et", // Estonian
        "lv", // Latvian
        "smi", // Sami
        "fo", // Faroese
        "fa", // Persian
        "ru", // Russian
        "zh-cn", // Chinese (China)
        "nl-be", // Flemish
        "ga", // Irish
        "sq", // Albanian
        "ro", // Romanian
        "cs", // Czech
        "sk", // Slovak
        "sl", // Slovenian
        "yi", // Yiddish
        "sr", // Serbian
        "mk", // Macedonian
        "bg", // Bulgarian
        "uk", // Ukrainian
        "be", // Belarusian
        "uz", // Uzbek
        "kk", // Kazakh
        "az", // Azerbaijani
        "hy-az", // Armenian (Azerbaijani)
        "hy", // Armenian
        "ka", // Georgian
        "mo", // Moldavian
        "ky", // Kirghiz
        "tg", // Tajik
        "tk", // Turkmen
        "mn-cn", // Mongolian (China)
        "mn", // Mongolian
        "ps", // Pushto
        "ku", // Kurdish
        "ks", // Kashmiri
        "sd", // Sindhi
        "bo", // Tibetan
        "ne", // Nepali
        "sa", // Sanskrit
        "mr", // Marathi
        "bn", // Bengali
        "as", // Assamese
        "gu", // Gujarati
        "pa", // Panjabi
        "or", // Oriya
        "ml", // Malayalam
        "kn", // Kannada
        "ta", // Tamil
        "te", // Telugu
        "si", // Sinhala
        "my", // Burmese
        "km", // Khmer
        "lo", // Lao
        "vi", // Vietnamese
        "id", // Indonesian
        "tl", // Tagalog
        "ms", // Malay
        "ms-bn", // Malay (Brunei)
        "am", // Amharic
        "86", // Empty
        "om", // Oromo
        "so", // Somali
        "sw", // Swahili
        "rw", // Kinyarwanda
        "rn", // Rundi
        "ny", // Nyanja
        "mg", // Malagasy
        "eo", // Esperanto
        // Gap 95-128
        "cy", // Welsh
        "eu", // Basque
        "ca", // Catalan
        "la", // Latin
        "qu", // Quechua
        "gn", // Guarani
        "ay", // Aymara
        "tt", // Tatar
        "ug", // Uighur
        "dz", // Dzongkha
        "jv" // Javanese
    };

    if (Language>94)
        Language-=(128-94); // Gap in the list

    if (Language>=ListSize)
        return Ztring::ToZtring(Language);

    return Ztring().From_UTF8(Languages[Language]);
}

//---------------------------------------------------------------------------
extern const char* Mpeg4_chan(int16u Ordering);
extern const char* Mpeg4_chan_Layout(int16u Ordering);

//---------------------------------------------------------------------------
const char* Mpegv_colour_primaries(int8u colour_primaries);
const char* Mpegv_transfer_characteristics(int8u transfer_characteristics);
const char* Mpegv_matrix_coefficients(int8u matrix_coefficients);
const char* Mpegv_matrix_coefficients_ColorSpace(int8u matrix_coefficients);
const char* Avs3V_matrix_coefficients(int8u matrix_coefficients);

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpeg4::File_Mpeg4()
:File__Analyze(), File__HasReferences()
{
    //Configuration
    ParserName="MPEG-4";
    #if MEDIAINFO_EVENTS
        ParserIDs[0]=MediaInfo_Parser_Mpeg4;
        StreamIDs_Width[0]=8;
    #endif //MEDIAINFO_EVENTS
    #if MEDIAINFO_DEMUX
        Demux_Level=2; //Container
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_TRACE
        //Trace_Layers_Update(0); //Container1
    #endif //MEDIAINFO_TRACE

    DataMustAlwaysBeComplete=false;

    //Temp
    mdat_MustParse=false;
    moov_mvhd_TimeScale=1;
    Vendor=0x00000000;
    FirstMdatPos=(int64u)-1;
    LastMdatPos=0;
    FirstMoovPos=(int64u)-1;
    MajorBrand=0x00000000;
    IsSecondPass=false;
    IsParsing_mdat=false;
    IsFragmented=false;
    StreamOrder=0;
    meta_pitm_item_ID=(int32u)-1;
    meta_iprp_ipco_Buffer=NULL;
    moov_trak_mdia_minf_stbl_stsd_Pos=0;
    moov_trak_tkhd_TrackID=(int32u)-1;
    mdat_Pos_NormalParsing=false;
    moof_traf_base_data_offset=(int64u)-1;
    data_offset_present=true;
    #if MEDIAINFO_NEXTPACKET
        ReferenceFiles_IsParsing=false;
    #endif //MEDIAINFO_NEXTPACKET
    mdat_Pos_Temp_ToJump=NULL;
    #if MEDIAINFO_DEMUX
        TimeCode_FrameOffset=0;
        TimeCode_DtsOffset=0;
    #endif //MEDIAINFO_DEMUX
    #if MEDIAINFO_CONFORMANCE
        IsCmaf=false;
    #endif
}

//---------------------------------------------------------------------------
File_Mpeg4::~File_Mpeg4()
{
    delete[] meta_iprp_ipco_Buffer;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4::Streams_Accept()
{
    if (!IsSub)
    {
        /*bool IsDashMpd=false;
        for (size_t Pos=0; pos<ftyps
        TestContinuousFileNames();*/
    }

    Streams_Accept_jp2();

    //Configuration
    Buffer_MaximumSize=64*1024*1024; //Some big frames are possible (e.g YUV 4:2:2 10 bits 1080p)
    File_Buffer_Size_Hint_Pointer=Config->File_Buffer_Size_Hint_Pointer_Get();
}


//---------------------------------------------------------------------------
void File_Mpeg4::Streams_Accept_jp2(bool IsJp2)
{
    //Warning: this method can be called several times

    if (IsSub || StreamKind_Last != Stream_General || !(IsJp2 || MajorBrand==0x6A703220 || MajorBrand==0x6a707820)) //"jp2 "
        return;

    StreamSource=IsStream; //TODO: do the difference between raw stream and sequence of files with file count being frame count
    TestContinuousFileNames();

    Stream_Prepare((Config->File_Names.size()>1 || Config->File_IsReferenced_Get())?Stream_Video:Stream_Image);
    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Format), "JPEG 2000");
    if (StreamKind_Last==Stream_Video)
        Fill(Stream_Video, StreamPos_Last, Video_FrameCount, Config->File_Names.size());
}

//---------------------------------------------------------------------------
void File_Mpeg4::Streams_Finish()
{
    #if MEDIAINFO_DEMUX
                //Handling of multiple frames in one block
        if (Config->Demux_Unpacketize_Get())
         {
        streams::iterator Stream_Temp = Streams.find((int32u)Element_Code);
        if (Stream_Temp != Streams.end() && Stream_Temp->second.Demux_EventWasSent)
             {
            Frame_Count_NotParsedIncluded = (int64u)-1;
            Open_Buffer_Continue(Stream_Temp->second.Parsers[0], Buffer + Buffer_Offset, 0);
            if (Config->Demux_EventWasSent)
                 return;
            Stream_Temp->second.Demux_EventWasSent = false;
            }
        }
    #endif //MEDIAINFO_DEMUX
    #if defined(MEDIAINFO_REFERENCES_YES) && MEDIAINFO_NEXTPACKET
        //Locators only
        if (ReferenceFiles_IsParsing)
        {
            ReferenceFiles->ParseReferences();
            #if MEDIAINFO_DEMUX
                if (Config->Demux_EventWasSent)
                    return;
            #endif //MEDIAINFO_DEMUX

            Streams_Finish_CommercialNames();
            return;
        }
    #endif //defined(MEDIAINFO_REFERENCES_YES) && MEDIAINFO_NEXTPACKET

    //Final Cut EIA-608 format
    if (Retrieve(Stream_General, 0, General_Format)==__T("Final Cut EIA-608"))
    {
        for (streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
            for (size_t Pos=0; Pos<Stream->second.Parsers.size(); Pos++)
            {
                File__Analyze* Parser=Stream->second.Parsers[Pos];
                if (!Parser)
                    continue;
                Parser->Finish();
                if (Parser->Count_Get(Stream_Text))
                {
                    Stream_Prepare(Stream_Text);
                    Fill(Stream_Text, StreamPos_Last, Text_ID, Stream->first==1?"608-1":"608-2");
                    Fill(Stream_Text, StreamPos_Last, "MuxingMode", __T("Final Cut"), Unlimited);
                    Merge(*Parser, Stream_Text, 0, StreamPos_Last);
                }

                //Law rating
                Ztring LawRating=Stream->second.Parsers[Pos]->Retrieve(Stream_General, 0, General_LawRating);
                if (!LawRating.empty())
                    Fill(Stream_General, 0, General_LawRating, LawRating, true);
                Ztring Title=Stream->second.Parsers[Pos]->Retrieve(Stream_General, 0, General_Title);
                if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                    Fill(Stream_General, 0, General_Title, Title);
            }

        return;
    }

    Fill_Flush();
    int64u File_Size_Total=File_Size;

    //TimeCode
    for (streams::iterator Temp=Streams.begin(); Temp!=Streams.end(); ++Temp)
        if (Temp->second.TimeCode)
            TimeCode_Associate(Temp->first);

    //For each stream
    vector<size_t> ToDelete_StreamPos;
    streams::iterator Temp=Streams.begin();
    while (Temp!=Streams.end())
    {
        //Preparing
        StreamKind_Last=Temp->second.StreamKind;
        StreamPos_Last=Temp->second.StreamPos;

        if (StreamKind_Last==Stream_Max && !Temp->second.MetaFor.empty())
        {
            ++Temp;
            continue;
        }
        if (Temp->second.hdlr_SubType && (StreamPos_Last==(size_t)-1 || Retrieve_Const(Stream_Other, StreamPos_Last, Other_Type).empty()))
        {
            bool ShowSubType=false;
            if (StreamKind_Last==Stream_Other)
            {
                const auto& Format=Retrieve_Const(Stream_Other, StreamPos_Last, Other_Format);
                const auto& CodecID=Retrieve_Const(Stream_Other, StreamPos_Last, Other_CodecID);
                if (Format.empty() || Format==CodecID)
                    ShowSubType=true;
            }
            if (StreamPos_Last==(size_t)-1)
            {
                Stream_Prepare(Stream_Other);
                ShowSubType=true;
            }
            if (ShowSubType)
                Fill(Stream_Other, StreamPos_Last, Other_Type, Ztring().From_CC4(Temp->second.hdlr_SubType));
        }
        if (StreamKind_Last == Stream_Video && !DisplayAspectRatio.empty() && Retrieve_Const(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio).empty())
        {
            Fill(Stream_Video, StreamPos_Last, Video_DisplayAspectRatio, DisplayAspectRatio);
        }
        if (StreamKind_Last==Stream_Video && !FrameRate_Real.empty() && Retrieve_Const(Stream_Video, StreamPos_Last, Video_FrameRate_Real).empty())
        {
            Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Real, FrameRate_Real);
        }
        if (StreamKind_Last==Stream_Video && Temp->second.Nclc)
        {
            const auto& Nclc=*Temp->second.Nclc;
            Fill(Stream_Video, StreamPos_Last, Video_colour_description_present, "Yes");
            Fill(Stream_Video, StreamPos_Last, Video_colour_primaries, Mpegv_colour_primaries(Nclc.colour_primaries));
            Fill(Stream_Video, StreamPos_Last, Video_transfer_characteristics, Mpegv_transfer_characteristics(Nclc.transfer_characteristics));
            const char* matrix_coefficients_String;
            if (Retrieve_Const(Stream_Video, StreamPos_Last, Video_CodecID)==__T("avs3"))
                matrix_coefficients_String=Avs3V_matrix_coefficients(Nclc.matrix_coefficients);
            else
                matrix_coefficients_String=Mpegv_matrix_coefficients(Nclc.matrix_coefficients);
            Fill(Stream_Video, StreamPos_Last, Video_matrix_coefficients, matrix_coefficients_String);
            if (Nclc.matrix_coefficients!=2)
                Fill(Stream_Video, StreamPos_Last, Video_ColorSpace, Mpegv_matrix_coefficients_ColorSpace(Nclc.matrix_coefficients), Unlimited, true, true);
            if (Nclc.HasFlags)
                Fill(Stream_Video, StreamPos_Last, Video_colour_range, Nclc.full_range_flag?"Full":"Limited");
        }

        //if (Temp->second.stsz_StreamSize)
        //    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), Temp->second.stsz_StreamSize);

        //flags and references
        if (StreamKind_Last != Stream_Menu)
        {
            Ztring TempString;
            std::vector<int32u>::iterator TrackID;
            if (StreamKind_Last == Stream_Text)
            {
                std::vector<int32u> TrackIDs = Temp->second.SubtitleFor;
                for (TrackID = Temp->second.Forced.begin(); TrackID != Temp->second.Forced.end(); ++TrackID)
                {
                    for (std::vector<int32u>::iterator TrackID2 = Streams[*TrackID].SubtitleFor.begin(); TrackID2 != Streams[*TrackID].SubtitleFor.end(); ++TrackID2)
                    {
                        TrackIDs.push_back(*TrackID2);
                    }
                }
                for (TrackID = Temp->second.ForcedFor.begin(); TrackID != Temp->second.ForcedFor.end(); ++TrackID)
                {
                    for (std::vector<int32u>::iterator TrackID2 = Streams[*TrackID].SubtitleFor.begin(); TrackID2 != Streams[*TrackID].SubtitleFor.end(); ++TrackID2)
                    {
                        TrackIDs.push_back(*TrackID2);
                    }
                }
                sort (TrackIDs.begin(), TrackIDs.end());
                TrackIDs.erase( unique(TrackIDs.begin(), TrackIDs.end()), TrackIDs.end());
                for (std::vector<int32u>::iterator TrackID = TrackIDs.begin(); TrackID != TrackIDs.end(); ++TrackID)
                {
                    if (TempString.size()) TempString.append(__T(","));
                    TempString.append(Ztring().From_Number(*TrackID));
                }
                if (TempString.size())
                {
                    Fill(StreamKind_Last, StreamPos_Last, "Subtitles For", TempString.To_UTF8().c_str());
                    TempString.clear();
                }
            }
            if (Temp->second.IsExcluded)
                Fill(StreamKind_Last, StreamPos_Last, "Default", "Excluded");
            else
            {
                if (Temp->second.IsEnabled)
                {
                    if (!Retrieve(StreamKind_Last, StreamPos_Last, "AlternateGroup").empty()) // Redundant information if there is not alternate group
                        Fill(StreamKind_Last, StreamPos_Last, "Default", "Yes");
                }
                else
                {
                    for (TrackID = Temp->second.FallBackTo.begin(); TrackID != Temp->second.FallBackTo.end(); ++TrackID)
                    {
                        if (Streams[*TrackID].IsEnabled)
                        {
                            if (TempString.size()) TempString.append(__T(" ,"));
                            else TempString=__T("Inherited From: ");
                            TempString.append(Ztring().From_Number(*TrackID));
                        }
                    }
                    for (TrackID = Temp->second.FallBackFrom.begin(); TrackID != Temp->second.FallBackFrom.end(); ++TrackID)
                    {
                        if (Streams[*TrackID].IsEnabled)
                        {
                            if (TempString.size()) TempString.append(__T(" ,"));
                            else TempString=__T("Inherited From: ");
                            TempString.append(Ztring().From_Number(*TrackID));
                        }
                    }
                    Fill(StreamKind_Last, StreamPos_Last, "Default", TempString.size()?TempString.To_UTF8().c_str():"No");
                    TempString.clear();
                }
            }
            if (StreamKind_Last == Stream_Text)
            {
                if (Temp->second.HasForcedSamples)
                {
                    if (Temp->second.AllForcedSamples)
                    {
                        if (Retrieve_Const(StreamKind_Last, StreamPos_Last, "Forced")!=__T("Yes"))
                            Fill(StreamKind_Last, StreamPos_Last, "Forced", "Yes");
                    }
                    else
                        Fill(StreamKind_Last, StreamPos_Last, "Forced", "Mixed");
                    if (Temp->second.ForcedFor.size())
                    {
                        for (TrackID = Temp->second.ForcedFor.begin(); TrackID != Temp->second.ForcedFor.end(); ++TrackID)
                        {
                            if (TempString.size()) TempString.append(__T(","));
                            TempString.append(Ztring().From_Number(*TrackID));
                        }
                        Fill(StreamKind_Last, StreamPos_Last, "Full Alternative", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
                }
                else
                    Fill(StreamKind_Last, StreamPos_Last, "Forced", "No");
                if (Temp->second.Forced.size())
                {
                    for (TrackID = Temp->second.Forced.begin(); TrackID != Temp->second.Forced.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size()) Fill(StreamKind_Last, StreamPos_Last, "Forced Alternative", TempString.To_UTF8().c_str());
                    TempString.clear();
                }
            }
            if (StreamKind_Last == Stream_Audio)
            {
                if (Temp->second.FallBackTo.size())
                {
                    for (TrackID = Temp->second.FallBackTo.begin(); TrackID != Temp->second.FallBackTo.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size())
                    {
                        Fill(StreamKind_Last, StreamPos_Last, "Fallback To", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
                }
                if (Temp->second.FallBackFrom.size())
                {
                    for (TrackID = Temp->second.FallBackFrom.begin(); TrackID != Temp->second.FallBackFrom.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size())
                    {
                        Fill(StreamKind_Last, StreamPos_Last, "Fallback From", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
                }
                if (Temp->second.Subtitle.size())
                {
                    std::vector<int32u> TrackIDs = Temp->second.Subtitle;
                    for (TrackID = Temp->second.Subtitle.begin(); TrackID != Temp->second.Subtitle.end(); ++TrackID)
                    {
                        std::vector<int32u>::iterator TrackID2;
                        for (TrackID2 = Streams[*TrackID].Forced.begin(); TrackID2 != Streams[*TrackID].Forced.end(); ++TrackID2)
                        {
                            TrackIDs.push_back(*TrackID2);
                        }
                        for (TrackID2 = Streams[*TrackID].ForcedFor.begin(); TrackID2 != Streams[*TrackID].ForcedFor.end(); ++TrackID2)
                        {
                            TrackIDs.push_back(*TrackID2);
                        }
                    }
                    sort (TrackIDs.begin(), TrackIDs.end());
                    TrackIDs.erase( unique(TrackIDs.begin(), TrackIDs.end()), TrackIDs.end());
                    for (std::vector<int32u>::iterator TrackID = TrackIDs.begin(); TrackID != TrackIDs.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size())
                    {
                        Fill(StreamKind_Last, StreamPos_Last, "Subtitles", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
                }
            }
            if (Temp->second.CC.size())
            {
                    for (TrackID = Temp->second.CC.begin(); TrackID != Temp->second.CC.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size())
                    {
                        Fill(StreamKind_Last, StreamPos_Last, "Closed Captions", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
            }
            else if (Temp->second.CCFor.size())
            {
                    for (TrackID = Temp->second.CCFor.begin(); TrackID != Temp->second.CCFor.end(); ++TrackID)
                    {
                        if (TempString.size()) TempString.append(__T(","));
                        TempString.append(Ztring().From_Number(*TrackID));
                    }
                    if (TempString.size())
                    {
                        Fill(StreamKind_Last, StreamPos_Last, "Closed Captions For", TempString.To_UTF8().c_str());
                        TempString.clear();
                    }
            }
            if (Temp->second.Chapters.size())
            {
                for (TrackID = Temp->second.Chapters.begin(); TrackID != Temp->second.Chapters.end(); ++TrackID)
                {
                    if (TempString.size()) TempString.append(__T(","));
                    TempString.append(Ztring().From_Number(*TrackID));
                }
                if (TempString.size())
                {
                    Fill(StreamKind_Last, StreamPos_Last, "Menus", TempString.To_UTF8().c_str());
                    TempString.clear();
                }
            }
            if (!Temp->second.Meta.empty())
            {
                for (TrackID = Temp->second.Meta.begin(); TrackID != Temp->second.Meta.end(); ++TrackID)
                {
                    if (Streams[*TrackID].Parsers.size()==1)
                        Merge(*Streams[*TrackID].Parsers[0], Stream_Video, 0, StreamPos_Last);
                    if (TempString.size()) TempString.append(__T(","));
                    TempString.append(Ztring().From_Number(*TrackID));
                }
                if (TempString.size())
                {
                    Fill(StreamKind_Last, StreamPos_Last, "Metas", TempString.To_UTF8().c_str());
                    TempString.clear();
                }
            }
            for (std::map<std::string, std::vector<int32u> >::iterator InfoList=Temp->second.Infos_List.begin(); InfoList!=Temp->second.Infos_List.end(); ++InfoList)
            {
                for (TrackID=InfoList->second.begin(); TrackID!=InfoList->second.end(); ++TrackID)
                {
                    if (!TempString.empty())
                        TempString.append(__T(","));
                    TempString.append(Ztring().From_Number(*TrackID));
                }
                if (!TempString.empty())
                {
                    Fill(StreamKind_Last, StreamPos_Last, InfoList->first.c_str(), TempString.To_UTF8().c_str());
                    TempString.clear();
                }
            }
        }

        if (Temp->second.ChaptersFor.size())
        {
            Ztring TempString;
            std::vector<int32u>::iterator TrackID;
            for (TrackID = Temp->second.ChaptersFor.begin(); TrackID != Temp->second.ChaptersFor.end(); ++TrackID)
            {
                if (TempString.size()) TempString.append(__T(","));
                TempString.append(Ztring().From_Number(*TrackID));
            }
            if (TempString.size())
            {
                Fill(StreamKind_Last, StreamPos_Last, "Menu For", TempString.To_UTF8().c_str());
                TempString.clear();
            }
        }

        //Edit lists coherencies
        if (Temp->second.edts.size()>1 && Temp->second.edts[0].Duration==Temp->second.tkhd_Duration)
        {
            bool Duplicates=true;
            for (size_t Pos=1; Pos<Temp->second.edts.size(); Pos++)
                if (Temp->second.edts[Pos-1].Delay!=Temp->second.edts[Pos].Delay || Temp->second.edts[Pos-1].Duration!=Temp->second.edts[Pos].Duration || Temp->second.edts[Pos-1].Rate!=Temp->second.edts[Pos].Rate)
                    Duplicates=false;
            if (Duplicates)
                Temp->second.edts.resize(1);
        }

        //Fragments
        if (IsFragmented)
        {
            if (Temp->second.mdhd_TimeScale)
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), Temp->second.stts_Duration/((float)Temp->second.mdhd_TimeScale)*1000, 0, true);
            Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount), Temp->second.stts_FrameCount, 10, true);
            if (Temp->second.StreamKind==Stream_Text && Retrieve_Const(Stream_Text, StreamPos_Last, Text_Format)==__T("Timed Text"))
                Fill(Stream_Text, StreamPos_Last, Text_Events_Total, Temp->second.stsz_MoreThan2_Count, 10, true);
        }

        //Duration/StreamSize
        if (moov_mvhd_TimeScale && Temp->second.TimeCode==NULL && Temp->second.mdhd_TimeScale)
        {
            Ztring Duration_stts_FirstFrame, Duration_stts_LastFrame;
            if (Temp->second.stts_Duration_FirstFrame)
                Duration_stts_FirstFrame.From_Number(((float32)(((int32s)Temp->second.stts_Duration_FirstFrame)-((int32s)Temp->second.stts[1].SampleDuration)))*1000/Temp->second.mdhd_TimeScale, 0); //The duration of the frame minus 1 normal frame duration
            if (Temp->second.stts_Duration_LastFrame)
                Duration_stts_LastFrame.From_Number(((float32)(((int32s)Temp->second.stts_Duration_LastFrame)-((int32s)Temp->second.stts[Temp->second.stts_Duration_FirstFrame?1:0].SampleDuration)))*1000/Temp->second.mdhd_TimeScale, 0); //The duration of the frame minus 1 normal frame duration

            float32 Duration_tkhd_H=((float32)(Temp->second.tkhd_Duration+1))/moov_mvhd_TimeScale;
            float32 Duration_tkhd_L=((float32)(Temp->second.tkhd_Duration-1))/moov_mvhd_TimeScale;
            float32 Duration_stts=((float32)Temp->second.stts_Duration)/Temp->second.mdhd_TimeScale;
            if (!IsFragmented && Duration_stts && !(Duration_stts>=Duration_tkhd_L && Duration_stts<=Duration_tkhd_H))
            {
                //There is a difference between media/stts atom and track atom
                Fill(StreamKind_Last, StreamPos_Last, "Source_Duration", Duration_stts*1000, 0);
                Fill(StreamKind_Last, StreamPos_Last, "Source_Duration_FirstFrame", Duration_stts_FirstFrame);
                Fill(StreamKind_Last, StreamPos_Last, "Source_Duration_LastFrame", Duration_stts_LastFrame);
                if ((Temp->second.stts.size()!=1 || Temp->second.mdhd_TimeScale<100 || Temp->second.stts[0].SampleDuration!=1) && !Temp->second.IsPcm)
                    if (Temp->second.stts_FrameCount)
                        Fill(StreamKind_Last, StreamPos_Last, "Source_FrameCount", Temp->second.stts_FrameCount);
                if (Temp->second.stsz_StreamSize)
                    Fill(StreamKind_Last, StreamPos_Last, "Source_StreamSize", Temp->second.stsz_StreamSize);

                //Calculating new properties based on track duration
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), ((float32)Temp->second.tkhd_Duration)/moov_mvhd_TimeScale*1000, 0, true);
                //Fill(StreamKind_Last, StreamPos_Last, "Duration_FirstFrame", Duration_stts_FirstFrame);
                Clear(StreamKind_Last, StreamPos_Last, "Duration_LastFrame"); //TODO

                int64u FrameCount;
                if (Temp->second.stts_Min && Temp->second.stts_Min==Temp->second.stts_Max)
                    FrameCount=float64_int64s(((float64)Temp->second.tkhd_Duration)/moov_mvhd_TimeScale*Temp->second.mdhd_TimeScale/Temp->second.stts_Min);
                else
                {
                    FrameCount=0;
                    int64u Ticks_Max=float64_int64s(((float64)Temp->second.tkhd_Duration)/moov_mvhd_TimeScale*Temp->second.mdhd_TimeScale);
                    int64u Ticks=0;
                    for (size_t stts_Pos=0; stts_Pos<Temp->second.stts.size(); stts_Pos++)
                    {
                        int64u Ticks_Complete = (int64u)Temp->second.stts[stts_Pos].SampleCount * (int64u)Temp->second.stts[stts_Pos].SampleDuration;
                        if (Ticks+Ticks_Complete>=Ticks_Max)
                        {
                            if (Temp->second.stts[stts_Pos].SampleDuration)
                                FrameCount+=float64_int64s(((float64)(Ticks_Max-Ticks))/Temp->second.stts[stts_Pos].SampleDuration);
                            break;
                        }
                        Ticks+=Ticks_Complete;
                        FrameCount+=Temp->second.stts[stts_Pos].SampleCount;
                    }
                }
                if ((Temp->second.stts.size()!=1 || Temp->second.mdhd_TimeScale<100 || Temp->second.stts[0].SampleDuration!=1) && !Temp->second.IsPcm)
                    Fill(StreamKind_Last, StreamPos_Last, "FrameCount", FrameCount, 10, true);

                if (Temp->second.stsz_Total.empty())
                    Fill(StreamKind_Last, StreamPos_Last, "StreamSize", FrameCount*Temp->second.stsz_Sample_Size*Temp->second.stsz_Sample_Multiplier);
                else if (FrameCount<=Temp->second.stsz_Total.size())
                {
                    int64u StreamSize=0;
                    for (size_t stsz_Pos=0; stsz_Pos<FrameCount; stsz_Pos++)
                        StreamSize+=Temp->second.stsz_Total[stsz_Pos];
                    bool HasEncodedBitRate=!Retrieve(StreamKind_Last, StreamPos_Last, "BitRate_Encoded").empty();
                    Fill(StreamKind_Last, StreamPos_Last, HasEncodedBitRate?"StreamSize_Encoded":"StreamSize", StreamSize);
                }
            }
            else
            {
                //Normal
                if (Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration)).empty())
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Duration), Duration_stts*1000, 0);
                Fill(StreamKind_Last, StreamPos_Last, "Duration_FirstFrame", Duration_stts_FirstFrame);
                Fill(StreamKind_Last, StreamPos_Last, "Duration_LastFrame", Duration_stts_LastFrame);
                if ((Temp->second.stts.size()!=1 || Temp->second.mdhd_TimeScale<100 || Temp->second.stts[0].SampleDuration!=1) && !Temp->second.IsPcm)
                    if (Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount)).empty() && Temp->second.stts_FrameCount)
                        Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_FrameCount), Temp->second.stts_FrameCount);
                bool HasPadding=(Temp->second.Parsers.size()==1 && !Temp->second.Parsers[0]->Retrieve(StreamKind_Last, StreamPos_Last, "BitRate_Encoded").empty()) || (Temp->second.Parsers.size()==1 && Temp->second.Parsers[0]->Buffer_TotalBytes && ((float32)Temp->second.Parsers[0]->Buffer_PaddingBytes)/Temp->second.Parsers[0]->Buffer_TotalBytes>0.02);
                if (Temp->second.stsz_StreamSize)
                    Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, HasPadding?Generic_StreamSize_Encoded:Generic_StreamSize), Temp->second.stsz_StreamSize);
            }
        }

        //Edit Lists
        float64 Delay=0;
        switch (Temp->second.edts.size())
        {
            case 0 :
                    break;
            case 1 :
                    if (Temp->second.edts[0].Duration==Temp->second.tkhd_Duration && Temp->second.edts[0].Rate==0x00010000 && moov_mvhd_TimeScale)
                    {
                        Delay=Temp->second.edts[0].Delay;
                        Delay=-Delay;
                        if (Temp->second.mdhd_TimeScale)
                            Delay/=Temp->second.mdhd_TimeScale; //In seconds
                    }
                    break;
            case 2 :
                    if (Temp->second.edts[0].Delay==(int32u)-1 && Temp->second.edts[0].Duration+Temp->second.edts[1].Duration==Temp->second.tkhd_Duration && Temp->second.edts[0].Rate==0x00010000 && Temp->second.edts[1].Rate==0x00010000 && moov_mvhd_TimeScale)
                    {
                        Delay=Temp->second.edts[0].Duration;
                        Temp->second.tkhd_Duration-=float64_int64s(Delay);
                        Delay/=moov_mvhd_TimeScale; //In seconds
                    }
                    break;
            default:
                    break; //TODO: handle more complex Edit Lists
        }
        if (Delay && !Retrieve(StreamKind_Last, StreamPos_Last, "Source_Duration").empty())
        {
            Delay+=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay)).To_float64()/1000; //TODO: use TimeCode value directly instead of the rounded value
            if (!Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source)).empty() && Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source))!=__T("Container"))
            {
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Original), Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay)));
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Original_Source), Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_Delay_Source)));
            }
            Fill(StreamKind_Last, StreamPos_Last, "Source_Delay", Delay*1000, 0, true);
            Fill(StreamKind_Last, StreamPos_Last, "Source_Delay_Source", "Container", Unlimited, true, true);
            Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Source_Delay", "N NT");
            Fill_SetOptions(StreamKind_Last, StreamPos_Last, "Source_Delay_Source", "N NT");
        }

        if (StreamKind_Last==Stream_Video && Temp->second.TimeCode==NULL)
        {
            if (Temp->second.mdhd_TimeScale && Temp->second.stts_Min && Temp->second.stts_Max)
            {
                if (Temp->second.stts_Min==0 || Temp->second.stts_Max==0 || (Temp->second.stts_Min!=Temp->second.stts_Max && ((float)Temp->second.mdhd_TimeScale)/Temp->second.stts_Min-((float)Temp->second.mdhd_TimeScale)/Temp->second.stts_Max>=0.001))
                {
                    if (Temp->second.stts_Max)
                        Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Minimum, ((float)Temp->second.mdhd_TimeScale)/Temp->second.stts_Max, 3, true);
                    if (Temp->second.stts_Min)
                        Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Maximum, ((float)Temp->second.mdhd_TimeScale)/Temp->second.stts_Min, 3, true);
                    if (Temp->second.stts_Duration)
                        Fill(Stream_Video, StreamPos_Last, Video_FrameRate,         ((float)Temp->second.stts_FrameCount)/Temp->second.stts_Duration*Temp->second.mdhd_TimeScale, 3, true);
                    Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Mode,    "VFR", Unlimited, true, true);
                }
                else
                {
                    Fill(Stream_Video, StreamPos_Last, Video_FrameRate,         ((float)Temp->second.mdhd_TimeScale)/Temp->second.stts_Max, 3, true);
                    Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Mode,    "CFR", Unlimited, true, true);
                }
            }
        }

        //Coherency
        if (!IsFragmented && Temp->second.stts_Duration!=Temp->second.mdhd_Duration && Temp->second.mdhd_TimeScale)
        {
            //There is a difference between media/mdhd atom and track atom
            Fill(StreamKind_Last, StreamPos_Last, "mdhd_Duration", ((float32)Temp->second.mdhd_Duration)/Temp->second.mdhd_TimeScale*1000, 0);
        }

        //When there are few frames, difficult to detect PCM
        if (Temp->second.IsPcm && !Temp->second.Parsers.empty() && !Temp->second.Parsers[0]->Status[IsAccepted])
        {
            for (size_t Pos=0; Pos<Temp->second.Parsers.size()-1; Pos++)
                delete Temp->second.Parsers[Pos];
            Temp->second.Parsers.erase(Temp->second.Parsers.begin(), Temp->second.Parsers.begin()+Temp->second.Parsers.size()-1);
            Temp->second.Parsers[0]->Accept();
        }

        //Parser specific
        if (Temp->second.Parsers.size()==1)
        {
            if (Config->ParseSpeed<=1.0 && !Temp->second.Parsers[0]->Status[IsFinished])
            {
                Fill(Temp->second.Parsers[0]);
                Temp->second.Parsers[0]->Open_Buffer_Unsynch();
            }

            //Finalizing and Merging
            Finish(Temp->second.Parsers[0]);
            if (StreamKind_Last==Stream_General)
            {
                //Special case for TimeCode without link
                for (std::map<int32u, stream>::iterator Target=Streams.begin(); Target!=Streams.end(); ++Target)
                    if (Target->second.StreamKind!=Stream_General)
                        Merge(*Temp->second.Parsers[0], Target->second.StreamKind, 0, Target->second.StreamPos);
            }
            else
            {
                //Hacks - Before
                Ztring FrameRate_Temp, FrameRate_Mode_Temp, Duration_Temp, Delay_Temp;
                if (StreamKind_Last==Stream_Video)
                {
                    if (Temp->second.Parsers[0] && Retrieve(Stream_Video, 0, Video_CodecID_Hint)==__T("DVCPRO HD"))
                    {
                        Temp->second.Parsers[0]->Clear(Stream_Video, 0, Video_FrameRate);
                        Temp->second.Parsers[0]->Clear(Stream_Video, 0, Video_Width);
                        Temp->second.Parsers[0]->Clear(Stream_Video, 0, Video_Height);
                        Temp->second.Parsers[0]->Clear(Stream_Video, 0, Video_DisplayAspectRatio);
                        Temp->second.Parsers[0]->Clear(Stream_Video, 0, Video_PixelAspectRatio);
                    }

                    FrameRate_Temp=Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate);
                    FrameRate_Mode_Temp=Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate_Mode);
                    Duration_Temp=Retrieve(Stream_Video, StreamPos_Last, Video_Duration);
                    Delay_Temp=Retrieve(Stream_Video, StreamPos_Last, Video_Delay);

                    //Special case: DV 1080i and MPEG-4 header is lying (saying this is 1920 pixel wide, but this is 1440 pixel wide)
                    if (Temp->second.Parsers[0]->Get(Stream_Video, 0, Video_Format)==__T("DV") && Retrieve(Stream_Video, StreamKind_Last, Video_Width)==__T("1080"))
                        Clear(Stream_Video, StreamKind_Last, Video_Width);
                }

                //Special case - Multiple sub-streams in a stream
                if ((Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format)==__T("ChannelGrouping") && Temp->second.Parsers[0]->Count_Get(Stream_Audio))
                 ||  Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format)==__T("Final Cut EIA-608")
                 ||  Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format)==__T("Final Cut CDP"))
                {
                    //Before
                    int64u StreamSize_Encoded=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize_Encoded)).To_int64u();
                    if (!StreamSize_Encoded)
                        StreamSize_Encoded=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize)).To_int64u();
                    Clear(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize));
                    if (StreamKind_Last==Stream_Audio)
                    {
                        Clear(Stream_Audio, StreamPos_Last, Audio_Format_Settings_Sign);
                    }
                    ZtringList StreamSave;
                    ZtringListList StreamMoreSave;
                    if (StreamKind_Last!=Stream_Max)
                    {
                        StreamSave.Write((*File__Analyze::Stream)[StreamKind_Last][StreamPos_Last].Read());
                        StreamMoreSave.Write((*Stream_More)[StreamKind_Last][StreamPos_Last].Read());
                    }

                    //Erasing former streams data
                    stream_t NewKind=StreamKind_Last;
                    size_t NewPos1;
                    Ztring ID;
                    vector<Ztring> StreamOrders;
                    if (Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format)==__T("ChannelGrouping"))
                    {
                        //Channel coupling, removing the 2 corresponding streams
                        NewPos1=StreamPos_Last-1;
                        ID=Retrieve(StreamKind_Last, NewPos1, General_ID)+__T(" / ")+Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
                        if (StreamSize_Encoded)
                            StreamSize_Encoded+=Retrieve(StreamKind_Last, NewPos1, Fill_Parameter(StreamKind_Last, Generic_StreamSize)).To_int64u();

                        StreamOrders.push_back(Retrieve_Const(StreamKind_Last, NewPos1, General_StreamOrder));
                        StreamOrders.push_back(Retrieve_Const(StreamKind_Last, StreamPos_Last, General_StreamOrder));
                        Stream_Erase(NewKind, StreamPos_Last);
                        Stream_Erase(NewKind, NewPos1);

                        streams::iterator NextStream=Temp;
                        ++NextStream;
                        size_t NewAudio_Count=Temp->second.Parsers[0]->Count_Get(Stream_Audio);
                        while (NextStream!=Streams.end())
                        {
                            if (NextStream->second.StreamKind==Stream_Audio)
                            {
                                NextStream->second.StreamPos-=2;
                                NextStream->second.StreamPos+=NewAudio_Count;
                            }
                            ++NextStream;
                        }
                    }
                    else
                    {
                        //One channel
                        NewPos1=StreamPos_Last;
                        ID=Retrieve(StreamKind_Last, NewPos1, General_ID);
                        Stream_Erase(StreamKind_Last, StreamPos_Last);
                    }

                    //After
                    size_t New_Count=Temp->second.Parsers[0]->Count_Get(NewKind);
                    for (size_t StreamPos=0; StreamPos<New_Count; StreamPos++)
                    {
                        Stream_Prepare(NewKind, NewPos1+StreamPos);
                        for (size_t Pos=0; Pos<StreamSave.size(); Pos++)
                            if (Pos!=General_ID && Retrieve(StreamKind_Last, StreamPos_Last, Pos).empty())
                                Fill(StreamKind_Last, StreamPos_Last, Pos, StreamSave[Pos]);
                        for (size_t Pos=0; Pos<StreamMoreSave.size(); Pos++)
                        {
                            Fill(StreamKind_Last, StreamPos_Last, StreamMoreSave(Pos, 0).To_UTF8().c_str(), StreamMoreSave(Pos, 1));
                            Fill_SetOptions(StreamKind_Last, StreamPos_Last, StreamMoreSave(Pos, 0).To_UTF8().c_str(), StreamMoreSave(Pos, Info_Options).To_UTF8().c_str());
                        }
                        Merge(*Temp->second.Parsers[0], StreamKind_Last, StreamPos, StreamPos_Last);
                        Ztring Parser_ID=ID+__T('-')+Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
                        if (ID.size()+1==Parser_ID.size())
                            Parser_ID.resize(ID.size());
                        if (!StreamOrders.empty())
                        {
                            Clear(StreamKind_Last, StreamPos_Last, General_StreamOrder);
                            for (auto& StreamOrder : StreamOrders)
                                Fill(StreamKind_Last, StreamPos_Last, General_StreamOrder, StreamOrder);
                        }
                        Fill(StreamKind_Last, StreamPos_Last, General_ID, Parser_ID, true);
                        if (StreamPos)
                        {
                            if (StreamSize_Encoded)
                                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize_Encoded), 0, 10, true);
                        }
                        else
                        {
                            if (StreamSize_Encoded)
                                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize_Encoded), StreamSize_Encoded, 10, true);
                        }
                    }
                    Ztring LawRating=Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_LawRating);
                    if (!LawRating.empty())
                        Fill(Stream_General, 0, General_LawRating, LawRating, true);
                    Ztring Title=Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Title);
                    if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                        Fill(Stream_General, 0, General_Title, Title);
                }
                else
                {
                    //Temp->second.Parsers[0]->Clear(StreamKind_Last, StreamPos_Last, "Delay"); //DV TimeCode is removed
                    size_t New_Count=Temp->second.Parsers[0]->Count_Get(StreamKind_Last);
                    Ztring ID;
                    ZtringList StreamSave;
                    ZtringListList StreamMoreSave;
                    if (New_Count > 1)
                    {
                        if (StreamKind_Last==Stream_Audio)
                        {
                            //Content from underlying format is preferred
                            Clear(Stream_Audio, StreamPos_Last, Audio_Channel_s_);
                            Clear(Stream_Audio, StreamPos_Last, Audio_BitRate);
                            Clear(Stream_Audio, StreamPos_Last, Audio_SamplingRate);
                        }
                        Clear(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize));
                        Clear(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize_Encoded));
                        Clear(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate));
                        Clear(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_BitRate_Encoded));
                        ID = Retrieve(StreamKind_Last, StreamPos_Last, General_ID);
                        if (StreamKind_Last != Stream_Max)
                        {
                            StreamSave.Write((*File__Analyze::Stream)[StreamKind_Last][StreamPos_Last].Read());
                            StreamMoreSave.Write((*Stream_More)[StreamKind_Last][StreamPos_Last].Read());
                        }
                    }
                    for (size_t Parser_StreamPos=0; Parser_StreamPos<New_Count; Parser_StreamPos++)
                    {
                        if (Parser_StreamPos)
                        {
                            //Inserting a new stream (and updating the other streams not yet parsed with the corresponding StreamPos)
                            Stream_Prepare(StreamKind_Last, StreamPos_Last + Parser_StreamPos);

                            streams::iterator NextStream=Temp;
                            ++NextStream;
                            while (NextStream!=Streams.end())
                            {
                                if (NextStream->second.StreamKind== StreamKind_Last)
                                   NextStream->second.StreamPos++;
                                ++NextStream;
                            }
                        }
                        Merge(*Temp->second.Parsers[0], StreamKind_Last, Parser_StreamPos, StreamPos_Last);
                        Ztring Parser_ID=Temp->second.Parsers[0]->Retrieve(StreamKind_Last, Parser_StreamPos, General_ID);
                        if (!Parser_ID.empty())
                        {
                            if (Parser_ID.find(__T(" / "))!=string::npos)
                            {
                                Parser_ID.insert(0, 1, __T('('));
                                size_t End=Parser_ID.find(__T('-'));
                                if (End==string::npos)
                                    End=Parser_ID.size();
                                Parser_ID.insert(End, 1, __T(')'));
                            }
                            Fill(StreamKind_Last, StreamPos_Last, General_ID, ID + __T('-') + Parser_ID, true);
                        }
                        if (Parser_StreamPos)
                        {
                            for (size_t Pos=0; Pos<StreamSave.size(); Pos++)
                            {
                                if (StreamKind_Last==Stream_Audio && Pos==Audio_Channel_s_)
                                    continue;
                                if (Retrieve(StreamKind_Last, StreamPos_Last, Pos).empty())
                                    Fill(StreamKind_Last, StreamPos_Last, Pos, StreamSave[Pos]);
                            }
                            for (size_t Pos=0; Pos<StreamMoreSave.size(); Pos++)
                                Fill(StreamKind_Last, StreamPos_Last, StreamMoreSave(Pos, 0).To_UTF8().c_str(), StreamMoreSave(Pos, 1));
                        }
                    }

                    //Law rating
                    Ztring LawRating=Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_LawRating);
                    if (!LawRating.empty())
                        Fill(Stream_General, 0, General_LawRating, LawRating, true);
                    Ztring Title=Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Title);
                    if (!Title.empty() && Retrieve(Stream_General, 0, General_Title).empty())
                        Fill(Stream_General, 0, General_Title, Title);
                    Ztring Recorded_Date=Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Recorded_Date);
                    if (!Recorded_Date.empty() && Retrieve(Stream_General, 0, General_Recorded_Date).empty())
                        Fill(Stream_General, 0, General_Recorded_Date, Recorded_Date); //TODO: something more generic (which General field from substream should be used for the 
                }

                //Hacks - After
                if (StreamKind_Last==Stream_Video)
                {
                    Fill(Stream_Video, StreamPos_Last, Video_Duration, Duration_Temp, true);
                    if (!FrameRate_Temp.empty() && FrameRate_Temp!=Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate))
                        Fill(Stream_Video, StreamPos_Last, Video_FrameRate, FrameRate_Temp, true);
                    if (!FrameRate_Mode_Temp.empty() && FrameRate_Mode_Temp!=Retrieve(Stream_Video, StreamPos_Last, Video_FrameRate_Mode))
                        Fill(Stream_Video, StreamPos_Last, Video_FrameRate_Mode, FrameRate_Mode_Temp, true);

                    //Special case for TimeCode and DV multiple audio
                    if (!Delay_Temp.empty() && Delay_Temp!=Retrieve(Stream_Video, StreamPos_Last, Video_Delay))
                    {
                        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
                            if (Retrieve(Stream_Audio, Pos, "MuxingMode_MoreInfo")==__T("Muxed in Video #1"))
                            {
                                //Fill(Stream_Audio, Pos, Audio_Delay_Original, Retrieve(Stream_Audio, Pos, Audio_Delay));
                                Fill(Stream_Audio, Pos, Audio_Delay, Retrieve(Stream_Video, StreamPos_Last, Video_Delay), true);
                                Fill(Stream_Audio, Pos, Audio_Delay_Settings, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Settings), true);
                                Fill(Stream_Audio, Pos, Audio_Delay_Source, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Source), true);
                                Fill(Stream_Audio, Pos, Audio_Delay_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original), true);
                                Fill(Stream_Audio, Pos, Audio_Delay_Original_Settings, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original_Settings), true);
                                Fill(Stream_Audio, Pos, Audio_Delay_Original_Source, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original_Source), true);
                            }
                        for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
                            if (Retrieve(Stream_Text, Pos, "MuxingMode_MoreInfo")==__T("Muxed in Video #1"))
                            {
                                //Fill(Stream_Text, Pos, Text_Delay_Original, Retrieve(Stream_Text, Pos, Text_Delay));
                                Fill(Stream_Text, Pos, Text_Delay, Retrieve(Stream_Video, StreamPos_Last, Video_Delay), true);
                                Fill(Stream_Text, Pos, Text_Delay_Settings, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Settings), true);
                                Fill(Stream_Text, Pos, Text_Delay_Source, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Source), true);
                                Fill(Stream_Text, Pos, Text_Delay_Original, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original), true);
                                Fill(Stream_Text, Pos, Text_Delay_Original_Settings, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original_Settings), true);
                                Fill(Stream_Text, Pos, Text_Delay_Original_Source, Retrieve(Stream_Video, StreamPos_Last, Video_Delay_Original_Source), true);
                            }
                    }
                }

                //Special case: AAC
                if (StreamKind_Last==Stream_Audio
                 && (Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("AAC")
                  || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("MPEG Audio")
                  || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("Vorbis")))
                    Clear(Stream_Audio, StreamPos_Last, Audio_BitDepth); //Resolution is not valid for AAC / MPEG Audio / Vorbis

                //Special case: DV with Audio or/and Text in the video stream
                if (StreamKind_Last==Stream_Video && Temp->second.Parsers[0] && (Temp->second.Parsers[0]->Count_Get(Stream_Audio) || Temp->second.Parsers[0]->Count_Get(Stream_Text)))
                {
                    //Video and Audio are together
                    size_t Audio_Count=Temp->second.Parsers[0]->Count_Get(Stream_Audio);
                    for (size_t Audio_Pos=0; Audio_Pos<Audio_Count; Audio_Pos++)
                    {
                        Fill_Flush();
                        Stream_Prepare(Stream_Audio);
                        size_t Pos=Count_Get(Stream_Audio)-1;
                        Merge(*Temp->second.Parsers[0], Stream_Audio, Audio_Pos, StreamPos_Last);
                        Fill(Stream_Audio, Pos, Audio_MuxingMode, Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format));
                        Fill(Stream_Audio, Pos, Audio_MuxingMode_MoreInfo, __T("Muxed in Video #")+Ztring().From_Number(Temp->second.StreamPos+1));
                        Fill(Stream_Audio, Pos, Audio_Duration, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Duration));
                        Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, 0); //Included in the DV stream size
                        Ztring ID=Retrieve(Stream_Audio, Pos, Audio_ID);
                        Fill(Stream_Audio, Pos, Audio_ID, Retrieve(Stream_Video, Temp->second.StreamPos, Video_ID)+__T("-")+ID, true);
                        Fill(Stream_Audio, Pos, "Delay", Retrieve(Stream_Video, Temp->second.StreamPos, "Delay"), true);
                        Fill(Stream_Audio, Pos, "Delay_DropFrame", Retrieve(Stream_Video, Temp->second.StreamPos, "Delay_DropFrame"), true);
                        Fill(Stream_Audio, Pos, "Delay_Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Delay_Source"), true);
                        Fill(Stream_Audio, Pos, "Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Source"));
                        Fill(Stream_Audio, Pos, "Source_Info", Retrieve(Stream_Video, Temp->second.StreamPos, "Source_Info"));
                    }

                    //Video and Text are together
                    size_t Text_Count=Temp->second.Parsers[0]->Count_Get(Stream_Text);
                    for (size_t Text_Pos=0; Text_Pos<Text_Count; Text_Pos++)
                    {
                        Fill_Flush();
                        Stream_Prepare(Stream_Text);
                        size_t Pos=Count_Get(Stream_Text)-1;
                        Merge(*Temp->second.Parsers[0], Stream_Text, Text_Pos, StreamPos_Last);
                        Fill(Stream_Text, Pos, Text_MuxingMode, Temp->second.Parsers[0]->Retrieve(Stream_General, 0, General_Format));
                        Fill(Stream_Text, Pos, Text_MuxingMode_MoreInfo, __T("Muxed in Video #")+Ztring().From_Number(Temp->second.StreamPos+1));
                        Fill(Stream_Text, Pos, Text_Duration, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Duration));
                        Fill(Stream_Text, Pos, Text_StreamSize_Encoded, 0); //Included in the DV stream size
                        Ztring ID=Retrieve(Stream_Text, Pos, Text_ID);
                        Fill(Stream_Text, Pos, Text_ID, Retrieve(Stream_Video, Temp->second.StreamPos, Video_ID)+__T("-")+ID, true);
                        Fill(Stream_Text, Pos, "Delay_Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Delay_Source"), true);
                        Fill(Stream_Text, Pos, "Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Source"));
                        Fill(Stream_Text, Pos, "Source_Info", Retrieve(Stream_Video, Temp->second.StreamPos, "Source_Info"));
                        Fill(Stream_Text, Pos, "Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Source"));
                        Fill(Stream_Text, Pos, "Source_Info", Retrieve(Stream_Video, Temp->second.StreamPos, "Source_Info"));
                    }

                    StreamKind_Last=Temp->second.StreamKind;
                    StreamPos_Last=Temp->second.StreamPos;
                }
            }
        }

        //ScanOrder_StoredDisplayedInverted
        // Priorizing https://developer.apple.com/library/mac/technotes/tn2162/_index.html#//apple_ref/doc/uid/DTS40013070-CH1-TNTAG10-THE__FIEL__IMAGEDESCRIPTION_EXTENSION__FIELD_FRAME_INFORMATION
        /*
        switch (Temp->second.fiel_detail)
        {
            case  1  :  // Separated fields, TFF
            case  6 :   // Separated fields, BFF
                        Fill(Stream_Video, StreamPos_Last, Video_ScanOrder, "TFF", Unlimited, true, true);
                        break;
            case  9  :  // Interleaved fields, TFF
            case 14 :   // Interleaved fields, BFF
                        Fill(Stream_Video, StreamPos_Last, Video_ScanOrder, "BFF", Unlimited, true, true);
                        break;
            default  :  ;
        }
        */

        //External file name specific
        if (Temp->second.MI && Temp->second.MI->Info)
        {
            //Preparing
            StreamKind_Last=Temp->second.StreamKind;
            StreamPos_Last=Temp->second.StreamPos;

            //Hacks - Before
            Ztring CodecID=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID));
            const Ztring Source=Retrieve(StreamKind_Last, StreamPos_Last, "Source");

            Merge(*Temp->second.MI->Info, Temp->second.StreamKind, 0, Temp->second.StreamPos);
            File_Size_Total+=Ztring(Temp->second.MI->Get(Stream_General, 0, General_FileSize)).To_int64u();

            //Hacks - After
            if (CodecID!=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID)))
            {
                if (!CodecID.empty())
                    CodecID+=__T(" / ");
                CodecID+=Retrieve(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID));
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_CodecID), CodecID, true);
            }
            if (Source!=Retrieve(StreamKind_Last, StreamPos_Last, "Source"))
            {
                const Ztring Source_Original=Retrieve(StreamKind_Last, StreamPos_Last, "Source");
                const Ztring Source_Original_Info=Retrieve(StreamKind_Last, StreamPos_Last, "Source_Info");
                const Ztring Source_Info = Retrieve(StreamKind_Last, StreamPos_Last, "Source_Info");
                Fill(StreamKind_Last, StreamPos_Last, "Source", Source, true);
                Fill(StreamKind_Last, StreamPos_Last, "Source_Info", Source_Info, true);
                Fill(StreamKind_Last, StreamPos_Last, "Source_Original", Source_Original, true);
                Fill(StreamKind_Last, StreamPos_Last, "Source_Original_Info", Source_Original_Info, true);
            }
            if (StreamKind_Last==Stream_Audio
             && Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)!=__T("PCM")
             && !Retrieve(Stream_Audio, StreamPos_Last, Audio_Channel_s__Original).empty())
            {
                Clear(Stream_Audio, StreamPos_Last, Audio_Channel_s__Original);
                Fill(Stream_Audio, StreamPos_Last, Audio_Channel_s_, 6, 10, true); //The PCM channel count is fake
            }

            //Muxing Mode
            Fill(StreamKind_Last, StreamPos_Last, "MuxingMode", Temp->second.MI->Get(Stream_General, 0, General_Format));

            //Special case: DV with Audio or/and Text in the video stream
            if (StreamKind_Last==Stream_Video && Temp->second.MI->Info && (Temp->second.MI->Info->Count_Get(Stream_Audio) || Temp->second.MI->Info->Count_Get(Stream_Text)))
            {
                //Video and Audio are together
                size_t Audio_Count=Temp->second.MI->Info->Count_Get(Stream_Audio);
                for (size_t Audio_Pos=0; Audio_Pos<Audio_Count; Audio_Pos++)
                {
                    Fill_Flush();
                    Stream_Prepare(Stream_Audio);
                    size_t Pos=Count_Get(Stream_Audio)-1;
                    Merge(*Temp->second.MI->Info, Stream_Audio, Audio_Pos, StreamPos_Last);
                    if (Retrieve(Stream_Audio, Pos, Audio_MuxingMode).empty())
                        Fill(Stream_Audio, Pos, Audio_MuxingMode, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Format), true);
                    else
                        Fill(Stream_Audio, Pos, Audio_MuxingMode, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Format)+__T(" / ")+Retrieve(Stream_Audio, Pos, Audio_MuxingMode), true);
                    Fill(Stream_Audio, Pos, Audio_MuxingMode_MoreInfo, __T("Muxed in Video #")+Ztring().From_Number(Temp->second.StreamPos+1));
                    Fill(Stream_Audio, Pos, Audio_Duration, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Duration), true);
                    Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, 0); //Included in the DV stream size
                    Ztring ID=Retrieve(Stream_Audio, Pos, Audio_ID);
                    Fill(Stream_Audio, Pos, Audio_ID, Retrieve(Stream_Video, Temp->second.StreamPos, Video_ID)+__T("-")+ID, true);
                    Fill(Stream_Audio, Pos, "Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Source"));
                    Fill(Stream_Audio, Pos, "Source_Info", Retrieve(Stream_Video, Temp->second.StreamPos, "Source_Info"));
                }

                //Video and Text are together
                size_t Text_Count=Temp->second.MI->Info->Count_Get(Stream_Text);
                for (size_t Text_Pos=0; Text_Pos<Text_Count; Text_Pos++)
                {
                    Fill_Flush();
                    Stream_Prepare(Stream_Text);
                    size_t Pos=Count_Get(Stream_Text)-1;
                    Merge(*Temp->second.MI->Info, Stream_Text, Text_Pos, StreamPos_Last);
                    if (Retrieve(Stream_Text, Pos, Text_MuxingMode).empty())
                        Fill(Stream_Text, Pos, Text_MuxingMode, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Format), true);
                    else
                        Fill(Stream_Text, Pos, Text_MuxingMode, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Format)+__T(" / ")+Retrieve(Stream_Text, Pos, Text_MuxingMode), true);
                    Fill(Stream_Text, Pos, Text_MuxingMode_MoreInfo, __T("Muxed in Video #")+Ztring().From_Number(Temp->second.StreamPos+1));
                    Fill(Stream_Text, Pos, Text_Duration, Retrieve(Stream_Video, Temp->second.StreamPos, Video_Duration));
                    Fill(Stream_Text, Pos, Text_StreamSize_Encoded, 0); //Included in the DV stream size
                    Ztring ID=Retrieve(Stream_Text, Pos, Text_ID);
                    Fill(Stream_Text, Pos, Text_ID, Retrieve(Stream_Video, Temp->second.StreamPos, Video_ID)+__T("-")+ID, true);
                    Fill(Stream_Text, Pos, "Source", Retrieve(Stream_Video, Temp->second.StreamPos, "Source"));
                    Fill(Stream_Text, Pos, "Source_Info", Retrieve(Stream_Video, Temp->second.StreamPos, "Source_Info"));
                }
            }
        }

        //Aperture size
        if (Temp->second.CleanAperture_Width)
        {
            Fill(Stream_Video, StreamPos_Last, "Width_CleanAperture", Temp->second.CleanAperture_Width, 0, true);
            Fill(Stream_Video, StreamPos_Last, "Height_CleanAperture", Temp->second.CleanAperture_Height, 0, true);
            Fill(Stream_Video, StreamPos_Last, "PixelAspectRatio_CleanAperture", Temp->second.CleanAperture_PixelAspectRatio, 3, true);
        }

        //Special case: QuickTime files and Stereo streams, there is a default value in QuickTime player, a QuickTime "standard"?
        if (StreamKind_Last==Stream_Audio
            && Retrieve(Stream_Audio, StreamPos_Last, Audio_Channel_s_)==__T("2")
            && Retrieve(Stream_Audio, StreamPos_Last, Audio_ChannelLayout).empty()
            && Retrieve(Stream_Audio, StreamPos_Last, Audio_ChannelPositions).empty()
            && Retrieve(Stream_General, 0, General_Format_Profile)==__T("QuickTime"))
        {
            Fill(Stream_Audio, StreamPos_Last, Audio_ChannelPositions, Mpeg4_chan(101));
            Fill(Stream_Audio, StreamPos_Last, Audio_ChannelLayout, Mpeg4_chan_Layout(101));
        }

        //Bitrate Mode
        if (StreamKind_Last!=Stream_Menu && Retrieve(StreamKind_Last, StreamPos_Last, "BitRate_Mode").empty())
        {
            if (Temp->second.stss.empty() && Temp->second.stss.size()!=Temp->second.stsz_Total.size() && !IsFragmented)
            {
                int64u Size_Min=(int64u)-1, Size_Max=0;
                for (size_t Pos=0; Pos<Temp->second.stsz_Total.size(); Pos++)
                {
                    if (Temp->second.stsz_Total[Pos]<Size_Min)
                        Size_Min=Temp->second.stsz_Total[Pos];
                    if (Temp->second.stsz_Total[Pos]>Size_Max)
                        Size_Max=Temp->second.stsz_Total[Pos];
                }

                if (Size_Min*(1.005+0.005)<Size_Max)
                    Fill(StreamKind_Last, StreamPos_Last, "BitRate_Mode", "VBR");
                else
                    Fill(StreamKind_Last, StreamPos_Last, "BitRate_Mode", "CBR");
            }
            else
            {
                //TODO: compute Bit rate mode from stsz and stss (in order to compute per GOP instead of per frame)
            }
        }

        for (std::map<string, Ztring>::iterator Info=Temp->second.Infos.begin(); Info!=Temp->second.Infos.end(); ++Info)
            Fill(StreamKind_Last, StreamPos_Last, Info->first.c_str(), Info->second);

        //Codec configuration box
        Ztring CodecConfigurationBoxInfo;
        for (size_t i=0; i<Temp->second.CodecConfigurationBoxInfo.size(); i++)
        {
            if (i)
                CodecConfigurationBoxInfo+=__T('+');
            CodecConfigurationBoxInfo+=Ztring().From_CC4(Temp->second.CodecConfigurationBoxInfo[i]);
        }
        Fill(StreamKind_Last, StreamPos_Last, "CodecConfigurationBox", CodecConfigurationBoxInfo);

        //ES_ID
        for (File_Mpeg4_Descriptors::es_id_infos::iterator ES_ID_Info=ES_ID_Infos.begin(); ES_ID_Info!=ES_ID_Infos.end(); ES_ID_Info++)
        {
            if (ES_ID_Info->first==Temp->first && ES_ID_Info->second.StreamKind==Temp->second.StreamKind)
            {
                if (Retrieve_Const(StreamKind_Last, StreamPos_Last, "Format_Profile").empty())
                    Fill(StreamKind_Last, StreamPos_Last, "Format_Profile", ES_ID_Info->second.ProfileLevelString);
                else if (StreamKind_Last==Stream_Audio && Retrieve_Const(Stream_Audio, StreamPos_Last, Audio_Format).find(__T("AAC"))!=string::npos && Retrieve_Const(Stream_Audio, StreamPos_Last, Audio_Format_Level).empty())
                {
                    //Legacy issue with AAC, "Format_Profile" was used for something else, cheating with "Format_Level" for storing profile+level
                    Fill(Stream_Audio, StreamPos_Last, Audio_Format_Level, ES_ID_Info->second.ProfileLevelString);
                }
            }
        }

        if (Temp->second.IsImage)
        {
            // A video stream is actually an image stream
            stream_t Source_StreamKind=StreamKind_Last;
            size_t Source_StreamPos=StreamPos_Last;
            ToDelete_StreamPos.push_back(StreamPos_Last);
            Stream_Prepare(Stream_Image);
            if (Temp->second.stsz_StreamSize)
                Fill(StreamKind_Last, StreamPos_Last, Fill_Parameter(StreamKind_Last, Generic_StreamSize), Temp->second.stsz_StreamSize, 10, true);
            size_t Size=Count_Get(Source_StreamKind, Source_StreamPos);
            for (size_t i=General_ID; i<Size; i++)
            {
                switch (i)
                {
                    case Video_FrameRate:
                    case Video_Sampled_Width:
                    case Video_Sampled_Height:
                        continue;
                }
                const Ztring& Value=Retrieve_Const(Source_StreamKind, Source_StreamPos, i);
                if (!Value.empty())
                {
                    const Ztring& Name=Retrieve_Const(Source_StreamKind, Source_StreamPos, i, Info_Name);
                    Fill(StreamKind_Last, StreamPos_Last, Name.To_UTF8().c_str(), Value, true);
                    const Ztring& Options=Retrieve_Const(Source_StreamKind, Source_StreamPos, i, Info_Options);
                    Fill_SetOptions(StreamKind_Last, StreamPos_Last, Name.To_UTF8().c_str(), Options.To_UTF8().c_str());
                }
            }
        }

        // Adapting from fake ID
        if (Temp->second.tkhd_Found && Temp->second.TrackID!=Temp->first)
        {
            Ztring CurrentID=Ztring::ToZtring(Temp->first);
            Ztring RealID=Ztring::ToZtring(Temp->second.TrackID);
            ZtringList IDs;
            IDs.Separator_Set(0, "-");
            for (size_t i=Stream_General+1; i<Stream_Max; i++)
            {
                size_t Count=Count_Get((stream_t)i);
                for (size_t j=0; j<Count; j++)
                {
                    IDs.Write(Retrieve_Const((stream_t)i, j, General_ID));
                    if (!IDs.empty() && IDs[0]==CurrentID)
                    {
                        IDs[0]=RealID;
                        Fill((stream_t)i, j, General_ID, IDs.Read(), true);
                        Fill((stream_t)i, j, "ConformanceErrors", "Yes", Unlimited, true, true);
                        Fill((stream_t)i, j, "ConformanceErrors tkhd", "Yes", Unlimited, true, true);
                        Fill((stream_t)i, j, "ConformanceErrors tkhd track_ID", ("track_ID "+RealID.To_UTF8()+" is already used by another track"));
                    }
                }
            }
        }

        ++Temp;
    }
    size_t ToDelete_StreamPos_Size=ToDelete_StreamPos.size();
    if (ToDelete_StreamPos_Size)
    {
        sort(ToDelete_StreamPos.begin(), ToDelete_StreamPos.end());
        for (size_t i=ToDelete_StreamPos.size()-1; i!=(size_t)-1; i--)
            Stream_Erase(Stream_Video, ToDelete_StreamPos[i]);
    }
    if (Vendor!=0x00000000 && Vendor!=0xFFFFFFFF)
    {
        Ztring VendorS=Mpeg4_Encoded_Library(Vendor);
        if (!Vendor_Version.empty())
        {
            VendorS+=__T(' ');
            VendorS+=Vendor_Version;
        }
        Fill(Stream_General, 0, General_Encoded_Library, VendorS);
        Fill(Stream_General, 0, General_Encoded_Library_Name, Mpeg4_Encoded_Library(Vendor));
        Fill(Stream_General, 0, General_Encoded_Library_Version, Vendor_Version);
    }

    if (File_Size_Total!=File_Size)
        Fill(Stream_General, 0, General_FileSize, File_Size_Total, 10, true);
    if (Count_Get(Stream_Video)==0 && Count_Get(Stream_Image)==0 && Count_Get(Stream_Audio)>0)
        Fill(Stream_General, 0, General_InternetMediaType, "audio/mp4", Unlimited, true, true);

    //Parsing sequence files
    #ifdef MEDIAINFO_REFERENCES_YES
        for (streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
            if (!Stream->second.File_Name.empty())
            {
                if (ReferenceFiles==NULL)
                    ReferenceFiles_Accept(this, Config);

                sequence* Sequence=new sequence;
                const Ztring& Format=Retrieve(Stream->second.StreamKind, Stream->second.StreamPos, Fill_Parameter(Stream->second.StreamKind, Generic_Format));
                if (Format==__T("QuickTime TC"))
                {
                    Sequence->Config["File_ForceParser"]=Format;
                    if (Stream->second.Parsers.size()==1)
                    {
                        File_Mpeg4_TimeCode* Parser=(File_Mpeg4_TimeCode*)Stream->second.Parsers[0];
                        ZtringListList Parser_Config;
                        Parser_Config(__T("NumberOfFrames")).From_Number(Parser->NumberOfFrames);
                        Parser_Config(__T("DropFrame")).From_Number(Parser->DropFrame?1:0);
                        Parser_Config(__T("NegativeTimes")).From_Number(Parser->NegativeTimes?1:0);
                        Sequence->Config["File_ForceParser_Config"]=Parser_Config.Read();
                    }
                }
                Sequence->AddFileName(Stream->second.File_Name);
                Sequence->StreamKind=Stream->second.StreamKind;
                Sequence->StreamPos=Stream->second.StreamPos;
                Sequence->StreamID=Retrieve(Stream->second.StreamKind, Stream->second.StreamPos, General_ID).To_int64u();
                if (Stream->second.StreamKind==Stream_Video)
                {
                    Sequence->FrameRate_Set(Retrieve(Stream_Video, Stream->second.StreamPos, Video_FrameRate).To_float64());

                    #if MEDIAINFO_IBIUSAGE
                        for (size_t stss_Pos=0; stss_Pos<Stream->second.stss.size(); stss_Pos++)
                        {
                            int64u Value=Stream->second.stss[stss_Pos];

                            //Searching the corresponding stco
                            std::vector<stream::stsc_struct>::iterator Stsc=Stream->second.stsc.begin();
                            int64u SamplePos=0;
                            for (; Stsc!=Stream->second.stsc.end(); ++Stsc)
                            {
                                std::vector<stream::stsc_struct>::iterator Stsc_Next=Stsc; ++Stsc_Next;
                                int64u CountOfSamples = ((Stsc_Next == Stream->second.stsc.end() ? ((int64u)Stream->second.stco.size()) : ((int64u)Stsc_Next->FirstChunk)) - (int64u)Stsc->FirstChunk) * (int64u)Stsc->SamplesPerChunk;
                                if (Stsc_Next!=Stream->second.stsc.end() && Value>=SamplePos+CountOfSamples)
                                    SamplePos+=CountOfSamples;
                                else
                                {
                                    int64u CountOfChunks=(Value-SamplePos)/Stsc->SamplesPerChunk;
                                    size_t stco_Pos=(size_t)(Stsc->FirstChunk-1+CountOfChunks); //-1 because first chunk is number 1
                                    if (stco_Pos<Stream->second.stco.size())
                                    {
                                        stream::stts_durations::iterator stts_Duration=Stream->second.stts_Durations.begin()+Stream->second.stts_Durations_Pos;
                                        ibi::stream::info IbiInfo;
                                        IbiInfo.StreamOffset=Stream->second.stco[stco_Pos];
                                        IbiInfo.FrameNumber=Value;
                                        IbiInfo.Dts=TimeCode_DtsOffset+(stts_Duration->DTS_Begin+(((int64u)stts_Duration->SampleDuration)*(Value-stts_Duration->Pos_Begin)))*1000000000/Stream->second.mdhd_TimeScale;
                                        Sequence->IbiStream.Add(IbiInfo);
                                    }
                                }
                            }
                        }
                    #endif //MEDIAINFO_IBIUSAGE

                }
                ReferenceFiles->AddSequence(Sequence);
            }

        if (ReferenceFiles)
        {
            ReferenceFiles->ParseReferences();
            #if MEDIAINFO_NEXTPACKET
                if (Config->NextPacket_Get() && ReferenceFiles->Sequences_Size())
                {
                    ReferenceFiles_IsParsing=true;
                    return;
                }
            #endif //MEDIAINFO_NEXTPACKET
        }
    #endif //MEDIAINFO_REFERENCES_YES

    //Commercial names
    Streams_Finish_CommercialNames();
}

//---------------------------------------------------------------------------
void File_Mpeg4::Streams_Finish_CommercialNames()
{
    if (Count_Get(Stream_Video)==1)
    {
        Streams_Finish_StreamOnly();
        if (Retrieve(Stream_Video, 0, Video_Format)==__T("DV") && Retrieve(Stream_Video, 0, Video_Format_Commercial)==__T("DVCPRO HD"))
        {
            int32u BitRate=Retrieve(Stream_Video, 0, Video_BitRate).To_int32u();
            int32u BitRate_Max=Retrieve(Stream_Video, 0, Video_BitRate_Maximum).To_int32u();

            if (BitRate_Max && BitRate>=BitRate_Max)
            {
                Clear(Stream_Video, 0, Video_BitRate_Maximum);
                Fill(Stream_Video, 0, Video_BitRate, BitRate_Max, 10, true);
                Fill(Stream_Video, 0, Video_BitRate_Mode, "CBR", Unlimited, true, true);
            }
        }
             if (!Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny).empty())
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny));
            Fill(Stream_General, 0, General_Format_Commercial, Retrieve(Stream_General, 0, General_Format)+__T(' ')+Retrieve(Stream_Video, 0, Video_Format_Commercial_IfAny));
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("18000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("18000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("18000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM EX 18");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM EX 18");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("25000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("25000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("25000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM EX 25");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM EX 25");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:0") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("35000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("35000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("35000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM EX 35");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM EX 35");
        }
        else if (Retrieve(Stream_Video, 0, Video_Format)==__T("MPEG Video") && Retrieve(Stream_Video, 0, Video_Format_Settings_GOP)!=__T("N=1") && Retrieve(Stream_Video, 0, Video_ChromaSubsampling)==__T("4:2:2") && (Retrieve(Stream_Video, 0, Video_BitRate)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Nominal)==__T("50000000") || Retrieve(Stream_Video, 0, Video_BitRate_Maximum)==__T("50000000")))
        {
            Fill(Stream_General, 0, General_Format_Commercial_IfAny, "XDCAM HD422");
            Fill(Stream_Video, 0, Video_Format_Commercial_IfAny, "XDCAM HD422");
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4::Read_Buffer_Unsynched()
{
    if (!IsSub && MajorBrand==0x6A703220) //"jp2 "
        return Read_Buffer_Unsynched_OneFramePerFile();

    for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
    {
        for (size_t Pos=0; Pos<Stream->second.Parsers.size(); Pos++)
            Stream->second.Parsers[Pos]->Open_Buffer_Unsynch();
    }

    if (mdat_Pos.empty())
    {
        IsParsing_mdat=false;
        return;
    }
    mdat_Pos_Temp=&mdat_Pos[0];
    while (mdat_Pos_Temp!=mdat_Pos_Max && mdat_Pos_Temp->Offset<File_GoTo)
        mdat_Pos_Temp++;
    if (mdat_Pos_Temp!=mdat_Pos_Max && mdat_Pos_Temp->Offset>File_GoTo)
        mdat_Pos_Temp--; //Previous frame
    if (mdat_Pos_Temp==mdat_Pos_Max)
    {
        IsParsing_mdat=false;
        return;
    }
    IsParsing_mdat_Set();

    #if MEDIAINFO_SEEK
        //Searching the ID of the first stream to be demuxed
        std::map<int32u, stream>::iterator Next_Stream=Streams.end();
        #if MEDIAINFO_DEMUX
            size_t Next_Stream_Stco=(size_t)-1;
        #endif //MEDIAINFO_DEMUX
        for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
        {
            for (size_t Stco_Pos=0; Stco_Pos<Stream->second.stco.size(); Stco_Pos++)
                if (Stream->second.stco[Stco_Pos]==mdat_Pos_Temp->Offset)
                {
                    Next_Stream=Stream;
                    #if MEDIAINFO_DEMUX
                        Next_Stream_Stco=Stco_Pos;
                    #endif //MEDIAINFO_DEMUX
                    break;
                }
            if (Next_Stream!=Streams.end())
                break;
        }
    #endif //MEDIAINFO_SEEK

    for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
    {
        #if MEDIAINFO_SEEK && MEDIAINFO_DEMUX
            //Searching the next position for this stream
            int64u StreamOffset=(int64u)-1;
            if (StreamOffset_Jump.empty() || File_GoTo==mdat_Pos[0].Offset)
                StreamOffset=mdat_Pos_Temp->Offset;
            else if (Next_Stream_Stco!=(size_t)-1)
            {
                //Searching the right place for this stream
                int64u StreamOffset_Temp=Next_Stream->second.stco[Next_Stream_Stco];
                std::map<int64u, int64u>::iterator StreamOffset_Jump_Temp;
                for (;;)
                {
                    StreamOffset_Jump_Temp=StreamOffset_Jump.find(StreamOffset_Temp);
                    if (StreamOffset_Jump_Temp==StreamOffset_Jump.end())
                        break;
                    if (Stream==Next_Stream)
                        StreamOffset_Temp=StreamOffset_Jump_Temp->first;
                    else
                    {
                        ++StreamOffset_Jump_Temp;
                        if (StreamOffset_Jump_Temp==StreamOffset_Jump.end())
                            break;
                        StreamOffset_Temp=StreamOffset_Jump_Temp->second;
                    }

                    if (!Stream->second.stco.empty() && StreamOffset_Temp>=Stream->second.stco[0] && StreamOffset_Temp<=Stream->second.stco[Stream->second.stco.size()-1])
                        for (size_t Stco_Pos=0; Stco_Pos<Stream->second.stco.size(); Stco_Pos++)
                            if (StreamOffset_Temp==Stream->second.stco[Stco_Pos])
                            {
                                StreamOffset=Stream->second.stco[Stco_Pos];
                                break;
                            }

                    if (StreamOffset!=(int64u)-1)
                        break;
                }
            }

            if (StreamOffset!=(int64u)-1)
                for (size_t stco_Pos=0; stco_Pos<Stream->second.stco.size(); stco_Pos++)
                    if (Stream->second.stco[stco_Pos]>=StreamOffset)
                    {
                        //Searching the corresponding frame position
                        std::vector<stream::stsc_struct>::iterator Stsc=Stream->second.stsc.begin();
                        int64u SamplePos=0;
                        for (; Stsc!=Stream->second.stsc.end();  ++Stsc)
                        {
                            std::vector<stream::stsc_struct>::iterator Stsc_Next=Stsc; ++Stsc_Next;
                            if (Stsc_Next!=Stream->second.stsc.end() && stco_Pos+1>=Stsc_Next->FirstChunk)
                            {
                                int64u CountOfSamples = ((int64u)Stsc_Next->FirstChunk - (int64u)Stsc->FirstChunk) * (int64u)Stsc->SamplesPerChunk;
                                SamplePos+=CountOfSamples;
                            }
                            else
                            {
                                int64u CountOfSamples = ((int64u)stco_Pos + 1 - (int64u)Stsc->FirstChunk) * (int64u)Stsc->SamplesPerChunk;
                                SamplePos+=CountOfSamples;

                                Stream->second.stts_FramePos=SamplePos;

                                //Searching the corresponding duration block position
                                for (stream::stts_durations::iterator Stts_Duration=Stream->second.stts_Durations.begin(); Stts_Duration!=Stream->second.stts_Durations.end(); ++Stts_Duration)
                                    if (SamplePos>=Stts_Duration->Pos_Begin && SamplePos<Stts_Duration->Pos_End)
                                    {
                                        Stream->second.stts_Durations_Pos=Stts_Duration-Stream->second.stts_Durations.begin();
                                        break;
                                    }

                                break;
                            }
                        }

                        break;
                    }

            if (Stream->second.Demux_Level&(1<<6))
            {
                Stream->second.Demux_Level|= (1<<7); //Add the flag, SPS/PPS must be sent
                Stream->second.Demux_Level&=~(1<<6); //In case of seek again, no need to do it again
            }
        #endif //MEDIAINFO_SEEK && MEDIAINFO_DEMUX
    }
}

//---------------------------------------------------------------------------
#if MEDIAINFO_SEEK
size_t File_Mpeg4::Read_Buffer_Seek (size_t Method, int64u Value, int64u ID)
{
    #if defined(MEDIAINFO_REFERENCES_YES)
        if (ReferenceFiles)
            return ReferenceFiles->Seek(Method, Value, ID);
    #endif //defined(MEDIAINFO_REFERENCES_YES)
    if (!IsSub && MajorBrand==0x6A703220) //"jp2 "
        return Read_Buffer_Seek_OneFramePerFile(Method, Value, ID);

    //Parsing
    switch (Method)
    {
        case 0  :
                    if (Value==0)
                        return Read_Buffer_Seek(3, 0, ID);

                    if (FirstMoovPos==(int64u)-1)
                        return 6;     //Internal error

                    if (Value>=LastMdatPos)
                    {
                        GoTo(File_Size);
                        Open_Buffer_Unsynch();
                        return 1;
                    }

                    {
                        //Looking for the minimal stream offset, for every video/audio/text stream
                        int64u JumpTo=File_Size;
                        for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                            switch (Stream->second.StreamKind)
                            {
                                case Stream_Video :
                                case Stream_Audio :
                                case Stream_Text  :
                                                    {
                                                        //Searching the corresponding chunk offset
                                                        std::vector<int64u>::iterator Stco=Stream->second.stco.begin();
                                                        if (Value<*Stco)
                                                            return Read_Buffer_Seek(3, 0, ID);

                                                        for (; Stco!=Stream->second.stco.end(); ++Stco)
                                                        {
                                                            std::vector<int64u>::iterator Stco_Next=Stco; ++Stco_Next;
                                                            if (Stco_Next!=Stream->second.stco.end() && Value>=*Stco && Value<*Stco_Next)
                                                            {
                                                                if (JumpTo>*Stco)
                                                                    JumpTo=*Stco;
                                                                break;
                                                            }
                                                        }
                                                    }
                                                    break;
                                default           : ;
                            }

                        GoTo(JumpTo);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
        case 1  :
                    if (Value==0)
                        return Read_Buffer_Seek(3, 0, ID);

                    if (FirstMoovPos==(int64u)-1)
                        return 6;     //Internal error

                    return Read_Buffer_Seek(0, FirstMdatPos+(LastMdatPos-FirstMdatPos)*Value/10000, ID);
        case 2  :   //Timestamp
                    #if MEDIAINFO_DEMUX
                    {
                        //Searching time stamp offset due to Time code offset
                        for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                            if (Stream->second.StreamKind==Stream_Video)
                            {
                                if (Value>TimeCode_DtsOffset) //Removing Time Code offset
                                    Value-=TimeCode_DtsOffset;
                                else
                                    Value=0; //Sooner
                                break;
                            }

                        //Looking for the minimal stream offset, for every video/audio/text stream
                        int64u JumpTo=File_Size;
                        for (std::map<int32u, stream>::iterator Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                            switch (Stream->second.StreamKind)
                            {
                                case Stream_Video :
                                case Stream_Audio :
                                case Stream_Text  :
                                                    {
                                                        int64u Value2=float64_int64s(((float64)Value)*Stream->second.mdhd_TimeScale/1000000000); //Transformed in mpeg4 ticks (per track)

                                                        //Searching the corresponding frame
                                                        for (stream::stts_durations::iterator stts_Duration=Stream->second.stts_Durations.begin(); stts_Duration!=Stream->second.stts_Durations.end(); ++stts_Duration)
                                                        {
                                                            if (Value2>=stts_Duration->DTS_Begin && Value2<stts_Duration->DTS_End)
                                                            {
                                                                int64u FrameNumber=stts_Duration->Pos_Begin+(Value2-stts_Duration->DTS_Begin)/stts_Duration->SampleDuration;

                                                                //Searching the I-Frame
                                                                if (!Stream->second.stss.empty())
                                                                {
                                                                    for (size_t Pos=0; Pos<Stream->second.stss.size(); Pos++)
                                                                        if (FrameNumber<=Stream->second.stss[Pos])
                                                                        {
                                                                            if (Pos && FrameNumber<Stream->second.stss[Pos])
                                                                                FrameNumber=Stream->second.stss[Pos-1];
                                                                            break;
                                                                        }
                                                                }

                                                                //Searching the corresponding stco
                                                                std::vector<stream::stsc_struct>::iterator Stsc=Stream->second.stsc.begin();
                                                                int64u SamplePos=0;
                                                                for (; Stsc!=Stream->second.stsc.end(); ++Stsc)
                                                                {
                                                                    std::vector<stream::stsc_struct>::iterator Stsc_Next=Stsc; ++Stsc_Next;
                                                                    int64u CountOfSamples = ((Stsc_Next == Stream->second.stsc.end() ? (int64u)Stream->second.stco.size() : (int64u)Stsc_Next->FirstChunk) - (int64u)Stsc->FirstChunk) * (int64u)Stsc->SamplesPerChunk;
                                                                    if (Stsc_Next!=Stream->second.stsc.end() && FrameNumber>=SamplePos+CountOfSamples)
                                                                        SamplePos+=CountOfSamples;
                                                                    else
                                                                    {
                                                                        int64u CountOfChunks=(FrameNumber-SamplePos)/Stsc->SamplesPerChunk;
                                                                        size_t stco_Pos=(size_t)(Stsc->FirstChunk-1+CountOfChunks); //-1 because first chunk is number 1
                                                                        if (stco_Pos>Stream->second.stco.size())
                                                                            return 2; //Invalid FrameNumber
                                                                        if (JumpTo>Stream->second.stco[stco_Pos])
                                                                            JumpTo=Stream->second.stco[stco_Pos];
                                                                        break;
                                                                    }
                                                                }

                                                            }
                                                        }

                                                    }
                                                    break;
                                default           : ;
                            }

                        if (!StreamOffset_Jump.empty())
                        {
                            std::map<int64u, int64u>::iterator StreamOffset_Current=StreamOffset_Jump.end();
                            do
                                --StreamOffset_Current;
                            while (StreamOffset_Current->second>JumpTo && StreamOffset_Current!=StreamOffset_Jump.begin());
                            JumpTo=StreamOffset_Current->second;
                        }

                        GoTo(JumpTo);
                        Open_Buffer_Unsynch();
                        return 1;
                    }
                    #else //MEDIAINFO_DEMUX
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX
        case 3  :
                    //FrameNumber
                    #if MEDIAINFO_DEMUX
                    {
                        //Looking for video stream
                        std::map<int32u, stream>::iterator Stream;
                        for (Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                            if (Stream->second.StreamKind==Stream_Video)
                                break;
                        if (Stream==Streams.end())
                            for (Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                                if (Stream->second.StreamKind==Stream_Audio)
                                    break;
                        if (Stream==Streams.end())
                            for (Stream=Streams.begin(); Stream!=Streams.end(); ++Stream)
                                if (Stream->second.StreamKind==Stream_Text)
                                    break;
                        if (Stream==Streams.end())
                            return 0; //Not supported

                        //Searching the I-Frame
                        if (!Stream->second.stss.empty())
                        {
                            for (size_t Pos=0; Pos<Stream->second.stss.size(); Pos++)
                                if (Value<=Stream->second.stss[Pos])
                                {
                                    if (Pos && Value<Stream->second.stss[Pos])
                                        Value=Stream->second.stss[Pos-1];
                                    break;
                                }
                        }

                        //Searching the corresponding stco
                        std::vector<stream::stsc_struct>::iterator Stsc=Stream->second.stsc.begin();
                        int64u SamplePos=0;
                        for (; Stsc!=Stream->second.stsc.end(); ++Stsc)
                        {
                            std::vector<stream::stsc_struct>::iterator Stsc_Next=Stsc; ++Stsc_Next;
                            int64u CountOfSamples = ((Stsc_Next == Stream->second.stsc.end() ? ((int64u)Stream->second.stco.size()) : ((int64u)Stsc_Next->FirstChunk)) - (int64u)Stsc->FirstChunk) * (int64u)Stsc->SamplesPerChunk;
                            if (Stsc_Next!=Stream->second.stsc.end() && Value>=SamplePos+CountOfSamples)
                                SamplePos+=CountOfSamples;
                            else
                            {
                                int64u CountOfChunks=(Value-SamplePos)/Stsc->SamplesPerChunk;
                                size_t stco_Pos=(size_t)(Stsc->FirstChunk-1+CountOfChunks); //-1 because first chunk is number 1
                                if (stco_Pos>=Stream->second.stco.size())
                                    return 2; //Invalid value
                                int64u Offset=Stream->second.stco[stco_Pos];

                                //Seeking back to audio/text frames before this video frame
                                if (!StreamOffset_Jump.empty())
                                {
                                    if (stco_Pos==0) //The first Stco is considered as the last byte of previous stram
                                    {
                                        if (!mdat_Pos.empty())
                                            Offset=mdat_Pos[0].Offset;
                                    }
                                    else
                                    {
                                        std::map<int64u, int64u>::iterator StreamOffset_Jump_Temp=StreamOffset_Jump.find(Stream->second.stco[stco_Pos]);
                                        if (StreamOffset_Jump_Temp!=StreamOffset_Jump.end())
                                            Offset=StreamOffset_Jump_Temp->second;
                                    }
                                }
                                else
                                {
                                    //TODO
                                }

                                GoTo(Offset);
                                Open_Buffer_Unsynch();
                                return 1;
                            }
                        }

                        return 2; //Invalid value
                    }
                    #else //MEDIAINFO_DEMUX
                        return (size_t)-1; //Not supported
                    #endif //MEDIAINFO_DEMUX
        default :   return 0;
    }
}
#endif //MEDIAINFO_SEEK

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4::Read_Buffer_Init()
{
    if (Config->ParseSpeed>=1.0)
        FrameCount_MaxPerStream=(int32u)-1;
    else if (Config->ParseSpeed>=0.7)
        FrameCount_MaxPerStream=2048;
    else if (Config->ParseSpeed<=0.3)
        FrameCount_MaxPerStream=128;
    else
        FrameCount_MaxPerStream=1024;

    #if MEDIAINFO_CONFORMANCE
        IsCmaf=MediaInfoLib::Config.Mp4Profile().find("cmfc")!=string::npos;
    #endif
}

//***************************************************************************
// Buffer
//***************************************************************************

//---------------------------------------------------------------------------
bool File_Mpeg4::Header_Begin()
{
    #if MEDIAINFO_DEMUX
        //Handling of multiple frames in one block
        if (IsParsing_mdat && Config->Demux_Unpacketize_Get())
        {
            stream &Stream_Temp=Streams[(int32u)Element_Code];
            if (Stream_Temp.Demux_EventWasSent)
            {
                Frame_Count_NotParsedIncluded=(int64u)-1;
                Open_Buffer_Continue(Stream_Temp.Parsers[0], Buffer+Buffer_Offset, 0);
                if (Config->Demux_EventWasSent)
                    return false;
                Stream_Temp.Demux_EventWasSent=false;
            }
        }
    #endif //MEDIAINFO_DEMUX

    if (IsParsing_mdat && Element_Level==0)
        Element_Begin0();

    return true;
}

//---------------------------------------------------------------------------
void File_Mpeg4::Header_Parse()
{
    //mdat
    if (IsParsing_mdat)
    {
        //Positionning
        if (mdat_Pos_Temp==mdat_Pos_Max || File_Offset+Buffer_Offset<mdat_Pos_Temp->Offset)
        {
            Header_Fill_Code(0, "(Junk)");
            int64u Size=mdat_Pos_Temp==mdat_Pos_Max?Element_TotalSize_Get():(mdat_Pos_Temp->Offset-(File_Offset+Buffer_Offset));
            if (Size>1 && Size>=Buffer_MaximumSize/2)
                Size=Buffer_MaximumSize;
            if (Size==Element_TotalSize_Get())
                IsParsing_mdat=false;
            Header_Fill_Size(Size);
            return;
        }

        //Filling
        Header_Fill_Code2(mdat_Pos_Temp->StreamID, Ztring::ToZtring(mdat_Pos_Temp->StreamID));
        Header_Fill_Size(mdat_Pos_Temp->Size);
        if (Buffer_Offset+mdat_Pos_Temp->Size<=Buffer_Size) //Only if we will not need it later (in case of partial data, this function will be called again for the same chunk)
        {
            mdat_Pos_Temp++;
            while (mdat_Pos_Temp!=mdat_Pos_Max)
            {
                if (mdat_Pos_NormalParsing && !Streams[mdat_Pos_Temp->StreamID].IsPriorityStream)
                    break;
                if (!mdat_Pos_NormalParsing && Streams[mdat_Pos_Temp->StreamID].IsPriorityStream)
                    break;
                mdat_Pos_Temp++;
            }
        }
        else
            Element_WaitForMoreData();

        //Hints
        if (File_Buffer_Size_Hint_Pointer && mdat_Pos_Temp!=mdat_Pos_Max && mdat_Pos_Temp->Offset+mdat_Pos_Temp->Size>File_Offset+Buffer_Size && mdat_Pos_Temp->Offset<File_Offset+Buffer_Size+128*1024)
        {
            size_t Buffer_Size_Target=(size_t)(mdat_Pos_Temp->Offset+mdat_Pos_Temp->Size-(File_Offset+Buffer_Size));
            if (Buffer_Size_Target<128*1024)
                Buffer_Size_Target=128*1024;
            (*File_Buffer_Size_Hint_Pointer)=Buffer_Size_Target;
        }

        return;
    }

    //Parsing
    int64u Size;
    int32u Size_32, Name;
    if (Element_Size==2)
    {
        if (!Element_IsComplete_Get())
        {
            Element_WaitForMoreData();
            return;
        }

        int16u Size_16;
        Peek_B2(Size_16);
        if (!Size_16)
        {
            Skip_B2(                                            "Size");

            //Filling
            Header_Fill_Code(0, "Junk");
            Header_Fill_Size(2);
            return;
        }
    }
    Get_B4 (Size_32,                                            "Size");
    if (Size_32==0 && (Element_Size==4 || Element_Size==8))
    {
        if (!Element_IsComplete_Get())
        {
            Element_WaitForMoreData();
            return;
        }

        //Filling
        Header_Fill_Code(0, "Junk");
        Header_Fill_Size(4);
        return;
    }
    Size=Size_32;
    Get_C4 (Name,                                               "Name");
    if (Name==0x33647666) //3dvf
        Name=0x6D6F6F76; //moov
    if (Name==0x61766964) //avid
        Name=0x6D646174; //mdat

    if (Size<8)
    {
        //Special case: until the end of the atom
            if (Size==0)
        {
            Size=Config->File_Current_Size-(File_Offset+Buffer_Offset);
            if (Status[IsAccepted] && Element_Level==2 && Name==0x00000000) //First real level (Level 1 is atom, level 2 is header block)
            {
                Element_Offset=0;
                Name=Elements::mdat;
            }
        }
        //Special case: Big files, size is 64-bit
        else if (Size==1)
        {
            //Reading Extended size
            Get_B8 (Size,                                       "Size (Extended)");
        }
        //Not in specs!
        else
        {
            Size=Config->File_Current_Size-(File_Offset+Buffer_Offset);
        }
    }

    //Specific case: file begin with "free" atom
    if (!Status[IsAccepted]
     && (Name==Elements::free
      || Name==Elements::skip
      || Name==Elements::wide))
    {
        Accept("MPEG-4");
        Fill(Stream_General, 0, General_Format, "QuickTime");
    }

    //Filling
    Header_Fill_Code(Name, Ztring().From_CC4(Name));
    Header_Fill_Size(Size);

    if (Name==0x6D6F6F76 && Buffer_Offset+Size>Buffer_Size-Buffer_Offset) //moov
    {
        File_Buffer_Size_Hint_Pointer=Config->File_Buffer_Size_Hint_Pointer_Get();

        //Hints
        if (File_Buffer_Size_Hint_Pointer && Size>128*1024)
        {
            size_t Buffer_Size_Target=(size_t)(Buffer_Offset+Size-(Buffer_Size-Buffer_Offset));
            if (Buffer_Size_Target<128*1024)
                Buffer_Size_Target=128*1024;
            (*File_Buffer_Size_Hint_Pointer)=Buffer_Size_Target;
        }
    }

    //Incoherencies
    if (Element_Level<=2 && File_Offset+Buffer_Offset+Size>File_Size)
        IsTruncated(File_Offset+Buffer_Offset+Size, false, "MPEG-4");
}

//---------------------------------------------------------------------------
struct Mpeg4_muxing
{
    int64u  MinimalOffset;
    int64u  MaximalOffset;

    Mpeg4_muxing()
    {
        MinimalOffset=(int64u)-1;
        MaximalOffset=0;
    }
};
struct stream_temp
{
    int64u stsc_SampleSizeOffset;
    int64u stts_Current;
    float64 DTS_Offset;
    size_t stco_Pos; //Chunk Offset
    size_t stsc_Pos; //Sample to Chunk
    size_t stsc_SampleNumber;
    size_t stts_Durations_Pos; //Time to Sample
    size_t stts_Durations_Pos_Offset; //For checking when we jump
    size_t stsz_Pos;

    stream_temp()
    {
        stco_Pos=0;
        stsc_Pos=0;
        stsc_SampleNumber=0;
        stts_Durations_Pos_Offset=0;
        stsc_SampleSizeOffset=0;
        stts_Durations_Pos=0;
        stts_Current=0;
        stsz_Pos=0;
        DTS_Offset=0;
    }
};

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void File_Mpeg4::stream::SplitAudio(File_Mpeg4::stream& Video, int32u moov_mvhd_TimeScale)
{
    //Complex audio edit lists are not supported, but detect fake complex edit lists
    bool ComplexAudioEditList = edts.size() != 1 || edts[0].Delay != 0;
    if (mdhd_TimeScale && ComplexAudioEditList && edts.size() == 2 && ((int64u)edts[1].Delay) * moov_mvhd_TimeScale / mdhd_TimeScale == edts[0].Duration)
        ComplexAudioEditList = false;

    //Check if we need to split
    if (Video.stts.size() != 1          //Complex video frame durations not supported
        || Video.edts.size() != 1       //Complex video edit lists are not supported
        || Video.edts[0].Delay != 0     //Complex video edit lists are not supported
        || ComplexAudioEditList         //Complex audio edit lists are not supported
        || stco.empty()                 //At least 1 audio stco must be present
        || stsc.empty()                 //At least 1 audio stsc must be present
        || !stsz.empty()                //Audio stsz must be empty
        || Video.mdhd_TimeScale == 0    //mdhd_TimeScale must be valid
        || mdhd_TimeScale == 0          //mdhd_TimeScale must be valid
        || stsz_Sample_Multiplier == 0) //stsz_Sample_Multiplier must be valid
        return;

    int32u FirstChunk_Offset = Video.stsc[0].FirstChunk;

    int64u AudioTicks_Total = 0;
    int64u VideoTicks_Total = 0;
    int64u AudioTicks_PerChunk = stsc[0].SamplesPerChunk;
    int32u VideoTicks_PerSample = Video.stts[0].SampleDuration;
    int32u SamplesInChunk = 0;
    int64u AudioTicks_Chunk = AudioTicks_PerChunk * Video.mdhd_TimeScale;

    int32u stco_Pos = 0;
    size_t stco_Size = stco.size();
    int32u stsc_Pos = 0;

    //Testing if it is usefull (at least 1 audio sample is 2x longer than video sample)
    bool ShouldSplit = false;
    size_t stsc_size = stsc.size();
    for (size_t i = 0; i<stsc_size; i++)
        if (stsc[i].SamplesPerChunk * Video.mdhd_TimeScale > VideoTicks_PerSample * mdhd_TimeScale * 2)
        {
            ShouldSplit = true;
            break;
        }
    if (!ShouldSplit)
        return;

    std::vector<stsc_struct> NewStsc;
    std::vector<int64u> NewStsz;
    std::vector<stts_struct> NewStts;

    for (;;)
    {
        //Add new sample for both video and audio
        VideoTicks_Total += VideoTicks_PerSample * mdhd_TimeScale;
        SamplesInChunk++;
        bool IsLastSampleInChunk = AudioTicks_Chunk < VideoTicks_PerSample * mdhd_TimeScale * 3 / 2;

        //Compute sample duration
        int64u SampleDuration;
        if (IsLastSampleInChunk)
            SampleDuration = AudioTicks_Chunk / Video.mdhd_TimeScale;
        else
        {
            //Compute sample size (with rounding)
            SampleDuration = VideoTicks_Total - AudioTicks_Total;
            if (SampleDuration > VideoTicks_PerSample * mdhd_TimeScale * 2)
            {
                //Handle case there is a big difference between video and audio time due to previous small audio packets
                SampleDuration = VideoTicks_PerSample * mdhd_TimeScale * 2;
                if (SampleDuration < AudioTicks_Chunk && AudioTicks_Chunk - SampleDuration < VideoTicks_PerSample * mdhd_TimeScale / 2)
                    SampleDuration -= (AudioTicks_Chunk - SampleDuration) / 2; //Trying to avoid next small audio packet
            }
            if (SampleDuration > AudioTicks_Chunk)
            {
                //Handle case the requested samples are more than the current packet
                IsLastSampleInChunk = true;
                SampleDuration = AudioTicks_Chunk / Video.mdhd_TimeScale;
            }
            else if (SampleDuration % Video.mdhd_TimeScale >= Video.mdhd_TimeScale / 2)
            {
                SampleDuration /= Video.mdhd_TimeScale;
                SampleDuration++;
            }
            else
                SampleDuration /= Video.mdhd_TimeScale;
        }

        //Add sample size
        NewStsz.push_back(SampleDuration * stsz_Sample_Size * stsz_Sample_Multiplier);

        //Add sample duration
        size_t stts_pos = NewStts.size();
        if (stts_pos && NewStts[stts_pos - 1].SampleDuration == SampleDuration)
            NewStts[stts_pos - 1].SampleCount++;
        else
        {
            NewStts.resize(stts_pos + 1);
            NewStts[stts_pos].SampleCount = 1;
            NewStts[stts_pos].SampleDuration = (int32u)SampleDuration;
        }

        if (IsLastSampleInChunk)
        {
            //Add samples per chunk
            size_t stsc_pos = NewStsc.size();
            NewStsc.resize(stsc_pos + 1);
            NewStsc[stsc_pos].FirstChunk = stco_Pos + FirstChunk_Offset;
            NewStsc[stsc_pos].SamplesPerChunk = SamplesInChunk;

            //Next chunk
            stco_Pos++;
            SamplesInChunk = 0;
            if (stco_Pos >= stco_Size)
            {
                AudioTicks_PerChunk = stts_Duration - (AudioTicks_Total + AudioTicks_Chunk) / Video.mdhd_TimeScale; //In case there is not all stco (e.g. partial parsing), we create a fake last stco with all remaining stts
                if (!AudioTicks_PerChunk)
                    break; //No more content
            }
            else if (stsc_Pos + 1<stsc.size() && stco_Pos + FirstChunk_Offset >= stsc[stsc_Pos + 1].FirstChunk)
            {
                stsc_Pos++;
                AudioTicks_PerChunk = stsc[stsc_Pos].SamplesPerChunk;
            }
            AudioTicks_Total += AudioTicks_Chunk;
            AudioTicks_Chunk = AudioTicks_PerChunk * Video.mdhd_TimeScale;

            continue;
        }

        //Next
        SampleDuration *= Video.mdhd_TimeScale;
        AudioTicks_Chunk -= SampleDuration;
        AudioTicks_Total += SampleDuration;
    }

    //Assigning new data (except stts)
    stsc = NewStsc;
    stsz = NewStsz;
    stsz_Total = NewStsz;
    stsz_Sample_Size = 0;

    //Replaying moov_trak_mdia_minf_stbl_stts()
    stts.clear();
    stts_Min=(int32u)-1;
    stts_Max=0;
    stts_FrameCount=0;
    stts_Duration=0;
    stts_Durations.clear();
    int32u NewStts_Size=(int32u)NewStts.size();
    int64u stts_Duration_Firstrame_Temp=stts_Duration_FirstFrame; //This items ill be not correct with default algo, as we create new scheme
    int64u stts_Duration_LastFrame_Temp=stts_Duration_LastFrame;
    for (int32u Pos = 0; Pos < NewStts_Size; Pos++)
        moov_trak_mdia_minf_stbl_stts_Common(NewStts[Pos].SampleCount, NewStts[Pos].SampleDuration, Pos, NewStts_Size);
    stts_Duration_FirstFrame=stts_Duration_Firstrame_Temp;
    stts_Duration_LastFrame=stts_Duration_LastFrame_Temp;
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
bool File_Mpeg4::BookMark_Needed()
{
    #if MEDIAINFO_HASH
        if (!mdat_MustParse && !mdat_Pos_NormalParsing && Config->File_Hash_Get().to_ulong() && FirstMdatPos<FirstMoovPos)
        {
            Element_Show();
            while (Element_Level>0)
                Element_End0();
            mdat_Pos_NormalParsing=true;
            GoTo(0);
            IsSecondPass=true;
            return false;
        }
    #endif //MEDIAINFO_HASH

    if (!mdat_MustParse || File_GoTo!=(int64u)-1 || Config->ParseSpeed<0)
        return false;

    //Handling of some wrong stsz and stsc atoms (ADPCM)
    if (!IsSecondPass)
        for (std::map<int32u, stream>::iterator Temp=Streams.begin(); Temp!=Streams.end(); ++Temp)
            if (Temp->second.StreamKind==Stream_Audio
             && (Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_CodecID)==__T("ima4")
              || Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_CodecID)==__T("11")))
            {
                Temp->second.stsz_StreamSize/=16;
                Temp->second.stsz_StreamSize*=17;
                float32 BitRate_Nominal=Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_BitRate_Nominal).To_float32();
                if (BitRate_Nominal)
                {
                    BitRate_Nominal/=16;
                    BitRate_Nominal*=17;
                    Fill(Stream_Audio, Temp->second.StreamPos, Audio_BitRate_Nominal, BitRate_Nominal, 0, true);
                }
                int64u Channels=Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_Channel_s_).To_int64u();
                if (Channels!=2)
                {
                    Temp->second.stsz_StreamSize/=2;
                    Temp->second.stsz_StreamSize*=Channels;
                }
                for (size_t Pos=0; Pos<Temp->second.stsc.size(); Pos++)
                {
                    Temp->second.stsc[Pos].SamplesPerChunk/=16;
                    Temp->second.stsc[Pos].SamplesPerChunk*=17;
                    if (Channels!=2)
                    {
                        Temp->second.stsc[Pos].SamplesPerChunk/=2;
                        Temp->second.stsc[Pos].SamplesPerChunk*=(int32u)Channels;
                    }
                }
            }

    //In case of second pass
    int64u ToJump=(int64u)-1;
    if (mdat_Pos.empty())
    {
        std::sort(mdat_Pos.begin(), mdat_Pos.end());

        #if MEDIAINFO_DEMUX
            std::map<int32u, struct Mpeg4_muxing> Muxing; //key is StreamID
        #endif //MEDIAINFO_DEMUX
        size_t  stco_Count=(size_t)-1;
        bool    stco_IsDifferent=false;
        int64u  LocalPTS=0;

        //For each stream
        for (std::map<int32u, stream>::iterator Temp=Streams.begin(); Temp!=Streams.end(); ++Temp)
            if (!Temp->second.Parsers.empty())
        {
            //Limit the detection
            #if defined(MEDIAINFO_SMPTEST0337_YES) && defined(MEDIAINFO_PCM_YES)
                if (Temp->second.IsPcm && Temp->second.Parsers.size()>=2 && !Temp->second.stsc.empty() && Temp->second.stsc[0].SamplesPerChunk>=48000/4) // 1/4 of second is enough for detection
                    ((File_Pcm*)Temp->second.Parsers[Temp->second.Parsers.size()-1])->Frame_Count_Valid=3; //3 for checking the PCM block after this one (next track) + Some files have 2 consecutive packets before the next stream, and we want to catch the channel grouping
            #endif //defined(MEDIAINFO_PCM_YES)

            //PCM split
            #if MEDIAINFO_DEMUX
            if (Temp->second.IsPcm && Config->Demux_SplitAudioBlocks_Get())
                for (std::map<int32u, stream>::iterator Temp2 = Streams.begin(); Temp2 != Streams.end(); ++Temp2) //Looking for the first video stream available
                    if (Temp2->second.StreamKind==Stream_Video)
                    {
                        Temp->second.SplitAudio(Temp2->second, moov_mvhd_TimeScale);
                        #if defined(MEDIAINFO_SMPTEST0337_YES) && defined(MEDIAINFO_PCM_YES)
                            if (Temp->second.Parsers.size()>=2 && Temp->second.stsz.size() && Temp->second.stco.size())
                                ((File_Pcm*)Temp->second.Parsers[Temp->second.Parsers.size()-1])->Frame_Count_Valid=Temp->second.stsz.size()/Temp->second.stco.size()+2; //+1 for handling rounding, +1 for taking the first frame of the next stco
                        #endif //defined(MEDIAINFO_PCM_YES)
                        break;
                    }
            #endif //#if MEDIAINFO_DEMUX

            if (!Temp->second.File_Name.empty())
            {
                #if MEDIAINFO_DEMUX
                    if (Config_Demux && Config->File_Demux_Interleave_Get()
                     && Temp->second.StreamKind!=Stream_Other) // e.g. a time code, we can live without demuxing the time code
                    {
                        //Remark: supporting both embedded and referenced streams is currently not supported
                        mdat_Pos.clear();
                        return false;
                    }
                    else
                #endif // MEDIAINFO_DEMUX
                       continue;
             }

            if (!Temp->second.stsz.empty() || Temp->second.stsz_Sample_Size)
            {
                if (!stco_IsDifferent)
                {
                    if (stco_Count==(size_t)-1)
                        stco_Count=Temp->second.stco.size();
                    else if (stco_Count!=Temp->second.stco.size())
                        stco_IsDifferent=true;
                }

                size_t Chunk_FrameCount=0;
                int32u Chunk_Number=1;
                int32u Sample_ByteSize=0;
                if (Temp->second.StreamKind==Stream_Audio)
                    Sample_ByteSize=Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_BitDepth).To_int32u()*Retrieve(Stream_Audio, Temp->second.StreamPos, Audio_Channel_s_).To_int32u()/8;

                #if MEDIAINFO_DEMUX
                    stream::stts_durations Temp_stts_Durations;
                #endif //MEDIAINFO_DEMUX
                    if (!Temp->second.stco.empty() && !Temp->second.stsc.empty())
                {
                    int64u* stco_Current = &Temp->second.stco[0];
                    int64u* stco_Max = stco_Current + Temp->second.stco.size();
                    int64u* stsz_Current = Temp->second.stsz.empty()?NULL:&Temp->second.stsz[0];
                    int32u* stsz_Current2 = Temp->second.stsz_FirstSubSampleSize.empty()?NULL:&Temp->second.stsz_FirstSubSampleSize[0];
                    int64u* stsz_Max = stsz_Current + Temp->second.stsz.size();
                    int32u* stsz_Max2 = stsz_Current2 + Temp->second.stsz_FirstSubSampleSize.size();
                    stream::stsc_struct* stsc_Current = &Temp->second.stsc[0];
                    stream::stsc_struct* stsc_Max = stsc_Current + Temp->second.stsc.size();
                    #if MEDIAINFO_DEMUX
                        int64u MinimalOffset = (int64u)-1;
                        int64u MaximalOffset = 0;
                    #endif //MEDIAINFO_DEMUX
                    for (; stco_Current<stco_Max; ++stco_Current)
                    {
                        #if MEDIAINFO_DEMUX
                            if (MinimalOffset>*stco_Current)
                                MinimalOffset = *stco_Current;
                            if (MaximalOffset < *stco_Current)
                                MaximalOffset = *stco_Current;
                        #endif //MEDIAINFO_DEMUX

                        while (stsc_Current + 1 < stsc_Max && Chunk_Number >= (stsc_Current + 1)->FirstChunk)
                            stsc_Current++;

                        if (Temp->second.stsz_Sample_Size == 0 && stsc_Current && !Temp->second.stsz.empty())
                        {
                            //Each sample has its own size
                            int64u Chunk_Offset = 0;
                            for (size_t Pos = 0; Pos < stsc_Current->SamplesPerChunk; Pos++)
                                if (*stsz_Current)
                                {
                                    mdat_Pos_Type mdat_Pos_Temp2;
                                    mdat_Pos_Temp2.Offset = *stco_Current + Chunk_Offset;
                                    mdat_Pos_Temp2.StreamID = Temp->first;
                                    if (stsz_Current2 && stsz_Current2 <= stsz_Max2 && *stsz_Current2)
                                        mdat_Pos_Temp2.Size = *stsz_Current2;
                                    else
                                        mdat_Pos_Temp2.Size = *stsz_Current;
                                    mdat_Pos.push_back(mdat_Pos_Temp2);
                                    if (Temp->second.IsCaption)
                                        mdat_Pos_Caption.push_back(mdat_Pos_Temp2);
                                    Chunk_Offset += *stsz_Current;
                                    stsz_Current++;
                                    if (stsz_Current2)
                                        stsz_Current2++;
                                    if (stsz_Current >= stsz_Max)
                                        break;
                                }
                            if (stsz_Current >= stsz_Max)
                                break;
                        }
                        else if (Temp->second.IsPcm && (!Sample_ByteSize || Temp->second.stsz_Sample_Size <= Sample_ByteSize) && stsc_Current && stsc_Current->SamplesPerChunk*Temp->second.stsz_Sample_Size*Temp->second.stsz_Sample_Multiplier < 0x1000000)
                        {
                            //Same size per sample, but granularity is too small
                            mdat_Pos_Type mdat_Pos_Temp2;
                            mdat_Pos_Temp2.Offset = *stco_Current;
                            mdat_Pos_Temp2.StreamID = Temp->first;
                            mdat_Pos_Temp2.Size = stsc_Current->SamplesPerChunk*Temp->second.stsz_Sample_Size*Temp->second.stsz_Sample_Multiplier;
                            mdat_Pos.push_back(mdat_Pos_Temp2);
                            if (Temp->second.IsCaption)
                                mdat_Pos_Caption.push_back(mdat_Pos_Temp2);

                            #if MEDIAINFO_DEMUX
                                if (Temp_stts_Durations.empty() || stsc_Current->SamplesPerChunk != Temp_stts_Durations[Temp_stts_Durations.size() - 1].SampleDuration)
                                {
                                    stream::stts_duration  stts_Duration;
                                    stts_Duration.Pos_Begin = Temp_stts_Durations.empty() ? 0 : Temp_stts_Durations[Temp_stts_Durations.size() - 1].Pos_End;
                                    stts_Duration.Pos_End = stts_Duration.Pos_Begin + 1;
                                    stts_Duration.SampleDuration = stsc_Current->SamplesPerChunk;
                                    stts_Duration.DTS_Begin = Temp_stts_Durations.empty() ? 0 : Temp_stts_Durations[Temp_stts_Durations.size() - 1].DTS_End;
                                    stts_Duration.DTS_End = stts_Duration.DTS_Begin + stts_Duration.SampleDuration;
                                    Temp_stts_Durations.push_back(stts_Duration);
                                    //Temp->second.stsc[stsc_Pos].SamplesPerChunk=1;
                                }
                                else
                                {
                                    Temp_stts_Durations[Temp_stts_Durations.size() - 1].Pos_End++;
                                    Temp_stts_Durations[Temp_stts_Durations.size() - 1].DTS_End += Temp_stts_Durations[Temp_stts_Durations.size() - 1].SampleDuration;
                                }
                            #endif //MEDIAINFO_DEMUX
                        }
                        else if (stsc_Current < stsc_Max)
                        {
                            //Same size per sample
                            int64u Chunk_Offset = 0;
                            for (size_t Pos = 0; Pos < stsc_Current->SamplesPerChunk; Pos++)
                                if (Temp->second.stsz_Sample_Size && Temp->second.stsz_Sample_Multiplier)
                                {
                                int64u Size = Temp->second.stsz_Sample_Size*Temp->second.stsz_Sample_Multiplier;
                                if (Size >= File_Size)
                                {
                                    stco_Current = stco_Max - 1;
                                    break; //Coherency issues
                                }
                                mdat_Pos_Type mdat_Pos_Temp2;
                                mdat_Pos_Temp2.Offset = *stco_Current + Chunk_Offset;
                                mdat_Pos_Temp2.StreamID = Temp->first;
                                mdat_Pos_Temp2.Size = Size;
                                bool UseIt=true;
                                if (Temp->second.TimeCode && Temp->second.edts.size()==1 && Temp->second.edts[0].Delay && Temp->second.stts.size()==1 && Temp->second.stts[0].SampleCount!=1 && Temp->second.stts[0].SampleDuration)
                                {
                                    LocalPTS+=Temp->second.stts[0].SampleDuration;
                                    if (LocalPTS<Temp->second.edts[0].Delay
                                     || LocalPTS*moov_mvhd_TimeScale>=Temp->second.edts[0].Delay*moov_mvhd_TimeScale+Temp->second.edts[0].Duration*Temp->second.mdhd_TimeScale)
                                        UseIt=false;
                                }
                                if (UseIt)
                                {
                                    Temp->second.LastUsedOffset=mdat_Pos_Temp2.Offset;
                                    if (Temp->second.FirstUsedOffset==(int64u)-1)
                                        Temp->second.FirstUsedOffset=mdat_Pos_Temp2.Offset;
                                    mdat_Pos.push_back(mdat_Pos_Temp2);
                                    if (Temp->second.IsCaption)
                                        mdat_Pos_Caption.push_back(mdat_Pos_Temp2);
                                }
                                Chunk_Offset += Size;
                                if (Chunk_Offset >= File_Size)
                                {
                                    stco_Current = stco_Max - 1;
                                    break; //Coherency issues
                                }
                                Chunk_FrameCount++;
                                }
                            if (!Temp->second.TimeCode && !Temp->second.IsCaption && Chunk_FrameCount >= FrameCount_MaxPerStream)
                                break;
                        }

                        Chunk_Number++;
                    }
                    #if MEDIAINFO_DEMUX
                        Muxing[Temp->first].MinimalOffset=MinimalOffset;
                        Muxing[Temp->first].MaximalOffset=MaximalOffset;
                    #endif //MEDIAINFO_DEMUX
                    for (size_t Pos=0; Pos<Temp->second.Parsers.size(); Pos++)
                        {
                        if(Temp->second.stts_Duration && Temp->second.mdhd_TimeScale)
                           Temp->second.Parsers[Pos]->Stream_BitRateFromContainer=Temp->second.stsz_StreamSize*8/(((float64)Temp->second.stts_Duration)/Temp->second.mdhd_TimeScale);
                        }
                    #if MEDIAINFO_DEMUX
                        if (FrameCount_MaxPerStream==(int32u)-1 && !Temp_stts_Durations.empty())
                        {
                            Temp->second.stts_Durations=Temp_stts_Durations;
                            for (stsc_Current=&Temp->second.stsc[0]; stsc_Current<stsc_Max; ++stsc_Current)
                                stsc_Current->SamplesPerChunk=1;
                            Temp->second.stts_FrameCount=Temp_stts_Durations[Temp_stts_Durations.size()-1].Pos_End;
                        }
                    #endif //MEDIAINFO_DEMUX
                }
            }

            //special cases
            #if MEDIAINFO_DEMUX
                if (Temp->second.stsz.empty() && Temp->second.StreamKind==Stream_Video && Retrieve(Stream_Video, Temp->second.StreamPos, Video_CodecID)==__T("AV1x"))
                {
                    //Found unknown data before the raw content
                    int64u Width=Retrieve(Stream_Video, Temp->second.StreamPos, Video_Width).To_int64u();
                    int64u Height=Retrieve(Stream_Video, Temp->second.StreamPos, Video_Height).To_int64u();
                    if (Width && Height && Temp->second.stsz_Sample_Size>(Width*Height*2))
                        Temp->second.Demux_Offset=Temp->second.stsz_Sample_Size-(Width*Height*2); // YUV 4:2:2 8 bit = 2 bytes per pixel
                }
            #endif //MEDIAINFO_DEMUX
        }
        std::sort(mdat_Pos.begin(), mdat_Pos.end(), &mdat_pos_sort);
        if (mdat_Pos.empty())
        {
            mdat_Pos_Temp=nullptr;
            mdat_Pos_Max=nullptr;
        }
        else
        {
            mdat_Pos_Temp=&mdat_Pos[0];
            mdat_Pos_Max=mdat_Pos_Temp+mdat_Pos.size();
        }

        #if MEDIAINFO_DEMUX
            if (!stco_IsDifferent && Muxing.size()==2)
            {
                std::map<int32u, struct Mpeg4_muxing>::iterator Muxing_1=Muxing.begin();
                std::map<int32u, struct Mpeg4_muxing>::iterator Muxing_2=Muxing.begin(); ++Muxing_2;
                if (Muxing_1->second.MaximalOffset>Muxing_2->second.MinimalOffset)
                    swap(Muxing_1, Muxing_2);
                if (Muxing_1->second.MaximalOffset<=Muxing_2->second.MinimalOffset)
                {
                    for (size_t stco_Pos=1; stco_Pos<stco_Count; stco_Pos++)
                    {
                        StreamOffset_Jump[Streams[Muxing_1->first].stco[stco_Pos]]=Streams[Muxing_2->first].stco[stco_Pos-1];
                        StreamOffset_Jump[Streams[Muxing_2->first].stco[stco_Pos]]=Streams[Muxing_1->first].stco[stco_Pos];
                    }
                    StreamOffset_Jump[Streams[Muxing_2->first].stco[0]]=Streams[Muxing_2->first].stco[stco_Count-1];
                }
            }
            else if (!mdat_Pos.empty() && Config->ParseSpeed>=1)
            {
                //Trying to see if we must interleave manually
                std::vector<int32u> Audios;
                int64u FirstStco_Last=0;
                int64u LastStco_Begin=(int64u)-1;
                for (streams::iterator Stream=Streams.begin(); Stream!=Streams.end(); Stream++)
                {
                    if (!Stream->second.stco.empty() && !Stream->second.IsPriorityStream)
                    {
                        if (Stream->second.stco[0]>FirstStco_Last)
                            FirstStco_Last=Stream->second.stco[0];
                        if (Stream->second.stco[Stream->second.stco.size()-1]<LastStco_Begin)
                            LastStco_Begin=Stream->second.stco[Stream->second.stco.size()-1];
                        Audios.push_back(Stream->first);
                    }
                }

                if (LastStco_Begin<FirstStco_Last)
                {
                    //We need to interleave
                    std::map<int32u, stream_temp> StreamsTemp;

                    int64u CurrentPos=mdat_Pos.begin()->Offset;
                    int64u OldPos=(int64u)-1;
                    for (;;)
                    {
                        bool IsInterleaving=false;
                        float64 Time=DBL_MAX;
                        for (std::vector<int32u>::iterator Temp=Audios.begin(); Temp!=Audios.end(); Temp++)
                        {
                            if (StreamsTemp[*Temp].stts_Durations_Pos_Offset==0)
                            {
                                stream::stts_durations::iterator stts_Duration = Streams[*Temp].stts_Durations.begin() + Streams[*Temp].stts_Durations_Pos;
                                int64u stts_Offset=stts_Duration->DTS_Begin+(((int64u)stts_Duration->SampleDuration)*(Frame_Count_NotParsedIncluded-stts_Duration->Pos_Begin));
                                if (!Streams[*Temp].edts.empty()) //TODO: common code about edit lists
                                {
                                    int64s Delay=0;
                                    switch (Streams[*Temp].edts.size())
                                    {
                                        case 0 :
                                                break;
                                        case 1 :
                                                if (Streams[*Temp].edts[0].Duration==Streams[*Temp].tkhd_Duration && Streams[*Temp].edts[0].Rate==0x00010000 && moov_mvhd_TimeScale)
                                                {
                                                    Delay=-((int64s)Streams[*Temp].edts[0].Delay);
                                                }
                                                break;
                                        case 2 :
                                                if (Streams[*Temp].edts[0].Delay==(int32u)-1 && Streams[*Temp].edts[0].Duration+Streams[*Temp].edts[1].Duration==Streams[*Temp].tkhd_Duration && Streams[*Temp].edts[0].Rate==0x00010000 && Streams[*Temp].edts[1].Rate==0x00010000 && moov_mvhd_TimeScale)
                                                {
                                                    Delay=((int64s)Streams[*Temp].edts[0].Duration)-Streams[*Temp].edts[1].Delay;
                                                }
                                                break;
                                        default:
                                                break; //TODO: handle more complex Edit Lists
                                    }

                                    if (-Delay<(int64s)stts_Offset)
                                        stts_Offset+=Delay;
                                    else
                                        stts_Offset=0;
                                }
                                StreamsTemp[*Temp].DTS_Offset=((float64)stts_Offset)/moov_mvhd_TimeScale;
                            }

                            float64 Temp_Time=StreamsTemp[*Temp].DTS_Offset+((float64)StreamsTemp[*Temp].stts_Current)/Streams[*Temp].mdhd_TimeScale;
                            if (Temp_Time<Time)
                                Time=Temp_Time;
                        }
                        for (std::vector<int32u>::iterator Temp=Audios.begin(); Temp!=Audios.end(); Temp++)
                            if (StreamsTemp[*Temp].stco_Pos<Streams[*Temp].stco.size() && StreamsTemp[*Temp].DTS_Offset+((float64)StreamsTemp[*Temp].stts_Current)/Streams[*Temp].mdhd_TimeScale<Time+0.001) // 1ms margin in order to be a bit more logcial with 1/1.001 frame rates
                            {
                                int64u NextPos=Streams[*Temp].stco[StreamsTemp[*Temp].stco_Pos]+StreamsTemp[*Temp].stsc_SampleSizeOffset;
                                if (CurrentPos!=NextPos)
                                {
                                    if (OldPos==(int64u)-1)
                                    {
                                        mdat_Pos_Type CurrentPos_mdat_Pos_Type;
                                        CurrentPos_mdat_Pos_Type.Offset=NextPos;
                                        mdat_pos::iterator CurrentPos_mdat_Pos_Lower=std::lower_bound(mdat_Pos.begin(), mdat_Pos.end(), CurrentPos_mdat_Pos_Type);
                                        if (CurrentPos_mdat_Pos_Lower!=mdat_Pos.end())
                                            mdat_Pos_Temp_ToJump=&(*CurrentPos_mdat_Pos_Lower);
                                    }
                                    else
                                        StreamOffset_Jump[OldPos]=NextPos;
                                    CurrentPos=NextPos;
                                }
                                mdat_Pos_Type CurrentPos_mdat_Pos_Type;
                                CurrentPos_mdat_Pos_Type.Offset=CurrentPos;
                                mdat_pos::iterator CurrentPos_mdat_Pos_Lower=std::lower_bound(mdat_Pos.begin(), mdat_Pos.end(), CurrentPos_mdat_Pos_Type);
                                if (CurrentPos_mdat_Pos_Lower!=mdat_Pos.end() && CurrentPos_mdat_Pos_Lower->Offset==CurrentPos)
                                {
                                    OldPos=CurrentPos+CurrentPos_mdat_Pos_Lower->Size;
                                    ++CurrentPos_mdat_Pos_Lower;
                                    if (CurrentPos_mdat_Pos_Lower!=mdat_Pos.end())
                                        CurrentPos=CurrentPos_mdat_Pos_Lower->Offset;
                                    else
                                        CurrentPos=File_Size;
                                }
                                ++StreamsTemp[*Temp].stsc_SampleNumber;
                                if (Streams[*Temp].stsz.empty())
                                    StreamsTemp[*Temp].stsc_SampleSizeOffset+=Streams[*Temp].stsz_Sample_Size;
                                else
                                {
                                    StreamsTemp[*Temp].stsc_SampleSizeOffset+=Streams[*Temp].stsz[StreamsTemp[*Temp].stsz_Pos];
                                    ++StreamsTemp[*Temp].stsz_Pos;
                                }
                                if (StreamsTemp[*Temp].stsc_SampleNumber>=Streams[*Temp].stsc[StreamsTemp[*Temp].stsc_Pos].SamplesPerChunk)
                                {
                                    StreamsTemp[*Temp].stsc_SampleNumber=0;
                                    StreamsTemp[*Temp].stsc_SampleSizeOffset=0;
                                    ++StreamsTemp[*Temp].stco_Pos;
                                    if (StreamsTemp[*Temp].stsc_Pos+1<Streams[*Temp].stsc.size() && StreamsTemp[*Temp].stco_Pos+1>=Streams[*Temp].stsc[StreamsTemp[*Temp].stsc_Pos+1].FirstChunk)
                                        StreamsTemp[*Temp].stsc_Pos++;
                                }
                                StreamsTemp[*Temp].stts_Current+=Streams[*Temp].stts_Durations[StreamsTemp[*Temp].stts_Durations_Pos].SampleDuration;
                                ++StreamsTemp[*Temp].stts_Durations_Pos_Offset;
                                if (StreamsTemp[*Temp].stts_Durations_Pos_Offset>=Streams[*Temp].stts_Durations[StreamsTemp[*Temp].stts_Durations_Pos].Pos_End)
                                    ++StreamsTemp[*Temp].stts_Durations_Pos;
                                IsInterleaving=true;
                            }

                        if (!IsInterleaving)
                        {
                            //The last one, in order not to loop
                            StreamOffset_Jump[OldPos]=File_Size;

                            break;
                        }
                    }
                }
            }
        #endif //MEDIAINFO_DEMUX
    }
    if (mdat_Pos.empty())
        return false;

    IsParsing_mdat=false;
    if (!mdat_Pos_ToParseInPriority_StreamIDs.empty())
    {
        //Hanlding StreamIDs to parse in priority (currently, only the first block of each stream is parsed in priority)
        if (!Streams[mdat_Pos_ToParseInPriority_StreamIDs[0]].stco.empty())
        {
            mdat_Pos_Type* Temp=&mdat_Pos[0];
            int64u stco_ToFind=Streams[mdat_Pos_ToParseInPriority_StreamIDs[0]].FirstUsedOffset;
            if (stco_ToFind==(int64u)-1)
                stco_ToFind=Streams[mdat_Pos_ToParseInPriority_StreamIDs[0]].stco.front();
            while (Temp<mdat_Pos_Max && Temp->Offset!=stco_ToFind)
                Temp++;
            if (Temp<mdat_Pos_Max && Temp->Offset<File_Size) //Skipping data not in a truncated file
            {
                Element_Show();
                if (mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.empty())
                {
                    while (Element_Level>0)
                        Element_End0();
                    Element_Begin1("Priority streams");
                }

                mdat_Pos_Temp=Temp;
                mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.push_back(Temp->StreamID);
                GoTo(Temp->Offset);
                IsParsing_mdat_Set();

                //Remove offset if it is not stripped
                const auto& StreamTemp=Streams[mdat_Pos_ToParseInPriority_StreamIDs[0]];
                if (StreamTemp.TimeCode && (StreamTemp.stts.size()>1 || (!StreamTemp.stts.empty() && StreamTemp.stts[0].SampleCount>1 && StreamTemp.Parsers.size()==1)))
                {
                    ((File_Mpeg4_TimeCode*)StreamTemp.Parsers[0])->FirstEditOffset=0;
                }
                if (StreamTemp.TimeCode && StreamTemp.Parsers.size()==1)
                {
                    ((File_Mpeg4_TimeCode*)StreamTemp.Parsers[0])->LastUsedOffset=StreamTemp.LastUsedOffset;
                    if (Config->ParseSpeed<=0.5 && StreamTemp.stco.size()>16)
                    {
                        ((File_Mpeg4_TimeCode*)StreamTemp.Parsers[0])->AllFramesParsed=false;
                        auto StreamID=Temp->StreamID;
                        for (int j=mdat_Pos.size()-1; j>=0; j--)
                            if (StreamID==mdat_Pos[j].StreamID && mdat_Pos[j].Offset!=stco_ToFind && mdat_Pos[j].Offset!=StreamTemp.FirstUsedOffset && mdat_Pos[j].Offset!=StreamTemp.LastUsedOffset)
                                mdat_Pos.erase(mdat_Pos.begin()+j);
                        if (mdat_Pos.empty())
                        {
                            mdat_Pos_Temp=nullptr;
                            mdat_Pos_Max=nullptr;
                        }
                        else
                        {
                            Temp=&mdat_Pos[0];
                            mdat_Pos_Max=Temp+mdat_Pos.size();
                            while (Temp<mdat_Pos_Max && Temp->Offset!=stco_ToFind)
                                Temp++;
                            mdat_Pos_Temp=Temp<mdat_Pos_Max?Temp:nullptr;
                        }
                    }
                    else
                        ((File_Mpeg4_TimeCode*)StreamTemp.Parsers[0])->AllFramesParsed=true;
                }
            }
        }
        mdat_Pos_ToParseInPriority_StreamIDs.erase(mdat_Pos_ToParseInPriority_StreamIDs.begin());
    }

    if (File_GoTo==(int64u)-1 && !mdat_Pos_NormalParsing && !mdat_Pos.empty() && mdat_Pos.begin()->Offset<File_Size)  //Skipping data not in a truncated file
    {
        Element_Show();
        while (Element_Level>0)
            Element_End0();
        Element_Begin1("Second pass");
        Element_WantNextLevel=true;

        // Don't parse twice
        sort(mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.begin(), mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.end());
        if (!mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.empty())
        {
            for (int i=mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.size()-1; i>=0; i--)
            {
                auto StreamID=mdat_Pos_ToParseInPriority_StreamIDs_ToRemove[i];
                for (int j=mdat_Pos.size()-1; j>=0; j--)
                    if (StreamID==mdat_Pos[j].StreamID)
                        mdat_Pos.erase(mdat_Pos.begin()+j);
            }
            mdat_Pos_ToParseInPriority_StreamIDs_ToRemove.clear();
        }

        if (mdat_Pos.empty())
        {
            mdat_Pos_Temp=nullptr;
            mdat_Pos_Max=nullptr;
        }
        else
        {
            mdat_Pos_Temp=mdat_Pos_Temp_ToJump?mdat_Pos_Temp_ToJump:&mdat_Pos[0];
            mdat_Pos_Max=&mdat_Pos[0]+mdat_Pos.size();
            ToJump=mdat_Pos_Temp->Offset;
            GoTo(ToJump);
        }
        IsParsing_mdat_Set();
        mdat_Pos_NormalParsing=true;
    }

    IsSecondPass=true;
    Buffer_TotalBytes=0; //Reset of the total bytes parsed as we start the second pass from the begining
    return false; //We do not want to use the bookmark feature, only detect the end of the file
}


//---------------------------------------------------------------------------
#if MEDIAINFO_CONFORMANCE
string File_Mpeg4::CreateElementName()
{
    if (IsParsing_mdat) {
        return "mdat";
    }
    string Result;
    for (size_t i = 1; i < Element_Level; i++) {
        Result += Ztring().From_CC4(Element[i].Code).Trim().To_UTF8();
        if (Result.empty()) {
            Result = "0x20202020"; // Full of spaces
        }
        else if (Result.back() >= '0' && Result.back() <= '9') {
            Result += '_';
        }
        Result += ' ';
    }
    if (!Result.empty())
        Result.pop_back();
    return Result;
}
#endif

//---------------------------------------------------------------------------
//Get language string from 2CC
Ztring File_Mpeg4::Language_Get(int16u Language)
{
    if (Language==0x7FFF || Language==0xFFFF)
        return Ztring();

    if (Language<0x400)
        return Mpeg4_Language_Apple(Language);

    Ztring ToReturn;
    ToReturn.append(1, (Char)((Language>>10&0x1F)+0x60));
    ToReturn.append(1, (Char)((Language>> 5&0x1F)+0x60));
    ToReturn.append(1, (Char)((Language>> 0&0x1F)+0x60));
    return ToReturn;
}

//---------------------------------------------------------------------------
//Check if it is Qt or Mp4
bool File_Mpeg4::IsQt()
{
    const Ztring& CodecID=Retrieve_Const(Stream_General, 0, General_CodecID);
    if (CodecID.empty() || CodecID==__T("qt  "))
        return true;
    const Ztring& CodecID_Compatible=Retrieve_Const(Stream_General, 0, General_CodecID_Compatible);
    for (size_t i=0; i<CodecID_Compatible.size(); i+=5)
        if (CodecID_Compatible.substr(i, 4)==__T("qt  "))
            return true;
    return false;
}

//---------------------------------------------------------------------------
//Get Metadata definition from 4CC
File_Mpeg4::method File_Mpeg4::Metadata_Get(std::string &Parameter, int64u Meta)
{
    File_Mpeg4::method Method;
    switch (Meta)
    {
        //http://atomicparsley.sourceforge.net/mpeg-4files.html
        //http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/QuickTime.html#ItemList
        case Elements::moov_meta___alb : Parameter="Album"; Method=Method_String; break;
        case Elements::moov_meta___ard : Parameter="Director"; Method=Method_String; break;
        case Elements::moov_meta___arg : Parameter="Arranger"; Method=Method_String; break;
        case Elements::moov_meta___ART : Parameter="Performer"; Method=Method_String; break;
        case Elements::moov_meta___aut : Parameter="Performer"; Method=Method_String; break;
        case Elements::moov_meta___con : Parameter="Conductor"; Method=Method_String; break;
        case Elements::moov_meta___cmt : Parameter="Comment"; Method=Method_String; break;
        case Elements::moov_meta___cpy : Parameter="Copyright"; Method=Method_String; break;
        case Elements::moov_meta___day : Parameter="Recorded_Date"; Method=Method_String; break;
        case Elements::moov_meta___des : Parameter="Title_More"; Method=Method_String; break;
        case Elements::moov_meta___dir : Parameter="Director"; Method=Method_String; break;
        case Elements::moov_meta___dis : Parameter="TermsOfUse"; Method=Method_String; break;
        case Elements::moov_meta___edl : Parameter="Tagged_Date"; Method=Method_String; break;
        case Elements::moov_meta___enc : Parameter="Encoded_Application"; Method=Method_String; break;
        case Elements::moov_meta___fmt : Parameter="Origin"; Method=Method_String; break;
        case Elements::moov_meta___gen : Parameter="Genre"; Method=Method_String; break;
        case Elements::moov_meta___grp : Parameter="Grouping"; Method=Method_String; break;
        case Elements::moov_meta___hos : Parameter="HostComputer"; Method=Method_String; break;
        case Elements::moov_meta___inf : Parameter="Title_More"; Method=Method_String; break;
        case Elements::moov_meta___key : Parameter="Keywords"; Method=Method_String; break;
        case Elements::moov_meta___lyr : Parameter="Lyrics"; Method=Method_String; break;
        case Elements::moov_meta___mak : Parameter="Make"; Method=Method_String; break;
        case Elements::moov_meta___mod : Parameter="Model"; Method=Method_String; break;
        case Elements::moov_meta___nam : Parameter="Title"; Method=Method_String3; break;
        case Elements::moov_meta___ope : Parameter="Original/Performer"; Method=Method_String; break;
        case Elements::moov_meta___prd : Parameter="Producer"; Method=Method_String; break;
        case Elements::moov_meta___PRD : Parameter="Product"; Method=Method_String; break;
        case Elements::moov_meta___prf : Parameter="Performer"; Method=Method_String; break;
        case Elements::moov_meta___req : Parameter="Comment"; Method=Method_String; break;
        case Elements::moov_meta___sne : Parameter="SoundEngineer"; Method=Method_String; break;
        case Elements::moov_meta___sol : Parameter="Conductor"; Method=Method_String; break;
        case Elements::moov_meta___src : Parameter="DistributedBy"; Method=Method_String; break;
        case Elements::moov_meta___st3 : Parameter="Subtitle"; Method=Method_String; break;
        case Elements::moov_meta___swr : Parameter="Encoded_Application"; Method=Method_String; break;
        case Elements::moov_meta___too : Parameter="Encoded_Application"; Method=Method_String; break;
        case Elements::moov_meta___url : Parameter="Track/Url"; Method=Method_String; break;
        case Elements::moov_meta___wrn : Parameter="Warning"; Method=Method_String; break;
        case Elements::moov_meta___wrt : Parameter="Composer"; Method=Method_String; break;
        case Elements::moov_meta___xpd : Parameter="ExecutiveProducer"; Method=Method_String; break;
        case Elements::moov_meta___xyz : Parameter="Recorded_Location"; Method=Method_String; break;
        case Elements::moov_meta__aART : Parameter="Album/Performer"; Method=Method_String2; break;
        case Elements::moov_meta__akID : Parameter="AppleStoreAccountType"; Method=Method_Binary; break;
        case Elements::moov_meta__albm : Parameter="Album"; Method=Method_String2; break; //Has a optional track number after the NULL byte
        case Elements::moov_meta__apID : Parameter="AppleStoreAccount"; Method=Method_String; break;
        case Elements::moov_meta__atID : Parameter="AlbumTitleID"; Method=Method_Binary; break;
        case Elements::moov_meta__auth : Parameter="Performer"; Method=Method_String2; break;
        case Elements::moov_meta__catg : Parameter="Category"; Method=Method_String; break;
        case Elements::moov_meta__cnID : Parameter="AppleStoreCatalogID"; Method=Method_String; break;
        case Elements::moov_meta__cpil : Parameter="Compilation"; Method=Method_Binary; break;
        case Elements::moov_meta__cprt : Parameter="Copyright"; Method=Method_String2; break;
        case Elements::moov_meta__desc : Parameter="Description"; Method=Method_String; break;
        case Elements::moov_meta__disk : Parameter="Part"; Method=Method_Binary; break;
        case Elements::moov_meta__dscp : Parameter="Title_More"; Method=Method_String2; break;
        case Elements::moov_meta__egid : Parameter="EpisodeGlobalUniqueID"; Method=Method_Binary; break;
        case Elements::moov_meta__flvr : Parameter="Flavour"; Method=Method_Binary; break;
        case Elements::moov_meta__gnre : Parameter="Genre"; Method=Method_String2; break;
        case Elements::moov_meta__geID : Parameter="GenreID"; Method=Method_Binary; break;
        case Elements::moov_meta__grup : Parameter="Grouping"; Method=Method_String; break;
        case Elements::moov_meta__hdvd : Parameter="HDVideo"; Method=Method_Binary; break;
        case Elements::moov_meta__itnu : Parameter="iTunesU"; Method=Method_Binary; break;
        case Elements::moov_meta__kind : Parameter="ServiceKind"; Method= Method_Binary; break;
        case Elements::moov_meta__keyw : Parameter="Keyword"; Method=Method_String; break;
        case Elements::moov_meta__ldes : Parameter="LongDescription"; Method=Method_String; break;
        case Elements::moov_meta__name : Parameter="Title"; Method=Method_String; break;
        case Elements::moov_meta__pcst : Parameter="Podcast"; Method=Method_Binary; break;
        case Elements::moov_meta__perf : Parameter="Performer"; Method=Method_String2; break;
        case Elements::moov_meta__pgap : Parameter.clear(); Method=Method_None; break;
        case Elements::moov_meta__plID : Parameter="PlayListID"; Method=Method_Binary; break;
        case Elements::moov_meta__purd : Parameter="PurchaseDate"; Method=Method_String; break;
        case Elements::moov_meta__purl : Parameter="PodcastURL"; Method=Method_String; break;
        case Elements::moov_meta__rate : Parameter="Rating"; Method=Method_Binary; break;
        case Elements::moov_meta__rtng : Parameter="Rating"; Method=Method_Binary; break;
        case Elements::moov_meta__sdes : Parameter="Description"; Method=Method_String; break;
        case Elements::moov_meta__sfID : Parameter="AppleStoreCountry"; Method=Method_Binary; break;
        case Elements::moov_meta__soaa : Parameter="Album/Performer/Sort"; Method=Method_String; break; //SortAlbumArtist
        case Elements::moov_meta__soal : Parameter="Album/Sort"; Method=Method_String2; break; //SortAlbum
        case Elements::moov_meta__soar : Parameter="Performer/Sort"; Method=Method_String; break; //SortArtist
        case Elements::moov_meta__soco : Parameter="Composer/Sort"; Method=Method_String; break; //SortComposer
        case Elements::moov_meta__sonm : Parameter="Title/Sort"; Method=Method_String; break; //SortName
        case Elements::moov_meta__sosn : Parameter="Title/Sort"; Method=Method_String; break; //SortShow
        case Elements::moov_meta__stik : Parameter="ContentType"; Method=Method_Binary; break;
        case Elements::moov_meta__tagc : Parameter="ServiceKind"; Method=Method_String3; break;
        case Elements::moov_meta__titl : Parameter="Title"; Method=Method_String2; break;
        case Elements::moov_meta__tool : Parameter="Encoded_Application"; Method=Method_String3; break;
        case Elements::moov_meta__tmpo : Parameter="BPM"; Method=Method_Binary; break;
        case Elements::moov_meta__trkn : Parameter="Track"; Method=Method_Binary; break;
        case Elements::moov_meta__tven : Parameter="Part_ID"; Method=Method_Binary; break; //TVEpisodeID
        case Elements::moov_meta__tves : Parameter="Part"; Method=Method_String; break; //TVEpisode
        case Elements::moov_meta__tvnn : Parameter="TVNetworkName"; Method=Method_String; break;
        case Elements::moov_meta__tvsh : Parameter="Collection"; Method=Method_String; break; //TVShow
        case Elements::moov_meta__tvsn : Parameter="Season"; Method=Method_String; break; //TVSeason
        case Elements::moov_meta__xid_ : Parameter="Vendor"; Method=Method_String; break;
        case Elements::moov_meta__year : Parameter="Recorded_Date"; Method=Method_String2; break;
        default :
            {
                Parameter.clear();
                Parameter.append(1, (char)((Meta&0xFF000000)>>24));
                Parameter.append(1, (char)((Meta&0x00FF0000)>>16));
                Parameter.append(1, (char)((Meta&0x0000FF00)>> 8));
                Parameter.append(1, (char)((Meta&0x000000FF)>> 0));
                Method=Method_String;
            }
    }

    Ztring Value;
    Value.append(1, (Char)((Meta&0xFF000000)>>24)); //Can not use From_CC4 because there is sometimes the (C) character, not in Ansi 7-bit, so wrongly decoded on UTF-8 systems
    Value.append(1, (Char)((Meta&0x00FF0000)>>16));
    Value.append(1, (Char)((Meta&0x0000FF00)>> 8));
    Value.append(1, (Char)((Meta&0x000000FF)>> 0));
    if (MediaInfoLib::Config.CustomMapping_IsPresent(__T("MP4"), Value))
        Parameter=MediaInfoLib::Config.CustomMapping_Get(__T("MP4"), Value).To_UTF8();
    
    //Cleanup of the parameter, removing anything not ANSI 7-bit
    for (size_t i=0; i<Parameter.size();)
    {
        if (((int8s)Parameter[i])<0)
            Parameter.erase(i, 1);
        else
            i++;
    }

    return Method;
}

//---------------------------------------------------------------------------
//Get Metadata definition from string
File_Mpeg4::method File_Mpeg4::Metadata_Get(std::string &Parameter, const std::string &Meta)
{
         if (Meta=="com.apple.quicktime.copyright") Parameter="Copyright";
    else if (Meta=="com.apple.quicktime.displayname") Parameter="Title";
    else if (Meta=="aspect_ratio") Parameter="DisplayAspectRatio";
    else if (Meta=="date_recorded") Parameter="Recorded_Date";
    else if (Meta=="DATE") Parameter="Encoded_Date";
    else if (Meta=="iTunEXTC") Parameter="ContentRating";
    else if (Meta=="iTunMOVI") Parameter="iTunMOVI";
    else if (Meta=="iTunNORM") Parameter.clear();
    else if (Meta=="iTunes_CDDB_IDs") Parameter.clear();
    else if (Meta=="iTunSMPB") Parameter.clear();
    else if (Meta=="PERFORMER") Parameter="Performer";
    else if (Meta=="PUBLISHER") Parameter="Publisher";
    else Parameter=Meta;
    return Method_String;
}

//---------------------------------------------------------------------------
void File_Mpeg4::Descriptors()
{
    //Preparing
    File_Mpeg4_Descriptors MI;
    MI.KindOfStream=StreamKind_Last;
    MI.PosOfStream=StreamPos_Last;
    MI.Parser_DoNotFreeIt=true;
    MI.ES_ID_Infos=ES_ID_Infos;
    #if MEDIAINFO_CONFORMANCE
        const auto& Stream=Streams[moov_trak_tkhd_TrackID];
        MI.SamplingRate=Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int16u();
        MI.stss=&Stream.stss;
        MI.stss_IsPresent=&Stream.stss_IsPresent;
        MI.IsCmaf=&IsCmaf;
        MI.sbgp=&Stream.sbgp;
        MI.sbgp_IsPresent=&Stream.sbgp_IsPresent;
        MI.sgpd_prol=&Stream.sgpd_prol;
        MI.stts=&Stream.stts;
        MI.FirstOutputtedDecodedSample=&Stream.FirstOutputtedDecodedSample;
    #endif

    int64u Elemen_Code_Save=Element_Code;
    Element_Code=moov_trak_tkhd_TrackID; //Element_Code is use for stream identifier
    Open_Buffer_Init(&MI);
    Element_Code=Elemen_Code_Save;
    mdat_MustParse=true; //Data is in MDAT

    //Parsing
    MI.TrackID=moov_trak_tkhd_TrackID;
    Open_Buffer_Continue(&MI);

    //Filling
    Finish(&MI);
    Merge(MI, StreamKind_Last, 0, StreamPos_Last);

    //Special case: AAC
    if (StreamKind_Last==Stream_Audio
     && (Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("AAC")
      || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("MPEG Audio")
      || Retrieve(Stream_Audio, StreamPos_Last, Audio_Format)==__T("Vorbis")))
        Clear(Stream_Audio, StreamPos_Last, Audio_BitDepth); //Resolution is not valid for AAC / MPEG Audio / Vorbis

    //Parser from Descriptor
    if (MI.Parser)
    {
        for (size_t Pos=0; Pos<Streams[moov_trak_tkhd_TrackID].Parsers.size(); Pos++)
            delete Streams[moov_trak_tkhd_TrackID].Parsers[Pos];
        Streams[moov_trak_tkhd_TrackID].Parsers_Clear();
        Streams[moov_trak_tkhd_TrackID].Parsers.push_back(MI.Parser);
        mdat_MustParse=true;
    }

    if (!MI.ES_ID_Infos.empty())
        ES_ID_Infos=MI.ES_ID_Infos;
}

//---------------------------------------------------------------------------
void File_Mpeg4::TimeCode_Associate(int32u TrackID)
{
    //Trying to detect time code attached to 1 video only but for all streams in reality
    int32u TimeCode_TrackID=(int32u)-1;
    bool   TimeCode_TrackID_MoreThanOne=false;
    for (std::map<int32u, stream>::iterator Strea=Streams.begin(); Strea!=Streams.end(); ++Strea)
        if (Strea->second.TimeCode_TrackID!=(int32u)-1)
        {
            if (TimeCode_TrackID==(int32u)-1)
                TimeCode_TrackID=Strea->second.TimeCode_TrackID;
            else
                TimeCode_TrackID_MoreThanOne=true;
        }
    if (!TimeCode_TrackID_MoreThanOne && TimeCode_TrackID!=(int32u)-1)
        for (std::map<int32u, stream>::iterator Strea=Streams.begin(); Strea!=Streams.end(); ++Strea)
            Strea->second.TimeCode_TrackID=TimeCode_TrackID; //For all tracks actually

    //Is it general or for a specific stream?
    bool IsGeneral=true;
    for (std::map<int32u, stream>::iterator Strea=Streams.begin(); Strea!=Streams.end(); ++Strea)
        if (Strea->second.TimeCode_TrackID==TrackID)
            IsGeneral=false;

    //For each track in the file (but only the last one will be used!)
    for (std::map<int32u, stream>::iterator Strea=Streams.begin(); Strea!=Streams.end(); ++Strea)
        if ((!Streams[TrackID].Parsers.empty() && (IsGeneral && Strea->second.StreamKind!=Stream_Max)) || Strea->second.TimeCode_TrackID==TrackID)
        {
            Streams[TrackID].Parsers[0]->Fill();
            if (Strea->second.StreamKind==Stream_Video)
            {
                Fill(Stream_Video, Strea->second.StreamPos, Video_Delay_Settings, Ztring(__T("DropFrame="))+(Streams[TrackID].TimeCode->DropFrame?__T("Yes"):__T("No")));
                Fill(Stream_Video, Strea->second.StreamPos, Video_Delay_Settings, Ztring(__T("24HourMax="))+(Streams[TrackID].TimeCode->H24?__T("Yes"):__T("No")));
                Fill(Stream_Video, Strea->second.StreamPos, Video_Delay_Settings, Ztring(__T("IsVisual="))+(Streams[TrackID].TimeCode_IsVisual?__T("Yes"):__T("No")));
            }
            if (Strea->second.StreamKind!=Stream_Other)
            {
                Fill(Strea->second.StreamKind, Strea->second.StreamPos, "Delay", Streams[TrackID].Parsers[0]->Get(Stream_General, 0, "Delay"));
                Fill(Strea->second.StreamKind, Strea->second.StreamPos, "Delay_DropFrame", Streams[TrackID].TimeCode->DropFrame?__T("Yes"):__T("No"));
                Fill(Strea->second.StreamKind, Strea->second.StreamPos, "Delay_Source", "Container");
            }

            //Fill(Strea->second.StreamKind, Strea->second.StreamPos, "TimeCode_FirstFrame", Streams[TrackID].Parsers[0]->Get(Stream_General, 0, "TimeCode_FirstFrame"));
            //Fill(Strea->second.StreamKind, Strea->second.StreamPos, "TimeCode_Source", Streams[TrackID].Parsers[0]->Get(Stream_General, 0, "TimeCode_Source"));
        }
 }

//---------------------------------------------------------------------------
void File_Mpeg4::AddCodecConfigurationBoxInfo()
{
    if (moov_trak_mdia_minf_stbl_stsd_Pos>1)
        return;
    Streams[moov_trak_tkhd_TrackID].CodecConfigurationBoxInfo.push_back((int32u)Element_Code);
}

//---------------------------------------------------------------------------
void File_Mpeg4::IsParsing_mdat_Set()
{
    IsParsing_mdat=true;

#if MEDIAINFO_ADVANCED
    //Checking if we need a fake time code track
    string TimeCode_String = Config->File_DefaultTimeCode_Get();
    if (TimeCode_String.size()==11
     &&  TimeCode_String[ 0] >= '0' && TimeCode_String[ 0] <= '9'
     &&  TimeCode_String[ 1] >= '0' && TimeCode_String[ 1] <= '9'
     &&  TimeCode_String[ 2] == ':'
     &&  TimeCode_String[ 3] >= '0' && TimeCode_String[ 3] <= '9'
     &&  TimeCode_String[ 4] >= '0' && TimeCode_String[ 4] <= '9'
     &&  TimeCode_String[ 5] >= ':'
     &&  TimeCode_String[ 6] >= '0' && TimeCode_String[ 6] <= '9'
     &&  TimeCode_String[ 7] >= '0' && TimeCode_String[ 7] <= '9'
     && (TimeCode_String[ 8] >= ':' || TimeCode_String[ 8] == ';')
     &&  TimeCode_String[ 9] >= '0' && TimeCode_String[ 9] <= '9'
     &&  TimeCode_String[10] >= '0' && TimeCode_String[10] <= '9' )
    {
        bool TimeCode_IsPresent = false;
        int32u TimeCode_ID = 0;
        for (std::map<int32u, stream>::iterator StreamTemp = Streams.begin(); StreamTemp != Streams.end(); ++StreamTemp)
        {
            if (StreamTemp->second.TimeCode)
                TimeCode_IsPresent = true;
            else if (StreamTemp->first >= TimeCode_ID)
                TimeCode_ID = StreamTemp->first + 1;
        }

        if (!TimeCode_IsPresent && TimeCode_ID)
        {
            stream::timecode *tc = new stream::timecode();
            tc->DropFrame = false;
            tc->H24 = false;
            tc->NegativeTimes = false;
            for (std::map<int32u, stream>::iterator StreamTemp = Streams.begin(); StreamTemp != Streams.end(); ++StreamTemp)
                if (StreamTemp->second.StreamKind == Stream_Video)
                {
                    tc->TimeScale = StreamTemp->second.mdhd_TimeScale;
                    tc->FrameDuration = StreamTemp->second.stts_Min;
                    if(tc->FrameDuration)
                       tc->NumberOfFrames = (int8u)float64_int64s(((float64)StreamTemp->second.mdhd_TimeScale) / StreamTemp->second.stts_Min);
                    else
                       tc->NumberOfFrames = 0; 
                    break;
                }

            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "QuickTime TC");
            Fill(Stream_Other, StreamPos_Last, Other_Title, "Fake");
            Streams[TimeCode_ID].StreamKind = Stream_Other;
            Streams[TimeCode_ID].StreamPos = StreamPos_Last;

            //Filling
            Streams[TimeCode_ID].TimeCode = tc;

            //Preparing TimeCode parser
            File_Mpeg4_TimeCode* Parser = new File_Mpeg4_TimeCode;
            Open_Buffer_Init(Parser);

            Parser->NumberOfFrames = tc->NumberOfFrames;
            Parser->DropFrame = tc->DropFrame;
            Parser->NegativeTimes = tc->NegativeTimes;

            int32u TimeCode_Value = TimeCode(TimeCode_String, tc->NumberOfFrames-1).ToFrames();
            
            int8u Buffer[4];
            int32u2BigEndian(Buffer, TimeCode_Value);
            Open_Buffer_Continue(Parser, Buffer, 4);

            Streams[TimeCode_ID].Parsers.push_back(Parser);
            for (std::map<int32u, stream>::iterator Strea=Streams.begin(); Strea!=Streams.end(); ++Strea)
                Strea->second.TimeCode_TrackID=TimeCode_ID; //For all tracks actually

            #if MEDIAINFO_DEMUX
            TimeCodeTrack_Check(Streams[TimeCode_ID], 0, TimeCode_ID);
            #endif //MEDIAINFO_DEMUX
        }
    }
#endif //MEDIAINFO_ADVANCED
}

//---------------------------------------------------------------------------
#if MEDIAINFO_DEMUX
void File_Mpeg4::TimeCodeTrack_Check(stream &Stream_Temp, size_t Pos, int32u StreamID)
{
    if (Stream_Temp.TimeCode) //If this is a TimeCode track
    {
        if (((File_Mpeg4_TimeCode*)Stream_Temp.Parsers[Pos])->Pos!=(int32u)-1)
        {
            for (std::map<int32u, stream>::iterator StreamTemp=Streams.begin(); StreamTemp!=Streams.end(); ++StreamTemp)
                if (StreamTemp->second.TimeCode_TrackID==StreamID)
                {
                    TimeCode_FrameOffset=((File_Mpeg4_TimeCode*)Stream_Temp.Parsers[Pos])->Pos;
                    float64 FrameRate_WithDF=Stream_Temp.TimeCode->NumberOfFrames;
                    if (Stream_Temp.TimeCode->DropFrame)
                    {
                        float64 FramesPerHour_NDF=FrameRate_WithDF*60*60;
                        FrameRate_WithDF*=(FramesPerHour_NDF-108)/FramesPerHour_NDF;
                    }
                    TimeCode_DtsOffset=float64_int64s(((float64)TimeCode_FrameOffset)*1000000000/FrameRate_WithDF);
                }
        }
    }
}
#endif //MEDIAINFO_DEMUX

//---------------------------------------------------------------------------
void File_Mpeg4::Skip_NulString(const char* Name)
{
    int64u End=Element_Offset;
    while (End<Element_Size && Buffer[Buffer_Offset+(size_t)End])
        End++;
    Skip_String(End-Element_Offset,                             Name);
    if (Element_Offset<Element_Size)
        Element_Offset++; //Skip zero
}

//***************************************************************************
// C++
//***************************************************************************

} //NameSpace

#endif //MEDIAINFO_MPEG4_YES
