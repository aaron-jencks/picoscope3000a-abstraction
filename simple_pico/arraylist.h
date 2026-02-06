#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#include <stddef.h>

typedef struct {
    void* arr;
    size_t mem_size;
    size_t capacity;
    size_t size;
} arraylist_t;

/**
 * creates a new arraylist with elements of the given size
 */
arraylist_t arraylist_create(size_t initial_capacity, size_t mem_size);
/**
 * de-allocates an arraylist
 */
void arraylist_destroy(arraylist_t arr);

/**
 * Adds a new element to the end of the arraylist
 */
size_t arraylist_append(arraylist_t arr, void* value);

#endif