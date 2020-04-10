#define OtterName "Otter Browser"
#define OtterUrl "https://otter-browser.org/"
#define OtterExeName "otter-browser.exe"
#define OtterIdentifier "OtterBrowser"
#ifndef OtterVersion
#define OtterVersion "1.0.81"
#endif
#ifndef OtterContext
#define OtterContext "-dev"
#endif
#ifndef OtterWorkingDir
#define OtterWorkingDir "Z:\otter-browser-inno"
#endif

[Setup]
AppId={{A0517512-5271-465D-AE59-D08F487B5CAF}
AppName={#OtterName}
AppVersion={#OtterVersion}{#OtterContext}
AppPublisher="Otter Browser Team"
AppPublisherURL={#OtterUrl}
AppSupportURL={#OtterUrl}
AppUpdatesURL={#OtterUrl}
DefaultDirName={pf}\{#OtterName}
DefaultGroupName={#OtterName}
AllowNoIcons=yes
LicenseFile={#OtterWorkingDir}\input\COPYING
OutputDir={#OtterWorkingDir}
#ifdef OtterWin64
OutputBaseFilename=otter-browser-win64-{#OtterVersion}-setup
#else
OutputBaseFilename=otter-browser-win32-{#OtterVersion}-setup
#endif
Compression=lzma2/ultra64
SolidCompression=yes
VersionInfoVersion={#OtterVersion}
#ifdef OtterWin64
ArchitecturesAllowed=x64 ia64
ArchitecturesInstallIn64BitMode=x64 ia64
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "{#OtterWorkingDir}\input\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Registry]
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities"; ValueName: "ApplicationDescription"; ValueType: String; ValueData: "Web browser controlled by the user, not vice-versa"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities"; ValueName: "ApplicationIcon"; ValueType: String; ValueData: "{app}\{#OtterExeName},0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities"; ValueName: "ApplicationName"; ValueType: String; ValueData: "{#OtterName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\UrlAssociations"; ValueName: "http"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\UrlAssociations"; ValueName: "https"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\UrlAssociations"; ValueName: "ftp"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\FileAssociations"; ValueName: ".html"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\FileAssociations"; ValueName: ".htm"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\FileAssociations"; ValueName: ".xhtml"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\FileAssociations"; ValueName: ".shtml"; ValueType: String; ValueData: "{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities\Startmenu"; ValueName: "StartMenuInternet"; ValueType: String; ValueData: "{#OtterName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\shell\open\command"; ValueName: ""; ValueType: String; ValueData: "{app}\{#OtterExeName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\DefaultIcon"; ValueName: ""; ValueType: String; ValueData: "{app}\{#OtterExeName},0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\InstallInfo"; ValueName: "IconsVisible"; ValueType: dword; ValueData: "1"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\RegisteredApplications"; ValueType: String; ValueName: "{#OtterIdentifier}"; ValueData: "Software\Clients\StartMenuInternet\{#OtterIdentifier}\Capabilities"; Flags: uninsdeletevalue uninsdeletekeyifempty

Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}"; ValueName: ""; ValueType: String; ValueData: "{#OtterName} Document"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}"; ValueName: "FriendlyTypeName"; ValueType: String; ValueData: "{#OtterName} Document"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}"; ValueName: "EditFlags"; ValueType: dword; ValueData: "2"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}\DefaultIcon"; ValueName: ""; ValueType: String; ValueData: "{app}\{#OtterExeName},1"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}\shell\open\ddeexec"; ValueName: ""; ValueType: String; ValueData: ""; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#OtterIdentifier}\shell\open\command"; ValueName: ""; ValueType: String; ValueData: "{%|%22}{app}\{#OtterExeName}{%|%22} {%|%22}%1{%|%22}"; Flags: uninsdeletekey

[Icons]
Name: "{group}\{#OtterName}"; Filename: "{app}\{#OtterExeName}"
Name: "{commondesktop}\{#OtterName}"; Filename: "{app}\{#OtterExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#OtterName}"; Filename: "{app}\{#OtterExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#OtterExeName}"; Description: "{cm:LaunchProgram,{#StringChange(OtterName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
