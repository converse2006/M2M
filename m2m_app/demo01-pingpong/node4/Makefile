#CC=arm-linux-uclibcgnueabi-gcc
#CC=arm-linux-gcc
CC=arm-none-linux-gnueabi-gcc
ANDROIDCC=arm-none-linux-gnueabi-gcc
CFLAGS=-static
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include

ANDROIDINC=-I /home/shocklink/MOEA/android/android-common-kernel/include

main: main.c
	$(CC) $(CFLAGS) main.c -o vnd_api04 $(INCLUDE)

clean:
	rm vnd_api04

android:
	$(ANDROIDCC) $(CFLAGS) main.c -o vnd_api04.eabi $(ANDROIDINC)

default: main
