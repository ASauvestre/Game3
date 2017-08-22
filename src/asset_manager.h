#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "table.h"
#include "parsing.h" // For String

struct Asset {
    char * name;
    char * full_path;
};

struct AssetManager {
    Array<char *> extensions;

    Array<String> assets_to_reload;

    // ------ Functions ------
    virtual void create_placeholder(char * name, char * path);
    virtual void reload_or_create_asset(String file_path, String file_name);

    void perform_reloads();
};

template <typename T>
struct AssetManager_Poly : AssetManager{
    Table<char *, T *> table;
};
