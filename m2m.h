
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
#define ROUTER_RFDx

typedef struct VND{
    uint32_t DeviceID;
    char DeviceType[3];
    int TotalDeviceNum;
    int Position[3];
    int Neighbors[8];
    int NeighborNum;
    long Shared_memory_address;
    uint64_t ND_power;
    uint64_t ND_localclock;
}VND;

/*typedef struct DeviceStruct{
    char DeviceType[3];
    long shm_location;
    int Neighbors[8];
}DeviceStruct;*/

typedef enum
{
    M2M_ERROR               =               0,
    M2M_SUCCESS             =               1,
    M2M_TRANS_NOT_READY     =               8,
}M2M_ERR_T;

//Special Write 
#define NET_DIST               0x0010
#define ZIGBEE                 0x0000
#define BLUETOOTH              0x0001
#define NETWORK_INIT           0x0000
#define NETWORK_SEND           0x0004
#define NETWORK_RECV           0x0008
#define NETWORK_EXIT           0x000C

//Debugging
#define M2MDEBUG 
#define DEBUG_M2M
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

#define TimingModel              1   // 0: fast 1: slow


#define MAX_NODE_NUM                 10
#define MAX_NODE_SIZE                100


extern void network_initial(int id);

extern void network_exit();

extern void network_send(int dest,char data[], int len, char type[]);

extern void network_recv(int len, char type[]);




#endif
