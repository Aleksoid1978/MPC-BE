/*****************************************************************
|
|    AP4 - DataInfo Atom
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4DataInfoAtom.h"

/*----------------------------------------------------------------------
|       AP4_DataInfoAtom::AP4_DataInfoAtom
+---------------------------------------------------------------------*/

AP4_DataInfoAtom::AP4_DataInfoAtom(Type             type,
                                   AP4_Size         size,
                                   AP4_ByteStream&  stream)
    : AP4_Atom(type)
{
    size -= AP4_ATOM_HEADER_SIZE;
    m_Data.SetDataSize(size);
    stream.Read(m_Data.UseData(), size);    
}
