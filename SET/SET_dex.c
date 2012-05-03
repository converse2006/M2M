#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SET_dex.h"

static uint16_t current_id = 0;

DexFile *SET_create_dex(char *path, uint32_t start, uint32_t end)
{
    DexFile *dex;
    int len;

    dex = (DexFile *) malloc(sizeof(DexFile));
    if (dex == NULL)
    {
        fprintf(stderr, "SET device: No memory space to allocate DexFile!\n");
        return NULL;
    }
    memset(dex, 0, sizeof(DexFile));
    
    dex->id = current_id++;

    len = strlen(path);
    dex->path = (char*) malloc(len+1);
    strncpy(dex->path, path, len+1);

    dex->start = start;
    dex->end = end;

    return dex;
}

void SET_delete_dex(DexFile *dex)
{
    if (dex == NULL)
        return;

    if (dex->count <= 0)
    {
        free(dex->path);
        free(dex);
    }
}

int SET_is_dex(char *path)
{
    int len = strlen(path);

    if (len > 4 && path[len-4] == '.' && path[len-3] == 'd' && path[len-2] == 'e' && path[len-1] == 'x')
        return 1;
    else
        return 0;
}
