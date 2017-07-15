#pragma once

#include <stdio.h>
#include <cstring>

#include "macros.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

#include "asset_manager.h"

struct PlatformTextureInfo;

struct TextureManager : AssetManager{

    Texture textures[MAX_NUM_ELEMENTS];
    HashTable map;

    int register_texture(Texture * texture) {
        map.add(texture->name, num_elements);

        num_elements++;
        
        return num_elements - 1;
    }

    Texture * get_new_texture_slot() {
        if(num_elements == MAX_NUM_ELEMENTS) {
            log_print("texture_manager", "No slots left in the catalog");
            return NULL;
        } else {
            return &textures[num_elements];
        }
    }

    int find_texture_index(char * name) {
        return map.get(name);
    }

    Texture find_texture(char * name) {
        int texture_index = find_texture_index(name);
        return textures[texture_index];
    }
};
