#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>  /* open() */
#include <unistd.h> /* close() */
#include "libelf.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "SET_dwarf.h"

#define WORD_MASK 0xfffffffc

static uint32_t *local_func_addr;
static int local_func_len;
static uint32_t local_offset;
static int local_func_idx;

static Func_DWARF *func_dwarf;

/* Get the size (in words) */
static int get_size(Dwarf_Debug dbg, Dwarf_Die die)
{
    Dwarf_Attribute attr;
    Dwarf_Unsigned size;
    int ret;

    ret = dwarf_attr(die, DW_AT_byte_size, &attr, NULL);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_attr()\n");
        return 1;
    }

    ret = dwarf_formudata(attr, &size, NULL);
    dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_formudata()\n");
        return 1;
    }

    /* Increment the parameter number of the function */
    func_dwarf[local_func_idx].param_size += (size + 3) >> 2;

    return 0;
}

static int get_type(Dwarf_Debug dbg, Dwarf_Die die)
{
    Dwarf_Attribute attr;
    Dwarf_Off off;
    Dwarf_Die type_die;
    Dwarf_Half tag;
    int ret;

    ret = dwarf_attr(die, DW_AT_type, &attr, NULL);
    if (ret == DW_DLV_ERROR)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_attr()\n");
        return 1;
    }
    if (ret == DW_DLV_NO_ENTRY)
        return 0;

    /* "DW_AT_type" is of the form "DW_FORM_ref4" */
    ret = dwarf_global_formref(attr, &off, NULL);
    dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_global_formref()\n");
        return 1;
    }

    /* Get the referenced DIE which is the type of "die" */
    ret = dwarf_offdie(dbg, off, &type_die, NULL);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_offdie()\n");
        return 1;
    }

    /* Get the type */
    ret = dwarf_tag(type_die, &tag, NULL);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_tag()\n");
        dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
        return 1;
    }

    switch (tag)
    {
        case DW_TAG_base_type:
            ret = get_size(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;

        case DW_TAG_pointer_type:
            /* Recursive call */
            /* FIXME */
//          get_type(dbg, type_die);
            ret = get_size(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;
        case DW_TAG_structure_type:
            ret = get_size(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;
        case DW_TAG_union_type:
            ret = get_size(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;
        case DW_TAG_enumeration_type:
            ret = get_size(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;
            /* The types above have "DW_AT_byte_size" */

        case DW_TAG_subroutine_type:
            /* function pointer */
            /* FIXME: Can be transformed to the real function by runtime address */
            break;
        case DW_TAG_typedef:
        case DW_TAG_const_type:
        case DW_TAG_volatile_type:
            ret = get_type(dbg, type_die);
            if (ret != 0)
            {
                dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
                return 1;
            }
            break;
        default:
            dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
            fprintf(stderr, "SET dwarf: Unspecified type\n");
            return 1;
    }
    dwarf_dealloc(dbg, type_die, DW_DLA_DIE);
    return 0;
}

/* Get the type and size (in bytes) of the parameter */
static int parse_parameter(Dwarf_Debug dbg, Dwarf_Die die)
{
    int ret;

    ret = get_type(dbg, die);
    if (ret != 0)
        return 1;

    return 0;
}

static int compare_u32(const void *a, const void *b)
{
    return ( *(uint32_t *)a - *(uint32_t *)b );
}

/* Check whether the DIE is a function */
static int process_one_DIE(Dwarf_Debug dbg, Dwarf_Die die, int *is_func)
{
    Dwarf_Half tag;
    int ret;

    ret = dwarf_tag(die, &tag, NULL);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_tag()\n");
        return 1;
    }

    if (tag != DW_TAG_subprogram)
    {
        *is_func = 0;
    }
    else    /* The DIE is a function */
    {
        Dwarf_Attribute attr;
        Dwarf_Die child;
        Dwarf_Die sib_die;
        char *name;
        Dwarf_Addr pc;
        uint32_t *match_addr;

        *is_func = 1;

        /* Function name */
        ret = dwarf_diename(die, &name, NULL);
        if (ret == DW_DLV_ERROR)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_diename()\n");
            return 1;
        }
        if (ret == DW_DLV_NO_ENTRY)
            return 0; /* The DIE is a concrete inlined instance? */

        /* Entry PC */
        ret = dwarf_attr(die, DW_AT_low_pc, &attr, NULL);
        if (ret == DW_DLV_ERROR)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_attr()\n");
            return 1;
        }
        if (ret == DW_DLV_NO_ENTRY)
            return 0; /* The DIE is an inlined function */

        ret = dwarf_formaddr(attr, &pc, NULL);
        dwarf_dealloc(dbg, attr, DW_DLA_ATTR);
        if (ret != DW_DLV_OK)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_formaddr()\n");
            return 1;
        }

        /* Get the index of "func_addr" */
        pc += local_offset;
        match_addr = (uint32_t*) bsearch(&pc, local_func_addr, local_func_len, sizeof(uint32_t), compare_u32);
        if (match_addr)
            local_func_idx = (match_addr - local_func_addr);
        else
            return 0;

        dwarf_dealloc(dbg, name, DW_DLA_STRING);
        /* Get the parameter(s) of this function */
        ret = dwarf_child(die, &child, NULL);
        if (ret == DW_DLV_ERROR)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_child()\n");
            return 1;
        }
        if (ret == DW_DLV_NO_ENTRY)
            return 0;

        while (1)
        {
            /* Check whether the DIE is a formal parameter */
            ret = dwarf_tag(child, &tag, NULL);
            if (ret != DW_DLV_OK)
            {
                fprintf(stderr, "SET dwarf: Error in dwarf_tag()\n");
                dwarf_dealloc(dbg, child, DW_DLA_DIE);
                return 1;
            }

            /* FIXME: break or continue? */
            if (tag != DW_TAG_formal_parameter)
                break;

            ret = parse_parameter(dbg, child);
            if(ret != 0)
            {
                dwarf_dealloc(dbg, child, DW_DLA_DIE);
                return 1;
            }
            
            ret = dwarf_siblingof(dbg, child, &sib_die, NULL);
            if (ret == DW_DLV_ERROR)
            {
                fprintf(stderr, "SET dwarf: Error in dwarf_siblingof()\n");
                dwarf_dealloc(dbg, child, DW_DLA_DIE);
                return 1;
            }
            if (ret == DW_DLV_NO_ENTRY)
                break;

            dwarf_dealloc(dbg, child, DW_DLA_DIE);

            child = sib_die;
        }
        dwarf_dealloc(dbg, child, DW_DLA_DIE);
    }
    return 0;
}

/* Recursively follow the DIE tree */
static int process_die_and_children(Dwarf_Debug dbg, Dwarf_Die in_die)
{
    Dwarf_Die cur_die = in_die;
    Dwarf_Die child;
    Dwarf_Die sib_die;
    int is_function;
    int ret;

    ret = process_one_DIE(dbg, in_die, &is_function);
    if (ret != 0)
        return 1;

    while (1)
    {
        if (!is_function)   /* Assume that there is no nested function, so we ignore the children of a function */ 
        {
            ret = dwarf_child(cur_die, &child, NULL);
            if (ret == DW_DLV_ERROR)
            {
                fprintf(stderr, "SET dwarf: Error in dwarf_child()\n");
                return 1;
            }

            /* Recursive call */
            if (ret == DW_DLV_OK)
            {
                ret = process_die_and_children(dbg, child);
                dwarf_dealloc(dbg, child, DW_DLA_DIE);
                if (ret != 0)
                    return 1;
            }
        }

        /* Current DIE has no children */
        ret = dwarf_siblingof(dbg, cur_die, &sib_die, NULL);
        if (ret == DW_DLV_ERROR)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_siblingof()\n");
            return 1;
        }

        if (cur_die != in_die)
            dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);

        if (ret == DW_DLV_NO_ENTRY)
        {
            /* Done at this level */
            break;
        }

        /* ret == DW_DLV_OK */
        cur_die = sib_die;
        ret = process_one_DIE(dbg, cur_die, &is_function);
        if (ret != 0)
            return 1;
    }
    return 0;
}

/* process each compilation unit(CU) in .debug_info */
static int process_CUs(Dwarf_Debug dbg)
{
    Dwarf_Unsigned next_cu_header;
    int ret;

    while ((ret = dwarf_next_cu_header(dbg, NULL, NULL, NULL, NULL, &next_cu_header, NULL)) == DW_DLV_OK)
    {
        /* process a single compilation unit in .debug_info. */
        Dwarf_Die cu_die;

        ret = dwarf_siblingof(dbg, NULL, &cu_die, NULL);
        if (ret != DW_DLV_OK)
        {
            fprintf(stderr, "SET dwarf: Error in dwarf_siblingof()\n");
            return 1;
        }

        ret = process_die_and_children(dbg, cu_die);
        dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
        if (ret != 0)
            return 1;
    }
    if (ret == DW_DLV_ERROR)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_next_cu_header()\n");
        return 1;
    }
    else
        return 0;
}

static int process_one_file(Elf *elf)
{
    Dwarf_Debug dbg;
    int ret;

    ret = dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, NULL);
    if (ret != DW_DLV_OK)
    {
        fprintf(stderr, "SET dwarf: Error in dwarf_elf_init()\n");
        return 1;
    }

    ret = process_CUs(dbg);
    dwarf_finish(dbg, NULL);

    return ret;
}

Func_DWARF *SET_fill_func_dwarf(char *name, uint32_t *func_addr, int len, uint32_t off)
{
    int fd;
    Elf_Cmd cmd;
    Elf *arf;   /* Used for an archive */
    Elf *elf;
    int ret;

    if (elf_version(EV_CURRENT) == EV_NONE)
    {
        fprintf(stderr, "SET dwarf: libelf.a is out of date\n");
        return NULL;
    }

    fd = open(name, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "SET dwarf: Cannot open %s\n", name);
        return NULL;
    }

    /* Function DWARF table */
    func_dwarf = (Func_DWARF *) malloc(sizeof(Func_DWARF) * len);
    if (func_dwarf == NULL)
    {
        fprintf(stderr, "SET dwarf: No memory space to allocate DWARF table!\n");
        close(fd);
        return NULL;
    }
    memset(func_dwarf, 0, sizeof(Func_DWARF) * len);

    local_func_addr = func_addr;
    local_func_len = len;
    local_offset = off;

    cmd = ELF_C_READ;
    arf = elf_begin(fd, cmd, NULL);
    while ((elf = elf_begin(fd, cmd, arf)) != NULL)
    {
        Elf32_Ehdr *eh32;

        eh32 = elf32_getehdr(elf);
        if (eh32 == NULL)
        {
            fprintf(stderr, "SET dwarf: %s is not a 32-bit ELF file\n", name);
            goto fail;
        }
        else
        {
            ret = process_one_file(elf);
            if (ret != 0)
                goto fail;
        }

        cmd = elf_next(elf);
        elf_end(elf);
    }
    elf_end(arf);
    close(fd);
    return func_dwarf;

fail:
    elf_end(elf);
    elf_end(arf);
    free(func_dwarf);
    close(fd);
    return NULL;
}
