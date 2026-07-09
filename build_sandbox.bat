@echo off
chcp 65001 >nul
echo ========================================
echo KeyroIME v1.0 sandbox test build script
echo ========================================
echo.

REM Enter the TSF shell directory.
cd src\tsf_shell

REM Create the build directory.
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

REM Configure CMake.
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64

if %errorLevel% NEQ 0 (
    echo CMake configuration failed.
    pause
    exit /b 1
)

REM Build the Release target.
echo Building Release target...
cmake --build . --config Release --target keyro_test_bench

if %errorLevel% NEQ 0 (
    echo Build failed.
    pause
    exit /b 1
)

REM Copy ime_core.dll when the compatibility DLL exists.
echo Copying ime_core.dll...
if exist "..\..\ime_core\target\release\ime_core.dll" (
    copy /Y "..\..\ime_core\target\release\ime_core.dll" "Release\" >nul
    echo [OK] ime_core.dll copied.
) else (
    echo [ERROR] ime_core.dll was not found.
    echo Build the Rust compatibility engine first:
    echo   cargo build --release --manifest-path src/ime_core/Cargo.toml
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed.
echo ========================================
echo.
echo Executable:
echo %CD%\Release\keyro_test_bench.exe
echo.
echo Run:
echo   cd %CD%\Release
echo   keyro_test_bench.exe
echo.

pause
