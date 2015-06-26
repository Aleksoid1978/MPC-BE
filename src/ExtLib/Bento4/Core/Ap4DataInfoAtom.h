/*****************************************************************
|
|    AP4 - DataInfo Atom
|
 ****************************************************************/

#ifndef _AP4_DATAINFO_ATOM_H_
#define _AP4_DATAINFO_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Types.h"
#include "Ap4Array.h"
#include "Ap4DataBuffer.h"

/*----------------------------------------------------------------------
|       AP4_DataInfoAtom
+---------------------------------------------------------------------*/
class AP4_DataInfoAtom : public AP4_Atom
{
public:
    AP4_DataInfoAtom(Type             type,
                     AP4_Size         size,
                     AP4_ByteStream&  stream);

    AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    const AP4_DataBuffer* GetData() const { return &m_Data; }

private:
    AP4_DataBuffer m_Data;
};

#endif // _AP4_DATAINFO_ATOM_H_
