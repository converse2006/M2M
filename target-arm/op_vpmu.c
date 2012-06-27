/* paslab : helper to caclulate cache reference */
#ifdef CONFIG_VND
#include "m2m.h"

#ifdef M2M_VPMU
extern VND GlobalVND;
extern long m2m_localtime_start[MAX_NODE_NUM];
extern int vpmu_trigger;
#endif 

#endif

void HELPER(dinero_access)(uint32_t addr,uint32_t rw, uint32_t size)
{
	if(VPMU_enabled)
	{
		/* we first calculate IRQ into account
		   int mode = env->uncached_cpsr & CPSR_M;
		   if(mode == ARM_CPU_MODE_IRQ)
		   return;
		 */

#if 0
		char *state = &(GlobalVPMU.state);
		char *timer_interrupt_exception = &(GlobalVPMU.timer_interrupt_exception);
		/* In timer interrupt state */
		if(unlikely(*state == 1 && *timer_interrupt_exception == 0)) {
			return;
		}
#endif

		if(vpmu_simulator_status(&GlobalVPMU, VPMU_DCACHE_SIM)) {
			if(rw == 0xff)/* PLD */
			{
				//cache_ref(env->regs[rd]+shift, D4XREAD , 32);
				addr = env->regs[(addr>>16)&0xf] + (addr&0xfff);
				rw = D4XREAD;
			}

			//evo0209
			if (GlobalVPMU.cpu_model == 1)
			{
				//uint32_t pa = cpu_get_phys_page_debug(env, addr);
				//uint32_t pt = pa & 0xFFFFE000; //cortex-a9 L1 cache has 256-entry, blocksize 32bytes, mask 13bits
				//addr = (addr & 0x00001FFF) | pt;
				addr = cpu_get_phys_page_debug(env, addr);
			}

			//evo0209 temp test
			if (GlobalVPMU.iomem_test == 1)
			{
				//printf("helper I/O addr: %x", addr);
				GlobalVPMU.iomem_test = 0;
				GlobalVPMU.iomem_qemu++;
			}
			else
				cache_ref(addr, rw, size);


			//chiaheng
#if 0       /* Considering the performance impact of I/O memory accesses. */ 
			if(cpu_get_phys_page_debug(env, addr) >= SYSTEM_RAM_START && 
					cpu_get_phys_page_debug(env, addr) < SYSTEM_RAM_END){
				/* The cache memory simulation. */
				cache_ref(addr, rw, size);
			}
			else {
				/* Accounting for I/O memory accesses. */
				GlobalVPMU.iomem_count++;
#if 0           /* Debuggin. */
				if (cpu_get_phys_page_debug(env, addr) >= VPMU_BASE_ADDR
						&& cpu_get_phys_page_debug(env, addr) < (VPMU_BASE_ADDR+VPMU_IOMEM_SIZE)) {
					printf("%s: Address (virtual, physical) = (%x, %x).\n", 
							__FUNCTION__, addr, cpu_get_phys_page_debug(env, addr));
					//fflush(stdout);
				}
#endif
			}
#endif
		}
	}
}

/* branch predictor */
void HELPER(branch_predictor)(uint32_t cur_sig)
{
    if(VPMU_enabled)
        if(vpmu_simulator_status(&GlobalVPMU, VPMU_BRANCH_SIM))
            put_event(0xb, cur_sig);
}

/* pacduo linux entry point of timer interrupt */
/* Top half = pac_timer_interrupt() and bottom half = run_timer_softirq() */
/*
#define ASM_DO_IRQ 0xc001f000
#define PAC_TIMER_INTERRUPT 0xc0027d20
#define RUN_KSOFTIRQD 0xc0034a30
#define RUN_TIMER_SOFTIRQ 0xc00396ec
*/
uint32_t ISR_addr[NUM_FIND_SYMBOLS]={};
/* paslab : helper to calculate counter */
void HELPER(vpmu_calculate)(void *opaque)
{
    static unsigned int previous_pc;
	struct ExtraTBInfo *extra_tb_info = (struct ExtraTBInfo *)opaque;
	int mode = env->uncached_cpsr & CPSR_M;
    /* state = 1 (in timer interrupt) */
    //static char state = 0;
    char *state = &(GlobalVPMU.state);
    //static unsigned int timer_interrupt_icount = 0;
    static uint32_t return_addr;
    static uint32_t last_issue_time = 0;

	if(env && VPMU_enabled)
	{
        /* catch when arm is busy */
        /*
        if(vpmu_simulator_status(&GlobalVPMU, VPMU_SAMPLING))
            vpmu_dump_machine_state_v2(&GlobalVPMU);
            */

        /*  Tim's legacy code */
        /*
		if( GlobalVPMU.swi_fired == 1 ) {
			//unsigned int insn ;
			if( mode == ARM_CPU_MODE_USR )
			{
				//GlobalVPMU.swi_fired = 0 ;
				//insn = ldl_code(extra_tb_info->pc -4 );
				if( extra_tb_info->pc == GlobalVPMU.swi_fired_pc )
				{
					//fprintf(stderr , "bingo , a previous matched pc\n" );
					//if( ( (( insn >> 24 ) & 0xf) == 0xf  )  ){
					GlobalVPMU.swi_fired = 0 ;
					GlobalVPMU.swi_enter_exit_count += 1 ;
					//  fprintf(stderr , "match swi do _cancel_swi() \n" );
					//}else{
					//  fprintf(stderr , "what?, match but not after swi!!\n" );
					//}

				}
			}
		}
        */

#if 0
        /* TODO: this mechanism should be wrapped */
        /* asm_do_IRQ handles all the hardware interrupts, not only for timer interrupts
         * pac_timer_interrupt() is the timer handler which asm_do_IRQ will call
         * run_softirqd is softirq handler in older Linux version
         * Linux handle arm interrupt with run_timer_softirq()*/
        //static char timer_interrupt_exception = 0;
        char *timer_interrupt_exception = &(GlobalVPMU.timer_interrupt_exception);

        if(env->regs[15] == ISR_addr[TICK_SCHED_TIMER])
        //if(env->regs[15] == ISR_addr[RUN_TIMER_SOFTIRQ]
        //    || env->regs[15] == ISR_addr[GOLDFISH_TIMER_INTERRUPT])
        {
            if(*state == 0) {
                /* calibrate the timer interrupt */
                int64_t tmp;
                tmp = vpmu_estimated_execution_time() - last_issue_time;
                /* timer interrupt interval = 10 ms = 10000 us */
                if(tmp > TIMER_INTERRUPT_INTERVAL) {
                    last_issue_time = vpmu_estimated_execution_time();
                    *timer_interrupt_exception = 1;
                }
                /* The link register in asm_do_irq are being cleared
                 * so we cannot use env->regs[14] directly */
                return_addr = GlobalVPMU.timer_interrupt_return_pc;
                //printf("vpmu remembered return PC=0x%x\n",return_addr);
            }
            *state = 1;
        }

        /* In timer interrupt state */
        if(unlikely(*state == 1)) {
            /* check if timer interrupt is returned */
            if(unlikely((return_addr - 4) == env->regs[15])) {
                /* timer interrupt returned */
                *state = 0;
                *timer_interrupt_exception = 0;
            } else {
                /* still in the timer interrupt stage
                 * Prevent timer interrupt to be counted, must return
                 */
                if(*timer_interrupt_exception == 0)
                    return;
            }
        }
#endif

        /* this TB is executed repeatly, we don't feed reference into cache
         * we calculate additional access in VPMU instead
         */
        if(vpmu_simulator_status(&GlobalVPMU, VPMU_ICACHE_SIM)) {
            if(previous_pc == extra_tb_info->icache_ref.address) {
                GlobalVPMU.additional_icache_access
                    += extra_tb_info->icache_ref.size;
            }
            else {
                previous_pc = extra_tb_info->icache_ref.address;

                cache_ref( extra_tb_info->icache_ref.address
                        , extra_tb_info->icache_ref.accesstype
                        , extra_tb_info->icache_ref.size );
            }
        }

        if(vpmu_simulator_status(&GlobalVPMU, VPMU_PIPELINE_SIM)) {
			//evo0209
			if (GlobalVPMU.cpu_model == 0)
				GlobalVPMU.ticks += extra_tb_info->ticks;
			else
			{
				//tianman
				int i;
#ifdef CONFIG_VPMU_VFP
				GlobalVPMU.VFP_BASE += extra_tb_info->vfp_base;
				for(i = 0; i < ARM_VFP_INSTRUCTIONS; i++)
				{
					GlobalVPMU.VFP_count[i] += extra_tb_info->vfp_count[i];
				}
#endif
				//for(i = 0; i < ARM_INSTRUCTIONS + 2; i++)
				//{
				//	//GlobalVPMU.ARM_count[i] += extra_tb_info->arm_count[i];
				//	GlobalVPMU.ticks += extra_tb_info->arm_count[i];
				//	//printf("%llu\n", GlobalVPMU.ARM_count[i]);
				//}
				//evo0209
				GlobalVPMU.ticks += extra_tb_info->ticks;
				//printf("OP_VPMU Ticks %llu, tb_ticks %llu, reduce %llu\n"
				//		, GlobalVPMU.ticks
				//		, extra_tb_info->ticks
				//		, extra_tb_info->dual_issue_reduce_ticks);
				GlobalVPMU.reduce_ticks += extra_tb_info->dual_issue_reduce_ticks;
				//printf("OP_VPMU Ticks %llu, reduce %llu\n", GlobalVPMU.ticks, GlobalVPMU.reduce_ticks);
				GlobalVPMU.ticks -= extra_tb_info->dual_issue_reduce_ticks;
			}
		}

        if(vpmu_simulator_status(&GlobalVPMU, VPMU_INSN_COUNT_SIM)) {

            GlobalVPMU.branch_count += extra_tb_info->branch_count;
            GlobalVPMU.all_changes_to_PC_count += extra_tb_info->all_changes_to_PC_count;

            if ( mode == ARM_CPU_MODE_USR ) {
                GlobalVPMU.USR_icount += extra_tb_info->insn_count ;
                GlobalVPMU.USR_load_count += extra_tb_info->load_count ;
                GlobalVPMU.USR_store_count += extra_tb_info->store_count ;
                GlobalVPMU.USR_ldst_count += (extra_tb_info->load_count
                                            + extra_tb_info->store_count);
            } else if( mode == ARM_CPU_MODE_SVC ) {
                if( GlobalVPMU.swi_fired == 1 ) {
                    /* TODO: remove these useless code */
                    GlobalVPMU.sys_call_icount += extra_tb_info->insn_count ;
                    GlobalVPMU.sys_call_load_count += extra_tb_info->load_count ;
                    GlobalVPMU.sys_call_store_count += extra_tb_info->store_count ;
                    GlobalVPMU.sys_call_ldst_count += (extra_tb_info->load_count
                                            + extra_tb_info->store_count);
                } else {
                    GlobalVPMU.SVC_icount += extra_tb_info->insn_count ;
                    GlobalVPMU.SVC_load_count += extra_tb_info->load_count ;
                    GlobalVPMU.SVC_store_count += extra_tb_info->store_count ;
                    GlobalVPMU.SVC_ldst_count += (extra_tb_info->load_count
                                            + extra_tb_info->store_count);
                }
            } else if(mode == ARM_CPU_MODE_IRQ) {
                    GlobalVPMU.IRQ_icount += extra_tb_info->insn_count;
                    GlobalVPMU.IRQ_load_count += extra_tb_info->load_count;
                    GlobalVPMU.IRQ_store_count += extra_tb_info->store_count;
                    GlobalVPMU.IRQ_ldst_count += (extra_tb_info->load_count
                                            + extra_tb_info->store_count);
            } else {
                GlobalVPMU.sys_rest_icount += extra_tb_info->insn_count ;
                GlobalVPMU.sys_rest_load_count += extra_tb_info->load_count ;
                GlobalVPMU.sys_rest_store_count += extra_tb_info->store_count ;
                GlobalVPMU.sys_rest_ldst_count += (extra_tb_info->load_count
                                            + extra_tb_info->store_count);
            }
        }
#ifdef CONFIG_VND
#ifdef M2M_VPMU
        if(vpmu_trigger != 0)
        {
            volatile uint64_t *m2m_localtime = (uint64_t *)m2m_localtime_start[GlobalVND.DeviceID];
            *m2m_localtime = vpmu_estimated_execution_time_ns();
        }
#endif
#endif
	}
}

