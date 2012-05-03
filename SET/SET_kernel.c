#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SET_object.h"
#include "SET_kernel.h"

static FuncRange execve, mmap, fork;
//tianman
static FuncRange kthread;

int get_special_kernel_func(char *path, KerFunc *funcTable, int size)
{
    /* Using EFD to obtain the information about object file */
    EFD *efd = efd_open_elf(path);
    if (efd == NULL)
        return -1;

    /* Push special functions into hash table */
    uint32_t i;
    char *funcName;
    uint32_t vaddr;
    int idx;
    enum KerEvent event;
    for (i = 0; i < efd_get_sym_num(efd); i++)
    {
        if (IS_FUNC(efd, i))
        {
            vaddr = efd_get_sym_value(efd, i) & THUMB_INSN_MASK;
            funcName = efd_get_sym_name(efd, i);
            switch (funcName[0])
            {
                case 'd':
                    if (strcmp(funcName, "do_execve") == 0)
                    {
                        execve.start = vaddr;
                        execve.end = vaddr + efd_get_sym_size(efd, i);
                        continue;
                    }
                    else if (strcmp(funcName, "do_exit") == 0)
                    {
                        event = EXIT;
                        break;
                    }
                    else if (strcmp(funcName, "do_fork") == 0)
                    {
                        fork.start = vaddr;
                        fork.end = vaddr + efd_get_sym_size(efd, i);
                    }
                    else
                        continue;
                case 'm':
                    if (strcmp(funcName, "mmap_region") == 0)
                    {
                        mmap.start = vaddr;
                        mmap.end = vaddr + efd_get_sym_size(efd, i);
                    }
                    continue;
                case 's':
                    if (strcmp(funcName, "search_binary_handler") == 0)
                    {
                        event = EXECVE;
                        break;
                    }
                    else
                        continue;
                case 'v':
                    if (strcmp(funcName, "vma_wants_writenotify") == 0)
                    {
                        event = MMAP;
                        break;
                    }
					//tianman
					else if (strcmp(funcName, "vsnprintf") == 0)
					{
						event = KTHREAD;
						break;
					}
                    else
                        continue;
                case 'w':
                    if (strcmp(funcName, "wake_up_new_task") == 0)
                    {
                        event = FORK;
                        break;
                    }
                    else
                        continue;
                case '_':
                    if (strcmp(funcName, "__switch_to") == 0)
                    {
                        event = CS;
                        break;
                    }
                    else
                        continue;
				//tianman
				case 'k':
					if (strcmp(funcName, "kthread_create") == 0)
					{
						kthread.start = vaddr;
						kthread.end = vaddr + efd_get_sym_size(efd, i);
						continue;
					}
					else
						continue;

                default:
                    continue;
            }

            /* Linear probing for searching a free location */
            for (idx = HASH(vaddr, size); funcTable[idx].addr != 0; idx = (idx+1) % size)
                ;
            funcTable[idx].addr = vaddr;
            funcTable[idx].event = event;
        }
    }

    int num = 0;
    for (i = 0; i < KER_TABLE_SIZE; i++)
    {
        if (funcTable[i].addr != 0)
        {
            switch (funcTable[i].event)
            {
                case EXECVE:
                    if (execve.start == 0 || execve.end == 0 || execve.end - execve.start == 0)
                        return -1;
                    funcTable[i].retFunc = &execve;
                    break;
                case MMAP:
                    if (mmap.start == 0 || mmap.end == 0 || mmap.end - mmap.start == 0)
                        return -1;
                    funcTable[i].retFunc = &mmap;
                    break;
                case FORK:
                    if (fork.start == 0 || fork.end == 0 || fork.end - fork.start == 0)
                        return -1;
                    funcTable[i].retFunc = &fork;
                    break;
				//tianman
				case KTHREAD:
					if (kthread.start == 0 || kthread.end == 0 || kthread.end - kthread.start == 0)
						return -1;
					funcTable[i].retFunc = &kthread;
					break;
                default:
                    break;  /* Impossible */
            }
            num++;
        }
    }

    /* Check whether all the special kernel functions are parsed */
    if (num != KER_EVENT_NUM)
        return -1;
    else
        return 0;
}
