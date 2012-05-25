

#include "sysbus.h"
#include "devices.h"
#include "sysemu.h"
#include "boards.h"
#include "arm-misc.h"
#include "net.h"
#include "string.h"
#include "qemu-queue.h"
#include "qemu-thread.h"
#include "sysbus.h"
#include "hw.h"
#include "qemu-char.h"
#include "console.h"
#include "config-host.h"
#include "qemu-timer.h"
#include "goldfish_device.h" 

#include "m2m.h"
#include "m2m_internal.h"
/***************************
 *  Structure Define
 */
typedef struct net_send{
    int ReceiverID;
    uint32_t DataAddress;
    unsigned int DataSize;
}net_send;

typedef struct net_recv{
    int SenderID;
    uint32_t DataAddress;
}net_recv;

typedef struct VNDState
{
    SysBusDevice    busdev;
    qemu_irq        irq;
    VND             *VND;
    QLIST_ENTRY(VNDState) entries;
}VNDState;

typedef struct VNDState_t
{
    CharDriverState *vpmu_monitor;
    QLIST_HEAD(VPMUStateList, VPMUState) head;
}VNDState_t;

/***************************
 *  Global variable
 */

/* QEMU realted device information, such as BUS, IRQ, VND strucutre. */ 
VNDState_t vnd_t;
struct VND GlobalVND;

/***************************
 *  Internal Function
 */

static void special_read(void *opaque, target_phys_addr_t addr)
{
    int level = 2; //M2M_DBG
    M2M_DBG(level, MESSAGE, "VND: special read");
}

static void special_write(void *opaque, target_phys_addr_t addr, uint32_t value)
{
    int level = 2; //M2M_DBG
    M2M_DBG(level, MESSAGE, "VND: special write,address = %x value is 0x%x(%d) ",addr, value,value);
    VND *vnd = ((VNDState *)opaque)->VND;
    M2M_ERR_T rc;

    switch(addr / NET_DIST)
    {
        case ZIGBEE: {
            switch(addr % NET_DIST)
            { 
                case NETWORK_INIT: {
                            M2M_DBG(level, MESSAGE, "Enter NETWORK_INIT ...");
                            vnd->DeviceID = value;
                            //M2M_DBG(level, MESSAGE, "Network device initialization: Device ID = %d",vnd->DeviceID);

                            rc = m2m_topology_init(vnd->DeviceID);

                            if(rc == M2M_SUCCESS)
                                rc = m2m_mm_init();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_dbp_init();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_route_processor_init();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_send_recv_init();
                            
                            if(rc == M2M_SUCCESS)
                                rc = m2m_time_init();

                            M2M_DBG(level, MESSAGE, "Exit NETWORK_INIT ...");
                        }break;

                case NETWORK_SEND: {
                            M2M_DBG(level, MESSAGE, "Enter NETWORK_SEND ...");
                            net_send* packet;
                            packet = (uint32_t *)v2p(value, 0);
                            m2m_send((uint32_t *)v2p(packet->DataAddress, 0), packet->DataSize, packet->ReceiverID,NULL);
                            M2M_DBG(level, MESSAGE, "Exit NETWORK_SEND ...");
                        }break;

                case NETWORK_RECV: {
                            M2M_DBG(level, MESSAGE, "Enter NETWORK_RECV ...");
                            net_recv* packet;
                            packet = (uint32_t *)v2p(value, 0);
                            m2m_recv((uint32_t *)v2p(packet->DataAddress, 0), -1, NULL);
                            M2M_DBG(level, MESSAGE, "Exit NETWORK_RECV ...");
                        }break;

                case NETWORK_EXIT: {
                            M2M_DBG(level, MESSAGE, "Enter NETWORK_EXIT ...");

                            rc = m2m_route_processor_exit();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_time_exit();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_mm_exit();

                            if(rc == M2M_SUCCESS)
                                rc = m2m_dbp_exit();

                            /*if(rc == M2M_SUCCESS)
                                rc = 
                            */

                            M2M_DBG(level, MESSAGE, "Exit NETWORK_EXIT ...");
                        }break;

                default:break;
            }
          }break;
    }
    
}

static CPUReadMemoryFunc *special_read_func[] = {  
    special_read,
    special_read,
    special_read
};

static CPUWriteMemoryFunc *special_write_func[] = {
    special_write,
    special_write,
    special_write
};

static void vnd_init_core(VNDState *s, CharDriverState *chr)
{
    s->VND = &GlobalVND;
}

static int vnd_init_sysbus(SysBusDevice *dev)
{
    fprintf(stderr,"VND initiate\n");
    VNDState *s = FROM_SYSBUS(VNDState, dev);

    CharDriverState* chr = qdev_init_chardev(&dev->qdev);

    vnd_init_core(s, chr);  //Initalization parameters
    
    int iomemtype = cpu_register_io_memory(special_read_func, special_write_func, s);

    sysbus_init_mmio(dev, VND_IOMEM_SIZE, iomemtype);

    sysbus_init_irq(dev, &s->irq);

    return 0;
}

static void vnd_register_devices(void)
{
    QLIST_INIT(&vnd_t.head);
    sysbus_register_dev("vnd", sizeof(VNDState), vnd_init_sysbus);
}

void vnd_init(uint32_t base, qemu_irq irq)  
{
    fprintf(stderr, "vnd_init !!");
    DeviceState *dev;
    SysBusDevice *s;

    dev = qdev_create(NULL, "vnd");
    qdev_init(dev);
    s = sysbus_from_qdev(dev);
    sysbus_mmio_map(s, 0, base);
    sysbus_connect_irq(s, 0, irq);
}

device_init(vnd_register_devices)
