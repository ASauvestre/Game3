#include <stdlib.h>
#include <cstring>

#pragma once

struct PlatformTextureInfo;

struct Texture {
    char * name;

    int width;
    int height;

    int bytes_per_pixel;

    unsigned char * bitmap;
    int width_in_bytes;
    int num_bytes;

    PlatformTextureInfo * platform_info; // Point to data structure containing platform specific fields
};

// Maps a string to an integer
// TODO: Maybe templatize this if we end up needing it.
struct HashTable {
    static const int MAX_SIZE = 16384;

    struct Element {
        char * key;
        int value;
    };

    struct Bucket {
        int num_elements;
        Element * elements;
    };
    
    Bucket values[MAX_SIZE];

    int hash(char* key) {
        int hash_result = 0;
        while(*key !='\0') {
            hash_result += (int) *key;
            key += sizeof(char); // Move to next character;
        }
        return hash_result % this->MAX_SIZE;
    }

    int add(char* key, int value) {

        int hash_result = hash(key);
        Element new_element;
        new_element.key = key;
        new_element.value = value;

        values[hash_result].elements = (Element *) realloc(values[hash_result].elements, 
                                                   sizeof(Element) * (values[hash_result].num_elements + 1));   
        values[hash_result].elements[values[hash_result].num_elements] = new_element;
        
        values[hash_result].num_elements++;

        return hash_result;
    }

    int get(char* key) {
        int hash_result = hash(key);

        Bucket bucket = values[hash_result];

        if(bucket.num_elements == 0) return -1;
        for(int i = 0; i < bucket.num_elements; i++) {
            if(strcmp(key, bucket.elements[i].key) == 0) {
                return bucket.elements[i].value;
            }
        }
        return -1;
    }
};

struct TextureManager {
    static const int MAX_NUM_TEXTURES = 1024; // @Temporary, make the catalog grow dynamically (or not, but look into it).

    int num_textures;

    Texture textures[MAX_NUM_TEXTURES];
    HashTable texture_indices_table;

    int register_texture(Texture * texture) {
        texture_indices_table.add(texture->name, num_textures);

        num_textures++;
        
        return num_textures - 1;
    }

    Texture * get_new_texture_slot() {
        if(num_textures == MAX_NUM_TEXTURES) {
            log_print("texture_manager", "No slots left in the catalog");
            return NULL;
        } else {
            return &textures[num_textures];
        }
    }

    int find_texture_index(char * name) {
        return texture_indices_table.get(name);
    }

    Texture find_texture(char * name) {
        int texture_index = find_texture_index(name);
        return textures[texture_index];
    }
};
