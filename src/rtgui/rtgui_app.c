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
 * 2012-01-13     Grissiom     first version(just a prototype of app API)
 * 2012-07-07     Bernard      move the send/recv message to the rtgui_system.c
 * 2019-05-15     onelife      Refactor
 */

#include "include/rthw.h"
#include "include/rtthread.h"

#include "../include/rtgui.h"
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


static void _app_constructor(void *obj);
static void _app_destructor(void *obj);
static rt_bool_t _app_event_handler(void *obj, rtgui_evt_generic_t *evt);

RTGUI_CLASS(
    app,
    CLASS_METADATA(object),
    _app_constructor,
    _app_destructor,
    _app_event_handler,
    sizeof(rtgui_app_t));


static void _app_constructor(void *obj) {
    rtgui_app_t *app = obj;

    app->name = RT_NULL;
    app->tid = RT_NULL;
    app->mb = RT_NULL;
    app->icon = RT_NULL;
    /* set EXITED so we can destroy an app that just created */
    app->state_flag = RTGUI_APP_FLAG_EXITED;
    app->ref_cnt = 0;
    app->window_cnt = 0;
    app->win_acti_cnt = 0;
    app->exit_code = 0;
    app->main_object = RT_NULL;
    app->on_idle = RT_NULL;
    LOG_D("app ctor");
}

static void _app_destructor(void *obj) {
    rtgui_app_t *app = obj;

    RT_ASSERT(app != RT_NULL);

    rt_free(app->name);
    app->name = RT_NULL;
}

static rtgui_app_t *_rtgui_app_create(const char *title, rtgui_evt_hdl_t evt_hdl,
    rt_bool_t is_srv) {
    rt_thread_t self = rt_thread_self();
    rtgui_app_t *app;

    RT_ASSERT(self);
    RT_ASSERT(!self->user_data); /* one thread one app only */
    RT_ASSERT(title);

    do {
        /* create app */
        app = (rtgui_app_t *)RTGUI_CREATE_INSTANCE(app, evt_hdl);
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
            if (rtgui_send_sync(srv, evt)) {
                LOG_E("create %s sync err", title);
                break;
            }
        }

        /* attach app to thread */
        self->user_data = (rt_uint32_t)app;
        LOG_D("create %s @%p", title, app);
        return app;
    } while (0);

    RTGUI_DELETE_INSTANCE(app);
    return RT_NULL;
}

rtgui_app_t *rtgui_srv_create(const char *title, rtgui_evt_hdl_t evt_hdl) {
    LOG_W("srv_hdl %p", evt_hdl);
    return _rtgui_app_create(title, evt_hdl, RT_TRUE);
}

rtgui_app_t *rtgui_app_create(const char *title, rtgui_evt_hdl_t evt_hdl) {
    return _rtgui_app_create(title, evt_hdl, RT_FALSE);
}
RTM_EXPORT(rtgui_app_create);

void rtgui_app_destroy(rtgui_app_t *app) {
    rtgui_app_t *srv;

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
            ret = rtgui_send_sync(srv, evt);
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
    RTGUI_DELETE_INSTANCE(app);
}
RTM_EXPORT(rtgui_app_destroy);

rtgui_app_t *rtgui_app_self(void) {
    rt_thread_t self;
    rtgui_app_t *app;

    /* get current thread */
    self = rt_thread_self();
    app = (rtgui_app_t *)(self->user_data);

    return app;
}
RTM_EXPORT(rtgui_app_self);

void rtgui_app_set_onidle(rtgui_app_t *app, rtgui_idle_hdl_t onidle) {
    _rtgui_application_check(app);
    app->on_idle = onidle;
}
RTM_EXPORT(rtgui_app_set_onidle);

rtgui_idle_hdl_t rtgui_app_get_onidle(rtgui_app_t *app) {
    _rtgui_application_check(app);
    return app->on_idle;
}
RTM_EXPORT(rtgui_app_get_onidle);

rt_inline rt_bool_t _app_dest_handler(
    rtgui_app_t *app, rtgui_evt_generic_t *evt) {
    rtgui_obj_t *tgt;
    (void)app;

    if (!evt->win_base.wid) return RT_FALSE;
    if (evt->win_base.wid->magic != RTGUI_WIN_MAGIC) return RT_FALSE;

    /* this window has been closed. */
    if (evt->win_base.wid && \
        (evt->win_base.wid->flag & RTGUI_WIN_FLAG_CLOSED)) {
        return RT_TRUE;
    }

    /* The dest window may have been destroyed when this event arrived. Check
     * against this condition. NOTE: we cannot use the TO_OBJECT because it
     * may be invalid already. */
    tgt = TO_OBJECT(evt->win_base.wid);
    if (tgt) {
        if (RTGUI_OBJECT_FLAG_VALID != (tgt->flag & RTGUI_OBJECT_FLAG_VALID)) {
            return RT_TRUE;
        }
        if (EVENT_HANDLER(tgt)) {
            return EVENT_HANDLER(tgt)(tgt, evt);
        } else {
            return RT_FALSE;
        }
    } else {
        LOG_E("evt %d no target", evt->base.type);
        return RT_FALSE;
    }
}

static rt_bool_t _app_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_app_t *app;
    rt_bool_t done = RT_TRUE;

    RT_ASSERT(obj != RT_NULL);
    RT_ASSERT(evt != RT_NULL);

    app = TO_APP(obj);

    switch (evt->base.type) {
    case RTGUI_EVENT_KBD:
        if (evt->kbd.win_acti_cnt == app->win_acti_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        if (evt->mouse.win_acti_cnt == app->win_acti_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_GESTURE:
        if (evt->gesture.win_acti_cnt == app->win_acti_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_WIN_ACTIVATE:
        app->win_acti_cnt++;
        done = _app_dest_handler(app, evt);
        break;

    case RTGUI_EVENT_PAINT:
    case RTGUI_EVENT_VPAINT_REQ:
    case RTGUI_EVENT_CLIP_INFO:
    case RTGUI_EVENT_WIN_DEACTIVATE:
    case RTGUI_EVENT_WIN_CLOSE:
    case RTGUI_EVENT_WIN_MOVE:
    case RTGUI_EVENT_WIN_SHOW:
    case RTGUI_EVENT_WIN_HIDE:
        done = _app_dest_handler(app, evt);
        break;

    case RTGUI_EVENT_APP_ACTIVATE:
        if (app->main_object) {
            /* send RTGUI_EVENT_WIN_SHOW */
            RTGUI_EVENT_WIN_SHOW_INIT(&evt->win_show);
            evt->win_show.wid = (rtgui_win_t *)app->main_object;
            done = EVENT_HANDLER(app->main_object)(app->main_object, evt);
        } else {
            LOG_W("%s without main_object", app->name);
        }
        break;

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
        done = EVENT_HANDLER(evt->model.view)(evt->model.view, evt);
        break;

    case RTGUI_EVENT_COMMAND:
        if (evt->command.wid) {
            done = _app_dest_handler(app, evt);
        } else {
            rtgui_topwin_t *top = rtgui_topwin_get_focus();
            if (top) {
                RT_ASSERT(top->flag & WINTITLE_ACTIVATE)
                /* send to focus window */
                evt->command.wid = top->wid;
                done = _app_dest_handler(app, evt);
            }
        }
        break;

    default:
        if (SUPER_HANDLER(app)) {
            done = SUPER_HANDLER(app)(app, evt);
        }
        break;
    }

    return done;
}

rt_inline void _rtgui_application_event_loop(rtgui_app_t *app) {
    rt_base_t current_cnt;
    rtgui_evt_generic_t *evt;

    current_cnt = ++app->ref_cnt;
    while (current_cnt <= app->ref_cnt) {
        rt_err_t ret;


        LOG_E("app %p", app);
        LOG_E("current_cnt %d", current_cnt);
        LOG_E("app->ref_cnt %d", app->ref_cnt);
        RT_ASSERT(current_cnt == app->ref_cnt);
        LOG_E("app->on_idle %d", app->on_idle);
        LOG_E("mb %d", app->mb->entry);
        evt = RT_NULL;

        if (app->on_idle) {
            ret = rtgui_recv(app, &evt, RT_WAITING_NO);
            LOG_E("loop1 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(TO_OBJECT(app), RT_NULL);
            }
        } else {
            LOG_E("mb %d", app->mb->entry);
            LOG_E("app->on_idle2 %d", app->on_idle);
            ret = rtgui_recv(app, &evt, RT_WAITING_FOREVER);
            LOG_E("rtgui_recv %d", ret);
            LOG_E("rtgui_recv %d", ret);
            LOG_E("rtgui_recv %d", ret);
            if (RT_EOK == ret) {
                LOG_E("loop2 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
                LOG_E("loop2 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
                LOG_E("loop2 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
                LOG_E("loop2 %p from %s", evt, evt->base.sender->mb->parent.parent.name);
                (void)EVENT_HANDLER(app)(app, evt);
                LOG_E("hdl %p", EVENT_HANDLER(app));
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

void rtgui_app_exit(rtgui_app_t *app, rt_uint16_t code) {
    if (!app->ref_cnt) return;

    app->ref_cnt--;
    app->exit_code = code;
}
RTM_EXPORT(rtgui_app_exit);

void rtgui_app_activate(rtgui_app_t *app) {
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

void rtgui_app_sleep(rtgui_app_t *app, rt_uint32_t ms) {
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
            ret = rtgui_recv(app, &evt, RT_WAITING_NO);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app)(app, evt);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(TO_OBJECT(app), RT_NULL);
            }
        } else {
            ret = rtgui_recv(app, &evt, timeout - current);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app)(app, evt);
            }
        }
        current = rt_tick_get();
    }

    app->ref_cnt--;
}
RTM_EXPORT(rtgui_app_sleep);

void rtgui_app_close(rtgui_app_t *app) {
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
 * set this app as window manager
 */
rt_err_t rtgui_app_set_as_wm(rtgui_app_t *app) {
    rtgui_app_t *srv;
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
            ret = rtgui_send_sync(srv, evt);
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

void rtgui_app_set_main_win(rtgui_app_t *app, rtgui_win_t *win) {
    _rtgui_application_check(app);
    app->main_object = TO_OBJECT(win);
    LOG_D("%s main win: %s", app->name, win->title);
}
RTM_EXPORT(rtgui_app_set_main_win);

rtgui_win_t *rtgui_app_get_main_win(rtgui_app_t *app) {
    return TO_WIN(app->main_object);
}
RTM_EXPORT(rtgui_app_get_main_win);

unsigned int rtgui_app_get_win_acti_cnt(void) {
    rtgui_app_t *app;

    app = rtgui_topwin_app_get_focus();
    if (app) {
        return app->win_acti_cnt;
    } else {
        return 0;
    }
}
RTM_EXPORT(rtgui_app_get_win_acti_cnt);
