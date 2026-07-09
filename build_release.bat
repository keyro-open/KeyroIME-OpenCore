@echo off
chcp 65001 >nul
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT_DIR=%~dp0"
set "RELEASE_DIR=%ROOT_DIR%release"
set "TSF_BUILD_DIR=%ROOT_DIR%src\tsf_shell\build"
set "SERVICE_EXE=%ROOT_DIR%target\keyro_service_runtime\x86_64-pc-windows-msvc\release\keyro_service.exe"
set "TSF_DLL=%TSF_BUILD_DIR%\Release\KeyroIME.dll"
set "TRAY_EXE=%TSF_BUILD_DIR%\Release\keyro_tray.exe"
set "SAMPLE_ASSET_DIR=%ROOT_DIR%src\keyro_service\assets"
set "FULL_ASSET_DIR=%ROOT_DIR%dictionary_dumps\full_assets_current"
set "DICTIONARY_ASSET_DIR=%SAMPLE_ASSET_DIR%"
if exist "%FULL_ASSET_DIR%\dictionary_manifest.json" set "DICTIONARY_ASSET_DIR=%FULL_ASSET_DIR%"
set "KEYROIME_DICTIONARY_ASSET_DIR=%DICTIONARY_ASSET_DIR%"

echo ========================================
echo KeyroIME v1.0 release build
echo ========================================

echo Building the Rust service...
echo Dictionary assets: "%KEYROIME_DICTIONARY_ASSET_DIR%"
cargo build --release --target x86_64-pc-windows-msvc --manifest-path "%ROOT_DIR%src\keyro_service\Cargo.toml" --target-dir "%ROOT_DIR%target\keyro_service_runtime"
if errorlevel 1 (
    echo Rust service build failed.
    exit /b 1
)

echo Running Rust tests...
cargo test --release --target x86_64-pc-windows-msvc --manifest-path "%ROOT_DIR%src\keyro_service\Cargo.toml" --target-dir "%ROOT_DIR%target\keyro_service_runtime"
if errorlevel 1 (
    echo Rust tests failed.
    exit /b 1
)

echo Configuring the TSF shell...
cmake -S "%ROOT_DIR%src\tsf_shell" -B "%TSF_BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo TSF shell configuration failed.
    exit /b 1
)

echo Building the TSF shell and smoke tests...
cmake --build "%TSF_BUILD_DIR%" --config Release --target KeyroIME keyro_tray keyro_tray_menu_smoke keyro_runtime_input_smoke keyro_ipc_bench keyro_ipc_failover_smoke keyro_tsf_activation_smoke keyro_local_fallback_smoke tsf_core
if errorlevel 1 (
    echo TSF shell build failed.
    exit /b 1
)

echo Running TSF activation smoke test...
"%TSF_BUILD_DIR%\Release\keyro_tsf_activation_smoke.exe"
if !ERRORLEVEL! NEQ 0 (
    echo TSF activation smoke test failed with exit code !ERRORLEVEL!.
    exit /b 1
)

echo Running local fallback smoke test...
"%TSF_BUILD_DIR%\Release\keyro_local_fallback_smoke.exe"
if !ERRORLEVEL! NEQ 0 (
    echo Local fallback smoke test failed with exit code !ERRORLEVEL!.
    exit /b 1
)

echo Running tray menu smoke test...
"%TSF_BUILD_DIR%\Release\keyro_tray_menu_smoke.exe"
if !ERRORLEVEL! NEQ 0 (
    echo Tray menu smoke test failed with exit code !ERRORLEVEL!.
    exit /b 1
)

echo Running IPC failover smoke test...
"%TSF_BUILD_DIR%\Release\keyro_ipc_failover_smoke.exe"
if !ERRORLEVEL! NEQ 0 (
    echo IPC failover smoke test failed with exit code !ERRORLEVEL!.
    exit /b 1
)

if not exist "%SERVICE_EXE%" (
    echo keyro_service.exe was not produced.
    exit /b 1
)
if not exist "%TSF_DLL%" (
    echo KeyroIME.dll was not produced.
    exit /b 1
)
if not exist "%TRAY_EXE%" (
    echo keyro_tray.exe was not produced.
    exit /b 1
)

if /i not "%RELEASE_DIR%"=="%ROOT_DIR%release" (
    echo Release directory safety check failed.
    exit /b 1
)
if exist "%RELEASE_DIR%" rd /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%" >nul 2>&1

echo Copying release files...
copy /y "%TSF_DLL%" "%RELEASE_DIR%\KeyroIME.dll" >nul
copy /y "%TRAY_EXE%" "%RELEASE_DIR%\keyro_tray.exe" >nul
copy /y "%SERVICE_EXE%" "%RELEASE_DIR%\keyro_service.exe" >nul
copy /y "%ROOT_DIR%install.bat" "%RELEASE_DIR%\install.bat" >nul
copy /y "%ROOT_DIR%THIRD_PARTY_NOTICES.md" "%RELEASE_DIR%\THIRD_PARTY_NOTICES.md" >nul
copy /y "%ROOT_DIR%LICENSE_ja.txt" "%RELEASE_DIR%\LICENSE_ja.txt" >nul
copy /y "%ROOT_DIR%LICENSE_en.txt" "%RELEASE_DIR%\LICENSE_en.txt" >nul
copy /y "%KEYROIME_DICTIONARY_ASSET_DIR%\dictionary_manifest.json" "%RELEASE_DIR%\dictionary_manifest.json" >nul
copy /y "%ROOT_DIR%README.release.ja.md" "%RELEASE_DIR%\README.ja.md" >nul
if errorlevel 1 (
    echo Copying release files failed.
    exit /b 1
)

echo ========================================
echo Release package created: "%RELEASE_DIR%"
echo ========================================
exit /b 0
