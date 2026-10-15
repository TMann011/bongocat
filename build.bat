@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo ============================================================
echo   Building Bongo Cat Memory Trainer (MSVC x64 C++)
echo ============================================================

:: 1. Check if cl.exe is already active in current environment
where cl.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [+] Using active MSVC compiler from PATH...
    goto :COMPILE
)

:: 2. Auto-discover Visual Studio via vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_INSTALL=%%i"
    )
    if defined VS_INSTALL (
        if exist "!VS_INSTALL!\VC\Auxiliary\Build\vcvars64.bat" (
            echo [+] Auto-detected Visual Studio at: !VS_INSTALL!
            call "!VS_INSTALL!\VC\Auxiliary\Build\vcvars64.bat" >nul
            goto :COMPILE
        )
    )
)

:: 3. Probing fallback paths
for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist "%%~P" (
        echo [+] Found MSVC build environment: %%~P
        call "%%~P" >nul
        goto :COMPILE
    )
)

echo [-] Error: Microsoft Visual C++ Build Tools or Visual Studio not found on this system.
echo [-] Please install Visual Studio with "Desktop development with C++" workload.
pause
exit /b 1

:COMPILE
echo [+] Compiling trainer.cpp into BongoTrainer.exe...
cl.exe /nologo /utf-8 /O2 /EHsc /MT /W3 "%~dp0trainer.cpp" /Fe:"%~dp0BongoTrainer.exe" user32.lib gdi32.lib shell32.lib advapi32.lib /link /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"

if %ERRORLEVEL% equ 0 (
    echo.
    echo ============================================================
    echo [+] BUILD SUCCESSFUL: BongoTrainer.exe generated!
    echo ============================================================
    if exist "%~dp0trainer.obj" del /f /q "%~dp0trainer.obj" >nul 2>&1
    echo [+] Ready to run: BongoTrainer.exe
) else (
    echo.
    echo [-] BUILD FAILED! Check error diagnostics above.
    pause
    exit /b 1
)
