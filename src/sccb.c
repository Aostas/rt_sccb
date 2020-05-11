#include <rtthread.h>

#define DBG_TAG               "SCCB"
#ifdef PKG_SCCB_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

#include "sccb.h"

#define SET_SDA(ops, val)   ops->set_sda(ops->data, val)
#define SET_SCL(ops, val)   ops->set_scl(ops->data, val)
#define GET_SDA(ops)        ops->get_sda(ops->data)

#define SDA_L(ops)          SET_SDA(ops, 0)
#define SDA_H(ops)          SET_SDA(ops, 1)
#define SCL_L(ops)          SET_SCL(ops, 0)
#define SCL_H(ops)          SET_SCL(ops, 1)

#define sccb_delay(ops,us)  ops->udelay(us);

static void sccb_start(struct rt_sccb_ops *ops)
{
    SDA_H(ops);
    SCL_H(ops);
    sccb_delay(ops, 5);
    SDA_L(ops);
    sccb_delay(ops, 5);
    SCL_L(ops);
}

static void sccb_stop(struct rt_sccb_ops *ops)
{
    SDA_L(ops);
    sccb_delay(ops, 5);
    SCL_H(ops);
    sccb_delay(ops, 5);
    SDA_H(ops);
    sccb_delay(ops, 5);
}

static void sccb_no_ack(struct rt_sccb_ops *ops)
{
    sccb_delay(ops, 5);
    SDA_H(ops);
    SCL_H(ops);
    sccb_delay(ops, 5);
    SCL_L(ops);
    sccb_delay(ops, 5);
    SDA_L(ops);
    sccb_delay(ops, 5);
}

static rt_int32_t sccb_writeb(struct rt_sccb_bus_device *bus, rt_uint8_t data)
{
    rt_int32_t i;
    rt_uint8_t bit;
    rt_uint8_t res;

    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;

    for (i = 7; i >= 0; i--)
    {
        bit = (data >> i) & 1;
        SET_SDA(ops, bit);
        SCL_H(ops);
        sccb_delay(ops, 5);
        SCL_L(ops);
    }
    SDA_H(ops);
    sccb_delay(ops, 5);
    SCL_H(ops);
    sccb_delay(ops, 5);
    res=GET_SDA(ops);
    SCL_L(ops);
    return res;
}

static rt_int32_t sccb_readb(struct rt_sccb_bus_device *bus)
{
    rt_uint8_t i;
    rt_uint8_t data = 0;
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;

    SDA_H(ops);
    for (i = 0; i < 8; i++)
    {
        sccb_delay(ops, 5);
        SCL_H(ops);
        data <<= 1;
        if (GET_SDA(ops))
            data |= 1;
        sccb_delay(ops, 5);
        SCL_L(ops);
    }
    return data;
}

static rt_size_t sccb_bus_device_read(rt_device_t dev,
                                      rt_off_t    pos,
                                      void       *buffer,
                                      rt_size_t   count)
{
    struct rt_sccb_bus_device *bus = (struct rt_sccb_bus_device *)dev->user_data;
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;
    rt_int32_t ret;
    rt_uint8_t dev_addr;
    rt_uint8_t reg_addr;
    rt_uint8_t *data=(rt_uint8_t *)buffer+1;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    dev_addr = pos & 0xff;
    reg_addr = *(rt_uint8_t *)buffer;

    rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
    sccb_start(ops);
    ret = sccb_writeb(bus, (dev_addr << 1)|RT_SCCB_WR);
    if (ret != RT_EOK)
    {
        LOG_E("can't find sccb dev [0x%02x]", dev_addr);
        goto out;
    }
    sccb_delay(ops, 100);
    sccb_writeb(bus, reg_addr);
    sccb_delay(ops, 100);
    sccb_stop(ops);
    sccb_delay(ops, 100);
    sccb_start(ops);
    ret = sccb_writeb(bus, (dev_addr << 1)|RT_SCCB_RD);
    if (ret != RT_EOK)
    {
        LOG_E("can't find sccb dev [0x%02x]", dev_addr);
        goto out;
    }
    sccb_delay(ops, 100);
    *data = sccb_readb(bus);
    sccb_no_ack(ops);

out:
    LOG_D("send stop condition");
    sccb_stop(ops);
    rt_mutex_release(&bus->lock);
    return 1;
}

static rt_size_t sccb_bus_device_write(rt_device_t dev,
                                       rt_off_t    pos,
                                       const void *buffer,
                                       rt_size_t   count)
{
    struct rt_sccb_bus_device *bus = (struct rt_sccb_bus_device *)dev->user_data;
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;
    rt_int32_t ret;
    rt_uint8_t dev_addr;
    rt_uint8_t reg_addr;
    rt_uint8_t *data=(rt_uint8_t *)buffer+1;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    dev_addr = pos & 0xff;
    reg_addr = *(rt_uint8_t *)buffer;

    rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
    sccb_start(ops);
    ret = sccb_writeb(bus, (dev_addr << 1)|RT_SCCB_WR);
    if (ret != RT_EOK)
    {
        LOG_E("can't find sccb dev [0x%02x]", dev_addr);
        goto out;
    }
    sccb_delay(ops, 100);
    sccb_writeb(bus, reg_addr);
    sccb_delay(ops, 100);
    ret = sccb_writeb(bus, *data);

out:
    LOG_D("send stop condition");
    sccb_stop(ops);
    rt_mutex_release(&bus->lock);
    return 1;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops sccb_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    i2c_bus_device_read,
    i2c_bus_device_write,
    RT_NULL
};
#endif

rt_err_t rt_sccb_bus_device_device_init(struct rt_sccb_bus_device *bus,
                                        const char                *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_SCCBBUS;
    /* initialize device interface */
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &sccb_ops;
#else
    device->init    = RT_NULL;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = sccb_bus_device_read;
    device->write   = sccb_bus_device_write;
    device->control = RT_NULL;
#endif

    /* register to device manager */
    rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}

rt_err_t rt_sccb_bus_device_register(struct rt_sccb_bus_device *bus,
                                     const char                *bus_name)
{
    rt_err_t res = RT_EOK;

    rt_mutex_init(&bus->lock, "sccb_bus_lock", RT_IPC_FLAG_FIFO);

    if (bus->timeout == 0) bus->timeout = RT_TICK_PER_SECOND;

    res = rt_sccb_bus_device_device_init(bus, bus_name);

    LOG_I("SCCB bus [%s] registered", bus_name);

    return res;
}

struct rt_sccb_bus_device *rt_sccb_bus_device_find(const char *bus_name)
{
    struct rt_sccb_bus_device *bus;
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL || dev->type != RT_Device_Class_I2CBUS)
    {
        LOG_E("SCCB bus %s not exist", bus_name);

        return RT_NULL;
    }

    bus = (struct rt_sccb_bus_device *)dev->user_data;

    return bus;
}

int rt_sccb_component_init(void)
{
    return 0;
}
INIT_COMPONENT_EXPORT(rt_sccb_component_init);

