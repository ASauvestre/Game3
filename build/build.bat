@echo off
if not exist ..\build mkdir ..\build
pushd ..\build
echo.
echo =================== Game3 Build System ===================
echo.

REM if the dll flag is set, recompile the dlls
FOR %%A IN (%*) DO (
    IF "%%A"=="/dll" (
	echo --------------------- Compiling DLLs ---------------------
	cl /nologo /LD /Zi /EHsc /Fe:d3d_renderer ..\src\d3d_renderer.cpp /link D3Dcompiler.lib d3d11.lib
	echo --------------------- DLLs Compiled ----------------------
	echo __________________________________________________________
	echo.
   )	
)

REM main files
echo ------------------ Compiling Main files ------------------
cl /Zi /EHsc /D WIN32 /Fe:win32_game ^
..\src\win32_main.cpp ^
..\src\game_main.cpp ^
..\src\renderer.cpp ^
/link /nologo user32.lib Gdi32.lib d3d11.lib D3DCompiler.lib Winmm.lib
echo ------------------ Main files Compiled -------------------

echo.
echo ==========================================================
echo.

REM Cleanup

REM del *.obj *.pdb *.ilk
popd