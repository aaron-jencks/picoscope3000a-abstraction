#ifndef DATA_H
#define DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <libps3000a/ps3000aApi.h>

#include "setup.h"

/**
 * TODO the samples and triggers likely need to be arraylists
 */
typedef struct {
    float* samples;             // the data samples collected
    size_t* triggers;           // the trigger indices
    size_t n_samples;           // the number of samples collected
    size_t n_triggers;          // the number of triggers recorded
    double actual_sample_rate;  // the actual sampling rate used by the oscilloscope while sampling
    PICO_STATUS status;         // the final result status
} oscilloscope_sampling_result_t;

typedef struct {
    PS3000A_CHANNEL channel;                    // the channel on the scope to sample
    double sample_rate;                         // the desired sample rate in samples/second
    size_t buffer_size;                         // max buffer size
    void(*callback)(int16_t*, size_t, void*);   // callback function for data (buffer, bufer_size, custom_parameter)
    bool use_trigger;                           // whether to use a trigger or not
    uint32_t pre_trigger_samples;               // number of samples to keep prior to each trigger
    uint32_t post_trigger_samples;              // number of samples to keep after each trigger
} oscilloscope_sampling_context_t;

/**
 * Samples data from the oscilloscope in streaming mode
 * Runs on the given scope and at the desired sample rate
 * Runs for duration ms and places the data in the result buffer
 * The function will allocate the buffer and arrays in the struct, do not allocate them yourself
 */
PICO_STATUS stream_data(oscilloscope_context_t* scope_config, oscilloscope_sampling_context_t* config, uint64_t duration, oscilloscope_sampling_result_t* result);

#endif