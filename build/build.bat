@echo off
set platform="WINDOWS"

if not exist ..\build mkdir ..\build
pushd ..\build

cl /Od /nologo /Zi /EHsc /D %platform% /I ..\src /Febuilder ^
	..\src\builder\builder.cpp

popd
