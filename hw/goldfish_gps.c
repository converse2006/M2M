/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "qemu_file.h"
#include "qemu-char.h"
#include "goldfish_device.h"
#include "qemu_debug.h"

#ifdef CONFIG_VPMU
#include "vpmu.h"
#endif

#define  DEBUG  0

#if DEBUG
#  define  D(...)  fprintf(stderr, __VA_ARGS__)
#else
#  define  D(...)  ((void)0)
#endif

struct goldfish_gps_state {
    struct goldfish_device dev;
    uint32_t status;
};

static uint32_t gps_read(void *opaque, target_phys_addr_t offset)
{
    struct goldfish_gps_state *s = opaque;

    //D("%s:  offset=0x%.4x\n", __func__, offset);

    return 0;
}

static void gps_write(void *opaque, target_phys_addr_t offset, uint32_t val)
{
    struct goldfish_gps_state *s = opaque;

    //D("%s: offset=0x%.4x val=%d\n", __func__, offset, val);
    switch(offset) {
        default:
            s->status = val;
            D("%s: set status to %d\n", __func__, val);
            break;
            /* We don't emulate the other attributes */
    }
}

static CPUReadMemoryFunc *gps_readfn[] = {
   gps_read,
   gps_read,
   gps_read
};

static CPUWriteMemoryFunc *gps_writefn[] = {
   gps_write,
   gps_write,
   gps_write
};

void goldfish_gps_init()
{
    struct goldfish_gps_state *s;

    s = (struct goldfish_gps_state *)qemu_mallocz(sizeof(*s));
    s->dev.name = "goldfish_gps";
    s->dev.id = 0;
    s->dev.base = 0; // will be allocated dynamically
    s->dev.size = 0x1000;
    s->dev.irq_count = 0;

    goldfish_device_add(&s->dev, gps_readfn, gps_writefn, s);

#ifdef CONFIG_VPMU
    GlobalVPMU.adev_stat_ptr->gps_status = &(s->status);
#endif
}

