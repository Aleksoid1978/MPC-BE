/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Export_FimsH
#define Export_FimsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
/// @brief Export_Fims
//***************************************************************************

class Export_Fims
{
public :
    //Constructeur/Destructeur
    Export_Fims ();
    ~Export_Fims ();

    //Input
    enum version
    {
        Version_1_1,
        Version_1_2,
        Version_1_3,
    };
    Ztring Transform(MediaInfo_Internal &MI, version Version=Version_1_2);
};

} //NameSpace
#endif
