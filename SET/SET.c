#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* access() */
#include "SET.h"
/* FIXME ifdef CONFIG_VPMU ? */
#include "vpmu.h"

//tianman
#include "SET_event.h"

SET set;
char path_android_out[FULL_PATH_LEN];
char path_set_out[FULL_PATH_LEN];
//evo0209
int trace_kernel = 0;

void SET_init()
{
	//tianman
	int ii;
	for (ii = 0; ii < 512; ii++)
		pid_base[ii][0] = -1;

    if (set.is_start)
    {
        fprintf(stderr, "SET device: Already started\n");
        return;
    }

    /* Specify the directory where object files (executable or library) located */
    char *out = getenv("ANDROID_PRODUCT_OUT");
    if (out == NULL || out[0] == 0 || access(out, F_OK))
    {
        fprintf(stderr, "SET device: init failed - invalid environment variable \"ANDROID_PRODUCT_OUT\"\n");
        return;
    }
    else
        strncpy(path_android_out, out, sizeof(path_android_out));

    out = getenv("SET_OUT");
    if (out == NULL || out[0] == 0 || access(out, F_OK))
    {
        fprintf(stderr, "SET device: init failed - invalid environment variable \"SET_OUT\"\n");
        return;
    }
    else
        strncpy(path_set_out, out, sizeof(path_set_out));

    memset(&set, 0, sizeof(SET));

    /* Get special kernel functions (used for dynamic instrumention of Linux kernel) */
    char kernel_path[FULL_PATH_LEN];
    snprintf(kernel_path, sizeof(kernel_path), "%s/vmlinux", path_set_out);
    if (get_special_kernel_func(kernel_path, set.kerFuncTable, KER_TABLE_SIZE) != 0)
    {
        fprintf(stderr, "SET device: init failed - unable to dynamic instrument kernel functions\n");
        return;
    }

    /* Succeed */
    set.is_start = 1;
    atexit(SET_exit);
    fprintf(stderr, "SET device: start\n");

    /* FIXME */
    SETProcess *process = SET_create_process("app_process", SET_PROCESS_MMAP);

	//tianman : for trace kernel
	if (trace_kernel)
	{
		set.model = 2;
    	SET_event_bootup();
	}

    if (process != NULL)
        SET_attach_process(process);
}

void SET_exit()
{
    if (!set.is_start)
        return;

    set.is_start = 0;

    int i;
    for (i = 0; i < set.procTableSize; i++)
        SET_delete_process(set.procTable[i]);

    set.procTableSize = 0;

    SET_stop_vpmu();

    fprintf(stderr, "SET device: exit\n");
}

int SET_has_process_being_tracing()
{
    SETProcess *process;
    int i;
    for (i = 0; i < set.procTableSize; i++)
    {
        process = set.procTable[i];

        if ((*(process->tracing) > 0) && (process->output != NULL))
            return 1;
    }
    return 0;
}

void SET_start_vpmu()
{
    /* If there are processes being tracing and VPMU already enabled, don't change VPMU model */
    if (VPMU_enabled)
        return;

    if (set.model == 0)
        return;

    VPMU_reset();
    switch(set.model)
    {
        case 1:
            //vpmu_model_setup(&GlobalVPMU, TIMING_MODEL_B);
			//evo0209
            vpmu_model_setup(&GlobalVPMU, TIMING_MODEL_F);
            VPMU_enabled = 1;
            fprintf(stderr, "SET device: Pipeline simulation\n");
            break;
        case 2:
            //vpmu_model_setup(&GlobalVPMU, TIMING_MODEL_D);
			//evo0209
            vpmu_model_setup(&GlobalVPMU, TIMING_MODEL_G);
            VPMU_enabled = 1;
            fprintf(stderr, "SET device: Pipeline and I/D cache simulation\n");
            break;
        case 3:
            vpmu_model_setup(&GlobalVPMU, TIMING_MODEL_E);
            VPMU_enabled = 1;
            fprintf(stderr, "SET device: Pipeline, I/D cache and branch simulation\n");
            break;
        default:
            VPMU_enabled = 0;
            set.model = 0;
            fprintf(stderr, "SET device: Wrong timing model!\n");
            return;
    }
}

void SET_stop_vpmu()
{
    if (VPMU_enabled)
    {
        VPMU_enabled = 0;
		VPMU_dump_result();
        fprintf(stderr, "SET device: Disable VPMU\n");
    }
}

/* Check whether the C binary of the process exists in host OS or not */
static int SET_set_C_binary(SETProcess *process)
{
    /* Absolute path of this program */
    snprintf(process->path_c, sizeof(process->path_c), "%s/%s", path_set_out, process->name);
    if (access(process->path_c, F_OK))
    {
        snprintf(process->path_c, sizeof(process->path_c), "%s/symbols/system/bin/%s", path_android_out, process->name);
        if (access(process->path_c, F_OK))
        {
            fprintf(stderr, "SET device: Cannot find C program \"%s\"\n", process->name);
            return -1;
        }
    }
    return 0;
}

/* Check whether the kernel binary exists in host OS or not */
static int SET_set_kernel_binary(SETProcess *process)
{
    snprintf(process->path_kernel, sizeof(process->path_kernel), "%s/vmlinux", path_set_out);
    if (access(process->path_kernel, F_OK))
    {
        fprintf(stderr, "SET device: Cannot find \"vmlinux\"\n");
        return -1;
    }
    return 0;
}

SETProcess *SET_create_process(char *name, ProcessType type)
{
    SETProcess *process;
    char output_path[FULL_PATH_LEN];

    if (name == NULL)
    {
        fprintf(stderr, "SET device: Invaild name!\n");
        return NULL;
    }

    /* Create process */
    process = (SETProcess *) malloc(sizeof(SETProcess));
    if (process == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate SETProcess!\n");
        return NULL;
    }
    memset(process, 0, sizeof(SETProcess));

    process->pid = -1;
    process->idx = -1;

    /* Create tracing count */
    process->tracing = (int *) malloc(sizeof(int));
    *process->tracing = 0;

    if (SET_set_process_type(process, type) != 0)
    {
        SET_delete_process(process);
        return NULL;
    }

    if (SET_set_process_name(process, name) != 0)
    {
        SET_delete_process(process);
        return NULL;
    }

    /* Create output stream */
    if (process->type & SET_PROCESS_OUTPUT)
    {
        snprintf(output_path, sizeof(output_path), "%s/%s.set", path_set_out, process->name);
        process->output = SET_output_create(output_path);
        if (process->output == NULL)
        {
            SET_delete_process(process);
            return NULL;
        }
    }

    return process;
}

SETProcess *SET_copy_process(SETProcess *process_src)
{
    SETProcess *process_dest;

    if (process_src == NULL)
        return NULL;

    /* Create process */
    process_dest = (SETProcess *) malloc(sizeof(SETProcess));
    if (process_dest == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate SETProcess!\n");
        return NULL;
    }

    memcpy(process_dest, process_src, sizeof(SETProcess));

    (*process_dest->tracing)++;

    /* Private data */
    if (process_src->kernel)
    {
        process_dest->kernel = (Stack *) calloc(1, sizeof(Stack));
        if (process_dest->kernel == NULL)
            goto fail;
    }
    if (process_src->irq)
    {
        process_dest->irq = (Stack *) calloc(1, sizeof(Stack));
        if (process_dest->irq == NULL)
            goto fail;
    }
    if (process_src->ic)
    {
        process_dest->ic = (Stack *) calloc(1, sizeof(Stack));
        if (process_dest->ic == NULL)
            goto fail;
    }
    if (process_src->user)
    {
        process_dest->user = (Stack *) calloc(1, sizeof(Stack));
        if (process_dest->user == NULL)
            goto fail;
    }

    return process_dest;

fail:
    fprintf(stderr, "SET device: No memory space to allocate Stack!\n");
    SET_delete_process(process_dest);
    return NULL;
}

void SET_delete_process(SETProcess *process)
{
    if (process == NULL)
        return;

    if (*process->tracing > 0)
        fprintf(stderr, "SET device: End tracing process \"%s\", pid: %d\n", process->name, process->pid);

    (*process->tracing)--;

    /* Free private data */
    if (process->kernel)
        free(process->kernel);
    if (process->irq)
        free(process->irq);
    if (process->ic)
        free(process->ic);
    if (process->user)
        free(process->user);

    /* Check if other shared processes (threads) still running. If no, free "shared" resources */
    /* 0 means no other threads, -1 means deleting before start tracing */
    if (*process->tracing < 1)
    {
        /* Kernel object */
        if (process->objKernel)
        {
            process->objKernel->count--;
            SET_delete_obj(process->objKernel);
        }

        /* User space object */
        int i;
        for (i = 0; i < process->objTableSize; i++)
        {
            process->objTable[i]->count--;
            SET_delete_obj(process->objTable[i]);
        }

        /* Dex */
        for (i = 0; i < process->dexTableSize; i++)
        {
            process->dexTable[i]->count--;
            SET_delete_dex(process->dexTable[i]);
        }

        /* Output */
        if (process->output)
            SET_output_close(process->output);

        /* Reference count */
        free(process->tracing);
    }

    free(process);
}

/* Add tracing process to the list */
void SET_attach_process(SETProcess *process)
{
    if (set.procTableSize == MAX_PROC_TABLE_SIZE)
        return;

    set.procTable[set.procTableSize] = process;
    process->idx = set.procTableSize;
    set.procTableSize++;
}

void SET_detach_process(SETProcess *process)
{
    if (set.procTableSize <= 0)
        return;

    int i = process->idx;
    if (i <= 0)
        return;

    /* To remove an entry in 'i' place, shift entries on its right to the left by one */
    for (; i < set.procTableSize; i++)
        set.procTable[i] = set.procTable[i+1];

    process->idx = -1;
    set.procTableSize--;
}

int SET_set_process_type(SETProcess *process, ProcessType type)
{
    /* Impossible process type flags */

    ProcessType changedType = type & (type ^ process->type);
    /* Check C binary if process name has been set and type flag "C" will be changed and set */
    if ((changedType & SET_PROCESS_USER_C) && (process->name[0] != '\0'))
        if ((SET_set_C_binary(process) != 0))
            return -1;

    if (changedType & (SET_PROCESS_KERNEL | SET_PROCESS_IRQ))
        if ((SET_set_kernel_binary(process) != 0))
            return -1;

    process->type = type;

    return 0;
}

int SET_set_process_name(SETProcess *process, char *name)
{
    int i, ch;

    /* Copy the process name from after last slash */
    for (i = 0; (ch = *(name++)) != '\0';)
    {
        if (ch == '/')
            i = 0;  /* overwrite what we wrote */
        else if (i < (sizeof(process->name) - 1))
            process->name[i++] = ch;
    }
    process->name[i] = '\0';

    /* Avoid duplicated process names */
    for (i = 0; i < set.procTableSize; i++)
    {
        if (strcmp(process->name, set.procTable[i]->name) == 0)
        {
            fprintf(stderr, "SET device: Duplicated process name %s!\n", process->name);
            return -1;
        }
    }

    if (process->type & SET_PROCESS_USER_C)
        if (SET_set_C_binary(process) != 0)
            return -1;

    return 0;
}

void SET_attach_obj(SETProcess *process, ObjFile *obj)
{
    if (process->objTableSize == MAX_OBJ_TABLE_SIZE)
        return;

    if (process->output)
        SET_output_write_obj(process->output, obj->id, obj->path);

    obj->count++;

    /* Linear search */
    int i, j;
    for (i = 0; i < process->objTableSize; i++)
    {
        if (obj->text_start < process->objTable[i]->text_start)
        {
            /* To insert an entry in 'i' place, shift entries on its right to the right by one */
            for (j = process->objTableSize; j > i; j--)
                process->objTable[j] = process->objTable[j-1];

            break;
        }
    }
    process->objTable[i] = obj;
    process->objTableSize++;
}

/* FIXME munmap() */
void SET_detach_obj(ObjFile *obj)
{
    obj->count--;
}

void SET_attach_dex(SETProcess *process, DexFile *dex)
{
    if (process->dexTableSize == MAX_DEX_TABLE_SIZE)
        return;

    if (process->output)
        SET_output_write_dex(process->output, dex->id, dex->path);

    dex->count++;

    /* Linear search */
    int i, j;
    for (i = 0; i < process->dexTableSize; i++)
    {
        if (dex->start < process->dexTable[i]->start)
        {
            /* To insert an entry in 'i' place, shift entries on its right to the right by one */
            for (j = process->dexTableSize; j > i; j--)
                process->dexTable[j] = process->dexTable[j-1];

            break;
        }
    }
    process->dexTable[i] = dex;
    process->dexTableSize++;
}

/* FIXME munmap() */
void SET_detach_dex(DexFile *dex)
{
    dex->count--;
}

//evo0209
void SET_trace_kernel()
{
	trace_kernel = 1;
}
