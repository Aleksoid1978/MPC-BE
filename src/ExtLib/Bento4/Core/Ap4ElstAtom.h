/*****************************************************************
|
|    AP4 - elst Atom
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_ELST_ATOM_H_
#define _AP4_ELST_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|       AP4_ElstAtom
+---------------------------------------------------------------------*/

class AP4_ElstEntry {
public:
    AP4_ElstEntry(AP4_UI64 segment_duration = 0, AP4_UI64 media_time = 0, AP4_UI16 media_rate = 1) :
        m_SegmentDuration(segment_duration),
        m_MediaTime(media_time),
        m_MediaRate(media_rate) {}

    AP4_UI64 m_SegmentDuration;
    AP4_UI64 m_MediaTime;
    AP4_UI16 m_MediaRate;
};

class AP4_ElstAtom : public AP4_Atom
{
public:
    AP4_ElstAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    AP4_Array<AP4_ElstEntry>& GetList() { return m_Entries; }
    AP4_UI64 GetDelay() const { return m_Delay; }
    AP4_UI64 GetStart() const { return m_Start; }

private:
    AP4_Array<AP4_ElstEntry>    m_Entries;
    AP4_UI64                    m_Delay;
    AP4_UI64                    m_Start;
};

#endif // _AP4_ELST_ATOM_H_
