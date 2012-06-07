#include <stdio.h>
#include <stdlib.h>
#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "vpmu.h"
//time sync parameters
extern int end_count, router_count, neighbor_end;
extern int neighbor_end_list[NODE_MAX_LINKS];
extern int neighbor_router_list[NODE_MAX_LINKS];

extern long m2m_localtime_start[MAX_NODE_NUM];
extern VND GlobalVND;
static inline int ack_retry(int node_from, int node_to);

M2M_ERR_T m2m_time_init()
{
    int level = 2;
    volatile  uint64_t *m2m_localtime;
    int ind;
    M2M_DBG(level, MESSAGE, "Enter m2m_time_init() ...");

    //Coordinator initialization all device local time = MAX_TIME(Mean that device not warm up yet)
    if(!strcmp(GlobalVND.DeviceType, "ZC"))
    {
        M2M_DBG(level, MESSAGE, "Coordinator initial all device local time = 0 ...");
        for(ind = 1; ind <= GlobalVND.TotalDeviceNum; ind++)
        {
            m2m_localtime = (uint64_t *)m2m_localtime_start[ind];
            *m2m_localtime = MAX_TIME;
        }
    }

    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    if(strcmp(GlobalVND.DeviceType,"ZR"))
        *m2m_localtime = 0; //NOTE: due to vpmu not startup
        //*m2m_localtime = get_vpmu_time();
    else
        *m2m_localtime = 0;//time_sync();


    volatile uint64_t *action_probe;
    action_probe = (uint64_t *)m2m_localtime_start[0];

    if(!strcmp(GlobalVND.DeviceType, "ZC"))
    {
        int count = 0;
        int start[MAX_NODE_NUM];
        for(ind = 2; ind <= GlobalVND.TotalDeviceNum; ind++)
            start[ind] = 0;
        *action_probe = 0;
        while(count < (GlobalVND.TotalDeviceNum - 1))
        {
            for(ind = 2; ind <= GlobalVND.TotalDeviceNum; ind++)
            {
                if(!start[ind])
                {
                    m2m_localtime = (uint64_t *)m2m_localtime_start[ind];
                    if(*m2m_localtime != MAX_TIME) 
                    {
                        start[ind] = 1;
                        count++;
                    }
                }
            }
        }
        *action_probe = 1;
        
    }
    else
    {
        while(*action_probe != 1){}
        sleep(1);
    }

    M2M_DBG(level, MESSAGE, "Exit m2m_time_init() ...");
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_time_exit()
{
    int level = 0;
    volatile  uint64_t *m2m_localtime;
    M2M_DBG(level, MESSAGE, "Enter m2m_time_exit() ...");

    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    //NOTE: When program finish set local time to MAX (MAX_TIME-1) to 
    //avoid block other device when they still running
    M2M_DBG(level, MESSAGE, "Program finish time: %llu", *m2m_localtime);
    M2M_DBG(level, MESSAGE, "Program total transmission time: %llu", GlobalVND.TotalTransTime);
    *m2m_localtime = MAX_TIME - 1;

    M2M_DBG(level, MESSAGE, "Exit m2m_time_exit() ...");
    return M2M_SUCCESS;
}

uint64_t get_vpmu_time()
{
    //Connect GlobalVPMU to get current time
    volatile uint64_t time = vpmu_estimated_execution_time_ns();
    return time;
}

uint64_t time_sync()
{
    volatile uint64_t *nt_ptr;
    volatile uint64_t *router_localtime_ptr;
    int ind;
    int warmup = 0;
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
                if(*nt_ptr == MAX_TIME)
                    warmup = 1;
                else if(tmp_time > *nt_ptr)
                    tmp_time = *nt_ptr;
            }
        else
            for(ind = 0; ind < router_count; ind++)
            {
                nt_ptr = (uint64_t *)m2m_localtime_start[neighbor_router_list[ind]];
                if(*nt_ptr == MAX_TIME)
                    warmup = 1;
                else if(tmp_time > *nt_ptr)
                    tmp_time = *nt_ptr;
            }

        //if exist connect device not warmup, just return itself;
        if(warmup == 1)
           return 0;
        else
           return tmp_time; 
    }
    else 
        return get_vpmu_time();
}

uint64_t transmission_latency(m2m_HQe_t *msg_info,  unsigned int next_hop_ID, char* networktype )
{
    //FIXME Currently we use random way to generate retransmission time
    //We need to build a formula with many parameter which effect the transtime
    //e.g. packet loss rate...
    int level = 2;
    srand(time(NULL));
    double latency_ms = 0;
    if(!strcmp(networktype, "zigbee"))
    {
        int ack_retry_num, cdma_retry_num;
        int ind;
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
        M2M_DBG(level, MESSAGE, "latency(ms) = %f (ns)= %llu\n", latency_ms, (uint64_t)(latency_ms * 1000000.0));
    //return (uint64_t)(latency_ms* 1000000.0);
    return (uint64_t)(5440000);
}

static inline int ack_retry(int node_from, int node_to)
{
    int retry_num = 0;
    //Best Case:
    //retry_num = 0;

    //Worst Case:
    //retry_num = 3;

    //Random Case:
    retry_num = rand()%4; 
    return retry_num;
}
