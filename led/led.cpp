#include <stdio.h>
#include <bcm2835.h>
#include "led.h"
#include "../utils/time.h"

static int input_pins[LED_RESERVE_AMOUNT];
static int count = 0;
enum LED_PHASE{ BEGIN=0, DURATION=1, END=2 };

struct {
	bool started = false;
	bool setup = false;
	bool done = false;
	int repeat = 0;
	int c_rep = 0;
	uint64_t last_change = -1;
	int led;
	int duration_ms;
	int start_delay_ms;
	int end_delay_ms;
	int state = BEGIN;
} typedef LEDInstruction;

static LEDInstruction queue[LED_RESERVE_AMOUNT];
static int last_queue_index = -1;
static uint64_t next_cycle_time = 0;
static int cycle = 0;

int setup_led(int input_pin)
{
	count++;
	input_pins[count] = input_pin;
	bcm2835_gpio_fsel(input_pins[count], BCM2835_GPIO_FSEL_OUTP);// use pin as output

	return count;
}

bool light_led(int which_led, int duration_ms, int start_delay_ms, int end_delay_ms, int repeat)
{
	last_queue_index++;

	if (last_queue_index >= LED_RESERVE_AMOUNT)
	{
		last_queue_index = 0;
	}

	LEDInstruction* inst = &queue[last_queue_index];

	if (inst->started && !inst->done)
	{
		fprintf(stderr, "LED queue limit (%d) exceeded and next instructions %d not done yet.", LED_RESERVE_AMOUNT, last_queue_index);
		return false;
	}

	inst->started = false;
	inst->setup = true;
	inst->done = false;
	inst->led = which_led;
	inst->duration_ms = duration_ms;
	inst->start_delay_ms = start_delay_ms;
	inst->end_delay_ms = end_delay_ms;
	inst->repeat = repeat;// -1 = infinite
	inst->c_rep = 0;
	inst->state = BEGIN;

	return true;
}

void set_led_state(int for_led, bool enabled)
{
	if (!input_pins[for_led])
	{
		fprintf(stderr, "cannot update led %d. setup_led was not called.\n", for_led);
		return;
	}

	bcm2835_gpio_write(input_pins[for_led], (enabled) ? HIGH : LOW);
}

bool quit_led_instructions(int for_led)
{
	set_led_state(for_led, false);

	for (int i = 0; i < LED_RESERVE_AMOUNT; i++)
	{
		LEDInstruction* c = &queue[i];

		if (c->led == for_led && !c->done)
		{
			c->done = true;
		}
	}

	return true;
}

bool update_led_state(LEDInstruction* current)
{
	if (!current->started && !current->done && current->setup)
	{
		current->started = true;
		current->last_change = system_current_time_millis();
		current->state = BEGIN;
		set_led_state(current->led, false);
	}

	if (!current->started || current->done)
	{
		return true;
	}

	uint64_t c_ms = system_current_time_millis();

	switch (current->state)
	{
		case BEGIN:
			if (c_ms - current->last_change >= current->start_delay_ms)
			{
				current->state = DURATION;
				set_led_state(current->led, true);
				current->last_change = c_ms;
			}
			return true;

		case DURATION:
			if (c_ms - current->last_change >= current->duration_ms)
			{
				set_led_state(current->led, false);
				current->state = END;
				current->last_change = c_ms;
			}
			return true;

		case END:
			if (c_ms - current->last_change >= current->end_delay_ms)
			{
				current->last_change = c_ms;

				if ((current->repeat > 0 && current->c_rep < current->repeat) || current->repeat == -1)
				{
					current->started = false;
					current->c_rep++;
				}
				else 
				{
					current->done = true;
				}
			}
			return true;
	}

	return true;
}

bool update_led_states()
{
	cycle++;

	if (cycle < LED_CYCLE_INTERVAL_CHECK_TIME)
	{
		return false;
	}

	cycle = 0;
	
	if (next_cycle_time > system_current_time_millis())
	{
		return false;
	}
	
	next_cycle_time = system_current_time_millis() + LED_CYCLE_INTERVAL_MS;

	for (int i = 0; i < LED_RESERVE_AMOUNT; i++)
	{
		update_led_state(&queue[i]);
	}

	return true;
}

bool has_active_instructions(int for_led)
{
	for (int i = 0; i < LED_RESERVE_AMOUNT; i++)
	{
		LEDInstruction* c = &queue[i];

		if (c && c->led == for_led && c->started)
		{
			return true;
		}
	}

	return false;
}
