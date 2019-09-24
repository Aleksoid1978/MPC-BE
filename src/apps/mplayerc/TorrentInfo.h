/*
 * (C) 2019 see Authors.txt
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
 * using code from TorrentInfo plug-in for FAR 3.0 https://sourceforge.net/projects/farplugs/
 */

#pragma once

struct bt_node;

class CTorrentInfo
{
public:
	CTorrentInfo() = default;
	~CTorrentInfo();

	const bool Read(const wchar_t* fileName);
	const std::wstring Magnet() const;

private:
	bt_node* Decode();
	const __int64 ReadInteger();
	const std::string ReadString();
	const bt_node* Search(const char* nodeName, const bt_node* nodeParent) const;

	const std::wstring CalcInfoHash() const;
	void GetAnnounceList(const bt_node* nodeAnnounce, std::list<std::string>& list) const;

private:
	bt_node*                   m_root = nullptr;
	size_t                     m_offset = 0;
	std::vector<unsigned char> m_data;
};
