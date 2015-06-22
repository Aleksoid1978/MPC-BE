/*****************************************************************
|
|    AP4 - Dvc1 Atom
|
 ****************************************************************/

#ifndef _AP4_DVC1_ATOM_H_
#define _AP4_DVC1_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Types.h"
#include "Ap4Array.h"
#include "Ap4DataBuffer.h"

/*----------------------------------------------------------------------
|       AP4_Dvc1Atom
+---------------------------------------------------------------------*/
class AP4_Dvc1Atom : public AP4_Atom
{
public:
    AP4_Dvc1Atom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    const AP4_DataBuffer* GetDecoderInfo() const { return &m_DecoderInfo; }

private:
    AP4_DataBuffer m_DecoderInfo;
    AP4_UI08       m_ProfileLevel;
};

#endif // _AP4_AVCC_ATOM_H_
