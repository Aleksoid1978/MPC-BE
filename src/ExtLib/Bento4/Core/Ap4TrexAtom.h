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

    AP4_UI32 GetTrackId() const                       { return m_TrackId;                       }
    AP4_UI32 GetDefaultSampleDescriptionIndex() const { return m_DefaultSampleDescriptionIndex; }
    AP4_UI32 GetDefaultSampleDuration() const         { return m_DefaultSampleDuration;         }
    AP4_UI32 GetDefaultSampleSize() const             { return m_DefaultSampleSize;             }
    AP4_UI32 GetDefaultSampleFlags() const            { return m_DefaultSampleFlags;            }

private:
    // members
    AP4_UI32 m_TrackId;
    AP4_UI32 m_DefaultSampleDescriptionIndex;
    AP4_UI32 m_DefaultSampleDuration;
    AP4_UI32 m_DefaultSampleSize;
    AP4_UI32 m_DefaultSampleFlags;
};

#endif // _AP4_TREX_ATOM_H_
