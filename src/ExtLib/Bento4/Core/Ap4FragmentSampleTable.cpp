/*****************************************************************
|
|    AP4 - Fragment Sample Table
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4FragmentSampleTable.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_FRAG_FLAG_SAMPLE_IS_DIFFERENCE = 0x00010000;

/*----------------------------------------------------------------------
|       AP4_FragmentSampleTable::AP4_FragmentSampleTable
+---------------------------------------------------------------------*/
AP4_FragmentSampleTable::AP4_FragmentSampleTable() :
    m_Duration(0)
{
}

/*----------------------------------------------------------------------
|       AP4_FragmentSampleTable::GetSample
+---------------------------------------------------------------------*/
AP4_Result
AP4_FragmentSampleTable::GetSample(AP4_Ordinal index, AP4_Sample& sample)
{
    if (index >= m_FragmentSamples.ItemCount()) return AP4_ERROR_OUT_OF_RANGE;

    // copy the sample
    sample = m_FragmentSamples[index];

    return AP4_SUCCESS;

}

/*----------------------------------------------------------------------
|       AP4_FragmentSampleTable::AddTrun
+---------------------------------------------------------------------*/
AP4_Result
AP4_FragmentSampleTable::AddTrun(AP4_TrunAtom* trun, AP4_TfhdAtom* tfhd, AP4_TrexAtom* trex, AP4_ByteStream& stream, AP4_UI64& dts_origin, AP4_Offset moof_offset, AP4_Offset& mdat_payload_offset)
{
    if (trun) {
        AP4_Flags tfhd_flags = tfhd->GetFlags();
        AP4_Flags trun_flags = trun->GetFlags();
    
        // update the number of samples
        AP4_Cardinal start = m_FragmentSamples.ItemCount();
        m_FragmentSamples.SetItemCount(start + trun->GetEntries().ItemCount());
        
        // base data offset
        AP4_Offset data_offset = 0;
        if (tfhd_flags & AP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT) {
            data_offset = tfhd->GetBaseDataOffset();
        } else {
            data_offset = moof_offset;
        }
        if (trun_flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
            data_offset += trun->GetDataOffset();
        }         
        // MS hack
        if (data_offset == moof_offset) {
            data_offset = mdat_payload_offset;
        } else {
            mdat_payload_offset = data_offset;
        }

        // sample description index
        AP4_UI32 sample_description_index = 0;
        if (tfhd_flags & AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT) {
            sample_description_index = tfhd->GetSampleDescriptionIndex();
        } else if (trex) {
            sample_description_index = trex->GetDefaultSampleDescriptionIndex();
        }
       
        // default sample size
        AP4_UI32 default_sample_size = 0;
        if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT) {
            default_sample_size = tfhd->GetDefaultSampleSize();
        } else if (trex) {
            default_sample_size = trex->GetDefaultSampleSize();
        }
    
        // default sample duration
        AP4_UI32 default_sample_duration = 0;
        if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT) {
            default_sample_duration = tfhd->GetDefaultSampleDuration();
        } else if (trex) {
            default_sample_duration = trex->GetDefaultSampleDuration();
        }
    
        // default sample flags
        AP4_UI32 default_sample_flags = 0;
        if (tfhd_flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT) {
            default_sample_flags = tfhd->GetDefaultSampleFlags();
        } else if (trex) {
            default_sample_flags = trex->GetDefaultSampleFlags();
        }

        // parse all trun entries to setup the samples
        AP4_UI64 dts = dts_origin ? dts_origin : m_Duration;
        for (AP4_Cardinal i = 0; i < trun->GetEntries().ItemCount(); i++) {
            const AP4_TrunAtom::Entry& entry  = trun->GetEntries()[i];
            AP4_Sample&                sample = m_FragmentSamples[start + i];
        
            // sample size
            if (trun_flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
                sample.SetSize(entry.sample_size);
            } else {
                sample.SetSize(default_sample_size);
            }
            mdat_payload_offset += sample.GetSize(); // update the payload offset
        
            // sample duration
            if (trun_flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
                sample.SetDuration(entry.sample_duration);
            } else {
                sample.SetDuration(default_sample_duration);
            }

            // sample flags
            AP4_UI32 sample_flags = default_sample_flags;
            if (i == 0 && (trun_flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT)) {
                sample_flags = trun->GetFirstSampleFlags();
            } else if (trun_flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
                sample_flags = entry.sample_flags;
            }
            if ((sample_flags & AP4_FRAG_FLAG_SAMPLE_IS_DIFFERENCE) == 0) {
                sample.SetSync(true);
            } else {
                sample.SetSync(false);
            }
        
            // sample description index
            if (sample_description_index >= 1) {
                sample.SetDescriptionIndex(sample_description_index - 1);
            }
        
            // data stream
            sample.SetDataStream(stream);
        
            // data offset
            sample.SetOffset(data_offset);
            data_offset += sample.GetSize();
        
            // dts and cts
            sample.SetDts(dts);
            AP4_UI32 offset = 0;
            if (trun_flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
                offset = entry.sample_composition_time_offset;
            }
            sample.SetCts(dts + offset);
        
            // update the counters
            dts        += sample.GetDuration();
            m_Duration += sample.GetDuration();
        }
    
        // update the dts
        dts_origin = dts;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_FragmentSampleTable::GetSampleIndexForTimeStamp
+---------------------------------------------------------------------*/
AP4_Result
AP4_FragmentSampleTable::GetSampleIndexForTimeStamp(AP4_SI64 ts, AP4_Ordinal& index)
{
    for (AP4_Ordinal i = 0; i < m_FragmentSamples.ItemCount(); i++) {
        if (m_FragmentSamples[i].GetCts() > ts) {
            index = i == 0 ? i : i - 1;
            return AP4_SUCCESS;
        }
    }

    return AP4_FAILURE;
}
