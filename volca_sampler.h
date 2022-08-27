#ifndef VOLCA_SAMPLER_H_INC
#define VOLCA_SAMPLER_H_INC

#include <bcm2835.h>

// filename of the recorded audio
#define RECORDER_OUT "input.wav"
#define EDITOR_IN "input_proc.wav"
#define EDITOR_OUT "output.wav"
#define CONVERTER_OUT "send.wav"

#define PROG_IDLE 0
#define PROG_RECORD 1
#define PROG_CONVERT 2
#define PROG_EDIT 3
#define PROG_TRANSFER 4
#define PROG_CANCEL_WAIT 5

#define PFLAG_SUPPRESS_CANCEL 1
#define PFLAG_SUPPRESS_EXPORT 2

#define TRIG_RECORD 1
#define TRIG_CANCEL 2
#define TRIG_REDO 4
#define TRIG_SHUTDOWN 8

#define CTRL_SAMPLE_0 1
#define CTRL_SAMPLE_1 2
#define CTRL_SPEEDUP 4
#define CTRL_THRESHOLD 8

#define LED_READY 1
#define LED_ERROR 2
#define LED_RECORD 3
#define LED_PROCESS 4
#define LED_TRANSFER 5
#define LED_NO_DATA 6

bool read_args(int argc, char *argv[]);
bool setup();
void cycle();
void cleanup();

#endif