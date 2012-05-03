#include "SET/SET_event.h"
/*
 * "target_addr" is the target address of branch/jump instruction
 * "return_addr" is the return address of branch/jump instruction
 */
void HELPER(branch_detector)(uint32_t target_addr, uint32_t return_addr)
{
    SET_event_branch(target_addr, return_addr);
}


void HELPER(swi_detector)(uint32_t return_addr)
{
    SET_event_syscall(return_addr);
}
