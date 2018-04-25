/*****************************************************************
|
|    AP4 - Track Objects
|
|    Copyright 2002 Gilles Boccon-Gibod & Julien Boeuf
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4ByteStream.h"
#include "Ap4HdlrAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4Track.h"
#include "Ap4Utils.h"
#include "Ap4Sample.h"
#include "Ap4DataBuffer.h"
#include "Ap4TrakAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4AtomSampleTable.h"
#include "Ap4SdpAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4MdhdAtom.h"
#include "Ap4ElstAtom.h"

/*----------------------------------------------------------------------
|       AP4_Track::AP4_Track
+---------------------------------------------------------------------*/
AP4_Track::AP4_Track(Type             type,
                     AP4_SampleTable* sample_table,
                     AP4_UI32         track_id, 
                     AP4_UI32         movie_time_scale,
                     AP4_UI32         media_time_scale,
                     AP4_UI64         media_duration,
                     const char*      language,
                     AP4_UI32         width,
                     AP4_UI32         height) :
    m_TrakAtomIsOwned(true),
    m_Type(type),
    m_SampleTable(sample_table),
    m_SampleTableIsOwned(false),
    m_MovieTimeScale(movie_time_scale ? 
                     movie_time_scale : 
                     AP4_TRACK_DEFAULT_MOVIE_TIMESCALE),
    m_MediaTimeScale(media_time_scale)
{
    // compute the default volume value
    unsigned int volume = 0;
    if (type == TYPE_AUDIO) volume = 0x100;

    // compute the handler type and name
    AP4_Atom::Type hdlr_type;
    const char* hdlr_name;
    switch (type) {
        case TYPE_AUDIO:
            hdlr_type = AP4_HANDLER_TYPE_SOUN;
            hdlr_name = "Bento4 Sound Handler";
            break;

        case TYPE_VIDEO:
            hdlr_type = AP4_HANDLER_TYPE_VIDE;
            hdlr_name = "Bento4 Video Handler";
            break;

        case TYPE_HINT:
            hdlr_type = AP4_HANDLER_TYPE_HINT;
            hdlr_name = "Bento4 Hint Handler";
            break;

        default:
            hdlr_type = 0;
            hdlr_name = NULL;
            break;
    }

    // compute the track duration in units of the movie time scale
    AP4_UI64 track_duration = AP4_ConvertTime(media_duration,
                                              media_time_scale,
                                              movie_time_scale);

    // create a trak atom
    m_TrakAtom = new AP4_TrakAtom(sample_table,
                                  hdlr_type, 
                                  hdlr_name,
                                  track_id, 
                                  0, 
                                  0, 
                                  track_duration,
                                  media_time_scale,
                                  media_duration,
                                  volume, 
                                  language,
                                  width, 
                                  height);
}

/*----------------------------------------------------------------------
|       AP4_Track::AP4_Track
+---------------------------------------------------------------------*/
AP4_Track::AP4_Track(AP4_TrakAtom&   atom, 
                     AP4_ByteStream& sample_stream, 
                     AP4_UI32        movie_time_scale) :
    m_TrakAtom(&atom),
    m_TrakAtomIsOwned(false),
    m_Type(TYPE_UNKNOWN),
    m_SampleTable(NULL),
    m_SampleTableIsOwned(true),
    m_MovieTimeScale(movie_time_scale),
    m_MediaTimeScale(0)
{
    // find the handler type
    AP4_Atom* sub = atom.FindChild("mdia/hdlr");
    if (sub) {
        AP4_HdlrAtom* hdlr = dynamic_cast<AP4_HdlrAtom*>(sub);
        if (hdlr) {
            AP4_Atom::Type type = hdlr->GetHandlerType();
            if (type == AP4_HANDLER_TYPE_SOUN) {
                m_Type = TYPE_AUDIO;
            } else if (type == AP4_HANDLER_TYPE_VIDE) {
                m_Type = TYPE_VIDEO;
            } else if (type == AP4_HANDLER_TYPE_TEXT ||
                       type == AP4_HANDLER_TYPE_SBTL ||
                       type == AP4_HANDLER_TYPE_TX3G) {
                m_Type = TYPE_TEXT;
            } else if (type == AP4_HANDLER_TYPE_SUBP) {
                m_Type = TYPE_SUBP;
            } else if (type == AP4_HANDLER_TYPE_HINT) {
                m_Type = TYPE_HINT;
            }
        }
    }

    // get the media time scale
    sub = atom.FindChild("mdia/mdhd");
    if (sub) {
        AP4_MdhdAtom* mdhd = dynamic_cast<AP4_MdhdAtom*>(sub);
        if (mdhd) {
            m_MediaTimeScale = mdhd->GetTimeScale();
        }
    }

    // create a facade for the stbl atom
    AP4_ContainerAtom* stbl = dynamic_cast<AP4_ContainerAtom*>(
        atom.FindChild("mdia/minf/stbl"));
    if (stbl) {
        m_SampleTable = new AP4_AtomSampleTable(stbl, sample_stream);
    }

    AP4_ElstAtom* elst = dynamic_cast<AP4_ElstAtom*>(
        atom.FindChild("edts/elst"));
    if (elst) {
        AP4_UI64 delay = elst->GetDelay();
        AP4_UI64 start = elst->GetStart();
        if ((delay || start) && m_SampleTable) {
            (dynamic_cast<AP4_AtomSampleTable*>(m_SampleTable))->SetTimeDelay(AP4_ConvertTime(delay, m_MovieTimeScale, m_MediaTimeScale) - start);
        }
    }

    if (m_Type == TYPE_VIDEO && stbl) {
        if (AP4_StssAtom* stss = dynamic_cast<AP4_StssAtom*>(stbl->FindChild("stss"))) {
            const AP4_Array<AP4_UI32>& entries = stss->GetEntries();
            if (AP4_SUCCEEDED(m_IndexEntries.EnsureCapacity(entries.ItemCount()))) {
                for (AP4_Cardinal i = 0; i < entries.ItemCount(); ++i) {
                    AP4_UI32 index = entries[i] - 1;

                    AP4_Sample sample;
                    if (AP4_SUCCEEDED(GetSample(index, sample))) {
                        REFERENCE_TIME rt = (REFERENCE_TIME)(10000000.0 / GetMediaTimeScale() * sample.GetCts());
                        if (AP4_FAILED(m_IndexEntries.Append(AP4_IndexTableEntry(index, sample.GetCts(), rt, sample.GetOffset())))) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|       AP4_Track::~AP4_Track
+---------------------------------------------------------------------*/
AP4_Track::~AP4_Track()
{
    if (m_TrakAtomIsOwned) delete m_TrakAtom;
    if (m_SampleTableIsOwned) delete m_SampleTable;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetId
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Track::GetId()
{
    return m_TrakAtom->GetId();
}

/*----------------------------------------------------------------------
|       AP4_Track::SetId
+---------------------------------------------------------------------*/
AP4_Result
AP4_Track::SetId(AP4_UI32 id)
{
    m_TrakAtom->SetId(id);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetDuration
+---------------------------------------------------------------------*/
AP4_UI64
AP4_Track::GetDuration()
{
    return (m_FragmentSampleTable.GetDuration() ? m_FragmentSampleTable.GetDuration() : m_TrakAtom->GetDuration());
}

/*----------------------------------------------------------------------
|       AP4_Track::GetDurationMs
+---------------------------------------------------------------------*/
AP4_Duration
AP4_Track::GetDurationMs()
{
    AP4_UI64 duration = GetDuration();
    return AP4_DurationMsFromUnits(duration, m_FragmentSampleTable.GetDuration() ? m_MediaTimeScale : m_MovieTimeScale);
}

/*----------------------------------------------------------------------
|       AP4_Track::GetDurationHighPrecision
+---------------------------------------------------------------------*/
double
AP4_Track::GetDurationHighPrecision()
{
    const AP4_UI64 duration = GetDuration();
    const AP4_UI32 units_per_second = m_FragmentSampleTable.GetDuration() ? m_MediaTimeScale : m_MovieTimeScale;
    return units_per_second ? (double)duration * 1000.0 / units_per_second : 0.0;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetSampleCount
+---------------------------------------------------------------------*/
AP4_Cardinal
AP4_Track::GetSampleCount()
{
    // delegate to the sample table
    return m_FragmentSampleTable.GetDuration() ? m_FragmentSampleTable.GetSampleCount() : m_SampleTable ? m_SampleTable->GetSampleCount() : 0;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetSample
+---------------------------------------------------------------------*/
AP4_Result 
AP4_Track::GetSample(AP4_Ordinal index, AP4_Sample& sample)
{
    // delegate to the sample table
    return m_FragmentSampleTable.GetDuration() ? m_FragmentSampleTable.GetSample(index, sample) : m_SampleTable ? m_SampleTable->GetSample(index, sample) : AP4_FAILURE;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetSampleDescription
+---------------------------------------------------------------------*/
AP4_SampleDescription*
AP4_Track::GetSampleDescription(AP4_Ordinal index)
{
    // delegate to the sample table
    return m_SampleTable ? m_SampleTable->GetSampleDescription(index) : NULL;
}

/*----------------------------------------------------------------------
|       AP4_Track::ReadSample
+---------------------------------------------------------------------*/
AP4_Result   
AP4_Track::ReadSample(AP4_Ordinal     index, 
                      AP4_Sample&     sample,
                      AP4_DataBuffer& data)
{
    AP4_Result result;

    // get the sample
    result = GetSample(index, sample);
    if (AP4_FAILED(result)) return result;

    // read the data
    return sample.ReadData(data);
}

/*----------------------------------------------------------------------
|       AP4_Track::GetSampleIndexForTimeStampMs
+---------------------------------------------------------------------*/
AP4_Result  
AP4_Track::GetSampleIndexForTimeStampMs(AP4_TimeStamp ts, AP4_Ordinal& index)
{
    // convert the ts in the timescale of the track's media
    ts = AP4_ConvertTime(ts, 1000, m_MediaTimeScale);

    return m_FragmentSampleTable.GetDuration() ? m_FragmentSampleTable.GetSampleIndexForTimeStamp(ts, index) : m_SampleTable->GetSampleIndexForTimeStamp(ts, index);
}

// MPC-BE custom code start
/*----------------------------------------------------------------------
|       AP4_Track::GetSampleIndexForRefTime
+---------------------------------------------------------------------*/
AP4_Result  
AP4_Track::GetSampleIndexForRefTime(REFERENCE_TIME rt, AP4_Ordinal& index)
{
    // MPC-BE custom code start
    //AP4_TimeStamp ts = (AP4_TimeStamp(rt) * m_MediaTimeScale + 5000000) / 10000000;
    AP4_TimeStamp ts = (AP4_TimeStamp)((double(rt) * m_MediaTimeScale + 5000000) / 10000000);
    // need calculate in double, because the (AP4_TimeStamp(rt) * m_MediaTimeScale) can give overflow
    // MPC-BE custom code end
    //AP4_TimeStamp ts = (AP4_TimeStamp)(double(rt) * m_MediaTimeScale / 10000000 + 0.5);

    return m_FragmentSampleTable.GetDuration() ? m_FragmentSampleTable.GetSampleIndexForTimeStamp(ts, index) : m_SampleTable->GetSampleIndexForTimeStamp(ts, index);
}

/*----------------------------------------------------------------------
|       AP4_Track::GetIndexForRefTime
+---------------------------------------------------------------------*/
AP4_Result
AP4_Track::GetIndexForRefTime(REFERENCE_TIME rt, AP4_Ordinal& index, AP4_SI64& cts, AP4_Offset& offset)
{
    if (!m_IndexEntries.ItemCount()) {
        return AP4_FAILURE;
    }

    for (AP4_Ordinal i = 0; i < m_IndexEntries.ItemCount(); i++) {
        if (m_IndexEntries[i].m_rt > rt) {
            const AP4_IndexTableEntry& indexEntry = m_IndexEntries[i ? i - 1 : 0];
            index  = indexEntry.m_index;
            cts    = indexEntry.m_cts;
            offset = indexEntry.m_offset;
            return AP4_SUCCESS;
        }
    }

    const AP4_IndexTableEntry& indexEntry = m_IndexEntries[m_IndexEntries.ItemCount() - 1];
    index  = indexEntry.m_index;
    cts    = indexEntry.m_cts;
    offset = indexEntry.m_offset;

    return AP4_SUCCESS;
}
// MPC-BE custom code end

/*----------------------------------------------------------------------
|       AP4_Track::SetMovieTimeScale
+---------------------------------------------------------------------*/
AP4_Result
AP4_Track::SetMovieTimeScale(AP4_UI32 time_scale)
{
    // check that we can convert
    if (m_MovieTimeScale == 0) return AP4_FAILURE;

    // convert from one time scale to the other
    m_TrakAtom->SetDuration(AP4_ConvertTime(m_TrakAtom->GetDuration(), 
                                            m_MovieTimeScale,
                                            time_scale));
    
    // keep the new movie timescale
    m_MovieTimeScale = time_scale;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetMediaTimeScale
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Track::GetMediaTimeScale()
{
    return m_MediaTimeScale;
}

// save the implementation for later
#if 0 
/*----------------------------------------------------------------------
|       AP4_HintTrack::SetSdpText
+---------------------------------------------------------------------*/
void
AP4_HintTrack::SetSdpText(const char* text)
{
    // build an sdp atom
    AP4_SdpAtom* sdp = new AP4_SdpAtom(text);

    // build the hnti
    AP4_ContainerAtom* hnti = new AP4_ContainerAtom(AP4_ATOM_TYPE_HNTI);
    hnti->AddChild(sdp);

    // check if there's already a user data atom
    AP4_ContainerAtom* udta = dynamic_cast<AP4_ContainerAtom*>(m_TrakAtom->FindChild("udta"));
    if (udta == NULL) {
        // otherwise create it
        udta = new AP4_ContainerAtom(AP4_ATOM_TYPE_UDTA);
        m_TrakAtom->AddChild(udta);
    }
    udta->AddChild(hnti);
}

#endif

/*----------------------------------------------------------------------
|       AP4_Track::GetTrackName
+---------------------------------------------------------------------*/
AP4_String
AP4_Track::GetTrackName()
{
    AP4_String TrackName;
    if(AP4_HdlrAtom* hdlr = dynamic_cast<AP4_HdlrAtom*>(m_TrakAtom->FindChild("mdia/hdlr")))
        TrackName = hdlr->GetHandlerName();
    return TrackName;
}

/*----------------------------------------------------------------------
|       AP4_Track::GetTrackLanguage
+---------------------------------------------------------------------*/
AP4_String
AP4_Track::GetTrackLanguage()
{
    AP4_String TrackLanguage;
    if(AP4_MdhdAtom* mdhd = dynamic_cast<AP4_MdhdAtom*>(m_TrakAtom->FindChild("mdia/mdhd")))
        TrackLanguage = mdhd->GetLanguage().c_str();
    return TrackLanguage;
}

/*----------------------------------------------------------------------
|       AP4_Track::CreateFragmentFromStdSamples
+---------------------------------------------------------------------*/
AP4_Result
AP4_Track::CreateFragmentFromStdSamples()
{
    if (m_SampleTable && m_SampleTable->GetSampleCount() && m_FragmentSampleTable.GetSampleCount() == 0) {
        AP4_Array<AP4_Sample>& samples = m_FragmentSampleTable.GetSampleTable();
        if (AP4_FAILED(samples.SetItemCount(m_SampleTable->GetSampleCount()))) {
            return AP4_FAILURE;
        }

        AP4_Duration duration = m_FragmentSampleTable.GetDuration();
        for (AP4_Cardinal i = 0; i < m_SampleTable->GetSampleCount(); i++) {
            AP4_Sample sample;
            if (AP4_SUCCEEDED(m_SampleTable->GetSample(i, sample))) {
                samples[i] = sample;
                duration += sample.GetDuration();
            }
        }

        if (duration > m_FragmentSampleTable.GetDuration()) {
            m_FragmentSampleTable.SetDuration(duration);
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Track::CreateFragmentFromStdSamples
+---------------------------------------------------------------------*/
AP4_Result
AP4_Track::CreateIndexFromFragment()
{
    m_IndexEntries.Clear();
    for (AP4_Cardinal index = 0; index < m_FragmentSampleTable.GetSampleCount(); index++) {
        AP4_Sample sample;
        if (AP4_SUCCEEDED(m_FragmentSampleTable.GetSample(index, sample)) && sample.IsSync()) {
            REFERENCE_TIME rt = (REFERENCE_TIME)(10000000.0 / GetMediaTimeScale() * sample.GetCts());
            if (AP4_FAILED(m_IndexEntries.Append(AP4_IndexTableEntry(index, sample.GetCts(), rt, sample.GetOffset())))) {
                return AP4_FAILURE;
            }
        }
    }

    return AP4_SUCCESS;
}