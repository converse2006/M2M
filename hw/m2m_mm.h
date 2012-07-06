/* 
 * Copyright (C) 2012 PASLab CSIE NTU. All rights reserved.
 *      - Chen Chun-Han <converse2006@gmail.com>
 */

#ifndef _M2M_MM_H_
#define _M2M_MM_H_

#include "m2m.h" 

#define x86_SHM_KEY                         8175
//DATA BUFFER
#define DATA_BUFFER_ENTRY_SIZE              4096  
#define DATA_BUFFER_ENTRY_NUM               4
#define DATA_BUFFER_SIZE                    (DATA_BUFFER_ENTRY_SIZE * DATA_BUFFER_ENTRY_NUM)
#define DATA_BUFFER_METADATA_ENTRY_SIZE     (4 + 8)      //DataFlag(4Bytes) + TansTime(8Bytess)
#define DATA_BUFFER_METADATA_SIZE           (DATA_BUFFER_ENTRY_NUM * DATA_BUFFER_METADATA_ENTRY_SIZE)
#define TOTAL_DATA_BUFFER_SIZE              (DATA_BUFFER_SIZE + DATA_BUFFER_METADATA_SIZE)

//HEADER QUEUE
#define NODE_MAX_LINKS                      8
#define HEADER_QUEUE_ENTRY_SIZE             128
#define HEADER_QUEUE_ENTRY_NUM              4
#define HEADER_QUEUE_SIZE                   (HEADER_QUEUE_ENTRY_NUM * HEADER_QUEUE_ENTRY_SIZE)
#define HEADER_QUEUE_METADATA_SIZE          (4 + 4 + 24)     //ProducerInd(4Bytes) + ConsumerInd(4Bytes)
#define TOTAL_HEADER_QUEUE_SIZE             (HEADER_QUEUE_SIZE + HEADER_QUEUE_METADATA_SIZE)

//HEADER QUEUE CONTROL FLAG
#define HEADER_QUEUE_CONTRLFLAG             (4 + 8)  //DataFlag(4Bytes) + TansTime(8Bytess)

//Zigbee device
#define END_DEVICE_SIZE                     (TOTAL_DATA_BUFFER_SIZE + TOTAL_HEADER_QUEUE_SIZE + HEADER_QUEUE_CONTRLFLAG)
#define COORDINATOR_SIZE                    (( TOTAL_HEADER_QUEUE_SIZE * NODE_MAX_LINKS) + END_DEVICE_SIZE + \
                                            TOTAL_HEADER_QUEUE_SIZE)

#ifdef ROUTER_RFD
#define ROUTER_SIZE                         (NODE_MAX_LINKS * TOTAL_HEADER_QUEUE_SIZE)
#else
#define ROUTER_SIZE                         COORDINATOR_SIZE
#endif 

#endif
