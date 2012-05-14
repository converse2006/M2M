#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"

extern VND GlobalVND;

/*static long m2m_local_HQ_meta;
static long m2m_local_HQe;
static long m2m_remote_HQ_meta;
static long m2m_remote_HQe;*/

extern long m2m_remote_meta_start[NODE_MAX_LINKS];
extern long m2m_remote_hq_buffer_start[NODE_MAX_LINKS];
extern long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_hq_meta_start[NODE_MAX_LINKS + 1];
extern long m2m_hq_buffer_start[NODE_MAX_LINKS + 1][HEADER_QUEUE_ENTRY_NUM];

static inline M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme);
static inline M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info);

M2M_ERR_T m2m_send_recv_init()
{
    long hq_meta_base,hq_buffer_base;
    volatile m2m_DB_meta_t *db_p = NULL;
    volatile m2m_HQ_meta_t *hq_p = NULL;
    int level = 2;
    M2M_DBG(level, GENERAL, "In m2m_send_recv_init() ...");
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
#endif
    }
    else if(!strcmp(GlobalVND.DeviceType,"ZC"))
    {
        int ind;
        for(ind = 0; ind < NODE_MAX_LINKS; ind++)
        {
            hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[ind];
            hq_p->producer = 0;
            hq_p->consumer = 0;
        }
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];
        hq_p->producer = 0;
        hq_p->consumer = 0;
    }
    else //ZED
    {
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
        hq_p->producer = 0;
        hq_p->consumer = 0;
    }

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

    if(sizeb <= M2M_MAX_HQE_SIZE)
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

static inline M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme)
{
    int count = 0;
    int flag = 1;
    int next_hop_ID; 

    volatile m2m_HQ_meta_t *meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_remote_meta_start[count];
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
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[count] + meta_ptr->producer * HEADER_QUEUE_ENTRY_SIZE);
                    M2M_DBG(level, MESSAGE, "[%d]remote RQe addr : %ld",GlobalVND.DeviceID, (long)remoteHQe_ptr);

                    //copy msg info to queue
                    msg_info->ForwardID = next_hop_ID;
                    msg_info->EntryNum = -1;
                    memcpy((void *)remoteHQe_ptr,(void *)msg_info,sizeof(m2m_HQe_t));

                    //copy real data to queue
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[count] + ((meta_ptr->producer + 1)%HEADER_QUEUE_ENTRY_NUM) * HEADER_QUEUE_ENTRY_SIZE) + sizeof(m2m_HQe_t);
                    memcpy((void *)remoteHQe_ptr, (void *)msg, size);
                    
                    meta_ptr->producer = index;
                } 
            break;
        case LARGE:
            break;
        default:
            break;
    }
    M2M_DBG(level, MESSAGE, "[%d]After producer = %d",GlobalVND.DeviceID, meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]After consumer = %d",GlobalVND.DeviceID, meta_ptr->consumer);
    M2M_DBG(level, MESSAGE, "[%d]Leaving m2m_post_remote_msg() ...",GlobalVND.DeviceID);
    return M2M_SUCCESS;
}
static inline M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info)
{
    volatile m2m_HQ_meta_t *meta_ptr;
    if(!strcmp(GlobalVND.DeviceType, "ZED"))
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
    else
        meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];

    volatile m2m_HQe_t *localHQe_ptr = NULL;
    volatile int index = 0;
    int level = 3;
    
    M2M_DBG(level, MESSAGE, "[%d]In m2m_get_local_msg() ...",GlobalVND.DeviceID);
    M2M_DBG(level, MESSAGE, "[%d]producer address : %ld", GlobalVND.DeviceID, (long)(&(meta_ptr->producer)));
    M2M_DBG(level, MESSAGE, "[%d]consumer address : %ld", GlobalVND.DeviceID, (long)(&(meta_ptr->consumer)));
    M2M_DBG(level, MESSAGE, "[%d]Before producer = %d", GlobalVND.DeviceID, meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]Before consumer = %d", GlobalVND.DeviceID, meta_ptr->consumer);

    if(meta_ptr->consumer == meta_ptr->producer)
        return M2M_TRANS_NOT_READY;
    else
    {
        index = meta_ptr->consumer;
        M2M_DBG(level, MESSAGE, "[%d]Getting message ...", GlobalVND.DeviceID);
        M2M_DBG(level, MESSAGE, "[%d]consumer = %d", GlobalVND.DeviceID, meta_ptr->consumer);

        if(!strcmp(GlobalVND.DeviceType, "ZED"))
        localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[0] + (index * HEADER_QUEUE_ENTRY_SIZE));
        else
        localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[NODE_MAX_LINKS] + (index * HEADER_QUEUE_ENTRY_SIZE));
        M2M_DBG(level, MESSAGE, "[%d] Header packet:\n PacketSize = %d SenderID = %d\n ReceiverID = %d\n EntryNum = %d\n SendTime = %ld\n TransTime = %ld\n", GlobalVND.DeviceID, localHQe_ptr->PacketSize, localHQe_ptr->SenderID, localHQe_ptr->EntryNum, localHQe_ptr->SendTime, localHQe_ptr->TransTime);

    } 
    
    return M2M_SUCCESS;
}
