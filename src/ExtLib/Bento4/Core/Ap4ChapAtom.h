/*****************************************************************
|
|    AP4 - chap Atom
|
|    Copyright 2012-2017 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_CHAP_ATOM_H_
#define _AP4_CHAP_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|       AP4_ChapAtom
+---------------------------------------------------------------------*/

class AP4_ChapAtom : public AP4_Atom
{
public:
    AP4_ChapAtom(AP4_Size        size,
                 AP4_ByteStream& stream);

    const AP4_Array<AP4_UI32>& GetChapterTrackEntries() { return m_ChapterTrackEntries; }

private:
    AP4_Array<AP4_UI32> m_ChapterTrackEntries;
};

#endif // _AP4_CHAP_ATOM_H_
