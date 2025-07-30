/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about PNG files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_PngH
#define MediaInfo_File_PngH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/TimeCode.h"
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Png
//***************************************************************************

class File_Png : public File__Analyze
{
public :
    //Constructor/Destructor
    File_Png();

    //In
    stream_t                    StreamKind;

private :
    //Streams management
    void Streams_Accept();
    void Streams_Finish();

    //Buffer - File header
    bool FileHeader_Begin();
    void FileHeader_Parse();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test() {return Demux_UnpacketizeContainer_Test_OneFramePerFile();}
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    void Read_Buffer_Unsynched();
    void Read_Buffer_AfterParsing();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID) {return Read_Buffer_Seek_OneFramePerFile(Method, Value, ID);}
    #endif //MEDIAINFO_SEEK

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void IDAT();
    void IEND() { Data_Common(); }
    void IHDR();
    void JDAT();
    void JHDR() { Data_Common(); }
    void MEND() { Data_Common(); }
    void MHDR();
    void PLTE() { Data_Common(); }
    void acTL() { Data_Common(); }
    void bKGD() { Data_Common(); }
    void caBX();
    void caNv() { Data_Common(); }
    void cHRM() { Data_Common(); }
    void cICP();
    void cLLI();
    void cLLi() { cLLI(); }
    void eXIf();
    void fcTL() { Data_Common(); }
    void fdAT() { Data_Common(); }
    void gAMA();
    void gdAT();
    void gmAP();
    void hIST() { Data_Common(); }
    void iCCP();
    void iTXt() { Textual(bitset8().set(IsCompressed).set(IsUTF8)); }
    void mDCV();
    void mDCv() { mDCV(); }
    void pHYs();
    void sBIT();
    void sPLT() { Data_Common(); }
    void sRGB() { Data_Common(); }
    void tEXt() { Textual(bitset8()); }
    void tIME();
    void tRNS() { Data_Common(); }
    void vpAg() { Data_Common(); }
    void zTXt() { Textual(bitset8().set(IsCompressed)); }

    //Helpers
    enum Text_Style
    {
        IsCompressed,
        IsUTF8,
    };
    void Textual(bitset8 Method);
    void Decode_RawProfile(const char* in, size_t in_len, const string& type);
    void Data_Common();

    //Temp
    int64u Data_Size;
    int32u Signature;
    std::shared_ptr<void> GainMap_metadata_ISO;
};

} //NameSpace

#endif
