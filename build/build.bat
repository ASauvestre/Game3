@echo off
if not exist ..\build mkdir ..\build
pushd ..\build
echo.
echo =================== Game3 Build System ===================
echo.

set platform="WINDOWS"

echo.
REM if the dll flag is set, recompile the dlls
FOR %%A IN (%*) DO (
    IF "%%A"=="/dll" (
	echo --------------------- Compiling DLLs ---------------------
	cl /nologo /LD /D %platform% /Zi /EHsc /Fe:d3d_renderer ^
	..\src\d3d_renderer.cpp ^
	..\src\asset_manager.cpp ^
	..\src\hash.cpp ^
	/link D3Dcompiler.lib d3d11.lib
	echo --------------------- DLLs Compiled ----------------------
	echo __________________________________________________________
	echo.
   )	
)

REM main files
echo ------------------ Compiling Main files ------------------
cl /Zi /EHsc /D %platform% /I ..\src /Fe:win32_game ^
..\src\game_main.cpp ^
..\src\renderer.cpp ^
..\src\asset_manager.cpp ^
..\src\texture_manager.cpp ^
..\src\hash.cpp ^
..\src\os\win32\core.cpp ^
..\src\os\win32\hotloader.cpp ^
/link /nologo user32.lib Gdi32.lib d3d11.lib D3DCompiler.lib Winmm.lib
echo ------------------ Main files Compiled -------------------

echo.
echo ==========================================================
echo.

REM Cleanup if flag is set
FOR %%A IN (%*) DO (
    IF "%%A"=="/cleanup" (
	del *.obj *.pdb *.ilk *.exp *.lib
   )	
)

popd