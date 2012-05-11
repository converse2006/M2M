
#ifndef _M2M_ROUTE_PROCESSOR_H_
#define _M2M_ROUTE_PROCESSOR_H_

#define M2M_MAX_HQE_SIZE    128
#define SMALL               118
#define LARGE               112

typedef struct m2m_HQe
{
    unsigned int index;
    unsigned int len;
    unsigned int tag;
    unsigned int sid;
    unsigned int rid;
}m2m_HQe_t;

typedef struct m2m_HQ_meta
{
    volatile unsigned int producer;
    volatile unsigned int consumer;
   // volatile unsigned int pad[6];
} m2m_HQ_meta_t; // aligned 32  

typedef struct m2m_DB_meta
{
    volatile unsigned int dataflag;
    volatile unsigned long long transtime;
} m2m_DB_meta_t; 
#endif
