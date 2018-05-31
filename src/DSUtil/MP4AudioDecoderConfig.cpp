/*
 * (C) 2018 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "GolombBuffer.h"
#include "MP4AudioDecoderConfig.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const BYTE AAC_MAX_SAMPLING_FREQUENCY_INDEX = 12;
static const unsigned int AacSamplingFreqTable[13] =
{
    96000, 88200, 64000, 48000, 
    44100, 32000, 24000, 22050, 
    16000, 12000, 11025, 8000, 
    7350
};

bool CMP4AudioDecoderConfig::ParseAudioObjectType(CGolombBuffer& parser, BYTE& object_type)
{
    if (parser.BitsLeft() < 5) return false;
    object_type = (BYTE)parser.BitRead(5);
    if (object_type == 31) {
        if (parser.BitsLeft() < 6) return false;
        object_type = (BYTE)(32 + parser.BitRead(6));
    }
    return true;
}

/*----------------------------------------------------------------------
|   CMP4AudioDecoderConfig::ParseExtension
+---------------------------------------------------------------------*/
bool CMP4AudioDecoderConfig::ParseExtension(CGolombBuffer& parser)
{
    if (parser.BitsLeft() < 16) return false;
    unsigned int sync_extension_type = parser.BitRead(11);
    if (sync_extension_type == 0x2b7) {
        bool result = ParseAudioObjectType(parser, m_Extension.m_ObjectType);
        if (!result) return result;
        if (m_Extension.m_ObjectType == AOT_SBR) {
            m_Extension.m_SbrPresent = (parser.BitRead(1) == 1);
            if (m_Extension.m_SbrPresent) {
                result = ParseSamplingFrequency(parser, 
                                                m_SamplingFrequencyIndex,
                                                m_SamplingFrequency);
                if (!result) return result;
                if (parser.BitsLeft() >= 12) {
                    sync_extension_type = parser.BitRead(11);
                    if (sync_extension_type == 0x548) {
                        m_Extension.m_PsPresent = (parser.BitRead(1) == 1);
                    }
                }
            }
        } else if (m_Extension.m_ObjectType == AOT_ER_BSAC) {
            m_Extension.m_SbrPresent = (parser.BitRead(1) == 1);
            if (m_Extension.m_SbrPresent) {
                result = ParseSamplingFrequency(parser, 
                                                m_SamplingFrequencyIndex,
                                                m_SamplingFrequency);
                if (!result) return result;
            } 
            parser.BitRead(4); // extensionChannelConfiguration           
        }
    }
    return true;
}

bool CMP4AudioDecoderConfig::ParseGASpecificInfo(CGolombBuffer& parser)
{
    if (parser.BitsLeft() < 2) return false;
    m_FrameLengthFlag = (parser.BitRead(1) == 1);
    m_DependsOnCoreCoder = (parser.BitRead(1) == 1);
    if (m_DependsOnCoreCoder) {
        if (parser.BitsLeft() < 14) return false;
        m_CoreCoderDelay = parser.BitRead(14);
    } else {
        m_CoreCoderDelay = 0;
    }
    if (parser.BitsLeft() < 1) return false;
    unsigned int extensionFlag = parser.BitRead(1);
    if (m_ChannelConfiguration == 0) {
        if (ParseProgramConfigElement(parser) != true) {
            return false;
        }
    }
    if (m_ObjectType == AOT_AAC_SCALABLE ||
        m_ObjectType == AOT_ER_AAC_SCALABLE) {
        if (parser.BitsLeft() < 3) return false;
        parser.BitRead(3); // layerNr
    }
    if (extensionFlag) {
        if (m_ObjectType == AOT_ER_BSAC) {
            if (parser.BitsLeft() < 16) return false;
            parser.BitRead(16); // numOfSubFrame (5); layer_length (11)
        }
        if (m_ObjectType == AOT_ER_AAC_LC       ||
            m_ObjectType == AOT_ER_AAC_SCALABLE ||
            m_ObjectType == AOT_ER_AAC_LD) {
            if (parser.BitsLeft() < 3) return false;
            parser.BitRead(3); // aacSectionDataResilienceFlag (1)
                                // aacScalefactorDataResilienceFlag (1)
                                // aacSpectralDataResilienceFlag (1)
        }
        if (parser.BitsLeft() < 1) return false;
        unsigned int extensionFlag3 = parser.BitRead(1);
        if (extensionFlag3) {
            // return false;
        }
    }
    
    return true;
}

bool CMP4AudioDecoderConfig::ParseSamplingFrequency(CGolombBuffer& parser, 
                                                    BYTE&          sampling_frequency_index,
                                                    unsigned int&  sampling_frequency)
{
    if (parser.BitsLeft() < 4) {
        return false;
    }

    sampling_frequency_index = parser.BitRead(4);
    if (sampling_frequency_index == 0xF) {
        if (parser.BitsLeft() < 24) {
            return false;
        }
        sampling_frequency = parser.BitRead(24);
    } else if (sampling_frequency_index <= AAC_MAX_SAMPLING_FREQUENCY_INDEX) {
        sampling_frequency = AacSamplingFreqTable[sampling_frequency_index];
    } else {
        sampling_frequency = 0;
        return false;
    }

    return true;
}

bool CMP4AudioDecoderConfig::ParseProgramConfigElement(CGolombBuffer& parser)
{
    if (parser.BitsLeft() < 45) return false;
    parser.BitRead(4); // element_instance_tag
    parser.BitRead(2); // object_type
    parser.BitRead(4); // sampling_frequency_index
    BYTE num_front_channel_elements = parser.BitRead(4);
    BYTE num_side_channel_elements  = parser.BitRead(4);
    BYTE num_back_channel_elements  = parser.BitRead(4);
    BYTE num_lfe_channel_elements   = parser.BitRead(2);
    BYTE num_assoc_data_elements    = parser.BitRead(3);
    BYTE num_valid_cc_elements      = parser.BitRead(4);
    if (parser.BitRead(1) == 1) { // mono_mixdown_present
        parser.BitRead(4);        // mono_mixdown_element_number
    }
    if (parser.BitRead(1) == 1) { // stereo_mixdown_present
        parser.BitRead(4);        // stereo_mixdown_element_number
    }
    if (parser.BitRead(1) == 1) { // matrix_mixdown_idx_present
        parser.BitRead(2);        // matrix_mixdown_idx
        parser.BitRead(1);        // pseudo_surround_enable
    }

    if (parser.BitsLeft() < num_front_channel_elements * 5) return false;
    for (BYTE i = 0; i < num_front_channel_elements; i++) {
        bool front_element_is_cpe = (parser.BitRead(1) == 1);
        parser.BitRead(4); // front_element_tag_select
        if (front_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_side_channel_elements * 5) return false;
    for (BYTE i = 0; i < num_side_channel_elements; i++) {
        bool side_element_is_cpe = (parser.BitRead(1) == 1);
        parser.BitRead(4); // side_element_tag_select
        if (side_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_back_channel_elements * 5) return false;
    for (BYTE i = 0; i < num_back_channel_elements; i++) {
        bool back_element_is_cpe = (parser.BitRead(1) == 1);
        parser.BitRead(4); // back_element_tag_select
        if (back_element_is_cpe) {
            m_ChannelCount += 2;
        } else {
            m_ChannelCount += 1;
        }
    }

    if (parser.BitsLeft() < num_lfe_channel_elements * 4) return false;
    for (BYTE i = 0; i < num_lfe_channel_elements; i++) {
        parser.BitRead(4); // lfe_element_tag_select
        m_ChannelCount += 1;
    }

    if (parser.BitsLeft() < num_assoc_data_elements * 4) return false;
    for (BYTE i = 0; i < num_assoc_data_elements; i++) {
        parser.BitRead(4); // assoc_data_element_tag_select
    }

    if (parser.BitsLeft() < num_valid_cc_elements * 5) return false;
    for (BYTE i = 0; i < num_valid_cc_elements; i++) {
        parser.BitRead(1); // cc_element_is_ind_sw
        parser.BitRead(4); // valid_cc_element_tag_select
    }

    if (parser.BitsLeft() < 8) return false;
    BYTE comment_field_bytes = parser.BitRead(8);
    if (comment_field_bytes) {
        if (parser.BitsLeft() < 8 * (int)comment_field_bytes) return false;
        parser.BitRead(8 * comment_field_bytes); // comment_field_data
    }

    return true;
}

bool CMP4AudioDecoderConfig::Parse(const BYTE* data, int data_size)
{
    CGolombBuffer parser(data, data_size);
    return Parse(parser);
}

bool CMP4AudioDecoderConfig::Parse(CGolombBuffer& parser)
{
    // parse the audio object type
    bool result = ParseAudioObjectType(parser, m_ObjectType);
    if (!result) return result;

    // parse the sampling frequency
    result = ParseSamplingFrequency(parser, 
                                    m_SamplingFrequencyIndex, 
                                    m_SamplingFrequency);
    if (!result) return result;

    if (parser.BitsLeft() < 4) return false;
    m_ChannelConfiguration = parser.BitRead(4);
    if (m_ChannelConfiguration < 8) {
        static const BYTE channels[8] = { 0, 1, 2, 3, 4, 5, 6, 8 };
        m_ChannelCount = channels[m_ChannelConfiguration];
    }

    if (m_ObjectType == AOT_SBR ||
        m_ObjectType == AOT_PS) {
        m_Extension.m_ObjectType = AOT_SBR;
        m_Extension.m_SbrPresent = true;
        m_Extension.m_PsPresent  = m_ObjectType == AOT_PS;
        result = ParseSamplingFrequency(parser, 
                                        m_SamplingFrequencyIndex, 
                                        m_SamplingFrequency);
        if (!result) return result;
        result = ParseAudioObjectType(parser, m_ObjectType);
        if (!result) return result;
        if (m_ObjectType == AOT_ER_BSAC) {
            if (parser.BitsLeft() < 4) return false;
            parser.BitRead(4); // extensionChannelConfiguration (4)
        }
    } else {
        m_Extension.m_ObjectType = 0;
        m_Extension.m_SbrPresent = false;
        m_Extension.m_PsPresent  = false;
    }
    
    switch (m_ObjectType) {
        case AOT_AAC_MAIN:
        case AOT_AAC_LC:
        case AOT_AAC_SSR:
        case AOT_AAC_LTP:
        case AOT_AAC_SCALABLE:
        case AOT_TWINVQ:
        case AOT_ER_AAC_LC:
        case AOT_ER_AAC_LTP:
        case AOT_ER_AAC_SCALABLE:
        case AOT_ER_AAC_LD:
        case AOT_ER_TWINVQ:
        case AOT_ER_BSAC:
            result = ParseGASpecificInfo(parser);
            if (result == true) {
                if (m_Extension.m_ObjectType !=  AOT_SBR &&
                    parser.BitsLeft() >= 16) {
                    result = ParseExtension(parser);
                }
            }
            break;

        default:
            return false;
    }

    if (!m_Extension.m_SbrPresent && m_SamplingFrequency <= 24000) {
        m_SamplingFrequency *= 2;
    }

    if (m_Extension.m_PsPresent) {
        // HE-AACv2 Profile, always stereo.
        m_ChannelCount = 2;
    }

    return result;
}