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

    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    AP4_UI32 GetNum() const { return m_num; }
    AP4_UI32 GetDen() const { return m_den; }

private:
    AP4_UI32 m_num;
    AP4_UI32 m_den;    
};

#endif // _AP4_PASP_ATOM_H_
