/*   
 * Copyright (C) 2012 PASLab CSIE NTU. All rights reserved.
 *      - Chen Chun-Han <converse2006@gmail.com>
 */

#ifndef _M2M_H_
#define _M2M_H_


/*------------------------------------------------------------*/
/*  Includes                                                  */
/*------------------------------------------------------------*/
/*#include <stdio.h>
#include <stdlib.h>*/
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
/*------------------------------------------------------------*/
/*  Virtual Network Device                                    */
/*------------------------------------------------------------*/
#define VND_BASE_ADDR 0xf0004000
#define VND_IOMEM_SIZE 0x2000 /* 0x0000~0x0fff is used by vpd */
#define ROUTER_RFDx
#define MAX_TIME (-1)
//#define MAX_TIME (18446744073709551615)
#define SLEEP_TIME 10

typedef struct VND{
    uint32_t DeviceID;
    char DeviceType[3];
    int TotalDeviceNum;
    int Position[3];
    int Neighbors[8];
    int NeighborNum;
    long Shared_memory_address;
    uint64_t TotalTransTimes;             //Total time for transmission waiting. 
    uint64_t TotalTransSendEventTimes;    //Total count for transmission send event 
    uint64_t TotalTransRecvEventTimes;    //Total count for transmission recv event 
    uint64_t TotalPacketLossTimes;        //Total count for one-hop transmission loss
    uint64_t TotalPacketTransTimes;       //Total count fot one-hop transmission 
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
    M2M_TRANS_FAIL          =               44,
    M2M_TRANS_SUCCESS       =               77,
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

#define M2M_SHOWINFOx          //Used to show map info 

/*------------------------------------------------------------*/
/*  System V IPC shared memory                                */
/*------------------------------------------------------------*/
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define TimingModel              1   // 0: fast 1: slow


#define MAX_NODE_NUM                 100
#define MAX_NODE_SIZE                100


extern void network_initial(int id);

extern void network_exit();

extern void network_send(int dest,char data[], int len, char type[]);

extern void network_recv(int len, char type[]);




#endif
