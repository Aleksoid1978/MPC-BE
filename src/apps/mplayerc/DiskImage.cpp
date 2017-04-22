/*
 * (C) 2014-2017 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"

#include <InitGuid.h>
#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include <WinIoCtl.h>
#include "DiskImage.h"

#define ENABLE_DTLITE 1
#define ENABLE_VIRTUALCLONEDRIVE 1
#define ENABLE_WIN8VIRTUALDISK 1

CDiskImage::CDiskImage()
	: m_DriveType(NONE)
	, m_DriveLetter(0)
	// Windows 8 VirtualDisk
	, m_hVirtualDiskModule(NULL)
	, m_OpenVirtualDiskFunc(NULL)
	, m_AttachVirtualDiskFunc(NULL)
	, m_GetVirtualDiskPhysicalPathFunc(NULL)
	, m_VHDHandle(INVALID_HANDLE_VALUE)
	// DAEMON Tools Lite
	, m_dtdrive(dt_none)
{
}

CDiskImage::~CDiskImage()
{
	UnmountDiskImage();

	if (m_hVirtualDiskModule) {
		FreeLibrary(m_hVirtualDiskModule);
		m_hVirtualDiskModule = NULL;
	}
}

void CDiskImage::Init()
{
	CRegKey key;

#if ENABLE_DTLITE
	// DAEMON Tools Lite
	m_dtlite_path.Empty();

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\DTLite.exe", KEY_READ)) {
		ULONG nChars = 0;
		if (ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &nChars)) {
			if (ERROR_SUCCESS == key.QueryStringValue(NULL, m_dtlite_path.GetBuffer(nChars), &nChars)) {
				m_dtlite_path.ReleaseBuffer(nChars);
			}
		}
		key.Close();
	}

	if (::PathFileExists(m_dtlite_path)) {
		// simple check
		m_DriveType = DTLITE;
		return;

		// detailed checking
		//SHELLEXECUTEINFO execinfo;
		//memset(&execinfo, 0, sizeof(execinfo));
		//execinfo.lpFile = m_dtlite_path;
		//execinfo.lpParameters = L"-get_count";
		//execinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		//execinfo.nShow = SW_HIDE;
		//execinfo.cbSize = sizeof(execinfo);
		//DWORD ec = 0;
		//if (ShellExecuteEx(&execinfo)) {
		//	WaitForSingleObject(execinfo.hProcess, INFINITE);
		//	if (GetExitCodeProcess(execinfo.hProcess, &ec) && ec != (DWORD)-1 && ec != 0) {
		//		m_DriveType = DTLITE;
		//		return;
		//	}
		//}
	}
#endif

#if ENABLE_VIRTUALCLONEDRIVE
	// Virtual CloneDrive
	m_vcd_path.Empty();

	if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\VCDMount.exe", KEY_READ)) {
		ULONG nChars = 0;
		if (ERROR_SUCCESS == key.QueryStringValue(NULL, NULL, &nChars)) {
			if (ERROR_SUCCESS == key.QueryStringValue(NULL, m_vcd_path.GetBuffer(nChars), &nChars)) {
				m_vcd_path.ReleaseBuffer(nChars);
			}
		}
		key.Close();
	}

	if (::PathFileExists(m_vcd_path)) {
		// simple check
		m_DriveType = VCD;
		return;
	}
#endif

#if ENABLE_WIN8VIRTUALDISK
	// Windows 8 VirtualDisk
	if (IsWin8orLater()) {
		m_hVirtualDiskModule = LoadLibrary(L"VirtDisk.dll");

		if (m_hVirtualDiskModule) {
			(FARPROC &)m_OpenVirtualDiskFunc			= GetProcAddress(m_hVirtualDiskModule, "OpenVirtualDisk");
			(FARPROC &)m_AttachVirtualDiskFunc			= GetProcAddress(m_hVirtualDiskModule, "AttachVirtualDisk");
			(FARPROC &)m_GetVirtualDiskPhysicalPathFunc	= GetProcAddress(m_hVirtualDiskModule, "GetVirtualDiskPhysicalPath");

			if (m_OpenVirtualDiskFunc && m_AttachVirtualDiskFunc && m_GetVirtualDiskPhysicalPathFunc) {
				m_DriveType = WIN8;
				return;
			}

			FreeLibrary(m_hVirtualDiskModule);
			m_hVirtualDiskModule = NULL;
		}
	}
#endif
}

bool CDiskImage::DriveAvailable()
{
	return (m_DriveType > NONE);
}

const LPCWSTR CDiskImage::GetExts()
{
	// DAEMON Tools Lite
	if (m_DriveType == DTLITE) {
		return L"*.iso;*.nrg;*.mdf;*.isz;*.ccd";
	}

	// Virtual CloneDrive
	if (m_DriveType == VCD) {
		return L"*.iso;*.ccd";
	}

	// Windows 8 VirtualDisk
	if (m_DriveType == WIN8) {
		return L"*.iso";
	}

	return NULL;
}

bool CDiskImage::CheckExtension(LPCWSTR pathName)
{
	CString ext = GetFileExt(pathName).MakeLower();

	if (CString(pathName).Right(7).MakeLower() == L".iso.wv") {
		ext = L".iso.wv";
	}

	// DAEMON Tools Lite
	if (m_DriveType == DTLITE && (ext == L".iso" || ext == L".nrg" || ext == L".mdf" || ext == L".isz" || ext == L".ccd" || ext == L".iso.wv")) {
		return true;
	}

	// Virtual CloneDrive
	if (m_DriveType == VCD && (ext == L".iso" || ext == L".ccd" || ext == L".iso.wv")) {
		return true;
	}

	// Windows 8 VirtualDisk
	if (m_DriveType == WIN8 && (ext == L".iso" || ext == L".iso.wv")) {
		return true;
	}

	return false;
}

WCHAR CDiskImage::MountDiskImage(LPCWSTR pathName)
{
	UnmountDiskImage();

	if (::PathFileExists(pathName)) {
		// DAEMON Tools Lite
		if (m_DriveType == DTLITE) {
			m_DriveLetter = MountDTLite(pathName);
		}
		// Virtual CloneDrive
		else if (m_DriveType == VCD) {
			m_DriveLetter = MountVCD(pathName);
		}
		// Windows 8 VirtualDisk
		else if (m_DriveType == WIN8) {
			m_DriveLetter = MountWin8(pathName);
		}

		if (m_DriveLetter) {
			// check for supported disc format
			CString anyfiles = CString(m_DriveLetter) + L":\\*.*";
			WIN32_FIND_DATA fd = { 0 };
			HANDLE hFind = FindFirstFile(anyfiles, &fd);
			if (hFind == INVALID_HANDLE_VALUE) {
				UnmountDiskImage();
			} else {
				FindClose(hFind);
			}
		}
	}

	return m_DriveLetter;
}

void CDiskImage::UnmountDiskImage()
{
	if (m_DriveLetter == 0) {
		return;
	}

	// DAEMON Tools Lite
	if (m_DriveType == DTLITE && m_dtdrive > dt_none) {
		SHELLEXECUTEINFO execinfo;
		memset(&execinfo, 0, sizeof(execinfo));
		execinfo.lpFile = m_dtlite_path;
		execinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		execinfo.nShow = SW_HIDE;
		execinfo.cbSize = sizeof(execinfo);

		CString parameters;
		if (m_dtdrive == dt_dt) {
			parameters.Format(L"-unmount dt, 0");
		} else if (m_dtdrive == dt_scsi) {
			parameters.Format(L"-unmount scsi, 0");
		}
		execinfo.lpParameters = parameters;

		if (ShellExecuteEx(&execinfo)) {
			// do not wait for execution result
			m_dtdrive = dt_none;
		}
	}

	// Virtual CloneDrive
	if (m_DriveType == VCD) {
		SHELLEXECUTEINFO execinfo;
		memset(&execinfo, 0, sizeof(execinfo));
		execinfo.lpFile = m_vcd_path;
		execinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		execinfo.nShow = SW_HIDE;
		execinfo.cbSize = sizeof(execinfo);
		execinfo.lpParameters = L"/d=0 /u";

		if (ShellExecuteEx(&execinfo)) {
			WaitForSingleObject(execinfo.hProcess, INFINITE);
		}
	}

	// Windows 8 VirtualDisk
	if (m_DriveType == WIN8) {
		SAFE_CLOSE_HANDLE(m_VHDHandle);
	}

	m_DriveLetter = 0;
}

WCHAR CDiskImage::MountWin8(LPCWSTR pathName)
{
	if (m_hVirtualDiskModule) {
		CString ISOVolumeName;

		VIRTUAL_STORAGE_TYPE vst;
		memset(&vst, 0, sizeof(VIRTUAL_STORAGE_TYPE));
		vst.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO;
		vst.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

		OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
		memset(&openParameters, 0, sizeof(OPEN_VIRTUAL_DISK_PARAMETERS));
		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;

		HANDLE tmpVHDHandle = INVALID_HANDLE_VALUE;
		DWORD ret_code = m_OpenVirtualDiskFunc(&vst, pathName, VIRTUAL_DISK_ACCESS_READ, OPEN_VIRTUAL_DISK_FLAG_NONE, &openParameters, &tmpVHDHandle);
		if (ret_code == ERROR_SUCCESS) {
			m_VHDHandle = tmpVHDHandle;

			ATTACH_VIRTUAL_DISK_PARAMETERS avdp;
			memset(&avdp, 0, sizeof(ATTACH_VIRTUAL_DISK_PARAMETERS));
			avdp.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
			ret_code = m_AttachVirtualDiskFunc(m_VHDHandle, NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY, 0, &avdp, NULL);

			if (ret_code == ERROR_SUCCESS) {
				WCHAR physicalDriveName[MAX_PATH] = L"";
				DWORD physicalDriveNameSize = _countof(physicalDriveName);

				ret_code = m_GetVirtualDiskPhysicalPathFunc(m_VHDHandle, &physicalDriveNameSize, physicalDriveName);
				if (ret_code == ERROR_SUCCESS) {
					WCHAR volumeNameBuffer[MAX_PATH] = L"";
					DWORD volumeNameBufferSize = _countof(volumeNameBuffer);
					HANDLE hVol = ::FindFirstVolumeW(volumeNameBuffer, volumeNameBufferSize);
					if (hVol != INVALID_HANDLE_VALUE) {
						do {
							size_t len = wcslen(volumeNameBuffer);
							if (volumeNameBuffer[len - 1] == '\\') {
								volumeNameBuffer[len - 1] = 0;
							}

							HANDLE volumeHandle = ::CreateFile(volumeNameBuffer,
																GENERIC_READ,
																FILE_SHARE_READ | FILE_SHARE_WRITE,
																NULL,
																OPEN_EXISTING,
																FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
																NULL);

							if (volumeHandle != INVALID_HANDLE_VALUE) {
								PSTORAGE_DEVICE_DESCRIPTOR	devDesc = {0};
								char						outBuf[512] = {0};
								STORAGE_PROPERTY_QUERY		query;

								memset(&query, 0, sizeof(STORAGE_PROPERTY_QUERY));
								query.PropertyId	= StorageDeviceProperty;
								query.QueryType		= PropertyStandardQuery;

								BOOL bIsVirtualDVD	= FALSE;
								ULONG bytesUsed		= 0;

								if (::DeviceIoControl(volumeHandle,
													  IOCTL_STORAGE_QUERY_PROPERTY,
													  &query,
													  sizeof(STORAGE_PROPERTY_QUERY),
													  &outBuf,
													  _countof(outBuf),
													  &bytesUsed,
													  NULL) && bytesUsed) {

									devDesc = (PSTORAGE_DEVICE_DESCRIPTOR)outBuf;
									if (devDesc->ProductIdOffset && outBuf[devDesc->ProductIdOffset]) {
										char* productID = DNew char[bytesUsed];
										memcpy(productID, &outBuf[devDesc->ProductIdOffset], bytesUsed);
										bIsVirtualDVD = !strncmp(productID, "Virtual DVD-ROM", 15);

										delete [] productID;
									}
								}

								if (bIsVirtualDVD) {
									STORAGE_DEVICE_NUMBER deviceInfo = {0};
									if (::DeviceIoControl(volumeHandle,
														  IOCTL_STORAGE_GET_DEVICE_NUMBER,
														  NULL,
														  0,
														  &deviceInfo,
														  sizeof(deviceInfo),
														  &bytesUsed,
														  NULL)) {

										CString tmp_physicalDriveName;
										tmp_physicalDriveName.Format(L"\\\\.\\CDROM%d", deviceInfo.DeviceNumber);
										if (physicalDriveName == tmp_physicalDriveName) {
											volumeNameBuffer[len - 1] = '\\';
											WCHAR VolumeName[MAX_PATH] = L"";
											DWORD VolumeNameSize = _countof(VolumeName);
											BOOL bRes = GetVolumePathNamesForVolumeNameW(volumeNameBuffer, VolumeName, VolumeNameSize, &VolumeNameSize);
											if (bRes) {
												ISOVolumeName = VolumeName;
											}
										}
									}
								}
								CloseHandle(volumeHandle);
							}
						} while(::FindNextVolumeW(hVol, volumeNameBuffer, volumeNameBufferSize) != FALSE && ISOVolumeName.IsEmpty());
						::FindVolumeClose(hVol);
					}
				}
			}
		}

		if (!ISOVolumeName.IsEmpty()) {
			return ISOVolumeName[0];
		}
	}

	SAFE_CLOSE_HANDLE(m_VHDHandle);

	return 0;
}

WCHAR CDiskImage::MountDTLite(LPCWSTR pathName)
{
	m_dtdrive = dt_none;

	SHELLEXECUTEINFO execinfo;
	memset(&execinfo, 0, sizeof(execinfo));
	execinfo.lpFile			= m_dtlite_path;
	execinfo.fMask			= SEE_MASK_NOCLOSEPROCESS;
	execinfo.nShow			= SW_HIDE;
	execinfo.cbSize			= sizeof(execinfo);

	DWORD ec = (DWORD)-1;
	execinfo.lpParameters = L"-get_letter dt, 0";
	if (!ShellExecuteEx(&execinfo)) {
		return 0;
	}
	WaitForSingleObject(execinfo.hProcess, INFINITE);
	if (GetExitCodeProcess(execinfo.hProcess, &ec) && ec < 26) {
		m_dtdrive = dt_dt;
	} else {
		ec = (DWORD)-1;
		execinfo.lpParameters = L"-get_letter scsi, 0";
		if (!ShellExecuteEx(&execinfo)) {
			return 0;
		}
		WaitForSingleObject(execinfo.hProcess, INFINITE);
		if (GetExitCodeProcess(execinfo.hProcess, &ec) && ec < 26) {
			m_dtdrive = dt_scsi;
		}
	}

	CString parameters;
	if (m_dtdrive == dt_dt) {
		parameters.Format(L"-mount dt, 0, \"%s\"", pathName);
	} else if (m_dtdrive == dt_scsi) {
		parameters.Format(L"-mount scsi, 0, \"%s\"", pathName);
	} else {
		return 0;
	}
	WCHAR letter = 'A' + ec;
	execinfo.lpParameters = parameters;

	ec = (DWORD)-1;
	if (ShellExecuteEx(&execinfo)) {
		WaitForSingleObject(execinfo.hProcess, INFINITE);
		if (GetExitCodeProcess(execinfo.hProcess, &ec) && ec == 0) {
			return letter;
		}
	}

	return 0;
}

WCHAR CDiskImage::MountVCD(LPCWSTR pathName)
{
	WCHAR letter = 0;

	DWORD VCD_drive = 0;
	CRegKey key;
	if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, L"Software\\Elaborate Bytes\\VirtualCloneDrive", KEY_READ)) {
		if (ERROR_SUCCESS == key.QueryDWORDValue(L"VCDDriveMask", VCD_drive)) { // get all VCD drives
			for (int i = 0; i < 26; i++) {
				if (VCD_drive & (1 << i)) {
					VCD_drive = (1 << i); // select the first drive
					letter = 'A' + i;
					break;
				}
			}
		}
		key.Close();
	}
	if (!VCD_drive) {
		return 0;
	}

	DWORD drives = GetLogicalDrives();
	if (!(drives & VCD_drive)) {
		return 0;
	}

	SHELLEXECUTEINFO execinfo;
	memset(&execinfo, 0, sizeof(execinfo));
	execinfo.lpFile			= m_vcd_path;
	execinfo.fMask			= SEE_MASK_NOCLOSEPROCESS;
	execinfo.nShow			= SW_HIDE;
	execinfo.cbSize			= sizeof(execinfo);

	CString parameters;
	parameters.Format(L"/d=0 \"%s\"", pathName);
	execinfo.lpParameters = parameters;

	DWORD ec = (DWORD)-1;
	if (ShellExecuteEx(&execinfo)) {
		WaitForSingleObject(execinfo.hProcess, INFINITE);
		if (GetExitCodeProcess(execinfo.hProcess, &ec) && ec == 0) {
			// wait until Virtual CloneDrive initialized.
			CString anyfiles = CString(letter) + L":\\*.*";
			WIN32_FIND_DATA fd = {0};
			HANDLE hFind;
			for (int i = 0; i < 100; i++) {
				hFind = FindFirstFile(anyfiles, &fd);
				if (hFind != INVALID_HANDLE_VALUE) {
					FindClose(hFind);
					Sleep(500); // sometimes Virtual CloneDrive need to give a little more time
					break;
				}
				Sleep(100);
			}

			return letter;
		}
	}

	return 0;
}
