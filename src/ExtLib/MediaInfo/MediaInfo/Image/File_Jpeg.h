/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about JPEG files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_JpegH
#define MediaInfo_File_JpegH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include <memory>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_Jpeg
//***************************************************************************

class File_Jpeg : public File__Analyze
{
public :
    //In
    stream_t StreamKind;
    bool     Interlaced;
    int8u    MxfContentKind;
    #if MEDIAINFO_DEMUX
    float64  FrameRate;
    #endif //MEDIAINFO_DEMUX

    //Constructor/Destructor
    File_Jpeg();

private :
    struct seek_item {
        string Type[2];
        string MuxingMode[2];
        string Mime;
        int64u Size;
        int64u Padding;
        int64u DependsOnFileOffset = 0;
        size_t DependsOnStreamPos = 0;
        bool IsParsed = false;
    };

    //Streams management
    void Streams_Accept();
    void Streams_Accept_PerImage(const seek_item& Item);
    void Streams_Finish();
    void Streams_Finish_PerImage();

    //Buffer - File header
    bool FileHeader_Begin();

    //Buffer - Synchro
    bool Synchronize();
    bool Synched_Test();
    void Synched_Init();

    //Buffer - Demux
    #if MEDIAINFO_DEMUX
    bool Demux_UnpacketizeContainer_Test();
    #endif //MEDIAINFO_DEMUX

    //Buffer - Global
    void Read_Buffer_Unsynched();
    void Read_Buffer_Continue();
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID) {return Read_Buffer_Seek_OneFramePerFile(Method, Value, ID);}
    #endif //MEDIAINFO_SEEK

    //Buffer - Per element
    void Header_Parse();
    bool Header_Parser_Fill_Size();
    void Data_Parse();

    //Elements
    void TEM () { Data_Common(); };
    void RE30() { Data_Common(); };
    void RE31() { Data_Common(); };
    void RE32() { Data_Common(); };
    void RE33() { Data_Common(); };
    void RE34() { Data_Common(); };
    void RE35() { Data_Common(); };
    void RE36() { Data_Common(); };
    void RE37() { Data_Common(); };
    void RE38() { Data_Common(); };
    void RE39() { Data_Common(); };
    void RE3A() { Data_Common(); };
    void RE3B() { Data_Common(); };
    void RE3C() { Data_Common(); };
    void RE3D() { Data_Common(); };
    void RE3E() { Data_Common(); };
    void RE3F() { Data_Common(); };
    void RE44() {}
    void SOC () { Data_Common(); };
    void CAP ();
    void SIZ ();
    void COD ();
    void COC () { Data_Common(); };
    void TLM () { Data_Common(); };
    void PLM () { Data_Common(); };
    void PLT () { Data_Common(); };
    void CPF () { Data_Common(); };
    void QCD ();
    void QCC () { Data_Common(); };
    void RGN () { Data_Common(); };
    void POC () { Data_Common(); };
    void PPM () { Data_Common(); };
    void PPT () { Data_Common(); };
    void CRG () { Data_Common(); };
    void CME ();
    void SEC () { Data_Common(); };
    void EPB () { Data_Common(); };
    void ESD () { Data_Common(); };
    void EPC () { Data_Common(); };
    void RED () { Data_Common(); };
    void SOT () { Data_Common(); };
    void SOP () { Data_Common(); };
    void EPH () { Data_Common(); };
    void SOD ();
    void ISEC() { Data_Common(); };
    void SOF_();
    void SOF0() {SOF_();};
    void SOF1() {SOF_();};
    void SOF2() {SOF_();};
    void SOF3() {SOF_();}
    void DHT () { Data_Common(); };
    void SOF5() {SOF_();}
    void SOF6() {SOF_();}
    void SOF7() {SOF_();}
    void JPG () { Data_Common(); };
    void SOF9() {SOF_();}
    void SOFA() {SOF_();}
    void SOFB() {SOF_();}
    void DAC () { Data_Common(); };
    void SOFD() {SOF_();}
    void SOFE() {SOF_();}
    void SOFF() {SOF_();}
    void RST0() { Data_Common(); };
    void RST1() { Data_Common(); };
    void RST2() { Data_Common(); };
    void RST3() { Data_Common(); };
    void RST4() { Data_Common(); };
    void RST5() { Data_Common(); };
    void RST6() { Data_Common(); };
    void RST7() { Data_Common(); };
    void SOI () { Data_Common(); };
    void EOI () { Data_Common(); };
    void SOS ();
    void DQT () { Data_Common(); };
    void DNL () { Data_Common(); };
    void DRI () { Data_Common(); };
    void DHP () { Data_Common(); };
    void EXP () { Data_Common(); };
    void APP0();
    void APP0_AVI1();
    void APP0_JFIF();
    void APP0_JFFF();
    void APP0_JFFF_JPEG();
    void APP0_JFFF_1B();
    void APP0_JFFF_3B();
    void APP1();
    void APP1_EXIF();
    void APP1_XMP();
    void APP1_XMP_Extension();
    void APP2();
    void APP2_ICC_PROFILE();
    void APP2_ISO21496_1();
    void APP2_MPF();
    void APP3() {Skip_XX(Element_Size, "Data");}
    void APP4() {Skip_XX(Element_Size, "Data");}
    void APP5() {Skip_XX(Element_Size, "Data");}
    void APP6() {Skip_XX(Element_Size, "Data");}
    void APP7() {Skip_XX(Element_Size, "Data");}
    void APP8() {Skip_XX(Element_Size, "Data");}
    void APP9() {Skip_XX(Element_Size, "Data");}
    void APPA() {Skip_XX(Element_Size, "Data");}
    void APPB();
    void APPB_JPEGXT();
    void APPB_JPEGXT_JUMB(int16u Instance, int32u SequenceNumber);
    void APPC() {Skip_XX(Element_Size, "Data");}
    void APPD();
    void APPE();
    void APPE_Adobe0();
    void APPF() {Skip_XX(Element_Size, "Data");}
    void JPG0() {Skip_XX(Element_Size, "Data");}
    void JPG1() {Skip_XX(Element_Size, "Data");}
    void JPG2() {Skip_XX(Element_Size, "Data");}
    void JPG3() {Skip_XX(Element_Size, "Data");}
    void JPG4() {Skip_XX(Element_Size, "Data");}
    void JPG5() {Skip_XX(Element_Size, "Data");}
    void JPG6() {Skip_XX(Element_Size, "Data");}
    void JPG7() {Skip_XX(Element_Size, "Data");}
    void JPG8() {Skip_XX(Element_Size, "Data");}
    void JPG9() {Skip_XX(Element_Size, "Data");}
    void JPGA() {Skip_XX(Element_Size, "Data");}
    void JPGB() {Skip_XX(Element_Size, "Data");}
    void JPGC() {Skip_XX(Element_Size, "Data");}
    void JPGD() {Skip_XX(Element_Size, "Data");}
    void COM();
    
    //Helpers
    void Data_Common();

    //Temp
    int64u Data_Size = 0;
    int8u APPE_Adobe0_transform = 0;
    bool  APP0_JFIF_Parsed = false;
    bool  SOS_SOD_Parsed = false;
    bool  CME_Text_Parsed = false;
    int64u GContainerItems_Offset = 0;
    size_t Seek_Items_PrimaryStreamPos = 0;
    string Seek_Items_PrimaryImageType;
    std::map<int64u, seek_item> Seek_Items;
    std::map<int64u, seek_item> Seek_Items_WithoutFirstImageOffset;
    std::shared_ptr<void> GainMap_metadata_Adobe;
    std::shared_ptr<void> GainMap_metadata_ISO;
    std::unique_ptr<File__Analyze> Exif_Parser;
    std::unique_ptr<File__Analyze> PSD_Parser;
    std::unique_ptr<File__Analyze> ICC_Parser;
    struct xmpext
    {
        xmpext(File__Analyze* NewParser)
            : Parser(NewParser)
        {
        }

        std::unique_ptr<File__Analyze> Parser;
        uintptr_t GContainerItems = {};
        int32u LastOffset = 0;
    };
    std::map<std::string, xmpext> XmpExt_List;
    struct jpegxtext
    {
        jpegxtext(File__Analyze* NewParser)
            : Parser(NewParser)
        {
        }

        std::unique_ptr<File__Analyze> Parser;
        int32u LastSequenceNumber = 0;
    };
    std::map<int16u, jpegxtext> JpegXtExt_List;
};

} //NameSpace

#endif
