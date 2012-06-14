#include <stdio.h>
#include <stdlib.h>
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

extern long m2m_remote_hq_meta_start[NODE_MAX_LINKS];
extern long m2m_remote_hq_buffer_start[NODE_MAX_LINKS][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];
extern long m2m_hq_meta_start[NODE_MAX_LINKS + 2];
extern long m2m_hq_buffer_start[NODE_MAX_LINKS + 2][HEADER_QUEUE_ENTRY_NUM];
extern long m2m_localtime_start[MAX_NODE_NUM];
extern long m2m_hq_conflag_start[MAX_NODE_NUM];

M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme);
M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info);

#ifdef M2M_LOGFILE
FILE *outFile; //For log communication pattern
#endif

M2M_ERR_T m2m_send_recv_init()
{
    int level = 2;
    volatile m2m_DB_meta_t *db_p = NULL;
    volatile m2m_HQ_meta_t *hq_p = NULL;
    volatile m2m_HQ_cf_t *hq_conflag_ptr;
    int ind;
    M2M_DBG(level, GENERAL, "Enter m2m_send_recv_init() ...");

    GlobalVND.TotalTransTimes = 0;

    //Initialize HQ control flag for small size
    //Due to small size communication without DB, so we can't use DB meta control flag
    //As result, we add a meta for small size to judge wheather header already deliver
    //to recevier side, Sender modify dataflag=>1 and Receiver routing processor will
    //modify dataflag=>0 and correspond arrivel time
    hq_conflag_ptr = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[GlobalVND.DeviceID];
    hq_conflag_ptr->dataflag = 0;
    hq_conflag_ptr->transtime = 0;


    //Data buffer metadata initialization
    if(!strcmp(GlobalVND.DeviceType, "ZR"))
    {
#ifndef ROUTER_RFD
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
        for(ind = 0; ind < NODE_MAX_LINKS + 2; ind++)
        {
            hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[ind];
            hq_p->producer = 0;
            hq_p->consumer = 0;
        }
    }
    else //ZED
    {
        hq_p = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
        hq_p->producer = 0;
        hq_p->consumer = 0;
    }

#ifdef M2M_LOGFILE
    char location[100] = "NetLogs/device";
    char post[]= ".txt";
    char devicenum[20];
    //itoa(GlobalVND.DeviceID, devicenum, 10);
    sprintf(devicenum, "%d", GlobalVND.DeviceID);
    strcat( location, devicenum);
    strcat( location, post);
    M2M_DBG(level, GENERAL, "Logging file at %s\n", location);
    outFile = fopen(location, "w");
    if(outFile == NULL)
    {
        fprintf(stderr, "Logging File open error!\n");
        exit(0);
    }
#endif

    M2M_DBG(level, GENERAL, "Exit m2m_send_recv_init() ...");
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_send(void *src_data, int sizeb, int receiverID, int tag)
{
    int level = 1;
    M2M_ERR_T rc =M2M_SUCCESS;
    volatile  m2m_HQe_t *msg;
    volatile  m2m_HQe_t m2m_msgbuf;
    M2M_DBG(level, MESSAGE, "Enter m2m_send() ...");

    if(receiverID == GlobalVND.DeviceID || receiverID > GlobalVND.TotalDeviceNum || receiverID < 0)
    {
        fprintf(stderr, "Error, ReceiverID equal to SenderID or exceed TotalDeviceNum\n");
        return M2M_ERROR; 
    }

    //Small communication scheme
    if(sizeb <= (HEADER_QUEUE_ENTRY_SIZE))
    {
        msg = &m2m_msgbuf;
        msg->PacketSize = sizeb;
        msg->tag = tag;
        msg->SenderID = GlobalVND.DeviceID;
        msg->ReceiverID = receiverID;
        M2M_DBG(level, MESSAGE, "[%d]Copying data to remote HQ ...", GlobalVND.DeviceID);
        do{
            rc = m2m_post_remote_msg(receiverID, src_data, sizeb, (m2m_HQe_t *)msg, SMALL);
        }while(rc != M2M_TRANS_SUCCESS && rc != M2M_TRANS_FAIL);
    }
    else //Large communication scheme
    {
        //FIXME This part not implement!!
    }
    
    M2M_DBG(level, MESSAGE, "Exit m2m_send() ...");
    if(rc == M2M_TRANS_FAIL)
        return M2M_TRANS_FAIL;
    else
        return M2M_TRANS_SUCCESS;
}

M2M_ERR_T m2m_recv(void *dst_data, int senderID, int tag)
{
    int level = 1;
    volatile  m2m_HQe_t m2m_msgbuf;
    M2M_ERR_T rc;
    M2M_DBG(level, MESSAGE, "Enter m2m_recv() ...");

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

    M2M_DBG(level, MESSAGE, "Exit m2m_recv() ...");
    return M2M_SUCCESS;

}

//static int db_index = 0;
M2M_ERR_T m2m_post_remote_msg(int receiverID,volatile void *msg,int size, m2m_HQe_t *msg_info,int scheme)
{
    int level = 1;
    volatile m2m_HQ_meta_t *hq_meta_ptr;
    volatile long *remoteHQe_ptr;
    volatile uint64_t *m2m_localtime;
    volatile m2m_HQ_cf_t *hq_conflag_ptr;
    int count = 0;
    int flag = 1;
    unsigned int next_hop_ID; 
#ifdef M2M_LOGFILE
    char logtext[200] = "[Send] ";
    char tmptext[100];
#endif

    M2M_DBG(level, MESSAGE, "Enter m2m_post_remote_msg() ...");

    hq_conflag_ptr = (m2m_HQ_cf_t *)(uintptr_t)m2m_hq_conflag_start[GlobalVND.DeviceID]; 
    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];

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
    { 
        fprintf(stderr, "[m2m_post_remote_msg]: Not found receiverID in neighbor list\n");
        exit(0);
    }

    //NOTE: ZED only connect to router/coordinator, so m2m_remote_hq_meta_start[0] indicate that device shm addr
    if(!strcmp(GlobalVND.DeviceType, "ZED"))
        hq_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_remote_hq_meta_start[0];
    else
        hq_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS + 1];


    /*M2M_DBG(level, MESSAGE, "[%d]Target: %d", GlobalVND.DeviceID, receiverID);
    M2M_DBG(level, MESSAGE, "[%d]producer address: %lx", GlobalVND.DeviceID, (long)(&(hq_meta_ptr->producer)));
    M2M_DBG(level, MESSAGE, "[%d]consumer address: %lx", GlobalVND.DeviceID, (long)(&(hq_meta_ptr->consumer)));
    M2M_DBG(level, MESSAGE, "[%d]Before producer = %d", GlobalVND.DeviceID, hq_meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]Before consumer = %d", GlobalVND.DeviceID, hq_meta_ptr->consumer);*/
    uint64_t fail_latency;
    //Start transmission
    switch(scheme)
    {
        case SMALL:


                //Search the header queue's entry
                if(!strcmp(GlobalVND.DeviceType, "ZED"))
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[0][hq_meta_ptr->producer]);
                else
                    remoteHQe_ptr =(long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS + 1][hq_meta_ptr->producer]);
                //    M2M_DBG(level, MESSAGE, "[%d]remote HQe addr : %lx",GlobalVND.DeviceID, remoteHQe_ptr);

                //Fill header 
                msg_info->ForwardID = GlobalVND.DeviceID;
                msg_info->EntryNum  = -1; //-1 impile small size message
                msg_info->SendTime  = get_vpmu_time();
                msg_info->TransTime = transmission_latency( (m2m_HQe_t *)msg_info, next_hop_ID, &fail_latency, "zigbee");

                //When transmission time calculate is fail transmission,
                if(msg_info->TransTime == 0) 
                {
#ifdef M2M_LOGFILE
                    sprintf(tmptext,"ReceiverID: %3d ", msg_info->ReceiverID);
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"Transmission Fail");
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"FailLatency: %15llu\n", fail_latency);
                    strcat(logtext, tmptext);
                    fputs(logtext, outFile);
#endif
                    M2M_DBG(level, MESSAGE, "fail_latency = %llu",fail_latency);
                   GlobalVND.TotalTransTimes += fail_latency;
                   GlobalVPMU.ticks += (fail_latency)*(GlobalVPMU.target_cpu_frequency / 1000.0);
                   *m2m_localtime = time_sync();
                    M2M_DBG(level, MESSAGE, "Packet fail in the middle way");
                    M2M_DBG(level, MESSAGE, "Exit m2m_post_remote_msg() ...");
                    return M2M_TRANS_FAIL;
                }

                M2M_DBG(level, MESSAGE, "[%d] Header packet:\n PacketSize = %d\n SenderID = %d\n ReceiverID = %d\n ForwardID = %d\n EntryNum = %d\n SendTime(ns)  = %10llu\n TransTime(ns) = %10llu\n", GlobalVND.DeviceID, msg_info->PacketSize, msg_info->SenderID, msg_info->ReceiverID, msg_info->ForwardID, msg_info->EntryNum, msg_info->SendTime, msg_info->TransTime);

                //Header Queue is full
                M2M_DBG(level, MESSAGE, "[WHILE]Sender waiting for receiver header queue have empty entry");
                while(((hq_meta_ptr->producer + 1) % HEADER_QUEUE_ENTRY_NUM) == hq_meta_ptr->consumer){}
                //Copy header to queue
                memcpy((void *)remoteHQe_ptr,(void *)msg_info,sizeof(m2m_HQe_t));

                if(!strcmp(GlobalVND.DeviceType, "ZED"))
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_remote_hq_buffer_start[0][((hq_meta_ptr->producer + 1)% HEADER_QUEUE_ENTRY_NUM)]);
                else
                    remoteHQe_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS + 1][((hq_meta_ptr->producer + 1)% HEADER_QUEUE_ENTRY_NUM)]);

                while(((hq_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM) == hq_meta_ptr->consumer){}
                //Copy real data to queue
                memcpy((void *)remoteHQe_ptr, (void *)msg, size);
                
                //Update hq control flag
                hq_conflag_ptr->dataflag = 77;
                hq_conflag_ptr->transtime = 0;

                hq_meta_ptr->producer = ((hq_meta_ptr->producer + 2) % HEADER_QUEUE_ENTRY_NUM);

                //M2M_DBG(level, MESSAGE, "[%d]After producer = %d",GlobalVND.DeviceID, hq_meta_ptr->producer);
                //M2M_DBG(level, MESSAGE, "[%d]After consumer = %d",GlobalVND.DeviceID, hq_meta_ptr->consumer);

                //Wait for transtime
                M2M_DBG(level, MESSAGE, "[CONVERSE]Waiting for [hq_conflag_ptr->dataflag != 77]\n");
                while(hq_conflag_ptr->dataflag == 77){}
                M2M_DBG(level, MESSAGE,"[%d]hq_conflag_ptr->dataflag = %d",GlobalVND.DeviceID,hq_conflag_ptr->dataflag);

                //FIXME the clock update need to consider different network type
                //and CPU busy/idle
                //ticks = ns * tick per ns
                if(hq_conflag_ptr->dataflag != 44)
                {
                M2M_DBG(level, MESSAGE,"[%d]Packet Arrivel Time = %llu",GlobalVND.DeviceID,(hq_conflag_ptr->transtime));
                M2M_DBG(level, MESSAGE,"[%d]Device Local Time = %llu",GlobalVND.DeviceID,(time_sync()));
                M2M_DBG(level, MESSAGE,"[%d]Recevier get header ... transmission time = %llu",GlobalVND.DeviceID,\
                                                                         (hq_conflag_ptr->transtime-time_sync()));
#ifdef M2M_LOGFILE
                    sprintf(tmptext,"ReceiverID: %3d ", msg_info->ReceiverID);
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"PacketSize: %4d ", msg_info->PacketSize);
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"SendTime   : %15llu ", msg_info->SendTime);
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"TransmissionTime: %15llu\n", (hq_conflag_ptr->transtime-time_sync()));
                    strcat(logtext, tmptext);
                    fputs(logtext, outFile);
#endif
                
                   GlobalVND.TotalTransTimes += (hq_conflag_ptr->transtime-time_sync());
                   GlobalVPMU.ticks += (hq_conflag_ptr->transtime-time_sync())*(GlobalVPMU.target_cpu_frequency / 1000.0);
                   *m2m_localtime = time_sync();
                    return M2M_TRANS_SUCCESS;
                }
                else
                {
#ifdef M2M_LOGFILE
                    sprintf(tmptext,"ReceiverID: %3d ", msg_info->ReceiverID);
                    strcat(logtext, tmptext);
                    sprintf(tmptext,"Transmission Fail\n");
                    strcat(logtext, tmptext);
                    fputs(logtext, outFile);
#endif
                   GlobalVND.TotalTransTimes += (hq_conflag_ptr->transtime-time_sync());
                   GlobalVPMU.ticks += (hq_conflag_ptr->transtime-time_sync())*(GlobalVPMU.target_cpu_frequency / 1000.0);
                   *m2m_localtime = time_sync();
                    M2M_DBG(level, MESSAGE, "Packet fail in the middle way");
                    M2M_DBG(level, MESSAGE, "Exit m2m_post_remote_msg() ...");
                    return M2M_TRANS_FAIL;
                }


                   
            break;
        case LARGE:
            break;
        default:
            break;
    }

    M2M_DBG(level, MESSAGE, "Exit m2m_post_remote_msg() ...");
    return M2M_SUCCESS;
}
M2M_ERR_T m2m_get_local_msg(int senderID,volatile void *msg, m2m_HQe_t *msg_info)
{
    int level = 1;
    volatile m2m_HQ_meta_t *hq_meta_ptr;
    volatile m2m_HQe_t     *localHQe_ptr;
    volatile long          *packet_ptr;


    if(!strcmp(GlobalVND.DeviceType, "ZED"))
        hq_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[0];
    else
        hq_meta_ptr = (m2m_HQ_meta_t *)(uintptr_t)m2m_hq_meta_start[NODE_MAX_LINKS];

    //M2M_DBG(level, MESSAGE, "hq_meta_ptr->producer = %d hq_meta_ptr->consumer = %d", hq_meta_ptr->producer,hq_meta_ptr->consumer);
    int producer = 0,consumer = 0;
    //M2M_DBG(level, MESSAGE, "[WHILE]Receiver waiting for packet in header queue");
    if(hq_meta_ptr->producer == hq_meta_ptr->consumer){
        /*if(producer != hq_meta_ptr->producer || consumer != hq_meta_ptr->consumer)
        {
            M2M_DBG(level, MESSAGE, "Pro = %d Con = %d", hq_meta_ptr->producer, hq_meta_ptr->consumer);
            producer = hq_meta_ptr->producer;
            consumer = hq_meta_ptr->consumer;
        }*/
        return M2M_TRANS_NOT_READY;
    }
    M2M_DBG(level, MESSAGE, "Enter m2m_get_local_msg() ...");
    M2M_DBG(level, MESSAGE, "Packer exist in header queue  ...");


    //When waiting for receving data, advance time to MAX
    uint64_t tmp_time;
    volatile uint64_t *m2m_localtime;
    m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
    if(strcmp(GlobalVND.DeviceType, "ZED"))
        *m2m_localtime = time_sync();
    tmp_time = *m2m_localtime;
    *m2m_localtime = MAX_TIME - 1; //Why "MAX_TIME -1" ? Because MAX_TIME used to judge "Device not warm up yet"

    /*M2M_DBG(level, MESSAGE, "[%d]Before producer = %d", GlobalVND.DeviceID, hq_meta_ptr->producer);
    M2M_DBG(level, MESSAGE, "[%d]Before consumer = %d", GlobalVND.DeviceID, hq_meta_ptr->consumer);*/


    if(!strcmp(GlobalVND.DeviceType, "ZED"))
        localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[0][hq_meta_ptr->consumer]);
    else
        localHQe_ptr = (m2m_HQe_t *)(uintptr_t) (m2m_hq_buffer_start[NODE_MAX_LINKS][hq_meta_ptr->consumer]);

    M2M_DBG(level, MESSAGE, "[%d] Header packet:\n PacketSize = %d\n SenderID = %d\n ReceiverID = %d\n EntryNum = %d\n SendTime = %llu\n TransTime = %llu\n", GlobalVND.DeviceID, localHQe_ptr->PacketSize, localHQe_ptr->SenderID, localHQe_ptr->ReceiverID, localHQe_ptr->EntryNum, localHQe_ptr->SendTime, localHQe_ptr->TransTime);

    //Small communication scheme
    if(localHQe_ptr->EntryNum == -1)
    {
        M2M_DBG(level, MESSAGE, "Small Size receive ...");
        if(!strcmp(GlobalVND.DeviceType, "ZED"))
            packet_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[0][((hq_meta_ptr->consumer + 1) % HEADER_QUEUE_ENTRY_NUM)]);
        else
            packet_ptr = (long *)(uintptr_t)(m2m_hq_buffer_start[NODE_MAX_LINKS][((hq_meta_ptr->consumer + 1) % HEADER_QUEUE_ENTRY_NUM)]);


        //Get data from header queue
        memcpy((void *)msg, (void *)packet_ptr, localHQe_ptr->PacketSize);

        //After finish recv(), resume device local time
        *m2m_localtime = tmp_time;
        hq_meta_ptr->consumer = (hq_meta_ptr->consumer + 2) % HEADER_QUEUE_ENTRY_NUM;

#ifdef M2M_LOGFILE
        char logtext[200] = "[Recv] ";
        char tmptext[100];
        sprintf(tmptext,"SenderID  : %3d ", localHQe_ptr->SenderID);
        strcat(logtext, tmptext);
        sprintf(tmptext,"PacketSize: %4d ", localHQe_ptr->PacketSize);
        strcat(logtext, tmptext);
        sprintf(tmptext,"ArrivalTime: %15llu ",(localHQe_ptr->SendTime+localHQe_ptr->TransTime));
        strcat(logtext, tmptext);
        sprintf(tmptext,"ReceiveTime     : %15llu\n",*m2m_localtime);
        strcat(logtext, tmptext);
        fputs(logtext, outFile);
#endif

        /*M2M_DBG(level, MESSAGE, "[%d]After producer = %d", GlobalVND.DeviceID, hq_meta_ptr->producer);
        M2M_DBG(level, MESSAGE, "[%d]After consumer = %d", GlobalVND.DeviceID, hq_meta_ptr->consumer);*/
    }
    else //Large communication scheme
    {
        //FIXME
    }


    M2M_DBG(level, MESSAGE,"[%d]Packet Arrivel Time = %llu",GlobalVND.DeviceID,(localHQe_ptr->SendTime + localHQe_ptr->TransTime));
    M2M_DBG(level, MESSAGE,"[%d]Device Local Time = %llu",GlobalVND.DeviceID,(*m2m_localtime));
/*    if(*m2m_localtime < (localHQe_ptr->SendTime + localHQe_ptr->TransTime))
    {
        M2M_DBG(level, MESSAGE,"[%d]Recevier get header ... transmission time = %llu",GlobalVND.DeviceID, \
                                                ((localHQe_ptr->SendTime + localHQe_ptr->TransTime)-*m2m_localtime));
    }
    else
    {
        M2M_DBG(level, MESSAGE,"[%d]Recevier get header ... No transmission time due to arrivel time < local time", \
                                                GlobalVND.DeviceID);
    }*/

 /*   M2M_DBG(level, MESSAGE,"[%d]Before time update after send()= %llu",GlobalVND.DeviceID,*m2m_localtime);
    M2M_DBG(level, MESSAGE,"[%d]Before time_sync= %llu",GlobalVND.DeviceID,time_sync());
    M2M_DBG(level, MESSAGE,"[%d]Before GlobalVPMU.ticks= %llu",GlobalVND.DeviceID,GlobalVPMU.ticks);*/

    //FIXME the clock update need to consider different network type
    //and CPU busy/idle
    //ticks = ns * tick per ns
    if(*m2m_localtime < (localHQe_ptr->SendTime + localHQe_ptr->TransTime))
    {
        GlobalVND.TotalTransTimes += ((localHQe_ptr->SendTime + localHQe_ptr->TransTime) - *m2m_localtime);
        GlobalVPMU.ticks += ((localHQe_ptr->SendTime + localHQe_ptr->TransTime) - *m2m_localtime) * \
                            (GlobalVPMU.target_cpu_frequency / 1000.0);
    }
            
    //When achieve transmission, update to correct localtime
    if(strcmp(GlobalVND.DeviceType, "ZED"))
    *m2m_localtime = time_sync();

    /*M2M_DBG(level, MESSAGE,"[%d]After GlobalVPMU.ticks= %llu",GlobalVND.DeviceID,GlobalVPMU.ticks);
    M2M_DBG(level, MESSAGE,"[%d]After time_sync= %llu",GlobalVND.DeviceID,time_sync());
    M2M_DBG(level, MESSAGE,"[%d]After time update after send() = %llu",GlobalVND.DeviceID,*m2m_localtime);*/

    M2M_DBG(level, MESSAGE, "Exit m2m_get_local_msg() ...");
    return M2M_SUCCESS;
}
M2M_ERR_T m2m_send_recv_exit()
{
    int level = 2;
    M2M_DBG(level, GENERAL, "Enter m2m_send_recv_exit() ...");

#ifdef M2M_LOGFILE
    fclose(outFile);
#endif

    M2M_DBG(level, GENERAL, "Exit m2m_send_recv_exit() ...");
    return M2M_SUCCESS;
}
