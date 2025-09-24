/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Init and Finalize part
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
#if defined(MEDIAINFO_FILE_YES)
#include "ZenLib/File.h"
#endif //defined(MEDIAINFO_FILE_YES)
#include "ZenLib/FileName.h"
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include "MediaInfo/MediaInfo_Internal.h"
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/ExternalCommandHelpers.h"
#if MEDIAINFO_IBI
    #include "MediaInfo/Multiple/File_Ibi.h"
#endif //MEDIAINFO_IBI
#if MEDIAINFO_FIXITY
    #ifndef WINDOWS
    //ZenLib has File::Copy only for Windows for the moment. //TODO: support correctly (including meta)
    #include <fstream>
    #endif //WINDOWS
#endif //MEDIAINFO_FIXITY
#include <algorithm>
using namespace ZenLib;
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//---------------------------------------------------------------------------
#if __cplusplus >= 201703 || (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#else
struct string_view {
public:
    constexpr string_view() noexcept {
    }
    constexpr string_view(const char* Data, size_t Size) noexcept
        : _Data(Data), _Size(Size) {
    }
    constexpr const char* data() const noexcept {
        return _Data;
    }
    constexpr size_t size() const noexcept {
        return _Size;
    }
private:
    const char* _Data{};
    size_t _Size{};
};
#endif
static string_view Find_Replacement(const char* Database, const string& ToFind) {
    if (ToFind.empty() || !ToFind.front()) {
        return {}; // Nothing to search
    }
    auto Current = Database;
    for (;;) {
        auto Location = strstr(Current, ToFind.c_str());
        if (!Location) {
            return { nullptr, 0 }; // No more to search
        }
        if ((Location != Database && Location[-1] != '\n') || Location[ToFind.size()] != ';') {
            Current = strchr(Location, '\n') + 1;  // Note: \n is always the last character of Database so no need to check if it is not found
            continue; // Not a full match
        }
        Location += ToFind.size() + 1; // Skip the key and the semicolon
        const auto Size = static_cast<size_t>(strchr(Location, '\n') - Location); // Note: \n is always the last character of Database so no need to check if it is not found
        return { Location, Size };
    }
}
static bool IsPresent(const char* Database, const string& ToFind) {
    if (ToFind.empty() || !ToFind.front()) {
        return false; // Nothing to search
    }
    auto Current = Database;
    for (;;) {
        auto Location = strstr(Current, ToFind.c_str());
        if (!Location) {
            return false; // No more to search
        }
        if ((Location != Database && Location[-1] != '\n') || Location[ToFind.size()] != '\n') {
            Current = Location + 1;
            continue; // Not a full match
        }
        return true;
    }
}
class NewlineRange
{
public:
    class Iterator
    {
    public:
        constexpr Iterator() {
        }
        Iterator(const char* pos) {
            update(pos);
        }

        constexpr string_view operator*() const {
            return { current, static_cast<size_t>(next - current) };
        }

        Iterator& operator++() {
            if (*next) {
                update(next + 1);
            }
            else {
                current = next;
            }
            return *this;
        }

        constexpr bool operator!=(const Iterator&) const {
            return *current;
        }

    private:
        void update(const char* pos) {
            current = pos;
            next = pos;
            while (*next && *next != '\n') {
                ++next;
            }
        }
        const char* current = {};
        const char* next = {};
    };

    constexpr NewlineRange(const char* str) : begin_(str) {}

    Iterator begin() const { return Iterator(begin_); }
    constexpr Iterator end() const { return Iterator(); }

private:
    const char* begin_;
};

//---------------------------------------------------------------------------
static const char* CompanySuffixes =
    ".,LTD\n"
    "5MP\n"
    "AB\n"
    "AG\n"
    "CAMERA\n"
    "CO\n"
    "CO.\n"
    "CO.,\n"
    "CO.LTD.\n"
    "COMPANY\n"
    "COMPUTER\n"
    "CORP\n"
    "CORP.\n"
    "CORPORATION\n"
    "CORPORATION.\n"
    "DIGITAL\n"
    "DIGITCAM\n"
    "DSC\n"
    "ELEC\n"
    "ELEC.\n"
    "ELECTRIC\n"
    "ELECTRONICS\n"
    "ENTERTAINMENT\n"
    "EUROPE\n"
    "FILM\n"
    "FOTOTECHNIC\n"
    "GERMANY\n"
    "GMBH\n"
    "GROUP\n"
    "IMAGING\n"
    "INC\n"
    "INC.\n"
    "INTERNATIONAL\n"
    "KOREA\n"
    "LABORATORIES\n"
    "LIMITED\n"
    "LIVING\n"
    "LTD\n"
    "LTD.\n"
    "MOBILE\n"
    "MOBILITY\n"
    "OP\n"
    "OPTICAL\n"
    "PHOTO\n"
    "PRODUCTS\n"
    "SOLUTIONS\n"
    "SYSTEMS\n"
    "TECH\n"
    "TECHNOLOGIES\n"
    "TECHNOLOGY\n"
    "WIRELESS\n"
;

//---------------------------------------------------------------------------
static const char* CompanyNames =
    "ACER\n"
    "AGFA\n"
    "AIPTEK\n"
    "APPLE\n"
    "ASUS\n"
    "ASHAMPOO\n"
    "BENQ\n"
    "BUSHNELL\n"
    "CANON\n"
    "CASIO\n"
    "DIGILIFE\n"
    "EPSON\n"
    "FRAUNHOFER\n"
    "FUJI\n"
    "FUJIFILM\n"
    "GATEWAY\n"
    "GE\n"
    "GOOGLE\n"
    "HASSELBLAD\n"
    "HITACHI\n"
    "HONOR\n"
    "HP\n"
    "HUAWEI\n"
    "INSTA360\n"
    "JENOPTIK\n"
    "KODAK\n"
    "KONICA\n"
    "KYOCERA\n"
    "JENOPTIFIED\n"
    "LEGEND\n"
    "LEICA\n"
    "LUMICRON\n"
    "MAGINON\n"
    "MAGINON\n"
    "MAMIYA\n"
    "MEDION\n"
    "MICROSOFT\n"
    "MINOLTA\n"
    "MOTOROLA\n"
    "MOULTRIE\n"
    "MUSTEK\n"
    "NIKON\n"
    "NIKON\n"
    "ODYS\n"
    "OLYMPUS\n"
    "PANTECH\n"
    "PARROT\n"
    "PENTACON\n"
    "PIONEER\n"
    "POLAROID\n"
    "RECONYX\n"
    "RED\n"
    "RICOH\n"
    "ROLLEI\n"
    "SAGEM\n"
    "SAMSUNG\n"
    "SANYO\n"
    "SEALIFE\n"
    "SHARP\n"
    "SIGMA\n"
    "SKANHEX\n"
    "SONY\n"
    "SUNPLUS\n"
    "SUPRA\n"
    "T-MOBILE\n"
    "TESCO\n"
    "TOSHIBA\n"
    "TRAVELER\n"
    "TRUST\n"
    "VISTAQUEST\n"
    "VIVITAR\n"
    "VIVO\n"
    "VODAFONE\n"
    "XIAOMI\n"
    "YAKUMO\n"
    "YASHICA\n"
    "ZEISS\n"
    "ZJMEDIA\n"
    "ZTE\n"
;

//---------------------------------------------------------------------------
static const char* CompanyNames_Replace =
    "AGFA GEVAERT;Agfa\n"
    "AGFAPHOTO;Agfa\n"
    "BENQ;BenQ\n"
    "BENQ-SIEMENS;BenQ\n"
    "BLACKBERRY;BlackBerry\n"
    "CAMERA;\n"
    "CONCORD;Jenoptik\n"
    "CREATIVE;Creative Labs\n"
    "DEFAULT;\n"
    "DOCOMO;DoCoMo\n"
    "EASTMAN KODAK;KODAK\n"
    "FLIR SYSTEMS;FLIR\n"
    "FS-NIKON;Nikon\n"
    "FUJI;FUJIFILM\n"
    "GENERAL;GE\n"
    "GEDSC;GE\n"
    "GRANDTECH;Jenoptik\n"
    "HMD GLOBAL;HMD\n"
    "HEWLETT PACKARD;HP\n"
    "HEWLETT-PACKARD;HP\n"
    "JENIMAGE;JENOPTIK\n"
    "JK;Kodak\n"
    "KONICA MINOLTA;Konica Minolta\n"
    "KONICA;Konica Minolta\n"
    "LG CYON;LG\n"
    "LG MOBILE;LG\n"
    "LGE;LG\n"
    "MINOLTA;Konica Minolta\n"
    "MOTOROL;MOTOROLA\n"
    "NORITSU KOKI;Noritsu Koki\n"
    "NTT DOCOMO;DoCoMo\n"
    "OC;OpenCube\n"
    "OM;Olympus\n"
    "PENTAX RICOH;Ricoh\n"
    "PENTAX;Ricoh\n"
    "PLA100Z;Polaroid\n"
    "PRAKTICA;Pentacon\n"
    "RESEARCH IN MOTION;BlackBerry\n"
    "SAMSUNG ANYCALL;Samsung\n"
    "SAMSUNG DIGITAL IMA;Samsung\n"
    "SAMSUNG TECHWIN;Samsung\n"
    "SAMSUNG TECHWIN CO,.;Samsung\n"
    "SONY ERICSSON;Sony Ericsson\n"
    "SEIKO EPSON;Epson\n"
    "SKANHEX TECHWIN;Skanhex\n"
    "SUPRA / MAGINON;supra / Maginon\n"
    "T-MOBILE;T-Mobile\n"
    "TELEDYNE FLIR;FLIR\n"
    "THOMSON GRASS VALLEY;Grass Valley\n"
    "VIVICAM;Vivitar\n"
    "WWL;Polaroid\n"
    "XIAOYI;Xiaomi\n"
;

//---------------------------------------------------------------------------
static const char* Model_Replace_Canon = // Based on https://en.wikipedia.org/wiki/Canon_EOS and https://wiki.magiclantern.fm/camera_models_map
    "EOS 1500D;EOS 2000D\n"
    "EOS 200D Mark II;EOS 250D\n"
    "EOS 3000D;EOS 4000D\n"
    "EOS 770D;EOS 77D\n"
    "EOS 8000D;EOS 760D\n"
    "EOS 9000D;EOS 77D\n"
    "EOS DIGITAL REBEL XT;EOS 350D\n"
    "EOS DIGITAL REBEL XTI;EOS 400D\n"
    "EOS DIGITAL REBEL;EOS 300D\n"
    "EOS HI;EOS 1200D\n"
    "EOS KISS DIGITAL N;EOS 350D\n"
    "EOS KISS DIGITAL X;EOS 400D\n"
    "EOS KISS DIGITAL;EOS 300D\n"
    "EOS KISS F;EOS 1000D\n"
    "EOS KISS M;EOS M50\n"
    "EOS KISS M2;EOS M50 Mark II\n"
    "EOS KISS M50m2;EOS M50 Mark II\n"
    "EOS KISS X10;EOS 250D\n"
    "EOS KISS X10I;EOS 850D\n"
    "EOS KISS X2;EOS 450D\n"
    "EOS KISS X3;EOS 500D\n"
    "EOS KISS X4;EOS 550D\n"
    "EOS KISS X5;EOS 600D\n"
    "EOS KISS X50;EOS 1100D\n"
    "EOS KISS X6I;EOS 650D\n"
    "EOS KISS X7;EOS 100D\n"
    "EOS KISS X70;EOS 1200D\n"
    "EOS KISS X7I;EOS 700D\n"
    "EOS KISS X80;EOS 1300D\n"
    "EOS KISS X8I;EOS 750D\n"
    "EOS KISS X9;EOS 200D\n"
    "EOS KISS X90;EOS 2000D\n"
    "EOS KISS X9I;EOS 800D\n"
    "EOS REBEL SL1;EOS 100D\n"
    "EOS REBEL SL2;EOS 200D\n"
    "EOS REBEL SL3;EOS 250D\n"
    "EOS REBEL T100;EOS 4000D\n"
    "EOS REBEL T1I;EOS 500D\n"
    "EOS REBEL T2I;EOS 550D\n"
    "EOS REBEL T3;EOS 1100D\n"
    "EOS REBEL T3I;EOS 600D\n"
    "EOS REBEL T4I;EOS 650D\n"
    "EOS REBEL T5;EOS 1200D\n"
    "EOS REBEL T5I;EOS 700D\n"
    "EOS REBEL T6;EOS 1300D\n"
    "EOS REBEL T6I;EOS 750D\n"
    "EOS REBEL T6S;EOS 760D\n"
    "EOS REBEL T7;EOS 2000D\n"
    "EOS REBEL T7I;EOS 800D\n"
    "EOS REBEL T8I;EOS 850D\n"
    "EOS REBEL XS;EOS 1000D\n"
    "EOS REBEL XSI;EOS 450D\n"
;

static const char* Model_Replace_OpenCube =
    "OCTK;Toolkit\n"
    "TK;Toolkit\n"
;

struct FindReplaceCompany_struct {
    const char* CompanyName;
    const char* Find;
    const char* Prefix;
};

static const FindReplaceCompany_struct Model_Replace[] = {
    { "Canon", Model_Replace_Canon, {} },
    { "OpenCube", Model_Replace_OpenCube, {} },
}; 

//---------------------------------------------------------------------------
static const char* Model_Name_Sony =
    "401SO;Z3\n"
    "402SO;Z4\n"
    "501SO;Z5\n"
    "502SO;X Performance\n"
    "601SO;XZ\n"
    "602SO;XZs\n"
    "701SO;XZ1\n"
    "702SO;XZ2\n"
    "801SO;XZ3\n"
    "802SO;1\n"
    "901SO;5\n"
    "902SO;8\n"
    "A001SO;10 II\n"
    "A002SO;5 II\n"
    "A101SO;1 III\n"
    "A102SO;10 III\n"
    "A103SO;5 III\n"
    "A201SO;1 IV\n"
    "A202SO;10 IV\n"
    "A203SO;Ace III\n"
    "A204SO;5 IV\n"
    "A301SO;1 V\n"
    "A302SO;10 V\n"
    "A401SO;1 VI\n"
    "A402SO;10 VI\n"
    "C1504;E\n"
    "C1505;E\n"
    "C1604;E dual\n"
    "C1605;E dual\n"
    "C1904;M\n"
    "C1905;M\n"
    "C2004;M dual\n"
    "C2005;M Dual\n"
    "C2005;M dual\n"
    "C2104;L\n"
    "C2105;L\n"
    "C2304;C\n"
    "C2305;C\n"
    "C5302;SP\n"
    "C5302;SP\n"
    "C5303;SP\n"
    "C5303;SP\n"
    "C5306;SP\n"
    "C5306;SP\n"
    "C5502;ZR\n"
    "C5503;ZR\n"
    "C6502;ZL\n"
    "C6503;ZL\n"
    "C6506;ZL\n"
    "C6602;Z\n"
    "C6603;Z\n"
    "C6606;Z\n"
    "C6616;Z\n"
    "C6802;Z Ultra\n"
    "C6806;Z Ultra\n"
    "C6833;Z Ultra\n"
    "C6843;Z Ultra\n"
    "C6902;Z1\n"
    "C6903;Z1\n"
    "C6906;Z1\n"
    "C6916;Z1s\n"
    "C6943;Z1\n"
    "D2004;E1\n"
    "D2005;E1\n"
    "D2104;E1 Dual\n"
    "D2104;E1 dual\n"
    "D2105;E1 Dual\n"
    "D2105;E1 dual\n"
    "D2114;E1\n"
    "D2202;E3\n"
    "D2203;E3\n"
    "D2206;E3\n"
    "D2212;E3 Dual\n"
    "D2243;E3\n"
    "D2302;M2 Dual\n"
    "D2302;M2 dual\n"
    "D2303;M2\n"
    "D2305;M2\n"
    "D2306;M2\n"
    "D2403;M2 Aqua\n"
    "D2406;M2 Aqua\n"
    "D2502;C3 Dual\n"
    "D2533;C3\n"
    "D5102;T3\n"
    "D5103;T3\n"
    "D5106;T3\n"
    "D5303;T2 Ultra\n"
    "D5306;T2 Ultra\n"
    "D5316;T2 Ultra\n"
    "D5316N;T2 Ultra\n"
    "D5322;T2 Ultra dual\n"
    "D5322;T2 Ultra\n"
    "D5503;Z1 Compact\n"
    "D5788;J1 Compact\n"
    "D5803;Z3 Compact\n"
    "D5833;Z3 Compact\n"
    "D6502;Z2\n"
    "D6503;Z2\n"
    "D6543;Z2\n"
    "D6563;Z2a\n"
    "D6603;Z3\n"
    "D6616;Z3\n"
    "D6633;Z3\n"
    "D6643;Z3\n"
    "D6646;Z3\n"
    "D6653;Z3\n"
    "D6683;Z3 Dual TD\n"
    "D6708;Z3v\n"
    "E2003;E4g\n"
    "E2006;E4g\n"
    "E2033;E4g Dual\n"
    "E2043;E4g Dual\n"
    "E2053;E4g\n"
    "E2104;E4\n"
    "E2105;E4\n"
    "E2115;E4 Dual\n"
    "E2124;E4 Dual\n"
    "E2303;M4 Aqua\n"
    "E2306;M4 Aqua\n"
    "E2312;M4 Aqua Dual\n"
    "E2333;M4 Aqua Dual\n"
    "E2353;M4 Aqua\n"
    "E2363;M4 Aqua Dual\n"
    "E5303;C4\n"
    "E5306;C4\n"
    "E5333;C4 Dual\n"
    "E5343;C4 Dual\n"
    "E5353;C4\n"
    "E5363;C4 Dual\n"
    "E5506;C5 Ultra\n"
    "E5533;C5 Ultra Dual\n"
    "E5553;C5 Ultra\n"
    "E5563;C5 Ultra Dual\n"
    "E5603;M5\n"
    "E5606;M5\n"
    "E5633;M5 Dual\n"
    "E5643;M5 Dual\n"
    "E5653;M5\n"
    "E5663;M5 Dual\n"
    "E5803;Z5 Compact\n"
    "E5823;Z5 Compact\n"
    "E6508;Z4v\n"
    "E6533;Z3+ Dual\n"
    "E6553;Z3+\n"
    "E6603;Z5\n"
    "E6633;Z5 Dual\n"
    "E6653;Z5\n"
    "E6683;Z5 Dual\n"
    "E6833;Z5 Premium Dual\n"
    "E6853;Z5 Premium\n"
    "E6883;Z5 Premium Dual\n"
    "F3111;XA\n"
    "F3112;XA\n"
    "F3113;XA\n"
    "F3115;XA\n"
    "F3116;XA\n"
    "F3211;XA Ultra\n"
    "F3212;XA Ultra\n"
    "F3213;XA Ultra\n"
    "F3215;XA Ultra\n"
    "F3216;XA Ultra\n"
    "F3311;E5\n"
    "F3313;E5\n"
    "F5121;X\n"
    "F5122;X\n"
    "F5321;X Compact\n"
    "F8131;X Performance\n"
    "F8132;X Performance\n"
    "F8331;XZ\n"
    "F8332;XZ\n"
    "G1109;Touch\n"
    "G1209;Hello\n"
    "G2199;R1\n"
    "G2299;R1 Plus\n"
    "G3112;XA1\n"
    "G3116;XA1\n"
    "G3121;XA1\n"
    "G3123;XA1\n"
    "G3125;XA1\n"
    "G3212;XA1 Ultra\n"
    "G3221;XA1 Ultra\n"
    "G3223;XA1 Ultra\n"
    "G3226;XA1 Ultra\n"
    "G3311;L1\n"
    "G3312;L1\n"
    "G3313;L1\n"
    "G3412;XA1 Plus\n"
    "G3416;XA1 Plus\n"
    "G3421;XA1 Plus\n"
    "G3423;XA1 Plus\n"
    "G3426;XA1 Plus\n"
    "G8141;XZ Premium\n"
    "G8142;XZ Premium Dual\n"
    "G8188;XZ Premium\n"
    "G8231;XZs\n"
    "G8232;XZs Dual\n"
    "G8341;XZ1\n"
    "G8342;XZ1 Dual\n"
    "G8343;XZ1\n"
    "G8441;XZ1 Compact\n"
    "H3113;XA2\n"
    "H3123;XA2\n"
    "H3133;XA2\n"
    "H3213;XA2 Ultra\n"
    "H3223;XA2 Ultra\n"
    "H3311;L2\n"
    "H3321;L2\n"
    "H3413;XA2 Plus\n"
    "H4113;XA2\n"
    "H4133;XA2\n"
    "H4213;XA2 Ultra\n"
    "H4233;XA2 Ultra\n"
    "H4311;L2\n"
    "H4331;L2\n"
    "H4413;XA2 Plus\n"
    "H4493;XA2 Plus\n"
    "H8116;XZ2 Premium\n"
    "H8166;XZ2 Premium Dual\n"
    "H8216;XZ2\n"
    "H8266;XZ2 Dual\n"
    "H8276;XZ2\n"
    "H8296;XZ2 Dual\n"
    "H8314;XZ2 Compact\n"
    "H8324;XZ2 Compact\n"
    "H8416;XZ3\n"
    "H9436;XZ3\n"
    "H9493;XZ3\n"
    "I3312;L3\n"
    "I4312;L3\n"
    "I4332;L3\n"
    "J3173;Ace\n"
    "J3273;8 Lite\n"
    "J8110;1\n"
    "J8170;1\n"
    "J8210;5\n"
    "J8270;5\n"
    "J9110;1 Dual\n"
    "J9180;1\n"
    "J9210;5 Dual\n"
    "J9260;5 Dual\n"
    "M35h;SP\n"
    "M35t;SP\n"
    "S39h;C\n"
    "SGP311;Tablet Z\n"
    "SGP321;Tablet Z\n"
    "SGP341;Tablet Z\n"
    "SGP412;Z Ultra\n"
    "SGP511;Z2 Tablet\n"
    "SGP512;Z2 Tablet\n"
    "SGP521;Z2 Tablet\n"
    "SGP541;Z2 Tablet\n"
    "SGP551;Z2 Tablet\n"
    "SGP561;Z2 Tablet\n"
    "SGP611;Z3 Tablet Compact\n"
    "SGP612;Z3 Tablet Compact\n"
    "SGP621;Z3 Tablet Compact\n"
    "SGP641;Z3 Tablet Compact\n"
    "SGP712;Z4 Tablet\n"
    "SGP771;Z4 Tablet\n"
    "SO-01F;Z1\n"
    "SO-01G;Z3\n"
    "SO-01H;Z5\n"
    "SO-01J;XZ\n"
    "SO-01K;XZ1\n"
    "SO-01L;XZ3\n"
    "SO-01M;5\n"
    "SO-02E;Z\n"
    "SO-02F;Z1f\n"
    "SO-02G;Z3 Compact\n"
    "SO-02H;Z5 Compact\n"
    "SO-02J;X Compact\n"
    "SO-02K;XZ1 Compact\n"
    "SO-02L;Ace\n"
    "SO-03E;Tablet Z\n"
    "SO-03F;Z2\n"
    "SO-03G;Z4\n"
    "SO-03H;Z5 Premium\n"
    "SO-03J;XZs\n"
    "SO-03K;XZ2\n"
    "SO-03L;1\n"
    "SO-04E;A\n"
    "SO-04F;A2\n"
    "SO-04G;A4\n"
    "SO-04H;X Performance\n"
    "SO-04J;XZ Premium\n"
    "SO-04K;XZ2 Premium\n"
    "SO-05F;Z2 Tablet\n"
    "SO-05G;Z4 Tablet\n"
    "SO-05K;XZ2 Compact\n"
    "SO-41A;10 II\n"
    "SO-41B;Ace II\n"
    "SO-51A;1 II\n"
    "SO-51B;1 III\n"
    "SO-51C;1 IV\n"
    "SO-51D;1 V\n"
    "SO-51E;1 VI\n"
    "SO-51F;1 VII\n"
    "SO-52A;5 II\n"
    "SO-52B;10 III\n"
    "SO-52C;10 IV\n"
    "SO-52D;10 V\n"
    "SO-52E;10 VI\n"
    "SO-53B;5 III\n"
    "SO-53C;Ace III\n"
    "SO-53D;5 V\n"
    "SO-54C;5 IV\n"
    "SOG01;1 II\n"
    "SOG02;5 II\n"
    "SOG03;1 III\n"
    "SOG04;10 III\n"
    "SOG05;5 III\n"
    "SOG06;1 IV\n"
    "SOG07;10 IV\n"
    "SOG08;Ace III\n"
    "SOG09;5 IV\n"
    "SOG10;1 V\n"
    "SOG11;10 V\n"
    "SOG12;5 V\n"
    "SOG13;1 VI\n"
    "SOG14;10 VI\n"
    "SOL23;Z1\n"
    "SOL24;Z Ultra\n"
    "SOL25;ZL2\n"
    "SOL26;Z3\n"
    "SOT21;Z2 Tablet\n"
    "SOT31;Z4 Tablet\n"
    "SOV31;Z4\n"
    "SOV32;Z5\n"
    "SOV33;X Performance\n"
    "SOV34;XZ\n"
    "SOV35;XZs\n"
    "SOV36;XZ1\n"
    "SOV37;XZ2\n"
    "SOV38;XZ2 Premium\n"
    "SOV39;XZ3\n"
    "SOV40;1\n"
    "SOV41;5\n"
    "SOV42;8\n"
    "SOV42-u;8\n"
    "SOV43;10 II\n"
    "XQ-AD51;L4\n"
    "XQ-AD52;L4\n"
    "XQ-AQ52;PRO\n"
    "XQ-AQ62;PRO\n"
    "XQ-AS42;5 II\n"
    "XQ-AS52;5 II\n"
    "XQ-AS62;5 II\n"
    "XQ-AS72;5 II\n"
    "XQ-AT42;1 II\n"
    "XQ-AT51;1 II\n"
    "XQ-AT52;1 II\n"
    "XQ-AU42;10 II\n"
    "XQ-AU51;10 II\n"
    "XQ-AU52;10 II\n"
    "XQ-BC42;1 IV\n"
    "XQ-BC52;1 IV\n"
    "XQ-BC62;1 IV\n"
    "XQ-BC72;1 IV\n"
    "XQ-BE42;PRO-I\n"
    "XQ-BE52;PRO-I\n"
    "XQ-BE62;PRO-I\n"
    "XQ-BE72;PRO-I\n"
    "XQ-BQ42;5 III\n"
    "XQ-BQ52;5 III\n"
    "XQ-BQ62;5 III\n"
    "XQ-BQ72;5 III\n"
    "XQ-BT44;10 III Lite\n"
    "XQ-BT52;10 III\n"
    "XQ-CC44;10 IV\n"
    "XQ-CC54;10 IV\n"
    "XQ-CC72;10 IV\n"
    "XQ-CQ44;5 IV\n"
    "XQ-CQ54;5 IV\n"
    "XQ-CQ62;5 IV\n"
    "XQ-CQ72;5 IV\n"
    "XQ-CT44;1 IV\n"
    "XQ-CT54;1 IV\n"
    "XQ-CT62;1 IV\n"
    "XQ-CT72;1 IV\n"
    "XQ-DC44;10 V\n"
    "XQ-DC54;10 V\n"
    "XQ-DC72;10 V\n"
    "XQ-DE44;5 V\n"
    "XQ-DE54;5 V\n"
    "XQ-DE72;5 V\n"
    "XQ-DQ44;1 V\n"
    "XQ-DQ54;1 V\n"
    "XQ-DQ62;1 V\n"
    "XQ-DQ72;1 V\n"
    "XQ-EC44;1 VI\n"
    "XQ-EC54;1 VI\n"
    "XQ-EC72;1 VI\n"
    "XQ-ES44;10 VI\n"
    "XQ-ES54;10 VI\n"
    "XQ-ES72;10 VI\n"
    "XQ-FS44;1 VII\n"
    "XQ-FS54;1 VII\n"
    "XQ-FS72;1 VII\n"
    "XQ-FE44;10 VII\n"
    "XQ-FE54;10 VII\n"
    "XQ-FE72;10 VII\n"
;

static const char* Model_Name_Sony_Ericsson =
    "E10a;X10 mini\n"
    "E10i;X10 mini\n"
    "IS11S;Acro\n"
    "IS12S;acro HD\n"
    "LT18i;arc S\n"
    "LT22i;P\n"
    "LT25i;V\n"
    "LT26i;S\n"
    "LT26ii;SL\n"
    "LT26w;acro S\n"
    "LT28at;ion\n"
    "LT28h;ion\n"
    "LT28i;ion\n"
    "LT29i;TX\n"
    "LT30a;T\n"
    "LT30p;T\n"
    "MK16a;pro\n"
    "MK16i;pro\n"
    "MT11a;neo V\n"
    "MT11i;neo V\n"
    "MT15a;neo\n"
    "MT15i;neo\n"
    "MT25i;neo L\n"
    "MT27i;sola\n"
    "R800a;PLAY\n"
    "R800at;PLAY\n"
    "R800i;PLAY\n"
    "R800x;PLAY\n"
    "S51SE;mini\n"
    "SGPT121;Tablet S\n"
    "SGPT122;Tablet S\n"
    "SGPT123;Tablet S\n"
    "SGPT131;Tablet S 3G\n"
    "SGPT132;Tablet S 3G\n"
    "SGPT133;Tablet S 3G\n"
    "SK17a;mini pro\n"
    "SK17i;mini pro\n"
    "SO-01B;X10\n"
    "SO-01C;Arc\n"
    "SO-01D;PLAY\n"
    "SO-01E;AX\n"
    "SO-02C;Acro\n"
    "SO-02D;S\n"
    "SO-03C;ray\n"
    "SO-03D;acro HD\n"
    "SO-04D;GX\n"
    "SO-05D;SX\n"
    "SOL21;VL\n"
    "SOL22;UL\n"
    "ST15a;mini\n"
    "ST15i;mini\n"
    "ST17i;active\n"
    "ST18a;ray\n"
    "ST18i;ray\n"
    "ST21a;tipo\n"
    "ST21a2;tipo dual\n"
    "ST21i;tipo\n"
    "ST21i2;tipo\n"
    "ST23a;miro\n"
    "ST23i;miro\n"
    "ST25a;U\n"
    "ST25i;U\n"
    "ST26a;J\n"
    "ST26i;J\n"
    "ST27a;Go\n"
    "ST27i;Go\n"
    "U20a;X10 mini pro\n"
    "U20i;X10 mini pro\n"
    "X10a;X10\n"
    "X10i;X10\n"
;

static constexpr const char* Model_Name_Samsung =
    "403SC;Tab 4 7.0\n"
    "404SC;S6 Edge\n"
    "EK-GC100;Camera\n"
    "EK-GC110;Camera\n"
    "EK-GC120;Camera\n"
    "EK-GC200;Camera 2\n"
    "EK-GN100;NX\n"
    "EK-KC10;Camera\n"
    "EK-KC12;Camera\n"
    "GT-B533;Chat\n"
    "GT-B551;Y Pro\n"
    "GT-B751;Pro\n"
    "GT-B7810;M Pro 2\n"
    "GT-B9062;(China)\n"
    "GT-I550;Europa\n"
    "GT-I551;Europa\n"
    "GT-I570;Spica\n"
    "GT-i5700;Spica\n"
    "GT-I580;Apollo\n"
    "GT-I815;W\n"
    "GT-I816;Ace 2\n"
    "GT-I819;S3 Mini\n"
    "GT-I820;S3 Mini Value Edition\n"
    "GT-I8200;S3 Mini\n"
    "GT-I8250;Beam\n"
    "GT-I8258;M Style\n"
    "GT-I826;Core\n"
    "GT-I8260;Core Safe\n"
    "GT-I8262;S3 Duos\n"
    "GT-I8268;Duos\n"
    "GT-I8530;Beam\n"
    "GT-I855;Win\n"
    "GT-I858;Core Advance\n"
    "GT-I873;Express\n"
    "GT-I900;S\n"
    "GT-I901;S\n"
    "GT-I905;S\n"
    "GT-I906;Grand Neo\n"
    "GT-I907;S Advance\n"
    "GT-I908;Grand\n"
    "GT-I910;S2\n"
    "GT-I911;Grand\n"
    "GT-I912;Grand\n"
    "GT-I915;Mega\n"
    "GT-I916;Grand Neo\n"
    "GT-I919;S4 Mini\n"
    "GT-I920;Mega 6.3\n"
    "GT-I921;S2\n"
    "GT-I922;Note\n"
    "GT-I923;Golden\n"
    "GT-I926;Premier\n"
    "GT-I929;S4 Active\n"
    "GT-I930;S3\n"
    "GT-I950;S4\n"
    "GT-I951;S4\n"
    "GT-N510;Note 8.0\n"
    "GT-N5110;Note 8.0\n"
    "GT-N5120;Note 8.0\n"
    "GT-N700;Note\n"
    "GT-N710;Note 2\n"
    "GT-N800;Note 10.1\n"
    "GT-N801;Note 10.1\n"
    "GT-N8020;Note 10.1\n"
    "GT-P100;Tab\n"
    "GT-P101;Tab\n"
    "GT-P310;Tab 2 7.0\n"
    "GT-P311;Tab 2 7.0\n"
    "GT-P5100;Tab 2 10.1\n"
    "GT-P511;Tab 2 10.1\n"
    "GT-P5200;Tab 3 10.1\n"
    "GT-P521;Tab 3 10.1\n"
    "GT-P5220;Tab 3 10.1\n"
    "GT-P620;Tab 7.0 Plus\n"
    "GT-P621;Tab 7.0 Plus\n"
    "GT-P6800;Tab 7.7\n"
    "GT-P6810;Tab 7.7\n"
    "GT-P7100;Tab 10.1 v\n"
    "GT-P7300;Tab 8.9\n"
    "GT-P7310;Tab 8.9\n"
    "GT-P7320;Tab 8.9\n"
    "GT-P750;Tab 10.1\n"
    "GT-P7501;Tab 10.1 N\n"
    "GT-P7510;Tab 10.1\n"
    "GT-P7511;Tab 10.1 N\n"
    "GT-S528;Star\n"
    "GT-S5283B;Star Trios\n"
    "GT-S530;Pocket\n"
    "GT-S536;Y\n"
    "GT-S557;Mini\n"
    "GT-S566;Gio\n"
    "GT-S567;Fit\n"
    "GT-S569;Xcover\n"
    "GT-S583;Ace\n"
    "GT-S601;Music\n"
    "GT-S610;Y Duos\n"
    "GT-S6108;Y Pop\n"
    "GT-S631;Young\n"
    "GT-S6352;Ace Duos\n"
    "GT-S6358;Ace\n"
    "GT-S650;Mini2\n"
    "GT-S679;Fame\n"
    "GT-S6792;Fame Lite Duos\n"
    "GT-S680;Ace Duos\n"
    "GT-S6800;Ace Advance\n"
    "GT-S681;Fame\n"
    "GT-S7262;Star Plus\n"
    "GT-S727;Ace 3\n"
    "GT-S739;Trend\n"
    "GT-S750;Ace Plus\n"
    "GT-S7566;S Duos\n"
    "GT-S7568I;Trend\n"
    "GT-S7572;Trend Duos\n"
    "GT-S771;Xcover 2\n"
    "GT-S789;Trend 2\n"
    "ISW11SC;S2 Wimax\n"
    "SC-01C;Tab\n"
    "SC-01D;Tab 10.1\n"
    "SC-01E;Tab 7.7 Plus\n"
    "SC-01F;Note 3\n"
    "SC-01G;Note Edge\n"
    "SC-01H;Active neo\n"
    "SC-01J;Note7\n"
    "SC-01K;Note8\n"
    "SC-01L;Note9\n"
    "SC-01M;Note10+\n"
    "SC-02B;S\n"
    "SC-02C;S2\n"
    "SC-02D;Tab 7.0 Plus\n"
    "SC-02E;Note 2\n"
    "SC-02F;Note 3\n"
    "SC-02G;S5 Active\n"
    "SC-02H;S7 Edge\n"
    "SC-02J;S8\n"
    "SC-02K;S9\n"
    "SC-02L;Feel2\n"
    "SC-02M;A20\n"
    "SC-03D;S2 LTE\n"
    "SC-03E;S3\n"
    "SC-03G;Tab S 8.4\n"
    "SC-03J;S8+\n"
    "SC-03K;S9+\n"
    "SC-03L;S10\n"
    "SC-04E;S4\n"
    "SC-04F;S5\n"
    "SC-04G;S6 Edge\n"
    "SC-04J;Feel\n"
    "SC-04L;S10+\n"
    "SC-05D;Note\n"
    "SC-05G;S6\n"
    "SC-05L;S10+ Olympic Games Edition\n"
    "SC-06D;S3\n"
    "SC-41A;A41\n"
    "SC-42A;A21\n"
    "SC-51A;S20 5G\n"
    "SC51Aa;S20 5G\n"
    "SC-51B;S21 5G\n"
    "SC-51C;S22\n"
    "SC-51D;S23\n"
    "SC-51E;S24\n"
    "SC-51F;S25\n"
    "SC-52A;S20+ 5G\n"
    "SC-52B;S21 Ultra 5G\n"
    "SC-52C;S22 Ultra\n"
    "SC-52D;S23 Ultra\n"
    "SC-52E;S24 Ultra\n"
    "SC-52F;S25 Ultra\n"
    "SC-53A;Note20 Ultra 5G\n"
    "SC-53B;A52 5G\n"
    "SC-53C;A53 5G\n"
    "SC-53D;A54 5G\n"
    "SC-53E;A55 5G\n"
    "SC-53F;A25 5G\n"
    "SC-54A;A51 5G\n"
    "SC-54B;Z Flip3 5G\n"
    "SC-54C;Z Flip4\n"
    "SC-54D;Z Flip5\n"
    "SC-54E;Z Flip6\n"
    "SC-54F;A36 5G\n"
    "SC-55B;Z Fold3 5G\n"
    "SC-55C;Z Fold4\n"
    "SC-55D;Z Fold5\n"
    "SC-55E;Z Fold6\n"
    "SC-56B;A22 5G\n"
    "SC-56C;A23 5G\n"
    "SCG01;S20 5G\n"
    "SCG02;S20+ 5G\n"
    "SCG03;S20 Ultra 5G\n"
    "SCG04;Z Flip 5G\n"
    "SCG06;Note20 Ultra 5G\n"
    "SCG07;A51 5G\n"
    "SCG08;A32 5G\n"
    "SCG09;S21 5G\n"
    "SCG10;S21+ 5G\n"
    "SCG11;Z Fold3 5G\n"
    "SCG12;Z Flip3 5G\n"
    "SCG13;S22\n"
    "SCG14;S22 Ultra\n"
    "SCG15;A53 5G\n"
    "SCG16;Z Fold4\n"
    "SCG17;Z Flip4\n"
    "SCG18;A23 5G\n"
    "SCG19;S23\n"
    "SCG20;S23 Ultra\n"
    "SCG21;A54 5G\n"
    "SCG22;Z Fold5\n"
    "SCG23;Z Flip5\n"
    "SCG24;S23 FE\n"
    "SCG25;S24\n"
    "SCG26;S24 Ultra\n"
    "SCG27;A55 5G\n"
    "SCG28;Z Fold6\n"
    "SCG29;Z Flip6\n"
    "SCG30;S24 FE\n"
    "SCG31;S25\n"
    "SCG32;S25 Ultra\n"
    "SCG33;A25 5G\n"
    "SCH-I20;Stellar\n"
    "SCH-I400;Continuum\n"
    "SCH-I405;Stratosphere\n"
    "SCH-I415;Stratosphere 2\n"
    "SCH-I43;S4 Mini\n"
    "SCH-I500;S\n"
    "SCH-i509;Y\n"
    "SCH-I53;S3\n"
    "SCH-I54;S4\n"
    "SCH-i559;Pop\n"
    "SCH-i569;Gio\n"
    "SCH-i579;Ace Duos\n"
    "SCH-I589;Ace Duos\n"
    "SCH-I605;Note 2\n"
    "SCH-I619;Ace\n"
    "SCH-I629;Fame\n"
    "SCH-I679;Ace 3\n"
    "SCH-I705;Tab 2 7.0\n"
    "SCH-I739;Trend2\n"
    "SCH-I759;Infinite\n"
    "SCH-I800;Tab\n"
    "SCH-I815;Tab 7.7\n"
    "SCH-I829;Style Duos\n"
    "SCH-I869;Win\n"
    "SCH-I879;Grand\n"
    "SCH-i889;Note\n"
    "SCH-I905;Tab 10.1\n"
    "SCH-i909;S\n"
    "SCH-I915;Tab 2 10.1\n"
    "SCH-I92;Note 10.1\n"
    "SCH-i929;S2 Duos\n"
    "SCH-I93;S3\n"
    "SCH-I959;S4\n"
    "SCH-L710;S3\n"
    "SCH-M828C;Precedent\n"
    "SCH-N719;Note 2\n"
    "SCH-P709;Mega 5.8\n"
    "SCH-P729;Mega 6.3\n"
    "SCH-P739;Tab 8.9\n"
    "SCH-R53;S3\n"
    "SCH-R740;Discover\n"
    "SCH-R760;S2 Epic\n"
    "SCH-R760X;S2\n"
    "SCH-R820;Admire\n"
    "SCH-R830;Axiom\n"
    "SCH-R830C;Admire 2\n"
    "SCH-R890;S4 Mini\n"
    "SCH-R91;Indulge\n"
    "SCH-R920;Attain\n"
    "SCH-R930;Aviator\n"
    "SCH-R940;Lightray\n"
    "SCH-R950;Note 2\n"
    "SCH-R960;Mega 6.3\n"
    "SCH-R97;S4\n"
    "SCH-S720C;Proclaim\n"
    "SCH-S735;Discover\n"
    "SCH-S738;Centura\n"
    "SCH-S950C;S\n"
    "SCH-S96;S3\n"
    "SCL21;S3 Progre\n"
    "SCL22;Note 3\n"
    "SCL23;S5\n"
    "SCL24;Note Edge\n"
    "SCT21;Tab S 10.5\n"
    "SCT22;Tab S9 FE+ 5G\n"
    "SCV31;S6 Edge\n"
    "SCV32;A8\n"
    "SCV33;S7 Edge\n"
    "SCV34;Note7\n"
    "SCV35;S8+\n"
    "SCV36;S8\n"
    "SCV37;Note8\n"
    "SCV38;S9\n"
    "SCV39;S9+\n"
    "SCV40;Note9\n"
    "SCV41;S10\n"
    "SCV42;S10+\n"
    "SCV43;A30\n"
    "SCV44;Fold\n"
    "SCV45;Note10+\n"
    "SCV46;A20\n"
    "SCV47;Z Flip\n"
    "SCV48;A41\n"
    "SCV49;A21\n"
    "SGH-I257;S4 Mini\n"
    "SGH-I257M;S4 Mini\n"
    "SGH-I317;Note 2\n"
    "SGH-I337M;S4\n"
    "SGH-I407;Amp\n"
    "SGH-I467;Note 8.0\n"
    "SGH-I497;Tab 2 10.1\n"
    "SGH-I527;Mega 6.3\n"
    "SGH-I527M;Mega 6.3\n"
    "SGH-I537;S4 Active\n"
    "SGH-I547;Rugby Pro\n"
    "SGH-I547C;Rugby\n"
    "SGH-I577;Exhilarate\n"
    "SGH-I71;Note\n"
    "SGH-I717;Note\n"
    "SGH-I727;S2 Skyrocket\n"
    "SGH-I727R;S2 LTE\n"
    "SGH-I74;S3\n"
    "SGH-I747;S3\n"
    "SGH-I747Z;Pocket Neo\n"
    "SGH-I757M;S2 HD LTE\n"
    "SGH-I777;S2\n"
    "SGH-I827;Ace Q\n"
    "SGH-I896;Captivate\n"
    "SGH-I897;Captivate\n"
    "SGH-I927;Glide\n"
    "SGH-I95;Tab 8.9\n"
    "SGH-M819N;Mega 6.3\n"
    "SGH-M91;S4\n"
    "SGH-N037;Note7\n"
    "SGH-N075T;J\n"
    "SGH-S959G;S2\n"
    "SGH-S970G;S4\n"
    "SGH-T49;Mini\n"
    "SGH-T58;Q\n"
    "SGH-T59;Exhibit\n"
    "SGH-T679;Exhibit 2\n"
    "SGH-T679M;W\n"
    "SGH-T699;S BlazeQ\n"
    "SGH-T769;S Blaze\n"
    "SGH-T779;Tab 2 10.1\n"
    "SGH-T849;Tab\n"
    "SGH-T859;Tab 10.1\n"
    "SGH-T869;Tab 7.0 Plus\n"
    "SGH-T879;Note\n"
    "SGH-T88;Note 2\n"
    "SGH-T95;S Vibrant\n"
    "SGH-T959P;S Fascinate\n"
    "SGH-T989;S2\n"
    "SGH-T989D;S2 X\n"
    "SGH-T99;S3\n"
    "SHV-E110S;S2\n"
    "SHV-E12;S2 HD LTE\n"
    "SHV-E14;Tab 8.9\n"
    "SHV-E16;Note\n"
    "SHV-E17;R-Style\n"
    "SHV-E21;S3\n"
    "SHV-E220S;Pop\n"
    "SHV-E23;Note 10.1\n"
    "SHV-E25;Note 2\n"
    "SHV-E27;Grand\n"
    "SHV-E30;S4\n"
    "SHV-E31;Mega 6.3\n"
    "SHV-E33;S4\n"
    "SHV-E330S;S4 LTE-A\n"
    "SHV-E37;S4 Mini\n"
    "SHV-E40;Golden\n"
    "SHV-E470S;S4 Active\n"
    "SHV-E50;Win\n"
    "SHW-M100S;A\n"
    "SHW-M110S;S\n"
    "SHW-M130K;K\n"
    "SHW-M130L;U\n"
    "SHW-M18;Tab\n"
    "SHW-M190S;S\n"
    "SHW-M220L;Neo\n"
    "SHW-M240;Ace\n"
    "SHW-M25;S2\n"
    "SHW-M29;Gio\n"
    "SHW-M300W;Tab 10.1\n"
    "SHW-M305W;Tab 8.9\n"
    "SHW-M34;M Style\n"
    "SHW-M38;Tab 10.1\n"
    "SHW-M430W;Tab 7.0 Plus\n"
    "SHW-M440S;S3\n"
    "SHW-M48;Note 10.1\n"
    "SHW-M500;Note 8.0\n"
    "SHW-M570S;Core Advance\n"
    "SHW-M58;Core Safe\n"
    "SM-A013;A01 Core\n"
    "SM-A015;A01\n"
    "SM-A022;A02\n"
    "SM-A025;A02s\n"
    "SM-A032;A03 Core\n"
    "SM-A035;A03\n"
    "SM-A037;A03s\n"
    "SM-A042;A04e\n"
    "SM-A045;A04\n"
    "SM-A047;A04s\n"
    "SM-A055;A05\n"
    "SM-A057;A05s\n"
    "SM-A065;A06\n"
    "SM-A066;A06 5G\n"
    "SM-A102;A10e\n"
    "SM-A105;A10\n"
    "SM-A107;A10s\n"
    "SM-A115;A11\n"
    "SM-A125;A12\n"
    "SM-A127;A12\n"
    "SM-A135;A13\n"
    "SM-A136;A13 5G\n"
    "SM-A137;A13\n"
    "SM-A145;A14\n"
    "SM-A146;A14 5G\n"
    "SM-A155;A15\n"
    "SM-A156;A15 5G\n"
    "SM-A165;A16\n"
    "SM-A166;A16 5G\n"
    "SM-A202;A20e\n"
    "SM-A202K;Jean2\n"
    "SM-A205;A20\n"
    "SM-A205S;Wide 4\n"
    "SM-A207;A20s\n"
    "SM-A215;A21\n"
    "SM-A217;A21s\n"
    "SM-A225;A22\n"
    "SM-A226;A22 5G\n"
    "SM-A233;A23 5G\n"
    "SM-A235;A23\n"
    "SM-A236;A23 5G\n"
    "SM-A245;A24\n"
    "SM-A253;A25 5G\n"
    "SM-A256;A25 5G\n"
    "SM-A260;A2 Core\n"
    "SM-A266;A26 5G\n"
    "SM-A300;A3\n"
    "SM-A305;A30\n"
    "SM-A307;A30s\n"
    "SM-A310;A3 (2016)\n"
    "SM-A315;A31\n"
    "SM-A320;A3 (2017)\n"
    "SM-A325;A32\n"
    "SM-A326;A32 5G\n"
    "SM-A336;A33 5G\n"
    "SM-A346;A34 5G\n"
    "SM-A356;A35 5G\n"
    "SM-A366;A36 5G\n"
    "SM-A405;A40\n"
    "SM-A415;A41\n"
    "SM-A426;A42 5G\n"
    "SM-A500;A5\n"
    "SM-A505;A50\n"
    "SM-A507;A50s\n"
    "SM-A510;A5 (2016)\n"
    "SM-A515;A51\n"
    "SM-A516;A51 5G\n"
    "SM-A520;A5 (2017)\n"
    "SM-A525;A52\n"
    "SM-A526;A52 5G\n"
    "SM-A528;A52s 5G\n"
    "SM-A530;A8 (2018)\n"
    "SM-A536;A53 5G\n"
    "SM-A556;A55 5G\n"
    "SM-A566;A56 5G\n"
    "SM-A600;A6\n"
    "SM-A605;A6+\n"
    "SM-A606;A60\n"
    "SM-A700;A7\n"
    "SM-A705;A70\n"
    "SM-A707;A70s\n"
    "SM-A710;A7 (2016)\n"
    "SM-A715;A71\n"
    "SM-A716;A71 5G\n"
    "SM-A720;A7 (2017)\n"
    "SM-A725;A72\n"
    "SM-A730;A8+ (2018)\n"
    "SM-A736;A73 5G\n"
    "SM-A750;A7 (2018)\n"
    "SM-A800;A8\n"
    "SM-A805;A80\n"
    "SM-A810;A8 (2016)\n"
    "SM-A900;A9 (2016)\n"
    "SM-A908;A90 5G\n"
    "SM-A910;A9 Pro\n"
    "SM-A920;A9 (2018)\n"
    "SM-C101;S4 Zoom\n"
    "SM-C105;S4 Zoom\n"
    "SM-C111;K Zoom\n"
    "SM-C115;K Zoom\n"
    "SM-C500;C5\n"
    "SM-C501;C5 Pro\n"
    "SM-C5560;C55 5G\n"
    "SM-C700;C7\n"
    "SM-C701;C7 Pro\n"
    "SM-C710;C8\n"
    "SM-C710;J7+\n"
    "SM-C900;C9 Pro\n"
    "SM-E025;F02s\n"
    "SM-E045;F04\n"
    "SM-E055;F05\n"
    "SM-E066;F06 5G\n"
    "SM-E135;F13\n"
    "SM-E145;F14\n"
    "SM-E146;F14 5G\n"
    "SM-E156;F15 5G\n"
    "SM-E166;F16 5G\n"
    "SM-E225;F22\n"
    "SM-E236;F23 5G\n"
    "SM-E346;F34 5G\n"
    "SM-E366;F36 5G\n"
    "SM-E426;F42 5G\n"
    "SM-E426S;Wide 5\n"
    "SM-E500;E5\n"
    "SM-E526;F52 5G\n"
    "SM-E546;F54 5G\n"
    "SM-E556;F55 5G\n"
    "SM-E625;F62\n"
    "SM-E700;E7\n"
    "SM-F127;F12\n"
    "SM-F415;F41\n"
    "SM-F700;Z Flip\n"
    "SM-F707;Z Flip 5G\n"
    "SM-F711;Z Flip3 5G\n"
    "SM-F721;Z Flip4\n"
    "SM-F731;Z Flip5\n"
    "SM-F741;Z Flip6\n"
    "SM-F761;Z Flip7 FE\n"
    "SM-F766;Z Flip7\n"
    "SM-F900;Fold\n"
    "SM-F907;Fold 5G\n"
    "SM-F916;Z Fold2 5G\n"
    "SM-F926;Z Fold3 5G\n"
    "SM-F936;Z Fold4\n"
    "SM-F946;Z Fold5\n"
    "SM-F956;Z Fold6\n"
    "SM-F958;Z Fold Special Edition\n"
    "SM-F966;Z Fold7\n"
    "SM-G110;Pocket 2\n"
    "SM-G130;Young 2\n"
    "SM-G150;Folder\n"
    "SM-G155;Folder\n"
    "SM-G160;Folder2\n"
    "SM-G165;Folder2\n"
    "SM-G165N;Elite\n"
    "SM-G310;Ace\n"
    "SM-G313;Ace 4\n"
    "SM-G313HZ;S Duos 3\n"
    "SM-G316;Ace 4\n"
    "SM-G316ML;Ace 4 Neo Duos\n"
    "SM-G318;Ace 4 Lite\n"
    "SM-G350;Core Plus\n"
    "SM-G350;Star 2 Plus\n"
    "SM-G351;Core LTE\n"
    "SM-G355;Core 2\n"
    "SM-G357;Ace\n"
    "SM-G358;Core Lite\n"
    "SM-G360;Core Prime\n"
    "SM-G361;Core Prime\n"
    "SM-G381;Win Pro\n"
    "SM-G3812B;S3 Slim\n"
    "SM-G3815;Express 2\n"
    "SM-G386;Core LTE\n"
    "SM-G386;Core\n"
    "SM-G388;Xcover 3\n"
    "SM-G389;Xcover 3\n"
    "SM-G390;Xcover 4\n"
    "SM-G398FN;XCover 4s\n"
    "SM-G5108;Core Max\n"
    "SM-G5108Q;Core Max Duos\n"
    "SM-G525;XCover 5\n"
    "SM-G530;Grand Prime\n"
    "SM-G531;Grand Prime\n"
    "SM-G532;J2 Prime\n"
    "SM-G550;On5\n"
    "SM-G550FY;On5 Pro\n"
    "SM-G5510;On5 (2016)\n"
    "SM-G552;On5 (2016)\n"
    "SM-G556B;XCover7\n"
    "SM-G570;J5 Prime\n"
    "SM-G5700;On5 (2016)\n"
    "SM-G600;On7\n"
    "SM-G600FY;On7 Pro\n"
    "SM-G600S;Wide\n"
    "SM-G610;J7 Prime\n"
    "SM-G610F;On Nxt\n"
    "SM-G611;J7 Prime 2\n"
    "SM-G615F;J7 Max\n"
    "SM-G615FU;On Max\n"
    "SM-G710;Grand2\n"
    "SM-G715;XCover Pro\n"
    "SM-G720;Grand Max\n"
    "SM-G730;S3 Mini\n"
    "SM-G730A;S3 Mini\n"
    "SM-G736;Xcover 6 Pro\n"
    "SM-G750;Mega 2\n"
    "SM-G766;Xcover 7 Pro\n"
    "SM-G770;S10 Lite\n"
    "SM-G780;S20 FE\n"
    "SM-G781;S20 FE 5G\n"
    "SM-G800;S5 Mini\n"
    "SM-G850;Alpha\n"
    "SM-G860P;S5 K Sport\n"
    "SM-G870;S5 Active\n"
    "SM-G875;S\n"
    "SM-G885;A8 Star\n"
    "SM-G887;A8s\n"
    "SM-G889;XCover Field Pro\n"
    "SM-G890;S6 Active\n"
    "SM-G891;S7 Active\n"
    "SM-G892;S8 Active\n"
    "SM-G900;S5\n"
    "SM-G901;S5 LTE-A\n"
    "SM-G903;S5 Neo\n"
    "SM-G906;S5\n"
    "SM-G910;Round\n"
    "SM-G920;S6\n"
    "SM-G925;S6 Edge\n"
    "SM-G928;S6 Edge+\n"
    "SM-G930;S7\n"
    "SM-G935;S7 Edge\n"
    "SM-G950;S8\n"
    "SM-G955;S8+\n"
    "SM-G960;S9\n"
    "SM-G965;S9+\n"
    "SM-G970;S10e\n"
    "SM-G973;S10\n"
    "SM-G975;S10+\n"
    "SM-G977;S10 5G\n"
    "SM-G980;S20\n"
    "SM-G981;S20 5G\n"
    "SM-G985;S20+\n"
    "SM-G986;S20+ 5G\n"
    "SM-G988;S20 Ultra 5G\n"
    "SM-G990;S21 FE 5G\n"
    "SM-G991;S21 5G\n"
    "SM-G996;S21+ 5G\n"
    "SM-G998;S21 Ultra 5G\n"
    "SM-J100;J1\n"
    "SM-J105;J1 Mini\n"
    "SM-J106;J1 Mini Prime\n"
    "SM-J110;J1 Ace\n"
    "SM-J111;J1 Ace\n"
    "SM-J120;J1\n"
    "SM-J200;J2\n"
    "SM-J210;J2 (2016)\n"
    "SM-J250;J2 Pro\n"
    "SM-J260;J2 Core\n"
    "SM-J260AZ;J2 Pure\n"
    "SM-J260T1;J2 (2018)\n"
    "SM-J3109;J3\n"
    "SM-J311;J3 Pro\n"
    "SM-J320;J3\n"
    "SM-J327;J3\n"
    "SM-J327P;J3 Emerge\n"
    "SM-J327V;J3 Eclipse\n"
    "SM-J327VPP;J3 Mission\n"
    "SM-J330;J3\n"
    "SM-J330G;J3 Pro\n"
    "SM-J337;J3\n"
    "SM-J337P;J3 Achieve\n"
    "SM-J337R4;J3 Aura\n"
    "SM-J337T;J3 Star\n"
    "SM-J337V;J3 V\n"
    "SM-J337W;J3 (2018)\n"
    "SM-J400;J4\n"
    "SM-J410;J4 Core\n"
    "SM-J415;J4+\n"
    "SM-J500;J5\n"
    "SM-J510;J5 (2016)\n"
    "SM-J530;J5\n"
    "SM-J600;J6\n"
    "SM-J600GF;On6\n"
    "SM-J610;J6+\n"
    "SM-J700;J7\n"
    "SM-J701;J7 Neo\n"
    "SM-J710;J7 (2016)\n"
    "SM-J710FN;On8\n"
    "SM-J720;J7 Duo\n"
    "SM-J727;J7\n"
    "SM-J727P;J7 Perx\n"
    "SM-J727S;Wide 2\n"
    "SM-J727V;J7 V\n"
    "SM-J730;J7\n"
    "SM-J737;J7\n"
    "SM-J737R4;J7 Aura\n"
    "SM-J737S;Wide 3\n"
    "SM-J737V;J7 V\n"
    "SM-J810;J8\n"
    "SM-J810GF;On8\n"
    "SM-L300;Watch7\n"
    "SM-L305;Watch7\n"
    "SM-L310;Watch7\n"
    "SM-L315;Watch7\n"
    "SM-L320;Watch8\n"
    "SM-L325;Watch8\n"
    "SM-L330;Watch8\n"
    "SM-L335;Watch8\n"
    "SM-L500;Watch8 Classic\n"
    "SM-L505U;Watch8 Classic\n"
    "SM-L700;Watch Ultra\n"
    "SM-L705;Watch Ultra\n"
    "SM-M013;M01 Core\n"
    "SM-M015;M01\n"
    "SM-M017;M01s\n"
    "SM-M022;M02\n"
    "SM-M025;M02s\n"
    "SM-M045;M04\n"
    "SM-M055;M05\n"
    "SM-M066;M06 5G\n"
    "SM-M105;M10\n"
    "SM-M107;M10s\n"
    "SM-M115;M11\n"
    "SM-M127;M12\n"
    "SM-M135;M13\n"
    "SM-M136;M13 5G\n"
    "SM-M145;M14\n"
    "SM-M146;M14 5G\n"
    "SM-M156;M15 5G\n"
    "SM-M156S;Wide 7\n"
    "SM-M166;M16 5G\n"
    "SM-M166S;Wide 8\n"
    "SM-M205;M20\n"
    "SM-M215;M21\n"
    "SM-M215G;M21 (2021)\n"
    "SM-M225;M22\n"
    "SM-M236;M23 5G\n"
    "SM-M305;M30\n"
    "SM-M307;M30s\n"
    "SM-M315;M31\n"
    "SM-M317;M31s\n"
    "SM-M325;M32\n"
    "SM-M326;M32 5G\n"
    "SM-M336;M33 5G\n"
    "SM-M336K;Jump 2\n"
    "SM-M346;M34 5G\n"
    "SM-M356;M35 5G\n"
    "SM-M366;M36 5G\n"
    "SM-M366K;Jump 4\n"
    "SM-M405;M40\n"
    "SM-M426;M42 5G\n"
    "SM-M446;Jump 3\n"
    "SM-M515;M51\n"
    "SM-M526;M52 5G\n"
    "SM-M536;M53 5G\n"
    "SM-M546;M54 5G\n"
    "SM-M556;M55 5G\n"
    "SM-M558;M55s 5G\n"
    "SM-M566;M56 5G\n"
    "SM-M625;M62\n"
    "SM-N750;Note 3 Neo\n"
    "SM-N770;Note10 Lite\n"
    "SM-N900;Note 3\n"
    "SM-N910;Note 4\n"
    "SM-N915;NoteEdge\n"
    "SM-N916;Note 4\n"
    "SM-N920;Note5\n"
    "SM-N930;Note7\n"
    "SM-N935;Note Fan Edition\n"
    "SM-N950;Note8\n"
    "SM-N960;Note9\n"
    "SM-N970;Note10\n"
    "SM-N971;Note10 5G\n"
    "SM-N975;Note10+\n"
    "SM-N976;Note10+ 5G\n"
    "SM-N980;Note20\n"
    "SM-N981;Note20 5G\n"
    "SM-N985;Note20 Ultra\n"
    "SM-N986;Note20 Ultra 5G\n"
    "SM-P200;Tab A with S Pen\n"
    "SM-P205;Tab A with S Pen\n"
    "SM-P350;Tab A 8.0\n"
    "SM-P355;Tab A 8.0\n"
    "SM-P550;Tab A 9.7\n"
    "SM-P555;Tab A 9.7\n"
    "SM-P580;Tab A (2016) with S Pen\n"
    "SM-P583;Tab A (2016) with S Pen\n"
    "SM-P585;Tab A (2016) with S Pen\n"
    "SM-P587;Tab A (2016) with S Pen\n"
    "SM-P588;Tab A (2016) with S Pen\n"
    "SM-P600;Note 10.1 (2014)\n"
    "SM-P601;Note 10.1\n"
    "SM-P602;Note 10.1\n"
    "SM-P605;Note 10.1\n"
    "SM-P607;Note 10.1 (2014)\n"
    "SM-P610;Tab S6 Lite\n"
    "SM-P613;Tab S6 Lite\n"
    "SM-P615;Tab S6 Lite\n"
    "SM-P617;Tab S6 Lite\n"
    "SM-P619;Tab S6 Lite\n"
    "SM-P620;Tab S6 Lite\n"
    "SM-P625;Tab S6 Lite\n"
    "SM-P900;Note Pro\n"
    "SM-P901;Note Pro 12.2\n"
    "SM-P905;Note Pro 12.2\n"
    "SM-P907;Note Pro 12.2\n"
    "SM-R860;Watch4\n"
    "SM-R861;Watch FE\n"
    "SM-R865;Watch4\n"
    "SM-R866;Watch FE\n"
    "SM-R870;Watch4\n"
    "SM-R875;Watch4\n"
    "SM-R880;Watch4 Classic\n"
    "SM-R885;Watch4 Classic\n"
    "SM-R890;Watch4 Classic\n"
    "SM-R895;Watch4 Classic\n"
    "SM-R900;Watch5\n"
    "SM-R905;Watch5\n"
    "SM-R910;Watch5\n"
    "SM-R915;Watch5\n"
    "SM-R920;Watch5 Pro\n"
    "SM-R925;Watch5 Pro\n"
    "SM-R930;Watch6\n"
    "SM-R935;Watch6\n"
    "SM-R940;Watch6\n"
    "SM-R945;Watch6\n"
    "SM-R950;Watch6 Classic\n"
    "SM-R955;Watch6 Classic\n"
    "SM-R960;Watch6 Classic\n"
    "SM-R965;Watch6 Classic\n"
    "SM-S111;A01\n"
    "SM-S124;A02s\n"
    "SM-S134;A03s\n"
    "SM-S236;A23 5G\n"
    "SM-S237;A23 5G\n"
    "SM-S260;J2\n"
    "SM-S320;J3 (2016)\n"
    "SM-S327;J3 Pop\n"
    "SM-S337;J3 Pop\n"
    "SM-S357;J3 Orbit\n"
    "SM-S366;A36 5G\n"
    "SM-S367;J3 Orbit\n"
    "SM-S426;A42 5G\n"
    "SM-S506;A50\n"
    "SM-S536;A53 5G\n"
    "SM-S550TL;On5\n"
    "SM-S711;S23 FE\n"
    "SM-S721;S24 FE\n"
    "SM-S727VL;J7 Pop\n"
    "SM-S737TL;J7 Sky Pro\n"
    "SM-S757BL;J7 Crown\n"
    "SM-S765;Ace Style\n"
    "SM-S766;Ace Style\n"
    "SM-S767VL;J7 Crown\n"
    "SM-S777C;J1\n"
    "SM-S820;Core Prime\n"
    "SM-S901;S22\n"
    "SM-S903VL;S5\n"
    "SM-S906;S22+\n"
    "SM-S906L;S6\n"
    "SM-S907VL;S6\n"
    "SM-S908;S22 Ultra\n"
    "SM-S911;S23\n"
    "SM-S916;S23+\n"
    "SM-S918;S23 Ultra\n"
    "SM-S920L;Grand Prime\n"
    "SM-S921;S24\n"
    "SM-S926;S24+\n"
    "SM-S928;S24 Ultra\n"
    "SM-S931;S25\n"
    "SM-S936;S25+\n"
    "SM-S937;S25 Edge\n"
    "SM-S938;S25 Ultra\n"
    "SM-S975L;S4\n"
    "SM-S978;E5\n"
    "SM-T110;Tab 3 Lite\n"
    "SM-T111;Tab 3 Lite\n"
    "SM-T113;Tab 3 Lite 7.0\n"
    "SM-T113NU;Tab 3 V 7.0\n"
    "SM-T116;Tab 3 VE 7.0\n"
    "SM-T116BU;Tab Plus 7.0\n"
    "SM-T116IR;Tab 3 Lite\n"
    "SM-T116NQ;Tab 3 Lite 7.0\n"
    "SM-T116NU;Tab 3 V 7.0\n"
    "SM-T116NY;Tab 3 V 7.0\n"
    "SM-T210;Tab 3 7.0\n"
    "SM-T2105;Tab 3 Kids\n"
    "SM-T210X;Tab 3 8.0\n"
    "SM-T211;Tab 3 7.0\n"
    "SM-T212;Tab 3 7.0\n"
    "SM-T215;Tab 3 7.0\n"
    "SM-T217;Tab 3 7.0\n"
    "SM-T220;Tab A7 Lite\n"
    "SM-T225;Tab A7 Lite\n"
    "SM-T227;Tab A7 Lite\n"
    "SM-T230;Tab 4 7.0\n"
    "SM-T231;Tab 4 7.0\n"
    "SM-T232;Tab 4 7.0\n"
    "SM-T235;Tab 4 7.0\n"
    "SM-T237;Tab 4 7.0\n"
    "SM-T239;Tab 4 7.0\n"
    "SM-T2519;Tab Q\n"
    "SM-T255S;W\n"
    "SM-T280;Tab A 7.0\n"
    "SM-T285;Tab A 7.0\n"
    "SM-T287;Tab A 7.0\n"
    "SM-T290;Tab A Kids Edition\n"
    "SM-T310;Tab 3 8.0\n"
    "SM-T311;Tab 3 8.0\n"
    "SM-T312;Tab 3 8.0\n"
    "SM-T315;Tab 3 8.0\n"
    "SM-T320;Tab Pro 8.4\n"
    "SM-T325;Tab Pro 8.4\n"
    "SM-T330;Tab 4 8.0\n"
    "SM-T331;Tab 4 8.0\n"
    "SM-T335;Tab 4 8.0\n"
    "SM-T337;Tab 4 8.0\n"
    "SM-T350;Tab A 8.0\n"
    "SM-T355;Tab A 8.0\n"
    "SM-T357;Tab A 8.0\n"
    "SM-T360;Tab A\n"
    "SM-T365;Tab A\n"
    "SM-T375;Tab E 8.0\n"
    "SM-T377;Tab E 8.0\n"
    "SM-T378;Tab E 8.0\n"
    "SM-T380;Tab A (2017)\n"
    "SM-T385;Tab A (2017)\n"
    "SM-T387;Tab A 8.0\n"
    "SM-T390;Tab Active 2\n"
    "SM-T395;Tab Active 2\n"
    "SM-T397;Tab Active 2\n"
    "SM-T500;Tab A7\n"
    "SM-T503;Tab A7\n"
    "SM-T505;Tab A7\n"
    "SM-T507;Tab A7\n"
    "SM-T509;Tab A7\n"
    "SM-T510;Tab A\n"
    "SM-T515;Tab A\n"
    "SM-T517;Tab A\n"
    "SM-T520;Tab Pro 10.1\n"
    "SM-T525;Tab Pro 10.1\n"
    "SM-T530;Tab 4 10.1\n"
    "SM-T531;Tab 4 10.0\n"
    "SM-T533;Tab 4 10.1\n"
    "SM-T535;Tab 4 10.0\n"
    "SM-T536;Tab 4 10.1\n"
    "SM-T537;Tab 4 10.0\n"
    "SM-T540;Tab Active Pro\n"
    "SM-T545;Tab Active Pro\n"
    "SM-T547;Tab Active Pro\n"
    "SM-T550;Tab A 9.7\n"
    "SM-T555;Tab A 9.7\n"
    "SM-T560;Tab E 9.6\n"
    "SM-T561;Tab E 9.6\n"
    "SM-T562;Tab E 9.6\n"
    "SM-T567;Tab E 9.6\n"
    "SM-T575;Tab Active 3\n"
    "SM-T577;Tab Active 3\n"
    "SM-T580;Tab A 10.1\n"
    "SM-T583;Tab Advanced 2\n"
    "SM-T585;Tab A 10.1\n"
    "SM-T587;Tab A 10.1\n"
    "SM-T590;Tab A 10.5 (2018)\n"
    "SM-T595;Tab A 10.5 (2018)\n"
    "SM-T597;Tab A 10.5 (2018)\n"
    "SM-T636;Tab Active 4 Pro 5G\n"
    "SM-T638;Tab Active 4 Pro 5G\n"
    "SM-T670;View\n"
    "SM-T677;View\n"
    "SM-T700;Tab S 8.4\n"
    "SM-T705;Tab S 8.4\n"
    "SM-T707;Tab S 8.4\n"
    "SM-T710;Tab S2 8.0\n"
    "SM-T713;Tab S2 8.0\n"
    "SM-T715;Tab S2 8.0\n"
    "SM-T719;Tab S2\n"
    "SM-T720;Tab S5e\n"
    "SM-T725;Tab S5e\n"
    "SM-T727;Tab S5e\n"
    "SM-T730;Tab S7 FE\n"
    "SM-T733;Tab S7 FE\n"
    "SM-T735;Tab S7 FE\n"
    "SM-T736;Tab S7 FE 5G\n"
    "SM-T737;Tab S7 FE\n"
    "SM-T738;Tab S7 FE 5G\n"
    "SM-T800;Tab S 10.5\n"
    "SM-T805;Tab S 10.5\n"
    "SM-T807;Tab S 10.5\n"
    "SM-T810;Tab S2 9.7\n"
    "SM-T813;Tab S2 9.7\n"
    "SM-T815;Tab S2 9.7\n"
    "SM-T817;Tab S2 9.7\n"
    "SM-T818;Tab S2 9.7\n"
    "SM-T819;Tab S2\n"
    "SM-T820;Tab S3\n"
    "SM-T825;Tab S3\n"
    "SM-T827;Tab S3\n"
    "SM-T830;Tab S4\n"
    "SM-T835;Tab S4\n"
    "SM-T837;Tab S4\n"
    "SM-T860;Tab S6\n"
    "SM-T865;Tab S6\n"
    "SM-T866;Tab S6 5G\n"
    "SM-T867;Tab S6\n"
    "SM-T870;Tab S7\n"
    "SM-T875;Tab S7\n"
    "SM-T878;Tab S7 5G\n"
    "SM-T900;Tab Pro 12.2\n"
    "SM-T905;Tab Pro 12.2\n"
    "SM-T927;View 2\n"
    "SM-T970;Tab S7+\n"
    "SM-T975;Tab S7+\n"
    "SM-T976;Tab S7+ 5G\n"
    "SM-T978;Tab S7+ 5G\n"
    "SMT-i9100;Tab\n"
    "SM-W2015;Golden 2\n"
    "SM-X110;Tab A9\n"
    "SM-X115;Tab A9\n"
    "SM-X117;Tab A9\n"
    "SM-X200;Tab A8\n"
    "SM-X205;Tab A8\n"
    "SM-X207;Tab A8\n"
    "SM-X210;Tab A9+\n"
    "SM-X216;Tab A9+ 5G\n"
    "SM-X218;Tab A9+ 5G\n"
    "SM-X300;Tab Active 5\n"
    "SM-X306;Tab Active 5 5G\n"
    "SM-X308;Tab Active 5 5G\n"
    "SM-X350;Tab Active 5 Pro\n"
    "SM-X356;Tab Active 5 Pro 5G\n"
    "SM-X358;Tab Active 5 Pro 5G\n"
    "SM-X510;Tab S9 FE\n"
    "SM-X516;Tab S9 FE 5G\n"
    "SM-X518;Tab S9 FE 5G\n"
    "SM-X520;Tab S10 FE\n"
    "SM-X526;Tab S10 FE 5G\n"
    "SM-X528;Tab S10 FE 5G\n"
    "SM-X610;Tab S9 FE+\n"
    "SM-X616;Tab S9 FE+ 5G\n"
    "SM-X620;Tab S10 FE+\n"
    "SM-X626;Tab S10 FE+ 5G\n"
    "SM-X700;Tab S8\n"
    "SM-X706;Tab S8 5G\n"
    "SM-X710;Tab S9\n"
    "SM-X716;Tab S9 5G\n"
    "SM-X800;Tab S8+\n"
    "SM-X806;Tab S8+ 5G\n"
    "SM-X808;Tab S8+ 5G\n"
    "SM-X810;Tab S9+\n"
    "SM-X816;Tab S9+ 5G\n"
    "SM-X818;Tab S9+ 5G\n"
    "SM-X820;Tab S10+\n"
    "SM-X826;Tab S10+ 5G\n"
    "SM-X828;Tab S10+ 5G\n"
    "SM-X900;Tab S8 Ultra\n"
    "SM-X906;Tab S8 Ultra 5G\n"
    "SM-X910;Tab S9 Ultra\n"
    "SM-X916;Tab S9 Ultra 5G\n"
    "SM-X920;Tab S10 Ultra\n"
    "SM-X926;Tab S10 Ultra 5G\n"
    "SPH-D70;S Epic\n"
    "SPH-D71;S2 Epic\n"
    "SPH-L300;Victory\n"
    "SPH-L520;S4 Mini\n"
    "SPH-L600;Mega 6.3\n"
    "SPH-L71;S3\n"
    "SPH-L72;S4\n"
    "SPH-L900;Note 2\n"
    "SPH-M820;Prevail\n"
    "SPH-M830;Rush\n"
    "SPH-M840;Prevail 2\n"
    "SPH-M950;Reverb\n"
    "SPH-P100;Tab 7.0\n"
    "SPH-P500;Tab 2 10.1\n"
    "SPH-P600;Note 10.1\n"
    "YP-G1;Player 4.0\n"
    "YP-G50;Player 50\n"
    "YP-G70;Player 5\n"
    "YP-GB1;Player 4\n"
    "YP-GB70;Player\n"
    "YP-GB70D;Player 70 Plus\n"
    "YP-GI1;Player 4.2\n"
    "YP-GI2;070\n"
    "YP-GP1;Player 5.8\n"
    "YP-GS1;Player 3.6\n"
;

static const char* Model_Name_Xiaomi =
    "21051182C;Pad 5\n"
    "21051182G;Pad 5\n"
    "2106118C;MIX 4\n"
    "2107113SG;11T Pro\n"
    "2107113SI;11T Pro\n"
    "2107113SR;11T Pro\n"
    "2107119DC;Mi 11 LE\n"
    "21081111RG;11T\n"
    "21091116I;11i\n"
    "21091116UI;11i HyperCharge\n"
    "2109119BC;Civi\n"
    "2109119DG;11 Lite 5G NE\n"
    "2109119DI;11 Lite NE\n"
    "2112123AC;12X\n"
    "2201122C;12 Pro\n"
    "2201122G;12 Pro\n"
    "2201123C;12\n"
    "2201123G;12\n"
    "2203121C;12S Ultra\n"
    "2203129G;12 Lite\n"
    "22061218C;MIX Fold 2\n"
    "2206122SC;12S Pro\n"
    "2206123SC;12S\n"
    "2207117BPG;POCO M5s\n"
    "22071212AG;12T\n"
    "2207122MC;12 Pro Dimensity\n"
    "22081212C;12T Pro\n"
    "22081212R;12T Pro\n"
    "22081212UG;12T Pro\n"
    "22081281AC;Pad 5 Pro\n"
    "2209129SC;Civi 2\n"
    "2210129SG;13 Lite\n"
    "2210132C;13 pro\n"
    "2210132G;13 Pro\n"
    "2211133C;13\n"
    "2211133G;13\n"
    "22200414R;12T Pro\n"
    "23043RP34C;Pad 6\n"
    "23043RP34G;Pad 6\n"
    "23043RP34G;pad 6\n"
    "23043RP34I;Pad 6\n"
    "23046PNC9C;civi 3\n"
    "23046RP50C;pad 6 Pro\n"
    "2304FPN6DC;13 Ultra\n"
    "2304FPN6DG;13 Ultra\n"
    "2306EPN60G;13T\n"
    "23078PND5G;13T Pro\n"
    "2307BRPDCC;Pad 6 Max 14\n"
    "23088PND5R;13T Pro\n"
    "2308CPXD0C;MIX Fold 3\n"
    "23116PN5BC;14 Pro\n"
    "2311BPN23C;14 Pro\n"
    "23127PN0CC;14\n"
    "23127PN0CG;14\n"
    "24018RPACG;Pad 6S Pro 12.4\n"
    "24030PN60G;14 Ultra\n"
    "24031PN0DC;14 Ultra\n"
    "24053PY09C;Civi 4\n"
    "24053PY09I;14 Civi\n"
    "2405CPX3DC;MIX Flip\n"
    "2405CPX3DG;MIX Flip\n"
    "2406APNFAG;14T\n"
    "24072PX77C;MIX Fold 4\n"
    "2407FPN8EG;14T Pro\n"
    "2407FPN8ER;14T Pro\n"
    "24091RPADC;Pad 7 Pro\n"
    "24091RPADG;Pad 7 Pro\n"
    "2410CRP4CC;Pad 7\n"
    "2410CRP4CG;Pad 7\n"
    "2410CRP4CI;Pad 7\n"
    "2410DPN6CC;15 Pro\n"
    "24117RK2CG;POCO F7 Pro\n"
    "24129PN74C;15\n"
    "24129PN74G;15\n"
    "24129PN74I;15\n"
    "25010PN30C;15 Ultra\n"
    "25010PN30G;15 Ultra\n"
    "25010PN30I;15 Ultra\n"
    "25019PNF3C;15 Ultra\n"
    "25032RP42C;Pad 7 Ultra\n"
    "25042PN24C;15S Pro\n"
    "A201XM;12T Pro\n"
    "A301XM;13T Pro\n"
    "A402XM;14T Pro\n"
    "M2002J9E;Mi 10 Lite zoom\n"
    "M2002J9G;Mi 10 lite 5G\n"
    "M2002J9S;Mi 10 lite 5G\n"
    "M2007J17G;Mi 10T Lite\n"
    "M2007J17I;Mi 10i\n"
    "M2007J1SC;Mi 10 Ultra\n"
    "M2007J3SG;Mi 10T Pro\n"
    "M2007J3SI;Mi 10T pro\n"
    "M2007J3SP;Mi 10T\n"
    "M2007J3SY;Mi 10T\n"
    "M2011J18C;Xiaomi MIX Fold\n"
    "M2011K2C;Mi 11\n"
    "M2011K2G;Mi 11\n"
    "M2012K11G;Mi 11i\n"
    "M2012K11I;Mi 11X Pro\n"
    "M2101K9AG;Mi 11 Lite\n"
    "M2101K9AI;Mi 11 Lite\n"
    "M2101K9C;Mi 11 Lite 5G\n"
    "M2101K9G;Mi 11 Lite 5G\n"
    "M2102J2SC;Mi 10S\n"
    "M2102K1AC;Mi 11 Pro\n"
    "M2102K1C;Mi 11 Ultra\n"
    "M2102K1G;Mi 11 Ultra\n"
    "M2105K81AC;Pad 5 Pro\n"
    "M2105K81C;Pad 5 Pro 5G\n"
    "XIG01;Mi 10 Lite 5G\n"
    "XIG04;13T\n"
    "XIG07;14T\n"
;

static const FindReplaceCompany_struct Model_Name[] = {
    { "Sony", Model_Name_Sony, "Xperia " },
    { "Sony Ericsson", Model_Name_Sony_Ericsson, "Xperia " },
    { "Samsung", Model_Name_Samsung, "Galaxy " },
    { "Xiaomi", Model_Name_Xiaomi, {} },
};

//---------------------------------------------------------------------------
static const char* VersionPrefixes =
    ", VERSION:\n"
    "FILE VERSION\n"
    "RELEASE\n"
    "V\n"
    "VER\n"
    "VERSION\n"
;

//---------------------------------------------------------------------------
extern MediaInfo_Config Config;
extern size_t DolbyVision_Compatibility_Pos(const Ztring& Value);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
Ztring File__Analyze_Encoded_Library_String (const Ztring &CompanyName, const Ztring &Name, const Ztring &Version, const Ztring &Date, const Ztring &Encoded_Library)
{
    if (!Name.empty())
    {
        Ztring String;
        if (!CompanyName.empty())
        {
            String+=CompanyName;
            String+=__T(" ");
        }
        String+=Name;
        if (!Version.empty())
        {
            String+=__T(" ");
            String+=Version;
        }
        if (!Date.empty())
        {
            String+=__T(" (");
            String+=Date;
            String+=__T(")");
        }
        return String;
    }
    else
        return Encoded_Library;
}

//---------------------------------------------------------------------------
void Merge_FillTimeCode(File__Analyze& In, const string& Prefix, const TimeCode& TC_Time, float FramesPerSecondF, bool DropFrame, TimeCode::rounding Rounding=TimeCode::Nearest, int32u Frequency=0)
{
    if (!TC_Time.IsSet())
        return;
    auto FramesPerSecondI=float32_int32s(FramesPerSecondF);
    TimeCode TC_Frames;
    string TC_WithExtraSamples_String;
    if (FramesPerSecondI)
    {
        TC_Frames=TC_Time.ToRescaled(FramesPerSecondI-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF), Rounding);
        bool IsRounded=false;
        if (Rounding==TimeCode::Floor)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames1=TC_Frames+1;
            TimeCode TC_Frames1_InTime=TC_Frames1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Floor);
            if (TC_Time==TC_Frames1_InTime)
            {
                TC_Frames=TC_Frames1;
                IsRounded=true;
            }
        }
        if (Rounding==TimeCode::Ceil)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames1=TC_Frames-1;
            TimeCode TC_Frames1_InTime=TC_Frames1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Ceil);
            if (TC_Time==TC_Frames1_InTime)
            {
                TC_Frames=TC_Frames1;
                IsRounded=true;
            }
        }
        TC_WithExtraSamples_String=TC_Frames.ToString();

        if (Frequency)
        {
            // With samples
            int64_t Samples;
            if (IsRounded)
                Samples=0;
            else
            {
                TimeCode TC_Frames_InSamples=TC_Frames.ToRescaled(Frequency-1, TimeCode::flags(), Rounding);
                TimeCode TC_Time_Samples=TC_Time.ToRescaled(Frequency-1, TimeCode::flags(), Rounding);
                TimeCode TC_ExtraSamples;
                if (Rounding==TimeCode::Ceil)
                    TC_ExtraSamples=TC_Frames_InSamples-TC_Time_Samples;
                else
                    TC_ExtraSamples=TC_Time_Samples-TC_Frames_InSamples;
                Samples=TC_ExtraSamples.ToFrames();
            }
            if (Samples)
            {
                if (Samples>=0)
                    TC_WithExtraSamples_String+=Rounding==TimeCode::Ceil?'-':'+';
                TC_WithExtraSamples_String+=std::to_string(Samples);
                TC_WithExtraSamples_String+="samples";
            }
        }
    }

    if (Prefix.find("TimeCode")!=string::npos)
    {
        In.Fill(Stream_Audio, 0, Prefix.c_str(), TC_WithExtraSamples_String, true, true);
        return;
    }

    string TC_WithExtraSubFrames_String;
    if (FramesPerSecondI)
    {
        // With subframes
        constexpr TimeCode::rounding TC_Frames_Sub_Rounding=TimeCode::Ceil;
        TimeCode TC_Frames_Sub=TC_Time.ToRescaled(FramesPerSecondI*100-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF), TC_Frames_Sub_Rounding);
        bool IsRounded=false;
        if (TC_Frames_Sub_Rounding==TimeCode::Floor)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames_Sub1=TC_Frames_Sub+1;
            TimeCode TC_Frames_Sub1_InTime=TC_Frames_Sub1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Floor);
            if (TC_Time==TC_Frames_Sub1_InTime)
            {
                TC_Frames_Sub=TC_Frames_Sub1;
                IsRounded=true;
            }
        }
        if (TC_Frames_Sub_Rounding==TimeCode::Ceil)
        {
            //Handling TC_Time rounding issue
            TimeCode TC_Frames_Sub1=TC_Frames_Sub-1;
            TimeCode TC_Frames_Sub1_InTime=TC_Frames_Sub1.ToRescaled(TC_Time.GetFramesMax(), TimeCode::flags(), TimeCode::Ceil);
            if (TC_Time==TC_Frames_Sub1_InTime)
            {
                TC_Frames_Sub=TC_Frames_Sub1;
                IsRounded=true;
            }
        }
        int64_t SubFrames=TC_Frames_Sub.ToFrames();
        int64_t SubFrames_Main=SubFrames/100;
        int64_t SubFrames_Part=SubFrames%100;
        TimeCode TC_Frames_Sub_Main=TimeCode(SubFrames_Main, FramesPerSecondI-1, TimeCode::DropFrame(DropFrame).FPS1001(FramesPerSecondI!=FramesPerSecondF));
        TC_WithExtraSubFrames_String=TC_Frames_Sub_Main.ToString();
        if (SubFrames_Part)
        {
            TC_WithExtraSubFrames_String+='.';
            auto Temp=std::to_string(SubFrames_Part);
            if (Temp.size()==1)
                Temp.insert(0, 1, '0');
            TC_WithExtraSubFrames_String+=Temp;
        }
    }
    
    In.Fill(Stream_Audio, 0, Prefix.c_str(), TC_Time.ToString(), true, true);
    In.Fill_SetOptions(Stream_Audio, 0, Prefix.c_str(), "N NTY");
    string ForDisplay;
    if (Prefix=="Dolby_Atmos_Metadata FirstFrameOfAction")
        ForDisplay=TC_Frames.ToString();
    else if (Prefix.find(" Start")+6==Prefix.size() || Prefix.find(" End")+4==Prefix.size())
        ForDisplay=TC_WithExtraSubFrames_String;
    else
        ForDisplay=TC_WithExtraSamples_String;
    In.Fill(Stream_Audio, 0, (Prefix+"/String").c_str(), TC_Time.ToString()+(ForDisplay.empty()?string():(" ("+ForDisplay+')')), true, true);
    In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/String").c_str(), "Y NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCode").c_str(), TC_Frames.ToString(), true, true);
    if (TC_Frames.IsValid())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCode").c_str(), "N NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCodeSubFrames").c_str(), TC_WithExtraSubFrames_String, true, true);
    if (!TC_WithExtraSubFrames_String.empty())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCodeSubFrames").c_str(), "N NTN");
    In.Fill(Stream_Audio, 0, (Prefix+"/TimeCodeSamples").c_str(), Frequency?TC_WithExtraSamples_String:string(), true, true);
    if (Frequency && !TC_WithExtraSamples_String.empty())
        In.Fill_SetOptions(Stream_Audio, 0, (Prefix+"/TimeCodeSamples").c_str(), "N NTN");
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_Global()
{
    if (IsSub)
        return;

    TestDirectory();

    #if MEDIAINFO_ADVANCED
        if (MediaInfoLib::Config.ExternalMetaDataConfig_Get().empty()) // ExternalMetadata is used directly only if there is no ExternalMetadata config (=another format)
        {
            Ztring ExternalMetadata=MediaInfoLib::Config.ExternalMetadata_Get();
            if (!ExternalMetadata.empty())
            {
                ZtringListList List;
                List.Separator_Set(0, MediaInfoLib::Config.LineSeparator_Get());
                List.Separator_Set(1, __T(";"));
                List.Write(ExternalMetadata);

                for (size_t i=0; i<List.size(); i++)
                {
                    // col 1&2 can be removed, conidered as "General;0"
                    // 1: stream kind (General, Video, Audio, Text...)
                    // 2: 0-based stream number
                    // 3: field name
                    // 4: field value
                    // 5 (optional): replace instead of ignoring if field is already present (metadata from the file)
                    if (List[i].size()<2 || List[i].size()>5)
                    {
                        MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid column size for external metadata");
                        continue;
                    }

                    Ztring StreamKindZ=Ztring(List[i][0]).MakeLowerCase();
                    stream_t StreamKind;
                    size_t   Offset;
                    if (List[i].size()<4)
                    {
                        StreamKind=Stream_General;
                        Offset=2;
                    }
                    else
                    {
                        Offset=0;
                             if (StreamKindZ==__T("general"))   StreamKind=Stream_General;
                        else if (StreamKindZ==__T("video"))     StreamKind=Stream_Video;
                        else if (StreamKindZ==__T("audio"))     StreamKind=Stream_Audio;
                        else if (StreamKindZ==__T("text"))      StreamKind=Stream_Text;
                        else if (StreamKindZ==__T("other"))     StreamKind=Stream_Other;
                        else if (StreamKindZ==__T("image"))     StreamKind=Stream_Image;
                        else if (StreamKindZ==__T("menu"))      StreamKind=Stream_Menu;
                        else
                        {
                            MediaInfoLib::Config.Log_Send(0xC0, 0xFF, 0, "Invalid column 0 for external metadata");
                            continue;
                        }
                    }
                    size_t StreamPos=(size_t)List[i][1].To_int64u();
                    bool ShouldReplace=List[i].size()>4-Offset && List[i][4-Offset].To_int64u();
                    if (ShouldReplace || Retrieve_Const(StreamKind, StreamPos, List[i][2-Offset].To_UTF8().c_str()).empty())
                        Fill(StreamKind, StreamPos, List[i][2-Offset].To_UTF8().c_str(), List[i][3-Offset], ShouldReplace);
                }
            }
        }
    #endif //MEDIAINFO_ADVANCED

    #if MEDIAINFO_ADVANCED
        //Default frame rate
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_FrameRate).empty() && Config->File_DefaultFrameRate_Get())
            Fill(Stream_Video, 0, Video_FrameRate, Config->File_DefaultFrameRate_Get());
    #endif //MEDIAINFO_ADVANCED

    //Video Frame count
    if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0 && Retrieve(Stream_Video, 0, Video_FrameCount).empty())
    {
        if (Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
        else if (Config->File_Names.size()>1 && StreamSource==IsStream)
            Fill(Stream_Video, 0, Video_FrameCount, Config->File_Names.size());
        #if MEDIAINFO_IBIUSAGE
        else
        {
            //External IBI
            std::string IbiFile=Config->Ibi_Get();
            if (!IbiFile.empty())
            {
                if (IbiStream)
                    IbiStream->Infos.clear(); //TODO: support IBI data from different inputs
                else
                    IbiStream=new ibi::stream;

                File_Ibi MI;
                Open_Buffer_Init(&MI, IbiFile.size());
                MI.Ibi=new ibi;
                MI.Open_Buffer_Continue((const int8u*)IbiFile.c_str(), IbiFile.size());
                if (!MI.Ibi->Streams.empty())
                    (*IbiStream)=(*MI.Ibi->Streams.begin()->second);
            }

            if (IbiStream && !IbiStream->Infos.empty() && IbiStream->Infos[IbiStream->Infos.size()-1].IsContinuous && IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber!=(int64u)-1)
                Fill(Stream_Video, 0, Video_FrameCount, IbiStream->Infos[IbiStream->Infos.size()-1].FrameNumber);
        }
        #endif //MEDIAINFO_IBIUSAGE
    }

    //Exception
    if (Retrieve(Stream_General, 0, General_Format)==__T("AC-3") && (Retrieve(Stream_General, 0, General_Format_Profile).find(__T("E-AC-3"))==0 || Retrieve(Stream_General, 0, General_Format_AdditionalFeatures).find(__T("Dep"))!=string::npos))
    {
        //Using AC-3 extensions + E-AC-3 extensions + "eb3" specific extension
        Ztring Extensions=Retrieve(Stream_General, 0, General_Format_Extensions);
        if (Extensions.find(__T(" eb3"))==string::npos)
        {
            Extensions+=__T(' ');
            Extensions+=MediaInfoLib::Config.Format_Get(__T("E-AC-3"), InfoFormat_Extensions);
            Extensions+=__T(" eb3");
            Fill(Stream_General, 0, General_Format_Extensions, Extensions, true);
            if (MediaInfoLib::Config.Legacy_Get())
                Fill(Stream_General, 0, General_Codec_Extensions, Extensions, true);
        }
    }

    #if MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)
        // Cropped
        if (Count_Get(Stream_Video)+Count_Get(Stream_Image) && MediaInfoLib::Config.Enable_FFmpeg_Get())
        {
            Ztring Command=External_Command_Exists(ffmpeg_PossibleNames);;
            if (!Command.empty())
            {
                auto StreamKind=Count_Get(Stream_Video)?Stream_Video:Stream_Image;
                {
                    const auto StreamCount=Count_Get(StreamKind);
                    for (size_t StreamPos=0; StreamPos<StreamCount; StreamPos++)
                    {
                        ZtringList Arguments;
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.push_back(__T("-noaccurate_seek"));
                            Arguments.push_back(__T("-ss"));
                            Arguments.push_back(Ztring::ToZtring(Retrieve_Const(Stream_Video, 0, Video_Duration).To_int64u()/2000));
                        }
                        Arguments.push_back(__T("-i"));
                        Arguments.push_back(Retrieve_Const(Stream_General, 0, General_CompleteName));
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.push_back(__T("-map"));
                            Arguments.push_back(__T("v:")+Ztring::ToZtring(StreamPos));
                        }
                        Arguments.push_back(__T("-vf"));
                        Arguments.push_back(__T("cropdetect=skip=0:round=1"));
                        if (StreamKind==Stream_Video)
                        {
                            Arguments.back().insert(0, __T("select='eq(pict_type,I)',"));
                            Arguments.push_back(__T("-copyts"));
                            Arguments.push_back(__T("-vframes"));
                            Arguments.push_back(__T("4"));
                        }
                        Arguments.push_back(__T("-f"));
                        Arguments.push_back(__T("null"));
                        Arguments.push_back(__T("-"));
                        Ztring Err;
                        External_Command_Run(Command, Arguments, nullptr, &Err);
                        auto Pos_Start=Err.rfind(__T("[Parsed_cropdetect_"));
                        if (Pos_Start!=string::npos)
                        {
                            auto Pos_End=Err.find(__T('\n'), Pos_Start);
                            if (Pos_End==string::npos)
                                Pos_End=Err.size();
                            Ztring Crop_Line=Err.substr(Pos_Start, Pos_End-Pos_Start);
                            ZtringList Crop;
                            Crop.Separator_Set(0, __T(" "));
                            Crop.Write(Crop_Line);
                            int32u Values[6];
                            memset(Values, -1, sizeof(Values));
                            int32u Width=Retrieve_Const(StreamKind, StreamPos, "Width").To_int32u();
                            int32u Height=Retrieve_Const(StreamKind, StreamPos, "Height").To_int32u();
                            for (const auto& Item : Crop)
                            {
                                if (Item.size()<=3
                                 || Item[0]<__T('x') || Item[0]>__T('y')
                                 || Item[1]<__T('1') || Item[1]>__T('2')
                                 || Item[2]!=__T(':')
                                 || Item[3]<__T('0') || Item[3]>__T('9'))
                                    continue;
                                Values[((Item[0]-__T('x'))<<1)|(Item[1]-__T('1'))]=Ztring(Item.substr(3)).To_int32u();
                            }
                            if (Values[0]!=(int32u)-1 && Values[1]!=(int32u)-1)
                            {
                                Values[4]=Values[1]-Values[0];
                                if (((int32s)Values[4])>=0)
                                {
                                    Values[4]++;
                                    if (Values[4]!=Width)
                                        Fill(StreamKind, StreamPos, "Active_Width", Values[4]);
                                }
                            }
                            if (Values[2]!=(int32u)-1 && Values[3]!=(int32u)-1)
                            {
                                Values[5]=Values[3]-=Values[2];
                                if (((int32s)Values[5])>=0)
                                {
                                    Values[5]++;
                                    if (Values[5]!=Height)
                                        Fill(StreamKind, StreamPos, "Active_Height", Values[5]);
                                }
                            }
                            if (((int32s)Values[4])>=0 && ((int32s)Values[5])>=0 && (Values[4]!=Width || Values[5]!=Height))
                            {
                                float32 PAR=Retrieve_Const(StreamKind, 0, "PixelAspectRatio").To_float32();
                                if (PAR)
                                    Fill(StreamKind, StreamPos, "Active_DisplayAspectRatio", ((float32)Values[4])/Values[5]*PAR, 2);
                            }
                        }
                    }
                }
            }
        }
    #endif //MEDIAINFO_ADVANCED && defined(MEDIAINFO_FILE_YES)

    Streams_Finish_StreamOnly();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_InterStreams();
    Streams_Finish_StreamOnly();
    Streams_Finish_StreamOnly_General_Curate(0);

    Config->File_ExpandSubs_Update((void**)(&Stream_More));

    if (!IsSub && !Config->File_IsReferenced_Get() && MediaInfoLib::Config.ReadByHuman_Get())
        Streams_Finish_HumanReadable();
}

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
void File__Analyze::TestContinuousFileNames(size_t CountOfFiles, Ztring FileExtension, bool SkipComputeDelay)
{
    if (IsSub || !Config->File_TestContinuousFileNames_Get())
        return;

    size_t Pos=Config->File_Names.size();
    if (!Pos)
        return;

    //Trying to detect continuous file names (e.g. video stream as an image or HLS)
    size_t Pos_Base = (size_t)-1;
    bool AlreadyPresent=Config->File_Names.size()==1?true:false;
    FileName FileToTest(Config->File_Names.Read(Config->File_Names.size()-1));
    #ifdef WIN32
        FileToTest.FindAndReplace(__T("/"), __T("\\"), 0, Ztring_Recursive); // "/" is sometimes used on Windows and it is considered as valid
    #endif //WIN32
    Ztring FileToTest_Name=FileToTest.Name_Get();
    Ztring FileToTest_Name_After=FileToTest_Name;
    size_t FileNameToTest_End=FileToTest_Name.size();
    while (FileNameToTest_End && !(FileToTest_Name[FileNameToTest_End-1]>=__T('0') && FileToTest_Name[FileNameToTest_End-1]<=__T('9')))
        FileNameToTest_End--;
    size_t FileNameToTest_Pos=FileNameToTest_End;
    while (FileNameToTest_Pos && FileToTest_Name[FileNameToTest_Pos-1]>=__T('0') && FileToTest_Name[FileNameToTest_Pos-1]<=__T('9'))
        FileNameToTest_Pos--;
    if (FileNameToTest_Pos!=FileToTest_Name.size() && FileNameToTest_Pos!=FileNameToTest_End)
    {
        size_t Numbers_Size=FileNameToTest_End-FileNameToTest_Pos;
        int64u Pos=Ztring(FileToTest_Name.substr(FileNameToTest_Pos)).To_int64u();
        FileToTest_Name.resize(FileNameToTest_Pos);
        FileToTest_Name_After.erase(0, FileToTest_Name.size()+Numbers_Size);

        /*
        for (;;)
        {
            Pos++;
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest.Path_Get()+PathSeparator+FileToTest_Name+Pos_Ztring+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
            if (!File::Exists(Next))
                break;
            Config->File_Names.push_back(Next);
        }
        */

        //Detecting with a smarter algo (but missing frames are not detected)
        Ztring FileToTest_Name_Begin=FileToTest.Path_Get()+PathSeparator+FileToTest_Name;
        Ztring FileToTest_Name_End=FileToTest_Name_After+__T('.')+(FileExtension.empty()?FileToTest.Extension_Get():FileExtension);
        Pos_Base = (size_t)Pos;
        size_t Pos_Add_Max = 1;
        #if MEDIAINFO_ADVANCED
            bool File_IgnoreSequenceFileSize=Config->File_IgnoreSequenceFilesCount_Get(); //TODO: double check if it is expected

            size_t SequenceFileSkipFrames=Config->File_SequenceFilesSkipFrames_Get();
            if (SequenceFileSkipFrames)
            {
                for (;;)
                {
                    size_t Pos_Add_Max_Old=Pos_Add_Max;
                    for (size_t TempPos=Pos_Add_Max; TempPos<=Pos_Add_Max+SequenceFileSkipFrames; TempPos++)
                    {
                        Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+TempPos);
                        if (Numbers_Size>Pos_Ztring.size())
                            Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
                        Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
                        if (File::Exists(Next))
                        {
                            Pos_Add_Max=TempPos+1;
                            break;
                        }
                    }
                    if (Pos_Add_Max==Pos_Add_Max_Old)
                        break;
                }
            }
            else
            {
        #endif //MEDIAINFO_ADVANCED
        for (;;)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Max);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (!File::Exists(Next))
                break;
            Pos_Add_Max<<=1;
            #if MEDIAINFO_ADVANCED
                if (File_IgnoreSequenceFileSize && Pos_Add_Max>=CountOfFiles)
                    break;
            #endif //MEDIAINFO_ADVANCED
        }
        size_t Pos_Add_Min = Pos_Add_Max >> 1;
        while (Pos_Add_Min+1<Pos_Add_Max)
        {
            size_t Pos_Add_Middle = Pos_Add_Min + ((Pos_Add_Max - Pos_Add_Min) >> 1);
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos_Base+Pos_Add_Middle);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Ztring Next=FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End;
            if (File::Exists(Next))
                Pos_Add_Min=Pos_Add_Middle;
            else
                Pos_Add_Max=Pos_Add_Middle;
        }

        #if MEDIAINFO_ADVANCED
            } //SequenceFileSkipFrames
        #endif //MEDIAINFO_ADVANCED

        size_t Pos_Max = Pos_Base + Pos_Add_Max;
        Config->File_Names.reserve(Pos_Add_Max);
        for (Pos=Pos_Base+1; Pos<Pos_Max; ++Pos)
        {
            Ztring Pos_Ztring; Pos_Ztring.From_Number(Pos);
            if (Numbers_Size>Pos_Ztring.size())
                Pos_Ztring.insert(0, Numbers_Size-Pos_Ztring.size(), __T('0'));
            Config->File_Names.push_back(FileToTest_Name_Begin+Pos_Ztring+FileToTest_Name_End);
        }

        if (!Config->File_IsReferenced_Get() && Config->File_Names.size()<CountOfFiles && AlreadyPresent)
            Config->File_Names.resize(1); //Removing files, wrong detection
    }

    if (Config->File_Names.size()==Pos)
        return;

    Config->File_IsImageSequence=true;
    if (StreamSource==IsStream)
        Frame_Count_NotParsedIncluded=Pos_Base;
    #if MEDIAINFO_DEMUX
        float64 Demux_Rate=Config->Demux_Rate_Get();
        if (!Demux_Rate)
            Demux_Rate=24;
        if (!SkipComputeDelay && Frame_Count_NotParsedIncluded!=(int64u)-1)
            Fill(Stream_Video, 0, Video_Delay, float64_int64s(Frame_Count_NotParsedIncluded*1000/Demux_Rate));
    #endif //MEDIAINFO_DEMUX

    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
    {
        for (; Pos<Config->File_Names.size(); Pos++)
        {
            int64u Size=File::Size_Get(Config->File_Names[Pos]);
            Config->File_Sizes.push_back(Size);
            Config->File_Size+=Size;
        }
    }
    #if MEDIAINFO_ADVANCED
        else
        {
            Config->File_Size=(int64u)-1;
            File_Size=(int64u)-1;
            Clear(Stream_General, 0, General_FileSize);
        }
    #endif //MEDIAINFO_ADVANCED

    File_Size=Config->File_Size;
    Element[0].Next=File_Size;
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFileSize_Get() || Config->File_Names.size()<=1)
    #endif //MEDIAINFO_ADVANCED
        Fill (Stream_General, 0, General_FileSize, File_Size, 10, true);
    #if MEDIAINFO_ADVANCED
        if (!Config->File_IgnoreSequenceFilesCount_Get())
    #endif //MEDIAINFO_ADVANCED
    {
        Fill (Stream_General, 0, General_CompleteName_Last, Config->File_Names[Config->File_Names.size()-1], true);
        Fill (Stream_General, 0, General_FolderName_Last, FileName::Path_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileName_Last, FileName::Name_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        Fill (Stream_General, 0, General_FileExtension_Last, FileName::Extension_Get(Config->File_Names[Config->File_Names.size()-1]), true);
        if (Retrieve(Stream_General, 0, General_FileExtension_Last).empty())
            Fill(Stream_General, 0, General_FileNameExtension_Last, Retrieve(Stream_General, 0, General_FileName_Last));
        else
            Fill(Stream_General, 0, General_FileNameExtension_Last, Retrieve(Stream_General, 0, General_FileName_Last)+__T('.')+Retrieve(Stream_General, 0, General_FileExtension_Last));
    }

    #if MEDIAINFO_ADVANCED
        if (Config->File_Source_List_Get())
        {
            Ztring SourcePath=FileName::Path_Get(Retrieve(Stream_General, 0, General_CompleteName));
            size_t SourcePath_Size=SourcePath.size()+1; //Path size + path separator size
            for (size_t Pos=0; Pos<Config->File_Names.size(); Pos++)
            {
                Ztring Temp=Config->File_Names[Pos];
                Temp.erase(0, SourcePath_Size);
                Fill(Stream_General, 0, "Source_List", Temp);
            }
            Fill_SetOptions(Stream_General, 0, "Source_List", "N NT");
        }
    #endif //MEDIAINFO_ADVANCED
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_FILE_YES)
// title-of-work/
//     title-of-work.wav
//     dpx/
//         title-of-work_0086880.dpx
//         title-of-work_0086881.dpx
//         ... etc ...
static void PotentialAudioNames_Scenario1(const Ztring& DpxName, Ztring& ContainerDirName, ZtringList& List)
{
    if (DpxName.size()<4)
        return;

    if (DpxName.substr(DpxName.size()-4)!=__T(".dpx"))
        return;

    size_t PathSeparator_Pos1=DpxName.find_last_of(__T("\\/"));
    if (PathSeparator_Pos1==string::npos)
        return;

    size_t PathSeparator_Pos2=DpxName.find_last_of(__T("\\/"), PathSeparator_Pos1-1);
    if (PathSeparator_Pos2==string::npos)
        return;

    size_t PathSeparator_Pos3=DpxName.find_last_of(__T("\\/"), PathSeparator_Pos2-1); //string::npos is accepted (relative path) 

    size_t TitleSeparator_Pos=DpxName.find_last_of(__T('_'));
    if (TitleSeparator_Pos==string::npos || TitleSeparator_Pos<=PathSeparator_Pos1)
        return;

    Ztring DirDpx=DpxName.substr(PathSeparator_Pos2+1, PathSeparator_Pos1-(PathSeparator_Pos2+1));
    if (DirDpx!=__T("dpx"))
        return;

    Ztring TitleDpx=DpxName.substr(PathSeparator_Pos1+1, TitleSeparator_Pos-(PathSeparator_Pos1+1));
    Ztring TitleDir=DpxName.substr(PathSeparator_Pos3+1, PathSeparator_Pos2-(PathSeparator_Pos3+1));
    if (TitleDpx!=TitleDir)
        return;

    ContainerDirName=DpxName.substr(0, PathSeparator_Pos2+1);
    List.push_back(ContainerDirName+TitleDpx+__T(".wav"));
}
void File__Analyze::TestDirectory()
{
    if (IsSub || !Config->File_TestDirectory_Get())
        return;

    if (Config->File_Names.size()<=1)
        return;

    Ztring ContainerDirName;
    ZtringList List;
    PotentialAudioNames_Scenario1(Config->File_Names[0], ContainerDirName, List);
    bool IsModified=false;
    for (size_t i=0; i<List.size(); i++)
    {
        MediaInfo_Internal MI;
        if (MI.Open(List[i]))
        {
            IsModified=true;
            Ztring AudioFileName=MI.Get(Stream_General, 0, General_CompleteName);
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<MI.Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Stream_Prepare((stream_t)StreamKind);
                    Merge(MI, (stream_t)StreamKind, StreamPos_Last, StreamPos);
                    if (AudioFileName.size()>ContainerDirName.size())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Source", AudioFileName.substr(ContainerDirName.size()));
                    Fill((stream_t)StreamKind, StreamPos_Last, "MuxingMode", MI.Get(Stream_General, 0, General_Format));
                    if (Retrieve_Const((stream_t)StreamKind, StreamPos_Last, "Encoded_Application").empty())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Encoded_Application", MI.Get(Stream_General, 0, General_Encoded_Application));
                    if (Retrieve_Const((stream_t)StreamKind, StreamPos_Last, "Encoded_Library").empty())
                        Fill((stream_t)StreamKind, StreamPos_Last, "Encoded_Library", MI.Get(Stream_General, 0, General_Encoded_Library));
                }
            #if MEDIAINFO_ADVANCED
                if (!Config->File_IgnoreSequenceFileSize_Get())
            #endif //MEDIAINFO_ADVANCED
            {
                File_Size+=MI.Get(Stream_General, 0, General_FileSize).To_int64u();
            }
        }
    }
    if (IsModified)
    {
        Ztring VideoFileName=Retrieve(Stream_General, 0, General_CompleteName);
        Ztring VideoFileName_Last=Retrieve(Stream_General, 0, General_CompleteName_Last);
        Ztring VideoMuxingMode=Retrieve_Const(Stream_General, 0, General_Format);
        if (VideoFileName.size()>ContainerDirName.size())
            Fill(Stream_Video, 0, "Source", VideoFileName.substr(ContainerDirName.size()));
        if (VideoFileName_Last.size()>ContainerDirName.size())
            Fill(Stream_Video, 0, "Source_Last", VideoFileName_Last.substr(ContainerDirName.size()));
        Fill(Stream_Video, 0, Video_MuxingMode, VideoMuxingMode);

        Fill(Stream_General, 0, General_CompleteName, ContainerDirName, true);
        Fill(Stream_General, 0, General_FileSize, File_Size, 10, true);
        Fill(Stream_General, 0, General_Format, "Directory", Unlimited, true, true);

        Clear(Stream_General, 0, General_CompleteName_Last);
        Clear(Stream_General, 0, General_FolderName_Last);
        Clear(Stream_General, 0, General_FileName_Last);
        Clear(Stream_General, 0, General_FileNameExtension_Last);
        Clear(Stream_General, 0, General_FileExtension_Last);
        Clear(Stream_General, 0, General_Format_String);
        Clear(Stream_General, 0, General_Format_Info);
        Clear(Stream_General, 0, General_Format_Url);
        Clear(Stream_General, 0, General_Format_Commercial);
        Clear(Stream_General, 0, General_Format_Commercial_IfAny);
        Clear(Stream_General, 0, General_Format_Version);
        Clear(Stream_General, 0, General_Format_Profile);
        Clear(Stream_General, 0, General_Format_Level);
        Clear(Stream_General, 0, General_Format_Compression);
        Clear(Stream_General, 0, General_Format_Settings);
        Clear(Stream_General, 0, General_Format_AdditionalFeatures);
        Clear(Stream_General, 0, General_InternetMediaType);
        Clear(Stream_General, 0, General_Duration);
        Clear(Stream_General, 0, General_Encoded_Application);
        Clear(Stream_General, 0, General_Encoded_Application_String);
        Clear(Stream_General, 0, General_Encoded_Application_CompanyName);
        Clear(Stream_General, 0, General_Encoded_Application_Name);
        Clear(Stream_General, 0, General_Encoded_Application_Version);
        Clear(Stream_General, 0, General_Encoded_Application_Url);
        Clear(Stream_General, 0, General_Encoded_Library);
        Clear(Stream_General, 0, General_Encoded_Library_String);
        Clear(Stream_General, 0, General_Encoded_Library_CompanyName);
        Clear(Stream_General, 0, General_Encoded_Library_Name);
        Clear(Stream_General, 0, General_Encoded_Library_Version);
        Clear(Stream_General, 0, General_Encoded_Library_Date);
        Clear(Stream_General, 0, General_Encoded_Library_Settings);
        Clear(Stream_General, 0, General_Encoded_OperatingSystem);
        Clear(Stream_General, 0, General_FrameCount);
        Clear(Stream_General, 0, General_FrameRate);
    }
}
#endif //defined(MEDIAINFO_FILE_YES)

//---------------------------------------------------------------------------
#if MEDIAINFO_FIXITY
bool File__Analyze::FixFile(int64u FileOffsetForWriting, const int8u* ToWrite, const size_t ToWrite_Size)
{
    if (Config->File_Names.empty())
        return false; //Streams without file names are not supported
        
    #ifdef WINDOWS
    File::Copy(Config->File_Names[0], Config->File_Names[0]+__T(".Fixed"));
    #else //WINDOWS
    //ZenLib has File::Copy only for Windows for the moment. //TODO: support correctly (including meta)
    if (!File::Exists(Config->File_Names[0]+__T(".Fixed")))
    {
        std::ofstream  Dest(Ztring(Config->File_Names[0]+__T(".Fixed")).To_Local().c_str(), std::ios::binary);
        if (Dest.fail())
            return false;
        std::ifstream  Source(Config->File_Names[0].To_Local().c_str(), std::ios::binary);
        if (Source.fail())
            return false;
        Dest << Source.rdbuf();
        if (Dest.fail())
            return false;
    }
    #endif //WINDOWS

    File F;
    if (!F.Open(Config->File_Names[0]+__T(".Fixed"), File::Access_Write))
        return false;

    if (!F.GoTo(FileOffsetForWriting))
        return false;

    F.Write(ToWrite, ToWrite_Size);

    return true;
}
#endif //MEDIAINFO_FIXITY

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            Streams_Finish_StreamOnly((stream_t)StreamKind, StreamPos);

    //For each kind of (*Stream)
    for (size_t Pos=0; Pos<Count_Get(Stream_General);  Pos++) Streams_Finish_StreamOnly_General(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Video);    Pos++) Streams_Finish_StreamOnly_Video(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Audio);    Pos++) Streams_Finish_StreamOnly_Audio(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Text);     Pos++) Streams_Finish_StreamOnly_Text(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Other);    Pos++) Streams_Finish_StreamOnly_Other(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Image);    Pos++) Streams_Finish_StreamOnly_Image(Pos);
    for (size_t Pos=0; Pos<Count_Get(Stream_Menu);     Pos++) Streams_Finish_StreamOnly_Menu(Pos);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly(stream_t StreamKind, size_t Pos)
{
    //Format
    if (Retrieve_Const(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Format)).empty())
        Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Format), Retrieve_Const(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_CodecID)));

    //BitRate from Duration and StreamSize
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (Duration>0 && StreamSize>0)
            Fill(StreamKind, Pos, "BitRate", StreamSize*8*1000/Duration, 0);
    }

    //BitRate_Encoded from Duration and StreamSize_Encoded
    if (StreamKind!=Stream_General && StreamKind!=Stream_Other && StreamKind!=Stream_Menu && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty())
    {
        float64 Duration=0;
        if (StreamKind==Stream_Video && !Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && !Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        int64u StreamSize_Encoded=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize_Encoded)).To_int64u();
        if (Duration>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Encoded), StreamSize_Encoded*8*1000/Duration, 0);
    }

    //Duration from Bitrate and StreamSize
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty())
    {
        int64u BitRate=Retrieve(StreamKind, Pos, "BitRate").To_int64u();
        int64u StreamSize=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).To_int64u();
        if (BitRate>0 && StreamSize>0)
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration), ((float64)StreamSize)*8*1000/BitRate, 0);
    }

    //StreamSize from BitRate and Duration
    if (StreamKind!=Stream_Other && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize)).empty() && !Retrieve(StreamKind, Pos, "BitRate").empty() && !Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).empty() && Retrieve(StreamKind, Pos, "BitRate").find(__T(" / "))==std::string::npos) //If not done the first time or by other routine
    {
        float64 BitRate=Retrieve(StreamKind, Pos, "BitRate").To_float64();
        float64 Duration=Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_Duration)).To_float64();
        if (BitRate>0 && Duration>0)
        {
            float64 StreamSize=BitRate*Duration/8/1000;
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_StreamSize), StreamSize, 0);
        }
    }

    //Bit rate and maximum bit rate
    if (!Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate)).empty() && Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate))==Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum)))
    {
        Clear(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Maximum));
        if (Retrieve(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode)).empty())
            Fill(StreamKind, Pos, Fill_Parameter(StreamKind, Generic_BitRate_Mode), "CBR");
    }

    //ServiceKind
    auto ServiceKind = Retrieve(StreamKind, Pos, "ServiceKind");
    if (!ServiceKind.empty())
    {
        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(ServiceKind);
        if (List.size()>1)
        {
            size_t HI_ME_Pos=(size_t)-1;
            size_t HI_D_Pos=(size_t)-1;
            static const auto HI_ME_Text=__T("HI-ME");
            static const auto HI_D_Text=__T("HI-D");
            static const auto VI_ME_Text=__T("VI-ME");
            static const auto VI_D_Text=__T("VI-D");
            for (size_t i=0; i<List.size(); i++)
            {
                const auto& Item=List[i];
                if (HI_ME_Pos==(size_t)-1 && (Item==HI_ME_Text || Item==VI_ME_Text))
                    HI_ME_Pos=i;
                if (HI_D_Pos==(size_t)-1 && (Item==HI_D_Text || Item==VI_D_Text))
                    HI_D_Pos=i;
            }
            if (HI_ME_Pos!=(size_t)-1 && HI_D_Pos!=(size_t)-1)
            {
                if (HI_ME_Pos>HI_D_Pos)
                    std::swap(HI_ME_Pos, HI_D_Pos);
                List[HI_ME_Pos]=__T("HI");
                List.erase(List.begin()+HI_D_Pos);
                Fill(StreamKind, Pos, "ServiceKind", List.Read(), true);
                List.Write(Retrieve(StreamKind, Pos, "ServiceKind/String"));
                List[HI_ME_Pos].From_UTF8("Hearing Impaired");
                List.erase(List.begin()+HI_D_Pos);
                Fill(StreamKind, Pos, "ServiceKind/String", List.Read(), true);
            }
        }
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_General(size_t StreamPos)
{
    //File extension test
    if (Retrieve(Stream_General, StreamPos, "FileExtension_Invalid").empty())
    {
        const Ztring& Name=Retrieve(Stream_General, StreamPos, General_FileName);
        const Ztring& Extension=Retrieve(Stream_General, StreamPos, General_FileExtension);
        const Ztring& FormatName=Retrieve(Stream_General, StreamPos, General_Format);
        auto IsMachOAndEmptyExtension = Extension.empty() && FormatName.rfind(__T("Mach-O"), 0)==0;
        if ((!Name.empty() && !IsMachOAndEmptyExtension) || !Extension.empty())
        {
            InfoMap &FormatList=MediaInfoLib::Config.Format_Get();
            InfoMap::iterator Format=FormatList.find(FormatName);
            if (Format!=FormatList.end())
            {
                ZtringList ValidExtensions;
                ValidExtensions.Separator_Set(0, __T(" "));
                ValidExtensions.Write(Retrieve(Stream_General, StreamPos, General_Format_Extensions));
                if (!ValidExtensions.empty() && ValidExtensions.Find(Extension)==string::npos)
                    Fill(Stream_General, StreamPos, "FileExtension_Invalid", ValidExtensions.Read());
            }
        }
    }

    //Audio_Channels_Total
    if (Retrieve_Const(Stream_General, StreamPos, General_Audio_Channels_Total).empty())
    {
        auto Audio_Count = Count_Get(Stream_Audio);
        int64u Channels_Total=0;
        for (size_t i=0; i<Audio_Count; i++)
        {
            int64u Channels=Retrieve_Const(Stream_Audio, i, Audio_Channel_s_).To_int64u();
            if (!Channels)
            {
                Channels_Total=0;
                break;
            }
            Channels_Total+=Channels;
        }
        if (Channels_Total)
            Fill(Stream_General, StreamPos, General_Audio_Channels_Total, Channels_Total);
    }

    //Exceptions (empiric)
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (Application_Name.size()>=5 && Application_Name.find(__T(", LLC"))==Application_Name.size()-5)
        {
            Fill(Stream_General, 0, General_Encoded_Application_CompanyName, Application_Name.substr(0, Application_Name.size()-5));
            Clear(Stream_General, StreamPos, General_Encoded_Application_Name);
        }
    }
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (Application_Name.size()>=5 && Application_Name.rfind(__T("Mac OS X "), 0)==0)
        {
            Fill(Stream_General, 0, General_Encoded_Application_Version, Application_Name.substr(9), true);
            const auto& Application_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName);
            if (Application_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, "Apple");
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, "Mac OS X", Unlimited, true, true);
        }
        if (Application_Name.size()>=5 && Application_Name.rfind(__T("Sorenson "), 0)==0)
        {
            auto Application_Name_Max=Application_Name.find(__T(" / "));
            if (Application_Name_Max!=(size_t)-1)
                Application_Name_Max-=9;
            Fill(Stream_General, 0, General_Encoded_Application_Name, Application_Name.substr(9, Application_Name_Max), true);
            const auto& Application_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName);
            if (Application_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_Application_CompanyName, "Sorenson");
        }
    }
    {
        const auto& Application_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        const auto& OperatingSystem_Version=Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Version);
        const auto& Hardware_Name=Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        if (OperatingSystem_Version.empty() && !Application_Name.empty() && Application_Name.find_first_not_of(__T("0123456789."))==string::npos && Hardware_Name.rfind(__T("iPhone "), 0)==0)
        {
            Fill(Stream_General, 0, General_Encoded_OperatingSystem_Version, Application_Name);
            Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "iOS", Unlimited, true, true);
            const auto& OperatingSystem_CompanyName=Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName);
            if (OperatingSystem_CompanyName.empty())
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Apple");
            Clear(Stream_General, StreamPos, General_Encoded_Application_Name);
        }
    }
    {
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name).empty()) {
            auto Application = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application).To_UTF8();
            auto pos = Application.rfind(" (Android)");
            if (Application.length() >= 12) {
            if (pos == Application.length() - 10) {
                Application.erase(pos, 10);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "Android");
            }
            pos = Application.rfind(" (Macintosh)");
            if (pos == Application.length() - 12) {
                Application.erase(pos, 12);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "macOS");
            }
            pos = Application.rfind(" (Windows)");
            if (pos == Application.length() - 10) {
                Application.erase(pos, 10);
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_Name, "Windows");
            }
            }
            if (Application != Retrieve_Const(Stream_General, 0, General_Encoded_Application).To_UTF8())
                Fill(Stream_General, 0, General_Encoded_Application, Application, true, true);
        }
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName).empty()) {
            const auto OperatingSystem = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name).To_UTF8();
            if (OperatingSystem == "Android")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Google");
            if (OperatingSystem == "macOS")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Apple");
            if (OperatingSystem == "Windows")
                Fill(Stream_General, 0, General_Encoded_OperatingSystem_CompanyName, "Microsoft");
        }
    }
    {
        const auto& Hardware_CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName);
        const auto& Hardware_Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        if (Hardware_Name.rfind(Hardware_CompanyName + __T(' '), 0) == 0)
            Fill(Stream_General, StreamPos, General_Encoded_Hardware_Name, Hardware_Name.substr(Hardware_CompanyName.size() + 1), true);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name).empty())
    {
        const auto& Performer = Retrieve_Const(Stream_General, StreamPos, General_Performer);
        const auto& Hardware_CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName);
        ZtringList PerformerList;
        PerformerList.Separator_Set(0, __T(" / "));
        PerformerList.Write(Performer);
        set<Ztring> HardwareName_List;
        for (size_t i = 0; i < PerformerList.size(); i++)
        {
            const auto& PerformerItem = PerformerList[i];
            auto ShortAndContainsHardwareCompanyName = PerformerItem.size() - Hardware_CompanyName.size() <= 16 && PerformerItem.rfind(Hardware_CompanyName + __T(' '), 0) == 0;
            if (ShortAndContainsHardwareCompanyName || Hardware_CompanyName == __T("Samsung") && PerformerItem.size() <= 32 && PerformerItem.rfind(__T("Galaxy "), 0) == 0)
            {
                ZtringList Items;
                Items.Separator_Set(0, __T(" "));
                Items.Write(PerformerItem);
                if (Items.size() < 6)
                {
                    auto IsLikelyName = false;
                    auto LastHasOnlyDigits = false;
                    for (const auto& Item : Items)
                    {
                        size_t HasUpper = 0;
                        size_t HasDigit = 0;
                        for (const auto& Value : Item)
                        {
                            HasUpper += IsAsciiUpper(Value);
                            HasDigit += IsAsciiDigit(Value);
                        }
                        LastHasOnlyDigits = HasDigit == Item.size();
                        if (Item.size() == 1 || HasUpper >= 2 || (HasDigit && HasDigit < Item.size()))
                            IsLikelyName = true;
                    }
                    if (IsLikelyName || LastHasOnlyDigits)
                    {
                        HardwareName_List.insert(PerformerItem.substr(ShortAndContainsHardwareCompanyName ? (Hardware_CompanyName.size() + 1) : 0));
                        PerformerList.erase(PerformerList.begin() + i);
                        continue;
                    }
                }
            }
        }
        if (HardwareName_List.size() == 1)
        {
            //Performer is likely the actual performer
            Fill(Stream_General, StreamPos, General_Encoded_Hardware_Name, *HardwareName_List.begin());
            Fill(Stream_General, StreamPos, General_Performer, PerformerList.Read(), true);
        }
    }
    {
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model);
        if (Name == Model)
        {
            //Name is actually the model (technical name), keeping only model
            Clear(Stream_General, StreamPos, General_Encoded_Hardware_Name);
        }
    }

    //OperatingSystem
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_String).empty())
    {
        //Filling
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_CompanyName);
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Name);
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, General_Encoded_OperatingSystem_Version);
        Ztring OperatingSystem = CompanyName;
        if (!Name.empty())
        {
            if (!OperatingSystem.empty())
                OperatingSystem += ' ';
            OperatingSystem += Name;
            if (!Version.empty())
            {
                OperatingSystem += ' ';
                OperatingSystem += Version;
            }
        }
        Fill(Stream_General, StreamPos, General_Encoded_OperatingSystem_String, OperatingSystem);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_General_Curate(size_t StreamPos)
{
    // Remove redundant content
    if (Retrieve_Const(Stream_General, StreamPos, General_Copyright) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Name)) {
        Clear(Stream_General, StreamPos, General_Copyright);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model)) {
        Clear(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName);
    }

    // Remove useless characters
    auto RemoveUseless = [&](size_t Parameter) {
        for (;;) {
            const auto& Value = Retrieve_Const(Stream_General, StreamPos, Parameter);
            if (Value.size() < 2) {
                return;
            }
            if (Value.find_first_not_of(__T("0. ")) == string::npos) {
                Clear(Stream_General, StreamPos, Parameter);
                continue;
            }
            if (Value.front() == '[' && Value.back() == ']') {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(1, Value.size() - 2), true);
                continue;
            }
            if (Value.front() == '<' && Value.back() == '>') {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(1, Value.size() - 2), true);
                continue;
            }
            if (Value.rfind(__T("< "), 0) == 0 && Value.find(__T(" >"), Value.size() - 2) != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(2, Value.size() - 4), true);
                continue;
            }
            if (Value.rfind(__T("Digital Camera "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(15), true);
                continue;
            }
            if (Value.rfind(__T("encoded by "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(11), true);
                continue;
            }
            if (Value.rfind(__T("This file was made by "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(22), true);
                continue;
            }
            if (Value.rfind(__T("made by "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(8), true);
                continue;
            }
            if (Value.rfind(__T("KODAK EASYSHARE "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, __T("KODAK EasyShare") + Value.substr(15), true);
                continue;
            }
            if (Value.rfind(__T("PENTAX "), 0) == 0) {
                Fill(Stream_General, StreamPos, Parameter, __T("Pentax ") + Value.substr(7), true);
                Fill(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName, "Ricoh", Unlimited, true, true);
                continue;
            }
            {
            auto Pos = Value.find(__T("(R)"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + Value.substr(Pos + 3), true);
                continue;
            }
            }
            {
            auto Pos = Value.find(__T(" DIGITAL CAMERA"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + Value.substr(Pos + 15), true);
                continue;
            }
            }
            {
            auto Pos = Value.find(__T(" Digital Camera"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + Value.substr(Pos + 15), true);
                continue;
            }
            }
            {
            auto Pos = Value.find(__T(" Series"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + Value.substr(Pos + 7), true);
                continue;
            }
            }
            {
            auto Pos = Value.find(__T(" series"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + Value.substr(Pos + 7), true);
                continue;
            }
            }
            {
            auto Pos = Value.find(__T(" ZOOM"));
            if (Pos != string::npos) {
                Fill(Stream_General, StreamPos, Parameter, Value.substr(0, Pos) + __T(" Zoom") + Value.substr(Pos + 5), true);
                continue;
            }
            }
            break;
        }
    };
    RemoveUseless(General_Encoded_Application);
    RemoveUseless(General_Encoded_Application_CompanyName);
    RemoveUseless(General_Encoded_Application_Name);
    RemoveUseless(General_Encoded_Library);
    RemoveUseless(General_Encoded_Library_Name);
    RemoveUseless(General_Encoded_Hardware_CompanyName);
    RemoveUseless(General_Encoded_Hardware_Model);

    // Remove company legal suffixes and rename some company trademarks
    auto RemoveLegal = [&](size_t Parameter) {
        bool DoAgain;
        do {
            DoAgain = false;
            const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
            if (CompanyName.empty()) {
                return;
            }
            auto CompanyNameU = CompanyName;
            CompanyNameU.MakeUpperCase();
            for (const auto& CompanySuffix : NewlineRange(CompanySuffixes)) {
                auto len = CompanySuffix.size();
                auto CompanySuffixU = Ztring().From_UTF8(CompanySuffix.data(), len);
                len++;
                if (len < CompanyNameU.size() - 1
                    && (CompanyNameU[CompanyNameU.size() - len] == ' '
                        || CompanyNameU[CompanyNameU.size() - len] == ','
                        || CompanyNameU[CompanyNameU.size() - len] == '_'
                        || CompanyNameU[CompanyNameU.size() - len] == '-')
                    && CompanyNameU.find(CompanySuffixU, CompanyNameU.size() - len) != string::npos) {
                    if (len < CompanyName.size() && CompanyName[CompanyName.size() - (len + 1)] == ',') {
                        len++;
                    }
                    #if defined(UNICODE) || defined (_UNICODE)
                    Fill(Stream_General, StreamPos, Parameter, CompanyName.substr(0, CompanyName.size() - len), true);
                    #else
                    Fill(Stream_General, StreamPos, Parameter, CompanyName.substr(0, CompanyName.size() - len), true, true);
                    #endif // defined(UNICODE) || defined (_UNICODE)
                    DoAgain = true;
                    break;
                }
                if (CompanyNameU == CompanySuffixU) {
                    Clear(Stream_General, StreamPos, Parameter);
                }
            }
        } while (DoAgain);

        {
            const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
            auto CompanyNameU = CompanyName;
            CompanyNameU.MakeUpperCase();
            if (CompanyNameU.size() > 8
                && CompanyNameU[0] == '('
                && CompanyNameU[1] == 'C'
                && CompanyNameU[2] == ')'
                && CompanyNameU[3] == ' '
                && IsAsciiDigit(CompanyNameU[4])
                && IsAsciiDigit(CompanyNameU[5])
                && IsAsciiDigit(CompanyNameU[6])
                && IsAsciiDigit(CompanyNameU[7])
                && CompanyNameU[8] == ' ') {
                #if defined(UNICODE) || defined (_UNICODE)
                Fill(Stream_General, StreamPos, Parameter, CompanyName.substr(9), true);
                #else
                Fill(Stream_General, StreamPos, Parameter, CompanyName.substr(9), true, true);
                #endif // defined(UNICODE) || defined (_UNICODE)
            }
        }

        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
        auto CompanyNameU = CompanyName;
        CompanyNameU.MakeUpperCase();
        auto CompanyNameUS = CompanyNameU.To_UTF8();
        auto Result = Find_Replacement(CompanyNames_Replace, CompanyNameUS);
        if (Result.data()) {
            Fill(Stream_General, StreamPos, Parameter, Result.data(), Result.size(), true, true);
        }
    };
    RemoveLegal(General_Encoded_Hardware_CompanyName);
    RemoveLegal(General_Encoded_Library_CompanyName);
    RemoveLegal(General_Encoded_Application_CompanyName);
    auto General_StreamPos_Count = Count_Get(Stream_General, StreamPos);
    for (size_t i = 0; i < General_StreamPos_Count; i++) {
        const auto Name = Retrieve_Const(Stream_General, StreamPos, i, Info_Name).To_UTF8();
        if (Name == "LensMake") {
            RemoveLegal(i);
        }
    }

    // Move versions found in name field
    auto MoveVersion = [&](size_t Parameter_Source, size_t Parameter_Version, size_t Parameter_Name = 0) {
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Source);
        if (Name.empty()) {
            return;
        }
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, Parameter_Version);

        size_t Version_Pos = Name.size();
        auto Extra_Pos = Version_Pos;
        auto IsLikelyVersion = false;

        // Version string
        Ztring NameU = Name;
        NameU.MakeUpperCase();
        for (const auto& VersionPrefix : NewlineRange(VersionPrefixes)) {
            Ztring Prefix;
            Prefix.From_UTF8(VersionPrefix.data(), VersionPrefix.size());
            auto Prefix_Pos = NameU.rfind(Prefix);
            auto Prefix_Pos_End = Prefix_Pos + Prefix.size();
            if (Prefix_Pos != string::npos
             && (!Prefix_Pos
                || Prefix.front() == ','
                || NameU[Prefix_Pos - 1] == ' ')
             && Prefix_Pos_End != NameU.size()
             && (VersionPrefix.size() > 1 || NameU.find_first_of(__T(".-_")) != string::npos)
             && (NameU[Prefix_Pos_End] == '.'
                || NameU[Prefix_Pos_End] == ' '
                || (NameU[Prefix_Pos_End] >= '0' && NameU[Prefix_Pos_End] <= '9'))) {
                Version_Pos = Prefix_Pos_End;
                if (Version_Pos < NameU.size() && NameU[Version_Pos] == '.') {
                    Version_Pos++;
                }
                Extra_Pos = Prefix_Pos;
                IsLikelyVersion = true;
                break;
            }
        }

        if (!IsLikelyVersion) {
            // Is it only a version number?
            auto Space_Pos = NameU.find(' ');
            auto Dot_Pos = NameU.find('.');
            auto Digit_Pos = NameU.find_first_of(__T("0123456789"));
            auto NotDigit_Pos = NameU.find_first_not_of(__T("0123456789"));
            auto Letter_Pos = NameU.find_first_not_of(__T("0123456789."));
            if (Space_Pos == string::npos
                && (Dot_Pos != string::npos || (Letter_Pos == string::npos && NotDigit_Pos != string::npos))
                && ((Digit_Pos != string::npos && Digit_Pos > Dot_Pos)
                    || (Letter_Pos == string::npos || Letter_Pos > Dot_Pos)
                    || (Digit_Pos <= 1))) {
                Version_Pos = 0;
                Extra_Pos = 0;
                IsLikelyVersion = true;
            }
        }

        if (!IsLikelyVersion) {
            // Is it a version number with only digits at the end
            auto Space_Pos = NameU.rfind(' ');
            if (Space_Pos == string::npos) {
                Space_Pos = NameU.rfind('-');
            }
            if (Space_Pos != string::npos) {
                auto NonDigit_Pos = NameU.find_first_not_of(__T("0123456789."), Space_Pos + 1);
                if (NonDigit_Pos == string::npos && NameU.find('.') != string::npos) { // At least 1 dot for avoiding e.g. names with one digit
                    if (Space_Pos) {
                        auto NonDigit_Pos2 = NameU.find_last_not_of(__T("0123456789."), Space_Pos - 1);
                        if (NonDigit_Pos != string::npos && NonDigit_Pos != Space_Pos - 1) {
                            Space_Pos = NonDigit_Pos2;
                        }
                    }
                    Version_Pos = Space_Pos + 1;
                    Extra_Pos = Space_Pos;
                    IsLikelyVersion = true;
                }
            }
        }

        if (!IsLikelyVersion) {
            return;
        }

        auto Plus_Pos = Name.rfind(__T(" + "), Extra_Pos);
        auto And_Pos = Name.rfind(__T(" & "), Extra_Pos);
        auto Slash_Pos = Name.rfind(__T(" / "), Extra_Pos);
        auto Comma_Pos = Extra_Pos ? Name.rfind(__T(", "), Extra_Pos - 1) : string::npos;
        if (Plus_Pos != string::npos || And_Pos != string::npos || Comma_Pos != string::npos) {
            return; // TODO: handle complex string e.g. with 2 versions
        }

        auto VersionFromName = Name.substr(Version_Pos);
        if (VersionFromName != Version) {
            Fill(Stream_General, StreamPos, Parameter_Version, VersionFromName);
        }
        if (!Extra_Pos) {
            Clear(Stream_General, StreamPos, Parameter_Source);
            return; // No name found
        }
        Fill(Stream_General, StreamPos, Parameter_Name ? Parameter_Name : Parameter_Source, Name.substr(0, Extra_Pos), true);
    };
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version) == __T("Unknown")) {
        Clear(Stream_General, StreamPos, General_Encoded_Application_Version);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name).empty() && Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName) == __T("Google")) {
        const auto& Application = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application);
        if (Application.rfind(__T("HDR+ "), 0) == 0) {
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_Version, Application.substr(5));
            }
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, Application.substr(0, 4));
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_CompanyName, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName));
            }
        }
    }
    MoveVersion(General_Encoded_Library_Name, General_Encoded_Library_Version);
    MoveVersion(General_Encoded_Library, General_Encoded_Library_Version, General_Encoded_Library_Name);
    MoveVersion(General_Encoded_Application_Name, General_Encoded_Application_Version);
    MoveVersion(General_Encoded_Application, General_Encoded_Application_Version, General_Encoded_Application_Name);

    // Move company name found in name field
    auto MoveCompanyName = [&](size_t Parameter_Search, size_t Parameter_CompanyName) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
        const auto& Search = Retrieve_Const(Stream_General, StreamPos, Parameter_Search);

        auto CompanyNameU = CompanyName;
        auto SearchU = Search;
        CompanyNameU.MakeUpperCase();
        SearchU.MakeUpperCase();
        if (!CompanyNameU.empty() && SearchU.size() > CompanyNameU .size() && SearchU[CompanyNameU.size()] == ' ' && SearchU.rfind(CompanyNameU, 0) == 0) {
            Fill(Stream_General, StreamPos, Parameter_Search, Search.substr(CompanyName.size() + 1), true);
        }
        else {
            auto SearchUS = SearchU.To_UTF8();
            for (const auto& ToSearch : NewlineRange(CompanyNames)) {
                if (SearchUS.rfind(ToSearch.data(), 0, ToSearch.size()) == 0) {
                    const auto ToSearch_Len = ToSearch.size();
                    if (SearchUS.size() > ToSearch_Len
                        && (SearchUS[ToSearch_Len] == ' '
                        || SearchUS[ToSearch_Len] == '_')) {
                        auto CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
                        CompanyName.MakeUpperCase();
                        auto CompanyName2 = Ztring(Search.substr(0, ToSearch_Len));
                        CompanyName2.MakeUpperCase();
                        if (CompanyName2 != CompanyName) {
                            Fill(Stream_General, StreamPos, Parameter_CompanyName, Search.substr(0, ToSearch_Len));
                        }
                        Fill(Stream_General, StreamPos, Parameter_Search, Search.substr(ToSearch_Len + 1), true);
                        break;
                    }
                    if (!SearchUS.compare(0, SearchUS.size(), ToSearch.data(), ToSearch.size())) {
                        Fill(Stream_General, StreamPos, Parameter_CompanyName, Search);
                        Clear(Stream_General, StreamPos, Parameter_Search);
                    }
                }
            }
        }
    };
    MoveCompanyName(General_Encoded_Hardware_Model, General_Encoded_Hardware_CompanyName);
    MoveCompanyName(General_Encoded_Library_Name, General_Encoded_Library_CompanyName);
    MoveCompanyName(General_Encoded_Application_Name, General_Encoded_Application_CompanyName);

    // Remove capitalization
    auto RemoveCapitalization = [&](size_t Parameter) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter);
        if (CompanyName.size() < 2) {
            return;
        }
        auto CompanyNameU = CompanyName;
        CompanyNameU.MakeUpperCase();
        auto CompanyNameUS = CompanyNameU.To_UTF8();
        for (const auto& ToSearch : NewlineRange(CompanyNames)) {
            if (CompanyNameUS.size() > 3 && !CompanyNameUS.compare(0, CompanyNameUS.size(), ToSearch.data(), ToSearch.size())) {
                auto Result = Find_Replacement(CompanyNames_Replace, CompanyNameUS);
                if (Result.data()) {
                    break;
                }
                for (size_t i = 1; i < CompanyName.size(); i++) {
                    auto& Letter = CompanyNameUS[i];
                    if (Letter >= 'A' && Letter <= 'Z') {
                        CompanyNameUS[i] += 'a' - 'A';
                    }
                }
                Fill(Stream_General, StreamPos, Parameter, CompanyNameUS, true, true);
            }
        }
    };
    RemoveCapitalization(General_Encoded_Hardware_CompanyName);
    RemoveCapitalization(General_Encoded_Library_CompanyName);
    RemoveCapitalization(General_Encoded_Application_CompanyName);

    // Model is sometimes the actual name
    auto ModelToName = [&](size_t Parameter_CompanyName, size_t Parameter_Model, size_t Parameter_Name) {
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_Name).empty())
            return;
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model).To_UTF8();
        if (CompanyName == "Samsung") {
            if (Model.find("Galaxy ") == 0) {
                Fill(Stream_General, StreamPos, Parameter_Name, Model);
                Clear(Stream_General, StreamPos, Parameter_Model);
            }
        }
        if (CompanyName == "HMD") {
            if (Model.find("TA-") == string::npos) {
                Fill(Stream_General, StreamPos, Parameter_Name, Model);
                Clear(Stream_General, StreamPos, Parameter_Model);
            }
        }
    };
    ModelToName(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Model, General_Encoded_Hardware_Name);

    // Special case
    auto Special = [&](size_t Parameter_CompanyName, size_t Parameter_Name) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Name).To_UTF8();
        if (CompanyName == "HMD" && Name.find("Nokia ") == 0) {
            Fill(Stream_General, StreamPos, Parameter_CompanyName, Name.substr(0, 5), true, true);
            Fill(Stream_General, StreamPos, Parameter_Name, Name.substr(6), true, true);
        }
    };
    Special(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Name);

    // Model name
    auto FillModelName = [&](size_t Parameter_CompanyName, size_t Parameter_Model, size_t Parameter_Name) {
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_Name).empty())
            return;
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        const auto IsSamsung = CompanyName == "Samsung";
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model).To_UTF8();
        for (const auto& ToSearch : Model_Name) {
            if (CompanyName == ToSearch.CompanyName) {
                auto Model2 = Model;
                for (;;) {
                    bool Found{};
                    auto Result = Find_Replacement(ToSearch.Find, Model2);
                    if (Result.data()) {
                        Found = true;
                        string Result2;
                        if (ToSearch.Prefix) {
                            Result2 = ToSearch.Prefix;
                        }
                        Result2.append(Result.data(), Result.size());
                        Fill(Stream_General, StreamPos, Parameter_Name, Result2);
                    }
                    if (!Found && IsSamsung && Model2.size() >= 7) {
                        Model2.pop_back();
                        continue;
                    }
                    break;
                }
                break;
            }
        }
    };
    FillModelName(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Model, General_Encoded_Hardware_Name);

    // Attempt to derive Samsung Galaxy model number
    auto DetermineModel = [&](size_t Parameter_CompanyName, size_t Parameter_Name, size_t Parameter_Application, size_t Parameter_Model) {
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_Model).empty())
            return;
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        const auto& Application = Retrieve_Const(Stream_General, StreamPos, Parameter_Application).To_UTF8();
        if (CompanyName == "Samsung") {
            auto Model = "SM-" + Application.substr(0, Application.size() - 8);
            bool Found{};
            for (const auto& ToSearch : Model_Name) {
                if (CompanyName == ToSearch.CompanyName) {
                    auto Model2 = Model;
                    for (;;) {
                        auto Result = Find_Replacement(ToSearch.Find, Model2);
                        if (Result.data()) {
                            Found = true;
                            Fill(Stream_General, StreamPos, Parameter_Model, Model);
                        }
                        if (!Found && Model2.size() >= 7) {
                            Model2.pop_back();
                            continue;
                        }
                        break;
                    }
                    break;
                }
            }
        }
        };
    DetermineModel(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Name, General_Encoded_Application, General_Encoded_Hardware_Model);

    // Crosscheck
    auto Crosscheck = [&](size_t Parameter_CompanyName_Source, size_t Parameter_Start, bool CheckName) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Source);
        if (!CompanyName.empty()) {
            const auto Parameter = Parameter_Start;
            const auto Parameter_String = ++Parameter_Start;
            const auto Parameter_CompanyName = ++Parameter_Start;
            const auto Parameter_Name = ++Parameter_Start;
            const auto& Application_CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
            if (Application_CompanyName.empty()) {
                const auto& Application = Retrieve_Const(Stream_General, StreamPos, CheckName ? Parameter_Name : Parameter);
                auto CompanyNameU = CompanyName;
                CompanyNameU.MakeUpperCase();
                auto ApplicationU = Application;
                ApplicationU.MakeUpperCase();
                if (ApplicationU.size() > CompanyNameU.size() && ApplicationU[CompanyNameU.size()] == ' ' && ApplicationU.rfind(CompanyNameU, 0) == 0) {
                    Fill(Stream_General, StreamPos, Parameter_CompanyName, CompanyName);
                    Fill(Stream_General, StreamPos, Parameter_Name, Application.substr(CompanyName.size() + 1), true);
                    if (!CheckName) {
                        Clear(Stream_General, StreamPos, Parameter);
                    }
                }
            }
        }
    };
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Application, false);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Application, true);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Library, false);
    Crosscheck(General_Encoded_Hardware_CompanyName, General_Encoded_Library, true);
    Crosscheck(General_Encoded_Library_CompanyName, General_Encoded_Application, false);
    Crosscheck(General_Encoded_Library_CompanyName, General_Encoded_Application, true);

    // Copy name from other sources
    auto CopyName = [&](size_t IfParameter, size_t Parameter_Name, size_t Parameter_Source) {
        const auto& If = Retrieve_Const(Stream_General, StreamPos, IfParameter);
        if (If.empty()) {
            return;
        }
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Name);
        if (!Name.empty()) {
            return;
        }
        const auto& Source = Retrieve_Const(Stream_General, StreamPos, Parameter_Source);
        if (Source.empty()) {
            return;
        }
        Fill(Stream_General, StreamPos, Parameter_Name, Source);
    };
    CopyName(General_Encoded_Library_CompanyName, General_Encoded_Library_Name, General_Encoded_Hardware_Name);
    CopyName(General_Encoded_Application_CompanyName, General_Encoded_Application_Name, General_Encoded_Library_Name);
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name).empty() && !Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version).empty()) {
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model);
        if (Model.rfind(__T("iPhone "), 0) == 0) {
            if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName).empty()) {
                Fill(Stream_General, StreamPos, General_Encoded_Application_CompanyName, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName));
            }
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, "iOS");
        }
        CopyName(General_Encoded_Hardware_Name, General_Encoded_Application_Name, General_Encoded_Hardware_Name);
        CopyName(General_Encoded_Hardware_Model, General_Encoded_Application_Name, General_Encoded_Hardware_Model);
    }
    if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model).rfind(__T("Pentax "), 0) == 0) {
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_CompanyName).empty()
            && !Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name).empty()) {
            Fill(Stream_General, StreamPos, General_Encoded_Application_CompanyName, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_CompanyName));
        }
        if (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model).substr(7) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name)) {
            Fill(Stream_General, StreamPos, General_Encoded_Application_Name, Retrieve_Const(Stream_General, StreamPos, General_Encoded_Hardware_Model), true);
        }
    }

    // Copy company name from other sources
    auto CopyCompanyName = [&](size_t Parameter_CompanyName_Source, size_t Parameter_CompanyName_Dest, size_t Parameter_Name_Source, size_t Parameter_Name_Dest) {
        const auto& CompanyName_Dest = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Dest);
        const auto& CompanyName_Source = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName_Source);
        const auto& Name_Dest = Retrieve_Const(Stream_General, StreamPos, Parameter_Name_Dest);
        if (!CompanyName_Dest.empty() || CompanyName_Source.empty() || Name_Dest.empty()) {
            return;
        }
        const auto& Name_Source = Retrieve_Const(Stream_General, StreamPos, Parameter_Name_Source);
        if (Name_Source != Name_Dest) {
            return;
        }
        Fill(Stream_General, StreamPos, Parameter_CompanyName_Dest, CompanyName_Source);
    };
    CopyCompanyName(General_Encoded_Library_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Library_Name, General_Encoded_Application_Name);
    CopyCompanyName(General_Encoded_Application_CompanyName, General_Encoded_Library_CompanyName, General_Encoded_Application_Name, General_Encoded_Library_Name);
    CopyCompanyName(General_Encoded_Hardware_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Hardware_Name, General_Encoded_Application_Name);
    CopyCompanyName(General_Encoded_Hardware_CompanyName, General_Encoded_Application_CompanyName, General_Encoded_Hardware_Model, General_Encoded_Application_Name);

    // Check if it is really a library
    {
        /*
        const auto& Application_Name = Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Name);
        if (!Application_Name.empty()
         && Application_Name == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Name)
         && (Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Version).empty() || Retrieve_Const(Stream_General, StreamPos, General_Encoded_Application_Version) == Retrieve_Const(Stream_General, StreamPos, General_Encoded_Library_Version))) {
            Clear(Stream_General, StreamPos, General_Encoded_Library);
            Clear(Stream_General, StreamPos, General_Encoded_Library_CompanyName);
            Clear(Stream_General, StreamPos, General_Encoded_Library_Name);
            Clear(Stream_General, StreamPos, General_Encoded_Library_Version);
        }
        */
    }

    // Redundancy
    if (Retrieve_Const(Stream_General, 0, General_Encoded_Application_Name).empty() && Retrieve_Const(Stream_General, 0, General_Encoded_Application).empty())
    {
        if (Retrieve_Const(Stream_General, 0, General_Comment) == __T("Created with GIMP") || Retrieve_Const(Stream_General, 0, General_Description) == __T("Created with GIMP"))
            Fill(Stream_General, 0, General_Encoded_Application, "GIMP");
    }
    if (Retrieve_Const(Stream_General, 0, General_Encoded_Application_Name) == __T("GIMP") || Retrieve_Const(Stream_General, 0, General_Encoded_Application) == __T("GIMP") || !Retrieve_Const(Stream_General, 0, General_Encoded_Application).rfind(__T("GIMP ")))
    {
        if (Retrieve_Const(Stream_General, 0, General_Comment) == __T("Created with GIMP"))
            Clear(Stream_General, StreamPos, General_Comment);
        if (Retrieve_Const(Stream_General, 0, General_Description) == __T("Created with GIMP"))
            Clear(Stream_General, StreamPos, General_Description);
    }

    // Remove synonyms
    auto RemoveSynonyms = [&](size_t Parameter_CompanyName, size_t Parameter_Model) {
        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName).To_UTF8();
        for (const auto& ToSearch : Model_Replace) {
            if (CompanyName == ToSearch.CompanyName) {
                const auto& Model = Ztring(Retrieve_Const(Stream_General, StreamPos, Parameter_Model)).MakeUpperCase().To_UTF8();
                auto Result = Find_Replacement(ToSearch.Find, Model);
                if (Result.data()) {
                    Fill(Stream_General, StreamPos, Parameter_Model, Result.data(), Result.size(), true, true);
                }
            }
        }
        };
    RemoveSynonyms(General_Encoded_Hardware_CompanyName, General_Encoded_Hardware_Model);
    RemoveSynonyms(General_Encoded_Application_CompanyName, General_Encoded_Application_Name);

    // Create displayed string
    auto CreateString = [&](size_t Parameter_Start, bool HasModel = false) {
        const auto Parameter = Parameter_Start;
        const auto Parameter_String = ++Parameter_Start;
        if (!Retrieve_Const(Stream_General, StreamPos, Parameter_String).empty()) {
            return;
        }
        const auto Parameter_CompanyName = ++Parameter_Start;
        const auto Parameter_Name = ++Parameter_Start;
        const auto Parameter_Model = ++Parameter_Start;
        const auto Parameter_Version = Parameter_Start + HasModel;

        const auto& CompanyName = Retrieve_Const(Stream_General, StreamPos, Parameter_CompanyName);
        const auto& Name = Retrieve_Const(Stream_General, StreamPos, Parameter_Name);
        const auto& Model = Retrieve_Const(Stream_General, StreamPos, Parameter_Model);
        const auto& Version = Retrieve_Const(Stream_General, StreamPos, Parameter_Version);

        auto Value = CompanyName;
        if (!Name.empty() && !Value.empty()) {
            Value += ' ';
        }
        Value += Name;
        if (HasModel && !Model.empty())
        {
            if (!Value.empty())
                Value += ' ';
            if (!Name.empty())
                Value += '(';
            Value += Model;
            if (!Name.empty())
                Value += ')';
        }
        if (!Value.empty() && !Version.empty()) {
            Value += ' ';
        }
        Value += Version;
        Fill(Stream_General, StreamPos, Parameter_String, Value, true);
    };
    CreateString(General_Encoded_Hardware, true);
    CreateString(General_Encoded_Library);
    CreateString(General_Encoded_Application);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Video(size_t Pos)
{
    //Frame count
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==1 && Count_Get(Stream_Audio)==0)
            Fill(Stream_Video, 0, Video_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameCount from Duration and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_FrameCount).empty())
    {
        int64s Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_int64s();
        bool DurationFromGeneral;
        if (Duration==0)
        {
            Duration=Retrieve(Stream_General, 0, General_Duration).To_int64s();
            DurationFromGeneral=Retrieve(Stream_General, 0, General_Format)!=Retrieve(Stream_Video, Pos, Audio_Format);
        }
        else
            DurationFromGeneral=false;
        float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
        if (Duration && FrameRate)
        {
            Fill(Stream_Video, Pos, Video_FrameCount, Duration*FrameRate/1000, 0);
            if (DurationFromGeneral && Retrieve_Const(Stream_Audio, Pos, Audio_Format)!=Retrieve_Const(Stream_General, 0, General_Format))
            {
                Fill(Stream_Video, Pos, "FrameCount_Source", "General_Duration");
                Fill_SetOptions(Stream_Video, Pos, "FrameCount_Source", "N NTN");
            }
        }
    }

    //Duration from FrameCount and FrameRate
    if (Retrieve(Stream_Video, Pos, Video_Duration).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 FrameRate=Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64();
        if (FrameCount && FrameRate)
        {
            Fill(Stream_Video, Pos, Video_Duration, FrameCount/FrameRate*1000, 0);
            Ztring Source=Retrieve(Stream_Video, Pos, "FrameCount_Source");
            if (!Source.empty())
            {
                Fill(Stream_Video, Pos, "Duration_Source", Source);
                Fill_SetOptions(Stream_Video, Pos, "Duration_Source", "N NTN");
            }
        }
    }

    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Video, Pos, Video_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Video, Pos, Video_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Video, Pos, Video_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Video, Pos, Video_FrameRate, FrameCount/Duration, 3);
    }

    //Pixel Aspect Ratio forced from picture pixel size and Display Aspect Ratio
    if (Retrieve(Stream_Video, Pos, Video_PixelAspectRatio).empty())
    {
        const Ztring& DAR_S=Retrieve_Const(Stream_Video, Pos, Video_DisplayAspectRatio);
        float DAR=DAR_S.To_float32();
        float Width=Retrieve(Stream_Video, Pos, Video_Width).To_float32();
        float Height=Retrieve(Stream_Video, Pos, Video_Height).To_float32();
        if (DAR && Height && Width)
        {
            if (DAR_S==__T("1.778"))
                DAR=((float)16)/9; //More exact value
            if (DAR_S==__T("1.333"))
                DAR=((float)4)/3; //More exact value
            Fill(Stream_Video, Pos, Video_PixelAspectRatio, DAR/(((float32)Width)/Height));
        }
    }

    //Pixel Aspect Ratio forced to 1.000 if none
    if (Retrieve(Stream_Video, Pos, Video_PixelAspectRatio).empty() && Retrieve(Stream_Video, Pos, Video_DisplayAspectRatio).empty())
        Fill(Stream_Video, Pos, Video_PixelAspectRatio, 1.000);

    //Standard
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && (Retrieve(Stream_Video, Pos, Video_Width)==__T("720") || Retrieve(Stream_Video, Pos, Video_Width)==__T("704")))
    {
             if (Retrieve(Stream_Video, Pos, Video_Height)==__T("576") && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }
    if (Retrieve(Stream_Video, Pos, Video_Standard).empty() && Retrieve(Stream_Video, Pos, Video_Width)==__T("352"))
    {
             if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("576") || Retrieve(Stream_Video, Pos, Video_Height)==__T("288")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("25.000"))
            Fill(Stream_Video, Pos, Video_Standard, "PAL");
        else if ((Retrieve(Stream_Video, Pos, Video_Height)==__T("486") || Retrieve(Stream_Video, Pos, Video_Height)==__T("480") || Retrieve(Stream_Video, Pos, Video_Height)==__T("243") || Retrieve(Stream_Video, Pos, Video_Height)==__T("240")) && Retrieve(Stream_Video, Pos, Video_FrameRate)==__T("29.970"))
            Fill(Stream_Video, Pos, Video_Standard, "NTSC");
    }

    //Known ScanTypes
    if (Retrieve(Stream_Video, Pos, Video_ScanType).empty()
     && (Retrieve(Stream_Video, Pos, Video_Format)==__T("RED")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("CineForm")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("DPX")
      || Retrieve(Stream_Video, Pos, Video_Format)==__T("EXR")))
            Fill(Stream_Video, Pos, Video_ScanType, "Progressive");

    //Useless chroma subsampling
    if (Retrieve(Stream_Video, Pos, Video_ColorSpace)==__T("RGB")
     && Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)==__T("4:4:4"))
        Clear(Stream_Video, Pos, Video_ChromaSubsampling);

    //Chroma subsampling position
    if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_String).empty() && !Retrieve(Stream_Video, Pos, Video_ChromaSubsampling).empty())
    {
        if (Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position).empty())
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling));
        else
            Fill(Stream_Video, Pos, Video_ChromaSubsampling_String, Retrieve(Stream_Video, Pos, Video_ChromaSubsampling)+__T(" (")+ Retrieve(Stream_Video, Pos, Video_ChromaSubsampling_Position)+__T(')'));
    }

    //MasteringDisplay_Luminance
    {
        const auto& Luminance_Min = Retrieve_Const(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Min);
        const auto& Luminance_Max = Retrieve_Const(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Max);
        const auto& Luminance = Retrieve(Stream_Video, Pos, Video_MasteringDisplay_Luminance);
        if (Luminance_Min.empty() && Luminance_Max.empty() && !Luminance.empty())
        {
            Fill(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Min, Luminance.SubString(__T("min: "), __T(" ")));
            Fill(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Max, Luminance.SubString(__T("max: "), __T(" ")));
        }
        const auto& Luminance_Original_Min = Retrieve_Const(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Original_Min);
        const auto& Luminance_Original_Max = Retrieve_Const(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Original_Max);
        const auto& Luminance_Original = Retrieve(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Original);
        if (Luminance_Original_Min.empty() && Luminance_Original_Max.empty() && !Luminance_Original.empty())
        {
            Fill(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Original_Min, Luminance.SubString(__T("min: "), __T(" ")));
            Fill(Stream_Video, Pos, Video_MasteringDisplay_Luminance_Original_Max, Luminance.SubString(__T("max: "), __T(" ")));
        }
    }

    //Commercial name
    if (Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility).rfind(__T("HDR10"), 0)==0
     && ((!Retrieve(Stream_Video, Pos, Video_BitDepth).empty() && Retrieve(Stream_Video, Pos, Video_BitDepth).To_int64u()<10) //e.g. ProRes has not bitdepth info
     || Retrieve(Stream_Video, Pos, Video_colour_primaries)!=__T("BT.2020")
     || Retrieve(Stream_Video, Pos, Video_transfer_characteristics)!=__T("PQ")
     || Retrieve(Stream_Video, Pos, Video_MasteringDisplay_ColorPrimaries).empty()
        ))
    {
        //We actually fill HDR10/HDR10+ by default, so it will be removed below if not fitting in the color related rules
        Clear(Stream_Video, Pos, Video_HDR_Format_Compatibility);
    }
    if (Retrieve(Stream_Video, Pos, Video_HDR_Format_String).empty())
    {
        ZtringList Summary;
        Summary.Separator_Set(0, __T(" / "));
        Summary.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format));
        ZtringList Commercial=Summary;
        size_t DolbyVision_Pos=(size_t)-1;
        for (size_t j=0; j<Summary.size(); j++)
            if (Summary[j]==__T("Dolby Vision"))
                DolbyVision_Pos=j;
        if (!Summary.empty())
        {
            ZtringList HDR_Format_Compatibility;
            HDR_Format_Compatibility.Separator_Set(0, __T(" / "));
            HDR_Format_Compatibility.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility));
            HDR_Format_Compatibility.resize(Summary.size());
            ZtringList ToAdd;
            ToAdd.Separator_Set(0, __T(" / "));
            for (size_t i=Video_HDR_Format_String+1; i<=Video_HDR_Format_Compression; i++)
            {
                ToAdd.Write(Retrieve(Stream_Video, Pos, i));
                ToAdd.resize(Summary.size());
                for (size_t j=0; j<Summary.size(); j++)
                {
                    if (!ToAdd[j].empty())
                    {
                        switch (i)
                        {
                            case Video_HDR_Format_Version: Summary[j]+=__T(", Version "); break;
                            case Video_HDR_Format_Level: Summary[j]+=__T('.'); break;
                            case Video_HDR_Format_Compression: ToAdd[j][0]+=0x20; if (ToAdd[j].size()==4) ToAdd[j].resize(2); ToAdd[j]+=__T(" metadata compression"); [[fallthrough]];
                            default: Summary[j] += __T(", ");
                        }
                        Summary[j]+=ToAdd[j];
                        if (i==Video_HDR_Format_Version && j==DolbyVision_Pos)
                        {
                            ToAdd.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Profile));
                            if (j<ToAdd.size())
                            {
                                const Ztring& Profile=ToAdd[j];
                                size_t Profile_Dot=Profile.find(__T('.'));
                                if (Profile_Dot!=string::npos)
                                {
                                    Profile_Dot++;
                                    if (Profile_Dot<Profile.size() && Profile[Profile_Dot]==__T('0'))
                                        Profile_Dot++;
                                    Summary[j]+=__T(", Profile ");
                                    Summary[j]+=Profile.substr(Profile_Dot);
                                    ToAdd.Write(Retrieve(Stream_Video, Pos, Video_HDR_Format_Compatibility));
                                    if (j<ToAdd.size())
                                    {
                                        const Ztring& Compatibility=ToAdd[j];
                                        size_t Compatibility_Pos=DolbyVision_Compatibility_Pos(Compatibility);
                                        if (Compatibility_Pos!=size_t()-1)
                                        {
                                            Summary[j]+=__T('.');
                                            Summary[j]+=Ztring::ToZtring(Compatibility_Pos, 16);
                                        }
                                    }
                                }
                            }
                            ToAdd.resize(Summary.size());
                        }
                    }
                }
            }
            for (size_t j=0; j<Summary.size(); j++)
                if (!HDR_Format_Compatibility[j].empty())
                {
                    Summary[j]+=__T(", ")+HDR_Format_Compatibility[j]+__T(" compatible");
                    Commercial[j]=HDR_Format_Compatibility[j];
                    if (!Commercial[j].empty())
                    {
                        auto Commercial_Reduce=Commercial[j].find(__T(' '));
                        if (Commercial_Reduce<Commercial[j].size()-1 && Commercial[j][Commercial_Reduce+1]>='0' && Commercial[j][Commercial_Reduce+1]<='9')
                            Commercial_Reduce=Commercial[j].find(__T(' '), Commercial_Reduce+1);
                        if (Commercial_Reduce!=string::npos)
                            Commercial[j].resize(Commercial_Reduce);
                    }
                }
            Fill(Stream_Video, Pos, Video_HDR_Format_String, Summary.Read());
            Fill(Stream_Video, Pos, Video_HDR_Format_Commercial, Commercial.Read());
        }
    }
    #if defined(MEDIAINFO_VC3_YES)
        if (Retrieve(Stream_Video, Pos, Video_Format_Commercial_IfAny).empty() && Retrieve(Stream_Video, Pos, Video_Format)==__T("VC-3") && Retrieve(Stream_Video, Pos, Video_Format_Profile).find(__T("HD"))==0)
        {
            //http://www.avid.com/static/resources/US/documents/dnxhd.pdf
            int64u Height=Retrieve(Stream_Video, Pos, Video_Height).To_int64u();
            int64u BitRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_BitRate).To_float64()/1000000);
            int64u FrameRate=float64_int64s(Retrieve(Stream_Video, Pos, Video_FrameRate).To_float64());
            int64u BitRate_Final=0;
            if (Height>=900 && Height<=1300)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=271 && BitRate<311) //291
                        BitRate_Final=290;
                    if (BitRate>=80 && BitRate<100) //90
                        BitRate_Final=90;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=222 && BitRate<262) //242
                        BitRate_Final=240;
                    if (BitRate>=65 && BitRate<85) //75
                        BitRate_Final=75;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=420 && BitRate<440) //440
                        BitRate_Final=440;
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //100
                        BitRate_Final=100;
                    if (BitRate>=40 && BitRate<50) //45
                        BitRate_Final=45;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=347 && BitRate<387) //367
                        BitRate_Final=365;
                    if (BitRate>=164 && BitRate<204) //184
                        BitRate_Final=185;
                    if (BitRate>=111 && BitRate<131) //121
                        BitRate_Final=120;
                    if (BitRate>=74 && BitRate<94) //84
                        BitRate_Final=85;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=332 && BitRate<372) //352
                        BitRate_Final=350;
                    if (BitRate>=156 && BitRate<196) //176
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //116
                        BitRate_Final=116;
                    if (BitRate>=70 && BitRate<90) //80
                        BitRate_Final=80;
                    if (BitRate>=31 && BitRate<41) //36
                        BitRate_Final=36;
                }
            }
            if (Height>=600 && Height<=800)
            {
                if (FrameRate==60)
                {
                    if (BitRate>=200 && BitRate<240) //220
                        BitRate_Final=220;
                    if (BitRate>=130 && BitRate<160) //145
                        BitRate_Final=145;
                    if (BitRate>=90 && BitRate<110) //110
                        BitRate_Final=100;
                }
                if (FrameRate==50)
                {
                    if (BitRate>=155 && BitRate<195) //175
                        BitRate_Final=175;
                    if (BitRate>=105 && BitRate<125) //115
                        BitRate_Final=115;
                    if (BitRate>=75 && BitRate<95) //85
                        BitRate_Final=85;
                }
                if (FrameRate==30)
                {
                    if (BitRate>=100 && BitRate<120) //110
                        BitRate_Final=110;
                    if (BitRate>=62 && BitRate<82) //72
                        BitRate_Final=75;
                    if (BitRate>=44 && BitRate<56) //51
                        BitRate_Final=50;
                }
                if (FrameRate==25)
                {
                    if (BitRate>=82 && BitRate<102) //92
                        BitRate_Final=90;
                    if (BitRate>=55 && BitRate<65) //60
                        BitRate_Final=60;
                    if (BitRate>=38 && BitRate<48) //43
                        BitRate_Final=45;
                }
                if (FrameRate==24)
                {
                    if (BitRate>=78 && BitRate<98) //88
                        BitRate_Final=90;
                    if (BitRate>=53 && BitRate<63) //58
                        BitRate_Final=60;
                    if (BitRate>=36 && BitRate<46) //41
                        BitRate_Final=41;
                }
            }

            if (BitRate_Final)
            {
                int64u BitDepth=Retrieve(Stream_Video, Pos, Video_BitDepth).To_int64u();
                if (BitDepth==8 || BitDepth==10)
                    Fill(Stream_Video, Pos, Video_Format_Commercial_IfAny, __T("DNxHD ")+Ztring::ToZtring(BitRate_Final)+(BitDepth==10?__T("x"):__T(""))); //"x"=10-bit
            }
        }
        if (Retrieve(Stream_Video, Pos, Video_Format_Commercial_IfAny).empty() && Retrieve(Stream_Video, Pos, Video_Format)==__T("VC-3") && Retrieve(Stream_Video, Pos, Video_Format_Profile).find(__T("RI@"))==0)
        {
            Fill(Stream_Video, Pos, Video_Format_Commercial_IfAny, __T("DNxHR ")+Retrieve(Stream_Video, Pos, Video_Format_Profile).substr(3));
        }
    #endif //defined(MEDIAINFO_VC3_YES)
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Audio(size_t Pos)
{
    // 
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded)==Retrieve(Stream_Audio, Pos, Audio_StreamSize))
        Clear(Stream_Audio, Pos, Audio_StreamSize_Encoded);
    if (Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded)==Retrieve(Stream_Audio, Pos, Audio_BitRate))
        Clear(Stream_Audio, Pos, Audio_BitRate_Encoded);

    //Dolby ED2 merge
    if (Retrieve(Stream_Audio, Pos, Audio_Format)==__T("Dolby ED2"))
    {
        int64u BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_int64u();
        int64u BitRate_Encoded=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_int64u();
        int64u StreamSize=Retrieve(Stream_Audio, Pos, Audio_StreamSize).To_int64u();
        int64u StreamSize_Encoded=Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded).To_int64u();
        for (size_t i=Pos+1; i<Count_Get(Stream_Audio);)
        {
            size_t OtherID_Count;
            Ztring OtherStreamOrder;
            Ztring OtherID;
            Ztring OtherID_String;
            if (Retrieve_Const(Stream_Audio, i, Audio_Format)==__T("Dolby ED2"))
            {
                //if (Retrieve_Const(Stream_Audio, i, Audio_Channel_s_).To_int64u())
                if (!Retrieve_Const(Stream_Audio, i, "Presentation0").empty())
                    break; // It is the next ED2
                OtherID_Count=0;
                OtherStreamOrder=Retrieve(Stream_Audio, i, Audio_StreamOrder);
                OtherID=Retrieve(Stream_Audio, i, Audio_ID);
                OtherID_String =Retrieve(Stream_Audio, i, Audio_ID_String);
            }
            if (i+7<Count_Get(Stream_Audio) // 8 tracks Dolby E
             && Retrieve_Const(Stream_Audio, i  , Audio_Format)==__T("Dolby E")
             && Retrieve_Const(Stream_Audio, i+7, Audio_Format)==__T("Dolby E"))
            {
                Ztring NextStreamOrder=Retrieve_Const(Stream_Audio, i, Audio_StreamOrder);
                Ztring NextID=Retrieve_Const(Stream_Audio, i, Audio_ID);
                size_t NextID_DashPos=NextID.rfind(__T('-'));
                if (NextID_DashPos!=(size_t)-1)
                    NextID.erase(NextID_DashPos);
                if (Retrieve_Const(Stream_Audio, i+7, Audio_ID)==NextID+__T("-8"))
                {
                    OtherID_Count=7;
                    OtherStreamOrder=NextStreamOrder;
                    OtherID=NextID;
                }
                NextID=Retrieve_Const(Stream_Audio, i, Audio_ID_String);
                NextID_DashPos=NextID.rfind(__T('-'));
                if (NextID_DashPos!=(size_t)-1)
                    NextID.erase(NextID_DashPos);
                if (Retrieve_Const(Stream_Audio, i+7, Audio_ID_String)==NextID+__T("-8"))
                {
                    OtherID_String=NextID;
                }
            }
            if (OtherID.empty())
                break;

            size_t OtherID_DashPos=OtherID.rfind(__T('-'));
            if (OtherID_DashPos!=(size_t)-1)
                OtherID.erase(0, OtherID_DashPos+1);
            if (!OtherID.empty() && OtherID[0]==__T('(') && OtherID[OtherID.size()-1]==__T(')'))
            {
                OtherID.resize(OtherID.size()-1);
                OtherID.erase(0, 1);
            }
            Ztring ID=Retrieve(Stream_Audio, Pos, Audio_ID);
            if (!ID.empty() && ID[ID.size()-1]==__T(')'))
            {
                ID.resize(ID.size()-1);
                ID+=__T(" / ");
                ID+=OtherID;
                ID+=__T(')');
                Fill(Stream_Audio, Pos, Audio_ID, ID, true);
            }
            else
            {
                Ztring CurrentID_String=Retrieve(Stream_Audio, Pos, Audio_ID_String);
                if (Retrieve_Const(Stream_General, 0, General_Format)==__T("MPEG-TS"))
                {
                    auto ProgramSeparator=OtherStreamOrder.find(__T('-'));
                    if (ProgramSeparator!=string::npos)
                        OtherStreamOrder.erase(0, ProgramSeparator+1);
                }
                Fill(Stream_Audio, Pos, Audio_StreamOrder, OtherStreamOrder);
                Fill(Stream_Audio, Pos, Audio_ID, OtherID);
                Fill(Stream_Audio, Pos, Audio_ID_String, CurrentID_String+__T(" / ")+OtherID_String, true);
            }
            for (size_t j=i+OtherID_Count; j>=i; j--)
            {
                BitRate+=Retrieve(Stream_Audio, j, Audio_BitRate).To_int64u();
                BitRate_Encoded+=Retrieve(Stream_Audio, j, Audio_BitRate_Encoded).To_int64u();
                StreamSize+=Retrieve(Stream_Audio, j, Audio_StreamSize).To_int64u();
                StreamSize_Encoded+=Retrieve(Stream_Audio, j, Audio_StreamSize_Encoded).To_int64u();
                Stream_Erase(Stream_Audio, j);
            }

            ZtringList List[6];
            for (size_t j=0; j<6; j++)
                List[j].Separator_Set(0, __T(" / "));
            List[0].Write(Get(Stream_Menu, 0, __T("Format")));
            List[1].Write(Get(Stream_Menu, 0, __T("Format/String")));
            List[2].Write(Get(Stream_Menu, 0, __T("List_StreamKind")));
            List[3].Write(Get(Stream_Menu, 0, __T("List_StreamPos")));
            List[4].Write(Get(Stream_Menu, 0, __T("List")));
            List[5].Write(Get(Stream_Menu, 0, __T("List/String")));
            bool IsNok=false;
            for (size_t j=0; j<6; j++)
                if (!List[j].empty() && List[j].size()!=List[3].size())
                    IsNok=true;
            if (!IsNok && !List[2].empty() && List[2].size()==List[3].size())
            {
                size_t Audio_Begin;
                for (Audio_Begin=0; Audio_Begin <List[2].size(); Audio_Begin++)
                    if (List[2][Audio_Begin]==__T("2"))
                        break;
                if (Audio_Begin!=List[2].size())
                {
                    for (size_t j=0; j<6; j++)
                        if (!List[j].empty() && Audio_Begin+i<List[j].size())
                            List[j].erase(List[j].begin()+Audio_Begin+i);
                    size_t Audio_End;
                    for (Audio_End=Audio_Begin+1; Audio_End<List[2].size(); Audio_End++)
                        if (List[2][Audio_End]!=__T("2"))
                            break;
                    for (size_t j=Audio_Begin+i; j<Audio_End; j++)
                        List[3][j].From_Number(List[3][j].To_int32u()-1-OtherID_Count);
                    Fill(Stream_Menu, 0, "Format", List[0].Read(), true);
                    Fill(Stream_Menu, 0, "Format/String", List[1].Read(), true);
                    Fill(Stream_Menu, 0, "List_StreamKind", List[2].Read(), true);
                    Fill(Stream_Menu, 0, "List_StreamPos", List[3].Read(), true);
                    Fill(Stream_Menu, 0, "List", List[4].Read(), true);
                    Fill(Stream_Menu, 0, "List/String", List[5].Read(), true);
                }
            }
        }
        if (BitRate)
            Fill(Stream_Audio, Pos, Audio_BitRate, BitRate, 10, true);
        if (BitRate_Encoded)
            Fill(Stream_Audio, Pos, Audio_BitRate_Encoded, BitRate_Encoded, 10, true);
        if (StreamSize)
            Fill(Stream_Audio, Pos, Audio_StreamSize, StreamSize, 10, true);
        if (StreamSize_Encoded)
            Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, StreamSize_Encoded, 10, true);
    }

    //Channels
    if (Retrieve(Stream_Audio, Pos, Audio_Channel_s_).empty())
    {
        const Ztring& CodecID=Retrieve(Stream_Audio, Pos, Audio_CodecID);
        if (CodecID==__T("samr")
         || CodecID==__T("sawb")
         || CodecID==__T("7A21")
         || CodecID==__T("7A22"))
        Fill(Stream_Audio, Pos, Audio_Channel_s_, 1); //AMR is always with 1 channel
    }

    //SamplingCount
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingCount).empty())
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        bool DurationFromGeneral; 
        if (Duration==0)
        {
            Duration=Retrieve(Stream_General, 0, General_Duration).To_float64();
            DurationFromGeneral=Retrieve(Stream_General, 0, General_Format)!=Retrieve(Stream_Audio, Pos, Audio_Format);
        }
        else
            DurationFromGeneral=false;
        float64 SamplingRate=Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64();
        if (Duration && SamplingRate)
        {
            Fill(Stream_Audio, Pos, Audio_SamplingCount, Duration/1000*SamplingRate, 0);
            if (DurationFromGeneral && Retrieve_Const(Stream_Audio, Pos, Audio_Format)!=Retrieve_Const(Stream_General, 0, General_Format))
            {
                Fill(Stream_Audio, Pos, "SamplingCount_Source", "General_Duration");
                Fill_SetOptions(Stream_Audio, Pos, "SamplingCount_Source", "N NTN");
            }
        }
    }

    //Frame count
    if (Retrieve(Stream_Audio, Pos, Audio_FrameCount).empty() && Frame_Count_NotParsedIncluded!=(int64u)-1 && File_Offset+Buffer_Size==File_Size)
    {
        if (Count_Get(Stream_Video)==0 && Count_Get(Stream_Audio)==1)
            Fill(Stream_Audio, 0, Audio_FrameCount, Frame_Count_NotParsedIncluded);
    }

    //FrameRate same as SampleRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64() == Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64())
        Clear(Stream_Audio, Pos, Audio_FrameRate);

    //SamplingRate
    if (Retrieve(Stream_Audio, Pos, Audio_SamplingRate).empty())
    {
        float64 BitDepth=Retrieve(Stream_Audio, Pos, Audio_BitDepth).To_float64();
        float64 Channels=Retrieve(Stream_Audio, Pos, Audio_Channel_s_).To_float64();
        float64 BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
        if (BitDepth && Channels && BitRate)
            Fill(Stream_Audio, Pos, Audio_SamplingRate, BitRate/Channels/BitDepth, 0);
    }

    //SamplesPerFrames
    if (Retrieve(Stream_Audio, Pos, Audio_SamplesPerFrame).empty())
    {
        float64 FrameRate=Retrieve(Stream_Audio, Pos, Audio_FrameRate).To_float64();
        float64 SamplingRate=0;
        ZtringList SamplingRates;
        SamplingRates.Separator_Set(0, " / ");
        SamplingRates.Write(Retrieve(Stream_Audio, Pos, Audio_SamplingRate));
        for (size_t i=0; i<SamplingRates.size(); ++i)
        {
            SamplingRate = SamplingRates[i].To_float64();
            if (SamplingRate)
                break; // Using the first valid one
        }
        if (FrameRate && SamplingRate && FrameRate!=SamplingRate)
        {
            float64 SamplesPerFrameF=SamplingRate/FrameRate;
            Ztring SamplesPerFrame;
            if (SamplesPerFrameF>1601 && SamplesPerFrameF<1602)
                SamplesPerFrame = __T("1601.6"); // Usually this is 29.970 fps PCM. TODO: check if it is OK in all cases
            else if (SamplesPerFrameF>800 && SamplesPerFrameF<801)
                SamplesPerFrame = __T("800.8"); // Usually this is 59.940 fps PCM. TODO: check if it is OK in all cases
            else
                SamplesPerFrame.From_Number(SamplesPerFrameF, 0);
            Fill(Stream_Audio, Pos, Audio_SamplesPerFrame, SamplesPerFrame);
        }
    }

    //ChannelLayout
    if (Retrieve_Const(Stream_Audio, Pos, Audio_ChannelLayout).empty())
    {
        ZtringList ChannelLayout_List;
        ChannelLayout_List.Separator_Set(0, __T(" "));
        ChannelLayout_List.Write(Retrieve_Const(Stream_Audio, Pos, Audio_ChannelLayout));
        size_t ChannelLayout_List_SizeBefore=ChannelLayout_List.size();
        
        size_t NumberOfSubstreams=(size_t)Retrieve_Const(Stream_Audio, Pos, "NumberOfSubstreams").To_int64u();
        for (size_t i=0; i<NumberOfSubstreams; i++)
        {
            static const char* const Places[]={ "ChannelLayout", "BedChannelConfiguration" };
            static constexpr size_t Places_Size=sizeof(Places)/sizeof(decltype(*Places));
            for (const auto Place : Places)
            {
                ZtringList AdditionaChannelLayout_List;
                AdditionaChannelLayout_List.Separator_Set(0, __T(" "));
                AdditionaChannelLayout_List.Write(Retrieve_Const(Stream_Audio, Pos, ("Substream"+std::to_string(i)+' '+Place).c_str()));
                for (auto& AdditionaChannelLayout_Item: AdditionaChannelLayout_List)
                {
                    if (std::find(ChannelLayout_List.cbegin(), ChannelLayout_List.cend(), AdditionaChannelLayout_Item)==ChannelLayout_List.cend())
                        ChannelLayout_List.push_back(std::move(AdditionaChannelLayout_Item));
                }
            }
        }
        if (ChannelLayout_List.size()!=ChannelLayout_List_SizeBefore)
        {
            Fill(Stream_Audio, Pos, Audio_Channel_s_, ChannelLayout_List.size(), 10, true);
            Clear(Stream_Audio, Pos, Audio_ChannelPositions);
            Fill(Stream_Audio, Pos, Audio_ChannelLayout, ChannelLayout_List.Read(), true);
        }
    }

    //Channel(s)
    if (Retrieve_Const(Stream_Audio, Pos, Audio_Channel_s_).empty())
    {
        size_t NumberOfSubstreams=(size_t)Retrieve_Const(Stream_Audio, Pos, "NumberOfSubstreams").To_int64u();
        if (NumberOfSubstreams==1)
        {
            auto Channels=Retrieve_Const(Stream_Audio, Pos, "Substream0 Channel(s)").To_int32u();
            if (Channels)
                Fill(Stream_Audio, Pos, Audio_Channel_s_, Channels);
        }
    }

    //Duration
    if (Retrieve(Stream_Audio, Pos, Audio_Duration).empty())
    {
        float64 SamplingRate=Retrieve(Stream_Audio, Pos, Audio_SamplingRate).To_float64();
        if (SamplingRate)
        {
            float64 Duration=Retrieve(Stream_Audio, Pos, Audio_SamplingCount).To_float64()*1000/SamplingRate;
            if (Duration)
            {
                Fill(Stream_Audio, Pos, Audio_Duration, Duration, 0);
                Ztring Source=Retrieve(Stream_Audio, Pos, "SamplingCount_Source");
                if (!Source.empty())
                {
                    Fill(Stream_Audio, Pos, "Duration_Source", Source);
                    Fill_SetOptions(Stream_Audio, Pos, "Duration_Source", "N NTN");
                }
            }
        }
    }

    //Stream size
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode)==__T("CBR"))
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        float64 BitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
        if (Duration && BitRate)
            Fill(Stream_Audio, Pos, Audio_StreamSize, Duration*BitRate/8/1000, 0, true);
    }
    if (Retrieve(Stream_Audio, Pos, Audio_StreamSize_Encoded).empty() && !Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode)==__T("CBR"))
    {
        float64 Duration=Retrieve(Stream_Audio, Pos, Audio_Duration).To_float64();
        float64 BitRate_Encoded=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_float64();
        if (Duration)
            Fill(Stream_Audio, Pos, Audio_StreamSize_Encoded, Duration*BitRate_Encoded/8/1000, 0, true);
    }

    //CBR/VBR
    if (Retrieve(Stream_Audio, Pos, Audio_BitRate_Mode).empty() && !Retrieve(Stream_Audio, Pos, Audio_Codec).empty())
    {
        Ztring Z1=MediaInfoLib::Config.Codec_Get(Retrieve(Stream_Audio, Pos, Audio_Codec), InfoCodec_BitRate_Mode, Stream_Audio);
        if (!Z1.empty())
            Fill(Stream_Audio, Pos, Audio_BitRate_Mode, Z1);
    }

    //Commercial name
    if (Retrieve(Stream_Audio, Pos, Audio_Format_Commercial_IfAny).empty() && Retrieve(Stream_Audio, Pos, Audio_Format)==__T("USAC") && Retrieve(Stream_Audio, Pos, Audio_Format_Profile).rfind(__T("Extended HE AAC@"), 0)==0)
    {
        Fill(Stream_Audio, Pos, Audio_Format_Commercial_IfAny, "xHE-AAC");
    }

    //Timestamp to timecode
    auto FramesPerSecondF=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate").To_float32();
    auto DropFrame=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata AssociatedVideo_FrameRate_DropFrame")==__T("Yes");
    auto SamplingRate=Retrieve_Const(Stream_Audio, 0, Audio_SamplingRate).To_int32u();
    const auto& FirstFrameOfAction=Retrieve_Const(Stream_Audio, 0, "Dolby_Atmos_Metadata FirstFrameOfAction");
    if (!FirstFrameOfAction.empty())
    {
        auto TC_FirstFrameOfAction=TimeCode(FirstFrameOfAction.To_UTF8());
        Merge_FillTimeCode(*this, "Dolby_Atmos_Metadata FirstFrameOfAction", TC_FirstFrameOfAction, FramesPerSecondF, DropFrame, TimeCode::Ceil, SamplingRate);
    }
    const auto& Start=Retrieve_Const(Stream_Audio, 0, "Programme0 Start");
    if (!Start.empty())
    {
        auto TC_End=TimeCode(Start.To_UTF8());
        Merge_FillTimeCode(*this, "Programme0 Start", TC_End, FramesPerSecondF, DropFrame, TimeCode::Floor, SamplingRate);
    }
    const auto& End=Retrieve_Const(Stream_Audio, 0, "Programme0 End");
    if (!End.empty())
    {
        auto TC_End=TimeCode(End.To_UTF8());
        Merge_FillTimeCode(*this, "Programme0 End", TC_End, FramesPerSecondF, DropFrame, TimeCode::Ceil, SamplingRate);
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Text(size_t Pos)
{
    //FrameRate from FrameCount and Duration
    if (Retrieve(Stream_Text, Pos, Text_FrameRate).empty())
    {
        int64u FrameCount=Retrieve(Stream_Text, Pos, Text_FrameCount).To_int64u();
        float64 Duration=Retrieve(Stream_Text, Pos, Text_Duration).To_float64()/1000;
        if (FrameCount && Duration)
           Fill(Stream_Text, Pos, Text_FrameRate, FrameCount/Duration, 3);
    }

    //FrameCount from Duration and FrameRate
    if (Retrieve(Stream_Text, Pos, Text_FrameCount).empty())
    {
        float64 Duration=Retrieve(Stream_Text, Pos, Text_Duration).To_float64()/1000;
        float64 FrameRate=Retrieve(Stream_Text, Pos, Text_FrameRate).To_float64();
        if (Duration && FrameRate)
           Fill(Stream_Text, Pos, Text_FrameCount, float64_int64s(Duration*FrameRate));
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Other(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Image(size_t Pos)
{
    //Commercial name
    if (Retrieve(Stream_Image, Pos, Image_HDR_Format_Compatibility).rfind(__T("HDR10"), 0)==0
     && ((!Retrieve(Stream_Image, Pos, Image_BitDepth).empty() && Retrieve(Stream_Image, Pos, Image_BitDepth).To_int64u()<10) //e.g. ProRes has not bitdepth info
      || Retrieve(Stream_Image, Pos, Image_colour_primaries)!=__T("BT.2020")
      || Retrieve(Stream_Image, Pos, Image_transfer_characteristics)!=__T("PQ")
      || Retrieve(Stream_Image, Pos, Image_MasteringDisplay_ColorPrimaries).empty()
        ))
    {
        //We actually fill HDR10/HDR10+ by default, so it will be removed below if not fitting in the color related rules
        Clear(Stream_Image, Pos, Image_HDR_Format_Compatibility);
    }
    if (Retrieve(Stream_Image, Pos, Image_HDR_Format_String).empty())
    {
        ZtringList Summary;
        Summary.Separator_Set(0, __T(" / "));
        Summary.Write(Retrieve(Stream_Image, Pos, Image_HDR_Format));
        ZtringList Commercial=Summary;
        if (!Summary.empty())
        {
            ZtringList HDR_Format_Compatibility;
            HDR_Format_Compatibility.Separator_Set(0, __T(" / "));
            HDR_Format_Compatibility.Write(Retrieve(Stream_Image, Pos, Image_HDR_Format_Compatibility));
            HDR_Format_Compatibility.resize(Summary.size());
            ZtringList ToAdd;
            ToAdd.Separator_Set(0, __T(" / "));
            for (size_t i=Image_HDR_Format_String+1; i<=Image_HDR_Format_Settings; i++)
            {
                ToAdd.Write(Retrieve(Stream_Image, Pos, i));
                ToAdd.resize(Summary.size());
                for (size_t j=0; j<Summary.size(); j++)
                {
                    if (!ToAdd[j].empty())
                    {
                        switch (i)
                        {
                            case Image_HDR_Format_Version: Summary[j]+=__T(", Version "); break;
                            case Image_HDR_Format_Level: Summary[j]+=__T('.'); break;
                            default: Summary[j] += __T(", ");
                        }
                        Summary[j]+=ToAdd[j];
                    }
                }
            }
            for (size_t j=0; j<Summary.size(); j++)
                if (!HDR_Format_Compatibility[j].empty())
                {
                    Summary[j]+=__T(", ")+HDR_Format_Compatibility[j]+__T(" compatible");
                    Commercial[j]=HDR_Format_Compatibility[j];
                    if (!Commercial[j].empty())
                    {
                        auto Commercial_Reduce=Commercial[j].find(__T(' '));
                        if (Commercial_Reduce<Commercial[j].size()-1 && Commercial[j][Commercial_Reduce+1]>='0' && Commercial[j][Commercial_Reduce+1]<='9')
                            Commercial_Reduce=Commercial[j].find(__T(' '), Commercial_Reduce+1);
                        if (Commercial_Reduce!=string::npos)
                            Commercial[j].resize(Commercial_Reduce);
                    }
                }
            Fill(Stream_Image, Pos, Image_HDR_Format_String, Summary.Read());
            Fill(Stream_Image, Pos, Image_HDR_Format_Commercial, Commercial.Read());
        }
    }

    if (Retrieve(Stream_Image, Pos, Image_Type_String).empty())
    {
        const auto& Type=Retrieve_Const(Stream_Image, Pos, Image_Type);
        if (!Type.empty())
        {
            auto Type_String=__T("Type_")+Type;
            auto Type_String2=MediaInfoLib::Config.Language_Get(Type_String);
            if (Type_String2==Type_String)
                Type_String2=Type;
            Fill(Stream_Image, Pos, Image_Type_String, Type_String2);
        }
    }
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_StreamOnly_Menu(size_t UNUSED(StreamPos))
{
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_InterStreams()
{
    //Duration if General not filled
    if (Retrieve(Stream_General, 0, General_Duration).empty())
    {
        int64u Duration=0;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
            {
                if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).empty())
                {
                    int64u Duration_Stream=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Duration)).To_int64u();
                    if (Duration_Stream>Duration)
                        Duration=Duration_Stream;
                }
            }

        //Filling
        if (Duration>0)
            Fill(Stream_General, 0, General_Duration, Duration);
    }

    //(*Stream) size if all stream sizes are OK
    if (Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        int64u StreamSize_Total=0;
        bool IsOK=true;
        //For all streams (Generic)
        for (size_t StreamKind=Stream_Video; StreamKind<Stream_Max; StreamKind++)
        {
                for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind); Pos++)
                {
                    if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize_Encoded)).To_int64u();
                    else if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Source_StreamSize)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_Source_StreamSize)).To_int64u();
                    else if (!Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).empty())
                        StreamSize_Total+=Retrieve((stream_t)StreamKind, Pos, Fill_Parameter((stream_t)StreamKind, Generic_StreamSize)).To_int64u();
                    else if (StreamKind!=Stream_Other && StreamKind!=Stream_Menu) //They have no big size, we never calculate them
                        IsOK=false; //StreamSize not available for 1 stream, we can't calculate
                }
        }

        //Filling
        auto StreamSize=File_Size-StreamSize_Total;
        if (IsOK && StreamSize_Total>0 && StreamSize_Total<File_Size && (File_Size==StreamSize_Total || !StreamSize || StreamSize>=4)) //to avoid strange behavior due to rounding, TODO: avoid rounding
            Fill(Stream_General, 0, General_StreamSize, StreamSize);
    }

    //OverallBitRate if we have one Audio stream with bitrate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Count_Get(Stream_Video) == 0 && Count_Get(Stream_Audio) == 1 && Retrieve(Stream_Audio, 0, Audio_BitRate).To_int64u() != 0 && (Retrieve(Stream_General, 0, General_Format) == Retrieve(Stream_Audio, 0, Audio_Format) || !Retrieve(Stream_General, 0, General_HeaderSize).empty()))
    {
        const Ztring& EncodedBitRate=Retrieve_Const(Stream_Audio, 0, Audio_BitRate_Encoded);
        Fill(Stream_General, 0, General_OverallBitRate, EncodedBitRate.empty()?Retrieve_Const(Stream_Audio, 0, Audio_BitRate):EncodedBitRate);
    }

    //OverallBitRate if Duration
    if (Retrieve(Stream_General, 0, General_OverallBitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()!=0 && !Retrieve(Stream_General, 0, General_FileSize).empty())
    {
        float64 Duration=0;
        if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_Duration)==Retrieve(Stream_Video, 0, General_Duration) && !Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
        {
            int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
            float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
            if (FrameCount && FrameRate)
                Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
        }
        if (Duration==0)
            Duration=Retrieve(Stream_General, 0, General_Duration).To_float64();
        Fill(Stream_General, 0, General_OverallBitRate, Retrieve(Stream_General, 0, General_FileSize).To_int64u()*8*1000/Duration, 0);
    }

    //Duration if OverallBitRate
    if (Retrieve(Stream_General, 0, General_Duration).empty() && Retrieve(Stream_General, 0, General_OverallBitRate).To_int64u()!=0)
        Fill(Stream_General, 0, General_Duration, Retrieve(Stream_General, 0, General_FileSize).To_float64()*8*1000/Retrieve(Stream_General, 0, General_OverallBitRate).To_float64(), 0);

    //Video bitrate can be the nominal one if <4s (bitrate estimation is not enough precise
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()<4000)
    {
        Fill(Stream_Video, 0, Video_BitRate, Retrieve(Stream_Video, 0, Video_BitRate_Nominal));
        Clear(Stream_Video, 0, Video_BitRate_Nominal);
    }

    //Video bitrate if we have all audio bitrates and overal bitrate
    if (Count_Get(Stream_Video)==1 && Retrieve(Stream_General, 0, General_OverallBitRate).size()>4 && Retrieve(Stream_Video, 0, Video_BitRate).empty() && Retrieve(Stream_Video, 0, Video_BitRate_Encoded).empty() && Retrieve(Stream_General, 0, General_Duration).To_int64u()>=1000) //BitRate is > 10 000 and Duration>10s, to avoid strange behavior
    {
        double GeneralBitRate_Ratio=0.98;  //Default container overhead=2%
        int32u GeneralBitRate_Minus=5000;  //5000 bps because of a "classic" stream overhead
        double VideoBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u VideoBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double AudioBitRate_Ratio  =0.98;  //Default container overhead=2%
        int32u AudioBitRate_Minus  =2000;  //2000 bps because of a "classic" stream overhead
        double TextBitRate_Ratio   =0.98;  //Default container overhead=2%
        int32u TextBitRate_Minus   =2000;  //2000 bps because of a "classic" stream overhead
        //Specific value depends of Container
        if (StreamSource==IsStream)
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-TS"))
        {
            GeneralBitRate_Ratio=0.98;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.97;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.96;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.96;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MPEG-PS"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (MediaInfoLib::Config.Format_Get(Retrieve(Stream_General, 0, General_Format), InfoFormat_KindofFormat)==__T("MPEG-4"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =1;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =1;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =1;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("Matroska"))
        {
            GeneralBitRate_Ratio=0.99;
            GeneralBitRate_Minus=0;
            VideoBitRate_Ratio  =0.99;
            VideoBitRate_Minus  =0;
            AudioBitRate_Ratio  =0.99;
            AudioBitRate_Minus  =0;
            TextBitRate_Ratio   =0.99;
            TextBitRate_Minus   =0;
        }
        if (Get(Stream_General, 0, __T("Format"))==__T("MXF"))
        {
            GeneralBitRate_Ratio=1;
            GeneralBitRate_Minus=1000;
            VideoBitRate_Ratio  =1.00;
            VideoBitRate_Minus  =1000;
            AudioBitRate_Ratio  =1.00;
            AudioBitRate_Minus  =1000;
            TextBitRate_Ratio   =1.00;
            TextBitRate_Minus   =1000;
        }

        //Testing
        float64 VideoBitRate=Retrieve(Stream_General, 0, General_OverallBitRate).To_float64()*GeneralBitRate_Ratio-GeneralBitRate_Minus;
        bool VideobitRateIsValid=true;
        for (size_t Pos=0; Pos<Count_Get(Stream_Audio); Pos++)
        {
            float64 AudioBitRate=0;
            if (!Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate_Encoded).To_float64();
            else if (!Retrieve(Stream_Audio, Pos, Audio_BitRate).empty() && Retrieve(Stream_Audio, Pos, Audio_BitRate)[0]<=__T('9')) //Note: quick test if it is a number
                AudioBitRate=Retrieve(Stream_Audio, Pos, Audio_BitRate).To_float64();
            else
                VideobitRateIsValid=false;
            if (VideobitRateIsValid && AudioBitRate_Ratio)
                VideoBitRate-=AudioBitRate/AudioBitRate_Ratio+AudioBitRate_Minus;
        }
        for (size_t Pos=0; Pos<Count_Get(Stream_Text); Pos++)
        {
            float64 TextBitRate;
            if (Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).empty())
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate).To_float64();
            else
                TextBitRate=Retrieve(Stream_Text, Pos, Text_BitRate_Encoded).To_float64();
            if (TextBitRate_Ratio)
                VideoBitRate-=TextBitRate/TextBitRate_Ratio+TextBitRate_Minus;
            else
                VideoBitRate-=1000; //Estimation: Text stream are not often big
        }
        if (VideobitRateIsValid && VideoBitRate>=10000) //to avoid strange behavior
        {
            VideoBitRate=VideoBitRate*VideoBitRate_Ratio-VideoBitRate_Minus;
            Fill(Stream_Video, 0, Video_BitRate, VideoBitRate, 0); //Default container overhead=2%
            if (Retrieve(Stream_Video, 0, Video_StreamSize).empty() && !Retrieve(Stream_Video, 0, Video_Duration).empty())
            {
                float64 Duration=0;
                if (!Retrieve(Stream_Video, 0, Video_FrameCount).empty() && !Retrieve(Stream_Video, 0, Video_FrameRate).empty())
                {
                    int64u FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount).To_int64u();
                    float64 FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate).To_float64();
                    if (FrameCount && FrameRate)
                        Duration=FrameCount*1000/FrameRate; //More precise (example: 1 frame at 29.97 fps)
                }
                if (Duration==0)
                    Duration=Retrieve(Stream_Video, 0, Video_Duration).To_float64();
                if (Duration)
                {
                    int64u StreamSize=float64_int64s(VideoBitRate/8*Duration/1000);
                    if (StreamSource==IsStream && File_Size!=(int64u)-1 && StreamSize>=File_Size*0.99)
                        StreamSize=File_Size;
                    Fill(Stream_Video, 0, Video_StreamSize, StreamSize);
                }
            }
        }
    }

    //General stream size if we have all streamsize
    if (File_Size!=(int64u)-1 && Retrieve(Stream_General, 0, General_StreamSize).empty())
    {
        //Testing
        int64s StreamSize=File_Size;
        bool StreamSizeIsValid=true;
        for (size_t StreamKind_Pos=Stream_General+1; StreamKind_Pos<Stream_Menu; StreamKind_Pos++)
            for (size_t Pos=0; Pos<Count_Get((stream_t)StreamKind_Pos); Pos++)
            {
                int64u StreamXX_StreamSize=0;
                if (!Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize_Encoded)).empty())
                    StreamXX_StreamSize+=Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize_Encoded)).To_int64u();
                else if (!Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize)).empty())
                    StreamXX_StreamSize+=Retrieve((stream_t)StreamKind_Pos, Pos, Fill_Parameter((stream_t)StreamKind_Pos, Generic_StreamSize)).To_int64u();
                if (StreamXX_StreamSize>0 || StreamKind_Pos==Stream_Text)
                    StreamSize-=StreamXX_StreamSize;
                else
                    StreamSizeIsValid=false;
            }
        if (StreamSizeIsValid && (!StreamSize || StreamSize>=4)) //to avoid strange behavior due to rounding, TODO: avoid rounding
            Fill(Stream_General, 0, General_StreamSize, StreamSize);
    }

    //General_OverallBitRate_Mode
    if (Retrieve(Stream_General, 0, General_OverallBitRate_Mode).empty())
    {
        bool IsValid=false;
        bool IsCBR=true;
        bool IsVBR=false;
        for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Menu; StreamKind++)
        {
            if (StreamKind==Stream_Image)
                continue;
            for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            {
                if (!IsValid)
                    IsValid=true;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))!=__T("CBR"))
                    IsCBR=false;
                if (Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_BitRate_Mode))==__T("VBR"))
                    IsVBR=true;
            }
        }
        if (IsValid)
        {
            if (IsCBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "CBR");
            if (IsVBR)
                Fill(Stream_General, 0, General_OverallBitRate_Mode, "VBR");
        }
    }

    //FrameRate if General not filled
    if (Retrieve(Stream_General, 0, General_FrameRate).empty() && Count_Get(Stream_Video))
    {
        Ztring FrameRate=Retrieve(Stream_Video, 0, Video_FrameRate);
        bool IsOk=true;
        if (FrameRate.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameRate2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameRate));
                    if (!FrameRate2.empty() && FrameRate2!=FrameRate)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameRate, FrameRate);
    }

    //FrameCount if General not filled
    if (Retrieve(Stream_General, 0, General_FrameCount).empty() && Count_Get(Stream_Video) && Retrieve(Stream_General, 0, "IsTruncated").empty())
    {
        Ztring FrameCount=Retrieve(Stream_Video, 0, Video_FrameCount);
        bool IsOk=true;
        if (FrameCount.empty())
        {
            for (size_t StreamKind=Stream_General+1; StreamKind<Stream_Max; StreamKind++)
                for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
                {
                    Ztring FrameCount2=Retrieve((stream_t)StreamKind, StreamPos, Fill_Parameter((stream_t)StreamKind, Generic_FrameCount));
                    if (!FrameCount2.empty() && FrameCount2!=FrameCount)
                        IsOk=false;
                }
        }
        if (IsOk)
            Fill(Stream_General, 0, General_FrameCount, FrameCount);
    }

    //Tags
    Tags();
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable()
{
    //Generic
    for (size_t StreamKind=Stream_General; StreamKind<Stream_Max; StreamKind++)
        for (size_t StreamPos=0; StreamPos<Count_Get((stream_t)StreamKind); StreamPos++)
            for (size_t Parameter=0; Parameter<Count_Get((stream_t)StreamKind, StreamPos); Parameter++)
                Streams_Finish_HumanReadable_PerStream((stream_t)StreamKind, StreamPos, Parameter);
}

//---------------------------------------------------------------------------
void File__Analyze::Streams_Finish_HumanReadable_PerStream(stream_t StreamKind, size_t StreamPos, size_t Parameter)
{
    const Ztring ParameterName=Retrieve(StreamKind, StreamPos, Parameter, Info_Name);
    const Ztring Value=Retrieve(StreamKind, StreamPos, Parameter, Info_Text);

    //Strings
    const Ztring &List_Measure_Value=MediaInfoLib::Config.Info_Get(StreamKind).Read(Parameter, Info_Measure);
            if (List_Measure_Value==__T(" byte"))
        FileSize_FileSize123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" bps") || List_Measure_Value==__T(" Hz"))
        Kilo_Kilo123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T(" ms"))
        Duration_Duration123(StreamKind, StreamPos, Parameter);
    else if (List_Measure_Value==__T("Yes"))
        YesNo_YesNo(StreamKind, StreamPos, Parameter);
    else
        Value_Value123(StreamKind, StreamPos, Parameter);

    //BitRate_Mode / OverallBitRate_Mode
    if (ParameterName==(StreamKind==Stream_General?__T("OverallBitRate_Mode"):__T("BitRate_Mode")) && MediaInfoLib::Config.ReadByHuman_Get())
    {
        Clear(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String");

        ZtringList List;
        List.Separator_Set(0, __T(" / "));
        List.Write(Retrieve(StreamKind, StreamPos, Parameter));

        //Per value
        for (size_t Pos=0; Pos<List.size(); Pos++)
            List[Pos]=MediaInfoLib::Config.Language_Get(Ztring(__T("BitRate_Mode_"))+List[Pos]);

        const Ztring Translated=List.Read();
        Fill(StreamKind, StreamPos, StreamKind==Stream_General?"OverallBitRate_Mode/String":"BitRate_Mode/String", Translated.find(__T("BitRate_Mode_"))?Translated:Value);
    }

    //Encoded_Application
    if ((   ParameterName==__T("Encoded_Application")
         || ParameterName==__T("Encoded_Application_CompanyName")
         || ParameterName==__T("Encoded_Application_Name")
         || ParameterName==__T("Encoded_Application_Version")
         || ParameterName==__T("Encoded_Application_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Application/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Application_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Application_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Application_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Application_Date");
        if (!CompanyName.empty() || !Name.empty())
        {
            Ztring String=CompanyName;
            if (!CompanyName.empty() && !Name.empty())
                String+=' ';
            String+=Name;
            if (!Version.empty())
            {
                String+=__T(" ");
                String+=Version;
            }
            if (!Date.empty())
            {
                String+=__T(" (");
                String+=Date;
                String+=__T(")");
            }
            Fill(StreamKind, StreamPos, "Encoded_Application/String", String, true);
        }
        else
            Fill(StreamKind, StreamPos, "Encoded_Application/String", Retrieve(StreamKind, StreamPos, "Encoded_Application"), true);
    }

    //Encoded_Library
    if ((   ParameterName==__T("Encoded_Library")
         || ParameterName==__T("Encoded_Library_CompanyName")
         || ParameterName==__T("Encoded_Library_Name")
         || ParameterName==__T("Encoded_Library_Version")
         || ParameterName==__T("Encoded_Library_Date"))
        && Retrieve(StreamKind, StreamPos, "Encoded_Library/String").empty())
    {
        Ztring CompanyName=Retrieve(StreamKind, StreamPos, "Encoded_Library_CompanyName");
        Ztring Name=Retrieve(StreamKind, StreamPos, "Encoded_Library_Name");
        Ztring Version=Retrieve(StreamKind, StreamPos, "Encoded_Library_Version");
        Ztring Date=Retrieve(StreamKind, StreamPos, "Encoded_Library_Date");
        Ztring Encoded_Library=Retrieve(StreamKind, StreamPos, "Encoded_Library");
        Fill(StreamKind, StreamPos, "Encoded_Library/String", File__Analyze_Encoded_Library_String(CompanyName, Name, Version, Date, Encoded_Library), true);
    }

    //Format_Settings_Matrix
    if (StreamKind==Stream_Video && Parameter==Video_Format_Settings_Matrix)
    {
        Fill(Stream_Video, StreamPos, Video_Format_Settings_Matrix_String, MediaInfoLib::Config.Language_Get_Translate(__T("Format_Settings_Matrix_"), Value), true);
    }

    //Scan type
    if (StreamKind==Stream_Video && Parameter==Video_ScanType)
    {
        Fill(Stream_Video, StreamPos, Video_ScanType_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_Original)
    {
        Fill(Stream_Video, StreamPos, Video_ScanType_Original_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanType_StoreMethod)
    {
        Ztring ToTranslate=Ztring(__T("StoreMethod_"))+Value;
        if (!Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock).empty())
            ToTranslate+=__T('_')+Retrieve(Stream_Video, StreamPos, Video_ScanType_StoreMethod_FieldsPerBlock);
        Ztring Translated=MediaInfoLib::Config.Language_Get(ToTranslate);
        Fill(Stream_Video, StreamPos, Video_ScanType_StoreMethod_String, Translated.find(__T("StoreMethod_"))?Translated:Value, true);
    }

    //Scan order
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Stored)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Stored_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }
    if (StreamKind==Stream_Video && Parameter==Video_ScanOrder_Original)
    {
        Fill(Stream_Video, StreamPos, Video_ScanOrder_Original_String, MediaInfoLib::Config.Language_Get_Translate(__T("Interlaced_"), Value), true);
    }

    //Interlacement
    if (StreamKind==Stream_Video && Parameter==Video_Interlacement)
    {
        const Ztring &Z1=Retrieve(Stream_Video, StreamPos, Video_Interlacement);
        if (Z1.size()==3)
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Ztring(__T("Interlaced_"))+Z1));
        else
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, MediaInfoLib::Config.Language_Get(Z1));
        if (Retrieve(Stream_Video, StreamPos, Video_Interlacement_String).empty())
            Fill(Stream_Video, StreamPos, Video_Interlacement_String, Z1, true);
    }

    //FrameRate_Mode
    if (StreamKind==Stream_Video && Parameter==Video_FrameRate_Mode)
    {
        Fill(Stream_Video, StreamPos, Video_FrameRate_Mode_String, MediaInfoLib::Config.Language_Get_Translate(__T("FrameRate_Mode_"), Value), true);
    }

    //Compression_Mode
    if (Parameter==Fill_Parameter(StreamKind, Generic_Compression_Mode))
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Compression_Mode_String), MediaInfoLib::Config.Language_Get_Translate(__T("Compression_Mode_"), Value), true);
    }

    //Delay_Source
    if (Parameter==Fill_Parameter(StreamKind, Generic_Delay_Source))
    {
        Fill(StreamKind, StreamPos, Fill_Parameter(StreamKind, Generic_Delay_Source_String), MediaInfoLib::Config.Language_Get_Translate(__T("Delay_Source_"), Value), true);
    }

    //Gop_OpenClosed
    if (StreamKind==Stream_Video && (Parameter==Video_Gop_OpenClosed || Parameter==Video_Gop_OpenClosed_FirstFrame))
    {
        Fill(Stream_Video, StreamPos, Parameter+1, MediaInfoLib::Config.Language_Get_Translate(__T("Gop_OpenClosed_"), Value), true);
    }
}

} //NameSpace
