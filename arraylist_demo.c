#include <stdint.h>
#include <stdio.h>

#include "simple_pico/arraylist.h"


int main() {
    arraylist_t arr = arraylist_create(1024*1024, sizeof(int16_t));
    int16_t value = 0;
    arraylist_append(&arr, &value);
    arraylist_destroy(&arr);
}
