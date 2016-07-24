/*
 * (C) 2006-2016 see Authors.txt
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
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdaiface.h>
#include "PPageCapture.h"


static struct cc_t {
	long code;
	AnalogVideoStandard standard;
	const char* str;
} s_countrycodes[] = {
	{  1, AnalogVideo_NTSC_M,   "USA"},
	//{1, AnalogVideo_NTSC_M,   "Anguilla"},
	//{1, AnalogVideo_NTSC_M,   "Antigua"},
	//{1, AnalogVideo_NTSC_M,   "Bahamas"},
	//{1, AnalogVideo_NTSC_M,   "Barbados"},
	//{1, AnalogVideo_NTSC_M,   "Bermuda"},
	//{1, AnalogVideo_NTSC_M,   "British Virgin Islands"},
	//{1, AnalogVideo_NTSC_M,   "Canada"},
	//{1, AnalogVideo_NTSC_M,   "Cayman Islands"},
	//{1, AnalogVideo_NTSC_M,   "Dominica"},
	//{1, AnalogVideo_NTSC_M,   "Dominican Republic"},
	//{1, AnalogVideo_NTSC_M,   "Grenada"},
	//{1, AnalogVideo_NTSC_M,   "Jamaica"},
	//{1, AnalogVideo_NTSC_M,   "Montserrat"},
	//{1, AnalogVideo_NTSC_M,   "Nevis"},
	//{1, AnalogVideo_NTSC_M,   "St. Kitts"},
	//{1, AnalogVideo_NTSC_M,   "St. Vincent and the Grenadines"},
	//{1, AnalogVideo_NTSC_M,   "Trinidad and Tobago"},
	//{1, AnalogVideo_NTSC_M,   "Turks and Caicos Islands"},
	//{1, AnalogVideo_NTSC_M,   "Barbuda"},
	//{1, AnalogVideo_NTSC_M,   "Puerto Rico"},
	//{1, AnalogVideo_NTSC_M,   "Saint Lucia"},
	//{1, AnalogVideo_NTSC_M,   "United States Virgin Islands"},
	{  2, AnalogVideo_NTSC_M,   "Canada"},
	{  7, AnalogVideo_SECAM_D,  "Russia"},
	//{7, AnalogVideo_SECAM_D,  "Kazakhstan"},
	//{7, AnalogVideo_SECAM_D,  "Kyrgyzstan"},
	//{7, AnalogVideo_SECAM_D,  "Tajikistan"},
	//{7, AnalogVideo_SECAM_D,  "Turkmenistan"},
	//{7, AnalogVideo_SECAM_D,  "Uzbekistan"},
	{ 20, AnalogVideo_SECAM_B,  "Egypt"},
	{ 27, AnalogVideo_PAL_I,    "South Africa"},
	{ 30, AnalogVideo_SECAM_B,  "Greece"},
	{ 31, AnalogVideo_PAL_B,    "Netherlands"},
	{ 32, AnalogVideo_PAL_B,    "Belgium"},
	{ 33, AnalogVideo_SECAM_L,  "France"},
	{ 34, AnalogVideo_PAL_B,    "Spain"},
	{ 36, AnalogVideo_PAL_B,    "Hungary"},
	{ 39, AnalogVideo_PAL_B,    "Italy"},
	{ 39, AnalogVideo_PAL_B,    "Vatican City"},
	{ 40, AnalogVideo_PAL_D,    "Romania"},
	{ 41, AnalogVideo_PAL_B,    "Switzerland"},
	{ 41, AnalogVideo_PAL_B,    "Liechtenstein"},
	{ 43, AnalogVideo_PAL_B,    "Austria"},
	{ 44, AnalogVideo_PAL_I,    "United Kingdom"},
	{ 45, AnalogVideo_PAL_B,    "Denmark"},
	{ 46, AnalogVideo_PAL_B,    "Sweden"},
	{ 47, AnalogVideo_PAL_B,    "Norway"},
	{ 48, AnalogVideo_PAL_B,    "Poland"},
	{ 49, AnalogVideo_PAL_B,    "Germany"},
	{ 51, AnalogVideo_NTSC_M,   "Peru"},
	{ 52, AnalogVideo_NTSC_M,   "Mexico"},
	{ 53, AnalogVideo_NTSC_M,   "Cuba"},
	{ 53, AnalogVideo_NTSC_M,   "Guantanamo Bay"},
	{ 54, AnalogVideo_PAL_N,    "Argentina"},
	{ 55, AnalogVideo_PAL_M,    "Brazil"},
	{ 56, AnalogVideo_NTSC_M,   "Chile"},
	{ 57, AnalogVideo_NTSC_M,   "Colombia"},
	{ 58, AnalogVideo_NTSC_M,   "Bolivarian Republic of Venezuela"},
	{ 60, AnalogVideo_PAL_B,    "Malaysia"},
	{ 61, AnalogVideo_PAL_B,    "Australia"},
	//{61, AnalogVideo_NTSC_M, "Cocos-Keeling Islands"},
	{ 62, AnalogVideo_PAL_B,    "Indonesia"},
	{ 63, AnalogVideo_NTSC_M,   "Philippines"},
	{ 64, AnalogVideo_PAL_B,    "New Zealand"},
	{ 65, AnalogVideo_PAL_B,    "Singapore"},
	{ 66, AnalogVideo_PAL_B,    "Thailand"},
	{ 81, AnalogVideo_NTSC_M_J, "Japan"},
	{ 82, AnalogVideo_NTSC_M,   "Korea (South)"},
	{ 84, AnalogVideo_NTSC_M,   "Vietnam"},
	{ 86, AnalogVideo_PAL_D,    "China"},
	{ 90, AnalogVideo_PAL_B,    "Turkey"},
	{ 91, AnalogVideo_PAL_B,    "India"},
	{ 92, AnalogVideo_PAL_B,    "Pakistan"},
	{ 93, AnalogVideo_PAL_B,    "Afghanistan"},
	{ 94, AnalogVideo_PAL_B,    "Sri Lanka"},
	{ 95, AnalogVideo_NTSC_M,   "Myanmar"},
	{ 98, AnalogVideo_SECAM_B,  "Iran"},
	{212, AnalogVideo_SECAM_B,  "Morocco"},
	{213, AnalogVideo_PAL_B,    "Algeria"},
	{216, AnalogVideo_SECAM_B,  "Tunisia"},
	{218, AnalogVideo_SECAM_B,  "Libya"},
	{220, AnalogVideo_SECAM_K,  "Gambia"},
	{221, AnalogVideo_SECAM_K,  "Senegal Republic"},
	{222, AnalogVideo_SECAM_B,  "Mauritania"},
	{223, AnalogVideo_SECAM_K,  "Mali"},
	{224, AnalogVideo_SECAM_K,  "Guinea"},
	{225, AnalogVideo_SECAM_K,  "Cote D'Ivoire"},
	{226, AnalogVideo_SECAM_K,  "Burkina Faso"},
	{227, AnalogVideo_SECAM_K,  "Niger"},
	{228, AnalogVideo_SECAM_K,  "Togo"},
	{229, AnalogVideo_SECAM_K,  "Benin"},
	{230, AnalogVideo_SECAM_B,  "Mauritius"},
	{231, AnalogVideo_PAL_B,    "Liberia"},
	{232, AnalogVideo_PAL_B,    "Sierra Leone"},
	{233, AnalogVideo_PAL_B,    "Ghana"},
	{234, AnalogVideo_PAL_B,    "Nigeria"},
	{235, AnalogVideo_PAL_B,    "Chad"},
	{236, AnalogVideo_PAL_B,    "Central African Republic"},
	{237, AnalogVideo_PAL_B,    "Cameroon"},
	{238, AnalogVideo_NTSC_M,   "Cape Verde Islands"},
	{239, AnalogVideo_PAL_B,    "Sao Tome and Principe"},
	{240, AnalogVideo_SECAM_B,  "Equatorial Guinea"},
	{241, AnalogVideo_SECAM_K,  "Gabon"},
	{242, AnalogVideo_SECAM_D,  "Congo"},
	{243, AnalogVideo_SECAM_K,  "Congo(DRC)"},
	{244, AnalogVideo_PAL_I,    "Angola"},
	{245, AnalogVideo_NTSC_M,   "Guinea-Bissau"},
	{246, AnalogVideo_NTSC_M,   "Diego Garcia"},
	{247, AnalogVideo_NTSC_M,   "Ascension Island"},
	{248, AnalogVideo_PAL_B,    "Seychelles Islands"},
	{249, AnalogVideo_PAL_B,    "Sudan"},
	{250, AnalogVideo_PAL_B,    "Rwanda"},
	{251, AnalogVideo_PAL_B,    "Ethiopia"},
	{252, AnalogVideo_PAL_B,    "Somalia"},
	{253, AnalogVideo_SECAM_K,  "Djibouti"},
	{254, AnalogVideo_PAL_B,    "Kenya"},
	{255, AnalogVideo_PAL_B,    "Tanzania"},
	{256, AnalogVideo_PAL_B,    "Uganda"},
	{257, AnalogVideo_SECAM_K,  "Burundi"},
	{258, AnalogVideo_PAL_B,    "Mozambique"},
	{260, AnalogVideo_PAL_B,    "Zambia"},
	{261, AnalogVideo_SECAM_K,  "Madagascar"},
	{262, AnalogVideo_SECAM_K,  "Reunion Island"},
	{263, AnalogVideo_PAL_B,    "Zimbabwe"},
	{264, AnalogVideo_PAL_I,    "Namibia"},
	{265, AnalogVideo_NTSC_M,   "Malawi"},
	{266, AnalogVideo_PAL_I,    "Lesotho"},
	{267, AnalogVideo_SECAM_K,  "Botswana"},
	{268, AnalogVideo_PAL_B,    "Swaziland"},
	{269, AnalogVideo_SECAM_K,  "Mayotte Island"},
	//{269, AnalogVideo_NTSC_M, "Comoros"},
	{290, AnalogVideo_NTSC_M,   "St. Helena"},
	{291, AnalogVideo_NTSC_M,   "Eritrea"},
	{297, AnalogVideo_NTSC_M,   "Aruba"},
	{298, AnalogVideo_PAL_B,    "Faroe Islands"},
	{299, AnalogVideo_NTSC_M,   "Greenland"},
	{350, AnalogVideo_PAL_B,    "Gibraltar"},
	{351, AnalogVideo_PAL_B,    "Portugal"},
	{352, AnalogVideo_PAL_B,    "Luxembourg"},
	{353, AnalogVideo_PAL_I,    "Ireland"},
	{354, AnalogVideo_PAL_B,    "Iceland"},
	{355, AnalogVideo_PAL_B,    "Albania"},
	{356, AnalogVideo_PAL_B,    "Malta"},
	{357, AnalogVideo_PAL_B,    "Cyprus"},
	{358, AnalogVideo_PAL_B,    "Finland"},
	{359, AnalogVideo_SECAM_D,  "Bulgaria"},
	{370, AnalogVideo_PAL_B,    "Lithuania"},
	{371, AnalogVideo_SECAM_D,  "Latvia"},
	{372, AnalogVideo_PAL_B,    "Estonia"},
	{373, AnalogVideo_SECAM_D,  "Moldova"},
	{374, AnalogVideo_SECAM_D,  "Armenia"},
	{375, AnalogVideo_SECAM_D,  "Belarus"},
	{376, AnalogVideo_NTSC_M,   "Andorra"},
	{377, AnalogVideo_SECAM_G,  "Monaco"},
	{378, AnalogVideo_PAL_B,    "San Marino"},
	{380, AnalogVideo_SECAM_D,  "Ukraine"},
	{381, AnalogVideo_PAL_B,    "Serbia and Montenegro"},
	{385, AnalogVideo_PAL_B,    "Croatia"},
	{386, AnalogVideo_PAL_B,    "Slovenia"},
	{387, AnalogVideo_PAL_B,    "Bosnia and Herzegovina"},
	{389, AnalogVideo_PAL_B,    "F.Y.R.O.M. (Former Yugoslav Republic of Macedonia)"},
	{420, AnalogVideo_PAL_D,    "Czech Republic"},
	{421, AnalogVideo_PAL_B,    "Slovak Republic"},
	{500, AnalogVideo_PAL_I,    "Falkland Islands (Islas Malvinas)"},
	{501, AnalogVideo_NTSC_M,   "Belize"},
	{502, AnalogVideo_NTSC_M,   "Guatemala"},
	{503, AnalogVideo_NTSC_M,   "El Salvador"},
	{504, AnalogVideo_NTSC_M,   "Honduras"},
	{505, AnalogVideo_NTSC_M,   "Nicaragua"},
	{506, AnalogVideo_NTSC_M,   "Costa Rica"},
	{507, AnalogVideo_NTSC_M,   "Panama"},
	{508, AnalogVideo_SECAM_K,  "St. Pierre and Miquelon"},
	{509, AnalogVideo_NTSC_M,   "Haiti"},
	{590, AnalogVideo_SECAM_K,  "Guadeloupe"},
	//{590, AnalogVideo_NTSC_M, "French Antilles"},
	{591, AnalogVideo_PAL_N,    "Bolivia"},
	{592, AnalogVideo_SECAM_K,  "Guyana"},
	{593, AnalogVideo_NTSC_M,   "Ecuador"},
	{594, AnalogVideo_SECAM_K,  "French Guiana"},
	{595, AnalogVideo_PAL_N,    "Paraguay"},
	{596, AnalogVideo_SECAM_K,  "Martinique"},
	{597, AnalogVideo_NTSC_M,   "Suriname"},
	{598, AnalogVideo_PAL_N,    "Uruguay"},
	{599, AnalogVideo_NTSC_M,   "Netherlands Antilles"},
	{670, AnalogVideo_NTSC_M,   "Saipan Island"},
	//{670, AnalogVideo_NTSC_M, "Rota Island"},
	//{670, AnalogVideo_NTSC_M, "Tinian Island"},
	{671, AnalogVideo_NTSC_M,   "Guam"},
	{672, AnalogVideo_NTSC_M,   "Christmas Island"},
	{672, AnalogVideo_NTSC_M,   "Australian Antarctic Territory"},
	//{672, AnalogVideo_PAL_B,  "Norfolk Island"},
	{673, AnalogVideo_PAL_B,    "Brunei"},
	{674, AnalogVideo_NTSC_M,   "Nauru"},
	{675, AnalogVideo_PAL_B,    "Papua New Guinea"},
	{676, AnalogVideo_NTSC_M,   "Tonga"},
	{677, AnalogVideo_NTSC_M,   "Solomon Islands"},
	{678, AnalogVideo_NTSC_M,   "Vanuatu"},
	{679, AnalogVideo_NTSC_M,   "Fiji Islands"},
	{680, AnalogVideo_NTSC_M,   "Palau"},
	{681, AnalogVideo_SECAM_K,  "Wallis and Futuna Islands"},
	{682, AnalogVideo_PAL_B,    "Cook Islands"},
	{683, AnalogVideo_NTSC_M,   "Niue"},
	{684, AnalogVideo_NTSC_M,   "Territory of American Samoa"},
	{685, AnalogVideo_PAL_B,    "Samoa"},
	{686, AnalogVideo_PAL_B,    "Kiribati Republic"},
	{687, AnalogVideo_SECAM_K,  "New Caledonia"},
	{688, AnalogVideo_NTSC_M,   "Tuvalu"},
	{689, AnalogVideo_SECAM_K,  "French Polynesia"},
	{690, AnalogVideo_NTSC_M,   "Tokelau"},
	{691, AnalogVideo_NTSC_M,   "Micronesia"},
	{692, AnalogVideo_NTSC_M,   "Marshall Islands"},
	{850, AnalogVideo_SECAM_D,  "Korea (North)"},
	{852, AnalogVideo_PAL_I,    "Hong Kong SAR"},
	{853, AnalogVideo_PAL_I,    "Macao SAR"},
	{855, AnalogVideo_PAL_B,    "Cambodia"},
	{856, AnalogVideo_PAL_B,    "Laos"},
	{871, AnalogVideo_NTSC_M,   "INMARSAT (Atlantic-East)"},
	{872, AnalogVideo_NTSC_M,   "INMARSAT (Pacific)"},
	{873, AnalogVideo_NTSC_M,   "INMARSAT (Indian)"},
	{874, AnalogVideo_NTSC_M,   "INMARSAT (Atlantic-West)"},
	{880, AnalogVideo_PAL_B,    "Bangladesh"},
	{886, AnalogVideo_NTSC_M,   "Taiwan"},
	{960, AnalogVideo_PAL_B,    "Maldives"},
	{961, AnalogVideo_SECAM_B,  "Lebanon"},
	{962, AnalogVideo_PAL_B,    "Jordan"},
	{963, AnalogVideo_SECAM_B,  "Syria"},
	{964, AnalogVideo_SECAM_B,  "Iraq"},
	{965, AnalogVideo_PAL_B,    "Kuwait"},
	{966, AnalogVideo_SECAM_B,  "Saudi Arabia"},
	{967, AnalogVideo_PAL_B,    "Yemen"},
	{968, AnalogVideo_PAL_B,    "Oman"},
	{971, AnalogVideo_PAL_B,    "United Arab Emirates"},
	{972, AnalogVideo_PAL_B,    "Israel"},
	{973, AnalogVideo_PAL_B,    "Bahrain"},
	{974, AnalogVideo_PAL_B,    "Qatar"},
	{975, AnalogVideo_NTSC_M,   "Bhutan"},
	{976, AnalogVideo_SECAM_D,  "Mongolia"},
	{977, AnalogVideo_PAL_B,    "Nepal"},
	{994, AnalogVideo_SECAM_D,  "Azerbaijan"},
	{995, AnalogVideo_SECAM_D,  "Georgia"},
};

// CPPageCapture dialog

IMPLEMENT_DYNAMIC(CPPageCapture, CPPageBase)

CPPageCapture::CPPageCapture()
	: CPPageBase(CPPageCapture::IDD, CPPageCapture::IDD)
	, m_iDefaultDevice(0)
{

}

CPPageCapture::~CPPageCapture()
{
}

void CPPageCapture::DoDataExchange(CDataExchange* pDX)
{
	CPPageBase::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_cbAnalogVideo);
	DDX_Control(pDX, IDC_COMBO2, m_cbAnalogAudio);
	DDX_Control(pDX, IDC_COMBO9, m_cbAnalogCountry);
	DDX_Control(pDX, IDC_COMBO4, m_cbDigitalNetworkProvider);
	DDX_Control(pDX, IDC_COMBO5, m_cbDigitalTuner);
	DDX_Control(pDX, IDC_COMBO3, m_cbDigitalReceiver);
	DDX_Radio(pDX, IDC_RADIO1, m_iDefaultDevice);
}

BEGIN_MESSAGE_MAP(CPPageCapture, CPPageBase)
END_MESSAGE_MAP()

// CPPageCapture message handlers

BOOL CPPageCapture::OnInitDialog()
{
	__super::OnInitDialog();

	SetCursor(m_hWnd, IDC_COMBO1, IDC_HAND);

	CAppSettings& s = AfxGetAppSettings();

	FindAnalogDevices();
	FindDigitalDevices();

	m_iDefaultDevice = s.iDefaultCaptureDevice;

	UpdateData(FALSE);

	return TRUE;
}

void CPPageCapture::FindAnalogDevices()
{
	CAppSettings& s = AfxGetAppSettings();
	int iSel = 0;

	// List video devised
	BeginEnumSysDev(CLSID_VideoInputDeviceCategory, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(L"FriendlyName"), &var, NULL);
		int i = m_cbAnalogVideo.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;

		if (SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName))) {
			m_vidnames.Add(CString(strName));

			if (s.strAnalogVideo == CString(strName)) {
				iSel = i;
			}

			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if (m_cbAnalogVideo.GetCount()) {
		m_cbAnalogVideo.SetCurSel(iSel);
	}

	{
		int i = m_cbAnalogAudio.AddString(L"<Video Capture Device>");
		m_audnames.Add(_T(""));

		if (s.strAnalogAudio.IsEmpty()) {
			iSel = i;
		}
	}

	// List audio devised
	iSel = 0;
	BeginEnumSysDev(CLSID_AudioInputDeviceCategory, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(L"FriendlyName"), &var, NULL);
		int i = m_cbAnalogAudio.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;

		if (SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName))) {
			m_audnames.Add(CString(strName));

			if (s.strAnalogAudio == CString(strName)) {
				iSel = i;
			}

			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if (m_cbAnalogAudio.GetCount()) {
		m_cbAnalogAudio.SetCurSel(iSel);
	}

	// Fill country
	iSel = 0;
	for (int j = 0; j < _countof(s_countrycodes); j++) {
		const char* standard;
		switch (s_countrycodes[j].standard) {
			case AnalogVideo_NTSC_M:      standard = "NTSC M";      break;
			case AnalogVideo_NTSC_M_J:    standard = "NTSC M J";    break;
			case AnalogVideo_NTSC_433:    standard = "NTSC 433";    break;
			case AnalogVideo_PAL_B:       standard = "PAL B";       break;
			case AnalogVideo_PAL_D:       standard = "PAL D";       break;
			case AnalogVideo_PAL_G:       standard = "PAL G";       break;
			case AnalogVideo_PAL_H:       standard = "PAL H";       break;
			case AnalogVideo_PAL_I:       standard = "PAL I";       break;
			case AnalogVideo_PAL_M:       standard = "PAL M";       break;
			case AnalogVideo_PAL_N:       standard = "PAL N";       break;
			case AnalogVideo_PAL_60:      standard = "PAL 60";      break;
			case AnalogVideo_SECAM_B:     standard = "SECAM B";     break;
			case AnalogVideo_SECAM_D:     standard = "SECAM D";     break;
			case AnalogVideo_SECAM_G:     standard = "SECAM G";     break;
			case AnalogVideo_SECAM_H:     standard = "SECAM H";     break;
			case AnalogVideo_SECAM_K:     standard = "SECAM K";     break;
			case AnalogVideo_SECAM_K1:    standard = "SECAM K1";    break;
			case AnalogVideo_SECAM_L:     standard = "SECAM L";     break;
			case AnalogVideo_SECAM_L1:    standard = "SECAM L1";    break;
			case AnalogVideo_PAL_N_COMBO: standard = "PAL N COMBO"; break;
		}

		CString str;
		str.Format(L"%ld - %S - %S", s_countrycodes[j].code, s_countrycodes[j].str, standard);

		int i = m_cbAnalogCountry.AddString(str);
		m_cbAnalogCountry.SetItemDataPtr(i, &s_countrycodes[j]);

		if (s.iAnalogCountry == s_countrycodes[j].code) {
			iSel = i;
		}
	}

	if (m_cbAnalogCountry.GetCount()) {
		m_cbAnalogCountry.SetCurSel(iSel);
	}
}

void CPPageCapture::FindDigitalDevices()
{
	CAppSettings&	s	 = AfxGetAppSettings();
	int				iSel = 0;

	BeginEnumSysDev(KSCATEGORY_BDA_NETWORK_PROVIDER, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		int i = m_cbDigitalNetworkProvider.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;

		if (SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName))) {
			m_providernames.Add(CString(strName));

			if (s.strBDANetworkProvider == CString(strName)) {
				iSel = i;
			}

			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if (m_cbDigitalNetworkProvider.GetCount()) {
		m_cbDigitalNetworkProvider.SetCurSel(iSel);
	}

	iSel = 0;
	BeginEnumSysDev(KSCATEGORY_BDA_NETWORK_TUNER, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		int i = m_cbDigitalTuner.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;

		if (SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName))) {
			m_tunernames.Add(CString(strName));

			if (s.strBDATuner == CString(strName)) {
				iSel = i;
			}

			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if (m_cbDigitalTuner.GetCount()) {
		m_cbDigitalTuner.SetCurSel(iSel);
	}

	iSel = 0;
	BeginEnumSysDev(KSCATEGORY_BDA_RECEIVER_COMPONENT, pMoniker) {
		CComPtr<IPropertyBag> pPB;
		pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB);

		CComVariant var;
		pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);
		int i = m_cbDigitalReceiver.AddString(CString(var.bstrVal));

		LPOLESTR strName = NULL;

		if (SUCCEEDED(pMoniker->GetDisplayName(NULL, NULL, &strName))) {
			m_receivernames.Add(CString(strName));

			if (s.strBDAReceiver == CString(strName)) {
				iSel = i;
			}

			CoTaskMemFree(strName);
		}
	}
	EndEnumSysDev

	if (m_cbDigitalReceiver.GetCount()) {
		m_cbDigitalReceiver.SetCurSel(iSel);
	}
}

BOOL CPPageCapture::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.iDefaultCaptureDevice = m_iDefaultDevice;

	if (m_cbAnalogVideo.GetCurSel()>=0) {
		s.strAnalogVideo = m_vidnames[m_cbAnalogVideo.GetCurSel()];
	}

	if (m_cbAnalogAudio.GetCurSel()>=0) {
		s.strAnalogAudio = m_audnames[m_cbAnalogAudio.GetCurSel()];
	}

	if (m_cbAnalogCountry.GetCurSel()>=0) {
		s.iAnalogCountry = ((cc_t*)m_cbAnalogCountry.GetItemDataPtr(m_cbAnalogCountry.GetCurSel()))->code;
	}

	if (m_cbDigitalNetworkProvider.GetCurSel()>=0) {
		s.strBDANetworkProvider = m_providernames[m_cbDigitalNetworkProvider.GetCurSel()];
	}

	if (m_cbDigitalTuner.GetCurSel()>=0) {
		s.strBDATuner = m_tunernames[m_cbDigitalTuner.GetCurSel()];
	}

	if (m_cbDigitalReceiver.GetCurSel()>=0) {
		s.strBDAReceiver = m_receivernames[m_cbDigitalReceiver.GetCurSel()];
	}

	return __super::OnApply();
}
