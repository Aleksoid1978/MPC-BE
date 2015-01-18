/*****************************************************************
|
|    AP4 - tfhd Atoms
|
|    Copyright 2014 Aleksoid1978
|
 ****************************************************************/

#ifndef _AP4_TFHD_ATOM_H_
#define _AP4_TFHD_ATOM_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_TFHD_FLAG_BASE_DATA_OFFSET_PRESENT         = 0x00001;
const AP4_UI32 AP4_TFHD_FLAG_SAMPLE_DESCRIPTION_INDEX_PRESENT = 0x00002;
const AP4_UI32 AP4_TFHD_FLAG_DEFAULT_SAMPLE_DURATION_PRESENT  = 0x00008;
const AP4_UI32 AP4_TFHD_FLAG_DEFAULT_SAMPLE_SIZE_PRESENT      = 0x00010;
const AP4_UI32 AP4_TFHD_FLAG_DEFAULT_SAMPLE_FLAGS_PRESENT     = 0x00020;
const AP4_UI32 AP4_TFHD_FLAG_DURATION_IS_EMPTY                = 0x10000;
const AP4_UI32 AP4_TFHD_FLAG_DEFAULT_BASE_IS_MOOF             = 0x20000;

/*----------------------------------------------------------------------
|       AP4_TfhdAtom
+---------------------------------------------------------------------*/

class AP4_TfhdAtom : public AP4_Atom
{
public:
    // methods
    AP4_TfhdAtom(AP4_Size         size,
                 AP4_ByteStream&  stream);

    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector) { return AP4_FAILURE; }
    virtual AP4_Result WriteFields(AP4_ByteStream& stream) { return AP4_FAILURE; }

    AP4_UI32 GetTrackId()                                { return m_TrackId;                   }
    void     SetTrackId(AP4_UI32 track_id)               { m_TrackId = track_id;               }
    AP4_UI64 GetBaseDataOffset()                         { return m_BaseDataOffset;            }
    void     SetBaseDataOffset(AP4_UI64 offset)          { m_BaseDataOffset = offset;          }
    AP4_UI32 GetSampleDescriptionIndex()                 { return m_SampleDescriptionIndex;    }
    void     SetSampleDescriptionIndex(AP4_UI32 indx)    { m_SampleDescriptionIndex = indx;    }
    AP4_UI32 GetDefaultSampleDuration()                  { return m_DefaultSampleDuration;     }
    void     SetDefaultSampleDuration(AP4_UI32 duration) { m_DefaultSampleDuration = duration; }
    AP4_UI32 GetDefaultSampleSize()                      { return m_DefaultSampleSize;         }
    void     SetDefaultSampleSize(AP4_UI32 size)         { m_DefaultSampleSize = size;         }
    AP4_UI32 GetDefaultSampleFlags()                     { return m_DefaultSampleFlags;        }
    void     SetDefaultSampleFlags(AP4_UI32 flags)       { m_DefaultSampleFlags = flags;       }

private:
    // members
    AP4_UI32 m_TrackId;
    AP4_UI64 m_BaseDataOffset;
    AP4_UI32 m_SampleDescriptionIndex;
    AP4_UI32 m_DefaultSampleDuration;
    AP4_UI32 m_DefaultSampleSize;
    AP4_UI32 m_DefaultSampleFlags;
};

#endif // _AP4_TFHD_ATOM_H_
