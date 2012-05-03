#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"    /* TARGET_PAGE_BITS */
#include "SET_object.h"

static int HashTableSize[32] = {1, 13, 61, 127, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139,\
                                524287, 1048573};

/* obj id 0, 1, 2 is to represent a function return in user, kernel space, and interrupt context respectively */
static uint16_t current_id = 2;

ObjFile *SET_create_obj(char *path, uint32_t start, uint32_t end, uint32_t pgoff)
{
    ObjFile *obj;
    EFD *efd;
    uint32_t i;

    obj = (ObjFile *) malloc(sizeof(ObjFile));
    if (obj == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate ObjFile!\n");
        return NULL;
    }
    memset(obj, 0, sizeof(ObjFile));

    /* Using EFD to obtain the information about object file */
    efd = efd_open_elf(path);
    if (efd == NULL)
        return NULL;

    /* First pass: count # of functions */
    int size = 0;
    for (i = 0; i < efd_get_sym_num(efd); i++)
        if (IS_FUNC(efd, i))
            size++;
    if (size == 0)  /* Some library may contain no ".text" section */
    {
        SET_delete_obj(obj);
        efd_close(efd);
        return NULL;
    }
    obj->funcTableSize = HashTableSize[31-__builtin_clz(size)];

    /* Create a hash table of functions */
    obj->funcTable = (Func *) malloc(sizeof(Func) * obj->funcTableSize);
    if (obj->funcTable == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate symbol table!\n");
        SET_delete_obj(obj);
        efd_close(efd);
        return NULL;
    }
    memset(obj->funcTable, 0, sizeof(Func) * obj->funcTableSize);

    /* Second pass: push functions into hash table and count the range of text section */
    int idx;
    uint32_t vaddr, vaddr_off = (efd->elf_hdr.e_type == ET_DYN) ? (start - (pgoff << TARGET_PAGE_BITS)) : 0;
    uint32_t symtabFuncIdx = 0;
    uint32_t min = 0xc0000000, max = 0;
    for (i = 0; i < efd_get_sym_num(efd); i++)
    {
        if (IS_FUNC(efd, i))
        {
            vaddr = (efd_get_sym_value(efd, i) & THUMB_INSN_MASK) + vaddr_off;
            /* Linear probing for searching a free location */
            for (idx = HASH(vaddr, obj->funcTableSize); obj->funcTable[idx].addr != 0; idx = (idx+1) % obj->funcTableSize)
            {
                /* Avoid duplicated addresses */
                if (obj->funcTable[idx].addr == vaddr)
                    break;
            }
            if (obj->funcTable[idx].addr != vaddr)
            {
                obj->funcTable[idx].addr = vaddr;
                obj->funcTable[idx].symtabFuncIdx = symtabFuncIdx++;
                if (vaddr < min)
                    min = vaddr;
                if (vaddr + efd_get_sym_size(efd, i) > max)
                    max = vaddr + efd_get_sym_size(efd, i);
            }
        }
    }

#ifdef CONFIG_SET_DWARF
    /* DWARF */
    int idx_dwarf = efd_find_sec_by_name(efd, ".debug_info");
    if (idx_dwarf == 0)
        obj->func_dwarf = NULL;
    else
        obj->func_dwarf = SET_fill_func_dwarf(path, obj->func_addr, obj->table_size, vaddr_off);
#endif
    efd_close(efd);

    /* Text bound */
    obj->text_start = min;
    obj->text_end = max;

    obj->id = current_id++;

    int len = strlen(path);
    obj->path = (char*) malloc(len+1);
    strncpy(obj->path, path, len+1);

    return obj;
}

void SET_delete_obj(ObjFile *obj)
{
    if (obj == NULL)
        return;

    if (obj->count <= 0)
    {
        if (obj->path)
            free(obj->path);
        if (obj->funcTable)
            free(obj->funcTable);
#ifdef CONFIG_SET_DWARF
        if (obj->func_dwarf)
            free(obj->func_dwarf);
#endif
        free(obj);
    }
}

