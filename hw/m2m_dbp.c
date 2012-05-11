#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"
#include "m2m_route_processor.h"

extern long NODE_SHM_LOCATION[MAX_NODE_NUM]; //Each node shared memory address
extern unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM]; //Each node link map
extern char NODE_TYPE[MAX_NODE_NUM][4];
extern VND GlobalVND;

long m2m_dbp_pool_size = DATA_BUFFER_SIZE;
long m2m_dbp_buffer_start[DATA_BUFFER_ENTRY_NUM];
long m2m_dbp_meta_size = DATA_BUFFER_METADATA_SIZE;
long m2m_dbp_meta_start[DATA_BUFFER_ENTRY_NUM];

long m2m_hq_meta_size = HEADER_QUEUE_METADATA_SIZE;
long m2m_hq_meta_start[NODE_MAX_LINKS + 1];
long m2m_hq_buffer_size = HEADER_QUEUE_SIZE;
long m2m_hq_buffer_start[NODE_MAX_LINKS + 1][HEADER_QUEUE_ENTRY_NUM];


long m2m_remote_meta_start[NODE_MAX_LINKS];
long m2m_remote_hq_buffer_start[NODE_MAX_LINKS];
M2M_ERR_T m2m_copy_to_dbp(void *data, int sizeb,int DeviceID)
{
    volatile m2m_HQ_meta_t *local_meta_ptr = NULL; 

    return M2M_SUCCESS;
}
M2M_ERR_T m2m_dbp_init()
{
    int level = 2;
    M2M_DBG(level, GENERAL, " In m2m_dbp_init....");
    int index, index_link, index_ent;
    int DB_data_base,DB_meta_base;
    int HQ_data_base,HQ_meta_base;

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
        
        DB_data_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE);
        DB_meta_base = GlobalVND.Shared_memory_address + (TOTAL_HEADER_QUEUE_SIZE) + (DATA_BUFFER_SIZE);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
        {
            m2m_dbp_buffer_start[index] = DB_data_base + (index * DATA_BUFFER_ENTRY_SIZE);  
            m2m_dbp_meta_start[index]   = DB_meta_base + (index * DATA_BUFFER_METADATA_ENTRY_SIZE);
        }


//#ifdef  M2MDEBUG
        //[DEBUG] Display shared memory layout (w/o base offset (GlobalVND.Shared_memory_address))
        /* 
        for(index = 0; index < HEADER_QUEUE_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_hq_buffer_start[%d]  = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                  index, m2m_hq_buffer_start[0][index] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]    = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                    0, m2m_hq_meta_start[0] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_buffer_start[%d] = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                 index, m2m_dbp_buffer_start[index] - GlobalVND.Shared_memory_address);
        for(index = 0; index < DATA_BUFFER_ENTRY_NUM; index++)
            fprintf(stderr, "[%s][%d] m2m_dbp_meta_start[%d]   = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                                                   index, m2m_dbp_meta_start[index] - GlobalVND.Shared_memory_address);
        */
//#endif
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
            
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] = GlobalVND.Shared_memory_address \
          + (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + (index_ent * HEADER_QUEUE_ENTRY_SIZE); 
            m2m_hq_meta_start[NODE_MAX_LINKS]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + HEADER_QUEUE_SIZE;
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
            
            for(index_ent = 0; index_ent < HEADER_QUEUE_ENTRY_NUM; index_ent++)
            m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] = GlobalVND.Shared_memory_address + \
             (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + (index_ent * HEADER_QUEUE_ENTRY_SIZE);

            m2m_hq_meta_start[NODE_MAX_LINKS]   = GlobalVND.Shared_memory_address + \
                          (TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + TOTAL_DATA_BUFFER_SIZE + HEADER_QUEUE_SIZE; 
        }

//#ifdef M2MDEBUG

        //[DEBUG] Display shared memory layout (w/o base offset (GlobalVND.Shared_memory_address))
        /*
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
            HEADER_QUEUE_ENTRY_NUM, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  HEADER_QUEUE_ENTRY_NUM, m2m_hq_meta_start[NODE_MAX_LINKS] - GlobalVND.Shared_memory_address);
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
            HEADER_QUEUE_ENTRY_NUM, index_ent, m2m_hq_buffer_start[NODE_MAX_LINKS][index_ent] - GlobalVND.Shared_memory_address);
            fprintf(stderr, "[%s][%d] m2m_hq_meta_start[%d]      = %ld \n", GlobalVND.DeviceType, GlobalVND.DeviceID, \
                  HEADER_QUEUE_ENTRY_NUM, m2m_hq_meta_start[NODE_MAX_LINKS] - GlobalVND.Shared_memory_address);
        }
        */
//#endif

    }
    
        for(index = 0; index < GlobalVND.NeighborNum; index++)
        {
            long remote_location;
            remote_location = NODE_SHM_LOCATION[GlobalVND.Neighbors[index]];
            int count = 0;
            //Calculate receiver header queue offset
            for(index_ent = 1; index_ent < GlobalVND.DeviceID; index_ent++)
                if(NODE_MAP[GlobalVND.Neighbors[index]][index_ent])
                    count++;
            /*fprintf(stderr, "%d header queue in Neighbor %d(%s)'s offset is:%d\n", GlobalVND.DeviceID, \
                    GlobalVND.Neighbors[index], NODE_TYPE[GlobalVND.Neighbors[index]], count);*/

            if(!strcmp(NODE_TYPE[GlobalVND.Neighbors[index]], "ZED"))
            {
                m2m_remote_hq_buffer_start[index] = remote_location;
                m2m_remote_meta_start[index] = remote_location + HEADER_QUEUE_SIZE ;
            }
            else
            {
                m2m_remote_hq_buffer_start[index] = remote_location + (HEADER_QUEUE_SIZE * count);
                m2m_remote_meta_start[index] = remote_location + (HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + \
                                              (HEADER_QUEUE_METADATA_ENTRY_SIZE * count);
            }
            /*fprintf(stderr, "m2m_remote_hq_buffer_start[%d] = %ld\n",index \
                                                                ,m2m_remote_hq_buffer_start[index] - remote_location);
            fprintf(stderr, "m2m_remote_meta_start[%d] = %ld\n",index \
                                                                ,m2m_remote_meta_start[index] - remote_location);*/
        }

    return M2M_SUCCESS;
}
