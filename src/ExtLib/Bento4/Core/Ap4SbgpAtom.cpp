/*****************************************************************
|
|    AP4 - sbgp Atoms
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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
|   includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4SbgpAtom.h"

/*----------------------------------------------------------------------
|   AP4_SbgpAtom::AP4_SbgpAtom
+---------------------------------------------------------------------*/
AP4_SbgpAtom::AP4_SbgpAtom(AP4_Size size, AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_SBGP, size, true, stream),
    m_GroupingType(0),
    m_GroupingTypeParameter(0)
{
    AP4_UI32 remains = size - GetHeaderSize();
    stream.ReadUI32(m_GroupingType);
    remains -= 4;
    if (m_Version >= 1) {
        stream.ReadUI32(m_GroupingTypeParameter);
        remains -= 4;
    }
    AP4_UI32 entry_count = 0;
    AP4_Result result = stream.ReadUI32(entry_count);
    if (AP4_FAILED(result)) return;
    remains -= 4;
    if (remains < entry_count * 8) {
        return;
    }
    m_Entries.SetItemCount(entry_count);
    for (AP4_UI32 i = 0; i < entry_count; i++) {
        Entry entry;
        stream.ReadUI32(entry.sample_count);
        stream.ReadUI32(entry.group_description_index);
        m_Entries[i] = entry;
    }
}
