#CC=arm-linux-uclibcgnueabi-gcc
#CC=arm-linux-gcc
CC=arm-none-linux-gnueabi-gcc
ANDROIDCC=arm-none-linux-gnueabi-gcc
CFLAGS=-static
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include

ANDROIDINC=-I /home/shocklink/MOEA/android/android-common-kernel/include

switch_vpmu: network_device.c
	$(CC) $(CFLAGS) network_device.c -o vnd_api03 $(INCLUDE)

clean:
	rm vnd_api03

android:
	$(ANDROIDCC) $(CFLAGS) network_device.c -o vnd_api03.eabi $(ANDROIDINC)

default: switch_vpmu
