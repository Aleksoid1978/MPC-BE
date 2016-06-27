/*****************************************************************
|
|    AP4 - mfhd Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_MFHD_ATOM_H_
#define _AP4_MFHD_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|       AP4_MfhdAtom
+---------------------------------------------------------------------*/

class AP4_MfhdAtom : public AP4_Atom
{
public:
    // methods
    AP4_MfhdAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    AP4_UI32 GetSequenceNumber() const                   { return m_SequenceNumber;            }
    void     SetSequenceNumber(AP4_UI32 sequence_number) { m_SequenceNumber = sequence_number; }

private:
    // members
    AP4_UI32 m_SequenceNumber;
};

#endif // _AP4_MFHD_ATOM_H_
