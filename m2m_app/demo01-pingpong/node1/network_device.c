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
#define MAP_MASK (MAP_SIZE-1)
void* addr;
int Network_Initialization(unsigned int, char*);
int Network_Send(unsigned int, char*, char* );
int Network_Recv(char* ,char* );
void ShowPacketInfo(net_send);
void* DeviceOpen(int, int);
typedef struct net_send{
    int ReceiverID;
    uint32_t DataAddress;
    unsigned int DataSize;
}net_send;

typedef struct net_recv{
    int SenderID; 
    uint32_t DataAddress;
}net_recv;

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

int main()
{
    char send_data[] = "This is what I only care about in my whole life!!!";
    char recv_data[100];
    int ret;
    ret = Network_Init(5, "Zigbee");
    Network_Send(1, send_data, "Zigbee");
    Network_Recv(recv_data, "Zigbee");
    printf("Receive data: %s\n",recv_data);
    ret = Network_Exit(5, "Zigbee");
}

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
    net_recv packet;
    packet.SenderID = -1;
    packet.DataAddress = (uint32_t)&message[0];
    //int Destination = (int)message;
    addr = DeviceOpen(sizeof(net_recv), 0);
    int PA = (int)(&packet);
    memcpy(addr + 8, &PA, sizeof(int));
}

int Network_Send(unsigned int ReceiverID, char* message, char* NetworkType)
{
    net_send packet;
    printf("Message size = %d\n",strlen(message));
    packet.ReceiverID = ReceiverID;
    packet.DataAddress = (uint32_t)&message[0];
    packet.DataSize = strlen(message);
    ShowPacketInfo(packet);

    addr = DeviceOpen(sizeof(net_send), 0);
    printf("packet address = %x\n", &packet);
    int PA = (int)(&packet);
    memcpy(addr+4,&PA,sizeof(int));
    return 1;
}

int Network_Init(unsigned int Device_ID, char* NetworkType)
{
    int ind;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
            printf("Network type: %s Init \n",NETWORK_TYPE[ind]);

    addr = DeviceOpen(20, 0);
    memcpy(addr,&Device_ID,4);
    return 1;
}

int Network_Exit(unsigned int Device_ID, char* NetworkType)
{
    int ind;
    for(ind = 0; ind < NETWORK_TYPE_NUM; ind++)
        if(!strcmp(NetworkType, NETWORK_TYPE[ind]))
            printf("Network type: %s Exit \n",NETWORK_TYPE[ind]);

    addr = DeviceOpen(20, 0);
    memcpy(addr+12,&Device_ID,4);
    return 1;
}
/*int main(int argc,char**argv)
{
	int c;
	unsigned int argv_2=0;
    char is_frequency=0;
	unsigned long long map_address=0xe0000000;
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
	
	//  used to read from /dev/mem  
	
	//unsigned int tmp;
	//memcpy(&tmp,addr,sizeof(char));
	//fprintf(stderr,"fuck=%x\n",tmp);
	
	return 0;
}*/
