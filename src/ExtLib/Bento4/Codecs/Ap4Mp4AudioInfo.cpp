/*****************************************************************
|
|    AP4 - AAC Info
|
|    Copyright 2002-2009 Axiomatic Systems, LLC
|    Copyright 2018 MPC-BE
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
|   includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4Mp4AudioInfo.h"
#include "Ap4DataBuffer.h"
#include "Ap4SampleDescription.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int AP4_AAC_MAX_SAMPLING_FREQUENCY_INDEX = 12;
static const unsigned int AP4_AacSamplingFreqTable[13] =
{
    96000, 88200, 64000, 48000, 
    44100, 32000, 24000, 22050, 
    16000, 12000, 11025, 8000, 
    7350
};

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDsiParser
+---------------------------------------------------------------------*/
class AP4_Mp4AudioDsiParser
{
public:
    AP4_Mp4AudioDsiParser(const AP4_UI08* data, AP4_Size data_size) :
        m_Data(data, data_size), 
        m_Position(0) {}
        
    AP4_Size BitsLeft() { return 8*m_Data.GetDataSize()-m_Position; }
    AP4_UI32 ReadBits(unsigned int n) {
        AP4_UI32 result = 0;
        const AP4_UI08* data = m_Data.GetData();
        while (n) {
            unsigned int bits_avail = 8-(m_Position%8);
            unsigned int chunk_size = bits_avail >= n ? n : bits_avail;
            unsigned int chunk_bits = (((unsigned int)(data[m_Position/8]))>>(bits_avail-chunk_size))&((1<<chunk_size)-1);
            result = (result << chunk_size) | chunk_bits;
            n -= chunk_size;
            m_Position += chunk_size;
        }
    
        return result;
    }
    
private:
    AP4_DataBuffer m_Data;
    unsigned int   m_Position;
};

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::AP4_Mp4AudioDecoderConfig
+---------------------------------------------------------------------*/
AP4_Mp4AudioDecoderConfig::AP4_Mp4AudioDecoderConfig()
{
    Reset();
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::Reset
+---------------------------------------------------------------------*/
void
AP4_Mp4AudioDecoderConfig::Reset()
{
    m_ObjectType             = 0;
    m_SamplingFrequencyIndex = 0;
    m_SamplingFrequency      = 0;
    m_ChannelCount           = 0;
    m_ChannelConfiguration   = CHANNEL_CONFIG_NONE;
    m_FrameLengthFlag        = false;
    m_DependsOnCoreCoder     = false;
    m_CoreCoderDelay         = 0;
    m_Extension.m_SbrPresent = false;
    m_Extension.m_PsPresent  = false;
    m_Extension.m_ObjectType = 0;
    m_Extension.m_SamplingFrequencyIndex = 0;
    m_Extension.m_SamplingFrequency      = 0;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::ParseAudioObjectType
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::ParseAudioObjectType(AP4_Mp4AudioDsiParser& parser, AP4_UI08& object_type)
{
    if (parser.BitsLeft() < 5) return AP4_ERROR_INVALID_FORMAT;
    object_type = (AP4_UI08)parser.ReadBits(5);
    if ((int)object_type == 31) {
        if (parser.BitsLeft() < 6) return AP4_ERROR_INVALID_FORMAT;
        object_type = (AP4_UI08)(32 + parser.ReadBits(6));
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::ParseExtension
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::ParseExtension(AP4_Mp4AudioDsiParser& parser)
{
    if (parser.BitsLeft() < 16) return AP4_ERROR_INVALID_FORMAT;
    unsigned int sync_extension_type = parser.ReadBits(11);
    if (sync_extension_type == 0x2b7) {
        AP4_Result result = ParseAudioObjectType(parser, m_Extension.m_ObjectType);
        if (AP4_FAILED(result)) return result;
        if (m_Extension.m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR) {
            m_Extension.m_SbrPresent = (parser.ReadBits(1) == 1);
            if (m_Extension.m_SbrPresent) {
                result = ParseSamplingFrequency(parser, 
                                                m_Extension.m_SamplingFrequencyIndex,
                                                m_Extension.m_SamplingFrequency);
                if (AP4_FAILED(result)) return result;
                if (parser.BitsLeft() >= 12) {
                    sync_extension_type = parser.ReadBits(11);
                    if (sync_extension_type == 0x548) {
                        m_Extension.m_PsPresent = (parser.ReadBits(1) == 1);
                    }
                }
            }
        } else if (m_Extension.m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC) {
            m_Extension.m_SbrPresent = (parser.ReadBits(1) == 1);
            if (m_Extension.m_SbrPresent) {
                result = ParseSamplingFrequency(parser, 
                                                m_Extension.m_SamplingFrequencyIndex,
                                                m_Extension.m_SamplingFrequency);
                if (AP4_FAILED(result)) return result;
            } 
            parser.ReadBits(4); // extensionChannelConfiguration           
        }
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::ParseProgramConfigElement
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::ParseProgramConfigElement(AP4_Mp4AudioDsiParser& parser)
{
    if (parser.BitsLeft() < 45) return AP4_ERROR_INVALID_FORMAT;
    parser.ReadBits(4); // element_instance_tag
    parser.ReadBits(2); // object_type
    parser.ReadBits(4); // sampling_frequency_index
    AP4_UI08 num_front_channel_elements = parser.ReadBits(4);
    AP4_UI08 num_side_channel_elements  = parser.ReadBits(4);
    AP4_UI08 num_back_channel_elements  = parser.ReadBits(4);
    AP4_UI08 num_lfe_channel_elements   = parser.ReadBits(2);
    AP4_UI08 num_assoc_data_elements    = parser.ReadBits(3);
    AP4_UI08 num_valid_cc_elements      = parser.ReadBits(4);
    if (parser.ReadBits(1) == 1) { // mono_mixdown_present
        parser.ReadBits(4);        // mono_mixdown_element_number
    }
    if (parser.ReadBits(1) == 1) { // stereo_mixdown_present
        parser.ReadBits(4);        // stereo_mixdown_element_number
    }
    if (parser.ReadBits(1) == 1) { // matrix_mixdown_idx_present
        parser.ReadBits(2);        // matrix_mixdown_idx
        parser.ReadBits(1);        // pseudo_surround_enable
    }

    if (parser.BitsLeft() < num_front_channel_elements * 5) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_front_channel_elements; i++) {
        bool front_element_is_cpe = (parser.ReadBits(1) == 1);
        parser.ReadBits(4); // front_element_tag_select
        if (front_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_side_channel_elements * 5) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_side_channel_elements; i++) {
        bool side_element_is_cpe = (parser.ReadBits(1) == 1);
        parser.ReadBits(4); // side_element_tag_select
        if (side_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_back_channel_elements * 5) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_back_channel_elements; i++) {
        bool back_element_is_cpe = (parser.ReadBits(1) == 1);
        parser.ReadBits(4); // back_element_tag_select
        if (back_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_lfe_channel_elements * 4) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_lfe_channel_elements; i++) {
        parser.ReadBits(4); // lfe_element_tag_select
        m_ChannelCount += 1;
    }

    if (parser.BitsLeft() < num_assoc_data_elements * 4) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_assoc_data_elements; i++) {
        parser.ReadBits(4); // assoc_data_element_tag_select
    }

    if (parser.BitsLeft() < num_valid_cc_elements * 5) return AP4_ERROR_INVALID_FORMAT;
    for (AP4_UI08 i = 0; i < num_valid_cc_elements; i++) {
        parser.ReadBits(1); // cc_element_is_ind_sw
        parser.ReadBits(4); // valid_cc_element_tag_select
    }

    if (parser.BitsLeft() < 8) return AP4_ERROR_INVALID_FORMAT;
    AP4_UI08 comment_field_bytes = parser.ReadBits(8);
    if (comment_field_bytes) {
        if (parser.BitsLeft() < 8 * (AP4_Size)comment_field_bytes) return AP4_ERROR_INVALID_FORMAT;
        parser.ReadBits(8 * comment_field_bytes); // comment_field_data
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::ParseGASpecificInfo
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::ParseGASpecificInfo(AP4_Mp4AudioDsiParser& parser)
{
    if (parser.BitsLeft() < 2) return AP4_ERROR_INVALID_FORMAT;
    m_FrameLengthFlag = (parser.ReadBits(1) == 1);
    m_DependsOnCoreCoder = (parser.ReadBits(1) == 1);
    if (m_DependsOnCoreCoder) {
        if (parser.BitsLeft() < 14) return AP4_ERROR_INVALID_FORMAT;
        m_CoreCoderDelay = parser.ReadBits(14);
    } else {
        m_CoreCoderDelay = 0;
    }
    if (parser.BitsLeft() < 1) return AP4_ERROR_INVALID_FORMAT;
    unsigned int extensionFlag = parser.ReadBits(1);
    if (m_ChannelConfiguration == CHANNEL_CONFIG_NONE) {
        if (ParseProgramConfigElement(parser) != AP4_SUCCESS) {
            return AP4_ERROR_INVALID_FORMAT;
        }
    }
    if (m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SCALABLE ||
        m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE) {
        if (parser.BitsLeft() < 3) return AP4_ERROR_INVALID_FORMAT;
        parser.ReadBits(3); // layerNr
    }
    if (extensionFlag) {
        if (m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC) {
            if (parser.BitsLeft() < 16) return AP4_ERROR_INVALID_FORMAT;
            parser.ReadBits(16); // numOfSubFrame (5); layer_length (11)
        }
        if (m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LC       ||
            m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE ||
            m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LD) {
            if (parser.BitsLeft() < 3) return AP4_ERROR_INVALID_FORMAT;
            parser.ReadBits(3); // aacSectionDataResilienceFlag (1)
                                // aacScalefactorDataResilienceFlag (1)
                                // aacSpectralDataResilienceFlag (1)
        }
        if (parser.BitsLeft() < 1) return AP4_ERROR_INVALID_FORMAT;
        unsigned int extensionFlag3 = parser.ReadBits(1);
        if (extensionFlag3) {
            return AP4_ERROR_NOT_SUPPORTED_YET;
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::ParseSamplingFrequency
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::ParseSamplingFrequency(AP4_Mp4AudioDsiParser& parser, 
                                                  unsigned int&          sampling_frequency_index,
                                                  unsigned int&          sampling_frequency)
{
    if (parser.BitsLeft() < 4) {
        return AP4_ERROR_INVALID_FORMAT;
    }

    sampling_frequency_index = parser.ReadBits(4);
    if (sampling_frequency_index == 0xF) {
        if (parser.BitsLeft() < 24) {
            return AP4_ERROR_INVALID_FORMAT;
        }
        sampling_frequency = parser.ReadBits(24);
    } else if (sampling_frequency_index <= AP4_AAC_MAX_SAMPLING_FREQUENCY_INDEX) {
        sampling_frequency = AP4_AacSamplingFreqTable[sampling_frequency_index];
    } else {
        sampling_frequency = 0;
        return AP4_ERROR_INVALID_FORMAT;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Mp4AudioDecoderConfig::Parse
+---------------------------------------------------------------------*/
AP4_Result
AP4_Mp4AudioDecoderConfig::Parse(const unsigned char* data, 
                                 AP4_Size             data_size)
{
    AP4_Result            result;
    AP4_Mp4AudioDsiParser bits(data, data_size);

    // default config
    Reset();
    
    // parse the audio object type
    result = ParseAudioObjectType(bits, m_ObjectType);
    if (AP4_FAILED(result)) return result;

    // parse the sampling frequency
    result = ParseSamplingFrequency(bits, 
                                    m_SamplingFrequencyIndex, 
                                    m_SamplingFrequency);
    if (AP4_FAILED(result)) return result;

    if (bits.BitsLeft() < 4) {
        return AP4_ERROR_INVALID_FORMAT;
    }
    m_ChannelConfiguration = (ChannelConfiguration)bits.ReadBits(4);
    m_ChannelCount = (unsigned int)m_ChannelConfiguration;
    if (m_ChannelCount == 7) {
        m_ChannelCount = 8;
    } else if (m_ChannelCount > 7) {
        m_ChannelCount = 0;
    }
    
    if (m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR ||
        m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_PS) {
        m_Extension.m_ObjectType = AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR;
        m_Extension.m_SbrPresent = true;
        m_Extension.m_PsPresent  = m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_PS;
        result = ParseSamplingFrequency(bits, 
                                        m_Extension.m_SamplingFrequencyIndex, 
                                        m_Extension.m_SamplingFrequency);
        if (AP4_FAILED(result)) return result;
        result = ParseAudioObjectType(bits, m_ObjectType);
        if (AP4_FAILED(result)) return result;
        if (m_ObjectType == AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC) {
            if (bits.BitsLeft() < 4) return AP4_ERROR_INVALID_FORMAT;
            bits.ReadBits(4); // extensionChannelConfiguration (4)
        }
    } else {
        m_Extension.m_ObjectType             = 0;
        m_Extension.m_SamplingFrequency      = 0;
        m_Extension.m_SamplingFrequencyIndex = 0;
        m_Extension.m_SbrPresent             = false;
        m_Extension.m_PsPresent              = false;
    }
    
    switch (m_ObjectType) {
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_MAIN:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LC:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SSR:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_LTP:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_SCALABLE:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_TWINVQ:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LC:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LTP:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_AAC_LD:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_TWINVQ:
        case AP4_MPEG4_AUDIO_OBJECT_TYPE_ER_BSAC:
            result = ParseGASpecificInfo(bits);
            if (result == AP4_SUCCESS) {
                if (m_Extension.m_ObjectType !=  AP4_MPEG4_AUDIO_OBJECT_TYPE_SBR &&
                    bits.BitsLeft() >= 16) {
                    result = ParseExtension(bits);
                }
            }
            if (result == AP4_ERROR_NOT_SUPPORTED_YET) {
                // not a fatal error
                result = AP4_SUCCESS;
            }
            if (result != AP4_SUCCESS) return result;
            break;

        default:
            return AP4_ERROR_NOT_SUPPORTED_YET;
    }

    return AP4_SUCCESS;
}
