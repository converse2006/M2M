#ifndef SET_OUT_H
#define SET_OUT_H

#include <stdint.h>
#include "config-host.h"

#define MAX_BUF_SIZE    1024

/* VPMU information */
typedef struct VPMUinfo {
    uint64_t    i_count;
    uint64_t    ldst_count;
    double      dcache_access_count;
    uint64_t    dcache_miss_count;
    double      icache_access_count;
    uint64_t    icache_miss_count;
    uint64_t    br_count;
	//evo0209
	uint64_t	eet;
	uint64_t	eet_pipe;
	uint64_t	eet_sys_mem;
	uint64_t	eet_io;
    uint64_t    eet_net;
} VPMUinfo;

typedef struct Method {
    uint16_t    id;
    uint32_t    addr;
} Method;

typedef struct SETOut {
    char        *path;

    FILE        *fp_type;                   /* output stream for type of each trace record */
    FILE        *fp_cc;                     /* output stream for cycle counts */
	//tianman
	FILE		*fp_power;					/* output stream for power consumption */
    FILE        *fp_vpmu;                   /* output stream for other vpmu information */
    FILE        *fp_pid;                    /* output stream for pid */
    uint8_t     buf_type[MAX_BUF_SIZE];     /* output buffer for type */
    uint64_t    buf_cc[MAX_BUF_SIZE];       /* output buffer for cycle count */
	//tianman
	uint64_t	buf_power[MAX_BUF_SIZE];
    VPMUinfo    buf_vpmu[MAX_BUF_SIZE];     /* output buffer for other vpmu information */
    int         buf_pid[MAX_BUF_SIZE];      /* output buffer for pid */
    int         buf_global_size;            /* current buffer size for "type", "pid", "cc" and "vpmu" */

    FILE        *fp_obj;                    /* output stream for object path and corresponding id */
    FILE        *fp_func;                   /* output stream for object id and function index */
    uint32_t    buf_func[MAX_BUF_SIZE];     /* output buffer for object id and function index */
    int         buf_func_size;              /* current buffer size for "func" */
#ifdef CONFIG_SET_DWARF
    FILE        *fp_dwarf;                  /* output stream for DWARF information */
    uint32_t    buf_dwarf[MAX_BUF_SIZE*8];  /* output buffer for DWARF information
                                             * FIXME: assume the buffer size does not exceed MAX_BUF_SIZE*8 */
    int         buf_dwarf_size;             /* current buffer size for dwarf */
#endif

    FILE        *fp_dex;                    /* output stream for dex path and corresponding id */
    FILE        *fp_method;                 /* output stream for dex id and method address */
    Method      buf_method[MAX_BUF_SIZE];   /* output buffer for dex id and method address */
    int         buf_method_size;            /* current buffer size for "func" */

    FILE        *fp_syscall;                /* output stream for sytem call */
    uint32_t    buf_syscall[MAX_BUF_SIZE];  /* output buffer for system call */
    int         buf_syscall_size;           /* current buffer size for "syscall" */
} SETOut;

SETOut *SET_output_create(char *pathname);
void SET_output_close(SETOut *);

int SET_output_open_type_vpmu(SETOut *, int model);
int SET_output_open_pid(SETOut *);
int SET_output_open_obj_func(SETOut *);
#ifdef CONFIG_SET_DWARF
int SET_output_open_dwarf(SETOut *);
#endif
int SET_output_open_dex_method(SETOut *);
int SET_output_open_syscall(SETOut *);

/* FIXME write "pid" is redundant */
void SET_output_write_pid(SETOut *, int pid);
void SET_output_write_obj(SETOut *, uint16_t id, char *path);
void SET_output_write_func(SETOut *, uint32_t id_index);
#ifdef CONFIG_SET_DWARF
void SET_output_write_dwarf(SETOut *, uint32_t argv);
#endif
void SET_output_write_dex(SETOut *, uint16_t id, char *path);
void SET_output_write_method(SETOut *, uint16_t id, uint32_t addr);
void SET_output_write_syscall(SETOut *, uint32_t nr_syscall);

#endif  /* SET_OUT_H */
