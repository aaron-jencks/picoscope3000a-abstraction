#include "setup.h"

#include <math.h>
#include <stdbool.h>

/**
 * initializes an oscilloscope for use given the config
 * 
 * populates the scope field of the config with the id of the opened scope
 */
PICO_STATUS oscilloscope_setup_all(oscilloscope_context_t* config) {
    PICO_STATUS result = ps3000aOpenUnit(&config->scope, config->serial_number);
    if(result != PICO_OK) return result;
    result = oscilloscope_setup_channels(config);
    if(result != PICO_OK) return result;
    return oscilloscope_setup_trigger(config);
}

/**
 * tears down the given oscilloscope
 */
PICO_STATUS oscilloscope_teardown(oscilloscope_context_t* config) {
    ps3000aCloseUnit(config->scope);
}

PICO_STATUS disable_channel(int16_t scope, PS3000A_CHANNEL channel) {
    return ps3000aSetChannel(
        scope, channel, false,
        PS3000A_DC, PS3000A_5V, 0.0
    );
}

/**
 * after opening the oscilloscope, this function can be used to initialize the channels of the scope
 */
PICO_STATUS oscilloscope_setup_channels(oscilloscope_context_t* config) {
    PICO_STATUS result;
    result = disable_channel(config->scope, PS3000A_CHANNEL_A);
    if(result != PICO_OK) return result;
    result = disable_channel(config->scope, PS3000A_CHANNEL_B);
    if(result != PICO_OK) return result;
    result = disable_channel(config->scope, PS3000A_CHANNEL_C);
    if(result != PICO_OK) return result;
    result = disable_channel(config->scope, PS3000A_CHANNEL_D);
    if(result != PICO_OK) return result;
    for(size_t i = 0; i < config->sampling_channel_count; i++) {
        oscilloscope_channel_context_t chan = config->sampling_channels[i];
        result = ps3000aSetChannel(
            config->scope, chan.channel, true,
            chan.coupling, chan.range, chan.analog_offset
        );
        if(result != PICO_OK) return result;
    }
    if(config->trigger_context != NULL) {
        oscilloscope_channel_context_t chan = config->trigger_context->channel;
        return ps3000aSetChannel(
            config->scope, chan.channel, true,
            chan.coupling, chan.range, chan.analog_offset
        );
    }
    return PICO_OK;
}

static inline double ps3000a_range_fullscale_volts(PS3000A_RANGE r) {
  switch (r) {
    case PS3000A_10MV: return 0.010;
    case PS3000A_20MV: return 0.020;
    case PS3000A_50MV: return 0.050;
    case PS3000A_100MV: return 0.100;
    case PS3000A_200MV: return 0.200;
    case PS3000A_500MV: return 0.500;
    case PS3000A_1V: return 1.0;
    case PS3000A_2V: return 2.0;
    case PS3000A_5V: return 5.0;
    case PS3000A_10V: return 10.0;
    case PS3000A_20V: return 20.0;
    // add other ranges your model supports
    default: return -1.0; // unknown
  }
}

static inline int16_t volts_to_adc(double volts,
                                   PS3000A_RANGE range,
                                   int16_t max_adc) {
  double fs = ps3000a_range_fullscale_volts(range);
  if (fs <= 0.0) return 0;

  // clamp input volts to range
  if (volts > fs) volts = fs;
  if (volts < -fs) volts = -fs;

  double code = (volts / fs) * (double)max_adc;

  // round to nearest int
  long v = (long)llround(code);

  if (v > max_adc) v = max_adc;
  if (v < -max_adc) v = -max_adc;

  return (int16_t)v;
}

/**
 * sets up the trigger for the oscilloscope after the unit has been opened and the channel have been setup.
 */
PICO_STATUS oscilloscope_setup_trigger(oscilloscope_context_t* config) {
    if(config->trigger_context == NULL) return ps3000aSetSimpleTrigger(
        config->scope, false,
        PS3000A_CHANNEL_A, 0,
        0, 0, 0
    );

    int16_t max_adc;
    PICO_STATUS result = ps3000aMaximumValue(config->scope, &max_adc);
    if(result != PICO_OK) return result;

    oscilloscope_trigger_context_t* trigger_config = config->trigger_context;

    int16_t adc_threshold = volts_to_adc(
        trigger_config->threshold / trigger_config->channel.attenuation, 
        trigger_config->channel.range, 
        max_adc
    );

    return ps3000aSetSimpleTrigger(
        config->scope, true,
        trigger_config->channel.channel, adc_threshold,
        trigger_config->mode, trigger_config->delay, trigger_config->autotrigger_delay
    );
}
