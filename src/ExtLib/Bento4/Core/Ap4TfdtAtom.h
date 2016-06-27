/*****************************************************************
|
|    AP4 - tfdt Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_TFDT_ATOM_H_
#define _AP4_TFDT_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|       AP4_TfdtAtom
+---------------------------------------------------------------------*/

class AP4_TfdtAtom : public AP4_Atom
{
public:
    // methods
    AP4_TfdtAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    AP4_UI64 GetBaseMediaDecodeTime() const                      { return m_BaseMediaDecodeTime;                   }
    void SetBaseMediaDecodeTime(AP4_UI64 base_media_decode_time) { m_BaseMediaDecodeTime = base_media_decode_time; }

private:
    // members
    AP4_UI64 m_BaseMediaDecodeTime;
};

#endif // _AP4_TFDT_ATOM_H_
