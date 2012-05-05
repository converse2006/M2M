
#ifndef _M2M_MM_H_
#define _M2M_MM_H_

#include "m2m.h" 

#define x86_SHM_KEY                  1122

#define DATA_BUFFER_ENTRY_SIZE       4096  
#define DATA_BUFFER_ENTRY_NUM        4
#define DATA_BUFFER_SIZE             (DATA_BUFFER_ENTRY_SIZE * DATA_BUFFER_ENTRY_NUM)
#define DATA_BUFFER_METADATA         DATA_BUFFER_ENTRY_NUM * 3 * 4 //DataFlag(Byte) + TansTime(2Bytes)
#define TOTAL_DATA_BUFFER_SIZE       DATA_BUFFER_SIZE + DATA_BUFFER_METADATA

#define NODE_MAX_LINKS               8
#define HEADER_QUEUE_ENTRY_SIZE      128
#define HEADER_QUEUE_ENTRY_NUM       4
#define HEADER_QUEUE_SIZE            (HEADER_QUEUE_ENTRY_NUM * HEADER_QUEUE_ENTRY_SIZE)
#define HEADER_QUEUE_METADATA        2 * 4 //ProducerInd + ConsumerInd
#define TOTAL_HEADER_QUEUE_SIZE      (HEADER_QUEUE_SIZE + HEADER_QUEUE_METADATA)

#define END_DEVICE_SIZE              (TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE)
#define COORDINATOR_SIZE             (( TOTAL_HEADER_QUEUE_SIZE * 8) + END_DEVICE_SIZE)

#ifdef ROUTER_RFD
#define ROUTER_SIZE                  (NODE_MAX_LINKS * TOTAL_HEADER_QUEUE_SIZE)
#else
#define ROUTER_SIZE                  COORDINATOR_SIZE
#endif 



               
//#define SHM_SIZE                     MAX_NODE_NUM * (DATA_BUFFER_ + (MAX_NODE_NUM - 1) * HEADER_QUEUE_PER_NODE + METADATA_PER_NODE )


#endif
