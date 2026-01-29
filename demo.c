#include <stdio.h>

#include "simple_pico/error_handling.h"
#include "simple_pico/setup.h"

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
        0, NULL,
        sampling_channels,
        1,
        &trigger_config
    };
    result = oscilloscope_setup_all(&config);
    if(result != PICO_OK) {
        oscilloscope_error_info_t error_data = parse_picoscope_error(result);
        printf("An error occurred!\n%s\n", error_data.meaning);
        return 1;
    } else {
        oscilloscope_teardown(&config);
        return 0;
    }
}
