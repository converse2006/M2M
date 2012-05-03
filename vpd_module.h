#ifndef __VPD_MODULE_H__
#define __VPD_MODULE_H__

#define VPD_MAJOR_NUM 0x7B //123
#define VPD_MINOR_NUM 0
#define VPD_MODULE_NAME "VPD"

#define VPD_IOCTL_TYPE_MAGIC 0x7B

#ifdef CONFIG_PACDSP
#define VPD_IOCTL_PLL1_CR_R     _IOR(VPD_IOCTL_TYPE_MAGIC,  0, uint32_t)
#define VPD_IOCTL_PLL1_CR_W     _IOW(VPD_IOCTL_TYPE_MAGIC,  1, uint32_t)
#define VPD_IOCTL_PLL1_SR_R     _IOR(VPD_IOCTL_TYPE_MAGIC,  2, uint32_t)

#define VPD_IOCTL_CG_DSP1TPR_R  _IOR(VPD_IOCTL_TYPE_MAGIC,  3, uint32_t)
#define VPD_IOCTL_CG_DSP1TPR_W  _IOW(VPD_IOCTL_TYPE_MAGIC,  4, uint32_t)
#define VPD_IOCTL_CG_DSP2TPR_R  _IOR(VPD_IOCTL_TYPE_MAGIC,  5, uint32_t)
#define VPD_IOCTL_CG_DSP2TPR_W  _IOW(VPD_IOCTL_TYPE_MAGIC,  6, uint32_t)
#define VPD_IOCTL_CG_DDRTPR_R   _IOR(VPD_IOCTL_TYPE_MAGIC,  7, uint32_t)
#define VPD_IOCTL_CG_DDRTPR_W   _IOW(VPD_IOCTL_TYPE_MAGIC,  8, uint32_t)
#define VPD_IOCTL_CG_AXITPR_R   _IOR(VPD_IOCTL_TYPE_MAGIC,  9, uint32_t)
#define VPD_IOCTL_CG_AXITPR_W   _IOW(VPD_IOCTL_TYPE_MAGIC, 10, uint32_t)

#define VPD_IOCTL_CG_DSP1CPR_R  _IOR(VPD_IOCTL_TYPE_MAGIC, 11, uint32_t)
#define VPD_IOCTL_CG_DSP2CPR_R  _IOR(VPD_IOCTL_TYPE_MAGIC, 12, uint32_t)
#define VPD_IOCTL_CG_DDRCPR_R   _IOR(VPD_IOCTL_TYPE_MAGIC, 13, uint32_t)
#define VPD_IOCTL_CG_AXICPR_R   _IOR(VPD_IOCTL_TYPE_MAGIC, 14, uint32_t)

#define VPD_IOCTL_PMU_LCD_CR_R  _IOR(VPD_IOCTL_TYPE_MAGIC, 15, uint32_t)
#define VPD_IOCTL_PMU_LCD_CR_W  _IOW(VPD_IOCTL_TYPE_MAGIC, 16, uint32_t)

#define VPD_IOCTL_VC1_TRCR_R    _IOR(VPD_IOCTL_TYPE_MAGIC, 17, uint32_t)
#define VPD_IOCTL_VC1_TRCR_W    _IOW(VPD_IOCTL_TYPE_MAGIC, 18, uint32_t)
#define VPD_IOCTL_VC1_SR_R      _IOR(VPD_IOCTL_TYPE_MAGIC, 19, uint32_t)
#define VPD_IOCTL_VC1_WDR_R     _IOR(VPD_IOCTL_TYPE_MAGIC, 20, uint32_t)
#define VPD_IOCTL_VC1_WDR_W     _IOW(VPD_IOCTL_TYPE_MAGIC, 21, uint32_t)
#define VPD_IOCTL_VC1_CR_R      _IOR(VPD_IOCTL_TYPE_MAGIC, 22, uint32_t)
#define VPD_IOCTL_VC1_CR_W      _IOW(VPD_IOCTL_TYPE_MAGIC, 23, uint32_t)

#define VPD_IOCTL_VC2_TRCR_R    _IOR(VPD_IOCTL_TYPE_MAGIC, 24, uint32_t)
#define VPD_IOCTL_VC2_TRCR_W    _IOW(VPD_IOCTL_TYPE_MAGIC, 25, uint32_t)
#define VPD_IOCTL_VC2_SR_R      _IOR(VPD_IOCTL_TYPE_MAGIC, 26, uint32_t)
#define VPD_IOCTL_VC2_WDR_R     _IOR(VPD_IOCTL_TYPE_MAGIC, 27, uint32_t)
#define VPD_IOCTL_VC2_WDR_W     _IOW(VPD_IOCTL_TYPE_MAGIC, 28, uint32_t)
#define VPD_IOCTL_VC2_CR_R      _IOR(VPD_IOCTL_TYPE_MAGIC, 29, uint32_t)
#define VPD_IOCTL_VC2_CR_W      _IOW(VPD_IOCTL_TYPE_MAGIC, 30, uint32_t)

#define VPD_IOCTL_MAXNR         30

/*
 * vpmu use 0x5000f000~0x5000ffff
 * vpd  use 0x50010000~0x50010fff
 */
#define VPD_BASE_ADDR           0x50010000

#define VPD_MMAP_PLL1_CR        0x04 //RW
#define VPD_MMAP_PLL1_SR        0x08 //R
#define VPD_MMAP_RST_SYSR       0x20 //RW Forbiddance

#define VPD_MMAP_CG_ARMBUSTPR   0x30 //R  Forbiddance
#define VPD_MMAP_CG_DSP1TPR     0x34 //RW
#define VPD_MMAP_CG_DSP2TPR     0x38 //RW
#define VPD_MMAP_CG_DDRTPR      0x3C //RW
#define VPD_MMAP_CG_AXITPR      0x40 //RW

#define VPD_MMAP_CG_ARMBUSCPR   0x50 //R  Forbiddance
#define VPD_MMAP_CG_DSP1CPR     0x54 //R
#define VPD_MMAP_CG_DSP2CPR     0x58 //R
#define VPD_MMAP_CG_DDRCPR      0x5C //R
#define VPD_MMAP_CG_AXICPR      0x60 //R

#define VPD_MMAP_AHB_TPR        0x44 //RW Reserved

#define VPD_MMAP_PMU_SUSPEND    0xb0 //RW Forbiddance
#define VPD_MMAP_PMU_LCD_CR     0xc0 //RW

#define VPD_MMAP_VC1_TRCR       0x74 //RW
#define VPD_MMAP_VC1_SR         0x84 //R
#define VPD_MMAP_VC1_WDR        0x7c //RW
#define VPD_MMAP_VC1_CR         0x70 //RW

#define VPD_MMAP_VC2_TRCR       0x94 //RW
#define VPD_MMAP_VC2_SR         0xa4 //R
#define VPD_MMAP_VC2_WDR        0x9c //RW
#define VPD_MMAP_VC2_CR         0x90 //RW

#define VPD_MMAP_END            0x0fff
#define VPD_MMAP_SIZE           (VPD_MMAP_END + 1)

#else
/*
 * vpmu use 0xf0000000~0xf0000fff
 * vpd  use 0xf0001000~0xf0001fff
 */
#define VPD_BASE_ADDR		0xf0001000

#endif

#endif
