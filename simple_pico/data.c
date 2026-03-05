#include "data.h"

#include "arraylist.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

float ADC_FLOAT_VALUES[] = {
    0.01, 0.02, 0.05, 
    0.1, 0.2, 0.5,
    1.0, 2.0, 5.0,
    10.0, 20.0, 50.0,
};

#define ADC_TO_FLOAT(s, ma, cr, off, att) ((((float)(s) / (float)(ma)) * (cr) + (off)) * (att))

float* convert_adc_to_float(int16_t* samples, size_t n_samples, int16_t max_adc, oscilloscope_channel_context_t channel) {
    float* result = (float*)malloc(sizeof(float)*n_samples);
    if(!result) {
        fprintf(stderr, "failed to allocate final float buffer\n");
        exit(1);
    }
    float range = ADC_FLOAT_VALUES[channel.range];
    for(size_t i = 0; i < n_samples; i++) {
        result[i] = ADC_TO_FLOAT(samples[i], max_adc, range, channel.analog_offset, channel.attenuation);
    }
    return result;
}

typedef struct {
    int16_t* buffer;
    size_t buffer_size;
    arraylist_t result_sample_buffer;
    arraylist_t result_trigger_index_buffer;
    oscilloscope_sampling_result_t* sample_result;
} callback_context_t;

static inline void initialize_sampling_result(oscilloscope_sampling_result_t* result) {
    if(!result) return;
    result->samples = NULL;
    result->triggers = NULL;
    result->n_samples = 0;
    result->n_triggers = 0;
    result->actual_sample_rate = 0.0;
    result->status = PICO_OK;
}

static inline void resolve_block_sample_counts(const oscilloscope_sampling_context_t* config, uint32_t* pre_trigger, uint32_t* post_trigger, uint32_t* total_samples) {
    *pre_trigger = 0;
    *post_trigger = 0;
    *total_samples = (uint32_t)config->buffer_size;

    if(!config->use_trigger) {
        *post_trigger = (uint32_t)config->buffer_size;
        return;
    }

    if(config->pre_trigger_samples > 0 || config->post_trigger_samples > 0) {
        *pre_trigger = config->pre_trigger_samples;
        *post_trigger = config->post_trigger_samples;
        *total_samples = config->pre_trigger_samples + config->post_trigger_samples;
        return;
    }

    *post_trigger = (uint32_t)config->buffer_size;
}

static PICO_STATUS find_block_timebase(int16_t scope, double requested_sample_rate, int32_t total_samples, uint32_t* timebase, float* time_interval_ns) {
    if(requested_sample_rate <= 0.0 || total_samples <= 0 || !timebase || !time_interval_ns) {
        return PICO_INVALID_PARAMETER;
    }

    const double target_interval_ns = 1e9 / requested_sample_rate;
    const uint32_t max_timebase_search = 100000;

    bool found_faster_timebase = false;
    uint32_t faster_timebase = 0;
    float faster_interval_ns = 0.0f;

    for(uint32_t candidate = 0; candidate <= max_timebase_search; candidate++) {
        float candidate_interval_ns = 0.0f;
        int32_t max_samples = 0;
        PICO_STATUS status = ps3000aGetTimebase2(
            scope, candidate, total_samples,
            &candidate_interval_ns, 0, &max_samples, 0
        );

        if(status == PICO_INVALID_TIMEBASE) continue;
        if(status != PICO_OK) return status;
        if(max_samples < total_samples) continue;

        if((double)candidate_interval_ns >= target_interval_ns) {
            *timebase = candidate;
            *time_interval_ns = candidate_interval_ns;
            return PICO_OK;
        }

        found_faster_timebase = true;
        faster_timebase = candidate;
        faster_interval_ns = candidate_interval_ns;
    }

    if(found_faster_timebase) {
        *timebase = faster_timebase;
        *time_interval_ns = faster_interval_ns;
        return PICO_OK;
    }

    return PICO_INVALID_TIMEBASE;
}

static PICO_STATUS wait_for_block_ready(int16_t scope, uint64_t timeout_ms) {
    const struct timespec poll_sleep = { 0, 1000000L }; // 1ms
    const clock_t start = clock();

    while(true) {
        int16_t ready = 0;
        PICO_STATUS status = ps3000aIsReady(scope, &ready);
        if(status != PICO_OK) return status;
        if(ready) return PICO_OK;

        double elapsed_ms = ((double)(clock() - start)) / CLOCKS_PER_MS;
        if(elapsed_ms >= (double)timeout_ms) return PICO_TIMEOUT;
        nanosleep(&poll_sleep, NULL);
    }
}

/**
 * the oscilloscope callback function used to mask the callback function of the oscilloscope api
 */
void oscilloscope_callback_wrapper(int16_t handle, int32_t noSamples, uint32_t startIndex, int16_t overflow, uint32_t triggerAt, int16_t triggered, int16_t autostop, void* parameter) {
    (void)handle;
    (void)overflow;
    (void)autostop;
    callback_context_t* context = (callback_context_t*)parameter;
    if(noSamples <= 0 || context->buffer_size == 0) return;

    uint32_t current_index = startIndex % context->buffer_size;
    for(size_t i = 0; i < noSamples; i++) {
        arraylist_append(&context->result_sample_buffer, &context->buffer[current_index++]);
        // buffer is a ring buffer, we need to handle that
        // noSamples may end up circling back to zero
        if(current_index >= context->buffer_size) current_index = 0;
    }
    if(triggered) arraylist_append(&context->result_trigger_index_buffer, &triggerAt);
}

PICO_STATUS oscilloscope_collect_block(oscilloscope_context_t* scope_config, oscilloscope_sampling_context_t* config, oscilloscope_sampling_result_t* result) {
    initialize_sampling_result(result);
    if(!scope_config || !config || !config->channel || !result) return PICO_INVALID_PARAMETER;
    if(config->buffer_size == 0 || config->buffer_size > INT32_MAX) return PICO_INVALID_PARAMETER;

    PICO_STATUS status = PICO_OK;
    bool block_started = false;
    int16_t* adc_buffer = NULL;

    uint32_t pre_trigger = 0, post_trigger = 0, total_samples_u32 = 0;
    resolve_block_sample_counts(config, &pre_trigger, &post_trigger, &total_samples_u32);
    if(total_samples_u32 == 0 || total_samples_u32 > INT32_MAX) return PICO_INVALID_PARAMETER;

    uint32_t timebase = 0;
    float time_interval_ns = 0.0f;
    status = find_block_timebase(scope_config->scope, config->sample_rate, (int32_t)total_samples_u32, &timebase, &time_interval_ns);
    if(status != PICO_OK) return status;

    int32_t max_samples = 0;
    status = ps3000aMemorySegments(scope_config->scope, 1, &max_samples);
    if(status != PICO_OK) return status;
    if(max_samples < (int32_t)total_samples_u32) return PICO_TOO_MANY_SAMPLES;

    status = ps3000aSetNoOfCaptures(scope_config->scope, 1);
    if(status != PICO_OK) return status;

    adc_buffer = (int16_t*)malloc(sizeof(int16_t) * total_samples_u32);
    if(!adc_buffer) return PICO_MEMORY;

    status = ps3000aSetDataBuffer(
        scope_config->scope,
        config->channel->channel,
        adc_buffer,
        (int32_t)total_samples_u32,
        0,
        PS3000A_RATIO_MODE_NONE
    );
    if(status != PICO_OK) goto cleanup;

    status = ps3000aRunBlock(
        scope_config->scope,
        (int32_t)pre_trigger,
        (int32_t)post_trigger,
        timebase,
        0,
        NULL,
        0,
        NULL,
        NULL
    );
    if(status != PICO_OK) goto cleanup;
    block_started = true;

    status = wait_for_block_ready(scope_config->scope, 5000);
    if(status != PICO_OK) goto cleanup;

    uint32_t no_of_samples = total_samples_u32;
    int16_t overflow = 0;
    status = ps3000aGetValues(
        scope_config->scope,
        0,
        &no_of_samples,
        1,
        PS3000A_RATIO_MODE_NONE,
        0,
        &overflow
    );
    if(status != PICO_OK) goto cleanup;

    result->samples = convert_adc_to_float(adc_buffer, no_of_samples, scope_config->max_adc, *(config->channel));
    result->n_samples = no_of_samples;
    result->actual_sample_rate = (time_interval_ns > 0.0f) ? (1e9 / (double)time_interval_ns) : 0.0;
    result->n_triggers = 0;
    result->triggers = NULL;
    result->status = PICO_OK;

cleanup:
    if(block_started) {
        PICO_STATUS stop_status = ps3000aStop(scope_config->scope);
        if(status == PICO_OK && stop_status != PICO_OK) status = stop_status;
    }
    free(adc_buffer);
    result->status = status;
    return status;
}

PICO_STATUS oscilloscope_collect_rapid_block(oscilloscope_context_t* scope_config, oscilloscope_sampling_context_t* config, uint32_t captures, oscilloscope_sampling_result_t* result) {
    initialize_sampling_result(result);
    if(!scope_config || !config || !config->channel || !result) return PICO_INVALID_PARAMETER;
    if(captures == 0) return PICO_INVALID_PARAMETER;
    if(config->buffer_size == 0 || config->buffer_size > INT32_MAX) return PICO_INVALID_PARAMETER;

    PICO_STATUS status = PICO_OK;
    bool block_started = false;
    int16_t* adc_buffer = NULL;
    int16_t* overflow_flags = NULL;

    uint32_t pre_trigger = 0, post_trigger = 0, total_samples_u32 = 0;
    resolve_block_sample_counts(config, &pre_trigger, &post_trigger, &total_samples_u32);
    if(total_samples_u32 == 0 || total_samples_u32 > INT32_MAX) return PICO_INVALID_PARAMETER;

    size_t total_sample_count = (size_t)total_samples_u32 * (size_t)captures;
    if(captures > 0 && total_sample_count / captures != (size_t)total_samples_u32) return PICO_MEMORY;

    uint32_t timebase = 0;
    float time_interval_ns = 0.0f;
    status = find_block_timebase(scope_config->scope, config->sample_rate, (int32_t)total_samples_u32, &timebase, &time_interval_ns);
    if(status != PICO_OK) return status;

    int32_t max_samples_per_segment = 0;
    status = ps3000aMemorySegments(scope_config->scope, captures, &max_samples_per_segment);
    if(status != PICO_OK) return status;
    if(max_samples_per_segment < (int32_t)total_samples_u32) return PICO_TOO_MANY_SAMPLES;

    status = ps3000aSetNoOfCaptures(scope_config->scope, captures);
    if(status != PICO_OK) return status;

    adc_buffer = (int16_t*)malloc(sizeof(int16_t) * total_sample_count);
    overflow_flags = (int16_t*)malloc(sizeof(int16_t) * captures);
    if(!adc_buffer || !overflow_flags) {
        status = PICO_MEMORY;
        goto cleanup;
    }

    for(uint32_t capture_index = 0; capture_index < captures; capture_index++) {
        int16_t* capture_buffer = adc_buffer + ((size_t)capture_index * (size_t)total_samples_u32);
        status = ps3000aSetDataBuffer(
            scope_config->scope,
            config->channel->channel,
            capture_buffer,
            (int32_t)total_samples_u32,
            capture_index,
            PS3000A_RATIO_MODE_NONE
        );
        if(status != PICO_OK) goto cleanup;
    }

    status = ps3000aRunBlock(
        scope_config->scope,
        (int32_t)pre_trigger,
        (int32_t)post_trigger,
        timebase,
        0,
        NULL,
        0,
        NULL,
        NULL
    );
    if(status != PICO_OK) goto cleanup;
    block_started = true;

    status = wait_for_block_ready(scope_config->scope, 5000);
    if(status != PICO_OK) goto cleanup;

    uint32_t no_of_samples = total_samples_u32;
    status = ps3000aGetValuesBulk(
        scope_config->scope,
        &no_of_samples,
        0,
        captures - 1,
        1,
        PS3000A_RATIO_MODE_NONE,
        overflow_flags
    );
    if(status != PICO_OK) goto cleanup;

    size_t actual_total_samples = (size_t)no_of_samples * (size_t)captures;
    result->samples = convert_adc_to_float(adc_buffer, actual_total_samples, scope_config->max_adc, *(config->channel));
    result->n_samples = actual_total_samples;
    result->actual_sample_rate = (time_interval_ns > 0.0f) ? (1e9 / (double)time_interval_ns) : 0.0;
    result->n_triggers = 0;
    result->triggers = NULL;
    result->status = PICO_OK;

cleanup:
    if(block_started) {
        PICO_STATUS stop_status = ps3000aStop(scope_config->scope);
        if(status == PICO_OK && stop_status != PICO_OK) status = stop_status;
    }

    {
        int32_t ignored_max = 0;
        (void)ps3000aSetNoOfCaptures(scope_config->scope, 1);
        (void)ps3000aMemorySegments(scope_config->scope, 1, &ignored_max);
    }

    free(overflow_flags);
    free(adc_buffer);
    result->status = status;
    return status;
}

/**
 * Samples data from the oscilloscope in streaming mode
 * Runs on the given scope and at the desired sample rate
 * Runs for duration ms and places the data in the result buffer
 * The function will allocate the buffer and arrays in the struct, do not allocate them yourself
 */
PICO_STATUS oscilloscope_stream_data(oscilloscope_context_t* scope_config, oscilloscope_sampling_context_t* config, uint64_t duration, oscilloscope_sampling_result_t* result) {
    PICO_STATUS result_status = PICO_OK;
    PICO_STATUS stop_status = PICO_OK;
    bool streaming_started = false;
    int16_t* buffer = (int16_t*)malloc(sizeof(int16_t) * config->buffer_size);
    if(!buffer) return PICO_MEMORY;
    result_status = ps3000aSetDataBuffer(scope_config->scope, config->channel->channel, buffer, config->buffer_size, 0, PS3000A_RATIO_MODE_NONE);
    if(result_status != PICO_OK) goto cleanup;
    uint32_t sample_period;
    PS3000A_TIME_UNITS period_units;
    convert_fs_to_ps(config->sample_rate, &sample_period, &period_units);
    uint32_t pre_trigger = 0, post_trigger = 0;
    if(config->use_trigger) {
        pre_trigger = config->pre_trigger_samples;
        post_trigger = config->post_trigger_samples;
    }
    result_status = ps3000aRunStreaming(scope_config->scope, &sample_period, period_units, pre_trigger, post_trigger, false, 1, PS3000A_RATIO_MODE_NONE, config->buffer_size);
    if(result_status != PICO_OK) goto cleanup;
    streaming_started = true;

    // we need arraylists here...
    callback_context_t callback_ctx = {
        buffer,
        config->buffer_size,
        arraylist_create(config->buffer_size, sizeof(int16_t)),
        arraylist_create(config->buffer_size, sizeof(uint32_t)),
        result
    };
    clock_t start = clock();

    while((((double)(clock() - start)) / CLOCKS_PER_MS) < (double)duration) {
        result_status = ps3000aGetStreamingLatestValues(scope_config->scope, oscilloscope_callback_wrapper, (void*)&callback_ctx);
        if(result_status != PICO_OK) goto cleanup_with_lists;
    }
    stop_status = ps3000aStop(scope_config->scope);
    streaming_started = false;
    if(stop_status != PICO_OK) {
        result_status = stop_status;
        goto cleanup_with_lists;
    }

    // convert ints to voltages
    result->samples = convert_adc_to_float(
        callback_ctx.result_sample_buffer.arr,
        callback_ctx.result_sample_buffer.size, 
        scope_config->max_adc,
        *(config->channel)
    );
    result->n_samples = callback_ctx.result_sample_buffer.size;
    result->n_triggers = callback_ctx.result_trigger_index_buffer.size;
    result->triggers = callback_ctx.result_trigger_index_buffer.arr;
    result->actual_sample_rate = 1.0 / ((double)sample_period * pow(10.0, (double)period_units * 3.0 - 15.0));
    result->status = PICO_OK;
    result_status = PICO_OK;

cleanup_with_lists:
    if(streaming_started) {
        stop_status = ps3000aStop(scope_config->scope);
        if(result_status == PICO_OK && stop_status != PICO_OK) {
            result_status = stop_status;
        }
    }
    arraylist_destroy(&callback_ctx.result_sample_buffer);
    if(result_status != PICO_OK) {
        arraylist_destroy(&callback_ctx.result_trigger_index_buffer);
    }

cleanup:
    free(buffer);
    return result_status;
}
