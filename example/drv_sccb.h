/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-08     balanceTWK   first version
 */

#ifndef __DRV_SCCB__
#define __DRV_SCCB__

#include <rtthread.h>
#include <rthw.h>
#include "sccb.h"

/* stm32 config class */
struct stm32_sccb_config
{
    rt_uint8_t scl;
    rt_uint8_t sda;
    const char *bus_name;
};
/* stm32 sccb dirver class */
struct stm32_sccb
{
    struct rt_sccb_ops ops;
    struct rt_sccb_bus_device sccb_bus;
};

#ifdef SCCB_USING_SCCB1
#define SCCB1_BUS_CONFIG                                  \
    {                                                     \
        .scl = SCCB_SCCB1_SCL_PIN,                         \
        .sda = SCCB_SCCB1_SDA_PIN,                         \
        .bus_name = "sccb1",                              \
    }
#endif

#ifdef SCCB_USING_SCCB2
#define SCCB2_BUS_CONFIG                                  \
    {                                                     \
        .scl = SCCB_SCCB2_SCL_PIN,                         \
        .sda = SCCB_SCCB2_SDA_PIN,                         \
        .bus_name = "sccb2",                              \
    }
#endif

int rt_hw_sccb_init(void);

#endif
