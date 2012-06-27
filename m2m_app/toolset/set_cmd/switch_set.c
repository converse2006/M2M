/* FIXME
 * 1. add -ls to show timing model and trace program
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "SET_device.h"

static void usage()
{
    fprintf(stderr, "Usage: switch_set [options] trace_program ...\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -m MODEL     Set timing model (Range: 1~3)\n");
    fprintf(stderr, "  -t TYPE      Specify program type\n");
    fprintf(stderr, "  -h           Display this information\n");
    exit(1);
}

int main(int argc, char **argv)
{
    /* Option */
    int ch;
    int index;
    int model = 0;
    int type = 0;
    int ps = 0;
    /* SET device */
    uint8_t *vaddr;
    int fd;

    if (argc == 1)
        usage();

    while ((ch = getopt(argc, argv, "hm:pt:")) != -1)
    {
        switch(ch)
        {
            case 'm':
                model = atoi(optarg);
                break;
            case 'p':
                ps = 1;
                break;
            case 't':
                type = atoi(optarg);
                break;
            case 'h':
                usage();
                break;
            case '?':
                exit(1);
            default:
                break;
        }
    }

    /* Check the type is valid */
    if (type < 0 || type > 128)
    {
        fprintf(stderr,"Error program type!\n");
        exit(1);
    }

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        fprintf(stderr,"Error when opening /dev/mem\n");
        exit(1);
    }
    vaddr = mmap(0, SET_IOMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SET_BASE_ADDR);
    close(fd);
    if (vaddr == MAP_FAILED)
    {
        fprintf(stderr,"Error when mmap /dev/mem\n");
        exit(1);
    }

    /* Traced program */
    for (index = optind; index < argc; index++)
    {
        printf("Traced program: %s\n", argv[index]);
        memcpy(vaddr + SET_DEV_REG_EVENT_TYPE, &type, 4);
        memcpy(vaddr + SET_DEV_REG_EVENT_NAME, &argv[index], 4);
    }

    /* Timing model */
    if (model > 0)
    {
        switch (model)
        {
            case 1:
                fprintf(stderr, "Timing model: pipeline simulation\n");
                break;
            case 2:
                fprintf(stderr, "Timing model: Pipeline and I/D cache simulation\n");
                break;
            case 3:
                fprintf(stderr, "Timing model: Pipeline, I/D cache and instruction simulation\n");
                break;
            default:
                fprintf(stderr, "Wrong timing model!\n");
                exit(1);
        }
        memcpy(vaddr + SET_DEV_REG_VPMU_SETUP, &model, 4);
    }

    return 0;
}
