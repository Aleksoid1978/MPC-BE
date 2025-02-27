/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2023 see Authors.txt
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
#include "MainFrm.h"
#include "PPageDVD.h"
#include "DSUtil/SysVersion.h"

struct {
	WORD lcid;
	LPCSTR iso639_1;
	LPCSTR name;
}
const dvdlangs[] = {
	{0x0436, "af", "Afrikaans"      },
	{0x045e, "am", "Amharic"        },
	{0x0401, "ar", "Arabic"         },
	{0x044d, "as", "Assamese"       },
	{0x042c, "az", "Azerbaijani"    },
	{0x046d, "ba", "Bashkir"        },
	{0x0423, "be", "Belarusian"     },
	{0x0402, "bg", "Bulgarian"      },
	{0x0845, "bn", "Bengali"        },
	{0x0451, "bo", "Tibetan"        },
	{0x047e, "br", "Breton"         },
	{0x0403, "ca", "Catalan"        },
	{0x0483, "co", "Corsican"       },
	{0x0405, "cs", "Czech"          },
	{0x0452, "cy", "Welsh"          },
	{0x0406, "da", "Danish"         },
	{0x0407, "de", "German"         },
	{0x0c51, "dz", "Dzongkha"       },
	{0x0408, "el", "Greek"          },
	{0x0409, "en", "English"        },
	{0x0c0a, "es", "Spanish"        },
	{0x0425, "et", "Estonian"       },
	{0x042d, "eu", "Basque"         },
	{0x0429, "fa", "Persian"        },
	{0x040b, "fi", "Finnish"        },
	{0x0438, "fo", "Faroese"        },
	{0x040c, "fr", "French"         },
	{0x0462, "fy", "Western Frisian"},
	{0x083c, "ga", "Irish"          },
	{0x0491, "gd", "Scottish Gaelic"},
	{0x0456, "gl", "Galician"       },
	{0x0474, "gn", "Guarani"        },
	{0x0447, "gu", "Gujarati"       },
	{0x0468, "ha", "Hausa"          },
	{0x040d, "he", "Hebrew"         },
	{0x0439, "hi", "Hindi"          },
	{0x041a, "hr", "Croatian"       },
	{0x040e, "hu", "Hungarian"      },
	{0x042b, "hy", "Armenian"       },
	{0x0421, "id", "Indonesian"     },
	{0x040f, "is", "Icelandic"      },
	{0x0410, "it", "Italian"        },
	{0x085d, "iu", "Inuktitut"      },
	{0x0411, "ja", "Japanese"       },
	{0x0437, "ka", "Georgian"       },
	{0x043f, "kk", "Kazakh"         },
	{0x046f, "kl", "Kalaallisut"    },
	{0x0453, "km", "Khmer"          },
	{0x044b, "kn", "Kannada"        },
	{0x0412, "ko", "Korean"         },
	{0x0492, "ku", "Kurdish"        },
	{0x0440, "ky", "Kyrgyz"         },
	{0x0476, "la", "Latin"          },
	{0x046e, "lb", "Luxembourgish"  },
	{0x0454, "lo", "Lao"            },
	{0x0427, "lt", "Lithuanian"     },
	{0x0426, "lv", "Latvian"        },
	{0x0481, "mi", "Maori"          },
	{0x042f, "mk", "Macedonian"     },
	{0x044c, "ml", "Malayalam"      },
	{0x0450, "mn", "Mongolian"      },
	{0x044e, "mr", "Marathi"        },
	{0x043e, "ms", "Malay"          },
	{0x043a, "mt", "Maltese"        },
	{0x0455, "my", "Burmese"        },
	{0x0461, "ne", "Nepali"         },
	{0x0413, "nl", "Dutch"          },
	{0x0414, "no", "Norwegian"      },
	{0x0482, "oc", "Occitan"        },
	{0x0472, "om", "Oromo"          },
	{0x0448, "or", "Oriya"          },
	{0x0446, "pa", "Punjabi"        },
	{0x0415, "pl", "Polish"         },
	{0x0463, "ps", "Pashto"         },
	{0x0416, "pt", "Portuguese"     },
	{0x046b, "qu", "Quechua"        },
	{0x0417, "rm", "Romansh"        },
	{0x0418, "ro", "Romanian"       },
	{0x0419, "ru", "Russian"        },
	{0x0487, "rw", "Kinyarwanda"    },
	{0x044f, "sa", "Sanskrit"       },
	{0x0859, "sd", "Sindhi"         },
	{0x045b, "si", "Sinhala"        },
	{0x041b, "sk", "Slovak"         },
	{0x0424, "sl", "Slovenian"      },
	{0x0477, "so", "Somali"         },
	{0x041c, "sq", "Albanian"       },
	{0x241a, "sr", "Serbian"        },
	{0x0430, "st", "Southern Sotho" },
	{0x041d, "sv", "Swedish"        },
	{0x0441, "sw", "Swahili"        },
	{0x0449, "ta", "Tamil"          },
	{0x044a, "te", "Telugu"         },
	{0x0428, "tg", "Tajik"          },
	{0x041e, "th", "Thai"           },
	{0x0873, "ti", "Tigrinya"       },
	{0x0442, "tk", "Turkmen"        },
	{0x0432, "tn", "Tswana"         },
	{0x041f, "tr", "Turkish"        },
	{0x0431, "ts", "Tsonga"         },
	{0x0444, "tt", "Tatar"          },
	{0x0480, "ug", "Uyghur"         },
	{0x0422, "uk", "Ukrainian"      },
	{0x0420, "ur", "Urdu"           },
	{0x0443, "uz", "Uzbek"          },
	{0x042a, "vi", "Vietnamese"     },
	{0x0488, "wo", "Wolof"          },
	{0x0434, "xh", "Xhosa"          },
	{0x043d, "yi", "Yiddish"        },
	{0x046a, "yo", "Yoruba"         },
	{0x0804, "zh", "Chinese"        },
	{0x0435, "zu", "Zulu"           },
};

// CPPageDVD dialog

IMPLEMENT_DYNAMIC(CPPageDVD, CPPageBase)
CPPageDVD::CPPageDVD()
	: CPPageBase(CPPageDVD::IDD, CPPageDVD::IDD)
{
}

CPPageDVD::~CPPageDVD()
{
}

void CPPageDVD::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Radio(pDX, IDC_RADIO1, m_iDVDLocation);
	DDX_Radio(pDX, IDC_RADIO3, m_iDVDLangType);
	DDX_Control(pDX, IDC_LIST1, m_lcids);
	DDX_Text(pDX, IDC_DVDPATH, m_dvdpath);
	DDX_Control(pDX, IDC_DVDPATH, m_dvdpathctrl);
	DDX_Control(pDX, IDC_BUTTON1, m_dvdpathselctrl);
	DDX_Check(pDX, IDC_CHECK2, m_bClosedCaptions);
	DDX_Check(pDX, IDC_CHECK3, m_bStartMainTitle);
}

void CPPageDVD::UpdateLCIDList()
{
	UpdateData();

	LCID lcid = m_iDVDLangType == 0 ? m_idMenuLang
				: m_iDVDLangType == 1 ? m_idAudioLang
				: m_idSubtitlesLang;

	for (int i = 0; i < m_lcids.GetCount(); i++) {
		if (m_lcids.GetItemData(i) == lcid) {
			m_lcids.SetCurSel(i);
			m_lcids.SetTopIndex(i);
			break;
		}
	}
}

BEGIN_MESSAGE_MAP(CPPageDVD, CPPageBase)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_RADIO3, IDC_RADIO5, OnBnClickedLangradio123)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_UPDATE_COMMAND_UI(IDC_DVDPATH, OnUpdateDVDPath)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateDVDPath)
END_MESSAGE_MAP()

// CPPageDVD message handlers

BOOL CPPageDVD::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_iDVDLocation	= s.bUseDVDPath ? 1 : 0;
	m_dvdpath		= s.strDVDPath;
	m_iDVDLangType	= 0;

	m_idMenuLang		= s.idMenuLang;
	m_idAudioLang		= s.idAudioLang;
	m_idSubtitlesLang	= s.idSubtitlesLang;
	m_bClosedCaptions	= s.bClosedCaptions;
	m_bStartMainTitle	= s.bStartMainTitle;

	UpdateData(FALSE);

	AddStringData(m_lcids, ResStr(IDS_AG_DEFAULT), 0x0000);

	wchar_t buffer[256] = {};

	for (const auto& dvdlang : dvdlangs) {
		CStringW code(dvdlang.iso639_1);
		LCID lcid = LocaleNameToLCID(code, 0);
		if (lcid != LOCALE_CUSTOM_UNSPECIFIED) {
#if 0
			int ret = GetLocaleInfoEx(code, LOCALE_SLOCALIZEDLANGUAGENAME, buffer, std::size(buffer));
			if (ret) {
				AddStringData(m_lcids, buffer, lcid);
			}
			else
#endif
			{
				AddStringData(m_lcids, CStringW(dvdlang.name), lcid);
			}
			ASSERT(dvdlang.lcid == lcid);
		}
	}

	UpdateLCIDList();

	return TRUE;
}

BOOL CPPageDVD::OnApply()
{
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.strDVDPath		= m_dvdpath;
	s.bUseDVDPath		= (m_iDVDLocation == 1);
	s.idMenuLang		= m_idMenuLang;
	s.idAudioLang		= m_idAudioLang;
	s.idSubtitlesLang	= m_idSubtitlesLang;
	s.bClosedCaptions	= !!m_bClosedCaptions;
	s.bStartMainTitle	= !!m_bStartMainTitle;

	AfxGetMainFrame()->SetClosedCaptions(s.bClosedCaptions);

	return __super::OnApply();
}

void CPPageDVD::OnBnClickedButton1()
{
	CString path;
	CString strTitle = ResStr(IDS_MAINFRM_46);

	{
		CFileDialog dlg(TRUE);
		IFileOpenDialog *openDlgPtr = dlg.GetIFileOpenDialog();

		if (openDlgPtr != nullptr) {
			openDlgPtr->SetTitle(strTitle);
			openDlgPtr->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);

			if (FAILED(openDlgPtr->Show(m_hWnd))) {
				openDlgPtr->Release();
				return;
			}

			openDlgPtr->Release();

			path = dlg.GetFolderPath();
		}
	}

	if (!path.IsEmpty()) {
		m_dvdpath = path;

		UpdateData(FALSE);

		SetModified();
	}
}

void CPPageDVD::OnBnClickedLangradio123(UINT nID)
{
	UpdateLCIDList();
}

void CPPageDVD::OnLbnSelchangeList1()
{
	LCID& lcid = m_iDVDLangType == 0 ? m_idMenuLang
				 : m_iDVDLangType == 1 ? m_idAudioLang
				 : m_idSubtitlesLang;

	lcid = m_lcids.GetItemData(m_lcids.GetCurSel());

	SetModified();
}

void CPPageDVD::OnUpdateDVDPath(CCmdUI* pCmdUI)
{
	UpdateData();

	pCmdUI->Enable(m_iDVDLocation == 1);
}
