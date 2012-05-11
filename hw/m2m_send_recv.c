#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"

extern VND GlobalVND;

static inline M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme);
static inline M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg,int size, m2m_HQe_t *msg_info);

M2M_ERR_T m2m_send(void *src_data, int sizeb, int receiverID, int tag)
{
    M2M_ERR_T rc;
    volatile  m2m_HQe_t *msg;
    volatile  m2m_HQe_t m2m_msgbuf;
    int level = 2;

    if(receiverID == GlobalVND.DeviceID || receiverID > GlobalVND.TotalDeviceNum)
    {
        fprintf(stderr, "Error, ReceiverID equal to SenderID or exceed TotalDeviceNum\n");
        return M2M_ERROR; 
    }

    if(sizeb <= M2M_MAX_HQE_SIZE)
    {
        msg = &m2m_msgbuf;
        msg->len = sizeb;
        msg->tag = tag;
        msg->sid = GlobalVND.DeviceID;
        msg->rid = receiverID;

        M2M_DBG(level, MESSAGE, "[%d]Copying data to remote HQ ...",GlobalVND.DeviceID);
        do{
            rc = m2m_post_remote_msg(receiverID, src_data, sizeb, (m2m_HQe_t *)msg, SMALL);
        }while(rc != M2M_SUCCESS);
    }
    
    M2M_DBG(level, MESSAGE, "[%d]End of send routine ...",GlobalVND.DeviceID);
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_recv(void *dst_data, int sizeb, int senderID, int tag)
{
    M2M_ERR_T rc;
    volatile  m2m_HQe_t m2m_msgbuf;
    int level = 2;
    if(senderID)
        if(senderID == GlobalVND.DeviceID || senderID > GlobalVND.TotalDeviceNum)
        {
            fprintf(stderr, "Error, SenderID equal to ReceiverID or exceed TotalDeviceNum\n");
            return M2M_ERROR; 
        }

    M2M_DBG(level, MESSAGE, "[%d]Copying data from local HQ ...",GlobalVND.DeviceID);                                                 
    do{ 
        rc = m2m_get_local_msg(senderID, dst_data, sizeb, (m2m_HQe_t *)&m2m_msgbuf);
    }while (rc != M2M_SUCCESS);

    return M2M_SUCCESS;

}

static inline M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme)
{
    return M2M_SUCCESS;
}
static inline M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg,int size, m2m_HQe_t *msg_info)
{
    return M2M_SUCCESS;
}
