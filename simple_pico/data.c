#include "data.h"

#include <time.h>


const clock_t CLOCKS_PER_MS = CLOCKS_PER_SEC * 1e-3;


/**
 * convert double sampling frequency into sample period
 */
static inline void convert_fs_to_ps(double sampling_frequency, uint32_t* period, PS3000A_TIME_UNITS* unit) {
    double period_sec = 1 / sampling_frequency;
    double multiplier[] = {
        1e-15, 1e-12, 1e-9, 1e-6, 1e-3, 1
    };
    for(PS3000A_TIME_UNITS i = PS3000A_FS; i < PS3000A_MAX_TIME_UNITS; i++) {
        double mult = multiplier[i];
        uint32_t period_int = (uint32_t)(period_sec / mult);
        if(period_int <= UINT32_MAX) {
            *period = period_int;
            *unit = i;
            break;
        }
    }
}

typedef struct {
    int16_t* buffer;
    oscilloscope_sampling_result_t* sample_result;
} callback_context_t;

/**
 * the oscilloscope callback function used to mask the callback function of the oscilloscope api
 */
void oscilloscope_callback_wrapper(int16_t handle, int32_t noSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autostop, void* parameter) {
    // buffer is a ring buffer, we need to handle that
    // noSamples may end up circling back to zero

}

/**
 * Samples data from the oscilloscope in streaming mode
 * Runs on the given scope and at the desired sample rate
 * Runs for duration ms and places the data in the result buffer
 * The function will allocate the buffer and arrays in the struct, do not allocate them yourself
 */
PICO_STATUS stream_data(oscilloscope_context_t* scope_config, oscilloscope_sampling_context_t* config, uint64_t duration, oscilloscope_sampling_result_t* result) {
    PICO_STATUS result_status;
    int16_t* buffer = (int16_t*)malloc(sizeof(int16_t) * config->buffer_size);
    if(!buffer) return PICO_MEMORY;
    result_status = ps3000aSetDataBuffer(scope_config->scope, config->channel, buffer, config->buffer_size, 0, PS3000A_RATIO_MODE_NONE);
    if(result_status != PICO_OK) return result_status;
    uint32_t sample_period;
    PS3000A_TIME_UNITS period_units;
    convert_fs_to_ps(config->sample_rate, &sample_period, &period_units);
    uint32_t pre_trigger = 0, post_trigger = 0;
    if(config->use_trigger) {
        pre_trigger = config->pre_trigger_samples;
        post_trigger = config->post_trigger_samples;
    }
    result_status = ps3000aRunStreaming(scope_config->scope, &sample_period, period_units, pre_trigger, post_trigger, false, 1, PS3000A_RATIO_MODE_NONE, config->buffer_size);
    if(result_status != PICO_OK) return result_status;

    // we need arraylists here...
    callback_context_t callback_ctx = {
        buffer,
        result
    };
    clock_t clocks_per_ms = CLOCKS_PER_SEC * 1e-3;
    clock_t start = clock();

    while(((clock() - start) / CLOCKS_PER_MS) < duration) {
        result_status = ps3000aGetStreamingLatestValues(scope_config->scope, oscilloscope_callback_wrapper, (void*)&callback_ctx);
        if(result_status != PICO_OK) return result_status;
    }

    // convert ints to voltages
}
