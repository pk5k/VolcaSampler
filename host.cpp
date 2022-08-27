#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/wait.h> 
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <bcm2835.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

extern char **environ;

void wait_for_pid(pid_t pid)
{
	int status;
	if (waitpid(pid, &status, 0) != -1) 
	{
		int es = WEXITSTATUS(status);
		fprintf(stdout, "pid %d stopped with exit status: %d - %s\n", pid, es, (WEXITSTATUS(status) != 0) ? "program may have crashed." : "program was closed normally.");
	} 
	else 
	{
		fprintf(stderr, "error while waiting for pid %d\n", pid);
		exit(1);
	}
}

void reset_ready_led()
{
	// if main program stops running, disable the ready led
	bcm2835_init();
	bcm2835_gpio_write(RPI_GPIO_P1_23, LOW);
	bcm2835_close();
}

bool dir_exists(const char* path)
{
	DIR* dir = opendir(path);

	if (dir) 
	{
	    closedir(dir);
	    return true;
	}
	else if (ENOENT == errno) 
	{
	    return false;
	} 
	else 
	{
		fprintf(stderr, "Unknown error %d while opening directory %s\n", errno, path);
		return false;
	}
}

bool create_dir(const char* path)
{
	int mkdir_res = mkdir(path, 0777);

	if (mkdir_res == 0)
	{
		return true;
	}
	else
	{
		fprintf(stderr, "error while creating path %s: exit status = %d, errno = %d", path, mkdir_res, errno);

		return false;
	}
}

void check_args(int argc, char *argv[])
{
	fprintf(stdout, "checking %d input arguments...\n", argc);

	for (int i = 0; i < argc; i++)
	{
		const char* current_arg = argv[i];

		if (strcmp(current_arg, "-e") == 0)
		{
			if (i + 1 > argc)
			{
				fprintf(stderr, "export argument (-e) was given but no path specified.");
				exit(1);
			}

			const char* path = argv[i+1];
			fprintf(stdout, "found export argument (-e) checking path %s...", path);

			if (dir_exists(path))
			{
				fprintf(stdout, "exists, nothing to do.\n");
			}
			else 
			{
				fprintf(stdout, "does not exist. Trying to create directory...");

				if (create_dir(path))
				{
					fprintf(stdout, "done.\n");
				}
				else 
				{
					fprintf(stderr, "failed to create directory %s - maybe program is not running as root?\n", path);
					exit(1);
				}
			}
		}
	}

	fprintf(stdout, "checking input arguments done.\n");
}

int main(int argc, char *argv[])
{
	fprintf(stdout, "volca sampler host\n");

	if (argc > 1)
	{
		check_args(argc, argv);
	}

	fprintf(stdout, "starting i2c0_adc...\n");

	pid_t pid;
    char *_argv[] = {};// no args for this
    int status = posix_spawn(&pid, "/home/sampler/i2c0_adc", NULL, NULL, _argv, environ);

    if (status == 0) 
    {
        wait_for_pid(pid);
    }
    else
    {
        fprintf(stderr, "posix_spawn failed: %d\n", status);
        exit(1);
    }

    fprintf(stdout, "starting volca_sampler...\n");

    status = posix_spawn(&pid, "/home/sampler/volca_sampler", NULL, NULL, argv, environ);// pass host arguments directly

    if (status == 0) 
    {
        wait_for_pid(pid);
        reset_ready_led();
    }
    else
    {
        fprintf(stderr, "posix_spawn failed: %d\n", status);
        exit(1);
    }

	return 0;
}

