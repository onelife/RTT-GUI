/*
 * File      : server.c
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
 * 2019-05-15     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
//#include <rtgui/touch.h>
#include "include/widgets/window.h"
#include "include/app/app.h"
#include "include/app/mouse.h"
#include "include/app/topwin.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_SVR"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static rtgui_app_t *_srv_app = RT_NULL;
static rtgui_app_t *_wm_app = RT_NULL;
static rtgui_hook_t _show_win_hook = RT_NULL;
static rtgui_hook_t _act_win_hook = RT_NULL;

/* Private functions ---------------------------------------------------------*/
static rt_bool_t _server_mouse_motion_handler(rtgui_evt_generic_t *evt) {
    rt_uint16_t x, y;
    rt_bool_t done = RT_TRUE;

    do {
        rtgui_topwin_t *top;

        x = evt->mouse.x;
        y = evt->mouse.y;

        top = rtgui_topwin_get_at(x, y);
        if (!top) break;

        if (top && top->monitor_list.next) {
            /* check whether the monitor exist */
            if (!rtgui_monitor_contain_point(&(top->monitor_list), x, y))
                break;
        }

        /* send RTGUI_EVENT_MOUSE_MOTION */
        evt->mouse.wid = top->wid;
        evt->mouse.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(top->wid->app, evt, 1/*RT_WAITING_NO*/);
        /* to prevent evt been free */
        done = RT_FALSE;
    } while (0);

    /* move mouse */
    rtgui_cursor_move(x, y);

    return done;
}

static rt_bool_t _server_mouse_button_handler(rtgui_evt_generic_t *evt) {
    rt_uint16_t x, y;
    rt_bool_t done = RT_TRUE;

    do {
        rtgui_topwin_t *top;

        x = evt->mouse.x;
        y = evt->mouse.y;

        #ifdef RTGUI_USING_WINMOVE
            if (rtgui_winmove_is_moving() && (evt->mouse.button & \
                (MOUSE_BUTTON_LEFT | MOUSE_BUTTON_UP))) {
                rtgui_win_t *win;
                rtgui_rect_t rect;

                if (rtgui_winmove_done(&rect, &win)) {
                    /* send RTGUI_EVENT_WIN_MOVE */
                    RTGUI_EVENT_REINIT(evt, WIN_MOVE);
                    evt->win_move.wid = win;
                    evt->win_move.x = rect.x1;
                    evt->win_move.y = rect.y1;
                    rtgui_request(win->app, evt, RT_WAITING_NO);
                    done = RT_FALSE;
                    break;
                }
            }
        #endif

        /* get the top which contains the mouse */
        top = rtgui_topwin_get_at(x, y);
        if (!top) break;

        /* only raise window if the button is pressed down */
        if (IS_MOUSE_EVENT_BUTTON(evt, DOWN) && \
            (rtgui_topwin_get_focus() != top)) {
            rtgui_topwin_activate(top);
        }

        /* send RTGUI_EVENT_MOUSE_BUTTON */
        evt->mouse.wid = top->wid;
        evt->mouse.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(top->app, evt, 1/*RT_WAITING_NO*/);
        done = RT_FALSE;
    } while (0);

    /* move mouse */
    rtgui_cursor_move(x, y);

    return done;
}

static rt_bool_t _server_touch_handler(rtgui_evt_generic_t *evt) {
     // if (!rtgui_touch_do_calibration(evt)) return RT_FALSE;
     if (IS_TOUCH_EVENT_TYPE(evt, MOTION)) {
        RTGUI_EVENT_REINIT(evt, MOUSE_MOTION);
        evt->mouse.x = evt->touch.data.point.x;
        evt->mouse.y = evt->touch.data.point.y;
        evt->mouse.button = MOUSE_BUTTON_NONE;
        return _server_mouse_motion_handler(evt);
    } else {
        RTGUI_EVENT_REINIT(evt, MOUSE_BUTTON);
        evt->mouse.x = evt->touch.data.point.x;
        evt->mouse.y = evt->touch.data.point.y;
        evt->mouse.button = MOUSE_BUTTON_LEFT;
        if (evt->touch.data.type == RTGUI_TOUCH_UP)
            evt->mouse.button |= MOUSE_BUTTON_UP;
        else
            evt->mouse.button |= MOUSE_BUTTON_DOWN;
        return _server_mouse_button_handler(evt);
     }
}

static rt_bool_t _server_keyboard_handler(rtgui_evt_generic_t *evt) {
    rtgui_topwin_t *top;
    rt_bool_t done = RT_TRUE;

    /* todo: handle input method and global shortcut */
    top = rtgui_topwin_get_focus();
    if (top) {
        RT_ASSERT(top->flag & RTGUI_TOPWIN_FLAG_ACTIVATE);

        /* send RTGUI_EVENT_KBD */
        /* change owner to server */
        RTGUI_EVENT_REINIT(evt, KBD);
        /* send to focus window */
        evt->kbd.wid = top->wid;
        evt->kbd.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(top->app, evt, RT_WAITING_NO);
        /* avoid free event */
        done = RT_FALSE;
    }

    return done;
}

static rt_bool_t _server_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rt_uint32_t ack = RT_EOK;
    rt_bool_t done = RT_TRUE;
    (void)obj;

    EVT_LOG("[SrvEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_APP_CREATE:
    case RTGUI_EVENT_APP_DESTROY:
        if (_wm_app) {
            /* forward to wm */
            rtgui_request(_wm_app, evt, RT_WAITING_FOREVER);
            done = RT_FALSE;
        }
        break;

    /* mouse and keyboard */
    case RTGUI_EVENT_MOUSE_MOTION:
        done = _server_mouse_motion_handler(evt);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        done = _server_mouse_button_handler(evt);
        break;

    case RTGUI_EVENT_KBD:
        /* handle keyboard event */
        done = _server_keyboard_handler(evt);
        break;

    case RTGUI_EVENT_TOUCH:
        /* handle touch event */
        LOG_D("touch: %x, %d (%d,%d)", evt->touch.data.id, evt->touch.data.type,
            evt->touch.data.point.x, evt->touch.data.point.y);
        done = _server_touch_handler(evt);
        break;

    /* window */
    case RTGUI_EVENT_WIN_CREATE:
        if (RT_EOK != rtgui_topwin_add(evt->base.origin, evt->win_create.wid,
            evt->win_create.parent_window))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_SHOW:
        if (_show_win_hook) _show_win_hook();
        if (RT_EOK != rtgui_topwin_show(evt->win_base.wid))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_HIDE:
        if (RT_EOK != rtgui_topwin_hide(evt->win_base.wid))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_MOVE:
        if (RT_EOK != rtgui_topwin_move(evt->win_move.wid, evt->win_move.x,
            evt->win_move.y))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_MODAL_ENTER:
        if (RT_EOK != rtgui_topwin_modal_enter(evt->win_modal_enter.wid))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_ACTIVATE:
        if (_act_win_hook) _act_win_hook();
        if (RT_EOK != rtgui_topwin_activate_win(evt->win_activate.wid))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_DESTROY:
        if (RT_EOK != rtgui_topwin_remove(evt->win_base.wid))
            ack = RT_ERROR;
        break;

    case RTGUI_EVENT_WIN_RESIZE:
        rtgui_topwin_resize(evt->win_resize.wid, &evt->win_resize.rect);
        break;

    case RTGUI_EVENT_SET_WM:
        if (!_wm_app) {
            _wm_app = evt->set_wm.app;
        } else {
            ack = RT_ERROR;
        }
        break;

    /* other */
    case RTGUI_EVENT_COMMAND:
        break;

    case RTGUI_EVENT_UPDATE_BEGIN:
        #ifdef RTGUI_USING_CURSOR
            rtgui_cursor_hide();
        #endif
        break;

    case RTGUI_EVENT_UPDATE_END:
        {
            /* handle screen update */
            rtgui_gfx_driver_t *drv = rtgui_get_gfx_device();
            if (drv) {
                rtgui_gfx_update_screen(drv,
                    &(evt->update_end.rect));
            }
            #ifdef RTGUI_USING_CURSOR
                rtgui_cursor_show();
            #endif
        }
        break;

    case RTGUI_EVENT_MONITOR_ADD:
        /* handle mouse monitor */
        rtgui_topwin_append_monitor_rect(evt->monitor.wid,
            &(evt->monitor.rect));
        break;

    case RTGUI_EVENT_MONITOR_REMOVE:
        rtgui_topwin_remove_monitor_rect(evt->monitor.wid,
            &(evt->monitor.rect));
        break;

    default:
        LOG_E("[SrvEVT] bad event [%x]", evt->base.type);
        ack = RT_ERROR;
        break;
    }

    EVT_LOG("[SrvEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);

    if (!done && !IS_EVENT_TYPE(evt, MOUSE_MOTION) && \
        !IS_EVENT_TYPE(evt, MOUSE_BUTTON) && \
        !IS_EVENT_TYPE(evt, KBD)) {
        LOG_E("[SrvEVT] %p not done", evt);
    }
    if (done && evt && !IS_EVENT_TYPE(evt, TIMER)) {
        rtgui_response(evt, ack);
        RTGUI_FREE_EVENT(evt);
    }
    return done;    // who care server handler return value?
}

static void server_entry(void *pram) {
    (void)pram;

    /* create server app */
    CREATE_SERVER_INSTANCE(_srv_app, _server_event_handler, "rtgui");
    if (!_srv_app) {
        LOG_E("create srv err");
        return;
    }

    /* init mouse and show */
    #ifdef RTGUI_USING_CURSOR
        rtgui_cursor_show();
    #endif

    /* loop event handler */
    rtgui_app_run(_srv_app);

    /* exit */
    rtgui_app_uninit(_srv_app);
    _srv_app = RT_NULL;
}

/* Public functions ----------------------------------------------------------*/
RTGUI_REFERENCE_GETTER(server, rtgui_app_t, _srv_app);
RTGUI_SETTER(server_show_win_hook, rtgui_hook_t, _show_win_hook);
RTGUI_SETTER(server_act_win_hook, rtgui_hook_t, _act_win_hook);

rt_err_t rtgui_server_init(void) {
    rt_thread_t tid;
    rt_err_t ret;

    do {
        ret = rtgui_cursor_init();
        if (RT_EOK != ret) break;

        tid = rt_thread_create(
            "rtgui",
            server_entry, RT_NULL,
            GUIENGIN_SVR_THREAD_STACK_SIZE,
            GUIENGINE_SVR_THREAD_PRIORITY,
            GUIENGINE_SVR_THREAD_TIMESLICE);
        if (!tid) {
            ret = -RT_ENOMEM;
            break;
        }
        rt_thread_startup(tid);
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("create srv err");
    }
    return ret;
}

rt_err_t rtgui_send_request(rtgui_evt_generic_t *evt, rt_int32_t timeout) {
    if (_srv_app)
        return rtgui_request(_srv_app, evt, timeout);

    LOG_E("post bf srv start");
    return -RT_ENOSYS;
}

rt_err_t rtgui_send_request_sync(rtgui_evt_generic_t *evt) {
    if (_srv_app)
        return rtgui_request_sync(_srv_app, evt);

    LOG_E("post sync bf srv start");
    return -RT_ENOSYS;
}
