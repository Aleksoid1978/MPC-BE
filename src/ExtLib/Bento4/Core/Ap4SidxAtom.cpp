/*****************************************************************
|
|    AP4 - sidx Atoms
|
|    Copyright 2016 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4SidxAtom.h"

/*----------------------------------------------------------------------
|       AP4_SidxAtom::AP4_SidxAtom
+---------------------------------------------------------------------*/

AP4_SidxAtom::AP4_SidxAtom(AP4_Size        size,
                           AP4_ByteStream& stream)
    : AP4_Atom(AP4_ATOM_TYPE_SIDX, size, true, stream)
{
    AP4_Offset offset = 0;
    stream.Tell(offset);
    offset += size;
    offset -= AP4_FULL_ATOM_HEADER_SIZE;

    stream.ReadUI32(m_ReferenceId);
    stream.ReadUI32(m_TimeScale);
    if (m_Version == 0) {
        AP4_UI32 earliest_presentation_time = 0;
        AP4_UI32 first_offset = 0;
        stream.ReadUI32(earliest_presentation_time);
        stream.ReadUI32(first_offset);
        m_EarliestPresentationTime = earliest_presentation_time;
        m_Offset                   = first_offset;
    } else {
        stream.ReadUI64(m_EarliestPresentationTime);
        stream.ReadUI64(m_Offset);
    }
    m_Offset += offset;
    offset = m_Offset;

    m_Duration = m_EarliestPresentationTime;

    AP4_UI16 reserved = 0;
    stream.ReadUI16(reserved);
    AP4_UI16 fragments_count = 0;
    stream.ReadUI16(fragments_count);

    m_Fragments.SetItemCount(fragments_count);
    for (AP4_UI16 i = 0; i < fragments_count; i++) {
        AP4_UI32 size = 0;
        stream.ReadUI32(size);
        size = size & 0x7FFFFFFF;
        AP4_UI32 duration = 0;
        stream.ReadUI32(duration);

        m_Fragments[i].m_Offset    = offset;
        m_Fragments[i].m_Size      = size;
        m_Fragments[i].m_StartTime = m_Duration;
        m_Fragments[i].m_Duration  = duration;

        AP4_UI32 reserved;
        stream.ReadUI32(reserved); // sap flags

        m_Duration += duration;
        offset += size;
    }
}
