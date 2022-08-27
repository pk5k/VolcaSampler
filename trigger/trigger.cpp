#include <stdio.h>
#include <bcm2835.h>
#include "trigger.h"
#include "../utils/time.h"

static void (*trig_callback[TRIGGER_RESERVE_AMOUNT])();
static void (*hold_callback[TRIGGER_RESERVE_AMOUNT])();

static uint64_t last_change[TRIGGER_RESERVE_AMOUNT];
static int input_pins[TRIGGER_RESERVE_AMOUNT];
static int trigger_type[TRIGGER_RESERVE_AMOUNT];
static int trigger_state[TRIGGER_RESERVE_AMOUNT];
static int count = 0;

int setup_trigger(int input_pin, int type, void (*_trig_callback)(), void (*_hold_callback)())
{
	count++;
	input_pins[count] = input_pin;
	bcm2835_gpio_fsel(input_pins[count], BCM2835_GPIO_FSEL_INPT);// use pin as input
	last_change[count] = system_current_time_millis();
	trig_callback[count] = _trig_callback;
	trigger_type[count] = type;
	trigger_state[count] = TRIGGER_STATE_NONE;

	if (!(type & TRIGGER_TYPE_HIGH) && !(type & TRIGGER_TYPE_LOW))
	{
		fprintf(stderr, "trigger type must either be %d (HIGH) or %d (LOW). Not %d", TRIGGER_TYPE_HIGH, TRIGGER_TYPE_LOW, type);

		return -1;
	}

	if (type & TRIGGER_TYPE_HOLD)
	{
		if (!_hold_callback)
		{
			fprintf(stderr, "trigger type %d (HOLD) was given but no hold_callback was specified. Add callback or remove hold flag.\n", TRIGGER_TYPE_HOLD);
			
			return -1;
		}

		hold_callback[count] = _hold_callback;
	}

	return count;
}

int current_pin_input(int for_trigger)
{
	if (!input_pins[for_trigger])
	{
		fprintf(stderr, "cannot read trigger %d. setup_trigger was not called.\n", for_trigger);
		return -1;
	}

	return bcm2835_gpio_lev(input_pins[for_trigger]);
}

bool update_trigger_state(int for_trigger)
{
	if (!trig_callback[for_trigger])
	{
		fprintf(stderr, "cannot update trigger state %d. setup_trigger was not called.\n", for_trigger);
		return false;
	}

	int current_feed = current_pin_input(for_trigger);

	if (current_feed < 0)
	{
		return false;
	}
//	printf("\n%d ----> %llu %s", current_feed, system_current_time_millis() - last_change, is_ing ? "true" : "false");

	int tt = trigger_type[for_trigger];
	int check_type = (tt & TRIGGER_TYPE_HIGH) ? HIGH : LOW;
	bool has_hold_type = (tt & TRIGGER_TYPE_HOLD);
	bool waits = (trigger_state[for_trigger] == TRIGGER_STATE_WAIT_FOR_RESOLVE);
	bool active_hold = (trigger_state[for_trigger] == TRIGGER_STATE_HOLD_ACTIVE);
	bool behind_cooldown = (system_current_time_millis() - last_change[for_trigger] > TRIGGER_COOLDOWN);
	bool trigger_is_active = (current_feed == check_type);

	if (trigger_is_active && behind_cooldown)
	{
		last_change[for_trigger] = system_current_time_millis();

		if (has_hold_type)
		{
			if (waits)
			{
				trigger_state[for_trigger] = TRIGGER_STATE_HOLD_ACTIVE;
				(hold_callback[for_trigger])();
			}
			else if (!active_hold)
			{
				trigger_state[for_trigger] = TRIGGER_STATE_WAIT_FOR_RESOLVE;
			}
		}
		else 
		{
			(trig_callback[for_trigger])();
		}
	}
	else if (!trigger_is_active && has_hold_type && ((waits && !behind_cooldown) || (active_hold && behind_cooldown)))
	{
		trigger_state[for_trigger] = TRIGGER_STATE_NONE;
		
		if (!(active_hold && tt & TRIGGER_TYPE_NO_TRIGGER_AFTER_HOLD))
		{
			(trig_callback[for_trigger])();
		}
	}

	return true;
}