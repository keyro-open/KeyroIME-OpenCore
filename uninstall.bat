@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SERVICE_NAME=KeyroIME_Service"
set "INSTALL_DIR=%ProgramFiles%\KeyroIME"
set "DATA_DIR=%ProgramData%\KeyroIME"
set "PURGE_DATA=0"
set "SILENT_MODE=0"
set "VALIDATE_ONLY=0"
if /i "%~1"=="/purge" set "PURGE_DATA=1"
if /i "%~1"=="/silent" set "SILENT_MODE=1"
if /i "%~2"=="/purge" set "PURGE_DATA=1"
if /i "%~2"=="/silent" set "SILENT_MODE=1"
if /i "%~1"=="/validate" set "VALIDATE_ONLY=1"

set "SYSTEM_TOOL_DIR=%SystemRoot%\System32"
if exist "%SystemRoot%\Sysnative\regsvr32.exe" set "SYSTEM_TOOL_DIR=%SystemRoot%\Sysnative"
set "REGSVR32_EXE=%SYSTEM_TOOL_DIR%\regsvr32.exe"
set "SC_EXE=%SYSTEM_TOOL_DIR%\sc.exe"
set "REG_EXE=%SYSTEM_TOOL_DIR%\reg.exe"

echo ========================================
echo KeyroIME OpenCore uninstaller
echo ========================================

if "%VALIDATE_ONLY%"=="1" (
    if /i not "%INSTALL_DIR%"=="%ProgramFiles%\KeyroIME" exit /b 1
    if /i not "%DATA_DIR%"=="%ProgramData%\KeyroIME" exit /b 1
    if not exist "%REGSVR32_EXE%" exit /b 1
    if not exist "%SC_EXE%" exit /b 1
    echo Uninstaller validation passed.
    exit /b 0
)

net session >nul 2>&1
if errorlevel 1 (
    echo Administrator privileges are required.
    call :WAIT_END
    exit /b 1
)

taskkill /im keyro_tray.exe /f >nul 2>&1

echo Stopping and removing the backend service...
"%SC_EXE%" query "%SERVICE_NAME%" >nul 2>&1
if not errorlevel 1 (
    "%SC_EXE%" stop "%SERVICE_NAME%" >nul 2>&1
    call :WAIT_SERVICE_STOPPED
    "%SC_EXE%" delete "%SERVICE_NAME%" >nul 2>&1
)

set "TSF_DLL_PATH="
for /f "usebackq delims=" %%P in (`powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$key=[Microsoft.Win32.Registry]::LocalMachine.OpenSubKey('SOFTWARE\Classes\CLSID\{8B4F9B54-7B15-4D8C-9E32-6D5A17B0A51E}\InprocServer32'); if($key){try{$key.GetValue('')}finally{$key.Dispose()}}"`) do set "TSF_DLL_PATH=%%P"

echo Unregistering the TSF text service...
if defined TSF_DLL_PATH if exist "!TSF_DLL_PATH!" (
    "%REGSVR32_EXE%" /u /s "!TSF_DLL_PATH!"
)

"%REG_EXE%" delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "KeyroIME_Tray" /f >nul 2>&1

if /i not "%INSTALL_DIR%"=="%ProgramFiles%\KeyroIME" (
    echo Installation directory safety check failed.
    call :WAIT_END
    exit /b 1
)

if exist "%INSTALL_DIR%" rd /s /q "%INSTALL_DIR%" >nul 2>&1
if exist "%INSTALL_DIR%" (
    echo Some installed files are still in use. Sign out and run this uninstaller again.
    call :WAIT_END
    exit /b 1
)

if "%PURGE_DATA%"=="1" (
    if /i not "%DATA_DIR%"=="%ProgramData%\KeyroIME" (
        echo Data directory safety check failed.
        call :WAIT_END
        exit /b 1
    )
    if exist "%DATA_DIR%" rd /s /q "%DATA_DIR%" >nul 2>&1
)

echo KeyroIME OpenCore was uninstalled successfully.
if "%PURGE_DATA%"=="0" echo User dictionary data was preserved in "%DATA_DIR%".
call :WAIT_END
exit /b 0

:WAIT_SERVICE_STOPPED
for /l %%I in (1,1,15) do (
    "%SC_EXE%" query "%SERVICE_NAME%" 2>nul | findstr /i "STOPPED" >nul
    if not errorlevel 1 exit /b 0
    timeout /t 1 /nobreak >nul 2>&1
)
exit /b 1

:WAIT_END
if "%SILENT_MODE%"=="1" exit /b 0
echo Press any key to close this window.
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown') | Out-Null" >nul 2>&1
exit /b 0
