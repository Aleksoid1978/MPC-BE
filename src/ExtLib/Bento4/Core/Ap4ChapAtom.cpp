/*****************************************************************
|
|    AP4 - chap Atom
|
|    Copyright 2012-2017 Aleksoid1978
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4ChapAtom.h"

/*----------------------------------------------------------------------
|       AP4_ChapAtom::AP4_ChapAtom
+---------------------------------------------------------------------*/

AP4_ChapAtom::AP4_ChapAtom(AP4_Size        size,
                           AP4_ByteStream& stream)
    : AP4_Atom(AP4_ATOM_TYPE_CHAP)
{
    const AP4_Size data_size = size - AP4_ATOM_HEADER_SIZE; // size and atom type
    const AP4_Cardinal numEntries = data_size / 4;
    if (numEntries) {
        m_ChapterTrackEntries.SetItemCount(numEntries);
        for (AP4_Cardinal i = 0; i < numEntries; i++) {
            stream.ReadUI32(m_ChapterTrackEntries[i]);
        }
    }
}
