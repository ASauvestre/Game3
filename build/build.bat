@echo off
set platform="WINDOWS"

if not exist ..\build mkdir ..\build
pushd ..\build


echo.
echo =================== Game3 Build System ===================
echo.
echo.


REM Enable optimization if release flag is set
FOR %%A IN (%*) DO (
    IF "%%A"=="/release" (
        set release_flags="/Ox /GL /Gw"
   )
)
set release_flags=%release_flags:"=%


REM Disable minimal rebuild if full flag is set
set min_build="/Gm"
set min_build=%min_build:"=%
FOR %%A IN (%*) DO (
    IF "%%A"=="/full" (
        set min_build=
    )
)


REM if the dll flag is set, recompile the dlls
FOR %%A IN (%*) DO (
    IF "%%A"=="/dll" (


    echo --------------------- Compiling DLLs ---------------------


    cl %min_build% %release_flags% /LD /nologo /Zi /EHsc /D %platform% /I ..\src /Fed3d_renderer ^
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


echo ------------------ Compiling Main files ------------------


cl %min_build% %release_flags% /nologo /Zi /EHsc /D %platform% /I ..\src /Fewin32_game ^
    ..\src\game_main.cpp ^
    ..\src\renderer.cpp ^
    ..\src\asset_manager.cpp ^
    ..\src\texture_manager.cpp ^
    ..\src\shader_manager.cpp ^
    ..\src\font_manager.cpp ^
    ..\src\room_manager.cpp ^
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
    del *.obj *.pdb *.ilk *.exp *.lib *.idb
   )
)


popd
