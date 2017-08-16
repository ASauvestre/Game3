#include "asset_manager.h"
#include "parsing.h"
#include "macros.h"

// @Cleanup think of something else to do here or remove it.
// bool AssetManager::register_asset(Asset * asset) {
//     return this->table.add(asset->name, asset);
// }

// @Cleanup, move this to placeholder making.
void AssetManager::init_asset(Asset * asset) {
    asset->last_reload_time = 0.0f;
    asset->reload_timeout   = 0.1f;
}

void AssetManager::reload_asset(String file_path, String file_name) {
    char * c_file_name = to_c_string(file_name);
    scope_exit(free(c_file_name));

    log_print("reload_asset", "Asset %s is up for reloading, but the manager has no reload_asset funtion", c_file_name);
}

void AssetManager::perform_reloads() {
    AssetManager * am = this; //@Cleanup

    int num_assets_to_reload = am->assets_to_reload.count;

    for(int i = 0; i < num_assets_to_reload; i++) {
        String file_path = am->assets_to_reload.data[i];
        scope_exit(free(file_path.data)); // Allocated by strdup in hotloader.cpp

        // @Redundant, this is already computed in hotloader.cpp, we could simply have another array with file names to reload.
        String file_name = find_char_from_right('/', file_path);

        if(!file_name.count) {
            file_name = file_path;
            push(&file_name); // Gets rid of '/'
        }
        // End @Redundant

        // @Incomplete, allow filtering by extension
        reload_asset(file_path, file_name);
    }
    am->assets_to_reload.reset(true);
}
