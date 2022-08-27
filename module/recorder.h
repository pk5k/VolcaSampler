#include <bcm2835.h>

#ifndef RECORDER_H_INC
#define RECORDER_H_INC

// device the microphone input will be used of
#define REC_DEVICE "hw:1"

#define REC_SAMPLERATE 44100
#define REC_CHANNELS 1
#define REC_BUFFER_FRAMES 512

void setup_recorder(const char* _rec_target_path, void (*_record_started)(), void (*_record_finished)(), void (*_record_interrupted)(int));
int feed_recorder_buffer();
int start_recording();
int stop_recording();
int cancel_recording();
void recorder_cleanup();

#endif