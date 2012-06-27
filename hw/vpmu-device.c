/*
 * Virtual Performance Monitor Unit
 * Copyright (c) 2009 PAS Lab, CSIE & GINM, National Taiwan University, Taiwan.
 */
#include "sysbus.h"
//#include "primecell.h"
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

#include <stdio.h>
#include <locale.h>

#include "vpmu.h"
#include "../SET/SET_event.h"
#include "qemu-timer.h"

#include "goldfish_device.h" //goldfish_battery_set_prop()
#include "power_supply.h" //POWER_SUPPLY_PROP_CAPACITY

#if defined(CONFIG_PACDSP)
#include "pacdsp.h"
#endif

#ifdef CONFIG_VND 
#include "m2m.h"
extern VND GlobalVND;
#endif 

//#define VPMU_DEBUG_MSG  1

/* Macro to enable/disable debug message for VPMU related code. */
#ifdef VPMU_DEBUG_MSG
    #define SIMPLE_DBG(str,...) fprintf(stderr, str,##__VA_ARGS__); \
                                fflush(stderr);
#else
    #define SIMPLE_DBG(str,...) {}
#endif

#define ERR_MSG(str,...)    fprintf(stderr, str,##__VA_ARGS__); \
                            fflush(stderr);

#define FDBG(str,...) do{\
    qemu_chr_printf(vpmu_t.vpmu_monitor, str "\r", ##__VA_ARGS__);\
    qemu_chr_printf(serial_hds[0],str "\r", ##__VA_ARGS__);\
    fprintf(fp, str, ##__VA_ARGS__);\
}while(0)

#define DBG(str,...) do{\
    qemu_chr_printf(vpmu_t.vpmu_monitor, str "\r", ##__VA_ARGS__);\
    qemu_chr_printf(serial_hds[0],str "\r", ##__VA_ARGS__);\
    fprintf(stderr,str,##__VA_ARGS__);\
}while(0)

#define RAWDBG(str,...) do{\
    qemu_chr_printf(vpmu_t.vpmu_monitor, str , ##__VA_ARGS__);\
    qemu_chr_printf(serial_hds[0],str , ##__VA_ARGS__);\
    fprintf(stderr,str,##__VA_ARGS__);\
}while(0)

#define VPMU_DUMP_READABLE_MSG() do{\
    FDBG("Instructions:\n");\
    FDBG(" Total instruction count       : %s\n", commaprint(vpmu_total_insn_count()));\
    FDBG("  ->User mode insn count       : %s\n", commaprint(vpmu->USR_icount));\
    FDBG("  ->Supervisor mode insn count : %s\n", commaprint(vpmu->SVC_icount));\
    FDBG("  ->IRQ mode insn count        : %s\n", commaprint(vpmu->IRQ_icount));\
    FDBG("  ->Other mode insn count      : %s\n", commaprint(vpmu->sys_rest_icount));\
    FDBG(" Total load/store instruction count : %s\n", commaprint(vpmu_total_ldst_count()));\
    FDBG("  ->User mode ld/st count      : %s\n", commaprint(vpmu->USR_ldst_count));\
    FDBG("  ->Supervisor mode ld/st count: %s\n", commaprint(vpmu->SVC_ldst_count));\
    FDBG("  ->IRQ mode ld/st count       : %s\n", commaprint(vpmu->IRQ_ldst_count));\
    FDBG("  ->Other mode ld/st count     : %s\n", commaprint(vpmu->sys_rest_ldst_count));\
    FDBG(" Total load instruction count  : %s\n", commaprint(vpmu_total_load_count()));\
    FDBG("  ->User mode load count       : %s\n", commaprint(vpmu->USR_load_count));\
    FDBG("  ->Supervisor mode load count : %s\n", commaprint(vpmu->SVC_load_count));\
    FDBG("  ->IRQ mode load count        : %s\n", commaprint(vpmu->IRQ_load_count));\
    FDBG("  ->Other mode load count      : %s\n", commaprint(vpmu->sys_rest_load_count));\
    FDBG(" Total store instruction count : %s\n", commaprint(vpmu_total_store_count()));\
    FDBG("  ->User mode store count      : %s\n", commaprint(vpmu->USR_store_count));\
    FDBG("  ->Supervisor mode store count: %s\n", commaprint(vpmu->SVC_store_count));\
    FDBG("  ->IRQ mode store count       : %s\n", commaprint(vpmu->IRQ_store_count));\
    FDBG("  ->Other mode store count     : %s\n", commaprint(vpmu->sys_rest_store_count));\
    FDBG(" Branch instruction count      : %s\n", commaprint(vpmu_branch_insn_count()));\
    FDBG("  ->Branch predict correct     : %s\n", commaprint(vpmu_branch_predict(PREDICT_CORRECT)));\
    FDBG("  ->Branch predict wrong       : %s\n", commaprint(vpmu_branch_predict(PREDICT_WRONG)));\
\
    FDBG("MISC:\n");\
    FDBG("  ->All changed to the PC      : %s\n", commaprint(vpmu->all_changes_to_PC_count));\
\
	FDBG("Cache:\n");\
	FDBG(" L1 D-cache access count : %s\n", commaprint((uint64_t)(vpmu->d4_levcache[2][0]->fetch[D4XREAD] + \
																	vpmu->d4_levcache[2][0]->fetch[D4XWRITE])));\
	FDBG("  ->Read miss count   : %s\n", commaprint((uint64_t)vpmu->d4_levcache[2][0]->miss[D4XREAD]));\
	FDBG("  ->Write miss count  : %s\n", commaprint((uint64_t)vpmu->d4_levcache[2][0]->miss[D4XWRITE]));\
	FDBG(" L1 I-cache access count : %s\n", commaprint((uint64_t)(vpmu->d4_levcache[1][0]->fetch[D4XINSTRN] + \
																	vpmu->additional_icache_access)));\
	FDBG("  ->Read miss count   : %s\n", commaprint((uint64_t)vpmu->d4_levcache[1][0]->miss[D4XINSTRN]));\
	if (vpmu->d4_maxlevel == 2) \
	{ \
		FDBG(" L2 U-cache access count : %s\n", commaprint((uint64_t)(vpmu->d4_levcache[0][1]->fetch[D4XREAD] + \
																		vpmu->d4_levcache[0][1]->fetch[D4XWRITE])));\
		FDBG("  ->Read miss count   : %s\n", commaprint((uint64_t)vpmu->d4_levcache[0][1]->miss[D4XREAD]));\
		FDBG("  ->Write miss count  : %s\n", commaprint((uint64_t)vpmu->d4_levcache[0][1]->miss[D4XWRITE]));\
	} \
\
    FDBG("Memory:\n");\
    FDBG("  ->System memory access  : %s\n", commaprint((uint64_t)(vpmu_L1_dcache_miss_count() + vpmu_L1_icache_miss_count())));\
    FDBG("  ->System memory cycles  : %s\n", commaprint((uint64_t)vpmu_sys_memory_access_cycle_count()));\
    /*FDBG("  ->I/O memory access     : %s\n", commaprint((uint64_t)vpmu->iomem_count));*/\
    FDBG("  ->I/O memory access     : %s\n", commaprint((uint64_t)vpmu->iomem_qemu));\
    FDBG("  ->I/O memory cycles     : %s\n", commaprint((uint64_t)vpmu_io_memory_access_cycle_count()));\
    FDBG("Total Cycle count         : %s\n", commaprint(vpmu_cycle_count()));\
\
    FDBG("Network:\n");\
    FDBG("  ->recv count            : %s \n", commaprint(vpmu_estimated_netrecv_count()));\
    FDBG("  ->send count            : %s \n", commaprint(vpmu_estimated_netsend_count()));\
    FDBG("  ->recv time             : %s nsec\n", commaprint(vpmu_estimated_netrecv_execution_time_ns()));\
    FDBG("  ->send time             : %s nsec\n", commaprint(vpmu_estimated_netsend_execution_time_ns()));\
\
    FDBG("Timing Information:\n");\
    FDBG("  ->Pipeline              : %s nsec\n", commaprint(vpmu_estimated_pipeline_execution_time_ns()));\
    FDBG("  ->System memory         : %s nsec\n", commaprint(vpmu_estimated_sys_memory_access_time_ns()));\
    FDBG("  ->I/O memory            : %s nsec\n", commaprint(vpmu_estimated_io_memory_access_time_ns()));\
    FDBG("  ->Network               : %s nsec\n", commaprint(vpmu_estimated_network_execution_time_ns()));\
    FDBG("Estimated execution time  : %s nsec\n", commaprint(vpmu_estimated_execution_time_ns()));\
\
}while(0)

//tianman
const char const *arm_vfp_instr_names[] = { /* string names for arm_vfp_instruction enum values */
	"fabsd",
	"fabss",
	"faddd",
	"fadds",
	"fcmpd",
	"fcmps",
	"fcmped",
	"fcmpes",
	"fcmpezd",
	"fcmpezs",
	"fcmpzd",
	"fcmpzs",
	"fcpyd",
	"fcpys",
	"fcvtds",
	"fcvtsd",
	"fdivd",
	"fdivs",
	"fldd",
	"flds",
	"fldmd",
	"fldms",
	"fldmx",
	"fmacd",
	"fmacs",
	"fmdhr",
	"fmdlr",
	"fmdrr",
	"fmrdh",
	"fmrdl",
	"fmrrd",
	"fmrrs",
	"fmrs",
	"fmrx",
	"fmscd",
	"fmscs",
	"fmsr",
	"fmsrr",
	"fmstat",
	"fmuld",
	"fmuls",
	"fmxr",
	"fnegd",
	"fnegs",
	"fnmacd",
	"fnmacs",
	"fnmscd",
	"fnmscs",
	"fnmuld",
	"fnmuls",
	"fsitod",
	"fsitos",
	"fsqrtd",
	"fsqrts",
	"fstd",
	"fsts",
	"fstmd",
	"fstms",
	"fstmx",
	"fsubd",
	"fsubs",
	"ftosid",
	"ftosis",
	"ftouid",
	"ftouis",
	"ftouizd",
	"ftouizs",
	"ftousizd",
	"ftousizs",
	"fuitod",
	"fuitos",
	"unknown",
	"not_instrumented",
	"total_count"
};

//tianman
const char const *arm_instr_names[] = {
	"b",
	"bl",
	"blx",
	"bx",
	"bxj",
	"adc",
	"add",
	"and",
	"bic",
	"cmn",
	"cmp",
	"eor",
	"mov",
	"mvn",
	"orr",
	"rsb",
	"rsc",
	"sbc",
	"sub",
	"teq",
	"tst",
	"mul",
	"muls",
	"mla",
	"mlas",
	"smla<x><y>",
	"smlal",
	"smlals",
	"smlal<x><y>",
	"smlaw<y>",
	"smuad",
	"smusd",
	"smlad",
	"smlsd",
	"smlald",
	"smlsld",
	"smmla",
	"smmls",
	"smmul",
	"smul<x><y>",
	"smull",
	"smulls",
	"smulw<y>",
	"umaal",
	"umlal",
	"umlals",
	"umull",
	"umulls",
	"qadd",
	"qdadd",
	"qadd16",
	"qaddsubx",
	"qsubaddx",
	"qsub16",
	"qadd8",
	"qsub8",
	"qsub",
	"qdsub",
	"sadd16",
	"saddsubx",
	"ssubaddx",
	"ssub16",
	"sadd8",
	"ssub8",
	"shadd16",
	"shaddsubx",
	"shsubaddx",
	"shsub16",
	"shadd8",
	"shsub8",
	"uadd16",
	"uaddsubx",
	"usubaddx",
	"usub16",
	"uadd8",
	"usub8",
	"uhadd16",
	"uhaddsubx",
	"uhsubaddx",
	"uhsub16",
	"uhadd8",
	"uhsub8",
	"uqadd16",
	"uqaddsubx",
	"uqsubaddx",
	"uqsub16",
	"uqadd8",
	"uqsub8",
	"sxtab16",
	"sxtab",
	"sxtah",
	"sxtb16",
	"sxtb",
	"sxth",
	"uxtab16",
	"uxtab",
	"uxtah",
	"uxtb16",
	"uxtb",
	"uxth",
	"clz",
	"usad8",
	"usada8",
	"pkh",
	"pkhbt",
	"pkhtb",
	"rev",
	"rev16",
	"revsh",
	"sel",
	"ssat",
	"ssat16",
	"usat",
	"usat16",
	"mrs",
	"msr",
	"cps",
	"setend",
	"ldr",
	"ldrb",
	"ldrbt",
	"ldrd",
	"ldrex",
	"ldrh",
	"ldrsb",
	"ldrsh",
	"ldrt",
	"str",
	"strb",
	"strbt",
	"strd",
	"strex",
	"strh",
	"strt",
	"ldm1", //see arm manual ARM DDI 0100I page A3-27
	"ldm2",
	"ldm3",
	"stm1",
	"stm2",
	"swp",
	"swpb",
	"bkpt",
	"swi",
	"cdp",
	"ldc",
	"mcr",
	"mcrr",
	"mrc",
	"mrrc",
	"stc",
	"pld",
	"rfe",
	"srs",
	"mcrr2",
	"mrrc2",
	"stc2",
	"ldc2",
	"cdp2",
	"mcr2",
	"mrc2",
	"coprocessor",
	"neon_dp",
	"neon_ls",
	"clrex",
	"dsb",
	"dmb",
	"isb",
	"movw",
	"movt",
	"unknown",
	"not_instrumented",
	"total_instructions",
	"arm_vfp_coprocessor"
};

//evo0209
uint32_t arm_instr_time[ARM_INSTRUCTIONS];

//tianman
const int const arm_vfp_instr_time[] = {
	1,//"fabsd"
	1,//"fabss"
	1,//"faddd"
	1,//"fadds"
	1,//"fcmpd"
	1,//"fcmps"
	1,//"fcmped"
	1,//"fcmpes"
	1,//"fcmpezd"
	1,//"fcmpezs"
	1,//"fcmpzd"
	1,//"fcmpzs"
	1,//"fcpyd"
	1,//"fcpys"
	1,//"fcvtds"
	1,//"fcvtsd"
	28,//"fdivd"
	14,//"fdivs"
	2,//"fldd"
	1,//"flds"
	2,//"fldmd"mod*
	1,//"fldms"mod*
	2,//"fldmx"mod*
	2,//"fmacd"
	1,//"fmacs"
	1,//"fmdhr"
	1,//"fmdlr"
	1,//"fmdrr"mod, armv6
	1,//"fmrdh"
	1,//"fmrdl"
	1,//"fmrrd"mod,armv6
	1,//"fmrrs"mod,armv6
	1,//"fmrs"
	1,//"fmrx"
	2,//"fmscd"
	1,//"fmscs"
	1,//"fmsr"
	1,//"fmsrr"mod,armv6
	1,//"fmstat"
	2,//"fmuld"
	1,//"fmuls"
	1,//"fmxr"
	1,//"fnegd"
	1,//"fnegs"
	2,//"fnmacd"
	1,//"fnmacs"
	2,//"fnmscd"
	1,//"fnmscs"
	2,//"fnmuld"
	1,//"fnmuls"
	1,//"fsitod"
	1,//"fsitos"
	28,//"fsqrtd"
	14,//"fsqrts"
	2,//"fstd"
	1,//"fsts"
	2,//"fstmd",mod*
	1,//"fstms",mod*
	2,//"fstmx",mod*
	1,//"fsubd",
	1,//"fsubs"
	1,//"ftosid"
	1,//"ftosis"
	1,//"ftouid"
	1,//"ftouis"
	1,//"ftouizd",
	1,//"ftouizs",
	1,//"ftousizd",
	1,//"ftousizs",
	1,//"fuitod"
	1,//"fuitos"
	0,//"unknown"
	0,//"not_instrumented"
	0//"total_count"

};

//tianman
const int const arm_vfp_latency_time[] = {
	4,//"fabsd"
	4,//"fabss"
	4,//"faddd"
	4,//"fadds"
	4,//"fcmpd"
	4,//"fcmps"
	4,//"fcmped"
	4,//"fcmpes"
	4,//"fcmpezd"
	4,//"fcmpezs"
	4,//"fcmpzd"
	4,//"fcmpzs"
	4,//"fcpyd"
	4,//"fcpys"
	4,//"fcvtds"
	4,//"fcvtsd"
	31,//"fdivd"
	17,//"fdivs"
	5,//"fldd"
	4,//"flds"
	5,//"fldmd"mod*
	4,//"fldms"mod*
	5,//"fldmx"mod*
	5,//"fmacd"
	4,//"fmacs"
	2,//"fmdhr"
	2,//"fmdlr"
	1,//"fmdrr"mod,un
	1,//"fmrdh"
	1,//"fmrdl"
	1,//"fmrrd"mod,un
	1,//"fmrrs"mod,un
	1,//"fmrs"
	1,//"fmrx"
	5,//"fmscd"
	4,//"fmscs"
	2,//"fmsr"
	1,//"fmsrr"mod,un
	2,//"fmstat"
	5,//"fmuld"
	4,//"fmuls"
	2,//"fmxr"
	4,//"fnegd"
	4,//"fnegs"
	5,//"fnmacd"
	4,//"fnmacs"
	5,//"fnmscd"
	4,//"fnmscs"
	5,//"fnmuld"
	4,//"fnmuls"
	4,//"fsitod"
	4,//"fsitos"
	31,//"fsqrtd"
	17,//"fsqrts"
	4,//"fstd"
	3,//"fsts"
	4,//"fstmd",mod*
	3,//"fstms",mod*
	4,//"fstmx",mod*
	4,//"fsubd",
	4,//"fsubs"
	4,//"ftosid"
	4,//"ftosis"
	4,//"ftouid"
	4,//"ftouis"
	4,//"ftouizd",
	4,//"ftouizs",
	4,//"ftousizd",
	4,//"ftousizs",
	4,//"fuitod"
	4,//"fuitos"
	0,//"unknown"
	0,//"not_instrumented"
	0//"total_count"

};

typedef struct VPMUState
{
    SysBusDevice     busdev;
    qemu_irq         irq;
    VPMU             *VPMU;
    QLIST_ENTRY(VPMUState) entries;
}VPMUState;
typedef struct VPMUState_t 
{
    CharDriverState *vpmu_monitor;
    QLIST_HEAD(VPMUStateList, VPMUState) head;
}VPMUState_t;
    
//evo0209 : for emulation time
static struct timeval t1, t2;

/* File struct for dump VPMU information */
FILE *fp, *power_fp;

/******************************
 * Global variables.
 *
 */

/* QEMU realted device information, such as BUS, IRQ, VPMU strucutre. */
VPMUState_t vpmu_t;
/* Indicator of status of VPMU. */
#ifdef CONFIG_VPMU_THREAD
volatile short VPMU_enabled = 0;
#else
short VPMU_enabled = 0;
#endif
/* Global variables for VPMU counters. */
struct VPMU GlobalVPMU;

/* Added by Chiaheng. 2010' 9' 24.
 * VPMU for Oprofile setting.
 * It's used accompanied with the mechanism described in op_model_itripac.c.
 * In oprofile, it used "reset_counter" to keep track of overflow value, which
 * is set by the user. When the performance counter reaches the overflow value,
 * the counter will generate the interrupt and set up overflow flag.
 * struct pmu_counter {
 *     volatile unsigned long ovf;
 *     unsigned long reset_counter;
 * };
 * It works when the CONFIG_VPMU_OPROFILE flag is set to true.
 */
struct cp14_pmu_state* cp14_pmu_state_pt;

spinlock_t vpmu_ovrflw_chk_lock;

static struct android_dev_stat adev_stat = {
    .cpu_util       = 0,
    .lcd_brightness = NULL,
    .audio_buf      = NULL,
    .audio_on       = 0, // 0:off others:on
    .net_mode       = 1, // 0:off 1:Wifi 2:3G
    .net_on         = 0, // 0:off 1:on
    .net_packets    = NULL,
    .gps_status     = NULL, // 0:off 1:idle 2:active
    .battery_sim    = 1, // 0:off 1:on
    .energy_left    = 0, // mJ
    .bat_ac_online  = NULL, // 0:offline 1:online
    .bat_capacity   = NULL,
};

//chiaheng
#include "vpmu_sampling.c"

#ifdef CONFIG_PACDSP
static struct pacduo_power_regs pacduo_pregs = {
    .PLL1_CR    = 0x10000011, //system clk is (PLL1_CR[31:24]+1)*24 MHz
    .PLL1_SR    = 0x1,
    .CG_DDRTPR  = 0x2,
    .CG_DSP1TPR = 0x2,
    .CG_DSP2TPR = 0x2,
    .CG_AXITPR  = 0x2,
    .VC1_TRCR   = 0xf,
    .VC1_SR     = 0x1cc,
    .VC1_WDR    = 0x308,
    .VC1_CR     = 0x0,
    .VC2_TRCR   = 0xf,
    .VC2_SR     = 0x1cc,
    .VC2_WDR    = 0x308,
    .VC2_CR     = 0x0,
    .PMU_LCD_CR = 0x1,
};

static const int freq_divider[] = {
    0, 1, 2, 3, 4, 6, 8, 12,
};

double pacduo_dsp_volt(int i)
{
    if(i == 0)
        return (pacduo_pregs.VC1_WDR-0x300)*0.025+0.8;
    else if (i == 1)
        return (pacduo_pregs.VC2_WDR-0x300)*0.025+0.8;
    else
        return 1.0;
}

int pacduo_dsp_freq(int i)
{
    if(i == 0) {
        if(pacduo_pregs.CG_DSP1TPR == 0) {
            return PACDUO_BASE_CLK;
        }
        else {
            return (PACDUO_BASE_CLK*((pacduo_pregs.PLL1_CR>>24)+1))/
                    freq_divider[pacduo_pregs.CG_DSP1TPR];
        }
    }
    else if (i == 1) {
        if(pacduo_pregs.CG_DSP2TPR == 0) {
            return PACDUO_BASE_CLK;
        }
        else {
            return (PACDUO_BASE_CLK*((pacduo_pregs.PLL1_CR>>24)+1))/
                    freq_divider[pacduo_pregs.CG_DSP2TPR];
        }
    }
    else { //default freq divider is 2
        return (PACDUO_BASE_CLK*((pacduo_pregs.PLL1_CR>>24)+1))/2;
    }
}

double pacduo_dsp_power(int i)
{
    double dsp_volt = pacduo_dsp_volt(i);
    int dsp_freq = pacduo_dsp_freq(i);

    return dsp_volt*(61.208123*dsp_volt+0.071121*dsp_freq-24.497915);
}

double pacduo_mpu_power(void)
{
    return ((pacduo_pregs.PLL1_CR>>24)-0x0b)*4.21806+156.97284; //linear regression
}
#endif

/* Performace Monitoring Registers */
#define CPPMNC    0  /* Performance Monitor Control register */
#define CPCCNT    1  /* Clock Counter register */
#define CPINTEN   4  /* Interrupt Enable register */
#define CPFLAG    5  /* Overflow Flag register */
#define CPEVTSEL  8  /* Event Selection register */

#define CPPMN0    0  /* Performance Count register 0 */
#define CPPMN1    1  /* Performance Count register 1 */
#define CPPMN2    2  /* Performance Count register 2 */
#define CPPMN3    3  /* Performance Count register 3 */
                                           //      opc_2    CRn

#define COND_SIG 99

void branch_set(unsigned int cur_sig)
{
    if(cur_sig == 33)
        fprintf(stderr,"sync error\n");
    static int last_signal = 0;
    if( last_signal == COND_SIG ) {
        if( cur_sig == COND_SIG ) {
            branch_update( 0 );//not taken
        }
        else {
            branch_update( 1 );//taken
            last_signal = 0;
        }
    }
    else {
        last_signal = cur_sig;
    }
}

void __test(int mode,int tmp)
{
    /*
    if(tmp == 1)
        printf("TH->");
    if(tmp == 2)
        printf("BH->");
    if(tmp == 1)
        fprintf(stderr,"\nTH->");
    if(tmp == 2)
        fprintf(stderr,"\nBH->");
    if(mode == ARM_CPU_MODE_USR)
        fprintf(stderr,"USR ");
    if(mode == ARM_CPU_MODE_SVC)
        fprintf(stderr,"SVC ");
    if(mode == ARM_CPU_MODE_IRQ)
        fprintf(stderr,"IRQ ");
    if(mode == ARM_CPU_MODE_SYS)
        fprintf(stderr,"SYS ");
    if(mode == ARM_CPU_MODE_FIQ)
        fprintf(stderr,"FIQ ");
    if(mode == ARM_CPU_MODE_ABT)
        fprintf(stderr,"ABT ");
    */

}
void branch_update(unsigned int taken)
{
    static char predictor = 0;
    static unsigned int predict_correct = 0, predict_wrong = 0;

    if(__builtin_expect((taken == BRANCH_PREDICT_SYNC),0))
    {
        GlobalVPMU.branch_predict_correct = predict_correct;
        GlobalVPMU.branch_predict_wrong = predict_wrong;
        return;
    }

    switch( predictor )
    {
        /* predict not taken */
        case 0:
            if( taken ) {
                predictor = 1;
                predict_wrong ++;
            } else
                predict_correct ++;
            break;
        case 1:
            if( taken ) {
                predictor = 3;
                predict_wrong ++;
            } else {
                predictor = 0;
                predict_correct ++;
            }
            break;
        /* predict taken */
        case 2:
            if( taken ) {
                predictor = 3;
                predict_correct ++;
            } else {
                predictor = 0;
                predict_wrong ++;
            }
            break;
        case 3:
            if( taken )
                predict_correct ++;
            else {
                predictor = 2;
                predict_wrong ++;
            }
            break;
        default:
                fprintf(stderr,"predictor error\n");
                exit(0);
    }
#if 0
    if(taken)
        fprintf(stderr,"taken\n");
    else
        fprintf(stderr,"not taken\n");
    fprintf(stderr,"predictor state=%d right=%u wrong=%u\n", predictor, predict_correct, predict_wrong);
#endif
}

/* this function cannot be used multiple times
 * the buffer in this function will be overwritten
 */
char *commaprint(unsigned long long n)
{
    static int comma = '\0';
    static char retbuf[30];
    char *p = &retbuf[sizeof(retbuf)-1];
    int i = 0;

    if(comma == '\0') {
        struct lconv *lcp = localeconv();
        if(lcp != NULL) {
            if(lcp->thousands_sep != NULL &&
                *lcp->thousands_sep != '\0')
                comma = *lcp->thousands_sep;
            else    comma = ',';
        }
    }

    *p = '\0';

    do {
        if(i%3 == 0 && i != 0)
            *--p = comma;
        *--p = '0' + n % 10;
        n /= 10;
        i++;
    } while(n != 0);

    return p;
}

/* the following array is used to deal with def-use register interlocks, which we
* can compute statically (ignoring conditions), very fortunately.
*
* the idea is that interlock_base contains the number of cycles "executed" from
* the start of a basic block.基本上不用管interlock_base，因為沒用到 It is set to 0 in trace_bb_start, and incremented
* in each call to get_insn_ticks_arm.
*
* interlocks[N] correspond to the value of interlock_base after which a register N
* can be used by another operation, it is set each time an instruction writes to
* the register in get_insn_ticks()
*/
 
static int interlocks[16];
static int interlock_base;
 
static void
_interlock_def(int reg, int delay)//interlock加delay
{
    if (reg >= 0)
        interlocks[reg] = interlock_base + delay;
}
 
static int 
_interlock_use(int reg)//lock和interlock_base的差異
{
    int delay = 0;
 
    if (reg >= 0)
    {   
        delay = interlocks[reg] - interlock_base;
        if (delay < 0)
            delay = 0;
    }   
    return delay;
}

// Compute the number of cycles that this instruction will take,
// not including any I-cache or D-cache misses. This function
// is called for each instruction in a basic block when that
// block is being translated.
int get_insn_ticks(uint32_t insn)
{
	int result = 1; /* by default, use 1 cycle */

	/* See Chapter 12 of the ARM920T Reference Manual for details about clock cycles */

	/* first check for invalid condition codes */
	if ((insn >> 28) == 0xf)
	{ // ARM DDI 0100I  ARM Architecture Reference Manual A3-41  temp_note by ppb
		if ((insn >> 25) == 0x7d) { /* BLX */
			//result = 3;
			//evo0209
			result = arm_instr_time[ARM_INSTRUCTION_BLX];
			goto Exit;
		}
		// added by ppb
		else if ((insn & 0x0d70f000) == 0x0550f000) { /* PLD */
			//result = 1;
			//evo0209
			result = arm_instr_time[ARM_INSTRUCTION_PLD];
			goto Exit;
		}
		// end added by ppb
		/* XXX: if we get there, we're either in an UNDEFINED instruction */
		/* or in co-processor related ones. For now, only return 1 cycle */
		goto Exit;
	}

	/* other cases */
	switch ((insn >> 25) & 7)
	{
		case 0:  // 25-27 bit = 0  temp_note by ppb
			if ((insn & 0x00000090) == 0x00000090) /* Multiplies, extra load/store, Table 3-2 */
				// extract out all (5-8 bit)= 1001  temp_note by  ppb
			{
				/* XXX: TODO: Add support for multiplier operand content penalties in the translator */

				// haven't done : ARM DDI 0222B p.8-20 SMULxy, SMLAxy, SMULWy, SMLAWy  by ppb
				if ((insn & 0x0fc000f0) == 0x00000090) /* 3-2: Multiply (accumulate) */
				{// ARM DDI 0100I  Figure A3-3 in A3-35   temp_note by ppb

					int Rm = (insn & 15);
					int Rs = (insn >> 8) & 15;
					int Rn = (insn >> 12) & 15;
					int Rd = (insn >> 16) & 15; // added by ppb

					if ((insn & 0x00200000) != 0) { /* MLA */
						//result += _interlock_use(Rn); // comment out by ppb
						if (_interlock_use(Rn) > 1) {   // added by ppb
							result += _interlock_use(Rn) - 1;
						}
					} else { /* MLU */
						if (Rn != 0) /* UNDEFINED */
							goto Exit;
					}
					/* cycles=2+m, assume m=1, this should be adjusted at interpretation time */
					//result += 2 + _interlock_use(Rm) + _interlock_use(Rs); // commented out by ppb
					// add the following if-else  by ppb
					if ((insn & 0x00100000) != 0) { /* MULS or MLAS  need 4 cycles */
						//result += 3 + _interlock_use(Rm) + _interlock_use(Rs);
						//evo0209
						result += arm_instr_time[ARM_INSTRUCTION_MULS] - 1 + _interlock_use(Rm) + _interlock_use(Rs);
					} else { // need 2 cycles
						//result += 1 + _interlock_use(Rm) + _interlock_use(Rs);
						//evo0209
						result += arm_instr_time[ARM_INSTRUCTION_MUL] - 1 + _interlock_use(Rm) + _interlock_use(Rs);
					}

					_interlock_def(Rd, result + 1); // added by ppb
				}
				else if ((insn & 0x0f8000f0) == 0x00800090) /* 3-2: Multiply (accumulate) long */
				{
					int Rm = (insn & 15);
					int Rs = (insn >> 8) & 15;
					int RdLo = (insn >> 12) & 15;
					int RdHi = (insn >> 16) & 15;

					if ((insn & 0x00200000) != 0) { /* SMLAL & UMLAL */  // (accumulate)
						//result += _interlock_use(RdLo) + _interlock_use(RdHi); //comment out by ppb
						if (_interlock_use(RdLo) > 1) { // added by ppb
							result += _interlock_use(RdLo) - 1;
						}
						if (_interlock_use(RdHi) > 1) { // added by ppb
							result += _interlock_use(RdHi) - 1;
						}
					}
					/* else SMLL and UMLL */

					/* cucles=3+m, assume m=1, this should be adjusted at interpretation time */
					//result += 3 + _interlock_use(Rm) + _interlock_use(Rs); commented out by ppb
					if ((insn & 0x00100000) != 0) { /* SMULLS, UMULLS, SMLALS, UMLALS, need 5 cycles */
						//result += 4 + _interlock_use(Rm) + _interlock_use(Rs);
						//evo0209
						result += arm_instr_time[ARM_INSTRUCTION_SMULLS] - 1 + _interlock_use(Rm) + _interlock_use(Rs);
					} else { // need 3 cycles
						//result += 2 + _interlock_use(Rm) + _interlock_use(Rs);
						//evo0209
						result += arm_instr_time[ARM_INSTRUCTION_SMULL] - 1 + _interlock_use(Rm) + _interlock_use(Rs);
					}

					_interlock_def(RdLo, result + 1); // added by ppb
					_interlock_def(RdHi, result + 1); // added by ppb
				}
				//else if ((insn & 0x0fd00ff0) == 0x01000090) /* 3-2: Swap/swap byte */ //by ppb
				else if ((insn & 0x0fb00ff0) == 0x01000090) /* 3-2: Swap/swap byte */
				{// ARM DDI 0100I  Figure A3-5 in A3-39   temp_note by ppb
					int Rm = (insn & 15);
					int Rd = (insn >> 8) & 15;

					result = 2 + _interlock_use(Rm);//2cycle是基本盤，如果load use的話需要3cycle
					//用_interlock_def來預防有人要接下來使用Rd(因為其他指令要用到Rd一定會call _interlock_use(Rd)
					//來測試是否和上個指令衝突而需多算的cycle)
					//_interlock_def(Rd, result+1);//rd這個lock=base(直到上個指令翻譯完總共花多少cycle)+(result+1)
					//result代表這個insn花了多少cycle
					// added by ppb
					if ((insn & 0x00400000) != 0) {	//SWPB
						//_interlock_def(Rd, result + 2);
						//evo0209
						_interlock_def(Rd, result + arm_instr_time[ARM_INSTRUCTION_SWPB] - 1);
					}
					else {				//SWP
						//_interlock_def(Rd, result + 1);
						//evo0209
						_interlock_def(Rd, result + arm_instr_time[ARM_INSTRUCTION_SWP] - 1);
					}
					// end added by ppb
				}
				//else if ((insn & 0x0e400ff0) == 0x00000090) /* 3-2: load/store halfword, reg offset */ //by ppb
				else if ((insn & 0x0e400ff0) == 0x000000b0) /* 3-2: load/store halfword, reg offset */
				{
					int Rm = (insn & 15);
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;

					result += _interlock_use(Rn) + _interlock_use(Rm);
					if ((insn & 0x00100000) != 0) /* it's a load, there's a 2-cycle interlock */
						//_interlock_def(Rd, result + 2);
						//evo0209
						_interlock_def(Rd, result + arm_instr_time[ARM_INSTRUCTION_LDRH] - 1);
				}
				//else if ((insn & 0x0e400ff0) == 0x00400090) /* 3-2: load/store halfword, imm offset */ //by ppb
				else if ((insn & 0x0e4000f0) == 0x004000b0) /* 3-2: load/store halfword, imm offset */
				{
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;

					result += _interlock_use(Rn);
					if ((insn & 0x00100000) != 0) /* it's a load, there's a 2-cycle interlock */
						//_interlock_def(Rd, result + 2);
						//evo0209
						_interlock_def(Rd, result + arm_instr_time[ARM_INSTRUCTION_LDRH] - 1);
				}
				else if ((insn & 0x0e500fd0) == 0x000000d0) /* 3-2: load/store two words, reg offset */
				{
					/* XXX: TODO: Enhanced DSP instructions */
					// added by ppb
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;
					result += 1 + _interlock_use(Rn); // 2 cycles
					if ((insn & 0x00000020) == 0)
						if (Rd != 15)
							//_interlock_def(Rd + 1, result + 1);
							//evo0209
							_interlock_def(Rd + 1, result + arm_instr_time[ARM_INSTRUCTION_LDR] - 1);
					// end added by ppb
				}
				else if ((insn & 0x0e500fd0) == 0x001000d0) /* 3-2: load/store half/byte, reg offset */
				{						// load signed half/byte reg offset
					int Rm = (insn & 15);
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;

					result += _interlock_use(Rn) + _interlock_use(Rm);
					if ((insn & 0x00100000) != 0) /* load, 2-cycle interlock */
						//_interlock_def(Rd, result + 2);
						//evo0209
						_interlock_def(Rd, result + arm_instr_time[ARM_INSTRUCTION_LDRSH] - 1);
				}
				else if ((insn & 0x0e5000d0) == 0x004000d0) /* 3-2: load/store two words, imm offset */
				{
					/* XXX: TODO: Enhanced DSP instructions */
					// added by ppb
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;
					result += 1 + _interlock_use(Rn); // 2 cycles
					if ((insn & 0x00000020) == 0)
						if (Rd != 15)
							//_interlock_def(Rd + 1, result+1);
							//evo0209
							_interlock_def(Rd + 1, arm_instr_time[ARM_INSTRUCTION_LDR] - 1);
					// end added by ppb
				}
				else if ((insn & 0x0e5000d0) == 0x005000d0) /* 3-2: load/store half/byte, imm offset */
				{						// load signed half/byte imm offset
					int Rd = (insn >> 12) & 15;
					int Rn = (insn >> 16) & 15;

					result += _interlock_use(Rn);
					if ((insn & 0x00100000) != 0) /* load, 2-cycle interlock */
						//_interlock_def(Rd, result+2);
						//evo0209
						_interlock_def(Rd, arm_instr_time[ARM_INSTRUCTION_LDRSH] - 1);
				}
				else
				{
					/* UNDEFINED */
				}
			}
			else if ((insn & 0x0f900000) == 0x01000000) /* Misc. instructions, table 3-3 */
				// BX or Halfword data transfer  temp_note by ppb
			{	// ARM DDI 0100I  Figure A3-4 in A3-37   temp_note by ppb
				switch ((insn >> 4) & 15)
				{
					case 0:
						if ((insn & 0x0fb0fff0) == 0x0120f000) /* move register to status register MSR*/
						{
							int Rm = (insn & 15);
							//result += _interlock_use(Rm); // codes bolow added by ppb
							// ppb
							if (((insn >> 16) & 0x0007) > 0)
								result += 2 + _interlock_use(Rm);
							else
								result += _interlock_use(Rm); // if only flags are updated(mask_f) by ppb
							// end ppb
						}
						else if ((insn & 0xfbf0fff) == 0x010f0000) { //MRS  added by ppb
							result += 1; // 2 cycles
						}
						break;

					case 1:
						//if ( ((insn & 0x0ffffff0) == 0x01200010) || /* branch/exchange */ // comment out by ppb
						//     ((insn & 0x0fff0ff0) == 0x01600010) ) /* count leading zeroes */
						if ((insn & 0x0ffffff0) == 0x012fff10)  /* branch/exchange */  // by ppb
						{
							int Rm = (insn & 15); // in ARM9EJ-S TRM Chap.8, BX = 3 cycles  by ppb
							//result += 2 + _interlock_use(Rm);
							//evo0209
							result += arm_instr_time[ARM_INSTRUCTION_BX] - 1 + _interlock_use(Rm);
						}
						else if ((insn & 0x0fff0ff0) == 0x016f0f10) /* count leading zeroes CLZ  by ppb */
						{
							int Rm = (insn & 15);
							//result += _interlock_use(Rm);
							//evo0209
							result += arm_instr_time[ARM_INSTRUCTION_CLZ] - 1 + _interlock_use(Rm);
						}
						break;

					case 3:
						//if ((insn & 0x0ffffff0) == 0x01200030) /* link/exchange */ // comment out by ppb
						if ((insn & 0x0ffffff0) == 0x012fff30) /* link/exchange */
						{
							int Rm = (insn & 15);
							//result += _interlock_use(Rm);
							//result += 2 + _interlock_use(Rm); // BLX = 3 cycles by ppb
							//evo0209
							result += arm_instr_time[ARM_INSTRUCTION_BLX] - 1 + _interlock_use(Rm);
						}
						break;

						// other case: software breakpoint   temp_note by ppb
					default:
						/* TODO: Enhanced DSP instructions */
						;
				}
			}
			else /* Data processing */
			{
				int Rm = (insn & 15);
				int Rn = (insn >> 16) & 15;

				result += _interlock_use(Rn) + _interlock_use(Rm);
				if ((insn & 0x10)) { /* register-controlled shift => 1 cycle penalty */
					int Rs = (insn >> 8) & 15;
					result += 1 + _interlock_use(Rs);
				}
				// added by ppb
				int Rd = (insn >> 12) & 15;
				if (Rd == 15) {
					result += 2;
				}
				// end added by ppb
			}
			break;

		case 1:
			//if ((insn & 0x01900000) == 0x01900000)
			if ((insn & 0x03b0f000) == 0x0320f000)
			{
				/* either UNDEFINED or move immediate to CPSR */
			}
			else /* Data processing immediate */
			{
				int Rn = (insn >> 12) & 15;
				result += _interlock_use(Rn);
			}
			break;

		case 2: /* load/store immediate */
			{
				int Rn = (insn >> 16) & 15;

				result += _interlock_use(Rn);
				if (insn & 0x00100000) { /* LDR */
					int Rd = (insn >> 12) & 15;

					if (Rd == 15) /* loading PC */
						//result = 5; // comment out by ppb
						result += 4;
					else
						_interlock_def(Rd,result+1);
				}
			}
			break;

		case 3:
			if ((insn & 0x10) == 0) /* load/store register offset */
			{
				int Rm = (insn & 15);
				int Rn = (insn >> 16) & 15;

				result += _interlock_use(Rm) + _interlock_use(Rn);

				if (insn & 0x00100000) { /* LDR */
					int Rd = (insn >> 12) & 15;
					if (Rd == 15) {
						if ((insn & 0xff0) == 0)// added by ppb
							result = 5;
						else // scaled offset
							result = 6; // added by ppb
					}
					else
						_interlock_def(Rd,result+1);
				}
				else { // store  added by ppb
					if ((insn & 0xff0) == 0)// added by ppb
						result = 1;
					else // scaled offset
						result = 2; // added by ppb
				}
			}
			/* else Media inst.(ARMv6) or UNDEFINED */
			break;

		case 4: /* load/store multiple */
			{
				int Rn = (insn >> 16) & 15; // base regiester  temp_note by ppb
				uint32_t mask = (insn & 0xffff);
				int count;

				for (count = 0; count < 15; count++) { // added by ppb
					if (mask & 1) {
						if (_interlock_use(count) == 1)
							result += 2; //second-cycle interlock by ppb
						break;
					}
					mask = mask >> 1;
				}

				mask = (insn & 0xffff);
				for (count = 0; mask; count++)
					mask &= (mask-1);

				result += _interlock_use(Rn);

				if (insn & 0x00100000) /* LDM */
				{
					int nn;

					if (insn & 0x8000) { /* loading PC */
						result = count+4;
					} else { /* not loading PC */
						//result = (count < 2) ? 2 : count; //comment out by ppb
						result += ( (count < 2) ? 2 : count ) - 1;
					}
					/* create defs, all registers locked until the end of the load */
					for (nn = 0; nn < 15; nn++)
						if ((insn & (1U << nn)) != 0)
							_interlock_def(nn,result);
				}
				else /* STM */
					//result = (count < 2) ? 2 : count; //comment out by ppb
					result += ( (count < 2) ? 2 : count ) - 1;
			}
			break;

		case 5: /* branch and branch+link */
			result = 3; //added by ppb
			break;

		case 6: /* coprocessor load/store */
			{
				int Rn = (insn >> 16) & 15;

				if (insn & 0x00100000)
					result += _interlock_use(Rn);

				/* XXX: other things to do ? */
			}
			break;

		default: /* i.e. 7 */
			/* XXX: TODO: co-processor related things */
			;
	}
Exit:
	interlock_base += result;
	return result;
}

void get_insn_parser(unsigned int insn, struct ExtraTBInfo* s)
{

    /* See Chapter 12 of the ARM920T Reference Manual for details about clock cycles */

    /* first check for invalid condition codes */
    if (( insn >> 28) == 0xf)/* Unconditional instructions.  */
    {
        if ((insn >> 25) == 0x7d) {  /* BLX */
            s->all_changes_to_PC_count += 1 ;
            goto Exit;
        }
    }
	/*bx b bl*/
    switch ((insn >> 25) & 7)
    {
        case 0:
			/* bx  */
			if ((insn & 0x0f900000) == 0x01000000)
			{         switch ((insn >> 4) & 15)
						{
							case 1:
								if ((insn & 0x0ffffff0) == 0x01200010)  /* branch/exchange */ /* bx */
								{
									s->all_changes_to_PC_count += 1 ;
									goto Exit;
								}
						}
			}
        case 5:  /* branch and branch+link */
			s->all_changes_to_PC_count += 1 ;
			goto Exit;
	}
	/* if rd is PC(r15) we do the counting, otherwise we don't*/
	/*LDM LDR MOV*/
	if( __builtin_expect(((insn >> 12) & 0xf) == 15 || ((insn>>16)&0xf) == 15 , 0) )
	{  /* other cases */
    switch ((insn >> 25) & 7)
    {
        case 0: /* bit[27-25] */
            if ((insn & 0x00000090) == 0x00000090)  /* Multiplies, extra load/store, Table 3-2 */
            {
                /* XXX: TODO: Add support for multiplier operand content penalties in the translator */

                if ((insn & 0x0fc000f0) == 0x00000090)   /* 3-2: Multiply (accumulate) */
                {
                    int  Rn = (insn >> 12) & 15;

                    if ((insn & 0x00200000) != 0) {  /* MLA */
                        ;
                    } else {   /* MLU */
                        if (Rn != 0)      /* UNDEFINED */
                            goto Exit;
                    }
                    /* cycles=2+m, assume m=1, this should be adjusted at interpretation time */
                    ;
                }
                else if ((insn & 0x0f8000f0) == 0x00800090)  /* 3-2: Multiply (accumulate) long */
                {
                    if ((insn & 0x00200000) != 0) { /* SMLAL & UMLAL */
                        ;
                    }
                    /* else SMLL and UMLL */

                    /* cucles=3+m, assume m=1, this should be adjusted at interpretation time */
                    ;
                }
                else if ((insn & 0x0fd00ff0) == 0x01000090)  /* 3-2: Swap/swap byte */
                {

                    ;
                }
                else if ((insn & 0x0e400ff0) == 0x00000090)  /* 3-2: load/store halfword, reg offset */
                {

                    ;
                    if ((insn & 0x00100000) != 0)  /* it's a load, there's a 2-cycle interlock */
                        ;
                }
                else if ((insn & 0x0e400ff0) == 0x00400090)  /* 3-2: load/store halfword, imm offset */
                {

                    ;
                    if ((insn & 0x00100000) != 0)  /* it's a load, there's a 2-cycle interlock */
                        ;
                }
                else if ((insn & 0x0e500fd0) == 0x000000d0) /* 3-2: load/store two words, reg offset */
                {
                    /* XXX: TODO: Enhanced DSP instructions */
                }
                else if ((insn & 0x0e500fd0) == 0x001000d0) /* 3-2: load/store half/byte, reg offset */
                {

                    ;
                    if ((insn & 0x00100000) != 0)  /* load, 2-cycle interlock */
                       ;
                }
                else if ((insn & 0x0e5000d0) == 0x004000d0) /* 3-2: load/store two words, imm offset */
                {
                    /* XXX: TODO: Enhanced DSP instructions */
                }
                else if ((insn & 0x0e5000d0) == 0x005000d0) /* 3-2: load/store half/byte, imm offset */
                {

                    ;
                    if ((insn & 0x00100000) != 0)  /* load, 2-cycle interlock */
                        ;
                }
                else
                {
                    /* UNDEFINED */
                }
            }
            else if ((insn & 0x0f900000) == 0x01000000)  /* Misc. instructions, table 3-3 */
            {
                switch ((insn >> 4) & 15)
                {
                    case 0:
                        if ((insn & 0x0fb0fff0) == 0x0120f000) /* move register to status register */
                        {
                            ;
                        }
                        break;

                    case 1:
                        if ( ((insn & 0x0ffffff0) == 0x01200010) ||  /* branch/exchange */
                             ((insn & 0x0fff0ff0) == 0x01600010) )   /* count leading zeroes */
                        {
                            ;
                        }
                        break;

                    case 3:
                        if ((insn & 0x0ffffff0) == 0x01200030)   /* link/exchange */
                        {
                            ;
                        }
                        break;

                    default:
                        /* TODO: Enhanced DSP instructions */
                        ;
                }
            }
            else  /* Data processing */
            {
                int  Rd = (insn >> 12) & 15;

				if(Rd==15)
					s->all_changes_to_PC_count += 1;
                if ((insn & 0x10)) {   /* register-controlled shift => 1 cycle penalty */
                    ;
                }
            }
            break;

        case 1:
            if ((insn & 0x01900000) == 0x01900000)
            {
                /* either UNDEFINED or move immediate to CPSR */
            }
            else  /* Data processing immediate */
            {
                int  Rn = (insn >> 12) & 15;
				if(Rn==15)
                s->all_changes_to_PC_count += 1 ;
            }
            break;

        case 2:  /* load/store immediate */
            {

                ;
                if (insn & 0x00100000) {  /* LDR */
                    int  Rd = (insn >> 12) & 15;

                    if (Rd == 15)  /* loading PC */
                        s->all_changes_to_PC_count += 1 ;
                    else
                        ;
                }
            }
            break;

        case 3:
            if ((insn & 0x10) == 0)  /* load/store register offset */
            {
                int  Rn = (insn >> 16) & 15;

                ;

                if (insn & 0x00100000) {  /* LDR */
					if(Rn==15)
						s->all_changes_to_PC_count += 1 ;
                    else
                        ;
                }
            }
            /* else UNDEFINED */
            break;

        case 4:  /* load/store multiple */
            {
                unsigned int mask = (insn & 0xffff);
                int       count;

                for (count = 0; mask; count++)
                    mask &= (mask-1);

                ;

                if (insn & 0x00100000)  /* LDM */
                {
                    int  nn;

                    if (insn & 0x8000) {  /* loading PC */
                        s->all_changes_to_PC_count += 1 ;
                    } else {  /* not loading PC */
                        ;
                    }
                    /* create defs, all registers locked until the end of the load */
                    for (nn = 0; nn < 15; nn++)
                        if ((insn & (1U << nn)) != 0)
                            ;
                }
                else  /* STM */
                {
                }
			}
            break;

        case 5:  /* branch and branch+link */
            break;

        case 6:  /* coprocessor load/store */
            {
                if (insn & 0x00100000)
                    ;

                /* XXX: other things to do ? */
            }
            break;

        default: /* i.e. 7 */
            /* XXX: TODO: co-processor related things */
            ;
    }
	}
Exit:
 ;
}

static uint32_t pxa2xx_clkpwr_read(void *opaque, int op2, int reg, int crm)
{
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;

    switch (reg) {
    case 6: /* Clock Configuration register */
        return s->clkcfg;

    case 7: /* Power Mode register */
        return 0;

    default:
        printf("%s:Integrator Bad register 0x%x\n", __FUNCTION__, reg);
        break;
    }
    return 0;
}

static void pxa2xx_clkpwr_write(void *opaque, int op2, int reg, int crm,
                uint32_t value)
{
/* paslab : temporarily remove this part */
#if 0
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;
    static const char *pwrmode[8] = {
        "Normal", "Idle", "Deep-idle", "Standby",
        "Sleep", "reserved (!)", "reserved (!)", "Deep-sleep",
    };

    switch (reg) {
    case 6: /* Clock Configuration register */
        s->clkcfg = value & 0xf;
        if (value & 2)
            printf("%s: CPU frequency change attempt\n", __FUNCTION__);
        break;

    case 7: /* Power Mode register */
        if (value & 8)
            printf("%s: CPU voltage change attempt\n", __FUNCTION__);
        switch (value & 7) {
        case 0:
            /* Do nothing */
            break;

        case 1:
            /* Idle */
            if (!(s->cm_regs[CCCR >> 2] & (1 << 31))) { /* CPDIS */
                cpu_interrupt(s->env, CPU_INTERRUPT_HALT);
                break;
            }
            /* Fall through.  */

        case 2:
            /* Deep-Idle */
            cpu_interrupt(s->env, CPU_INTERRUPT_HALT);
            s->pm_regs[RCSR >> 2] |= 0x8;   /* Set GPR */
            goto message;

        case 3:
            s->env->uncached_cpsr =
                    ARM_CPU_MODE_SVC | CPSR_A | CPSR_F | CPSR_I;
            s->env->cp15.c1_sys = 0;
            s->env->cp15.c1_coproc = 0;
            s->env->cp15.c2_base0 = 0;
            s->env->cp15.c3 = 0;
            s->pm_regs[PSSR >> 2] |= 0x8;   /* Set STS */
            s->pm_regs[RCSR >> 2] |= 0x8;   /* Set GPR */
            /*                                                                                                                
             * The scratch-pad register is almost universally used
             * for storing the return address on suspend.  For the
             * lack of a resuming bootloader, perform a jump
             * directly to that address.
             */
            memset(s->env->regs, 0, 4 * 15);
            s->env->regs[15] = s->pm_regs[PSPR >> 2];

#if 0
            buffer = 0xe59ff000;    /* ldr     pc, [pc, #0] */
            cpu_physical_memory_write(0, &buffer, 4);
            buffer = s->pm_regs[PSPR >> 2];
            cpu_physical_memory_write(8, &buffer, 4);
#endif

            /* Suspend */
            cpu_interrupt(cpu_single_env, CPU_INTERRUPT_HALT);

            goto message;

        default:
        message:
            printf("%s: machine entered %s mode\n", __FUNCTION__,
                            pwrmode[value & 7]);
        }
        break;

    default:
        printf("%s: Bad register 0x%x\n", __FUNCTION__, reg);
        break;
    }
#endif
    SIMPLE_DBG("pxa2xx_clkpwr_read: not handled function.\n");
    return;
}

/* Read Performance Monitoring Registers. */
static uint32_t pxa2xx_perf_read(void *opaque, int op2, int reg, int crm)
{
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;
    uint32_t temp_value=0;
    int64_t tmp;

    /* vpmu points to GlobalVPMU */
    //VPMU *vpmu = s->vpmu_t;

    switch (reg) {
    case CPPMNC:
        temp_value=s->pmnc;
        SIMPLE_DBG("pxa2xx_perf_read: read PMNC reg=%u.\n", temp_value);
        break;

    case CPCCNT:
        //shocklink if (s->pmnc & 1)
        tmp = qemu_get_clock(vm_clock);
        temp_value = (uint32_t) tmp;
        SIMPLE_DBG("pxa2xx_perf_read: read CCNT reg=%lld.\n", tmp);
        break;

    case CPFLAG:
        temp_value=s->flag;
        SIMPLE_DBG("pxa2xx_perf_read: read FLAG reg=%u.\n", temp_value);
        break;

    case CPEVTSEL:
        temp_value=s->evtsel;
        SIMPLE_DBG("pxa2xx_perf_read: read EVTSEL reg=%u.\n", temp_value);
        break;

    case CPINTEN:
        temp_value=s->inten;
        SIMPLE_DBG("pxa2xx_perf_read: read INTEN reg=%u.\n", temp_value);
        break;

    default:
        ERR_MSG("%s: Bad register 0x%x\n", __FUNCTION__, reg);
        break;
    }
    return temp_value;
}

static void pxa2xx_perf_write(void *opaque, int op2, int reg, int crm,
                uint32_t value)
{
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;
    /* Bit mask of valid registers managed by FLAG register. */
    const uint32_t FLAG_MASK                = 0x0000001F;

    switch (reg) {
    case CPPMNC:
        //fprintf(stderr,"write pmnc=%d\n",value);
        s->pmnc = value;
        SIMPLE_DBG("%s: write PMNC=%x.\n",__FUNCTION__, value);
        break;

    case CPCCNT:
        ERR_MSG( "%s: write CCNT=%x. Not Handled.\n", __FUNCTION__, value);
        break;

    case CPINTEN:
        s->inten=value;
        SIMPLE_DBG("%s: write INTEN=%x.\n", __FUNCTION__, value);
        break;

    case CPFLAG:
        {
            /* Write to FLAG:
             * 1 = clear the bit.
             * 0 = no change.
             * Obtain the registers, which we want to reset its value.
             */
            uint32_t tmp    = value & FLAG_MASK;
            tmp             = (~tmp) & s->flag;
            s->flag         = tmp;
            SIMPLE_DBG("%s: write FLAG=%x.\n", __FUNCTION__,  tmp);
            /* Pull down the interrupt line. */
            vpmu_perf_counter_overflow_handled(s);
            break;
        }

    case CPEVTSEL:
        /* The value is corresponding to the event number */
        s->evtsel=value;
        SIMPLE_DBG("%s: write EVTSEL=%x.\n", __FUNCTION__, value);
        break;

    default:
        ERR_MSG( "%s: Bad register 0x%x.\n", __FUNCTION__, reg);
        break;
    }
}
uint32_t pas_cp14_read(void *opaque, int op2, int reg, int crm)
{
    static uint64_t time;
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;
    /* vpmu points to GlobalVPMU */
    VPMU *vpmu = s->vpmu_t;
    uint32_t temp_value=0;

    SIMPLE_DBG("%s: read from CP 14 cmd -> CRn=%d, CRm=%d.\n", __FUNCTION__, reg, crm);

    switch (crm) {
    case 0:
        return pxa2xx_clkpwr_read(opaque, op2, reg, crm);

    /* READ PMNC, INTEN, FLAG, EVTSEL registers. */
    case 1:
        return pxa2xx_perf_read(opaque, op2, reg, crm);
    case 2:
        //fprintf(stderr,"!!s->pmnc=%d!!!\n",s->pmnc);
        switch (reg) {
        case CPPMN0:
            {
                if(s->pmnc&1)
                    SIMPLE_DBG("%s: read PMN0, but all counters are not enabled, PMNC=%u\n",
                               __FUNCTION__, s->pmnc);
#ifndef CONFIG_VPMU_OPROFILE
                return vpmu_hardware_event_select(s->evtsel, vpmu);
#else
                temp_value = vpmu_hardware_event_select(s->evtsel, vpmu);

                SIMPLE_DBG("%s: event[%d]=%u, pmn_reset_value[0]=%u, counter[0]=%u.\n",
                           __FUNCTION__, s->evtsel, (temp_value - s->pmn_reset_value[0] +  s->pmn[0]),
                           s->pmn_reset_value[0], s->pmn[0]);

                return temp_value - s->pmn_reset_value[0] + s->pmn[0];
#endif

#if 0
                /* Instruction Miss Count event 0x00
                 * 不知為何perfex -e 0 會使得s->evtsel變成-1*/
                if(s->evtsel==-1)
                {
                    return (uint32_t)GlobalVPMU.icache->miss[D4XINSTRN];
                }   
                /* Data sheet model : report estimated time */
                if(s->evtsel==0x01)
                {
                    return vpmu_estimated_execution_time();

/* deprecated
                    static uint32_t previousFetch=1,previousMiss=1,previousTick=1,
                                    nowFetch=1,nowMiss=1,nowTick=1,
                                    diffFetch=1,diffMiss=1,diffTick=1;
                    static char __FirstTime__=0;

                    nowFetch=GlobalVPMU.dcache->fetch[D4XREAD]+ GlobalVPMU.dcache->fetch[D4XWRITE];
                    nowMiss=GlobalVPMU.dcache->miss[D4XREAD]+GlobalVPMU.dcache->miss[D4XWRITE]+GlobalVPMU.icache->miss[D4XINSTRN];
                    nowTick=GlobalVPMU.ticks;
                    diffFetch=nowFetch-previousFetch;
                    diffMiss=nowMiss-previousMiss;
                    diffTick=nowTick-previousTick;

                    static uint32_t time=0;
                    if(__FirstTime__==1)
                        time+=(int)((diffFetch*14.5+diffMiss*360) / 1000) + (int)(diffTick/520);
                    else
                        time=0;

                    previousFetch=GlobalVPMU.dcache->fetch[D4XREAD]+ GlobalVPMU.dcache->fetch[D4XWRITE];
                    previousMiss=GlobalVPMU.dcache->miss[D4XREAD]+GlobalVPMU.dcache->miss[D4XWRITE]+GlobalVPMU.icache->miss[D4XINSTRN];
                    previousTick=GlobalVPMU.ticks;
                    //fprintf(stderr,"time=%d",time);

                    if(__FirstTime__==1)
                        return time;
                    else
                    {
                        __FirstTime__=1;
                        return 0;
                    }*/
                    /*return (uint32_t)(
                    (GlobalVPMU.dcache->fetch[D4XREAD]+ GlobalVPMU.dcache->fetch[D4XWRITE])*14.5//hit latency
                    +(GlobalVPMU.dcache->miss[D4XREAD]+GlobalVPMU.dcache->miss[D4XWRITE]+GlobalVPMU.icache->fetch[D4XINSTRN])*360//miss penalty
                    )/1000
                    +(int)GlobalVPMU.ticks/520;*/

                }
                if(s->evtsel==0x03)
                    return vpmu_branch_predict(PREDICT_CORRECT);
                if(s->evtsel==0x04)
                    return vpmu_branch_predict(PREDICT_WRONG);
                if(s->evtsel==0x05)
                    return vpmu_branch_insn_count();
                if(s->evtsel==0x07)
                    return (uint32_t)vpmu_total_insn_count();
                if(s->evtsel==0x0A)
                    return (uint32_t)vpmu_dcache_access_count();
                if(s->evtsel==0x0B)
                {   /*fprintf(stderr,"MS=%.1f#%.1f#%.1f#%.1f#%.1f COMP=%.1f %.1f CAP=%.1f %.1f Conf=%.1f %.1f\n"
                        ,GlobalVPMU.dcache->miss[D4XREAD]
                        ,GlobalVPMU.dcache->miss[D4XWRITE]
                        ,GlobalVPMU.dcache->miss[D4XINSTRN]
                        ,GlobalVPMU.dcache->miss[D4XMISC]
                        ,GlobalVPMU.dcache->miss[D4PREFETCH]
                        ,GlobalVPMU.dcache->comp_miss[D4XREAD]
                        ,GlobalVPMU.dcache->comp_miss[D4XWRITE]
                        ,GlobalVPMU.dcache->cap_miss[D4XREAD]
                        ,GlobalVPMU.dcache->cap_miss[D4XWRITE]
                        ,GlobalVPMU.dcache->conf_miss[D4XREAD]
                        ,GlobalVPMU.dcache->conf_miss[D4XWRITE]);
*/
        //fprintf(stderr,"pxa2xx_perf_read return %d\n",(uint32_t)(GlobalVPMU.dcache->miss[D4XREAD]+GlobalVPMU.dcache->miss[D4XWRITE]));
                      return vpmu_dcache_miss_count();
                }
                if(s->evtsel==0x0D)
                    return GlobalVPMU.all_changes_to_PC_count;
                /* Virtual Power Device events, added by gkk886. */
                if(s->evtsel == 0xff)
                {
                        return (uint32_t)(vpmu_total_insn_count()*4.2*(1/1000000.0)*
                                           ((GlobalVPMU.cap/1000000.0)*
                                            (GlobalVPMU.volt_info[GlobalVPMU.mult-15]/1000000.0)*
                                            (GlobalVPMU.volt_info[GlobalVPMU.mult-15]/1000000.0)+
                                            (GlobalVPMU.base_power)/
                                            (GlobalVPMU.freq_table[0])));
                }
#endif
            }
        case CPPMN1:
        case CPPMN2:
        case CPPMN3:
            time=vpmu_estimated_execution_time_ns();
            SIMPLE_DBG("%s: read PMN1/PMN2/PMN3 - second=%llu. (CRn=%d) Not Handled.\n",
                       __FUNCTION__, time/1000000000, reg);
            return time/1000000000;
        default:
            ERR_MSG( "%s: Bad register 0x%d. Only support PMN0-3.\n", __FUNCTION__, reg);
            break;
        }
        /* Fall through */
    default:
        ERR_MSG( "%s: Bad register (CRm value) 0x%x\n", __FUNCTION__, reg);
        break;
    }
    return 0;
}
void pas_cp14_write(void *opaque, int op2, int reg, int crm,
                uint32_t value)
{
    struct cp14_pmu_state *s = (struct cp14_pmu_state *) opaque;
    /* vpmu points to GlobalVPMU */
    VPMU *vpmu = s->vpmu_t;

    SIMPLE_DBG("%s: write to CP 14 cmd -> CRn=%d, CRm=%d. value=%x\n", __FUNCTION__, reg, crm, value);

    switch (crm) {
    case 0:
        pxa2xx_clkpwr_write(opaque, op2, reg, crm, value);
        break;
    case 1:
        pxa2xx_perf_write(opaque, op2, reg, crm, value);
        break;
    case 2: /* xsc2_read_pmc */
        switch (reg) {
        case CPPMN0:
            s->pmn[0]=value;
            SIMPLE_DBG("%s: write PMN0=%x.\n", __FUNCTION__, value);

            /* VPMU for Oprofile setting.
             * This variable is used to remember the current value of performance data.
             */
            #ifdef CONFIG_VPMU_OPROFILE
                s->pmn_reset_value[0]=vpmu_hardware_event_select(s->evtsel, vpmu);
                SIMPLE_DBG("\t pmn_reset_value[0]=%u, global vpmu[%d]=%u.\n",
                        s->pmn_reset_value[0], s->evtsel, vpmu_hardware_event_select(s->evtsel, vpmu));
            #endif

            return;

        case CPPMN1:
            s->pmn[1]=value;
            SIMPLE_DBG( "%s: write PMN1=%u. Not Handled.\n", __FUNCTION__, value);
            return;

        case CPPMN2:
            s->pmn[2]=value;
            SIMPLE_DBG( "%s: write PMN2=%u. Not Handled.\n", __FUNCTION__, value);
            return;

        case CPPMN3:
            s->pmn[3]=value;
            SIMPLE_DBG( "%s: write PMN3=%u. Not Handled.\n", __FUNCTION__, value);
            return;

        default:
            ERR_MSG( "%s: Bad register 0x%d. Only support PMN0-3.\n", __FUNCTION__, reg);
            return;
        }
        /* Fall through */
    default:
        ERR_MSG( "%s: Bad register (CRm value) 0x%x\n", __FUNCTION__, reg);
        break;
    }
}
void tb_extra_info_init(struct ExtraTBInfo * tb_extra , unsigned long pc)
{
    tb_extra->mem_pattern = 0;
    tb_extra->mem_ref_count = 0 ;
    tb_extra->same_hit_count = 0 ;
    //tb_extra->last_mem_ref_count = 0 ;
    tb_extra->pc = pc ;
    //tb_extra->target_mem_address = tb_extra->last_mem_address ;
    //tb_extra->target_mem_ref_count= &tb_extra->last_mem_ref_count ;
    tb_extra->same_ideal_flush_count = 1 ;
    tb_extra->ticks = 0 ;
    tb_extra->insn_count =0;
    tb_extra->branch_count = 0;
    tb_extra->all_changes_to_PC_count = 0;
    tb_extra->load_count = 0;
    tb_extra->store_count = 0;

	//tianman
	int i;
#ifdef CONFIG_VPMU_VFP
	for(i=0;i<ARM_VFP_INSTRUCTIONS;i++){
		tb_extra->vfp_count[i]=0;
	}
#endif
	for(i=0;i<ARM_INSTRUCTIONS+2;i++){
		tb_extra->arm_count[i]=0;
	}

	//evo0209
	tb_extra->insn_buf[0] = 0;
	tb_extra->insn_buf[1] = 0;
	tb_extra->insn_buf_index = 0;
	tb_extra->dual_issue_reduce_ticks = 0;
}

d4cache* d4_mem_create(void)
{
    d4cache * mem = d4new( NULL );
    mem->name = strdup( "dinero Main Memory ");
    if ( mem == NULL )
        printf("Main mem error \n" );
    return mem ;
}

//evo0209
static inline void d4_cache_init(d4cache *c, int lev, int idu)
{
	c->name = malloc (30);
	if (c->name == NULL)
		printf("malloc failure initializing l%d%ccache\n", lev+1, idu==0?'u':(idu==1?'i':'d'));
	sprintf (c->name, "l%d-%ccache", lev+1, idu==0?'u':(idu==1?'i':'d'));

	c->flags |= GlobalVPMU.d4_level_doccc[idu][lev] ? D4F_CCC : 0;
	if (idu == 1)
		c->flags |= D4F_RO;
	c->lg2blocksize = clog2 (GlobalVPMU.d4_level_blocksize[idu][lev]);
	c->lg2subblocksize = clog2 (GlobalVPMU.d4_level_subblocksize[idu][lev]);
	c->lg2size = clog2 (GlobalVPMU.d4_level_size[idu][lev]);
	c->assoc = GlobalVPMU.d4_level_assoc[idu][lev];

	switch (GlobalVPMU.d4_level_replacement[idu][lev]) {
		default:  printf("replacement policy '%c' initialization botch\n", GlobalVPMU.d4_level_replacement[idu][lev]);
		case 'l': c->replacementf = d4rep_lru; c->name_replacement = "LRU"; break;
		case 'f': c->replacementf = d4rep_fifo; c->name_replacement = "FIFO"; break;
		case 'r': c->replacementf = d4rep_random; c->name_replacement = "random"; break;
	}

	switch (GlobalVPMU.d4_level_fetch[idu][lev]) {
		default:  printf("fetch policy '%c' initialization botch\n", GlobalVPMU.d4_level_fetch[idu][lev]);
		case 'd': c->prefetchf = d4prefetch_none; c->name_prefetch = "demand only"; break;
		case 'a': c->prefetchf = d4prefetch_always; c->name_prefetch = "always"; break;
		case 'm': c->prefetchf = d4prefetch_miss; c->name_prefetch = "miss"; break;
		case 't': c->prefetchf = d4prefetch_tagged; c->name_prefetch = "tagged"; break;
		case 'l': c->prefetchf = d4prefetch_loadforw; c->name_prefetch = "load forward"; break;
		case 's': c->prefetchf = d4prefetch_subblock; c->name_prefetch = "subblock"; break;
	}

	switch (GlobalVPMU.d4_level_walloc[idu][lev]) {
		default:  printf("write allocate policy '%c' initialization botch\n", GlobalVPMU.d4_level_walloc[idu][lev]);
		case 0:   c->wallocf = d4walloc_impossible; c->name_walloc = "impossible"; break;
		case 'a': c->wallocf = d4walloc_always; c->name_walloc = "always"; break;
		case 'n': c->wallocf = d4walloc_never; c->name_walloc = "never"; break;
		case 'f': c->wallocf = d4walloc_nofetch; c->name_walloc = "nofetch"; break;
	}

	switch (GlobalVPMU.d4_level_wback[idu][lev]) {
		default:  printf("write back policy '%c' initialization botch\n", GlobalVPMU.d4_level_wback[idu][lev]);
		case 0:   c->wbackf = d4wback_impossible; c->name_wback = "impossible"; break;
		case 'a': c->wbackf = d4wback_always; c->name_wback = "always"; break;
		case 'n': c->wbackf = d4wback_never; c->name_wback = "never"; break;
		case 'f': c->wbackf = d4wback_nofetch; c->name_wback = "nofetch"; break;
	}

	c->prefetch_distance = GlobalVPMU.d4_level_prefetch_distance[idu][lev] * GlobalVPMU.d4_level_subblocksize[idu][lev];
	c->prefetch_abortpercent = GlobalVPMU.d4_level_prefetch_abortpercent[idu][lev];
}


#ifdef CONFIG_VPMU_THREAD

#define TRACE_QUEUE_SIZE 100
#define TRACE_SET 500
#define BRANCH_QUEUE_SIZE 200 //100 is enough
pthread_t d_cache_thread;
pthread_t branch_predictor_thread;
pthread_mutex_t mutex_sync_cache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_sync_branch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_suspend = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  branch_cond  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cache_cond  = PTHREAD_COND_INITIALIZER;

/* Buffer for threads */
d4memref TraceBuffer[TRACE_SET][TRACE_QUEUE_SIZE];
uint8_t BranchTrace[BRANCH_QUEUE_SIZE];

/* varibles for synchronization */
volatile uint32_t set_used = 0;
volatile unsigned int branch_buffer_size = 0;

void *branch_thread(void * ptr)
{
    fprintf(stderr,"VPMU worker thread (branch predictor) : start\n");
    unsigned int branch_index_tail = 0;
    while(1) {
        while(VPMU_enabled) {
            /* buffer empty : wait */
            while(branch_buffer_size == 0) {
                if(!VPMU_enabled)
                    goto SUSPEND;
            }
            branch_set(BranchTrace[branch_index_tail]);
            BranchTrace[branch_index_tail]=33;

            branch_index_tail ++;
            branch_index_tail %= BRANCH_QUEUE_SIZE;

            pthread_mutex_lock(&mutex_sync_branch);
            branch_buffer_size --;
            pthread_mutex_unlock(&mutex_sync_branch);
        }
SUSPEND:
        /* suspend thread when VPMU is disabled */
        pthread_mutex_lock(&mutex_suspend);
        pthread_cond_wait(&branch_cond, &mutex_suspend);
        pthread_mutex_unlock(&mutex_suspend);
    }
}

void *dinero_thread(void * opaque)
{
    VPMU *vpmu = (VPMU *)opaque;
    fprintf(stderr,"VPMU worker thread (cache simulator)  : start\n");
    d4memref r;
    /* static int seems to take more time to access */
    unsigned int set=0, index=0;
    while(1) {
        while(VPMU_enabled || set_used > 0) {
            if(set_used > 0) {
                while(index != TRACE_QUEUE_SIZE) {
                    r = TraceBuffer[set][index];
                    /* this can dump trace */
                    //fprintf(stderr,"DEQ: TYPE=%d ADDR=%x SIZE=%d \n", r.accesstype, r.address, r.size);
                    if(unlikely(r.accesstype == D4XINSTRN))
                        d4ref(vpmu->icache, r);
                    else
                        d4ref(vpmu->dcache, r);
                    index++;
                }
                /* consume one set */
                pthread_mutex_lock( &mutex_sync_cache );
                set_used--;
                pthread_mutex_unlock( &mutex_sync_cache );

                index=0;
                set++;
                if(set == TRACE_SET)
                    set=0;
            }
        }
        /* suspend thread when VPMU is disabled */
        pthread_mutex_lock(&mutex_suspend);
        pthread_cond_wait(&cache_cond, &mutex_suspend);
        pthread_mutex_unlock(&mutex_suspend);
    }
}
#endif

/* future work : use this as a global API to put events
 * TODO : replace "cache_ref"*/
void put_event(uint32_t event, uint32_t cur_sig)
{
    if(event == 0xb)
    {
#ifdef CONFIG_VPMU_THREAD
        static unsigned int branch_index_head = 0;
        BranchTrace[branch_index_head] = (uint8_t)cur_sig;

        /* buffer full : wait */
        while(branch_buffer_size == (BRANCH_QUEUE_SIZE - 1))
            ;

        branch_index_head ++;
        branch_index_head %= BRANCH_QUEUE_SIZE;

        pthread_mutex_lock(&mutex_sync_branch);
        branch_buffer_size ++;
        pthread_mutex_unlock(&mutex_sync_branch);
#else
        branch_set(cur_sig);
#endif
    }
}

void cache_ref( unsigned int addr, int rw, int data_size )
{
    d4memref r;

    r.address = addr;
    r.accesstype = rw;
    r.size = data_size;

#ifndef CONFIG_VPMU_THREAD
    if(unlikely(r.accesstype == D4XINSTRN))
        d4ref( GlobalVPMU.icache , r );
    else
        d4ref( GlobalVPMU.dcache , r );
#else
    static int set=0, index=0;

    /* set full */
    while(set_used == TRACE_SET)
        ;

    TraceBuffer[set][index] = r;
    index++;

    /* fill one set line */
    if( index == TRACE_QUEUE_SIZE ) {
        /* produce one line */
        pthread_mutex_lock( &mutex_sync_cache );
        set_used++;
        pthread_mutex_unlock( &mutex_sync_cache );

        index=0;
        set++;
        if(set == TRACE_SET)
            set=0;
    }
    /* this can dump trace */
    //fprintf(stderr,"ENQ: TYPE=%d ADDR=%x SIZE=%d\n", r.accesstype, r.address, r.size);
#endif
}

//evo0209
void ResetCache(d4cache *d)
{
    /*static d4memref r;
    //r.accesstype = D4XCOPYB;   shocklink
    r.accesstype = D4XINVAL;
    r.address = 0;
    r.size = 0;
    d4ref( d,r );
    */
    d->fetch[D4XREAD] = 0;
    d->fetch[D4XWRITE] = 0;
    d->fetch[D4XINSTRN] = 0;
    d->miss[D4XREAD] = 0;
    d->miss[D4XWRITE] = 0;
    d->miss[D4XINSTRN] = 0;

    d->comp_miss[D4XREAD] = 0;
    d->cap_miss[D4XREAD]  = 0;
    d->conf_miss[D4XREAD] = 0;

    d->comp_miss[D4XINSTRN] = 0;
    d->cap_miss[D4XINSTRN] = 0;
    d->conf_miss[D4XINSTRN] = 0;

    d->comp_miss[D4XWRITE] = 0;
    d->cap_miss[D4XWRITE]  = 0;
    d->conf_miss[D4XWRITE] = 0;
}

uint32_t vpmu_vpd_mpu_power(void)
{
//evo0209
//#ifdef CONFIG_PACDSP
//    vpmu_update_eet_epc();
//    return GlobalVPMU.epc_mpu;
//#else
//    //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(4.34*adev_stat.cpu_util));
//    return (uint32_t)(4.34 * adev_stat.cpu_util);
//#endif

	/* evo0209 fixed : check the power estimation mode is mpu or peripheral */
	if (VPMU_enabled)
	{
		vpmu_update_eet_epc();
		return GlobalVPMU.epc_mpu;
	}
	else
		return (uint32_t)(4.34 * adev_stat.cpu_util);
}

uint32_t vpmu_vpd_dsp1_power(void)
{
#ifdef CONFIG_PACDSP
    pacdsp_state_t *pac;
    uint64_t dsp_tick = 0;
    int dsp_freq;
    int i;

    FOREACH_PACDSP_I(pac, i) {
        if(i == 0) {
            dsp_tick = paciss_state_get_tick(pac->pac);
            break;
        }
    }
    dsp_freq = pacduo_dsp_freq(0);

    return (uint32_t)(((dsp_tick/dsp_freq)*GlobalVPMU.dsp1_power)/1000.0);
#else
    return 0;
#endif
}

uint32_t vpmu_vpd_dsp2_power(void)
{
#ifdef CONFIG_PACDSP
    pacdsp_state_t *pac;
    uint64_t dsp_tick = 0;
    int dsp_freq;
    int i;

    FOREACH_PACDSP_I(pac, i) {
        if(i == 1) {
            dsp_tick = paciss_state_get_tick(pac->pac);
            break;
        }
    }
    dsp_freq = pacduo_dsp_freq(1);

    return (uint32_t)(((dsp_tick/dsp_freq)*GlobalVPMU.dsp2_power)/1000.0);
#else
    return 0;
#endif
}

uint32_t vpmu_vpd_lcd_power(void)
{
    //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*
    //        (*(adev_stat.lcd_brightness) ? *(adev_stat.lcd_brightness)*2.4+287.3 : 0.0));
    return (uint32_t)(*(adev_stat.lcd_brightness) ? *(adev_stat.lcd_brightness)*2.4+287.3 : 0.0);
}

uint32_t vpmu_vpd_audio_power(void)
{
    //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(adev_stat.audio_on ? 384.62 : 0.0));
    return (uint32_t)(adev_stat.audio_on ? 384.62 : 0.0);
}

uint32_t vpmu_vpd_net_power(void)
{
    if(adev_stat.net_mode == 1) //Wifi mode
        //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(adev_stat.net_on ? 720.0 : 38.0));
        return (uint32_t)(adev_stat.net_on ? 720.0 : 38.0);
    else if(adev_stat.net_mode == 2) //3G mode
        //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(adev_stat.net_on ? 401.0 : 0.0));
        return (uint32_t)(adev_stat.net_on ? 401.0 : 0.0);
    else //off
        return 0;
}

uint32_t vpmu_vpd_gps_power(void)
{
    if(*(adev_stat.gps_status) == 2) //active
        //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(429.55));
        return (uint32_t)(429.55);
    else if(*(adev_stat.gps_status) == 1) //idle
        //return (uint32_t)((vpmu_estimated_execution_time()/1000.0)*(173.55));
        return (uint32_t)(173.55);
    else //off
        return 0;
}

uint32_t vpmu_vpd_total_power(void)
{
#ifdef CONFIG_PACDSP
    return vpmu_vpd_mpu_power()+vpmu_vpd_dsp1_power()+vpmu_vpd_dsp2_power();
#else
    return vpmu_vpd_mpu_power()+vpmu_vpd_lcd_power()+vpmu_vpd_audio_power()+
        vpmu_vpd_net_power()+vpmu_vpd_gps_power();
#endif
}

//tianman
void print_arm_count(int print)
{
	int i;
	uint64_t counted = 0;
	uint64_t total_counted = 0;

	GlobalVPMU.ARM_count[ARM_INSTRUCTION_COPROCESSOR] = GlobalVPMU.ARM_count[ARM_INSTRUCTION_COPROCESSOR] -
		GlobalVPMU.ARM_count[ARM_INSTRUCTIONS+1];

	if (print == 1)
	{
		//evo0209 sorting
		int j;
		uint64_t tmp_count[ARM_INSTRUCTIONS];
		uint64_t tmp_count_key[ARM_INSTRUCTIONS];
		uint64_t t1, t2;

		for (i = 0; i < ARM_INSTRUCTIONS; i++)
		{
			tmp_count[i] = GlobalVPMU.ARM_count[i];
			tmp_count_key[i] = i;
		}

		for (i = ARM_INSTRUCTIONS - 1; i > 0; i--)
			for (j = 0; j <= i - 1; j++)
			{
				if (tmp_count[j] < tmp_count[j + 1])
				{
					t1 = tmp_count[j];
					tmp_count[j] = tmp_count[j + 1];
					tmp_count[j + 1] = t1;

					t2 = tmp_count_key[j];
					tmp_count_key[j] = tmp_count_key[j + 1];
					tmp_count_key[j + 1] = t2;
				}
			}
		//evo0209 end

		for (i = 0; i < ARM_INSTRUCTIONS; i++) {
			t1 = tmp_count_key[i];
			if (GlobalVPMU.ARM_count[t1] > 0)
			{
				//print top 10 only
				if (i < 10)
				{
					fprintf(stderr, "%s: %llu ",arm_instr_names[t1], GlobalVPMU.ARM_count[t1]);
					fprintf(stderr, "need = %d spend cycle = %llu\n", arm_instr_time[t1], GlobalVPMU.ARM_count[t1] * arm_instr_time[t1]);
				}
				if (t1 < (ARM_INSTRUCTIONS - 2))
				{
					counted += GlobalVPMU.ARM_count[t1];
					total_counted += GlobalVPMU.ARM_count[t1] * arm_instr_time[t1];
				}
			}
		}

		counted += GlobalVPMU.ARM_count[ARM_INSTRUCTIONS + 1];
		total_counted += GlobalVPMU.ARM_count[ARM_INSTRUCTIONS + 1];
		fprintf(stderr, "%s: %llu \n",arm_instr_names[ARM_INSTRUCTIONS], GlobalVPMU.ARM_count[ARM_INSTRUCTIONS + 1]);
		fprintf(stderr, "Counted instructions: %llu\n", counted);
		fprintf(stderr, "total Counted cycle: %llu\n", total_counted - GlobalVPMU.reduce_ticks);
		fprintf(stderr, "Dual issue reduce ticks: %d\n", GlobalVPMU.reduce_ticks);
	}
	else
	{
		//GlobalVPMU.ticks = 0;
		//for (i = 0; i < ARM_INSTRUCTIONS; i++)
		//{
		//	if (GlobalVPMU.ARM_count[i] > 0) {
		//		if (i < (ARM_INSTRUCTIONS - 2))
		//			GlobalVPMU.ticks += GlobalVPMU.ARM_count[i] * arm_instr_time[i];
		//	}
		//}
		//GlobalVPMU.ticks += GlobalVPMU.ARM_count[ARM_INSTRUCTIONS + 1];
		//GlobalVPMU.ticks -= GlobalVPMU.reduce_ticks;
		//printf("Ticks %llu, reduce %llu\n", GlobalVPMU.ticks, GlobalVPMU.reduce_ticks);
	}
}

//tianman
#ifdef CONFIG_VPMU_VFP
void print_vfp_count(void)
{
	int i;
	uint64_t counted = 0;
	uint64_t total_counted = 0;

	for (i = 0; i < ARM_VFP_INSTRUCTIONS; i++) {
		if (GlobalVPMU.VFP_count[i] > 0) {
			fprintf(stderr, "%s: %llu ",arm_vfp_instr_names[i], GlobalVPMU.VFP_count[i]);
			fprintf(stderr, "need = %d spend cycle = %llu\n", arm_vfp_instr_time[i], GlobalVPMU.VFP_count[i]*arm_vfp_instr_time[i]);

			if (i < (ARM_VFP_INSTRUCTIONS - 2)){
				counted += GlobalVPMU.VFP_count[i];
				total_counted += GlobalVPMU.VFP_count[i]*arm_vfp_instr_time[i];
			}

		}
	}
	fprintf(stderr, "total latency: %llu\n", GlobalVPMU.VFP_BASE);
	fprintf(stderr, "Counted instructions: %llu\n", counted);
	fprintf(stderr, "total Counted cycle: %llu\n", total_counted);

}
#endif

uint64_t vpmu_branch_insn_count(void)
{
    return GlobalVPMU.branch_count;
}

#ifdef CONFIG_VPMU_THREAD
static inline void wait_cache_thread_buf(void)
{
    while(set_used > 0)
        ;
}

void VPMU_sync_all(void)
{
    fprintf(stderr,"VPMU SYNC\n");
    /* synchronization for D-cache/I-cache buffer */
    if(vpmu_simulator_status(&GlobalVPMU, VPMU_DCACHE_SIM)
            ||vpmu_simulator_status(&GlobalVPMU, VPMU_ICACHE_SIM)) {
        wait_cache_thread_buf();
    }
}

#endif

#if 0
uint32_t vpmu_dcache_access_count(void)
{
    return (uint32_t)
        GlobalVPMU.dcache->fetch[D4XREAD]
        + GlobalVPMU.dcache->fetch[D4XWRITE];
}
#endif

uint64_t cb_dcache_access_count(void)
{
    return (uint64_t)vpmu_L1_dcache_access_count();
}

uint64_t cb_icache_access_count(void)
{
    return (uint64_t)vpmu_L1_icache_access_count();
}

//double vpmu_dcache_access_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return GlobalVPMU.dcache->fetch[D4XREAD]
//        + GlobalVPMU.dcache->fetch[D4XWRITE];
//}
//
//uint64_t vpmu_dcache_miss_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return (uint64_t)
//        (GlobalVPMU.dcache->miss[D4XREAD]
//        +GlobalVPMU.dcache->miss[D4XWRITE]);
//}
//
//uint64_t vpmu_dcache_read_miss_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return (uint64_t)GlobalVPMU.dcache->miss[D4XREAD];
//}
//
//uint64_t vpmu_dcache_write_miss_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return (uint64_t)GlobalVPMU.dcache->miss[D4XWRITE];
//}
//
//double vpmu_icache_access_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_ICACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return GlobalVPMU.icache->fetch[D4XINSTRN]
//        + (double)GlobalVPMU.additional_icache_access;
//}
//
//uint64_t vpmu_icache_miss_count(void)
//{
//#ifdef CONFIG_VPMU_THREAD
//    if(GlobalVPMU.timing_register & VPMU_ICACHE_SIM)
//        wait_cache_thread_buf();
//#endif
//    return (uint64_t)GlobalVPMU.icache->miss[D4XINSTRN];
//}

//evo0209
uint64_t vpmu_L1_dcache_access_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[2][0]->fetch[D4XREAD]
			+ GlobalVPMU.d4_levcache[2][0]->fetch[D4XWRITE]);
}

//evo0209
uint64_t vpmu_L1_dcache_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[2][0]->miss[D4XREAD]
			+ GlobalVPMU.d4_levcache[2][0]->miss[D4XWRITE]);
}

//evo0209
uint64_t vpmu_L1_dcache_read_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[2][0]->miss[D4XREAD]);
}

//evo0209
uint64_t vpmu_L1_dcache_write_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[2][0]->miss[D4XWRITE]);
}

//evo0209
//only for unified L2 cache now !!!!
uint64_t vpmu_L2_dcache_access_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[0][1]->fetch[D4XREAD]
			+ GlobalVPMU.d4_levcache[0][1]->fetch[D4XWRITE]);
}

//evo0209
//only for unified L2 cache now !!!!
uint64_t vpmu_L2_dcache_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[0][1]->miss[D4XREAD]
			+ GlobalVPMU.d4_levcache[0][1]->miss[D4XWRITE]);
}

//evo0209
//only for unified L2 cache now !!!!
uint64_t vpmu_L2_dcache_read_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[0][1]->miss[D4XREAD]);
}

//evo0209
//only for unified L2 cache now !!!!
uint64_t vpmu_L2_dcache_write_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_DCACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)(GlobalVPMU.d4_levcache[0][1]->fetch[D4XWRITE]);
}

uint64_t vpmu_L1_icache_access_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_ICACHE_SIM)
		wait_cache_thread_buf();
#endif
	return GlobalVPMU.d4_levcache[1][0]->fetch[D4XINSTRN]
		+ (double)GlobalVPMU.additional_icache_access;
}

uint64_t vpmu_L1_icache_miss_count(void)
{
#ifdef CONFIG_VPMU_THREAD
	if(GlobalVPMU.timing_register & VPMU_ICACHE_SIM)
		wait_cache_thread_buf();
#endif
	return (uint64_t)GlobalVPMU.d4_levcache[1][0]->miss[D4XINSTRN];
}

uint64_t vpmu_total_insn_count(void)
{
    return GlobalVPMU.USR_icount
         + GlobalVPMU.SVC_icount
         + GlobalVPMU.sys_call_icount
         + GlobalVPMU.sys_rest_icount;
}

uint64_t vpmu_total_load_count(void)
{
    return GlobalVPMU.USR_load_count
         + GlobalVPMU.SVC_load_count
         + GlobalVPMU.IRQ_load_count
         + GlobalVPMU.sys_call_load_count
         + GlobalVPMU.sys_rest_load_count;
}

uint64_t vpmu_total_store_count(void)
{
    return GlobalVPMU.USR_store_count
         + GlobalVPMU.SVC_store_count
         + GlobalVPMU.IRQ_store_count
         + GlobalVPMU.sys_call_store_count
         + GlobalVPMU.sys_rest_store_count;
}

uint64_t vpmu_total_ldst_count(void)
{
    return GlobalVPMU.USR_ldst_count
         + GlobalVPMU.SVC_ldst_count
         + GlobalVPMU.IRQ_ldst_count
         + GlobalVPMU.sys_call_ldst_count
         + GlobalVPMU.sys_rest_ldst_count;
}

uint64_t vpmu_cycle_count(void)
{
    //uint64_t nowMiss=0;
	//evo0209
	uint64_t L1_nowMiss = 0;
	uint64_t L2_nowMiss = 0;

    switch(GlobalVPMU.timing_register & (0xf<<28))
    {
        case TIMING_MODEL_A:
        case TIMING_MODEL_B:
            break; // do nothing
        case TIMING_MODEL_C:
            //nowMiss = vpmu_dcache_miss_count();
			//evo0209
			L1_nowMiss = vpmu_L1_dcache_miss_count();
			if (GlobalVPMU.d4_maxlevel > 1)
				L2_nowMiss = vpmu_L2_dcache_miss_count();
            break;
        case TIMING_MODEL_D:
        case TIMING_MODEL_E:
		//evo0209 fixed for SET plus insn_count_sim
		case TIMING_MODEL_F:
		case TIMING_MODEL_G:
            //nowMiss = vpmu_dcache_miss_count() + vpmu_icache_miss_count();
			//evo0209
			L1_nowMiss = vpmu_L1_dcache_miss_count() + vpmu_L1_icache_miss_count();
			if (GlobalVPMU.d4_maxlevel > 1)
				L2_nowMiss = vpmu_L2_dcache_miss_count();
            break;
        default:
            fprintf(stderr,"Wrong Model Number\n");
            break;
    }
#ifdef CONFIG_PACDSP
    //return GlobalVPMU.ticks + GlobalVPMU.offset + nowMiss * CACHE_MISS_LATENCY;
	//evo0209, chiaheng
	return GlobalVPMU.ticks + GlobalVPMU.offset
							+ (L1_nowMiss + GlobalVPMU.iomem_count) * GlobalVPMU.l1_cache_miss_lat
							+ L2_nowMiss * GlobalVPMU.l2_cache_miss_lat;
#else
    //return GlobalVPMU.ticks + nowMiss * CACHE_MISS_LATENCY;

	//evo0209, chiaheng
	//printf("Ticks %u, L1_miss %u, L2_miss %u, iomem_count %u, L1_miss_lat %u, L2_miss_lat %u\n",
	//		GlobalVPMU.ticks, L1_nowMiss, L2_nowMiss, GlobalVPMU.iomem_count,
	//		GlobalVPMU.l1_cache_miss_lat, GlobalVPMU.l2_cache_miss_lat);
	//printf("Total count %llu\n", GlobalVPMU.ticks + (L1_nowMiss + GlobalVPMU.iomem_count) * GlobalVPMU.l1_cache_miss_lat
	//		                            + L2_nowMiss * GlobalVPMU.l2_cache_miss_lat);
	//return GlobalVPMU.ticks + (L1_nowMiss + GlobalVPMU.iomem_count) * GlobalVPMU.l1_cache_miss_lat
	//						+ L2_nowMiss * GlobalVPMU.l2_cache_miss_lat;
	//printf("PIPELINE %llu, MISS %llu, IO %llu\n",
	//		GlobalVPMU.ticks,
	//		L1_nowMiss * GlobalVPMU.l1_cache_miss_lat,
	//		GlobalVPMU.iomem_qemu * GlobalVPMU.l1_cache_miss_lat);

	return GlobalVPMU.ticks + (L1_nowMiss + GlobalVPMU.iomem_qemu) * GlobalVPMU.l1_cache_miss_lat
							+ L2_nowMiss * GlobalVPMU.l2_cache_miss_lat;
#endif
}

//chiaheng
uint64_t vpmu_pipeline_cycle_count (void)
{
#ifdef CONFIG_PACDSP
	return GlobalVPMU.ticks + GlobalVPMU.offset;
#else
	return GlobalVPMU.ticks;
#endif
}

//chiaheng
uint64_t vpmu_io_memory_access_cycle_count (void)
{
	//return GlobalVPMU.iomem_count * GlobalVPMU.l1_cache_miss_lat;
	return GlobalVPMU.iomem_qemu * GlobalVPMU.l1_cache_miss_lat;
}

//chiaheng
uint64_t vpmu_sys_memory_access_cycle_count(void)
{
	//evo0209
	if (GlobalVPMU.cpu_model == 1)
		return ((vpmu_L1_dcache_miss_count() + vpmu_L1_icache_miss_count())
				* GlobalVPMU.l1_cache_miss_lat) +
			(vpmu_L2_dcache_miss_count() * GlobalVPMU.l2_cache_miss_lat);
	else
		return (vpmu_L1_dcache_miss_count() + vpmu_L1_icache_miss_count())
			* GlobalVPMU.l1_cache_miss_lat;
}

static inline uint64_t branch_predict_correct(void)
{
    return (uint64_t)vpmu_branch_predict(PREDICT_CORRECT);
}

static inline uint64_t branch_predict_wrong(void)
{
    return (uint64_t)vpmu_branch_predict(PREDICT_WRONG);
}

uint32_t vpmu_branch_predict(uint8_t request)
{
#ifdef CONFIG_VPMU_THREAD
    if(GlobalVPMU.timing_register & VPMU_BRANCH_SIM)
        while(branch_buffer_size > 0)
        {
        }
#endif
    /* VPMU sync with branch predictor */
    branch_update(BRANCH_PREDICT_SYNC);
    if(request == PREDICT_CORRECT)
        return GlobalVPMU.branch_predict_correct;
    if(request == PREDICT_WRONG)
        return GlobalVPMU.branch_predict_wrong;
    fprintf(stderr,"%s" "unexpected condition\n",__FUNCTION__);
    return 0;
}

#if defined(CONFIG_PACDSP)
/* deprecated */
#if 0
static inline uint32_t dsp_real_exec_time(unsigned int dsp_frequency)
{
    //fprintf(stderr,"dsp_real_exec_time=%d\n",(paciss_state_get_tick((&pacdsp.head)->lh_first->pac)/dsp_frequency));
    return (paciss_state_get_tick((&pacdsp.head)->lh_first->pac)/dsp_frequency);
}
#endif

static inline uint32_t vpmu_side_dsp_exec_time(void)
{
    /* shocklink modify later
    fprintf(stderr,"vpmu_side=%u\n",GlobalVPMU.dsp_start_time);
    return GlobalVPMU.dsp_start_time;
    */
}

/* find the slowest(timing) running DSP time */
static inline uint64_t vpmu_find_min_dsp_time(struct VPMU *vpmu)
{
    pacdsp_state_t *pac;
    uint64_t tmp[16] = {0};
    uint64_t dsp_time;
    uint64_t min = UINT64_MAX;
    int i;

    FOREACH_PACDSP_I(pac, i) {
        tmp[i] = paciss_state_get_tick(pac->pac);
    }
    for(i=0;i<16;i++) {
        /* If DSPs never start, don't consider these */
        if(tmp[i] <= DSP_START_CYCLE)
            continue;
        /* If DSPs finished, don't consider these */
        if(vpmu->dsp_working_period[i] != 0)
            continue;
        dsp_time = tmp[i]/GlobalVPMU.target_cpu_frequency + vpmu->dsp_start_time[i];
        if(dsp_time < min) {
            min = dsp_time;
        }
    }
    return min;
}

static char dsp_count = 0;
void vpmu_dsp_in(uint32_t id)
{
    if(dsp_count == 0) {
        fprintf(stderr,"vpmu_dsp_in\n");
        /* check VPMU state to prevent "Wrong Model Number" */
        if(VPMU_enabled)
            GlobalVPMU.dsp_running = 1;
    }
    if(VPMU_enabled)
        GlobalVPMU.dsp_start_time[id] = vpmu_estimated_execution_time();
    dsp_count ++;
}

void vpmu_dsp_out(uint32_t id)
{
    if(VPMU_enabled)
        GlobalVPMU.dsp_working_period[id] += vpmu_estimated_execution_time()
                                        - GlobalVPMU.dsp_start_time[id];
    dsp_count --;
    if(dsp_count == 0) {
        fprintf(stderr,"vpmu_dsp_out\n");
        GlobalVPMU.dsp_running = 0;
    }

    uint64_t tmp = 0;
    uint64_t dsp_time;
    int i;
    pacdsp_state_t *pac;

    FOREACH_PACDSP_I(pac, i) {
        if(i == id)
            tmp = paciss_state_get_tick(pac->pac);
    }
    dsp_time = tmp/GlobalVPMU.target_cpu_frequency + GlobalVPMU.dsp_start_time[id];

    /* Synchronization point: DSP finished its jobs
     * If any DSP is running faster than ARM
     * Add offset to ARM timing */
    int64_t test = dsp_time - vpmu_estimated_execution_time();
    if(test > 0) {
        GlobalVPMU.offset += test * GlobalVPMU.target_cpu_frequency;
    }
}

char suspend=0;
#if 0
void vpmu_suspend_qemu(void)
{
    if(1) {
        suspend = 0;
        uint32_t qemu_working_time;
        pacdsp_state_t *pac;
        uint64_t tmp[16] = {0};
        uint64_t max = 0;
        int i;

        FOREACH_PACDSP_I(pac, i) {
            tmp[i] = paciss_state_get_tick(pac->pac);
        }
        for(i=0;i<16;i++) {
            uint64_t dsp_time = tmp[i]/GlobalVPMU.target_cpu_frequency
                + GlobalVPMU.dsp_start_time[i];
            if(dsp_time > max) {
                max = dsp_time;
            }
        }
        qemu_working_time = vpmu_estimated_execution_time();

//#define SUSPEND_DBG
#ifdef SUSPEND_DBG
        fprintf(stderr,"(suspend) qemu=%u dsp time(max)=%llu ",vpmu_estimated_execution_time(),max);
#endif

        struct timespec ts;
        /* waiting 0.000001 sec equals to 6160~6500us in dsp */
        /* waiting 0.1 sec equals to 700us in dsp */
        ts.tv_sec=0;
        ts.tv_nsec=1000;
        //ts.tv_nsec=1000000;
        //ts.tv_nsec=14000000;
        sigset_t sis, osis;
        sigfillset(&sis);
        sigprocmask(SIG_BLOCK, &sis, &osis);
        while(qemu_working_time > (max + 3) && GlobalVPMU.dsp_running)
        {
            suspend = 1;
#ifdef SUSPEND_DBG
            //fprintf(stderr,"(suspend) qemu waiting = qemu=%u dsp=%llu \n",qemu_working_time,max);
            fprintf(stderr,"(suspend) qemu waiting ");
#endif
            while (nanosleep(&ts, &ts));

            FOREACH_PACDSP_I(pac, i) {
                tmp[i] = paciss_state_get_tick(pac->pac);
            }
            for(i=0;i<16;i++) {
                uint64_t dsp_time = tmp[i]/GlobalVPMU.target_cpu_frequency
                    + GlobalVPMU.dsp_start_time[i];
                if(dsp_time > max) {
                    max = dsp_time;
                }
            }
        }
        sigprocmask(SIG_SETMASK, &osis, NULL);
        suspend = 0;
    }
}
#endif

void vpmu_suspend_qemu_v2(void *opaque)
{
    VPMU *vpmu = (VPMU *)opaque;
    suspend = 0;
    uint32_t qemu_working_time;
    uint64_t min = UINT64_MAX;
    min = vpmu_find_min_dsp_time(vpmu);
    qemu_working_time = vpmu_estimated_execution_time();

    //#define SUSPEND_DBG
#ifdef SUSPEND_DBG
    fprintf(stderr,"(suspend) qemu=%u dsp time(min)=%llu ",vpmu_estimated_execution_time(),min);
#endif

    struct timespec ts;
    /* waiting 0.000001 sec equals to 6160~6500us in dsp */
    /* waiting 0.1 sec equals to 700us in dsp */
    ts.tv_sec=0;
    ts.tv_nsec=1000;
    //ts.tv_nsec=1000000;
    //ts.tv_nsec=14000000;
    sigset_t sis, osis;
    sigfillset(&sis);
    sigprocmask(SIG_BLOCK, &sis, &osis);
    while(GlobalVPMU.dsp_running && (qemu_working_time > min)) {
        suspend = 1;
#ifdef SUSPEND_DBG
        fprintf(stderr,"(suspend) qemu waiting ");
#endif
        /* sleep count from 0 to 218454 */
        while (nanosleep(&ts, &ts))
            ;
        /* NOTE: maybe prevent screen dead locking */
        static uint32_t test = 0;
        test++;
        if(unlikely(test > 50000)) {
            test = 0;
            break;
        }

        min = vpmu_find_min_dsp_time(vpmu);
    }
    sigprocmask(SIG_SETMASK, &osis, NULL);
    suspend = 0;
}
#endif

#if 0
void vpmu_dump_machine_state(void *opaque)
{
    static uint64_t last;
    VPMU *vpmu = (VPMU *)opaque;
    FILE *fp = vpmu->dump_fp;
    static uint8_t first = 0;
    static struct timeval t1, t2;
    unsigned long long time;
#if defined(CONFIG_PACDSP)
    pacdsp_state_t *pac;
    size_t i;
#endif

    if(likely(!VPMU_enabled)) {
        return;
    }
    if(unlikely(first == 0)) {
        last = vpmu_estimated_execution_time();
        gettimeofday(&t1, NULL);
        fprintf(fp,"A: EstimatedExecTime(us) CycleCount "
                "TotalInsnCount USR_InsnCount SVC_InsnCount IRQ_InsnCount REST_InsnCount "
                "TotalLDSTInsnCount USR_LDST_InsnCount SVC_LDST_InsnCount IRQ_LDST_InsnCount REST_LDST_InsnCount "
                "BranchInsnCount BranchPredictCorrect BranchPredictWrong AllChangeToPC "
                "D-CacheFetch D-CacheReadMiss D-CacheWriteMiss "
                "TotalDCacheMiss*32 D-Cache-ByteRead D-Cache-ByteWrite D-Cache-ByteRead+Write "
                "I-CacheFetch I-CacheMiss EmulationTime\n");
#if defined(CONFIG_PACDSP)
        fprintf(fp,"B: ExecTime(us) CycleCount ReadSize WriteSize BranchCycle\n");
#endif
        fflush(fp);
        first = 1;
    } else {
#if 0
        /* When the loading is heavy, this timer call back will be called frequently.
         *                  is low  ,                                     less frequently.
         * This behavior caused the period between call back function is almost the same
         */
        if(GlobalVPMU.dsp_running) {
            cnow = vpmu_estimated_execution_time();
            fprintf(stderr," diff = %lld",cnow - last);
            /* if ARM is likely to be idle */
            if((cnow - last) < 800) {
                uint64_t tmp[16] = {0};
                int i;
                uint64_t max = 0;
                FOREACH_PACDSP_I(pac, i) {
                    tmp[i] = paciss_state_get_tick(pac->pac);

                }
                for(i=0;i<16;i++) {
                    uint64_t dsp_time = tmp[i]/GlobalVPMU.target_cpu_frequency
                                + GlobalVPMU.dsp_start_time[i];
                    if(dsp_time > max) {
                        max = dsp_time;
                    }
                }
                /*fprintf(stderr,"max=%llu offset=%llu\n",max,
                        ((max/GlobalVPMU.target_cpu_frequency + GlobalVPMU.dsp_start_time) - vpmu_estimated_execution_time()));
                */
                /* if any DSP is running faster than ARM
                 * , add offset to ARM timing vpmu_cycle_count() */
                int64_t test = max - vpmu_estimated_execution_time();
                if(test > 0) {
                    vpmu->offset += test * GlobalVPMU.target_cpu_frequency;
                    fprintf(stderr,"arm is like to be idle, turn ARM timer forward\n");
                }
            }
            last = vpmu_estimated_execution_time();
        }
#endif
        gettimeofday(&t2, NULL);
        time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

#if 1
#if defined(CONFIG_PACDSP)
        /* if ARM is likely to be idle */
        if(GlobalVPMU.dsp_running) {
            uint64_t period = 0;
            static uint64_t last_time = 0;
            static char counter = 0;
            static char dump_counter = 0;
            period = time - last_time;
            last_time = time;
            //fprintf(stderr,"period = %llu",period);

            /* To SEE the MAX period */
            /*
            static uint64_t max=0;
            if(period < 200000)
                if(max<period) {
                    max=period;
                    fprintf(stderr,"max=%llu\n",max);
                }
                */

            /* if the vpmu_suspend_qemu() is working
             * don't tune time fast forward
             */
            if(suspend) {
                //fprintf(stderr,"VPMU is suspending, don't tune time forward");
                return;
            }
            /* if the timer period is too long
             * and happened 3 times consecutively
             * , ARM is likely to be idle */
            /* When DSP running, ARM idle
             * Tune time forward*/
            if(period > 200000) {
                if(counter < 1) {
                    counter++;
                } else {
                    uint64_t tmp[16] = {0};
                    uint64_t min = UINT64_MAX;
                    int i;
                    FOREACH_PACDSP_I(pac, i) {
                        tmp[i] = paciss_state_get_tick(pac->pac);
                    }
                    for(i=0;i<16;i++) {
                        if(tmp[i] == 0)
                            continue;
                        uint64_t dsp_time = tmp[i]/GlobalVPMU.target_cpu_frequency
                            + GlobalVPMU.dsp_start_time[i];
                        if(dsp_time < min) {
                            min = dsp_time;
                        }
                    }
                    fprintf(stderr,"min=%llu\n",min);
                    /* if any DSP is running faster than ARM
                     * , add offset to ARM timing vpmu_cycle_count() */
                    int64_t test = min - vpmu_estimated_execution_time();
                    if(test > 0) {
                        vpmu->offset += test * GlobalVPMU.target_cpu_frequency;
                        idle = 1;
                        fprintf(stderr,"\n!!!!!!!arm is likely to be idle, turn ARM timer forward\n");
                        fprintf(stderr,"(dump)qemu=%u dsp=%llu\n",vpmu_estimated_execution_time(),min);
                    }
                    counter = 0;
                }
            } else {
                /* When DSP running, ARM busy
                 * vpmu_suspend_qemu() will take place
                 * this caused the sample rate is relatively high than normal
                 * Trigger 1 dump / x count */
                dump_counter++;
                if(dump_counter < 0) {
                    return;
                } else {
                    dump_counter = 0;
                }
                counter = 0;
                idle = 0;
            }
        }
#endif
#endif
        fprintf(fp,"ARM: %u %llu "
                , vpmu_estimated_execution_time(), vpmu_cycle_count());
        fprintf(fp,"%llu %llu %llu %llu %llu "
                , vpmu_total_insn_count(), vpmu->USR_icount
                , vpmu->SVC_icount, vpmu->IRQ_icount, vpmu->sys_rest_icount);
        fprintf(fp,"%llu %llu %llu %llu %llu "
                , vpmu_total_ldst_count(), vpmu->USR_ldst_count
                , vpmu->SVC_ldst_count, vpmu->IRQ_ldst_count
                , vpmu->sys_rest_ldst_count);
        fprintf(fp,"%llu %u %u %llu "
                , vpmu_branch_insn_count(), vpmu_branch_predict(PREDICT_CORRECT)
                , vpmu_branch_predict(PREDICT_WRONG), vpmu->all_changes_to_PC_count);
        fprintf(fp,"%.0f %.0f %.0f "
                , vpmu_L1_dcache_access_count(), vpmu->dcache->miss[D4XREAD]
                , vpmu->dcache->miss[D4XWRITE]);
        fprintf(fp,"%.0f %.0f %.0f %.0f "
               ,(vpmu->dcache->miss[D4XREAD]+vpmu->dcache->miss[D4XWRITE])*32
               , vpmu->dcache->bytes_read, vpmu->dcache->bytes_written
               , vpmu->dcache->bytes_read+vpmu->dcache->bytes_written);
        fprintf(fp,"%.0f %.0f "
                , vpmu_icache_access_count(), vpmu->icache->miss[D4XINSTRN]);
        fprintf(fp,"%llu \n"
                , time);
#if defined(CONFIG_PACDSP)
        FOREACH_PACDSP_I(pac, i) {
            if(pac->worker_thread_running == 1) {
                fprintf(fp, "PACDSP%d: %llu %lld %lld %lld %u\n",
                    i + 1,
                    paciss_state_get_tick(pac->pac)/GlobalVPMU.target_cpu_frequency 
                        + GlobalVPMU.dsp_start_time[i],
                    paciss_state_get_tick(pac->pac),
                    paciss_state_get_ext_read(pac->pac),
                    paciss_state_get_ext_write(pac->pac),
                    pac->pac->status.branch_cycle);
            }
        }
#endif
        fflush(fp);
    }
}
#endif

/*-------------------------------------
 * VPMU for Oprofile feature.
 * This function is invoked whenever "host_alarm_handler()"
 * in vl.c is called.
 *
 * This function checks if the "virtualized" PMN counters are overflowed.
 * If so, it will set up FLAG register and raise interrupt.
 * IMPORTANT: Note that we currently handles PMN0.
 *
 * Descirption of the behaviors of PMN0-3 counters.
 * When an event counter reaches its maximum value
 * 0xFFFFFFFF, the next event it needs to count causes it to roll over to zero
 * and set its corresponding overflow flag (bit 1, 2, 3 or 4) in FLAG. An
 * interrupt request is generated when its corresponding interrupt enable
 * (bit 1, 2, 3 or 4) is set in INTEN.
 *
 * For more detail of Performance Monitoring Registers please refer to: Ch. 11 in
 * "3rd Generation Intel XScale Microarchitecture Developer’s Manual_31628302.pdf".
 *
 *-------------------------------------*/
//#include <time.h>
void vpmu_perf_counter_overflow_testing (void *opaque)
{
    struct      cp14_pmu_state* cp14_pmu_pt = (struct cp14_pmu_state*) opaque;
    /* Mask to check the E bit in PMNC register. */
    const uint32_t PMNC_Ebit_MASK           = 0x00000001;
    /* Mask to select the mask for PMN0 register. */
    const uint32_t PMN0_MASK                = 0x00000002;
    /*
     * cnt_value keeps the reset value set by Oprofile User.
     * I.e., the value that user specified (inteval) when starts Oprofile.
     * reset_value keeps the snapshot global VPMU value, which is the baseline
     * for counting the performance result of the program.
     * cur_value always gets the latest value from global VPMU.
     *
     * The simulated counter is called "virtual" counter.
     * The value of "virtualized" counter = cur_value - reset_value + cnt_value.
     */
    uint32_t    cnt_value           = 0,
                reset_value         = 0,
                cur_value           = 0,
                vir_cnt_value       = 0,
                vpmu_value_inteval  = 0,
                vir_value_interval  = 0;

    /* Obtaining the latest values. */
    cnt_value           = cp14_pmu_pt->pmn[0];
    reset_value         = cp14_pmu_pt->pmn_reset_value[0];
    cur_value           = vpmu_hardware_event_select(cp14_pmu_pt->evtsel,
                                             cp14_pmu_pt->vpmu_t);
    /* Calculate the content of the "virtual" counter seen by Oprofile.
     * Currently, the variable is deprecated.
     */
    vir_cnt_value       = cur_value - reset_value + cnt_value;

    /* Calculate the differences observed by the Global VPMU counter. */
    vpmu_value_inteval   = cur_value - reset_value;
    /* Calculate the expected value, specified by the user. */
    vir_value_interval  = ((uint32_t)0xFFFFFFFF - cnt_value);

    /* Check if the "virtual" counter is overflowed. */
    if ( vpmu_value_inteval > vir_value_interval )
    {
        //spin_lock(vpmu_ovrflw_chk_lock);
        /* If the corresponding bit is set in INTEN resiter,
         * raise the interrupt after set up the FLAG register.
         * Check if the interrupt of PMN0 register is enabled in
         * Interrupt Enable Register (INTEN) && check if the PMN counters are activated.
         */
        if ( (cp14_pmu_pt->inten & PMN0_MASK) && (cp14_pmu_pt->pmnc & PMNC_Ebit_MASK) )
        {
            /* When the counter is overflowed, set up the overflow flag (0x00000010)
             * in FLAG register. Currently, only PMN0 is monitored.
             */
            cp14_pmu_pt->flag |= PMN0_MASK;
            qemu_irq_raise(QLIST_FIRST(&vpmu_t.head)->irq);

            SIMPLE_DBG("%s: Interrupt triggered by PMN0 overflow.\n", __FUNCTION__);
        }

        /* Reset cnt_value to current global VPMU data. */
        cp14_pmu_pt->pmn_reset_value[0] = cur_value;
        //spin_unlock(vpmu_ovrflw_chk_lock);

        SIMPLE_DBG("\tPMN0 Overflowed. VPMU experienced offset=%u, interested interval=%u.\n", \
                   vpmu_value_inteval, vir_value_interval);
    }


    /* Used to calculate the frequency of this function to be called.
     * As it turns out, on the Ubuntu 9.10 with Intel(R) Core(TM) i7 CPU 920  @ 2.67GHz,
     * the frequency is about 0.10 second.
     */
    //static uint32_t time_cnt=0;
    //time_t t1=time(NULL);
    //printf("%d seconds elapsed, counter=%u\n", t1, time_cnt++);
}

/*-------------------------------------
 * VPMU for Oprofile feature.
 * This function is invoked after OProfile done with handling the
 * performance counter overflow interrupt.
 *
 * This function is invoked in pxa2xx_perf_write(), when Oprofile
 * tries to write clear the FLAG bits.
 *
 *
 *------------------------------------*/
void vpmu_perf_counter_overflow_handled (void *opaque)
{
    struct      cp14_pmu_state* cp14_pmu_pt = (struct cp14_pmu_state*) opaque;
    /* Mask to select the mask for PMN0 register. */
    const uint32_t PMN0_MASK                = 0x00000002;

    /* Pull down the interrupt for the PMN0 when ...
     * the corresponding bit is set in INTEN resiter and
     * the corresponding bit is cleared in FLAG register.
     */
    if ( (cp14_pmu_pt->inten & PMN0_MASK) && !(cp14_pmu_pt->flag & PMN0_MASK) )
    {
        qemu_irq_lower(QLIST_FIRST(&vpmu_t.head)->irq);

        SIMPLE_DBG("%s: PMN0 overflow interrupt has been serviced.\n", __FUNCTION__);
    }

}

void vpmu_dump_machine_state_v2(void *opaque)
{
    static uint64_t last_vpmu_time;
    VPMU *vpmu = (VPMU *)opaque;
    FILE *fp = vpmu->dump_fp;
    static uint8_t first = 0;
    static struct timeval t1, t2;
    unsigned long long emu_time;
#if defined(CONFIG_PACDSP)
    pacdsp_state_t *pac;
    size_t i;
#endif

    if(!VPMU_enabled) {
        return;
    }
    if(unlikely(first == 0)) {
        last_vpmu_time = vpmu_estimated_execution_time();
        gettimeofday(&t1, NULL);
        fprintf(fp,"A: EstimatedExecTime(us) CycleCount "
                "TotalInsnCount USR_InsnCount SVC_InsnCount IRQ_InsnCount REST_InsnCount "
                "TotalLDSTInsnCount USR_LDST_InsnCount SVC_LDST_InsnCount IRQ_LDST_InsnCount REST_LDST_InsnCount "
                "BranchInsnCount BranchPredictCorrect BranchPredictWrong AllChangeToPC "
                "D-CacheFetch D-CacheReadMiss D-CacheWriteMiss "
                "TotalDCacheMiss*32 D-Cache-ByteRead D-Cache-ByteWrite D-Cache-ByteRead+Write "
                "I-CacheFetch I-CacheMiss EmulationTime MPU-PowerConsumption(uJ)\n");
#if defined(CONFIG_PACDSP)
        fprintf(fp,"B: ExecTime(us) CycleCount ExtReadSize ExtWriteSize LDMReadSize LDMWriteSize InstReadSize BranchCycle DSP-PowerConsumption(uJ)\n");
#endif
        fflush(fp);
        first = 1;
    } else {
        gettimeofday(&t2, NULL);
        emu_time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

#if defined(CONFIG_PACDSP)
#if 0
        /* DON'T TUNE ARM time forward, This code is useless */
        /* if ARM is likely to be idle */
        if(GlobalVPMU.dsp_running) {
            static char counter = 0;
            /* if the vpmu_suspend_qemu() is working
             * don't tune time fast forward
             */
            if(suspend) {
                //fprintf(stderr,"VPMU is suspending, don't tune time forward");
                //return;
                goto DUMP;
            }
            /* if the timer period is too long
             * and happened 3 times consecutively
             * , ARM is likely to be idle */
            /* When DSP running, ARM idle
             * Tune time forward*/
            if(counter > 3) {
                uint64_t min = UINT64_MAX;
                min = vpmu_find_min_dsp_time(vpmu);
                //fprintf(stderr,"min=%llu vpmu=%lu\n",min, vpmu_estimated_execution_time());
                /* if any DSP is running faster than ARM
                 * , add offset to ARM timing vpmu_cycle_count() */
                int64_t test = min - vpmu_estimated_execution_time();
                if(test > 100) {
                    if(test > 750)
                        vpmu->offset += 750 * GlobalVPMU.target_cpu_frequency;
                    else
                        vpmu->offset += test * GlobalVPMU.target_cpu_frequency;
                    //fprintf(stderr,"\n!!!!!!!arm is likely to be idle, turn ARM timer forward\n");
                    //fprintf(stderr,"(dump)qemu=%u dsp=%llu\n",vpmu_estimated_execution_time(),min);
                }
                counter = 0;
            } else {
                /* When DSP running, ARM busy
                 * vpmu_suspend_qemu() will take place
                 * this caused the sample rate is relatively high than normal
                 * Trigger 1 dump / x count */
                counter ++;
            }
        }
#endif
#endif
        static int dsp_count = 0, arm_count = 0;
        if(GlobalVPMU.dsp_running) {
            dsp_count++;
            if(dsp_count == 6) {
                dsp_count = 0;
                goto DUMP;
            } else {
                return;
            }
        } else {
#if 0
            static uint64_t current_vpmu_time;
            /* dump period */
            /* 1500 seems to be better (when arm is idle, 1500 per timer trigger */
            current_vpmu_time = vpmu_estimated_execution_time();
            if((current_vpmu_time - last_vpmu_time) > 700) {
                last_vpmu_time = current_vpmu_time;
            } else {
                return;
            }
#else
            arm_count++;
            if(arm_count == 3) {
                arm_count = 0;
                goto DUMP;
            } else {
                return;
            }
#endif
        }
DUMP:
        fprintf(fp,"ARM: %u %llu "
                , vpmu_estimated_execution_time(), vpmu_cycle_count());
        fprintf(fp,"%llu %llu %llu %llu %llu "
                , vpmu_total_insn_count(), vpmu->USR_icount
                , vpmu->SVC_icount, vpmu->IRQ_icount, vpmu->sys_rest_icount);
        fprintf(fp,"%llu %llu %llu %llu %llu "
                , vpmu_total_ldst_count(), vpmu->USR_ldst_count
                , vpmu->SVC_ldst_count, vpmu->IRQ_ldst_count
                , vpmu->sys_rest_ldst_count);
        fprintf(fp,"%llu %u %u %llu "
                , vpmu_branch_insn_count(), vpmu_branch_predict(PREDICT_CORRECT)
                , vpmu_branch_predict(PREDICT_WRONG), vpmu->all_changes_to_PC_count);
        fprintf(fp,"%.0f %.0f %.0f "
                , vpmu_L1_dcache_access_count(), vpmu->dcache->miss[D4XREAD]
                , vpmu->dcache->miss[D4XWRITE]);
        fprintf(fp,"%.0f %.0f %.0f %.0f "
               ,(vpmu->dcache->miss[D4XREAD]+vpmu->dcache->miss[D4XWRITE])*32
               , vpmu->dcache->bytes_read, vpmu->dcache->bytes_written
               , vpmu->dcache->bytes_read+vpmu->dcache->bytes_written);
        fprintf(fp,"%.0f %.0f "
                , vpmu_L1_icache_access_count(), vpmu->icache->miss[D4XINSTRN]);
        fprintf(fp,"%llu "
                , emu_time);
        fprintf(fp,"%d\n"
                , vpmu_vpd_mpu_power());
#if defined(CONFIG_PACDSP)
        FOREACH_PACDSP_I(pac, i) {
            if(pac->worker_thread_running == 1) {
                int dsp_freq;
                uint64_t pac_time  = paciss_state_get_tick(pac->pac);
                uint64_t ext_read  = paciss_state_get_ext_read_sz(pac->pac);
                uint64_t ext_write = paciss_state_get_ext_write_sz(pac->pac);
                uint64_t ldm_read  = paciss_state_get_ldm_read_sz(pac->pac);
                uint64_t ldm_write = paciss_state_get_ldm_write_sz(pac->pac);
                uint64_t inst_read = paciss_state_get_inst_read_sz(pac->pac);
                uint32_t branch_cycle = pac->pac->status.branch_cycle;

                dsp_freq = pacduo_dsp_freq(i);

                fprintf(fp, "PACDSP%d: %llu %lld %lld %lld %lld %lld %lld %u ",
                    i + 1,
                    pac_time/dsp_freq
                        + GlobalVPMU.dsp_start_time[i],
                    pac_time,
                    ext_read, ext_write,
                    ldm_read, ldm_write,
                    inst_read,
                    branch_cycle);
                if(i == 0)
                    fprintf(fp, " %d", vpmu_vpd_dsp1_power());
                else if(i == 1)
                    fprintf(fp, " %d", vpmu_vpd_dsp2_power());
                else
                    fprintf(fp, " 0");
                fprintf(fp, "\n");
            }
        }
#endif
        fflush(fp);
    }
}

#if defined(CONFIG_PACDSP)
#if 0
uint32_t vpmu_dsp_considered_execution_time(void)
{
    static uint32_t nowFetch=1,nowMiss=1,nowTick=1;
    static uint32_t time=0;
    nowFetch=GlobalVPMU.dcache->fetch[D4XREAD]+ GlobalVPMU.dcache->fetch[D4XWRITE];
    nowMiss=GlobalVPMU.dcache->miss[D4XREAD]+GlobalVPMU.dcache->miss[D4XWRITE]+GlobalVPMU.icache->miss[D4XINSTRN];
    nowTick=GlobalVPMU.ticks;

    uint32_t vpmu_dsp_time=vpmu_side_dsp_exec_time();
    uint32_t dsp_real_time=dsp_real_exec_time(GlobalVPMU.dsp_frequency);
    if(vpmu_dsp_time < dsp_real_time ) {
        time=(int)((nowFetch*14.5+nowMiss*360) / 1000) + (int)(nowTick/520)
            -vpmu_dsp_time+dsp_real_time;
    }
    else {
        time=(int)((nowFetch*14.5+nowMiss*360) / 1000) + (int)(nowTick/520);
    }
    return time;
}
#endif

/* deprecated */
uint32_t vpmu_dsp_considered_execution_time(void)
{
#if 0
    deprecated
    uint64_t nowMiss=1;
    uint32_t time=0;
    double nowFetch=0;
    uint64_t nowTick=0;

    nowFetch = vpmu_dcache_access_count() + vpmu_icache_access_count();
    nowMiss = vpmu_dcache_miss_count() + vpmu_icache_miss_count();
    nowTick = vpmu_cycle_count();

    uint32_t vpmu_dsp_time = vpmu_side_dsp_exec_time();
    uint32_t dsp_real_time = dsp_real_exec_time(GlobalVPMU.dsp_frequency);
    if(vpmu_dsp_time < dsp_real_time ) {
        time = (uint32_t)(
            (nowFetch*TARGET_HIT_LATENCY + (double)nowMiss*TARGET_MISS_LATENCY)/1000
            + (double)nowTick/GlobalVPMU.target_cpu_frequency
            - (double)vpmu_dsp_time+dsp_real_time );
    }
    else {
        time = (uint32_t)(
            (nowFetch*TARGET_HIT_LATENCY + (double)nowMiss*TARGET_MISS_LATENCY)/1000 + (double)nowTick/GlobalVPMU.target_cpu_frequency) ;
    }
    return time;
#endif
}
#endif

void vpmu_update_eet_epc(void)
{
    uint64_t time = 0;

    switch(GlobalVPMU.timing_register & (0xf<<28))
    {
        case TIMING_MODEL_A:
            time = (vpmu_total_insn_count() - GlobalVPMU.last_ticks) * TARGET_AVG_CPI
                / (GlobalVPMU.target_cpu_frequency / 1000.0);
            GlobalVPMU.last_ticks = vpmu_total_insn_count();
            break;
        case TIMING_MODEL_B:
        case TIMING_MODEL_C:
        case TIMING_MODEL_D:
        case TIMING_MODEL_E:
		//evo0209
		case TIMING_MODEL_F:
		case TIMING_MODEL_G:
            time = (vpmu_cycle_count() - GlobalVPMU.last_ticks)
                / (GlobalVPMU.target_cpu_frequency / 1000.0);
            GlobalVPMU.last_ticks = vpmu_cycle_count();

			//evo0209 : for debug
			//printf("vpmu_cycle_count: %llu, total_cycle: %llu\n",
			//		vpmu_cycle_count(),
			//		vpmu_estimated_pipeline_execution_time_ns() +
			//		vpmu_estimated_io_memory_access_time_ns() +
			//		vpmu_estimated_sys_memory_access_time_ns());
			//printf("pipeline %llu, miss %llu, io %llu\n",
			//		vpmu_pipeline_cycle_count(),
			//		vpmu_sys_memory_access_cycle_count(),
			//		vpmu_io_memory_access_cycle_count());
            break;
        default:
            fprintf(stderr,"Wrong Model Number\n");
            break;
    }
	//evo0209 : [Bug] the eet_ns is not match the sum of total pipeline + mem + io
    //GlobalVPMU.eet_ns += time;
    GlobalVPMU.epc_mpu += time * GlobalVPMU.mpu_power / 1000000.0;
}

//chiaheng
uint64_t vpmu_estimated_pipeline_execution_time_ns (void)
{
	/* FIXME...
	 * Use last_pipeline_cycle_count to save tmp data
	 * as did above by using GlobalVPMU.last_ticks.
	 */
	return vpmu_pipeline_cycle_count() / (GlobalVPMU.target_cpu_frequency / 1000.0);
}

//chiaheng
uint64_t vpmu_estimated_io_memory_access_time_ns (void)
{
	/* FIXME...
	 * Use last_pipeline_cycle_count to save tmp data
	 * as did above by using GlobalVPMU.last_ticks.
	 */
	return vpmu_io_memory_access_cycle_count() / (GlobalVPMU.target_cpu_frequency / 1000.0);
}

//chiaheng
uint64_t vpmu_estimated_sys_memory_access_time_ns (void)
{
	/* FIXME...
	 * Use last_pipeline_cycle_count to save tmp data
	 * as did above by using GlobalVPMU.last_ticks.
	 */
	return vpmu_sys_memory_access_cycle_count() / (GlobalVPMU.target_cpu_frequency / 1000.0);
}

//converse2006
uint64_t vpmu_estimated_netsend_execution_time_ns()
{
    return GlobalVPMU.netsend_qemu;
    //return (GlobalVPMU.netsend_qemu / (GlobalVPMU.target_cpu_frequency / 1000.0));
}

//converse2006
uint64_t vpmu_estimated_netrecv_execution_time_ns()
{
    return GlobalVPMU.netrecv_qemu;
    //return (GlobalVPMU.netrecv_qemu / (GlobalVPMU.target_cpu_frequency / 1000.0));
}

//converse2006
uint64_t vpmu_estimated_netsend_count()
{
    return GlobalVPMU.netsend_count;
}

//converse2006
uint64_t vpmu_estimated_netrecv_count()
{
    return GlobalVPMU.netrecv_count;
}

//converse2006
uint64_t vpmu_estimated_network_execution_time_ns()
{
    return (vpmu_estimated_netsend_execution_time_ns() + vpmu_estimated_netrecv_execution_time_ns());
}

uint64_t vpmu_estimated_execution_time_ns(void)
{
	//evo0209 : [Bug] the eet_ns is not match the sum of total pipeline + mem + io
    //vpmu_update_eet_epc();
	//return GlobalVPMU.eet_ns;
    
	return vpmu_estimated_pipeline_execution_time_ns() +
			vpmu_estimated_io_memory_access_time_ns() +
            vpmu_estimated_network_execution_time_ns() + //converse2006 added
			vpmu_estimated_sys_memory_access_time_ns();
}

/* return micro second */
uint32_t vpmu_estimated_execution_time(void)
{
    vpmu_update_eet_epc();
    return GlobalVPMU.eet_ns / 1000;
}

//chiaheng
uint32_t vpmu_hardware_event_select(uint32_t evtsel, VPMU* vpmu)
{
    /* use vpmu wrapping function as many as possible
     * to avoid sync problem
     */
    switch(evtsel)
    {
        /* Instruction Miss Count event 0x00
         * 不知為何perfex -e 0 會使得s->evtsel變成-1*/
        case -1:
        case 0:
            return (uint32_t)vpmu_total_insn_count();
        case 0x01:
            return (uint32_t)vpmu->USR_icount;
        case 0x02:
            return (uint32_t)vpmu->SVC_icount;
        case 0x03:
            return (uint32_t)vpmu->IRQ_icount;
        case 0x04:
            return (uint32_t)vpmu->sys_rest_icount;

        case 0x05:
            return (uint32_t)vpmu_total_ldst_count();
        case 0x06:
            return (uint32_t)vpmu->USR_ldst_count;
        case 0x07:
            return (uint32_t)vpmu->SVC_ldst_count;
        case 0x08:
            return (uint32_t)vpmu->IRQ_ldst_count;
        case 0x09:
            return (uint32_t)vpmu->sys_rest_ldst_count;

        case 0x0a:
            return (uint32_t)vpmu_total_load_count();
        case 0x0b:
            return (uint32_t)vpmu->USR_load_count;
        case 0x0c:
            return (uint32_t)vpmu->SVC_load_count;
        case 0x0d:
            return (uint32_t)vpmu->IRQ_load_count;
        case 0x0e:
            return (uint32_t)vpmu->sys_rest_load_count;

        case 0x0f:
            return (uint32_t)vpmu_total_store_count();
        case 0x10:
            return (uint32_t)vpmu->USR_store_count;
        case 0x11:
            return (uint32_t)vpmu->SVC_store_count;
        case 0x12:
            return (uint32_t)vpmu->IRQ_store_count;
        case 0x13:
            return (uint32_t)vpmu->sys_rest_store_count;
        case 0x14:
            return (uint32_t)vpmu->all_changes_to_PC_count;

        case 0x20:
            return (uint32_t)vpmu_branch_insn_count();
        case 0x21:
            return vpmu_branch_predict(PREDICT_CORRECT);
        case 0x22:
            return vpmu_branch_predict(PREDICT_WRONG);

        case 0x30:
            return (uint32_t)vpmu_L1_dcache_access_count();
        case 0x31:
            return (uint32_t)vpmu->dcache->miss[D4XWRITE];
        case 0x32:
            return (uint32_t)vpmu->dcache->miss[D4XREAD];
        case 0x33:
            return (uint32_t)vpmu->dcache->miss[D4XREAD] + vpmu->dcache->miss[D4XWRITE];
        case 0x34:
            return (uint32_t)vpmu_L1_icache_access_count();
        case 0x35:
            return (uint32_t)vpmu->icache->miss[D4XINSTRN];

        /* Memory events. */
        case 0x40:
            return (uint32_t)(vpmu_L1_dcache_miss_count() + vpmu_L1_icache_miss_count());
        case 0x41:
            return (uint32_t)vpmu->iomem_count;

        /* estimate execution time */
        case 0x50:
            return (uint32_t)vpmu_cycle_count();
        case 0x51:
            return (uint32_t)vpmu_estimated_execution_time_ns();
        case 0x52:
            return (uint32_t)vpmu_pipeline_cycle_count();
        case 0x53:
            return (uint32_t)vpmu_estimated_pipeline_execution_time_ns();
        case 0x54:
            return (uint32_t)vpmu_sys_memory_access_cycle_count();
        case 0x55:
            return (uint32_t)vpmu_estimated_sys_memory_access_time_ns();
        case 0x56:
            return (uint32_t)vpmu_io_memory_access_cycle_count();
        case 0x57:
            return (uint32_t)vpmu_estimated_io_memory_access_time_ns();

        /* Virtual Power Device events, added by gkk886. */
        case 0xf0:
            return vpmu_vpd_mpu_power(); //uJ or mW
        case 0xf1:
            return vpmu_vpd_dsp1_power(); //uJ
        case 0xf2:
            return vpmu_vpd_dsp2_power(); //uJ
        case 0xf3:
            return vpmu_vpd_lcd_power(); //uJ or mW
        case 0xf4:
            return vpmu_vpd_audio_power(); //uJ or mW
        case 0xf5:
            return vpmu_vpd_net_power(); //uJ or mW
        case 0xf6:
            return vpmu_vpd_gps_power(); //uJ or mW
        case 0xff:
            return vpmu_vpd_total_power(); //uJ or mW

        default:
            fprintf(stderr,"(VPMU): No such hardware event (%s return 0)\n", __FUNCTION__);
    }
    return 0;
}
//uint32_t vpmu_hardware_event_select(uint32_t evtsel, VPMU* vpmu)
//{
//    /* use vpmu wrapping function as many as possible
//     * to avoid sync problem
//     */
//    switch(evtsel)
//    {
//        /* Instruction Miss Count event 0x00
//         * 不知為何perfex -e 0 會使得s->evtsel變成-1*/
//        case -1:
//        case 0x00:
//            return (uint32_t)vpmu_total_insn_count();
//        case 0x01:
//            return (uint32_t)vpmu->USR_icount;
//        case 0x02:
//            return (uint32_t)vpmu->SVC_icount;
//        case 0x03:
//            return (uint32_t)vpmu->IRQ_icount;
//        case 0x04:
//            return (uint32_t)vpmu->sys_rest_icount;
//        case 0x05:
//            return (uint32_t)vpmu_total_ldst_count();
//        case 0x06:
//            return (uint32_t)vpmu->USR_ldst_count;
//        case 0x07:
//            return (uint32_t)vpmu->SVC_ldst_count;
//        case 0x08:
//            return (uint32_t)vpmu->IRQ_ldst_count;
//        case 0x09:
//            return (uint32_t)vpmu->sys_rest_ldst_count;
//        case 0x0a:
//            return (uint32_t)vpmu->all_changes_to_PC_count;
//        case 0x0b:
//            return (uint32_t)vpmu_branch_insn_count();
//        case 0x0c:
//            return vpmu_branch_predict(PREDICT_CORRECT);
//        case 0x0d:
//            return vpmu_branch_predict(PREDICT_WRONG);
//
//        case 0x0e:
//            return (uint32_t)vpmu->sim[0][0]();
//        case 0x0f:
//            return (uint32_t)vpmu->sim[0][1]();
//        case 0x10:
//            return (uint32_t)vpmu->sim[0][2]();
//        case 0x11:
//            return (uint32_t)vpmu->sim[0][3]();
//        case 0x12:
//            return (uint32_t)vpmu->sim[0][4]();
//
//        case 0x13:
//            return (uint32_t)vpmu->sim[1][0]();
//        case 0x14:
//            return (uint32_t)vpmu->sim[1][1]();
//        case 0x15:
//            return (uint32_t)vpmu->sim[1][2]();
//        case 0x16:
//            return (uint32_t)vpmu->sim[1][3]();
//        case 0x17:
//            return (uint32_t)vpmu->sim[1][4]();
//
//        case 0x18:
//            return (uint32_t)vpmu->sim[2][0]();
//        case 0x19:
//            return (uint32_t)vpmu->sim[2][1]();
//        case 0x1a:
//            return (uint32_t)vpmu->sim[2][2]();
//        case 0x1b:
//            return (uint32_t)vpmu->sim[2][3]();
//        case 0x1c:
//            return (uint32_t)vpmu->sim[2][4]();
//
//        case 0x1d:
//            return (uint32_t)vpmu->sim[3][0]();
//        case 0x1e:
//            return (uint32_t)vpmu->sim[3][1]();
//        case 0x1f:
//            return (uint32_t)vpmu->sim[3][2]();
//        case 0x20:
//            return (uint32_t)vpmu->sim[3][3]();
//        case 0x21:
//            return (uint32_t)vpmu->sim[3][4]();
//
//        case 0x30:
//            return (uint32_t)vpmu_cycle_count();
//            /*
//        case 0x0e:
//            return (uint32_t)vpmu_dcache_access_count();
//        case 0x0f:
//            return (uint32_t)vpmu_dcache_miss_count();
//        case 0x10:
//            return (uint32_t)vpmu_dcache_read_miss_count();
//        case 0x11:
//            return (uint32_t)vpmu_dcache_write_miss_count();
//        case 0x13:
//            return (uint32_t)vpmu_icache_access_count();
//        case 0x14:
//            return (uint32_t)vpmu_icache_miss_count();
//        case 0x15:
//            return (uint32_t)vpmu_cycle_count();
//            */
//
//        /* estimate execution time */
//        case 0xFFFFF:
//            return vpmu_estimated_execution_time();
//            return (uint32_t)vpmu_estimated_execution_time_ns();
//
//        /* Virtual Power Device events, added by gkk886. */
//        case 0xf0:
//            return vpmu_vpd_mpu_power(); //uJ or mW
//        case 0xf1:
//            return vpmu_vpd_dsp1_power(); //uJ
//        case 0xf2:
//            return vpmu_vpd_dsp2_power(); //uJ
//        case 0xf3:
//            return vpmu_vpd_lcd_power(); //uJ or mW
//        case 0xf4:
//            return vpmu_vpd_audio_power(); //uJ or mW
//        case 0xf5:
//            return vpmu_vpd_net_power(); //uJ or mW
//        case 0xf6:
//            return vpmu_vpd_gps_power(); //uJ or mW
//        case 0xff:
//            return vpmu_vpd_total_power(); //uJ or mW
//        default:
//            fprintf(stderr,"(VPMU): No such hardware event (%s return 0)\n", __FUNCTION__);
//    }
//    return 0;
//}

//evo0209 fixed
void VPMU_update_slowdown_ratio(int value)
{
	static count = 0;
#ifdef CONFIG_PACDSP
    GlobalVPMU.slowdown_ratio = ((pacduo_pregs.PLL1_CR>>24)-0x0b)*0.229341+2.683357; //linear regression
#else
	if (count == 0)
	{
	    GlobalVPMU.slowdown_ratio = value;
		count++;
	}
	//GlobalVPMU.slowdown_ratio = 7.0; //for FR.t43.m2.arm926
	//GlobalVPMU.slowdown_ratio = 9.0; //for FRJNI.t43.m2.arm926
	//GlobalVPMU.slowdown_ratio = 32.0; //for FR.t43.m2.cortex-a9
	//GlobalVPMU.slowdown_ratio = 52.0; //for FRJNI.t43.m2.cortex-a9
#endif
}

/* turn "on" simulator don't turn simulator off */
void vpmu_simulator_turn_on(VPMU* vpmu, uint32_t mask)
{
    vpmu->timing_register |= mask;
#ifdef CONFIG_VPMU_THREAD
    if(mask & VPMU_BRANCH_SIM)
        pthread_cond_signal(&branch_cond);
    if(mask & (VPMU_DCACHE_SIM | VPMU_ICACHE_SIM))
        pthread_cond_signal(&cache_cond);
#endif
}

void vpmu_model_setup(VPMU* vpmu, uint32_t model)
{
    vpmu->timing_register = model;
    switch(model)
    {
        case TIMING_MODEL_A:
            vpmu_simulator_turn_on(vpmu, VPMU_INSN_COUNT_SIM);
            break;
        case TIMING_MODEL_B:
            vpmu_simulator_turn_on(vpmu, VPMU_PIPELINE_SIM);
            break;
        case TIMING_MODEL_C:
            vpmu_simulator_turn_on(vpmu, VPMU_PIPELINE_SIM | VPMU_DCACHE_SIM);
            break;
        case TIMING_MODEL_D:
            vpmu_simulator_turn_on(vpmu, VPMU_PIPELINE_SIM | VPMU_DCACHE_SIM
                    | VPMU_ICACHE_SIM);
            break;
        case TIMING_MODEL_E:
            vpmu_simulator_turn_on(vpmu, VPMU_INSN_COUNT_SIM | VPMU_DCACHE_SIM
                    | VPMU_ICACHE_SIM | VPMU_BRANCH_SIM | VPMU_PIPELINE_SIM);
            break;
		//evo0209 : fixed for SET plus insn_count_sim
        case TIMING_MODEL_F:
            vpmu_simulator_turn_on(vpmu, VPMU_INSN_COUNT_SIM | VPMU_PIPELINE_SIM);
            break;
        case TIMING_MODEL_G:
            vpmu_simulator_turn_on(vpmu, VPMU_INSN_COUNT_SIM | VPMU_DCACHE_SIM
                    | VPMU_ICACHE_SIM | VPMU_PIPELINE_SIM);
            break;
        default:
            fprintf(stderr,"function \"%s\" : nothing to setup\n",__FUNCTION__);
            break;
    }
	
	//evo0209
	gettimeofday( &t1, NULL );
}

/* opt == 0 (print all the status) return value is 0
 * opt != 0 (return whether the specific simulator is on
 * return 1(on) 0(off) */
int8_t vpmu_simulator_status(VPMU* vpmu, uint32_t opt)
{
    unsigned int ctrl_reg = vpmu->timing_register;
    if(opt == 0) {
        ctrl_reg & VPMU_INSN_COUNT_SIM ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"Instruction Simulation\n");

        ctrl_reg & VPMU_DCACHE_SIM ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"Data Cache Simulation\n");

        ctrl_reg & VPMU_ICACHE_SIM ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"Insn Cache Simulation\n");

        ctrl_reg & VPMU_BRANCH_SIM ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"Branch Predictor Simulation\n");

        ctrl_reg & VPMU_PIPELINE_SIM ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"Pipeline Simulation\n");

        ctrl_reg & VPMU_SAMPLING ?
            fprintf(stderr,"○ : "):fprintf(stderr,"x : ");
        fprintf(stderr,"VPMU sampling mechanism\n");
        return 0;
    }
    else {
        switch(opt)
        {
            case VPMU_INSN_COUNT_SIM:
                if(ctrl_reg & VPMU_INSN_COUNT_SIM)
                    return 1;
                else
                    return 0;
            case VPMU_DCACHE_SIM:
                if(ctrl_reg & VPMU_DCACHE_SIM)
                    return 1;
                else
                    return 0;
            case VPMU_ICACHE_SIM:
                if(ctrl_reg & VPMU_ICACHE_SIM)
                    return 1;
                else
                    return 0;
            case VPMU_BRANCH_SIM:
                if(ctrl_reg & VPMU_BRANCH_SIM)
                    return 1;
                else
                    return 0;
            case VPMU_PIPELINE_SIM:
                if(ctrl_reg & VPMU_PIPELINE_SIM)
                    return 1;
                else
                    return 0;
            case VPMU_SAMPLING:
                if(ctrl_reg & VPMU_SAMPLING)
                    return 1;
                else
                    return 0;
            default:
                fprintf(stderr,"(VPMU) No such simulator\n");
                return -1;
        }
    }
}

//evo0209
int lev, idu;
static inline void VPMU_set_config(int config_id, char* value)
{
	switch (config_id)
	{
		case 0:
			GlobalVPMU.target_cpu_frequency = strtol(value, NULL, 10);
			break;
		case 1:
			GlobalVPMU.l1_cache_miss_lat = atof(value) * GlobalVPMU.target_cpu_frequency / 1000;
			break;
		case 2:
			GlobalVPMU.l2_cache_miss_lat = atof(value) * GlobalVPMU.target_cpu_frequency / 1000;
			break;
		case 24:
		case 3:
			lev = 0;
			GlobalVPMU.d4_maxlevel = strtol(value, NULL, 10);
			if (config_id >= 22)
				idu = 2;
			else
				idu = 1;
			break;
		case 35:
		case 25:
		case 14:
		case 4:
			GlobalVPMU.d4_level_size[idu][lev] = strtol(value, NULL, 10);
			break;
		case 36:
		case 26:
		case 15:
		case 5:
			GlobalVPMU.d4_level_blocksize[idu][lev] = strtol(value, NULL, 10);
			break;
		case 37:
		case 27:
		case 16:
		case 6:
			GlobalVPMU.d4_level_subblocksize[idu][lev] = strtol(value, NULL, 10);
			break;
		case 38:
		case 28:
		case 17:
		case 7:
			GlobalVPMU.d4_level_assoc[idu][lev] = strtol(value, NULL, 10);
			break;
		case 39:
		case 29:
		case 18:
		case 8:
			GlobalVPMU.d4_level_replacement[idu][lev] = value[0];
			break;
		case 40:
		case 30:
		case 19:
		case 9:
			GlobalVPMU.d4_level_walloc[idu][lev] = value[0];
			break;
		case 41:
		case 31:
		case 20:
		case 10:
			GlobalVPMU.d4_level_wback[idu][lev] = value[0];
			break;
		case 42:
		case 32:
		case 21:
		case 11:
			GlobalVPMU.d4_level_fetch[idu][lev] = value[0];
			break;
		case 43:
		case 33:
		case 22:
		case 12:
			GlobalVPMU.d4_level_prefetch_abortpercent[idu][lev] = strtol(value, NULL, 10);
			break;
		case 44:
		case 34:
		case 23:
		case 13:
			GlobalVPMU.d4_level_prefetch_distance[idu][lev] = strtol(value, NULL, 10);
			lev++;
			if (lev == 1)
				idu = 0;
			break;
		default:
			arm_instr_time[config_id - 45] = strtol(value, NULL, 10);
			break;
	}
}

/*
 * Implement by evo0209
 * Set VPMU configuration depend on config file in vpmu_config/
 */
static inline void VPMU_set_config_from_file(void)
{
	FILE *config_file;
	if (GlobalVPMU.cpu_model == 0)
    {
		    config_file = fopen("./external/qemu-paslab/vpmu_config/def-config-arm926", "r");
		    config_file = fopen("./external/qemu-paslab/vpmu_config/def-config-arm7tdmi", "r");
            //config_file = fopen("./external/qemu-paslab/vpmu_config/def-config-arm11", "r");

            //fprintf(stderr, "VPMU Configure: ARM11\n");
            //fprintf(stderr, "VPMU Configure: ARM926\n");
            fprintf(stderr, "VPMU Configure: ARM7TDMI\n");
    }
	else if (GlobalVPMU.cpu_model == 1)
    {
		config_file = fopen("./external/qemu-paslab/vpmu_config/def-config-cortex-a9", "r");
            fprintf(stderr, "VPMU Configure: ARMCortex-A9\n");
    }

	char tmp[255];
	char *pch;
	int count = 0;
	int line = 0;

	if (config_file == NULL)
	{
		printf("Config file open error\n");
		exit(0);
	}

	while (fgets(tmp, 255, config_file) != NULL)
	{
		if (tmp[0] != '#')
		{
			pch = strtok(tmp, ",");
			while (pch != NULL)
			{
				count++;
				if (count == 2)
				{
					VPMU_set_config(line, pch);
					line++;
				}
				pch = strtok(NULL, ",");
			}
			count = 0;
		}
	}

	//for debug
	int i;
	//printf("total item in file, %d\n", line);
	printf("cpu_freq, %d\n", GlobalVPMU.target_cpu_frequency);
	printf("l1_miss_lat, %d (cycle)\n", GlobalVPMU.l1_cache_miss_lat);
	printf("l2_miss_lat, %d (cycle)\n", GlobalVPMU.l2_cache_miss_lat);
	//printf("maxlevel, %d\n", GlobalVPMU.d4_maxlevel);
	//char type;
	//for (i = 0; i < 4; i++)
	//{
	//	if (i == 0) {idu = 1; lev = 0; type = 'I';}
	//	else if (i == 1) {idu = 0; lev = 1; type = 'U';}
	//	else if (i == 2) {idu = 2; lev = 0; type = 'D';}
	//	else {idu = 0; lev = 1; type = 'U';}

	//	printf("L%d-%c size, %d\n", lev + 1, type, GlobalVPMU.d4_level_size[idu][lev]);
	//	printf("L%d-%c blocksize, %d\n", lev + 1, type, GlobalVPMU.d4_level_blocksize[idu][lev]);
	//	printf("L%d-%c subblocksize, %d\n", lev + 1, type, GlobalVPMU.d4_level_subblocksize[idu][lev]);
	//	printf("L%d-%c assoc, %d\n", lev + 1, type, GlobalVPMU.d4_level_assoc[idu][lev]);
	//	printf("L%d-%c replacement, %c\n", lev + 1, type, GlobalVPMU.d4_level_replacement[idu][lev]);
	//	printf("L%d-%c walloc, %c\n", lev + 1, type, GlobalVPMU.d4_level_walloc[idu][lev]);
	//	printf("L%d-%c wback, %c\n", lev + 1, type, GlobalVPMU.d4_level_wback[idu][lev]);
	//	printf("L%d-%c prefetch, %c\n", lev + 1, type, GlobalVPMU.d4_level_fetch[idu][lev]);
	//	printf("L%d-%c prefetch percent, %d\n", lev + 1, type, GlobalVPMU.d4_level_prefetch_abortpercent[idu][lev]);
	//	printf("L%d-%c prefetch distance, %d\n", lev + 1, type, GlobalVPMU.d4_level_prefetch_distance[idu][lev]);
	//}
	//for (i = 0; i < 173; i++)
	//	printf("%s, %llu\n", arm_instr_names[i], arm_instr_time[i]);

	fclose(config_file);
}

/* evo0209 fixed for multi-level cache init */
void VPMU_cache_init(d4cache *mem, d4cache **dcache, d4cache **icache)
{
	//evo0209
	static char memname[] = "memory";

	int i, lev, idu;
	d4cache *c = NULL,  /* avoid may be used uninitialized' warning in gcc */
			*ci,
			*cd;

	mem = cd = ci = d4new(NULL);
	if (ci == NULL)
		printf("cannot create simulated memory\n");
	ci->name = memname;

	for (lev = GlobalVPMU.d4_maxlevel - 1;  lev >= 0;  lev--) {
		for (idu = 0;  idu < 3;  idu++) {
			if (GlobalVPMU.d4_level_size[idu][lev] != 0) {
				switch (idu) {
					case 0: cd = ci = c = d4new (ci); break;    /* u */
					case 1:      ci = c = d4new (ci); break;    /* i */
					case 2:      cd = c = d4new (cd); break;    /* d */
				}
				if (c == NULL)
					printf("cannot create level %d %ccache\n",
							lev + 1, idu == 0 ? 'u':(idu == 1 ? 'i':'d'));
				d4_cache_init(c, lev, idu);
				GlobalVPMU.d4_levcache[idu][lev] = c;
			}
		}
	}

	i = d4setup();
	if (i != 0 ){
		printf("Cache Create Error %d\n" , i );
	}
	*dcache = cd;
	*icache = ci;
}

void VPMU_reset(void)
{
    int i = 0;
    GlobalVPMU.need_tb_flush = 1;
    GlobalVPMU.USR_icount = 0;
    GlobalVPMU.SYS_icount = 0;
    GlobalVPMU.FIQ_icount = 0;
    GlobalVPMU.IRQ_icount = 0;
    GlobalVPMU.SVC_icount = 0;
    GlobalVPMU.ABT_icount = 0;
    GlobalVPMU.UND_icount = 0;
    GlobalVPMU.sys_call_icount = 0;
    GlobalVPMU.sys_rest_icount=0;

    GlobalVPMU.USR_ldst_count = 0;
    GlobalVPMU.SVC_ldst_count = 0;
    GlobalVPMU.IRQ_ldst_count = 0;
    GlobalVPMU.sys_call_ldst_count = 0;
    GlobalVPMU.sys_rest_ldst_count = 0;

    GlobalVPMU.USR_load_count = 0;
    GlobalVPMU.SVC_load_count = 0;
    GlobalVPMU.IRQ_load_count = 0;
    GlobalVPMU.sys_call_load_count = 0;
    GlobalVPMU.sys_rest_load_count = 0;

    GlobalVPMU.USR_store_count = 0;
    GlobalVPMU.SVC_store_count = 0;
    GlobalVPMU.IRQ_store_count = 0;
    GlobalVPMU.sys_call_store_count = 0;
    GlobalVPMU.sys_rest_store_count = 0;

    GlobalVPMU.timer_interrupt_return_pc = 0;
    GlobalVPMU.state = 0;
    GlobalVPMU.timer_interrupt_exception = 0;

    GlobalVPMU.ticks = 0;
    GlobalVPMU.cpu_idle_time_ns = 0;
    GlobalVPMU.eet_ns = 0;
    GlobalVPMU.last_ticks = 0;
    GlobalVPMU.epc_mpu = 0;
    GlobalVPMU.branch_count = 0;
    GlobalVPMU.all_changes_to_PC_count = 0;
    GlobalVPMU.additional_icache_access = 0;
	//chiaheng
	GlobalVPMU.iomem_count = 0;
	//evo0209
	GlobalVPMU.iomem_test = 0;
	GlobalVPMU.iomem_qemu = 0;

    //converse2006
    GlobalVPMU.netrecv_count = 0;
    GlobalVPMU.netsend_count = 0;
    GlobalVPMU.netrecv_qemu = 0;
    GlobalVPMU.netsend_qemu = 0;

    GlobalVPMU.timing_register = 0;

    GlobalVPMU.swi_fired = 0;
    GlobalVPMU.swi_fired_pc = 0;
    GlobalVPMU.swi_enter_exit_count = 0;

    for(i=0;i<16;i++) {
        GlobalVPMU.dsp_start_time[i] = 0;
        GlobalVPMU.dsp_working_period[i] = 0;
    }
    GlobalVPMU.dsp_running = 0;
    GlobalVPMU.dump_fp = NULL;

	//tianman
#ifdef CONFIG_VPMU_VFP
	GlobalVPMU.VFP_BASE=0;
	for(i=0;i<ARM_VFP_INSTRUCTIONS;i++){
		GlobalVPMU.VFP_count[i]=0;
	}
#endif
	for(i=0;i<ARM_INSTRUCTIONS+2;i++){
		GlobalVPMU.ARM_count[i]=0;
	}

	GlobalVPMU.reduce_ticks = 0;
	//assign the value by using VPMU_select_cpu_model depend on start up args
	//GlobalVPMU.cpu_model = 1;

	//evo0209
	int idu, lev;
	for (idu = 0; idu < 3; idu++)
		for (lev = 0; lev < 5; lev++)
			if (GlobalVPMU.d4_levcache[idu][lev] != NULL)
				ResetCache(GlobalVPMU.d4_levcache[idu][lev]);

	//evo0209
	VPMU_set_config_from_file();
}

//evo0209
#undef FDBG
#define FDBG(str,...) fprintf(stderr, str, ##__VA_ARGS__);\
					  if (fp != NULL) {fprintf(fp, str, ##__VA_ARGS__);}
void VPMU_dump_result()
{
	VPMU* vpmu = &GlobalVPMU;
#ifdef CONFIG_VPMU_THREAD
	VPMU_sync_all();
#endif
	gettimeofday( &t2, NULL );

	if((vpmu->dump_fp) != NULL)
		fclose(vpmu->dump_fp);
	vpmu->dump_fp = NULL;

	fp = fopen("/tmp/profile.txt","w+");
	if (fp == NULL)
		fprintf(stderr, "Cannot open /tmp/profile.txt, please check the permission!\n");
	else
		fseek(fp,0,SEEK_END);

	FDBG("==== Program Profile ====\n\n");
	FDBG("   === QEMU/ARM ===\n");
	VPMU_DUMP_READABLE_MSG();

#if defined(CONFIG_PACDSP)
	/* deprecated */
	/*FDBG("DSP included Estimated execution time : %s usec\n"
	  , commaprint(vpmu_dsp_considered_execution_time()));
	 */
#endif
	FDBG("Emulation time        :%s usec \n"
			, commaprint((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))
		);
	/*FDBG("MIPS: %.2f\n\n",
	  (double)vpmu_total_insn_count()
	  /(( t2.tv_sec - t1.tv_sec) * 1000000 + ( t2.tv_usec - t1.tv_usec))
	  );
	 */
	fclose(fp);
}

//evo0209
void VPMU_select_cpu_model(int model)
{
	/* cpu_model 0: arm926
	 * cpu_model 1: cortex-a9
	 */
	GlobalVPMU.cpu_model = model;
}

/* paslab : monitoring init */
static uint32_t special_read(void *opaque, target_phys_addr_t addr)
{  
    /* vpmu points to GlobalVPMU */
    //VPMUState *vpmu=((VPMUState *)opaque)->VPMU;
    uint32_t ret = 0;

#ifdef PASDEBUG
    DBG("read qemu device at addr=0x%x\n", addr);
#endif
    switch(addr) {
#ifdef CONFIG_PACDSP
    case VPD_MMAP_PLL1_CR:
        ret = pacduo_pregs.PLL1_CR;
        break;
    case VPD_MMAP_PLL1_SR:
        ret = pacduo_pregs.PLL1_SR;
        break;
    case VPD_MMAP_CG_DSP1TPR:
    case VPD_MMAP_CG_DSP1CPR:
        ret = pacduo_pregs.CG_DSP1TPR;
        break;
    case VPD_MMAP_CG_DSP2TPR:
    case VPD_MMAP_CG_DSP2CPR:
        ret = pacduo_pregs.CG_DSP2TPR;
        break;
    case VPD_MMAP_CG_DDRTPR:
    case VPD_MMAP_CG_DDRCPR:
        ret = pacduo_pregs.CG_DDRTPR;
        break;
    case VPD_MMAP_CG_AXITPR:
    case VPD_MMAP_CG_AXICPR:
        ret = pacduo_pregs.CG_AXITPR;
        break;
    case VPD_MMAP_PMU_LCD_CR:
        ret = pacduo_pregs.PMU_LCD_CR;
        break;
    case VPD_MMAP_VC1_TRCR:
        ret = pacduo_pregs.VC1_TRCR;
        break;
    case VPD_MMAP_VC1_SR:
        ret = pacduo_pregs.VC1_SR;
        break;
    case VPD_MMAP_VC1_WDR:
        ret = pacduo_pregs.VC1_WDR;
        break;
    case VPD_MMAP_VC1_CR:
        ret = pacduo_pregs.VC1_CR;
        break;
    case VPD_MMAP_VC2_TRCR:
        ret = pacduo_pregs.VC2_TRCR;
        break;
    case VPD_MMAP_VC2_SR:
        ret = pacduo_pregs.VC2_SR;
        break;
    case VPD_MMAP_VC2_WDR:
        ret = pacduo_pregs.VC2_WDR;
        break;
    case VPD_MMAP_VC2_CR:
        ret = pacduo_pregs.VC2_CR;
        break;
#else
#endif
    default:
        fprintf(stderr,"read unknow address 0x%.4x of vpd\n", addr);
    }

    return ret;
}
/* paslab : addr was 32-bit addr in 0.9.1 but offset in 0.10.5 ??? */
//extern CharDriverState *serial_hds[MAX_SERIAL_PORTS];
static void special_write(void *opaque, target_phys_addr_t addr , uint32_t value) 
{
    //evo0209 : move to global
	//static struct timeval t1, t2;
	int ii;
    static int find_entry = 0;
    static int count = 0, timer = 0;
    static uint32_t last_packets = 0, last_audio = 0;
    VPMU *vpmu=((VPMUState *)opaque)->VPMU;

#ifdef PASDEBUG
    DBG("write qemu device at addr=0x%x value=%d\n", addr, value);
#endif
    switch(addr) {
    case VPMU_MMAP_ENABLE:
    if(value == 0 || (value <= 10 && value >= 2))
    {
        VPMU_reset();
		//evo0209 fixed: change the API name to ResetCache, and move to VPMU_reset
        //InvalidateCache( GlobalVPMU.dcache );
        //InvalidateCache( GlobalVPMU.icache );
        vpmu->dump_fp = fopen("/tmp/dump.txt","w+");
        VPMU_enabled = 1;

        switch(value)
        {
            case 2:
                vpmu_model_setup(vpmu, TIMING_MODEL_A);
                vpmu_simulator_status(vpmu, 0);
                break;
            case 3:
                vpmu_model_setup(vpmu, TIMING_MODEL_B);
                vpmu_simulator_status(vpmu, 0);
                break;
            case 4:
                vpmu_model_setup(vpmu, TIMING_MODEL_C);
                vpmu_simulator_status(vpmu, 0);
                break;
            case 5:
                vpmu_model_setup(vpmu, TIMING_MODEL_D);
                vpmu_simulator_status(vpmu, 0);
                break;
            case 6:
                vpmu_model_setup(vpmu, TIMING_MODEL_E);
                vpmu_simulator_status(vpmu, 0);
                break;
			//tianman
			case 7:
				for (ii = 0; ii < 512; ii++)
				{
					if (pid_base[ii][0] == 1)
						printf("PID: %d, Process_name: %s\n", ii, pid_name_base[ii]);
				}
				break;
            default:
                fprintf(stderr,"Use default timing model\n");
                vpmu_model_setup(vpmu, TIMING_MODEL_C);
                vpmu_simulator_status(vpmu, 0);
                break;
        }

		//evo0209 : move to vpmu_model_setup() for SET can use it
        //gettimeofday( &t1, NULL );
    }   
    else if(value == 1)
    { 
        VPMU_enabled = 0;
#ifdef CONFIG_VPMU_THREAD
        VPMU_sync_all();
#endif
        gettimeofday( &t2, NULL );

        if((vpmu->dump_fp) != NULL)
            fclose(vpmu->dump_fp);
        vpmu->dump_fp = NULL;

        fp=fopen("/tmp/profile.txt","w+");
        fseek(fp,0,SEEK_END);

        FDBG("==== Program Profile ====\n\n");
        FDBG("   === QEMU/ARM ===\n");
        VPMU_DUMP_READABLE_MSG();

#if defined(CONFIG_PACDSP)
        /* deprecated */
        /*FDBG("DSP included Estimated execution time : %s usec\n"
              , commaprint(vpmu_dsp_considered_execution_time()));
              */
#endif
        FDBG("Emulation time        :%s usec \n"
              , commaprint((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))
            );
        /*FDBG("MIPS: %.2f\n\n",
           (double)vpmu_total_insn_count()
           /(( t2.tv_sec - t1.tv_sec) * 1000000 + ( t2.tv_usec - t1.tv_usec))
           );
           */
        fclose(fp);
        /* for debug: don't use vpmu wrapping functions */
        fprintf(stderr,"%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %u %u %llu\n"
                       "%.0f %.0f %.0f\n"
                       "%.0f %.0f\n"
                       "%llu\n"
        , vpmu_total_insn_count(), vpmu->USR_icount, vpmu->SVC_icount, vpmu->IRQ_icount, vpmu->sys_rest_icount
        , vpmu_total_ldst_count(), vpmu->USR_ldst_count, vpmu->SVC_ldst_count, vpmu->IRQ_ldst_count, vpmu->sys_rest_ldst_count
        , vpmu_total_load_count(), vpmu->USR_load_count, vpmu->SVC_load_count, vpmu->IRQ_load_count, vpmu->sys_rest_load_count
        , vpmu_total_store_count(), vpmu->USR_store_count, vpmu->SVC_store_count, vpmu->IRQ_store_count, vpmu->sys_rest_store_count
        , vpmu_branch_insn_count(), vpmu_branch_predict(PREDICT_CORRECT), vpmu_branch_predict(PREDICT_WRONG), vpmu->all_changes_to_PC_count
        , vpmu_L1_dcache_access_count(), vpmu->dcache->miss[D4XREAD], vpmu->dcache->miss[D4XWRITE]
        , vpmu_L1_icache_access_count(), vpmu->icache->miss[D4XINSTRN]
        , vpmu_cycle_count());
        fprintf(stderr, "Emulation time: %s usec \n",
                commaprint((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)));
        fprintf(stderr, "%d (eet in us)\n", vpmu_estimated_execution_time());
        fprintf(stderr, "%llu (idle time in us)\n", vpmu->cpu_idle_time_ns/1000);
        fprintf(stderr, "%d (mpu power consumption)\n", vpmu_vpd_mpu_power());

#ifdef CONFIG_VPMU_VFP
		print_vfp_count();//tianman
#endif
		//print_arm_count();//tianman
#if 0
#if defined(CONFIG_PACDSP)
        int i;
        PACDSPState *pac;
        FDBG("    === PACDSP/PACISS ===\n");
        FOREACH_PACDSP_I(pac, i) {
            paciss_state_t *env = pac->pac;
            int dsp_freq;
            uint64_t lastc = paciss_state_get_tick(pac->pac);

            dsp_freq = pacduo_dsp_freq(i);

            FDBG("PACDSP%d ticks: %lld\n", i,
                  lastc);
            FDBG("PACDSP%d branch cycle: %d\n",i , env->status.branch_cycle);
            FDBG("PACDSP%d simulation cycle(include branch cycle): %lld\n"
                  ,i , env->status.sim_cycle);
            FDBG("PACDSP%d ext memory read stall cycles:: %lld\n", i,
                 paciss_state_get_ext_read_cycle(env));
            FDBG("PACDSP%d ext memory write stall cycles:: %lld\n", i,
                 paciss_state_get_ext_write_cycle(env));
            FDBG("PACDSP%d ext memory read size: %lld (bytes)\n", i,
                 paciss_state_get_ext_read_sz(env));
            FDBG("PACDSP%d ext memory write size: %lld (bytes)\n", i,
                 paciss_state_get_ext_write_sz(env));
            FDBG("PACDSP%d stall cycle: %d\n",i , env->status.stall_cycle);
            FDBG("PACDSP%d instruction cache miss cycle: %d\n",i , env->status.inst_cache_miss_cycle);
            FDBG("PACDSP%d Estimated execution time: %f sec\n"
                  , i, ((double)lastc/(dsp_freq*1000000.0)));
        }
#endif
#endif
    } else if(value == 11) {
        gettimeofday( &t2, NULL );
        unsigned long long time = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

#undef FDBG
#define FDBG(str,...) fprintf(stderr, str, ##__VA_ARGS__)
        VPMU_DUMP_READABLE_MSG();
#undef FDBG

        fprintf(stderr,"Emulation time %s usec \n"
              , commaprint((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))
            );
        /*
        fprintf(stderr,"A: "
                "TotalInsnCount USR_InsnCount SVC_InsnCount IRQ_InsnCount REST_InsnCount "
                "TotalLDSTInsnCount USR_LDST_InsnCount SVC_LDST_InsnCount IRQ_LDST_InsnCount REST_LDST_InsnCount "
                "TotalLoadInsnCount USR_Load_InsnCount SVC_Load_InsnCount IRQ_Load_InsnCount REST_Load_InsnCount "
                "TotalStoreInsnCount USR_Store_InsnCount SVC_Store_InsnCount IRQ_Store_InsnCount REST_Store_InsnCount "
                "BranchInsnCount BranchPredictCorrect BranchPredictWrong AllChangeToPC "
                "D-CacheFetch D-CacheReadMiss D-CacheWriteMiss "
                "I-CacheFetch I-CacheMiss CycleCount EmulationTime "
                "TimerINTInsnCount TimerINTLDSTInsnCount TimerINTLoadInsnCount TimerINTStoreInsnCount\n");
                */
        /* for debug: don't use vpmu wrapping functions */
        fprintf(stderr,"\n%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %llu %llu %llu %llu\n"
                       "%llu %u %u %llu\n"
                       "%.0f %.0f %.0f\n"
                       "%.0f %.0f\n"
                       "%llu %llu\n"
        , vpmu_total_insn_count(), vpmu->USR_icount, vpmu->SVC_icount, vpmu->IRQ_icount, vpmu->sys_rest_icount
        , vpmu_total_ldst_count(), vpmu->USR_ldst_count, vpmu->SVC_ldst_count, vpmu->IRQ_ldst_count, vpmu->sys_rest_ldst_count
        , vpmu_total_load_count(), vpmu->USR_load_count, vpmu->SVC_load_count, vpmu->IRQ_load_count, vpmu->sys_rest_load_count
        , vpmu_total_store_count(), vpmu->USR_store_count, vpmu->SVC_store_count, vpmu->IRQ_store_count, vpmu->sys_rest_store_count
        , vpmu_branch_insn_count(), vpmu_branch_predict(PREDICT_CORRECT), vpmu_branch_predict(PREDICT_WRONG), vpmu->all_changes_to_PC_count
        , vpmu_L1_dcache_access_count(), vpmu->dcache->miss[D4XREAD], vpmu->dcache->miss[D4XWRITE]
        , vpmu_L1_icache_access_count(), vpmu->icache->miss[D4XINSTRN]
        , vpmu_cycle_count(), time
);
        gettimeofday( &t1, NULL );
    } else if (value == 16) {
        fprintf(stderr, "total power: %d\n", vpmu_vpd_total_power());
        fprintf(stderr, "  mpu power: %d\n", vpmu_vpd_mpu_power());
        fprintf(stderr, " dsp1 power: %d\n", vpmu_vpd_dsp1_power());
        fprintf(stderr, " dsp2 power: %d\n", vpmu_vpd_dsp2_power());
    }
        break;
    case VPMU_MMAP_BYPASS_ISR_ADDR:
        if (find_entry < NUM_FIND_SYMBOLS)
        {
            /* timer irq entry of target OS */
            ISR_addr[find_entry] = value;
            fprintf(stderr, "ISR_addr[%d] = %8x\n", find_entry, value);
            find_entry++;
        }
        break;
    case 0x0011:
        DBG("Total instruction count : %llu\n",vpmu_total_insn_count());
        break;
    case 0x0012:
        DBG("Total branch instruction count : %llu\n",vpmu_branch_insn_count());
        break;
    case 0x0013:
        DBG("Data memory access count : %f\n",vpmu_L1_dcache_access_count());
        break;
    case 0x0014:
        DBG("Cycle count : %llu\n",vpmu_cycle_count());
        break;
    case 0x0015:
        DBG("Estimated execution time : %u\n",vpmu_estimated_execution_time());
        break;
    case VPMU_MMAP_POWER_ENABLE:
        fprintf(stderr, "VPMU status : %s\n", VPMU_enabled ? "enable" : "disable");
        break;
    case VPMU_MMAP_BYPASS_CPU_UTIL:
        if(count == 0) {
			//chiaheng
            //power_fp=fopen("/tmp/power_dump.txt","w+");
            //fprintf(power_fp, "total-power LCD CPU Wifi 3G GPS Audio\n");
            //fprintf(stderr, "VPMU: start power sampling\n");
            adev_stat.energy_left = VPD_BAT_CAPACITY * VPD_BAT_VOLTAGE * 3600 *
                                    (*(adev_stat.bat_capacity)/100.0);
            last_packets = *(adev_stat.net_packets);
        }
        else if(count % 2 == 0) {
            adev_stat.cpu_util = value;
            adev_stat.audio_on = last_audio + *(adev_stat.audio_buf);
            if(adev_stat.net_mode == 1) { //Wifi mode
                if(*(adev_stat.net_packets) - last_packets > 8) {
                    adev_stat.net_on = 1;
                }
                else {
                    adev_stat.net_on = 0;
                }
				//chiaheng : Moving power dumping code to a dedicated thread: vpmu_perf_monitoring_thread.
                //fprintf(power_fp, "%d %d %d %d 0 %d %d\n", vpmu_vpd_total_power(),
                //        vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(), vpmu_vpd_net_power(),
                //        vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
            }
            else if(adev_stat.net_mode == 2) { //3G mode
                if(*(adev_stat.net_packets) > last_packets) {
                    timer = 0;
                    adev_stat.net_on = 1;
                }
                else { //*(adev_stat.net_packets) == last_packets
                    timer++;
                    if(timer >= 6) adev_stat.net_on = 0;
                }
				//chiaheng
                //fprintf(power_fp, "%d %d %d 0 %d %d %d\n", vpmu_vpd_total_power(),
                //        vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(), vpmu_vpd_net_power(),
                //        vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
            }
            else { //off
				//chiaheng
                //fprintf(power_fp, "%d %d %d 0 0 %d %d\n", vpmu_vpd_total_power(),
                //        vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(),
                //        vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
            }
            last_packets = *(adev_stat.net_packets);
            if(*(adev_stat.bat_ac_online) == 0 && adev_stat.battery_sim == 1) {
                if(adev_stat.energy_left > vpmu_vpd_total_power())
                    adev_stat.energy_left -= vpmu_vpd_total_power();
                else
                    adev_stat.energy_left = 0;
                goldfish_battery_set_prop(0, POWER_SUPPLY_PROP_CAPACITY, adev_stat.energy_left * 100.0 /
                                          (VPD_BAT_CAPACITY * VPD_BAT_VOLTAGE * 3600));
            }
        }
        else {
            last_audio = *(adev_stat.audio_buf);
        }
        count++;
        break;
    case VPMU_MMAP_SELECT_NET_MODE:
        if(value >= 0 || value <= 2)
            adev_stat.net_mode = value;
        break;
    case VPMU_MMAP_BATTERY_ENABLE:
        if(value == 0 || value == 1)
            adev_stat.battery_sim = value;
        break;
#ifdef CONFIG_PACDSP
    case VPD_MMAP_PLL1_CR:
        if((value>>24) >= 0x0b && (value>>24) <= 0x15) {
            if (VPMU_enabled)
                vpmu_update_eet_epc();
            pacduo_pregs.PLL1_CR = value;
            GlobalVPMU.mpu_freq = ((value>>24)+1)*PACDUO_BASE_CLK/2;
            GlobalVPMU.mpu_power = pacduo_mpu_power();
            GlobalVPMU.dsp1_power = pacduo_dsp_power(0);
            GlobalVPMU.dsp2_power = pacduo_dsp_power(1);
        }
        else {
            fprintf(stderr, "PLL1_CR setting out of range (0x%.8x)\n", value);
        }
        break;
    case VPD_MMAP_CG_DSP1TPR:
        pacduo_pregs.CG_DSP1TPR = value;
        GlobalVPMU.dsp1_power = pacduo_dsp_power(0);
        break;
    case VPD_MMAP_CG_DSP2TPR:
        pacduo_pregs.CG_DSP2TPR = value;
        GlobalVPMU.dsp2_power = pacduo_dsp_power(1);
        break;
    case VPD_MMAP_CG_DDRTPR:
        pacduo_pregs.CG_DDRTPR = value;
        break;
    case VPD_MMAP_CG_AXITPR:
        pacduo_pregs.CG_AXITPR = value;
        break;
    case VPD_MMAP_PMU_LCD_CR:
        pacduo_pregs.PMU_LCD_CR = value;
        break;
    case VPD_MMAP_VC1_TRCR:
        pacduo_pregs.VC1_TRCR = value;
        break;
    case VPD_MMAP_VC1_WDR:
        if(value >= 0x300 && value <= 0x31f) {
            pacduo_pregs.VC1_WDR = value;
            GlobalVPMU.dsp1_power = pacduo_dsp_power(0);
        }
        break;
    case VPD_MMAP_VC1_CR:
        pacduo_pregs.VC1_CR = value;
        break;
    case VPD_MMAP_VC2_TRCR:
        pacduo_pregs.VC2_TRCR = value;
        break;
    case VPD_MMAP_VC2_WDR:
        if(value >= 0x300 && value <= 0x31f) {
            pacduo_pregs.VC2_WDR = value;
            GlobalVPMU.dsp2_power = pacduo_dsp_power(1);
        }
        break;
    case VPD_MMAP_VC2_CR:
        pacduo_pregs.VC2_CR = value;
        break;
#endif
    default:
        fprintf(stderr,"write 0x%d to unknow address 0x%.4x of vpd\n", value, addr);
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

/* paslab : register functions to monitor enable/disable VPMU signals */
void register_VPMU_monitor( uint32_t base , uint32_t size ){
   int iomemtype = cpu_register_io_memory(  special_read_func, special_write_func, NULL );
   cpu_register_physical_memory( base , size , iomemtype );
}

static inline uint64_t dinero_init(void)
{
    VPMU_cache_init( d4_mem_create() , &GlobalVPMU.dcache , &GlobalVPMU.icache  );
    return 0;
}

static inline void vpmu_initiate_attached_simulators(VPMU* vpmu_t)
{
    int i;
    for(i=0;i<10;i++) {
        if(vpmu_t->sim[i][5] != NULL) {
            //fprintf(stderr,"init id = %d\n",i);
            vpmu_t->sim[i][5]();
        }
    }
}

static inline void vpmu_terminate_attached_simulators(void)
{
    VPMU* vpmu_t = &GlobalVPMU;
    int i;
    for(i=0;i<10;i++) {
        if(vpmu_t->sim[i][6] != NULL) {
            //fprintf(stderr,"terminate id = %d\n",i);
            vpmu_t->sim[i][6]();
        }
    }
}

static void vpmu_init_core(VPMUState *s, CharDriverState *chr)
{
#ifdef CONFIG_PACDSP
    GlobalVPMU.mpu_freq = ((pacduo_pregs.PLL1_CR>>24)+1)*PACDUO_BASE_CLK/2;
    GlobalVPMU.mpu_power = pacduo_mpu_power();
    GlobalVPMU.dsp1_power = pacduo_dsp_power(0);
    GlobalVPMU.dsp2_power = pacduo_dsp_power(1);
#else
    GlobalVPMU.mpu_freq = GlobalVPMU.target_cpu_frequency;
    GlobalVPMU.mpu_power = ((0x10000011 >> 24) - 0x0b) * 4.21806 + 156.97284;
    GlobalVPMU.dsp1_power = 0;
    GlobalVPMU.dsp2_power = 0;
#endif
	//evo0209 : set the slowdown ratio 1 by default
    VPMU_update_slowdown_ratio(1);
	printf("Divide frequency ratio: %.2f\n", GlobalVPMU.slowdown_ratio);

    /* initialize android devices state pointer */
    GlobalVPMU.adev_stat_ptr = &adev_stat;

    s->VPMU = &GlobalVPMU;

    vpmu_t.vpmu_monitor= chr ?: qemu_chr_open("vpmu", "vc", NULL);

    uint64_t (*cb[5])(void);
    cb[0] = cb_dcache_access_count;
    cb[1] = vpmu_L1_dcache_miss_count;
    cb[2] = vpmu_L1_dcache_read_miss_count;
    cb[3] = vpmu_L1_dcache_write_miss_count;
    vpmu_attach_simulator("dcache", cb, 4, dinero_init, NULL);

    cb[0] = cb_icache_access_count;
    cb[1] = vpmu_L1_icache_miss_count;
    vpmu_attach_simulator("icache", cb, 2, NULL, NULL);

    cb[0] = branch_predict_correct;
    cb[1] = branch_predict_wrong;
    vpmu_attach_simulator("Branch Predictor", cb, 2, NULL, NULL);

	//evo0209
	VPMU_set_config_from_file();

    vpmu_initiate_attached_simulators(s->VPMU);
    atexit(vpmu_terminate_attached_simulators);

#ifdef CONFIG_VPMU_THREAD
    pthread_create( &d_cache_thread, NULL, dinero_thread, s->VPMU);
    pthread_create( &branch_predictor_thread, NULL, branch_thread, s->VPMU);
#endif

	//chiaheng
	// Create the performance monitoring thread, which dump the selected events.
	pthread_create( &perf_mntr_thread, NULL, vpmu_perf_monitoring_thread, s->VPMU);
	// Register termination function for the thread.
	//atexit();
    
    QLIST_INSERT_HEAD(&vpmu_t.head, s, entries);
}

static inline uint64_t null_func(void)
{
    printf("(VPMU): No such hardware event (%s return 0)\n", __FUNCTION__);
    return 0;
}

#define VPMU_ASSIGN_FUNC(ID) \
do {\
    for(i = 0;i < number_of_event;i++) {\
        GlobalVPMU.sim[ID][i] = cb[i];\
    }\
    for(;i < 5;i++) {\
        GlobalVPMU.sim[ID][i] = null_func;\
    }\
    if(init != NULL)\
        GlobalVPMU.sim[ID][5] = init;\
    else\
        GlobalVPMU.sim[ID][5] = NULL;\
    if(reset != NULL)\
        GlobalVPMU.sim[ID][6] = reset;\
    else\
        GlobalVPMU.sim[ID][6] = NULL;\
} while(0)

int vpmu_attach_simulator(char* description, uint64_t (*cb[5])(void), int number_of_event, uint64_t init(void), uint64_t reset(void))
{
    static int id = 3;
    int i;
    if(strcmp(description, "dcache") == 0) {
        fprintf(stderr,"D-cache registered ID = 0 event NO. = 0x0e to 0x%x\n", 0x0e + number_of_event - 1);
        VPMU_ASSIGN_FUNC(0);
        return 0;
    }
    if(strcmp(description, "icache") == 0) {
        fprintf(stderr,"I-cache registered ID = 1 event NO. = 0x13 to 0x%x\n", 0x13 + number_of_event - 1);
        VPMU_ASSIGN_FUNC(1);
        return 1;
    }
    if(strcmp(description, "Branch Predictor") == 0) {
        fprintf(stderr,"%s registered ID = 2 event NO. = 0x%x to 0x%x\n", description, 0x0e + (2*5)
                , 0x0e + (2*5) +number_of_event - 1);
        VPMU_ASSIGN_FUNC(2);
        return 2;
    }
    fprintf(stderr,"%s registered ID = %d event NO. = 0x%x to 0x%x\n", description, id, 0x0e + (id*5)
            , 0x0e + (id*5) + number_of_event - 1);
    VPMU_ASSIGN_FUNC(id);
    return id++;
}

static int vpmu_init_sysbus(SysBusDevice *dev)
{
#ifdef CONFIG_VPMU_THREAD
    fprintf(stderr,"VPMU (threading) initiate\n");
#else
    fprintf(stderr,"VPMU (sequential) initiate\n");
#endif

    VPMUState *s = FROM_SYSBUS(VPMUState, dev);
    
    CharDriverState* chr = qdev_init_chardev(&dev->qdev);

    vpmu_init_core(s, chr);

    int iomemtype = cpu_register_io_memory(special_read_func, special_write_func, s);

    sysbus_init_mmio(dev, VPMU_IOMEM_SIZE, iomemtype);
    
    sysbus_init_irq(dev, &s->irq);
    
    return 0;
}

static void vpmu_register_devices(void)
{
    QLIST_INIT(&vpmu_t.head);
    sysbus_register_dev("vpmu", sizeof(VPMUState), vpmu_init_sysbus);
}

/* Legacy helper function.  Should go away when machine config files are
   implemented.  */
void vpmu_init(uint32_t base, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *s;

    dev = qdev_create(NULL, "vpmu");
    qdev_init(dev);    
    s = sysbus_from_qdev(dev);
    sysbus_mmio_map(s, 0, base);
    sysbus_connect_irq(s, 0, irq);
}

device_init(vpmu_register_devices);
