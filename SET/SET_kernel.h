/* Dynamic instrumentation of kernel functions */
#ifndef SET_KERNEL_H
#define SET_KERNEL_H

#include <stdint.h>

enum KerEvent {
    EXECVE,
    EXIT,
    CS,
    MMAP,
    FORK,
//tianman
	KTHREAD
};

//tianman
//#define KER_EVENT_NUM 5
#define KER_EVENT_NUM 6

typedef struct FuncRange {
    uint32_t    start;
    uint32_t    end;
} FuncRange;

typedef struct KerFunc {
    uint32_t    addr;
    enum KerEvent event;
    FuncRange   *retFunc;
} KerFunc;

#define KER_TABLE_SIZE 83

int get_special_kernel_func(char *path, KerFunc *funcTable, int size);

#endif
