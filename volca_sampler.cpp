#include "volca_sampler.h"
#include <stdio.h>
#include <stdbool.h>  
#include <stdlib.h>
#include <unistd.h>
#include <bcm2835.h>

#include "Adafruit_ADS1X15_RPi/Adafruit_ADS1015.h"

#include "control/control.h"
#include "control/converter.h"
#include "trigger/trigger.h"
#include "led/led.h"

#include "module/recorder.h"
#include "module/converter.h"
#include "module/player.h"
#include "module/editor.h"
#include "module/error.h"

#include "utils/store.h"

static Adafruit_ADS1115 ads;// ADC instance
static int program = PROG_IDLE;// program state
static int program_flags = 0;// influence program behaviour 
static int trigger_map[10];
static int ctrl_map[10];
static int led_map[10];
static char* export_path = NULL;
static bool verbose = false;

int current_sample_number()
{
	return current_sample_state(ctrl_map[CTRL_SAMPLE_0]) * 10 + current_sample_state(ctrl_map[CTRL_SAMPLE_1]);
}

/** MODULE CALLBACKS **/

// common error interrupt for all modules
void program_interrupted(int error)
{
	quit_led_instructions(led_map[LED_ERROR]);
	quit_led_instructions(led_map[LED_RECORD]);
	quit_led_instructions(led_map[LED_PROCESS]);
	quit_led_instructions(led_map[LED_TRANSFER]);

	if (error & MOD_ERR_CANCEL)
	{
		light_led(led_map[LED_RECORD], 250, 0, 500);
		light_led(led_map[LED_PROCESS], 250, 250, 250);
		light_led(led_map[LED_TRANSFER], 250, 500, 0);

		fprintf(stderr, "cancel occured. Program %d was canceled.\n", program);
	}
	else if (error & MOD_ERR_NODATA_THRESH)
	{
		light_led(led_map[LED_NO_DATA], 250, 0, 250, 4);
		light_led(led_map[LED_ERROR], 250, 250, 0, 4);

		fprintf(stderr, "no data, threshold is too high.\n");
	}
	else 
	{
		light_led(led_map[LED_ERROR], 500, 0, 100, 4);

		fprintf(stderr, "error %d occured. Program %d was stopped.\n", error, program);
	}
	
	program = PROG_IDLE;
}

void shutdown_trigger()
{
	fprintf(stdout, "shutdown initiated...\n");
	quit_led_instructions(led_map[LED_READY]);
}

void recording_started()
{
	program = PROG_RECORD;
	light_led(led_map[LED_RECORD], 250, 0, 250, -1);
}

void recording_finished()
{
	program = PROG_IDLE;

	quit_led_instructions(led_map[LED_RECORD]);

	editor_set_as_input(RECORDER_OUT);
	
	int es = editor_start(current_speedup_state(ctrl_map[CTRL_SPEEDUP]), current_threshold_state(ctrl_map[CTRL_THRESHOLD]));

	if (es != MOD_ERR_NONE)
	{
		program_interrupted(es);
	}
}

void converter_started()
{
	program = PROG_CONVERT;

	if (!has_active_instructions(led_map[LED_PROCESS]))
	{
		light_led(led_map[LED_PROCESS], 250, 0, 250, -1);
	}
}

void converter_finished()
{
	program = PROG_IDLE;
	
	quit_led_instructions(led_map[LED_PROCESS]);

	int ps = player_start();

	if (ps != MOD_ERR_NONE)
	{
		program_interrupted(ps);
	}
}

void player_started()
{
	program = PROG_TRANSFER;
	light_led(led_map[LED_TRANSFER], 250, 0, 250, -1);
}

void player_finished()
{	
	program = PROG_IDLE;
	quit_led_instructions(led_map[LED_TRANSFER]);

	bool has_se_flag = (program_flags & PFLAG_SUPPRESS_EXPORT);
	
	// only export successful transfers
	if (export_path && !has_se_flag)
	{
		store_sample_to_slot(export_path, EDITOR_IN, current_sample_number());
		// the sample number which is selected after transfer finished will be used as store filename 
		// -> change after "processing" task to store it on a different place than transfereed to volca sample
	}
	else if (has_se_flag)
	{
		program_flags -= PFLAG_SUPPRESS_EXPORT;
	}
}

void editor_started()
{
	program = PROG_EDIT;
	light_led(led_map[LED_PROCESS], 250, 0, 250, -1);
}

void editor_finished()
{
	program = PROG_IDLE;
	int cs = converter_start(current_sample_number());

	if (cs == MOD_ERR_NODATA_THRESH)
	{
		program_interrupted(cs);
	}
	else if (cs != 0)
	{
		program_interrupted(cs);
		fprintf(stderr, "converter start failed, maybe threshold was set too high.\n");
	}
}

/** TRIGGER CALLBACKS **/
void record_trigger()
{
	switch (program)
	{
		case PROG_IDLE:
			program = PROG_IDLE;
			start_recording();
			break;

		case PROG_RECORD:
			program = PROG_IDLE;
			stop_recording();
			break;

		case PROG_CANCEL_WAIT:
			int cs = converter_start(current_sample_number(), true);

			if (cs == MOD_ERR_NONE)
			{
				program_flags |= PFLAG_SUPPRESS_CANCEL;
			}
			else 
			{
				program_interrupted(cs);
			}
			break;
	}
}

void cancel_trigger()
{
	fprintf(stdout, "Cancel triggered.\n");

	if (program_flags & PFLAG_SUPPRESS_CANCEL)
	{
		fprintf(stdout, "Cancel suppressed.\n");
		program_flags -= PFLAG_SUPPRESS_CANCEL;
		return;
	}

	switch (program)
	{
		case PROG_RECORD:
			cancel_recording();
			break;

		case PROG_EDIT:
			cancel_editor();
			break;

		case PROG_CONVERT:
			cancel_converter();
			break;

		case PROG_TRANSFER:
			cancel_player();
			break;

		case PROG_CANCEL_WAIT:
			fprintf(stdout, "cancel hold ended without additional input.\n");
			program = PROG_IDLE;
			break;

		default:
			fprintf(stdout, "nothing to cancel.\n");
	}
}

void hold_cancel_trigger()
{
	fprintf(stdout, "Cancel hold was triggered.\n");
	program = PROG_CANCEL_WAIT;

	light_led(led_map[LED_RECORD], 250, 0, 100, 1);
	light_led(led_map[LED_PROCESS], 250, 0, 100, 1);
	light_led(led_map[LED_TRANSFER], 250, 0, 100, 1);
}

void redo_trigger()
{
	switch (program)
	{
		case PROG_IDLE: {
				fprintf(stdout, "Redo triggered.\n");
				// if a output file of recorder exists, act like it was recorded now and start editor
				FILE* out_file = fopen(EDITOR_IN, "r");

				if (!out_file)
				{
					fprintf(stdout, "%s does not exist - no redo will be executed.\n", EDITOR_IN);
					return;
				}

				fclose(out_file);

				program_flags += PFLAG_SUPPRESS_EXPORT;
				
				int es = editor_start(current_speedup_state(ctrl_map[CTRL_SPEEDUP]), current_threshold_state(ctrl_map[CTRL_THRESHOLD]));

				if (es != MOD_ERR_NONE)
				{
					program_interrupted(es);
				}
			}
			break;

		case PROG_RECORD: {
			int cr = cancel_recording();
			int sr = start_recording();

			if (cr != MOD_ERR_NONE || sr != MOD_ERR_NONE)
			{
				program_interrupted(cr | sr);
			}
		}
		break;

		case PROG_CANCEL_WAIT: {
			program = PROG_IDLE;
			program_flags |= PFLAG_SUPPRESS_CANCEL;

			if (load_sample_from_slot(export_path, EDITOR_IN, current_sample_number()))
			{
				light_led(led_map[LED_RECORD], 250, 0, 250, 1);
				light_led(led_map[LED_PROCESS], 250, 250, 0, 1);
			}
			else 
			{
				program_interrupted(MOD_ERR_UNKNOWN);
			}
		}
		break;
	}
	
}

// cycle helper

void update_triggers(int which)
{
	if (which & TRIG_RECORD)
	{
		update_trigger_state(trigger_map[TRIG_RECORD]);
	}

	if (which & TRIG_CANCEL) 
	{
		update_trigger_state(trigger_map[TRIG_CANCEL]);
	}

	if (which & TRIG_REDO)
	{
		update_trigger_state(trigger_map[TRIG_REDO]);
	}

	if (which & TRIG_SHUTDOWN)
	{
		update_trigger_state(trigger_map[TRIG_SHUTDOWN]);
	}
}

// main program loop cycle:
void cycle()
{
	switch (program)
	{
		case PROG_CONVERT:
			feed_converter_buffer();
			update_triggers(TRIG_CANCEL | TRIG_SHUTDOWN);
			break;

		case PROG_TRANSFER:
			feed_player_buffer();
			update_triggers(TRIG_CANCEL | TRIG_SHUTDOWN);
	    	break;

		case PROG_RECORD:
			feed_recorder_buffer();
			update_triggers(TRIG_RECORD | TRIG_CANCEL | TRIG_REDO | TRIG_SHUTDOWN);
			break;
		
		case PROG_EDIT:
			editor_process_chain();
			update_triggers(TRIG_CANCEL | TRIG_SHUTDOWN);
			break;

		case PROG_CANCEL_WAIT:
		case PROG_IDLE:
			update_triggers(TRIG_RECORD | TRIG_REDO | TRIG_CANCEL | TRIG_SHUTDOWN);

			if (verbose)
			{
    			printf("States: speedup = %s threshold = %s sample0 = %d sample1 = %d current number: %d\n", current_speedup_state(ctrl_map[CTRL_SPEEDUP]), current_threshold_state(ctrl_map[CTRL_THRESHOLD]), current_sample_state(ctrl_map[CTRL_SAMPLE_0]), current_sample_state(ctrl_map[CTRL_SAMPLE_1]), current_sample_number());
			}

		    usleep(10000);
			break;
	}

	if (update_led_states() && verbose)
	{
		fprintf(stdout, "LED states were updated.\n");
	}
}

/********************/

bool setup()
{
	int init_res = bcm2835_init();

	if (!init_res)
	{
		fprintf(stderr, "failed to init bcm2835\n");
		return false;
	}

	if (!bcm2835_i2c_begin())
	{
		fprintf(stderr, "unable to init i2c\n");
		return false;
	}

	// i2c0 P5 setup:
    bcm2835_gpio_fsel(0, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(1, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(28, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(29, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(28, BCM2835_GPIO_FSEL_ALT0);
    bcm2835_gpio_set_pud(28, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_fsel(29, BCM2835_GPIO_FSEL_ALT0);
    bcm2835_gpio_set_pud(29, BCM2835_GPIO_PUD_UP);
    //I2C Port 0 is now routed to P5 header and so to /dev/i2c-0

	led_map[LED_READY] = setup_led(RPI_GPIO_P1_23);
	led_map[LED_ERROR] = setup_led(RPI_GPIO_P1_26);
	led_map[LED_RECORD] = setup_led(RPI_GPIO_P1_24);
	led_map[LED_PROCESS] = setup_led(RPI_GPIO_P1_22);
	led_map[LED_TRANSFER] = setup_led(RPI_GPIO_P1_18);
	led_map[LED_NO_DATA] = setup_led(RPI_GPIO_P1_16);

	set_led_state(led_map[LED_READY], false);
	set_led_state(led_map[LED_ERROR], false);
	set_led_state(led_map[LED_RECORD], false);
	set_led_state(led_map[LED_PROCESS], false);
	set_led_state(led_map[LED_TRANSFER], false);
	set_led_state(led_map[LED_NO_DATA], false);

	// ADC init:
  	ads.setGain(GAIN_ONE);
  	ads.begin(!verbose);// begin(false) for dbg info, use i2c channel 0, 1 is required for wakeup/shutdown functionality

  	ctrl_map[CTRL_SAMPLE_0] = setup_control(0, &ads);
  	ctrl_map[CTRL_SAMPLE_1] = setup_control(1, &ads);
  	ctrl_map[CTRL_SPEEDUP] = setup_control(2, &ads);
  	ctrl_map[CTRL_THRESHOLD] = setup_control(3, &ads);

  	trigger_map[TRIG_RECORD] = setup_trigger(RPI_GPIO_P1_11, TRIGGER_TYPE_HIGH, &record_trigger);
  	trigger_map[TRIG_CANCEL] = setup_trigger(RPI_GPIO_P1_19, TRIGGER_TYPE_HIGH | TRIGGER_TYPE_HOLD, &cancel_trigger, &hold_cancel_trigger);
  	trigger_map[TRIG_REDO] = setup_trigger(RPI_GPIO_P1_21, TRIGGER_TYPE_HIGH, &redo_trigger);
  	trigger_map[TRIG_SHUTDOWN] = setup_trigger(RPI_GPIO_P1_05, TRIGGER_TYPE_LOW, &shutdown_trigger);

	setup_recorder(RECORDER_OUT, &recording_started, &recording_finished, &program_interrupted);
	setup_editor(EDITOR_IN, EDITOR_OUT, &editor_started, &editor_finished, &program_interrupted);
	setup_converter(EDITOR_OUT, CONVERTER_OUT, &converter_started, &converter_finished, &program_interrupted);
	setup_player(CONVERTER_OUT, &player_started, &player_finished, &program_interrupted);

	set_led_state(led_map[LED_READY], true);

	// read every control once (speedup results in x1 for the first sample processing after startup...)
	fprintf(stdout, "Volca Sampler started. Current control states:\nspeedup = %s threshold = %s current number: %d\n", 
		current_speedup_state(ctrl_map[CTRL_SPEEDUP]), current_threshold_state(ctrl_map[CTRL_THRESHOLD]), current_sample_number());

	return true; 
}

void cleanup()
{
	set_led_state(led_map[LED_READY], false);

	editor_cleanup();
	player_cleanup();
	recorder_cleanup();
	converter_cleanup();

	bcm2835_i2c_end();
	bcm2835_close();

	if (export_path)
	{
		free(export_path);
	}
}

bool read_args(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++)
	{
		const char* current_arg = argv[i];

		// -e export_path -> place where successfully sent sample files will be stored to and read from
		if (strcmp(current_arg, "-e") == 0 && i + 1 < argc)
		{
			export_path = (char*) malloc(strlen(argv[i + 1]));
			strcpy(export_path, argv[i + 1]);
		}

		// -v -> verbose mode
		if (strcmp(current_arg, "-v") == 0)
		{
			verbose = true;
		}
	}

	return true;
}
