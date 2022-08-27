#include <sys/stat.h>
#include "../syro/korg_syro_volcasample_adapter.h"
#include "../syro/korg_syro_volcasample.h"
#include "converter.h"
#include "error.h"

static void (*converter_started)();
static void (*converter_finished)();
static void (*converter_interrupted)(int);

static SyroData syro_data;
static SyroStatus status;
static SyroHandle handle;
static int num_of_data = 1;
static uint8_t *buf_dest;
static uint32_t size_dest;
static uint32_t frame, write_pos;
static int16_t left, right;
static char* convert_file_in = NULL;
static char* convert_file_out = NULL;
static bool started = false;
static struct stat st;

void setup_converter(const char* _convert_file_in, const char* _convert_file_out, void (*_converter_started)(), void (*_converter_finished)(), void (*_converter_interrupted)(int))
{
	converter_started = _converter_started;// if recording has been started, this function will be called afterwards
	converter_finished = _converter_finished;// if recording has been stopped, this function will be called afterwards
	converter_interrupted = _converter_interrupted;// if recording has been canceled or an error occured after converter_started(), ...

	convert_file_in = (char*) malloc(sizeof(_convert_file_in));
	strcpy(convert_file_in, _convert_file_in);

	convert_file_out = (char*) malloc(sizeof(_convert_file_out));
	strcpy(convert_file_out, _convert_file_out);
}

int converter_start(unsigned int sample_number, bool erase)
{
	if (convert_file_in == NULL)
	{
		fprintf(stderr, "Cannot start converter. setup_converter was not called.");
		return MOD_ERR_UNKNOWN;
	}

	syro_data.DataType = (erase) ? DataType_Sample_Erase : DataType_Sample_Liner;
	syro_data.Number = sample_number;

	if (!erase)
	{
		stat(convert_file_in, &st);
		int precheck_size = st.st_size;

		if (precheck_size <= sizeof(wav_header)) 
		{
			fprintf(stdout, "Failed to prepare file %s for conversion - threshold is too high or no input was given.\n", convert_file_in);

			return MOD_ERR_NODATA_THRESH;
		}

		// prepare
		if (!setup_file_sample(convert_file_in, &syro_data))
		{
			fprintf(stdout, "Failed to prepare file %s for conversion\n", convert_file_in);
			return MOD_ERR_UNKNOWN;
		}
	
		fprintf(stdout, "File prepared for sample #%d and input file %s...\n", sample_number, convert_file_in);
	}
	else 
	{
		fprintf(stdout, "File prepared for sample #%d to be erased...\n", sample_number);
	}

	// start
	status = SyroVolcaSample_Start(&handle, &syro_data, num_of_data, 0, &frame);

	if (status != Status_Success) 
	{
		fprintf(stdout, "SDK Start error, %d \n", status);
		free_syrodata(&syro_data, num_of_data);

		return MOD_ERR_UNKNOWN;
	}
	
	fprintf(stdout, "SDK start...\n");

	size_dest = (frame * 4) + sizeof(wav_header);
	
	buf_dest = (uint8_t*) malloc(size_dest);

	if (!buf_dest) 
	{
		fprintf(stdout, " - Not enough memory to write file.\n");
		SyroVolcaSample_End(handle);
		free_syrodata(&syro_data, num_of_data);
		return MOD_ERR_UNKNOWN;
	}

	fprintf(stdout, " - enough space available...\n");
	
	memcpy(buf_dest, wav_header, sizeof(wav_header));
	set_32Bit_value((buf_dest + WAV_POS_RIFF_SIZE), ((frame * 4) + 0x24));
	set_32Bit_value((buf_dest + WAV_POS_DATA_SIZE), (frame * 4));
	
	fprintf(stdout, " - headers written...\n");

	// convert loop
	write_pos = sizeof(wav_header);

	fprintf(stdout, " - converting...\n");

	started = true;
	converter_started();

	return MOD_ERR_NONE;
}

int converter_write_buffer()
{
	fprintf(stdout, " - writing file %s...\n", convert_file_out);

	if (convert_file_out == NULL)
	{
		fprintf(stderr, "Cannot write buffer. No output file specified.\n");
		return MOD_ERR_UNKNOWN;
	}

	// write
	if (write_file(convert_file_out, buf_dest, size_dest)) 
	{
		fprintf(stdout, "Done. Complete to transfer.\n");

		return MOD_ERR_NONE;
	} 
	else 
	{
		fprintf(stdout, "Error. Failed to write buffer to %s.\n", convert_file_out);

		return MOD_ERR_UNKNOWN;
	}
}

int teardown_converter()
{
	if (!started)
	{
		fprintf(stderr, "Converted was not started. Cannot stop.");
		return MOD_ERR_UNKNOWN;
	}

	SyroVolcaSample_End(handle);
	free_syrodata(&syro_data, num_of_data);
	fprintf(stdout, "SDK end\n");
	free(buf_dest);
	
	started = false;
	
	return MOD_ERR_NONE;
}

int converter_stop()
{
	int success = teardown_converter();
	converter_finished();

	return success;
}

int feed_converter_buffer()
{
	if (!started)
	{
		fprintf(stderr, "Cannot feed buffer. Converter is not started.\n");
		return MOD_ERR_UNKNOWN;
	}

	if (frame) 
	{
		SyroVolcaSample_GetSample(handle, &left, &right);
		buf_dest[write_pos++] = (uint8_t)left;
		buf_dest[write_pos++] = (uint8_t)(left >> 8);
		buf_dest[write_pos++] = (uint8_t)right;
		buf_dest[write_pos++] = (uint8_t)(right >> 8);
		frame--;
	}
	else 
	{
		fprintf(stdout, " - conversion complete...\n");
		
		return converter_write_buffer() | converter_stop();
	}

	return MOD_ERR_NONE;
}

int cancel_converter()
{
	int success = teardown_converter();

	converter_interrupted(MOD_ERR_CANCEL);
	return success;
}

void converter_cleanup()
{
	free(convert_file_in);
	free(convert_file_out);
}