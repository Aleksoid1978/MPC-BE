/*****************************************************************
|
|    AP4 - tfdt Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4TfdtAtom.h"

/*----------------------------------------------------------------------
|       AP4_TfdtAtom::AP4_TfdtAtom
+---------------------------------------------------------------------*/

AP4_TfdtAtom::AP4_TfdtAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_TFDT, size, true, stream)
    , m_BaseMediaDecodeTime(0)
{
    if (m_Version == 0) {
        AP4_UI32 value = 0;
        stream.ReadUI32(value);
        m_BaseMediaDecodeTime = value;
    } else if (m_Version == 1) {
        stream.ReadUI64(m_BaseMediaDecodeTime);
    }
}
