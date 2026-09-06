@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PRODUCT_NAME=KeyroIME"
set "SERVICE_NAME=KeyroIME_Service"
set "SERVICE_DISPLAY=KeyroIME Service"
set "PIPE_NAME=\\.\pipe\KeyroIME.Service.v1"
set "INSTALL_DIR=%ProgramFiles%\KeyroIME"
set "DATA_DIR=%ProgramData%\KeyroIME"
set "ROOT_DIR=%~dp0"
set "PRODUCT_VERSION=1.0.6.16"
if exist "%ROOT_DIR%VERSION" set /p PRODUCT_VERSION=<"%ROOT_DIR%VERSION"
set "SILENT_MODE=0"
set "VALIDATE_ONLY=0"
if /i "%~1"=="/silent" set "SILENT_MODE=1"
if /i "%~1"=="/validate" set "VALIDATE_ONLY=1"

set "TSF_DLL_SRC=%ROOT_DIR%KeyroIME.dll"
set "TRAY_EXE_SRC=%ROOT_DIR%keyro_tray.exe"
set "SERVICE_EXE_SRC=%ROOT_DIR%keyro_service.exe"
set "NOTICE_SRC=%ROOT_DIR%THIRD_PARTY_NOTICES.md"
set "LICENSE_JA_SRC=%ROOT_DIR%LICENSE_ja.txt"
set "LICENSE_EN_SRC=%ROOT_DIR%LICENSE_en.txt"
set "DICTIONARY_MANIFEST_SRC=%ROOT_DIR%dictionary_manifest.json"
set "SYSTEM_TOOL_DIR=%SystemRoot%\System32"
if exist "%SystemRoot%\Sysnative\regsvr32.exe" set "SYSTEM_TOOL_DIR=%SystemRoot%\Sysnative"

set "REGSVR32_EXE=%SYSTEM_TOOL_DIR%\regsvr32.exe"
set "SC_EXE=%SYSTEM_TOOL_DIR%\sc.exe"
set "ICACLS_EXE=%SYSTEM_TOOL_DIR%\icacls.exe"
set "REG_EXE=%SYSTEM_TOOL_DIR%\reg.exe"
set "SCHTASKS_EXE=%SYSTEM_TOOL_DIR%\schtasks.exe"
set "TASKLIST_EXE=%SYSTEM_TOOL_DIR%\tasklist.exe"

if not exist "%TSF_DLL_SRC%" set "TSF_DLL_SRC=%ROOT_DIR%src\tsf_shell\build\Release\KeyroIME.dll"
if not exist "%TRAY_EXE_SRC%" set "TRAY_EXE_SRC=%ROOT_DIR%src\tsf_shell\build\Release\keyro_tray.exe"
if not exist "%SERVICE_EXE_SRC%" set "SERVICE_EXE_SRC=%ROOT_DIR%target\keyro_service_runtime\x86_64-pc-windows-msvc\release\keyro_service.exe"
if not exist "%SERVICE_EXE_SRC%" set "SERVICE_EXE_SRC=%ROOT_DIR%target\keyro_service_runtime\release\keyro_service.exe"
if not exist "%SERVICE_EXE_SRC%" set "SERVICE_EXE_SRC=%ROOT_DIR%src\keyro_service\target\x86_64-pc-windows-msvc\release\keyro_service.exe"
if not exist "%SERVICE_EXE_SRC%" set "SERVICE_EXE_SRC=%ROOT_DIR%src\keyro_service\target\release\keyro_service.exe"

echo ========================================
echo KeyroIME v%PRODUCT_VERSION% installer
echo ========================================

echo Validating release files...
if not exist "%TSF_DLL_SRC%" goto MISSING_TSF
if not exist "%TRAY_EXE_SRC%" goto MISSING_TRAY
if not exist "%SERVICE_EXE_SRC%" goto MISSING_SERVICE

if exist "%ROOT_DIR%SHA256SUMS.txt" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$root=$env:ROOT_DIR; $ok=$true; foreach($line in Get-Content -LiteralPath (Join-Path $root 'SHA256SUMS.txt')) { if($line -notmatch '^([0-9a-fA-F]{64}) \*(.+)$'){$ok=$false; break}; $path=Join-Path $root $Matches[2]; if(-not (Test-Path -LiteralPath $path) -or (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash -ne $Matches[1]){$ok=$false; break} }; if($ok){exit 0}else{exit 1}" >nul 2>&1
    if errorlevel 1 (
        echo Release checksum validation failed.
        call :WAIT_END
        exit /b 1
    )
)

call :CHECK_X64_BINARY "%TSF_DLL_SRC%" "KeyroIME.dll"
if errorlevel 1 goto INVALID_BINARY
call :CHECK_X64_BINARY "%TRAY_EXE_SRC%" "keyro_tray.exe"
if errorlevel 1 goto INVALID_BINARY
call :CHECK_X64_BINARY "%SERVICE_EXE_SRC%" "keyro_service.exe"
if errorlevel 1 goto INVALID_BINARY

if "%VALIDATE_ONLY%"=="1" (
    echo Release file validation passed.
    exit /b 0
)

net session >nul 2>&1
if errorlevel 1 (
    echo Administrator privileges are required.
    call :WAIT_END
    exit /b 1
)

for /f "usebackq delims=" %%H in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$s=(Get-FileHash -Algorithm SHA256 -LiteralPath $env:TSF_DLL_SRC).Hash+(Get-FileHash -Algorithm SHA256 -LiteralPath $env:TRAY_EXE_SRC).Hash+(Get-FileHash -Algorithm SHA256 -LiteralPath $env:SERVICE_EXE_SRC).Hash; $sha=[Security.Cryptography.SHA256]::Create(); try { $bytes=[Text.Encoding]::ASCII.GetBytes($s); $hash=$sha.ComputeHash($bytes); ([BitConverter]::ToString($hash)).Replace('-','').Substring(0,12) } finally { $sha.Dispose() }"`) do set "BUILD_ID=%%H"
if not defined BUILD_ID (
    echo Failed to calculate the release build identifier.
    call :WAIT_END
    exit /b 1
)

set "DEPLOY_DIR=%INSTALL_DIR%\bin\%BUILD_ID%"
set "TSF_DLL_DEST=%DEPLOY_DIR%\KeyroIME.dll"
set "TRAY_EXE_DEST=%DEPLOY_DIR%\keyro_tray.exe"
set "SERVICE_EXE_DEST=%DEPLOY_DIR%\keyro_service.exe"
set "NOTICE_DEST=%DEPLOY_DIR%\THIRD_PARTY_NOTICES.md"
set "LICENSE_JA_DEST=%DEPLOY_DIR%\LICENSE_ja.txt"
set "LICENSE_EN_DEST=%DEPLOY_DIR%\LICENSE_en.txt"
set "DICTIONARY_MANIFEST_DEST=%DEPLOY_DIR%\dictionary_manifest.json"

taskkill /im keyro_tray.exe /f >nul 2>&1
timeout /t 1 /nobreak >nul 2>&1

echo Stopping the previous service...
"%SC_EXE%" query "%SERVICE_NAME%" >nul 2>&1
if not errorlevel 1 (
    "%SC_EXE%" stop "%SERVICE_NAME%" >nul 2>&1
    timeout /t 2 /nobreak >nul 2>&1
    "%SC_EXE%" delete "%SERVICE_NAME%" >nul 2>&1
    call :WAIT_SERVICE_GONE
)

if /i not "%INSTALL_DIR%"=="%ProgramFiles%\KeyroIME" (
    echo Installation directory safety check failed.
    call :WAIT_END
    exit /b 1
)

if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%" >nul 2>&1
if not exist "%DEPLOY_DIR%" (
    echo Failed to create the deployment directory.
    call :WAIT_END
    exit /b 1
)

echo Deploying KeyroIME build %BUILD_ID%...
call :COPY_VERIFIED "%TSF_DLL_SRC%" "%TSF_DLL_DEST%"
if errorlevel 1 goto COPY_TSF_FAILED
call :COPY_VERIFIED "%SERVICE_EXE_SRC%" "%SERVICE_EXE_DEST%"
if errorlevel 1 goto COPY_SERVICE_FAILED
call :COPY_VERIFIED "%TRAY_EXE_SRC%" "%TRAY_EXE_DEST%"
if errorlevel 1 goto COPY_TRAY_FAILED

if exist "%NOTICE_SRC%" copy /y "%NOTICE_SRC%" "%NOTICE_DEST%" >nul 2>&1
if exist "%LICENSE_JA_SRC%" copy /y "%LICENSE_JA_SRC%" "%LICENSE_JA_DEST%" >nul 2>&1
if exist "%LICENSE_EN_SRC%" copy /y "%LICENSE_EN_SRC%" "%LICENSE_EN_DEST%" >nul 2>&1
if exist "%DICTIONARY_MANIFEST_SRC%" copy /y "%DICTIONARY_MANIFEST_SRC%" "%DICTIONARY_MANIFEST_DEST%" >nul 2>&1

"%ICACLS_EXE%" "%DEPLOY_DIR%" /inheritance:e /grant *S-1-15-2-1:RX /grant *S-1-15-2-2:RX /grant *S-1-5-32-545:RX >nul 2>&1
if errorlevel 1 (
    echo Failed to set deployment directory permissions.
    call :WAIT_END
    exit /b 1
)

echo Registering the tray startup entry...
"%REG_EXE%" add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "KeyroIME_Tray" /t REG_SZ /d "\"%TRAY_EXE_DEST%\"" /f >nul 2>&1
if errorlevel 1 (
    echo Failed to register the tray startup entry.
    call :WAIT_END
    exit /b 1
)

echo Registering the TSF text service...
"%REGSVR32_EXE%" /s "%TSF_DLL_DEST%"
if errorlevel 1 (
    echo TSF registration failed.
    call :WAIT_END
    exit /b 1
)

"%REG_EXE%" query "HKLM\SOFTWARE\Classes\CLSID\{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}\InprocServer32" /v ThreadingModel 2>nul | findstr /i "Apartment" >nul
if errorlevel 1 (
    echo COM registration validation failed.
    call :WAIT_END
    exit /b 1
)

echo Registering and starting the backend service...
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%" >nul 2>&1
if not exist "%DATA_DIR%" (
    echo Failed to create the service data directory.
    call :WAIT_END
    exit /b 1
)
"%ICACLS_EXE%" "%DATA_DIR%" /inheritance:e /grant:r "*S-1-5-19:^(OI^)^(CI^)M" "*S-1-5-18:^(OI^)^(CI^)F" "*S-1-5-32-544:^(OI^)^(CI^)F" >nul 2>&1
if errorlevel 1 (
    echo Failed to secure the service data directory.
    call :WAIT_END
    exit /b 1
)

"%SC_EXE%" create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE_DEST%\"" start= auto type= own obj= "NT AUTHORITY\LocalService" DisplayName= "%SERVICE_DISPLAY%" >nul 2>&1
if errorlevel 1 (
    echo Backend service registration failed.
    call :WAIT_END
    exit /b 1
)
"%SC_EXE%" qc "%SERVICE_NAME%" 2>nul | findstr /i "LocalService" >nul
if errorlevel 1 (
    echo Backend service account validation failed.
    call :WAIT_END
    exit /b 1
)
"%SC_EXE%" description "%SERVICE_NAME%" "KeyroIME Japanese input backend service" >nul 2>&1
"%SC_EXE%" failure "%SERVICE_NAME%" reset= 60 actions= restart/5000/restart/5000/""/0 >nul 2>&1
"%SC_EXE%" start "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Backend service startup failed.
    call :WAIT_END
    exit /b 1
)

call :WAIT_PIPE
if errorlevel 1 (
    echo Backend service pipe did not become ready.
    call :WAIT_END
    exit /b 1
)

call :START_TRAY_UNELEVATED
if errorlevel 1 (
    echo Failed to start the tray process with limited user privileges.
    call :WAIT_END
    exit /b 1
)

echo ========================================
echo Installation completed: "%DEPLOY_DIR%"
echo ========================================
call :WAIT_END
exit /b 0

:MISSING_TSF
echo KeyroIME.dll was not found.
goto INSTALL_FAILED
:MISSING_TRAY
echo keyro_tray.exe was not found.
goto INSTALL_FAILED
:MISSING_SERVICE
echo keyro_service.exe was not found.
goto INSTALL_FAILED
:INVALID_BINARY
echo All release binaries must be x64 PE files.
goto INSTALL_FAILED
:COPY_TSF_FAILED
echo Failed to deploy KeyroIME.dll.
goto INSTALL_FAILED
:COPY_SERVICE_FAILED
echo Failed to deploy keyro_service.exe.
goto INSTALL_FAILED
:COPY_TRAY_FAILED
echo Failed to deploy keyro_tray.exe.
goto INSTALL_FAILED
:INSTALL_FAILED
call :WAIT_END
exit /b 1

:COPY_VERIFIED
if not exist "%~2" goto COPY_VERIFIED_COPY
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "if ((Get-FileHash -Algorithm SHA256 -LiteralPath '%~1').Hash -eq (Get-FileHash -Algorithm SHA256 -LiteralPath '%~2').Hash) { exit 0 }; exit 1" >nul 2>&1
if !ERRORLEVEL! EQU 0 exit /b 0
:COPY_VERIFIED_COPY
copy /y "%~1" "%~2" >nul 2>&1
if !ERRORLEVEL! NEQ 0 exit /b 1
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "if ((Get-FileHash -Algorithm SHA256 -LiteralPath '%~1').Hash -eq (Get-FileHash -Algorithm SHA256 -LiteralPath '%~2').Hash) { exit 0 }; exit 1" >nul 2>&1
exit /b !ERRORLEVEL!

:CHECK_X64_BINARY
set "KEYRO_CHECK_PATH=%~1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$path=$env:KEYRO_CHECK_PATH; try { $fs=[System.IO.File]::OpenRead($path); try { $br=New-Object System.IO.BinaryReader($fs); $fs.Position=0x3C; $pe=$br.ReadInt32(); $fs.Position=$pe+4; $machine=$br.ReadUInt16(); if ($machine -eq 0x8664) { exit 0 }; exit 1 } finally { $fs.Dispose() } } catch { exit 1 }" >nul 2>&1
set "CHECK_EXIT=!ERRORLEVEL!"
set "KEYRO_CHECK_PATH="
exit /b !CHECK_EXIT!

:WAIT_SERVICE_GONE
for /l %%I in (1,1,15) do (
    "%SC_EXE%" query "%SERVICE_NAME%" >nul 2>&1
    if errorlevel 1 exit /b 0
    timeout /t 1 /nobreak >nul 2>&1
)
exit /b 1

:WAIT_PIPE
for /l %%I in (1,1,10) do (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$p=[System.IO.Directory]::GetFiles('\\.\pipe\') -contains '\\.\pipe\KeyroIME.Service.v1'; if($p){exit 0}else{exit 1}" >nul 2>&1
    if not errorlevel 1 exit /b 0
    timeout /t 1 /nobreak >nul 2>&1
)
exit /b 1

:START_TRAY_UNELEVATED
set "TRAY_TASK_NAME=KeyroIME_Tray_Launch_!RANDOM!!RANDOM!"
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$action=New-ScheduledTaskAction -Execute $env:TRAY_EXE_DEST; $trigger=New-ScheduledTaskTrigger -AtLogOn; $principal=New-ScheduledTaskPrincipal -UserId ([Security.Principal.WindowsIdentity]::GetCurrent().Name) -LogonType Interactive -RunLevel Limited; $settings=New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -StartWhenAvailable; Register-ScheduledTask -TaskName $env:TRAY_TASK_NAME -Action $action -Trigger $trigger -Principal $principal -Settings $settings -Force | Out-Null; Start-ScheduledTask -TaskName $env:TRAY_TASK_NAME" >nul 2>&1
set "TRAY_START_EXIT=!ERRORLEVEL!"
set "TRAY_PROCESS_READY=0"
if !TRAY_START_EXIT! EQU 0 (
    for /l %%I in (1,1,10) do (
        if !TRAY_PROCESS_READY! EQU 0 (
            "%TASKLIST_EXE%" /FI "IMAGENAME eq keyro_tray.exe" 2>nul | findstr /i "keyro_tray.exe" >nul
            if !ERRORLEVEL! EQU 0 (
                set "TRAY_PROCESS_READY=1"
            ) else (
                timeout /t 1 /nobreak >nul 2>&1
            )
        )
    )
    if !TRAY_PROCESS_READY! EQU 0 set "TRAY_START_EXIT=1"
)
"%SCHTASKS_EXE%" /Delete /TN "!TRAY_TASK_NAME!" /F >nul 2>&1
set "TRAY_TASK_NAME="
set "TRAY_PROCESS_READY="
exit /b !TRAY_START_EXIT!

:WAIT_END
if "%SILENT_MODE%"=="1" exit /b 0
echo Press any key to close this window.
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown') | Out-Null" >nul 2>&1
exit /b 0
