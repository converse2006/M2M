/* Handling DEX format */
#ifndef SET_DEX_H
#define SET_DEX_H

#include <stdint.h>
#include <sys/queue.h>

typedef struct DexFile {
    uint16_t    id;
    char        *path;
    uint32_t    start;
    uint32_t    end;
    int         count;
} DexFile;

DexFile *SET_create_dex(char *path, uint32_t start, uint32_t end);
void SET_delete_dex(DexFile *);

int SET_is_dex(char *path);

#endif
