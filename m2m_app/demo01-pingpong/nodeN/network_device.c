#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<string.h>
#include <errno.h>
#include"asm-generic/fcntl.h"
#include "sys/mman.h"
#define MAP_SIZE 0x1000
#define NDIST 16
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
		__LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
//FIXME Currently, only support Zigbee
//if need to support other network
//memory map I/O offset need to fix
//ex: NDIST * offset + addr
#define MAP_MASK (MAP_SIZE-1)
//void* addr;

typedef struct net_send{
    int ReceiverID;
    uint32_t DataAddress;
    unsigned int DataSize;
}net_send;

typedef struct net_recv{
    int SenderID; 
    uint32_t DataAddress;
}net_recv;

int Network_Init(unsigned int, char*);
int Network_Exit(char*);
int Network_Send(unsigned int, char*, char* );
int Network_Recv(char* ,char* );
void ShowPacketInfo(net_send);
void* DeviceOpen(int, int);

enum Network_Type{Zigbee, Wifi, Bluetooth};
unsigned int DeviceID;
#define NETWORK_TYPE_NUM 3
char* NETWORK_TYPE[NETWORK_TYPE_NUM] = {"Zigbee","Wifi","Bluetooth"};

//
//                       [1.ZC]
//                         |
//                       [2.ZR]
//                        /|\
//                       / | \
//                      /  |  \
//                     /   |   \
//                    /    |    \
//              [3.ZED] [4.ZED] [5.ZED]
//


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

void* DeviceOpen(int size, int offset)
{
    void* addr;
	unsigned long long map_address=0xe0000000;
	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){   
		fprintf(stderr,"gpio: Error opening /dev/mem\n");
		exit(-1);
	}   
	//addr 	= addr + (map_address & MAP_MASK);
	addr = mmap(0, size, PROT_READ| PROT_WRITE, MAP_SHARED, fd, map_address & ~MAP_MASK); 
    close(fd);
    return addr;
}

int Network_Recv(char* message, char* NetworkType)
{
    void* addr;
    net_recv packet;
    packet.SenderID = -1;
    packet.DataAddress = (uint32_t)&message[0];
    //int Destination = (int)message;
    addr = DeviceOpen(sizeof(net_recv), 0);
    int PA = (int)(&packet);
    memcpy(addr + 8, &PA, sizeof(int));
    return 1;
}

int Network_Send(unsigned int ReceiverID, char* message, char* NetworkType)
{
    void* addr;
    net_send packet;
    //printf("Message size = %d\n",strlen(message));
    packet.ReceiverID = ReceiverID;
    packet.DataAddress = (uint32_t)&message[0];
    packet.DataSize = strlen(message);
    //ShowPacketInfo(packet);

    addr = DeviceOpen(sizeof(net_send), 0);
    //printf("packet address = %x\n", &packet);
    int PA = (int)(&packet);
    memcpy(addr+4,&PA,sizeof(int));
    return 1;
}

int vpmu_control(int para_num, int open_mode, int offset, int timing_model)
{
    void* addr;
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
    if(open_mode == 0)
        argv_2 = timing_model;
    memcpy(addr,&argv_2,1);

    return 0;
}

int Network_Init(unsigned int Device_ID, char* NetworkType)
{
    void* addr;
    int ind;
    int flag = 0;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
        {
            flag = 1;
            printf("Network type: %s Init \n",NETWORK_TYPE[ind]);
        }
    if(!flag)
    {
        fprintf(stderr, "Network type does not support!");
        return 0;
    }

    DeviceID = Device_ID;
    addr = DeviceOpen(20, 0);
    memcpy(addr,&Device_ID,4);

    int para_num = 3;
    int open = 0;
    int offset = 0;
    int timing_model = 3;
    vpmu_control(para_num,open, offset, timing_model);

    return 1;
}

int Network_Exit(char* NetworkType)
{
    void* addr;

    int para_num = 3;
    int open = 1;
    int offset = 0;
    int timing_model = 3;
    vpmu_control(para_num,open, offset, timing_model);

    int ind;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
            printf("Network type: %s Exit \n",NETWORK_TYPE[ind]);

    addr = DeviceOpen(20, 0);
    memcpy(addr+12,&DeviceID,4);

    return 1;
}
