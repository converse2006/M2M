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
static void do_usage(void) 
{
    fprintf(stderr, "Usage: switch_vpmu [Option] value\n");
    fprintf(stderr, "Option:\n");
    fprintf(stderr, "-o   --offset   Assign offset(in hex) relative to 0x50011000\n");
    fprintf(stderr, "                Write to mem address\n");
    fprintf(stderr, "     --on       Switch vpmu ON\n");
    fprintf(stderr, "     --off      Switch vpmu OFF\n");
	exit(0);
}
	void* addr;
struct option longopts[]={
	{"on",no_argument,NULL,0},
	{"off",no_argument,NULL,1},
	{"offset",required_argument,NULL,'o'},
	{"help",required_argument,NULL,'h'}
};
int main(int argc,char**argv)
{
	int c;
	unsigned int argv_2=0;
    char is_frequency=0;
	unsigned long long map_address=0xf0000000;
    if(argc==1)
    {
        do_usage();
        exit(0);
    }
    while ((c = getopt_long (argc, argv, "01o:", longopts, NULL)) != -1)
	{
		switch(c)
		{
			case 0:
				fprintf(stderr,"turn on\n");
				argv_2=0;
				break;
			case 1:
				fprintf(stderr,"turn off\n");
				argv_2=1;
				break;
            case 'o':
                map_address += strtoul(optarg,NULL,16);
                fprintf(stderr,"offset is %X\n",map_address);

                if(map_address==0x50011016)
                    is_frequency=1;

                break;
            case 'h':
                do_usage();
                break;
			default:
                do_usage();
				break;
		}
	}
    if(is_frequency) {
    if( argc >= (optind+1) )
    {
        fprintf(stderr,"argv[optind]=%s optind=%d argc=%d\n",argv[optind],optind,argc);
        argv_2 = strtoul(argv[optind],NULL,10);
    }
    else {
        fprintf(stderr,"frequency value needed \n");
        exit(0);
    }
    }
	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){   
		fprintf(stderr,"gpio: Error opening /dev/mem\n");
		exit(-1);
	}   
	addr = mmap(0, 20, PROT_READ| PROT_WRITE, MAP_SHARED, fd, map_address & ~MAP_MASK); 
	//addr=mmap(0, 20, PROT_READ| PROT_WRITE, MAP_SHARED, fd, MAP_ADDRESS & ~MAP_MASK); 

	if(addr == (void *) -1) {
		printf("map_base:%x \n",addr);FATAL; }
	//printf("Memory mapped at address %p.\n", addr);

	addr 	= addr + (map_address & MAP_MASK);
	//addr 	= addr + (MAP_ADDRESS & MAP_MASK);

	//printf("New Memory mapped at address %p.\n", addr);
    close(fd);
	//sprintf((char *)addr, "%c", atoi(argv[2]));//似乎會傳2個character
	if( is_frequency )
    	memcpy(addr,&argv_2,4);
    else
    {
        if( argc >= (optind+1) )
        {
            argv_2 = strtoul(argv[optind],NULL,10);
        }
        memcpy(addr,&argv_2,1);
    }
	
	/*  used to read from /dev/mem  */
	/*
	unsigned int tmp;
	memcpy(&tmp,addr,sizeof(char));
	fprintf(stderr,"fuck=%x\n",tmp);
	*/
	return 0;
}
