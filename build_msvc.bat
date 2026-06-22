@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem  QHarnesscc - MSVC one-click build script (English only)
rem ============================================================

set "QT_PREFIX=E:\programProject\Qt\5.14.2\msvc2017_64"
set "SOURCE_DIR=%~dp0"
set "BUILD_DIR=%SOURCE_DIR%build"

rem --- Locate Visual Studio via vswhere ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Install Visual Studio with C++ build tools.
    exit /b 1
)

set "VS_MAJOR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -all -property installationVersion`) do (
    set "VS_VERSION=%%i"
    set "VS_MAJOR=!VS_VERSION:~0,2!"
)

rem --- Map VS version to generator ---
rem For VS 18 (Visual Studio 2026+), cmake may auto-select the default
rem VS generator, so we omit -G to let cmake decide.
set "CMAKE_GEN="
set "USE_AUTO_GEN=0"
if "!VS_MAJOR!"=="17" set "CMAKE_GEN=Visual Studio 17 2022"
if "!VS_MAJOR!"=="16" set "CMAKE_GEN=Visual Studio 16 2019"
if "!VS_MAJOR!"=="15" set "CMAKE_GEN=Visual Studio 15 2017"
if not defined CMAKE_GEN set "USE_AUTO_GEN=1"

echo [INFO] Qt prefix : %QT_PREFIX%
if "!USE_AUTO_GEN!"=="0" (
    echo [INFO] Generator : !CMAKE_GEN!
) else (
    echo [INFO] Generator : auto (detected VS !VS_MAJOR!, letting cmake choose default)
)

rem --- Configure ---
set "CMAKE_ARGS=-S %SOURCE_DIR% -B %BUILD_DIR% -A x64 -DCMAKE_PREFIX_PATH=%QT_PREFIX%"
if "!USE_AUTO_GEN!"=="0" set "CMAKE_ARGS=-G "!CMAKE_GEN!" %CMAKE_ARGS%"

cmake %CMAKE_ARGS%
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

rem --- Build ---
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Build succeeded: %BUILD_DIR%\app\Release\qharness_app.exe
endlocal
