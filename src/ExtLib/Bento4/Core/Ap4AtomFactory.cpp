/*****************************************************************
|
|    AP4 - Atom Factory
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
#include "Ap4AtomFactory.h"
#include "Ap4SampleEntry.h"
#include "Ap4IsmaCryp.h"
#include "Ap4UrlAtom.h"
#include "Ap4MoovAtom.h"
#include "Ap4MfhdAtom.h"
#include "Ap4MvhdAtom.h"
#include "Ap4TrakAtom.h"
#include "Ap4HdlrAtom.h"
#include "Ap4DrefAtom.h"
#include "Ap4TkhdAtom.h"
#include "Ap4MdhdAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4StscAtom.h"
#include "Ap4StcoAtom.h"
#include "Ap4Co64Atom.h"
#include "Ap4StszAtom.h"
#include "Ap4EsdsAtom.h"
#include "Ap4SttsAtom.h"
#include "Ap4CttsAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4FtypAtom.h"
#include "Ap4VmhdAtom.h"
#include "Ap4SmhdAtom.h"
#include "Ap4NmhdAtom.h"
#include "Ap4HmhdAtom.h"
#include "Ap4SchmAtom.h"
#include "Ap4FrmaAtom.h"
#include "Ap4TimsAtom.h"
#include "Ap4TfdtAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrexAtom.h"
#include "Ap4TrunAtom.h"
#include "Ap4SidxAtom.h"
#include "Ap4RtpAtom.h"
#include "Ap4SdpAtom.h"
#include "Ap4IkmsAtom.h"
#include "Ap4IsfmAtom.h"
#include "Ap4TrefTypeAtom.h"
#include "Ap4FtabAtom.h"
#include "Ap4ChplAtom.h"
#include "Ap4DataAtom.h"
#include "Ap4DcomAtom.h"
#include "Ap4CmvdAtom.h"
#include "Ap4EndaAtom.h"
#include "Ap4PaspAtom.h"
#include "Ap4ChapAtom.h"
#include "Ap4ElstAtom.h"
#include "Ap4DataInfoAtom.h"
#include "Ap4Dvc1Atom.h"
#include "Ap4Utils.h"
/*----------------------------------------------------------------------
|       class variables
+---------------------------------------------------------------------*/
AP4_AtomFactory AP4_AtomFactory::DefaultFactory;

/*----------------------------------------------------------------------
|       AP4_AtomFactory::AddTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::AddTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Add(handler);
}

/*----------------------------------------------------------------------
|       AP4_AtomFactory::RemoveTypeHandler
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::RemoveTypeHandler(TypeHandler* handler)
{
    return m_TypeHandlers.Remove(handler);
}

/*----------------------------------------------------------------------
|       AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_Atom*&      atom)
{
    AP4_Size bytes_available = 0;
    if (AP4_FAILED(stream.GetSize(bytes_available)) ||
        bytes_available == 0) {
        bytes_available = (AP4_Size)((unsigned long)(-1));
    }
    return CreateAtomFromStream(stream, bytes_available, atom, NULL);
}

/*----------------------------------------------------------------------
|       AP4_AtomFactory::CreateAtomFromStream
+---------------------------------------------------------------------*/
AP4_Result
AP4_AtomFactory::CreateAtomFromStream(AP4_ByteStream& stream, 
                                      AP4_Size&       bytes_available,
                                      AP4_Atom*&      atom,
                                      AP4_Atom*          parent)
{
    AP4_Result result;

    // NULL by default
    atom = NULL;

    // check that there are enough bytes for at least a header
    if (bytes_available < 8) return AP4_ERROR_EOS;

    // remember current stream offset
    AP4_Offset start;
    stream.Tell(start);

    // read atom size

    AP4_UI64 size = 0;
    result = stream.ReadUI32((AP4_UI32&)size);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }

    if (size == 0) {
        // atom extends to end of file
        AP4_Size streamSize = 0;
        stream.GetSize(streamSize);
        if (streamSize >= start) {
            size = streamSize - start;
        }
    }

    // check the size (we don't handle extended size yet)
    if (size != 1 && size < 8 || size > bytes_available) {
        stream.Seek(start);
        return AP4_ERROR_INVALID_FORMAT;
    }

    // read atom type
    AP4_Atom::Type type;
    result = stream.ReadUI32(type);
    if (AP4_FAILED(result)) {
        stream.Seek(start);
        return result;
    }

    if (size == 1)
    {
        AP4_UI64 size_high;

        result = stream.ReadUI64(size_high);
        if (AP4_FAILED(result) ) {
            stream.Seek(start);
            return AP4_ERROR_INVALID_FORMAT;
        }
        size = size_high;
    }
    
    // create the atom
    switch (type) {
        case AP4_ATOM_TYPE_MOOV:
            atom = new AP4_MoovAtom(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_MVHD:
            atom = new AP4_MvhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_TRAK:
            atom = new AP4_TrakAtom(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_HDLR:
            atom = new AP4_HdlrAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_DREF:
            atom = new AP4_DrefAtom(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_URL:
            atom = new AP4_UrlAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_TKHD:
            atom = new AP4_TkhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_MDHD:
            atom = new AP4_MdhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_STSD:
            atom = new AP4_StsdAtom(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_STSC:
            atom = new AP4_StscAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_STCO:
            atom = new AP4_StcoAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_CO64:
            atom = new AP4_Co64Atom(size, stream);
            break;

        case AP4_ATOM_TYPE_STSZ:
            atom = new AP4_StszAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_STTS:
            atom = new AP4_SttsAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_CTTS:
            atom = new AP4_CttsAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_STSS:
            atom = new AP4_StssAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_MP4S:
            atom = new AP4_Mp4sSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_MP4A:
            atom = parent && parent->GetType() == AP4_ATOM_TYPE_STSD 
                ? (AP4_Atom*)new AP4_Mp4aSampleEntry(size, stream, *this)
                : (AP4_Atom*)new AP4_UnknownAtom(type, size, false, stream);
            break;

        case AP4_ATOM_TYPE_MP4V:
            atom = new AP4_Mp4vSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_AVC1:
            atom = new AP4_Avc1SampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_HVC1:
        case AP4_ATOM_TYPE_HEV1:
            atom = new AP4_Hvc1SampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_ENCA:
            atom = new AP4_EncaSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_ENCV:
            atom = new AP4_EncvSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_ESDS:
            atom = new AP4_EsdsAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_VMHD:
            atom = new AP4_VmhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_SMHD:
            atom = new AP4_SmhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_NMHD:
            atom = new AP4_NmhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_HMHD:
            atom = new AP4_HmhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_FRMA:
            atom = new AP4_FrmaAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_SCHM:
            atom = new AP4_SchmAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_FTYP:
            atom = new AP4_FtypAtom(size, stream);
            break;
          
        case AP4_ATOM_TYPE_RTP:
            if (m_Context == AP4_ATOM_TYPE_HNTI) {
                atom = new AP4_RtpAtom(size, stream);
            } else {
                atom = new AP4_RtpHintSampleEntry(size, stream, *this);
            }
            break;

        case AP4_ATOM_TYPE_TIMS:
            atom = new AP4_TimsAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_SDP:
            atom = new AP4_SdpAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_IKMS:
            atom = new AP4_IkmsAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_ISFM:
            atom = new AP4_IsfmAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_HINT:
            atom = new AP4_TrefTypeAtom(type, size, stream);
            break;

        // container atoms
        case AP4_ATOM_TYPE_TREF:
        case AP4_ATOM_TYPE_HNTI:
        case AP4_ATOM_TYPE_STBL:
        case AP4_ATOM_TYPE_MDIA:
        case AP4_ATOM_TYPE_DINF:
        case AP4_ATOM_TYPE_MINF:
        case AP4_ATOM_TYPE_SCHI:
        case AP4_ATOM_TYPE_SINF:
        case AP4_ATOM_TYPE_UDTA:
        case AP4_ATOM_TYPE_ILST:
        case AP4_ATOM_TYPE_NAM:
        case AP4_ATOM_TYPE_ART:
        case AP4_ATOM_TYPE_WRT:
        case AP4_ATOM_TYPE_ALB:
        case AP4_ATOM_TYPE_DAY:
        case AP4_ATOM_TYPE_DESC:
        case AP4_ATOM_TYPE_CPRT:
        case AP4_ATOM_TYPE_TOO:
        case AP4_ATOM_TYPE_CMT:
        case AP4_ATOM_TYPE_GEN:
        case AP4_ATOM_TYPE_TRKN:
        case AP4_ATOM_TYPE_EDTS:
        case AP4_ATOM_TYPE_WAVE: 
        case AP4_ATOM_TYPE_CMOV:
        case AP4_ATOM_TYPE_COVR: {
            AP4_UI32 context = m_Context;
            m_Context = type; // set the context for the children
            atom = new AP4_ContainerAtom(type, size, false, stream, *this);
            m_Context = context; // restore the previous context
            break;
        }

        case AP4_ATOM_TYPE_TREX:
            atom = new AP4_TrexAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_MOOF:
        case AP4_ATOM_TYPE_TRAF:
        case AP4_ATOM_TYPE_MVEX:
            atom = new AP4_ContainerAtom(type, size, false, stream, *this);
            break;

        case AP4_ATOM_TYPE_MFHD:
            atom = new AP4_MfhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_TFDT:
            atom = new AP4_TfdtAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_TFHD:
            atom = new AP4_TfhdAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_TRUN:
            atom = new AP4_TrunAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_SIDX:
            atom = new AP4_SidxAtom(size, stream);
            break;

        // full container atoms
        case AP4_ATOM_TYPE_META:
            atom = new AP4_ContainerAtom(type, size, true, stream, *this);
            break;

        // other
        case AP4_ATOM_TYPE_AVCC:
            atom = new AP4_DataInfoAtom(type, size, stream);
            break;

        case AP4_ATOM_TYPE_HVCC:
            atom = new AP4_DataInfoAtom(type, size, stream);
            break;

        case AP4_ATOM_TYPE_TEXT:
            atom = new AP4_TextSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_TX3G:
            atom = new AP4_Tx3gSampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_FTAB:
            atom = new AP4_FtabAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_RAW:
            {
                AP4_Offset pos;
                AP4_UI16 ch_count_test;

                stream.Tell(pos);
                stream.Seek(pos + 16);
                stream.ReadUI16(ch_count_test);
                stream.Seek(pos);

                if (ch_count_test == 0) {
                    atom = new AP4_VisualSampleEntry(type, size, stream, *this);
                } else {
                    atom = new AP4_AudioSampleEntry(type, size, stream, *this);
                }
            }
            break;

        case AP4_ATOM_TYPE_CVID:
        case AP4_ATOM_TYPE_SVQ1:
        case AP4_ATOM_TYPE_SVQ2:
        case AP4_ATOM_TYPE_SVQ3:
        case AP4_ATOM_TYPE_H261:
        case AP4_ATOM_TYPE_h263:
        case AP4_ATOM_TYPE_H263:
        case AP4_ATOM_TYPE_S263:
        case AP4_ATOM_TYPE_JPEG:
        case AP4_ATOM_TYPE_MJP2:
        case AP4_ATOM_TYPE_PNG:
        case AP4_ATOM_TYPE_RLE:
        case AP4_ATOM_TYPE_MJPA:
        case AP4_ATOM_TYPE_MJPB:
        case AP4_ATOM_TYPE_RPZA:
        case AP4_ATOM_TYPE_DIVX:
        case AP4_ATOM_TYPE_8BPS:
        case AP4_ATOM_TYPE_3IV1:
        case AP4_ATOM_TYPE_3IV2:
        case AP4_ATOM_TYPE_IV32:
        case AP4_ATOM_TYPE_IV41:
        case AP4_ATOM_TYPE_VP31:

        case AP4_ATOM_TYPE_HDV1:
        case AP4_ATOM_TYPE_HDV2:
        case AP4_ATOM_TYPE_HDV3:
        case AP4_ATOM_TYPE_HDV4:
        case AP4_ATOM_TYPE_HDV5:
        case AP4_ATOM_TYPE_HDV6:
        case AP4_ATOM_TYPE_HDV7:
        case AP4_ATOM_TYPE_HDV8:
        case AP4_ATOM_TYPE_HDV9:
        case AP4_ATOM_TYPE_HDVA:

        case AP4_ATOM_TYPE_XDV1:
        case AP4_ATOM_TYPE_XDV2:
        case AP4_ATOM_TYPE_XDV3:
        case AP4_ATOM_TYPE_XDV4:
        case AP4_ATOM_TYPE_XDV5:
        case AP4_ATOM_TYPE_XDV6:
        case AP4_ATOM_TYPE_XDV7:
        case AP4_ATOM_TYPE_XDV8:
        case AP4_ATOM_TYPE_XDV9:
        case AP4_ATOM_TYPE_XDVA:
        case AP4_ATOM_TYPE_XDVB:
        case AP4_ATOM_TYPE_XDVC:
        case AP4_ATOM_TYPE_XDVD:
        case AP4_ATOM_TYPE_XDVE:
        case AP4_ATOM_TYPE_XDVF:

        case AP4_ATOM_TYPE_XD51:
        case AP4_ATOM_TYPE_XD54:
        case AP4_ATOM_TYPE_XD55:
        case AP4_ATOM_TYPE_XD59:
        case AP4_ATOM_TYPE_XD5A:
        case AP4_ATOM_TYPE_XD5B:
        case AP4_ATOM_TYPE_XD5C:
        case AP4_ATOM_TYPE_XD5D:
        case AP4_ATOM_TYPE_XD5E:
        case AP4_ATOM_TYPE_XD5F:

        case AP4_ATOM_TYPE_XDHD:

        case AP4_ATOM_TYPE_ICOD:
        case AP4_ATOM_TYPE_AVdn:
        case AP4_ATOM_TYPE_FFV1:
        case AP4_ATOM_TYPE_OVC1:
        case AP4_ATOM_TYPE_VC1:
        case AP4_ATOM_TYPE_WMV1:
        case AP4_ATOM_TYPE_CUVC:
        case AP4_ATOM_TYPE_CHQX:
        case AP4_ATOM_TYPE_CFHD:
        case AP4_ATOM_TYPE_DIV3:
        case AP4_ATOM_TYPE_3IVD:
        // RAW Video
        case AP4_ATOM_TYPE_R10g:
        case AP4_ATOM_TYPE_R10k:
        case AP4_ATOM_TYPE_b16g:
        case AP4_ATOM_TYPE_b48r:
        case AP4_ATOM_TYPE_b64a:
        case AP4_ATOM_TYPE_V210:
        case AP4_ATOM_TYPE_v210:
        case AP4_ATOM_TYPE_v308:
        case AP4_ATOM_TYPE_v408:
        case AP4_ATOM_TYPE_2vuy:
        case AP4_ATOM_TYPE_2Vuy:
        case AP4_ATOM_TYPE_DVOO:
        case AP4_ATOM_TYPE_yuvs:
        case AP4_ATOM_TYPE_v410:
        case AP4_ATOM_TYPE_yv12:
        case AP4_ATOM_TYPE_yuv2:
        // Apple ProRes
        case AP4_ATOM_TYPE_APCN:
        case AP4_ATOM_TYPE_APCH:
        case AP4_ATOM_TYPE_APCO:
        case AP4_ATOM_TYPE_APCS:
        case AP4_ATOM_TYPE_AP4H:
        // Motion-JPEG
        case AP4_ATOM_TYPE_MJPG:
        case AP4_ATOM_TYPE_AVDJ:
        case AP4_ATOM_TYPE_DMB1:
        // DV
        case AP4_ATOM_TYPE_DVC:
        case AP4_ATOM_TYPE_DVCP:
        case AP4_ATOM_TYPE_DVPP:
        case AP4_ATOM_TYPE_DV5N:
        case AP4_ATOM_TYPE_DV5P:
        case AP4_ATOM_TYPE_DVHQ:
        case AP4_ATOM_TYPE_DVH5:
        case AP4_ATOM_TYPE_DVH6:
        // MagicYUV
        case AP4_ATOM_TYPE_M8RG:
        case AP4_ATOM_TYPE_M8RA:
        case AP4_ATOM_TYPE_M8G0:
        case AP4_ATOM_TYPE_M8Y0:
        case AP4_ATOM_TYPE_M8Y2:
        case AP4_ATOM_TYPE_M8Y4:
        case AP4_ATOM_TYPE_M8YA:
            atom = new AP4_VisualSampleEntry(type, size, stream, *this);
            break;

        case AP4_ATOM_TYPE_SAMR:
        case AP4_ATOM_TYPE__MP3:
        case AP4_ATOM_TYPE_IMA4:
        case AP4_ATOM_TYPE_QDMC:
        case AP4_ATOM_TYPE_QDM2:
        case AP4_ATOM_TYPE_SPEX:
        case AP4_ATOM_TYPE_NONE:
        case AP4_ATOM_TYPE_TWOS:
        case AP4_ATOM_TYPE_SOWT:
        case AP4_ATOM_TYPE_IN24:
        case AP4_ATOM_TYPE_IN32:
        case AP4_ATOM_TYPE_FL32:
        case AP4_ATOM_TYPE_FL64:
        case AP4_ATOM_TYPE_LPCM:
        case AP4_ATOM_TYPE_ALAW:
        case AP4_ATOM_TYPE_ULAW:
        case AP4_ATOM_TYPE_NMOS:
        case AP4_ATOM_TYPE_ALAC:
        case AP4_ATOM_TYPE_MAC3:
        case AP4_ATOM_TYPE_MAC6:
        case AP4_ATOM_TYPE_SAWB:
        case AP4_ATOM_TYPE_OWMA:
        case AP4_ATOM_TYPE_WMA:
            atom = new AP4_AudioSampleEntry(type, size, stream, *this);
            break;

        case AP4_ATOM_TYPE__AC3: // AC3-in-MP4 from ISO Standard
        case AP4_ATOM_TYPE_SAC3: // AC3-in-MP4 from Nero Stuff >.<
            atom = new AP4_AC3SampleEntry(size, stream, *this);
            break;
        case AP4_ATOM_TYPE_EAC3:
            atom = new AP4_EAC3SampleEntry(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_CHPL:
            atom = new AP4_ChplAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_DATA:
            atom = new AP4_DataAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_DCOM:
            atom = new AP4_DcomAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_CMVD:
            atom = new AP4_CmvdAtom(size, stream, *this);
            break;

        case AP4_ATOM_TYPE_ENDA:
            atom = new AP4_EndaAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_PASP:
            atom = new AP4_PaspAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_CHAP:
            atom = new AP4_ChapAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_ELST:
            atom = new AP4_ElstAtom(size, stream);
            break;

        case AP4_ATOM_TYPE_DVC1:
            atom = new AP4_Dvc1Atom(size, stream);
            break;

        case AP4_ATOM_TYPE_WFEX:
            atom = new AP4_DataInfoAtom(type, size, stream);
            break;

        default:
            if(parent && parent->GetType() == AP4_ATOM_TYPE_STSD && (type & 0xffff0000) == AP4_ATOM_TYPE('m', 's', 0, 0)) {
                atom = new AP4_AudioSampleEntry(type, size, stream, *this);
            } else {// try all the external type handlers
                atom = NULL;
                AP4_List<TypeHandler>::Item* handler_item = m_TypeHandlers.FirstItem();
                while (handler_item) {
                    TypeHandler* handler = handler_item->GetData();
                    if (AP4_SUCCEEDED(handler->CreateAtom(type, size, stream, atom))) {
                        break;
                    }
                    handler_item = handler_item->GetNext();
                }
                if (atom == NULL) {
                    // no custom handlers, create a generic atom
                    atom = new AP4_UnknownAtom(type, size, false, stream);
                }
            }

            break;
    }

    // skip to the end of the atom
    AP4_Size streamSize = 0;
    stream.GetSize(streamSize);
    bytes_available -= size;
    result = stream.Seek(MIN(start + size, streamSize));
    if (AP4_FAILED(result)) {
        delete atom;
        atom = NULL;
    }

    return result;
}

