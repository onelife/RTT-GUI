/***************************************************************************//**
 * @file    rttgui.cpp
 * @brief   Arduino RT-Thread GUI library main
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <Arduino.h>
#include <rttgui.h>

extern "C" {
    #ifdef RT_USING_ULOG
    # define LOG_TAG "RTT_GUI"
    # include "components/utilities/ulog/ulog.h"
    #else
    # define LOG_E(format, args...) rt_kprintf(format "\n", ##args)
    # define LOG_I(format, args...) rt_kprintf(format "\n", ##args)
    #endif
}

/* === RT-Thread GUI Class === */

void RT_GUI::begin(void) {
    rt_device_t dev;
    rt_err_t ret;

    dev = rt_device_find(CONFIG_GUI_DEVICE_NAME);
    RT_ASSERT(RT_NULL != dev);
    ret = rtgui_graphic_set_device(dev);
    RT_ASSERT(RT_EOK == ret);
    ret = rtgui_system_server_init();
    RT_ASSERT(RT_EOK == ret);
    (void)ret;

    LOG_I("GUI server satrted");
}

/* RT_Thread GUI instance */
RT_GUI RTT_GUI;
