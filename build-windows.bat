@echo off
setlocal

echo ==========================================
echo   Tico Limiter - Windows Build Script
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
    echo [!] CMake not found.
    echo     Install via: winget install Kitware.CMake
    pause
    exit /b 1
)

:: Check MSVC is now available
where cl >nul 2>&1
if errorlevel 1 (
    echo [!] MSVC compiler still not found after loading VS environment.
    echo     Make sure "Desktop development with C++" is installed in VS Build Tools.
    pause
    exit /b 1
)

echo [1/4] Configuring CMake...
if defined AAX_SDK_PATH (
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_AAX_SDK_PATH=%AAX_SDK_PATH% -G "Visual Studio 17 2022" -A x64
) else (
    cmake -B build -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 (
    echo [!] CMake configuration failed.
    pause
    exit /b 1
)

echo [2/4] Building (this may take a few minutes)...
cmake --build build --config Release
if errorlevel 1 (
    echo [!] Build failed.
    pause
    exit /b 1
)

echo [3/4] Installing VST3...
set "VST3_DIR=%COMMONPROGRAMFILES%\VST3"
if not exist "%VST3_DIR%" mkdir "%VST3_DIR%"
xcopy /E /Y "build\TicoLimiter_artefacts\Release\VST3\Tico Limiter.vst3" "%VST3_DIR%\Tico Limiter.vst3\" >nul

echo [4/4] Creating installer...
where makens >nul 2>&1
if errorlevel 1 (
    echo [!] NSIS not found, skipping installer.
    echo     Install via: winget install NSIS.NSIS
) else (
    makens installer.nsi >nul 2>&1
    if exist "TicoLimiter-Setup.exe" (
        echo   Installer: TicoLimiter-Setup.exe
    )
)

echo.
echo ==========================================
echo   Done! Tico Limiter installed to:
echo   %VST3_DIR%\Tico Limiter.vst3
echo ==========================================
echo.
pause
