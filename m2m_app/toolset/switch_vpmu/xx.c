#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<string.h>
#include <errno.h>
#include"asm-generic/fcntl.h"
#include "sys/mman.h"
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 0x1000
#define MAP_MASK (MAP_SIZE-1)
#define u8 unsigned char

void* addr;
int xx(int para_num, int open_mode, int offset, int timing_model)
{
	unsigned int argv_2=0;
	unsigned long long map_address=0xf0000000;
	switch(open_mode)
	{
		case 0:
            map_address += (unsigned long long)offset;
            fprintf(stderr,"offset is %x\n",map_address);
            break;
		case 1:
            fprintf(stderr,"turn off\n");
			argv_2=1;
			break;
		default:
			break;
	}


	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
    {   
		fprintf(stderr,"gpio: Error opening /dev/mem\n");
		exit(-1);
	}   
	addr = mmap(0, 20, PROT_READ| PROT_WRITE, MAP_SHARED, fd, map_address & ~MAP_MASK); 

	if(addr == (void *) -1) {
		printf("map_base:%x \n",addr);FATAL; }

	addr 	= addr + (map_address & MAP_MASK);

    close(fd);
    argv_2 = timing_model;
    memcpy(addr,&argv_2,1);
    return 0;
	
}
int main(int argc,char**argv)
{

    int para_num = 3;
    int open = 0;
    int offset = 0;
    int timing_model = 3;
    xx(para_num,open, offset, timing_model);
	return 0;
}

