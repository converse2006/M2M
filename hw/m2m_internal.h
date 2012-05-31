#ifndef _M2M_INTERNAL_H_
#define _M2M_INTERNAL_H_

#include <stdio.h>
#include "m2m.h"
#include "m2m_route_processor.h"

#define M2MDEBUG 
#ifdef M2MDEBUG
#include <assert.h>
#define M2M_DEBUG_LEVEL 2

#define M2M_DBG(level, CATEGORY, str, ...) \
    M2M_DEBUG_##CATEGORY(level, str, ##__VA_ARGS__)

#define M2M_DEBUG_MESSAGE(level, str, ...) \
    if(level <= M2M_DEBUG_LEVEL) \
        fprintf(stderr, "+[M2M DBG] %s:%.3d: " str "\n", __FILE__,__LINE__, ##__VA_ARGS__);

#define M2M_DEBUG_GENERAL(level, str, ...) \
    if(level <= M2M_DEBUG_LEVEL) \
        fprintf(stderr, "+[M2M DBG] " str "\n", ##__VA_ARGS__);

#define M2M_DEBUG_ASSERT(level, str, ...) \
    if (level <= M2M_DEBUG_LEVEL) \
        assert((str));
#else

#define M2M_DBG(level, CATEGORY, str, ...)
#define M2M_DEBUG_MESSAGE(level, str, ...)
#define M2M_DEBUG_GENERAL(level, str, ...)
#define M2M_DEBUG_ASSERT(level, str, ...)

#endif
// ==== "CONSTRUCTOR" CALLS
extern M2M_ERR_T m2m_topology_init(int);
extern M2M_ERR_T m2m_mm_init();
extern M2M_ERR_T m2m_route_processor_init();
extern M2M_ERR_T m2m_dbp_init();
extern M2M_ERR_T m2m_time_init();
extern M2M_ERR_T m2m_send_recv_init();
extern M2M_ERR_T m2m_send(void *, int, int , int );
extern M2M_ERR_T m2m_recv(void *, int, int );
extern unsigned int route_discovery(int , int);
extern uint64_t get_vpmu_time();
extern uint64_t transmission_latency(m2m_HQe_t* , unsigned int, char*);
extern uint64_t time_sync();

// ==== "DESTRUCTOR" CALLS
extern M2M_ERR_T m2m_mm_exit();
extern M2M_ERR_T m2m_route_processor_exit();
extern M2M_ERR_T m2m_dbp_exit();
extern M2M_ERR_T m2m_time_exit();

#endif
