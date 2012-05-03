/*
 * Smart Event Tracing (SET) device
 * Copyright (C) 2010 PAS Lab, CSIE & GINM, National Taiwan University, Taiwan.
 */

#include <assert.h>
#include <pthread.h>
#include "sysbus.h"
#include "SET/SET.h"
#include "SET/SET_event.h"
#include "SET/SET_ctrl.h"
#include "SET_device.h"

typedef struct SETState {
    SysBusDevice    busdev;
    qemu_irq        irq;
} SETState;

static ProcessType type;
static pthread_t ctrl_tid;

static uint32_t SET_read(void *opaque, target_phys_addr_t addr)
{
    SETProcess *tracing_process = set.tracing_process;

    switch (addr)
    {
        case SET_DEV_REG_SET_CTRL:
            if (tracing_process)
                return 1;
            else
                return 0;
        default:
            fprintf(stderr, "SET device: read unknown address %d\n", addr);
            return 0;
    }
}

static void SET_write(void *opaque, target_phys_addr_t addr, uint32_t val)
{
    SETProcess *process;
    char *name;

    switch (addr)
    {
        case SET_DEV_REG_VPMU_SETUP:
            set.model = val;
            break;

        case SET_DEV_REG_EVENT_TYPE:
            type = (ProcessType) val;
            break;

        case SET_DEV_REG_EVENT_NAME:
            name = (char *) v2p(val, 0);

			//evo0209
			if (!strcmp(name, "stop"))
				isSamplingOn = 0;

            process = SET_create_process(name, type);
            if (process != NULL)
                SET_attach_process(process);
            break;

        case SET_DEV_REG_APP_NAME:
            name = (char *) v2p(val, 0);

            /* Quick verification of "<pre-initialized>" */
            if (name[0] != '<')
                SET_event_start_dalvik(name);
            break;

        /* Dalvik method */
        case SET_DEV_REG_METHOD_ENTRY:
            SET_event_method_entry(val);
            break;

        case SET_DEV_REG_METHOD_EXIT:
            SET_event_method_exit(val);
            break;

		//tianman
		case SET_DEV_REG_JNI_ENTRY:    
			SET_event_jni_entry();         
			break;

		case SET_DEV_REG_JNI_EXIT:     
			SET_event_jni_exit();          
			break;


        default:
            fprintf(stderr, "SET device: write unknown address %d\n", addr);
    }
}

static CPUReadMemoryFunc *SET_read_func[] = {
    (CPUReadMemoryFunc*) SET_read,
    (CPUReadMemoryFunc*) SET_read,
    (CPUReadMemoryFunc*) SET_read
};

static CPUWriteMemoryFunc *SET_write_func[] = {
    (CPUWriteMemoryFunc*) SET_write,
    (CPUWriteMemoryFunc*) SET_write,
    (CPUWriteMemoryFunc*) SET_write
};

static void SET_init_sysbus(SysBusDevice *dev)
{
    SETState *s = FROM_SYSBUS(SETState, dev);

    int iomemtype = cpu_register_io_memory(SET_read_func, SET_write_func, s);

    sysbus_init_mmio(dev, SET_IOMEM_SIZE, iomemtype);

    sysbus_init_irq(dev, &s->irq);

    /* Start SET */
    SET_init();
    /* Create a thread to wait for an external control program */
    pthread_create(&ctrl_tid, NULL, SET_ctrl_thread, NULL);
}

static void SET_register_devices(void)
{
    sysbus_register_dev("SET", sizeof(SETState), SET_init_sysbus);
}

/*
 * Legacy helper function. Should go away when machine config files are implemented
 * This function will call SET_init_sysbus() automatically
 */
void SET_device_init(uint32_t base, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *s;

    dev = qdev_create(NULL, "SET");
    qdev_init(dev);
    s = sysbus_from_qdev(dev);
    sysbus_mmio_map(s, 0, base);
}

device_init(SET_register_devices);
