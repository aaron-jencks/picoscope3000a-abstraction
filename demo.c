#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "simple_pico/data.h"
#include "simple_pico/error_handling.h"
#include "simple_pico/setup.h"

void handle_errors(PICO_STATUS result, bool teardown, oscilloscope_context_t* scope) {
    if(result != PICO_OK) {
        oscilloscope_error_info_t error_data = parse_picoscope_error(result);
        printf("An error occurred!\n%s\n", error_data.meaning);
        if(teardown) {
            oscilloscope_teardown(scope);
        }
        exit(1);
    }
}

int main() {
    PICO_STATUS result;
    oscilloscope_trigger_context_t trigger_config = {
        {
            PS3000A_CHANNEL_B, 
            PS3000A_DC, PS3000A_5V,
            0.0, 1.0
        },
        PS3000A_RISING,
        3.3, 0, 0
    };
    oscilloscope_channel_context_t sampling_channels[] = {
        {
            PS3000A_CHANNEL_A,
            PS3000A_DC, PS3000A_5V,
            0.0, 1.0
        }
    };
    oscilloscope_context_t config = {
        0, 0, NULL,
        sampling_channels,
        1,
        &trigger_config
    };
    result = oscilloscope_setup_all(&config);
    handle_errors(result, false, &config);

    oscilloscope_sampling_context_t sampling_config = {
        &sampling_channels[0],
        20000000,           // 20MHz
        1024 * 1024 * 1024, // 1MB
        false, 0, 0         // no triggers
    };

    oscilloscope_sampling_result_t sampling_result;

    result = oscilloscope_stream_data(&config, &sampling_config, 1000, &sampling_result);
    handle_errors(result, true, &config);

    oscilloscope_teardown(&config);
    return 0;
}
