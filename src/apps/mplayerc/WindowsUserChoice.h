/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * (C) 2022 see Authors.txt
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

#ifndef SHELL_WINDOWSUSERCHOICE_H__
#define SHELL_WINDOWSUSERCHOICE_H__

#include <windows.h>
#include <memory>

namespace UserChoice {

/*
 * Result from CheckUserChoiceHash()
 *
 * NOTE: Currently the only positive result is OK_V1 , but the enum
 * could be extended to indicate different versions of the hash.
 */
enum class CheckUserChoiceHashResult {
  OK_V1,         // Matched the current version of the hash (as of Win10 20H2).
  ERR_MISMATCH,  // The hash did not match.
  ERR_OTHER,     // Error reading or generating the hash.
};

/*
 * Generate a UserChoice Hash, compare it with the one that is stored.
 *
 * See comments on CheckBrowserUserChoiceHashes(), which calls this to check
 * each of the browser associations.
 *
 * @param aExt      File extension or protocol association to check
 * @param aUserSid  String SID of the current user
 *
 * @return Result of the check, see CheckUserChoiceHashResult
 */
CheckUserChoiceHashResult CheckUserChoiceHash(const wchar_t* aExt,
                                              const wchar_t* aUserSid);

/*
 * Get the registry path for the given association, file extension or protocol.
 *
 * @return The path, or nullptr on failure.
 */
std::unique_ptr<wchar_t[]> GetAssociationKeyPath(const wchar_t* aExt);

/*
 * Get the current user's SID
 *
 * @return String SID for the user of the current process, nullptr on failure.
 */
std::unique_ptr<wchar_t[]> GetCurrentUserStringSid();

/*
 * Generate the UserChoice Hash
 *
 * @param aExt          file extension or protocol being registered
 * @param aUserSid      string SID of the current user
 * @param aProgId       ProgId to associate with aExt
 * @param aTimestamp    approximate write time of the UserChoice key (within
 *                      the same minute)
 *
 * @return UserChoice Hash, nullptr on failure.
 */
std::unique_ptr<wchar_t[]> GenerateUserChoiceHash(const wchar_t* aExt,
                                                     const wchar_t* aUserSid,
                                                     const wchar_t* aProgId,
                                                     SYSTEMTIME aTimestamp);

/*
 * Build a ProgID from a base and AUMI
 *
 * @param aProgIDBase   A base, such as FirefoxHTML or FirefoxURL
 * @param aAumi         The AUMI of the installation
 *
 * @return Formatted ProgID.
 */
std::unique_ptr<wchar_t[]> FormatProgID(const wchar_t* aProgIDBase,
                                           const wchar_t* aAumi);

/*
 * Check that the given ProgID exists in HKCR
 *
 * @return true if it could be opened for reading, false otherwise.
 */
bool CheckProgIDExists(const wchar_t* aProgID);

} // namespace UserChoice

#endif  // SHELL_WINDOWSUSERCHOICE_H__
