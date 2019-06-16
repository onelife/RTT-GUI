/*
 * File      : app.c
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
 * 2019-05-15     onelife      refactor and rename to "app.c"
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rthw.h" // rtt: rt_hw_interrupt_disable()

#include "include/rtgui.h"
#include "include/app.h"
#include "include/widgets/window.h"
#include "include/widgets/topwin.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_APP"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define _rtgui_application_check(app)           \
    do {                                        \
        RT_ASSERT(app != RT_NULL);              \
        RT_ASSERT(app->tid != RT_NULL);         \
        RT_ASSERT(app->tid->user_data != 0);    \
        RT_ASSERT(app->mb != RT_NULL);          \
    } while (0)

/* Private function prototypes -----------------------------------------------*/
static void _app_constructor(void *obj);
static void _app_destructor(void *obj);
static rt_bool_t _app_event_handler(void *obj, rtgui_evt_generic_t *evt);

/* Private variables ---------------------------------------------------------*/
struct rt_mempool *rtgui_event_pool = RT_NULL;

RTGUI_CLASS(
    app,
    CLASS_METADATA(object),
    _app_constructor,
    _app_destructor,
    _app_event_handler,
    sizeof(rtgui_app_t));

/* Private functions ---------------------------------------------------------*/
static void _app_constructor(void *obj) {
    rtgui_app_t *app = obj;

    app->tid = RT_NULL;
    app->main_win = RT_NULL;
    app->name = RT_NULL;
    app->flag = RTGUI_APP_FLAG_EXITED;
    app->mb = RT_NULL;
    app->icon = RT_NULL;
    app->on_idle = RT_NULL;
    app->user_data = RT_NULL;
    app->ref_cnt = 0;
    app->win_cnt = 0;
    app->act_cnt = 0;
    app->exit_code = 0;
    LOG_D("app ctor");
}

static void _app_destructor(void *obj) {
    rtgui_app_t *app = obj;

    if (app->name) {
        rtgui_free(app->name);
        app->name = RT_NULL;
    }
}

rt_err_t rtgui_app_init(rtgui_app_t *app, const char *name, rt_bool_t is_srv) {
    rt_err_t ret;

    RT_ASSERT(name);
    ret = RT_EOK;

    do {
        rt_thread_t self = rt_thread_self();
        if (!self) {
            LOG_E("not thread contex");
            ret = -RT_ERROR;
            break;
        }
        if (!self->user_data) {
            LOG_E("already has app");
            ret = -RT_ERROR;
            break;
        }
        app->tid = self;

        app->name = rt_strdup(name);
        if (!app->name) {
            LOG_E("name mem err");
            ret = -RT_ENOMEM;
            break;
        }
        app->mb = rt_mb_create(name, RTGUI_MB_SIZE, RT_IPC_FLAG_FIFO);
        if (!app->mb) {
            LOG_E("mb %s mem err", name);
            ret = -RT_ENOMEM;
            break;
        }
        self->user_data = (rt_uint32_t)app;

        if (is_srv) {
            rtgui_event_pool = rt_mp_create(name, RTGUI_EVENT_POOL_NUMBER,
                sizeof(rtgui_evt_generic_t));
            if (!rtgui_event_pool) {
                LOG_E("mp %s mem err", name);
                ret = -RT_ENOMEM;
                break;
            }
        } else {
            rtgui_app_t *srv = rtgui_get_server();
            rtgui_evt_generic_t *evt;

            if (!srv) {
                LOG_E("srv not start");
                ret = -RT_ERROR;
                break;
            }

            /* send RTGUI_EVENT_APP_CREATE */
            RTGUI_CREATE_EVENT(evt, APP_CREATE, RT_WAITING_FOREVER);
            if (!evt) {
                ret = -RT_ENOMEM;
                break;
            }
            evt->app_create.app = app;
            ret = rtgui_request_sync(srv, evt);
            if (RT_EOK != ret) break;
        }

        LOG_D("create %s @%p", name, app);
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_app_init);

void rtgui_app_uninit(rtgui_app_t *app) {
    do {
        rtgui_app_t *srv;

        if (!IS_APP_FLAG(app, EXITED)) {
            LOG_E("uninit bf exit %s", app->name);
            break;
        }

        srv = rtgui_get_server();
        if (srv != rtgui_app_self()) {
            rtgui_evt_generic_t *evt;

            /* send RTGUI_EVENT_APP_DESTROY */
            RTGUI_CREATE_EVENT(evt, APP_DESTROY, RT_WAITING_FOREVER);
            if (!evt) break;
            evt->app_destroy.app = app;
            if (rtgui_request_sync(srv, evt)) break;
        }

        rt_mb_delete(app->mb);
        app->tid->user_data = RT_NULL;
        DELETE_INSTANCE(app);
    } while (0);
}
RTM_EXPORT(rtgui_app_uninit);

rtgui_app_t *rtgui_app_self(void) {
    rt_thread_t self = rt_thread_self();
    if (!self) return RT_NULL;
    return (rtgui_app_t *)(self->user_data);
}
RTM_EXPORT(rtgui_app_self);

RTGUI_MEMBER_SETTER_GETTER(rtgui_app_t, app, rtgui_idle_hdl_t, on_idle);

rt_inline rt_bool_t _app_dest_handler(
    rtgui_app_t *app, rtgui_evt_generic_t *evt) {
    rtgui_obj_t *tgt;
    (void)app;

    if (!evt->win_base.wid) return RT_FALSE;
    if (evt->win_base.wid->_magic != RTGUI_WIN_MAGIC) return RT_FALSE;

    /* this window has been closed. */
    if (evt->win_base.wid && IS_WIN_FLAG(evt->win_base.wid, CLOSED))
        return RT_TRUE;

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
        LOG_E("[AppEVT] %d no target", evt->base.type);
        return RT_FALSE;
    }
}

static rt_bool_t _app_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_app_t *app;
    rt_bool_t done = RT_TRUE;

    RT_ASSERT(obj != RT_NULL);
    RT_ASSERT(evt != RT_NULL);

    EVT_LOG("[AppEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);
    app = TO_APP(obj);

    switch (evt->base.type) {
    case RTGUI_EVENT_KBD:
        if (evt->kbd.act_cnt == app->act_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        if (evt->mouse.act_cnt == app->act_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_GESTURE:
        if (evt->gesture.act_cnt == app->act_cnt) {
            done = _app_dest_handler(app, evt);
        }
        break;

    case RTGUI_EVENT_WIN_ACTIVATE:
        app->act_cnt++;
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
        if (app->main_win) {
            /* send RTGUI_EVENT_WIN_SHOW */
            RTGUI_EVENT_REINIT(evt, WIN_SHOW);
            evt->win_show.wid = app->main_win;
            done = EVENT_HANDLER(app->main_win)(app->main_win, evt);
        } else {
            LOG_W("[AppEVT] %s without main_win", app->name);
        }
        break;

    case RTGUI_EVENT_APP_DESTROY:
        rtgui_app_exit(app, 0);
        break;

    case RTGUI_EVENT_TIMER:
    {
        rtgui_timer_t *timer = evt->timer.timer;
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
                RT_ASSERT(top->flag & RTGUI_TOPWIN_FLAG_ACTIVATE)
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

    EVT_LOG("[AppEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    if (!done) {
        LOG_W("[AppEVT] %p not done", evt);
    }
    if (done && evt) {
        RTGUI_FREE_EVENT(evt);
    }

    return done;
}

rt_inline void _rtgui_application_event_loop(rtgui_app_t *app) {
    rt_base_t current_cnt;
    rtgui_evt_generic_t *evt;

    current_cnt = ++app->ref_cnt;
    while (current_cnt <= app->ref_cnt) {
        rt_err_t ret;

        if (current_cnt != app->ref_cnt) {
            LOG_E("%s cnt %d != %d", app->name, current_cnt, app->ref_cnt);
        }
        evt = RT_NULL;
        LOG_D("%s mb: %d", app->name, app->mb->entry);

        if (app->on_idle) {
            ret = rtgui_wait(app, &evt, RT_WAITING_NO);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(TO_OBJECT(app), RT_NULL);
            }
        } else {
            ret = rtgui_wait(app, &evt, RT_WAITING_FOREVER);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app)(app, evt);
            }
        }
    }
}

rt_base_t rtgui_app_run(rtgui_app_t *app) {
    APP_FLAG_CLEAR(app, EXITED);

    LOG_D("loop %s ", app->name);
    _rtgui_application_event_loop(app);

    if (!app->ref_cnt) {
        APP_FLAG_SET(app, EXITED);
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
    RTGUI_CREATE_EVENT(evt, APP_ACTIVATE, RT_WAITING_FOREVER);
    if (evt) {
        evt->app_activate.app = app;
        ret = rtgui_request(app, evt, RT_WAITING_FOREVER);
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
        rtgui_evt_generic_t *evt;
        rt_err_t ret;

        RT_ASSERT(current_cnt == app->ref_cnt);
        if (app->on_idle) {
            ret = rtgui_wait(app, &evt, RT_WAITING_NO);
            if (RT_EOK == ret) {
                (void)EVENT_HANDLER(app)(app, evt);
            } else if (-RT_ETIMEOUT == ret) {
                app->on_idle(TO_OBJECT(app), RT_NULL);
            }
        } else {
            ret = rtgui_wait(app, &evt, timeout - current);
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
    RTGUI_CREATE_EVENT(evt, APP_DESTROY, RT_WAITING_FOREVER);
    if (evt) {
        evt->app_destroy.app = app;
        ret = rtgui_request(app, evt, RT_WAITING_FOREVER);
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
        RTGUI_CREATE_EVENT(evt, SET_WM, RT_WAITING_FOREVER);
        if (evt) {
            evt->set_wm.app = app;
            ret = rtgui_request_sync(srv, evt);
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

/* Public functions ----------------------------------------------------------*/
RTGUI_MEMBER_SETTER_GETTER(rtgui_app_t, app, rtgui_win_t*, main_win);

unsigned int rtgui_get_app_act_cnt(void) {
    rtgui_app_t *app;

    app = rtgui_topwin_get_focus_app();
    if (app) {
        return app->act_cnt;
    } else {
        return 0;
    }
}
RTM_EXPORT(rtgui_get_app_act_cnt);
