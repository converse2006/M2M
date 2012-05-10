#include <pthread.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"

// ---- Globals ----
pthread_t route_processor;
long device_shm_location;
extern long shm_address_location;
extern int BeforeNetworkTypeDevice[3];
extern long NODE_SHM_LOCATION[MAX_NODE_NUM]; 
extern VND GlobalVND;
extern long shm_address_location;


static void *m2m_route_processor_create(void *args)
{
    int level = 2;
    M2M_DBG(level, GENERAL, "In m2m_route_processor_create ...");

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(1)
    {
        //FIXME route algorithm implement
    }

    pthread_exit(NULL);
}

M2M_ERR_T m2m_route_processor_init()
{
    long route_process_location;
    long hq_meta_base[NODE_MAX_LINKS];
    long hq_data_base[NODE_MAX_LINKS];
    int level = 2;
     
    m2m_HQ_meta_t *p = NULL;
    M2M_ERR_T rc = M2M_SUCCESS;
    //NODE_SHM_LOCATION + base shm address
    int ind;
    for( ind = 1; ind <= GlobalVND.TotalDeviceNum; ind++)
        NODE_SHM_LOCATION[ind] += shm_address_location;

    //Calculate device metadata addr
    device_shm_location = NODE_SHM_LOCATION[GlobalVND.DeviceID];

    /*(COORDINATOR_SIZE * BeforeNetworkTypeDevice[0]) + \
      (ROUTER_SIZE * BeforeNetworkTypeDevice[1]) + \
      (END_DEVICE_SIZE * BeforeNetworkTypeDevice[2]);*/
    if(strcmp(GlobalVND.DeviceType, "ZED"))
    {
        route_process_location = device_shm_location;
        int ind;
        for(ind = 0; ind < NODE_MAX_LINKS ; ind++)
        {
            hq_meta_base[ind] = route_process_location + HEADER_QUEUE_SIZE * NODE_MAX_LINKS \
                                + ind * sizeof(unsigned int) * 2;
            hq_data_base [ind] = route_process_location + ind * HEADER_QUEUE_SIZE;
            p = (m2m_HQ_meta_t *)(uintptr_t)hq_meta_base[ind];
            p->producer = 0;
            p->consumer = 0;
            //M2M_DBG(level, GENERAL, "hq_meta_base[%d] = %d ", ind, hq_meta_base[ind]);
            //M2M_DBG(level, GENERAL, "hq_data_base[%d] = %d ", ind, hq_data_base[ind]);
        }
        rc = pthread_create(&route_processor,NULL,m2m_route_processor_create,NULL);
    }

    M2M_DBG(level, GENERAL, "Device shm location offset = %d",device_shm_location);
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_route_processor_exit()
{
    if(strcmp(GlobalVND.DeviceType, "ZED"))
    {
        //cancel the service processor
        pthread_cancel(route_processor);
        pthread_join(route_processor,NULL);
    }

    return M2M_SUCCESS;
}
