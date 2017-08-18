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
    cl /LD /nologo /Zi /EHsc /D %platform% /I ..\src /Fe:d3d_renderer ^
        ..\src\d3d_renderer.cpp ^
        ..\src\asset_manager.cpp ^
        ..\src\hash.cpp ^
        ..\src\parsing.cpp ^
        ..\src\os\win32\file_loader.cpp ^
    /link D3Dcompiler.lib d3d11.lib
    echo --------------------- DLLs Compiled ----------------------
    echo __________________________________________________________
    echo.
   )
)

REM main files
echo ------------------ Compiling Main files ------------------
cl /nologo /Zi /EHsc /D %platform% /I ..\src /Fe:win32_game ^
    ..\src\game_main.cpp ^
    ..\src\renderer.cpp ^
    ..\src\asset_manager.cpp ^
    ..\src\texture_manager.cpp ^
    ..\src\shader_manager.cpp ^
    ..\src\font_manager.cpp ^
    ..\src\hash.cpp ^
    ..\src\os\win32\core.cpp ^
    ..\src\os\win32\hotloader.cpp ^
    ..\src\os\win32\file_loader.cpp ^
    ..\src\parsing.cpp ^
/link user32.lib
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
