#include <stdio.h>
#include <bcm2835.h>
#include "recorder.h"
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include "error.h"

static void (*record_started)();
static void (*record_finished)();
static void (*record_interrupted)(int);

static short* write_buffer;
static int write_buffer_frames = REC_BUFFER_FRAMES;
static int write_err;
static unsigned int write_rate = REC_SAMPLERATE;
static snd_pcm_t* capture_handle = NULL;
static snd_pcm_hw_params_t* hw_params;
static snd_pcm_format_t write_format = SND_PCM_FORMAT_S16_LE;

static SNDFILE* recorder_target = NULL;
static SF_INFO target_info;
static char* rec_target_path = NULL;

void setup_recorder(const char* _rec_target_path, void (*_record_started)(), void (*_record_finished)(), void (*_record_interrupted)(int))
{
	record_started = _record_started;// if recording has been started, this function will be called afterwards
	record_finished = _record_finished;// if recording has been stopped, this function will be called afterwards
	record_interrupted = _record_interrupted;// if recording has been canceled or an error occured after record_started was called, ...

	rec_target_path = (char*) malloc(sizeof(_rec_target_path));
	strcpy(rec_target_path, _rec_target_path);// wav file the data will be recorded to
}

int prepare_recording_target()
{
	if (!rec_target_path)
	{
		fprintf(stderr, "recording target not specified.\n");
		return MOD_ERR_UNKNOWN;
	}

	FILE* tmp_open = fopen(rec_target_path, "w");// w will erase all contents
	fclose(tmp_open);

	target_info.samplerate = REC_SAMPLERATE;
	target_info.channels = REC_CHANNELS;
    target_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	recorder_target = sf_open(rec_target_path, SFM_WRITE, &target_info);

	if (!recorder_target)
	{
		fprintf(stderr, "could not open recorder target.\n");
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "target file %s ready.\n", rec_target_path);

	return MOD_ERR_NONE;
}

int close_recording_target()
{
	if (!recorder_target)
	{
		fprintf(stderr, "Recording was not started. Cannot close recording target.\n");
		return MOD_ERR_UNKNOWN;
	}

	sf_close(recorder_target);
	fprintf(stdout, "target file closed.\n");
	
	return MOD_ERR_NONE;
}

int start_recording()
{
	if (prepare_recording_target() != 0)
	{
		fprintf(stderr, "Failed to prepare recording target.\n");
		return MOD_ERR_UNKNOWN;
	}

	if ((write_err = snd_pcm_open(&capture_handle, REC_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) 
	{
		fprintf(stderr, "cannot open audio device %s (%s)\n", REC_DEVICE, snd_strerror(write_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "audio interface opened\n");
		   
	if ((write_err = snd_pcm_hw_params_malloc(&hw_params)) < 0) 
	{
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(write_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params allocated\n");
				 
	if ((write_err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) 
	{
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params initialized\n");

	if ((write_err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
	{
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params access setted\n");

	if ((write_err = snd_pcm_hw_params_set_format(capture_handle, hw_params, write_format)) < 0) 
	{
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params format setted\n");

	if ((write_err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &write_rate, 0)) < 0) 
	{
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params rate setted\n");

	if ((write_err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, REC_CHANNELS)) < 0) 
	{
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(write_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params channels setted\n");

	if ((write_err = snd_pcm_hw_params (capture_handle, hw_params)) < 0) 
	{
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "hw_params setted\n");

	snd_pcm_hw_params_free(hw_params);

	fprintf(stdout, "hw_params freed\n");

	if ((write_err = snd_pcm_prepare (capture_handle)) < 0) 
	{
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(write_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "audio interface prepared\n");

	write_buffer = (short*) malloc(REC_BUFFER_FRAMES * snd_pcm_format_width(write_format) / 8);//*2?

	fprintf(stdout, "buffer allocated\n");
	fprintf(stdout, "recording sample to %s...\n", rec_target_path);

	record_started();
	
	return MOD_ERR_NONE;
}

int feed_recorder_buffer()
{
	if (capture_handle == NULL)
	{
		fprintf(stderr, "Recording is not started.\n");
		
		cancel_recording();
		return MOD_ERR_UNKNOWN;
	}

	if ((write_err = snd_pcm_readi(capture_handle, write_buffer, write_buffer_frames)) != write_buffer_frames) 
	{
		if (snd_pcm_recover(capture_handle, write_err, 1) < 0)
		{
  			fprintf(stderr, "read from audio interface failed (%d %s)\n", write_err, snd_strerror(write_err));

  			cancel_recording();
  			return MOD_ERR_UNKNOWN;
		}
	}

	int wrote = sf_write_short(recorder_target, write_buffer, write_buffer_frames);

	//fprintf(stdout, "wrote %d bytes\n", wrote);
	
	return MOD_ERR_NONE;
}

int teardown_recorder()
{
	if (capture_handle == NULL)
	{
		fprintf(stderr, "Recording is not started.\n");
		return MOD_ERR_UNKNOWN;
	}

	printf("...stopping recording.\n");

	close_recording_target();

	free(write_buffer);
	fprintf(stdout, "buffer freed\n");

	snd_pcm_close(capture_handle);
	capture_handle = NULL;

	fprintf(stdout, "audio interface closed\n");

	return MOD_ERR_NONE;
}

int cancel_recording()
{
	int success = teardown_recorder();
	record_interrupted(MOD_ERR_CANCEL);

	return success;
}

int stop_recording()
{
	int success = teardown_recorder();
	record_finished();

	return success;
}

void recorder_cleanup()
{
	free(rec_target_path);
	free(capture_handle);
}