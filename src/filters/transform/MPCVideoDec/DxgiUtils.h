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

#pragma once

// CDXGIFactory1

class CDXGIFactory1
{
public:
	inline static CDXGIFactory1& GetInstance()
	{
		if (nullptr == m_pInstance.get())
			m_pInstance.reset(new CDXGIFactory1());
		return *m_pInstance;
	}

	IDXGIFactory1* GetFactory() const;

protected:
	CComPtr<IDXGIFactory1> m_pDXGIFactory1;

private:
	CDXGIFactory1();   // Private because singleton
	static std::unique_ptr<CDXGIFactory1> m_pInstance;
};

////////////////

HRESULT GetDxgiAdapters(std::list<DXGI_ADAPTER_DESC>& dxgi_adapters);
