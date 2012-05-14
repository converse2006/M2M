#include <pthread.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"
#include "vpmu.h"

// ---- Globals ----
pthread_t route_processor;
long device_shm_location;
extern long shm_address_location;
extern int BeforeNetworkTypeDevice[3];
extern long NODE_SHM_LOCATION[MAX_NODE_NUM]; 
extern unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM];
extern VND GlobalVND;
extern long shm_address_location;

//-------------dijkstra------------------
#include "dijkstra.h"
#define GRAPHSIZE MAX_NODE_NUM
#define MAXSIZE GRAPHSIZE*GRAPHSIZE
#define MAX(a, b) ((a > b) ? (a) : (b))

int route_path[40];
int route_hops;
int e; /* The number of nonzero edges in the graph */
//int n = GlobalVND.TotalDeviceNum; /* The number of nodes in the graph */
long dist[GRAPHSIZE][GRAPHSIZE]; /* dist[i][j] is the distance between node i and j; or 0 if there is no direct connection */
long d[GRAPHSIZE]; /* d[i] is the length of the shortest path between the source (s) and node i */
int prev[GRAPHSIZE]; /* prev[i] is the node that comes right before i in the shortest path from the source to i*/
//-------------dijkstra------------------

int route_discovery(int start, int end)
{ 
    dijkstra(start);

    printD(start);

    printf("\n");
    printf("Path from %d to %d: ", start, end);
    clearPath();
    routePath(end);
    printPath(start);
    printf("\n");
    if(TimingModel == 0)
        return route_path[route_hops - 1];
    return route_path[0];

}

uint64_t get_vpmu_time()
{
    uint64_t time = (vpmu_cycle_count() - GlobalVPMU.last_ticks)
                       / (GlobalVPMU.target_cpu_frequency / 1000.0);
    return time;
}

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

// ======================= Inline function =================================
/*
 * Prints the shortest path from the source to dest.
 *
 * dijkstra(int) MUST be run at least once BEFORE
 * this is called
 */

void printD(int id) {
    int i;

    printf("Distances (From %d):\n",id);
    for (i = 1; i <= GlobalVND.TotalDeviceNum; ++i)
        printf("%10d", i);
    printf("\n");
    for (i = 1; i <= GlobalVND.TotalDeviceNum; ++i) {
        printf("%10ld", d[i]);
    }
    printf("\n");
}


void clearPath()
{
    route_hops = -1;
}

void routePath(int dest) {
    if (prev[dest] != -1)
        routePath(prev[dest]);
    route_path[route_hops] = dest;
    route_hops++;
}

void printPath(int start_node)
{
    int x;
    printf("hops = %d\n",route_hops);
    printf("%3d ", start_node);
    for(x = 0; x < route_hops; x++)
    {
        printf("-> %3d ",route_path[x]);
    }
}

void dijkstra(int s) {
    int i, k, mini;
    int visited[GRAPHSIZE];

    for (i = 1; i <= GlobalVND.TotalDeviceNum; ++i) {
        d[i] = MAXSIZE;
        prev[i] = -1; /* no path has yet been found to i */
        visited[i] = 0; /* the i-th element has not yet been visited */
    }

    d[s] = 0;

    for (k = 1; k <= GlobalVND.TotalDeviceNum; ++k) {
        mini = -1;
        for (i = 1; i <= GlobalVND.TotalDeviceNum; ++i)
            if (!visited[i] && ((mini == -1) || (d[i] < d[mini])))
                mini = i;

        visited[mini] = 1;

        for (i = 1; i <= GlobalVND.TotalDeviceNum; ++i)
            if (NODE_MAP[mini][i])
                if (d[mini] + NODE_MAP[mini][i] < d[i]) {
                    d[i] = d[mini] + NODE_MAP[mini][i];
                    prev[i] = mini;
                }
    }
}

