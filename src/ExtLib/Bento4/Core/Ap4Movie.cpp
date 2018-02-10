/*****************************************************************
|
|    AP4 - Movie
|
|    Copyright 2002-2005 Gilles Boccon-Gibod
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
#include "Ap4File.h"
#include "Ap4Atom.h"
#include "Ap4TfdtAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4MfhdAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Movie.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|       AP4_TrackFinderById
+---------------------------------------------------------------------*/
class AP4_TrackFinderById : public AP4_List<AP4_Track>::Item::Finder
{
public:
    AP4_TrackFinderById(AP4_UI32 track_id) : m_TrackId(track_id) {}
    AP4_Result Test(AP4_Track* track) const {
        return track->GetId() == m_TrackId ? AP4_SUCCESS : AP4_FAILURE;
    }
private:
    AP4_UI32 m_TrackId;
};

/*----------------------------------------------------------------------
|       AP4_TrackFinderByType
+---------------------------------------------------------------------*/
class AP4_TrackFinderByType : public AP4_List<AP4_Track>::Item::Finder
{
public:
    AP4_TrackFinderByType(AP4_Track::Type type, AP4_Ordinal index = 0) : 
      m_Type(type), m_Index(index) {}
    AP4_Result Test(AP4_Track* track) const {
        if (track->GetType() == m_Type && m_Index-- == 0) {
            return AP4_SUCCESS;
        } else {
            return AP4_FAILURE;
        }
    }
private:
    AP4_Track::Type     m_Type;
    mutable AP4_Ordinal m_Index;
};

/*----------------------------------------------------------------------
|       AP4_Movie::AP4_Movie
+---------------------------------------------------------------------*/
AP4_Movie::AP4_Movie(AP4_UI32 time_scale) :
    m_SidxAtom(NULL),
    m_Stream(NULL)
{
    m_MoovAtom = new AP4_MoovAtom();
    m_MvhdAtom = new AP4_MvhdAtom(0, 0, 
                                  time_scale, 
                                  0, 
                                  0x00010000,
                                  0x0100);
    m_MoovAtom->AddChild(m_MvhdAtom);
}

/*----------------------------------------------------------------------
|       AP4_Movie::AP4_Moovie
+---------------------------------------------------------------------*/
AP4_Movie::AP4_Movie(AP4_MoovAtom* moov, AP4_ByteStream& mdat) :
    m_MoovAtom(moov),
    m_SidxAtom(NULL),
    m_Stream(NULL)
{
    // ignore null atoms
    if (moov == NULL) return;

    // get the time scale
    AP4_UI32 time_scale;
    m_MvhdAtom = dynamic_cast<AP4_MvhdAtom*>(moov->GetChild(AP4_ATOM_TYPE_MVHD));
    if (m_MvhdAtom) {
        time_scale = m_MvhdAtom->GetTimeScale();
    } else {
        time_scale = 0;
    }

    // get all tracks
    AP4_List<AP4_TrakAtom>* trak_atoms;
    trak_atoms = &moov->GetTrakAtoms();
    AP4_List<AP4_TrakAtom>::Item* item = trak_atoms->FirstItem();
    while (item) {
        AP4_Track* track = new AP4_Track(*item->GetData(), 
                                         mdat,
                                         time_scale);
        // small hack for tracks with the same numbers
        AP4_UI32 trackId = track->GetId();
        AP4_Track* oldTrack = GetTrack(trackId);
        while (oldTrack) {
            trackId++;
            track->SetId(trackId);
            oldTrack = GetTrack(trackId);
        }
        //

        m_Tracks.Add(track);
        item = item->GetNext();
    }
}
    
/*----------------------------------------------------------------------
|       AP4_Movie::~AP4_Movie
+---------------------------------------------------------------------*/
AP4_Movie::~AP4_Movie()
{
    m_Tracks.DeleteReferences();
    delete m_MoovAtom;

    for (AP4_Cardinal i = 0; i < m_MoofAtomEntries.ItemCount(); i++) {
        if (m_MoofAtomEntries[i]) {
            delete m_MoofAtomEntries[i]; m_MoofAtomEntries[i] = nullptr;
        }
    }

    AP4_RELEASE(m_Stream);
}

/*----------------------------------------------------------------------
|       AP4_Movie::Inspect
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::Inspect(AP4_AtomInspector& inspector)
{
    // dump the moov atom
    return m_MoovAtom->Inspect(inspector);
}

/*----------------------------------------------------------------------
|       AP4_Movie::GetTrack
+---------------------------------------------------------------------*/
AP4_Track*
AP4_Movie::GetTrack(AP4_UI32 track_id)
{
    AP4_Track* track = NULL;
    if (AP4_SUCCEEDED(m_Tracks.Find(AP4_TrackFinderById(track_id), track))) {
        return track;
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|       AP4_Movie::GetTrack
+---------------------------------------------------------------------*/
AP4_Track*
AP4_Movie::GetTrack(AP4_Track::Type track_type, AP4_Ordinal index)
{
    AP4_Track* track = NULL;
    if (AP4_SUCCEEDED(m_Tracks.Find(AP4_TrackFinderByType(track_type, index), track))) {
        return track;
    } else {
        return NULL;
    }
}

/*----------------------------------------------------------------------
|       AP4_Movie::AddTrack
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::AddTrack(AP4_Track* track)
{
    // assign an ID to the track unless it already has one
    if (track->GetId() == 0) {
        track->SetId(m_Tracks.ItemCount()+1);
    }

    // if we don't have a time scale, use the one from the track
    if (m_MvhdAtom->GetTimeScale() == 0) {
        m_MvhdAtom->SetTimeScale(track->GetMediaTimeScale());
    }

    // adjust the parent time scale of the track
    track->SetMovieTimeScale(m_MvhdAtom->GetTimeScale());

    // update the movie duration
    if (m_MvhdAtom->GetDuration() < track->GetDuration()) {
        m_MvhdAtom->SetDuration(track->GetDuration());
    }
    
    // attach the track as a child
    m_MoovAtom->AddChild(track->GetTrakAtom());
    m_Tracks.Add(track);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|       AP4_Movie::GetTimeScale
+---------------------------------------------------------------------*/
AP4_UI32
AP4_Movie::GetTimeScale()
{
    if (m_MvhdAtom) {
        return m_MvhdAtom->GetTimeScale();
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|       AP4_Movie::GetDuration
+---------------------------------------------------------------------*/
AP4_UI64
AP4_Movie::GetDuration()
{
    if (m_MvhdAtom) {
        return m_MvhdAtom->GetDuration();
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|       AP4_Movie::GetDurationMs
+---------------------------------------------------------------------*/
AP4_Duration
AP4_Movie::GetDurationMs()
{
    if (m_MvhdAtom) {
        return m_MvhdAtom->GetDurationMs();
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------
|   AP4_Movie::HasFragments
+---------------------------------------------------------------------*/
bool
AP4_Movie::HasFragments()
{
    if (m_MoovAtom == NULL) return false;
    if (m_MoovAtom->GetChild(AP4_ATOM_TYPE_MVEX)) {
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------
|   AP4_Movie::GetFragmentsDuration
+---------------------------------------------------------------------*/
AP4_Duration
AP4_Movie::GetFragmentsDuration()
{
    if (m_SidxAtom) {
        return m_SidxAtom->GetDuration();
    }
    return 0;
}

/*----------------------------------------------------------------------
|   AP4_Movie::GetFragmentsDurationMs
+---------------------------------------------------------------------*/
AP4_Duration
AP4_Movie::GetFragmentsDurationMs()
{
    if (m_SidxAtom) {
        return AP4_ConvertTime(m_SidxAtom->GetDuration(), m_SidxAtom->GetTimeScale(), 1000);
    }
    return 0;
}

/*----------------------------------------------------------------------
|   AP4_Movie::ProcessMoof
+---------------------------------------------------------------------*/
void
AP4_Movie::ProcessMoof(AP4_ContainerAtom* moof, AP4_ByteStream& stream, AP4_Offset offset, AP4_Duration dts/* = 0*/, bool bClearSampleTable/* = false*/)
{
    if (moof) {
        AP4_Offset moof_offset = offset - moof->GetSize();
        AP4_Offset mdat_payload_offset = offset + AP4_ATOM_HEADER_SIZE;

        AP4_MfhdAtom* mfhd = AP4_DYNAMIC_CAST(AP4_MfhdAtom, moof->GetChild(AP4_ATOM_TYPE_MFHD));
        if (mfhd) {
            for (AP4_List<AP4_Atom>::Item* item = moof->GetChildren().FirstItem();
                                           item;
                                           item = item->GetNext()) {
                AP4_Atom* atom = item->GetData();
                if (atom->GetType() == AP4_ATOM_TYPE_TRAF) {
                    AP4_ContainerAtom* traf = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
                    if (traf) {
                        AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->GetChild(AP4_ATOM_TYPE_TFHD));
                        if (!tfhd) {
                            continue;
                        }
                        AP4_Track* track = GetTrack(tfhd->GetTrackId());
                        if (!track) {
                            continue;
                        }

                        AP4_TfdtAtom* tfdt = AP4_DYNAMIC_CAST(AP4_TfdtAtom, traf->GetChild(AP4_ATOM_TYPE_TFDT));

                        AP4_TrexAtom*      trex = NULL;
                        AP4_ContainerAtom* mvex = AP4_DYNAMIC_CAST(AP4_ContainerAtom, m_MoovAtom->GetChild(AP4_ATOM_TYPE_MVEX));
                        if (mvex) {
                            for (AP4_List<AP4_Atom>::Item* child_item = mvex->GetChildren().FirstItem();
                                                           child_item;
                                                           child_item = child_item->GetNext()) {
                                AP4_Atom* child_atom = child_item->GetData();
                                if (child_atom->GetType() == AP4_ATOM_TYPE_TREX) {
                                    trex = AP4_DYNAMIC_CAST(AP4_TrexAtom, child_atom);
                                    if (trex && trex->GetTrackId() == tfhd->GetTrackId()) break;
                                    trex = NULL;
                                }
                            }
                        }

                        AP4_FragmentSampleTable* sampleTable = track->GetFragmentSampleTable();
                        if (bClearSampleTable) {
                            sampleTable->Clear();
                        }

                        AP4_Cardinal sample_count = 0;
                        for (AP4_List<AP4_Atom>::Item* child_item = traf->GetChildren().FirstItem();
                                                       child_item;
                                                       child_item = child_item->GetNext()) {
                            AP4_Atom* child_atom = child_item->GetData();
                            if (child_atom->GetType() == AP4_ATOM_TYPE_TRUN) {
                                AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, child_atom);
                                if (trun) {
                                    sample_count += trun->GetEntries().ItemCount();
                                }
                            }
                        }

                        if (!sample_count) {
                            return;
                        }

                        if (sampleTable->GetSampleCount() == 0) {
                            track->CreateFragmentFromStdSamples();
                        }

                        if (AP4_FAILED(sampleTable->EnsureCapacity(sample_count + sampleTable->GetSampleCount()))) {
                            return;
                        }

                        AP4_UI64 dts_origin = tfdt ? tfdt->GetBaseMediaDecodeTime() : dts;
                        for (AP4_List<AP4_Atom>::Item* child_item = traf->GetChildren().FirstItem();
                                                       child_item;
                                                       child_item = child_item->GetNext()) {
                            AP4_Atom* child_atom = child_item->GetData();
                            if (child_atom->GetType() == AP4_ATOM_TYPE_TRUN) {
                                AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, child_atom);
                                if (trun) {
                                    sampleTable->AddTrun(trun, tfhd, trex, stream, dts_origin, moof_offset, mdat_payload_offset);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_Movie::SetSidxAtom
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::SetSidxAtom(AP4_SidxAtom* atom, AP4_ByteStream& stream)
{
    if (!atom
            || m_SidxAtom
            || atom->IsEmpty()) {
        return AP4_FAILURE;
    }

    m_CurrentMoof = 0;
    m_Stream = &stream;
    m_Stream->AddReference();
    m_SidxAtom = atom;
 
    AP4_UI32 mediaTimeScale = m_SidxAtom->GetTimeScale();
    AP4_Track* track = GetTrack(m_SidxAtom->GetReferenceId());
    if (track) {
        mediaTimeScale = track->GetMediaTimeScale();
	}

    AP4_Array<AP4_SidxAtom::Fragments>& fragments = m_SidxAtom->GetSampleTable();
    m_FragmentsIndexEntries.SetItemCount(fragments.ItemCount());
    for (AP4_Cardinal i = 0; i < fragments.ItemCount(); i++) {
        AP4_SidxAtom::Fragments& fragment = fragments[i];
        auto convertTime = [&] (AP4_Duration& time) {
            time = AP4_ConvertTime(time, m_SidxAtom->GetTimeScale(), mediaTimeScale);
        };

        convertTime(fragment.m_StartTime);
        convertTime(fragment.m_Duration);

        AP4_IndexTableEntry& entry = m_FragmentsIndexEntries[i];
        entry.m_cts    = (AP4_SI64)fragment.m_StartTime;
        entry.m_index  = i;
        entry.m_offset = fragment.m_Offset;
        entry.m_rt     = (REFERENCE_TIME)(10000000.0 / m_SidxAtom->GetTimeScale() * fragment.m_StartTime);
    }

    m_MoofAtomEntries.SetItemCount(fragments.ItemCount());
    m_MoofOffsetEntries.SetItemCount(fragments.ItemCount());

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Movie::SelectMoof
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::SelectMoof(const REFERENCE_TIME rt)
{
    if (m_SidxAtom) {
        const AP4_TimeStamp ts = (AP4_TimeStamp)((double(rt) * m_SidxAtom->GetTimeScale() + 5000000) / 10000000);
        AP4_Array<AP4_SidxAtom::Fragments>& fragments = m_SidxAtom->GetSampleTable();
        for (AP4_Cardinal index = fragments.ItemCount() - 1; index >= 0; index--) {
            if (fragments[index].m_StartTime <= ts || index == 0) {
                if (m_CurrentMoof == index) {
                    return AP4_SUCCESS;
                }
                if (AP4_SUCCEEDED(SwitchMoof(index, fragments[index].m_Offset, fragments[index].m_Size, fragments[index].m_StartTime))) {
                    m_CurrentMoof = index;
                    return AP4_SUCCESS;
                }
                break;
            }
        }
    }

    return AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_Movie::SwitchMoof
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::SwitchMoof(AP4_Cardinal index, AP4_UI64 offset, AP4_UI64 size, AP4_Duration dts)
{
    if (m_SidxAtom) {
        AP4_Atom* atom = NULL;
        if (m_MoofAtomEntries[index]) {
            atom   = m_MoofAtomEntries[index];
            offset = m_MoofOffsetEntries[index];
        } else {
            m_Stream->Seek(offset);
            if (AP4_SUCCEEDED(AP4_AtomFactory::DefaultFactory.CreateAtomFromStream(*m_Stream, size, atom, NULL))) {
                m_Stream->Tell(offset);
            }
        }
        if (atom && atom->GetType() == AP4_ATOM_TYPE_MOOF) {
            ProcessMoof(AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom),
                        *m_Stream, offset, dts, true);
            
            if (!m_MoofAtomEntries[index]) {
                m_MoofAtomEntries[index]   = AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom);
                m_MoofOffsetEntries[index] = offset;
            }
            return AP4_SUCCESS;
        }

        if (atom) {
            delete atom;
        }
    }

    return AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_Movie::SwitchNextMoof
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::SwitchNextMoof()
{
    if (m_SidxAtom) {
        AP4_Array<AP4_SidxAtom::Fragments>& fragments = m_SidxAtom->GetSampleTable();
        if (m_CurrentMoof >= 0 && m_CurrentMoof < fragments.ItemCount() - 1) {
            AP4_SidxAtom::Fragments& fragment = fragments[m_CurrentMoof + 1];
            if (AP4_SUCCEEDED(SwitchMoof(m_CurrentMoof + 1, fragment.m_Offset, fragment.m_Size, fragment.m_StartTime))) {
                m_CurrentMoof++;
                return AP4_SUCCESS;
            }
        }
    }

    return AP4_FAILURE;
}

/*----------------------------------------------------------------------
|   AP4_Movie::SwitchFirstMoof
+---------------------------------------------------------------------*/
AP4_Result
AP4_Movie::SwitchFirstMoof()
{
    if (m_SidxAtom) {
        AP4_Array<AP4_SidxAtom::Fragments>& fragments = m_SidxAtom->GetSampleTable();
        if (fragments.ItemCount()) {
            AP4_SidxAtom::Fragments& fragment = fragments[0];
            if (AP4_SUCCEEDED(SwitchMoof(0, fragment.m_Offset, fragment.m_Size, fragment.m_StartTime))) {
                return AP4_SUCCESS;
            }
        }
    }

    return AP4_FAILURE;
}