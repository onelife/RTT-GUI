/*
 * File      : event.h
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
#ifndef __RTGUI_EVENT_H__
#define __RTGUI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
// #include "include/rtdevice.h"
#include "./kbddef.h"

/* Exported defines ----------------------------------------------------------*/
/* base event init */
#define RTGUI_EVENT(e)                      ((rtgui_evt_base_t *)(e))
#define RTGUI_EVENT_INIT(e, t)              \
    do {                                    \
        extern struct rtgui_app* rtgui_app_self(void); \
        (e)->type = (t);  \
        (e)->user = 0;                      \
        (e)->sender = rtgui_app_self();     \
        (e)->ack = RT_NULL;                 \
    } while (0)

/* app event init */
#define RTGUI_EVENT_APP_CREATE_INIT(e)      RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_APP_CREATE)
#define RTGUI_EVENT_APP_DESTROY_INIT(e)     RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_APP_DESTROY)
#define RTGUI_EVENT_APP_ACTIVATE_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_APP_ACTIVATE)

/* wm event init */
#define RTGUI_EVENT_SET_WM_INIT(e)          RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_SET_WM);

/* window event init */
#define RTGUI_EVENT_WIN_CREATE_INIT(e)      RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_CREATE)
#define RTGUI_EVENT_WIN_DESTROY_INIT(e)     RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_DESTROY)
#define RTGUI_EVENT_WIN_SHOW_INIT(e)        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_SHOW)
#define RTGUI_EVENT_WIN_HIDE_INIT(e)        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_HIDE)
#define RTGUI_EVENT_WIN_ACTIVATE_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_ACTIVATE)
#define RTGUI_EVENT_WIN_DEACTIVATE_INIT(e)  RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_DEACTIVATE)
#define RTGUI_EVENT_WIN_CLOSE_INIT(e)       RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_CLOSE)
#define RTGUI_EVENT_WIN_MOVE_INIT(e)        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_MOVE)
#define RTGUI_EVENT_WIN_RESIZE_INIT(e)      RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_RESIZE)
#define RTGUI_EVENT_WIN_UPDATE_END_INIT(e)  RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_UPDATE_END)
#define RTGUI_EVENT_WIN_MODAL_ENTER_INIT(e) RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_WIN_MODAL_ENTER)
/* other window event init */
#define RTGUI_EVENT_GET_RECT(e, i)          &(((rtgui_rect_t*)(e + 1))[i])
#define RTGUI_EVENT_UPDATE_BEGIN_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_UPDATE_BEGIN)
#define RTGUI_EVENT_UPDATE_END_INIT(e)      RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_UPDATE_END)
#define RTGUI_EVENT_MONITOR_ADD_INIT(e)     RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MONITOR_ADD)
#define RTGUI_EVENT_MONITOR_REMOVE_INIT(e)  RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MONITOR_REMOVE)
#define RTGUI_EVENT_CLIP_INFO_INIT(e)       RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_CLIP_INFO)
#define RTGUI_EVENT_PAINT_INIT(e)           RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_PAINT)
#define RTGUI_EVENT_TIMER_INIT(e)           RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_TIMER)
#define RTGUI_EVENT_SHOW_INIT(e)            RTGUI_EVENT_INIT((e), RTGUI_EVENT_SHOW)
#define RTGUI_EVENT_HIDE_INIT(e)            RTGUI_EVENT_INIT((e), RTGUI_EVENT_HIDE)
#define RTGUI_EVENT_UPDATE_TOPLVL_INIT(e)   \
    do {                                    \
        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_UPDATE_TOPLVL); \
        (e)->toplvl = RT_NULL;              \
    } while (0)

#define RTGUI_EVENT_VPAINT_REQ_INIT(e, win, cm) \
do { \
    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_VPAINT_REQ); \
    (e)->wid = win;                         \
    (e)->cmp = cm;                          \
    (e)->sender = (e);                      \
    (e)->buffer = RT_NULL;                  \
    rt_completion_init((e)->cmp);           \
} while (0)

/* gesture event init */
#define RTGUI_EVENT_GESTURE_INIT(e, gtype)  \
    do {                                    \
        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_GESTURE); \
        (e)->type = gtype;                  \
    } while (0)

/* mouse event init */
#define RTGUI_EVENT_MOUSE_MOTION_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MOUSE_MOTION)
#define RTGUI_EVENT_MOUSE_BUTTON_INIT(e)    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MOUSE_BUTTON)
#define RTGUI_MOUSE_BUTTON_LEFT             0x01
#define RTGUI_MOUSE_BUTTON_RIGHT            0x02
#define RTGUI_MOUSE_BUTTON_MIDDLE           0x03
#define RTGUI_MOUSE_BUTTON_WHEELUP          0x04
#define RTGUI_MOUSE_BUTTON_WHEELDOWN        0x08
#define RTGUI_MOUSE_BUTTON_DOWN             0x10
#define RTGUI_MOUSE_BUTTON_UP               0x20

/* keyboard event init */
#define RTGUI_EVENT_KBD_INIT(e)             RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_KBD)
#define RTGUI_KBD_IS_SET_CTRL(e)            ((e)->mod & (RTGUI_KMOD_LCTRL | RTGUI_KMOD_RCTRL))
#define RTGUI_KBD_IS_SET_ALT(e)             ((e)->mod & (RTGUI_KMOD_LALT  | RTGUI_KMOD_RALT))
#define RTGUI_KBD_IS_SET_SHIFT(e)           ((e)->mod & (RTGUI_KMOD_LSHIFT| RTGUI_KMOD_RSHIFT))
#define RTGUI_KBD_IS_UP(e)                  ((e)->type == RTGUI_KEYUP)
#define RTGUI_KBD_IS_DOWN(e)                ((e)->type == RTGUI_KEYDOWN)

/* touch event init */
#define RTGUI_EVENT_TOUCH_INIT(e)           RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_TOUCH)
#define RTGUI_TOUCH_UP                      0x01
#define RTGUI_TOUCH_DOWN                    0x02
#define RTGUI_TOUCH_MOTION                  0x03

/* user command event init */
#define RTGUI_EVENT_COMMAND_INIT(e)         RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_COMMAND)
#define RTGUI_CMD_UNKNOWN                   0x00
#define RTGUI_CMD_WM_CLOSE                  0x10
#define RTGUI_CMD_USER_INT                  0x20
#define RTGUI_CMD_USER_STRING               0x21

/* widget event init */
#define RTGUI_WIDGET_EVENT_INIT(e, t)       \
    do {                                    \
        (e)->type = (t);                    \
        (e)->sender = RT_NULL;              \
        (e)->ack = RT_NULL;                 \
    } while (0)
#define RTGUI_EVENT_SCROLLED_INIT(e)        RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_SCROLLED)
#define RTGUI_EVENT_FOCUSED_INIT(e)         RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_FOCUSED)
#define RTGUI_EVENT_RESIZE_INIT(e)          RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_RESIZE)
#define RTGUI_SCROLL_LINEUP                 0x01
#define RTGUI_SCROLL_LINEDOWN               0x02
#define RTGUI_SCROLL_PAGEUP                 0x03
#define RTGUI_SCROLL_PAGEDOWN               0x04

/* ? */
#define _RTGUI_EVENT_MV_INIT_TYPE(T)        \
rt_inline void RTGUI_EVENT_MV_MODEL_##T##_INIT( \
    struct rtgui_event_mv_model *e) { \
    RTGUI_EVENT_INIT(&((e)->parent), RTGUI_EVENT_MV_MODEL); \
    (e)->parent.user = RTGUI_MV_DATA_##T;   \
}

_RTGUI_EVENT_MV_INIT_TYPE(ADDED);
_RTGUI_EVENT_MV_INIT_TYPE(CHANGED);
_RTGUI_EVENT_MV_INIT_TYPE(DELETED);

#undef _RTGUI_EVENT_MV_INIT_TYPE

#define _RTGUI_EVENT_MV_IS_TYPE(T)          \
rt_inline rt_bool_t RTGUI_EVENT_MV_MODEL_IS_##T( \
    struct rtgui_event_mv_model *e) { \
    return e->parent.user == RTGUI_MV_DATA_##T; \
}

_RTGUI_EVENT_MV_IS_TYPE(ADDED);
_RTGUI_EVENT_MV_IS_TYPE(CHANGED);
_RTGUI_EVENT_MV_IS_TYPE(DELETED);

#undef _RTGUI_EVENT_MV_IS_TYPE

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_EVENT_H__ */
