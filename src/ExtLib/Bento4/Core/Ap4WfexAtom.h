/*****************************************************************
|
|    AP4 - avcC Atom
|
 ****************************************************************/

#ifndef _AP4_WFEX_ATOM_H_
#define _AP4_WFEX_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Types.h"
#include "Ap4Array.h"
#include "Ap4DataBuffer.h"

/*----------------------------------------------------------------------
|       AP4_WfexAtom
+---------------------------------------------------------------------*/
class AP4_WfexAtom : public AP4_Atom
{
public:
    AP4_WfexAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    const AP4_DataBuffer* GetDecoderInfo() const { return &m_DecoderInfo; }

private:
    AP4_DataBuffer m_DecoderInfo;
};

#endif // _AP4_AVCC_ATOM_H_
