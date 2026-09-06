@echo off
chcp 65001 >nul
setlocal EnableExtensions

set "ROOT_DIR=%~dp0"
set "BUILD_DIR=%ROOT_DIR%src\tsf_shell\build"
set "PRODUCT_VERSION=1.0.6.16"
if exist "%ROOT_DIR%VERSION" set /p PRODUCT_VERSION=<"%ROOT_DIR%VERSION"

echo ========================================
echo KeyroIME v%PRODUCT_VERSION% sandbox build
echo ========================================

echo Configuring the TSF shell...
cmake -S "%ROOT_DIR%src\tsf_shell" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configuration failed.
    exit /b 1
)

echo Building the sandbox test bench...
cmake --build "%BUILD_DIR%" --config Release --target keyro_test_bench
if errorlevel 1 (
    echo Sandbox build failed.
    exit /b 1
)

echo ========================================
echo Sandbox build completed.
echo Executable: "%BUILD_DIR%\Release\keyro_test_bench.exe"
echo ========================================
exit /b 0
