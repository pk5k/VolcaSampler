#include <stdio.h>
#include <bcm2835.h>
#include <sys/timeb.h>
#include <alsa/asoundlib.h>
#include <sndfile.h>
#include "player.h"
#include "error.h"

static void (*player_started)();
static void (*player_finished)();
static void (*player_interrupted)(int);

static short* read_buffer;
static int read_err;
static int dir;
static int readcount = 0;
static int pcmrc;
static unsigned int player_rate = PLAYER_SAMPLERATE;
static snd_pcm_t* output_handle = NULL;
static snd_pcm_hw_params_t* player_hw_params;
static snd_pcm_format_t read_format = SND_PCM_FORMAT_S16_LE;
static snd_pcm_uframes_t player_frames;
static unsigned int player_buffer_time = 500000;/* ring buffer length in us */
static unsigned int player_period_time = 100000;/* period time in us */

static SNDFILE* player_source = NULL;
static SF_INFO read_target_info;
static char* player_source_path = NULL;

void setup_player(const char* _player_source_path, void (*_player_started)(), void (*_player_finished)(), void (*_player_interrupted)(int))
{
	player_started = _player_started;// if player has been started, this function will be called afterwards
	player_finished = _player_finished;// if player has been stopped, this function will be called afterwards
	player_interrupted = _player_interrupted;// if player has beed canceled or an error occured after player_started() was called

	player_source_path = (char*) malloc(sizeof(_player_source_path));
	strcpy(player_source_path, _player_source_path); // path of the wav file the data will be played of
}

int prepare_player_source()
{
	if (player_source_path == NULL)
	{
		fprintf(stderr, "No player source specified.\n");
		return MOD_ERR_UNKNOWN;
	}

	read_target_info.samplerate = PLAYER_SAMPLERATE;
	read_target_info.channels = PLAYER_CHANNELS;
    read_target_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

	player_source = sf_open(player_source_path, SFM_READ, &read_target_info);

	if (!player_source)
	{
		fprintf(stderr, "unable to open source file %s\n", player_source_path);
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "source file %s is ready.\n", player_source_path);
	
	return MOD_ERR_NONE;
}

int close_recording_source()
{
	if (player_source == NULL)
	{
		fprintf(stderr, "target file was not open.\n");
		return MOD_ERR_UNKNOWN;
	}

	sf_close(player_source);
	fprintf(stdout, "target file closed.\n");
	
	return MOD_ERR_NONE;
}

int teardown_player()
{
	if (output_handle == NULL)
	{
		fprintf(stderr, "Cannot stop player, player is not started.\n");
		return MOD_ERR_UNKNOWN;
	}

	printf("...stopping player.\n");

	close_recording_source();

	free(read_buffer);
	fprintf(stdout, "buffer freed\n");
	snd_pcm_close(output_handle);
	
	output_handle = NULL;
	
	fprintf(stdout, "audio interface closed\n");

	return MOD_ERR_NONE;
}

int cancel_player()
{
	int success = teardown_player();
	player_interrupted(MOD_ERR_CANCEL);

	return success;
}

int player_stop()
{
	int success = teardown_player();
	player_finished();

	return success;
}

int player_start()
{
	prepare_player_source();

	printf("source file prepared, starting player...\n");

	if ((read_err = snd_pcm_open(&output_handle, PLAYER_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
	{
		fprintf(stderr, "cannot open audio device %s (%s)\n", PLAYER_DEVICE, snd_strerror(read_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "audio interface opened\n");
		   
	if ((read_err = snd_pcm_hw_params_malloc(&player_hw_params)) < 0) 
	{
		fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(read_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params allocated\n");
				 
	if ((read_err = snd_pcm_hw_params_any(output_handle, player_hw_params)) < 0) 
	{
		fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params initialized\n");

	if ((read_err = snd_pcm_hw_params_set_access(output_handle, player_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
	{
		fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params access setted\n");

	if ((read_err = snd_pcm_hw_params_set_format(output_handle, player_hw_params, read_format)) < 0) 
	{
		fprintf(stderr, "cannot set sample format (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params format setted\n");

	if ((read_err = snd_pcm_hw_params_set_rate_near(output_handle, player_hw_params, &player_rate, 0)) < 0) 
	{
		fprintf(stderr, "cannot set sample rate (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params rate setted\n");

	if ((read_err = snd_pcm_hw_params_set_channels(output_handle, player_hw_params, PLAYER_CHANNELS)) < 0) 
	{
		fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(read_err));

		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params channels setted\n");

	/* disable hardware resampling */
    if ((read_err = snd_pcm_hw_params_set_rate_resample(output_handle, player_hw_params, 0)) < 0) 
    {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(read_err));

        return MOD_ERR_UNKNOWN;
    }

    /* set the buffer time */
    if ((read_err = snd_pcm_hw_params_set_buffer_time_near(output_handle, player_hw_params, &player_buffer_time, &dir)) < 0) 
    {
        printf("Unable to set buffer time %u for playback: %s\n", player_buffer_time, snd_strerror(read_err));

        return MOD_ERR_UNKNOWN;
    }
    
    /* set the period time */
    if ((read_err = snd_pcm_hw_params_set_period_time_near(output_handle, player_hw_params, &player_period_time, &dir)) < 0)
    {
        printf("Unable to set period time %u for playback: %s\n", player_period_time, snd_strerror(read_err));

        return MOD_ERR_UNKNOWN;
    }

    if ((read_err = snd_pcm_hw_params_get_period_size(player_hw_params, &player_frames, &dir)) < 0) 
    {
        printf("Unable to get period size for playback: %s\n", snd_strerror(read_err));

        return MOD_ERR_UNKNOWN;
    }
    
    fprintf(stderr,"frames in a period: %d\n", (int)player_frames);

	if ((read_err = snd_pcm_hw_params(output_handle, player_hw_params)) < 0) 
	{
		fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "player_hw_params setted\n");

	snd_pcm_hw_params_free(player_hw_params);

	fprintf(stdout, "player_hw_params freed\n");

	if ((read_err = snd_pcm_prepare(output_handle)) < 0) 
	{
		fprintf(stderr, "cannot prepare audio interface for use (%s)\n", snd_strerror(read_err));
		
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, "audio interface prepared\n");

    read_buffer = (short*) malloc(player_frames * read_target_info.channels * sizeof(short));

    fprintf(stdout, "buffer allocated\n");
    fprintf(stdout, "transfering sample...\n");

    player_started();

	return MOD_ERR_NONE;
}

int feed_player_buffer()
{
	if (output_handle == NULL)
	{
		fprintf(stderr, "Cannot transfer, player is not started.\n");
		cancel_player();
		
		return MOD_ERR_UNKNOWN;
	}
	
    if ((readcount = sf_readf_short(player_source, read_buffer, player_frames)) > 0) 
    {
        pcmrc = snd_pcm_writei(output_handle, read_buffer, readcount);

        if (pcmrc == -EPIPE) 
        {
            fprintf(stderr, "Underrun!\n");
            snd_pcm_prepare(output_handle);
            cancel_player();

            return MOD_ERR_UNKNOWN;
        }
        else if (pcmrc < 0) 
        {
            fprintf(stderr, "Error writing to PCM device: %s\n", snd_strerror(pcmrc));
            cancel_player();

            return MOD_ERR_UNKNOWN;
        }
        else if (pcmrc != readcount) 
        {
            fprintf(stderr,"PCM write difffers from PCM read.\n");
            cancel_player();
            
            return MOD_ERR_UNKNOWN;
        }
    }
    else 
    {
    	return player_stop();
    }
	
	return MOD_ERR_NONE;
}

void player_cleanup()
{
	free(player_source_path);
}