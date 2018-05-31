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

#pragma once

class CGolombBuffer;

const BYTE AOT_AAC_MAIN              = 1;  /**< AAC Main Profile                             */
const BYTE AOT_AAC_LC                = 2;  /**< AAC Low Complexity                           */
const BYTE AOT_AAC_SSR               = 3;  /**< AAC Scalable Sample Rate                     */
const BYTE AOT_AAC_LTP               = 4;  /**< AAC Long Term Predictor                      */
const BYTE AOT_SBR                   = 5;  /**< Spectral Band Replication                    */
const BYTE AOT_AAC_SCALABLE          = 6;  /**< AAC Scalable                                 */
const BYTE AOT_TWINVQ                = 7;  /**< Twin VQ                                      */
const BYTE AOT_CELP                  = 8;  /**< CELP                                         */
const BYTE AOT_HVXC                  = 9;  /**< HVXC                                         */
const BYTE AOT_TTSI                  = 12; /**< TTSI                                         */
const BYTE AOT_MAIN_SYNTHETIC        = 13; /**< Main Synthetic                               */
const BYTE AOT_WAVETABLE_SYNTHESIS   = 14; /**< WavetableSynthesis                           */
const BYTE AOT_GENERAL_MIDI          = 15; /**< General MIDI                                 */
const BYTE AOT_ALGORITHMIC_SYNTHESIS = 16; /**< Algorithmic Synthesis                        */
const BYTE AOT_ER_AAC_LC             = 17; /**< Error Resilient AAC Low Complexity           */
const BYTE AOT_ER_AAC_LTP            = 19; /**< Error Resilient AAC Long Term Prediction     */
const BYTE AOT_ER_AAC_SCALABLE       = 20; /**< Error Resilient AAC Scalable                 */
const BYTE AOT_ER_TWINVQ             = 21; /**< Error Resilient Twin VQ                      */
const BYTE AOT_ER_BSAC               = 22; /**< Error Resilient Bit Sliced Arithmetic Coding */
const BYTE AOT_ER_AAC_LD             = 23; /**< Error Resilient AAC Low Delay                */
const BYTE AOT_ER_CELP               = 24; /**< Error Resilient CELP                         */
const BYTE AOT_ER_HVXC               = 25; /**< Error Resilient HVXC                         */
const BYTE AOT_ER_HILN               = 26; /**< Error Resilient HILN                         */
const BYTE AOT_ER_PARAMETRIC         = 27; /**< Error Resilient Parametric                   */
const BYTE AOT_SSC                   = 28; /**< SSC                                          */
const BYTE AOT_PS                    = 29; /**< Parametric Stereo                            */
const BYTE AOT_MPEG_SURROUND         = 30; /**< MPEG Surround                                */
const BYTE AOT_LAYER_1               = 32; /**< MPEG Layer 1                                 */
const BYTE AOT_LAYER_2               = 33; /**< MPEG Layer 2                                 */
const BYTE AOT_LAYER_3               = 34; /**< MPEG Layer 3                                 */
const BYTE AOT_DST                   = 35; /**< DST Direct Stream Transfer                   */
const BYTE AOT_ALS                   = 36; /**< ALS Lossless Coding                          */
const BYTE AOT_SLS                   = 37; /**< SLS Scalable Lossless Coding                 */
const BYTE AOT_SLS_NON_CORE          = 38; /**< SLS Sclable Lossless Coding Non-Core         */
const BYTE AOT_ER_AAC_ELD            = 39; /**< Error Resilient AAC ELD                      */
const BYTE AOT_SMR_SIMPLE            = 40; /**< SMR Simple                                   */
const BYTE AOT_SMR_MAIN              = 41; /**< SMR Main                                     */

class CMP4AudioDecoderConfig {
public:
    CMP4AudioDecoderConfig();

    bool ParseProgramConfigElement(CGolombBuffer& parser);

    bool Parse(const BYTE* data, int data_size);
    bool Parse(CGolombBuffer& parser);

    void Reset();
    
    BYTE         m_ObjectType;             /**< Type identifier for the audio data */
    BYTE         m_SamplingFrequencyIndex; /**< Index of the sampling frequency in the sampling frequency table */
    unsigned int m_SamplingFrequency;      /**< Sampling frequency */
    BYTE         m_ChannelCount;           /**< Number of audio channels */
    BYTE         m_ChannelConfiguration;   /**< Channel configuration */
    bool         m_FrameLengthFlag;        /**< Frame Length Flag     */
    bool         m_DependsOnCoreCoder;     /**< Depends on Core Coder */
    unsigned int m_CoreCoderDelay;         /**< Core Code delay       */

    struct {
        bool     m_SbrPresent;             /**< SBR is present        */
        bool     m_PsPresent;              /**< PS is present         */
        BYTE     m_ObjectType;             /**< Extension object type */
    } m_Extension;
    
private:
    bool ParseAudioObjectType(CGolombBuffer& parser, BYTE& object_type);
    bool ParseGASpecificInfo(CGolombBuffer& parser);
    bool ParseSamplingFrequency(CGolombBuffer& parser, 
                                BYTE&          sampling_frequency_index,
                                unsigned int&  sampling_frequency);
    bool ParseExtension(CGolombBuffer& parser);
};
