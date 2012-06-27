#CC=arm-linux-uclibcgnueabi-gcc
#CC=arm-linux-gcc
CC=arm-none-linux-gnueabi-gcc
ANDROIDCC=arm-none-linux-gnueabi-gcc
CFLAGS=-static
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include

ANDROIDINC=-I /home/shocklink/MOEA/android/android-common-kernel/include

switch_vpmu: vpmu-test.c
	$(CC) $(CFLAGS) vpmu-test.c -o switch_vpmu $(INCLUDE)

clean:
	rm switch_vpmu

android:
	$(ANDROIDCC) $(CFLAGS) vpmu-test.c -o switch_vpmu.eabi $(ANDROIDINC)

default: switch_vpmu
