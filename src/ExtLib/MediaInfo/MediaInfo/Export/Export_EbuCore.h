/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef Export_EbuCoreH
#define Export_EbuCoreH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/MediaInfo_Internal.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
/// @brief Export_EbuCore
//***************************************************************************

class Export_EbuCore
{
public :
    //Constructeur/Destructeur
    Export_EbuCore ();
    ~Export_EbuCore ();

    //Input
    enum version
    {
        Version_1_5,
        Version_1_6,
        Version_1_8,
        Version_Max
    };
    enum acquisitiondataoutputmode
    {
        AcquisitionDataOutputMode_Default,
        AcquisitionDataOutputMode_parameterSegment,
        AcquisitionDataOutputMode_segmentParameter,
        AcquisitionDataOutputMode_Max,
    };
    enum format
    {
        Format_XML,
        Format_JSON,
        Format_Max,
    };

    ZenLib::Ztring Transform(MediaInfo_Internal &MI, version Version=version(Version_Max-1), acquisitiondataoutputmode AcquisitionDataOutputMode=AcquisitionDataOutputMode_Default, format Format=Format_XML, Ztring ExternalMetadataValues=Ztring(), Ztring ExternalMetaDataConfig=Ztring());
};

} //NameSpace
#endif
