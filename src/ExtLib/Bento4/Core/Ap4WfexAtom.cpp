/*****************************************************************
|
|    AP4 - avcC Atom
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4WfexAtom.h"

/*----------------------------------------------------------------------
|       AP4_WfexAtom::AP4_WfexAtom
+---------------------------------------------------------------------*/

AP4_WfexAtom::AP4_WfexAtom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_WFEX)
{
    size -= AP4_ATOM_HEADER_SIZE;
    m_DecoderInfo.SetDataSize(size);
    stream.Read(m_DecoderInfo.UseData(), size);    
}
