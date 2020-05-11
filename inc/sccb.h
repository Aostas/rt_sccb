#ifndef __SCCB_H__
#define __SCCB_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif


#define RT_SCCB_WR               (0)
#define RT_SCCB_RD               (1)

struct rt_sccb_bus_device
{
    struct rt_device parent;
    struct rt_mutex lock;
    rt_uint32_t  timeout;
    void *priv;
};

struct rt_sccb_ops
{
    void *data;            /* private data for lowlevel routines */
    void (*set_sda)(void *data, rt_int32_t state);
    void (*set_scl)(void *data, rt_int32_t state);
    rt_int32_t (*get_sda)(void *data);

    void (*udelay)(rt_uint32_t us);

    rt_uint32_t timeout;   /* in tick */
};

rt_err_t rt_sccb_bus_device_register(struct rt_sccb_bus_device *bus,
                                    const char               *bus_name);
struct rt_sccb_bus_device *rt_sccb_bus_device_find(const char *bus_name);
int rt_sccb_core_init(void);

#ifdef __cplusplus
}
#endif

#endif/*__SCCB_H__*/
