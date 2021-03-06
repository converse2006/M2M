/* 
 * Copyright (C) 2012 PASLab CSIE NTU. All rights reserved.
 *      - Chen Chun-Han <converse2006@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"

/* global variables--------------*/
int shmid;
long shm_address;
long shm_address_location;
unsigned long long SHM_SIZE;

extern int TotalNetworkTypeDevice[3];
extern VND GlobalVND;

M2M_ERR_T m2m_mm_init()
{
    int level = 4;
    M2M_DBG(level, MESSAGE, "Enter m2m_mm_init() ...");
    M2M_ERR_T rc = M2M_SUCCESS;
    key_t key = x86_SHM_KEY; 

    SHM_SIZE = TotalNetworkTypeDevice[0] * COORDINATOR_SIZE + \
               TotalNetworkTypeDevice[1] * ROUTER_SIZE + \
               TotalNetworkTypeDevice[2] * END_DEVICE_SIZE + \
               MAX_NODE_NUM * sizeof(uint64_t);   
     
    M2M_DBG(level, MESSAGE, "COORDINATOR_SIZE         = %d", COORDINATOR_SIZE);
    M2M_DBG(level, MESSAGE, "ROUTER_SIZE              = %d", ROUTER_SIZE);
    M2M_DBG(level, MESSAGE, "END_DEVICE_SIZE          = %d", END_DEVICE_SIZE);
    M2M_DBG(level, MESSAGE, "Total shared memory size = %llu", SHM_SIZE);

    // get the shared memory id
    if(GlobalVND.DeviceID == 1)
    {
        if((shmid = shmget(key, SHM_SIZE + 1000, IPC_CREAT | 0666)) < 0)
        {
            perror("shmget failed");
            exit(-1);
        }
    }
    else
    {
        if((shmid = shmget(key, SHM_SIZE + 1000, 0666)) < 0)
        {
            perror("shmget failed");
            exit(-1);
        }
    }


    // Attach the segment to data space
    if((shm_address = shmat(shmid,NULL,0)) == (char *) -1) 
    {
        perror("shmat failed");
        exit(-1);
    }


    shm_address_location = (long)shm_address;
    M2M_DBG(level, MESSAGE,"Shared Memory Start addr = %lx \n", shm_address_location);

    M2M_DBG(level, MESSAGE, "Exit m2m_mm_init() ...");
    return rc;
}

M2M_ERR_T m2m_mm_exit()
{
    int level = 4;
    int errno;
    struct shmid_ds *shmid_ds=NULL;
    M2M_DBG(level, MESSAGE, "Enter m2m_mm_exit() ...");

    if(!strcmp(GlobalVND.DeviceType, "ZC"))
    {

        // Detach the shared memory
        if((errno = shmdt((void *)shm_address)) == -1)
        {
            perror("shmdt failed");
            exit(-1);
        }
        if((errno = shmctl(shmid,IPC_RMID,shmid_ds)) == -1)
        {
            perror("shmctl failed");
            exit(-1);
        }

    }

    M2M_DBG(level, MESSAGE, "Exit m2m_mm_exit() ...");
    return M2M_SUCCESS;
}
