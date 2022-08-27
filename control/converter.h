#ifndef CONTROL_CONVERTER_H_INC
#define CONTROL_CONVERTER_H_INC

// SAMPLE_CONTROL_STATES:
#define CSTATE_SAMPLE_0_LO 0
#define CSTATE_SAMPLE_0_HI 2500

#define CSTATE_SAMPLE_1_LO 2501
#define CSTATE_SAMPLE_1_HI 3500

#define CSTATE_SAMPLE_2_LO 3501
#define CSTATE_SAMPLE_2_HI 6000

#define CSTATE_SAMPLE_3_LO 6001
#define CSTATE_SAMPLE_3_HI 9500

#define CSTATE_SAMPLE_4_LO 9501
#define CSTATE_SAMPLE_4_HI 13000

#define CSTATE_SAMPLE_5_LO 13001
#define CSTATE_SAMPLE_5_HI 16000

#define CSTATE_SAMPLE_6_LO 16001
#define CSTATE_SAMPLE_6_HI 19000

#define CSTATE_SAMPLE_7_LO 19001
#define CSTATE_SAMPLE_7_HI 21000

#define CSTATE_SAMPLE_8_LO 21001
#define CSTATE_SAMPLE_8_HI 24000

#define CSTATE_SAMPLE_9_LO 24001
#define CSTATE_SAMPLE_9_HI 30000

// SPEEDUP_CONTROL_STATES:
#define CSTATE_SPEEDUP_X1_LO 0
#define CSTATE_SPEEDUP_X1_HI 3000
 
#define CSTATE_SPEEDUP_X2_LO 3001
#define CSTATE_SPEEDUP_X2_HI 7000

#define CSTATE_SPEEDUP_X3_LO 7001
#define CSTATE_SPEEDUP_X3_HI 32000

#define SPEEDUP_X1 1
#define SPEEDUP_X2 2
#define SPEEDUP_X3 3

// THRESHOLD LIMITS:
#define CSTATE_THRESHOLD_MAX 24000

#define CSTATE_THRESHOLD_INTERPOLATE_TO 6 //adc value of control will be interpolated from 0 to 12db where an adc value of CSTATE_THRESHOLD_MAX counts as maximum

int map_adc_to_sample_state(int which_value);
int current_sample_state(int control_id);

char* map_adc_to_speedup_state(int which_value);
char* current_speedup_state(int control_id);

char* map_adc_to_threshold_state(int which_value);
char* current_threshold_state(int control_id);

#endif // CONVERTER_H_INC