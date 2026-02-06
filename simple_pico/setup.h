#ifndef SETUP_H
#define SETUP_H

#include <stddef.h>
#include <stdint.h>

#include <libps3000a/ps3000aApi.h>

/**
 * Represents a single sampling channel on the oscilloscope
 */
typedef struct {
    PS3000A_CHANNEL channel;    // the oscilloscope channel
    PS3000A_COUPLING coupling;  // the type of coupling to use
    PS3000A_RANGE range;        // the voltage range of the channel
    float analog_offset;        // the analog offset to use for the channel
    float attenuation;          // attentuation multiplier (ie. 10x, 1x, etc...)
} oscilloscope_channel_context_t;

/**
 * Represents triggering information used to setup the trigger for the oscilloscope context
 */
typedef struct {
    oscilloscope_channel_context_t channel; // channel to use for the trigger
    PS3000A_THRESHOLD_DIRECTION mode;       // the triggering mode for the trigger
    float threshold;                        // the voltage at which the trigger will fire, this is before attentuation, internally, this voltage is divided by attenuation before usage, do not pre-divide it.
    uint32_t delay;                         // delay in samples before the first sample is collected after a trigger occurs
    int16_t autotrigger_delay;              // delay in ms to wait before auto-triggering when no trigger is detected, 0 mean to wait indefinitely
} oscilloscope_trigger_context_t;

/**
 * Represents the context for the oscilloscope to define things like channels and triggering information
 */
typedef struct {
    int16_t scope;                                      // the oscilloscope identifier
    int16_t  max_adc;                                   // the max adc value for the scope
    int8_t* serial_number;                              // if not-null, tries to open a specific scope
    oscilloscope_channel_context_t* sampling_channels;  // list of active channels for sampling
    size_t sampling_channel_count;                      // number of sampling channels
    oscilloscope_trigger_context_t* trigger_context;    // the trigger config, if NULL then no trigger is used
} oscilloscope_context_t;


/**
 * initializes an oscilloscope for use given the config
 * 
 * populates the scope field of the config with the id of the opened scope
 */
PICO_STATUS oscilloscope_setup_all(oscilloscope_context_t* config);

/**
 * tears down the given oscilloscope
 */
PICO_STATUS oscilloscope_teardown(oscilloscope_context_t* config);

/**
 * after opening the oscilloscope, this function can be used to initialize the channels of the scope
 */
PICO_STATUS oscilloscope_setup_channels(oscilloscope_context_t* config);

/**
 * sets up the trigger for the oscilloscope after the unit has been opened and the channel have been setup.
 */
PICO_STATUS oscilloscope_setup_trigger(oscilloscope_context_t* config);

#endif