/*
 * (C) 2020 see Authors.txt
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

#define RAPIDJSON_SSE2
#include "./rapidjson/document.h"

template <typename T>
inline bool getJsonValue(const rapidjson::Value& jsonValue, const char* name, T& value)
{
	if (const auto& it = jsonValue.FindMember(name); it != jsonValue.MemberEnd()) {
		if constexpr (std::is_same_v<T, int>) {
			if (it->value.IsInt()) {
				value = it->value.GetInt();
				return true;
			}
		} else if constexpr (std::is_same_v<T, float>) {
			if (it->value.IsFloat()) {
				value = it->value.GetFloat();
				return true;
			}
		} else if constexpr (std::is_same_v<T, CStringA>) {
			if (it->value.IsString()) {
				value = it->value.GetString();
				return true;
			}
		} else if constexpr (std::is_same_v<T, CString>) {
			if (it->value.IsString()) {
				value = UTF8ToWStr(it->value.GetString());
				return true;
			}
		}
	}

	return false;
};

#define isJsonArray(jsonArray, jsonValue, name)   const auto& jsonArray = jsonValue.FindMember(name); jsonArray != jsonValue.MemberEnd() && jsonArray->value.IsArray() && !jsonArray->value.Empty()
#define isJsonObject(jsonObject, jsonValue, name) const auto& jsonObject = jsonValue.FindMember(name); jsonObject != jsonValue.MemberEnd() && jsonObject->value.IsObject()
