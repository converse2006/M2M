CC=arm-none-linux-gnueabi-g++
CFLAGS= -ljpeg -static -O2 #-march=armv4t
#INCLUDE=-I /home/gkk886/pacduo_benchmark/linux-2.6-pac/include -I /home/gkk886/pacduo_benchmark/linux-2.6-pac/arch/arm/include


main: main.cpp
	$(CC) main.cpp -o surveillance `pkg-config --cflags --libs opencv` `pkg-config --cflags --libs zlib` $(CFLAGS) $(INCLUDE)

clean:
	rm surveillance*

default: main
