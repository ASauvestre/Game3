#include "asset_manager.h"
#include "parsing.h"
#include "macros.h"

// @Incomplete, make those pointers instead of v functions
void AssetManager::create_placeholder(char * name, char * path) {
    log_print("create_placeholder", "we ned to create a placeholder for asset %s, but the manager has no create_placeholder function", name);
}

void AssetManager::reload_or_create_asset(String file_path, String file_name) {
    char * c_file_name = to_c_string(file_name);
    scope_exit(free(c_file_name));

    log_print("reload_or_create_asset", "Asset %s is up for reloading, but the manager has no reload_or_create_asset function", c_file_name);
}

void AssetManager::perform_reloads() {
    for_array(this->assets_to_reload.data, this->assets_to_reload.count) {
        //scope_exit(free(it->full_path.data));
        reload_or_create_asset(it->full_path, it->name);
    }

    this->assets_to_reload.reset(true);
}
