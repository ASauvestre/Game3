#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "array.h"

#define COMMON_TYPES_IMPLEMENTATION
#include "common_types.h"

// Maps a string to an integer
// @Incomplete Maybe templatize this if we end up needing it.
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

        Bucket * bucket = &values[hash_result];

        if(bucket->num_elements == 0) return -1;
        for(int i = 0; i < bucket->num_elements; i++) {
            if(strcmp(key, bucket->elements[i].key) == 0) {
                return bucket->elements[i].value;
            }
        }
        return -1;
    }

    void remove(char * key) {
        int index = get(key);

        if(index == -1) return; // Key is not in the table

        Bucket * bucket = &values[index];

        for(int i = 0; i < bucket->num_elements; i++) {
            if(strcmp(key, bucket->elements[i].key) == 0) {

                // Shrink array
                bucket->elements[i] = bucket->elements[bucket->num_elements - 1];

                bucket->elements = (Element *) realloc(bucket->elements, sizeof(Element) * (bucket->num_elements - 1));

                bucket->num_elements--;
            }
        }
    }
};

struct AssetManager {

    // ------ Constants ------
    static const int MAX_NUM_ASSETS = 1024; // @Temporary, make the array grow dynamically (or not, but look into it).

    // ------ Variables ------
    Asset * assets[MAX_NUM_ASSETS];

    int num_assets;

    HashTable map;

    Array<char *> directories;

    Array<char *> assets_to_reload;


    // ------ Functions ------

    void * allocate(int size);

    int register_asset(Asset * asset);

    int find_asset_index(char * name);

    Asset * find_asset_by_name(char * name);

    void perform_reloads();

    virtual void reload_asset(char * file_path, char * file_name, char * extension);
};
