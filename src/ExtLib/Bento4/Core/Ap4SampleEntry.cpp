/*****************************************************************
|
|    AP4 - sample entries
|
|    Copyright 2002 Gilles Boccon-Gibod & Julien Boeuf
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4SampleEntry.h"
#include "Ap4Utils.h"
#include "Ap4AtomFactory.h"
#include "Ap4TimsAtom.h"
#include "Ap4SampleDescription.h"
#include "Ap4FtabAtom.h"
#include "Ap4EndaAtom.h"

/*----------------------------------------------------------------------
|       AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type format,
                                 AP4_UI16       data_reference_index) :
    AP4_ContainerAtom(format, AP4_ATOM_HEADER_SIZE+8, false),
    m_DataReferenceIndex(data_reference_index)
{
    m_Reserved1[0] = 0;
    m_Reserved1[1] = 0;
    m_Reserved1[2] = 0;
    m_Reserved1[3] = 0;
    m_Reserved1[4] = 0;
    m_Reserved1[5] = 0;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type format,
                                 AP4_Size       size) :
    AP4_ContainerAtom(format, size)
{
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::AP4_SampleEntry
+---------------------------------------------------------------------*/
AP4_SampleEntry::AP4_SampleEntry(AP4_Atom::Type   format,
                                 AP4_Size         size,
                                 AP4_ByteStream&  stream,
                                 AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(format, size)
{
    // read the fields before the children atoms
    AP4_Size fields_size = GetFieldsSize();
    ReadFields(stream);

    // read children atoms (ex: esds and maybe others)
    ReadChildren(atom_factory, stream, size-AP4_ATOM_HEADER_SIZE-fields_size);
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_SampleEntry::GetFieldsSize()
{
    return 8;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::ReadFields(AP4_ByteStream& stream)
{
    stream.Read(m_Reserved1, sizeof(m_Reserved1), NULL);
    stream.ReadUI16(m_DataReferenceIndex);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // reserved1
    result = stream.Write(m_Reserved1, sizeof(m_Reserved1));
    if (AP4_FAILED(result)) return result;

    // data reference index
    result = stream.WriteUI16(m_DataReferenceIndex);
    if (AP4_FAILED(result)) return result;
    
    return result;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::Write
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::Write(AP4_ByteStream& stream)
{
    AP4_Result result;

    // write the header
    result = WriteHeader(stream);
    if (AP4_FAILED(result)) return result;

    // write the fields
    result = WriteFields(stream);
    if (AP4_FAILED(result)) return result;

    // write the children atoms
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("data_reference_index", m_DataReferenceIndex);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::Inspect
+---------------------------------------------------------------------*/
AP4_Result
AP4_SampleEntry::Inspect(AP4_AtomInspector& inspector)
{
    // inspect the header
    InspectHeader(inspector);

    // inspect the fields
    InspectFields(inspector);

    // inspect children
    m_Children.Apply(AP4_AtomListInspector(inspector));

    // finish
    inspector.EndElement();

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::OnChildChanged
+---------------------------------------------------------------------*/
void
AP4_SampleEntry::OnChildChanged(AP4_Atom*)
{
    // remcompute our size
    m_Size = GetHeaderSize()+GetFieldsSize();
    m_Children.Apply(AP4_AtomSizeAdder(m_Size));

    // update our parent
    if (m_Parent) m_Parent->OnChildChanged(this);
}

/*----------------------------------------------------------------------
|       AP4_SampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_SampleEntry::ToSampleDescription()
{
    return new AP4_UnknownSampleDescription(this);
}

/*----------------------------------------------------------------------
|       AP4_MpegSampleEntry::AP4_MpegSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegSampleEntry::AP4_MpegSampleEntry(AP4_Atom::Type    format, 
                                         AP4_EsDescriptor* descriptor) :
    AP4_SampleEntry(format)
{
    if (descriptor) AddChild(new AP4_EsdsAtom(descriptor));
}

/*----------------------------------------------------------------------
|       AP4_MpegSampleEntry::AP4_MpegSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegSampleEntry::AP4_MpegSampleEntry(AP4_Atom::Type format,
                                         AP4_Size       size) :
    AP4_SampleEntry(format, size)
{
}

/*----------------------------------------------------------------------
|       AP4_MpegSampleEntry::AP4_MpegSampleEntry
+---------------------------------------------------------------------*/
AP4_MpegSampleEntry::AP4_MpegSampleEntry(AP4_Atom::Type   format,
                                         AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_SampleEntry(format, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|       AP4_MpegSampleEntry::AP4_MpegSampleEntry
+---------------------------------------------------------------------*/
const AP4_DecoderConfigDescriptor* 
AP4_MpegSampleEntry::GetDecoderConfigDescriptor()
{
    AP4_Atom* child = GetChild(AP4_ATOM_TYPE_ESDS);

    if(!child) {
        child = GetChild(AP4_ATOM_TYPE_WAVE);
        if (AP4_ContainerAtom* wave = dynamic_cast<AP4_ContainerAtom*>(child)) {
            child = wave->GetChild(AP4_ATOM_TYPE_ESDS);
        }
    }

    if (child) {
        AP4_EsdsAtom* esds = (AP4_EsdsAtom*)child;

        // get the es descriptor
        const AP4_EsDescriptor* es_desc = esds->GetEsDescriptor();
        if (es_desc == NULL) return NULL;

        // get the decoder config descriptor
        return es_desc->GetDecoderConfigDescriptor();
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|       AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry(AP4_EsDescriptor* descriptor) :
    AP4_MpegSampleEntry(AP4_ATOM_TYPE_MP4S, descriptor)
{
}

/*----------------------------------------------------------------------
|       AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4sSampleEntry::AP4_Mp4sSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_MpegSampleEntry(AP4_ATOM_TYPE_MP4S, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|       AP4_Mp4sSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_Mp4sSampleEntry::ToSampleDescription()
{
    // get the decoder config descriptor
    const AP4_DecoderConfigDescriptor* dc_desc;
    dc_desc = GetDecoderConfigDescriptor();
    if (dc_desc == NULL) return NULL;
    const AP4_DataBuffer* dsi = NULL;
    const AP4_DecoderSpecificInfoDescriptor* dsi_desc =
        dc_desc->GetDecoderSpecificInfoDescriptor();
    if (dsi_desc != NULL) {
        dsi = &dsi_desc->GetDecoderSpecificInfo();
    }

    // create a sample description
    return new AP4_MpegSystemSampleDescription(
        dc_desc->GetStreamType(),
        dc_desc->GetObjectTypeIndication(),
        dsi,
        dc_desc->GetBufferSize(),
        dc_desc->GetMaxBitrate(),
        dc_desc->GetAvgBitrate());
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::AP4_AudioSampleEntry
+---------------------------------------------------------------------*/
AP4_AudioSampleEntry::AP4_AudioSampleEntry(AP4_Atom::Type    format,
                                           AP4_EsDescriptor* descriptor,
                                           AP4_UI32          sample_rate,
                                           AP4_UI16          sample_size,
                                           AP4_UI16          channel_count) :
    AP4_MpegSampleEntry(format, descriptor),
    m_QtVersion(0),
    m_QtRevision(0),
    m_QtVendor(0),
    m_ChannelCount(channel_count),
    m_SampleSize(sample_size),
    m_QtCompressionId(0),
    m_QtPacketSize(0),
    m_SampleRate(sample_rate),
    m_SampleRateExtra(0),
    m_QtV1SamplesPerPacket(0),
    m_QtV1BytesPerPacket(0),
    m_QtV1BytesPerFrame(0),
    m_QtV1BytesPerSample(0),
    m_QtV2StructSize(0),
    m_QtV2SampleRate64(0.0),
    m_QtV2ChannelCount(0),
    m_QtV2Reserved(0),
    m_QtV2BitsPerChannel(0),
    m_QtV2FormatSpecificFlags(0),
    m_QtV2BytesPerAudioPacket(0),
    m_QtV2LPCMFramesPerAudioPacket(0),
// MPC-BE custom code start
    m_Endian(ENDIAN_NOTSET)
// MPC-BE custom code end
{
    m_Size += 20;
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::AP4_AudioSampleEntry
+---------------------------------------------------------------------*/
AP4_AudioSampleEntry::AP4_AudioSampleEntry(AP4_Atom::Type   format,
                                           AP4_Size         size,
                                           AP4_ByteStream&  stream,
                                           AP4_AtomFactory& atom_factory) :
    AP4_MpegSampleEntry(format, size)
{
    // read fields
    ReadFields(stream);

    // hack to get the correct WAVEFORMATEX in MP4Splitter.cpp
    // (need more information about audio formats used in older movs (QuickTime 2.x))
    if (m_QtVersion == 0 ||                            // fill QtV1 values for the old movs
        m_QtVersion == 1 && m_QtV1BytesPerPacket == 0) // fixing empty QtV1 values for some (broken?) movs.
    {
        switch( format )
        {
        case AP4_ATOM_TYPE_ALAW:
        case AP4_ATOM_TYPE_ULAW:
            m_SampleSize = 8; // sometimes this value is incorrect
            m_QtV1SamplesPerPacket = 1;
            m_QtV1BytesPerPacket   = 1;
            break;
        case AP4_ATOM_TYPE_IMA4:
            m_QtV1SamplesPerPacket = 64;
            m_QtV1BytesPerPacket   = 34;
            break;
        case AP4_ATOM_TYPE_MAC3:
            m_QtV1SamplesPerPacket = 3;
            m_QtV1BytesPerPacket   = 1;
            break;
        case AP4_ATOM_TYPE_MAC6:
            m_QtV1SamplesPerPacket = 6;
            m_QtV1BytesPerPacket   = 1;
            break;
        case AP4_ATOM_TYPE_SAMR:
            m_ChannelCount = 1; // sometimes this value is incorrect
            m_SampleRate   = 8000<<16; // sometimes this value is zero
            //m_QtV1SamplesPerPacket = 160; // mean value, correct it if you know how.
            //m_QtV1BytesPerPacket   = 20; // mean value, correct it if you know how.
            // disabled, because the wrong values can break playback.
            break;
        default: // NONE, RAW, TWOS, SOWT and other
            m_QtV1SamplesPerPacket = 1;
            m_QtV1BytesPerPacket   = m_SampleSize / 8;
        }
        m_QtV1BytesPerFrame    = m_ChannelCount * m_QtV1BytesPerPacket;
        m_QtV1BytesPerSample   = m_SampleSize / 8;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // must be called after m_QtVersion was already set
    AP4_Size fields_size = GetFieldsSize();

    // read children atoms (ex: esds and maybe others)
    ReadChildren(atom_factory, stream, size-AP4_ATOM_HEADER_SIZE-fields_size);

    // read AP4_ATOM_TYPE_ENDA
    m_Endian = ENDIAN_NOTSET;
    if(AP4_Atom* child = GetChild(AP4_ATOM_TYPE_WAVE))
        if(AP4_ContainerAtom* wave = dynamic_cast<AP4_ContainerAtom*>(child))
            if (AP4_EndaAtom* endian = dynamic_cast<AP4_EndaAtom*>(wave->GetChild(AP4_ATOM_TYPE_ENDA)))
                m_Endian = endian->ReadEndian();
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::AP4_AudioSampleEntry
+---------------------------------------------------------------------*/
AP4_AudioSampleEntry::AP4_AudioSampleEntry(AP4_Atom::Type   format,
                                           AP4_Size         size) :
    AP4_MpegSampleEntry(format, size)
{
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_AudioSampleEntry::GetFieldsSize()
{
    AP4_Size size = AP4_SampleEntry::GetFieldsSize()+20;
    if (m_QtVersion == 1) {
        size += 16;
    } else if (m_QtVersion == 2) {
        size += 36+m_QtV2Extension.GetDataSize();
    }
    
    return size;
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetSampleRate
+---------------------------------------------------------------------*/
AP4_UI32
AP4_AudioSampleEntry::GetSampleRate()
{
    if (m_QtVersion == 2) {
        return (AP4_UI32)(m_QtV2SampleRate64);
    } else {
        return m_SampleRateExtra ? m_SampleRateExtra : m_SampleRate>>16;
    }
}

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::GetChannelCount
+---------------------------------------------------------------------*/
AP4_UI16
AP4_AudioSampleEntry::GetChannelCount()
{
    if (m_QtVersion == 2) {
        return m_QtV2ChannelCount;
    } else {
        return m_ChannelCount;
    }
}  

/*----------------------------------------------------------------------
|   AP4_AudioSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < AP4_SUCCESS) return result;

    // read the fields of this class
    stream.ReadUI16(m_QtVersion);
    stream.ReadUI16(m_QtRevision);
    stream.ReadUI32(m_QtVendor);
    stream.ReadUI16(m_ChannelCount);
    stream.ReadUI16(m_SampleSize);
    stream.ReadUI16(m_QtCompressionId);
    stream.ReadUI16(m_QtPacketSize);
    stream.ReadUI32(m_SampleRate);

    // MPC-BE custom code start
    if (!(m_SampleRate >> 16)) {
        m_SampleRate <<= 16;
    }
    // MPC-BE custom code end

    // if this is a QT V1 entry, read the extension
    if (m_QtVersion == 1) {
        stream.ReadUI32(m_QtV1SamplesPerPacket);
        stream.ReadUI32(m_QtV1BytesPerPacket);
        stream.ReadUI32(m_QtV1BytesPerFrame);
        stream.ReadUI32(m_QtV1BytesPerSample);
    } else if (m_QtVersion == 2) {
        stream.ReadUI32(m_QtV2StructSize);
        stream.ReadDouble(m_QtV2SampleRate64);
        stream.ReadUI32(m_QtV2ChannelCount);
        stream.ReadUI32(m_QtV2Reserved);
        stream.ReadUI32(m_QtV2BitsPerChannel);
        stream.ReadUI32(m_QtV2FormatSpecificFlags);
        stream.ReadUI32(m_QtV2BytesPerAudioPacket);
        stream.ReadUI32(m_QtV2LPCMFramesPerAudioPacket);
        if (m_QtV2StructSize > 72) {
            unsigned int ext_size = m_QtV2StructSize-72;
            m_QtV2Extension.SetDataSize(ext_size);
            stream.Read(m_QtV2Extension.UseData(), ext_size);
        }
        m_QtV1SamplesPerPacket =
        m_QtV1BytesPerPacket   =
        m_QtV1BytesPerFrame    =
        m_QtV1BytesPerSample   = 0;
    } else {
        m_QtV1SamplesPerPacket         = 0;
        m_QtV1BytesPerPacket           = 0;
        m_QtV1BytesPerFrame            = 0;
        m_QtV1BytesPerSample           = 0;
        m_QtV2StructSize               = 0;
        m_QtV2SampleRate64             = 0.0;
        m_QtV2ChannelCount             = 0;
        m_QtV2Reserved                 = 0;
        m_QtV2BitsPerChannel           = 0;
        m_QtV2FormatSpecificFlags      = 0;
        m_QtV2BytesPerAudioPacket      = 0;
        m_QtV2LPCMFramesPerAudioPacket = 0;
    }

    m_SampleRateExtra = 0;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);

    // 
    result = stream.WriteUI16(m_QtVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_QtRevision);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_QtVendor);
    if (AP4_FAILED(result)) return result;

    // channel count
    result = stream.WriteUI16(m_ChannelCount);
    if (AP4_FAILED(result)) return result;
    
    // sample size 
    result = stream.WriteUI16(m_SampleSize);
    if (AP4_FAILED(result)) return result;

    // predefined1
    result = stream.WriteUI16(m_QtCompressionId);
    if (AP4_FAILED(result)) return result;

    // reserved3
    result = stream.WriteUI16(m_QtPacketSize);
    if (AP4_FAILED(result)) return result;

    // sample rate
    result = stream.WriteUI32(m_SampleRate);
    if (AP4_FAILED(result)) return result;

    if(m_QtVersion == 1)
    {
        result = stream.WriteUI32(m_QtV1SamplesPerPacket);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerPacket);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerFrame);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_QtV1BytesPerSample);
        if (AP4_FAILED(result)) return result;
    }
    else if(m_QtVersion != 0)
    {
        return AP4_FAILURE;
    }

    return result;
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AudioSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // dump the fields from the base class
    AP4_SampleEntry::InspectFields(inspector);

    // fields
    inspector.AddField("channel_count", m_ChannelCount);
    inspector.AddField("sample_size", m_SampleSize);
    inspector.AddField("sample_rate", m_SampleRate>>16);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_AudioSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_AudioSampleEntry::ToSampleDescription()
{
    // get the decoder config descriptor
    const AP4_DecoderConfigDescriptor* dc_desc;
    dc_desc = GetDecoderConfigDescriptor();
    if (dc_desc == NULL) return NULL;
    const AP4_DataBuffer* dsi = NULL;
    const AP4_DecoderSpecificInfoDescriptor* dsi_desc =
        dc_desc->GetDecoderSpecificInfoDescriptor();
    if (dsi_desc != NULL) {
        dsi = &dsi_desc->GetDecoderSpecificInfo();
    }

    // create a sample description
    return new AP4_MpegAudioSampleDescription(
        dc_desc->GetObjectTypeIndication(),
        GetSampleRate(),
        GetSampleSize(),
        GetChannelCount(),
        dsi,
        dc_desc->GetBufferSize(),
        dc_desc->GetMaxBitrate(),
        dc_desc->GetAvgBitrate());
}

/*----------------------------------------------------------------------
|       AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry(AP4_UI32          sample_rate, 
                                         AP4_UI16          sample_size,
                                         AP4_UI16          channel_count,
                                         AP4_EsDescriptor* descriptor) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE_MP4A, 
                         descriptor,
                         sample_rate, 
                         sample_size, 
                         channel_count)
{
}

/*----------------------------------------------------------------------
|       AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4aSampleEntry::AP4_Mp4aSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE_MP4A, size, stream, atom_factory)
{
}

static const AP4_Byte qt_default_palette_4[4 * 3] = {
  0x93, 0x65, 0x5E,
  0xFF, 0xFF, 0xFF,
  0xDF, 0xD0, 0xAB,
  0x00, 0x00, 0x00
};

static const AP4_Byte qt_default_palette_16[16 * 3] = {
  0xFF, 0xFB, 0xFF,
  0xEF, 0xD9, 0xBB,
  0xE8, 0xC9, 0xB1,
  0x93, 0x65, 0x5E,
  0xFC, 0xDE, 0xE8,
  0x9D, 0x88, 0x91,
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF,
  0x47, 0x48, 0x37,
  0x7A, 0x5E, 0x55,
  0xDF, 0xD0, 0xAB,
  0xFF, 0xFB, 0xF9,
  0xE8, 0xCA, 0xC5,
  0x8A, 0x7C, 0x77,
  0x00, 0x00, 0x00
};

static const AP4_Byte qt_default_palette_256[256 * 3] = {
  /*   0, 0x00 */  0xFF, 0xFF, 0xFF,
  /*   1, 0x01 */  0xFF, 0xFF, 0xCC,
  /*   2, 0x02 */  0xFF, 0xFF, 0x99,
  /*   3, 0x03 */  0xFF, 0xFF, 0x66,
  /*   4, 0x04 */  0xFF, 0xFF, 0x33,
  /*   5, 0x05 */  0xFF, 0xFF, 0x00,
  /*   6, 0x06 */  0xFF, 0xCC, 0xFF,
  /*   7, 0x07 */  0xFF, 0xCC, 0xCC,
  /*   8, 0x08 */  0xFF, 0xCC, 0x99,
  /*   9, 0x09 */  0xFF, 0xCC, 0x66,
  /*  10, 0x0A */  0xFF, 0xCC, 0x33,
  /*  11, 0x0B */  0xFF, 0xCC, 0x00,
  /*  12, 0x0C */  0xFF, 0x99, 0xFF,
  /*  13, 0x0D */  0xFF, 0x99, 0xCC,
  /*  14, 0x0E */  0xFF, 0x99, 0x99,
  /*  15, 0x0F */  0xFF, 0x99, 0x66,
  /*  16, 0x10 */  0xFF, 0x99, 0x33,
  /*  17, 0x11 */  0xFF, 0x99, 0x00,
  /*  18, 0x12 */  0xFF, 0x66, 0xFF,
  /*  19, 0x13 */  0xFF, 0x66, 0xCC,
  /*  20, 0x14 */  0xFF, 0x66, 0x99,
  /*  21, 0x15 */  0xFF, 0x66, 0x66,
  /*  22, 0x16 */  0xFF, 0x66, 0x33,
  /*  23, 0x17 */  0xFF, 0x66, 0x00,
  /*  24, 0x18 */  0xFF, 0x33, 0xFF,
  /*  25, 0x19 */  0xFF, 0x33, 0xCC,
  /*  26, 0x1A */  0xFF, 0x33, 0x99,
  /*  27, 0x1B */  0xFF, 0x33, 0x66,
  /*  28, 0x1C */  0xFF, 0x33, 0x33,
  /*  29, 0x1D */  0xFF, 0x33, 0x00,
  /*  30, 0x1E */  0xFF, 0x00, 0xFF,
  /*  31, 0x1F */  0xFF, 0x00, 0xCC,
  /*  32, 0x20 */  0xFF, 0x00, 0x99,
  /*  33, 0x21 */  0xFF, 0x00, 0x66,
  /*  34, 0x22 */  0xFF, 0x00, 0x33,
  /*  35, 0x23 */  0xFF, 0x00, 0x00,
  /*  36, 0x24 */  0xCC, 0xFF, 0xFF,
  /*  37, 0x25 */  0xCC, 0xFF, 0xCC,
  /*  38, 0x26 */  0xCC, 0xFF, 0x99,
  /*  39, 0x27 */  0xCC, 0xFF, 0x66,
  /*  40, 0x28 */  0xCC, 0xFF, 0x33,
  /*  41, 0x29 */  0xCC, 0xFF, 0x00,
  /*  42, 0x2A */  0xCC, 0xCC, 0xFF,
  /*  43, 0x2B */  0xCC, 0xCC, 0xCC,
  /*  44, 0x2C */  0xCC, 0xCC, 0x99,
  /*  45, 0x2D */  0xCC, 0xCC, 0x66,
  /*  46, 0x2E */  0xCC, 0xCC, 0x33,
  /*  47, 0x2F */  0xCC, 0xCC, 0x00,
  /*  48, 0x30 */  0xCC, 0x99, 0xFF,
  /*  49, 0x31 */  0xCC, 0x99, 0xCC,
  /*  50, 0x32 */  0xCC, 0x99, 0x99,
  /*  51, 0x33 */  0xCC, 0x99, 0x66,
  /*  52, 0x34 */  0xCC, 0x99, 0x33,
  /*  53, 0x35 */  0xCC, 0x99, 0x00,
  /*  54, 0x36 */  0xCC, 0x66, 0xFF,
  /*  55, 0x37 */  0xCC, 0x66, 0xCC,
  /*  56, 0x38 */  0xCC, 0x66, 0x99,
  /*  57, 0x39 */  0xCC, 0x66, 0x66,
  /*  58, 0x3A */  0xCC, 0x66, 0x33,
  /*  59, 0x3B */  0xCC, 0x66, 0x00,
  /*  60, 0x3C */  0xCC, 0x33, 0xFF,
  /*  61, 0x3D */  0xCC, 0x33, 0xCC,
  /*  62, 0x3E */  0xCC, 0x33, 0x99,
  /*  63, 0x3F */  0xCC, 0x33, 0x66,
  /*  64, 0x40 */  0xCC, 0x33, 0x33,
  /*  65, 0x41 */  0xCC, 0x33, 0x00,
  /*  66, 0x42 */  0xCC, 0x00, 0xFF,
  /*  67, 0x43 */  0xCC, 0x00, 0xCC,
  /*  68, 0x44 */  0xCC, 0x00, 0x99,
  /*  69, 0x45 */  0xCC, 0x00, 0x66,
  /*  70, 0x46 */  0xCC, 0x00, 0x33,
  /*  71, 0x47 */  0xCC, 0x00, 0x00,
  /*  72, 0x48 */  0x99, 0xFF, 0xFF,
  /*  73, 0x49 */  0x99, 0xFF, 0xCC,
  /*  74, 0x4A */  0x99, 0xFF, 0x99,
  /*  75, 0x4B */  0x99, 0xFF, 0x66,
  /*  76, 0x4C */  0x99, 0xFF, 0x33,
  /*  77, 0x4D */  0x99, 0xFF, 0x00,
  /*  78, 0x4E */  0x99, 0xCC, 0xFF,
  /*  79, 0x4F */  0x99, 0xCC, 0xCC,
  /*  80, 0x50 */  0x99, 0xCC, 0x99,
  /*  81, 0x51 */  0x99, 0xCC, 0x66,
  /*  82, 0x52 */  0x99, 0xCC, 0x33,
  /*  83, 0x53 */  0x99, 0xCC, 0x00,
  /*  84, 0x54 */  0x99, 0x99, 0xFF,
  /*  85, 0x55 */  0x99, 0x99, 0xCC,
  /*  86, 0x56 */  0x99, 0x99, 0x99,
  /*  87, 0x57 */  0x99, 0x99, 0x66,
  /*  88, 0x58 */  0x99, 0x99, 0x33,
  /*  89, 0x59 */  0x99, 0x99, 0x00,
  /*  90, 0x5A */  0x99, 0x66, 0xFF,
  /*  91, 0x5B */  0x99, 0x66, 0xCC,
  /*  92, 0x5C */  0x99, 0x66, 0x99,
  /*  93, 0x5D */  0x99, 0x66, 0x66,
  /*  94, 0x5E */  0x99, 0x66, 0x33,
  /*  95, 0x5F */  0x99, 0x66, 0x00,
  /*  96, 0x60 */  0x99, 0x33, 0xFF,
  /*  97, 0x61 */  0x99, 0x33, 0xCC,
  /*  98, 0x62 */  0x99, 0x33, 0x99,
  /*  99, 0x63 */  0x99, 0x33, 0x66,
  /* 100, 0x64 */  0x99, 0x33, 0x33,
  /* 101, 0x65 */  0x99, 0x33, 0x00,
  /* 102, 0x66 */  0x99, 0x00, 0xFF,
  /* 103, 0x67 */  0x99, 0x00, 0xCC,
  /* 104, 0x68 */  0x99, 0x00, 0x99,
  /* 105, 0x69 */  0x99, 0x00, 0x66,
  /* 106, 0x6A */  0x99, 0x00, 0x33,
  /* 107, 0x6B */  0x99, 0x00, 0x00,
  /* 108, 0x6C */  0x66, 0xFF, 0xFF,
  /* 109, 0x6D */  0x66, 0xFF, 0xCC,
  /* 110, 0x6E */  0x66, 0xFF, 0x99,
  /* 111, 0x6F */  0x66, 0xFF, 0x66,
  /* 112, 0x70 */  0x66, 0xFF, 0x33,
  /* 113, 0x71 */  0x66, 0xFF, 0x00,
  /* 114, 0x72 */  0x66, 0xCC, 0xFF,
  /* 115, 0x73 */  0x66, 0xCC, 0xCC,
  /* 116, 0x74 */  0x66, 0xCC, 0x99,
  /* 117, 0x75 */  0x66, 0xCC, 0x66,
  /* 118, 0x76 */  0x66, 0xCC, 0x33,
  /* 119, 0x77 */  0x66, 0xCC, 0x00,
  /* 120, 0x78 */  0x66, 0x99, 0xFF,
  /* 121, 0x79 */  0x66, 0x99, 0xCC,
  /* 122, 0x7A */  0x66, 0x99, 0x99,
  /* 123, 0x7B */  0x66, 0x99, 0x66,
  /* 124, 0x7C */  0x66, 0x99, 0x33,
  /* 125, 0x7D */  0x66, 0x99, 0x00,
  /* 126, 0x7E */  0x66, 0x66, 0xFF,
  /* 127, 0x7F */  0x66, 0x66, 0xCC,
  /* 128, 0x80 */  0x66, 0x66, 0x99,
  /* 129, 0x81 */  0x66, 0x66, 0x66,
  /* 130, 0x82 */  0x66, 0x66, 0x33,
  /* 131, 0x83 */  0x66, 0x66, 0x00,
  /* 132, 0x84 */  0x66, 0x33, 0xFF,
  /* 133, 0x85 */  0x66, 0x33, 0xCC,
  /* 134, 0x86 */  0x66, 0x33, 0x99,
  /* 135, 0x87 */  0x66, 0x33, 0x66,
  /* 136, 0x88 */  0x66, 0x33, 0x33,
  /* 137, 0x89 */  0x66, 0x33, 0x00,
  /* 138, 0x8A */  0x66, 0x00, 0xFF,
  /* 139, 0x8B */  0x66, 0x00, 0xCC,
  /* 140, 0x8C */  0x66, 0x00, 0x99,
  /* 141, 0x8D */  0x66, 0x00, 0x66,
  /* 142, 0x8E */  0x66, 0x00, 0x33,
  /* 143, 0x8F */  0x66, 0x00, 0x00,
  /* 144, 0x90 */  0x33, 0xFF, 0xFF,
  /* 145, 0x91 */  0x33, 0xFF, 0xCC,
  /* 146, 0x92 */  0x33, 0xFF, 0x99,
  /* 147, 0x93 */  0x33, 0xFF, 0x66,
  /* 148, 0x94 */  0x33, 0xFF, 0x33,
  /* 149, 0x95 */  0x33, 0xFF, 0x00,
  /* 150, 0x96 */  0x33, 0xCC, 0xFF,
  /* 151, 0x97 */  0x33, 0xCC, 0xCC,
  /* 152, 0x98 */  0x33, 0xCC, 0x99,
  /* 153, 0x99 */  0x33, 0xCC, 0x66,
  /* 154, 0x9A */  0x33, 0xCC, 0x33,
  /* 155, 0x9B */  0x33, 0xCC, 0x00,
  /* 156, 0x9C */  0x33, 0x99, 0xFF,
  /* 157, 0x9D */  0x33, 0x99, 0xCC,
  /* 158, 0x9E */  0x33, 0x99, 0x99,
  /* 159, 0x9F */  0x33, 0x99, 0x66,
  /* 160, 0xA0 */  0x33, 0x99, 0x33,
  /* 161, 0xA1 */  0x33, 0x99, 0x00,
  /* 162, 0xA2 */  0x33, 0x66, 0xFF,
  /* 163, 0xA3 */  0x33, 0x66, 0xCC,
  /* 164, 0xA4 */  0x33, 0x66, 0x99,
  /* 165, 0xA5 */  0x33, 0x66, 0x66,
  /* 166, 0xA6 */  0x33, 0x66, 0x33,
  /* 167, 0xA7 */  0x33, 0x66, 0x00,
  /* 168, 0xA8 */  0x33, 0x33, 0xFF,
  /* 169, 0xA9 */  0x33, 0x33, 0xCC,
  /* 170, 0xAA */  0x33, 0x33, 0x99,
  /* 171, 0xAB */  0x33, 0x33, 0x66,
  /* 172, 0xAC */  0x33, 0x33, 0x33,
  /* 173, 0xAD */  0x33, 0x33, 0x00,
  /* 174, 0xAE */  0x33, 0x00, 0xFF,
  /* 175, 0xAF */  0x33, 0x00, 0xCC,
  /* 176, 0xB0 */  0x33, 0x00, 0x99,
  /* 177, 0xB1 */  0x33, 0x00, 0x66,
  /* 178, 0xB2 */  0x33, 0x00, 0x33,
  /* 179, 0xB3 */  0x33, 0x00, 0x00,
  /* 180, 0xB4 */  0x00, 0xFF, 0xFF,
  /* 181, 0xB5 */  0x00, 0xFF, 0xCC,
  /* 182, 0xB6 */  0x00, 0xFF, 0x99,
  /* 183, 0xB7 */  0x00, 0xFF, 0x66,
  /* 184, 0xB8 */  0x00, 0xFF, 0x33,
  /* 185, 0xB9 */  0x00, 0xFF, 0x00,
  /* 186, 0xBA */  0x00, 0xCC, 0xFF,
  /* 187, 0xBB */  0x00, 0xCC, 0xCC,
  /* 188, 0xBC */  0x00, 0xCC, 0x99,
  /* 189, 0xBD */  0x00, 0xCC, 0x66,
  /* 190, 0xBE */  0x00, 0xCC, 0x33,
  /* 191, 0xBF */  0x00, 0xCC, 0x00,
  /* 192, 0xC0 */  0x00, 0x99, 0xFF,
  /* 193, 0xC1 */  0x00, 0x99, 0xCC,
  /* 194, 0xC2 */  0x00, 0x99, 0x99,
  /* 195, 0xC3 */  0x00, 0x99, 0x66,
  /* 196, 0xC4 */  0x00, 0x99, 0x33,
  /* 197, 0xC5 */  0x00, 0x99, 0x00,
  /* 198, 0xC6 */  0x00, 0x66, 0xFF,
  /* 199, 0xC7 */  0x00, 0x66, 0xCC,
  /* 200, 0xC8 */  0x00, 0x66, 0x99,
  /* 201, 0xC9 */  0x00, 0x66, 0x66,
  /* 202, 0xCA */  0x00, 0x66, 0x33,
  /* 203, 0xCB */  0x00, 0x66, 0x00,
  /* 204, 0xCC */  0x00, 0x33, 0xFF,
  /* 205, 0xCD */  0x00, 0x33, 0xCC,
  /* 206, 0xCE */  0x00, 0x33, 0x99,
  /* 207, 0xCF */  0x00, 0x33, 0x66,
  /* 208, 0xD0 */  0x00, 0x33, 0x33,
  /* 209, 0xD1 */  0x00, 0x33, 0x00,
  /* 210, 0xD2 */  0x00, 0x00, 0xFF,
  /* 211, 0xD3 */  0x00, 0x00, 0xCC,
  /* 212, 0xD4 */  0x00, 0x00, 0x99,
  /* 213, 0xD5 */  0x00, 0x00, 0x66,
  /* 214, 0xD6 */  0x00, 0x00, 0x33,
  /* 215, 0xD7 */  0xEE, 0x00, 0x00,
  /* 216, 0xD8 */  0xDD, 0x00, 0x00,
  /* 217, 0xD9 */  0xBB, 0x00, 0x00,
  /* 218, 0xDA */  0xAA, 0x00, 0x00,
  /* 219, 0xDB */  0x88, 0x00, 0x00,
  /* 220, 0xDC */  0x77, 0x00, 0x00,
  /* 221, 0xDD */  0x55, 0x00, 0x00,
  /* 222, 0xDE */  0x44, 0x00, 0x00,
  /* 223, 0xDF */  0x22, 0x00, 0x00,
  /* 224, 0xE0 */  0x11, 0x00, 0x00,
  /* 225, 0xE1 */  0x00, 0xEE, 0x00,
  /* 226, 0xE2 */  0x00, 0xDD, 0x00,
  /* 227, 0xE3 */  0x00, 0xBB, 0x00,
  /* 228, 0xE4 */  0x00, 0xAA, 0x00,
  /* 229, 0xE5 */  0x00, 0x88, 0x00,
  /* 230, 0xE6 */  0x00, 0x77, 0x00,
  /* 231, 0xE7 */  0x00, 0x55, 0x00,
  /* 232, 0xE8 */  0x00, 0x44, 0x00,
  /* 233, 0xE9 */  0x00, 0x22, 0x00,
  /* 234, 0xEA */  0x00, 0x11, 0x00,
  /* 235, 0xEB */  0x00, 0x00, 0xEE,
  /* 236, 0xEC */  0x00, 0x00, 0xDD,
  /* 237, 0xED */  0x00, 0x00, 0xBB,
  /* 238, 0xEE */  0x00, 0x00, 0xAA,
  /* 239, 0xEF */  0x00, 0x00, 0x88,
  /* 240, 0xF0 */  0x00, 0x00, 0x77,
  /* 241, 0xF1 */  0x00, 0x00, 0x55,
  /* 242, 0xF2 */  0x00, 0x00, 0x44,
  /* 243, 0xF3 */  0x00, 0x00, 0x22,
  /* 244, 0xF4 */  0x00, 0x00, 0x11,
  /* 245, 0xF5 */  0xEE, 0xEE, 0xEE,
  /* 246, 0xF6 */  0xDD, 0xDD, 0xDD,
  /* 247, 0xF7 */  0xBB, 0xBB, 0xBB,
  /* 248, 0xF8 */  0xAA, 0xAA, 0xAA,
  /* 249, 0xF9 */  0x88, 0x88, 0x88,
  /* 250, 0xFA */  0x77, 0x77, 0x77,
  /* 251, 0xFB */  0x55, 0x55, 0x55,
  /* 252, 0xFC */  0x44, 0x44, 0x44,
  /* 253, 0xFD */  0x22, 0x22, 0x22,
  /* 254, 0xFE */  0x11, 0x11, 0x11,
  /* 255, 0xFF */  0x00, 0x00, 0x00
};


/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::AP4_VisualSampleEntry
+---------------------------------------------------------------------*/
AP4_VisualSampleEntry::AP4_VisualSampleEntry(
    AP4_Atom::Type    format, 
    AP4_EsDescriptor* descriptor,
    AP4_UI16          width,
    AP4_UI16          height,
    AP4_UI16          depth,
    const char*       compressor_name) :
    AP4_MpegSampleEntry(format, descriptor),
    m_Predefined1(0),
    m_Reserved2(0),
    m_Width(width),
    m_Height(height),
    m_HorizResolution(0x00480000),
    m_VertResolution(0x00480000),
    m_Reserved3(0),
    m_FrameCount(1),
    m_CompressorName(compressor_name),
    m_Depth(depth),
    m_ColorTableId(0),
    m_hasPalette(false)
{
    memset(m_Predefined2, 0, sizeof(m_Predefined2));
    memset(m_Palette, 0, sizeof(m_Palette));
    m_Size += 70;
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::AP4_VisualSampleEntry
+---------------------------------------------------------------------*/
AP4_VisualSampleEntry::AP4_VisualSampleEntry(AP4_Atom::Type   format,
                                             AP4_Size         size, 
                                             AP4_ByteStream&  stream,
                                             AP4_AtomFactory& atom_factory) :
    AP4_MpegSampleEntry(format, size)
{
    // read fields
    AP4_Size fields_size = GetFieldsSize();
    ReadFields(stream);

    // read children atoms (ex: esds and maybe others)
    ReadChildren(atom_factory, stream, size-AP4_ATOM_HEADER_SIZE-fields_size);
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_VisualSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+70;
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // read fields from this class
    stream.ReadUI16(m_Predefined1);
    stream.ReadUI16(m_Reserved2);
    stream.Read(m_Predefined2, sizeof(m_Predefined2), NULL);
    stream.ReadUI16(m_Width);
    stream.ReadUI16(m_Height);
    stream.ReadUI32(m_HorizResolution);
    stream.ReadUI32(m_VertResolution);
    stream.ReadUI32(m_Reserved3);
    stream.ReadUI16(m_FrameCount);

    char compressor_name[33];
    stream.Read(compressor_name, 32);
    int name_length = compressor_name[0];
    if (name_length > 0 && name_length < 32) {
        compressor_name[name_length + 1] = 0;
        m_CompressorName = &compressor_name[1];
    }

    stream.ReadUI16(m_Depth);
    stream.ReadUI16(m_ColorTableId);

    // code from ffmpeg;
    m_hasPalette = false;
    memset(m_Palette, 0, sizeof(m_Palette));

    AP4_UI16 color_depth     = m_Depth & 0x1F;
    AP4_UI16 color_greyscale = m_Depth & 0x20;
    if ((color_depth == 2) || (color_depth == 4) || (color_depth == 8)) {
        AP4_UI16 color_count;
        AP4_Byte a, r, g, b;
        if (color_greyscale) {
            /* compute the greyscale palette */
            m_Depth            = color_depth;
            color_count        = 1 << color_depth;
            int color_index    = 255;
            AP4_UI16 color_dec = 256 / (color_count - 1);
            for (AP4_UI16 j = 0; j < color_count; j++) {
                r = g = b = color_index;
                m_Palette[j] = (0xFFU << 24) | (r << 16) | (g << 8) | (b);
                color_index -= color_dec;
                if (color_index < 0)
                    color_index = 0;
            }
        } else if (m_ColorTableId) {
            /* if flag bit 3 is set, use the default palette */
            const AP4_Byte *color_table;
            color_count = 1 << color_depth;
            if (color_depth == 2)
                color_table = qt_default_palette_4;
            else if (color_depth == 4)
                color_table = qt_default_palette_16;
            else
                color_table = qt_default_palette_256;

            for (AP4_UI16 j = 0; j < color_count; j++) {
                r = color_table[j * 3 + 0];
                g = color_table[j * 3 + 1];
                b = color_table[j * 3 + 2];
                m_Palette[j] = (0xFFU << 24) | (r << 16) | (g << 8) | (b);
            }
        } else {
            /* load the palette from the file */
            AP4_UI32 color_start;
            AP4_UI16 color_end;
            stream.ReadUI32(color_start);
            stream.ReadUI16(color_count);
            stream.ReadUI16(color_end);
            if ((color_start <= 255) && (color_end <= 255)) {
                for (AP4_UI16 j = color_start; j <= color_end; j++) {
                    AP4_Byte skip;
                    stream.ReadUI08(a);
                    stream.ReadUI08(skip);
                    stream.ReadUI08(r);
                    stream.ReadUI08(skip);
                    stream.ReadUI08(g);
                    stream.ReadUI08(skip);
                    stream.ReadUI08(b);
                    stream.ReadUI08(skip);
                    m_Palette[j] = (a << 24 ) | (r << 16) | (g << 8) | (b);
                }
            }
        }
        m_hasPalette = true;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
        
    // write the fields of the base class
    result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;

    // predefined1
    result = stream.WriteUI16(m_Predefined1);
    if (AP4_FAILED(result)) return result;
    
    // reserved2
    result = stream.WriteUI16(m_Reserved2);
    if (AP4_FAILED(result)) return result;
    
    // predefined2
    result = stream.Write(m_Predefined2, sizeof(m_Predefined2));
    if (AP4_FAILED(result)) return result;
    
    // width
    result = stream.WriteUI16(m_Width);
    if (AP4_FAILED(result)) return result;
    
    // height
    result = stream.WriteUI16(m_Height);
    if (AP4_FAILED(result)) return result;
    
    // horizontal resolution
    result = stream.WriteUI32(m_HorizResolution);
    if (AP4_FAILED(result)) return result;
    
    // vertical resolution
    result = stream.WriteUI32(m_VertResolution);
    if (AP4_FAILED(result)) return result;
    
    // reserved3
    result = stream.WriteUI32(m_Reserved3);
    if (AP4_FAILED(result)) return result;
    
    // frame count
    result = stream.WriteUI16(m_FrameCount);
    if (AP4_FAILED(result)) return result;
    
    // compressor name
    unsigned char compressor_name[32];
    unsigned int name_length = m_CompressorName.length();
    if (name_length > 31) name_length = 31;
    compressor_name[0] = name_length;
    for (unsigned int i=0; i<name_length; i++) {
        compressor_name[i+1] = m_CompressorName[i];
    }
    for (unsigned int i=name_length+1; i<32; i++) {
        compressor_name[i] = 0;
    }
    result = stream.Write(compressor_name, 32);
    if (AP4_FAILED(result)) return result;
    
    // depth
    result = stream.WriteUI16(m_Depth);
    if (AP4_FAILED(result)) return result;
    
    // color table id
    result = stream.WriteUI16(m_ColorTableId);
    if (AP4_FAILED(result)) return result;
    
    return result;
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_VisualSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // dump the fields of the base class
    AP4_SampleEntry::InspectFields(inspector);

    // fields
    inspector.AddField("width", m_Width);
    inspector.AddField("height", m_Height);
    inspector.AddField("compressor", m_CompressorName.c_str());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_VisualSampleEntry::ToSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_VisualSampleEntry::ToSampleDescription()
{
    // get the decoder config descriptor
    const AP4_DecoderConfigDescriptor* dc_desc;
    dc_desc = GetDecoderConfigDescriptor();
    if (dc_desc == NULL) return NULL;
    const AP4_DataBuffer* dsi = NULL;
    const AP4_DecoderSpecificInfoDescriptor* dsi_desc =
        dc_desc->GetDecoderSpecificInfoDescriptor();
    if (dsi_desc != NULL) {
        dsi = &dsi_desc->GetDecoderSpecificInfo();
    }

    // create a sample description
    return new AP4_MpegVideoSampleDescription(
        dc_desc->GetObjectTypeIndication(),
        m_Width,
        m_Height,
        m_Depth,
        m_CompressorName.c_str(),
        dsi,
        dc_desc->GetBufferSize(),
        dc_desc->GetMaxBitrate(),
        dc_desc->GetAvgBitrate());
}

/*----------------------------------------------------------------------
|       AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry(AP4_UI16          width,
                                         AP4_UI16          height,
                                         AP4_UI16          depth,
                                         const char*       compressor_name,
                                         AP4_EsDescriptor* descriptor) :
    AP4_VisualSampleEntry(AP4_ATOM_TYPE_MP4V, 
                          descriptor,
                          width, 
                          height, 
                          depth, 
                          compressor_name)
{
}

/*----------------------------------------------------------------------
|       AP4_Mp4vSampleEntry::AP4_Mp4aSampleEntry
+---------------------------------------------------------------------*/
AP4_Mp4vSampleEntry::AP4_Mp4vSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_VisualSampleEntry(AP4_ATOM_TYPE_MP4V, size, stream, atom_factory)
{
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry
+---------------------------------------------------------------------*/
AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry(AP4_UI16 hint_track_version,
                                               AP4_UI16 highest_compatible_version,
                                               AP4_UI32 max_packet_size,
                                               AP4_UI32 timescale):
    AP4_SampleEntry(AP4_ATOM_TYPE_RTP),
    m_HintTrackVersion(hint_track_version),
    m_HighestCompatibleVersion(highest_compatible_version),
    m_MaxPacketSize(max_packet_size)
{
    // build an atom for timescale
    AddChild(new AP4_TimsAtom(timescale));
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry
+---------------------------------------------------------------------*/
AP4_RtpHintSampleEntry::AP4_RtpHintSampleEntry(AP4_Size         size,
                                               AP4_ByteStream&  stream,
                                               AP4_AtomFactory& atom_factory): 
    AP4_SampleEntry(AP4_ATOM_TYPE_RTP, size)
{
    // read fields
    AP4_Size fields_size = GetFieldsSize();
    ReadFields(stream);

    // read children atoms (ex: esds and maybe others)
    ReadChildren(atom_factory, stream, size-AP4_ATOM_HEADER_SIZE-fields_size);
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::~AP4_RtpHintSampleEntry
+---------------------------------------------------------------------*/
AP4_RtpHintSampleEntry::~AP4_RtpHintSampleEntry() 
{
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_RtpHintSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+8;
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // data
    result = stream.ReadUI16(m_HintTrackVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_HighestCompatibleVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI32(m_MaxPacketSize);
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // data
    result = stream.WriteUI16(m_HintTrackVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_HighestCompatibleVersion);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(m_MaxPacketSize);
    if (AP4_FAILED(result)) return result;

    return result;
}

/*----------------------------------------------------------------------
|       AP4_RtpHintSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_RtpHintSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // sample entry
    AP4_SampleEntry::InspectFields(inspector);
    
    // fields
    inspector.AddField("hint_track_version", m_HintTrackVersion);
    inspector.AddField("highest_compatible_version", m_HighestCompatibleVersion);
    inspector.AddField("max_packet_size", m_MaxPacketSize);
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_TextSampleEntry::AP4_TextSampleEntry
+---------------------------------------------------------------------*/
AP4_TextSampleEntry::AP4_TextSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory): 
    AP4_SampleEntry(AP4_ATOM_TYPE_TEXT, size)
{
    // read fields
    ReadFields(stream);
}

/*----------------------------------------------------------------------
|       AP4_TextSampleEntry::~AP4_TextSampleEntry
+---------------------------------------------------------------------*/
AP4_TextSampleEntry::~AP4_TextSampleEntry() 
{
}

/*----------------------------------------------------------------------
|       AP4_TextSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TextSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // data
    result = stream.ReadUI32(m_Description.DisplayFlags);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI32(m_Description.TextJustification);
    if (AP4_FAILED(result)) return result;
    result = stream.Read(&m_Description.BackgroundColor, 4);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Top);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Left);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Bottom);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Right);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.StartChar);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.EndChar);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.Ascent);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.Font.Id);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.Style.Font.Face);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.Style.Font.Size);
    if (AP4_FAILED(result)) return result;
    result = stream.Read(&m_Description.Style.Font.Color, 4);
    if (AP4_FAILED(result)) return result;

    // TODO: stream.ReadString(); -> m_Description.DefaultFontName

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_TextSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TextSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // TODO: data

    return result;
}

/*----------------------------------------------------------------------
|       AP4_TextSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_TextSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // sample entry
    AP4_SampleEntry::InspectFields(inspector);
    
    // TODO: fields
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::AP4_Tx3gSampleEntry
+---------------------------------------------------------------------*/
AP4_Tx3gSampleEntry::AP4_Tx3gSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory): 
    AP4_SampleEntry(AP4_ATOM_TYPE_TX3G, size)
{
    // read fields
    AP4_Size fields_size = GetFieldsSize();
    ReadFields(stream);

    // read children atoms (fdat? blnk?)
    ReadChildren(atom_factory, stream, size-AP4_ATOM_HEADER_SIZE-fields_size);
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::~AP4_Tx3gSampleEntry
+---------------------------------------------------------------------*/
AP4_Tx3gSampleEntry::~AP4_Tx3gSampleEntry() 
{
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_Tx3gSampleEntry::GetFieldsSize()
{
    return AP4_SampleEntry::GetFieldsSize()+4+1+1+4+2+2+2+2+2+2+2+1+1+4;
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Tx3gSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::ReadFields(stream);
    if (result < 0) return result;

    // data
    result = stream.ReadUI32(m_Description.DisplayFlags);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.HorizontalJustification);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.VerticalJustification);
    if (AP4_FAILED(result)) return result;
    result = stream.Read(&m_Description.BackgroundColor, 4);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Top);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Left);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Bottom);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.TextBox.Right);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.StartChar);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.EndChar);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI16(m_Description.Style.Font.Id);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.Style.Font.Face);
    if (AP4_FAILED(result)) return result;
    result = stream.ReadUI08(m_Description.Style.Font.Size);
    if (AP4_FAILED(result)) return result;
    result = stream.Read(&m_Description.Style.Font.Color, 4);
    if (AP4_FAILED(result)) return result;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Tx3gSampleEntry::WriteFields(AP4_ByteStream& stream)
{
    // sample entry
    AP4_Result result = AP4_SampleEntry::WriteFields(stream);
    if (AP4_FAILED(result)) return result;
    
    // TODO: data

    return result;
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_Tx3gSampleEntry::InspectFields(AP4_AtomInspector& inspector)
{
    // sample entry
    AP4_SampleEntry::InspectFields(inspector);
    
    // TODO: fields
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Tx3gSampleEntry::GetFontNameById
+---------------------------------------------------------------------*/

AP4_Result 
AP4_Tx3gSampleEntry::GetFontNameById(AP4_Ordinal Id, AP4_String& Name)
{
    if(AP4_FtabAtom* ftab = dynamic_cast<AP4_FtabAtom*>(GetChild(AP4_ATOM_TYPE_FTAB)))
    {
        AP4_Array<AP4_FtabAtom::AP4_Tx3gFontRecord> FontRecords = ftab->GetFontRecords();

        for(int i = 0, j = FontRecords.ItemCount(); i < j; i++)
        {
            if(Id == FontRecords[i].Id)
            {
                Name = FontRecords[i].Name;
                return AP4_SUCCESS;
            }
        }
    }

    return AP4_FAILURE;
}

static int freq[]     = { 48000, 44100, 32000, 24000, 22050, 16000 };
static int channels[] = { 2, 1, 2, 3, 3, 4, 4, 5 };

/*----------------------------------------------------------------------
|       AP4_AC3SampleEntry::AP4_AC3SampleEntry
+---------------------------------------------------------------------*/
AP4_AC3SampleEntry::AP4_AC3SampleEntry(AP4_Size         size,
                                       AP4_ByteStream&  stream,
                                       AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE__AC3, size),
    m_ExtSize(0)
{
    // read fields
    ReadFields(stream);

    AP4_Size fields_size = GetFieldsSize();

    // read children atoms
    ReadChildren(atom_factory, stream, size - AP4_ATOM_HEADER_SIZE - fields_size);
}

/*----------------------------------------------------------------------
|       AP4_AC3SampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AC3SampleEntry::ReadFields(AP4_ByteStream& stream)
{
    AP4_AudioSampleEntry::ReadFields(stream);

    // SampleSize field from AudioSampleEntry shall be ignored
    m_SampleSize = 0;

    // AC3SpecificBox - 'dac3' atom
    AP4_UI32 size, atom;
    stream.ReadUI32(size);
    stream.ReadUI32(atom);
    if (atom != AP4_ATOM_TYPE('d','a','c','3')) {
        AP4_Offset offset;
        stream.Tell(offset);
        stream.Seek(offset - 8);
        return AP4_SUCCESS;
    }

    m_ExtSize = size;

    AP4_UI32 data;
    stream.ReadUI24(data);

    // fscod
    m_SampleRateExtra = freq[(data >> 22) & 0x3];

    // acmod + lfeon
    m_ChannelCount = channels[(data >> 11) & 0x7] + ((data >> 10) & 0x1);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_AC3SampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_AC3SampleEntry::GetFieldsSize()
{
    return AP4_AudioSampleEntry::GetFieldsSize() + m_ExtSize;
}

/*----------------------------------------------------------------------
|       AP4_EAC3SampleEntry::AP4_EAC3SampleEntry
+---------------------------------------------------------------------*/
AP4_EAC3SampleEntry::AP4_EAC3SampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE_EAC3, size),
    m_ExtSize(0)
{
    // read fields
    ReadFields(stream);

    AP4_Size fields_size = GetFieldsSize();

    // read children atoms
    ReadChildren(atom_factory, stream, size - AP4_ATOM_HEADER_SIZE - fields_size);
}

/*----------------------------------------------------------------------
|       AP4_EAC3SampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_EAC3SampleEntry::ReadFields(AP4_ByteStream& stream)
{
    AP4_AudioSampleEntry::ReadFields(stream);

    // SampleSize field from AudioSampleEntry shall be ignored
    m_SampleSize = 0;

    // EAC3SpecificBox - 'dec3' atom
    AP4_UI32 size, atom;
    stream.ReadUI32(size);
    stream.ReadUI32(atom);
    if (atom != AP4_ATOM_TYPE('d','e','c','3')) {
        AP4_Offset offset;
        stream.Tell(offset);
        stream.Seek(offset - 8);
        return AP4_SUCCESS;
    }

    m_ExtSize = size;

    AP4_UI16 skip;
    stream.ReadUI16(skip); // data_rate and substream_count
    AP4_UI32 data;
    stream.ReadUI24(data);

    // fscod
    m_SampleRateExtra = freq[(data >> 6) & 0x3];

    // acmod + lfeon
    m_ChannelCount = channels[(data >> 9) & 0x7] + ((data >> 8) & 0x1);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_EAC3SampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_EAC3SampleEntry::GetFieldsSize()
{
    return AP4_AudioSampleEntry::GetFieldsSize() + m_ExtSize;
}

/*----------------------------------------------------------------------
|       AP4_FLACSampleEntry::AP4_FLACSampleEntry
+---------------------------------------------------------------------*/
AP4_FLACSampleEntry::AP4_FLACSampleEntry(AP4_Size         size,
                                         AP4_ByteStream&  stream,
                                         AP4_AtomFactory& atom_factory) :
    AP4_AudioSampleEntry(AP4_ATOM_TYPE_FLAC, size),
    m_ExtSize(0)
{
    // read fields
    ReadFields(stream);

    AP4_Size fields_size = GetFieldsSize();

    // read children atoms
    ReadChildren(atom_factory, stream, size - AP4_ATOM_HEADER_SIZE - fields_size);
}

#define FLAC_METADATA_TYPE_STREAMINFO  0
#define FLAC_STREAMINFO_SIZE          34

/*----------------------------------------------------------------------
|       AP4_FLACSampleEntry::ReadFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_FLACSampleEntry::ReadFields(AP4_ByteStream& stream)
{
    AP4_AudioSampleEntry::ReadFields(stream);

    // FLACSpecificBox - 'dfLa' atom
    AP4_UI32 size, atom;
    stream.ReadUI32(size);
    stream.ReadUI32(atom);
    if (atom != AP4_ATOM_TYPE('d','f','L','a')) {
        AP4_Offset offset;
        stream.Tell(offset);
        stream.Seek(offset - 8);
        return AP4_SUCCESS;
    }

    m_ExtSize = size;

    AP4_UI08 version;
    stream.ReadUI08(version);
    if (version != 0) {
        return AP4_SUCCESS;
    }
    AP4_UI32 flags;
    stream.ReadUI24(flags);

    AP4_UI08 type;
    stream.ReadUI08(type); type = type & 0x7F;
    stream.ReadUI24(size);
    if (type != FLAC_METADATA_TYPE_STREAMINFO || size != FLAC_STREAMINFO_SIZE) {
        return AP4_SUCCESS;
    }

    m_Data.SetDataSize(size);
    stream.Read(m_Data.UseData(), size);

    const AP4_Byte* data = m_Data.UseData();
    m_SampleRateExtra = (data[10] << 12) | (data[11] << 4 ) | (data[12] >> 4);
    m_ChannelCount = ((data[12] >> 1) & 0x7) + 1;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_FLACSampleEntry::GetFieldsSize
+---------------------------------------------------------------------*/
AP4_Size
AP4_FLACSampleEntry::GetFieldsSize()
{
    return AP4_AudioSampleEntry::GetFieldsSize() + m_ExtSize;
}
