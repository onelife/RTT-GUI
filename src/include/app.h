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
 * 2019-05-15     onelife      refactor and rename to "app.h"
 */

#ifndef __APP_H__
#define __APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CREATE_SERVER_INSTANCE(obj, hdl, name) \
    do {                                    \
        obj = (rtgui_app_t *)CREATE_INSTANCE(app, hdl); \
        if (!obj) break;                    \
        if (rtgui_app_init(obj, name, RT_TRUE)) \
            DELETE_INSTANCE(obj);           \
    } while (0)

#define CREATE_APP_INSTANCE(obj, hdl, name) \
    do {                                    \
        obj = (rtgui_app_t *)CREATE_INSTANCE(app, hdl); \
        if (!obj) break;                    \
        if (rtgui_app_init(obj, name, RT_FALSE)) \
            DELETE_INSTANCE(obj);           \
    } while (0)

#define DELETE_APP_INSTANCE(obj)            rtgui_app_uninit(obj)


MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_app_t, app, rtgui_idle_hdl_t, on_idle);

rt_err_t rtgui_app_init(rtgui_app_t *app, const char *name, rt_bool_t is_srv);
void rtgui_app_uninit(rtgui_app_t *app);

rt_base_t rtgui_app_run(rtgui_app_t *app);
void rtgui_app_exit(rtgui_app_t *app, rt_uint16_t code);
void rtgui_app_activate(rtgui_app_t *app);
void rtgui_app_close(rtgui_app_t *app);
void rtgui_app_sleep(rtgui_app_t *app, rt_uint32_t ms);
/* return the rtgui_app struct on current thread */
rtgui_app_t *rtgui_app_self(void);
rt_err_t rtgui_app_set_as_wm(rtgui_app_t *app);

MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_app_t, app, rtgui_win_t*, main_win);

/* get the topwin belong app window activate count */
unsigned int rtgui_get_app_act_cnt(void);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: __APP_H__ */

