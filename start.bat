@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Retrac OGFN - build
cd /d "%~dp0"

rem ============================================================
rem  start.bat  -  reconfigure + recompile + lance
rem    start.bat          reconfigure + build Release + lance
rem    start.bat build    build Release seulement
rem    start.bat run      lance sans recompiler
rem    start.bat debug    build Debug + lance
rem    start.bat clean    supprime build\ puis rebuild complet
rem    start.bat vs       build puis ouvre la solution
rem ============================================================

set "ROOT=%~dp0"
set "BUILD=%ROOT%build"
set "EXE=%BUILD%\bin\RetracOGFN.exe"
set "CONFIG=Release"
set "DO_BUILD=1"
set "DO_RUN=1"

:args
if "%~1"=="" goto args_done
set "ARG=%~1"
if /i "%ARG%"=="build"   set "DO_RUN=0"
if /i "%ARG%"=="run"     set "DO_BUILD=0"
if /i "%ARG%"=="clean"   set "DO_CLEAN=1"
if /i "%ARG%"=="debug"   set "CONFIG=Debug"
if /i "%ARG%"=="release" set "CONFIG=Release"
if /i "%ARG%"=="vs"      set "DO_OPEN=1"
shift
goto args

:args_done
echo.
echo   Retrac OGFN - build (%CONFIG%)
echo   --------------------------------------------
echo.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)
if defined VSPATH (
    call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
) else (
    echo [!] Visual Studio avec les outils C++ introuvables.
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo [!] cmake introuvable. Relance depuis un "x64 Native Tools Command Prompt".
    goto fail
)

where git >nul 2>nul
if errorlevel 1 echo [i] git absent : le premier configure peut echouer (ImGui).

if defined DO_CLEAN (
    if exist "%BUILD%" (
        echo [*] Suppression de build\...
        rmdir /s /q "%BUILD%"
    )
)

if "%DO_BUILD%"=="1" (
    if not exist "%BUILD%\CMakeCache.txt" (
        echo [*] Configuration ^(x64^)...
        rem Pas de -G : CMake choisit le Visual Studio installe (VS 2022 -> v143,
        rem VS 2026 -> v144). Forcer "17 2022" casse si seul VS 18 est present (MSB8020).
        cmake -S "%ROOT%." -B "%BUILD%" -A x64
        if errorlevel 1 goto fail
    )
    echo [*] Compilation ^(%CONFIG%^)...
    cmake --build "%BUILD%" --config %CONFIG% --parallel
    if errorlevel 1 goto fail
    if not exist "%EXE%" goto fail
    echo [ok] build\bin\RetracOGFN.exe
    echo.
)

if defined DO_OPEN if exist "%BUILD%\RetracOGFN.sln" start "" "%BUILD%\RetracOGFN.sln"

if "%DO_RUN%"=="1" (
    if not exist "%EXE%" goto fail
    echo [*] Lancement ^(UAC^)...
    start "" "%EXE%"
    goto done
)

pause

:done
endlocal
exit /b 0

:fail
echo.
echo [!] Echec.
pause
endlocal
exit /b 1
