;
; (C) 2009-2024 see Authors.txt
;
; This file is part of MPC-BE.
;
; MPC-BE is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; MPC-BE is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

#if VER < EncodeVer(6,3,1)
  #error A more recent version of Inno Setup 6 is required to compile this script (6.2.2 or newer)
#endif
#ifndef UNICODE
  #error Use the Unicode Inno Setup
#endif

; Requirements:
; Inno Setup Unicode: http://www.jrsoftware.org/isdl.php

; If you want to compile the 32-bit version define "Win32Build" (uncomment the define below or use build.bat)
#define localize
;#define Win32Build

; Don't forget to update the DirectX SDK number in "include\Version.h" (not updated so often)

; From now on you shouldn't need to change anything

#define ISPP_INVOKED
#include "..\include\Version.h"

#define app_name         "MPC-BE"
#define copyright_year   str(MPC_YEAR_COMMENTS)
#define app_url          str(MPC_VERSION_COMMENTS)
#if MPC_VERSION_STATUS == 1 && MPC_VERSION_REV == 0
  #define app_version    str(MPC_VERSION_MAJOR) + "." + str(MPC_VERSION_MINOR) + "." + str(MPC_VERSION_PATCH)
#else
  #define app_version    str(MPC_VERSION_MAJOR) + "." + str(MPC_VERSION_MINOR) + "." + str(MPC_VERSION_PATCH) + "." + str(MPC_VERSION_REV)
#endif  

#define bin_dir        = "..\_bin"

#define bindir_x64 = bin_dir + "\mpc-be_x64"
#define bindir_x86 = bin_dir + "\mpc-be_x86"

#ifdef Win32Build
  #define bindir       = bin_dir + "\mpc-be_x86"
  #define mpcbe_exe    = "mpc-be.exe"
  #define mpcbe_ini    = "mpc-be.ini"
  #define dxdir        = "MPC_components\DirectX\x86"
  #define BeveledLabel = app_name + " " + app_version
  #define Description  = app_name + " " + app_version
  #define VisualElementsManifest = "VisualElements\mpc-be.VisualElementsManifest.xml"
#else
  #define bindir       = bin_dir + "\mpc-be_x64"
  #define mpcbe_exe    = "mpc-be64.exe"
  #define mpcbe_ini    = "mpc-be64.ini"
  #define dxdir        = "MPC_components\DirectX\x64"
  #define BeveledLabel = app_name + " x64 " + app_version
  #define Description  = app_name + " x64 " + app_version
  #define VisualElementsManifest = "VisualElements\mpc-be64.VisualElementsManifest.xml"
#endif
#define mpcvr_desc     = "MPC Video Renderer 0.9.1"
#define mpcscriptsrc_desc = "MPC Script Source 0.2.5"

[Setup]
#ifdef Win32Build
AppId={{903D098F-DD50-4342-AD23-DA868FCA3126}
DefaultGroupName={#app_name}
OutputBaseFilename={#app_name}.{#app_version}.x86
UninstallDisplayName={#app_name} {#app_version}
AppName={#app_name}
AppVerName={#app_name} {#app_version}
VersionInfoDescription={#app_name} Setup
VersionInfoProductName={#app_name}
#else
AppId={{FE09AF6D-78B2-4093-B012-FCDAF78693CE}
DefaultGroupName={#app_name} x64
OutputBaseFilename={#app_name}.{#app_version}.x64
UninstallDisplayName={#app_name} x64 {#app_version}
ArchitecturesAllowed=x64 arm64
ArchitecturesInstallIn64BitMode=x64 arm64
AppName={#app_name} x64
AppVerName={#app_name} x64 {#app_version}
VersionInfoDescription={#app_name} x64 Setup
VersionInfoProductName={#app_name} x64
#endif
AppVersion={#app_version}
AppPublisher={#app_name} Team
AppPublisherURL={#app_url}
AppSupportURL={#app_url}
AppUpdatesURL={#app_url}
AppContact={#app_url}
AppCopyright=Copyright © {#copyright_year} all contributors, see Authors.txt
VersionInfoCompany={#app_name} Team
VersionInfoCopyright=Copyright © {#copyright_year}, {#app_name} Team
VersionInfoProductVersion={#app_version}
VersionInfoProductTextVersion={#app_version}
VersionInfoTextVersion={#app_version}
VersionInfoVersion={#app_version}
UninstallDisplayIcon={app}\{#mpcbe_exe}
DefaultDirName={code:GetInstallFolder}
LicenseFile=..\LICENSE.txt
OutputDir=.
SetupIconFile=..\src\apps\mplayerc\res\icon.ico
AppReadmeFile={app}\Readme.md
WizardImageFile=WizardImageFile.bmp
WizardSmallImageFile=WizardSmallImageFile.bmp
Compression=lzma2/ultra
InternalCompressLevel=ultra
SolidCompression=yes
AllowNoIcons=yes
ShowTasksTreeLines=yes
DisableDirPage=auto
DisableProgramGroupPage=auto
MinVersion=6.1.7600
AppMutex={#app_name}
ChangesAssociations=true
#ifdef Sign
SignTool=OpenSourceSign
#endif

[Languages]
Name: en; MessagesFile: compiler:Default.isl

#ifdef localize
Name: br; MessagesFile: compiler:Languages\BrazilianPortuguese.isl
Name: by; MessagesFile: Languages\Belarusian.isl
Name: ca; MessagesFile: compiler:Languages\Catalan.isl
Name: cz; MessagesFile: compiler:Languages\Czech.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: el; MessagesFile: Languages\Greek.isl
Name: es; MessagesFile: compiler:Languages\Spanish.isl
Name: eu; MessagesFile: Languages\Basque.isl
Name: fr; MessagesFile: compiler:Languages\French.isl
Name: he; MessagesFile: compiler:Languages\Hebrew.isl
Name: hr; MessagesFile: Languages\Croatian.isl
Name: hu; MessagesFile: compiler:Languages\Hungarian.isl
Name: hy; MessagesFile: compiler:Languages\Armenian.isl
Name: it; MessagesFile: compiler:Languages\Italian.isl
Name: ja; MessagesFile: compiler:Languages\Japanese.isl
Name: kr; MessagesFile: compiler:Languages\Korean.isl
Name: nl; MessagesFile: compiler:Languages\Dutch.isl
Name: pl; MessagesFile: compiler:Languages\Polish.isl
Name: ro; MessagesFile: Languages\Romanian.isl
Name: ru; MessagesFile: compiler:Languages\Russian.isl
Name: sc; MessagesFile: Languages\ChineseSimplified.isl
Name: sk; MessagesFile: compiler:Languages\Slovak.isl
Name: sl; MessagesFile: compiler:Languages\Slovenian.isl
Name: sv; MessagesFile: compiler:Languages\Swedish.isl
Name: tc; MessagesFile: Languages\ChineseTraditional.isl
Name: tr; MessagesFile: compiler:Languages\Turkish.isl
Name: ua; MessagesFile: compiler:Languages\Ukrainian.isl
#endif

; Include installer's custom messages
#include ".\CustomMessages\custom_messages.iss"
#ifdef localize
#include ".\CustomMessages\custom_messages.br.iss"
#include ".\CustomMessages\custom_messages.by.iss"
#include ".\CustomMessages\custom_messages.ca.iss"
#include ".\CustomMessages\custom_messages.cz.iss"
#include ".\CustomMessages\custom_messages.de.iss"
#include ".\CustomMessages\custom_messages.el.iss"
#include ".\CustomMessages\custom_messages.es.iss"
#include ".\CustomMessages\custom_messages.eu.iss"
#include ".\CustomMessages\custom_messages.fr.iss"
#include ".\CustomMessages\custom_messages.he.iss"
#include ".\CustomMessages\custom_messages.hu.iss"
#include ".\CustomMessages\custom_messages.hy.iss"
#include ".\CustomMessages\custom_messages.it.iss"
#include ".\CustomMessages\custom_messages.ja.iss"
#include ".\CustomMessages\custom_messages.kr.iss"
#include ".\CustomMessages\custom_messages.nl.iss"
#include ".\CustomMessages\custom_messages.pl.iss"
#include ".\CustomMessages\custom_messages.ro.iss"
#include ".\CustomMessages\custom_messages.ru.iss"
#include ".\CustomMessages\custom_messages.sc.iss"
#include ".\CustomMessages\custom_messages.sk.iss"
#include ".\CustomMessages\custom_messages.sl.iss"
#include ".\CustomMessages\custom_messages.sv.iss"
#include ".\CustomMessages\custom_messages.tc.iss"
#include ".\CustomMessages\custom_messages.tr.iss"
#include ".\CustomMessages\custom_messages.ua.iss"
#endif

[Messages]
WelcomeLabel1=[name/ver]
BeveledLabel={#BeveledLabel}

[Types]
Name: default; Description: {cm:types_DefaultInstallation}
Name: custom;  Description: {cm:types_CustomInstallation}; Flags: iscustom

[Components]
Name: "main";          Description: "{#Description}";           Types: default custom; Flags: fixed
Name: "mpciconlib";    Description: "{cm:comp_mpciconlib}";     Types: default custom
#ifdef localize
Name: "mpcresources";  Description: "{cm:comp_mpcresources}";   Types: default custom; Flags: disablenouninstallwarning
#endif
Name: "mpcbeshellext"; Description: "{cm:comp_mpcbeshellext}";  Types: custom;         Flags: disablenouninstallwarning;
Name: "intel_msdk";    Description: "{cm:comp_intel_msdk}";     Types: custom;         Flags: disablenouninstallwarning;
Name: "mpcvr";         Description: "{#mpcvr_desc}";            Types: default custom; Flags: disablenouninstallwarning;
Name: "mpcscriptsrc";  Description: "{#mpcscriptsrc_desc}";     Types: custom;         Flags: disablenouninstallwarning;

[Tasks]
Name: desktopicon;              Description: {cm:CreateDesktopIcon};     GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user;         Description: {cm:tsk_CurrentUser};       GroupDescription: {cm:AdditionalIcons}; Flags: exclusive
Name: desktopicon\common;       Description: {cm:tsk_AllUsers};          GroupDescription: {cm:AdditionalIcons}; Flags: unchecked exclusive
Name: pintotaskbar;             Description: {cm:PinToTaskBar};          GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.4

Name: longpathsenable;          Description: {cm:tsk_LongPathsEnable};   GroupDescription: {cm:tsk_Other};       Flags: checkedonce unchecked; MinVersion: 10.0.14393; Check: not LongPathIsEnabled()
;;ResetSettings
Name: reset_settings;           Description: {cm:tsk_ResetSettings};     GroupDescription: {cm:tsk_Other};       Flags: checkedonce unchecked; Check: SettingsExist()

[Files]
Source: "{#bindir}\{#mpcbe_exe}";            DestDir: "{app}"; Flags: ignoreversion;                                   Components: main
Source: "{#bindir}\mpciconlib.dll";          DestDir: "{app}"; Flags: ignoreversion;                                   Components: mpciconlib
Source: "{#dxdir}\d3dcompiler_47.dll";       DestDir: "{app}"; Flags: ignoreversion;                                   Components: main;          Check: not D3DCompiler_47_DLLExists();
Source: "{#dxdir}\d3dx9_43.dll";             DestDir: "{app}"; Flags: ignoreversion;                                   Components: main;          Check: not D3DX9_43_DLLExists();
Source: "{#bindir_x64}\MPCBEShellExt64.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete regserver noregerror; Components: mpcbeshellext; Check: IsWin64
Source: "{#bindir_x86}\MPCBEShellExt.dll";   DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete regserver noregerror; Components: mpcbeshellext;
#ifdef localize
Source: "{#bindir}\Lang\mpcresources.??.dll";             DestDir: "{app}\Lang"; Flags: ignoreversion; Components: mpcresources
#endif
Source: "..\LICENSE.txt";                  DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "..\docs\Authors.txt";             DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "..\docs\Authors mpc-hc team.txt"; DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "..\docs\Changelog.txt";           DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "..\docs\Changelog.Rus.txt";       DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "..\docs\README.md";               DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "Shaders\*.hlsl";                  DestDir: "{commonappdata}\{#app_name}\Shaders"; Flags: ignoreversion; Components: main;
Source: "Shaders\*.hlsl";                  DestDir: "{app}\Shaders";                     Flags: ignoreversion; Components: main; Check: IniUsed()
Source: "Shaders11\*.hlsl";                DestDir: "{commonappdata}\{#app_name}\Shaders11"; Flags: ignoreversion; Components: main;
Source: "Shaders11\*.hlsl";                DestDir: "{app}\Shaders11";                   Flags: ignoreversion; Components: main; Check: IniUsed()
Source: "VisualElements\*.png";            DestDir: "{app}";                             Flags: ignoreversion; Components: main
Source: "{#VisualElementsManifest}";       DestDir: "{app}";                             Flags: ignoreversion; Components: main
#ifdef Win32Build
Source: "MPC_components\IntelMediaSDK\libmfxsw32.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: intel_msdk;
Source: "MPC_components\MpcVideoRenderer\MpcVideoRenderer.ax"; DestDir: "{app}\Filters"; Flags: regserver; Components: mpcvr;
Source: "MPC_components\MpcScriptSource\MpcScriptSource.ax"; DestDir: "{app}\Filters"; Flags: regserver; Components: mpcscriptsrc;
#else
Source: "MPC_components\IntelMediaSDK\libmfxsw64.dll"; DestDir: "{app}"; Flags: ignoreversion; Components: intel_msdk;
Source: "MPC_components\MpcVideoRenderer\MpcVideoRenderer64.ax"; DestDir: "{app}\Filters"; Flags: regserver; Components: mpcvr;
Source: "MPC_components\MpcScriptSource\MpcScriptSource64.ax"; DestDir: "{app}\Filters"; Flags: regserver; Components: mpcscriptsrc;
#endif

[Icons]
#ifdef Win32Build
Name: {group}\{#app_name};                           Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version};            WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name};                   Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version};            WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name};                     Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version};            WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {group}\{cm:UninstallProgram,{#app_name}};     Filename: {uninstallexe};          Comment: {cm:UninstallProgram,{#app_name}};     WorkingDir: {app}
#else
Name: {group}\{#app_name} x64;                       Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version} x64;        WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0
Name: {commondesktop}\{#app_name} x64;               Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version} x64;        WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\{#app_name} x64;                 Filename: {app}\{#mpcbe_exe};      Comment: {#app_name} {#app_version} x64;        WorkingDir: {app}; IconFilename: {app}\{#mpcbe_exe}; IconIndex: 0; Tasks: desktopicon\user
Name: {group}\{cm:UninstallProgram,{#app_name} x64}; Filename: {uninstallexe};          Comment: {cm:UninstallProgram,{#app_name} x64}; WorkingDir: {app}
#endif
Name: {group}\Changelog;                             Filename: {app}\Changelog.txt;     Comment: {cm:ViewChangelog};                    WorkingDir: {app}
Name: {group}\ChangelogRus;                          Filename: {app}\Changelog.Rus.txt; Comment: {cm:ViewChangelog};                    WorkingDir: {app}
Name: {group}\{cm:ProgramOnTheWeb,{#app_name}};      Filename: {#app_url}

[Run]
Filename: "{app}\{#mpcbe_exe}";      WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent unchecked;           Description: "{cm:LaunchProgram,{#app_name}}"
Filename: "{app}\Changelog.txt";     WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent unchecked shellexec; Description: "{cm:ViewChangelog}"; Check: IsInactiveLang('ru')
Filename: "{app}\Changelog.Rus.txt"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent unchecked shellexec; Description: "{cm:ViewChangelog}"; Languages: ru

[InstallDelete]
Type: files; Name: "{userdesktop}\{#app_name}.lnk";   Check: not WizardIsTaskSelected('desktopicon\user')   and IsUpgrade()
Type: files; Name: "{commondesktop}\{#app_name}.lnk"; Check: not WizardIsTaskSelected('desktopicon\common') and IsUpgrade()
Type: files; Name: "{app}\AUTHORS";                   Check: IsUpgrade()
Type: files; Name: "{app}\ChangeLog";                 Check: IsUpgrade()
Type: files; Name: "{app}\ChangeLogRus";              Check: IsUpgrade()
Type: files; Name: "{app}\COPYING";                   Check: IsUpgrade()
Type: filesandordirs; Name: "{commonappdata}\{#app_name}\Shaders"; Tasks: reset_settings
Type: filesandordirs; Name: "{commonappdata}\{#app_name}\Shaders11"; Tasks: reset_settings
#ifdef localize
; remove the old language dlls when upgrading
Type: files; Name: "{app}\mpcresources.??.dll"
#endif

[UninstallDelete]

;[UninstallRun]
;Filename: "{app}\{#mpcbe_exe}"; Parameters: "/unregall"; WorkingDir: "{app}"; Flags: runhidden

;[Registry]
;Root: "HKCU"; Subkey: "Software\{#app_name}\ShellExt"; ValueType: string; ValueName: "MpcPath"; ValueData: "{app}\{#mpcbe_exe}"; Flags: uninsdeletekey; Components: mpcbeshellext

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Clients\Media\MPC-BE"; Flags: dontcreatekey uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\{#mpcbe_exe}"; ValueType: string; ValueName: ""; ValueData: "{app}\{#mpcbe_exe}"; Flags: deletekey uninsdeletekey

[Code]
function IsProcessorFeaturePresent(Feature: Integer): Boolean;
external 'IsProcessorFeaturePresent@kernel32.dll stdcall';

const
  installer_mutex = 'mpcbe_setup_mutex';
  LOAD_LIBRARY_AS_DATAFILE = $2;

  SHCONTCH_NOPROGRESSBOX = 4;
  SHCONTCH_RESPONDYESTOALL = 16;

function LoadLibraryEx(lpFileName: String; hFile: THandle; dwFlags: DWORD): THandle; external 'LoadLibraryExW@kernel32.dll stdcall';
function LoadString(hInstance: THandle; uID: SmallInt; var lpBuffer: Char; nBufferMax: Integer): Integer; external 'LoadStringW@user32.dll stdcall';
function SetFileAttributes(lpFileName: String; dwFileAttributes: Cardinal): Integer; external 'SetFileAttributesW@kernel32.dll stdcall';

var
  TasksList: TNewCheckListBox;

// thank for code to "El Sanchez" from forum.oszone.net
procedure PinToTaskbar(Filename: String; IsPin: Boolean);
var
  hInst: THandle;
  buf: array [0..255] of char;
  i, Res: Integer;
  strVerb, sVBSFile: String;
  objShell, colVerbs, oFile: Variant;
begin
  if (GetWindowsVersion shr 24 < 6) or ((GetWindowsVersion shr 24 = 6) and ((GetWindowsVersion shr 16) and $FF < 1)) then Exit; // Windows 7 check
  if not FileExists(Filename) then Exit;

  Log('Start PinToTaskbar');
  if IsPin then Res := 5386 else Res := 5387;

  begin
    hInst := LoadLibraryEx(ExpandConstant('{sys}\shell32.dll'), 0, LOAD_LIBRARY_AS_DATAFILE);
    if hInst <> 0 then
    try
      for i := 0 to LoadString(hInst, Res, buf[0], 255)-1 do strVerb := strVerb + Buf[i];
      try
        objShell := CreateOleObject('Shell.Application');
      except
        ShowExceptionMessage;
        Exit;
      end;
      oFile    := objShell.Namespace(ExtractFileDir(Filename)).ParseName(ExtractFileName(Filename));
      colVerbs := oFile.Verbs;

      if IsWin64 and (Pos (ExpandConstant ('{pf64}\'), Filename) = 1) then
      begin
        sVBSFile := GenerateUniqueName (GetTempDir, 'mpc_be.vbs');
        SaveStringToFile (sVBSFile, \
          'Set oShell=CreateObject("Shell.Application")'  + #13 + \
          'Set oVerbs=oShell.NameSpace("' + ExtractFileDir (Filename) + '").ParseName("' + ExtractFileName (Filename) + '").Verbs' + #13 + \
          'For Each oVerb In oVerbs'                      + #13 + \
          '  If (oVerb="' + strVerb + '") Then'           + #13 + \
          '    oVerb.DoIt'                                + #13 + \
          '    Exit For'                                  + #13 + \
          '  End If'                                      + #13 + \
          'Next'                                , False);

        exec( ExpandConstant ('{win}\Sysnative\cscript.exe'), '"' + sVBSFile + '" /B', '', SW_HIDE, ewWaitUntilTerminated, i);
        DeleteFile (sVBSFile);
      end else
      begin
        for i := colVerbs.Count downto 1 do if colVerbs.Item[i].Name = strVerb then
        begin
          if (IsPin and oFile.IsLink) then
            DeleteFile (ExpandConstant ('{userappdata}\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar\') + ExtractFileName (Filename));

          colVerbs.Item[i].DoIt;
          Break;
        end;
      end;
    finally
      FreeDLL(hInst);
    end;
  end;
  Log('PinToTaskbar done.');
end;

procedure Unzip(ZipPath, FileName, TargetPath: String); 
var
  ShellObj, ZipFile, Item, TargetFolder: Variant;
  str: String;
begin
  if FileExists(ZipPath) then
  begin
    Log('Start unziping ' + ZipPath);
    try
      ShellObj := CreateOleObject('Shell.Application');
      ZipFile := ShellObj.NameSpace(ZipPath);
      ForceDirectories(TargetPath);
      TargetFolder := ShellObj.NameSpace(TargetPath);
      if Length(FileName)>0 then
      begin
        Item := ZipFile.ParseName(FileName);
        TargetFolder.CopyHere(Item, SHCONTCH_NOPROGRESSBOX or SHCONTCH_RESPONDYESTOALL);
      end
      else
        TargetFolder.CopyHere(ZipFile.Items, SHCONTCH_NOPROGRESSBOX or SHCONTCH_RESPONDYESTOALL);
    except
      Log('ERROR: unziping failed');
      str := 'Unable to extract ' + ZipPath;
      if Length(FileName)>0 then str := str + '\' + FileName;
      MsgBox(str, mbError, MB_OK);
      Exit;
    end;
    Log('Unzip done.');
  end else
    Log('ERROR: File ' + ZipPath + ' not found');
end;

function GetInstallFolder(Default: String): String;
var
  sInstallPath: String;
begin
  Result := '';
  if RegQueryStringValue(HKLM, 'SOFTWARE\{#app_name}', 'ExePath', sInstallPath) then
  begin
    Result := ExtractFileDir(sInstallPath);
  end;

  if (Result = '') or not DirExists(Result) then
  begin
    Result := ExpandConstant('{pf}\{#app_name}');
  end;
end;

function D3DX9_43_DLLExists(): Boolean;
begin
  if FileExists(ExpandConstant('{sys}\d3dx9_43.dll')) then
    Result := True
  else
    Result := False;
end;

function D3DCompiler_47_DLLExists(): Boolean;
begin
  if FileExists(ExpandConstant('{sys}\d3dcompiler_47.dll')) then
    Result := True
  else
    Result := False;
end;

function Is_SSE2_Supported(): Boolean;
begin
  // PF_XMMI64_INSTRUCTIONS_AVAILABLE
  Result := IsProcessorFeaturePresent(10);
end;

function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;

function IniUsed(): Boolean;
begin
  Result := FileExists(ExpandConstant('{app}\{#mpcbe_ini}'));
end;

function LongPathIsEnabled(): Boolean;
var
  Value: Cardinal ;
begin
  if not RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\FileSystem', 'LongPathsEnabled', Value) then
    Value := 0;

  if Value = 0 then
    Result := False // LongPathsEnabled not found or not enabled
  else
    Result := True; // is enabled
end;

// Check if settings exist
function SettingsExist(): Boolean;
begin
  if RegKeyExists(HKEY_CURRENT_USER, 'Software\{#app_name}') or
  FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
    Result := True
  else
    Result := False;
end;

function GetCustomTask(TaskIndex: Integer): Boolean;
begin 
  Result := TasksList.Checked[TaskIndex];
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the License page
  if IsUpgrade() and (PageID = wpLicense) then
    Result := True;
end;

procedure CleanUpSettingsAndFiles();
Var
  resCode: integer;
begin
  Log('Start CleanUpSettingsAndFiles');
  // Unregister all extensions, include custom
  Exec(ExpandConstant('{app}\{#mpcbe_exe}'), ' /unregall', ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, resCode);

  DeleteFile(ExpandConstant('{app}\{#mpcbe_ini}'));
  DeleteFile(ExpandConstant('{app}\default.mpcpl'));
  DeleteFile(ExpandConstant('{userappdata}\{#app_name}\default.mpcpl'));
  RemoveDir(ExpandConstant('{userappdata}\{#app_name}'));
  RegDeleteKeyIncludingSubkeys(HKCU, 'Software\{#app_name}');
  RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\{#app_name}');
  Log('CleanUpSettingsAndFiles done.');
end;

procedure CurPageChanged(CurPageID: Integer); 
  begin 
  if CurPageID = wpReady then 
  begin 
  if GetCustomTask(0) then 
      WizardForm.ReadyMemo.Lines.Add(ExpandConstant('      {cm:AssociationVideo}'));
	if GetCustomTask(1) then 
      WizardForm.ReadyMemo.Lines.Add(ExpandConstant('      {cm:AssociationAudio}')); 
	if GetCustomTask(2) then 
      WizardForm.ReadyMemo.Lines.Add(ExpandConstant('      {cm:AssociationPlaylist}')); 
  end; 
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  sLanguage: String;
  sRegParams: String;
  resCode: integer;
  checksum: String;
begin
  if CurStep = ssPostInstall then
  begin
    if WizardIsTaskSelected('pintotaskbar') then
      PinToTaskbar(ExpandConstant('{app}\{#mpcbe_exe}'), True);

    if WizardIsTaskSelected('longpathsenable') then
      RegWriteDWordValue(HKLM, 'SYSTEM\CurrentControlSet\Control\FileSystem', 'LongPathsEnabled', 1);

    if WizardIsTaskSelected('reset_settings') then
      CleanUpSettingsAndFiles();

    sLanguage := ExpandConstant('{cm:langcode}');
    RegWriteStringValue(HKLM, 'SOFTWARE\{#app_name}', 'ExePath', ExpandConstant('{app}\{#mpcbe_exe}'));

    if WizardIsComponentSelected('mpcresources') and FileExists(ExpandConstant('{app}\{#mpcbe_ini}')) then
      SetIniString('Settings', 'Language', sLanguage, ExpandConstant('{app}\{#mpcbe_ini}'))
    else
      RegWriteStringValue(HKCU, 'Software\{#app_name}\Settings', 'Language', sLanguage);

    if GetCustomTask(0) or GetCustomTask(1) or GetCustomTask(2) then
    begin
      if GetCustomTask(0) then
        sRegParams := sRegParams + ' /regvid';
      if GetCustomTask(1) then 
        sRegParams := sRegParams + ' /regaud';
      if GetCustomTask(2) then
        sRegParams := sRegParams + ' /regpl';
      Exec(ExpandConstant('{app}\{#mpcbe_exe}'), sRegParams, ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, resCode);
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usUninstall) then
    PinToTaskbar(ExpandConstant('{app}\{#mpcbe_exe}'), False);

  // When uninstalling, ask the user to delete settings
  if ((CurUninstallStep = usUninstall) and SettingsExist()) then
  begin
    if SuppressibleMsgBox(CustomMessage('msg_DeleteSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
    begin
      DelTree(ExpandConstant('{userappdata}\{#app_name}\Shaders\*.psh'), False, True, False);
      DelTree(ExpandConstant('{userappdata}\{#app_name}\Shaders\*.hlsl'), False, True, False);
      RemoveDir(ExpandConstant('{userappdata}\{#app_name}\Shaders'));
      DelTree(ExpandConstant('{app}\Shaders\*.psh'), False, True, False);
      DelTree(ExpandConstant('{app}\Shaders\*.hlsl'), False, True, False);
      RemoveDir(ExpandConstant('{app}\Shaders'));
      DelTree(ExpandConstant('{userappdata}\{#app_name}\Shaders11\*.hlsl'), False, True, False);
      RemoveDir(ExpandConstant('{userappdata}\{#app_name}\Shaders11'));
      DelTree(ExpandConstant('{app}\Shaders11\*.hlsl'), False, True, False);
      RemoveDir(ExpandConstant('{app}\Shaders11'));
      CleanUpSettingsAndFiles();
    end;
  end;
end;

function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running display a message and stop installation
  if CheckForMutexes(installer_mutex) and not WizardSilent() then
  begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end else
  begin
    Result := True;
    CreateMutex(installer_mutex);

    if not Is_SSE2_Supported() then
    begin
      SuppressibleMsgBox(CustomMessage('msg_simd_sse2'), mbCriticalError, MB_OK, MB_OK);
      Result := False;
    end;
  end;
end;

function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex) then
  begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end else
  begin
    Result := True;
    CreateMutex(installer_mutex);
  end;
end;

function IsInactiveLang(Lang: String): Boolean;
begin
  if ActiveLanguage() = lang then
  begin
    Result := False;
  end else
  begin
    Result := True;
  end;
end;

procedure InitializeWizard();
Var
  DeltaY : Integer;
  Idx : Integer;
  Path : String;
  CustomSelectTasksPage: TWizardPage;
begin
  DeltaY := ScaleY(1);

  WizardForm.ComponentsList.Height := WizardForm.ComponentsList.Height + DeltaY;

  //Idx := WizardForm.ComponentsList.Items.IndexOf(ExpandConstant('{cm:comp_intel_msdk}'));
  //WizardForm.ComponentsList.Checked[Idx] := False;
  //Idx := WizardForm.ComponentsList.Items.IndexOf('{#mpcvr_desc}');
  //WizardForm.ComponentsList.Checked[Idx] := False;

  CustomSelectTasksPage := CreateCustomPage(wpSelectTasks, SetupMessage(msgWizardSelectTasks), SetupMessage(msgSelectTasksDesc));
  TasksList := TNewCheckListBox.Create(WizardForm);
  TasksList.Left := WizardForm.TasksList.Left; 
  TasksList.Top := WizardForm.SelectTasksLabel.Top; 
  TasksList.Width := WizardForm.TasksList.Width; 
  TasksList.Height := WizardForm.TasksList.Top + WizardForm.TasksList.Height - WizardForm.SelectTasksLabel.Top; 
  TasksList.BorderStyle := WizardForm.TasksList.BorderStyle;
  TasksList.Color := WizardForm.TasksList.Color;
  TasksList.ShowLines := WizardForm.TasksList.ShowLines;
  TasksList.MinItemHeight := WizardForm.TasksList.MinItemHeight;
  TasksList.ParentColor := WizardForm.TasksList.ParentColor;
  TasksList.WantTabs := WizardForm.TasksList.WantTabs;
  TasksList.Parent := CustomSelectTasksPage.Surface;

  TasksList.AddCheckBox(ExpandConstant('{cm:AssociationVideo}'),    '', 0, False, True, False, True, nil);
  TasksList.AddCheckBox(ExpandConstant('{cm:AssociationAudio}'),    '', 0, False, True, False, True, nil);
  TasksList.AddCheckBox(ExpandConstant('{cm:AssociationPlaylist}'), '', 0, False, True, False, True, nil);
end;
