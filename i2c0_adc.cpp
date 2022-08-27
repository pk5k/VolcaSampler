#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <bcm2835.h>
#include <math.h>
#include "Adafruit_ADS1X15_RPi/Adafruit_ADS1015.h"

static Adafruit_ADS1115 ads;// ADC instance

void setup_base()
{
	if (!bcm2835_init())
	{
		fprintf(stderr, "bcm2385 failed to initialize.\n");
		return;
	}

	fprintf(stdout, "bcm2385 initialized\n");

	if (!bcm2835_i2c_begin())
	{
		fprintf(stdout, "i2c failed to initialize.\n");
		return;
	}

	fprintf(stdout, "i2c initialized\n");	
}

void setup_p5_routing()
{
	bcm2835_gpio_fsel(0, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(1, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(28, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(29, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(28, BCM2835_GPIO_FSEL_ALT0);
    bcm2835_gpio_set_pud(28, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_fsel(29, BCM2835_GPIO_FSEL_ALT0);
    bcm2835_gpio_set_pud(29, BCM2835_GPIO_PUD_UP);
    
    fprintf(stdout, "i2c0 P5 routing setup.\n");

    //I2C Port 0 is now routed to P5 header and so to /dev/i2c-0
}

void setup_ads()
{
	// ADC init:
  	ads.setGain(GAIN_ONE);
  	ads.begin(false);// begin(false) for dbg info, use i2c channel 0, 1 is required for wakeup/shutdown functionality

    fprintf(stdout, "ADS1115 setup.\n");
}

void access(int channel)
{
	fprintf(stdout, "- channel %d ", channel);
    int res = ads.readADC_SingleEnded(channel);
    bool resto = (res > pow(2, 15));

    fprintf(stdout, "result: %d -> %s.\n", res, resto ? "value exceeded 15bit range" : "ok");
}

int main(int argc, char *argv[])
{
	fprintf(stdout, "volca sampler i2c0 adc setup:\n");

	setup_base();
	setup_p5_routing();
	setup_ads();

	fprintf(stdout, "accessing all adc inputs once...\n");

	access(0);
	access(1);
	access(2);
	access(3);
  	
    fprintf(stdout, "i2c0 adc setup complete.\nclosing connections...");

  	bcm2835_i2c_end();
  	bcm2835_close();

  	fprintf(stdout, "done.\nvolca_sampler can now be started...\n");

	return 0;
}

