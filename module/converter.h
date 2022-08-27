#ifndef MODULE_CONVERTER_H_INC
#define MODULE_CONVERTER_H_INC

#include "../syro/korg_syro_volcasample_adapter.h"


void setup_converter(const char* _convert_file_in, const char* _convert_file_out, void (*_converter_started)(), void (*_converter_finished)(), void (*_converter_interrupted)(int));
int converter_start(unsigned int sample_number, bool erase = false);
int feed_converter_buffer();
int converter_stop();
int cancel_converter();
void converter_cleanup();

#endif