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
#include "./kbddef.h"

/* Exported defines ----------------------------------------------------------*/
#ifdef RTGUI_LOG_EVENT
# ifdef RT_USING_ULOG
#  define EVT_LOG                           LOG_I
# else
#  define EVT_LOG(format, args...)          rt_kprintf(format "\n", ##args)
# endif
#else
# define EVT_LOG(format, args...)
#endif

#define RTGUI_EVENT_REINIT(evt, name)       \
    do {                                    \
        extern rtgui_app_t* rtgui_app_self(void); \
        (evt)->base.type = RTGUI_EVENT_##name; \
        (evt)->base.user = 0;               \
        (evt)->base.origin = rtgui_app_self(); \
        (evt)->base.ack = RT_NULL;          \
    } while (0)

#define _CREATE_EVENT(evt, name, timeout)   \
    do {                                    \
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(rtgui_event_pool, timeout); \
        if (!evt) {                         \
            LOG_E("mp alloc err");          \
            break;                          \
        }                                   \
        RTGUI_EVENT_REINIT(evt, name);      \
    } while (0)

#define RTGUI_CREATE_EVENT(evt, name, timeout) \
    _CREATE_EVENT(evt, name, timeout);      \
    if (timeout) {                          \
        EVT_LOG("[EVT] Create %s @%p", rtgui_event_text(evt), evt); \
    }

#define RTGUI_FREE_EVENT(evt)               \
    rt_mp_free(evt);                        \
    EVT_LOG("[EVT] Free %s @%p", rtgui_event_text(evt), evt)

#define IS_EVENT_TYPE(e, tname)             \
    ((e)->base.type == RTGUI_EVENT_##tname)

/* timer */
#define IS_TIMER_EVENT_STATE(e, sname)      \
    (e->timer.timer->state == RTGUI_TIMER_ST_##sname)

#define RTGUI_EVENT_VPAINT_REQ_INIT(e, win, cm) \
do {                                        \
    RTGUI_EVENT_REINIT(e, VPAINT_REQ);        \
    (e)->wid = win;                         \
    (e)->cmp = cm;                          \
    (e)->origin = (e);                      \
    (e)->buffer = RT_NULL;                  \
    rt_completion_init((e)->cmp);           \
} while (0)

/* gesture event */
#define RTGUI_EVENT_GESTURE_INIT(e, gtype)  \
    do {                                    \
        RTGUI_EVENT_REINIT(e, GESTURE);     \
        (e)->type = gtype;                  \
    } while (0)

/* mouse */
typedef enum rtgui_mouse_btn {
    RTGUI_MOUSE_BUTTON_NONE                 = 0x00,
    RTGUI_MOUSE_BUTTON_LEFT                 = 0x01,
    RTGUI_MOUSE_BUTTON_RIGHT                = 0x02,
    RTGUI_MOUSE_BUTTON_MIDDLE               = 0x03,
    RTGUI_MOUSE_BUTTON_WHEELUP              = 0x04,
    RTGUI_MOUSE_BUTTON_WHEELDOWN            = 0x08,
    RTGUI_MOUSE_BUTTON_DOWN                 = 0x10,
    RTGUI_MOUSE_BUTTON_UP                   = 0x20,
} rtgui_mouse_btn_t;
#define IS_MOUSE_EVENT_BUTTON(e, bname)    (e->mouse.button & RTGUI_MOUSE_BUTTON_##bname)

/* touch */
#define IS_TOUCH_EVENT_TYPE(e, tname)       \
    (e->touch.data.type == RTGUI_TOUCH_##tname)

/* keyboard */
#define IS_KBD_EVENT_KEY(e, kname)          (e->kbd.key == RTGUIK_##kname)
#define IS_KBD_EVENT_MOD(e, mname)          (e->kbd.mod == RTGUI_KMOD_##mname)
#define IS_KBD_EVENT_MOD2(e, mname)         \
    (e->kbd.mod & (RTGUI_KMOD_L##mname | RTGUI_KMOD_R##mname))
#define IS_KBD_EVENT_TYPE(e, tname)         (e->kbd.type == RTGUI_KEY##tname)

/* user command */
#define RTGUI_CMD_UNKNOWN                   0x00
#define RTGUI_CMD_WM_CLOSE                  0x10
#define RTGUI_CMD_USER_INT                  0x20
#define RTGUI_CMD_USER_STRING               0x21

/* widget event init */
#define RTGUI_WIDGET_EVENT_INIT(e, t)       \
    do {                                    \
        (e)->type = (t);                    \
        (e)->origin = RT_NULL;              \
        (e)->ack = RT_NULL;                 \
    } while (0)
#define RTGUI_SCROLL_LINEUP                 0x01
#define RTGUI_SCROLL_LINEDOWN               0x02
#define RTGUI_SCROLL_PAGEUP                 0x03
#define RTGUI_SCROLL_PAGEDOWN               0x04

/* TODO(onelife): ? */
#define _RTGUI_EVENT_MV_INIT_TYPE(T)        \
rt_inline void RTGUI_EVENT_MV_MODEL_##T##_INIT( \
    struct rtgui_event_mv_model *e) { \
    RTGUI_EVENT_REINIT(e, MV_MODEL); \
    e->base.user = RTGUI_MV_DATA_##T;   \
}

_RTGUI_EVENT_MV_INIT_TYPE(ADDED);
_RTGUI_EVENT_MV_INIT_TYPE(CHANGED);
_RTGUI_EVENT_MV_INIT_TYPE(DELETED);

#undef _RTGUI_EVENT_MV_INIT_TYPE

#define _RTGUI_EVENT_MV_IS_TYPE(T)          \
rt_inline rt_bool_t RTGUI_EVENT_MV_MODEL_IS_##T( \
    struct rtgui_event_mv_model *e) { \
    return e->base.user == RTGUI_MV_DATA_##T; \
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
