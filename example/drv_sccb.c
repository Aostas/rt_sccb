#include <board.h>
#include "drv_sccb.h"
#include "drv_config.h"

#ifdef PKG_USING_SCCB

#define LOG_TAG              "drv.sccb"
#include <drv_log.h>

#if !defined(SCCB_USING_SCCB1) && !defined(SCCB_USING_SCCB2)
#error "Please define at least one SCCB_USING_SCCBx"
#endif

static const struct stm32_sccb_config sccb_config[] =
{
#ifdef SCCB_USING_SCCB1
    SCCB1_BUS_CONFIG,
#endif
#ifdef SCCB_USING_SCCB2
    SCCB2_BUS_CONFIG,
#endif
};

static struct stm32_sccb sccb_obj[sizeof(sccb_config) / sizeof(sccb_config[0])];

/**
 * This function initializes the sccb pin.
 *
 * @param Stm32 sccb dirver class.
 */
static void stm32_sccb_gpio_init(struct stm32_sccb *sccb)
{
    struct stm32_sccb_config* cfg = (struct stm32_sccb_config*)sccb->ops.data;

    rt_pin_mode(cfg->scl, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(cfg->sda, PIN_MODE_OUTPUT_OD);

    rt_pin_write(cfg->scl, PIN_HIGH);
    rt_pin_write(cfg->sda, PIN_HIGH);
}

/**
 * This function sets the sda pin.
 *
 * @param Stm32 config class.
 * @param The sda pin state.
 */
static void stm32_set_sda(void *data, rt_int32_t state)
{
    struct stm32_sccb_config* cfg = (struct stm32_sccb_config*)data;
    if (state)
    {
        rt_pin_write(cfg->sda, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->sda, PIN_LOW);
    }
}

/**
 * This function sets the scl pin.
 *
 * @param Stm32 config class.
 * @param The scl pin state.
 */
static void stm32_set_scl(void *data, rt_int32_t state)
{
    struct stm32_sccb_config* cfg = (struct stm32_sccb_config*)data;
    if (state)
    {
        rt_pin_write(cfg->scl, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->scl, PIN_LOW);
    }
}

/**
 * This function gets the sda pin state.
 *
 * @param The sda pin state.
 */
static rt_int32_t stm32_get_sda(void *data)
{
    struct stm32_sccb_config* cfg = (struct stm32_sccb_config*)data;
    return rt_pin_read(cfg->sda);
}

/**
 * The time delay function.
 *
 * @param microseconds.
 */
static void stm32_udelay(rt_uint32_t us)
{
    rt_uint32_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t reload = SysTick->LOAD;

    ticks = us * reload / (1000000 / RT_TICK_PER_SECOND);
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

static const struct rt_sccb_ops stm32_sccb_ops_default =
{
    .data     = RT_NULL,
    .set_sda  = stm32_set_sda,
    .set_scl  = stm32_set_scl,
    .get_sda  = stm32_get_sda,
    .udelay   = stm32_udelay,
    .timeout  = 100
};

/**
 * if i2c is locked, this function will unlock it
 *
 * @param stm32 config class
 *
 * @return RT_EOK indicates successful unlock.
 */
static rt_err_t stm32_sccb_bus_unlock(const struct stm32_sccb_config *cfg)
{
    rt_int32_t i = 0;

    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        while (i++ < 9)
        {
            rt_pin_write(cfg->scl, PIN_HIGH);
            stm32_udelay(100);
            rt_pin_write(cfg->scl, PIN_LOW);
            stm32_udelay(100);
        }
    }
    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

/* I2sccbnitialization function */
int rt_hw_sccb_init(void)
{
    rt_size_t obj_num = sizeof(sccb_obj) / sizeof(struct stm32_sccb);
    rt_err_t result;

    for (int i = 0; i < obj_num; i++)
    {
        sccb_obj[i].ops = stm32_sccb_ops_default;
        sccb_obj[i].ops.data = (void*)&sccb_config[i];
        sccb_obj[i].sccb_bus.priv = &sccb_obj[i].ops;
        stm32_sccb_gpio_init(&sccb_obj[i]);
        result = rt_sccb_bus_device_register(&sccb_obj[i].sccb_bus, sccb_config[i].bus_name);
        RT_ASSERT(result == RT_EOK);
        stm32_sccb_bus_unlock(&sccb_config[i]);

        LOG_D("%s init done, pin scl: %d, pin sda %d",
        sccb_config[i].bus_name,
        sccb_config[i].scl,
        sccb_config[i].sda);
    }

    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_sccb_init);

#endif /* RT_USING_SCCB */
