/*****************************************************************
|
|    AP4 - sidx Atoms
|
|    Copyright 2016 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_SIDX_ATOM_H_
#define _AP4_SIDX_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|       AP4_SidxAtom
+---------------------------------------------------------------------*/

class AP4_SidxAtom : public AP4_Atom
{
public:
    // types
    struct Fragments {
        AP4_UI64 m_Offset        = 0;
        AP4_UI32 m_Size          = 0;
        AP4_Duration m_StartTime = 0;
        AP4_Duration m_Duration  = 0;
    };

    // methods
    AP4_SidxAtom(AP4_Size        size,
                 AP4_ByteStream& stream);
    AP4_UI32              GetReferenceId() const { return m_ReferenceId; }
    AP4_UI32              GetTimeScale()   const { return m_TimeScale;   }
    AP4_UI64              GetOffset()      const { return m_Offset;      }
    AP4_UI64              GetSegmentEnd()  const { return m_SegmentEnd;  }
    AP4_Duration          GetDuration()    const { return m_Duration;    }
    bool                  IsLastSegment()  const { return m_LastSegment; }

    AP4_Array<Fragments>& GetSampleTable()       { return m_Fragments;   }
    bool                  IsEmpty()        const { return m_Fragments.ItemCount() == 0; }

    AP4_Result            Append(AP4_SidxAtom* atom);

private:
    // members
    AP4_UI32             m_ReferenceId              = 0;
    AP4_UI32             m_TimeScale                = 0;
    AP4_UI64             m_EarliestPresentationTime = 0;
    AP4_UI64             m_Offset                   = 0;
    AP4_UI64             m_SegmentEnd               = 0;
    AP4_Duration         m_Duration                 = 0;
    bool                 m_LastSegment              = false;
    AP4_Array<Fragments> m_Fragments;
};

#endif // _AP4_SIDX_ATOM_H_
