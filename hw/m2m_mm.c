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
    int level = 2;
    M2M_ERR_T rc = M2M_SUCCESS;
    key_t key = x86_SHM_KEY; 
    M2M_DBG(level, GENERAL, "COORDINATOR_SIZE = %d", COORDINATOR_SIZE);
    M2M_DBG(level, GENERAL, "ROUTER_SIZE = %d", ROUTER_SIZE);
    M2M_DBG(level, GENERAL, "END_DEVICE_SIZE = %d", END_DEVICE_SIZE);

    SHM_SIZE = TotalNetworkTypeDevice[0] * COORDINATOR_SIZE + \
               TotalNetworkTypeDevice[1] * ROUTER_SIZE + \
               TotalNetworkTypeDevice[2] * END_DEVICE_SIZE;   
     
    M2M_DBG(level, GENERAL, "Total shared memory size = %d", SHM_SIZE);
    // get the shared memory id
    if((shmid = shmget(key,SHM_SIZE,IPC_CREAT | 0666)) < 0)
    {perror("shmget failed");   exit(-1);}

    // Attach the segment to data space
    if((shm_address = shmat(shmid,NULL,0)) == (char *) -1) 
    {perror("shmat failed");    exit(-1);}

    shm_address_location = (long)shm_address;
    //fprintf(stderr, "shm_address_location = %d \n", shm_address_location);

    return rc;
}

M2M_ERR_T m2m_mm_exit()
{
    int level = 2;
    int errno;
    struct shmid_ds *shmid_ds=NULL;
    //if(!strcmp(GlobalVND.DeviceType, "ZC"))
    {
        //M2M_DBG(level, GENERAL, "Coordinator in m2m_mm_exit");

        // Detach the shared memory
        if((errno = shmdt((void *)shm_address)) == -1)
            {perror("shmdt failed");    exit(-1);}
        if((errno = shmctl(shmid,IPC_RMID,shmid_ds)) == -1)
        {perror("shmctl failed");    exit(-1);}

        printf("Remove the shared region\n");
    }
    /*else
        M2M_DBG(level, GENERAL, "Others in m2m_mm_exit");*/
    return M2M_SUCCESS;
}
