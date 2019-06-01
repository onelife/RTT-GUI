/*
 * File      : rtgui.h
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
 * 2009-10-04     Bernard      first version
 * 2019-05-17     onelife      refactor
 */
#ifndef __RTGUI_H__
#define __RTGUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTGUI_RESTRICT                      __restrict
#ifdef _MSC_VER
# define RTGUI_PURE
#else
# define RTGUI_PURE                         __attribute__((pure))
#endif

/* Includes ------------------------------------------------------------------*/
#include "guiconfig.h"
#include "./types.h"
#include "./event.h"
#include "./arch.h"
#include "./driver.h"

/* Exported defines ----------------------------------------------------------*/
/* Object */
#define _OBJECT_METADATA                    CLASS_METADATA(object)
#define IS_OBJECT(obj)                      IS_INSTANCE(obj, _OBJECT_METADATA)
#define TO_OBJECT(obj)                      \
    CAST_(obj, _OBJECT_METADATA, rtgui_obj_t)
/* App */
#define _APP_METADATA                       CLASS_METADATA(app)
#define IS_APP(obj)                         IS_INSTANCE((obj), _APP_METADATA)
#define TO_APP(obj)                         \
    CAST_(obj, _APP_METADATA, rtgui_app_t)
/* Box sizer */
#define _BOX_METADATA                       CLASS_METADATA(box)
#define IS_BOX(obj)                         IS_INSTANCE(obj, _BOX_METADATA)
#define TO_BOX(obj)                         \
    CAST_(obj, _BOX_METADATA, rtgui_box_t)
/* Widget */
#define _WIDGET_METADATA                    CLASS_METADATA(widget)
#define IS_WIDGET(obj)                      IS_INSTANCE(obj, _WIDGET_METADATA)
#define TO_WIDGET(obj)                      \
    CAST_(obj, _WIDGET_METADATA, rtgui_widget_t)
/* Container */
#define _CONTAINER_METADATA                 CLASS_METADATA(container)
#define IS_CONTAINER(obj)                   \
    IS_INSTANCE((obj), _CONTAINER_METADATA)
#define TO_CONTAINER(obj)                   \
    CAST_(obj, _CONTAINER_METADATA, rtgui_container_t)
/* Title */
#define _TITLE_METADATA                     CLASS_METADATA(title)
#define IS_TITLE(obj)                       IS_INSTANCE((obj), _TITLE_METADATA)
#define TO_TITLE(obj)                       \
    CAST_(obj, _TITLE_METADATA, rtgui_title_t)
/* Window */
#define _WIN_METADATA                       CLASS_METADATA(win)
#define IS_WIN(obj)                         IS_INSTANCE((obj), _WIN_METADATA)
#define TO_WIN(obj)                         \
    CAST_(obj, _WIN_METADATA, rtgui_win_t)

#define _MIN(x, y)                          (((x) < (y)) ? (x) : (y))
#define _UI_MAX(x, y)                       (((x) > (y)) ? (x) : (y))
#define _BIT2BYTE(bits)                     ((bits + 7) >> 3)
#define _UI_ABS(x)                          (((x) >= 0) ? (x) : -(x))

#define RECT_W(r)                           ((r).x2 - (r).x1 + 1)
#define RECT_H(r)                           ((r).y2 - (r).y1 + 1)

/* Global variables ----------------------------------------------------------*/
extern struct rt_mempool *rtgui_event_pool;
extern rtgui_point_t rtgui_empty_point;

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(object);
CLASS_PROTOTYPE(app);
CLASS_PROTOTYPE(box);
CLASS_PROTOTYPE(widget);
CLASS_PROTOTYPE(container);
CLASS_PROTOTYPE(win);
CLASS_PROTOTYPE(title);

/* Exported functions ------------------------------------------------------- */
void *rtgui_create_instance(const rtgui_class_t *cls, rtgui_evt_hdl_t evt_hdl);
void rtgui_delete_instance(void *_obj);
rt_bool_t rtgui_is_subclass_of(const rtgui_class_t *cls,
    const rtgui_class_t *_super);
const rtgui_class_t *rtgui_class_of(void *_obj);
void *rtgui_object_cast_check(void *_obj, const rtgui_class_t *cls,
    const char *func, int line);

/* RTGUI server definitions */
/* top win manager init */
void rtgui_topwin_init(void);
void rtgui_server_init(void);

void rtgui_server_set_show_win_hook(void (*hk)(void));
void rtgui_server_set_act_win_hook(void (*hk)(void));

/* post an event to server */
rt_err_t rtgui_server_post_event(rtgui_evt_generic_t *evt);
rt_err_t rtgui_server_post_event_sync(rtgui_evt_generic_t *evt);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_H__ */
