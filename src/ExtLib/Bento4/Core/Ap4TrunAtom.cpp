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
    AP4_UI32 sample_count = 0;
    stream.ReadUI32(sample_count);

    // read optional fields
    int optional_fields_count = (int)ComputeOptionalFieldsCount(m_Flags);
    if (m_Flags & AP4_TRUN_FLAG_DATA_OFFSET_PRESENT) {
        AP4_UI32 offset = 0;
        stream.ReadUI32(offset);
        m_DataOffset = (AP4_SI32)offset;
        --optional_fields_count;
    }
    if (m_Flags & AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT) {
        stream.ReadUI32(m_FirstSampleFlags);
        --optional_fields_count;
    }

    // discard unknown optional fields 
    for (int i = 0; i < optional_fields_count; i++) {
        AP4_UI32 discard;
        stream.ReadUI32(discard);
    }

    int record_fields_count = (int)ComputeRecordFieldsCount(m_Flags);
    m_Entries.SetItemCount(sample_count);
    for (AP4_UI32 i = 0; i < sample_count; i++) {
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT) {
            stream.ReadUI32(m_Entries[i].sample_duration);
            --record_fields_count;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT) {
            stream.ReadUI32(m_Entries[i].sample_size);
            --record_fields_count;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT) {
            stream.ReadUI32(m_Entries[i].sample_flags);
            --record_fields_count;
        }
        if (m_Flags & AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT) {
            stream.ReadUI32(m_Entries[i].sample_composition_time_offset);
            --record_fields_count;
        }
    
        // skip unknown fields 
        for (int j = 0;j < record_fields_count; j++) {
            AP4_UI32 discard;
            stream.ReadUI32(discard);
        }
    }
}
