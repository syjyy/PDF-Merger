; Compile after scripts/build.ps1. The application remains entirely C++.
#ifndef AppVersion
  #define AppVersion "1.2.0"
#endif
#ifndef PayloadDir
  #define PayloadDir "..\dist\OrderedPdfMerger"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
#define OfficeVersion "26.8.2"
#define OfficeName "LibreOffice_26.8.2_Win_x86-64.msi"
#define OfficeUrl "https://download.documentfoundation.org/libreoffice/stable/26.8.2/win/x86_64/LibreOffice_26.8.2_Win_x86-64.msi"
#define OfficeHash "ff4b911403b67ee9e6249f820f9782c44bb32c94fdaa45b0da53a8e419e5307a"

[Setup]
AppId={{40B7A4AA-91E4-4ABA-BFC3-775486329D38}
AppName=PDF Merger
AppVersion={#AppVersion}
AppPublisher=PDF Merger contributors
DefaultDirName={autopf}\PDF Merger
DefaultGroupName=PDF Merger
DisableProgramGroupPage=yes
DisableDirPage=no
LicenseFile=..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=PDF-Merger-Setup-windows-x64
SetupIconFile=..\static\app-icon.ico
UninstallDisplayIcon={app}\OrderedPdfMerger.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"

[CustomMessages]
english.OfficeTitle=Word and Excel support
english.OfficeDescription=Configure the LibreOffice conversion engine.
english.OfficePrompt=PDF and images work without LibreOffice. Choose how to enable Word / Excel conversion:
english.OfficeInstall=Download and install LibreOffice {#OfficeVersion} (Internet and administrator access required)
english.OfficeExisting=Use installed LibreOffice (automatically detected; otherwise choose its folder)
english.OfficeSkip=Skip for now (PDF and images only)
english.ExistingTitle=Existing LibreOffice
english.ExistingDescription=Select the conversion engine executable.
english.ExistingPrompt=Automatic detection found no usable installation. Select the LibreOffice installation folder or its program subfolder. Setup locates the conversion executable for you.
english.OfficeFile=LibreOffice folder (no need to find an EXE):
english.DirectoryTitle=LibreOffice installation folder
english.DirectoryDescription=Choose a separate installation location for LibreOffice.
english.DirectoryPrompt=The official LibreOffice MSI will install system-wide here. If the selected folder contains other files, a LibreOffice subfolder will be chosen automatically (numbered if needed). Setup creates the folder; no manual creation is required. LibreOffice remains installed after uninstalling this tool. Download size is several hundred MB.
english.DirectoryLabel=LibreOffice installation folder:
english.InvalidOffice=LibreOffice was not found in this folder. Select its installation folder, or go back and choose download and install.
english.InvalidDirectory=Choose an absolute local folder outside Windows, the app installation folder, and temporary folders. Setup will automatically choose and create a suitable subfolder when needed.
english.DownloadTitle=Downloading LibreOffice
english.DownloadDescription=Downloading the official package and checking its SHA-256 hash.
english.InstallingOffice=Installing LibreOffice. This may take several minutes...
english.OfficeFailed=LibreOffice installation failed (code %1). The application was not installed. Retry, choose an existing installation, or skip Word / Excel support.
english.OfficeMissing=LibreOffice installation completed, but soffice.exe was not found in the selected folder. Retry or choose its actual path.
english.OfficeReady=LibreOffice: %1
english.SkipSummary=LibreOffice: skipped (PDF and images only)
english.DesktopIcon=Create a desktop shortcut
english.Launch=Launch PDF Merger
chinesesimplified.OfficeTitle=Word 和 Excel 支持
chinesesimplified.OfficeDescription=配置 LibreOffice 文档转换引擎。
chinesesimplified.OfficePrompt=PDF 和图片不需要 LibreOffice。请选择 Word / Excel 转换方式：
chinesesimplified.OfficeInstall=下载并安装 LibreOffice {#OfficeVersion}（需要网络和管理员权限）
chinesesimplified.OfficeExisting=使用已安装的 LibreOffice（自动识别；找不到时只需选文件夹）
chinesesimplified.OfficeSkip=暂时跳过（仅使用 PDF 和图片）
chinesesimplified.ExistingTitle=选择已安装的 LibreOffice
chinesesimplified.ExistingDescription=选择文档转换程序。
chinesesimplified.ExistingPrompt=未自动找到可用安装。请选择 LibreOffice 安装文件夹或其中的 program 文件夹，向导会自动定位转换程序，无需翻找 EXE。也可返回选择下载安装。
chinesesimplified.OfficeFile=LibreOffice 安装文件夹（无需选择 EXE）：
chinesesimplified.DirectoryTitle=LibreOffice 安装位置
chinesesimplified.DirectoryDescription=为 LibreOffice 选择独立的安装目录。
chinesesimplified.DirectoryPrompt=使用官方 MSI 为所有用户安装。所选目录如有其他文件，会自动补上 LibreOffice 子目录；重名时自动编号，无需手动新建。卸载本工具时会保留 LibreOffice。需下载数百 MB，安装后在本机转换。
chinesesimplified.DirectoryLabel=LibreOffice 安装目录：
chinesesimplified.InvalidOffice=此文件夹内未找到 LibreOffice。请选择其安装文件夹，或返回选择下载安装。
chinesesimplified.InvalidDirectory=请选择 Windows、应用安装目录和临时目录以外的本机绝对路径。需要子目录时，安装器会自动补充并创建，无需手动新建。
chinesesimplified.DownloadTitle=正在下载 LibreOffice
chinesesimplified.DownloadDescription=从官方地址下载，并核对 SHA-256 校验值。
chinesesimplified.InstallingOffice=正在安装 LibreOffice，可能需要数分钟…
chinesesimplified.OfficeFailed=LibreOffice 安装失败（代码 %1），本工具尚未安装。可重试、选择已有安装，或跳过 Word / Excel 支持。
chinesesimplified.OfficeMissing=LibreOffice 安装完成，但指定目录未找到 soffice.exe。请重试或选择其实际路径。
chinesesimplified.OfficeReady=LibreOffice：%1
chinesesimplified.SkipSummary=LibreOffice：已跳过（仅 PDF 和图片）
chinesesimplified.DesktopIcon=创建桌面快捷方式
chinesesimplified.Launch=启动 PDF Merger

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\PDF Merger"; Filename: "{app}\OrderedPdfMerger.exe"
Name: "{autodesktop}\PDF Merger"; Filename: "{app}\OrderedPdfMerger.exe"; Tasks: desktopicon
Name: "{group}\Uninstall PDF Merger"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: files; Name: "{app}\settings.ini"

[Run]
Filename: "{app}\OrderedPdfMerger.exe"; Description: "{cm:Launch}"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Code]
var
  OfficePage: TInputOptionWizardPage;
  ExistingPage: TInputDirWizardPage;
  OfficeDirectoryPage: TInputDirWizardPage;
  DownloadPage: TDownloadWizardPage;
  OfficeExecutable: String;
  OfficeInstalled: Boolean;
  OfficeRestart: Boolean;

function ResolveOffice(Path: String): String;
begin
  Result := '';
  Path := Trim(Path);
  if (Length(Path) > 1) and (Path[1] = '"') and (Path[Length(Path)] = '"') then
    Path := Copy(Path, 2, Length(Path) - 2);
  if (CompareText(ExtractFileName(Path), 'soffice.exe') = 0) and FileExists(Path) then
    Result := Path
  else if FileExists(AddBackslash(Path) + 'program\soffice.exe') then
    Result := AddBackslash(Path) + 'program\soffice.exe'
  else if FileExists(AddBackslash(Path) + 'soffice.exe') then
    Result := AddBackslash(Path) + 'soffice.exe';
end;

function RegistryOffice(Root: Integer): String;
var Value: String;
begin
  Result := '';
  if RegQueryStringValue(Root, 'Software\Microsoft\Windows\CurrentVersion\App Paths\soffice.exe', '', Value) then
    Result := ResolveOffice(Value);
  if Result <> '' then Exit;
  if RegQueryStringValue(Root, 'Software\LibreOffice\UNO\InstallPath', '', Value) then
    Result := ResolveOffice(Value);
end;

function DetectedOffice(Directory: String): String;
var
  Candidate: String;
begin
  { InitializeWizard runs before the app constant is available. }
  Result := ResolveOffice(GetIniString('Dependencies', 'LibreOffice', '', AddBackslash(Directory) + 'settings.ini'));
  if Result <> '' then Exit;
  Result := RegistryOffice(HKCU64);
  if Result <> '' then Exit;
  Result := RegistryOffice(HKLM64);
  if Result <> '' then Exit;
  Result := RegistryOffice(HKCU32);
  if Result <> '' then Exit;
  Result := RegistryOffice(HKLM32);
  if Result <> '' then Exit;
  Candidate := ExpandConstant('{pf64}\LibreOffice\program\soffice.exe');
  if FileExists(Candidate) then begin Result := Candidate; Exit; end;
  Candidate := ExpandConstant('{pf32}\LibreOffice\program\soffice.exe');
  if FileExists(Candidate) then begin Result := Candidate; Exit; end;
  Result := '';
end;

procedure InitializeWizard;
var Found: String;
begin
  Found := DetectedOffice(ExpandConstant('{autopf}\PDF Merger'));
  OfficePage := CreateInputOptionPage(wpSelectDir, CustomMessage('OfficeTitle'),
    CustomMessage('OfficeDescription'), CustomMessage('OfficePrompt'), True, False);
  OfficePage.Add(CustomMessage('OfficeInstall'));
  if Found <> '' then OfficePage.Add(CustomMessage('OfficeExisting') + #13#10 + Found)
  else OfficePage.Add(CustomMessage('OfficeExisting'));
  OfficePage.Add(CustomMessage('OfficeSkip'));
  ExistingPage := CreateInputDirPage(OfficePage.ID, CustomMessage('ExistingTitle'),
    CustomMessage('ExistingDescription'), CustomMessage('ExistingPrompt'), False, '');
  ExistingPage.Add(CustomMessage('OfficeFile'));
  OfficeDirectoryPage := CreateInputDirPage(ExistingPage.ID, CustomMessage('DirectoryTitle'),
    CustomMessage('DirectoryDescription'), CustomMessage('DirectoryPrompt'), False, '');
  OfficeDirectoryPage.Add(CustomMessage('DirectoryLabel'));
  OfficeDirectoryPage.Values[0] := ExpandConstant('{pf64}\LibreOffice');
  ExistingPage.Values[0] := Found;
  if Found <> '' then OfficePage.SelectedValueIndex := 1
  else OfficePage.SelectedValueIndex := 0;
  { Unattended installs never silently acquire a large external dependency. }
  if WizardSilent then OfficePage.SelectedValueIndex := 2;
  DownloadPage := CreateDownloadPage(CustomMessage('DownloadTitle'), CustomMessage('DownloadDescription'), nil);
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := ((PageID = ExistingPage.ID) and ((OfficePage.SelectedValueIndex <> 1) or
    (ResolveOffice(ExistingPage.Values[0]) <> ''))) or
    ((PageID = OfficeDirectoryPage.ID) and (OfficePage.SelectedValueIndex <> 0));
end;

function Within(Path, Parent: String): Boolean;
begin
  Path := Lowercase(AddBackslash(ExpandFileName(Path)));
  Parent := Lowercase(AddBackslash(ExpandFileName(Parent)));
  Result := Pos(Parent, Path) = 1;
end;

function DirectoryHasEntries(Path: String): Boolean;
var Find: TFindRec;
begin
  Result := False;
  if FindFirst(AddBackslash(Path) + '*', Find) then begin
    try
      repeat
        if (Find.Name <> '.') and (Find.Name <> '..') then begin
          Result := True;
          Exit;
        end;
      until not FindNext(Find);
    finally FindClose(Find); end;
  end;
end;

procedure CompleteOfficeDirectory;
var Path, Candidate: String; Number: Integer;
begin
  Path := Trim(OfficeDirectoryPage.Values[0]);
  if (Length(Path) < 3) or (Copy(Path, 2, 2) <> ':\') or
    (Pos('"', Path) > 0) then Exit;
  Path := ExpandFileName(Path);
  if FileExists(AddBackslash(Path) + 'program\soffice.exe') then begin
    OfficeDirectoryPage.Values[0] := Path;
    Exit;
  end;
  { Never repurpose a user's populated directory as an MSI installation root. }
  if DirectoryHasEntries(Path) or FileExists(Path) or
    (Length(RemoveBackslashUnlessRoot(Path)) <= 3) then begin
    Candidate := AddBackslash(Path) + 'LibreOffice';
    Number := 2;
    while FileExists(Candidate) or DirectoryHasEntries(Candidate) do begin
      Candidate := AddBackslash(Path) + 'LibreOffice-' + IntToStr(Number);
      Number := Number + 1;
    end;
    Path := Candidate;
  end;
  { The MSI creates this path only after the user confirms the ready page. }
  OfficeDirectoryPage.Values[0] := Path;
end;

function DirectorySafe: Boolean;
var Path: String; Find: TFindRec; HasFiles: Boolean;
begin
  Path := OfficeDirectoryPage.Values[0];
  Result := False;
  if (Length(Path) < 4) or (Copy(Path, 2, 2) <> ':\') or (Pos('"', Path) > 0) then Exit;
  if Within(Path, ExpandConstant('{win}')) or Within(Path, GetEnv('TEMP')) or
     Within(Path, WizardDirValue) or Within(WizardDirValue, Path) then Exit;
  HasFiles := False;
  if FindFirst(AddBackslash(Path) + '*', Find) then begin
    try
      repeat
        if (Find.Name <> '.') and (Find.Name <> '..') then HasFiles := True;
      until not FindNext(Find);
    finally FindClose(Find); end;
  end;
  Result := (not HasFiles) or FileExists(AddBackslash(Path) + 'program\soffice.exe');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var Found: String;
begin
  Result := True;
  if CurPageID = wpSelectDir then begin
    { Recheck settings after the user chooses a custom application directory. }
    Found := DetectedOffice(WizardDirValue);
    if Found <> '' then begin
      ExistingPage.Values[0] := Found;
      OfficePage.SelectedValueIndex := 1;
    end;
  end;
  if CurPageID = ExistingPage.ID then begin
    Found := ResolveOffice(ExistingPage.Values[0]);
    Result := Found <> '';
    if Result then ExistingPage.Values[0] := Found;
    if not Result then MsgBox(CustomMessage('InvalidOffice'), mbError, MB_OK);
  end;
  if CurPageID = OfficeDirectoryPage.ID then begin
    CompleteOfficeDirectory;
    Result := DirectorySafe;
    if not Result then MsgBox(CustomMessage('InvalidDirectory'), mbError, MB_OK);
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
var Dependency: String;
begin
  if OfficePage.SelectedValueIndex = 0 then Dependency :=
    FmtMessage(CustomMessage('OfficeReady'), [OfficeDirectoryPage.Values[0]])
  else if OfficePage.SelectedValueIndex = 1 then Dependency :=
    FmtMessage(CustomMessage('OfficeReady'), [ExistingPage.Values[0]])
  else Dependency := CustomMessage('SkipSummary');
  Result := MemoDirInfo + NewLine + NewLine + Dependency + NewLine + NewLine + MemoTasksInfo;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var Code: Integer; Parameters: String;
begin
  Result := '';
  if OfficePage.SelectedValueIndex = 2 then begin OfficeExecutable := ''; Exit; end;
  if OfficePage.SelectedValueIndex = 1 then begin
    OfficeExecutable := ResolveOffice(ExistingPage.Values[0]);
    if not FileExists(OfficeExecutable) then Result := CustomMessage('InvalidOffice');
    Exit;
  end;
  if OfficeInstalled then begin NeedsRestart := OfficeRestart; Exit; end;
  if not DirectorySafe then begin Result := CustomMessage('InvalidDirectory'); Exit; end;
  DownloadPage.Clear;
  DownloadPage.Add('{#OfficeUrl}', '{#OfficeName}', '{#OfficeHash}');
  DownloadPage.Show;
  try
    try
      DownloadPage.Download;
    except
      Result := GetExceptionMessage;
      Exit;
    end;
  finally DownloadPage.Hide; end;
  WizardForm.StatusLabel.Caption := CustomMessage('InstallingOffice');
  Parameters := '/i "' + ExpandConstant('{tmp}\{#OfficeName}') + '" /passive /norestart ' +
    'INSTALLLOCATION="' + RemoveBackslashUnlessRoot(OfficeDirectoryPage.Values[0]) + '" ' +
    'REGISTER_ALL_MSO_TYPES=0 CREATEDESKTOPLINK=0 RebootYesNo=No';
  if not Exec(ExpandConstant('{sys}\msiexec.exe'), Parameters, '', SW_SHOWNORMAL,
    ewWaitUntilTerminated, Code) then begin
    Result := FmtMessage(CustomMessage('OfficeFailed'), [IntToStr(Code)]); Exit;
  end;
  if (Code <> 0) and (Code <> 3010) then begin
    Result := FmtMessage(CustomMessage('OfficeFailed'), [IntToStr(Code)]); Exit;
  end;
  OfficeExecutable := AddBackslash(OfficeDirectoryPage.Values[0]) + 'program\soffice.exe';
  if not FileExists(OfficeExecutable) then begin Result := CustomMessage('OfficeMissing'); Exit; end;
  OfficeInstalled := True;
  OfficeRestart := Code = 3010;
  NeedsRestart := OfficeRestart;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then begin
    { WritePrivateProfileStringW needs a UTF-16 BOM to retain Chinese paths. }
    if not SaveStringToFile(ExpandConstant('{app}\settings.ini'), #$FF#$FE, False) then
      RaiseException('Unable to create dependency settings. Run setup again.');
    if not SetIniString('Dependencies', 'LibreOffice', OfficeExecutable,
      ExpandConstant('{app}\settings.ini')) then
      RaiseException('Unable to save LibreOffice path. Run setup again.');
  end;
end;
