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

#ifndef _AP4_MOVIE_H_
#define _AP4_MOVIE_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4MoovAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4Track.h"
#include "Ap4List.h"
#include "Ap4ByteStream.h"
#include "Ap4SidxAtom.h"

#include <map>

/*----------------------------------------------------------------------
|       AP4_Movie
+---------------------------------------------------------------------*/
class AP4_Movie {
public:
    // methods
    AP4_Movie(AP4_UI32 time_scale = 0);
    AP4_Movie(AP4_MoovAtom* moov, AP4_ByteStream& mdat);
    virtual ~AP4_Movie();
    AP4_Result Inspect(AP4_AtomInspector& inspector);

    AP4_MoovAtom* GetMoovAtom() { return m_MoovAtom;}
    AP4_MvhdAtom* GetMvhdAtom() { return m_MvhdAtom;}
    AP4_List<AP4_Track>& GetTracks() { return m_Tracks; }
    AP4_Track* GetTrack(AP4_UI32 track_id);
    AP4_Track* GetTrack(AP4_Track::Type type, AP4_Ordinal index = 0);
    AP4_Result AddTrack(AP4_Track* track);
    AP4_UI32   GetTimeScale();
    AP4_Duration GetDuration();
    AP4_Duration GetDurationMs();

    bool         HasFragments();
    AP4_Duration GetFragmentsDuration();
    AP4_Duration GetFragmentsDurationMs();

    const bool   HasFragmentsIndex();
    const AP4_Array<AP4_IndexTableEntry>& GetFragmentsIndexEntries();

    void         ProcessMoof(AP4_ContainerAtom* moof, AP4_ByteStream& stream, AP4_Offset offset, AP4_Duration dts = 0, bool bClearSampleTable = false);
    AP4_Result   SetSidxAtoms(std::map<AP4_UI32, AP4_SidxAtom*> sidxAtoms, AP4_ByteStream& stream);

    AP4_Result   SelectMoof(const REFERENCE_TIME rt);
    AP4_Result   SwitchMoof(AP4_UI32 id, AP4_Cardinal index, AP4_UI64 offset, AP4_UI64 size, AP4_Duration dts);
    AP4_Result   SwitchNextMoof();
    AP4_Result   SwitchFirstMoof();

private:
    // members
    AP4_MoovAtom*       m_MoovAtom;
    AP4_MvhdAtom*       m_MvhdAtom;
    AP4_List<AP4_Track> m_Tracks;

    struct fragmentsData {
        AP4_SidxAtom*                  SidxAtom;
        AP4_Cardinal                   CurrentMoof;
        AP4_Array<AP4_IndexTableEntry> FragmentsIndexEntries;
        AP4_Array<AP4_ContainerAtom*>  MoofAtomEntries;
        AP4_Array<AP4_Offset>          MoofOffsetEntries;
    };

    AP4_ByteStream*                   m_Stream;
    std::map<AP4_UI32, fragmentsData> m_fragmentsDataEntries;
};

#endif // _AP4_MOVIE_H_
