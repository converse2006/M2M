CC=arm-none-linux-gnueabi-gcc
CFLAGS= -O2 -static  #-march=armv4t
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include

ANDROIDINC=-I /home/shocklink/MOEA/android/android-common-kernel/include

main: main.c
	$(CC) main.c -o multihops $(CFLAGS) $(INCLUDE)

clean:
	rm multihops

default: main
