#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "table.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

struct AssetManager {

    Table<char *, Asset *> table;

    Array<char *> directories;

    Array<char *> assets_to_reload;
    // ------ Functions ------
    virtual void reload_asset(char * file_path, char * file_name, char * extension);

    void perform_reloads();
};
