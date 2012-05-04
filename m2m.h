
#ifndef _M2M_H_
#define _M2M_H_


/*------------------------------------------------------------*/
/*  Includes                                                  */
/*------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
/*------------------------------------------------------------*/
/*  Virtual Network Device                                    */
/*------------------------------------------------------------*/
#define VND_BASE_ADDR 0xe0000000
#define VND_IOMEM_SIZE 0x2000 /* 0x0000~0x0fff is used by vpd */
typedef struct VND{
    uint32_t DeviceID;
    uint64_t ND_power;
    uint64_t ND_localclock;
}VND;

typedef enum
{
    M2M_SUCCESS             =               1,
}M2M_ERR_T;

//Special Write 
#define NET_DIST               0x000C
#define ZIGBEE                 0x0000
#define BLUETOOTH              0x0001
#define NETWORK_INITIALIZATION 0x0000
#define NETWORK_SEND           0x0004
#define NETWORK_RECV           0x0008

//Debugging
#define M2MDEBUG 
#define DEBUG_MSG
#ifdef M2MDEBUG
#include <stdio.h>
#include <assert.h>
#define M2M_DEBUG_LEVEL 2

#define M2M_DBG(level, CATEGORY, str, ...) \
    M2M_DEBUG_##CATEGORY(level, str, ##__VA_ARGS__)

#define M2M_DEBUG_MESSAGE(level, str, ...) \
    if(level <= M2M_DEBUG_LEVEL) \
        fprintf(stderr, "+[M2M DBG] %s:%.3d: " str "\n", __FILE__,__LINE__, ##__VA_ARGS__);

#define M2M_DEBUG_GENERAL(level, str, ...) \
    if(level <= M2M_DEBUG_LEVEL) \
        fprintf(stderr, "+[M2M DBG] " str "\n", ##__VA_ARGS__);

#define M2M_DEBUG_ASSERT(level, str, ...) \
    if (level <= M2M_DEBUG_LEVEL) \
        assert((str));
#else

#define M2M_DBG(level, CATEGORY, str, ...)
#define M2M_DEBUG_MESSAGE(level, str, ...)
#define M2M_DEBUG_GENERAL(level, str, ...)
#define M2M_DEBUG_ASSERT(level, str, ...)

#endif
/*------------------------------------------------------------*/
/*  System V IPC shared memory                                */
/*------------------------------------------------------------*/
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define TimingModel              0   // 0: fast 1: slow
//#define NODE_NUM                 5
#define SHM_KEY                  5566
#define METADATA_PER_NODE        40

#define DATA_BUFFER_ENTRY_SIZE   100  
#define DATA_BUFFER_ENTRY_NUM    50
#define DATA_BUFFER_SIZE   (DATA_BUFFER_ENTRY_SIZE * DATA_BUFFER_ENTRY_NUM)
#define DATA_BUFFER_METADATA     DATA_BUFFER_ENTRY_SIZE //bit

#define HEADER_QUEUE_ENTRY_SIZE  22
#define HEADER_QUEUE_ENTRY_NUM   10
#define HEADER_QUEUE_SIZE  (HEADER_QUEUE_ENTRY_SIZE * HEADER_QUEUE_ENTRY_SIZE)
#define HEADER_QUEUE_METADATA    2 //unsigned char
//#define SHM_SIZE                 1057152
//NODE_NUM * (DATA_BUFFER_PER_NODE + (NODE_NUM - 1) * HEADER_QUEUE_PER_NODE + METADATA_PER_NODE )
#define MAX_NODE_NUM             10
#define MAX_NODE_SIZE            100



unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM];
unsigned int NODE_LOCATION[MAX_NODE_NUM][3];

long shm_address_location;
typedef struct node_info {
    int id;
    char role[3];
    int location[3];
    int link[8];
    int linknum;
} node_info_t;
node_info_t node;

extern void network_initial(int id);

extern void network_exit();

extern void network_send(int dest,char data[], int len, char type[]);

extern void network_recv(int len, char type[]);

#endif
