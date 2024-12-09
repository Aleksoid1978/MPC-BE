/*
 * (C) 2006-2024 see Authors.txt
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

#define IDS_R_SETTINGS						L"Settings"
#define IDS_R_EXTERNAL_FILTERS				L"ExternalFilters"
#define IDS_R_INTERNAL_FILTERS				L"InternalFilters"
#define IDS_R_COMMANDS						L"Commands2"
#define IDS_R_LOGINS						L"Logins"
#define IDS_R_SHADERS						L"Shaders"
#define IDS_R_FILEFORMATS					L"FileFormats"
#define IDS_R_FILTERS_PRIORITY				L"FiltersPriority"
#define IDS_R_PNSPRESETS					L"Settings\\PnSPresets"

#define IDS_RS_FAV_REMEMBERPOS				L"FavRememberPos"
#define IDS_RS_FAV_RELATIVEDRIVE			L"FavRelativeDrive"
#define IDS_RS_FILEPOS						L"RememberFilePos"
#define IDS_RS_DVDPOS						L"RememberDVDPos"
#define IDS_RS_LANGUAGE						L"Language"
#define IDS_RS_GLOBALMEDIA					L"UseGlobalMedia"
#define IDS_RS_TITLEBARTEXT					L"TitleBarText"
#define IDS_RS_SEEKBARTEXT					L"SeekBarText"
#define IDS_RS_CONTROLSTATE					L"ControlState"
#define IDS_RS_LOOP							L"Loop"
#define IDS_RS_LOOPNUM						L"LoopNum"
#define IDS_RS_ENABLESUBTITLES				L"EnableSubtitles"
#define IDS_RS_FORCEDSUBTITLES				L"ForcedSubtitles"
#define IDS_RS_PRIORITIZEEXTERNALSUBTITLES	L"PrioritizeExternalSubtitles"
#define IDS_RS_DISABLEINTERNALSUBTITLES		L"DisableInternalSubtitles"
#define IDS_RS_AUTORELOADEXTSUBTITLES		L"AutoReloadExtSubtitles"
#define IDS_RS_USE_SUBRESYNC				L"UseSubresync"
#define IDS_RS_SUBTITLEPATHS				L"SubtitlePaths"
#define IDS_RS_USEDEFAULTSUBTITLESSTYLE		L"UseDefaultsubtitlesStyle"
#define IDS_RS_THUMBWIDTH					L"ThumbWidth"

#define IDS_RS_SUBSAVEEXTERNALSTYLEFILE		L"SubSaveExternalStyleFile"

// Window size
#define IDS_RS_WINDOWMODESTARTUP			L"WindowModeStartup"
#define IDS_RS_SPECIFIEDWINDOWSIZE			L"SpecifiedWindowSize"
#define IDS_RS_LASTWINDOWSIZE				L"LastWindowSize"
#define IDS_RS_LASTWINDOWPOS				L"LastWindowPos"
#define IDS_RS_WINDOWMODEPLAYBACK			L"WindowModePlayback"
#define IDS_RS_AUTOSCALEFACTOR				L"AutoScaleFactor"
#define IDS_RS_AUTOFITFACTOR				L"AutoFitFactor"
#define IDS_RS_RESETWINDOWAFTERCLOSINGFILE	L"ResetWindowAfterClosingFile"
#define IDS_RS_REMEMBERWINDOWPOS			L"RememberWindowPos"
#define IDS_RS_LASTWINDOWTYPE				L"LastWindowType"
#define IDS_RS_LIMITWINDOWPROPORTIONS		L"LimitWindowProportions"
#define IDS_RS_SNAPTODESKTOPEDGES			L"SnapToDesktopEdges"

// Audio
#define IDS_R_AUDIO							L"Audio"
#define IDS_RS_VOLUME						L"Volume"
#define IDS_RS_MUTE							L"Mute"
#define IDS_RS_BALANCE						L"Balance"
#define IDS_RS_AUDIORENDERER				L"AudioRenderer"
#define IDS_RS_AUDIORENDERER2				L"AudioRenderer2"
#define IDS_RS_DUALAUDIOOUTPUT				L"DualAudioOtput"
#define IDS_RS_AUDIOMIXER					L"Mixer"
#define IDS_RS_AUDIOMIXERLAYOUT				L"MixerLayout"
#define IDS_RS_AUDIOSTEREOFROMDECODER		L"StereoFromDecoder"
#define IDS_RS_AUDIOBASSREDIRECT			L"BassRedirect"
#define IDS_RS_AUDIOLEVELCENTER				L"LevelCenter"
#define IDS_RS_AUDIOLEVELSURROUND			L"LevelSurround"
#define IDS_RS_AUDIOGAIN					L"Gain"
#define IDS_RS_AUDIOAUTOVOLUMECONTROL		L"AutoVolumeControl"
#define IDS_RS_AUDIONORMBOOST				L"NormBoost"
#define IDS_RS_AUDIONORMLEVEL				L"NormLevel"
#define IDS_RS_AUDIONORMREALEASETIME		L"NormRealeaseTime"
#define IDS_RS_AUDIOSAMPLEFORMATS			L"SampleFormats"
#define IDS_RS_ENABLEAUDIOTIMESHIFT			L"EnableTimeShift"
#define IDS_RS_AUDIOTIMESHIFT				L"TimeShift"
#define IDS_RS_AUDIOFILTERS					L"AudioFilters"
#define IDS_RS_AUDIOFILTER1					L"Filter1"
#define IDS_RS_AUDIOFILTERS_NOTFORSTEREO	L"FiltersDisableForStereo"

// Mouse
#define IDS_R_MOUSE							L"Mouse"
#define IDS_RS_MOUSE_BTN_LEFT				L"ButtonLeft"
#define IDS_RS_MOUSE_BTN_LEFT_OPENRECENT	L"ButtonLeftOpenRecent"
#define IDS_RS_MOUSE_BTN_LEFT_DBLCLICK		L"ButtonLeftDblClick"
#define IDS_RS_MOUSE_EASYMOVE				L"EasyMove"
#define IDS_RS_MOUSE_BTN_RIGHT				L"ButtonRight"
#define IDS_RS_MOUSE_BTN_MIDDLE				L"ButtonMiddle"
#define IDS_RS_MOUSE_BTN_X1					L"ButtonX1"
#define IDS_RS_MOUSE_BTN_X2					L"ButtonX2"
#define IDS_RS_MOUSE_WHEEL_UP				L"WheelUp"
#define IDS_RS_MOUSE_WHEEL_DOWN				L"WheelDown"
#define IDS_RS_MOUSE_WHEEL_LEFT				L"WheelLeft"
#define IDS_RS_MOUSE_WHEEL_RIGHT			L"WheelRight"

// WebServer
#define IDS_R_WEBSERVER						L"WebServer"
#define IDS_RS_ENABLEWEBSERVER				L"EnableWebServer"
#define IDS_RS_WEBSERVERPORT				L"Port"
#define IDS_RS_WEBSERVERQUALITY				L"ImageQuality"
#define IDS_RS_WEBSERVERPRINTDEBUGINFO		L"PrintDebugInfo"
#define IDS_RS_WEBSERVERUSECOMPRESSION		L"UseCompression"
#define IDS_RS_WEBROOT						L"WebRoot"
#define IDS_RS_WEBSERVERLOCALHOSTONLY		L"LocalhostOnly"
#define IDS_RS_WEBUI_ENABLE_PREVIEW			L"EnablePreview"
#define IDS_RS_WEBSERVERCGI					L"CGIHandlers"
#define IDS_RS_WEBDEFINDEX					L"DefaultPage"

// �nlineServices
#define IDS_R_ONLINESERVICES				L"OnlineServices"
#define IDS_R_YOUTUBECACHE					L"OnlineServices\\YoutubeCache"
#define IDS_RS_YOUTUBE_PAGEPARSER			L"YoutubePageParser"
#define IDS_RS_YOUTUBE_VIDEOFORMAT			L"YoutubeVideoFormat"
#define IDS_RS_YOUTUBE_RESOLUTION			L"YoutubeResolution"
#define IDS_RS_YOUTUBE_60FPS				L"Youtube60fps"
#define IDS_RS_YOUTUBE_HDR					L"YoutubeHDR"
#define IDS_RS_YOUTUBE_AUDIOFORMAT			L"YoutubeAudioFormat"
#define IDS_RS_YOUTUBE_AUDIOLANGUAGE		L"YoutubeAudioLanguage"
#define IDS_RS_YOUTUBE_LOAD_PLAYLIST		L"YoutubeLoadPlaylist"
#define IDS_RS_YDL_ENABLE					L"YDLEnable"
#define IDS_RS_YDL_EXEPATH					L"YDLExePath"
#define IDS_RS_YDL_MAXHEIGHT				L"YDLMaxHeight"
#define IDS_RS_YDL_MAXIMUM_QUALITY			L"YDLMaximumQuality"
#define IDS_RS_ACESTREAM_ADDRESS			L"AceStreamAddress"
#define IDS_RS_TORRSERVER_ADDRESS			L"TorrServerAddress"

#define IDS_RS_USER_AGENT					L"UserAgent"

// Capture
#define IDS_R_CAPTURE						L"Capture"
#define IDS_RS_DEFAULT_CAPTURE				L"DefaultCapture"
#define IDS_RS_VIDEO_DISP_NAME				L"VidDispName"
#define IDS_RS_AUDIO_DISP_NAME				L"AudDispName"
#define IDS_RS_COUNTRY						L"Country"

// DVBConfiguration
#define IDS_R_DVB							L"DVB"
#define IDS_RS_BDA_NETWORKPROVIDER			L"BDANetworkProvider"
#define IDS_RS_BDA_TUNER					L"BDATuner"
#define IDS_RS_BDA_RECEIVER					L"BDAReceiver"
#define IDS_RS_BDA_STANDARD					L"BDAStandard"
#define IDS_RS_BDA_SCAN_FREQ_START			L"BDAScanFreqStart"
#define IDS_RS_BDA_SCAN_FREQ_END			L"BDAScanFreqEnd"
#define IDS_RS_BDA_BANDWIDTH				L"BDABandWidth"
#define IDS_RS_BDA_USE_OFFSET				L"BDAUseOffset"
#define IDS_RS_BDA_OFFSET					L"BDAOffset"
#define IDS_RS_BDA_IGNORE_ENCRYPTEDCHANNELS	L"BDAIgnoreEncryptedChannels"
#define IDS_RS_DVB_LAST_CHANNEL				L"LastChannel"

// DVD
#define IDS_R_DVD							L"DVD"
#define IDS_RS_DVD_USEPATH					L"UseDVDPath"
#define IDS_RS_DVD_PATH						L"DVDPath"
#define IDS_RS_DVD_MENULANG					L"MenuLang"
#define IDS_RS_DVD_AUDIOLANG				L"AudioLang"
#define IDS_RS_DVD_SUBTITLESLANG			L"SubtitlesLang"
#define IDS_RS_DVD_CLOSEDCAPTIONS			L"ClosedCaptions"
#define IDS_RS_DVD_STARTMAINTITLE			L"StartMainTitle"

// Updater
#define IDS_R_UPDATER						L"Settings\\Updater"
#define IDS_RS_UPDATER_AUTO_CHECK			L"UpdaterAutoCheck"
#define IDS_RS_UPDATER_DELAY				L"UpdaterDelay"
#define IDS_RS_UPDATER_LAST_CHECK			L"UpdaterLastCheck"

// OSD
#define IDS_R_OSD							L"OSD"
#define IDS_RS_SHOWOSD						L"ShowOSD"
#define IDS_RS_OSD_SIZE						L"Size"
#define IDS_RS_OSD_FONT						L"Font"
#define IDS_RS_OSD_FONTSHADOW				L"FontShadow"
#define IDS_RS_OSD_FONTAA					L"FontAA"
#define IDS_RS_OSD_FONTCOLOR				L"FontColor"
#define IDS_RS_OSD_GRADCOLOR1				L"GradColor1"
#define IDS_RS_OSD_GRADCOLOR2				L"GradColor2"
#define IDS_RS_OSD_TRANSPARENT				L"Transparent"
#define IDS_RS_OSD_BORDER					L"Border"
#define IDS_RS_OSD_REMAINING_TIME			L"RemainingTime"
#define IDS_RS_OSD_LOCAL_TIME				L"LocalTime"
#define IDS_RS_OSD_FILE_NAME				L"FileName"

// Theme
#define IDS_R_THEME							L"Theme"
#define IDS_RS_USEDARKTHEME					L"UseDarkTheme"
#define IDS_RS_THEMEBRIGHTNESS				L"ThemeBrightness"
#define IDS_RS_THEMECOLOR					L"ThemeColor"
#define IDS_RS_TOOLBARCOLORFACE				L"ToolbarColorFace"
#define IDS_RS_TOOLBARCOLOROUTLINE			L"ToolbarColorOutline"
#define IDS_RS_DARKMENU						L"DarkMenu"
#define IDS_RS_DARKTITLE					L"DarkTitle"

// FullScreen
#define IDS_RS_LAUNCHFULLSCREEN				L"LaunchFullScreen"
#define IDS_RS_FULLSCREENCTRLS				L"FullScreenCtrls"
#define IDS_RS_FULLSCREENCTRLSTIMEOUT		L"FullScreenCtrlsTimeOut"
#define IDS_RS_EXITFULLSCREENATTHEEND		L"ExitFullscreenAtTheEnd"
#define IDS_RS_EXITFULLSCREENATFOCUSLOST	L"ExitFullscreenAtFocusLost"
#define IDS_RS_FULLSCREENMONITOR			L"FullScreenMonitor"
#define IDS_RS_FULLSCREENMONITORID			L"FullScreenMonitorID"
#define IDS_RS_FULLSCREENRES				L"Settings\\FullscreenRes"
#define IDS_RS_FULLSCREENRES_ENABLE			L"Enable"
#define IDS_RS_FULLSCREENRES_APPLY_DEF		L"ApplyDefault"
#define IDS_RS_RESTORERESAFTEREXIT			L"RestoreResAfterExit"
#define IDS_RS_DISPLAYMODECHANGEDELAY		L"DisplayModeChangeDelay"

// Playlist
#define IDS_RS_PLAYLIST						L"Playlist"
#define IDS_RS_PLAYLIST_SHUFFLE				L"Shuffle"
#define IDS_RS_PLAYLIST_REMEMBERMAIN		L"RememberMainPlaylist"
#define IDS_RS_PLAYLIST_HIDEINFULLSCREEN	L"HideInFullScreen"
#define IDS_RS_PLAYLIST_SHOWPTOOLTIP		L"ShowTooltip"
#define IDS_RS_PLAYLIST_SHOWSEARCHBAR		L"ShowSearchBar"
#define IDS_RS_PLAYLIST_NEXTONERROR			L"NextOnError"
#define IDS_RS_PLAYLIST_SKIP_INVALID		L"SkipInvalid"
#define IDS_RS_PLAYLIST_DETERMINEDURATION	L"DetermineDuration"

#define IDS_RS_PLAYLISTFONTPERCENT			L"PlaylistFontPercent"
#define IDS_RS_PLAYLISTTABSSETTINGS			L"TabsSettings"

#define IDS_RS_STEREO3D_MODE				L"Stereo3DMode"
#define IDS_RS_STEREO3D_SWAPLEFTRIGHT		L"Stereo3DSwapLeftRight"
#define IDS_RS_SHADERLIST					L"ShaderList"
#define IDS_RS_SHADERLISTSCREENSPACE		L"ShaderListScreenSpace"
#define IDS_RS_SHADERS11POSTSCALE			L"Shaders11PostScale"
#define IDS_RS_TITLEBARTEXTTITLE			L"TitleBarTextTitle"
#define IDS_RS_REWIND						L"Rewind"
#define IDS_RS_VOLUME_STEP					L"VolumeStep"
#define IDS_RS_SPEED_STEP					L"SpeedStep"
#define IDS_RS_SPEED_NOTRESET				L"SpeedNotReset"
#define IDS_RS_MULTIINST					L"MultipleInstances"
#define IDS_RS_ALWAYSONTOP					L"AlwaysOnTop"
#define IDS_RS_SPSTYLE						L"SPDefaultStyle"
#define IDS_RS_SPOVERRIDEPLACEMENT			L"SPOverridePlacement"
#define IDS_RS_SPHORPOS						L"SPHorPos"
#define IDS_RS_SPVERPOS						L"SPVerPos"

#define IDS_RS_HIDECAPTIONMENU				L"HideCaptionMenu"
#define IDS_RS_HIDENAVIGATION				L"HideNavigation"
#define IDS_RS_DEFAULTVIDEOFRAME			L"DefaultVideoFrame"
#define IDS_RS_NOSMALLUPSCALE				L"NoSmallUpscale"
#define IDS_RS_NOSMALLDOWNSCALE				L"NoSmallDownscale"
#define IDS_RS_PANSCANZOOM					L"PanScanZoom"
#define IDS_RS_BUFFERDURATION				L"BufferDuration"
#define IDS_RS_NETWORKTIMEOUT				L"NetworkTimeout"
#define IDS_RS_NETRECEIVETIMEOUT			L"NetworkReceiveTimeout"
#define IDS_RS_SUBDELAYINTERVAL				L"SubDelayInterval"
#define IDS_RS_LOGOFILE						L"LogoFile"
#define IDS_RS_AUDIOWINDOWMODE				L"AudioWindowMode"
#define IDS_RS_ADDSIMILARFILES				L"AddSimilarFiles"
#define IDS_RS_ENABLEWORKERTHREADFOROPENING	L"EnableWorkerThreadForOpening"
#define IDS_RS_AUTOLOADAUDIO				L"AutoloadAudio"
#define IDS_RS_PRIORITIZEEXTERNALAUDIO		L"PrioritizeExternalAudio"
#define IDS_RS_AUDIOPATHS					L"AudioPaths"
#define IDS_RS_SUBTITLERENDERER				L"SubtitleRenderer"
#define IDS_RS_SUBTITLESLANGORDER			L"SubtitlesLanguageOrder"
#define IDS_RS_AUDIOSLANGORDER				L"AudiosLanguageOrder"
#define IDS_RS_INTERNALSELECTTRACKLOGIC		L"UseInternalSelectTrackLogic"
#define IDS_RS_REMEMBERSELECTEDTRACKS		L"RememberSelectedTracks"
#define IDS_RS_SEARCHKEYFRAMES				L"SearchKeyframes"
#define IDS_RS_WINLIRCADDR					L"WinLircAddr"
#define IDS_RS_WINLIRC						L"UseWinLirc"
#define IDS_RS_TRAYICON						L"TrayIcon"
#define IDS_RS_KEEPASPECTRATIO				L"KeepAspectRatio"
#define IDS_RS_UICEADDR						L"UICEAddr"
#define IDS_RS_UICE							L"UseUICE"
#define IDS_RS_JUMPDISTS					L"JumpDistS"
#define IDS_RS_JUMPDISTM					L"JumpDistM"
#define IDS_RS_JUMPDISTL					L"JumpDistL"
#define IDS_RS_REPORTFAILEDPINS				L"ReportFailedPins"
#define IDS_RS_KEEPHISTORY					L"KeepHistory"
#define IDS_RS_RECENT_FILES_NUMBER			L"RecentFilesNumber"
#define IDS_RS_RECENT_FILES_MENU_ELLIPSIS	L"RecentFilesMenuEllipsis"
#define IDS_RS_RECENT_FILES_SHOW_URL_TITLE	L"RecentFilesShowUrlTitle"
#define IDS_RS_HISTORY_ENTRIES_MAX			L"HistoryEntriesMax"
#define IDS_RS_LOGOID						L"LogoID2"
#define IDS_RS_LOGOEXT						L"LogoExt"
#define IDS_RS_COMPMONDESKARDIFF			L"CompMonDeskARDiff"
#define IDS_RS_HIDECDROMSSUBMENU			L"HideCDROMsSubMenu"
#define IDS_RS_ONTOP						L"OnTop"
#define IDS_RS_SNAPSHOTPATH					L"SnapShotPath"
#define IDS_RS_PRIORITY						L"Priority"
#define IDS_RS_SNAPSHOTEXT					L"SnapShotExt"
#define IDS_RS_SNAPSHOT_SUBTITLES			L"SnapShotSubtitles"
#define IDS_RS_ISDB							L"ISDb"
#define IDS_RS_ASPECTRATIO_X				L"AspectRatioX"
#define IDS_RS_ASPECTRATIO_Y				L"AspectRatioY"
#define IDS_RS_THUMBROWS					L"ThumbRows"
#define IDS_RS_THUMBCOLS					L"ThumbCols"
#define IDS_RS_THUMBQUALITY					L"ThumbQuality"
#define IDS_RS_THUMBLEVELPNG				L"ThumbLevelPNG"
#define IDS_RS_PREVENT_MINIMIZE				L"PreventMinimize"
#define IDS_RS_PAUSEMINIMIZEDVIDEO			L"PauseMinimizedVideo"
#define IDS_RS_WIN7TASKBAR					L"UseWin7TaskBar"
#define IDS_RS_EXIT_AFTER_PB				L"ExitAfterPlayBack"
#define IDS_RS_CLOSE_FILE_AFTER_PB			L"CloseFileAfterPlayBack"
#define IDS_RS_NEXT_AFTER_PB				L"SearchInDirAfterPlayBack"
#define IDS_RS_NEXT_AFTER_PB_LOOPED			L"SearchInDirAfterPlayBackLooped"
#define IDS_RS_NO_SEARCH_IN_FOLDER			L"DontUseSearchInFolder"
#define IDS_RS_USE_TIME_TOOLTIP				L"UseTimeTooltip"
#define IDS_RS_TIME_TOOLTIP_POSITION		L"TimeTooltipPosition"

#define IDS_RS_ASSOCIATED_WITH_ICON			L"AssociatedWithIcon"

#define IDS_RS_LAST_OPEN_FILE				L"LastOpenFile" // obsolete
#define IDS_RS_LAST_OPEN_DIR				L"LastOpenDir"
#define IDS_RS_LAST_SAVED_PLAYLIST_DIR		L"LastSavedPlaylistDir"
#define IDS_RS_LAST_OPEN_FILTER_DIR			L"LastOpenFilterDir"

#define IDS_RS_TOGGLESHADER					L"ToggleShader"
#define IDS_RS_TOGGLESHADERSSCREENSPACE		L"ToggleShaderScreenSpace"

#define IDS_RS_FASTSEEK_KEYFRAME			L"FastSeek"
#define IDS_RS_HIDE_WINDOWED_MOUSE_POINTER	L"HideWindowedMousePointer"
#define IDS_RS_MIN_MPLS_DURATION			L"MinMPlsDuration"
#define IDS_RS_MINI_DUMP					L"MiniDump"
#define IDS_RS_FFMPEG_EXEPATH				L"FFmpegExePath"

#define IDS_RS_LCD_SUPPORT					L"LcdSupport"
#define IDS_RS_WINMEDIACONTROLS				L"WinMediaControls"
#define IDS_RS_SMARTSEEK					L"UseSmartSeek"
#define IDS_RS_SMARTSEEK_ONLINE				L"UseSmartSeekOnline"
#define IDS_RS_SMARTSEEK_SIZE				L"SmartSeekSize"
#define IDS_RS_SMARTSEEK_VIDEORENDERER		L"SmartSeekVideoRenderer"
#define IDS_RS_CHAPTER_MARKER				L"ChapterMarker"
#define IDS_RS_FILENAMEONSEEKBAR			L"FileNameOnSeekBar"

#define IDS_RS_REMAINING_TIME				L"RemainingTime"
#define IDS_RS_SHOW_ZERO_HOURS				L"ShowZeroHours"

#define IDS_RS_SHADERS_INITIALIZED			L"Initialized"

#define IDS_RS_USE_FLYBAR					L"UseFlybar"
#define IDS_RS_USE_FLYBAR_ONTOP				L"UseFlybarOnTop"

#define IDS_RS_DLGPROPX						L"DlgPropX"
#define IDS_RS_DLGPROPY						L"DlgPropY"

#define IDS_RS_LASTFILEINFOPAGE				L"LastFileInfoPage"

#define IDS_RS_CONTEXTFILES					L"ShowContextFiles"
#define IDS_RS_CONTEXTDIR					L"ShowContextDir"

#define IDS_RS_PASTECLIPBOARDURL			L"PasteClipboardURL"

// Dialogs

#define IDS_R_DLG_OPTIONS					L"Dialogs\\Options"
#define IDS_RS_LASTUSEDPAGE					L"LastUsedPage"
#define IDS_R_ACCELCOLWIDTHS				L"AccelColWidths"

#define IDS_R_DLG_SUBTITLEDL				L"Dialogs\\SubtitleDl"
#define IDS_RS_DLG_SUBTITLEDL_COLWIDTH		L"ColWidth"

#define IDS_R_DLG_ORGANIZE_FAV				L"Dialogs\\OrganizeFavorites"

#define IDS_R_DLG_HISTORY					L"Dialogs\\History"
#define IDS_R_HISTORYCOLWIDTHS				L"HistoryColWidths"

#define IDS_R_DLG_CMD_LINE_HELP				L"Dialogs\\CmdLineHelp"

#define IDS_R_DLG_GOTO						L"Dialogs\\GoTo"
#define IDS_RS_DLG_GOTO_LASTTIMEFMT			L"LastTimeFmt"
#define IDS_RS_DLG_GOTO_FPS					L"fps"
