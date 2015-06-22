/*****************************************************************
|
|    AP4 - Dvc1 Atom
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/

#include "Ap4.h"
#include "Ap4Dvc1Atom.h"

/*----------------------------------------------------------------------
|       AP4_Dvc1Atom::AP4_Dvc1Atom
+---------------------------------------------------------------------*/

AP4_Dvc1Atom::AP4_Dvc1Atom(AP4_Size         size,
                           AP4_ByteStream&  stream)
    : AP4_Atom(AP4_ATOM_TYPE_DVC1)
{
    size -= AP4_ATOM_HEADER_SIZE;
    stream.ReadUI08(m_ProfileLevel);
    size -= 1;
    AP4_UI08 Reserved[6];
    stream.Read(Reserved, sizeof(Reserved), NULL);
    size -= 6;

    m_DecoderInfo.SetDataSize(size);
    stream.Read(m_DecoderInfo.UseData(), size);    
}
