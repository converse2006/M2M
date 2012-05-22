#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"
#include "vpmu.h"

extern VND GlobalVND;

/*static long m2m_local_HQ_meta;
static long m2m_local_HQe;
static long m2m_remote_HQ_meta;
static long m2m_remote_HQe;*/

extern long m2m_remote_meta_start[NODE_MAX_LINKS];
extern long m2m_remote_hq_buffer_start[NODE_MAX_LINKS][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_hq_meta_start[NODE_MAX_LINKS + 2];
extern long m2m_hq_buffer_start[NODE_MAX_LINKS + 2][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_localtime_start[MAX_NODE_NUM];
extern long m2m_hq_conflag_start[MAX_NODE_NUM];

static inline M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme);
static inline M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info);

//local time
extern uint64_t *m2m_localtime;

M2M_ERR_T m2m_send_recv_init()
{
    volatile m2m_DB_meta_t *db_p = NULL;
    volatile m2m_HQ_meta_t *hq_p = NULL;
    volatile m2m_HQ_cf_t *hq_conflag_ptr;
    int level = 2;
    M2M_DBG(level, GENERAL, "Enter m2m_send_recv_init() ...");

    hq_conflag_ptr = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[GlobalVND.DeviceID];
    hq_conflag_ptr->dataflag = 0;
    hq_conflag_ptr->transtime = 0;


    //Data buffer metadata initialization
    if(!strcmp(GlobalVND.DeviceType,"ZR"))
    {
#ifndef ROUTER_RFD
        int ind;
        for(ind = 0; ind < DATA_BUFFER_ENTRY_NUM; ind++)
        {
            db_p = (m2m_DB_meta_t *)(uintptr_t)m2m_dbp_meta_start[ind];
            db_p->dataflag  = 0;
            db_p->transtime = 0;
        }
#endif        
    }
    else
    {
        int ind;
        for(ind = 0; ind < DATA_BUFFER_ENTRY_NUM; ind++)
        {
            db_p = (m2m_DB_meta_t *)(uintptr_t)m2m_dbp_meta_start[ind];
            db_p->dataflag  = 0;
            db_p->transtime = 0;
        }

    }
    //Header queue metadata initialzation
    if(!strcmp(GlobalVND.DeviceType,"ZR"))
    {
        int ind;
        for(ind = 0; ind < NODE_MAX_LINKS; ind++)
        {
            hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[ind];
            hq_p->producer = 0;
            hq_p->consumer = 0;
        }
#ifndef ROUTER_RFD
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];
        hq_p->producer = 0;
        hq_p->consumer = 0;
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];
        hq_p->producer = 0;
        hq_p->consumer = 0;
#endif
    }
    else if(!strcmp(GlobalVND.DeviceType,"ZC"))
    {
        int ind;
        for(ind = 0; ind < NODE_MAX_LINKS + 2; ind++)
        {
            hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[ind];
            hq_p->producer = 0;
            hq_p->consumer = 0;
        }
        /*hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];
        hq_p->producer = 0;
        hq_p->consumer = 0;*/
    }
    else //ZED
    {
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
        hq_p->producer = 0;
        hq_p->consumer = 0;
    }

    M2M_DBG(level, GENERAL, "Exit m2m_send_recv_init() ...");
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_send(void *src_data, int sizeb, int receiverID, int tag)
{
    M2M_ERR_T rc;
    volatile  m2m_HQe_t *msg;
    volatile  m2m_HQe_t m2m_msgbuf;
    int level = 2;

    if(receiverID == GlobalVND.DeviceID || receiverID > GlobalVND.TotalDeviceNum || receiverID < 0)
    {
        fprintf(stderr, "Error, ReceiverID equal to SenderID or exceed TotalDeviceNum\n");
        return M2M_ERROR; 
    }


    if(sizeb <= HEADER_QUEUE_ENTRY_SIZE)
    {
        msg = &m2m_msgbuf;
        msg->PacketSize = sizeb;
        msg->tag = tag;
        msg->SenderID = GlobalVND.DeviceID;
        msg->ReceiverID = receiverID;
        M2M_DBG(level, MESSAGE, "[%d]Copying data to remote HQ ...",GlobalVND.DeviceID);
        do{
            rc = m2m_post_remote_msg(receiverID, src_data, sizeb, (m2m_HQe_t *)msg, SMALL);
        }while(rc != M2M_SUCCESS);
    }
    
    M2M_DBG(level, MESSAGE, "[%d]End of send routine ...",GlobalVND.DeviceID);
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_recv(void *dst_data, int senderID, int tag)
{
    M2M_ERR_T rc;
    volatile  m2m_HQe_t m2m_msgbuf;
    int level = 2;
    if(senderID != -1)
        if(senderID == GlobalVND.DeviceID || senderID > GlobalVND.TotalDeviceNum || senderID < 0)
        {
            fprintf(stderr, "Error, SenderID equal to ReceiverID or exceed TotalDeviceNum\n");
            return M2M_ERROR; 
        }

    M2M_DBG(level, MESSAGE, "[%d]Copying data from local HQ ...",GlobalVND.DeviceID);                                                 
    do{ 
        rc = m2m_get_local_msg(senderID, dst_data, (m2m_HQe_t *)&m2m_msgbuf);
    }while (rc != M2M_SUCCESS);

    return M2M_SUCCESS;

}

static int db_index = 0;
static M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme)
{
    int count = 0;
    int flag = 1;
    unsigned int next_hop_ID; 

    volatile m2m_HQ_meta_t *meta_ptr;
    volatile m2m_HQ_cf_t *hq_conflag_ptr = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[GlobalVND.DeviceID]; 
    volatile long *remoteHQe_ptr = NULL;
    volatile int index = 0;
    int level = 2;

    //routing next hop device ID; 
    next_hop_ID = route_discovery(GlobalVND.DeviceID, receiverID);
    while(flag && count < GlobalVND.TotalDeviceNum)
    {
        if( GlobalVND.Neighbors[count] == next_hop_ID)
        {
           flag = 0;
           break;
        }
       count++;
    }
    if(flag)
    { //FIXME Error happen,coding handle process
        fprintf(stderr, "[m2m_post_remote_msg]: Not found receiverID in neighbor list\n");
        exit(0);
    }
    if(!strcmp(GlobalVND.DeviceType, "ZED"))
    {
        //NOTE: ZED only connect to router/coordinator, so m2m_remote_meta_start[0] indicate that device shm addr
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_remote_meta_start[0];

        //Wait until next_hop device finish initialization
        uint64_t *nexthop_localtime = (uint64_t *)m2m_localtime_start[next_hop_ID];
        //M2M_DBG(level, MESSAGE,"[%d]Neighbor %d localtime = %llu",GlobalVND.DeviceID, next_hop_ID, *nexthop_localtime);
        if(*nexthop_localtime == -1) //alway check receiver device warmup or not
        return M2M_TRANS_NOT_READY;
        M2M_DBG(level, MESSAGE, "[%d]Neighbor %d localtime = %llu",GlobalVND.DeviceID, next_hop_ID, *nexthop_localtime);
    }
    else
    {
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];

        //Wait until next_hop device finish initialization
        uint64_t *nexthop_localtime = (uint64_t *)m2m_localtime_start[next_hop_ID];
        //M2M_DBG(level, MESSAGE,"[%d]Neighbor %d localtime = %llu",GlobalVND.DeviceID, next_hop_ID, *nexthop_localtime);
        if(*nexthop_localtime == -1) //alway check receiver device warmup or not
        return M2M_TRANS_NOT_READY;
        M2M_DBG(level, MESSAGE, "[%d]Neighbor %d localtime = %llu",GlobalVND.DeviceID, next_hop_ID, *nexthop_localtime);
    }


    M2M_DBG(level, MESSAGE, "[%d]In m2m_post_remote_msg() ...",GlobalVND.DeviceID);
    M2M_DBG(level, MESSAGE, "[%d]Target: %d",GlobalVND.DeviceID,receiverID);
    M2M_DBG(level, MESSAGE, "[%d]producer address: %ld",GlobalVND.DeviceID, (long)(&(meta_ptr->producer)));
    M2M_DBG(level, MESSAGE, "[%d]consumer address: %ld",GlobalVND.DeviceID, (long)(&(meta_ptr->consumer)));

    M2M_DBG(level, MESSAGE, "[%d]Before producer = %d",GlobalVND.DeviceID, meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]Before consumer = %d",GlobalVND.DeviceID, meta_ptr->consumer);
    switch(scheme)
    {
        case SMALL:
                index = (meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM;

                //Queue is full
                if(index == meta_ptr->consumer)
                {
                    M2M_DBG(level, MESSAGE, "[%d]Leaving m2m_post_remote_msg() ... (Queue is full)",GlobalVND.DeviceID);
                    return M2M_TRANS_NOT_READY;
                }
                else
                {
                    //sreach the header queue's entry
                    if(!strcmp(GlobalVND.DeviceType, "ZED"))
                    {
                        remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[0][meta_ptr->producer]);
                        M2M_DBG(level, MESSAGE, "[%d]remote HQe addr : %ld",GlobalVND.DeviceID, m2m_remote_hq_buffer_start[count][meta_ptr->producer]);
                    }
                    else
                    {
                        remoteHQe_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->producer]);
                        M2M_DBG(level, MESSAGE, "[%d]remote HQe addr : %ld",GlobalVND.DeviceID, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->producer]);
                    }
                    //copy msg info to queue
                    msg_info->ForwardID = GlobalVND.DeviceID;
                    msg_info->EntryNum = -1; //-1 impile small size messag
                    msg_info->SendTime = get_vpmu_time();
                    msg_info->TransTime = transmission_latency( (m2m_HQe_t *)msg_info, next_hop_ID, "zigbee" );
        M2M_DBG(level, MESSAGE, "[%d] Header packet:\n PacketSize = %d\n SenderID = %d\n ReceiverID = %d\n ForwardID = %d\n EntryNum = %d\n SendTime(ns)  = %10llu\n TransTime(ns) = %10llu\n", GlobalVND.DeviceID, msg_info->PacketSize, msg_info->SenderID, msg_info->ReceiverID, msg_info->ForwardID, msg_info->EntryNum, msg_info->SendTime, msg_info->TransTime);

                    memcpy((void *)remoteHQe_ptr,(void *)msg_info,sizeof(m2m_HQe_t));

                    //copy real data to queue
                    if(!strcmp(GlobalVND.DeviceType, "ZED"))
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[0][meta_ptr->producer] + sizeof(m2m_HQe_t));
                    else
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS + 1][meta_ptr->producer] + sizeof(m2m_HQe_t));

                    memcpy((void *)remoteHQe_ptr, (void *)msg, size);
                    
                    meta_ptr->producer = index;

                    M2M_DBG(level, MESSAGE, "[%d]After producer = %d",GlobalVND.DeviceID, meta_ptr->producer);
                    M2M_DBG(level, MESSAGE, "[%d]After consumer = %d",GlobalVND.DeviceID, meta_ptr->consumer);
                    M2M_DBG(level, MESSAGE, "[%d]Leaving m2m_post_remote_msg() ...",GlobalVND.DeviceID);

                    //update control flag
                    hq_conflag_ptr->dataflag = 1;
                    hq_conflag_ptr->transtime = 0;

                    //update local time(ns)
                    *m2m_localtime = msg_info->SendTime;
                    M2M_DBG(level, MESSAGE, "localclock = %llu",*m2m_localtime);

                    //wait for transtime
                    fprintf(stderr,"[CONVERSE]Waiting for [hq_conflag_ptr->dataflag != 1]\n");
                    while(hq_conflag_ptr->dataflag == 1)
                    {
                        //TODO if performace drop,there can optimze to get better
                        usleep(SLEEP_TIME);
                    }

                   //FIXME the clock update need to consider different network type
                   //and CPU busy/idle
                   //ticks = ns * tick per ns
                   *m2m_localtime = time_sync();
                   M2M_DBG(level, MESSAGE,"[%d]Before time update after send() = %llu",GlobalVND.DeviceID,*m2m_localtime);
                   M2M_DBG(level, MESSAGE,"[%d]Recevier get header ... transmission time = %llu",GlobalVND.DeviceID, hq_conflag_ptr->transtime);
                   GlobalVPMU.ticks += hq_conflag_ptr->transtime * (GlobalVPMU.target_cpu_frequency / 1000.0);
                   *m2m_localtime = time_sync();
                   M2M_DBG(level, MESSAGE,"[%d]After time update after send() = %llu",GlobalVND.DeviceID,*m2m_localtime);
                   hq_conflag_ptr->transtime = 0;
                   hq_conflag_ptr->dataflag = 0;
                   
                } 
            break;
        case LARGE:
            break;
        default:
            break;
    }
    return M2M_SUCCESS;
}
static M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info)
{
    volatile m2m_HQ_meta_t *meta_ptr;
    if(!strcmp(GlobalVND.DeviceType, "ZED"))
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
    else
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];

    volatile m2m_HQe_t *localHQe_ptr = NULL;
    volatile long      *packet_ptr;
    volatile int index = 0;
    int level = 2;

    //When waiting for receving data, advance time to MAX
    uint64_t tmp_time;
    uint64_t *loctime_ptr;
    loctime_ptr = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    tmp_time = *loctime_ptr;
    *loctime_ptr = MAX_TIME - 1;
    
    if(meta_ptr->consumer == meta_ptr->producer)
        return M2M_TRANS_NOT_READY;



    M2M_DBG(level, MESSAGE, "[%d]In m2m_get_local_msg() ...",GlobalVND.DeviceID);
    M2M_DBG(level, MESSAGE, "[%d]producer address : %ld", GlobalVND.DeviceID, (long)(&(meta_ptr->producer)));
    M2M_DBG(level, MESSAGE, "[%d]consumer address : %ld", GlobalVND.DeviceID, (long)(&(meta_ptr->consumer)));
    M2M_DBG(level, MESSAGE, "[%d]Before producer = %d", GlobalVND.DeviceID, meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]Before consumer = %d", GlobalVND.DeviceID, meta_ptr->consumer);

        index = meta_ptr->consumer;
        M2M_DBG(level, MESSAGE, "[%d]Getting message ...", GlobalVND.DeviceID);
        M2M_DBG(level, MESSAGE, "[%d]consumer = %d", GlobalVND.DeviceID, meta_ptr->consumer);

        if(!strcmp(GlobalVND.DeviceType, "ZED"))
        {
            localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[0][index]);
            packet_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[0][index] + sizeof(m2m_HQe_t));
        }
        else
        {
            localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[NODE_MAX_LINKS][index]);
            packet_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS][index] + sizeof(m2m_HQe_t));
        }
        M2M_DBG(level, MESSAGE, "[%d] Header packet:\n PacketSize = %d\n SenderID = %d\n ReceiverID = %d\n EntryNum = %d\n SendTime = %llu\n TransTime = %llu\n", GlobalVND.DeviceID, localHQe_ptr->PacketSize, localHQe_ptr->SenderID, localHQe_ptr->ReceiverID, localHQe_ptr->EntryNum, localHQe_ptr->SendTime, localHQe_ptr->TransTime);


        memcpy((void *)msg, (void *)packet_ptr, localHQe_ptr->PacketSize);
        meta_ptr->consumer = (meta_ptr->consumer + 1) % HEADER_QUEUE_ENTRY_NUM;


        //FIXME the clock update need to consider different network type
        //and CPU busy/idle
        //ticks = ns * tick per ns
        if(tmp_time > (localHQe_ptr->SendTime + localHQe_ptr->TransTime))
        {
            uint64_t offset_time = (localHQe_ptr->SendTime + localHQe_ptr->TransTime) - tmp_time;
            GlobalVPMU.ticks += offset_time * (GlobalVPMU.target_cpu_frequency / 1000.0);
        }
            
        //When achieve transmission, update to correct localtime
        //*loctime_ptr = tmp_time;
        *loctime_ptr = time_sync();

    return M2M_SUCCESS;
}
