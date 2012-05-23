#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "vpmu.h"
//time sync parameters
extern int end_count, router_count, neighbor_end;
extern int neighbor_end_list[NODE_MAX_LINKS];
extern int neighbor_router_list[NODE_MAX_LINKS];

extern uint64_t *m2m_localtime;
extern long m2m_localtime_start[MAX_NODE_NUM];
extern VND GlobalVND;
static inline int ack_retry(int node_from, int node_to);

M2M_ERR_T m2m_time_init()
{
    int level = 2;
    M2M_DBG(level, GENERAL, "In m2m_time_init() ...");
    //Coordinator initialization all device local time = 0
    if(!strcmp(GlobalVND.DeviceType, "ZC"))
    {
        M2M_DBG(level, GENERAL, "Coordinator initial all device local time = 0 ...");
        int ind;
        for(ind = 1; ind <= GlobalVND.TotalDeviceNum; ind++)
        {
            m2m_localtime = (uint64_t *)m2m_localtime_start[ind];
            *m2m_localtime = -1;
        }
    }

    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    if(strcmp(GlobalVND.DeviceType,"ZR"))
        *m2m_localtime = get_vpmu_time();
    else
        *m2m_localtime = time_sync();
    M2M_DBG(level, GENERAL, "[%d] m2m_localtime = %llu",GlobalVND.DeviceID, *m2m_localtime);
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_time_exit()
{
    int level = 2;
    M2M_DBG(level, GENERAL, "In m2m_time_exit() ...");
    *m2m_localtime = MAX_TIME - 1;
    return M2M_SUCCESS;
}

uint64_t get_vpmu_time()
{
    uint64_t time = vpmu_estimated_execution_time_ns();
    return time;
}

uint64_t time_sync()
{
    volatile uint64_t *nt_ptr;
    volatile uint64_t *router_localtime_ptr;
    int ind;
    uint64_t tmp_time = MAX_TIME;

    //Get current time
    //ZED & ZC Have CPU, so their time from VPMU by call get_vpmu_time()
    //to get current time(ns)
    //ZR just update its time with other device which it connected

    if(!strcmp(GlobalVND.DeviceType, "ZR"))
    {
        //When negihbor device exist device type = ZED
        if(neighbor_end)
            for(ind = 0; ind < end_count; ind++)
            {
                nt_ptr = (uint64_t *)m2m_localtime_start[neighbor_end_list[ind]];
                if(tmp_time > *nt_ptr && *nt_ptr != MAX_TIME)
                    tmp_time = *nt_ptr;
            }
        else
            for(ind = 0; ind < router_count; ind++)
            {
                nt_ptr = (uint64_t *)m2m_localtime_start[neighbor_router_list[ind]];
                if(tmp_time > *nt_ptr && *nt_ptr != MAX_TIME)
                    tmp_time = *nt_ptr;
            }

        if(tmp_time == MAX_TIME)
            return 0;
        else
            return tmp_time;
    }
    else
        return get_vpmu_time();
}

double transmission_latency(m2m_HQe_t *msg_info,  unsigned int next_hop_ID, char* networktype )
{
    srand(time(NULL));
    double latency_ms = 0;
    if(!strcmp(networktype, "zigbee"))
    {
        int ack_retry_num, cdma_retry_num;
        int ind,hops;
        unsigned int FromDeviceID = msg_info->ForwardID;
        unsigned int ToDeviceID = next_hop_ID;

        //Calculate communication time from Intermediate node to Next node
        //It's one-hop latency
        //fprintf(stderr, "Node from = %d, Node to = %d\n", FromDeviceID, ToDeviceID);
        ack_retry_num = ack_retry(FromDeviceID, ToDeviceID);
        for(ind = 0; ind <= ack_retry_num; ind++) //retry for ACK failed
        {
            int ind2;
            cdma_retry_num = rand()% 5 +1; //worst case: 4+1
            //fprintf(stderr, "%d->%d cdma retry num = %d\n", FromDeviceID, ToDeviceID, cdma_retry_num);
            //CSMA-CA and retries
            for(ind2 = 0; ind2 < cdma_retry_num; ind2++)
            {
                int times = pow(2, ind2);
                int BE = rand() % times ;  //worst case: times-1
                if(!ind2) BE = 0;
                //fprintf(stderr,"BE = %d\n", BE);
                latency_ms += (BE * 0.32); //random delay: up to max value of 5
                latency_ms += 0.128; //perform CCA (Clear Channdel Assessment)
            }
            //payload latency
            latency_ms += msg_info->PacketSize * 0.032; //250kbps => 32us/byte 

            if((ind+1) <= ack_retry_num)
                latency_ms += 0.864; //assuem ACK failed
        }

        //TODO If we need more roughly communication time estimation
        //     One way that calculate from source to destination from
        //     source node directly. Only need to modify below code
        //     and hide above code 19-43
/*  
        int node_from = node.id;
        int node_to = route_path[0];
        for(hops = 0; hops < route_hops; hops++)
        {

            printf("Node from = %d, Node to = %d\n", node_from, node_to);
            ack_retry_num = ack_retry(node_from, node_to);
            for(ind = 0; ind <= ack_retry_num; ind++) //retry for ACK failed
            {
                int ind2;
                cdma_retry_num = rand()% 5 +1; //worst case: 4+1
                printf("%d->%d cdma retry num = %d\n", node_from, node_to, cdma_retry_num);
                //CSMA-CA and retries
                for(ind2 = 0; ind2 < cdma_retry_num; ind2++)
                {
                    int times = pow(2, ind2);
                    int BE = rand() % times ;  //worst case: times-1
                    if(!ind2) BE = 0;
                    //printf("BE = %d\n", BE);
                    latency_ms += (BE * 0.32); //random delay: up to max value of 5
                    latency_ms += 0.128; //perform CCA (Clear Channdel Assessment)
                }
                //payload latency
                latency_ms += payload * 0.032; //250kbps => 32us/byte 

                if((ind+1) <= ack_retry_num)
                    latency_ms += 0.864; //assuem ACK failed
            }
            node_from = node_to;
            node_to = route_path[hops+1];
        }
*/

    }
        printf("latency(ms) = %f (ns)= %llu\n", latency_ms, (uint64_t)(latency_ms* 1000000));
    return (uint64_t)(latency_ms* 1000000);
}

static inline int ack_retry(int node_from, int node_to)
{
    int retry_num = rand()%4; //worst case: 3
    //printf("%d->%d ack retry num = %d\n", node_from, node_to, retry_num);
    return retry_num;
}
