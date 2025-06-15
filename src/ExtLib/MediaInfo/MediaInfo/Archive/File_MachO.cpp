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
#if defined(MEDIAINFO_MACHO_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Archive/File_MachO.h"
#include <cmath>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Infos
//***************************************************************************

//---------------------------------------------------------------------------
const char* MachO_Magic(int32u Magic) {
    switch (Magic) {
    case 0xCEFAEDFE: return "32-bit LE";
    case 0xCFFAEDFE: return "64-bit LE";
    case 0xFEEDFACE: return "32-bit BE";
    case 0xFEEDFACF: return "64-bit BE";
    case 0xCAFEBABE: return "Universal 32-bit";
    case 0xCAFEBABF: return "Universal 64-bit";
    default: return "";
    }
}

//---------------------------------------------------------------------------
// https://github.com/opensource-apple/cctools/blob/master/include/mach/machine.h
string MachO_cputype(int32u cputype) {
    switch (cputype) {
    case 0x00000001: return "VAX";
    case 0x00000002: return "ROMP";
    case 0x00000004: return "NS32032";
    case 0x00000005: return "NS32332";
    case 0x00000006: return "MC680x0";
    case 0x00000007: return "Intel i386";
    case 0x01000007: return "AMD x86-64";
    case 0x00000008: return "MIPS";
    case 0x00000009: return "NS32352";
    case 0x0000000B: return "HP-PA";
    case 0x0000000C: return "ARM";
    case 0x0100000C: return "ARM64";
    case 0x0000000D: return "MC88000";
    case 0x0000000E: return "SPARC";
    case 0x0000000F: return "Intel i860 (big-endian)";
    case 0x00000010: return "Intel i860 (little-endian)";
    case 0x00000011: return "RS/6000";
    case 0x00000012: return "PowerPC";
    case 0x01000012: return "PowerPC 64-bit";
    default: return "0x" + Ztring().From_CC4(cputype).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Mach-O#Mach-O_header
string MachO_filetype(int32u filetype) {
    switch (filetype) {
    case 0x00000001: return "Relocatable object";
    case 0x00000002: return "Demand paged executable";
    case 0x00000003: return "Shared library";
    case 0x00000004: return "Core";
    case 0x00000005: return "Preloaded executable";
    case 0x00000006: return "Dynamically bound shared library";
    case 0x00000007: return "Dynamic link editor";
    case 0x00000008: return "Dynamically bound bundle";
    case 0x00000009: return "Shared library stub for static linking only";
    case 0x0000000A: return "Companion file with only debug sections";
    case 0x0000000B: return "x86_64 kexts";
    default: return "0x" + Ztring().From_CC4(filetype).To_UTF8();
    }
}

//---------------------------------------------------------------------------
// Byte sizes
static const char* Unit_Letters = "KMGTPEZY";
string Power2_WithUnits(int32u Offset) {
    int32u Quotient  = Offset / 10;
    int32u Remainder = Offset % 10;
    if (!Quotient) {
        return std::to_string(Offset) + " B";
    }
    if (Quotient > 8) {
        Quotient = 8;
        Remainder = Offset / 80;
    }
    return std::to_string(1 << Remainder) + ' ' + Unit_Letters[Quotient - 1] + "iB";
}

//***************************************************************************
// Buffer - File header
//***************************************************************************

//---------------------------------------------------------------------------
bool File_MachO::FileHeader_Begin() {
    //Element_Size
    if (File_Size < 32) {
        Reject();
        return false;
    }
    if (Buffer_Size < 4) {
        return false; //Must wait for more data
    }

    auto MagicValue = CC4(Buffer);
    switch (MagicValue) {
    case 0xCEFAEDFE: // 32-bit Mach-O (little-endian)
    case 0xCFFAEDFE: // 64-bit Mach-O (little-endian)
    case 0xFEEDFACE: // 32-bit Mach-O (big-endian)
    case 0xFEEDFACF: // 64-bit Mach-O (big-endian)
    case 0xCAFEBABE: // 32-bit Universal fat binary
    case 0xCAFEBABF: // 64-bit Universal fat binary
        break;
    default:
        Reject();
        return false;
    }

    //All should be OK...
    return true;
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_MachO::Read_Buffer_Continue() {
    //Parsing
    if (File_Offset + Buffer_Offset != 0) {
        Element_Begin1("arch");
    }
    int32u magic, cputype{}, filetype{}, nfat_arch{};
    Element_Begin1("Header");
    Get_B4 (magic,                                              "magic"); Param_Info1(MachO_Magic(magic)); Element_Info1(MachO_Magic(magic));
    switch (magic) {
    case 0xCEFAEDFE: // 32-bit Mach-O (little-endian)
    case 0xCFFAEDFE: // 64-bit Mach-O (little-endian)
        Get_L4 (cputype,                                        "cputype");
        #if MEDIAINFO_TRACE
        Param_Info1(MachO_cputype(cputype));
        Element_Info1(MachO_cputype(cputype));
        if (File_Offset + Buffer_Offset != 0) {
            Element_Level--;
            Param_Info1(MachO_Magic(magic));
            Element_Info1(MachO_cputype(cputype));
            Element_Level++;
        }
        #endif
        Skip_L4(                                                "cpusubtype");
        Get_L4 (filetype,                                       "filetype");
        Skip_L4(                                                "ncmds");
        Skip_L4(                                                "sizeofcmds");
        Skip_L4(                                                "flags");
        if (magic == 0xCFFAEDFE)
            Skip_L4(                                            "reserved");
        break;
    case 0xFEEDFACE: // 32-bit Mach-O (big-endian)
    case 0xFEEDFACF: // 64-bit Mach-O (big-endian)
        Get_B4 (cputype,                                        "cputype"); Param_Info1(MachO_cputype(cputype)); Element_Info1(MachO_cputype(cputype));
        #if MEDIAINFO_TRACE
        Param_Info1(MachO_cputype(cputype));
        Element_Info1(MachO_cputype(cputype));
        if (File_Offset + Buffer_Offset != 0) {
            Element_Level--;
            Param_Info1(MachO_Magic(magic));
            Element_Info1(MachO_cputype(cputype));
            Element_Level++;
        }
        #endif
        Skip_B4(                                                "cpusubtype");
        Get_B4 (filetype,                                       "filetype");
        Skip_B4(                                                "ncmds");
        Skip_B4(                                                "sizeofcmds");
        Skip_B4(                                                "flags");
        if (magic == 0xFEEDFACF)
            Skip_B4(                                            "reserved");
        break;
    case 0xCAFEBABE: // 32-bit Universal fat binary
    case 0xCAFEBABF: // 64-bit Universal fat binary
        if (File_Offset + Buffer_Offset != 0) {
            Reject();
            Element_End0();
            return;
        }
        Get_B4 (nfat_arch,                                      "nfat_arch");
        if (!nfat_arch || nfat_arch > (File_Size - 4) / (magic == 0xCAFEBABE ? 12 : 24)) {
            Reject();
            Element_End0();
            return;
        }
        for (int32u i = 0; i < nfat_arch; ++i) {
            Element_Begin1("arch");
            BinaryInfo binary{};
            int64u offset;
            Get_B4 (binary.cputype,                             "cputype"); Param_Info1(MachO_cputype(binary.cputype)); Element_Info1(MachO_cputype(binary.cputype));
            Skip_B4(                                            "cpusubtype");
            if (magic == 0xCAFEBABE) {
                int32u offset32{}, size32{};
                Get_B4 (offset32,                               "offset");
                Get_B4 (size32,                                 "size");
                offset = offset32;
                binary.size = size32;
                Get_B4 (binary.align,                           "align"); Param_Info1(Power2_WithUnits(binary.align));
            } else {
                Get_B8 (offset,                                 "offset");
                Get_B8 (binary.size,                            "size");
                Get_B4 (binary.align,                           "align"); Param_Info1(Power2_WithUnits(binary.align));
                Skip_B4(                                        "reserved");
            }
            if (binary.align >= 64 || binary.size < (magic == 0xCAFEBABE ? 28 : 32) || offset > File_Size || binary.size > File_Size - offset || Universal_Positions.find(offset) != Universal_Positions.end()) {
                Reject();
                Element_End0();
                Element_End0();
                return;
            }
            Universal_Positions[offset] = binary;
            Element_End0();
        }
        break;
    }
    Element_End0();
    #if MEDIAINFO_TRACE
    if (File_Offset + Buffer_Offset == 0) {
        if (!Universal_Positions.empty()) {
            Skip_XX(Universal_Positions.begin()->first - Element_Offset, "Padding");
        }
    }
    else {
        Skip_XX(Universal_Current->second.size - Element_Offset, "(Note parsed)");
        Element_End0();
    }
    #endif

    FILLING_BEGIN();
        Accept();
        if (Universal_Positions.empty()) {
            Fill(Stream_General, 0, General_Format, "Mach-O");
            Fill(Stream_General, 0, General_Format_Profile, MachO_filetype(filetype));
            Fill(Stream_General, 0, General_Format_Profile, MachO_cputype(cputype));
            Finish();
        }
        else if (File_Offset + Buffer_Offset == 0) {
            Fill(Stream_General, 0, General_Format, "Mach-O Universal");
            Universal_Current = Universal_Positions.begin();
            GoTo(Universal_Current->first);
        }
        else {
            Stream_Prepare(Stream_Other);
            Fill(Stream_Other, StreamPos_Last, Other_Type, "Binary");
            Fill(Stream_Other, StreamPos_Last, Other_Format, "Mach-O");
            Fill(Stream_Other, StreamPos_Last, Other_Format_Profile, MachO_filetype(filetype));
            Fill(Stream_Other, StreamPos_Last, Other_Format_Profile, MachO_cputype(Universal_Current->second.cputype));
            Fill(Stream_Other, StreamPos_Last, Other_StreamSize, Universal_Current->second.size);
            Fill(Stream_Other, StreamPos_Last, "Alignment", 1 << Universal_Current->second.align);
            Fill_SetOptions(Stream_Other, StreamPos_Last, "Alignment", "N NIY");
            Fill(Stream_Other, StreamPos_Last, "Alignment/String", Power2_WithUnits(Universal_Current->second.align));
            Fill_SetOptions(Stream_Other, StreamPos_Last, "Alignment/String", "Y NTN");
            Universal_Current++;
            if (Universal_Current == Universal_Positions.end()) {
                if (File_Size != (int64u)-1) {
                    Skip_XX(File_Size - (File_Offset + Buffer_Offset + Element_Offset), "Padding");
                }
                Finish();
            }
            else {
                GoTo(Universal_Current->first);
            }

        }
    FILLING_END();
}

} //NameSpace

#endif //MEDIAINFO_MACHO_YES
