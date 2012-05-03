/* FIXME DWARF */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* access() */
#include <sys/stat.h> /* mkdir() */
#include <sys/types.h> /* mkdir() */
#include "SET_out.h"

#include "vpmu.h"

#define SET_OUTPUT_FUNC     0x1
#define SET_OUTPUT_METHOD   0x2
#define SET_OUTPUT_SYSCALL  0x3

static inline void SET_output_buffer_flush_pid(SETOut *output)
{
    fwrite(output->buf_pid, 4, output->buf_global_size, output->fp_pid);
}

static inline void SET_output_buffer_flush_type_vpmu(SETOut *output)
{
    /* FIXME */
    SET_output_buffer_flush_pid(output);

    fwrite(output->buf_type, 1, output->buf_global_size, output->fp_type);

    if (output->fp_cc)
    {
        fwrite(output->buf_cc, 8, output->buf_global_size, output->fp_cc);

		//tianman
		if (output->fp_power)
	        fwrite(output->buf_power, 8, output->buf_global_size, output->fp_power);

        if (output->fp_vpmu)
            fwrite(output->buf_vpmu, sizeof(VPMUinfo), output->buf_global_size, output->fp_vpmu);
    }
    output->buf_global_size = 0;
}

static inline void SET_output_buffer_flush_func(SETOut *output)
{
    fwrite(output->buf_func, 4, output->buf_func_size, output->fp_func);
    output->buf_func_size = 0;
}

static inline void SET_output_buffer_flush_method(SETOut *output)
{
    fwrite(output->buf_method, sizeof(Method), output->buf_method_size, output->fp_method);
    output->buf_method_size = 0;
}

static inline void SET_output_buffer_flush_syscall(SETOut *output)
{
    fwrite(output->buf_syscall, 4, output->buf_syscall_size, output->fp_syscall);
    output->buf_syscall_size = 0;
}

static inline void SET_output_buffer_flush_all(SETOut *output)
{
    SET_output_buffer_flush_type_vpmu(output);

    if (output->fp_func)
        SET_output_buffer_flush_func(output);

    if (output->fp_method)
        SET_output_buffer_flush_method(output);

#ifdef CONFIG_SET_DWARF
    fwrite(output->buf_dwarf, 4, output->buf_dwarf_size, output->fp_dwarf);
    output->buf_dwarf_size = 0;
#endif

    if (output->fp_syscall)
        SET_output_buffer_flush_syscall(output);
}


SETOut *SET_output_create(char *pathname)
{
    SETOut *output;
    int len;

    /* Remove a file. If "pathname" is an non-empty directory, not removed */
    if (access(pathname, F_OK) == 0)
        remove(pathname);

    mkdir(pathname, 0755);

    output = (SETOut *) malloc(sizeof(SETOut));
    if (output == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate Event output!\n");
        return NULL;
    }
    memset(output, 0, sizeof(SETOut));

    len = strlen(pathname);
    output->path = (char*) malloc(len+1);
    strncpy(output->path, pathname, len+1);

    return output;
}

void SET_output_close(SETOut *output)
{
    /* Write nonempty buffer to disk */
    SET_output_buffer_flush_all(output);

    if (output->fp_type)
        fclose(output->fp_type);

    if (output->fp_cc)
        fclose(output->fp_cc);

	//tianman
	if (output->fp_power)
		fclose(output->fp_power);
    
    if (output->fp_vpmu)
        fclose(output->fp_vpmu);

    if (output->fp_pid)
        fclose(output->fp_pid);

    if (output->fp_obj)
        fclose(output->fp_obj);

    if (output->fp_func)
        fclose(output->fp_func);

#ifdef CONFIG_SET_DWARF
    if (output->fp_dwarf)
        fclose(output->fp_dwarf);
#endif

    if (output->fp_dex)
        fclose(output->fp_dex);

    if (output->fp_method)
        fclose(output->fp_method);

    if (output->fp_syscall)
        fclose(output->fp_syscall);

    free(output->path);
    free(output);
}

int SET_output_open_type_vpmu(SETOut *output, int model)
{
    char path[256], path_vpmu[256], path_power[256];

    snprintf(path, sizeof(path), "%s/type", output->path);
    output->fp_type = fopen(path, "w");
    if (output->fp_type == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing type\n");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/cc", output->path);
    snprintf(path_vpmu, sizeof(path_vpmu), "%s/vpmu", output->path);
    snprintf(path_power, sizeof(path_power), "%s/power", output->path);

    switch(model)
    {
        case 3:
        case 2:
        case 1:
			//evo0209
            output->fp_vpmu = fopen(path_vpmu, "w");
            if (output->fp_vpmu == NULL)
            {
                fprintf(stderr, "SET device: Cannot open a file for writing VPMU information\n");
                return -1;
            }

            output->fp_cc = fopen(path, "w");
            if (output->fp_cc == NULL)
            {
                fprintf(stderr, "SET device: Cannot open a file for writing cycle counts\n");
                return -1;
            }
           
			//tianman
			output->fp_power = fopen(path_power, "w");
            if (output->fp_power == NULL)
            {
                fprintf(stderr, "SET device: Cannot open a file for writing power consumption\n");
                return -1;
            }
        case 0:
            break;
        default:
            fprintf(stderr, "SET device: Wrong timing model!\n");
            return -1;
    }

    /* Remove "[event].set/cc" if exists */
    if (output->fp_cc == NULL && access(path, F_OK) == 0)
        remove(path);

    /* Remove "[event].set/vpmu" if exists */
    if (output->fp_vpmu == NULL && access(path_vpmu, F_OK) == 0)
        remove(path_vpmu);
    
	//tianman
	/* Remove "[event].set/power" if exists */
    if (output->fp_power == NULL && access(path_power, F_OK) == 0)
        remove(path_power);
    
    return 0;
}

int SET_output_open_pid(SETOut *output)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/pid", output->path);
    output->fp_pid = fopen(path, "w");
    if (output->fp_pid == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing pid\n");
        return -1;
    }

    return 0;
}

int SET_output_open_obj_func(SETOut *output)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/obj", output->path);
    output->fp_obj = fopen(path, "w");
    if (output->fp_obj == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing object path and id\n");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/func", output->path);
    output->fp_func = fopen(path, "w");
    if (output->fp_func == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing object id and function index\n");
        return -1;
    }

    return 0;
}

#ifdef CONFIG_SET_DWARF
int SET_output_open_dwarf(SETOut *output)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/dwarf", output->path);
    output->fp_dwarf = fopen(path, "w");
    if (output->fp_dwarf == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing DWARF information\n");
        return -1;
    }

    return 0;
}
#endif

int SET_output_open_syscall(SETOut *output)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/syscall", output->path);
    output->fp_syscall = fopen(path, "w");
    if (output->fp_syscall == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing system call #\n");
        return -1;
    }

    return 0;
}

int SET_output_open_dex_method(SETOut *output)
{
    char path[256];

    snprintf(path, sizeof(path), "%s/dex", output->path);
    output->fp_dex = fopen(path, "w");
    if (output->fp_dex == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing dex path and id\n");
        return -1;
    }

    snprintf(path, sizeof(path), "%s/method", output->path);
    output->fp_method = fopen(path, "w");
    if (output->fp_method == NULL)
    {
        fprintf(stderr, "SET device: Cannot open a file for writing dex id and method address\n");
        return -1;
    }

    return 0;
}

void SET_output_write_pid(SETOut *output, int pid)
{
    output->buf_pid[output->buf_global_size] = pid;
}

static inline void SET_output_write_type_vpmu(SETOut *output, uint8_t type)
{
    output->buf_type[output->buf_global_size] = type;

    if (output->fp_cc)
    {
        output->buf_cc[output->buf_global_size] = vpmu_cycle_count();
		
		//tianman
		if (output->fp_power)
			output->buf_power[output->buf_global_size] = vpmu_vpd_mpu_power();

        if (output->fp_vpmu)
        {
            output->buf_vpmu[output->buf_global_size].i_count = vpmu_total_insn_count();
            output->buf_vpmu[output->buf_global_size].ldst_count = vpmu_total_ldst_count();
            output->buf_vpmu[output->buf_global_size].dcache_access_count = vpmu_L1_dcache_access_count();
            output->buf_vpmu[output->buf_global_size].dcache_miss_count = vpmu_L1_dcache_miss_count();
            output->buf_vpmu[output->buf_global_size].icache_access_count = vpmu_L1_icache_access_count();
            output->buf_vpmu[output->buf_global_size].icache_miss_count = vpmu_L1_icache_miss_count();
            output->buf_vpmu[output->buf_global_size].br_count = vpmu_branch_insn_count();
			//evo0209
            output->buf_vpmu[output->buf_global_size].eet = vpmu_estimated_execution_time_ns();
            output->buf_vpmu[output->buf_global_size].eet_pipe = vpmu_estimated_pipeline_execution_time_ns();
            output->buf_vpmu[output->buf_global_size].eet_sys_mem = vpmu_estimated_sys_memory_access_time_ns();
            output->buf_vpmu[output->buf_global_size].eet_io = vpmu_estimated_io_memory_access_time_ns();
        }
    }
    output->buf_global_size++;
    if (output->buf_global_size == MAX_BUF_SIZE)
        SET_output_buffer_flush_type_vpmu(output);
}

void SET_output_write_obj(SETOut *output, uint16_t id, char *path)
{
    fwrite(&id, sizeof(uint16_t), 1, output->fp_obj);
    fwrite(path, sizeof(char), strlen(path)+1, output->fp_obj);
}

void SET_output_write_func(SETOut *output, uint32_t id_index)
{
    output->buf_func[output->buf_func_size] = id_index;
    output->buf_func_size++;
    if (output->buf_func_size == MAX_BUF_SIZE)
        SET_output_buffer_flush_func(output);

    SET_output_write_type_vpmu(output, SET_OUTPUT_FUNC);
}

#ifdef CONFIG_SET_DWARF
void SET_output_write_dwarf(SETOut *output, uint32_t argv)
{
    output->buf_dwarf[output->buf_dwarf_size++] = argv;
}
#endif

void SET_output_write_dex(SETOut *output, uint16_t id, char *path)
{
    fwrite(&id, sizeof(uint16_t), 1, output->fp_dex);
    fwrite(path, sizeof(char), strlen(path)+1, output->fp_dex);
}

void SET_output_write_method(SETOut *output, uint16_t id, uint32_t addr)
{
    output->buf_method[output->buf_method_size].id = id;
    output->buf_method[output->buf_method_size].addr = addr;
    output->buf_method_size++;
    if (output->buf_method_size == MAX_BUF_SIZE)
        SET_output_buffer_flush_method(output);

    SET_output_write_type_vpmu(output, SET_OUTPUT_METHOD);
}

void SET_output_write_syscall(SETOut *output, uint32_t nr_syscall)
{
    output->buf_syscall[output->buf_syscall_size] = nr_syscall;
    output->buf_syscall_size++;
    if (output->buf_syscall_size == MAX_BUF_SIZE)
        SET_output_buffer_flush_syscall(output);

    SET_output_write_type_vpmu(output, SET_OUTPUT_SYSCALL);
}
