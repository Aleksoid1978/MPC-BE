/*****************************************************************
|
|    AP4 - tfhd Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4TfhdAtom.h"

/*----------------------------------------------------------------------
|       AP4_TfhdAtom::AP4_TfhdAtom
+---------------------------------------------------------------------*/

AP4_TfhdAtom::AP4_TfhdAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_TFHD, size, true, stream)
    , m_TrackId(0)
    , m_BaseDataOffset(0)
    , m_SampleDescriptionIndex(1)
    , m_DefaultSampleDuration(0)
    , m_DefaultSampleSize(0)
    , m_DefaultSampleFlags(0)
{
    stream.ReadUI32(m_TrackId);
    if (m_Flags & AP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT) {
        stream.ReadUI64(m_BaseDataOffset);
    }
    if (m_Flags & AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT) {
        stream.ReadUI32(m_SampleDescriptionIndex);
    }
    if (m_Flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT) {
        stream.ReadUI32(m_DefaultSampleDuration);
    }
    if (m_Flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT) {
        stream.ReadUI32(m_DefaultSampleSize);
    }
    if (m_Flags & AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT) {
        stream.ReadUI32(m_DefaultSampleFlags);
    }
}
