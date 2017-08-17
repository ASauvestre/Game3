#include "hash.h"
#include "array.h"

// @Note: This is a variant of an OA hash table where keys, values, hashes and states are stored in their own array. Good for iterating, but very bad for resizing when it gets big. we only use it to load our assets, which are all loaded at startup, so it's fine.
template <typename K, typename V>
struct Table {

    int count = 0, allocated = 0;

    Array<K> keys;
    Array<V> values;
    Array<unsigned int> hashes; // Stored so that we don't have to recompute it when we search.

    Array<bool> mask;

    inline unsigned int get_hash(K key) { return murmur_hash_2(key, strlen(key), 0); } //@Incomplete, assumes char *.

    bool add     (K key, V value);

    V    find    (K key);

    bool remove  (K key);

    bool reserve (int to_reserve);
};

static const float MAX_LOAD_FACTOR = 0.8f; // @Incomplete, determine what is the best value to put here, Wikipedia says OA becomes crap after 0.8 but it might be considering a more efficent rehashing.
static const int MIN_SIZE_TABLE = 1;

template <typename K, typename V>
bool Table<K, V>::reserve(int to_reserve) {
    if(to_reserve < MIN_SIZE_TABLE) to_reserve = MIN_SIZE_TABLE;
    if(to_reserve < this->allocated) return true;

    Table<K, V> new_table;

    bool success = true;

    success &= new_table.keys.reserve(to_reserve);
    success &= new_table.values.reserve(to_reserve);
    success &= new_table.hashes.reserve(to_reserve);
    success &= new_table.mask.reserve(to_reserve, true); // Memset to 0

    if(!success) return false;

    new_table.allocated = to_reserve;

    for_array(this->mask.data, this->mask.allocated) {
        if(*it) {
            new_table.add(this->keys.data[it_index], this->values.data[it_index]);
        }
    }

    this->keys.reset(true);
    this->values.reset(true);
    this->hashes.reset(true);
    this->mask.reset(true);

    *this = new_table;

    return true;
}

template <typename K, typename V>
bool Table<K, V>::add(K key, V value) {
    if(this->count >= this->allocated * MAX_LOAD_FACTOR) { // @Temporary,
        bool success = this->reserve(this->allocated * 2);
        if(!success) return false; // Table was full, but we couldn't expand it.
    }

    unsigned int hash = get_hash(key);
    int slot = hash % this->allocated;

    int index = slot; // Not modifying slot for easy debugging purposes.
    bool looped = false;

    while(this->mask.data[index]) {
        if(hash == this->hashes.data[index]) {
            if(strcmp(key, this->keys.data[index])) { // @Incomplete, assuming char * keys
                // We already have this key, so we can't add it again.
                return false;
            }
        }

        index = index + 1;

        // Looping back to the beginning
        if(index == this->mask.allocated) {
            if(looped) {
                return false; // We failed to find an empty slot, which should never happen since we expanded it at the
                              // beginning or already failed.
            }

            looped = true;
            index = 0;
        }
    }

    this->keys.add_at_index(key, index);
    this->values.add_at_index(value, index);
    this->hashes.add_at_index(hash, index);
    this->mask.add_at_index(true, index);

    this->count = this->count + 1;

    return true;
}

template <typename K, typename V>
bool Table<K, V>::remove(K key) {
    if(!this->allocated) return false;

    unsigned int hash = get_hash(key);
    int slot = hash % this->allocated;

    int index = slot; // Not modifying slot for easy debugging purposes.
    bool looped = false;

    while(this->mask.data[index]) {
        if(hash == this->hashes.data[index]) {
            if(strcmp(key, this->keys.data[index]) == 0) { // @Incomplete, assuming char * keys
                this->mask.data[index] = false;
                return true;
            }
        }

        index = index + 1;

        // Looping back to the beginning
        if(index == this->mask.allocated) {
            if(looped) {
                return false;
            }
            looped = true;
            index = 0;
        }
    }

    return false;
}

template <typename K, typename V>
V Table<K, V>::find(K key) {
    if(!this->allocated) return NULL; // @Incomplete This assumes we're storing pointers, which we might not be. Maybe send the result back through an argument and return a success bool.

    unsigned int hash = get_hash(key);
    int slot = hash % this->allocated;

    int index = slot; // Not modifying slot for easy debugging purposes.
    bool looped = false;

    while(this->mask.data[index]) {
        if(hash == this->hashes.data[index]) {
            if(strcmp(key, this->keys.data[index]) == 0) { // @Incomplete, assuming char * keys
                return this->values.data[index];
            }
        }

        index = index + 1;

        // Looping back to the beginning
        if(index == this->mask.allocated) {
            if(looped) { // @Incomplete If we loop, we don't have to wait to reach the end again to quit.
                return NULL;
            }
            looped = true;
            index = 0;
        }
    }

    return NULL;
}
