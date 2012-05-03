#ifndef __VPMU_MODULE_H__
#define __VPMU_MODULE_H__

#define VPMU_MAJOR_NUM 0x7A //122
#define VPMU_MINOR_NUM 0
#define VPMU_MODULE_NAME "VPMU"

#define VPMU_IOCTL_TYPE_MAGIC 0x7A

#define VPMU_IOCTL_ENABLE_R             _IOR(VPMU_IOCTL_TYPE_MAGIC, 0, uint32_t)
#define VPMU_IOCTL_ENABLE_W             _IOW(VPMU_IOCTL_TYPE_MAGIC, 1, uint32_t)
#define VPMU_IOCTL_MEMTRACE_R           _IOR(VPMU_IOCTL_TYPE_MAGIC, 2, uint32_t)
#define VPMU_IOCTL_MEMTRACE_W           _IOW(VPMU_IOCTL_TYPE_MAGIC, 3, uint32_t)
#define VPMU_IOCTL_DUMP_INSN_CNT        _IOW(VPMU_IOCTL_TYPE_MAGIC, 4, uint32_t)
#define VPMU_IOCTL_DUMP_BRANCH_CNT      _IOW(VPMU_IOCTL_TYPE_MAGIC, 5, uint32_t)
#define VPMU_IOCTL_DUMP_DCACHE_CNT      _IOW(VPMU_IOCTL_TYPE_MAGIC, 6, uint32_t)
#define VPMU_IOCTL_DUMP_CYCLE_CNT       _IOW(VPMU_IOCTL_TYPE_MAGIC, 7, uint32_t)
#define VPMU_IOCTL_DUMP_EET             _IOW(VPMU_IOCTL_TYPE_MAGIC, 8, uint32_t)
#define VPMU_IOCTL_SELECT_NET_MODE_R    _IOR(VPMU_IOCTL_TYPE_MAGIC, 9, uint32_t)
#define VPMU_IOCTL_SELECT_NET_MODE_W    _IOW(VPMU_IOCTL_TYPE_MAGIC, 10, uint32_t)
#define VPMU_IOCTL_BATTERY_ENABLE_R     _IOR(VPMU_IOCTL_TYPE_MAGIC, 11, uint32_t)
#define VPMU_IOCTL_BATTERY_ENABLE_W     _IOW(VPMU_IOCTL_TYPE_MAGIC, 12, uint32_t)
#define VPMU_IOCTL_TEST_R               _IOR(VPMU_IOCTL_TYPE_MAGIC, 13, uint32_t)
#define VPMU_IOCTL_TEST_W               _IOW(VPMU_IOCTL_TYPE_MAGIC, 14, uint32_t)

#define VPMU_IOCTL_MAXNR                14

#ifdef CONFIG_PACDSP
#define VPMU_MMAP_START		0x5000f000
#else
#define VPMU_MMAP_START		0xf0000000
#endif

#define VPMU_MMAP_ENABLE            0x0000
#define VPMU_MMAP_MEMTRACE          0x0004
#define VPMU_MMAP_BYPASS_ISR_ADDR   0x0008
#define VPMU_MMAP_POWER_ENABLE      0x0100
#define VPMU_MMAP_BYPASS_CPU_UTIL   0x0104
#define VPMU_MMAP_SELECT_NET_MODE   0x0108
#define VPMU_MMAP_BATTERY_ENABLE    0x010c

#define VPMU_MMAP_END		0x0fff
#define VPMU_MMAP_SIZE		(VPMU_MMAP_END + 1)

#endif
