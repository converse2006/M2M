#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"

extern long NODE_SHM_LOCATION[MAX_NODE_NUM]; //Each node shared memory address
extern unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM]; //Each node link map
extern char NODE_TYPE[MAX_NODE_NUM][4];
extern VND GlobalVND;
extern long shm_address_location;


long m2m_localtime_start[MAX_NODE_NUM];
long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];
long m2m_hq_meta_start[NODE_MAX_LINKS + 2];
long m2m_hq_buffer_start[NODE_MAX_LINKS + 2][HEADER_QUEUE_ENTRY_NUM];
long m2m_hq_conflag_start[MAX_NODE_NUM];

long m2m_remote_meta_start[NODE_MAX_LINKS];
long m2m_remote_hq_buffer_start[NODE_MAX_LINKS][HEADER_QUEUE_ENTRY_NUM];
M2M_ERR_T m2m_copy_to_dbp(void *data, int sizeb,int DeviceID)
{
    volatile m2m_HQ_meta_t *local_meta_ptr = NULL; 

    return M2M_SUCCESS;
}
M2M_ERR_T m2m_dbp_init()
{
    int level = 2;
    M2M_DBG(level, GENERAL, "Enter m2m_dbp_init() ...");
    int index, index_link, index_ent;
    int DB_data_base,DB_meta_base;
    int HQ_data_base,HQ_meta_base;

    //NODE_SHM_LOCATION + base shm address
    int ind;

    //Note: "(MAX_NODE_NUM * sizeof(uint64_t)" is shared memory of localtime(ns)
    for( ind = 1; ind <= GlobalVND.TotalDeviceNum; ind++)
    {
        //NOTE: MAX_NODE_NUM alway equal to (totoal node number + 1) due to ID start from 1
        NODE_SHM_LOCATION[ind] += (shm_address_location + (MAX_NODE_NUM * sizeof(uint64_t)));
        fprintf(stderr, "NODE_SHM_LOCATION[%d] = %ld\n", ind, NODE_SHM_LOCATION[ind]);
        m2m_localtime_start[ind] = shm_address_location + ind * sizeof(uint64_t);
        fprintf(stderr, "m2m_localtime_start[%d] = %ld\n", ind, m2m_localtime_start[ind]);

        if(!strcmp(NODE_TYPE[ind], "ZED"))
            m2m_hq_conflag_start[ind] = NODE_SHM_LOCATION[ind] + TOTAL_HEADER_QUEUE_SIZE;
        else if(!strcmp(NODE_TYPE[ind], "ZR")) 
        {
#ifndef ROUTER_RFD
            m2m_hq_conflag_start[ind] = NODE_SHM_LOCATION[ind] + ((NODE_MAX_LINKS + 2) * TOTAL_HEADER_QUEUE_SIZE) + \
                                        TOTAL_DATA_BUFFER_SIZE;
#endif
        }
        else if(!strcmp(NODE_TYPE[ind], "ZC")) 
            m2m_hq_conflag_start[ind] = NODE_SHM_LOCATION[ind] + ((NODE_MAX_LINKS + 2) * TOTAL_HEADER_QUEUE_SIZE) + \
                                        TOTAL_DATA_BUFFER_SIZE;
        fprintf(stderr, "m2m_hq_conflag_start[%d] = %ld\n", ind, m2m_hq_conflag_start[ind]);
    }

    GlobalVND.Shared_memory_address = NODE_SHM_LOCATION[GlobalVND.DeviceID];

    if(!strcmp(GlobalVND.DeviceType, "ZED")) //End device layout
    {
        HQ_data_base = GlobalVND.Shared_memory_address;
        HQ_meta_base = GlobalVND.Shared_memory_address + (HEADER_QUEUE_SIZE); 
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
        {
            m2m_hq_buffer_start[0][index_ent] = HQ_data_base + (index_ent * HEADER_QUEUE_ENTRY_SIZE);  
        }
            m2m_hq_meta_start[0]   = HQ_meta_base ;  
        
        m2m_hq_conflag_start[GlobalVND.DeviceID] = GlobalVND.Shared_memory_address + TOTAL_HEADER_QUEUE_SIZE;
        DB_data_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE) + HEADER_QUEUE_CONTRLFLAG;
        DB_meta_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE) + HEADER_QUEUE_CONTRLFLAG +(DATA_BUFFER_SIZE);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
        {
            m2m_dbp_buffer_start[index] = DB_data_base + (index * DATA_BUFFER_ENTRY_SIZE);  
            m2m_dbp_meta_start[index]   = DB_meta_base + (index * DATA_BUFFER_METADATA_ENTRY_SIZE);
        }


    }
    else if(!strcmp(GlobalVND.DeviceType, "ZR") || !strcmp(GlobalVND.DeviceType, "ZC")) //Router & Coordinator layout
    {
        HQ_data_base = GlobalVND.Shared_memory_address;
        HQ_meta_base = GlobalVND.Shared_memory_address + (NODE_MAX_LINKS * HEADER_QUEUE_SIZE); 
        for(index_link = 0; index_link < NODE_MAX_LINKS; index_link++)
        {
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                m2m_hq_buffer_start[index_link][index_ent] = HQ_data_base + (index_link * HEADER_QUEUE_SIZE) \
                                                              + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
            m2m_hq_meta_start[index_link]   = HQ_meta_base + (index_link * HEADER_QUEUE_METADATA_ENTRY_SIZE);
        }
        if(!strcmp(GlobalVND.DeviceType, "ZR"))
        {
#ifndef ROUTER_RFD    
        DB_data_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS);
        DB_meta_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + DATA_BUFFER_SIZE;
            for(index_ent = 0; index_ent < DATA_BUFFER_ENTRY_NUM; index_ent++)
            {
                m2m_dbp_buffer_start[index_ent] = DB_data_base + (index_ent * HEADER_QUEUE_SIZE) \
                                                             +(index_ent * DATA_BUFFER_ENTRY_SIZE);
                m2m_dbp_meta_start[index_ent]   = DB_meta_base + (index_ent * DATA_BUFFER_METADATA_ENTRY_SIZE);
            }
            
            //One HQ for recv from router, One HQ for send to router
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            {
            m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] = GlobalVND.Shared_memory_address + \
             (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
            m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent] = GlobalVND.Shared_memory_address + \
             (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE \
             + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
            }

            m2m_hq_meta_start[NODE_MAX_LINKS]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + HEADER_QUEUE_SIZE; 
            m2m_hq_meta_start[NODE_MAX_LINKS + 1]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE \
                          + HEADER_QUEUE_SIZE; 

            //For small size transmission
            m2m_hq_conflag_start[GlobalVND.DeviceID] = GlobalVND.Shared_memory_address + \
                        (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE *2;
#endif
        }
        else
        {
        DB_data_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS);
        DB_meta_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + DATA_BUFFER_SIZE;
            for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            {
                m2m_dbp_buffer_start[index] = DB_data_base + (index * DATA_BUFFER_ENTRY_SIZE);
                m2m_dbp_meta_start[index]   = DB_meta_base + (index * DATA_BUFFER_METADATA_ENTRY_SIZE);
            }
            
            //One HQ for recv from router, One HQ for send to router
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            {
            m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] = GlobalVND.Shared_memory_address + \
             (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
            m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent] = GlobalVND.Shared_memory_address + \
             (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE \
             + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
            }

            m2m_hq_meta_start[NODE_MAX_LINKS]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + HEADER_QUEUE_SIZE; 
            m2m_hq_meta_start[NODE_MAX_LINKS + 1]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE \
                          + HEADER_QUEUE_SIZE; 

            //For small size transmission
            m2m_hq_conflag_start[GlobalVND.DeviceID] = GlobalVND.Shared_memory_address + \
                        (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE *2;
        }
    }
    
        for(index = 0; index < GlobalVND.NeighborNum; index++)
        {
            long remote_location;
            remote_location = NODE_SHM_LOCATION[GlobalVND.Neighbors[index]];
            fprintf(stderr, "NODE_SHM_LOCATION[%d(GlobalVND.Neighbors[%d]))] = %ld\n",GlobalVND.Neighbors[index], index, NODE_SHM_LOCATION[GlobalVND.Neighbors[index]]);
            int count = 0;
            //Calculate receiver header queue offset
            for(index_ent = 1; index_ent < GlobalVND.DeviceID; index_ent++)
                if(NODE_MAP[GlobalVND.Neighbors[index]][index_ent])
                    count++;
            //fprintf(stderr, "%d header queue in Neighbor %d(%s)'s offset is:%d\n", GlobalVND.DeviceID, \
                    GlobalVND.Neighbors[index], NODE_TYPE[GlobalVND.Neighbors[index]], count);

            if(!strcmp(NODE_TYPE[GlobalVND.Neighbors[index]], "ZED"))
            {
                for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                m2m_remote_hq_buffer_start[index][index_ent] = remote_location + /*(HEADER_QUEUE_SIZE * count)*/ + \
                                                               (index_ent * HEADER_QUEUE_ENTRY_SIZE );
                m2m_remote_meta_start[index] = remote_location + HEADER_QUEUE_SIZE ;
            }
            else
            {
                for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                m2m_remote_hq_buffer_start[index][index_ent] = remote_location + (HEADER_QUEUE_SIZE * count) \
                                                               + (index_ent * HEADER_QUEUE_ENTRY_SIZE);
                m2m_remote_meta_start[index] = remote_location + (HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + \
                                              (HEADER_QUEUE_METADATA_ENTRY_SIZE * count);
            }
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                fprintf(stderr, "m2m_remote_hq_buffer_start[%d][%d] = %ld\n",index ,index_ent,\
                                                    m2m_remote_hq_buffer_start[index][index_ent] /*- remote_location*/);
            fprintf(stderr, "m2m_remote_meta_start[%d] = %ld\n",index \
                                                               ,m2m_remote_meta_start[index] /*- remote_location*/);
        }
//#ifdef  M2MDEBUG
        //[DEBUG] Display shared memory layout (w/o base offset (GlobalVND.Shared_memory_address))
    /*
    if(!strcmp(GlobalVND.DeviceType, "ZED")) //End device layout
    {
        for(index = 0; index < HEADER_QUEUE_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d]  = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                  index, m2m_hq_buffer_start[0][index] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]    = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                    0, m2m_hq_meta_start[0] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start    = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                        m2m_hq_conflag_start[GlobalVND.DeviceID] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index] - GlobalVND.Shared_memory_address);
    }

        //[DEBUG] Display shared memory layout (w/o base offset (GlobalVND.Shared_memory_address))
    else if(!strcmp(GlobalVND.DeviceType, "ZR") || !strcmp(GlobalVND.DeviceType, "ZC")) //Router & Coordinator layout
    {
        for(index_link = 0; index_link < NODE_MAX_LINKS; index_link++)
        {
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, \
GlobalVND.DeviceID, index_link, index_ent, m2m_hq_buffer_start[index_link][index_ent] - GlobalVND.Shared_memory_address);
        }
        for(index_link = 0; index_link < NODE_MAX_LINKS; index_link++)
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                        index_link, m2m_hq_meta_start[index_link] - GlobalVND.Shared_memory_address);
        if(!strcmp(GlobalVND.DeviceType, "ZR"))
        {
#ifndef ROUTER_RFD
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]     = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index] - GlobalVND.Shared_memory_address);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
            NODE_MAX_LINKS, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS, m2m_hq_meta_start[NODE_MAX_LINKS] - GlobalVND.Shared_memory_address);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
            NODE_MAX_LINKS + 1, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1 , m2m_hq_meta_start[NODE_MAX_LINKS + 1] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                            m2m_hq_conflag_start[GlobalVND.DeviceID] - GlobalVND.Shared_memory_address);
#endif
        }
        else
        {
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]     = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index] - GlobalVND.Shared_memory_address);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
            NODE_MAX_LINKS, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS, m2m_hq_meta_start[NODE_MAX_LINKS] - GlobalVND.Shared_memory_address);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
            NODE_MAX_LINKS + 1, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1 , m2m_hq_meta_start[NODE_MAX_LINKS + 1] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                            m2m_hq_conflag_start[GlobalVND.DeviceID] - GlobalVND.Shared_memory_address);
        }
    }*/
    

        
    if(!strcmp(GlobalVND.DeviceType, "ZED")) //End device layout
    {
        for(index = 0; index < HEADER_QUEUE_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d]  = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                  index, m2m_hq_buffer_start[0][index]);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]    = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                    0, m2m_hq_meta_start[0]);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start    = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                                         m2m_hq_conflag_start[GlobalVND.DeviceID]);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index]);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index]);
    }

        //[DEBUG] Display shared memory layout (w/o base offset (GlobalVND.Shared_memory_address))
    else if(!strcmp(GlobalVND.DeviceType, "ZR") || !strcmp(GlobalVND.DeviceType, "ZC")) //Router & Coordinator layout
    {
        for(index_link = 0; index_link < NODE_MAX_LINKS; index_link++)
        {
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
                fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, \
GlobalVND.DeviceID, index_link, index_ent, m2m_hq_buffer_start[index_link][index_ent]);
        }
        for(index_link = 0; index_link < NODE_MAX_LINKS; index_link++)
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                        index_link, m2m_hq_meta_start[index_link]);
        if(!strcmp(GlobalVND.DeviceType, "ZR"))
        {
#ifndef ROUTER_RFD
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index]);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]     = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index]);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent]);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS , m2m_hq_meta_start[NODE_MAX_LINKS]);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent]);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1, m2m_hq_meta_start[NODE_MAX_LINKS + 1]);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                                         m2m_hq_conflag_start[GlobalVND.DeviceID]);
#endif
        }
        else
        {
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index]);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]     = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index]);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent]);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS , m2m_hq_meta_start[NODE_MAX_LINKS]);
        for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d][%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS + 1][index_ent]);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  NODE_MAX_LINKS + 1, m2m_hq_meta_start[NODE_MAX_LINKS + 1]);
            fprintf(stderr, "[%s][%d] m2m_hq_conflag_start      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID,
                                                         m2m_hq_conflag_start[GlobalVND.DeviceID]);
        }
    }
    
//#endif

    M2M_DBG(level, GENERAL, "Exit m2m_dbp_init() ...");
    return M2M_SUCCESS;
}

M2M_ERR_T m2m_dbp_exit()
{
    int ind,ind2;
    int level = 2;
    M2M_DBG(level, GENERAL, "Enter m2m_dbp_exit() ...");
    //Clean GlobalVND data
    //GlobalVND.DeviceID = 0;
    for(ind = 0; ind < 3; ind++)
        GlobalVND.DeviceType[ind] = NULL;  
    for(ind = 0; ind < MAX_NODE_NUM; ind++)
        for(ind2 = 0; ind2 < 4; ind2++)
            NODE_TYPE[ind][ind2] = NULL;
    M2M_DBG(level, GENERAL, "Exit m2m_dbp_exit() ...");
    return M2M_SUCCESS;
}
