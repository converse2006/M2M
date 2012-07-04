#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include "sys/mman.h"
#include  <fcntl.h>
#define MAP_SIZE 0x1000
#define NDIST 16
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
//FIXME Currently, only support Zigbee
//if need to support other network
//memory map I/O offset need to fix
//ex: NDIST * offset + addr
#define VPMU_BASED_ADDR 0xf0000000
#define VPMU_IOMEM_SIZE 0x2000
#define SET_BASE_ADDR   0xf0002000
#define SET_IOMEM_SIZE  0x1000
#define VND_BASED_ADDR  0xf0004000
#define VND_IOMEM_SIZE  0x1000
#define MAP_MASK (MAP_SIZE-1)
#define NETWORK_TYPE_NUM 3
#define PACKETSIZE 101  //93 64 46 32 20 8
#define Netoffx //NOTE: This is need also mark "CONFIG_VND" at android/config/linux-x86/config-host.h
#define VPMUoffx


typedef struct net_init{
    int DeviceID;
    int rv; //return value
}net_init;

typedef struct net_exit{
    int DeviceID;
    int rv; //return value
}net_exit;

typedef struct net_send{
    int ReceiverID;
    uint32_t DataAddress;
    unsigned int DataSize;
    int rv; //return value
}net_send;

typedef struct net_recv{
    int SenderID; 
    uint32_t DataAddress;
    int rv; //return value
}net_recv;

int Network_Init(unsigned int, const char*, int);
int Network_Exit(const char*);
int Network_Send(unsigned int, char*, const char* );
int Network_Recv(char* ,const char* );
void ShowPacketInfo(net_send);
void* VNDOpen(int, int);

void* vnd_addr;
enum Network_Type{Zigbee, Wifi, Bluetooth};
unsigned int DeviceID;
const char* NETWORK_TYPE[NETWORK_TYPE_NUM] = {"Zigbee","Wifi","Bluetooth"};

void ShowPacketInfo(net_send PACKET)
{
    printf("Packet Info\n");
    printf("===========================\n");
    printf("Packet Receiver: %d\n",PACKET.ReceiverID);
    printf("Packet Address: %x\n",PACKET.DataAddress);
    printf("Packet Size: %d\n",PACKET.DataSize);
    printf("Packet Content: ");
    char* address = (char*)PACKET.DataAddress;
    int ind = 0;
    for(ind = 0; ind < PACKET.DataSize; ind++)
        printf("%c", *(address+ind));
    printf("\n");
    printf("===========================\n");
}

void* VNDOpen(int size, int offset)
{
	unsigned long long map_address = VND_BASED_ADDR;
	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){   
		fprintf(stderr,"gpio: Error opening /dev/mem\n");
		exit(-1);
	} 

	//addr 	= addr + (map_address & MAP_MASK);
	vnd_addr = mmap(0, size, PROT_READ| PROT_WRITE, MAP_SHARED, fd, map_address & ~MAP_MASK); 
    close(fd);
}

int Network_Recv(char* message, const char* NetworkType)
{
    net_recv packet;
    packet.rv = 1;
#ifndef Netoff
    packet.SenderID = -1;
    bzero(message, PACKETSIZE);
    packet.DataAddress = (uint32_t)&message[0];

    int PA = (int)(&packet);
    memcpy(vnd_addr + 8, &PA, sizeof(int));
#endif
    return packet.rv;
}

int Network_Send(unsigned int ReceiverID, char* message, const char* NetworkType)
{
    net_send packet;
    packet.rv = 1;
#ifndef Netoff
    packet.ReceiverID = ReceiverID;
    packet.DataAddress = (uint32_t)&message[0];
    packet.DataSize = PACKETSIZE;//strlen(message);
    //ShowPacketInfo(packet);

    int PA = (int)(&packet);
    memcpy(vnd_addr + 4, &PA, sizeof(int));
#endif

    return packet.rv;
}

int vpmu_control(int para_num, int open_mode, int offset, int timing_model)
{
    void *addr;
	unsigned int argv_2=0;
	unsigned long long map_address = VPMU_BASED_ADDR;
	switch(open_mode)
	{
		case 0:
            map_address += (unsigned long long)offset;
            //fprintf(stderr,"offset is %x\n",map_address);
            break;
		case 1:
            //fprintf(stderr,"turn off\n");
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

    if(addr == (void *) -1)
    {
        fprintf(stderr, "Error when mmap /dev/mem\n");
        exit(1);
    }

	addr 	= addr + (map_address & MAP_MASK);

    close(fd);
    if(open_mode == 0)
        argv_2 = timing_model;
    memcpy(addr,&argv_2,1);

    return 0;
}


int Network_Init(unsigned int Device_ID, const char* NetworkType/*, int set, int program_type*/, int timing_model)
{
    net_init netinit;
    netinit.rv = 1;
#ifndef Netoff
    int ind;
    int flag = 0;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
        {
            flag = 1;
            //printf("Network type: %s Init \n",NETWORK_TYPE[ind]);
        }
    if(!flag)
    {
        fprintf(stderr, "Network type does not support!");
        return 0;
    }

    DeviceID = Device_ID;
    netinit.DeviceID =  Device_ID;
    int PA = (int)(&netinit);

    VNDOpen(20, 0);
    memcpy(vnd_addr,&PA, sizeof(int));
#endif

#ifndef VPMUoff
    {
        if(timing_model != 0)
        {
            int para_num = 3;
            int open = 0;
            int offset = 0;
            //int timing_model = 5;
            vpmu_control(para_num,open, offset, timing_model);
        }
    }
#endif

    return netinit.rv;
}

int Network_Exit(const char* NetworkType)
{
    net_exit netexit;
    netexit.rv = 1;
    void* addr;

#ifndef VPMUoff
    int para_num = 3;
    int open = 1;
    int offset = 0;
    int timing_model = 3;
    vpmu_control(para_num,open, offset, timing_model);
#endif

#ifndef Netoff
    int ind;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
            printf("Network type: %s Exit \n",NETWORK_TYPE[ind]);

    netexit.DeviceID =  DeviceID;
    int PA = (int)(&netexit);
    memcpy(vnd_addr+12,&PA, sizeof(int));
#endif

    return netexit.rv;
}
