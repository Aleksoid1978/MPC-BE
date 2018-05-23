/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a zlib-style license that can
 *  be found in the License.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "ZenLib/PreComp.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "ZenLib/Conf_Internal.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#ifdef ZENLIB_USEWX
#else //ZENLIB_USEWX
    #ifdef WINDOWS
        #undef __TEXT
        #include <windows.h>
        #include <shlobj.h>
        #ifdef WINDOWS_UWP
            #include <wrl.h>
            #include <windows.foundation.h>
            #include <windows.storage.h>
            #include <windows.storage.accesscache.h>
            #include <windows.storage.streams.h>
            #include <windows.security.cryptography.h>
            #include <windows.security.cryptography.core.h>
            using namespace Microsoft::WRL;
            using namespace Microsoft::WRL::Wrappers;
            using namespace ABI::Windows::Foundation;
            using namespace ABI::Windows::Foundation::Collections;
            using namespace ABI::Windows::Storage;
            using namespace ABI::Windows::Storage::AccessCache;
            using namespace ABI::Windows::Storage::Streams;
            using namespace ABI::Windows::Security::Cryptography;
            using namespace ABI::Windows::Security::Cryptography::Core;
        #endif
    #endif
#endif //ZENLIB_USEWX
#include "ZenLib/OS_Utils.h"
//---------------------------------------------------------------------------

namespace ZenLib
{

//***************************************************************************
// OS info
//***************************************************************************

//---------------------------------------------------------------------------
bool IsWin9X ()
{
    return false; //Hardcoded value because we don't support Win9x anymore
}

//***************************************************************************
// WinRT Helpers
//***************************************************************************

#if defined(WINDOWS) && defined(WINDOWS_UWP)
//---------------------------------------------------------------------------
HRESULT Add_Item_To_FUA(HStringReference Path, ComPtr<IStorageItem> &Item)
{
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght = 0;
    UINT32 Hash_Lenght = 0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(Path.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        ComPtr<IStorageItemAccessList> FutureAccess_List;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) &&
            Permissions_Manager &&
            SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
            FutureAccess_List)
            return FutureAccess_List->AddOrReplace(Path_Hash.Get(), *Item.GetAddressOf(), HString::MakeReference(L"").Get());
    }

    return E_FAIL;
}

//---------------------------------------------------------------------------
HRESULT Get_File(HStringReference File_Name, ComPtr<IStorageFile> &File)
{
    //Try to access file directly
    ComPtr<IStorageFileStatics> Storage;
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFile).Get(), &Storage)) && Storage)
    {
        ComPtr<IAsyncOperation<StorageFile*> > Async_GetFile;
        if (SUCCEEDED(Storage->GetFileFromPathAsync(File_Name.Get(), &Async_GetFile)) &&
            SUCCEEDED(Await(Async_GetFile)) &&
            SUCCEEDED(Async_GetFile->GetResults(&File)) &&
            File)
            return S_OK;
    }

    //Try to access file by sha256 hashed path in future access list
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght=0;
    UINT32 Hash_Lenght=0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(File_Name.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) && Permissions_Manager)
        {
            ComPtr<IStorageItemAccessList> FutureAccess_List;
            ComPtr<IAsyncOperation<StorageFile*> > Async_GetFile_FutureAccess;
            if (SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
                FutureAccess_List &&
                SUCCEEDED(FutureAccess_List->GetFileAsync(Path_Hash.Get(), &Async_GetFile_FutureAccess)) &&
                Await(Async_GetFile_FutureAccess) &&
                SUCCEEDED(Async_GetFile_FutureAccess->GetResults(&File)) &&
                File)
                return S_OK;
        }
    }

    return E_FAIL;
}

//---------------------------------------------------------------------------
HRESULT Get_Folder(HStringReference Folder_Name, ComPtr<IStorageFolder> &Folder)
{
    //Try to access folder directly
    ComPtr<IStorageFolderStatics> Storage;
    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFolder).Get(), &Storage)) && Storage)
    {
        ComPtr<IAsyncOperation<StorageFolder*> > Async_GetFolder;
        if (SUCCEEDED(Storage->GetFolderFromPathAsync(Folder_Name.Get(), &Async_GetFolder)) &&
            SUCCEEDED(Await(Async_GetFolder)) &&
            SUCCEEDED(Async_GetFolder->GetResults(&Folder)) &&
            Folder)
            return S_OK;
    }

    //Try to access folder by sha256 hashed path in future access list
    ComPtr<IBuffer> Path_Buffer;
    ComPtr<IBuffer> Hash_Buffer;
    ComPtr<ICryptographicBufferStatics> Cryptographic_Buffer_Statics;
    ComPtr<IHashAlgorithmProviderStatics> Hash_Provider_Statics;
    ComPtr<IHashAlgorithmProvider> Hash_Provider;
    UINT32 Buffer_Lenght = 0;
    UINT32 Hash_Lenght = 0;
    HString Path_Hash;

    if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer).Get(), &Cryptographic_Buffer_Statics)) &&
        Cryptographic_Buffer_Statics &&
        SUCCEEDED(Cryptographic_Buffer_Statics->ConvertStringToBinary(Folder_Name.Get(), BinaryStringEncoding_Utf16LE, &Path_Buffer)) &&
        Path_Buffer &&
        SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider).Get(), &Hash_Provider_Statics)) &&
        Hash_Provider_Statics &&
        SUCCEEDED(Hash_Provider_Statics->OpenAlgorithm(HString::MakeReference(L"SHA256").Get(), &Hash_Provider)) &&
        Hash_Provider &&
        SUCCEEDED(Hash_Provider->HashData(*Path_Buffer.GetAddressOf(), &Hash_Buffer)) &&
        Hash_Buffer &&
        SUCCEEDED(Hash_Buffer->get_Length(&Buffer_Lenght)) &&
        Buffer_Lenght &&
        SUCCEEDED(Hash_Provider->get_HashLength(&Hash_Lenght)) &&
        Hash_Lenght &&
        Buffer_Lenght == Hash_Lenght &&
        SUCCEEDED(Cryptographic_Buffer_Statics->EncodeToBase64String(*Hash_Buffer.GetAddressOf(), Path_Hash.GetAddressOf())))
    {
        ComPtr<IStorageApplicationPermissionsStatics> Permissions_Manager;
        if (SUCCEEDED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_AccessCache_StorageApplicationPermissions).Get(), &Permissions_Manager)) && Permissions_Manager)
        {
            ComPtr<IStorageItemAccessList> FutureAccess_List;
            ComPtr<IAsyncOperation<StorageFolder*> > Async_GetFolder_FutureAccess;
            if (SUCCEEDED(Permissions_Manager->get_FutureAccessList(&FutureAccess_List)) &&
                FutureAccess_List &&
                SUCCEEDED(FutureAccess_List->GetFolderAsync(Folder_Name.Get(), &Async_GetFolder_FutureAccess)) &&
                Await(Async_GetFolder_FutureAccess) &&
                SUCCEEDED(Async_GetFolder_FutureAccess->GetResults(&Folder)) &&
                Folder)
                return S_OK;
        }
    }

    return E_FAIL;
}
#endif

//***************************************************************************
// Shell
//***************************************************************************

void Shell_Execute(const Ztring &ToExecute)
{
    #ifdef ZENLIB_USEWX
    #else //ZENLIB_USEWX
        #if defined(WINDOWS) && !defined(WINDOWS_UWP)
            ShellExecute(NULL, __T("open"), ToExecute.c_str(), NULL, NULL, 0);
        #else
            //Not supported
        #endif
    #endif //ZENLIB_USEWX
}

//***************************************************************************
// Directories
//***************************************************************************

//---------------------------------------------------------------------------
// Select directory code
// Extracted from TBffolder by Torsten Johann (t-johann@gmx.de)

Ztring Directory_Select_Caption;

#if defined(WINDOWS) && !defined(WINDOWS_UWP)
    #ifdef UNICODE
        char    InitDirA[MAX_PATH];
        wchar_t InitDir [MAX_PATH];

        int __stdcall ShowOpenFolder_CallbackProc (HWND hwnd, UINT uMsg, LPARAM, LPARAM)
        {
            if (uMsg==BFFM_INITIALIZED)
            {
                SetWindowText(hwnd, Directory_Select_Caption.c_str());    // Caption
                SendMessage  (hwnd, BFFM_ENABLEOK, 0, TRUE);
                SendMessage  (hwnd, BFFM_SETSELECTION, true, (LPARAM)&InitDir);
            }
            return 0;
        }

        Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption)
        {
            //Caption
            Directory_Select_Caption=Caption;

            //Values
            LPMALLOC        Malloc;
            LPSHELLFOLDER   ShellFolder;
            BROWSEINFO      BrowseInfo;
            LPITEMIDLIST    ItemIdList;

            //Initializing the SHBrowseForFolder function
            if (SHGetMalloc(&Malloc)!=NOERROR)
                return Ztring();
            if (SHGetDesktopFolder(&ShellFolder)!=NOERROR)
                return Ztring();
            ZeroMemory(&BrowseInfo, sizeof(BROWSEINFOW));
            BrowseInfo.ulFlags+=BIF_RETURNONLYFSDIRS;
            BrowseInfo.hwndOwner=(HWND)Handle;
            BrowseInfo.pszDisplayName=InitDir;
            BrowseInfo.lpszTitle=Title.c_str();
            BrowseInfo.lpfn=ShowOpenFolder_CallbackProc;

            //Displaying
            ItemIdList=SHBrowseForFolder(&BrowseInfo);

            //Releasing
            ShellFolder->Release();
            if (ItemIdList!=NULL)
            {
                SHGetPathFromIDList(ItemIdList, InitDir);
                Malloc->Free(ItemIdList);
                Malloc->Release();

                //The value
                return InitDir;
            }
            else
                return Ztring();
        }

    #else
        char InitDirA[MAX_PATH];

        int __stdcall ShowOpenFolder_CallbackProc (HWND hwnd, UINT uMsg, LPARAM, LPARAM)
        {
            if (uMsg==BFFM_INITIALIZED)
            {
                SetWindowText (hwnd, Directory_Select_Caption.c_str());    // Caption
                SendMessage   (hwnd, BFFM_ENABLEOK, 0, TRUE);
                SendMessage   (hwnd, BFFM_SETSELECTION, true, (LPARAM)&InitDirA);
            }
            return 0;
        }

        Ztring OpenFolder_Show(void* Handle, const Ztring &Title, const Ztring &Caption)
        {
            //Caption
            Directory_Select_Caption=Caption;

            //Values
            LPMALLOC        Malloc;
            LPSHELLFOLDER   ShellFolder;
            BROWSEINFO      BrowseInfo;
            LPITEMIDLIST    ItemIdList;

            //Initializing the SHBrowseForFolder function
            if (SHGetMalloc(&Malloc)!=NOERROR)
                return Ztring();
            if (SHGetDesktopFolder(&ShellFolder)!=NOERROR)
                return Ztring();
            ZeroMemory(&BrowseInfo, sizeof(BROWSEINFO));
            BrowseInfo.ulFlags+=BIF_RETURNONLYFSDIRS;
            BrowseInfo.hwndOwner=(HWND)Handle;
            BrowseInfo.pszDisplayName=InitDirA;
            BrowseInfo.lpszTitle=Title.c_str();
            BrowseInfo.lpfn=ShowOpenFolder_CallbackProc;

            //Displaying
            ItemIdList=SHBrowseForFolder(&BrowseInfo);

            //Releasing
            ShellFolder->Release();
            if (ItemIdList!=NULL)
            {
                SHGetPathFromIDList(ItemIdList, InitDirA);
                Malloc->Free(ItemIdList);
                Malloc->Release();

                //The value
                return InitDirA;
            }
            else
                return Ztring();
        }
    #endif //UNICODE
#endif //WINDOWS

} //namespace ZenLib
