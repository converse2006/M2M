#include <stdio.h>
#include <stdlib.h>
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

#ifdef M2M_LOGFILE
FILE *routeoutFile;
#endif

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
    volatile uint64_t *router_localtime_ptr;
    M2M_DBG(level, MESSAGE, "Enter m2m_route_processor_create ...");

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //update local clock
    volatile uint64_t *action_probe;
    action_probe = (uint64_t *)m2m_localtime_start[0];
    while((*action_probe) != 1);

    M2M_DBG(level, MESSAGE, "End device VPMU start working ...");
    router_localtime_ptr =(uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    while(1)
    {
              //  if(*router_localtime_ptr < get_vpmu_time())
              //      *router_localtime_ptr = get_vpmu_time();
    }
    pthread_exit(NULL);
}

static void *m2m_route_processor_create(void *args)
{
    int level = 2;
    int index;
    volatile uint64_t *router_localtime_ptr;
    volatile uint64_t router_localtime;
    volatile m2m_HQ_meta_t *meta_ptr;
    volatile m2m_HQe_t     *packet_ptr;
    M2M_DBG(level, MESSAGE, "Enter m2m_route_processor_create ...");

#ifdef M2M_LOGFILE
    char location[100] = "NetLogs/Routedevice";
    char post[]= ".txt";
    char devicenum[20];
    sprintf(devicenum, "%d", GlobalVND.DeviceID);
    strcat( location, devicenum);
    strcat( location, post);
    M2M_DBG(level, GENERAL, "Logging file at %s\n", location);
    routeoutFile = fopen(location, "w");
    if(routeoutFile == NULL)
    {
        fprintf(stderr, "Logging File open error!\n");
        exit(0);
    }
#endif

    //enable async cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //NOTE: routing process have its own local time
    //promotion time by packet
    router_localtime_ptr  = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    if(!strcmp(GlobalVND.DeviceType, "ZR"))
        *router_localtime_ptr = time_sync();
    //NOTE: thie time index promote by packet
    router_localtime = 0;

    M2M_DBG(level, MESSAGE, "*router_localtime_ptr = %llu",*router_localtime_ptr);

    int search_num = 0;
    if(!strcmp(GlobalVND.DeviceType,"ZR")) //"+ 1" for device itself packet deliver to other device
    {
#ifndef ROUTER_RFD
        search_num = GlobalVND.NeighborNum;
#else
        search_num = GlobalVND.NeighborNum + 1;
#endif
    }
    else
        search_num = GlobalVND.NeighborNum + 1;

    while(1)
    {

        M2M_DBG(level, MESSAGE, "[WHILE]Router waiting for header, now header queue is empty");
        int flag = 1;
        while(flag != 0) //Check header queue is empty or not
        {
            for(index = 0; index < GlobalVND.NeighborNum; index++)
            {
                meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[index];
                if(meta_ptr->producer != meta_ptr->consumer)
                    flag = 0;
            }

            if(flag != 0)
            {
                meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];
                if(meta_ptr->producer != meta_ptr->consumer)
                    flag = 0;
            }


            //Header Queue is empty, update local clock(ZR)
            //NOTE: When router connected device all dead device or call recv and waiting for,
            //      we record that device local time 

            //TODO This part need to check!! Router/Coordinator
            if(!strcmp(GlobalVND.DeviceType,"ZR"))
                *router_localtime_ptr = time_sync();

        }
        
        M2M_DBG(level, MESSAGE, "Header Queue exist packet!!");
        
        //Exist packet in header queue
        //FIXME MAX_TIME now equal to -1
        uint64_t packet_mintime = MAX_TIME;
        uint64_t packet_sendtime = MAX_TIME;
        unsigned int packet_from = -1;
        unsigned int packet_next = -1;
        volatile unsigned int packet_ind = -1;

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
#ifdef M2M_SHOWINFO
            if(index == GlobalVND.NeighborNum)
            {
                M2M_DBG(level,MESSAGE,"m2m_hq_meta_start[%d] = %lx m2m_hq_buffer_start[%d] = %lx\n",NODE_MAX_LINKS + 1, \
 m2m_hq_meta_start[NODE_MAX_LINKS + 1], NODE_MAX_LINKS + 1, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->consumer]);
            }
            else
            {
                M2M_DBG(level, MESSAGE,"m2m_hq_meta_start[%d] = %lx m2m_hq_buffer_start[%d] = %lx\n", index,\
                                 m2m_hq_meta_start[index], index, m2m_hq_buffer_start[index][meta_ptr->consumer]);
            }
#endif
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
        int neigbor_device_deadline[NODE_MAX_LINKS + 2];
        for(index = 0; index < NODE_MAX_LINKS + 2; index++)
            neigbor_device_deadline[index] = 0;
        volatile uint64_t *neighbor_localtime;

        //NOTE: if search_num equal to 2,it mean one is sender and another is receiver
        //based on algorithm, wait for nothing!
        M2M_DBG(level, MESSAGE, "[WHILE]Router waiting for other device time later than current packet time");
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
                        neighbor_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
                        M2M_DBG(level, MESSAGE,"Device %d local clock = %llu", GlobalVND.DeviceID, *neighbor_localtime);
                        //FIXME this usleep could delete!
                        //usleep(SLEEP_TIME);

                        if(packet_mintime <= *neighbor_localtime)
                        {
                            neigbor_device_deadline[NODE_MAX_LINKS + 1] = 1;
                            deadline_count++;
                            M2M_DBG(level, MESSAGE, "Device %d local clock EXCEED!!", GlobalVND.DeviceID);
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
                        neighbor_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.Neighbors[index]];
                        M2M_DBG(level, MESSAGE,"Device %d local clock = %llu\n ", \
                                                GlobalVND.Neighbors[index], *neighbor_localtime);

                        if(packet_mintime /*- one_hop_latency(GlobalVND.Neighbors[index])*/ <= *neighbor_localtime)
                        {
                            neigbor_device_deadline[index] = 1;
                            deadline_count++;
                            M2M_DBG(level, MESSAGE, "Device %d local clock EXCEED!!\n ", GlobalVND.Neighbors[index]);
                        }
                    }

                }
            }
            //FIXME update local time  need more better way
            if(!strcmp(GlobalVND.DeviceType,"ZR"))
                *router_localtime_ptr = time_sync();            
            if(deadline_count == search_num - 2)
                deadline = 1;
        }

        M2M_DBG(level, MESSAGE, "Routing Processor forward packet from %d -> %d",packet_from ,packet_next);
        volatile int count = 0;
        int find_flag = 0;
        int transfail = 0;
        volatile m2m_HQ_meta_t   *FROM_meta_ptr;
        volatile m2m_HQe_t       *FROM_packet;
        volatile long            *FROM_packet_ptr;
        volatile m2m_HQ_meta_t   *TO_meta_ptr;
        volatile long            *TO_packet_ptr;

        M2M_DBG(level, MESSAGE,"[%d]The earliest packet:\n Packet Index = %d\n Packet From = %d\n Packet_To = %d\n Packet Arrivel Time = %10llu\n Packet SendTime     = %10llu\n ", GlobalVND.DeviceID, packet_ind, packet_from, packet_next, packet_mintime, packet_sendtime);

#ifdef M2M_LOGFILE
        FROM_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[packet_ind];
        FROM_packet = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];

        char logtext[200] = "[Route] ";
        char tmptext[100];
        sprintf(tmptext,"SenderID: %3d ", FROM_packet->SenderID);
        strcat(logtext, tmptext);
        sprintf(tmptext,"ForwardID: %3d ", GlobalVND.DeviceID);
        strcat(logtext, tmptext);
        sprintf(tmptext,"ReceiverID: %3d ", FROM_packet->ReceiverID);
        strcat(logtext, tmptext);
        sprintf(tmptext,"PacketSize: %4d ", FROM_packet->PacketSize);
        strcat(logtext, tmptext);
        sprintf(tmptext,"ArrivalTime: %15llu ",(FROM_packet->SendTime + FROM_packet->TransTime));
        strcat(logtext, tmptext);
        sprintf(tmptext,"ReceiveTime: %15llu\n", router_localtime);
        strcat(logtext, tmptext);
        fputs(logtext, routeoutFile);
#endif


        if(packet_next == GlobalVND.DeviceID)
        {

            FROM_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[packet_ind];
            FROM_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];
            FROM_packet = (m2m_HQe_t *)(uintptr_t)m2m_hq_buffer_start[packet_ind][FROM_meta_ptr->consumer];

            TO_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];
            TO_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[NODE_MAX_LINKS][TO_meta_ptr->producer];


            //NOTE: Due to packet send to device itself,just forward packet without any packet modification
            //But Arrival time need to depend on router receive time to change transmission time
            if(router_localtime > (FROM_packet-> SendTime + FROM_packet->TransTime))
                FROM_packet->TransTime += router_localtime - (FROM_packet-> SendTime + FROM_packet->TransTime);
            else
                router_localtime = (FROM_packet-> SendTime + FROM_packet->TransTime);
            if(FROM_packet->EntryNum == -1) //Small Communication Scheme
            {
                //Check receiver header queue is full or not
                M2M_DBG(level, MESSAGE,"[CONVERSE]Waiting for [(TO_meta_ptr->consumer + 1) != TO_meta_ptr->consumer]\n");
                while(((TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM) == TO_meta_ptr->consumer){}
                memcpy((void *)TO_packet_ptr, (void *)FROM_packet_ptr, sizeof(m2m_HQe_t));
                TO_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[NODE_MAX_LINKS][(TO_meta_ptr->producer + 1)%HEADER_QUEUE_ENTRY_NUM];
                FROM_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[packet_ind][(FROM_meta_ptr->consumer + 1)%HEADER_QUEUE_ENTRY_NUM];
                while(((TO_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM) == TO_meta_ptr->consumer){}
                memcpy((void *)TO_packet_ptr, (void *)FROM_packet_ptr, FROM_packet->PacketSize);

                M2M_DBG(level, MESSAGE, "[%d]Before FROM_consumer(%d) = %d", \
                        GlobalVND.DeviceID, FROM_packet_ptr, FROM_meta_ptr->consumer);
                M2M_DBG(level, MESSAGE, "[%d]Before TO_producer(%d = %d", \
                        GlobalVND.DeviceID, TO_packet_ptr, TO_meta_ptr->producer);

                FROM_meta_ptr->consumer = ((FROM_meta_ptr->consumer + 2) % HEADER_QUEUE_ENTRY_NUM);
                TO_meta_ptr->producer   = ((TO_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM);
    
                M2M_DBG(level, MESSAGE, "[%d]After FROM_consumer(%d) = %d", \
                        GlobalVND.DeviceID, FROM_packet_ptr, FROM_meta_ptr->consumer);
                M2M_DBG(level, MESSAGE, "[%d]After TO_producer(%d = %d", \
                        GlobalVND.DeviceID, TO_packet_ptr, TO_meta_ptr->producer);
            }
            else //Large Communication Scheme
            {
                //FIXME
            }



            //Update sender/receiver local time
            volatile m2m_HQ_cf_t *hq_conflag = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[FROM_packet->SenderID];
            hq_conflag->transtime = router_localtime;
            hq_conflag->dataflag  = 0;
            M2M_DBG(level, MESSAGE,"hq_conflag->transtime = %llu", hq_conflag->transtime);
            M2M_DBG(level, MESSAGE,"hq_conflag->dataflag = %d", hq_conflag->dataflag);

        }
        else
        {
            while(!find_flag && count < GlobalVND.TotalDeviceNum)
            {
                if( GlobalVND.Neighbors[count] == packet_next)
                    find_flag = 1;
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
          
            //Update packet header
            FROM_packet->ForwardID = GlobalVND.DeviceID;

            //FIXME ZR how to fix when ZR are FFD will send data 
            if(router_localtime > (FROM_packet-> SendTime + FROM_packet->TransTime))
                FROM_packet-> SendTime = router_localtime;
            else
            {
                router_localtime = FROM_packet-> SendTime + FROM_packet->TransTime;
                if(FROM_packet->SenderID != GlobalVND.DeviceID)
                    FROM_packet->SendTime = FROM_packet-> SendTime + FROM_packet->TransTime;
            }
            uint64_t fail_latency;
            if(FROM_packet->SenderID != GlobalVND.DeviceID)
                FROM_packet->TransTime = transmission_latency((m2m_HQe_t *)FROM_packet, packet_next, &fail_latency, "zigbee");
            M2M_DBG(level, MESSAGE,"FROM_packet->TransTime = %d",FROM_packet->TransTime);
            
            //When transmission time calculate is fail transmission,
            //router need to signal original sender that transmission fail
            if(FROM_packet->TransTime == 0)
            {
                M2M_DBG(level, MESSAGE,"transmission fail");
                transfail = 1;
                if(FROM_packet->EntryNum == -1) //Small Communication Scheme
                {
                    //Update sender control flag to index trans fail
                    volatile m2m_HQ_cf_t *hq_conflag = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[FROM_packet->SenderID];
                    hq_conflag->transtime = FROM_packet-> SendTime + &fail_latency;
                    router_localtime = FROM_packet-> SendTime + &fail_latency;
                    hq_conflag->dataflag  = 44; //Signal sender transmission fail
                    M2M_DBG(level, MESSAGE,"hq_conflag->dataflag = %d", hq_conflag->dataflag);
                    M2M_DBG(level, MESSAGE,"hq_conflag->transtime = %d",hq_conflag->transtime);
                    
                    //Remove header in header queue
                    FROM_meta_ptr->consumer = (FROM_meta_ptr->consumer + 2) % HEADER_QUEUE_ENTRY_NUM;
                }
                else //Large Communication Scheme
                {
                    //FIXME
                }

            }
            else
            {
                router_localtime = FROM_packet-> SendTime + FROM_packet->TransTime;
            }
            if(transfail != 1) {

            if(FROM_packet->EntryNum == -1) //Small Communication Scheme
            {
                //Check receiver header queue is full or not
                M2M_DBG(level, MESSAGE, "[WHILE]Router waiting for receiver header queue is full");
                while(((TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM) == TO_meta_ptr->consumer){}
                memcpy((void *)TO_packet_ptr, (void *)FROM_packet_ptr, sizeof(m2m_HQe_t));
                TO_packet_ptr = (long *)(uintptr_t)m2m_remote_hq_buffer_start[count][((TO_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM)];
                FROM_packet_ptr = (long *)(uintptr_t)m2m_hq_buffer_start[packet_ind][(FROM_meta_ptr->consumer + 1)%HEADER_QUEUE_ENTRY_NUM];
                while(((TO_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM) == TO_meta_ptr->consumer){}
                memcpy((void *)TO_packet_ptr, (void *)FROM_packet_ptr, FROM_packet->PacketSize);
   

                M2M_DBG(level, MESSAGE, "[%d]Before FROM_consumer(%lx) = %d", GlobalVND.DeviceID, FROM_packet_ptr, FROM_meta_ptr->consumer);
                M2M_DBG(level, MESSAGE, "[%d]Before TO_producer(%lx = %d", GlobalVND.DeviceID, TO_packet_ptr, TO_meta_ptr->producer);

                FROM_meta_ptr->consumer = (FROM_meta_ptr->consumer + 2) % HEADER_QUEUE_ENTRY_NUM;
                TO_meta_ptr->producer   = (TO_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM;

                M2M_DBG(level, MESSAGE, "[%d]After FROM_consumer(%lx) = %d", GlobalVND.DeviceID, FROM_packet_ptr, FROM_meta_ptr->consumer);
                M2M_DBG(level, MESSAGE, "[%d]After TO_producer(%lx) = %d", GlobalVND.DeviceID, TO_packet_ptr, TO_meta_ptr->producer);
            }
            else //Large Communication Scheme
            {
                //FIXME
            }
            //Update sender/receiver local time
            if(FROM_packet->ReceiverID == packet_next && !strcmp(NODE_TYPE[packet_next], "ZED"))
            {
                M2M_DBG(level, MESSAGE,"m2m_hq_conflag_start[%d] = %lx", FROM_packet->SenderID, \
                                                                         m2m_hq_conflag_start[FROM_packet->SenderID]);
                volatile m2m_HQ_cf_t *hq_conflag = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[FROM_packet->SenderID];
                hq_conflag->transtime = router_localtime;
                hq_conflag->dataflag  = 0;
                M2M_DBG(level, MESSAGE,"hq_conflag->transtime = %llu", hq_conflag->transtime);
                M2M_DBG(level, MESSAGE,"hq_conflag->dataflag = %d", hq_conflag->dataflag);
            }

            } //if(transfail != 1)
        }

        if(transfail != 1)
        {
#ifdef M2M_LOGFILE
        char logtext[200] = "[Route] Deliver Success ";
        char tmptext[100];
        sprintf(tmptext,"Receiver receive Time: %15llu\n", router_localtime);
        strcat(logtext, tmptext);
        fputs(logtext, routeoutFile);
#endif
            M2M_DBG(level, MESSAGE, "PACKET TO/FROM:\n TO_packet_addr = %lx\n FROM_packet_addr = %lx\n Packet Size = %d\n SendTime = %llu\n TransTime = %llu\n", TO_packet_ptr, FROM_packet_ptr, FROM_packet->PacketSize, FROM_packet->SendTime, FROM_packet->TransTime);
        }
        else
        {
#ifdef M2M_LOGFILE
        char logtext[200] = "[Route] Deliver Fail ";
        char tmptext[100];
        sprintf(tmptext,"Router local Time: %15llu\n", router_localtime);
        strcat(logtext, tmptext);
        fputs(logtext, routeoutFile);
#endif
            M2M_DBG(level, MESSAGE, "PACKET FROM %d TO %d Transmission Fail on %d", FROM_packet->SenderID, FROM_packet->ReceiverID, FROM_packet->ForwardID);
        }



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
        //rc = pthread_create(&route_processor,NULL,m2m_enddevice_processor_create,NULL);
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
    //FIXME how to handle exit function 
    //when connected device not finish yet?

    //cancel the service processor
    if(strcmp(GlobalVND.DeviceType, "ZED"))
    {
#ifdef M2M_LOGFILE
        fclose(routeoutFile);
#endif
        pthread_cancel(route_processor);
        pthread_join(route_processor,NULL);
    }

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


