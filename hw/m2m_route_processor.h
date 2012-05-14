
#ifndef _M2M_ROUTE_PROCESSOR_H_
#define _M2M_ROUTE_PROCESSOR_H_

#define M2M_MAX_HQE_SIZE    128
#define SMALL               118
#define LARGE               112

typedef struct m2m_HQe
{
    unsigned int tag;                        //4 Bytes
    unsigned int PacketSize;                 //4 Bytes
    unsigned int SenderID;                   //4 Bytes
    unsigned int ReceiverID;                 //4 Bytes
    unsigned int ForwardID;                  //4 Bytes
    int EntryNum;                            //4 Bytes
    unsigned long long SendTime;             //8 Bytes
    unsigned long long TransTime             //8 Bytes
}m2m_HQe_t;                                  //Total 40 Bytes


typedef struct m2m_HQ_meta
{
    volatile unsigned int producer;          //4 Bytes
    volatile unsigned int consumer;          //4 Bytes
} m2m_HQ_meta_t;                             //Total 8 Bytes

typedef struct m2m_HQ_cf
{
    volatile unsigned int dataflag;          //4 Bytes
    volatile unsigned long long transtime;   //8 Bytes
}m2m_HQ_cf_t;                                //Total 12 Bytes
typedef struct m2m_DB_meta
{
    volatile unsigned int dataflag;          //4 Bytes
    volatile unsigned long long transtime;   //8 Bytes
} m2m_DB_meta_t;                             //Total 12 Bytes
#endif
