/*****************************************************************
|
|    AP4 - trex Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_TREX_ATOM_H_
#define _AP4_TREX_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|       AP4_TrexAtom
+---------------------------------------------------------------------*/

class AP4_TrexAtom : public AP4_Atom
{
public:
    // methods
    AP4_TrexAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    AP4_UI32 GetTrackId()                       { return m_TrackId;                       }
    AP4_UI32 GetDefaultSampleDescriptionIndex() { return m_DefaultSampleDescriptionIndex; }
    AP4_UI32 GetDefaultSampleDuration()         { return m_DefaultSampleDuration;         }
    AP4_UI32 GetDefaultSampleSize()             { return m_DefaultSampleSize;             }
    AP4_UI32 GetDefaultSampleFlags()            { return m_DefaultSampleFlags;            }

private:
    // members
    AP4_UI32 m_TrackId;
    AP4_UI32 m_DefaultSampleDescriptionIndex;
    AP4_UI32 m_DefaultSampleDuration;
    AP4_UI32 m_DefaultSampleSize;
    AP4_UI32 m_DefaultSampleFlags;
};

#endif // _AP4_TREX_ATOM_H_
