/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about DCP/IMF Composition Playlist files
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_DcpCplH
#define MediaInfo_File_DcpCplH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
#include "MediaInfo/Multiple/File_DcpPkl.h"
#include <vector>
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

class File__ReferenceFilesHelper;

//***************************************************************************
// Class File_DcpCpl
//***************************************************************************

class File_DcpCpl : public File__Analyze
{
public :
    //Constructor/Destructor
    File_DcpCpl();
    ~File_DcpCpl();

private :
    //Streams management
    void Streams_Finish ();

    //Buffer - Global
    #if MEDIAINFO_SEEK
    size_t Read_Buffer_Seek (size_t Method, int64u Value, int64u ID);
    #endif //MEDIAINFO_SEEK

    //Buffer - File header
    bool FileHeader_Begin();

    //PKL
    size_t PKL_Pos;
    void MergeFromAm (File_DcpPkl::streams &StreamsToMerge);

    //Temp
    File__ReferenceFilesHelper*     ReferenceFiles;
    #if MEDIAINFO_ADVANCED
        struct descriptor
        {
            std::vector<descriptor*> SubDescriptors;
            int16u Jpeg2000_Rsiz;

            descriptor()
            : Jpeg2000_Rsiz((int16u)-1)
            {
            }

            ~descriptor()
            {
                for (std::vector<descriptor*>::iterator SubDescriptor=SubDescriptors.begin(); SubDescriptor!=SubDescriptors.end(); ++SubDescriptor)
                    delete *SubDescriptor; //*SubDescriptor=NULL;
            }
        };
        std::map<string, descriptor*> EssenceDescriptorList;
    #endif //MEDIAINFO_ADVANCED
};

} //NameSpace

#endif

