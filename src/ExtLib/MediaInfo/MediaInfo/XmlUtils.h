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
#ifndef MediaInfo_Xml_Utils
#define MediaInfo_Xml_Utils
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "tinyxml2.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

// Returns the local name (with no namespace prefix) of an XMLElement.
const char* LocalName(const tinyxml2::XMLElement* Elem);
// Returns the local name (with no namespace prefix) of an XMLElement and also
// the the namespace name.
const char* LocalName(const tinyxml2::XMLElement* Elem, const char* &NameSpace /* out */);

// Returns true if the given XMLElement has a local name <Name> and namespace name
// <NameSpace>
bool MatchQName(const tinyxml2::XMLElement* Elem, const char* Name, const char* NameSpace);

} //NameSpace

#endif

