@echo off
setlocal

rem ============================================================
rem  QHarnesscc - Regression test runner (English only)
rem  Builds qharness_core_tests (Release) and runs all tests.
rem  Assumes build\ is already configured (run build_msvc.bat first).
rem  Exits non-zero on build failure or any test failure.
rem ============================================================

set "SOURCE_DIR=%~dp0"
set "BUILD_DIR=%SOURCE_DIR%build"

rem --- Remove trailing backslash to avoid quote-gluing ---
if "%SOURCE_DIR:~-1%"=="\" set "SOURCE_DIR=%SOURCE_DIR:~0,-1%"

if not exist "%BUILD_DIR%" (
    echo [ERROR] build\ not found. Run build_msvc.bat first to configure.
    exit /b 1
)

echo === Build qharness_core_tests (Release) ===
cmake --build "%BUILD_DIR%" --config Release --target qharness_core_tests
if errorlevel 1 (
    echo [FAIL] Build failed.
    exit /b 1
)

echo.
echo === Run qharness_core_tests ===
"%BUILD_DIR%\out\Release\qharness_core_tests.exe"
if errorlevel 1 (
    echo.
    echo [FAIL] Some tests failed.
    exit /b 1
)

echo.
echo [PASS] All regression tests passed.
endlocal
exit /b 0
