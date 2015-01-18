/*****************************************************************
|
|    AP4 - trun Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_TRUN_ATOM_H_
#define _AP4_TRUN_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_TRUN_FLAG_DATA_OFFSET_PRESENT                    = 0x0001;
const AP4_UI32 AP4_TRUN_FLAG_FIRST_SAMPLE_FLAGS_PRESENT             = 0x0004;
const AP4_UI32 AP4_TRUN_FLAG_SAMPLE_DURATION_PRESENT                = 0x0100;
const AP4_UI32 AP4_TRUN_FLAG_SAMPLE_SIZE_PRESENT                    = 0x0200;
const AP4_UI32 AP4_TRUN_FLAG_SAMPLE_FLAGS_PRESENT                   = 0x0400;
const AP4_UI32 AP4_TRUN_FLAG_SAMPLE_COMPOSITION_TIME_OFFSET_PRESENT = 0x0800;

/*----------------------------------------------------------------------
|       AP4_TrunAtom
+---------------------------------------------------------------------*/

class AP4_TrunAtom : public AP4_Atom
{
public:

    // types
    struct Entry {
        Entry() : sample_duration(0), sample_size(0), sample_flags(0), sample_composition_time_offset(0) {}
        AP4_UI32 sample_duration;
        AP4_UI32 sample_size;
        AP4_UI32 sample_flags;
        AP4_UI32 sample_composition_time_offset;
    };

    static AP4_UI32 ComputeOptionalFieldsCount(AP4_UI32 flags);
    static AP4_UI32 ComputeRecordFieldsCount(AP4_UI32 flags);

    // methods
    AP4_TrunAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    AP4_SI32                GetDataOffset()                { return m_DataOffset;       }
    void                    SetDataOffset(AP4_SI32 offset) { m_DataOffset = offset;     }
    AP4_UI32                GetFirstSampleFlags()          { return m_FirstSampleFlags; }
    const AP4_Array<Entry>& GetEntries()                   { return m_Entries;          }
    AP4_Array<Entry>&       UseEntries()                   { return m_Entries;          }
    AP4_Result              SetEntries(const AP4_Array<Entry>& entries);

private:
    // members
    AP4_SI32         m_DataOffset;
    AP4_UI32         m_FirstSampleFlags;
    AP4_Array<Entry> m_Entries;
};

#endif // _AP4_TRUN_ATOM_H_
