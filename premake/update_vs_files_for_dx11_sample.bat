@echo off

if %1.==. goto usage
if %2.==. goto usage

set arg1=%1
set arg2=%2

:: strip off relative path
if "%arg1:~0,3%" == "..\" set arg1=%arg1:~3%

set startdir=%cd%
cd %~dp0

if exist ..\amd_lib\premake (
    echo --- amd_lib ---
    cd ..\amd_lib\premake
    call :createvsfiles %arg2%
    call :createvsfileswithminimaldependencies %arg2%
    cd %~dp0
)

if exist ..\amd_sdk\premake (
    echo --- amd_sdk ---
    cd ..\amd_sdk\premake
    call :createvsfiles %arg2%
    call :createvsfileswithminimaldependencies %arg2%
    cd %~dp0
)

echo --- dxut core ---
cd ..\dxut\Core
call :createvsfiles %arg2%

echo --- dxut optional ---
cd ..\Optional
call :createvsfiles %arg2%
cd ..\..\
:: we don't keep solution files for amd_lib, amd_sdk, or dxut
call :cleanslnfiles

echo --- %arg1% ---
cd %arg1%\premake
call :createvsfiles %arg2%

cd %startdir%

goto :EOF

::--------------------------
:: SUBROUTINES
::--------------------------

:: run premake for passed-in action (e.g. vs2012, vs2013, vs2015)
:createvsfiles
..\..\premake\premake5.exe %1
goto :EOF

:: run premake for passed-in action (e.g. vs2012, vs2013, vs2015)
:: minimal-dependencies version
:createvsfileswithminimaldependencies
..\..\premake\premake5.exe --file=premake5_minimal.lua %1
goto :EOF

:: delete unnecessary sln files
:cleanslnfiles
if exist amd_lib\build\nul del /f /q amd_lib\build\AMD_LIB_*.sln
if exist amd_sdk\build\nul del /f /q amd_sdk\build\AMD_SDK_*.sln
del /f /q dxut\Core\DXUT_*.sln
del /f /q dxut\Optional\DXUTOpt_*.sln
goto :EOF

::--------------------------
:: usage should be last
::--------------------------

:usage
echo   usage: %0 sample_dir_name premake_action
echo      or: %0 ..\sample_dir_name premake_action
echo example: %0 forwardplus11 vs2012
echo      or: %0 ..\forwardplus11 vs2012
