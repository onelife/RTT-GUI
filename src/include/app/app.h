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

#ifndef __RTGUI_APP_H__
#define __RTGUI_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _APP_METADATA                       CLASS_METADATA(app)
#define IS_APP(obj)                         IS_INSTANCE((obj), _APP_METADATA)
#define TO_APP(obj)                         CAST_(obj, _APP_METADATA, rtgui_app_t)

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

#define APP_FLAG(a)                         (TO_APP(a)->flag)
#define APP_FLAG_CLEAR(a, fname)            APP_FLAG(a) &= ~RTGUI_APP_FLAG_##fname
#define APP_FLAG_SET(a, fname)              APP_FLAG(a) |= RTGUI_APP_FLAG_##fname
#define IS_APP_FLAG(a, fname)               (APP_FLAG(a) & RTGUI_APP_FLAG_##fname)

#define APP_SETTER(mname)                   rtgui_app_set_##mname

/* Exported types ------------------------------------------------------------*/
typedef enum rtgui_app_flag {
    RTGUI_APP_FLAG_INIT                     = 0x04,
    RTGUI_APP_FLAG_EXITED                   = RTGUI_APP_FLAG_INIT,
    RTGUI_APP_FLAG_SHOWN                    = 0x08,
    RTGUI_APP_FLAG_KEEP                     = 0x80,
} rtgui_app_flag_t;

struct rtgui_app {
    rtgui_obj_t _super;
    rt_thread_t tid;
    rtgui_win_t *main_win;
    char *name;
    rtgui_app_flag_t flag;
    rt_mailbox_t mb;
    rtgui_image_t *icon;
    rtgui_idle_hdl_t on_idle;
    void *user_data;
    /* PRIVATE */
    rt_ubase_t ref_cnt;
    rt_ubase_t win_cnt;
    rt_ubase_t act_cnt;                     /* activate count */
    rt_base_t exit_code;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(app);

#undef __RTGUI_APP_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
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
MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_app_t, app, rtgui_idle_hdl_t, on_idle);

/* get the topwin belong app window activate count */
unsigned int rtgui_get_app_act_cnt(void);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_APP_H__ */

