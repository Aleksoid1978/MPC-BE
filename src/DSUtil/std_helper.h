/*
 * (C) 2018-2020 see Authors.txt
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

#pragma once

//#include <list>
//#include <guiddef.h>
#include <regex>

// this operator is needed to use GUID or CLSID as key in std::map
inline bool operator < (const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(GUID)) < 0;
}

// returns an iterator on the found element, or last if nothing is found
template <class T>
auto FindInListByPointer(std::list<T>& list, const T* p)
{
	auto it = list.begin();
	for (; it != list.end(); ++it) {
		if (&(*it) == p) {
			break;
		}
	}

	return it;
}

template <class T>
bool Contains(std::list<T>& list, const T& item)
{
	return std::find(list.cbegin(), list.cend(), item) != list.cend();
}

template <class T>
bool Contains(std::vector<T>& vector, const T& item)
{
	return std::find(vector.cbegin(), vector.cend(), item) != vector.cend();
}

template <class StringT, class T>
StringT RegExpParse(const T* szIn, const T* szRE)
{
	try {
		const std::basic_regex<T> regex(szRE);
		std::match_results<const T*> match;
		if (std::regex_search(szIn, match, regex) && match.size() == 2) {
			return StringT(match[1].first, match[1].length());
		}
	} catch (const std::regex_error& e) {
		UNREFERENCED_PARAMETER(e);
		DLog(L"RegExpParse(): regex error - '%S'", e.what());
		ASSERT(FALSE);
	}

	return StringT();
};
