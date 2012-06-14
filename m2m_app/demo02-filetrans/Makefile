#CC=arm-linux-uclibcgnueabi-gcc
#CC=arm-linux-gcc
CC=arm-none-linux-gnueabi-g++
ANDROIDCC=arm-none-linux-gnueabi-g++
CFLAGS=-static  #-march=armv4t
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include

ANDROIDINC=-I /home/shocklink/MOEA/android/android-common-kernel/include

main: main.c
	$(CC) $(CFLAGS) main.c -o vnd_api $(INCLUDE)

clean:
	rm vnd_api

android:
	$(ANDROIDCC) $(CFLAGS) main.c -o vnd_api.eabi $(ANDROIDINC)

default: main
