/*****************************************************************
|
|    AP4 - trex Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4TrexAtom.h"

/*----------------------------------------------------------------------
|       AP4_TrexAtom::AP4_TrexAtom
+---------------------------------------------------------------------*/

AP4_TrexAtom::AP4_TrexAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_TREX, size, true, stream)
    , m_TrackId(0)
    , m_DefaultSampleDescriptionIndex(0)
    , m_DefaultSampleDuration(0)
    , m_DefaultSampleSize(0)
    , m_DefaultSampleFlags(0)
{
    stream.ReadUI32(m_TrackId);
    stream.ReadUI32(m_DefaultSampleDescriptionIndex);
    stream.ReadUI32(m_DefaultSampleDuration);
    stream.ReadUI32(m_DefaultSampleSize);
    stream.ReadUI32(m_DefaultSampleFlags);
}
