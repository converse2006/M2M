/*    
 * Copyright (C) 2012 PASLab CSIE NTU. All rights reserved.
 *      - Chen Chun-Han <converse2006@gmail.com>
 */

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
static inline int RandomHit(int Percent);
volatile int vpmu_trigger = 0;

M2M_ERR_T m2m_time_init()
{
    int level = 2;
    volatile  uint64_t *m2m_localtime;
    int ind;
    srand(time(NULL));
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
        vpmu_trigger = 77;
        
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
    
    vpmu_trigger = 0;
    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    //NOTE: When program finish set local time to MAX (MAX_TIME-1) to 
    //avoid block other device when they still running
    fprintf(stderr, "Program finish time: %llu\n", *m2m_localtime);
    fprintf(stderr, "Program total transmission time: %llu\n", GlobalVND.TotalTransTimes);
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

uint64_t transmission_latency(m2m_HQe_t *msg_info,  unsigned int next_hop_ID,uint64_t* fail_latency, char* networktype)
{
    int level = 2;
    uint64_t TotalTransTimeus = 0;
    GlobalVND.TotalPacketTransTimes++;  //[M2M Perf]Total count fot one-hop transmission
    if(!strcmp(networktype, "zigbee"))
    {
        unsigned int FromDeviceID = msg_info->ForwardID;
        unsigned int ToDeviceID = next_hop_ID;


        //Calculate communication time from Intermediate node to Next node
        //It's one-hop latency
        //fprintf(stderr, "Node from = %d, Node to = %d\n", FromDeviceID, ToDeviceID);
        int ack_success = 0;
        int NB_ack  = 0; //Counting Ack retries
        while(ack_success == 0)
        {
            int BE = minBE;
            int NB_csma = 0; //Counting CSMA-CA retries
            int cdma_ca_success = 0;
            while(cdma_ca_success == 0)
            {

                int tmpBE = pow(2, BE);
                int Backofftime = rand()%tmpBE;
                //[NOTE]: Potential Problem
                //Due to index 0 & -1 is used to return latency time for time synchronization
                //But it does not need to return transmission fail but I did not fix this bug
                //TODO: When use index 0 & -1 we need to used math formula to return the value!


                //fprintf(stderr, "Backofftime = %d\n",Backofftime);
                TotalTransTimeus += Backofftime * AUnitBackoffPeriod; //Random Backoff Time
                TotalTransTimeus += CCADetectionTime;                 //Perform CCA (Clear Channel  Assessment)

                int csma_fail = RandomHit(ChannelBusyRateCSMA * Percentage);
                if(csma_fail == 1)
                {
                    //fprintf(stderr, "csma_fail\n");
                    if(BE < maxBE)
                        BE++;
                    if(NB_csma == MaxCSMABackoffs)
                    {
                        //fprintf(stderr, "csma_fail over limit\n");
                        cdma_ca_success = -1;
                    }
                    else
                        NB_csma++;
                }
                else
                {
                    //fprintf(stderr, "csma_success\n");
                    cdma_ca_success = 1;
                }
            
            }
            if(cdma_ca_success == -1) //CSMA-CA Perfrom fail due to channel busy
            {
                GlobalVND.TotalPacketLossTimes++; //[M2M Perf] Total count for one-hop transmission loss
                *fail_latency = (uint64_t)(TotalTransTimeus * 1000.0);
                return 0;
            }

            TotalTransTimeus += (msg_info->PacketSize + HEADERSIZE) * DataRate;
            TotalTransTimeus += AckWaitDuration;

            int ack_fail = RandomHit(ChannelBusyRateTx * Percentage);
            if(ack_fail == 1)
            {
                //fprintf(stderr, "ack_fail\n");
                if(NB_ack == MaxFrameRetries)
                {
                    //fprintf(stderr, "ack_fail over limit\n");
                    ack_success = -1;
                }
                else
                    NB_ack++;
            }
            else
            {
                //fprintf(stderr, "ack_success\n");
                ack_success = 1;
            }

        }
        if(ack_success == -1) //Ack Perfrom fail due to channel busy
        {
            GlobalVND.TotalPacketLossTimes++; //[M2M Perf] Total count for one-hop transmission loss
            *fail_latency = (uint64_t)(TotalTransTimeus * 1000.0);
            return 0;
        }

        //fprintf(stderr, "TotalTransTimeus = %llu\n", TotalTransTimeus);


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
    M2M_DBG(level, MESSAGE, "latency(us) = %llu (ns)= %llu\n", TotalTransTimeus, (uint64_t)(TotalTransTimeus * 1000.0));
    return (uint64_t)(TotalTransTimeus * 1000.0);
}
static inline int RandomHit(int Percent)
{
    int hitbox[Percentage]={0};
    int ind;
    //Setting Hitbox
    for(ind = 0; ind < Percent; ind++)
    {
        int flag = 0;
        do{
            int hitnum = rand()%Percentage;
            if(hitbox[hitnum] != 1)
            {
                flag = 1;
               hitbox[hitnum] = 1; 
            }
        }while(flag != 1);
    }

    int guessnum = rand()%Percentage;
    if(hitbox[guessnum] == 1)
        return 1;
    else
        return 0;
    
}

