#include <stdlib.h>
#include "converter.h"
#include "control.h"

static char* last_speedup_alloc = NULL;
static char* last_threshold_alloc = NULL;

int map_adc_to_sample_state(int which_value)
{
	int base = (int)(which_value / 100) * 100;
//fprintf(stdout, "%d sample\n", base);
	if (base >= CSTATE_SAMPLE_0_LO && base <= CSTATE_SAMPLE_0_HI) return 0;
	if (base >= CSTATE_SAMPLE_1_LO && base <= CSTATE_SAMPLE_1_HI) return 1;
	if (base >= CSTATE_SAMPLE_2_LO && base <= CSTATE_SAMPLE_2_HI) return 2;
	if (base >= CSTATE_SAMPLE_3_LO && base <= CSTATE_SAMPLE_3_HI) return 3;
	if (base >= CSTATE_SAMPLE_4_LO && base <= CSTATE_SAMPLE_4_HI) return 4;
	if (base >= CSTATE_SAMPLE_5_LO && base <= CSTATE_SAMPLE_5_HI) return 5;
	if (base >= CSTATE_SAMPLE_6_LO && base <= CSTATE_SAMPLE_6_HI) return 6;
	if (base >= CSTATE_SAMPLE_7_LO && base <= CSTATE_SAMPLE_7_HI) return 7;
	if (base >= CSTATE_SAMPLE_8_LO && base <= CSTATE_SAMPLE_8_HI) return 8;
	if (base >= CSTATE_SAMPLE_9_LO && base <= CSTATE_SAMPLE_9_HI) return 9;

	return -1;
}

int current_sample_state(int control_id)
{
	return map_adc_to_sample_state(control_current_state(control_id));
}

char* map_adc_to_speedup_state(int which_value)
{
	int base = (int)(which_value / 100) * 100;
	int speed = -1;
//fprintf(stdout, "%d speed - %d / %d / %d\n", which_value, CSTATE_SPEEDUP_X1_HI, CSTATE_SPEEDUP_X2_HI, CSTATE_SPEEDUP_X3_HI);
	if (base >= CSTATE_SPEEDUP_X1_LO && base <= CSTATE_SPEEDUP_X1_HI) speed = SPEEDUP_X1;
	if (base >= CSTATE_SPEEDUP_X2_LO && base <= CSTATE_SPEEDUP_X2_HI) speed = SPEEDUP_X2;
	if (base >= CSTATE_SPEEDUP_X3_LO && base <= CSTATE_SPEEDUP_X3_HI) speed = SPEEDUP_X3;

	if (speed < 0)
	{
		fprintf(stderr, "base %d out of range.\n", base);
		return (char*)"-1";
	}

	//fprintf(stdout, "base %d", base);

	char* ret = (char*) malloc(sizeof(speed) + 1);
	sprintf(ret, "%d.0", speed);

	if (last_speedup_alloc)
	{
		free(last_speedup_alloc);
	}

	last_speedup_alloc = ret;

	return ret;
}

char* current_speedup_state(int control_id)
{
	return map_adc_to_speedup_state(control_current_state(control_id));
}

char* map_adc_to_threshold_state(int which_value)
{
	int base = (int)(which_value / 100) * 100;
	//fprintf(stdout, "%d thresh\n", which_value);
	if (base > CSTATE_THRESHOLD_MAX)
	{
		base = CSTATE_THRESHOLD_MAX;
	}

	double threshold_db = (double) CSTATE_THRESHOLD_INTERPOLATE_TO / CSTATE_THRESHOLD_MAX * base;
	
	char* ret = (char*) malloc(sizeof(base) + 1);
	sprintf(ret, "%.01f", threshold_db);

	if (last_threshold_alloc)
	{
		free(last_threshold_alloc);// free last allocd threshold state to avoid memory leak.
	}

	last_threshold_alloc = ret;

	return ret;
}

char* current_threshold_state(int control_id)
{
	return map_adc_to_threshold_state(control_current_state(control_id));
}
