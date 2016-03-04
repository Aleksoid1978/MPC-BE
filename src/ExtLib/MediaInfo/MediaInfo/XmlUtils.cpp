/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Utility methods for TinyXML-2
// Contributor: Peter Chapman, pchapman@vidcheck.com
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/XmlUtils.h"
#include <string>
#include <cstddef>
using namespace tinyxml2;
//---------------------------------------------------------------------------

// Finds the value of the named attribute within the given element or its
// closest ancestor
static const char *AncestorAttrVal(const XMLElement* Elem, const char* AttrName)
{
    if (!Elem)
        return NULL;
    if (const XMLAttribute* Attr=Elem->FindAttribute(AttrName))
        return Attr->Value();
    const XMLElement* ParentElem=NULL;
    if (const XMLNode* ParentNode=Elem->Parent())
        ParentElem=ParentNode->ToElement();
    return AncestorAttrVal(ParentElem, AttrName);
}

namespace MediaInfoLib
{

//---------------------------------------------------------------------------

const char *LocalName(const XMLElement* Elem)
{
    const char* Name=Elem->Name();
    const char* Colon=strrchr(Name, ':');
    return Colon ? Colon+1 : Name;
}

//---------------------------------------------------------------------------

const char *LocalName(const XMLElement* Elem, const char* &Ns /* out */)
{
    const char* Name=Elem->Name();
    if (const char* Colon=strrchr(Name, ':'))
    {
        // Search element and ancestors for given namespace prefix
        Ns=AncestorAttrVal(Elem, ("xmlns:"+std::string(Name, Colon)).c_str());
        // NB no default namespace if given prefix not found, will return NULL
        return Colon+1;
    }
    else
    {
        Ns=AncestorAttrVal(Elem, "xmlns");
        if (!Ns)
            Ns = ""; // default namespace
        return Name;
    }
}

//---------------------------------------------------------------------------

bool MatchQName(const XMLElement* Elem, const char* Name, const char* NameSpace)
{
    if (strcmp(LocalName(Elem), Name))
        return false; // exit cheaply if local name doesn't match
    const char *ElemNs;
    LocalName(Elem, ElemNs); // look up namespace
    return ElemNs && // ElemNs can be null if Elem uses a bad namespace prefix
           !strcmp(ElemNs, NameSpace); // compare namespaces
}

}
