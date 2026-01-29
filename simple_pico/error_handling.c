#include "error_handling.h"

char* PICO_STATUS_MEANING[] = {
    "The PicoScope is functioning correctly",
    "An attempt has been made to open more than PS3000A_MAX_UNITS",
    "Not enough memory could be allocated on the host machine",
    "No PicoScope could be found",
    "Unable to download firmware",
    "Still attempting to open the scope",
    "The operation failed without a clear error",
    "The PicoScope is not responding to commands from the PC",
    "The confirguration information in the PicoScope has become corrupt or is missing",
    "The picopp.sys file is too old to be used with the device driver",
    "The EEPROM has become corrupt, so the device will use a default setting",
    "The operating system on the PC is not supported by this driver",
    "There is no device with the handle value passed",
    "A parameter value is not valid",
    "The timebase is not supported or is invalid",
    "The voltage range is not supported or is invalid",
    "The channel number is not valid on this device or no channels have been set",
    "The channel set fcor a trigger is not available on this device",
    "The channel set for a condition is not available on this device",
    "the device does not have a signal generator",
    "Streaming has failed to start or has stopped without user request",
    "Block failed to start - a parameter may have been set wrongly",
    "A parameter that was required is NULL",
    "No data is available from a run block call",
    "The buffer passed for the information was too small",
    "ETS is not supported on this device",
    "The auto trigger time is less than the time it will take to collect the pre-trigger data",
    "The collection of data has stalled as unread data would be overwritten",
    "The number of samples requested is more than available in the current memory segment",
    "Not possible to create number of segments requested",
    "A null pointer has been passed in the trigger function or one of the parameters is out of range",
    "One or more of the hold-off parameters are out of range",
    "One or more of the source details are incorrect",
    "One or more of the conditions are incorrect",
    "The driver's thread is currently in the ps3000a...Ready callback function and therefore the action cannot be carried out",
    "An attempt is being made to get stored data while streaming. Either stop streaming, or use getLatestValues",
    "There are no samples available because the run has not been completed",
    "The memory index is out of range",
    "The device is busy",
    "The start time to get stored data is out of range",
    "The information number requested is not a valid number",
    "The handle is invalid, so no information is available about the device",
    "The sample interval selected for streaming is out of range",
    "There was an error with the trigger",
    "Driver cannot allocate memory",
    "Incorrect parameter passed to the signal generator",
    "Conflict between the shots and sweeps parameters sent to the signal generator",
    "", "", "", // unused codes
    "Attempt to set different EXT input thresholds for signal generator and oscilloscope trigger",
    "The combined peak to peak voltage and the analog offset voltage exceed the allowable voltage the signal generator can produce",
    "NULL pointer passed as delay parameter",
    "The buffers for overview data have not been set while streaming",
    "The analog offset voltage is out of range",
    "The analog peak voltage is out of range",
    "A block collection has been cancelled",
    "The segment index is not currently being used",
    "The wrong GetValues function has been called for the collection mode in use",
    "The function is not available",
    "The aggregation ratio requested is out of range",
    "The oscilloscope is in an invalid state",
    "The number of segments allocated is fewer than the number of captures requested",
    "You called a driver function while another driver function was still being processed",
    "Reserved",
    "An invalid coupling type was specified",
    "An attempt was made to get data before a data buffer was defined",
    "The selected downsampling mode is not allowed",
    "An invalid paramter was passed to the trigger",
    "The driver was unable to contact the oscilloscope",
    "An error occured while setting up the signal generator",
    "There was an error with the FPGA",
    "Power Manager error",
    "An impossible analog offset value was specified",
    "Unable to configure the PicoScope due to a lock issue",
    "The oscilloscope's analog board is not detected, or is not connected to the digigtal board",
    "Unable to configure the signal generator",
    "The FPGA cannot be initialized, so unit cannot be opened",
    "The frequency for the external clock is not with 5% of the stated value",
    "The FPGA could not lock the clock signal",
    "You are trying to configure the AUX input as both a trigger and a reference clock",
    "You are trying to configure the AUX input as both a pulse width qualifier and a reference clock",
    "The scaling file set cannot be opened",
    "The frequency of the memory is reporting incorrectly",
    "The i2C that is being actioned is not responding to requests",
    "There are no captures available and therefore no data can be returned",
    "The capture mode the device is currently running in does not support the current request",
    "", // unused codes, 5f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // 60-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // 70-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // 80-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // 90-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // a0-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // b0-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // c0-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // d0-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // e0-f
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", // f0-f
    "", "", "", // 100-102
    "Reserved",
    "The device is not currently connected via the IP Network socket and thus the call made is not supported",
    "An IP address that is not correct has been passed to the driver",
    "The IP socket has failed",
    "The IP socket has timed out",
    "The settings requested have failed to be set",
    "The network connection has failed",
    "Unable to load the WS2 dll",
    "The IP port is invalid",
    "The type of coupling requested is not supported on the opened device",
    "Bandwidth limit is not supported on the opened device",
    "The value requested for the bandwidth limit is out of range",
    "The arbitrary waveform generator is not supported by the opened device",
    "Data has been requested with the ETS mode set, but run block has not been called, or stop has been called",
    "White noise is not supported on the opened device",
    "The wave type requested is not supported by the opened device",
    "A port number that does not evaluate to either PS3000A_DIGITAL_PORT0 or PS3000A_DIGITAL_PORT1, the ports that are supported",
    "The digital channel is not in the range PS3000A_DIGITAL_CHANNEL0 to PS3000_DIGITAL_CHANNEL15, the digital channels that are supported",
    "The digital trigger direction is not a valid trigger direction",
    "The signal generator does not support pseudo-random bit stream",
    "When a digital port is enabled, ETS sample mode is not available for use",
    "Not applicable for this device",
    "4-Channel only - The DC power supply is connected",
    "4-channel only - The DC power supply is not connected",
    "Incorrect power mode passed for current power source",
    "The supply voltage from the USB source is too low",
    "The oscilloscope is in the process of capturing data",
    "A USB3.0 device is connected to a non-USB3.0 port"
};

/**
 * returns a more descriptive error from a list of known picoscope errors
 */
oscilloscope_error_info_t parse_picoscope_error(PICO_STATUS status) {
    oscilloscope_error_info_t result = {
        status, PICO_STATUS_MEANING[status]
    };
    return result;
}