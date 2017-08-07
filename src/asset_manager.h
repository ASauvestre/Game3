#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "table.h"

struct Asset {
    char * name;
};

struct AssetManager {
    Array<char *> directories;

    Array<char *> assets_to_reload;
    // ------ Functions ------
    virtual void reload_asset(char * file_path, char * file_name, char * extension);

    void perform_reloads();
};

template <typename T>
struct AssetManager_Poly : AssetManager{
    Table<char *, T *> table;
};
