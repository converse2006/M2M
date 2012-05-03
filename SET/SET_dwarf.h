#ifndef SET_DWARF_H
#define SET_DWARF_H

typedef struct Func_DWARF {
    uint8_t param_size;     /* in words */
} Func_DWARF;

Func_DWARF *SET_fill_func_dwarf(char *name, uint32_t *func_addr, int len, uint32_t off);

#endif
