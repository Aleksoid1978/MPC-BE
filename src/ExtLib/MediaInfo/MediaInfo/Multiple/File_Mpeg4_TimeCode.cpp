/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
// Pre-compilation
#include "MediaInfo/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Setup.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#if defined(MEDIAINFO_MPEG4_YES)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/Multiple/File_Mpeg4_TimeCode.h"
#include "MediaInfo/TimeCode.h"
#include "MediaInfo/MediaInfo_Config_MediaInfo.h"
#include <limits>
using namespace std;
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
File_Mpeg4_TimeCode::File_Mpeg4_TimeCode()
:File__Analyze()
{
    //Out
    Pos=numeric_limits<int64s>::max();

    FirstEditOffset=0;
    FirstEditDuration=(int64u)-1;
    NumberOfFrames=0;
    FrameMultiplier=1;
    DropFrame=false;
    NegativeTimes=false;
    tkhd_Duration=0;
    mvhd_Duration_TimeScale=0;

    //Temp
    FrameMultiplier_Pos=0;
}

//***************************************************************************
// Streams management
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4_TimeCode::Streams_Accept()
{
    Stream_Prepare(Stream_Other);
    Fill(Stream_Other, StreamPos_Last, Other_Type, "Time code");
}

//---------------------------------------------------------------------------
void File_Mpeg4_TimeCode::Streams_Fill()
{
    if (Pos!=numeric_limits<int64s>::max())
    {
        float64 FrameRate_WithDF;
        if (tmcd_Duration && tmcd_Duration_TimeScale)
        {
            FrameRate_WithDF=(float64)tmcd_Duration_TimeScale/(float64)tmcd_Duration;
            if (!NumberOfFrames)
                NumberOfFrames=(int8u)float64_int64s(FrameRate_WithDF)/FrameMultiplier;
        }
        else
        {
            FrameRate_WithDF=NumberOfFrames;
            if (DropFrame)
            {
                int FramesToRemove=0;
                int NumberOfFramesMultiplier=0;
                while (NumberOfFrames>NumberOfFramesMultiplier)
                {
                    FramesToRemove+=108;
                    NumberOfFramesMultiplier+=30;
                }
                float64 FramesPerHour_NDF=FrameRate_WithDF*60*60;
                FrameRate_WithDF*=(FramesPerHour_NDF-FramesToRemove)/FramesPerHour_NDF;
            }

        }
        Fill(Stream_General, 0, "Delay", Pos*FrameMultiplier*1000/FrameRate_WithDF, 0);
    }
}

//---------------------------------------------------------------------------
void File_Mpeg4_TimeCode::Streams_Finish()
{
    if (Pos!=numeric_limits<int64s>::max())
    {
        float64 FrameRate_WithDF;
        if (tmcd_Duration && tmcd_Duration_TimeScale)
        {
            FrameRate_WithDF=(float64)tmcd_Duration_TimeScale/(float64)tmcd_Duration;
            if (!NumberOfFrames)
                NumberOfFrames=(int8u)float64_int64s(FrameRate_WithDF)/FrameMultiplier;
        }
        else
        {
            FrameRate_WithDF=NumberOfFrames;
            if (DropFrame)
            {
                int FramesToRemove=0;
                int NumberOfFramesMultiplier=0;
                while (NumberOfFrames>NumberOfFramesMultiplier)
                {
                    FramesToRemove+=108;
                    NumberOfFramesMultiplier+=30;
                }
                float64 FramesPerHour_NDF=FrameRate_WithDF*60*60;
                FrameRate_WithDF*=(FramesPerHour_NDF-FramesToRemove)/FramesPerHour_NDF;
            }

        }

        TimeCode TC((int64_t)Pos, NumberOfFrames-1, TimeCode::DropFrame(DropFrame));
        if (FrameMultiplier>1)
        {
            int64s Frames=TC.GetFrames();
            TC-=TC.GetFrames();
            TC=TimeCode((int64_t)(TC.ToFrames()*FrameMultiplier), NumberOfFrames*FrameMultiplier-1, TimeCode::DropFrame(DropFrame));
            TC+=Frames*FrameMultiplier;
        }
        Fill(Stream_Other, StreamPos_Last, Other_TimeCode_FirstFrame, TC.ToString().c_str());
        int64s FrameCount;
        if (NumberOfFrames==mdhd_Duration_TimeScale) // Prioritize mdhd or edit list for in case there are drop frame and integer frame rate
        {
            if (FirstEditDuration!=(int64u)-1)
            {
                float64 FrameCountF=(float64)FirstEditDuration/mvhd_Duration_TimeScale*FrameRate_WithDF*FrameMultiplier;
                FrameCount=(int64u)float64_int64s(FrameCountF);
                if (FrameCountF-FrameCount>0.01) // TODO: avoid rouding issues and better way to manage partial frames
                    FrameCount++;
            }
            else
                FrameCount=mdhd_Duration-FirstEditOffset;
        }
        else if (mvhd_Duration_TimeScale)
        {
            float64 FrameCountF=(float64)tkhd_Duration/mvhd_Duration_TimeScale*FrameRate_WithDF*FrameMultiplier;
            FrameCount=(int64u)float64_int64s(FrameCountF);
            if (FrameCountF-FrameCount>0.01) // TODO: avoid rouding issues and better way to manage partial frames
                FrameCount++;
        }
        else
            FrameCount=0;
        if (FrameCount)
            Fill(Stream_Other, StreamPos_Last, Other_FrameCount, FrameCount);
        if (Frame_Count==1)
        {
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_Stripped, "Yes");
            if (FrameCount)
                Fill(Stream_Other, StreamPos_Last, Other_TimeCode_LastFrame, (TC+(FrameCount-1)).ToString().c_str());
        }
        else
        {
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_Stripped, "No");
            TimeCode TC_Last((int64_t)Pos_Last, NumberOfFrames-1, TimeCode::DropFrame(DropFrame));
            if (FrameMultiplier>1)
            {
                int64s Frames=TC_Last.GetFrames();
                TC_Last-=TC_Last.GetFrames();
                TC_Last=TimeCode((int64_t)(TC_Last.ToFrames()*FrameMultiplier), NumberOfFrames*FrameMultiplier-1, TimeCode::DropFrame(DropFrame));
                TC_Last+=Frames*FrameMultiplier+(Config->ParseSpeed<=0.5?(FrameMultiplier-1):FrameMultiplier_Pos);
            }
            Fill(Stream_Other, StreamPos_Last, Other_TimeCode_LastFrame, TC_Last.ToString().c_str());
        }
    }
}

//***************************************************************************
// Buffer - Global
//***************************************************************************

//---------------------------------------------------------------------------
void File_Mpeg4_TimeCode::Read_Buffer_Init()
{
    ZtringListList Map=Config->File_ForceParser_Config_Get();
    if (!Map.empty())
    {
        NumberOfFrames=Map(__T("NumberOfFrames")).To_int8u();
        DropFrame=Map(__T("DropFrame")).To_int8u()?true:false;
        NegativeTimes=Map(__T("NegativeTimes")).To_int8u()?true:false;
    }
}

//---------------------------------------------------------------------------
void File_Mpeg4_TimeCode::Read_Buffer_Continue()
{
    //Parsing
    while (Element_Offset<Element_Size)
    {
        int32u Position=0;
        Get_B4 (Position,                                       "Position");
        int64s Pos_Last_Temp;
        if (NegativeTimes)
            Pos_Last_Temp=(int32s)Position;
        else
            Pos_Last_Temp=Position;
        Pos_Last_Temp+=FirstEditOffset;
        if (Pos==numeric_limits<int64s>::max()) //First time code
        {
            Pos=Pos_Last_Temp;
            if (Config->ParseSpeed<=0.5 && Element_Offset!=Element_Size)
                Skip_XX(Element_Size-Element_Offset,            "Other positions");
        }
        else if (Config->ParseSpeed>0.5)
        {
            FrameMultiplier_Pos++;
            if (FrameMultiplier_Pos>=FrameMultiplier)
            {
                FrameMultiplier_Pos=0;
                Pos_Last++;
            }
            if (Pos_Last!=Pos_Last_Temp)
            {
                const Ztring& Discontinuities=Retrieve_Const(Stream_Other, 0, "Discontinuities");
                if (Discontinuities.size()>25*10)
                {
                    if (Discontinuities[Discontinuities.size()-1]!=']')
                        Fill(Stream_Other, 0, "Discontinuities", "[...]");
                }
                else
                {
                    Pos_Last--;
                    TimeCode TC_Last1((int64_t)Pos_Last, NumberOfFrames-1, TimeCode::DropFrame(DropFrame));
                    if (FrameMultiplier>1)
                    {
                        int64s Frames=TC_Last1.GetFrames();
                        TC_Last1-=TC_Last1.GetFrames();
                        TC_Last1=TimeCode((int64_t)(TC_Last1.ToFrames()*FrameMultiplier), NumberOfFrames*FrameMultiplier-1, TimeCode::DropFrame(DropFrame));
                        TC_Last1+=Frames*FrameMultiplier;
                    }
                    string Discontinuity=TC_Last1.ToString();
                    TimeCode TC_Last2((int64_t)Pos_Last_Temp, NumberOfFrames-1, TimeCode::DropFrame(DropFrame));
                    if (FrameMultiplier>1)
                    {
                        int64s Frames=TC_Last2.GetFrames();
                        TC_Last2-=TC_Last2.GetFrames();
                        TC_Last2=TimeCode((int64_t)(TC_Last2.ToFrames()*FrameMultiplier), NumberOfFrames*FrameMultiplier-1, TimeCode::DropFrame(DropFrame));
                        TC_Last2+=(Frames+1)*FrameMultiplier-1;
                    }
                    Discontinuity+='-';
                    Discontinuity+=TC_Last2.ToString();
                    Fill(Stream_Other, 0, "Discontinuities", Discontinuity);
                }
            }
        }
        Pos_Last=Pos_Last_Temp;
    }

    FILLING_BEGIN();
        Frame_Count+=Element_Size/4;

        if (!Status[IsAccepted])
        {
            Accept("TimeCode");
            Fill("TimeCode");
        }
    FILLING_END();
}

}

#endif //MEDIAINFO_MPEG4_YES
