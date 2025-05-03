/*
 * (C) 2018-2025 see Authors.txt
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

#include <regex>

// this operator is needed to use GUID or CLSID as key in std::map
inline bool operator < (const GUID & a, const GUID & b)
{
	return memcmp(&a, &b, sizeof(GUID)) < 0;
}

// helper function to create array without specifying size
// example: auto test_array = make_array<float>(0, 1, 3.14, 2,718, );
template <typename V, typename... T>
constexpr auto make_array(T&&... t)
-> std::array < V, sizeof...(T) >
{
	return { { std::forward<T>(t)... } };
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

// returns an iterator on the found element, or last if nothing is found
template <class T>
auto FindInListByPointer(std::list<std::unique_ptr<T>>& list, const T* p)
{
	auto it = list.begin();
	for (; it != list.end(); ++it) {
		if ((*it).get() == p) {
			break;
		}
	}

	return it;
}

// returns an iterator on the found element, or last if nothing is found
template <class T>
auto FindInListByPointer(std::list<CComPtr<T>>& list, const T* p)
{
	auto it = list.begin();
	for (; it != list.end(); ++it) {
		if ((*it).p == p) {
			break;
		}
	}

	return it;
}

template <class T>
[[nodiscard]] bool Contains(const T& container, const typename T::value_type& item)
{
	return std::find(container.cbegin(), container.cend(), item) != container.cend();
}

template <typename V, typename... T>
constexpr auto MakeArray(T&&... t)
->std::array < V, sizeof...(T) >
{
	return { { std::forward<T>(t)... } };
}

template <class T>
CStringT<T, StrTraitMFC<T>> RegExpParse(const T* szIn, const T* szRE)
{
	using StringT = CStringT<T, StrTraitMFC<T>>;
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

// helper type for the visitor #4
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;