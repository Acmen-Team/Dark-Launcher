@echo off
pushd %~dp0\..\
call vendor\premake\premake5.exe --file=Launcher.lua vs2022
popd
PAUSE