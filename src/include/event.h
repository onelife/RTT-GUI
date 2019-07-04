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
    {                                       \
        extern rtgui_app_t* rtgui_app_self(void); \
        (evt)->base.type = RTGUI_EVENT_##name; \
        (evt)->base.origin = rtgui_app_self(); \
        (evt)->base.ack = RT_NULL;          \
    }

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
    {                                       \
        _CREATE_EVENT(evt, name, timeout);  \
        if (timeout) {                      \
            EVT_LOG("[EVT] Create %s @%p", rtgui_event_text(evt), evt); \
        }                                   \
    }

#define RTGUI_FREE_EVENT(evt)               \
    {                                       \
        rt_mp_free(evt);                    \
        EVT_LOG("[EVT] Free %s @%p", rtgui_event_text(evt), evt); \
    }

/* type */
#define IS_EVENT_TYPE(e, tname)             ((e)->base.type == RTGUI_EVENT_##tname)
/* timer */
#define IS_TIMER_EVENT_STATE(e, sname)      (e->timer.timer->state == RTGUI_TIMER_ST_##sname)
/* mouse */
#define IS_MOUSE_EVENT_BUTTON(e, bname)     (e->mouse.button & MOUSE_BUTTON_##bname)
/* touch */
#define IS_TOUCH_EVENT_TYPE(e, tname)       (e->touch.data.type == RTGUI_TOUCH_##tname)


// TODO: below?
#define RTGUI_EVENT_VPAINT_REQ_INIT(e, win, cm) \
    do {                                    \
        RTGUI_EVENT_REINIT(e, VPAINT_REQ);  \
        (e)->wid = win;                     \
        (e)->cmp = cm;                      \
        (e)->origin = (e);                  \
        (e)->buffer = RT_NULL;              \
        rt_completion_init((e)->cmp);       \
    } while (0)

/* gesture */
#define RTGUI_EVENT_GESTURE_INIT(e, gtype)  \
    do {                                    \
        RTGUI_EVENT_REINIT(e, GESTURE);     \
        (e)->type = gtype;                  \
    } while (0)

/* keyboard */
#define IS_KBD_EVENT_KEY(e, kname)          (e->kbd.key == RTGUIK_##kname)
#define IS_KBD_EVENT_MOD(e, mname)          (e->kbd.mod == RTGUI_KMOD_##mname)
#define IS_KBD_EVENT_MOD2(e, mname)         (e->kbd.mod & (RTGUI_KMOD_L##mname | RTGUI_KMOD_R##mname))
#define IS_KBD_EVENT_TYPE(e, tname)         (e->kbd.type == RTGUI_KEY##tname)

/* user command */
#define RTGUI_CMD_UNKNOWN                   0x00
#define RTGUI_CMD_WM_CLOSE                  0x10
#define RTGUI_CMD_USER_INT                  0x20
#define RTGUI_CMD_USER_STRING               0x21

#if 0
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
#endif

/* Exported types ------------------------------------------------------------*/
typedef enum rtgui_evt_type {
    /* app event */
    RTGUI_EVENT_APP_CREATE                  = 0x0000,
    RTGUI_EVENT_APP_DESTROY,
    RTGUI_EVENT_APP_ACTIVATE,
    /* WM event */
    RTGUI_EVENT_SET_WM,
    /* win event */
    RTGUI_EVENT_WIN_CREATE                  = 0x1000,
    RTGUI_EVENT_WIN_DESTROY,
    RTGUI_EVENT_WIN_SHOW,
    RTGUI_EVENT_WIN_HIDE,
    RTGUI_EVENT_WIN_MOVE,
    RTGUI_EVENT_WIN_RESIZE,
    RTGUI_EVENT_WIN_CLOSE,
    RTGUI_EVENT_WIN_ACTIVATE,
    RTGUI_EVENT_WIN_DEACTIVATE,
    RTGUI_EVENT_WIN_UPDATE_END,
    /* sent after window setup and before the app setup */
    RTGUI_EVENT_WIN_MODAL_ENTER,
    /* widget event */
    RTGUI_EVENT_SHOW                        = 0x2000,
    RTGUI_EVENT_HIDE,
    RTGUI_EVENT_PAINT,
    RTGUI_EVENT_FOCUSED,
    RTGUI_EVENT_SCROLLED,
    RTGUI_EVENT_RESIZE,
    RTGUI_EVENT_SELECTED,
    RTGUI_EVENT_UNSELECTED,
    RTGUI_EVENT_MV_MODEL,
    RTGUI_EVENT_UPDATE_TOPLVL               = 0x3000,
    RTGUI_EVENT_UPDATE_BEGIN,
    RTGUI_EVENT_UPDATE_END,
    RTGUI_EVENT_MONITOR_ADD,
    RTGUI_EVENT_MONITOR_REMOVE,
    RTGUI_EVENT_TIMER,
    /* virtual paint event (server -> client) */
    RTGUI_EVENT_VPAINT_REQ,
    /* clip rect information */
    RTGUI_EVENT_CLIP_INFO,
    /* mouse and keyboard event */
    RTGUI_EVENT_MOUSE_MOTION                = 0x4000,
    RTGUI_EVENT_MOUSE_BUTTON,
    RTGUI_EVENT_KBD,
    RTGUI_EVENT_TOUCH,
    RTGUI_EVENT_GESTURE,
    WBUS_NOTIFY_EVENT                       = 0x5000,
    /* user command */
    RTGUI_EVENT_COMMAND                     = 0xF000,
} rtgui_evt_type_t;

/* base */
struct rtgui_evt_base {
    rtgui_evt_type_t type;
    rtgui_app_t *origin;
    rt_mailbox_t ack;
};

/* app */
struct rtgui_evt_app {
    struct rtgui_evt_base base;
    rtgui_app_t *app;
};

/* window manager */
struct rtgui_event_set_wm {
    struct rtgui_evt_base base;
    rtgui_app_t *app;
};

/* topwin */
struct rtgui_event_update_toplvl {
    struct rtgui_evt_base base;
    rtgui_win_t *toplvl;
};

/* timer */
struct rtgui_event_timer {
    struct rtgui_evt_base base;
    rtgui_timer_t *timer;
};

/* window */
#define _WINDOW_EVENT_BASE_ELEMENTS         \
    struct rtgui_evt_base base;             \
    rtgui_win_t *wid;

struct rtgui_event_win {
    _WINDOW_EVENT_BASE_ELEMENTS;
};

struct rtgui_event_win_create {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rtgui_win_t *parent_window;
};

struct rtgui_event_win_move {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rt_int16_t x, y;
};

struct rtgui_event_win_resize {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rtgui_rect_t rect;
};

// struct rtgui_event_win_update_end {
//     _WINDOW_EVENT_BASE_ELEMENTS;
//     rtgui_rect_t rect;
// };

#define rtgui_event_win_destroy             rtgui_event_win
#define rtgui_event_win_show                rtgui_event_win
#define rtgui_event_win_hide                rtgui_event_win
#define rtgui_event_win_activate            rtgui_event_win
#define rtgui_event_win_deactivate          rtgui_event_win
#define rtgui_event_win_close               rtgui_event_win
#define rtgui_event_win_modal_enter         rtgui_event_win

/* other window */
struct rtgui_event_update_begin {
    struct rtgui_evt_base base;
    /* the update rect */
    rtgui_rect_t rect;
};

struct rtgui_event_update_end {
    struct rtgui_evt_base base;
    /* the update rect */
    rtgui_rect_t rect;
};

struct rtgui_event_monitor {
    _WINDOW_EVENT_BASE_ELEMENTS;
    /* the monitor rect */
    rtgui_rect_t rect;
};

struct rtgui_event_paint {
    _WINDOW_EVENT_BASE_ELEMENTS;
    /* rect to be updated */
    rtgui_rect_t rect;
};

struct rtgui_event_clip_info {
    _WINDOW_EVENT_BASE_ELEMENTS;
    /* the number of rects */
    /*rt_uint32_t num_rect;*/
    /*rtgui_rect_t *rects*/
};

#define rtgui_event_show rtgui_evt_base
#define rtgui_event_hide rtgui_evt_base

struct rtgui_event_focused {
    struct rtgui_evt_base base;
    rtgui_widget_t *widget;
};

struct rtgui_event_resize {
    struct rtgui_evt_base base;
    rt_int16_t x, y;
    rt_int16_t w, h;
};

// TODO(onelife): ??
struct rt_completion {
    void *hi;
};

struct rtgui_event_vpaint_req {
    _WINDOW_EVENT_BASE_ELEMENTS;
    struct rtgui_event_vpaint_req *origin;
    struct rt_completion *cmp;
    rtgui_dc_t* buffer;
};

/* gesture */
enum rtgui_gesture_type {
    RTGUI_GESTURE_NONE                      = 0x0000,
    RTGUI_GESTURE_TAP                       = 0x0001,
    /* used when zoom in and out. */
    RTGUI_GESTURE_LONGPRESS                 = 0x0002,
    RTGUI_GESTURE_DRAG_HORIZONTAL           = 0x0004,
    RTGUI_GESTURE_DRAG_VERTICAL             = 0x0008,
    RTGUI_GESTURE_DRAG                      = (RTGUI_GESTURE_DRAG_HORIZONTAL | \
                                               RTGUI_GESTURE_DRAG_VERTICAL),
    RTGUI_GESTURE_FINISH                    = 0x8000,   /* PINCH or DRAG done */
    RTGUI_GESTURE_CANCEL                    = 0x4000,
    RTGUI_GESTURE_TYPE_MASK                 = 0x0FFF,
};

struct rtgui_event_gesture {
    _WINDOW_EVENT_BASE_ELEMENTS;
    enum rtgui_gesture_type type;
    rt_uint32_t act_cnt;               /* window activate count */
};

/* mouse */
typedef enum rtgui_mouse_btn {
    MOUSE_BUTTON_NONE                       = 0x00,
    MOUSE_BUTTON_LEFT                       = 0x01,
    MOUSE_BUTTON_RIGHT                      = 0x02,
    MOUSE_BUTTON_MIDDLE                     = 0x03,
    MOUSE_WHEEL_UP                          = 0x04,
    MOUSE_WHEEL_DOWN                        = 0x08,
    MOUSE_BUTTON_DOWN                       = 0x10,
    MOUSE_BUTTON_UP                         = 0x20,
} rtgui_mouse_btn_t;

struct rtgui_event_mouse {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rt_uint16_t x, y;
    rt_uint16_t button;
    rt_tick_t ts;                           /* timestamp */
    /* id of touch session(from down to up). Different session should have
     * different id. id should never be 0. */
    rt_uint32_t id;
    rt_uint32_t act_cnt;               /* window activate count */
};

/* beyboard */
struct rtgui_event_kbd {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rt_uint32_t act_cnt;               /* window activate count */
    rt_uint16_t type;                       /* key down or up */
    rt_uint16_t key;                        /* current key */
    rt_uint16_t mod;                        /* current key modifiers */
    rt_uint16_t unicode;                    /* translated character */
};

/* touch */
typedef enum rtgui_touch_type {
    RTGUI_TOUCH_NONE                        = 0x00,
    RTGUI_TOUCH_UP                          = 0x01,
    RTGUI_TOUCH_DOWN                        = 0x02,
    RTGUI_TOUCH_MOTION                      = 0x03,
} rtgui_touch_type_t;

struct rtgui_touch {
    rt_uint16_t id;
    rtgui_touch_type_t type;
    rtgui_point_t point;
};

/* handled by server */
struct rtgui_event_touch {
    struct rtgui_evt_base base;
    rtgui_touch_t data;
};

/* widget */
// struct rtgui_event_scrollbar {
//     struct rtgui_evt_base base;
//     rt_uint8_t event;
// };

/* data model? */
typedef enum rtgui_event_model_mode {
    RTGUI_MV_DATA_ADDED,
    RTGUI_MV_DATA_CHANGED,
    RTGUI_MV_DATA_DELETED,
} rtgui_event_model_mode_t;

struct rtgui_event_mv_model {
    struct rtgui_evt_base base;
    struct rtgui_mv_model *model;
    struct rtgui_mv_view  *view;
    rt_size_t first_data_changed_idx;
    rt_size_t last_data_changed_idx;
};

/* user command */
#ifndef GUIENGINE_CMD_STRING_MAX
# define GUIENGINE_CMD_STRING_MAX           (16)
#endif

struct rtgui_event_command {
    _WINDOW_EVENT_BASE_ELEMENTS;
    rt_int32_t type;
    rt_int32_t command_id;
    char command_string[GUIENGINE_CMD_STRING_MAX];
};

/* generic event */
union rtgui_evt_generic {
    struct rtgui_evt_base base;
    struct rtgui_evt_app app_create;
    struct rtgui_evt_app app_destroy;
    struct rtgui_evt_app app_activate;
    struct rtgui_event_set_wm set_wm;
    struct rtgui_event_update_toplvl update_toplvl;
    struct rtgui_event_timer timer;
    struct rtgui_event_win win_base;
    struct rtgui_event_win_create win_create;
    struct rtgui_event_win_destroy win_destroy;
    struct rtgui_event_win_move win_move;
    struct rtgui_event_win_resize win_resize;
    struct rtgui_event_win_show win_show;
    struct rtgui_event_win_hide win_hide;
    // struct rtgui_event_win_update_end win_update;
    struct rtgui_event_win_activate win_activate;
    struct rtgui_event_win_deactivate win_deactivate;
    struct rtgui_event_win_close win_close;
    struct rtgui_event_win_modal_enter win_modal_enter;
    struct rtgui_event_update_begin update_begin;
    struct rtgui_event_update_end update_end;
    struct rtgui_event_monitor monitor;
    struct rtgui_event_paint paint;
    struct rtgui_event_focused focused;
    struct rtgui_event_resize resize;
    struct rtgui_event_vpaint_req vpaint_req;
    struct rtgui_event_clip_info clip_info;
    struct rtgui_event_mouse mouse;
    struct rtgui_event_kbd kbd;
    struct rtgui_event_touch touch;
    struct rtgui_event_gesture gesture;
    // struct rtgui_event_scrollbar scrollbar;
    struct rtgui_event_mv_model model;
    struct rtgui_event_command command;
};

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_EVENT_H__ */
