#include "asset_manager.h"
#include "parsing.h"
#include "macros.h"

// @Cleanup think of something else to do here or remove it.
// bool AssetManager::register_asset(Asset * asset) {
//     return this->table.add(asset->name, asset);
// }

void AssetManager::reload_asset(char * file_path, char * file_name, char * extension) {
    log_print("reload_asset", "Asset %s is up for reloading, but the manager has no reload_asset funtion", file_name);
}

void AssetManager::perform_reloads() {
    AssetManager * am = this; //@Cleanup

    int num_assets_to_reload = am->assets_to_reload.count;

    for(int i = 0; i < num_assets_to_reload; i++) {
        char * file_path = am->assets_to_reload.data[i];
        scope_exit(free(file_path)); // Allocated by strdup in hotloader.cpp

        char * file_name = find_char_from_right('/', file_path); // @Redundant, this is already computed in hotloader.cpp, we could simply have another array with file names to reload.

        if(file_name == NULL) {
            file_name = file_path + 1; // Root directory
        }

        char * extension = find_char_from_left('.', file_name);

        if(extension == NULL) {
            log_print("perform_manager_reloads", "Skipped file %s because it has no extension", file_name);
            continue;
        }

        // Do reloads based on extension @Temporary, allow managers to filter by extensions
        if(strcmp("png", extension) == 0) {

            // @Incomplete, we're getting two of those when PS saves, make sure we update only once.
            // @Incomplete @Temporary, the extension check shouldn't happen here.
            reload_asset(file_path, file_name, extension);

            // log_print("perform_manager_reloads", "Texture with file_name %s and extension %s up for reload. Full path is : %s", file_name, extension, file_path);

        }
    }

    am->assets_to_reload.reset(true);
}
