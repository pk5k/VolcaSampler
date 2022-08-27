#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sox.h>
#include "editor.h"
#include "error.h"

static void (*editor_started)();
static void (*editor_finished)();
static void (*editor_interrupted)(int);

static sox_format_t* in = NULL;
static sox_format_t* out = NULL;
static sox_signalinfo_t interm_signal;

static sox_effects_chain_t* fx_chain = NULL;
static sox_effect_t* fx;
static char * args[10];

static char* source_file = NULL;
static char* target_file = NULL;

void setup_editor(const char* _source_file, const char* _target_file, void (*_editor_started)(), void (*_editor_finished)(), void (*_editor_interrupted)(int))
{
	editor_started = _editor_started;
	editor_finished = _editor_finished;
	editor_interrupted = _editor_interrupted;
	
	source_file = (char*) malloc(sizeof(_source_file));
	target_file = (char*) malloc(sizeof(_target_file));

	strcpy(source_file, _source_file);
	strcpy(target_file, _target_file);
}

int add_effect(const char* fx_name, char* args[], int argc)
{
	if (sox_effect_options(fx, argc, args) != SOX_SUCCESS)
	{
		fprintf(stderr, "Failed to setup sox input effect.\n");
		
		return MOD_ERR_UNKNOWN;
	}

	if (sox_add_effect(fx_chain, fx, &interm_signal, &out->signal) != SOX_SUCCESS)
	{
		fprintf(stderr, "Failed to add input effect to sox effect-chain.\n");

		return MOD_ERR_UNKNOWN;
	}

	free(fx);

	return MOD_ERR_NONE;
}

int editor_fx_chain(char* speedup_factor, char* threshold)
{
	if (in == NULL) 
	{
		fprintf(stderr, "Editor is not ready yet, cannot create effect-chain.\n");
		return MOD_ERR_UNKNOWN;
	}

	int res;
	// The first effect in the effect chain must be something that can source samples
	// -> built-in input handler (inputs data from an audio file)
	fx = sox_create_effect(sox_find_effect("input"));
	args[0] = (char *)in;
	res = add_effect("input", args, 1);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// Trim noisefloor at threshold (before speedup, otherwise duration configuration below would dependend on speedup setting)
	fx = sox_create_effect(sox_find_effect("silence"));
	args[0] = (char*)"1";// above periods = cut off beginning
	args[1] = (char*)"0.1";//seconds peak must be above threshold to count as "non silence"
	args[2] = threshold;
	res = add_effect("silence", args, 3);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// Reverse sample to cut off end with silence fx
	fx = sox_create_effect(sox_find_effect("reverse"));
	res = add_effect("reverse", NULL, 0);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// Trim noisefloor at threshold, this time for the end which acts now as beginning - settings are identical
	fx = sox_create_effect(sox_find_effect("silence"));
	args[0] = (char*)"1";// above periods = cut off beginning
	args[1] = (char*)"0.1";//seconds peak must be above threshold to count as "non silence"
	args[2] = threshold;
	res = add_effect("silence", args, 3);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// Reverse again to have sample in normal state again
	fx = sox_create_effect(sox_find_effect("reverse"));
	res = add_effect("reverse", NULL, 0);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// Speed-Up effect
	fx = sox_create_effect(sox_find_effect("speed"));
	args[0] = speedup_factor;
	res  = add_effect("speed", args, 1);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	 // Note: interm_signal.rate changed now, we need to rate it back

    fx = sox_create_effect(sox_find_effect("rate"));
    args[0] = (char*)"44100";

    res = add_effect("rate", args, 1);
    
	if (res != MOD_ERR_NONE)
	{
		return res;
	}    

	// normalize
	fx = sox_create_effect(sox_find_effect("gain"));
	args[0] = (char*) "-n";// normalize loudest peak to 0db
	res = add_effect("gain", args, 1);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	// The last effect in the effect chain must be something that only consumes samples 
	// -> built-in output handler (outputs data to an audio file)
	fx = sox_create_effect(sox_find_effect("output"));
	args[0] = (char *)out;
	res = add_effect("output", args, 1);

	if (res != MOD_ERR_NONE)
	{
		return res;
	}

	fprintf(stdout, "Editor effect-chain setup.\n");

	return MOD_ERR_NONE;
}

int editor_start(char* speedup_factor, char* threshold)
{
	if (source_file == NULL || target_file == NULL)
	{
		fprintf(stderr, "Cannot start editor. setup_editor was not called.");
		return MOD_ERR_UNKNOWN;
	}

	if (sox_init() != SOX_SUCCESS) 
	{
		fprintf(stderr, "Failed to initialize sox.\n");
		return MOD_ERR_UNKNOWN;
	}

	in = sox_open_read(source_file, NULL, NULL, NULL);
	fprintf(stdout, "input file %s opened.\n", source_file);

	out = sox_open_write(target_file, &in->signal, NULL, NULL, NULL, NULL);
	fprintf(stdout, "output file %s opened.\n", target_file);

	interm_signal = in->signal;

	fx_chain = sox_create_effects_chain(&in->encoding, &out->encoding);

	fprintf(stdout, "Editor ready.\n");

	if (editor_fx_chain(speedup_factor, threshold) != MOD_ERR_NONE)
	{
		return MOD_ERR_UNKNOWN;
	}
	
	editor_started();

	return MOD_ERR_NONE;
}

int editor_process_chain()
{
	if (fx_chain == NULL)
	{
		fprintf(stderr, "No effect-chain to process.\n");

		cancel_editor();
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "Processing...\n");

	// Flow samples through the effects processing chain until EOF is reached
	sox_flow_effects(fx_chain, NULL, NULL);

	fprintf(stdout, "...Done\n");

	editor_stop();

	return MOD_ERR_NONE;
}

int teardown_editor()
{
	if (in == NULL || out == NULL)
	{
		fprintf(stderr, "Editor was not started.");
		return MOD_ERR_UNKNOWN;
	}

	if (fx_chain == NULL)
	{
		fprintf(stderr, "effect-chain is missing.");
		return MOD_ERR_UNKNOWN;
	}

	sox_delete_effects_chain(fx_chain);
	sox_close(out);
	sox_close(in);
	sox_quit();

	in = NULL;
	out = NULL;
	fx_chain = NULL;

	fprintf(stdout, "sox closed\n");
	fprintf(stdout, "Editor stopped.\n");

	return MOD_ERR_NONE;
}

int cancel_editor()
{
	int success = teardown_editor();

	editor_interrupted(MOD_ERR_CANCEL);
	return success;
}

int editor_stop()
{
	int success = teardown_editor();

	editor_finished();

	return success;
}

void editor_cleanup()
{
	free(source_file);
	free(target_file);
}

int editor_set_as_input(const char* filename)
{
	FILE* new_in = fopen(filename, "r");
	FILE* existing = fopen(source_file, "r");

	if (!new_in)
	{
		fprintf(stderr, "editor_set_as_input failed because input file %s does not exist\n", filename);
	}
	
	fclose(new_in);

	if (existing)
	{
		fclose(existing);

		if (remove(source_file) != 0)
		{
			fprintf(stderr, "could not remove existing file %s\n", source_file);
		}
	}

	int res = rename(filename, source_file);

	if (res != 0)
	{
		fprintf(stderr, "rename %s to %s failed\n", filename, source_file);

		return MOD_ERR_UNKNOWN;
	}

	return MOD_ERR_NONE;
}