#include <stdio.h>
#include "volca_sampler.h"

int main(int argc, char *argv[])
{
	if (!read_args(argc, argv))
	{
		fprintf(stderr, "Read args failed.\n");
	}

  	if (!setup())
  	{
  		fprintf(stderr, "Setup failed.\n");
  		return -1;
  	}

	while (true)
	{
		cycle();
	}

	cleanup();

	return 0;
}

