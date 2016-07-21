/*****************************************************************
|
|    AP4 - File
|
|    Copyright 2002 Gilles Boccon-Gibod
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
#include "Ap4AtomFactory.h"
#include "Ap4MoovAtom.h"
#include "Ap4SidxAtom.h"
#include <Windows.h>

#define TIMEOUT 5000

/*----------------------------------------------------------------------
|       AP4_File::AP4_File
+---------------------------------------------------------------------*/
AP4_File::AP4_File(AP4_Movie* movie) :
    m_Movie(movie)
{
}

/*----------------------------------------------------------------------
|       AP4_File::AP4_File
+---------------------------------------------------------------------*/
AP4_File::AP4_File(AP4_ByteStream& stream, bool bURL, AP4_AtomFactory& atom_factory)
    : m_Movie(NULL)
    , m_FileType(NULL)
{
    AP4_SidxAtom* sidxAtom = NULL;
    bool bBreak = false;
    const ULONGLONG start = GetTickCount64();
    // get all atoms
    AP4_Atom* atom;
    while (!bBreak && AP4_SUCCEEDED(atom_factory.CreateAtomFromStream(stream, atom))) {
        switch (atom->GetType()) {
            case AP4_ATOM_TYPE_MOOV:
                m_Movie = new AP4_Movie(AP4_DYNAMIC_CAST(AP4_MoovAtom, atom),
                                        stream);
                break;
            case AP4_ATOM_TYPE_MOOF:
                if (m_Movie && !sidxAtom) {
                    AP4_Offset offset;
                    stream.Tell(offset);
                    m_Movie->ProcessMoof(AP4_DYNAMIC_CAST(AP4_ContainerAtom, atom),
                                         stream, offset);

                    if (bURL && (GetTickCount64() - start >= TIMEOUT)) {
                        bBreak = true;
                    }
                }

                delete atom;
                break;
            case AP4_ATOM_TYPE_FTYP:
                m_FileType = AP4_DYNAMIC_CAST(AP4_FtypAtom, atom);
                m_OtherAtoms.Add(atom);
                break;
            case AP4_ATOM_TYPE_SIDX:
                if (!sidxAtom) {
                    sidxAtom = AP4_DYNAMIC_CAST(AP4_SidxAtom, atom);
                    m_OtherAtoms.Add(atom);
                } else {
                    sidxAtom->Append(AP4_DYNAMIC_CAST(AP4_SidxAtom, atom));
                    delete atom;
                }

                if (sidxAtom->IsLastSegment() || AP4_FAILED(stream.Seek(sidxAtom->GetSegmentEnd()))) {
                    bBreak = true;
                }

                if (bURL && (GetTickCount64() - start >= TIMEOUT)) {
                    bBreak = true;
                }
                break;
            default:
                m_OtherAtoms.Add(atom);
        }
    }

    if (m_Movie && AP4_SUCCEEDED(m_Movie->SetSidxAtom(sidxAtom, stream))) {
        m_Movie->SwitchFirstMoof();
        return;
    }

    if (m_Movie && m_Movie->HasFragments()) {
        for (AP4_List<AP4_Track>::Item* item = m_Movie->GetTracks().FirstItem();
                item;
                item = item->GetNext()) {
            AP4_Track* track = item->GetData();
            if (track->GetType() == AP4_Track::TYPE_VIDEO) {
                track->CreateIndexFromFragment();
                if (track->HasIndex()) {
                    break;
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|       AP4_File::~AP4_File
+---------------------------------------------------------------------*/
AP4_File::~AP4_File()
{
    delete m_Movie;
    m_OtherAtoms.DeleteReferences();
}

/*----------------------------------------------------------------------
|       AP4_File::Inspect
+---------------------------------------------------------------------*/
AP4_Result
AP4_File::Inspect(AP4_AtomInspector& inspector)
{
    // dump the moov atom first
    if (m_Movie) m_Movie->Inspect(inspector);

    // dump the other atoms
    m_OtherAtoms.Apply(AP4_AtomListInspector(inspector));

    return AP4_SUCCESS;
}
