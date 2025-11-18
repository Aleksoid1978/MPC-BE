/*****************************************************************
|
|    AP4 - trun Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4TrunAtom.h"

/*----------------------------------------------------------------------
|   AP4_TrunAtom::ComputeOptionalFieldsCount
+---------------------------------------------------------------------*/
AP4_UI32
AP4_TrunAtom::ComputeOptionalFieldsCount(AP4_UI32 flags)
{
    // count the number of bits set to 1 in the LSB of the flags
    AP4_UI32 count = 0;
    for (AP4_UI32 i = 0; i < 8; i++) {
        if (flags & (1 << i)) ++count;
    }

    return count;
}

/*----------------------------------------------------------------------
|   AP4_TrunAtom::ComputeOptionalFieldsCount
+---------------------------------------------------------------------*/
AP4_UI32
AP4_TrunAtom::ComputeRecordFieldsCount(AP4_UI32 flags)
{
    // count the number of bits set to 1 in the second byte of the flags
    AP4_UI32 count = 0;
    for (AP4_UI32 i = 0; i < 8; i++) {
        if (flags & (1 << (i + 8))) ++count;
    }

    return count;
}

/*----------------------------------------------------------------------
|       AP4_TrunAtom::AP4_TrunAtom
+---------------------------------------------------------------------*/

AP4_TrunAtom::AP4_TrunAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_TRUN, size, true, stream)
    , m_DataOffset(0)
    , m_FirstSampleFlags(0)
{
    if (size < AP4_FULL_ATOM_HEADER_SIZE + 4) {
        return;
    }
    AP4_UI32 sample_count = 0;
    stream.ReadUI32(sample_count);
    AP4_Size bytes_left = size - AP4_FULL_ATOM_HEADER_SIZE - 4;

    // read optional fields
    int optional_fields_count = (int)ComputeOptionalFieldsCount(m_Flags);
    if (m_Flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
        AP4_UI32 offset = 0;
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(offset))) {
            return;
        }
        m_DataOffset = (AP4_SI32)offset;
        if (optional_fields_count == 0) {
            return;
        }
        --optional_fields_count;
        bytes_left -= 4;
    }
    if (m_Flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT) {
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_FirstSampleFlags))) {
            return;
        }
        if (optional_fields_count == 0) {
            return;
        }
        --optional_fields_count;
        bytes_left -= 4;
    }

    // discard unknown optional fields
    for (int i = 0; i < optional_fields_count; i++) {
        AP4_UI32 discard;
        if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(discard))) {
            return;
        }
        bytes_left -= 4;
    }

    int record_fields_count = (int)ComputeRecordFieldsCount(m_Flags);
    if (!record_fields_count) {
        // nothing to read
        return;
    }

    if ((bytes_left / (record_fields_count * 4) < sample_count)) {
        // not enough data for all samples, the format is invalid
        return;
    }

    m_Entries.SetItemCount(sample_count);
    if (record_fields_count == 0) {
        return;
    }

    for (AP4_UI32 i = 0; i < sample_count; i++) {
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_duration))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_size))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(m_Entries[i].sample_flags))) {
                return;
            }
            --record_fields_count;
            bytes_left -= 4;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
            AP4_UI32 time_offset;
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(time_offset))) {
                return;
            }
            m_Entries[i].sample_composition_time_offset = static_cast<AP4_SI32>(time_offset);
            --record_fields_count;
            bytes_left -= 4;
        }

        // skip unknown fields
        for (int j = 0;j < record_fields_count; j++) {
            AP4_UI32 discard;
            if (bytes_left < 4 || AP4_FAILED(stream.ReadUI32(discard))) {
                return;
            }
            bytes_left -= 4;
        }
    }
}
