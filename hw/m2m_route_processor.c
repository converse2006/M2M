#include <pthread.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"
#include "vpmu.h"

// ---- Globals ----
pthread_t route_processor;
long device_shm_location;

//time sync parameters
int end_count, router_count, neighbor_end; //Record neighbor node different type number
int neighbor_end_list[NODE_MAX_LINKS];     //List of neighbor node is end deivce 
int neighbor_router_list[NODE_MAX_LINKS];  //List of neighbor node is router/coordinator

extern long shm_address_location;
extern int BeforeNetworkTypeDevice[3];
extern long NODE_SHM_LOCATION[MAX_NODE_NUM]; 
extern unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM];
extern char NODE_TYPE[MAX_NODE_NUM][4];
extern VND GlobalVND;
extern long m2m_remote_hq_meta_start[NODE_MAX_LINKS];
extern long m2m_remote_hq_buffer_start[NODE_MAX_LINKS][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_hq_meta_start[NODE_MAX_LINKS + 2];
extern long m2m_hq_buffer_start[NODE_MAX_LINKS + 2][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_hq_conflag_start[MAX_NODE_NUM];

uint64_t time_sync();
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
extern long m2m_localtime_start[MAX_NODE_NUM];
//-------------dijkstra------------------
unsigned int route_discovery(int start, int end)
{ 
    dijkstra(start);

    //printD(start);

    //printf("\n");
    //printf("Path from %d to %d: ", start, end);
    clearPath();
    routePath(end);
    //printPath(start);
    //printf("\n");
    if(TimingModel == 0)
        return route_path[route_hops - 1];
    return route_path[0];

}

static void *m2m_enddevice_processor_create(void *args)
{
    int level = 2;
    uint64_t *router_localtime_ptr;
    uint64_t tmp_time;
    M2M_DBG(level, MESSAGE, "Enter m2m_route_processor_create ...");

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //update local clock
    router_localtime_ptr =(uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    while(1)
    {
                if(*router_localtime_ptr < get_vpmu_time())
                    *router_localtime_ptr = get_vpmu_time();

                usleep(SLEEP_TIME);
    }
    pthread_exit(NULL);
}

static void *m2m_route_processor_create(void *args)
{
    int level = 2;
    volatile uint64_t *router_localtime_ptr;
    volatile uint64_t tmp_router_localtime;
    volatile m2m_HQ_meta_t *meta_ptr;
    volatile m2m_HQe_t     *packet_ptr;
    M2M_DBG(level, MESSAGE, "Enter m2m_route_processor_create ...");

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //update local clock
    router_localtime_ptr  = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    if(*router_localtime_ptr != (MAX_TIME - 1))
        tmp_router_localtime = *router_localtime_ptr;
    *router_localtime_ptr = time_sync();

    M2M_DBG(level, MESSAGE, "*router_localtime_ptr = %llu",*router_localtime_ptr);
    while(1)
    {
        int search_num;
        if(!strcmp(GlobalVND.DeviceType,"ZR")) //"+ 1" for device itself packet deliver to other device
#ifndef ROUTER_RFD
            search_num = GlobalVND.NeighborNum;
#else
            search_num = GlobalVND.NeighborNum + 1;
#endif
        else
            search_num = GlobalVND.NeighborNum + 1;

        int flag = 1;
        int lock = 0;
        while(flag) //Check header queue is empty or not
        {
            int ind;
            for(ind = 0; ind < search_num; ind++)
            {
                if(ind == GlobalVND.NeighborNum)
                    meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];
                else
                    meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[ind];

                //M2M_DBG(level, MESSAGE, "[%d]m2m_hq_meta_start[%d] : %ld",GlobalVND.DeviceID, index, m2m_hq_meta_start[ind]);
                if(meta_ptr->producer != meta_ptr->consumer)
                    flag = 0;
            }

            //Header Queue is empty, update local clock(ZR)
            //NOTE: When router connected device all dead device or call recv and waiting for,
            //      we record that device local time 
            if(*router_localtime_ptr != MAX_TIME -1)
                tmp_router_localtime = *router_localtime_ptr;
            *router_localtime_ptr = time_sync();

            if(lock == 0)
            {
                M2M_DBG(level, MESSAGE, "local time = %llu",*router_localtime_ptr);
                lock++;
            }
            //FIXME this function can remove sleep function!! I GUESS!!
            usleep(SLEEP_TIME);
        }
        
        M2M_DBG(level, MESSAGE, "Header Queue exist packet!!");
        
        //Exist packet in header queue
        //FIXME MAX_TIME now equal to -1
        uint64_t packet_mintime = MAX_TIME;
        uint64_t packet_sendtime = MAX_TIME;
        unsigned int packet_from = -1;
        unsigned int packet_next = -1;
        unsigned int packet_ind = -1;
        int index;

        //Find Current Earliest Packet
        for(index = 0; index < search_num; index++)
        {   
            if(index == GlobalVND.NeighborNum)
            {
                meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];
                packet_ptr = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->consumer];
            }
            else
            {
                meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[index];
                packet_ptr = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[index][meta_ptr->consumer];
            }

            if(index == GlobalVND.NeighborNum)
            {
                M2M_DBG(level,MESSAGE,"m2m_hq_meta_start[%d] = %ld m2m_hq_buffer_start[%d] = %ld\n",NODE_MAX_LINKS + 1, \
 m2m_hq_meta_start[NODE_MAX_LINKS + 1], NODE_MAX_LINKS + 1, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->consumer]);
            }
            else
            {
                M2M_DBG(level, MESSAGE,"m2m_hq_meta_start[%d] = %ld m2m_hq_buffer_start[%d] = %ld\n", index,\
                                 m2m_hq_meta_start[index], index, m2m_hq_buffer_start[index][meta_ptr->consumer]);
            }

            if(meta_ptr->producer != meta_ptr->consumer)
            {
                if(packet_mintime > (packet_ptr->SendTime + packet_ptr->TransTime) ||
                  (packet_mintime == (packet_ptr->SendTime + packet_ptr->TransTime) 
                  && packet_sendtime > packet_ptr->SendTime) )
                {
                    packet_mintime  = packet_ptr->SendTime + packet_ptr->TransTime;
                    packet_sendtime = packet_ptr->SendTime;
                    packet_from = packet_ptr->ForwardID;
                    if(GlobalVND.DeviceID != packet_ptr->ReceiverID)
                        packet_next = route_discovery(GlobalVND.DeviceID, packet_ptr->ReceiverID);
                    else
                        packet_next = GlobalVND.DeviceID;
                    if(index == GlobalVND.NeighborNum)
                        packet_ind = NODE_MAX_LINKS + 1;
                    else
                        packet_ind = index;
                }
            }
        }
        


            M2M_DBG(level, MESSAGE,"[%d]Current earliest packet:\n Packet Index = %d\n Packet From = %d\n Packet_To = %d\n Packet Arrivel Time = %10llu\n Packet SendTime     = %10llu\n ", GlobalVND.DeviceID, packet_ind, packet_from, packet_next, packet_mintime, packet_sendtime);

        //Wait other neighbor device(w/o Sender and Next_Hop) local clock 
        //exceed (packet_mintime - one_hop_latency) 

        int deadline = 0,deadline_count = 0;
        int neigbor_device_deadline[NODE_MAX_LINKS + 2]={0};
        volatile uint64_t *loc_time;

        //NOTE: if search_num equal to 2,it mean one is sender and another is receiver
        //based on algorithm, wait for nothing!
        if(search_num > 2) 
        while(!deadline)
        {
            for(index = 0; index < search_num; index++)
            {
                //Device itself(ZC/ZR = ZED + Routing thread)
                if(index == GlobalVND.NeighborNum)
                {
                    //Check header queue due to coming header deliver time may earily than packet_mintime
                    meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];
                    if(meta_ptr->producer != meta_ptr->consumer)
                    {
                        //if exist earily than packet_mintime, reset deadline
                        packet_ptr = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->consumer];
                        if(packet_mintime > (packet_ptr->SendTime + packet_ptr->TransTime) ||
                           (packet_mintime == (packet_ptr->SendTime + packet_ptr->TransTime) 
                            && packet_sendtime > packet_ptr->SendTime) )
                        {
                            int i;
                            for(i = 0; i < NODE_MAX_LINKS + 2; i++)
                                neigbor_device_deadline[i] = 0;
                            deadline_count = 0;

                            packet_mintime = packet_ptr->SendTime + packet_ptr->TransTime;
                            packet_from = packet_ptr->ForwardID;
                            packet_sendtime = packet_ptr->SendTime;
                            if(GlobalVND.DeviceID != packet_ptr->ReceiverID)
                                packet_next = route_discovery(GlobalVND.DeviceID, packet_ptr->ReceiverID);
                            else
                                packet_next = GlobalVND.DeviceID;
                            packet_ind = NODE_MAX_LINKS + 1;
                        }
                    }

                    //if not,check device local clock and ignore packet_from/packet_next device local time
                    if(neigbor_device_deadline[NODE_MAX_LINKS + 1] != 1 && 
                       packet_from != GlobalVND.DeviceID && packet_next != GlobalVND.DeviceID)
                    {
                        loc_time = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
                        M2M_DBG(level, MESSAGE,"Device %d local clock = %llu\n ", GlobalVND.DeviceID, *loc_time);
                        //FIXME this usleep could delete!
                        usleep(SLEEP_TIME);

                        if(packet_mintime <= *loc_time && *loc_time != MAX_TIME)
                        {
                            neigbor_device_deadline[NODE_MAX_LINKS + 1] = 1;
                            deadline_count++;
                            M2M_DBG(level, MESSAGE, "Device %d local clock EXCEED!!\n ", GlobalVND.DeviceID);
                        }
                    }
                }
                else
                {
                    //Check header queue due to coming header deliver time may earily than packet_mintime
                    meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[index];
                    if(meta_ptr->producer != meta_ptr->consumer)
                    {
                        //if exist earily than packet_mintime, reset deadline
                        packet_ptr = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[index][meta_ptr->consumer];
                        if(packet_mintime > (packet_ptr->SendTime + packet_ptr->TransTime) ||
                           (packet_mintime == (packet_ptr->SendTime + packet_ptr->TransTime) 
                            && packet_sendtime > packet_ptr->SendTime) )
                        {
                            int i;
                            for(i = 0; i < NODE_MAX_LINKS + 2; i++)
                                neigbor_device_deadline[i] = 0;
                            deadline_count = 0;

                            packet_mintime = packet_ptr->SendTime + packet_ptr->TransTime;
                            packet_from = packet_ptr->ForwardID;
                            packet_sendtime = packet_ptr->SendTime;
                            if(GlobalVND.DeviceID != packet_ptr->ReceiverID)
                                packet_next = route_discovery(GlobalVND.DeviceID, packet_ptr->ReceiverID);
                            else
                                packet_next = GlobalVND.DeviceID;
                            packet_ind = index;
                        }
                    }

                    //if not,check device local clock and ignore packet_from/packet_next device local time
                    if(neigbor_device_deadline[index] != 1 && 
                       packet_from != GlobalVND.Neighbors[index] && packet_next != GlobalVND.Neighbors[index])
                    {
                        loc_time = (uint64_t *)m2m_localtime_start[GlobalVND.Neighbors[index]];
                        M2M_DBG(level, MESSAGE,"Device %d local clock = %llu\n ", GlobalVND.Neighbors[index], *loc_time);
                        //FIXME usleep could be delete!!
                        usleep(SLEEP_TIME);

                        if(packet_mintime /*- one_hop_latency(GlobalVND.Neighbors[index])*/ <= *loc_time && *loc_time != MAX_TIME)
                        {
                            neigbor_device_deadline[index] = 1;
                            deadline_count++;
                            M2M_DBG(level, MESSAGE, "Device %d local clock EXCEED!!\n ", GlobalVND.Neighbors[index]);
                        }
                    }

                }
            }
            //FIXME update local time  need more better way
            if(*router_localtime_ptr != MAX_TIME -1)
                tmp_router_localtime = *router_localtime_ptr;
            *router_localtime_ptr = time_sync();            
            if(deadline_count == search_num - 2)
                deadline = 1;
        }

        //Wait until receiver finish initialization
        M2M_DBG(level, MESSAGE, "Waiting receiver local time achieve!!!\n");
        volatile uint64_t *nextdev_time = (uint64_t *)m2m_localtime_start[packet_next];
        while(*nextdev_time == MAX_TIME)
            usleep(SLEEP_TIME);

        M2M_DBG(level, MESSAGE, "Routing Processor forward packet from %d -> %d",packet_from ,packet_next);
        int count = 0;
        int find_flag = 0;
        volatile m2m_HQ_meta_t   *FROM_meta_ptr;
        volatile m2m_HQe_t       *FROM_packet;
        volatile long                     *FROM_packet_ptr;
        volatile m2m_HQ_meta_t   *TO_meta_ptr;
        volatile long                     *TO_packet_ptr;

        if(packet_next == GlobalVND.DeviceID)
        {

            FROM_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[packet_ind];
            FROM_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];
            FROM_packet = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];

            TO_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];
            TO_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[NODE_MAX_LINKS][TO_meta_ptr->producer];

            //Check receiver header queue is full or not
            M2M_DBG(level, MESSAGE, "[CONVERSE]Waiting for [(TO_meta_ptr->consumer + 1) != TO_meta_ptr->consumer]\n");
            while(((TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM) == TO_meta_ptr->consumer)
                usleep(SLEEP_TIME);

            //NOTE: Due to packet send to device itself,just forward packet without any packet modification

            memcpy((void *)TO_packet_ptr,(void *)FROM_packet_ptr,sizeof(m2m_HQe_t) + FROM_packet->PacketSize);

            FROM_meta_ptr->consumer = (FROM_meta_ptr->consumer + 1) % HEADER_QUEUE_ENTRY_NUM;
            TO_meta_ptr->producer   = (TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM;

            //update sender/receiver local time
            volatile m2m_HQ_cf_t *hq_conflag = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[FROM_packet->SenderID];
            hq_conflag->transtime = FROM_packet->SendTime + FROM_packet->TransTime;
            hq_conflag->dataflag  = 0;
            M2M_DBG(level, MESSAGE,"hq_conflag->transtime = %llu", hq_conflag->transtime);
            M2M_DBG(level, MESSAGE,"hq_conflag->dataflag = %d", hq_conflag->dataflag);

        }
        else
        {
            while(!find_flag && count < GlobalVND.TotalDeviceNum)
            {
                if( GlobalVND.Neighbors[count] == packet_next)
                {
                    find_flag = 1;
                    break;
                }
                else
                    count++;
            }
            M2M_DBG(level, MESSAGE, "GlobalVND.Neighbors[%d] == packet_next\n", count);
            if(find_flag == 0)
            { 
                fprintf(stderr, "[m2m_post_remote_msg]: Not found receiverID in neighbor list\n");
                exit(0);
            }
            
            FROM_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[packet_ind];
            FROM_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];
            FROM_packet = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];

            TO_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_remote_hq_meta_start[count];
            TO_packet_ptr = (long *)(uintptr_t)m2m_remote_hq_buffer_start[count][TO_meta_ptr->producer];
          
            //update packet header
            FROM_packet->ForwardID = GlobalVND.DeviceID;
            //FIXME this if is really meanful? or just alway go one condition!?
            if(*router_localtime_ptr > (FROM_packet->SendTime + FROM_packet->TransTime) \
               && *router_localtime_ptr != (MAX_TIME -1))
            {
                M2M_DBG(level, MESSAGE,"*router_localtime_ptr(%llu) > (FROM_packet->SendTime + FROM_packet->TransTime)\n", *router_localtime_ptr);
                FROM_packet-> SendTime = *router_localtime_ptr;
                FROM_packet->TransTime = transmission_latency((m2m_HQe_t *)FROM_packet, packet_next, "zigbee");
                *router_localtime_ptr += FROM_packet->TransTime;
            }
            else if(*router_localtime_ptr == (MAX_TIME -1) \
                    && tmp_router_localtime > (FROM_packet->SendTime + FROM_packet->TransTime))
            {
                M2M_DBG(level, MESSAGE,"tmp_router_localtime_ptr(%llu) > (FROM_packet->SendTime + FROM_packet->TransTime)\n", tmp_router_localtime);
                FROM_packet-> SendTime = tmp_router_localtime;
                FROM_packet->TransTime = transmission_latency((m2m_HQe_t *)FROM_packet, packet_next, "zigbee");
                tmp_router_localtime += FROM_packet->TransTime;
            }
            else
            {
                FROM_packet->SendTime = FROM_packet-> SendTime + FROM_packet->TransTime;
                FROM_packet->TransTime = transmission_latency((m2m_HQe_t *)FROM_packet, packet_next, "zigbee");
                *router_localtime_ptr = FROM_packet-> SendTime + FROM_packet->TransTime;
            }

            //Check receiver header queue is full or not
            M2M_DBG(level, MESSAGE,"[CONVERSE]Waiting for [(TO_meta_ptr->producer + 1) != TO_meta_ptr->consumer]\n");
            while((TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM == TO_meta_ptr->consumer)
                usleep(SLEEP_TIME);

            memcpy((void *)TO_packet_ptr,(void *)FROM_packet_ptr,(sizeof(m2m_HQe_t) + FROM_packet->PacketSize));
            FROM_meta_ptr->consumer = (FROM_meta_ptr->consumer + 1) % HEADER_QUEUE_ENTRY_NUM;
            TO_meta_ptr->producer   = (TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM;

            //update sender/receiver local time
            if(FROM_packet->ReceiverID == packet_next && !strcmp(NODE_TYPE[packet_next], "ZED"))
            {
                M2M_DBG(level, MESSAGE,"m2m_hq_conflag_start[%d] = %ld", FROM_packet->SenderID, \
                                                                         m2m_hq_conflag_start[FROM_packet->SenderID]);
                volatile m2m_HQ_cf_t *hq_conflag = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[FROM_packet->SenderID];
                if(*router_localtime_ptr > (FROM_packet->SendTime + FROM_packet->TransTime) \
                  && *router_localtime_ptr != (MAX_TIME -1))
                        hq_conflag->transtime = *router_localtime_ptr;
                else if(*router_localtime_ptr == (MAX_TIME -1) \
                  && tmp_router_localtime > (FROM_packet->SendTime + FROM_packet->TransTime))
                    hq_conflag->transtime = tmp_router_localtime;
                else
                    hq_conflag->transtime = FROM_packet-> SendTime + FROM_packet->TransTime;

                hq_conflag->dataflag  = 0;
                M2M_DBG(level, MESSAGE,"hq_conflag->transtime = %llu", hq_conflag->transtime);
                M2M_DBG(level, MESSAGE,"hq_conflag->dataflag = %d", hq_conflag->dataflag);
                /*while(hq_conflag->dataflag)
                {
                    hq_conflag->dataflag = 0;
                    usleep(SLEEP_TIME);
                }*/
            }
        }
            M2M_DBG(level, MESSAGE, "PACKET TO/FROM:\n TO_packet_addr = %ld\n FROM_packet_addr = %ld\n Packet Size = %d\n SendTime = %llu\n TransTime = %llu\n", TO_packet_ptr, FROM_packet_ptr, FROM_packet->PacketSize, FROM_packet->SendTime, FROM_packet->TransTime);


    //Trick: 
    if(!strcmp(GlobalVND.DeviceType, "ZR"))
        GlobalVPMU.ticks = *router_localtime_ptr * (GlobalVPMU.target_cpu_frequency / 1000.0);


    }

    pthread_exit(NULL);
}

M2M_ERR_T m2m_route_processor_init()
{
    int level = 2;
    long route_process_location;
    M2M_ERR_T rc = M2M_SUCCESS;
     
    M2M_DBG(level, MESSAGE, "Enter m2m_router_processor_init() ...");

    //Calculate device metadata addr
    device_shm_location = NODE_SHM_LOCATION[GlobalVND.DeviceID];

    neighbor_end = 0;
    end_count = 0;
    router_count = 0;

    if(strcmp(GlobalVND.DeviceType, "ZED"))
    {
        route_process_location = device_shm_location;
        int ind;

        //count neighbor list
        //NOTE: ZC need to counting due to when ZED directly connect with ZC
        for(ind = 0; ind < GlobalVND.NeighborNum; ind++)
        {
            //Decide router how to sync its time (if exist end device, sync with all end device)
            if(!strcmp(NODE_TYPE[GlobalVND.Neighbors[ind]],"ZED"))
            {
                neighbor_end_list[end_count] = GlobalVND.Neighbors[ind];
                end_count++;
                if(!neighbor_end)
                      neighbor_end++;
            }
            else
            {
                neighbor_router_list[router_count] = GlobalVND.Neighbors[ind];
                router_count++;
            }
        }
#ifdef M2M_SHOWINFO
        //Check nieghbors
        fprintf(stderr, "[%d]Neighbor(End Device): ", GlobalVND.DeviceID);
        for(ind = 0; ind < end_count; ind++)
            fprintf(stderr, "%d ",neighbor_end_list[ind]);
        fprintf(stderr, "\n\n");
        fprintf(stderr, "[%d]Neighbor(Router Device): ", GlobalVND.DeviceID);
        for(ind = 0; ind < router_count; ind++)
            fprintf(stderr, "%d ",neighbor_router_list[ind]);
        fprintf(stderr, "\n\n");
#endif
        //Create routing thread
        rc = pthread_create(&route_processor,NULL,m2m_route_processor_create,NULL);
    }
    else
    {
        //End device need to get vpmu time and update local time on shared memory
        rc = pthread_create(&route_processor,NULL,m2m_enddevice_processor_create,NULL);
    }

    M2M_DBG(level, MESSAGE, "Exit m2m_router_processor_init() ...");
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_route_processor_exit()
{
    int level = 2;
    M2M_DBG(level, MESSAGE, "Enter m2m_router_processor_exit() ...");
    
    neighbor_end = 0;
    end_count =0;
    router_count = 0;

    //cancel the service processor
    pthread_cancel(route_processor);
    pthread_join(route_processor,NULL);

    M2M_DBG(level, MESSAGE, "Exit m2m_router_processor_exit() ...");
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


