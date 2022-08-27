#ifndef PLAYER_H_INC
#define PLAYER_H_INC

// device the microphone input will be used of
#define PLAYER_DEVICE "hw:1"

#define PLAYER_SAMPLERATE 44100
#define PLAYER_CHANNELS 2

void setup_player(const char* _player_source_path, void (*_player_started)(), void (*_player_finished)(), void (*_player_interrupted)(int));
int player_start();
int feed_player_buffer();
int player_stop();
int cancel_player();
void player_cleanup();

#endif