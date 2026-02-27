#include "arraylist.h"
#include "error_handling.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libps3000a/ps3000aApi.h>

/**
 * creates a new arraylist with elements of the given size
 */
arraylist_t arraylist_create(size_t initial_capacity, size_t mem_size) {
    arraylist_t result = {
        malloc(initial_capacity * mem_size),
        mem_size,
        initial_capacity,
        0
    };
    if(!result.arr) {
        fprintf(stderr, "failed to allocate arraylist\n");
        exit(1);
    }
    return result;
}
/**
 * de-allocates an arraylist
 */
void arraylist_destroy(arraylist_t* arr) {
    if(!arr) return;
    free(arr->arr);
    arr->arr = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

/**
 * Adds a new element to the end of the arraylist
 */
size_t arraylist_append(arraylist_t* arr, const void* value) {
    if(!arr || !value) {
        fprintf(stderr, "invalid arraylist append arguments\n");
        exit(1);
    }
    if(arr->size == arr->capacity) {
        arr->capacity <<= 1;
        arr->arr = realloc(arr->arr, arr->capacity * arr->mem_size);
        if(!arr->arr) {
            fprintf(stderr, "failed to re-allocate arraylist\n");
            exit(1);
        }
    }
    memcpy((char*)arr->arr + arr->size * arr->mem_size, value, arr->mem_size);
    return arr->size++;
}
