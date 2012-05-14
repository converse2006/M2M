#ifndef _M2M_INTERNAL_H_
#define _M2M_INTERNAL_H_

#include "m2m.h"
// ==== "CONSTRUCTOR" CALLS
extern M2M_ERR_T m2m_topology_init(int);
extern M2M_ERR_T m2m_mm_init();
extern M2M_ERR_T m2m_mm_exit();
extern M2M_ERR_T m2m_route_processor_init();
extern M2M_ERR_T m2m_route_processor_exit();
extern M2M_ERR_T m2m_dbp_init();
extern M2M_ERR_T m2m_dbp_exit();
extern M2M_ERR_T m2m_send(void *, int, int , int );
extern M2M_ERR_T m2m_recv(void *, int, int );
extern M2M_ERR_T m2m_send_recv_init();
extern int route_discovery(int , int);


#endif
