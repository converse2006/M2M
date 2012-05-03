#ifndef SET_DEVICE_H
#define SET_DEVICE_H

#define SET_BASE_ADDR   0xf0002000
#define SET_IOMEM_SIZE  0x1000

#define SET_DEV_REG_SET_CTRL        (0 << 2)
#define SET_DEV_REG_VPMU_SETUP      (1 << 2)
#define SET_DEV_REG_EVENT_TYPE      (2 << 2)
#define SET_DEV_REG_EVENT_NAME      (3 << 2)

#define SET_DEV_REG_APP_NAME        (21 << 2)
#define SET_DEV_REG_METHOD_ENTRY    (22 << 2)
#define SET_DEV_REG_METHOD_EXIT     (23 << 2)
//tianman
#define SET_DEV_REG_JNI_ENTRY       (24 << 2)
#define SET_DEV_REG_JNI_EXIT        (25 << 2)

#endif /* SET_DEVICE_H */
