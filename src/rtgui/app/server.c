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
#include "include/driver.h"
//#include <rtgui/touch.h>
#include "include/widgets/window.h"
#include "include/app/app.h"
#include "include/app/topwin.h"
#include "include/app/mouse.h"

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
static rt_bool_t _server_mouse_motion_event_handler(rtgui_evt_generic_t *evt) {
    rt_bool_t done = RT_TRUE;

    do {
        rtgui_topwin_t *topwin = rtgui_topwin_get_at(
            evt->mouse.x, evt->mouse.y);
        if (topwin && topwin->monitor_list.next) {
            // FIXME:
            /* check whether the monitor exist */
            if (!rtgui_mouse_monitor_contains_point(
                &(topwin->monitor_list), evt->mouse.x, evt->mouse.y)) {
                topwin = RT_NULL;
            }
        }
        if (!topwin) break;

        /* send RTGUI_EVENT_MOUSE_MOTION */
        /* change owner to server */
        RTGUI_EVENT_REINIT(evt, MOUSE_MOTION);
        evt->mouse.wid = topwin->wid;
        evt->mouse.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(topwin->wid->app, evt, RT_WAITING_NO);
        //TODO: topwin->wid->app? topwin->app?
        done = RT_FALSE;
    } while (0);

    /* move mouse */
    rtgui_mouse_moveto(evt->mouse.x, evt->mouse.y);

    return done;
}

static rt_bool_t _server_mouse_btn_event_handler(rtgui_evt_generic_t *evt) {
    rtgui_topwin_t *topwin;
    rt_bool_t done = RT_TRUE;

    do {
        #ifdef RTGUI_USING_WINMOVE
        if (rtgui_winrect_is_moved() && (evt->mouse.button & \
            (RTGUI_MOUSE_BUTTON_LEFT | RTGUI_MOUSE_BUTTON_UP))) {
            rtgui_win_t *win;
            rtgui_rect_t rect;

            if (rtgui_winrect_moved_done(&rect, &win)) {
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

        /* send RTGUI_EVENT_MOUSE_BUTTON */
        /* change owner to server */
        RTGUI_EVENT_REINIT(evt, MOUSE_BUTTON);
        /* set cursor position */
        rtgui_mouse_set_position(evt->mouse.x, evt->mouse.y);

        /* get the topwin which contains the mouse */
        topwin = rtgui_topwin_get_at(evt->mouse.x, evt->mouse.y);
        if (!topwin) break;

        /* only raise window if the button is pressed down */
        if ((evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) && \
            (rtgui_topwin_get_focus() != topwin)) {
            rtgui_topwin_activate(topwin);
        }

        evt->mouse.wid = topwin->wid;
        evt->mouse.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(topwin->app, evt, RT_WAITING_NO);
        done = RT_FALSE;
    } while (0);

    return done;
}

static rt_bool_t _server_touch_event_handler(rtgui_evt_generic_t *evt) {
    (void)evt;
    return RT_FALSE;
    // evt->touch
    //  if (rtgui_touch_do_calibration(evt) == RT_TRUE)
    //  {
    //      struct rtgui_event_mouse emouse;

    //      /* convert it as a mouse event to rtgui */
    //      if (evt->up_down == RTGUI_TOUCH_MOTION)
    //      {
    //          RTGUI_EVENT_MOUSE_MOTION_INIT(&emouse);
    //          emouse.x = evt->x;
    //          emouse.y = evt->y;
    //          emouse.button = 0;

    //          _server_mouse_motion_event_handler(&emouse);
    //      }
    //      else
    //      {
    //          RTGUI_EVENT_MOUSE_BUTTON_INIT(&emouse);
    //          emouse.x = evt->x;
    //          emouse.y = evt->y;
    //          emouse.button = RTGUI_MOUSE_BUTTON_LEFT;
    //          if (evt->up_down == RTGUI_TOUCH_UP)
    //              emouse.button |= RTGUI_MOUSE_BUTTON_UP;
    //          else
    //              emouse.button |= RTGUI_MOUSE_BUTTON_DOWN;

    //          _server_mouse_btn_event_handler(&emouse);
    //      }
    //  }
}

static rt_bool_t _server_kbd_event_handler(rtgui_evt_generic_t *evt) {
    rtgui_topwin_t *topwin;
    rt_bool_t done = RT_TRUE;

    /* todo: handle input method and global shortcut */
    topwin = rtgui_topwin_get_focus();
    if (topwin) {
        RT_ASSERT(topwin->flag & RTGUI_TOPWIN_FLAG_ACTIVATE);  // TODO(onelife): ?

        /* send RTGUI_EVENT_KBD */
        /* change owner to server */
        RTGUI_EVENT_REINIT(evt, KBD);
        /* send to focus window */
        evt->kbd.wid = topwin->wid;
        evt->kbd.act_cnt = rtgui_get_app_act_cnt();
        rtgui_request(topwin->app, evt, RT_WAITING_NO);
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

    /* mouse and keyboard evt */
    case RTGUI_EVENT_MOUSE_MOTION:
        /* handle mouse motion event */
        done = _server_mouse_motion_event_handler(evt);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        /* handle mouse button */
        done = _server_mouse_btn_event_handler(evt);
        break;

    case RTGUI_EVENT_TOUCH:
        /* handle touch event */
        done = _server_touch_event_handler(evt);
        break;

    case RTGUI_EVENT_KBD:
        /* handle keyboard event */
        done = _server_kbd_event_handler(evt);
        break;

    /* window event */
    case RTGUI_EVENT_WIN_CREATE:
        if (RT_EOK != rtgui_topwin_add(evt->base.origin, evt->win_create.wid,
            evt->win_create.parent_window))
        // if (RT_EOK != rtgui_topwin_add(&evt->win_create))
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

    /* other event */
    case RTGUI_EVENT_COMMAND:
        break;

    case RTGUI_EVENT_UPDATE_BEGIN:
        #ifdef RTGUI_USING_MOUSE_CURSOR
            /* hide cursor */
            rtgui_mouse_hide_cursor();
        #endif
        break;

    case RTGUI_EVENT_UPDATE_END:
        {
            /* handle screen update */
            rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();
            if (drv) {
                rtgui_graphic_driver_screen_update(drv,
                    &(evt->update_end.rect));
            }
            #ifdef RTGUI_USING_MOUSE_CURSOR
                /* show cursor */
                rtgui_mouse_show_cursor();
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
        LOG_E("[SrvEVT] bad event [%d]", evt->base.type);
        ack = RT_ERROR;
        break;
    }

    EVT_LOG("[SrvEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    if (!done) {
        LOG_W("[SrvEVT] %p not done", evt);
    }
    if (done && evt) {
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
    #ifdef RTGUI_USING_MOUSE_CURSOR
        rtgui_mouse_show_cursor();
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
        ret = rtgui_mouse_init();
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

rt_err_t rtgui_send_request(rtgui_evt_generic_t *evt) {
    if (_srv_app)
        return rtgui_request(_srv_app, evt, RT_WAITING_FOREVER);

    LOG_E("post bf srv start");
    return -RT_ENOSYS;
}

rt_err_t rtgui_send_request_sync(rtgui_evt_generic_t *evt) {
    if (_srv_app)
        return rtgui_request_sync(_srv_app, evt);

    LOG_E("post sync bf srv start");
    return -RT_ENOSYS;
}
