/*****************************************************************
|
|    AP4 - elst Atom
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4ElstAtom.h"

/*----------------------------------------------------------------------
|       AP4_ElstAtom::AP4_ElstAtom
+---------------------------------------------------------------------*/

AP4_ElstAtom::AP4_ElstAtom(AP4_Size size, AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_ELST, size, true, stream)
    , m_Delay(0)
    , m_Start(0)
{
    AP4_UI32 edit_start_index = 0;
    AP4_UI64 empty_duration = 0;
    AP4_UI32 entry_count;
    stream.ReadUI32(entry_count);
    m_Entries.EnsureCapacity(entry_count);
    for (AP4_UI32 i = 0; i < entry_count; i++) {
        AP4_UI64 segment_duration = 0;
        AP4_UI64 media_time = 0;
        AP4_UI16 media_rate = 0;
        AP4_UI16 zero = 0;

        if (m_Version == 0) {
            AP4_UI32 value;
            stream.ReadUI32(value);
            segment_duration = value;
            stream.ReadUI32(value);
            media_time = (AP4_SI32)value;
            stream.ReadUI16(media_rate);
            stream.ReadUI16(zero);
        } else {
            stream.ReadUI64(segment_duration);
            stream.ReadUI64(media_time);
            stream.ReadUI16(media_rate);
            stream.ReadUI16(zero);
        }
        m_Entries.Append(AP4_ElstEntry(segment_duration, media_time, media_rate));

        if (i == 0 && media_time == -1) {
            m_Delay = segment_duration;
            edit_start_index = 1;
        } else if (i == edit_start_index && media_time >= 0) {
            m_Start = media_time;
        }
    }
}
