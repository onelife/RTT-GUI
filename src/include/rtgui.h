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
#include "include/types.h"
#include "include/event.h"
#include "include/arch.h"
#include "include/dc.h"
#include "include/driver.h"
#include "include/color.h"

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
/* Label */
#define _LABEL_METADATA                     CLASS_METADATA(label)
#define IS_LABEL(obj)                       IS_INSTANCE(obj, _LABEL_METADATA)
#define TO_LABEL(obj)                       \
    CAST_(obj, _LABEL_METADATA, rtgui_label_t)
/* Button */
#define _BUTTON_METADATA                    CLASS_METADATA(button)
#define IS_BUTTON(obj)                      IS_INSTANCE(obj, _BUTTON_METADATA)
#define TO_BUTTON(obj)                      \
    CAST_(obj, _BUTTON_METADATA, rtgui_button_t)
/* Window */
#define _WIN_METADATA                       CLASS_METADATA(win)
#define IS_WIN(obj)                         IS_INSTANCE((obj), _WIN_METADATA)
#define TO_WIN(obj)                         \
    CAST_(obj, _WIN_METADATA, rtgui_win_t)

#define _MIN(x, y)                          (((x) < (y)) ? (x) : (y))
#define _MAX(x, y)                          (((x) > (y)) ? (x) : (y))

#define _BIT2BYTE(bits)                     ((bits + 7) >> 3)
#define _UI_ABS(x)                          (((x) >= 0) ? (x) : -(x))

#define RECT_W(r)                           ((r).x2 - (r).x1 + 1)
#define RECT_H(r)                           ((r).y2 - (r).y1 + 1)
#define IS_RECT_NO_SIZE(r)                  \
    (((r).x1 == (r).x2) && ((r).y1 == (r).y2))
#define RECT_CLEAR(r)                       \
    rt_memset(&r, 0x00, sizeof(rtgui_rect_t))


#define BORDER_SIZE_DEFAULT                 (2)
#define MARGIN_WIDGET_DEFAULT               (3)

#define TITLE_HEIGHT                        (20)
#define TITLE_CB_WIDTH                      (16)
#define TITLE_CB_HEIGHT                     (16)
#define TITLE_BORDER_SIZE                   (2)

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
CLASS_PROTOTYPE(title);
CLASS_PROTOTYPE(label);
CLASS_PROTOTYPE(button);
CLASS_PROTOTYPE(win);

/* Exported functions ------------------------------------------------------- */
void *rtgui_create_instance(const rtgui_class_t *cls, rtgui_evt_hdl_t evt_hdl);
void rtgui_delete_instance(void *_obj);
rt_bool_t rtgui_is_subclass_of(const rtgui_class_t *cls,
    const rtgui_class_t *_super);
const rtgui_class_t *rtgui_class_of(void *_obj);
void *rtgui_object_cast_check(void *_obj, const rtgui_class_t *cls,
    const char *func, int line);

/* server */
REFERENCE_GETTER_PROTOTYPE(server, rtgui_app_t);
SETTER_PROTOTYPE(server_show_win_hook, rtgui_hook_t);
SETTER_PROTOTYPE(server_act_win_hook, rtgui_hook_t);
rt_err_t rtgui_server_init(void);
rt_err_t rtgui_send_request(rtgui_evt_generic_t *evt, rt_int32_t timeout);
rt_err_t rtgui_send_request_sync(rtgui_evt_generic_t *evt);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_H__ */
