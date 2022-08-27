#ifndef REDO_TRIGGER_H_INC
#define REDO_TRIGGER_H_INC

#define TRIGGER_COOLDOWN 1000
#define TRIGGER_RESERVE_AMOUNT 10

#define TRIGGER_TYPE_LOW 1 // fire callback on low pin input (opener)
#define TRIGGER_TYPE_HIGH 2 // fire callback on high pin input (closer)

/* additional config to fire hold callback if button is still hold after cooldown: 
	- if released while cooldown the regular callback will be fired, 
	- if released after the hold callback was fired, the trigger callback will be fired afterwards
*/
#define TRIGGER_TYPE_HOLD 4 

// if this flag is set, the trigger callback won't be called if the hold callback was called
#define TRIGGER_TYPE_NO_TRIGGER_AFTER_HOLD 8


#define TRIGGER_STATE_NONE 1
#define TRIGGER_STATE_WAIT_FOR_RESOLVE 2
#define TRIGGER_STATE_HOLD_ACTIVE 3

int setup_trigger(int input_pin, int type, void (*_trig_callback)(), void (*_hold_callback)() = NULL);
bool update_trigger_state(int for_trigger);

#endif