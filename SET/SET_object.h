/* Handling object file in ELF format */
#ifndef SET_OBJECT_H
#define SET_OBJECT_H

#include <stdint.h>
#include "efd.h"
#ifdef CONFIG_SET_DWARF
#include "SET_dwarf.h"
#endif

#define IS_FUNC(efd, i) ((efd_get_sym_type(efd, i) == STT_FUNC) \
                        && (efd_get_sym_vis(efd, i) == STV_DEFAULT) \
                        && (efd_get_sym_shndx(efd, i) != SHN_UNDEF))
#define THUMB_INSN_MASK 0xfffffffe

#define HASH(addr, size)  (((addr)>>2)%(size))

typedef struct Func {
    uint32_t    addr;
    uint32_t    symtabFuncIdx;                  /* index of function symbol table, used for post-processing */
} Func;

typedef struct ObjFile {
    uint16_t    id;
    char        *path;
    Func        *funcTable;                     /* hash table of function addresses */
    int         funcTableSize;
#ifdef CONFIG_SET_DWARF
    Func_DWARF  *func_dwarf;                    /* contain debugging information */
#endif
    uint32_t    text_start;
    uint32_t    text_end;
    int         count;
} ObjFile;

#define MAX_STACK_SIZE      1024    /* you can set this value up to 65536 */
typedef struct Stack {
    uint32_t    stack[MAX_STACK_SIZE];
    int         size;
} Stack;

ObjFile *SET_create_obj(char *path, uint32_t start, uint32_t end, uint32_t pgoff);
void SET_delete_obj(ObjFile *);

#endif
