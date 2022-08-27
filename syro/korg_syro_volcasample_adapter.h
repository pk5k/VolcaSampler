#ifndef KORG_SYRO_VOLCASAMPLE_ADAPTER_H_INC
#define KORG_SYRO_VOLCASAMPLE_ADAPTER_H_INC

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "korg_syro_comp.h"
#include "korg_syro_type.h"
#include "korg_syro_func.h"
#include "korg_syro_comp.h"
#include "korg_syro_volcasample.h"

#define	MAX_SYRO_DATA	10

#define WAVFMT_POS_ENCODE	0x00
#define WAVFMT_POS_CHANNEL	0x02
#define WAVFMT_POS_FS		0x04
#define WAVFMT_POS_BIT		0x0E

#define WAV_POS_RIFF_SIZE	0x04
#define WAV_POS_WAVEFMT		0x08
#define WAV_POS_DATA_SIZE	0x28

const uint8_t wav_header[44] = {
	'R' , 'I' , 'F',  'F',		// 'RIFF'
	0x00, 0x00, 0x00, 0x00,		// Size (data size + 0x24)
	'W',  'A',  'V',  'E',		// 'WAVE'
	'f',  'm',  't',  ' ',		// 'fmt '
	0x10, 0x00, 0x00, 0x00,		// Fmt chunk size
	0x01, 0x00,					// encode(wav)
	0x02, 0x00,					// channel = 2
	0x44, 0xAC, 0x00, 0x00,		// Fs (44.1kHz)
	0x10, 0xB1, 0x02, 0x00,		// Bytes per sec (Fs * 4)
	0x04, 0x00,					// Block Align (2ch,16Bit -> 4)
	0x10, 0x00,					// 16Bit
	'd',  'a',  't',  'a',		// 'data'
	0x00, 0x00, 0x00, 0x00		// data size(bytes)
};

void set_32Bit_value(uint8_t *ptr, uint32_t dat);
uint32_t get_32Bit_value(uint8_t *ptr);
uint16_t get_16Bit_value(uint8_t *ptr);
uint8_t *read_file(char *filename, uint32_t *psize);
bool write_file(char *filename, uint8_t *buf, uint32_t size);
int get_commandline_number(char *str, int *ppos);
bool setup_file_sample(char *filename, SyroData *syro_data);
bool setup_file_all(char *filename, SyroData *syro_data);
bool setup_file_pattern(char *filename, SyroData *syro_data);
int analayze_commandline(int num_of_argv, char **argv, SyroData *syro_data);
void free_syrodata(SyroData *syro_data, int num_of_data);

#endif