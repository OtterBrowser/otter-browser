#define MyAppName "Otter Browser"
#define MyAppVersion "0.9.07-dev"
#define MyAppURL "http://otter-browser.org/"
#define MyAppExeName "otter-browser.exe"
#define MyAppIdentifier "OtterBrowser"

#ifndef SRC_DIR
  #define SRC_DIR "Z:\otter-browser-inno\input"
#endif
#ifndef OUT_DIR
  #ifdef _OUT_DIR
    ;iscc 5.5.5 interprets -DOUT_DIR as -DO, which disables output
    #define OUT_DIR _OUT_DIR
  #else
    #define OUT_DIR "Z:\otter-browser-inno\output"
  #endif
#endif

[Setup]
AppId={{A0517512-5271-465D-AE59-D08F487B5CAF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
InfoBeforeFile=inno\gpl3.rtf
OutputDir={#OUT_DIR}
OutputBaseFilename=otter-browser-{#MyAppVersion}-setup
Compression=lzma2/ultra64
SolidCompression=yes

;LZMANumBlockThreads=4
LZMABlockSize=76800
LZMAUseSeparateProcess=yes
LZMANumFastBytes=256

WizardImageFile=inno\images\wizardimage.bmp

#ifdef SIGNING
SignedUninstaller=yes
SignedUninstallerDir=uninst
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl,inno\lang\en.extra.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,inno\lang\pt_BR.extra.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl,inno\lang\de.extra.isl"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl,inno\lang\hu.extra.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl,inno\lang\it.extra.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "nepali"; MessagesFile: "compiler:Languages\Nepali.islu"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl,inno\lang\pl.extra.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl,inno\lang\ru.extra.isl"
Name: "scottishgaelic"; MessagesFile: "compiler:Languages\ScottishGaelic.isl"
Name: "serbiancyrillic"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "serbianlatin"; MessagesFile: "compiler:Languages\SerbianLatin.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl,inno\lang\sl.extra.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl,inno\lang\es.extra.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "inno\images\wizardimage_small.bmp"; Flags: dontcopy
Source: "inno\images\installsplash.bmp"; Flags: dontcopy
Source: "inno\images\installsplash_small.bmp"; Flags: dontcopy
#ifndef TESTING
Source: "{#SRC_DIR}\otter-browser.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\icudt53.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\icuin53.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\icuuc53.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\Qt5*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SRC_DIR}\iconengines\*.*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs
Source: "{#SRC_DIR}\imageformats\*.*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs
Source: "{#SRC_DIR}\locale\*.*"; DestDir: "{app}\locale"; Flags: ignoreversion recursesubdirs
Source: "{#SRC_DIR}\platforms\*.*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "{#SRC_DIR}\printsupport\*.*"; DestDir: "{app}\printsupport"; Flags: ignoreversion recursesubdirs
Source: "{#SRC_DIR}\sqldrivers\*.*"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs
#endif

[Registry]
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities"; ValueName: "ApplicationDescription"; ValueType: String; ValueData: "Web browser controlled by the user, not vice-versa"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities"; ValueName: "ApplicationIcon"; ValueType: String; ValueData: "{app}\{#MyAppExeName},0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities"; ValueName: "ApplicationName"; ValueType: String; ValueData: "{#MyAppName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\UrlAssociations"; ValueName: "http"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\UrlAssociations"; ValueName: "https"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\UrlAssociations"; ValueName: "ftp"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\FileAssociations"; ValueName: ".html"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\FileAssociations"; ValueName: ".htm"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\FileAssociations"; ValueName: ".xhtml"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\FileAssociations"; ValueName: ".shtml"; ValueType: String; ValueData: "{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities\Startmenu"; ValueName: "StartMenuInternet"; ValueType: String; ValueData: "{#MyAppName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\shell\open\command"; ValueName: ""; ValueType: String; ValueData: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\DefaultIcon"; ValueName: ""; ValueType: String; ValueData: "{app}\{#MyAppExeName},0"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\InstallInfo"; ValueName: "IconsVisible"; ValueType: dword; ValueData: "1"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\RegisteredApplications"; ValueType: String; ValueName: "{#MyAppIdentifier}"; ValueData: "Software\Clients\StartMenuInternet\{#MyAppIdentifier}\Capabilities"; Flags: uninsdeletevalue uninsdeletekeyifempty

Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}"; ValueName: ""; ValueType: String; ValueData: "{#MyAppName} Document"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}"; ValueName: "FriendlyTypeName"; ValueType: String; ValueData: "{#MyAppName} Document"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}"; ValueName: "EditFlags"; ValueType: dword; ValueData: "2"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}\DefaultIcon"; ValueName: ""; ValueType: String; ValueData: "{app}\{#MyAppExeName},1"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}\shell\open\ddeexec"; ValueName: ""; ValueType: String; ValueData: ""; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\{#MyAppIdentifier}\shell\open\command"; ValueName: ""; ValueType: String; ValueData: "{%|%22}{app}\{#MyAppExeName}{%|%22} {%|%22}%1{%|%22}"; Flags: uninsdeletekey

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
const
	RTFHeader = '{\rtf1\deff0{\fonttbl{\f0\fswiss\fprq2\fcharset0 Tahoma;}{\f1\fnil\fcharset2 Symbol;}}\viewkind4\uc1\fs16';
	RTFPara	  = '\par ';

var
	ReadyMemoRichText: String;

	InstallMode: (imNone, imSimple, imCustom, imRebootContinue);
	btnInstall, btnCustomize: TNewButton;

//#include "utils.isi"

procedure CleanUpCustomWelcome();
begin
	WizardForm.NextButton.Visible := True;
	btnInstall.Visible := False;
	btnCustomize.Visible := False;

	WizardForm.Bevel.Visible := True;
end;


procedure InstallOnClick(Sender: TObject);
begin
	InstallMode := imSimple;

	CleanUpCustomWelcome();

	WizardForm.NextButton.OnClick(TNewButton(Sender).Parent);
end;


procedure CustomizeOnClick(Sender: TObject);
begin
	InstallMode := imCustom;
	
	CleanUpCustomWelcome();

	WizardForm.NextButton.OnClick(TNewButton(Sender).Parent);
end;


procedure InitCustomPages();
begin
	btnInstall := TNewButton.Create(WizardForm);
	with btnInstall do
	begin
		Parent := WizardForm;
		Width := WizardForm.NextButton.Width;
		Height := WizardForm.NextButton.Height;
		Left := WizardForm.NextButton.Left;
		Top := WizardForm.NextButton.Top;
		Caption := SetupMessage(msgButtonInstall);
		Default := True;
		Visible := False;

		OnClick := @InstallOnClick;
	end;

	btnCustomize := TNewButton.Create(WizardForm);
	with btnCustomize do
	begin
		Parent := WizardForm;
		Width := WizardForm.NextButton.Width;
		Height := WizardForm.NextButton.Height;
		Left := WizardForm.ClientWidth - (WizardForm.CancelButton.Left + WizardForm.CancelButton.Width);
		Top := WizardForm.NextButton.Top;
		Visible := False;

		Caption := CustomMessage('Customize');

		OnClick := @CustomizeOnClick;
	end;

end;


procedure ReadyFaceLift();
var rtfNewReadyMemo: TRichEditViewer;
begin
	WizardForm.ReadyMemo.Visible := False;

	rtfNewReadyMemo := TRichEditViewer.Create(WizardForm.ReadyMemo.Parent);
	with rtfNewReadyMemo do
	begin
		Visible := False;
		Parent := WizardForm.ReadyMemo.Parent;
		Scrollbars := ssVertical;
		Color := WizardForm.Color;
		UseRichEdit := True;
		RTFText := ReadyMemoRichText;
		ReadOnly := True;
		Left := WizardForm.ReadyMemo.Left;
		Top := WizardForm.ReadyMemo.Top;
		Width := WizardForm.ReadyMemo.Width;
		Height := WizardForm.ReadyMemo.Height;
		Visible := True;
	end;
end;


procedure InstallingFaceLift();
begin
	with WizardForm.ProgressGauge do //make the progressbar shorter, and move it to the bottom of the installing page
	begin
		Height := ScaleY(16);
		Top := WizardForm.InstallingPage.ClientHeight - Top - Height;

		WizardForm.StatusLabel.Top := Top - WizardForm.FilenameLabel.Height - ScaleY(4);
		WizardForm.FilenameLabel.Top := Top + Height + ScaleY(4);
	end;
end;


function CopyW(const S: String; const Start, Len: Integer): String; //work-around for unicode-related bug in Copy
begin                                                               //TODO: is this still needed?
	Result := Copy(S, Start, Len);
end;


function Unicode2RTF(const pIn: String): String; //convert to RTF-compatible unicode
var	i: Integer;
	c: SmallInt;
begin
	Result := '';
	for i := 1 to Length(pIn) do
		if Ord(pIn[i]) <= 127 then
		begin
			Result := Result + pIn[i];
		end else
		begin
			c := Ord(pIn[i]); //code points above 7FFF must be expressed as negative numbers, that's why SmallInt is used
			Result := Result + '\u' + IntToStr(c) + '?';
		end;
end;


function Replace(pSearchFor, pReplaceWith, pText: String): String;
begin
    StringChangeEx(pText,pSearchFor,pReplaceWith,True);

	Result := pText;
end;


function ParseReadyMemoText(pSpaces,pText: String): String;
var sTemp: String;
begin

	sTemp := CopyW(pText,Pos(#10,pText)+1,Length(pText));
	sTemp := Replace('\','\\',sTemp);
	sTemp := Replace('{','\{',sTemp);
	sTemp := Replace('}','\}',sTemp);
	sTemp := Replace(#13#10,RTFpara,sTemp);
	sTemp := Replace(pSpaces,'',sTemp);
	sTemp := '\b ' + CopyW(pText,1,Pos(#13,pText)-1) + '\par\sb0' +
						'\li284\b0 ' + sTemp + '\par \pard';

	Result := Unicode2RTF(sTemp);
end;


function UpdateReadyMemo(pSpace, pNewLine, pMemoUserInfo, pMemoDirInfo, pMemoTypeInfo, pMemoComponentsInfo, pMemoGroupInfo, pMemoTasksInfo: String): String;
var sText: String;
begin
	(* Prepare the text for new Ready Memo *)
	
	sText := RTFHeader;
	if pMemoDirInfo <> '' then
		sText := sText + ParseReadyMemoText(pSpace,pMemoDirInfo);

	if pMemoTypeInfo <> '' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoTypeInfo);

	if pMemoComponentsInfo <> '' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoComponentsInfo);

	if pMemoGroupInfo <> '' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoGroupInfo);

	If pMemoTasksInfo<>'' then
		sText := sText + '\sb100' + ParseReadyMemoText(pSpace,pMemoTasksInfo);
		
	ReadyMemoRichText := Copy(sText,1,Length(sText)-6) + '}';

	Result := 'If you see this, something went wrong';
end;


procedure UpdateWizardImages();
var NewBitmap1,NewBitmap2: TFileStream;
begin
	if WizardForm.WizardBitmapImage.Height < 386 then //use smaller image when not using Large Fonts
	begin
		try
			NewBitmap1 := TFileStream.Create(ExpandConstant('{tmp}\installsplash_small.bmp'),fmOpenRead);
			WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(NewBitmap1);
			try
				NewBitmap2 := TFileStream.Create(ExpandConstant('{tmp}\wizardimage_small.bmp'),fmOpenRead);
				WizardForm.WizardBitmapImage2.Bitmap.LoadFromStream(NewBitmap2);
			except
				Log('Error loading image: ' + GetExceptionMessage);
			finally
				if NewBitmap2 <> nil then
					NewBitmap2.Free;
			end;
		except
			Log('Error loading image: ' + GetExceptionMessage);
		finally
			if NewBitmap1 <> nil then
				NewBitmap1.Free;
		end;
	end
	else
	begin
		try
			NewBitmap1 := TFileStream.Create(ExpandConstant('{tmp}\installsplash.bmp'),fmOpenRead);
			WizardForm.WizardBitmapImage.Bitmap.LoadFromStream(NewBitmap1);
		except
			Log('Error loading image: ' + GetExceptionMessage);
		finally
			if NewBitmap1 <> nil then
				NewBitmap1.Free;
		end;
	end;
	WizardForm.WizardBitmapImage.Width := WizardForm.ClientWidth;
end;


procedure InitializeWizard();
begin
	UpdateWizardImages();
	InitCustomPages();
end;


function InitializeSetup(): Boolean;
begin

	try
		ExtractTemporaryFile('wizardimage_small.bmp');
		ExtractTemporaryFile('installsplash.bmp');
		ExtractTemporaryFile('installsplash_small.bmp');
	except
		Log('Error extracting temporary file');
		MsgBox(CustomMessage('ErrorExtractingTemp'),mbError,MB_OK);
		Result := False;
		exit;
	end;

	Result := True;

end;


procedure InfoBeforeLikeLicense();
begin
	WizardForm.InfoBeforeClickLabel.Visible := False;
	WizardForm.InfoBeforeMemo.Height := WizardForm.InfoBeforeMemo.Height + WizardForm.InfoBeforeMemo.Top - WizardForm.InfoBeforeClickLabel.Top;
	WizardForm.InfoBeforeMemo.Top := WizardForm.InfoBeforeClickLabel.Top;
end;


procedure PrepareWelcomePage();
begin
	if not WizardSilent then
	begin
		WizardForm.NextButton.Visible := False;

		btnInstall.Visible := True;
		btnInstall.TabOrder := 1;
		btnCustomize.Visible := True;

		WizardForm.Bevel.Visible := False;
		WizardForm.WelcomeLabel1.Visible := False;
		WizardForm.WelcomeLabel2.Visible := False;
	end;
end;


procedure CurPageChanged(pCurPageID: Integer); 
begin
	case pCurPageID of
		wpWelcome:
			PrepareWelcomePage();
		wpInfoBefore:
			InfoBeforeLikeLicense();
		wpReady:
			ReadyFaceLift();
		wpInstalling:
			InstallingFaceLift();
	end;
end;


function ShouldSkipPage(pPageID: Integer): Boolean;
begin
	Result := ((InstallMode = imSimple) or (InstallMode = imRebootContinue)) and (pPageID <> wpFinished);
	                                                               //skip all pages except for the finished one if using simple
	                                                               //install mode
end;
