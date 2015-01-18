/*****************************************************************
|
|    AP4 - hvcC Atom
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4HvcCAtom.h"

/*----------------------------------------------------------------------
|       AP4_HvcCAtom::AP4_HvcCAtom
+---------------------------------------------------------------------*/

AP4_HvcCAtom::AP4_HvcCAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_HVCC)
{
    size -= AP4_ATOM_HEADER_SIZE;
    m_DecoderInfo.SetDataSize(size);
    stream.Read(m_DecoderInfo.UseData(), size);
}
