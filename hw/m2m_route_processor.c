#include <pthread.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"

// ---- Globals ----
pthread_t route_processor;

extern char* shm_address_location;
extern int BeforeNetworkTypeDevice[3];
M2M_ERR_T m2m_route_processor_init()
{
    int level = 2;
    M2M_ERR_T rc = M2M_SUCCESS;
    long device_location = (COORDINATOR_SIZE * BeforeNetworkTypeDevice[0]) + \
                           (ROUTER_SIZE * BeforeNetworkTypeDevice[1]) + \
                           (END_DEVICE_SIZE * BeforeNetworkTypeDevice[2]);
    M2M_DBG(level, GENERAL, "Device shm location offset = %d",device_location);
    return rc;
}
