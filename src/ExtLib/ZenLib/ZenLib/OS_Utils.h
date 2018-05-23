/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef ZenOS_UtilsH
#define ZenOS_UtilsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Ztring.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef WINDOWS_UWP
#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#endif
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// OS Information
//***************************************************************************

//---------------------------------------------------------------------------
bool IsWin9X ();
// Execute
//***************************************************************************

void Shell_Execute(const Ztring &ToExecute);

//***************************************************************************
// Directorues
//***************************************************************************

Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption);

//***************************************************************************
// WinRT Helpers
//***************************************************************************
#ifdef WINDOWS_UWP
template <typename T>
inline HRESULT Await(const Microsoft::WRL::ComPtr<T> &Operation)
{
    if (!Operation)
        return E_POINTER;

    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncInfo> Info;
    HRESULT Result=Operation.As(&Info);
    if (FAILED(Result))
        return Result;

    ABI::Windows::Foundation::AsyncStatus Status;
    while (SUCCEEDED(Result=Info->get_Status(&Status)) && Status==Started)
        Sleep(0); //Yield

    if (FAILED(Result) || Status!=Completed) {
        HRESULT Error_Code;
        Result=Info->get_ErrorCode(&Error_Code);
        if (FAILED(Result))
            return Result;
        return Error_Code;
    }

    return Result;
}

HRESULT Add_Item_To_FUA(Microsoft::WRL::Wrappers::HStringReference Path, Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageItem> &Item);
HRESULT Get_File(Microsoft::WRL::Wrappers::HStringReference File_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFile> &File);
HRESULT Get_Folder(Microsoft::WRL::Wrappers::HStringReference Folder_Name,  Microsoft::WRL::ComPtr<ABI::Windows::Storage::IStorageFolder> &Folder);
#endif

} //namespace ZenLib
#endif
