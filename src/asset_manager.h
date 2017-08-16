#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "table.h"
#include "parsing.h" // For String

struct Asset {
    char * name;

    double last_reload_time = 0.0f;
    double reload_timeout   = 0.1f;
};

struct AssetManager {
    Array<char *> extensions;

    Array<String> assets_to_reload;

    // ------ Functions ------
    virtual void create_placeholder(char * name);
    virtual void reload_or_create_asset(String file_path, String file_name);

    void perform_reloads();

    void init_asset(Asset * asset);


};

template <typename T>
struct AssetManager_Poly : AssetManager{
    Table<char *, T *> table;
};
