#include "../Adafruit_ADS1X15_RPi/Adafruit_ADS1015.h"
#include "control.h"

static int control_adc_map[10];
static Adafruit_ADS1115* adc_ptr[10];
static int count = 0;

int setup_control(int adc_channel, Adafruit_ADS1115* adc_instance)
{
	count++;
	control_adc_map[count] = adc_channel;
	adc_ptr[count] = adc_instance;
	
	return count;	
}

int control_current_state(int control_id)
{
	if (!adc_ptr[control_id])
	{
		fprintf(stderr, "Cannot read current state of control %d. setup_control was not called.\n", control_id);
		return -1;
	}

	return adc_ptr[control_id]->readADC_SingleEnded(control_adc_map[control_id]);
}