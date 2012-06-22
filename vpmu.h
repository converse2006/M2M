#ifndef __VPMU_H__
#define __VPMU_H__
#include <stdint.h>
#include "d4-7/d4.h"
#include "vpmu_module.h"
#ifdef CONFIG_PACDSP
#include "vpd_module.h"
#endif

#ifdef CONFIG_VPMU_THREAD
extern volatile short VPMU_enabled;
#else
extern short VPMU_enabled;
#endif

extern struct VPMU GlobalVPMU;
extern struct cp14_pmu_state* cp14_pmu_state_pt;

#define VPMU_IOMEM_SIZE 0x2000 /* 0x0000~0x0fff is used by vpd */

//chiaheng
// Define maximum supported events (event ids) defined in
//"struct VPMU".
#define MAX_VPMU_SUPPORT_EVENT  256
#define CUR_VPMU_SUPPORT_EVENT  46
#define EVENT_ON                1
#define EVENT_OFF               0

/* The staring address of system RAM. */
#define SYSTEM_RAM_START        0x00000000
#define SYSTEM_RAM_END          0x05ffffff

/* target board cache latency to esitmate execution time */
//evo0209
//#define CACHE_MISS_LATENCY 30 //cycles

#ifdef CONFIG_PACDSP
#define VPMU_BASE_ADDR 0x5000f000
#define TARGET_CPU_FREQUENCY GlobalVPMU.mpu_freq
#define PACDUO_BASE_CLK 24 //24MHz in PACDUO
#define DSP_START_CYCLE 5
#else
#define VPMU_BASE_ADDR 0xf0000000
//evo0209
//#define TARGET_CPU_FREQUENCY 204
#endif

#define TARGET_AVG_CPI 1.51189302527

/* timer interrupt interval = 10 ms = 10000 us */
#define TIMER_INTERRUPT_INTERVAL 10000

#define NUM_FIND_SYMBOLS 1
extern uint32_t ISR_addr[NUM_FIND_SYMBOLS];

enum {
    TICK_SCHED_TIMER = 0,
    //RUN_TIMER_SOFTIRQ = 0,
    //GOLDFISH_TIMER_INTERRUPT,
};

#define PREDICT_CORRECT 0
#define PREDICT_WRONG 1
#define BRANCH_PREDICT_SYNC 2

#define CCCR   0x00  /* Core Clock Configuration register */
#define CKEN   0x04  /* Clock Enable register */
#define OSCC   0x08  /* Oscillator Configuration register */
#define CCSR   0x0c  /* Core Clock Status register */

/* Timing model selector */
#define TIMING_MODEL_A 0x1<<28
#define TIMING_MODEL_B 0x2<<28
#define TIMING_MODEL_C 0x3<<28
#define TIMING_MODEL_D 0x4<<28
#define TIMING_MODEL_E 0x5<<28
#define TIMING_MODEL_F 0x6<<28
#define TIMING_MODEL_G 0x7<<28

#define VPMU_INSN_COUNT_SIM 0x1<<0
#define VPMU_DCACHE_SIM     0x1<<1
#define VPMU_ICACHE_SIM     0x1<<2
#define VPMU_BRANCH_SIM     0x1<<3
#define VPMU_PIPELINE_SIM   0x1<<4
#define VPMU_SAMPLING       0x1<<5


//tianman
/* Do not change the order of the instructions in the blocks marked by with - | -. */
enum arm_instructions {
	ARM_INSTRUCTION_B,
	ARM_INSTRUCTION_BL,
	ARM_INSTRUCTION_BLX,
	ARM_INSTRUCTION_BX,
	ARM_INSTRUCTION_BXJ,
	ARM_INSTRUCTION_ADC,
	ARM_INSTRUCTION_ADD,
	ARM_INSTRUCTION_AND,
	ARM_INSTRUCTION_BIC,
	ARM_INSTRUCTION_CMN,
	ARM_INSTRUCTION_CMP,
	ARM_INSTRUCTION_EOR,
	ARM_INSTRUCTION_MOV,
	ARM_INSTRUCTION_MVN,
	ARM_INSTRUCTION_ORR,
	ARM_INSTRUCTION_RSB,
	ARM_INSTRUCTION_RSC,
	ARM_INSTRUCTION_SBC,
	ARM_INSTRUCTION_SUB,
	ARM_INSTRUCTION_TEQ,
	ARM_INSTRUCTION_TST,
	ARM_INSTRUCTION_MUL,  /* - */
	ARM_INSTRUCTION_MULS, /* - */
	ARM_INSTRUCTION_MLA,  /* - */
	ARM_INSTRUCTION_MLAS, /* - */
	ARM_INSTRUCTION_SMLAXY,
	ARM_INSTRUCTION_SMLAL,  /* - */
	ARM_INSTRUCTION_SMLALS, /* - */
	ARM_INSTRUCTION_SMLALXY,
	ARM_INSTRUCTION_SMLAWY,
	ARM_INSTRUCTION_SMUAD, /* - */
	ARM_INSTRUCTION_SMUSD, /* | */
	ARM_INSTRUCTION_SMLAD, /* | */
	ARM_INSTRUCTION_SMLSD, /* - */
	ARM_INSTRUCTION_SMLALD, /* - */
	ARM_INSTRUCTION_SMLSLD, /* - */
	ARM_INSTRUCTION_SMMLA,
	ARM_INSTRUCTION_SMMLS,
	ARM_INSTRUCTION_SMMUL,
	ARM_INSTRUCTION_SMULXY,
	ARM_INSTRUCTION_SMULL,  /* - */
	ARM_INSTRUCTION_SMULLS, /* - */
	ARM_INSTRUCTION_SMULWY,
	ARM_INSTRUCTION_UMAAL,
	ARM_INSTRUCTION_UMLAL,  /* - */
	ARM_INSTRUCTION_UMLALS, /* - */
	ARM_INSTRUCTION_UMULL,  /* - */
	ARM_INSTRUCTION_UMULLS, /* - */
	ARM_INSTRUCTION_QADD,
	ARM_INSTRUCTION_QDADD,
	ARM_INSTRUCTION_QADD16,   /* - */
	ARM_INSTRUCTION_QADDSUBX, /* | */
	ARM_INSTRUCTION_QSUBADDX, /* | */
	ARM_INSTRUCTION_QSUB16,   /* | */
	ARM_INSTRUCTION_QADD8,    /* | */
	ARM_INSTRUCTION_QSUB8,    /* - */
	ARM_INSTRUCTION_QSUB,
	ARM_INSTRUCTION_QDSUB,
	ARM_INSTRUCTION_SADD16,   /* - */
	ARM_INSTRUCTION_SADDSUBX, /* | */
	ARM_INSTRUCTION_SSUBADDX, /* | */
	ARM_INSTRUCTION_SSUB16,   /* | */
	ARM_INSTRUCTION_SADD8,    /* | */
	ARM_INSTRUCTION_SSUB8,    /* - */
	ARM_INSTRUCTION_SHADD16,   /* - */
	ARM_INSTRUCTION_SHADDSUBX, /* | */
	ARM_INSTRUCTION_SHSUBADDX, /* | */
	ARM_INSTRUCTION_SHSUB16,   /* | */
	ARM_INSTRUCTION_SHADD8,    /* | */
	ARM_INSTRUCTION_SHSUB8,    /* - */
	ARM_INSTRUCTION_UADD16,   /* - */
	ARM_INSTRUCTION_UADDSUBX, /* | */
	ARM_INSTRUCTION_USUBADDX, /* | */
	ARM_INSTRUCTION_USUB16,   /* | */
	ARM_INSTRUCTION_UADD8,    /* | */
	ARM_INSTRUCTION_USUB8,    /* - */
	ARM_INSTRUCTION_UHADD16,   /* - */
	ARM_INSTRUCTION_UHADDSUBX, /* | */
	ARM_INSTRUCTION_UHSUBADDX, /* | */
	ARM_INSTRUCTION_UHSUB16,   /* | */
	ARM_INSTRUCTION_UHADD8,    /* | */
	ARM_INSTRUCTION_UHSUB8,    /* - */
	ARM_INSTRUCTION_UQADD16,   /* - */
	ARM_INSTRUCTION_UQADDSUBX, /* | */
	ARM_INSTRUCTION_UQSUBADDX, /* | */
	ARM_INSTRUCTION_UQSUB16,   /* | */
	ARM_INSTRUCTION_UQADD8,    /* | */
	ARM_INSTRUCTION_UQSUB8,    /* - */
	ARM_INSTRUCTION_SXTAB16, /* - */
	ARM_INSTRUCTION_SXTAB,   /* | */
	ARM_INSTRUCTION_SXTAH,   /* | */
	ARM_INSTRUCTION_SXTB16,  /* | */
	ARM_INSTRUCTION_SXTB,    /* | */
	ARM_INSTRUCTION_SXTH,    /* - */
	ARM_INSTRUCTION_UXTAB16, /* - */
	ARM_INSTRUCTION_UXTAB,   /* | */
	ARM_INSTRUCTION_UXTAH,   /* | */
	ARM_INSTRUCTION_UXTB16,  /* | */
	ARM_INSTRUCTION_UXTB,    /* | */
	ARM_INSTRUCTION_UXTH,    /* - */
	ARM_INSTRUCTION_CLZ,
	ARM_INSTRUCTION_USAD8,
	ARM_INSTRUCTION_USADA8,
	ARM_INSTRUCTION_PKH,
	ARM_INSTRUCTION_PKHBT,
	ARM_INSTRUCTION_PKHTB,
	ARM_INSTRUCTION_REV,
	ARM_INSTRUCTION_REV16,
	ARM_INSTRUCTION_REVSH,
	ARM_INSTRUCTION_SEL,
	ARM_INSTRUCTION_SSAT,
	ARM_INSTRUCTION_SSAT16,
	ARM_INSTRUCTION_USAT,
	ARM_INSTRUCTION_USAT16,
	ARM_INSTRUCTION_MRS,
	ARM_INSTRUCTION_MSR,
	ARM_INSTRUCTION_CPS,
	ARM_INSTRUCTION_SETEND,
	ARM_INSTRUCTION_LDR,
	ARM_INSTRUCTION_LDRB,
	ARM_INSTRUCTION_LDRBT,
	ARM_INSTRUCTION_LDRD,
	ARM_INSTRUCTION_LDREX,
	ARM_INSTRUCTION_LDRH,
	ARM_INSTRUCTION_LDRSB,
	ARM_INSTRUCTION_LDRSH,
	ARM_INSTRUCTION_LDRT,
	ARM_INSTRUCTION_STR,
	ARM_INSTRUCTION_STRB,
	ARM_INSTRUCTION_STRBT,
	ARM_INSTRUCTION_STRD,
	ARM_INSTRUCTION_STREX,
	ARM_INSTRUCTION_STRH,
	ARM_INSTRUCTION_STRT,
	ARM_INSTRUCTION_LDM1, //See Arm manual ARM DDI 0100I page A3-27
	ARM_INSTRUCTION_LDM2,
	ARM_INSTRUCTION_LDM3,
	ARM_INSTRUCTION_STM1,
	ARM_INSTRUCTION_STM2,
	ARM_INSTRUCTION_SWP,
	ARM_INSTRUCTION_SWPB,
	ARM_INSTRUCTION_BKPT,
	ARM_INSTRUCTION_SWI,
	ARM_INSTRUCTION_CDP,
	ARM_INSTRUCTION_LDC,
	ARM_INSTRUCTION_MCR,
	ARM_INSTRUCTION_MCRR,
	ARM_INSTRUCTION_MRC,
	ARM_INSTRUCTION_MRRC,
	ARM_INSTRUCTION_STC,
	ARM_INSTRUCTION_PLD,
	ARM_INSTRUCTION_RFE,
	ARM_INSTRUCTION_SRS,
	ARM_INSTRUCTION_MCRR2,
	ARM_INSTRUCTION_MRRC2,
	ARM_INSTRUCTION_STC2,
	ARM_INSTRUCTION_LDC2,
	ARM_INSTRUCTION_CDP2,
	ARM_INSTRUCTION_MCR2,
	ARM_INSTRUCTION_MRC2,
	ARM_INSTRUCTION_COPROCESSOR,
	ARM_INSTRUCTION_NEON_DP,
	ARM_INSTRUCTION_NEON_LS,
	ARM_INSTRUCTION_CLREX,
	ARM_INSTRUCTION_DSB,
	ARM_INSTRUCTION_DMB,
	ARM_INSTRUCTION_ISB,
	ARM_INSTRUCTION_MOVW,
	ARM_INSTRUCTION_MOVT,
	ARM_INSTRUCTION_UNKNOWN,
	ARM_INSTRUCTION_NOT_INSTRUMENTED,
	ARM_INSTRUCTION_TOTAL_COUNT,
	ARM_INSTRUCTIONS
};

//tianman
/* Do not change the order of the instructions in the blocks marked by with - | -. */
enum arm_vfp_instructions {
	ARM_VFP_INSTRUCTION_FABSD, /* - */
	ARM_VFP_INSTRUCTION_FABSS, /* - */
	ARM_VFP_INSTRUCTION_FADDD, /* - */
	ARM_VFP_INSTRUCTION_FADDS, /* - */
	ARM_VFP_INSTRUCTION_FCMPD, /* - */
	ARM_VFP_INSTRUCTION_FCMPS, /* - */
	ARM_VFP_INSTRUCTION_FCMPED,  /* - */
	ARM_VFP_INSTRUCTION_FCMPES,  /* - */
	ARM_VFP_INSTRUCTION_FCMPEZD, /* - */
	ARM_VFP_INSTRUCTION_FCMPEZS, /* - */
	ARM_VFP_INSTRUCTION_FCMPZD, /* - */
	ARM_VFP_INSTRUCTION_FCMPZS, /* - */
	ARM_VFP_INSTRUCTION_FCPYD, /* - */
	ARM_VFP_INSTRUCTION_FCPYS, /* - */
	ARM_VFP_INSTRUCTION_FCVTDS, /* - */
	ARM_VFP_INSTRUCTION_FCVTSD, /* - */
	ARM_VFP_INSTRUCTION_FDIVD, /* - */
	ARM_VFP_INSTRUCTION_FDIVS, /* - */
	ARM_VFP_INSTRUCTION_FLDD, /* - */
	ARM_VFP_INSTRUCTION_FLDS, /* - */
	ARM_VFP_INSTRUCTION_FLDMD, /* - */
	ARM_VFP_INSTRUCTION_FLDMS, /* - */
	ARM_VFP_INSTRUCTION_FLDMX,
	ARM_VFP_INSTRUCTION_FMACD,
	ARM_VFP_INSTRUCTION_FMACS,
	ARM_VFP_INSTRUCTION_FMDHR,
	ARM_VFP_INSTRUCTION_FMDLR,
	ARM_VFP_INSTRUCTION_FMDRR,
	ARM_VFP_INSTRUCTION_FMRDH,
	ARM_VFP_INSTRUCTION_FMRDL,
	ARM_VFP_INSTRUCTION_FMRRD, /* - */
	ARM_VFP_INSTRUCTION_FMRRS, /* - */
	ARM_VFP_INSTRUCTION_FMRS,
	ARM_VFP_INSTRUCTION_FMRX,
	ARM_VFP_INSTRUCTION_FMSCD, /* - */
	ARM_VFP_INSTRUCTION_FMSCS, /* - */
	ARM_VFP_INSTRUCTION_FMSR,
	ARM_VFP_INSTRUCTION_FMSRR,
	ARM_VFP_INSTRUCTION_FMSTAT,
	ARM_VFP_INSTRUCTION_FMULD, /* - */
	ARM_VFP_INSTRUCTION_FMULS, /* - */
	ARM_VFP_INSTRUCTION_FMXR,
	ARM_VFP_INSTRUCTION_FNEGD, /* - */
	ARM_VFP_INSTRUCTION_FNEGS, /* - */
	ARM_VFP_INSTRUCTION_FNMACD, /* - */
	ARM_VFP_INSTRUCTION_FNMACS, /* - */
	ARM_VFP_INSTRUCTION_FNMSCD, /* - */
	ARM_VFP_INSTRUCTION_FNMSCS, /* - */
	ARM_VFP_INSTRUCTION_FNMULD, /* - */
	ARM_VFP_INSTRUCTION_FNMULS, /* - */
	ARM_VFP_INSTRUCTION_FSITOD, /* - */
	ARM_VFP_INSTRUCTION_FSITOS, /* - */
	ARM_VFP_INSTRUCTION_FSQRTD, /* - */
	ARM_VFP_INSTRUCTION_FSQRTS, /* - */
	ARM_VFP_INSTRUCTION_FSTD, /* - */
	ARM_VFP_INSTRUCTION_FSTS, /* - */
	ARM_VFP_INSTRUCTION_FSTMD,
	ARM_VFP_INSTRUCTION_FSTMS,
	ARM_VFP_INSTRUCTION_FSTMX,
	ARM_VFP_INSTRUCTION_FSUBD, /* - */
	ARM_VFP_INSTRUCTION_FSUBS, /* - */
	ARM_VFP_INSTRUCTION_FTOSID, /* - */
	ARM_VFP_INSTRUCTION_FTOSIS, /* - */
	ARM_VFP_INSTRUCTION_FTOUID, /* - */
	ARM_VFP_INSTRUCTION_FTOUIS, /* - */
	ARM_VFP_INSTRUCTION_FTOUIZD,
	ARM_VFP_INSTRUCTION_FTOUIZS,
	ARM_VFP_INSTRUCTION_FTOUSIZD,
	ARM_VFP_INSTRUCTION_FTOUSIZS,
	ARM_VFP_INSTRUCTION_FUITOD, /* - */
	ARM_VFP_INSTRUCTION_FUITOS, /* - */
	ARM_VFP_INSTRUCTION_UNKNOWN,
	ARM_VFP_INSTRUCTION_NOT_INSTRUMENTED,
	ARM_VFP_INSTRUCTION_TOTAL_COUNT,
	ARM_VFP_INSTRUCTIONS
};

//evo0209
extern uint32_t arm_instr_time[ARM_INSTRUCTIONS];

//chiaheng
enum VPMU_EVENT_ID_MAPPING {
	// Performance Events To Be Monitored.
	// Instructions related events.
	TOT_INST=0,
	USR_INST,
	SVC_INST,
	IRQ_INST,
	REST_INST,
	TOT_LD_ST_INST,
	TOT_USR_LD_ST_INST,
	TOT_SVC_LD_ST_INST,
	TOT_IRQ_LD_ST_INST,
	TOT_REST_LD_ST_INST,
	TOT_LD_INST,
	TOT_USR_LD_INST,
	TOT_SVC_LD_INST,
	TOT_IRQ_LD_INST,
	TOT_REST_LD_INST,
	TOT_ST_INST,
	TOT_USR_ST_INST,
	TOT_SVC_ST_INST,
	TOT_IRQ_ST_INST,
	TOT_REST_ST_INST,
	PC_CHANGE,
	// Branch related events.
	BRANCH_INST=32,
	BRANCH_PREDICT_CORRECT,
	BRANCH_PREDICT_WRONG,
	// Cache realted events.
	// Please make sure you have turned on the cache simulation.,
	DCACHE_ACCESSES_CNT=48,
	DCACHE_WRITE_MISS,
	DCACHE_READ_MISS,
	DCACHE_MISS,
	ICACHE_ACCESSES_CNT,
	ICACHE_MISS,
	// Memory acess.
	SYSTEM_MEM_ACCESS=64,
	IO_MEM_ACCESS,
	// Estimated execution time,
	ELAPSED_CPU_CYC=80,
	ELAPSED_EXE_TIME,
	ELAPSED_PIPELINE_CYC,
	ELAPSED_PIPELINE_EXE_TIME,
	ELAPSED_SYSTEM_MEM_CYC,
	ELAPSED_SYSTEM_MEM_ACC_TIME,
	ELAPSED_IO_MEM_CYC,
	ELAPSED_IO_MEM_ACC_TIME,
	// Energy events,
	MPU_ENERGY=240,
	//DSP1_ENERGY,
	//DSP2_ENERGY,
	LCD_ENERGY=243,
	AUDIO_ENERGY,
	NET_ENERGY,
	GPS_ENERGY,
	TOTAL_ENERGY=255
};


struct cp14_pmu_state {
    /* Power management */
    uint32_t pm_base;
    uint32_t pm_regs[0x40];

    /* Clock management */
    uint32_t cm_base;
    uint32_t cm_regs[4];
    uint32_t clkcfg;
    /* Performance monitoring */
    uint32_t pmnc;
    uint32_t evtsel;
    uint32_t pmn[4];
    uint32_t flag;
    uint32_t inten;
    struct VPMU * vpmu_t;
    /* VPMU for Oprofile */
    /* Check vpmu-device.c */
    uint32_t pmn_reset_value[4];
};

struct ExtraTBInfo
{
    unsigned int insn_count ;
    unsigned char load_count;
    unsigned char store_count;
    unsigned char mem_pattern;
    unsigned char mem_ref_count ;
    unsigned char last_mem_ref_count ;
    unsigned char same_hit_count;
    unsigned char same_ideal_flush_count;
    //unsigned int  ticks;
	uint64_t ticks;
    //unsigned int  mem_address[TIM_TB_memaddress_SIZE ];
    //unsigned int  last_mem_address[TIM_TB_memaddress_SIZE ];
    unsigned char *target_mem_ref_count;
    unsigned int  *target_mem_address ;
    d4memref      icache_ref ;
    unsigned int  branch_count;
    unsigned int  all_changes_to_PC_count;
    unsigned long pc;

	//tianman
	int vfp_locks[32];
	uint64_t vfp_base;
	uint64_t  vfp_count[ARM_VFP_INSTRUCTIONS];
	uint64_t  arm_count[ARM_INSTRUCTIONS+2];

	//evo0209
	uint64_t insn_buf[2];
	int insn_buf_index;
	uint64_t dual_issue_reduce_ticks;
};

struct DSPInfo
{
    unsigned long long cycle_count;
    unsigned long long mem_read_size_count;
    unsigned long long mem_write_size_count;
};

#define VPD_BAT_CAPACITY 1150
#define VPD_BAT_VOLTAGE 3.7
struct android_dev_stat {
    uint32_t cpu_util;
    uint32_t *lcd_brightness;
    int *audio_buf;
    uint32_t audio_on; // 0:off others:on
    char net_mode; // 0:off 1:Wifi 2:3G
    char net_on; // 0:off 1:on
    uint32_t *net_packets;
    uint32_t *gps_status; // 0:off 1:idle 2:active
    char battery_sim; // 0:off 1:on
    uint32_t energy_left; // mJ
    int *bat_ac_online; // 0:offline 1:online
    int *bat_capacity;
};

typedef struct VPMU{
	//tianman
	uint64_t VFP_count[ARM_VFP_INSTRUCTIONS];
	uint64_t ARM_count[ARM_INSTRUCTIONS+2];
	uint64_t VFP_BASE;
	uint64_t target_cpu_frequency;

	/* evo0209 : for chose cpu model */
	int cpu_model;

    /* TODO: remove unused varibles SYS/FIQ/ABT/UND_icount*/
    /* TODO: define a struct contains
     * icount/ldst_count/load_count/store_count for all the 7 ARM modes */
    unsigned long long USR_icount;
    unsigned long long SYS_icount;
    unsigned long long FIQ_icount;
    unsigned long long IRQ_icount;
    unsigned long long SVC_icount;
    unsigned long long ABT_icount;
    unsigned long long UND_icount;
    unsigned long long sys_call_icount;
    /* insn count of FIQ+IRQ+ABT+UND+SYS */
    unsigned long long sys_rest_icount;

    unsigned long long USR_ldst_count;
    unsigned long long SVC_ldst_count;
    unsigned long long IRQ_ldst_count;
    unsigned long long sys_call_ldst_count;
    /* ldst count of FIQ+IRQ+ABT+UND+SYS */
    unsigned long long sys_rest_ldst_count;

    unsigned long long USR_load_count;
    unsigned long long SVC_load_count;
    unsigned long long IRQ_load_count;
    unsigned long long sys_call_load_count;
    /* load count of FIQ+IRQ+ABT+UND+SYS */
    unsigned long long sys_rest_load_count;

    unsigned long long USR_store_count;
    unsigned long long SVC_store_count;
    unsigned long long IRQ_store_count;
    unsigned long long sys_call_store_count;
    /* store count of FIQ+IRQ+ABT+UND+SYS */
    unsigned long long sys_rest_store_count;

    /* for timer interrupt */
    unsigned int  timer_interrupt_return_pc;
    char state;
    char timer_interrupt_exception;
    char need_tb_flush;

    /* slowdown guest timing for correct timer interrupt generation */
    double slowdown_ratio;

    /* last halted vm_clock is saved here for idle time caculation */
    uint64_t cpu_halted_vm_clock;
    uint64_t last_vm_clock;

    //unsigned long long ticks;
	uint64_t ticks;
    uint64_t cpu_idle_time_ns;
    uint64_t eet_ns; //estimated execution time in ns
    uint64_t last_ticks; //tmp data for calculate eet_ns
    unsigned long epc_mpu; //estimated power consumption for mpu
    unsigned long long branch_count;
    unsigned long long all_changes_to_PC_count;
    unsigned long long additional_icache_access;

	//chiaheng
	uint64_t iomem_count;
	uint64_t iomem_qemu;
	int iomem_test;

    //converse2006
    uint64_t netrecv_count;
    uint64_t netsend_count;
    uint64_t netsend_qemu;
    uint64_t netrecv_qemu;

	/* evo0209 : for dual issue check */
	uint64_t reduce_ticks;

	/* Dinero IV cache configuration and data structure */
	d4cache *dcache;
	d4cache *icache;
	d4cache *d4_levcache[3][5];
	int d4_maxlevel;    /* the highest level actually used */
	unsigned int d4_level_blocksize[3][5];
	unsigned int d4_level_subblocksize[3][5];
	unsigned int d4_level_size[3][5];
	unsigned int d4_level_assoc[3][5];
	int d4_level_doccc[3][5];
	int d4_level_replacement[3][5];
	int d4_level_fetch[3][5];
	int d4_level_walloc[3][5];
	int d4_level_wback[3][5];
	int d4_level_prefetch_abortpercent[3][5];
	int d4_level_prefetch_distance[3][5];
	uint64_t l1_cache_miss_lat;
	uint64_t l2_cache_miss_lat;

    /* branch prediction */
    unsigned int branch_predict_correct;
    unsigned int branch_predict_wrong;

    /* timing model selector : bit[31-28] (model selector) bit[27-0] (simulator on-off table) */
    unsigned int timing_register;

    //struct ExtraTBInfo *currentTb_info;
    //struct ExtraTBInfo *lastTb_info;

    char swi_fired;
    unsigned long swi_fired_pc; 
    unsigned long swi_enter_exit_count;

    /* DSP information */
    unsigned int dsp_start_time[16];
    unsigned int dsp_working_period[16];
    uint8_t dsp_running;
    /*
     * Virtual Power Device Info
     * added by gkk886
     */
    double mpu_power;
    double dsp1_power;
    double dsp2_power;
    /* MPU frequency */
    int mpu_freq;
    /* Android devices state*/
    struct android_dev_stat *adev_stat_ptr;
#ifdef CONFIG_PACDSP
    /* DSP status */
    struct DSPInfo DSPstate[16];
    uint64_t offset;
#else
#endif
    FILE *dump_fp;
    /* call back function for more simulators */
    /* 10 simulators at most, 7 call backs for each, last two call back are init/reset */
    uint64_t (*sim[10][7])(void);
}VPMU;

static inline int clog2 (unsigned int x)
{
    int i;
    for (i = -1;  x != 0;  i++)
    x >>= 1;
    return i;
}

/*********************
 * perfctr interface *
 *********************/
uint32_t vpmu_hardware_event_select(uint32_t, VPMU*);
uint32_t pas_cp14_read(void* , int , int , int );
void pas_cp14_write(void *, int , int , int , uint32_t );

/*************************
 * VPMU access functions *
 *************************/
uint64_t vpmu_branch_insn_count(void);
uint32_t vpmu_branch_predict(uint8_t);
uint64_t vpmu_cycle_count(void);
//double vpmu_dcache_access_count(void);
//uint64_t vpmu_dcache_miss_count(void);
//uint64_t vpmu_dcache_read_miss_count(void);
//uint64_t vpmu_dcache_write_miss_count(void);
//double vpmu_icache_access_count(void);
//uint64_t vpmu_icache_miss_count(void);
//evo0209
uint64_t vpmu_L1_dcache_access_count(void);
uint64_t vpmu_L1_dcache_miss_count(void);
uint64_t vpmu_L1_dcache_read_miss_count(void);
uint64_t vpmu_L1_dcache_write_miss_count(void);
uint64_t vpmu_L2_dcache_access_count(void);
uint64_t vpmu_L2_dcache_miss_count(void);
uint64_t vpmu_L2_dcache_read_miss_count(void);
uint64_t vpmu_L2_dcache_write_miss_count(void);
uint64_t vpmu_L1_icache_access_count(void);
uint64_t vpmu_L1_icache_miss_count(void);
uint64_t vpmu_total_insn_count(void);
uint64_t vpmu_total_ldst_count(void);
uint64_t vpmu_total_load_count(void);
uint64_t vpmu_total_store_count(void);
uint32_t vpmu_estimated_execution_time(void);
uint64_t vpmu_estimated_execution_time_ns(void);
//chiaheng
uint64_t vpmu_estimated_pipeline_execution_time_ns(void);
uint64_t vpmu_estimated_io_memory_access_time_ns(void);
uint64_t vpmu_estimated_sys_memory_access_time_ns(void);

//void vpmu_dump_machine_state(void *);
void vpmu_dump_machine_state_v2(void *);
/*
 * VPD access funcs
 * added by gkk886
 */
uint32_t vpmu_vpd_mpu_power(void);
uint32_t vpmu_vpd_dsp1_power(void);
uint32_t vpmu_vpd_dsp2_power(void);
uint32_t vpmu_vpd_lcd_power(void);
uint32_t vpmu_vpd_audio_power(void);
uint32_t vpmu_vpd_net_power(void);
uint32_t vpmu_vpd_gps_power(void);
uint32_t vpmu_vpd_total_power(void);

/*******************
 * VPMU management *
 *******************/
void vpmu_model_setup(VPMU*, uint32_t);
int8_t vpmu_simulator_status(VPMU*, uint32_t);
void vpmu_simulator_turn_on(VPMU*, uint32_t);
void vpmu_update_eet_epc(void);
//evo0209 fixed
void VPMU_update_slowdown_ratio(int);
/* reset all values in VPMU */
void VPMU_reset(void);
//evo0209
void VPMU_dump_result(void);
void VPMU_select_cpu_model(int);
void ResetCache(d4cache * d );
void VPMU_cache_init( d4cache* , d4cache ** , d4cache ** );
/* cache reference function */
void cache_ref( unsigned int , int ,int );
/* Support VPMU for Oprofile feature. */
/* Working only when CONFIG_VPMU_OPROFILE flag is ture. */
// Added by Chiaheng. 2010' 9' 23.
void vpmu_perf_counter_overflow_testing (void *);
void vpmu_perf_counter_overflow_handled (void *);

/********
 * MISC *
 ********/
void queue_insert(d4memref);
d4memref queue_del(void);
/* add commas to a number every 3 digits */
char *commaprint(unsigned long long);
d4cache* d4_mem_create(void);
void tb_extra_info_init(struct ExtraTBInfo *  , unsigned long );
/* branch prediction */
void branch_set(unsigned int);
void branch_update(unsigned int);
/* pipeline ex stage simulation */
int get_insn_ticks(uint32_t);
void get_insn_parser(unsigned int, struct ExtraTBInfo*);
int get_insn_ticks_thumb(uint32_t);
void put_event(uint32_t, uint32_t);

#if defined(CONFIG_PACDSP)
void vpmu_dsp_in(uint32_t);
void vpmu_dsp_out(uint32_t);
//void vpmu_suspend_qemu(void);
void vpmu_suspend_qemu_v2(void *);
/* deprecated */
uint32_t vpmu_dsp_considered_execution_time(void);

/*
 * pacduo power registers
 * added by gkk886
 */
struct pacduo_power_regs {
    uint32_t PLL1_CR; //system clk is (PLL1_CR[31:24]+1)*24 MHz
    uint32_t PLL1_SR;
    uint32_t CG_DDRTPR;
    uint32_t CG_DSP1TPR;
    uint32_t CG_DSP2TPR;
    uint32_t CG_AXITPR;
    uint32_t VC1_TRCR;
    uint32_t VC1_SR;
    uint32_t VC1_WDR;
    uint32_t VC1_CR;
    uint32_t VC2_TRCR;
    uint32_t VC2_SR;
    uint32_t VC2_WDR;
    uint32_t VC2_CR;
    uint32_t PMU_LCD_CR;
};

/*
 * pacduo dsp power model
 * added by gkk886
 */
double pacduo_dsp_volt(int);
int pacduo_dsp_freq(int);
int pacduo_mpu_freq(void);
double pacduo_dsp_power(int);

double pacduo_mpu_power(void);
#endif

#ifdef CONFIG_VPMU_THREAD
void VPMU_sync_all(void);
void *branch_thread(void *);
void *dinero_thread(void *);
#endif

/* TODO: remove it in future
 *       [TcTsaI]
 */
void register_VPMU_monitor( uint32_t base , uint32_t size );
#endif
