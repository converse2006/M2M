#ifndef SET_H
#define SET_H

#include <stdint.h>
#include <sys/queue.h>
#include "SET_out.h"
#include "SET_object.h"
#include "SET_dex.h"
#include "SET_kernel.h"
#include "SET_ctrl_sampling.h"

#define PROCESS_NAME_LEN    32
#define FULL_PATH_LEN       256
#define MAX_OBJ_TABLE_SIZE  256
#define MAX_DEX_TABLE_SIZE  256
#define MAX_PROC_TABLE_SIZE 1024

typedef uint8_t ProcessType;

/* Flags of process type */
#define SET_PROCESS_OUTPUT  0x01    /* Indicate tracing this process will create output traces */
#define SET_PROCESS_KERNEL  0x02
#define SET_PROCESS_SYSCALL 0x04
#define SET_PROCESS_LIBRARY 0x08
#define SET_PROCESS_USER_C  0x10    /* Indicate this process's entry point is from C program */
#define SET_PROCESS_DALVIK  0x20
#define SET_PROCESS_IRQ     0x40
#define SET_PROCESS_MMAP    0x80    /* Indicate tracing mmap event (SET_PROCESS_LIBRARY or SET_PROCESS_DALVIK imply this) */

/* Data structure for tracing process */
typedef struct SETProcess {
    ProcessType type;
    char        name[PROCESS_NAME_LEN];         /* process name */
    int         pid;
    SETOut      *output;
    int         *tracing;                       /* 0 means not in tracing. 1 means starting tracing
                                                   >1 means tracing shared among other processes */
    /* Kernel */
    char        path_kernel[FULL_PATH_LEN];
    ObjFile     *objKernel;
    Stack       *kernel;                        /* function call stack that stores return addresses in kernel space */

    /* Interrupt Context */
    Stack       *irq;                           /* interrupt request stack */
    Stack       *ic;                            /* function call stack that stores return addresses in Interrupt Context */

    /* User C */
    char        path_c[FULL_PATH_LEN];
    ObjFile     *objTable[MAX_OBJ_TABLE_SIZE];
    int         objTableSize;
    Stack       *user;                          /* function call stack that stores return addresses in user space */
    uint32_t    return_addr;                    /* return address for library calls */

    /* Dalvik */
    DexFile     *dexTable[MAX_DEX_TABLE_SIZE];
    int         dexTableSize;

    int         idx;
    void        *hash_pos;                      /* index of hash table */
} SETProcess;

typedef struct SET {
    int         is_start;
    KerFunc     kerFuncTable[KER_TABLE_SIZE];   /* Special kernel functions */
    SETProcess  *procTable[MAX_PROC_TABLE_SIZE];
    int         procTableSize;
    SETProcess  *tracing_process;
    int         model;                          /* Global, independent of SETProcess */
} SET;

extern SET set;
extern char path_android_out[FULL_PATH_LEN];
extern char path_set_out[FULL_PATH_LEN];
//evo0209
extern int trace_kernel;

void SET_init(void);
void SET_exit(void);

/* Control SET state */
int SET_has_process_being_tracing(void);

/* Control VPMU via SET */
void SET_start_vpmu(void);
void SET_stop_vpmu(void);

SETProcess *SET_create_process(char *, ProcessType);
SETProcess *SET_copy_process(SETProcess *);
void SET_delete_process(SETProcess *);

void SET_attach_process(SETProcess *);
void SET_detach_process(SETProcess *);

/* Control tracing process */
int SET_set_process_name(SETProcess *, char *);
int SET_set_process_type(SETProcess *, ProcessType);

void SET_attach_obj(SETProcess *, ObjFile *);
void SET_detach_obj(ObjFile *);

void SET_attach_dex(SETProcess *, DexFile *);
void SET_detach_dex(DexFile *);

//evo0209
void SET_trace_kernel(void);

#endif  /* SET_H */
