/*
 * File      : rtgui_app.c
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
 * 2012-01-13     Grissiom     first version(just a prototype of application API)
 * 2012-07-07     Bernard      move the send/recv message to the rtgui_system.c
 * 2019-05-15     onelife      Refactor
 */

#include "include/rthw.h"
#include "include/rtthread.h"

#include "../include/rtgui_system.h"
#include "../include/rtgui_app.h"
#include "../include/widgets/window.h"
#include "../include/widgets/topwin.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_APP"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */


#define _rtgui_application_check(app)           \
    do {                                        \
        RT_ASSERT(app != RT_NULL);              \
        RT_ASSERT(app->tid != RT_NULL);         \
        RT_ASSERT(app->tid->user_data != 0);    \
        RT_ASSERT(app->mb != RT_NULL);          \
    } while (0)


static void _rtgui_app_constructor(struct rtgui_app *app) {
    /* set event handler */
    rtgui_object_set_event_handler(
        RTGUI_OBJECT(app), rtgui_app_event_handler);

    app->name           = RT_NULL;
    app->tid            = RT_NULL;
    app->mb             = RT_NULL;
    app->icon           = RT_NULL;
    /* set EXITED so we can destroy an application that just created */
    app->state_flag     = RTGUI_APP_FLAG_EXITED;
    app->ref_cnt      = 0;
    app->window_cnt     = 0;
    app->win_acti_cnt   = 0;
    app->exit_code      = 0;
    app->main_object    = RT_NULL;
    app->on_idle        = RT_NULL;
}

static void _rtgui_app_destructor(struct rtgui_app *app) {
    RT_ASSERT(app != RT_NULL);

    rt_free(app->name);
    app->name = RT_NULL;
}

DEFINE_CLASS_TYPE(
    application, "application",
    RTGUI_PARENT_TYPE(object),
    _rtgui_app_constructor,
    _rtgui_app_destructor,
    sizeof(struct rtgui_app));

static rtgui_app_t *_rtgui_app_create(const char *title, rt_bool_t is_srv) {
    rt_thread_t self = rt_thread_self();
    rtgui_app_t *app;

    RT_ASSERT(self);
    RT_ASSERT(!self->user_data); /* one thread one app only */
    RT_ASSERT(title);

    do {
        /* create application */
        app = RTGUI_APP(rtgui_object_create(RTGUI_APP_TYPE));
        if (!app) {
            LOG_E("create %s err", title);
            break;
        }
        app->tid = self;
        app->mb = rt_mb_create(title, RTGUI_MB_SIZE, RT_IPC_FLAG_FIFO);
        if (!app->mb) {
            LOG_E("mb %s mem err", title);
            break;
        }
        app->name = rt_strdup(title);
        if (!app->name) {
            LOG_E("name mem err");
            break;
        }

        if (is_srv) {
            rtgui_event_pool = rt_mp_create(title, RTGUI_EVENT_POOL_NUMBER,
                sizeof(rtgui_evt_generic_t));
            if (!rtgui_event_pool) {
                LOG_E("mp %s mem err", title);
                break;
            }
        } else {
            rtgui_app_t *srv;
            rtgui_evt_generic_t *evt;

            srv = rtgui_get_server();
            if (!srv) {
                LOG_E("srv not started");
                break;
            }

            evt = (rtgui_evt_generic_t *)rt_mp_alloc(rtgui_event_pool, 0);
            if (!evt) {
                LOG_E("get mp err", title);
                break;
            }
            /* send RTGUI_EVENT_APP_CREATE */
            RTGUI_EVENT_APP_CREATE_INIT(&evt->app_create);
            evt->app_create.app = app;
            if (rtgui_send_sync(srv, app, evt)) {
                LOG_E("%s sync %s err", title, RTGUI_APP_TYPE->name);
                break;
            }
        }

        /* attach app to thread */
        self->user_data = (rt_uint32_t)app;
        LOG_D("create %s @%p", title, app);
        return app;
    } while (0);

    rtgui_object_destroy(RTGUI_OBJECT(app));
    return RT_NULL;
}

rtgui_app_t *rtgui_srv_create(const char *title) {
    return _rtgui_app_create(title, RT_TRUE);
}

rtgui_app_t *rtgui_app_create(const char *title) {
    return _rtgui_app_create(title, RT_FALSE);
}
RTM_EXPORT(rtgui_app_create);

void rtgui_app_destroy(struct rtgui_app *app) {
    struct rtgui_app *srv;

    _rtgui_application_check(app);
    if (!(app->state_flag & RTGUI_APP_FLAG_EXITED)) {
        LOG_E("destroy bf stop %s", app->name);
        return;
    }

    srv = rtgui_get_server();
    if (srv != rtgui_app_self()) {
        rtgui_evt_generic_t *evt;
        rt_err_t ret;

        /* send RTGUI_EVENT_APP_DESTROY */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_APP_DESTROY_INIT(&evt->app_destroy);
            evt->app_destroy.app = app;
            ret = rtgui_send_sync(srv, app, evt);
            if (ret) {
                LOG_E("destroy %s err [%d]", app->name, ret);
                return;
            }
        } else {
            LOG_E("get mp err");
            return;
        }
    }

    app->tid->user_data = 0;
    rt_mb_delete(app->mb);
    rtgui_object_destroy(RTGUI_OBJECT(app));
}
RTM_EXPORT(rtgui_app_destroy);

struct rtgui_app *rtgui_app_self(void) {
    rt_thread_t self;
    struct rtgui_app *app;

    /* get current thread */
    self = rt_thread_self();
    app = (struct rtgui_app *)(self->user_data);

    return app;
}
RTM_EXPORT(rtgui_app_self);

void rtgui_app_set_onidle(struct rtgui_app *app, rtgui_idle_func_t onidle) {
    _rtgui_application_check(app);
    app->on_idle = onidle;
}
RTM_EXPORT(rtgui_app_set_onidle);

rtgui_idle_func_t rtgui_app_get_onidle(struct rtgui_app *app) {
    _rtgui_application_check(app);
    return app->on_idle;
}
RTM_EXPORT(rtgui_app_get_onidle);

rt_inline rt_bool_t _rtgui_application_dest_handle(
    struct rtgui_app *app, union rtgui_evt_generic *evt) {
    struct rtgui_obj *tgt;
    (void)app;

    if (!evt->win_base.wid) return RT_FALSE;
    if (evt->win_base.wid->magic != RTGUI_WIN_MAGIC) return RT_FALSE;

    /* this window has been closed. */
    if (evt->win_base.wid && \
        (evt->win_base.wid->flag & RTGUI_WIN_FLAG_CLOSED)) {
        return RT_TRUE;
    }

    /* The dest window may have been destroyed when this event arrived. Check
     * against this condition. NOTE: we cannot use the RTGUI_OBJECT because it
     * may be invalid already. */
    tgt = RTGUI_OBJECT(evt->win_base.wid);
    if (tgt) {
        if (RTGUI_OBJECT_FLAG_VALID != (tgt->flag & RTGUI_OBJECT_FLAG_VALID)) {
            return RT_TRUE;
        }
        if (tgt->event_handler) {
            return tgt->event_handler(tgt, evt);
        } else {
            return RT_FALSE;
        }
    } else {
        LOG_E("evt %d no target", evt->base.type);
        return RT_FALSE;
    }
}

rt_bool_t rtgui_app_event_handler(struct rtgui_obj *obj,
    union rtgui_evt_generic *evt) {
    struct rtgui_app *app;

    RT_ASSERT(obj != RT_NULL);
    RT_ASSERT(evt != RT_NULL);

    app = RTGUI_APP(obj);

    switch (evt->base.type) {
    case RTGUI_EVENT_KBD:
        if (evt->kbd.win_acti_cnt != app->win_acti_cnt) break;
        return _rtgui_application_dest_handle(app, evt);

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        if (evt->mouse.win_acti_cnt != app->win_acti_cnt) break;
        return _rtgui_application_dest_handle(app, evt);

    case RTGUI_EVENT_GESTURE:
        if (evt->gesture.win_acti_cnt != app->win_acti_cnt) break;
        return  _rtgui_application_dest_handle(app, evt);

    case RTGUI_EVENT_WIN_ACTIVATE:
        app->win_acti_cnt++;
        return _rtgui_application_dest_handle(app, evt);

    case RTGUI_EVENT_PAINT:
    case RTGUI_EVENT_VPAINT_REQ:
    case RTGUI_EVENT_CLIP_INFO:
    case RTGUI_EVENT_WIN_DEACTIVATE:
    case RTGUI_EVENT_WIN_CLOSE:
    case RTGUI_EVENT_WIN_MOVE:
    case RTGUI_EVENT_WIN_SHOW:
    case RTGUI_EVENT_WIN_HIDE:
        return _rtgui_application_dest_handle(app, evt);

    case RTGUI_EVENT_APP_ACTIVATE:
        if (app->main_object) {
            /* send RTGUI_EVENT_WIN_SHOW */
            RTGUI_EVENT_WIN_SHOW_INIT(&evt->win_show);
            evt->win_show.wid = (struct rtgui_win *)app->main_object;
            return rtgui_object_handle(app->main_object, evt);
        } else {
            LOG_W("%s without main_object", app->name);
        }

    case RTGUI_EVENT_APP_DESTROY:
        rtgui_app_exit(app, 0);
        break;

    case RTGUI_EVENT_TIMER:
    {
        struct rtgui_timer *timer = evt->timer.timer;
        rt_base_t level = rt_hw_interrupt_disable();
        timer->pending_cnt--;
        rt_hw_interrupt_enable(level);

        RT_ASSERT(timer->pending_cnt >= 0);
        if (RTGUI_TIMER_ST_DESTROY_PENDING == timer->state) {
            /* Truly destroy the timer when there is no pending event. */
            if (!timer->pending_cnt) {
                rtgui_timer_destory(timer);
            }
        } else if ((RTGUI_TIMER_ST_RUNNING == timer->state) && timer->timeout) {
            /* call timeout function */
            timer->timeout(timer, timer->user_data);
        }
        break;
    }

    case RTGUI_EVENT_MV_MODEL:
        RT_ASSERT(evt->model.view);
        return rtgui_object_handle(RTGUI_OBJECT(evt->model.view), evt);

    case RTGUI_EVENT_COMMAND:
        if (evt->command.wid) {
            return _rtgui_application_dest_handle(app, evt);
        } else {
            struct rtgui_topwin *top = rtgui_topwin_get_focus();
            if (top) {
                RT_ASSERT(top->flag & WINTITLE_ACTIVATE)
                /* send to focus window */
                evt->command.wid = top->wid;
                return _rtgui_application_dest_handle(app, evt);
            }
        }

    default:
        return rtgui_object_event_handler(obj, evt);
    }

    return RT_TRUE;
}
RTM_EXPORT(rtgui_app_event_handler);

rt_inline void _rtgui_application_event_loop(rtgui_app_t *app) {
    rt_base_t current_cnt;

    current_cnt = ++app->ref_cnt;
    while (current_cnt <= app->ref_cnt) {
        rtgui_evt_generic_t *evt = RT_NULL;
        rt_err_t ret;

        RT_ASSERT(current_cnt == app->ref_cnt);
        if (app->on_idle) {
            ret = rtgui_recv(&evt, RT_WAITING_NO);
            if (RT_EOK == ret) {
                RTGUI_OBJECT(app)->event_handler(RTGUI_OBJECT(app), evt);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(RTGUI_OBJECT(app), RT_NULL);
            }
        } else {
            ret = rtgui_recv(&evt, RT_WAITING_FOREVER);
            if (RT_EOK == ret) {
                LOG_E("loop2 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
                RTGUI_OBJECT(app)->event_handler(RTGUI_OBJECT(app), evt);
            }
        }
    }
}

rt_base_t rtgui_app_run(rtgui_app_t *app) {
    _rtgui_application_check(app);
    app->state_flag &= ~RTGUI_APP_FLAG_EXITED;

    LOG_D("loop %s ", app->name);
    _rtgui_application_event_loop(app);

    if (!app->ref_cnt) {
        app->state_flag |= RTGUI_APP_FLAG_EXITED;
        LOG_D("exit %s", app->name);
    }

    return app->exit_code;
}
RTM_EXPORT(rtgui_app_run);

void rtgui_app_exit(struct rtgui_app *app, rt_uint16_t code) {
    if (!app->ref_cnt) return;

    app->ref_cnt--;
    app->exit_code = code;
}
RTM_EXPORT(rtgui_app_exit);

void rtgui_app_activate(struct rtgui_app *app) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;

    /* send RTGUI_EVENT_APP_ACTIVATE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_APP_ACTIVATE_INIT(&evt->app_activate);
        evt->app_activate.app = app;
        ret = rtgui_send(app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("active %s err [%d]", app->name, ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }
}
RTM_EXPORT(rtgui_app_activate);

void rtgui_app_sleep(struct rtgui_app *app, rt_uint32_t ms) {
    rt_tick_t current, timeout;
    rt_uint16_t current_cnt;

    current = rt_tick_get();
    timeout = current + (rt_tick_t)rt_tick_from_millisecond(ms);

    current_cnt = ++app->ref_cnt;
    while (current_cnt <= app->ref_cnt && (current < timeout)) {
        rtgui_evt_generic_t *evt = RT_NULL;
        rt_err_t ret;

        RT_ASSERT(current_cnt == app->ref_cnt);
        if (app->on_idle) {
            ret = rtgui_recv(&evt, RT_WAITING_NO);
            if (RT_EOK == ret) {
                RTGUI_OBJECT(app)->event_handler(RTGUI_OBJECT(app), evt);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(RTGUI_OBJECT(app), RT_NULL);
            }
        } else {
            ret = rtgui_recv(&evt, timeout - current);
            if (RT_EOK == ret) {
                RTGUI_OBJECT(app)->event_handler(RTGUI_OBJECT(app), evt);
            }
        }
        current = rt_tick_get();
    }

    app->ref_cnt--;
}
RTM_EXPORT(rtgui_app_sleep);

void rtgui_app_close(struct rtgui_app *app) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;

    /* send RTGUI_EVENT_APP_DESTROY */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_APP_DESTROY_INIT(&evt->app_destroy);
        evt->app_destroy.app = app;
        ret = rtgui_send(app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("close %s err [%d]", app->name, ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }
}
RTM_EXPORT(rtgui_app_close);

/**
 * set this application as window manager
 */
rt_err_t rtgui_app_set_as_wm(struct rtgui_app *app) {
    struct rtgui_app *srv;
    rt_err_t ret;

    _rtgui_application_check(app);

    srv = rtgui_get_server();
    if (srv) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_SET_WM */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_SET_WM_INIT(&evt->set_wm);
            evt->set_wm.app = app;
            ret = rtgui_send_sync(srv, app, evt);
            if (ret) {
                LOG_E("%s set WM err [%d]", app->name, ret);
                return ret;
            }
        } else {
            LOG_E("get mp err");
            ret = -RT_ERROR;
        }
    } else {
        ret = -RT_ENOSYS;
    }

    return ret;
}
RTM_EXPORT(rtgui_app_set_as_wm);

void rtgui_app_set_main_win(struct rtgui_app *app, struct rtgui_win *win) {
    _rtgui_application_check(app);
    app->main_object = RTGUI_OBJECT(win);
    LOG_D("%s main win: %s", app->name, win->title);
}
RTM_EXPORT(rtgui_app_set_main_win);

struct rtgui_win *rtgui_app_get_main_win(struct rtgui_app *app) {
    return RTGUI_WIN(app->main_object);
}
RTM_EXPORT(rtgui_app_get_main_win);

unsigned int rtgui_app_get_win_acti_cnt(void) {
    struct rtgui_app *app;

    app = rtgui_topwin_app_get_focus();
    if (app) {
        return app->win_acti_cnt;
    } else {
        return 0;
    }
}
RTM_EXPORT(rtgui_app_get_win_acti_cnt);
