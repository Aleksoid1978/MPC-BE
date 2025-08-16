/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2025 see Authors.txt
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
#include "PPageAccelTbl.h"

//#define MASK_NUMBER  0xFFF
#define DUP_KEY      (1<<12)
#define DUP_MOUSE    (1<<13)
#define DUP_MOUSE_FS (1<<14)
#define DUP_APPCMD   (1<<15)
#define DUP_RMCMD    (1<<16)
#define MASK_DUP     (DUP_KEY|DUP_MOUSE|DUP_MOUSE_FS|DUP_APPCMD|DUP_RMCMD)

struct APP_COMMAND {
	UINT		appcmd;
	LPCWSTR		cmdname;
};

const APP_COMMAND g_CommandList[] = {
	{0,									L""},
	{APPCOMMAND_BROWSER_BACKWARD,		L"BROWSER_BACKWARD"},
	{APPCOMMAND_BROWSER_FORWARD,		L"BROWSER_FORWARD"},
	{APPCOMMAND_BROWSER_REFRESH,		L"BROWSER_REFRESH"},
	{APPCOMMAND_BROWSER_STOP,			L"BROWSER_STOP"},
	{APPCOMMAND_BROWSER_SEARCH,			L"BROWSER_SEARCH"},
	{APPCOMMAND_BROWSER_FAVORITES,		L"BROWSER_FAVORITES"},
	{APPCOMMAND_BROWSER_HOME,			L"BROWSER_HOME"},
	{APPCOMMAND_VOLUME_MUTE,			L"VOLUME_MUTE"},
	{APPCOMMAND_VOLUME_DOWN,			L"VOLUME_DOWN"},
	{APPCOMMAND_VOLUME_UP,				L"VOLUME_UP"},
	{APPCOMMAND_MEDIA_NEXTTRACK,		L"MEDIA_NEXTTRACK"},
	{APPCOMMAND_MEDIA_PREVIOUSTRACK,	L"MEDIA_PREVIOUSTRACK"},
	{APPCOMMAND_MEDIA_STOP,				L"MEDIA_STOP"},
	{APPCOMMAND_MEDIA_PLAY_PAUSE,		L"MEDIA_PLAY_PAUSE"},
	{APPCOMMAND_LAUNCH_MAIL,			L"LAUNCH_MAIL"},
	{APPCOMMAND_LAUNCH_MEDIA_SELECT,	L"LAUNCH_MEDIA_SELECT"},
	{APPCOMMAND_LAUNCH_APP1,			L"LAUNCH_APP1"},
	{APPCOMMAND_LAUNCH_APP2,			L"LAUNCH_APP2"},
	{APPCOMMAND_BASS_DOWN,				L"BASS_DOWN"},
	{APPCOMMAND_BASS_BOOST,				L"BASS_BOOST"},
	{APPCOMMAND_BASS_UP,				L"BASS_UP"},
	{APPCOMMAND_TREBLE_DOWN,			L"TREBLE_DOWN"},
	{APPCOMMAND_TREBLE_UP,				L"TREBLE_UP"},
	{APPCOMMAND_MICROPHONE_VOLUME_MUTE, L"MICROPHONE_VOLUME_MUTE"},
	{APPCOMMAND_MICROPHONE_VOLUME_DOWN, L"MICROPHONE_VOLUME_DOWN"},
	{APPCOMMAND_MICROPHONE_VOLUME_UP,	L"MICROPHONE_VOLUME_UP"},
	{APPCOMMAND_HELP,					L"HELP"},
	{APPCOMMAND_FIND,					L"FIND"},
	{APPCOMMAND_NEW,					L"NEW"},
	{APPCOMMAND_OPEN,					L"OPEN"},
	{APPCOMMAND_CLOSE,					L"CLOSE"},
	{APPCOMMAND_SAVE,					L"SAVE"},
	{APPCOMMAND_PRINT,					L"PRINT"},
	{APPCOMMAND_UNDO,					L"UNDO"},
	{APPCOMMAND_REDO,					L"REDO"},
	{APPCOMMAND_COPY,					L"COPY"},
	{APPCOMMAND_CUT,					L"CUT"},
	{APPCOMMAND_PASTE,					L"PASTE"},
	{APPCOMMAND_REPLY_TO_MAIL,			L"REPLY_TO_MAIL"},
	{APPCOMMAND_FORWARD_MAIL,			L"FORWARD_MAIL"},
	{APPCOMMAND_SEND_MAIL,				L"SEND_MAIL"},
	{APPCOMMAND_SPELL_CHECK,			L"SPELL_CHECK"},
	{APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE, L"DICTATE_OR_COMMAND_CONTROL_TOGGLE"},
	{APPCOMMAND_MIC_ON_OFF_TOGGLE,		L"MIC_ON_OFF_TOGGLE"},
	{APPCOMMAND_CORRECTION_LIST,		L"CORRECTION_LIST"},
	{APPCOMMAND_MEDIA_PLAY,				L"MEDIA_PLAY"},
	{APPCOMMAND_MEDIA_PAUSE,			L"MEDIA_PAUSE"},
	{APPCOMMAND_MEDIA_RECORD,			L"MEDIA_RECORD"},
	{APPCOMMAND_MEDIA_FAST_FORWARD,		L"MEDIA_FAST_FORWARD"},
	{APPCOMMAND_MEDIA_REWIND,			L"MEDIA_REWIND"},
	{APPCOMMAND_MEDIA_CHANNEL_UP,		L"MEDIA_CHANNEL_UP"},
	{APPCOMMAND_MEDIA_CHANNEL_DOWN,		L"MEDIA_CHANNEL_DOWN"},
	{APPCOMMAND_DELETE,					L"DELETE"},
	{APPCOMMAND_DWM_FLIP3D,				L"DWM_FLIP3D"},
	{MCE_DETAILS,						L"MCE_DETAILS"},
	{MCE_GUIDE,							L"MCE_GUIDE"},
	{MCE_TVJUMP,						L"MCE_TVJUMP"},
	{MCE_STANDBY,						L"MCE_STANDBY"},
	{MCE_OEM1,							L"MCE_OEM1"},
	{MCE_OEM2,							L"MCE_OEM2"},
	{MCE_MYTV,							L"MCE_MYTV"},
	{MCE_MYVIDEOS,						L"MCE_MYVIDEOS"},
	{MCE_MYPICTURES,					L"MCE_MYPICTURES"},
	{MCE_MYMUSIC,						L"MCE_MYMUSIC"},
	{MCE_RECORDEDTV,					L"MCE_RECORDEDTV"},
	{MCE_DVDANGLE,						L"MCE_DVDANGLE"},
	{MCE_DVDAUDIO,						L"MCE_DVDAUDIO"},
	{MCE_DVDMENU,						L"MCE_DVDMENU"},
	{MCE_DVDSUBTITLE,					L"MCE_DVDSUBTITLE"},
	{MCE_RED,							L"MCE_RED"},
	{MCE_GREEN,							L"MCE_GREEN"},
	{MCE_YELLOW,						L"MCE_YELLOW"},
	{MCE_BLUE,							L"MCE_BLUE"},
	{MCE_MEDIA_NEXTTRACK,				L"MCE_MEDIA_NEXTTRACK"},
	{MCE_MEDIA_PREVIOUSTRACK,			L"MCE_MEDIA_PREVIOUSTRACK"}
};

// CPPageAccelTbl dialog

IMPLEMENT_DYNAMIC(CPPageAccelTbl, CPPageBase)
CPPageAccelTbl::CPPageAccelTbl()
	: CPPageBase(CPPageAccelTbl::IDD, CPPageAccelTbl::IDD)
	, m_list(0)
	, m_WinLircLink(L"http://winlirc.sourceforge.net/")
{
}

BOOL CPPageAccelTbl::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN
			&& (pMsg->hwnd == m_WinLircEdit.m_hWnd)) {
		OnApply();
		return TRUE;
	}

	return __super::PreTranslateMessage(pMsg);
}

void CPPageAccelTbl::UpdateKeyDupFlags()
{
	for (int row = 0; row < m_list.GetItemCount(); row++) {
		auto itemData = (ITEMDATA*)m_list.GetItemData(row);
		const wmcmd& wc = m_wmcmds[itemData->index];

		itemData->flag &= ~DUP_KEY;

		if (wc.key) {
			for (size_t j = 0; j < m_wmcmds.size(); j++) {
				if (itemData->index == j) { continue; }

				if (wc.key == m_wmcmds[j].key && (wc.fVirt & (FCONTROL | FALT | FSHIFT)) == (m_wmcmds[j].fVirt & (FCONTROL | FALT | FSHIFT))) {
					itemData->flag |= DUP_KEY;
					break;
				}
			}
		}
	}
}

void CPPageAccelTbl::UpdateAppcmdDupFlags()
{
	for (int row = 0; row < m_list.GetItemCount(); row++) {
		auto itemData = (ITEMDATA*)m_list.GetItemData(row);
		const wmcmd& wc = m_wmcmds[itemData->index];

		itemData->flag &= ~DUP_APPCMD;

		if (wc.appcmd) {
			for (size_t j = 0; j < m_wmcmds.size(); j++) {
				if (itemData->index == j) { continue; }

				if (wc.appcmd == m_wmcmds[j].appcmd) {
					itemData->flag |= DUP_APPCMD;
					break;
				}
			}
		}
	}
}

void CPPageAccelTbl::UpdateRmcmdDupFlags()
{
	for (int row = 0; row < m_list.GetItemCount(); row++) {
		auto itemData = (ITEMDATA*)m_list.GetItemData(row);
		const wmcmd& wc = m_wmcmds[itemData->index];

		itemData->flag &= ~DUP_RMCMD;

		if (wc.rmcmd.GetLength()) {
			for (size_t j = 0; j < m_wmcmds.size(); j++) {
				if (itemData->index == j) { continue; }

				if (wc.rmcmd.CompareNoCase(m_wmcmds[j].rmcmd) == 0) {
					itemData->flag |= DUP_RMCMD;
					break;
				}
			}
		}
	}
}

void CPPageAccelTbl::UpdateAllDupFlags()
{
	UpdateKeyDupFlags();
	UpdateAppcmdDupFlags();
	UpdateRmcmdDupFlags();
}

CString CPPageAccelTbl::MakeAccelModLabel(BYTE fVirt)
{
	CString str;
	if (fVirt & FCONTROL) {
		str += L"Ctrl";
	}
	if (fVirt & FALT) {
		if (!str.IsEmpty()) {
			str += L" + ";
		}
		str += L"Alt";
	}
	if (fVirt & FSHIFT) {
		if (!str.IsEmpty()) {
			str += L" + ";
		}
		str += L"Shift";
	}
	if (str.IsEmpty()) {
		str = ResStr(IDS_AG_NONE);
	}

	return str;
}

CString CPPageAccelTbl::MakeAccelShortcutLabel(UINT id)
{
	auto& wmcmds = AfxGetAppSettings().wmcmds;
	for (auto& wc : wmcmds) {
		ACCEL& a = wc;
		if (a.cmd == id) {
			return(MakeAccelShortcutLabel(a));
		}
	}

	return(L"");
}

CString CPPageAccelTbl::MakeAccelShortcutLabel(ACCEL& a)
{
	// Reference page for Virtual-Key Codes: http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.100%29.aspx
	CString str;

	switch (a.key) {
		case VK_LBUTTON:    str = L"LBtn";        break;
		case VK_RBUTTON:    str = L"RBtn";        break;
		case VK_CANCEL:     str = L"Cancel";      break;
		case VK_MBUTTON:    str = L"MBtn";        break;
		case VK_XBUTTON1:   str = L"X1Btn";       break;
		case VK_XBUTTON2:   str = L"X2Btn";       break;
		case VK_BACK:       str = L"Back";        break;
		case VK_TAB:        str = L"Tab";         break;
		case VK_CLEAR:      str = L"Clear";       break;
		case VK_RETURN:     str = L"Enter";       break;
		case VK_SHIFT:      str = L"Shift";       break;
		case VK_CONTROL:    str = L"Ctrl";        break;
		case VK_MENU:       str = L"Alt";         break;
		case VK_PAUSE:      str = L"Pause";       break;
		case VK_CAPITAL:    str = L"Capital";     break;
		//case VK_KANA:     str = L"Kana";        break;
		case VK_HANGUL:     str = L"Hangul";      break;
		case VK_IME_ON:     str = L"IME On";      break;
		case VK_JUNJA:      str = L"Junja";       break;
		case VK_FINAL:      str = L"Final";       break;
		//case VK_HANJA:    str = L"Hanja";       break;
		case VK_KANJI:      str = L"Kanji";       break;
		case VK_IME_OFF:    str = L"IME Off";     break;
		case VK_ESCAPE:     str = L"Escape";      break;
		case VK_CONVERT:    str = L"Convert";     break;
		case VK_NONCONVERT: str = L"Non Convert"; break;
		case VK_ACCEPT:     str = L"Accept";      break;
		case VK_MODECHANGE: str = L"Mode Change"; break;
		case VK_SPACE:      str = L"Space";       break;
		case VK_PRIOR:      str = L"PgUp";        break;
		case VK_NEXT:       str = L"PgDn";        break;
		case VK_END:        str = L"End";         break;
		case VK_HOME:       str = L"Home";        break;
		case VK_LEFT:       str = L"Left";        break;
		case VK_UP:         str = L"Up";          break;
		case VK_RIGHT:      str = L"Right";       break;
		case VK_DOWN:       str = L"Down";        break;
		case VK_SELECT:     str = L"Select";      break;
		case VK_PRINT:      str = L"Print";       break;
		case VK_EXECUTE:    str = L"Execute";     break;
		case VK_SNAPSHOT:   str = L"Snapshot";    break;
		case VK_INSERT:     str = L"Insert";      break;
		case VK_DELETE:     str = L"Delete";      break;
		case VK_HELP:       str = L"Help";        break;
		case VK_LWIN:       str = L"LWin";        break;
		case VK_RWIN:       str = L"RWin";        break;
		case VK_APPS:       str = L"Apps";        break;
		case VK_SLEEP:      str = L"Sleep";       break;
		case VK_NUMPAD0:    str = L"Numpad 0";    break;
		case VK_NUMPAD1:    str = L"Numpad 1";    break;
		case VK_NUMPAD2:    str = L"Numpad 2";    break;
		case VK_NUMPAD3:    str = L"Numpad 3";    break;
		case VK_NUMPAD4:    str = L"Numpad 4";    break;
		case VK_NUMPAD5:    str = L"Numpad 5";    break;
		case VK_NUMPAD6:    str = L"Numpad 6";    break;
		case VK_NUMPAD7:    str = L"Numpad 7";    break;
		case VK_NUMPAD8:    str = L"Numpad 8";    break;
		case VK_NUMPAD9:    str = L"Numpad 9";    break;
		case VK_MULTIPLY:   str = L"Multiply";    break;
		case VK_ADD:        str = L"Add";         break;
		case VK_SEPARATOR:  str = L"Separator";   break;
		case VK_SUBTRACT:   str = L"Subtract";    break;
		case VK_DECIMAL:    str = L"Decimal";     break;
		case VK_DIVIDE:     str = L"Divide";      break;
		case VK_F1:         str = L"F1";          break;
		case VK_F2:         str = L"F2";          break;
		case VK_F3:         str = L"F3";          break;
		case VK_F4:         str = L"F4";          break;
		case VK_F5:         str = L"F5";          break;
		case VK_F6:         str = L"F6";          break;
		case VK_F7:         str = L"F7";          break;
		case VK_F8:         str = L"F8";          break;
		case VK_F9:         str = L"F9";          break;
		case VK_F10:        str = L"F10";         break;
		case VK_F11:        str = L"F11";         break;
		case VK_F12:        str = L"F12";         break;
		case VK_F13:        str = L"F13";         break;
		case VK_F14:        str = L"F14";         break;
		case VK_F15:        str = L"F15";         break;
		case VK_F16:        str = L"F16";         break;
		case VK_F17:        str = L"F17";         break;
		case VK_F18:        str = L"F18";         break;
		case VK_F19:        str = L"F19";         break;
		case VK_F20:        str = L"F20";         break;
		case VK_F21:        str = L"F21";         break;
		case VK_F22:        str = L"F22";         break;
		case VK_F23:        str = L"F23";         break;
		case VK_F24:        str = L"F24";         break;
		case VK_NUMLOCK:    str = L"Numlock";     break;
		case VK_SCROLL:     str = L"Scroll";      break;
		//case VK_OEM_NEC_EQUAL:    str = L"OEM NEC Equal";     break;
		case VK_OEM_FJ_JISHO:       str = L"OEM FJ Jisho";      break;
		case VK_OEM_FJ_MASSHOU:     str = L"OEM FJ Msshou";     break;
		case VK_OEM_FJ_TOUROKU:     str = L"OEM FJ Touroku";    break;
		case VK_OEM_FJ_LOYA:        str = L"OEM FJ Loya";       break;
		case VK_OEM_FJ_ROYA:        str = L"OEM FJ Roya";       break;
		case VK_LSHIFT:             str = L"LShift";            break;
		case VK_RSHIFT:             str = L"RShift";            break;
		case VK_LCONTROL:           str = L"LCtrl";             break;
		case VK_RCONTROL:           str = L"RCtrl";             break;
		case VK_LMENU:              str = L"LAlt";              break;
		case VK_RMENU:              str = L"RAlt";              break;
		case VK_BROWSER_BACK:       str = L"Browser Back";      break;
		case VK_BROWSER_FORWARD:    str = L"Browser Forward";   break;
		case VK_BROWSER_REFRESH:    str = L"Browser Refresh";   break;
		case VK_BROWSER_STOP:       str = L"Browser Stop";      break;
		case VK_BROWSER_SEARCH:     str = L"Browser Search";    break;
		case VK_BROWSER_FAVORITES:  str = L"Browser Favorites"; break;
		case VK_BROWSER_HOME:       str = L"Browser Home";      break;
		case VK_VOLUME_MUTE:        str = L"Volume Mute";       break;
		case VK_VOLUME_DOWN:        str = L"Volume Down";       break;
		case VK_VOLUME_UP:          str = L"Volume Up";         break;
		case VK_MEDIA_NEXT_TRACK:   str = L"Media Next Track";  break;
		case VK_MEDIA_PREV_TRACK:   str = L"Media Prev Track";  break;
		case VK_MEDIA_STOP:         str = L"Media Stop";        break;
		case VK_MEDIA_PLAY_PAUSE:   str = L"Media Play/Pause";  break;
		case VK_LAUNCH_MAIL:        str = L"Launch Mail";       break;
		case VK_LAUNCH_MEDIA_SELECT:str = L"Launch Media Select"; break;
		case VK_LAUNCH_APP1:        str = L"Launch App1";       break;
		case VK_LAUNCH_APP2:        str = L"Launch App2";       break;
		case VK_OEM_1:      str = L"OEM 1";       break;
		case VK_OEM_PLUS:   str = L"Plus";        break;
		case VK_OEM_COMMA:  str = L"Comma";       break;
		case VK_OEM_MINUS:  str = L"Minus";       break;
		case VK_OEM_PERIOD: str = L"Period";      break;
		case VK_OEM_2:      str = L"OEM 2";       break;
		case VK_OEM_3:      str = L"OEM 3";       break;
		case VK_OEM_4:      str = L"[";           break;
		case VK_OEM_5:      str = L"OEM 5";       break;
		case VK_OEM_6:      str = L"]";           break;
		case VK_OEM_7:      str = L"OEM 7";       break;
		case VK_OEM_8:      str = L"OEM 8";       break;
		case VK_OEM_AX:     str = L"OEM AX";      break;
		case VK_OEM_102:    str = L"OEM 102";     break;
		case VK_ICO_HELP:   str = L"ICO Help";    break;
		case VK_ICO_00:     str = L"ICO 00";      break;
		case VK_PROCESSKEY: str = L"Process Key"; break;
		case VK_ICO_CLEAR:  str = L"ICO Clear";   break;
		case VK_PACKET:     str = L"Packet";      break;
		case VK_OEM_RESET:  str = L"OEM Reset";   break;
		case VK_OEM_JUMP:   str = L"OEM Jump";    break;
		case VK_OEM_PA1:    str = L"OEM PA1";     break;
		case VK_OEM_PA2:    str = L"OEM PA2";     break;
		case VK_OEM_PA3:    str = L"OEM PA3";     break;
		case VK_OEM_WSCTRL: str = L"OEM WSCtrl";  break;
		case VK_OEM_CUSEL:  str = L"OEM CUSEL";   break;
		case VK_OEM_ATTN:   str = L"OEM ATTN";    break;
		case VK_OEM_FINISH: str = L"OEM Finish";  break;
		case VK_OEM_COPY:   str = L"OEM Copy";    break;
		case VK_OEM_AUTO:   str = L"OEM Auto";    break;
		case VK_OEM_ENLW:   str = L"OEM ENLW";    break;
		case VK_OEM_BACKTAB:str = L"OEM Backtab"; break;
		case VK_ATTN:       str = L"ATTN";        break;
		case VK_CRSEL:      str = L"CRSEL";       break;
		case VK_EXSEL:      str = L"EXSEL";       break;
		case VK_EREOF:      str = L"EREOF";       break;
		case VK_PLAY:       str = L"Play";        break;
		case VK_ZOOM:       str = L"Zoom";        break;
		case VK_NONAME:     str = L"Noname";      break;
		case VK_PA1:        str = L"PA1";         break;
		case VK_OEM_CLEAR:  str = L"OEM Clear";   break;
		case 0x07:
		case 0x0A:
		case 0x0B:
		case 0x5E:
		case VK_NAVIGATION_VIEW:
		case VK_NAVIGATION_MENU:
		case VK_NAVIGATION_UP:
		case VK_NAVIGATION_DOWN:
		case VK_NAVIGATION_LEFT:
		case VK_NAVIGATION_RIGHT:
		case VK_NAVIGATION_ACCEPT:
		case VK_NAVIGATION_CANCEL:
		case 0xB8:
		case 0xB9:
		case 0xC1:
		case 0xC2:
		case VK_GAMEPAD_A:
		case VK_GAMEPAD_B:
		case VK_GAMEPAD_X:
		case VK_GAMEPAD_Y:
		case VK_GAMEPAD_RIGHT_SHOULDER:
		case VK_GAMEPAD_LEFT_SHOULDER:
		case VK_GAMEPAD_LEFT_TRIGGER:
		case VK_GAMEPAD_RIGHT_TRIGGER:
		case VK_GAMEPAD_DPAD_UP:
		case VK_GAMEPAD_DPAD_DOWN:
		case VK_GAMEPAD_DPAD_LEFT:
		case VK_GAMEPAD_DPAD_RIGHT:
		case VK_GAMEPAD_MENU:
		case VK_GAMEPAD_VIEW:
		case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:
		case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON:
		case VK_GAMEPAD_LEFT_THUMBSTICK_UP:
		case VK_GAMEPAD_LEFT_THUMBSTICK_DOWN:
		case VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT:
		case VK_GAMEPAD_LEFT_THUMBSTICK_LEFT:
		case VK_GAMEPAD_RIGHT_THUMBSTICK_UP:
		case VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN:
		case VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT:
		case VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT:
		case 0xE0:
			str.Format(L"Reserved (0x%02x)", (WCHAR)a.key);
			break;
		case 0x0E:
		case 0x0F:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
		case 0x40:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
		case 0xE8:
			str.Format(L"Unassigned (0x%02x)", (WCHAR)a.key);
			break;
		case 0xFF:
			str = L"Multimedia keys";
			break;
		default:
			//	if ('0' <= a.key && a.key <= '9' || 'A' <= a.key && a.key <= 'Z')
			str.Format(L"%c", (WCHAR)a.key);
			break;
	}

	if (a.fVirt&(FCONTROL|FALT|FSHIFT)) {
		str = MakeAccelModLabel(a.fVirt) + L" + " + str;
	}

	str.Replace(L" + ", L"+");

	return(str);
}

CString CPPageAccelTbl::MakeAppCommandLabel(UINT id)
{
	for (auto& command : g_CommandList) {
		if (command.appcmd == id) {
			return CString(command.cmdname);
		}
	}
	return CString("");
}

void CPPageAccelTbl::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_WinLircAddr);
	DDX_Control(pDX, IDC_EDIT1, m_WinLircEdit);
	DDX_Control(pDX, IDC_STATICLINK, m_WinLircLink);
	DDX_Check(pDX, IDC_CHECK1, m_bWinLirc);
	DDX_Control(pDX, IDC_EDIT3, m_FilterEdit);
	DDX_Check(pDX, IDC_CHECK2, m_bGlobalMedia);
}

BEGIN_MESSAGE_MAP(CPPageAccelTbl, CPPageBase)
	ON_NOTIFY(LVN_BEGINLABELEDITW, IDC_LIST1, OnBeginlabeleditList)
	ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
	ON_NOTIFY(LVN_ENDLABELEDITW, IDC_LIST1, OnEndlabeleditList)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedSelectAll)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedResetSelected)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, OnCustomdrawList)
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_EN_CHANGE(IDC_EDIT3, OnChangeFilterEdit)
END_MESSAGE_MAP()

// CPPageAccelTbl message handlers

static WNDPROC OldControlProc;

static LRESULT CALLBACK ControlProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		if ((LOWORD(wParam)== 'A' || LOWORD(wParam) == 'a')	&&(GetKeyState(VK_CONTROL) < 0)) {
			CPlayerListCtrl *pList = (CPlayerListCtrl*)CWnd::FromHandle(control);

			for (int i = 0, j = pList->GetItemCount(); i < j; i++) {
				pList->SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			}

			return 0;
		}
	}

	return CallWindowProcW(OldControlProc, control, message, wParam, lParam); // call control's own windowproc
}

BOOL CPPageAccelTbl::OnInitDialog()
{
	__super::OnInitDialog();

	CAppSettings& s = AfxGetAppSettings();

	m_wmcmds = s.wmcmds;
	m_bWinLirc = s.bWinLirc;
	m_WinLircAddr = s.strWinLircAddr;
	m_bGlobalMedia = s.bGlobalMedia;

	ASSERT(std::size(s.AccelTblColWidths) == COL_COUNT);

	UpdateData(FALSE);

	{
		GetDlgItem(IDC_CHECK9)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATICLINK2)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT2)->ShowWindow(SW_HIDE);

		CRect r;
		GetDlgItem(IDC_PLACEHOLDER)->GetWindowRect(r);
		ScreenToClient(r);

		m_list.CreateEx(
			WS_EX_CLIENTEDGE,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
			r, this, IDC_LIST1);

		m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

		m_list.DeleteAllItems();
		for (int i = 0, n = m_list.GetHeaderCtrl()->GetItemCount(); i < n; i++) {
			m_list.DeleteColumn(0);
		}

		m_list.InsertColumn(COL_CMD, ResStr(IDS_AG_COMMAND), LVCFMT_LEFT);
		m_list.InsertColumn(COL_KEY, ResStr(IDS_AG_KEY), LVCFMT_LEFT);
		m_list.InsertColumn(COL_ID, L"ID", LVCFMT_LEFT);
		m_list.InsertColumn(COL_APPCMD, ResStr(IDS_AG_APP_COMMAND), LVCFMT_LEFT);
		m_list.InsertColumn(COL_RMCMD, L"RemoteCmd", LVCFMT_LEFT);
		m_list.InsertColumn(COL_RMREPCNT, L"RepCnt", LVCFMT_LEFT);

		for (int i = 0; i < COL_COUNT; i++) {
			m_list.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
			m_headerColWidths[i] = m_list.GetColumnWidth(i);
		}

		for (size_t i = 0; i < m_wmcmds.size(); i++) {
			const wmcmd& wc = m_wmcmds[i];
			int row = m_list.InsertItem(m_list.GetItemCount(), wc.GetName(), COL_CMD);
			auto itemData = std::make_unique<ITEMDATA>();
			itemData->index = i;
			m_list.SetItemData(row, (DWORD_PTR)itemData.get());
			m_pItemsData.emplace_back(std::move(itemData));
		}

		SetupList();
		SetupColWidths(true);
	}

	// subclass the keylist control
	OldControlProc = (WNDPROC) SetWindowLongPtrW(m_list.m_hWnd, GWLP_WNDPROC, (LONG_PTR) ControlProc);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageAccelTbl::OnApply()
{
	AfxGetMyApp()->UnregisterHotkeys();
	UpdateData();

	CAppSettings& s = AfxGetAppSettings();

	s.wmcmds = m_wmcmds;

	std::vector<ACCEL> Accel(m_wmcmds.size());
	for (size_t i = 0; i < Accel.size(); i++) {
		Accel[i] = m_wmcmds[i];
	}
	if (s.hAccel) {
		DestroyAcceleratorTable(s.hAccel);
	}
	s.hAccel = CreateAcceleratorTableW(Accel.data(), Accel.size());

	GetParentFrame()->m_hAccelTable = s.hAccel;

	s.bWinLirc = !!m_bWinLirc;
	s.strWinLircAddr = m_WinLircAddr;
	if (s.bWinLirc) {
		s.WinLircClient.Connect(m_WinLircAddr);
	}
	s.bGlobalMedia = !!m_bGlobalMedia;

	AfxGetMyApp()->RegisterHotkeys();

	for (int i = 0; i < COL_COUNT; i++) {
		s.AccelTblColWidths[i] = m_list.GetColumnWidth(i);
	}

	return __super::OnApply();
}

void CPPageAccelTbl::OnBnClickedSelectAll()
{
	m_list.SetFocus();

	for (int i = 0, j = m_list.GetItemCount(); i < j; i++) {
		m_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CPPageAccelTbl::OnBnClickedResetSelected()
{
	m_list.SetFocus();

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	if (!pos) {
		return;
	}

	while (pos) {
		int ni = m_list.GetNextSelectedItem(pos);
		auto itemData = (ITEMDATA*)m_list.GetItemData(ni);

		wmcmd& wc = m_wmcmds[itemData->index];
		wc.Restore();
	}

	SetupList();
	SetupColWidths(false);

	SetModified();
}

void CPPageAccelTbl::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (pItem->iItem < 0) {
		return;
	}

	if (pItem->iSubItem == COL_KEY || pItem->iSubItem == COL_APPCMD
			|| pItem->iSubItem == COL_RMCMD || pItem->iSubItem == COL_RMREPCNT) {
		*pResult = TRUE;
	}
}

static BYTE s_mods[] = {0,FALT,FCONTROL,FSHIFT,FCONTROL|FALT,FCONTROL|FSHIFT,FALT|FSHIFT,FCONTROL|FALT|FSHIFT};

void CPPageAccelTbl::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->iItem < 0 || pItem->iItem >= (int)m_wmcmds.size()) {
		*pResult = FALSE;
		return;
	}

	*pResult = TRUE;

	auto itemData = (ITEMDATA*)m_list.GetItemData(pItem->iItem);
	const wmcmd& wc = m_wmcmds[itemData->index];

	std::list<CString> sl;
	int nSel = -1;

	switch (pItem->iSubItem) {
		case COL_KEY: {
			m_list.ShowInPlaceWinHotkey(pItem->iItem, pItem->iSubItem);
			CWinHotkeyCtrl* pWinHotkey = (CWinHotkeyCtrl*)m_list.GetDlgItem(IDC_WINHOTKEY1);
			UINT cod = 0, mod = 0;

			if (wc.fVirt & FALT) {
				mod |= MOD_ALT;
			}
			if (wc.fVirt & FCONTROL) {
				mod |= MOD_CONTROL;
			}
			if (wc.fVirt & FSHIFT) {
				mod |= MOD_SHIFT;
			}
			cod = wc.key;
			pWinHotkey->SetWinHotkey(cod, mod);
			break;
		}
		case COL_APPCMD:
			for (unsigned i = 0; i < std::size(g_CommandList); i++) {
				sl.emplace_back(g_CommandList[i].cmdname);
				if (wc.appcmd == g_CommandList[i].appcmd) {
					nSel = i;
				}
			}

			m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, sl, nSel);
			break;
		case COL_RMCMD:
		case COL_RMREPCNT:
			m_list.ShowInPlaceEdit(pItem->iItem, pItem->iSubItem);
			break;
		default:
			*pResult = FALSE;
			break;
	}
}

void CPPageAccelTbl::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFOW* pDispInfo = (NMLVDISPINFOW*)pNMHDR;
	LVITEMW* pItem = &pDispInfo->item;

	*pResult = FALSE;

	if (!m_list.m_fInPlaceDirty || pItem->iItem < 0 || pItem->iItem >= (int)m_wmcmds.size()) {
		return;
	}

	auto itemData = (ITEMDATA*)m_list.GetItemData(pItem->iItem);
	wmcmd& wc = m_wmcmds[itemData->index];

	switch (pItem->iSubItem) {
		case COL_KEY: {
			UINT cod, mod;
			CWinHotkeyCtrl* pWinHotkey = (CWinHotkeyCtrl*)m_list.GetDlgItem(IDC_WINHOTKEY1);
			pWinHotkey->GetWinHotkey(&cod, &mod);
			wc.fVirt = 0;
			if (mod & MOD_ALT)     { wc.fVirt |= FALT; }
			if (mod & MOD_CONTROL) { wc.fVirt |= FCONTROL; }
			if (mod & MOD_SHIFT)   { wc.fVirt |= FSHIFT; }
			wc.fVirt |= FVIRTKEY;
			wc.key = cod;
			CString str;
			HotkeyToString(cod, mod, str);
			m_list.SetItemText(pItem->iItem, COL_KEY, str);
			*pResult = TRUE;
			UpdateKeyDupFlags();
		}
		break;
		case COL_APPCMD: {
			unsigned k = pItem->lParam;
			if (k >= std::size(g_CommandList)) {
				break;
			}
			wc.appcmd = g_CommandList[k].appcmd;
			m_list.SetItemText(pItem->iItem, COL_APPCMD, pItem->pszText);
			*pResult = TRUE;
			UpdateAppcmdDupFlags();
		}
		break;
		case COL_RMCMD:
			wc.rmcmd = CStringA(CString(pItem->pszText)).Trim();
			wc.rmcmd.Replace(' ', '_');
			m_list.SetItemText(pItem->iItem, COL_RMCMD, CString(wc.rmcmd));
			*pResult = TRUE;
			UpdateRmcmdDupFlags();
			break;
		case COL_RMREPCNT:
			CString str = CString(pItem->pszText).Trim();
			wc.rmrepcnt = wcstol(str, nullptr, 10);
			str.Format(L"%d", wc.rmrepcnt);
			m_list.SetItemText(pItem->iItem, COL_RMREPCNT, str);
			*pResult = TRUE;
			break;
	}

	if (*pResult) {
		m_list.RedrawWindow();
		SetModified();
	}
}

void CPPageAccelTbl::OnCustomdrawList( NMHDR* pNMHDR, LRESULT* pResult )
{
	// https://www.codeproject.com/Articles/79/Neat-Stuff-to-Do-in-List-Controls-Using-Custom-Dra
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	*pResult = CDRF_DODEFAULT;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage) {
		auto itemData = (ITEMDATA*)m_list.GetItemData(pLVCD->nmcd.dwItemSpec);
		auto dup = itemData->flag;

		if (pLVCD->iSubItem == COL_CMD && dup
				|| pLVCD->iSubItem == COL_KEY && (dup & DUP_KEY)
				|| pLVCD->iSubItem == COL_APPCMD && (dup & DUP_APPCMD)
				|| pLVCD->iSubItem == COL_RMCMD && (dup & DUP_RMCMD)) {
			pLVCD->clrTextBk = RGB(255, 130, 120);
		}
		else {
			pLVCD->clrTextBk = GetSysColor(COLOR_WINDOW);
		}

		*pResult = CDRF_DODEFAULT;
	}
}

void CPPageAccelTbl::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nStatusTimerID) {
		UpdateData();

		if (m_bWinLirc) {
			CString addr;
			m_WinLircEdit.GetWindowTextW(addr);
			AfxGetAppSettings().WinLircClient.Connect(addr);
		}

		m_WinLircEdit.Invalidate();

		m_counter++;
	} else if (nIDEvent == m_nFilterTimerID) {
		KillTimer(m_nFilterTimerID);
		FilterList();
	} else {
		__super::OnTimer(nIDEvent);
	}
}

void CPPageAccelTbl::OnChangeFilterEdit()
{
	KillTimer(m_nFilterTimerID);
	m_nFilterTimerID = SetTimer(2, 100, nullptr);
}

HBRUSH CPPageAccelTbl::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

	int status = -1;

	if (*pWnd == m_WinLircEdit) {
		status = AfxGetAppSettings().WinLircClient.GetStatus();
	}

	if (status == 0 || (status == 2 && (m_counter&1))) {
		pDC->SetTextColor(RGB(255,0,0));
	} else if (status == 1) {
		pDC->SetTextColor(RGB(0,128,0));
	}

	return hbr;
}

BOOL CPPageAccelTbl::OnSetActive()
{
	m_nStatusTimerID = SetTimer(1, 1000, nullptr);

	return CPPageBase::OnSetActive();
}

BOOL CPPageAccelTbl::OnKillActive()
{
	KillTimer(1);

	return CPPageBase::OnKillActive();
}

void CPPageAccelTbl::OnCancel()
{
	CAppSettings& s = AfxGetAppSettings();

	if (!s.bWinLirc) {
		s.WinLircClient.DisConnect();
	}

	__super::OnCancel();
}

void CPPageAccelTbl::SetupList()
{
	for (int row = 0; row < m_list.GetItemCount(); row++) {
		auto itemData = (ITEMDATA*)m_list.GetItemData(row);
		const wmcmd& wc = m_wmcmds[itemData->index];

		CString hotkey;
		HotkeyModToString(wc.key, wc.fVirt, hotkey);
		CString id;
		id.Format(L"%d", wc.cmd);
		CString repcnt;
		repcnt.Format(L"%d", wc.rmrepcnt);

		m_list.SetItemText(row, COL_KEY, hotkey);
		m_list.SetItemText(row, COL_ID, id);
		m_list.SetItemText(row, COL_APPCMD, MakeAppCommandLabel(wc.appcmd));
		m_list.SetItemText(row, COL_RMCMD, CString(wc.rmcmd));
		m_list.SetItemText(row, COL_RMREPCNT, repcnt);
	}

	UpdateAllDupFlags();
}

void CPPageAccelTbl::SetupColWidths(bool bUserValue)
{
	CAppSettings& s = AfxGetAppSettings();

	for (int i = COL_CMD; i < COL_COUNT; i++) {
		int width = bUserValue ? s.AccelTblColWidths[i] : 0;
		auto& headerW = m_headerColWidths[i];

		if (width <= 0 || width > 2000) {
			m_list.SetColumnWidth(i, LVSCW_AUTOSIZE);
			width = m_list.GetColumnWidth(i);
			if (headerW > width) {
				width = headerW;
			}
		}
		else {
			if (width < 25) {
				width = 25;
			}
		}
		m_list.SetColumnWidth(i, width);

		s.AccelTblColWidths[i] = width;
	}
}

void CPPageAccelTbl::FilterList()
{
	CString filter;
	m_FilterEdit.GetWindowText(filter);

	m_list.SetRedraw(false);
	m_list.DeleteAllItems();
	m_pItemsData.clear();

	if (filter.IsEmpty()) {
		for (size_t i = 0; i < m_wmcmds.size(); i++) {
			const wmcmd& wc = m_wmcmds[i];
			int row = m_list.InsertItem(m_list.GetItemCount(), wc.GetName(), COL_CMD);
			auto itemData = std::make_unique<ITEMDATA>();
			itemData->index = i;
			m_list.SetItemData(row, (DWORD_PTR)itemData.get());
			m_pItemsData.emplace_back(std::move(itemData));
		}
	} else {
		auto LowerCase = [](CString& str) {
			if (!str.IsEmpty()) {
				::CharLowerBuffW(str.GetBuffer(), str.GetLength());
			}
		};

		LowerCase(filter);

		for (size_t i = 0; i < m_wmcmds.size(); i++) {
			const wmcmd& wc = m_wmcmds[i];

			CString hotkey, id, name;

			HotkeyModToString(wc.key, wc.fVirt, hotkey); hotkey.MakeLower();
			id.Format(L"%d", wc.cmd);
			name = wc.GetName(); LowerCase(name);

			if (name.Find(filter) != -1 || hotkey.Find(filter) != -1 || id.Find(filter) != -1) {
				int row = m_list.InsertItem(m_list.GetItemCount(), wc.GetName(), COL_CMD);
				auto itemData = std::make_unique<ITEMDATA>();
				itemData->index = i;
				m_list.SetItemData(row, (DWORD_PTR)itemData.get());
				m_pItemsData.emplace_back(std::move(itemData));
			}
		}
	}

	SetupList();
	m_list.SetRedraw(true);
	m_list.RedrawWindow();
}
