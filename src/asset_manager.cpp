#include "asset_manager.h"
#include "parsing.h"
#include "macros.h"

// Default allocator
void * AssetManager::allocate(int size) {
    return malloc(size);
}

int AssetManager::register_asset(Asset * asset) {
    if(num_assets == MAX_NUM_ASSETS) {
        log_print("asset_manager", "No slots left in the catalog");
        return -1;
    }

    // Check if it already exists, if it doesn't, add it to the table.
    int index = find_asset_index(asset->name);

    if(index == -1) {
        index = num_assets;

        map.add(asset->name, index);

         num_assets++;
    }

    assets[index] = asset; // @Leak if asset was heap allocated, which is the case for Textures

    return index; // Return index of added asset
}

int AssetManager::find_asset_index(char * name) {
    return map.get(name);
}

Asset * AssetManager::find_asset_by_name(char * name) {
    int index = find_asset_index(name);

    if(index == -1) {
        return NULL;
    }

    return assets[index];
}

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
