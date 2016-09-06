@echo off

if %1.==. goto usage

set update_all_arg1=%1

:: strip off relative path
if "%update_all_arg1:~0,3%" == "..\" set update_all_arg1=%update_all_arg1:~3%

set update_all_startdir=%cd%
cd %~dp0
call update_vs_files_for_dx11_sample.bat %update_all_arg1% vs2010
call update_vs_files_for_dx11_sample.bat %update_all_arg1% vs2012
call update_vs_files_for_dx11_sample.bat %update_all_arg1% vs2013
call update_vs_files_for_dx11_sample.bat %update_all_arg1% vs2015

:: we don't keep VS2010 projects for the sample
del /f /q ..\%update_all_arg1%\build\%update_all_arg1%_2010.*

cd %update_all_startdir%

goto :EOF

::--------------------------
:: usage should be last
::--------------------------

:usage
echo   usage: %0 sample_dir_name
echo      or: %0 ..\sample_dir_name
echo example: %0 forwardplus11
echo      or: %0 ..\forwardplus11
