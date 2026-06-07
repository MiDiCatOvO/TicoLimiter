@echo off
setlocal

echo ==========================================
echo   Tico Limiter - Build and Package
echo ==========================================
echo.

:: Find and load VS Build Tools environment
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if defined VS_PATH (
    echo [*] Found VS at: %VS_PATH%
    call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=amd64 >nul 2>&1
) else (
    echo [!] Visual Studio Build Tools not found.
    echo     Install via: winget install Microsoft.VisualStudio.2022.BuildTools
    echo     Then run the installer and select "Desktop development with C++".
    pause
    exit /b 1
)

:: Check for CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [!] CMake not found. Install: winget install Kitware.CMake
    pause
    exit /b 1
)

:: Check MSVC
where cl >nul 2>&1
if errorlevel 1 (
    echo [!] MSVC not found. Make sure "Desktop development with C++" is installed.
    pause
    exit /b 1
)

echo [1/4] Configuring...
if defined AAX_SDK_PATH (
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_AAX_SDK_PATH=%AAX_SDK_PATH% -G "Visual Studio 17 2022" -A x64
) else (
    cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 goto :fail

echo [2/4] Compiling (this may take a few minutes)...
cmake --build build --config Release
if errorlevel 1 goto :fail

echo [3/4] Creating installer...
where makens >nul 2>&1
if errorlevel 1 (
    echo [!] NSIS not found. Install: winget install NSIS.NSIS
    pause
    exit /b 1
)
makens installer.nsi
if errorlevel 1 goto :fail

echo [4/4] Done!
echo.
echo   Installer: TicoLimiter-Setup.exe
echo.
pause
exit /b 0

:fail
echo.
echo [!] Build failed. Check errors above.
pause
exit /b 1
