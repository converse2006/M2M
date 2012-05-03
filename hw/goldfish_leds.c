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

#define LEDS_BRIGHTNESS 0x08

struct goldfish_leds_state {
    struct goldfish_device dev;
    uint32_t brightness;
};

static uint32_t leds_read(void *opaque, target_phys_addr_t offset)
{
    struct goldfish_leds_state *s = opaque;

    //D("%s:  offset=0x%.4x\n", __func__, offset);

    return 0;
}

static void leds_write(void *opaque, target_phys_addr_t offset, uint32_t val)
{
    struct goldfish_leds_state *s = opaque;

    //D("%s: offset=0x%.4x val=%d\n", __func__, offset, val);
    switch(offset) {
        case LEDS_BRIGHTNESS:
            s->brightness = val;
            D("%s: set brightness to %d\n", __func__, val);
            break;
        default:
            /* We don't emulate the other attributes */
            break;
    }
}

static CPUReadMemoryFunc *leds_readfn[] = {
   leds_read,
   leds_read,
   leds_read
};

static CPUWriteMemoryFunc *leds_writefn[] = {
   leds_write,
   leds_write,
   leds_write
};

void goldfish_leds_init(uint32_t base)
{
    struct goldfish_leds_state *s;

    s = (struct goldfish_leds_state *)qemu_mallocz(sizeof(*s));
    s->dev.name = "goldfish_leds";
    s->dev.id = 0;
    s->dev.base = base;
    s->dev.size = 0x1000;
    s->dev.irq_count = 0;

    goldfish_device_add(&s->dev, leds_readfn, leds_writefn, s);

#ifdef CONFIG_VPMU
    GlobalVPMU.adev_stat_ptr->lcd_brightness = &(s->brightness);
#endif
}

