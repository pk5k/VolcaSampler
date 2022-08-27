#ifndef LED_H_INC
#define LED_H_INC

#define LED_RESERVE_AMOUNT 10
#define LED_CYCLE_INTERVAL_CHECK_TIME 5 // check time after this amount of loops
#define LED_CYCLE_INTERVAL_MS 50 // update LEDs all 50ms

int setup_led(int input_pin);
bool light_led(int which_led, int duration_ms, int start_delay_ms = 0, int end_delay_ms = 0, int repeat = 0);
void set_led_state(int for_led, bool enabled);
bool update_led_states();
bool quit_led_instructions(int for_led);
bool has_active_instructions(int for_led);

#endif