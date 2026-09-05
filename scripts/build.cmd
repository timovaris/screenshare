@echo off
setlocal

REM Build using Visual Studio 2022 C++ tools, Windows SDK, and CMake.

set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build\windows"

REM Locate latest VS Build Tools installation with C++ tools.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo ERROR: vswhere.exe not found. Install Visual Studio Build Tools.
  exit /b 1
)

for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
  set "VSINSTALL=%%I"
)

if not defined VSINSTALL (
  echo ERROR: No VS Build Tools installation with C++ found.
  exit /b 1
)

set "CMAKE=cmake"
where cmake >nul 2>nul
if errorlevel 1 set "CMAKE=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

"%CMAKE%" -S "%ROOT%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

"%BUILD_DIR%\Release\RegionModelTests.exe"
if errorlevel 1 exit /b 1
echo Build complete: %BUILD_DIR%\Release\RegShare.exe
