#!/bin/bash
RASPBIAN_SDK_HOME=/Users/pkopf/Projects/code/raspbian-sdk;
$RASPBIAN_SDK_HOME/prebuilt/bin/arm-linux-gnueabihf-clang \
	-o volca_sampler \
		main.cpp volca_sampler.cpp \
		utils/time.cpp utils/store.cpp \
		Adafruit_ADS1X15_RPi/Adafruit_ADS1015.cpp \
		syro/korg_syro_comp.c syro/korg_syro_func.c syro/korg_syro_volcasample.c syro/korg_syro_volcasample_adapter.cpp \
		control/control.cpp control/converter.cpp \
		trigger/trigger.cpp \
		led/led.cpp \
		module/recorder.cpp module/converter.cpp module/player.cpp module/editor.cpp \
	-I$RASPBIAN_SDK_HOME/sysroot/usr/local/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include/libsndfile \
	-L$RASPBIAN_SDK_HOME/sysroot/usr/local/lib -L$RASPBIAN_SDK_HOME/sysroot/usr/lib -L$RASPBIAN_SDK_HOME/sysroot/lib -L$RASPBIAN_SDK_HOME/usr/lib/arm-linux-gnueabihf \
	-rpath -rpath-link -lcrypt -lrt -lm -lpthread \
	-lwiringPi -lbcm2835 -lasound -ldl -lsndfile -logg -lflac -lvorbis -lvorbisenc -lopus -lltdl -lpng -lz -lmagic -lgsm -lbz2 -lgomp -lsox \
	$@

$RASPBIAN_SDK_HOME/prebuilt/bin/arm-linux-gnueabihf-clang \
	-o i2c0_adc \
		i2c0_adc.cpp \
		Adafruit_ADS1X15_RPi/Adafruit_ADS1015.cpp \
	-I$RASPBIAN_SDK_HOME/sysroot/usr/local/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include/libsndfile \
	-L$RASPBIAN_SDK_HOME/sysroot/usr/local/lib -L$RASPBIAN_SDK_HOME/sysroot/usr/lib -L$RASPBIAN_SDK_HOME/sysroot/lib -L$RASPBIAN_SDK_HOME/usr/lib/arm-linux-gnueabihf \
	-rpath -rpath-link -lcrypt -lrt -lm -lpthread \
	-lwiringPi -lbcm2835 \
	$@

$RASPBIAN_SDK_HOME/prebuilt/bin/arm-linux-gnueabihf-clang \
	-o volca_sampler_host \
		host.cpp \
	-I$RASPBIAN_SDK_HOME/sysroot/usr/local/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include -I$RASPBIAN_SDK_HOME/sysroot/usr/include/libsndfile \
	-L$RASPBIAN_SDK_HOME/sysroot/usr/local/lib -L$RASPBIAN_SDK_HOME/sysroot/usr/lib -L$RASPBIAN_SDK_HOME/sysroot/lib -L$RASPBIAN_SDK_HOME/usr/lib/arm-linux-gnueabihf \
	-lbcm2835 \
	$@


bash "deploy.sh"