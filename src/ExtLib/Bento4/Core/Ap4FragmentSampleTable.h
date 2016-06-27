/*****************************************************************
|
|    AP4 - Fragment Sample Table
|
|    Copyright 2014-2016 Aleksoid1978
|
 ****************************************************************/
#ifndef _AP4_FRAGMENTSAMPLE_TABLE_H_
#define _AP4_FRAGMENTSAMPLE_TABLE_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4Sample.h"
#include "Ap4Array.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4TrunAtom.h"

/*----------------------------------------------------------------------
|       AP4_FragmentSampleTable
+---------------------------------------------------------------------*/
class AP4_FragmentSampleTable {
public:
    // constructor
    AP4_FragmentSampleTable();

    // methods
    AP4_Result             AddTrun(AP4_TrunAtom* trun,
                                   AP4_TfhdAtom* tfhd,
                                   AP4_TrexAtom* trex,
                                   AP4_ByteStream& stream,
                                   AP4_UI64& dts_origin,
                                   AP4_Offset moof_offset,
                                   AP4_Offset& mdat_payload_offset);
    AP4_Result             GetSample(AP4_Ordinal index, AP4_Sample& sample);
    AP4_Cardinal           GetSampleCount()                          { return m_FragmentSamples.ItemCount(); }
    AP4_Duration           GetDuration()                             { return m_Duration; }
    void                   SetDuration(AP4_Duration duration)        { m_Duration = duration; }
    AP4_Array<AP4_Sample>& GetSampleTable()                          { return m_FragmentSamples; }
    AP4_Result             EnsureCapacity(AP4_Cardinal sample_count) { return m_FragmentSamples.EnsureCapacity(sample_count); }
    AP4_Result             GetSampleIndexForTimeStamp(AP4_SI64 ts,
                                                      AP4_Ordinal& index);
    void                   Clear();

private:
    // members
    AP4_Array<AP4_Sample> m_FragmentSamples;
    AP4_Duration          m_Duration;
};

#endif // _AP4_FRAGMENTSAMPLE_TABLE_H_
