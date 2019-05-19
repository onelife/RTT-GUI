/*
 * File      : rtgui_app.h
 * This file is part of RT-Thread GUI Engine
 * COPYRIGHT (C) 2006 - 2017, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-13     Grissiom     first version
 * 2019-05-15     onelife      refactor
 */

#ifndef __TO_APP_H__
#define __TO_APP_H__

#include "include/rtthread.h"
#include "./rtgui.h"
#include "./rtgui_system.h"

#ifdef __cplusplus
extern "C" {
#endif

RTGUI_CLASS_PROTOTYPE(application);

/** Gets the type of a application */
#define _APP_METADATA                       CLASS_METADATA(application)
/** Casts the object to an rtgui_workbench */
#define TO_APP(obj)                         \
    RTGUI_CAST(obj, _APP_METADATA, rtgui_app_t)
/** Checks if the object is an rtgui_workbench */
#define IS_APP(obj)                         IS_INSTANCE((obj), _APP_METADATA)

/**
 * create an application named @myname on current thread.
 *
 * @param name the name of the application
 *
 * @return a pointer to rtgui_app_t on success. RT_NULL on failure.
 */
rtgui_app_t *rtgui_srv_create(const char *name);
rtgui_app_t *rtgui_app_create(const char *name);
void rtgui_app_destroy(rtgui_app_t *app);
rt_bool_t rtgui_app_event_handler(rtgui_obj_t *obj,
    rtgui_evt_generic_t *event);

rt_base_t rtgui_app_run(rtgui_app_t *app);
void rtgui_app_exit(rtgui_app_t *app, rt_uint16_t code);
void rtgui_app_activate(rtgui_app_t *app);
void rtgui_app_close(rtgui_app_t *app);
void rtgui_app_sleep(rtgui_app_t *app, rt_uint32_t ms);

void rtgui_app_set_onidle(rtgui_app_t *app, rtgui_idle_hdl_t onidle);
rtgui_idle_hdl_t rtgui_app_get_onidle(rtgui_app_t *app);

/**
 * return the rtgui_app struct on current thread
 */
rtgui_app_t *rtgui_app_self(void);

rt_err_t rtgui_app_set_as_wm(rtgui_app_t *app);

void rtgui_app_set_main_win(rtgui_app_t *app, rtgui_win_t *win);
rtgui_win_t* rtgui_app_get_main_win(rtgui_app_t *app);

/* get the topwin belong app window activate count */
unsigned int rtgui_app_get_win_acti_cnt(void);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: __TO_APP_H__ */

