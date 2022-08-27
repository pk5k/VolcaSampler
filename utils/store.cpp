#include "store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool load_sample_from_slot(const char* export_path, const char* target_name, int slot)
{
	if (!export_path)
	{
		fprintf(stderr, "No export path specified. Run volca_sampler with configuration -e.\n");
		return false;
	}

	char* source_name = (char*) malloc(strlen(export_path) + sizeof slot + strlen("/.wav"));
 	sprintf(source_name, "%s/%d.wav", export_path, slot);

	FILE* fr = fopen(source_name, "r");

	if (!fr)
	{
		fprintf(stderr, "File %s cannot be opened for reading.\n", source_name);
		return false;
	}
	
   	FILE* fw = fopen(target_name, "w");
 
   	if (!fw)
   	{
      fclose(fr);
      fprintf(stderr, "File %s cannot be opened for writing.\n", target_name);

      return false;
   	}
 
 	int ch;

   	while ((ch = fgetc(fr)) != EOF)
   	{
      fputc(ch, fw);
   	}

   	fprintf(stdout, "Sample slot %d loaded from %s to %s\n", slot, source_name, target_name);

	fclose(fr);
	fclose(fw);
   	free(source_name);

	return true;
}

bool store_sample_to_slot(const char* export_path, const char* sample_file, int slot)
{
	if (!export_path)
	{
		fprintf(stderr, "No export path specified. Run volca_sampler with configuration -e.\n");
		return false;
	}

	FILE* fr = fopen(sample_file, "r");

	if (!fr)
	{
		fprintf(stderr, "File %s cannot be opened for reading.\n", sample_file);
		return false;
	}

	char* target_name = (char*) malloc(strlen(export_path) + sizeof slot + strlen("/.wav"));
 	sprintf(target_name, "%s/%d.wav", export_path, slot);

   	FILE* fw = fopen(target_name, "w");
 
   	if (!fw)
   	{
      fclose(fr);
      fprintf(stderr, "File %s cannot be opened for writing.\n", target_name);

      return false;
   	}
 
 	int ch;

   	while ((ch = fgetc(fr)) != EOF)
   	{
      fputc(ch, fw);
   	}

   	fprintf(stdout, "Sample %s stored to slot %d at %s\n", sample_file, slot, target_name);

	fclose(fr);
	fclose(fw);
   	free(target_name);

	return true;
}