#ifndef MyAppVersion
  #define MyAppVersion "1.0.6.16"
#endif
#ifndef BuildId
  #define BuildId "LOCALBUILD"
#endif
#ifndef ReleaseDir
  #define ReleaseDir "..\release"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
#ifndef UiTestMode
  #define UiTestMode 0
#endif

#define MyAppName "KeyroIME OpenCore"
#define MyAppPublisher "LocalPro Co., Ltd."
#define MyAppUrl "https://keyro.jp/keyroime_opencore/"

[Setup]
AppId={{C88798DB-7075-4CC1-89F7-AD9C84DABF5E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppUrl}
AppSupportURL={#MyAppUrl}support
AppUpdatesURL={#MyAppUrl}
AppCopyright=Copyright (C) 2024-2026 LocalPro Co., Ltd.
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=KeyroIME OpenCore guided installer
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
DefaultDirName={autopf}\KeyroIME
DefaultGroupName=KeyroIME OpenCore
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\bin\{#BuildId}\keyro_tray.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
#if UiTestMode
PrivilegesRequired=lowest
#else
PrivilegesRequired=admin
#endif
UsedUserAreasWarning=no
CloseApplications=no
RestartApplications=no
DisableWelcomePage=yes
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
DisableFinishedPage=no
AllowNoIcons=yes
WizardStyle=modern
WizardSizePercent=120
WizardResizable=no
ShowLanguageDialog=auto
LanguageDetectionMethod=uilanguage
SetupLogging=yes
OutputDir={#OutputDir}
OutputBaseFilename=KeyroIME_Setup_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
english.CoverTitle=Welcome to KeyroIME OpenCore
english.CoverDescription=Confirm the installation location and license terms.
english.InstallPath=Installation location
english.BrowseButton=Browse...
english.BrowseTitle=Select the KeyroIME installation folder
english.CopyrightLine=Copyright (C) 2024-2026 LocalPro Co., Ltd.
english.LicenseLine=Free and open-source software under the GNU General Public License version 3. Sharing must include the corresponding source and required notices.
english.InstallButton=Install
english.ProgressTitle=Installing KeyroIME OpenCore
english.ProgressDescription=Please wait while the product introduction is displayed.
english.Progress1=Direct number and symbol input
english.Progress2=Dynamic JIS / ANSI keyboard switching
english.Progress3=KeyroIME Pro promotional placeholder
english.Progress4=Community support and contribution
english.ProgressWait=Installation is being prepared. This 12-second presentation cannot be skipped.
english.UiTestComplete=UI presentation test completed. Close Setup to exit without installing.
english.InstallingStatus=Applying system integration settings...
english.FinishButton=Finish
english.FinishUsage=Basic controls: Alt+~ switches input mode. A single Shift press switches punctuation. Select KeyroIME from the Windows input menu to begin.
english.LaunchTray=Launch the KeyroIME status tray now
english.InvalidInstallPath=Select an absolute path on a local drive.
english.ServiceRemovalError=The previous KeyroIME service could not be removed. Restart Windows and run Setup again.
english.DataDirectoryError=The KeyroIME data directory could not be created.
english.AclError=Failed to apply secure file permissions.
english.TsfRegistrationError=Failed to register the KeyroIME TSF text service.
english.TsfValidationError=The KeyroIME TSF registration could not be verified.
english.ServiceRegistrationError=Failed to register the KeyroIME backend service.
english.ServicePathValidationError=The KeyroIME service executable path was not stored securely.
english.ServiceConfigurationError=Failed to configure the KeyroIME backend service.
english.ServiceStartupError=Failed to start the KeyroIME backend service.
english.PipeStartupError=The KeyroIME backend service did not become ready.

japanese.CoverTitle=KeyroIME OpenCore セットアップ
japanese.CoverDescription=インストール先とライセンス条件を確認してください。
japanese.InstallPath=インストール先
japanese.BrowseButton=参照...
japanese.BrowseTitle=KeyroIME のインストール先を選択してください
japanese.CopyrightLine=Copyright (C) 2024-2026 株式会社LocalPro
japanese.LicenseLine=GNU GPLv3 に基づく自由なオープンソースソフトウェアです。共有時は、対応ソースと必要な表示を含めてください。
japanese.InstallButton=インストール
japanese.ProgressTitle=KeyroIME OpenCore をインストールしています
japanese.ProgressDescription=製品紹介を表示しています。しばらくお待ちください。
japanese.Progress1=数字・記号をそのまま入力
japanese.Progress2=JIS / ANSI キーボードを動的に切り替え
japanese.Progress3=KeyroIME Pro 紹介枠（プレースホルダー）
japanese.Progress4=コミュニティとコントリビューション
japanese.ProgressWait=インストールを準備しています。この12秒間の表示はスキップできません。
japanese.UiTestComplete=画面表示テストが完了しました。インストールせずに終了するには、セットアップを閉じてください。
japanese.InstallingStatus=システム統合設定を適用しています...
japanese.FinishButton=完了
japanese.FinishUsage=基本操作: Alt+~ で入力状態を切り替えます。Shift 単押しで句読点を切り替えます。Windows の入力メニューから KeyroIME を選択してください。
japanese.LaunchTray=KeyroIME 状態トレイを今すぐ起動する
japanese.InvalidInstallPath=ローカルドライブ上の絶対パスを選択してください。
japanese.ServiceRemovalError=以前の KeyroIME サービスを削除できませんでした。Windows を再起動してからセットアップを再実行してください。
japanese.DataDirectoryError=KeyroIME データフォルダーを作成できませんでした。
japanese.AclError=安全なファイルアクセス権を設定できませんでした。
japanese.TsfRegistrationError=KeyroIME TSF テキストサービスを登録できませんでした。
japanese.TsfValidationError=KeyroIME TSF 登録を確認できませんでした。
japanese.ServiceRegistrationError=KeyroIME バックエンドサービスを登録できませんでした。
japanese.ServicePathValidationError=KeyroIME サービスの実行ファイルパスを安全に登録できませんでした。
japanese.ServiceConfigurationError=KeyroIME バックエンドサービスを設定できませんでした。
japanese.ServiceStartupError=KeyroIME バックエンドサービスを開始できませんでした。
japanese.PipeStartupError=KeyroIME バックエンドサービスの準備が完了しませんでした。

[Files]
Source: "{#ReleaseDir}\KeyroIME.dll"; DestDir: "{app}\bin\{#BuildId}"; Flags: ignoreversion uninsrestartdelete
Source: "{#ReleaseDir}\keyro_service.exe"; DestDir: "{app}\bin\{#BuildId}"; Flags: ignoreversion uninsrestartdelete
Source: "{#ReleaseDir}\keyro_tray.exe"; DestDir: "{app}\bin\{#BuildId}"; Flags: ignoreversion uninsrestartdelete
Source: "{#ReleaseDir}\LICENSE_ja.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#ReleaseDir}\LICENSE_en.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#ReleaseDir}\THIRD_PARTY_NOTICES.md"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#ReleaseDir}\dictionary_manifest.json"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "assets\cover.bmp"; Flags: dontcopy noencryption
Source: "assets\progress-1.bmp"; Flags: dontcopy noencryption
Source: "assets\progress-2.bmp"; Flags: dontcopy noencryption
Source: "assets\progress-3.bmp"; Flags: dontcopy noencryption
Source: "assets\progress-4.bmp"; Flags: dontcopy noencryption
Source: "assets\finish.bmp"; Flags: dontcopy noencryption

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "KeyroIME_Tray"; ValueData: """{app}\bin\{#BuildId}\keyro_tray.exe"""; Flags: uninsdeletevalue

[Run]
Filename: "{app}\bin\{#BuildId}\keyro_tray.exe"; Description: "{cm:LaunchTray}"; Flags: nowait postinstall skipifsilent runasoriginaluser

[UninstallDelete]
Type: filesandordirs; Name: "{app}\bin"
Type: filesandordirs; Name: "{app}\licenses"

[Code]
const
  ServiceName = 'KeyroIME_Service';
  PipeName = '\\.\pipe\KeyroIME.Service.v1';
  TipRegistryPath = 'SOFTWARE\Classes\CLSID\{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}\InprocServer32';
  ServiceRegistryPath = 'SYSTEM\CurrentControlSet\Services\KeyroIME_Service';
  CarouselStageDurationMs = 3000;
  CarouselMinimumDurationMs = 12000;
  PipeReadyAttempts = 100;
  PipeReadyDelayMs = 100;

var
  CoverPage: TWizardPage;
  CarouselPage: TWizardPage;
  CoverImage: TBitmapImage;
  CarouselImage: TBitmapImage;
  InstallingImage: TBitmapImage;
  FinishImage: TBitmapImage;
  InstallPathLabel: TNewStaticText;
  InstallPathEdit: TNewEdit;
  BrowseButton: TNewButton;
  LicenseLabel: TNewStaticText;
  CarouselStatus: TNewStaticText;
  CarouselProgress: TNewProgressBar;
  InstallingStatus: TNewStaticText;
  InstallingProgress: TNewProgressBar;
  FinishUsageLabel: TNewStaticText;
  CarouselTimerId: Longword;
  CarouselStartedAt: Cardinal;
  CarouselCurrentStage: Integer;
  CarouselStarted: Boolean;
  CarouselActive: Boolean;
  CarouselComplete: Boolean;
  ProgressImagePaths: array[0..3] of String;

function GetTickCount: Cardinal;
  external 'GetTickCount@kernel32.dll stdcall';
function WaitNamedPipe(const lpNamedPipeName: String; nTimeOut: Cardinal): Boolean;
  external 'WaitNamedPipeW@kernel32.dll stdcall';
function SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc: Longword): Longword;
  external 'SetTimer@user32.dll stdcall';
function KillTimer(hWnd, nIDEvent: Longword): Boolean;
  external 'KillTimer@user32.dll stdcall';

function ProductDirectory: String;
begin
  Result := ExpandConstant('{app}\bin\{#BuildId}');
end;

function ProductDllPath: String;
begin
  Result := ProductDirectory + '\KeyroIME.dll';
end;

function ProductServicePath: String;
begin
  Result := ProductDirectory + '\keyro_service.exe';
end;

function SystemTool(const Name: String): String;
begin
  Result := ExpandConstant('{sys}\' + Name);
end;

function IsAbsoluteLocalPath(const Path: String): Boolean;
begin
  Result :=
    (Length(Path) >= 3) and
    (Path[2] = ':') and
    (Path[3] = '\') and
    (Pos('\\', Path) <> 1);
end;

procedure BrowseButtonClick(Sender: TObject);
var
  SelectedDirectory: String;
begin
  SelectedDirectory := InstallPathEdit.Text;
  if BrowseForFolder(CustomMessage('BrowseTitle'), SelectedDirectory, True) then
    InstallPathEdit.Text := SelectedDirectory;
end;

procedure LoadCarouselStage(const Stage: Integer);
begin
  if CarouselCurrentStage = Stage then
    Exit;
  CarouselImage.Bitmap.LoadFromFile(ProgressImagePaths[Stage]);
  CarouselCurrentStage := Stage;
end;

procedure AdvanceAfterCarousel;
begin
  WizardForm.NextButton.OnClick(WizardForm.NextButton);
end;

procedure UpdateCarousel(Arg1, Arg2, Arg3, Arg4: Longword);
var
  Elapsed: Cardinal;
  Stage: Integer;
  Position: Integer;
begin
  if not CarouselActive then
    Exit;

  Elapsed := GetTickCount - CarouselStartedAt;
  if Elapsed >= CarouselMinimumDurationMs then
    Stage := 3
  else
    Stage := (Elapsed div CarouselStageDurationMs) mod 4;
  LoadCarouselStage(Stage);

  if Elapsed >= CarouselMinimumDurationMs then
    Position := 1000
  else
    Position := (Elapsed * 1000) div CarouselMinimumDurationMs;
  CarouselProgress.Position := Position;

  if Elapsed >= CarouselMinimumDurationMs then
  begin
    if CarouselTimerId <> 0 then
    begin
      KillTimer(0, CarouselTimerId);
      CarouselTimerId := 0;
    end;
    CarouselActive := False;
    CarouselComplete := True;
#if UiTestMode
    CarouselStatus.Caption := CustomMessage('UiTestComplete');
    WizardForm.NextButton.Enabled := False;
    WizardForm.CancelButton.Enabled := True;
#else
    WizardForm.NextButton.Enabled := True;
    AdvanceAfterCarousel;
#endif
  end;
end;

procedure StartCarousel;
begin
  CarouselStarted := True;
  CarouselActive := True;
  CarouselComplete := False;
  CarouselCurrentStage := -1;
  CarouselProgress.Position := 0;
  CarouselStartedAt := GetTickCount;
  LoadCarouselStage(0);
  CarouselTimerId := SetTimer(0, 0, 100, CreateCallback(@UpdateCarousel));
  if CarouselTimerId = 0 then
    RaiseException('Failed to start the installer presentation timer.');
end;

procedure InitializeWizard;
begin
  ExtractTemporaryFile('cover.bmp');
  ExtractTemporaryFile('progress-1.bmp');
  ExtractTemporaryFile('progress-2.bmp');
  ExtractTemporaryFile('progress-3.bmp');
  ExtractTemporaryFile('progress-4.bmp');
  ExtractTemporaryFile('finish.bmp');

  ProgressImagePaths[0] := ExpandConstant('{tmp}\progress-1.bmp');
  ProgressImagePaths[1] := ExpandConstant('{tmp}\progress-2.bmp');
  ProgressImagePaths[2] := ExpandConstant('{tmp}\progress-3.bmp');
  ProgressImagePaths[3] := ExpandConstant('{tmp}\progress-4.bmp');

  CoverPage := CreateCustomPage(
    wpWelcome, CustomMessage('CoverTitle'), CustomMessage('CoverDescription'));

  CoverImage := TBitmapImage.Create(WizardForm);
  CoverImage.Parent := CoverPage.Surface;
  CoverImage.Left := 0;
  CoverImage.Top := 0;
  CoverImage.Width := CoverPage.SurfaceWidth;
  CoverImage.Height := ScaleY(176);
  CoverImage.Stretch := True;
  CoverImage.Bitmap.LoadFromFile(ExpandConstant('{tmp}\cover.bmp'));

  InstallPathLabel := TNewStaticText.Create(WizardForm);
  InstallPathLabel.Parent := CoverPage.Surface;
  InstallPathLabel.Left := 0;
  InstallPathLabel.Top := ScaleY(188);
  InstallPathLabel.Caption := CustomMessage('InstallPath');
  InstallPathLabel.AutoSize := True;

  InstallPathEdit := TNewEdit.Create(WizardForm);
  InstallPathEdit.Parent := CoverPage.Surface;
  InstallPathEdit.Left := 0;
  InstallPathEdit.Top := ScaleY(207);
  InstallPathEdit.Width := CoverPage.SurfaceWidth - ScaleX(105);
  InstallPathEdit.Text := WizardForm.DirEdit.Text;

  BrowseButton := TNewButton.Create(WizardForm);
  BrowseButton.Parent := CoverPage.Surface;
  BrowseButton.Left := InstallPathEdit.Left + InstallPathEdit.Width + ScaleX(8);
  BrowseButton.Top := InstallPathEdit.Top - ScaleY(1);
  BrowseButton.Width := ScaleX(97);
  BrowseButton.Height := InstallPathEdit.Height + ScaleY(2);
  BrowseButton.Caption := CustomMessage('BrowseButton');
  BrowseButton.OnClick := @BrowseButtonClick;

  LicenseLabel := TNewStaticText.Create(WizardForm);
  LicenseLabel.Parent := CoverPage.Surface;
  LicenseLabel.Left := 0;
  LicenseLabel.Top := ScaleY(242);
  LicenseLabel.Width := CoverPage.SurfaceWidth;
  LicenseLabel.Height := ScaleY(58);
  LicenseLabel.AutoSize := False;
  LicenseLabel.WordWrap := True;
  LicenseLabel.Caption :=
    CustomMessage('CopyrightLine') + #13#10 + CustomMessage('LicenseLine');

  CarouselPage := CreateCustomPage(
    CoverPage.ID, CustomMessage('ProgressTitle'), CustomMessage('ProgressDescription'));

  CarouselImage := TBitmapImage.Create(WizardForm);
  CarouselImage.Parent := CarouselPage.Surface;
  CarouselImage.Left := 0;
  CarouselImage.Top := 0;
  CarouselImage.Width := CarouselPage.SurfaceWidth;
  CarouselImage.Height := ScaleY(214);
  CarouselImage.Stretch := True;

  CarouselStatus := TNewStaticText.Create(WizardForm);
  CarouselStatus.Parent := CarouselPage.Surface;
  CarouselStatus.Left := 0;
  CarouselStatus.Top := ScaleY(232);
  CarouselStatus.Width := CarouselPage.SurfaceWidth;
  CarouselStatus.Height := ScaleY(34);
  CarouselStatus.AutoSize := False;
  CarouselStatus.WordWrap := True;
  CarouselStatus.Caption := CustomMessage('ProgressWait');

  CarouselProgress := TNewProgressBar.Create(WizardForm);
  CarouselProgress.Parent := CarouselPage.Surface;
  CarouselProgress.Left := 0;
  CarouselProgress.Top := ScaleY(276);
  CarouselProgress.Width := CarouselPage.SurfaceWidth;
  CarouselProgress.Height := ScaleY(18);
  CarouselProgress.Min := 0;
  CarouselProgress.Max := 1000;

  InstallingImage := TBitmapImage.Create(WizardForm);
  InstallingImage.Parent := WizardForm.InstallingPage;
  InstallingImage.Left := 0;
  InstallingImage.Top := 0;
  InstallingImage.Width := WizardForm.InstallingPage.ClientWidth;
  InstallingImage.Height := ScaleY(232);
  InstallingImage.Stretch := True;
  InstallingImage.Bitmap.LoadFromFile(ProgressImagePaths[3]);
  InstallingImage.Visible := False;

  InstallingStatus := TNewStaticText.Create(WizardForm);
  InstallingStatus.Parent := WizardForm.InstallingPage;
  InstallingStatus.Left := 0;
  InstallingStatus.Top := ScaleY(244);
  InstallingStatus.Width := WizardForm.InstallingPage.ClientWidth;
  InstallingStatus.Caption := CustomMessage('InstallingStatus');
  InstallingStatus.Visible := False;

  InstallingProgress := TNewProgressBar.Create(WizardForm);
  InstallingProgress.Parent := WizardForm.InstallingPage;
  InstallingProgress.Left := 0;
  InstallingProgress.Top := ScaleY(277);
  InstallingProgress.Width := WizardForm.InstallingPage.ClientWidth;
  InstallingProgress.Height := ScaleY(18);
  InstallingProgress.Min := 0;
  InstallingProgress.Max := 1000;
  InstallingProgress.Position := 1000;
  InstallingProgress.Visible := False;

  FinishImage := TBitmapImage.Create(WizardForm);
  FinishImage.Parent := WizardForm.FinishedPage;
  FinishImage.Left := 0;
  FinishImage.Top := 0;
  FinishImage.Width := WizardForm.FinishedPage.ClientWidth;
  FinishImage.Height := ScaleY(178);
  FinishImage.Stretch := True;
  FinishImage.Bitmap.LoadFromFile(ExpandConstant('{tmp}\finish.bmp'));
  FinishImage.Visible := False;

  FinishUsageLabel := TNewStaticText.Create(WizardForm);
  FinishUsageLabel.Parent := WizardForm.FinishedPage;
  FinishUsageLabel.Left := 0;
  FinishUsageLabel.Top := ScaleY(188);
  FinishUsageLabel.Width := WizardForm.FinishedPage.ClientWidth;
  FinishUsageLabel.Height := ScaleY(50);
  FinishUsageLabel.AutoSize := False;
  FinishUsageLabel.WordWrap := True;
  FinishUsageLabel.Caption := CustomMessage('FinishUsage');
  FinishUsageLabel.Visible := False;

  CarouselStarted := False;
  CarouselActive := False;
  CarouselComplete := False;
  CarouselCurrentStage := -1;
  CarouselTimerId := 0;
end;

procedure DeinitializeSetup;
begin
  if CarouselTimerId <> 0 then
    KillTimer(0, CarouselTimerId);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = CoverPage.ID then
  begin
    InstallPathEdit.Text := Trim(InstallPathEdit.Text);
    if not IsAbsoluteLocalPath(InstallPathEdit.Text) then
    begin
      MsgBox(CustomMessage('InvalidInstallPath'), mbError, MB_OK);
      Result := False;
      Exit;
    end;
    WizardForm.DirEdit.Text := InstallPathEdit.Text;
  end
  else if (CurPageID = CarouselPage.ID) and (not CarouselComplete) then
    Result := False;
end;

function BackButtonClick(CurPageID: Integer): Boolean;
begin
  Result := CurPageID <> CarouselPage.ID;
end;

procedure CancelButtonClick(
  CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if CarouselActive or (CurPageID = wpInstalling) then
  begin
    Cancel := False;
    Confirm := False;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := WizardSilent and
    ((PageID = CoverPage.ID) or (PageID = CarouselPage.ID));
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = CoverPage.ID then
  begin
    WizardForm.NextButton.Caption := CustomMessage('InstallButton');
    WizardForm.NextButton.Enabled := True;
    WizardForm.BackButton.Visible := False;
    WizardForm.CancelButton.Enabled := True;
  end
  else if CurPageID = CarouselPage.ID then
  begin
    WizardForm.NextButton.Enabled := False;
    WizardForm.BackButton.Visible := True;
    WizardForm.BackButton.Enabled := False;
    WizardForm.CancelButton.Enabled := False;
    if not CarouselStarted then
      StartCarousel;
  end
  else if CurPageID = wpInstalling then
  begin
    WizardForm.StatusLabel.Visible := False;
    WizardForm.FilenameLabel.Visible := False;
    WizardForm.ProgressGauge.Visible := False;
    WizardForm.NextButton.Enabled := False;
    WizardForm.BackButton.Enabled := False;
    WizardForm.CancelButton.Enabled := False;
    InstallingImage.Visible := True;
    InstallingStatus.Visible := True;
    InstallingProgress.Visible := True;
  end
  else if CurPageID = wpFinished then
  begin
    WizardForm.NextButton.Caption := CustomMessage('FinishButton');
    WizardForm.NextButton.Enabled := True;
    WizardForm.BackButton.Visible := False;
    WizardForm.CancelButton.Enabled := True;
    WizardForm.FinishedHeadingLabel.Visible := False;
    WizardForm.FinishedLabel.Visible := False;
    FinishImage.Visible := True;
    FinishUsageLabel.Visible := True;
    WizardForm.RunList.Top := ScaleY(248);
    WizardForm.RunList.Height := ScaleY(45);
  end;
end;

function QueryServiceExists: Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec(
    SystemTool('sc.exe'), 'query "' + ServiceName + '"', '',
    SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

function StopAndDeleteExistingService: Boolean;
var
  I: Integer;
  ResultCode: Integer;
begin
  Result := True;
  if not QueryServiceExists then
    Exit;

  Exec(
    SystemTool('sc.exe'), 'stop "' + ServiceName + '"', '',
    SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(1000);
  Exec(
    SystemTool('sc.exe'), 'delete "' + ServiceName + '"', '',
    SW_HIDE, ewWaitUntilTerminated, ResultCode);

  for I := 1 to 50 do
  begin
    if not QueryServiceExists then
      Exit;
    Sleep(100);
  end;
  Result := False;
end;

function UnregisterCurrentTip: Boolean;
var
  CurrentDll: String;
  ResultCode: Integer;
begin
  Result := True;
  if RegQueryStringValue(HKLM64, TipRegistryPath, '', CurrentDll) and
     FileExists(CurrentDll) then
  begin
    Result := Exec(
      SystemTool('regsvr32.exe'), '/u /s "' + CurrentDll + '"', '',
      SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  NeedsRestart := False;
  Exec(
    SystemTool('taskkill.exe'), '/im keyro_tray.exe /f', '',
    SW_HIDE, ewWaitUntilTerminated, ResultCode);
  if not StopAndDeleteExistingService then
  begin
    Result := CustomMessage('ServiceRemovalError');
    Exit;
  end;
  if not UnregisterCurrentTip then
    Result := CustomMessage('TsfRegistrationError');
end;

procedure RunChecked(
  const FileName, Parameters, ErrorMessage: String);
var
  ResultCode: Integer;
begin
  if (not Exec(
        FileName, Parameters, '', SW_HIDE,
        ewWaitUntilTerminated, ResultCode)) or (ResultCode <> 0) then
    RaiseException(ErrorMessage + ' (' + IntToStr(ResultCode) + ')');
end;

function WaitForServicePipe: Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to PipeReadyAttempts do
  begin
    if WaitNamedPipe(PipeName, PipeReadyDelayMs) then
    begin
      Result := True;
      Exit;
    end;
    Sleep(PipeReadyDelayMs);
  end;
end;

procedure InstallSystemComponents;
var
  RegisteredDll: String;
  RegisteredServicePath: String;
  AppDirectory: String;
  DataDirectory: String;
begin
  AppDirectory := ExpandConstant('{app}');
  DataDirectory := ExpandConstant('{commonappdata}\KeyroIME');
  if (not DirExists(DataDirectory)) and (not ForceDirectories(DataDirectory)) then
    RaiseException(CustomMessage('DataDirectoryError'));

  RunChecked(
    SystemTool('icacls.exe'),
    '"' + AppDirectory + '" /inheritance:r /grant:r ' +
    '"*S-1-5-18:(OI)(CI)F" "*S-1-5-32-544:(OI)(CI)F" ' +
    '"*S-1-5-32-545:(OI)(CI)RX" "*S-1-5-19:(OI)(CI)RX" ' +
    '"*S-1-15-2-1:(OI)(CI)RX" "*S-1-15-2-2:(OI)(CI)RX"',
    CustomMessage('AclError'));
  RunChecked(
    SystemTool('icacls.exe'),
    '"' + DataDirectory + '" /inheritance:r /grant:r ' +
    '"*S-1-5-19:(OI)(CI)M" "*S-1-5-18:(OI)(CI)F" ' +
    '"*S-1-5-32-544:(OI)(CI)F"',
    CustomMessage('AclError'));

  RunChecked(
    SystemTool('regsvr32.exe'), '/s "' + ProductDllPath + '"',
    CustomMessage('TsfRegistrationError'));
  if (not RegQueryStringValue(
        HKLM64, TipRegistryPath, '', RegisteredDll)) or
     (RegisteredDll <> ProductDllPath) then
    RaiseException(CustomMessage('TsfValidationError'));

  RunChecked(
    SystemTool('sc.exe'),
    'create "' + ServiceName + '" binPath= "\"' + ProductServicePath + '\"" ' +
    'start= auto type= own obj= "NT AUTHORITY\LocalService" ' +
    'DisplayName= "KeyroIME Service"',
    CustomMessage('ServiceRegistrationError'));
  if (not RegQueryStringValue(
        HKLM64, ServiceRegistryPath, 'ImagePath', RegisteredServicePath)) or
     (RegisteredServicePath <> '"' + ProductServicePath + '"') then
    RaiseException(CustomMessage('ServicePathValidationError'));
  RunChecked(
    SystemTool('sc.exe'),
    'description "' + ServiceName + '" "KeyroIME Japanese input backend service"',
    CustomMessage('ServiceConfigurationError'));
  RunChecked(
    SystemTool('sc.exe'),
    'failure "' + ServiceName + '" reset= 60 ' +
    'actions= restart/5000/restart/5000/""/0',
    CustomMessage('ServiceConfigurationError'));
  RunChecked(
    SystemTool('sc.exe'), 'start "' + ServiceName + '"',
    CustomMessage('ServiceStartupError'));

  if not WaitForServicePipe then
    RaiseException(CustomMessage('PipeStartupError'));
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    InstallSystemComponents;
end;

function HasCommandLineParameter(const Name: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
  begin
    if Uppercase(ParamStr(I)) = Uppercase(Name) then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    Exec(
      SystemTool('taskkill.exe'), '/im keyro_tray.exe /f', '',
      SW_HIDE, ewWaitUntilTerminated, ResultCode);
    StopAndDeleteExistingService;
    UnregisterCurrentTip;
  end
  else if (CurUninstallStep = usPostUninstall) and
          HasCommandLineParameter('/PURGEDATA') then
    DelTree(ExpandConstant('{commonappdata}\KeyroIME'), True, True, True);
end;
