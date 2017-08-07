#pragma once

#include <assert.h>

#define JOIN2(a,b) a##_##b
#define JOIN(a,b) JOIN2(a,b)

// This macro loops over all the elements of an array and gives a pointer
// to the current value "it" and the current index "it_index". Very ugly,
// but this is about as good as it gets.

#define for_array(__for_array_data, __for_array_count)              \
    int JOIN(__for_array_internal,__LINE__) = 1;                    \
    for(int it_index = 0; it_index < __for_array_count; it_index++) \
        for(auto it = &__for_array_data[it_index];                  \
            JOIN(__for_array_internal,__LINE__)++ > 0;              \
            JOIN(__for_array_internal,__LINE__) = 0)                \

template <typename T>
struct Array {

    int count = 0, allocated = 0;

    T * data = NULL;

    bool add             (T item);
    bool add_at_index    (T item, int index);
    int  add_if_unique   (T item);

    int  remove          (T item);
    void remove_by_index (int index);

    int  find            (T item);

    void reset           ();
    void reset           (bool free_memory);

    bool reserve         (int size);
    bool reserve         (int size, bool zero);

};

// ************************ //
// ---- Implementation ---- //
// ************************ //
static const int MIN_SIZE_ARRAY = 8;

template <typename T>
bool Array<T>::add_at_index(T item, int index) {
    if (index >= this->allocated) {
        return false;
    }

    // We're copying the item, maybe simply assigning it is preferable. We'll see.
    memcpy(&this->data[index], &item, sizeof(T)); // @Warning potential overwrite.
    // this->data[this->count] = item;

    this->count += 1;

    return true;

}

template <typename T>
bool Array<T>::add(T item){
    if (this->count >= this->allocated) { // Should never be > unless someone messes up in theory but let's be safe.
        int new_size = this->allocated * 2; // Double the reserved memory size

        if(new_size < MIN_SIZE_ARRAY) {
            new_size = MIN_SIZE_ARRAY;
        }

        bool success = this->reserve(new_size);

        if (!success) return false; // We couldn't add the element.
    }

	// We're copying the item, maybe simply assigning it is preferable. We'll see.
    memcpy(&this->data[this->count], &item, sizeof(T));
	// this->data[this->count] = item;

    this->count += 1;

    return true;
}

template <typename T>
int Array<T>::add_if_unique(T item) {
    int index_in_array = this->find(item);

    // If it's already in the array, just return it current index;
    if (index_in_array >= 0) return index_in_array;

    // It wasn't in the array, let's add it.
    if(!this->add(item)) return -1; // Failed to add.

    return this->count - 1;
}

template <typename T>
void Array<T>::remove_by_index(int index) {
    int size = sizeof(T);

    this->count -= 1;
    memcpy(&this->data[index], &this->data[this->count], size);
    this->data[index] = this->data[this->count];
}

template <typename T>
int Array<T>::remove(T item) {
    int removed = 0;

    int size = sizeof(T);
    for_array(this->data, this->count) {
        if (memcmp( it, &item, size) == 0) {
            this->remove_by_index(it_index);

            removed += 1;
            it_index -= 1;
        }
    }

    return removed;
}

template <typename T>
int Array<T>::find(T item) {
    int size = sizeof(T);
    for_array(this->data, this->count) {
        if (memcmp(it, &item, size) == 0) {
            return it_index;
        }
    }

    return -1;
}

// We could make this one call the other reset with false, but this seems nicer
template <typename T>
void Array<T>::reset() {
    this->count = 0;
}
template <typename T>
void Array<T>::reset(bool free_memory) {
    this->count = 0;

    if(free_memory) {
        this->allocated = 0;
        free(this->data);
        this->data = NULL;
    }

}

template <typename T>
bool Array<T>::reserve(int to_reserve) {
    return this->reserve(to_reserve, false);
}

template <typename T>
bool Array<T>::reserve(int to_reserve, bool zero) {
    // Trying to allocate a size smaller than we currently have, so don't do anything.
    // Maybe we should allow this as long as size is greater than count. Let's return
    // true, since even if we did not allocate, everything is fine.
    if(to_reserve < this->allocated) return true;

    int size = sizeof(T);
    void * new_block = realloc(this->data, to_reserve * size); // @Incomplete we should allow a custom allocator in the future.

    assert(new_block);

    if (!new_block) return false; // We failed to allocate memory, let's let our caller know and deal with it.

    if(zero) {
        memset((char *)new_block + this->allocated, 0, to_reserve - this->allocated);
    }

    this->data = (T *) new_block;
    this->allocated = to_reserve;

    return true;
}
