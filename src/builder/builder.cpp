#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define PLATFORM "WINDOWS"

#define B2S(arg) (arg? "TRUE":"FALSE")

int main(int argc, char * argv[]) {

    bool is_release_build = false;
    bool is_min_build     = true;
    bool do_dlls          = false;
    bool do_cleanup       = false;

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "/full") == 0) {
            is_min_build = false;
            continue;
        }

        if(strcmp(argv[i], "/dll") == 0) {
            do_dlls = true;
            continue;
        }

        if(strcmp(argv[i], "/release") == 0) {
            is_release_build = true;
            is_min_build     = false; // That one happens anyway
            do_dlls          = true; // For release builds, we force to rebuild dlls
            continue;
        }

        if(strcmp(argv[i], "/cleanup") == 0) {
            do_cleanup = true;
            continue;
        }
    }

    char flags[1024];

    sprintf(flags, "%s %s /D %s /nologo /Zi /EHsc /I ../src", (is_min_build ? "/Gm":""), (is_release_build ? "/Ox /GL /Gw":"/Od"), PLATFORM);

    printf("\n=================== Game3 Build System ===================\n\n");

    printf("Debug::::::::%s\n", B2S(!is_release_build));
    printf("Incremental::%s\n", B2S(is_min_build));
    printf("DLLs:::::::::%s\n", B2S(do_dlls));
    printf("Cleanup::::::%s\n", B2S(do_cleanup));

    printf("\n\n");

    if(do_dlls) {
        printf("--------------------- Compiling DLLs ---------------------\n");
        // d3d_renderer
        {

            char command[2048];
            sprintf(command, "cl /LD %s /Fed3d_renderer ^ \
                ../src/d3d_renderer.cpp         ^ \
                ../src/asset_manager.cpp        ^ \
                ../src/hash.cpp                 ^ \
                ../src/parsing.cpp              ^ \
                ../src/os/win32/file_loader.cpp ^ \
                /link D3Dcompiler.lib d3d11.lib", flags);

            system(command);
        }

        printf("--------------------- DLLs Compiled ----------------------\n");
    }

    printf("__________________________________________________________\n\n");

    // Main files
    {
        printf("------------------ Compiling Main files ------------------\n");

        char command[2048];
        sprintf(command, "cl %s /Fewin32_game ^ \
        ../src/game_main.cpp             ^ \
        ../src/renderer.cpp              ^ \
        ../src/asset_manager.cpp         ^ \
        ../src/texture_manager.cpp       ^ \
        ../src/shader_manager.cpp        ^ \
        ../src/font_manager.cpp          ^ \
        ../src/room_manager.cpp          ^ \
        ../src/hash.cpp                  ^ \
        ../src/parsing.cpp               ^ \
        ../src/math_m.cpp                ^ \
        ../src/os/win32/core.cpp         ^ \
        ../src/os/win32/hotloader.cpp    ^ \
        ../src/os/win32/file_loader.cpp  ^ \
        ../src/os/win32/sound_player.cpp ^ \
        /link user32.lib dsound.lib dxguid.lib", flags);

        system(command);

        printf("------------------ Main files Compiled -------------------\n");
    }


    printf("__________________________________________________________\n\n");

    if(do_cleanup) {
        system("del *.obj *.pdb *.ilk *.exp *.lib *.idb");
        printf("All cleaned up!");
    }
}
