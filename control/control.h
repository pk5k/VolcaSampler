#ifndef CONTROL_H_INC
#define CONTROL_H_INC

#include "../Adafruit_ADS1X15_RPi/Adafruit_ADS1015.h"

// control_id will be the handle to the given adc_channel on the adc_instance
int setup_control(int adc_channel, Adafruit_ADS1115* adc_instance);

// get the selected sample-number on this control (0-9)
int control_current_state(int control_id);

#endif