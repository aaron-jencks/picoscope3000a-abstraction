#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <libps3000a/ps3000aApi.h>

typedef struct {
    PICO_STATUS status;
    char* meaning;
} oscilloscope_error_info_t;

/**
 * returns a more descriptive error from a list of known picoscope errors
 */
oscilloscope_error_info_t parse_picoscope_error(PICO_STATUS status);

#endif