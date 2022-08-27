#!/bin/bash

chmod 0777 volca_sampler
scp ./volca_sampler sampler:~/volca_sampler
scp ./i2c0_adc sampler:~/i2c0_adc
scp ./volca_sampler_host sampler:~/volca_sampler_host