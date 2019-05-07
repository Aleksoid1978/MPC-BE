/*****************************************************************
|
|    AP4 - pasp Atom
|
|    Copyright 2011 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_PASP_ATOM_H_
#define _AP4_PASP_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|       AP4_PaspAtom
+---------------------------------------------------------------------*/

class AP4_PaspAtom : public AP4_Atom
{
public:
    AP4_PaspAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    AP4_UI32 GetHSpacing() const { return m_hSpacing; }
    AP4_UI32 GetVSpacing() const { return m_vSpacing; }

private:
    AP4_UI32 m_hSpacing;
    AP4_UI32 m_vSpacing;
};

#endif // _AP4_PASP_ATOM_H_
