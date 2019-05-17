/*
 * File      : window.c
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
 * 2019-05-15     onelife      Refactor
 */

#include "../include/dc.h"
#include "../include/color.h"
#include "../include/image.h"
#include "../include/rtgui_system.h"
#include "../include/rtgui_server.h"
#include "../include/rtgui_app.h"

#include "../include/widgets/window.h"
#include "../include/widgets/title.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_WIN"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

static void _rtgui_win_constructor(rtgui_win_t *win);
static void _rtgui_win_destructor(rtgui_win_t *win);
static rt_bool_t _rtgui_win_deal_close(struct rtgui_win *win,
    rtgui_evt_generic_t *event, rt_bool_t force);

DEFINE_CLASS_TYPE(
    win, "win",
    RTGUI_PARENT_TYPE(container),
    _rtgui_win_constructor,
    _rtgui_win_destructor,
    sizeof(struct rtgui_win));


static void _rtgui_win_constructor(rtgui_win_t *win) {
    /* set toplevel to self */
    RTGUI_WIDGET(win)->toplevel = win;

    /* init win property */
    win->update = 0;
    win->drawing = 0;

    RTGUI_WIDGET(win)->flag |= RTGUI_WIDGET_FLAG_FOCUSABLE;
    win->parent_window = RT_NULL;
    win->app = rtgui_app_self();
    /* init window attribute */
    win->on_activate   = RT_NULL;
    win->on_deactivate = RT_NULL;
    win->on_close      = RT_NULL;
    win->on_key        = RT_NULL;
    win->title         = RT_NULL;
    win->_title_wgt    = RT_NULL;
    win->modal_code    = RTGUI_MODAL_OK;

    /* initialize last mouse event handled widget */
    win->last_mevent_widget = RT_NULL;
    win->focused_widget = RT_NULL;

    /* set window hide */
    RTGUI_WIDGET_HIDE(win);

    /* set window style */
    win->style = RTGUI_WIN_STYLE_DEFAULT;

    win->flag  = RTGUI_WIN_FLAG_INIT;

    rtgui_object_set_event_handler(RTGUI_OBJECT(win), rtgui_win_event_handler);

    /* init user data */
    win->user_data = 0;

    win->_do_show = rtgui_win_do_show;
}

static void _rtgui_win_destructor(rtgui_win_t *win) {
    if (win->flag & RTGUI_WIN_FLAG_CONNECTED) {
        rtgui_evt_generic_t *evt;
        rt_err_t ret;

        /* send RTGUI_EVENT_WIN_DESTROY */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_WIN_DESTROY_INIT(&evt->win_destroy);
            evt->win_destroy.wid = win;
            ret = rtgui_server_post_event_sync(evt);
            if (ret) {
                rt_mp_free(evt);
                LOG_E("destroy %s err [%d]", win->title, ret);
                return;
            }
        } else {
            LOG_E("get mp err");
            return;
        }
    }

    /* release field */
    if (win->_title_wgt) {
        rtgui_widget_destroy(RTGUI_WIDGET(win->_title_wgt));
        win->_title_wgt = RT_NULL;
    }
    if (RT_NULL != win->title) {
        rt_free(win->title);
        win->title = RT_NULL;
    }
    rtgui_region_fini(&win->outer_clip);
    /* release external clip info */
    win->drawing = 0;
}

static rt_bool_t _rtgui_win_create_in_server(struct rtgui_win *win) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;

    if (win->flag & RTGUI_WIN_FLAG_CONNECTED) return RT_TRUE;

    /* send RTGUI_EVENT_WIN_CREATE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_WIN_CREATE_INIT(&evt->win_create);
        evt->win_create.parent_window = win->parent_window;
        evt->win_create.wid = win;
        evt->win_create.parent.user = win->style;
        ret = rtgui_server_post_event_sync(evt);
        if (ret) {
            rt_mp_free(evt);
            LOG_E("create %s err [%d]", win->title, ret);
            return RT_FALSE;
        }
    } else {
        LOG_E("get mp err");
        return RT_FALSE;
    }

    win->flag |= RTGUI_WIN_FLAG_CONNECTED;
    return RT_TRUE;
}

rt_err_t rtgui_win_init(struct rtgui_win *win, struct rtgui_win *parent_window,
    const char *title, rtgui_rect_t *rect, rt_uint16_t style) {
    rt_err_t ret;

    if (!win) return -RT_ERROR;

    do {
    /* set parent window */
        win->parent_window = parent_window;
        /* set title, rect and style */
        if (title) {
            win->title = rt_strdup(title);
        } else {
            win->title = RT_NULL;
        }

        rtgui_widget_set_rect(RTGUI_WIDGET(win), rect);
        win->style = style;

        if (!((style & RTGUI_WIN_STYLE_NO_TITLE) && \
              (style & RTGUI_WIN_STYLE_NO_BORDER))) {
            struct rtgui_rect trect = *rect;

            win->_title_wgt = rtgui_win_title_create(win);
            if (!win->_title_wgt) {
                LOG_E("create title %s failed", title);
                ret = -RT_ERROR;
                break;
            }

            if (!(style & RTGUI_WIN_STYLE_NO_BORDER)) {
                rtgui_rect_inflate(&trect, WINTITLE_BORDER_SIZE);
            }
            if (!(style & RTGUI_WIN_STYLE_NO_TITLE)) {
                trect.y1 -= WINTITLE_HEIGHT;
            }
            rtgui_widget_set_rect(RTGUI_WIDGET(win->_title_wgt), &trect);
            /* Update the clip of the wintitle manually. */
            rtgui_region_subtract_rect(
                &(RTGUI_WIDGET(win->_title_wgt)->clip),
                &(RTGUI_WIDGET(win->_title_wgt)->clip),
                &(RTGUI_WIDGET(win)->extent));

            /* The window title is always un-hidden for simplicity. */
            rtgui_widget_show(RTGUI_WIDGET(win->_title_wgt));
            rtgui_region_init_with_extents(&win->outer_clip, &trect);
            win->outer_extent = trect;
        } else {
            rtgui_region_init_with_extents(&win->outer_clip, rect);
            win->outer_extent = *rect;
        }

        if (!_rtgui_win_create_in_server(win)) {
            ret = -RT_ERROR;
            break;
        }

        win->app->window_cnt++;
        ret = RT_EOK;
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_win_init);

rt_err_t rtgui_win_fini(struct rtgui_win* win) {
    rt_err_t ret;

    win->magic = 0;
    ret = RT_EOK;
    do {
        /* close the window first if it's not. */
        if (!(win->flag & RTGUI_WIN_FLAG_CLOSED)) {
            rtgui_evt_generic_t *evt;

            /* send RTGUI_EVENT_WIN_CLOSE */
            evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
            if (evt) {
                RTGUI_EVENT_WIN_CLOSE_INIT(&evt->win_close);
                evt->win_close.wid = win;
                (void)_rtgui_win_deal_close(win, evt, RT_TRUE);
            } else {
                LOG_E("get mp err");
                ret = -RT_ENOMEM;
                break;
            }

            if (win->style & RTGUI_WIN_STYLE_DESTROY_ON_CLOSE) break;
        }

        if (win->flag & RTGUI_WIN_FLAG_MODAL) {
            /* set the RTGUI_WIN_STYLE_DESTROY_ON_CLOSE flag so the window will
               be destroyed after the event_loop */
            win->style |= RTGUI_WIN_STYLE_DESTROY_ON_CLOSE;
            rtgui_win_end_modal(win, RTGUI_MODAL_CANCEL);
        }
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_win_fini);

rtgui_win_t *rtgui_win_create(struct rtgui_win *parent_win, const char *title,
    rtgui_rect_t *rect, rt_uint16_t style) {
    struct rtgui_win *win;

    /* allocate win memory */
    win = RTGUI_WIN(rtgui_widget_create(RTGUI_WIN_TYPE));
    if (RT_NULL == win) {
        LOG_E("create %s failed", title);
        return RT_NULL;
    }

    if (RT_EOK != rtgui_win_init(win, parent_win, title, rect, style)) {
        LOG_E("init %s failed", title);
        rtgui_widget_destroy(RTGUI_WIDGET(win));
        return RT_NULL;
    }

    return win;
}
RTM_EXPORT(rtgui_win_create);

rtgui_win_t *rtgui_mainwin_create(struct rtgui_win *parent_win,
    const char *title, rt_uint16_t style) {
    struct rtgui_rect rect;

    /* get rect of main window */
    rtgui_get_mainwin_rect(&rect);
    return rtgui_win_create(parent_win, title, &rect, style);
}
RTM_EXPORT(rtgui_mainwin_create);

static rt_bool_t _rtgui_win_deal_close(struct rtgui_win *win,
    rtgui_evt_generic_t *evt, rt_bool_t force) {
    if (win->on_close) {
        if (!(win->on_close(RTGUI_OBJECT(win), evt)) && !force) {
            return RT_FALSE;
        }
    }

    rtgui_win_hide(win);
    win->flag |= RTGUI_WIN_FLAG_CLOSED;

    if (win->flag & RTGUI_WIN_FLAG_MODAL) {
        /* rtgui_win_end_modal cleared the RTGUI_WIN_FLAG_MODAL in win->flag so
         * we have to record it. */
        rtgui_win_end_modal(win, RTGUI_MODAL_CANCEL);
    }

    win->app->window_cnt--;
    if (!win->app->window_cnt && \
        !(win->app->state_flag & RTGUI_APP_FLAG_KEEP)) {
        rtgui_app_exit(rtgui_app_self(), 0);
    }

    if (win->style & RTGUI_WIN_STYLE_DESTROY_ON_CLOSE) {
        rtgui_win_destroy(win);
    }

    return RT_TRUE;
}

void rtgui_win_destroy(struct rtgui_win *win) {
    /* close the window first if it's not. */
    if (!(win->flag & RTGUI_WIN_FLAG_CLOSED)) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_WIN_CLOSE */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_WIN_CLOSE_INIT(&evt->win_close);
            evt->win_close.wid = win;
            (void)_rtgui_win_deal_close(win, evt, RT_TRUE);
            if (win->style & RTGUI_WIN_STYLE_DESTROY_ON_CLOSE) return;
        } else {
            LOG_E("get mp err");
            return;
        }
    }

    if (win->flag & RTGUI_WIN_FLAG_MODAL) {
        /* set the RTGUI_WIN_STYLE_DESTROY_ON_CLOSE flag so the window will be
         * destroyed after the event_loop */
        win->style |= RTGUI_WIN_STYLE_DESTROY_ON_CLOSE;
        rtgui_win_end_modal(win, RTGUI_MODAL_CANCEL);
    } else {
        rtgui_widget_destroy(RTGUI_WIDGET(win));
    }
}
RTM_EXPORT(rtgui_win_destroy);

/* send a close event to myself to get a consistent behavior */
rt_bool_t rtgui_win_close(struct rtgui_win *win) {
    rtgui_evt_generic_t *evt;

    /* send RTGUI_EVENT_WIN_CLOSE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_WIN_CLOSE_INIT(&evt->win_close);
        evt->win_close.wid = win;
        return _rtgui_win_deal_close(win, evt, RT_TRUE);
    } else {
        LOG_E("get mp err");
        return RT_FALSE;
    }
}
RTM_EXPORT(rtgui_win_close);

rt_err_t rtgui_win_enter_modal(struct rtgui_win *win) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;
    rt_base_t exit_code;

    /* send RTGUI_EVENT_WIN_MODAL_ENTER */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_WIN_MODAL_ENTER_INIT(&evt->win_modal_enter);
        evt->win_modal_enter.wid = win;
        ret = rtgui_server_post_event_sync(evt);
        if (ret) {
            rt_mp_free(evt);
            LOG_E("enter modal %s err [%d]", win->title, ret);
            return ret;
        }
    } else {
        LOG_E("get mp err");
        return -RT_ERROR;
    }

    LOG_D("enter modal %s", win->title);
    win->flag |= RTGUI_WIN_FLAG_MODAL;
    win->app_ref_count = win->app->ref_cnt + 1;
    exit_code = rtgui_app_run(win->app);
    LOG_E("modal %s ret %d", win->title, exit_code);

    win->flag &= ~RTGUI_WIN_FLAG_MODAL;
    rtgui_win_hide(win);

    return ret;
}
RTM_EXPORT(rtgui_win_enter_modal);

rt_err_t rtgui_win_do_show(struct rtgui_win *win) {
    rt_err_t ret;

    if (!win) return -RT_ERROR;
    win->flag &= ~RTGUI_WIN_FLAG_CLOSED;
    win->flag &= ~RTGUI_WIN_FLAG_CB_PRESSED;

    do {
        rtgui_evt_generic_t *evt;

        /* if it does not register into server, create it in server */
        if (!(win->flag & RTGUI_WIN_FLAG_CONNECTED)) {
            if (!_rtgui_win_create_in_server(win)) {
                LOG_E("create %s err", win->title);
                ret = -RT_ERROR;
                break;
            }
        }
        /* set window unhidden before notify the server */
        rtgui_widget_show(RTGUI_WIDGET(win));

        /* send RTGUI_EVENT_WIN_SHOW */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (!evt) {
            LOG_E("get mp err");
            ret = -RT_ERROR;
            break;
        }

        RTGUI_EVENT_WIN_SHOW_INIT(&evt->win_show);
        evt->win_show.wid = win;
        ret = rtgui_server_post_event_sync(evt);
        if (ret) {
            rt_mp_free(evt);
            /* It could not be shown if a parent window is hidden. */
            rtgui_widget_hide(RTGUI_WIDGET(win));
            LOG_E("show %s err [%d]", win->title, ret);
            break;
        }

        if (!win->focused_widget) {
            rtgui_widget_focus(RTGUI_WIDGET(win));
        }
        /* set main window */
        if (!win->app->main_object) {
            rtgui_app_set_main_win(win->app, win);
        }

        if (win->flag & RTGUI_WIN_FLAG_MODAL) {
            ret = rtgui_win_enter_modal(win);
        }
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_win_do_show);

rt_err_t rtgui_win_show(struct rtgui_win *win, rt_bool_t is_modal) {
    RTGUI_WIDGET_UNHIDE(win);
    win->magic = RTGUI_WIN_MAGIC;
    if (is_modal) win->flag |= RTGUI_WIN_FLAG_MODAL;

    if (win->_do_show) return win->_do_show(win);
    return rtgui_win_do_show(win);
}
RTM_EXPORT(rtgui_win_show);

void rtgui_win_end_modal(struct rtgui_win *win, rtgui_modal_code_t modal_code) {
    rt_uint32_t cnt = 0;
    if (win == RT_NULL || !(win->flag & RTGUI_WIN_FLAG_MODAL))
        return;

    while (win->app_ref_count < win->app->ref_cnt) {
        rtgui_app_exit(win->app, 0);
        cnt++;
        if (cnt >= 1000) {
            LOG_E("*** called rtgui_win_end_modal -> rtgui_app_exit %d times",
                cnt);
            RT_ASSERT(cnt < 1000);
        }
    }

    rtgui_app_exit(win->app, modal_code);

    /* remove modal mode */
    win->flag &= ~RTGUI_WIN_FLAG_MODAL;
}
RTM_EXPORT(rtgui_win_end_modal);

void rtgui_win_hide(struct rtgui_win *win) {
    rtgui_evt_generic_t *evt;

    RT_ASSERT(win != RT_NULL);
    if (RTGUI_WIDGET_IS_HIDE(win) || !(win->flag & RTGUI_WIN_FLAG_CONNECTED)) {
        return;
    }

    /* send RTGUI_EVENT_WIN_HIDE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        rt_err_t ret;

        RTGUI_EVENT_WIN_HIDE_INIT(&evt->win_hide);
        evt->win_hide.wid = win;
        ret = rtgui_server_post_event_sync(evt);
        if (ret) {
            rt_mp_free(evt);
            LOG_E("hide %s err [%d]", win->title, ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }

    rtgui_widget_hide(RTGUI_WIDGET(win));
    win->flag &= ~RTGUI_WIN_FLAG_ACTIVATE;
}
RTM_EXPORT(rtgui_win_hide);

rt_err_t rtgui_win_activate(struct rtgui_win *win) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;

    /* send RTGUI_EVENT_WIN_ACTIVATE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_WIN_ACTIVATE_INIT(&evt->win_activate);
        evt->win_activate.wid = win;
        ret = rtgui_server_post_event_sync(evt);
        if (ret) {
            rt_mp_free(evt);
            LOG_E("create %s err [%d]", win->title, ret);
        }
    } else {
        LOG_E("get mp err");
        ret = -RT_ERROR;
    }

    return ret;
}
RTM_EXPORT(rtgui_win_activate);

rt_bool_t rtgui_win_is_activated(struct rtgui_win *win) {
    RT_ASSERT(win != RT_NULL);

    if (win->flag & RTGUI_WIN_FLAG_ACTIVATE) return RT_TRUE;
    return RT_FALSE;
}
RTM_EXPORT(rtgui_win_is_activated);

void rtgui_win_move(struct rtgui_win *win, int x, int y) {
    struct rtgui_widget *wgt;
    int dx, dy;

    if (RT_NULL == win) return;

    if (win->_title_wgt) {
        wgt = RTGUI_WIDGET(win->_title_wgt);
        dx = x - wgt->extent.x1;
        dy = y - wgt->extent.y1;
        rtgui_widget_move_to_logic(wgt, dx, dy);

        wgt = RTGUI_WIDGET(win);
        rtgui_widget_move_to_logic(wgt, dx, dy);
    } else {
        wgt = RTGUI_WIDGET(win);
        dx = x - wgt->extent.x1;
        dy = y - wgt->extent.y1;
        rtgui_widget_move_to_logic(wgt, dx, dy);
    }
    rtgui_rect_move(&win->outer_extent, dx, dy);

    if (win->flag & RTGUI_WIN_FLAG_CONNECTED) {
        rtgui_evt_generic_t *evt;
        rt_err_t ret;

        rtgui_widget_hide(RTGUI_WIDGET(win));

        /* send RTGUI_EVENT_WIN_MOVE */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_WIN_MOVE_INIT(&evt->win_move);
            evt->win_move.wid = win;
            evt->win_move.x = x;
            evt->win_move.y = y;
            ret = rtgui_server_post_event_sync(evt);
            if (ret) {
                rt_mp_free(evt);
                LOG_E("move %s err [%d]", win->title, ret);
                return;
            }
        } else {
            LOG_E("get mp err");
            return;
        }
    }
    rtgui_widget_show(RTGUI_WIDGET(win));
}
RTM_EXPORT(rtgui_win_move);

static rt_bool_t rtgui_win_ondraw(struct rtgui_win *win) {
    struct rtgui_dc *dc;
    struct rtgui_rect rect;
    union rtgui_evt_generic *evt;

    evt = rtgui_malloc(sizeof(struct rtgui_event_paint));
    if (!evt) return RT_FALSE;

    /* begin drawing */
    dc = rtgui_dc_begin_drawing(RTGUI_WIDGET(win));
    if (RT_NULL == dc) return RT_FALSE;

    /* get window rect */
    rtgui_widget_get_rect(RTGUI_WIDGET(win), &rect);
    /* fill area */
    rtgui_dc_fill_rect(dc, &rect);

    /* widget drawing */

    /* paint each widget */
    RTGUI_EVENT_PAINT_INIT(&evt->paint);
    evt->paint.wid = RT_NULL;

    rtgui_container_dispatch_event(RTGUI_CONTAINER(win), evt);

    rtgui_dc_end_drawing(dc, 1);

    rtgui_free(evt);

    return RT_FALSE;
}

void rtgui_win_update_clip(struct rtgui_win *win) {
    struct rtgui_container *cnt;
    struct rtgui_list_node *node;

    if (RT_NULL == win) return;
    if (win->flag & RTGUI_WIN_FLAG_CLOSED) return;

    if (win->_title_wgt) {
        /* Reset the inner clip of title. */
        RTGUI_WIDGET(win->_title_wgt)->extent = win->outer_extent;
        rtgui_region_copy(
            &RTGUI_WIDGET(win->_title_wgt)->clip,
            &win->outer_clip);
        rtgui_region_subtract_rect(
            &RTGUI_WIDGET(win->_title_wgt)->clip,
            &RTGUI_WIDGET(win->_title_wgt)->clip,
            &RTGUI_WIDGET(win)->extent);
        /* Reset the inner clip of window. */
        rtgui_region_intersect_rect(
            &RTGUI_WIDGET(win)->clip,
            &win->outer_clip,
            &RTGUI_WIDGET(win)->extent);
    } else {
        RTGUI_WIDGET(win)->extent = win->outer_extent;
        rtgui_region_copy(&RTGUI_WIDGET(win)->clip, &win->outer_clip);
    }

    /* update the clip info of each child */
    cnt = RTGUI_CONTAINER(win);
    rtgui_list_foreach(node, &(cnt->children)) {
        rtgui_widget_t *child = rtgui_list_entry(node, rtgui_widget_t, sibling);

        rtgui_widget_update_clip(child);
    }
}
RTM_EXPORT(rtgui_win_update_clip);

static rt_bool_t _win_handle_mouse_btn(struct rtgui_win *win,
    union rtgui_evt_generic *evt) {
    /* check whether has widget which handled mouse event before.
     *
     * Note #1: that the widget should have already received mouse down
     * event and we should only feed the mouse up event to it here.
     *
     * Note #2: the widget is responsible to clean up
     * last_mevent_widget on mouse up event(but not overwrite other
     * widgets). If not, it will receive two mouse up events.
     */
    if (win->last_mevent_widget && \
        (evt->mouse.button & RTGUI_MOUSE_BUTTON_UP)) {
        if (RTGUI_OBJECT(win->last_mevent_widget)->event_handler(
                RTGUI_OBJECT(win->last_mevent_widget), evt)) {
            /* clean last mouse event handled widget */
            win->last_mevent_widget = RT_NULL;
            return RT_TRUE;
        }
    }

    /** if a widget will destroy the window in the event_handler(or in
     * on_* callbacks), it should return RT_TRUE. Otherwise, it will
     * crash the application.
     *
     * TODO: add it in the doc
     */
    return rtgui_container_dispatch_mouse_event(RTGUI_CONTAINER(win), evt);
}

rt_bool_t rtgui_win_event_handler(struct rtgui_obj *obj,
    union rtgui_evt_generic *evt) {
    struct rtgui_win *win;

    RT_ASSERT(obj != RT_NULL);
    RT_ASSERT(evt != RT_NULL);

    win = RTGUI_WIN(obj);

    LOG_D("win rx %d from %s", evt->base.type, evt->base.sender->mb->parent.parent.name);
    switch (evt->base.type) {
    case RTGUI_EVENT_WIN_SHOW:
        rtgui_win_do_show(win);
        break;

    case RTGUI_EVENT_WIN_HIDE:
        rtgui_win_hide(win);
        break;

    case RTGUI_EVENT_WIN_CLOSE:
        _rtgui_win_deal_close(win, evt, RT_FALSE);
        /* don't broadcast WIN_CLOSE event to others */
        return RT_TRUE;

    case RTGUI_EVENT_WIN_MOVE:
        /* move window */
        rtgui_win_move(win, evt->win_move.x, evt->win_move.y);
        break;

    case RTGUI_EVENT_WIN_ACTIVATE:
        if ((win->flag & RTGUI_WIN_FLAG_UNDER_MODAL) || \
            RTGUI_WIDGET_IS_HIDE(win)) {
            /* activate a hide window */
            return RT_TRUE;
        }

        win->flag |= RTGUI_WIN_FLAG_ACTIVATE;
        /* There are many cases where the paint event will follow this activate
         * event and just repaint the title is not a big deal. So just repaint
         * the title if there is one. If you want to update the content of the
         * window, do it in the on_activate callback.*/
        if (win->_title_wgt) {
            rtgui_widget_update(RTGUI_WIDGET(win->_title_wgt));
        }
        if (win->on_activate != RT_NULL) {
            win->on_activate(RTGUI_OBJECT(obj), evt);
        }
        break;

    case RTGUI_EVENT_WIN_DEACTIVATE:
        win->flag &= ~RTGUI_WIN_FLAG_ACTIVATE;
        /* No paint event follow the deactive event. So we have to update
         * the title manually to reflect the change. */
        if (win->_title_wgt) {
            rtgui_widget_update(RTGUI_WIDGET(win->_title_wgt));
        }
        if (win->on_deactivate != RT_NULL) {
            win->on_deactivate(RTGUI_OBJECT(obj), evt);
        }
        break;

    case RTGUI_EVENT_WIN_UPDATE_END:
        break;

    case RTGUI_EVENT_CLIP_INFO:
        /* update win clip */
        rtgui_win_update_clip(win);
        break;

    case RTGUI_EVENT_PAINT:
        if (win->_title_wgt) {
            rtgui_widget_update(RTGUI_WIDGET(win->_title_wgt));
        }
        rtgui_win_ondraw(win);
        break;

    #ifdef GUIENGIN_USING_VFRAMEBUFFER
        case RTGUI_EVENT_VPAINT_REQ:
        {
            struct rtgui_event_vpaint_req *req = \
                (struct rtgui_event_vpaint_req *)evt;
            struct rtgui_dc *dc;

            /* get drawing dc */
            dc = rtgui_win_get_drawing(win);

            req->sender->buffer = dc;
            rt_completion_done(req->sender->cmp);

            break;
        }
    #endif

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (!rtgui_rect_contains_point(&RTGUI_WIDGET(win)->extent,
                evt->mouse.x, evt->mouse.y)) {
            return _win_handle_mouse_btn(win, evt);
        }
        if (win->_title_wgt) {
            struct rtgui_obj *tobj = RTGUI_OBJECT(win->_title_wgt);
            return tobj->event_handler(tobj, evt);
        }
        break;

    case RTGUI_EVENT_MOUSE_MOTION:
        return rtgui_container_dispatch_mouse_event(RTGUI_CONTAINER(win), evt);

    case RTGUI_EVENT_KBD:
        /* we should dispatch key event firstly */
        if (!(win->flag & RTGUI_WIN_FLAG_HANDLE_KEY))
        {
            struct rtgui_widget *widget;
            rt_bool_t res = RT_FALSE;
            /* we should dispatch the key event just once. Once entered the
             * dispatch mode, we should swtich to key handling mode. */
            win->flag |= RTGUI_WIN_FLAG_HANDLE_KEY;

            /* dispatch the key event */
            for (widget = win->focused_widget;
                    widget && !res;
                    widget = widget->parent)
            {
                if (RTGUI_OBJECT(widget)->event_handler != RT_NULL)
                    res = RTGUI_OBJECT(widget)->event_handler(
                              RTGUI_OBJECT(widget), evt);
            }

            win->flag &= ~RTGUI_WIN_FLAG_HANDLE_KEY;
            return res;
        }
        else
        {
            /* in key handling mode(it may reach here in
             * win->focused_widget->event_handler call) */
            if (win->on_key != RT_NULL)
                return win->on_key(RTGUI_OBJECT(win), evt);
        }
        break;

    case RTGUI_EVENT_COMMAND:
        if (rtgui_container_dispatch_event(RTGUI_CONTAINER(obj), evt) != RT_TRUE)
        {
        }
        else return RT_TRUE;
        break;

    default:
        return rtgui_container_event_handler(obj, evt);
    }

    return RT_FALSE;
}
RTM_EXPORT(rtgui_win_event_handler);

void rtgui_win_set_rect(rtgui_win_t *win, rtgui_rect_t *rect) {
    if (!win || !rect) return;
    RTGUI_WIDGET(win)->extent = *rect;

    if (win->flag & RTGUI_WIN_FLAG_CONNECTED) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_WIN_RESIZE */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            rt_err_t ret;

            RTGUI_EVENT_WIN_RESIZE_INIT(&evt->win_resize);
            evt->win_resize.wid = win;
            evt->win_resize.rect = *rect;
            ret= rtgui_server_post_event(evt);
            if (ret) {
                rt_mp_free(evt);
                LOG_E("resize %s err [%d]", win->title, ret);
            }
        }
    }
}
RTM_EXPORT(rtgui_win_set_rect);

void rtgui_win_set_onactivate(rtgui_win_t *win, rtgui_evt_hdl_p handler)
{
    if (win != RT_NULL)
    {
        win->on_activate = handler;
    }
}
RTM_EXPORT(rtgui_win_set_onactivate);

void rtgui_win_set_ondeactivate(rtgui_win_t *win,
    rtgui_evt_hdl_p handler) {
    if (win) win->on_deactivate = handler;
}
RTM_EXPORT(rtgui_win_set_ondeactivate);

void rtgui_win_set_onclose(rtgui_win_t *win, rtgui_evt_hdl_p handler) {
    if (win) win->on_close = handler;
}
RTM_EXPORT(rtgui_win_set_onclose);

void rtgui_win_set_onkey(rtgui_win_t *win, rtgui_evt_hdl_p handler) {
    if (win) win->on_key = handler;
}
RTM_EXPORT(rtgui_win_set_onkey);

void rtgui_win_set_title(rtgui_win_t *win, const char *title)
{
    /* send title to server */
    if (win->flag & RTGUI_WIN_FLAG_CONNECTED)
    {
    }

    /* modify in local side */
    if (win->title != RT_NULL)
    {
        rtgui_free(win->title);
        win->title = RT_NULL;
    }

    if (title != RT_NULL)
    {
        win->title = rt_strdup(title);
    }
}
RTM_EXPORT(rtgui_win_set_title);

char *rtgui_win_get_title(rtgui_win_t *win)
{
    RT_ASSERT(win != RT_NULL);

    return win->title;
}
RTM_EXPORT(rtgui_win_get_title);

#ifdef GUIENGIN_USING_VFRAMEBUFFER
# include "../include/driver.h"

struct rtgui_dc *rtgui_win_get_drawing(rtgui_win_t * win) {
    struct rtgui_dc *dc;
    struct rtgui_rect rect;

    if (rtgui_app_self() == RT_NULL)
        return RT_NULL;
    if (win == RT_NULL || !(win->flag & RTGUI_WIN_FLAG_CONNECTED))
        return RT_NULL;

    if (win->app == rtgui_app_self()) {
        /* under the same application context */
        rtgui_region_t region;
        rtgui_region_t clip_region;

        rtgui_region_init(&clip_region);
        rtgui_region_copy(&clip_region, &win->outer_clip);

        rtgui_graphic_driver_vmode_enter();

        rtgui_graphic_driver_get_rect(RT_NULL, &rect);
        region.data = RT_NULL;
        region.extents.x1 = rect.x1;
        region.extents.y1 = rect.y1;
        region.extents.x2 = rect.x2;
        region.extents.y2 = rect.y2;

        /* remove clip */
        rtgui_region_reset(&win->outer_clip,
                           &RTGUI_WIDGET(win)->extent);
        rtgui_region_intersect(&win->outer_clip,
                               &win->outer_clip,
                               &region);
        rtgui_win_update_clip(win);
        /* use virtual framebuffer */
        rtgui_widget_update(RTGUI_WIDGET(win));

        /* get the extent of widget */
        rtgui_widget_get_extent(RTGUI_WIDGET(win), &rect);

        dc = rtgui_graphic_driver_get_rect_buffer(RT_NULL, &rect);

        rtgui_graphic_driver_vmode_exit();

        /* restore the clip information of window */
        rtgui_region_reset(&RTGUI_WIDGET(win)->clip,
                           &RTGUI_WIDGET(win)->extent);
        rtgui_region_intersect(&(RTGUI_WIDGET(win)->clip),
                               &(RTGUI_WIDGET(win)->clip),
                               &clip_region);
        rtgui_region_fini(&region);
        rtgui_region_fini(&clip_region);

        rtgui_win_update_clip(win);
    } else {
        /* send vpaint_req to the window and wait for response */
        rtgui_evt_generic_t *evt;
        struct rt_completion cmp;
        int freeze;

        /* make sure the screen is not locked. */
        freeze = rtgui_screen_lock_freeze();

        /* send RTGUI_EVENT_VPAINT_REQ */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
        if (evt) {
            RTGUI_EVENT_VPAINT_REQ_INIT(&evt->vpaint_req, win, &cmp);
            ret = rtgui_send(win->app, evt, RT_WAITING_FOREVER);
            if (ret) {
                LOG_E("vpaint req %s err [%d]", win->wid->title, ret);
                return RT_NULL;
            }
        } else {
            LOG_E("get mp err");
            return RT_NULL;
        }

        rt_completion_wait(evt->vpaint_req.cmp, RT_WAITING_FOREVER);
        /* wait for vpaint_ack event */
        dc = evt->vpaint_req.buffer;
        rtgui_screen_lock_thaw(freeze);
    }

    return dc;
}
RTM_EXPORT(rtgui_win_get_drawing);
#endif

static const rt_uint8_t close_byte[14] =
{
    0x06, 0x18, 0x03, 0x30, 0x01, 0xE0, 0x00,
    0xC0, 0x01, 0xE0, 0x03, 0x30, 0x06, 0x18
};

/* window drawing */
void rtgui_theme_draw_win(struct rtgui_win_title *wint)
{
    struct rtgui_dc *dc;
    struct rtgui_win *win;
    rtgui_rect_t rect;

    if (!wint)
        return;

    win = RTGUI_WIDGET(wint)->toplevel;
    RT_ASSERT(win);

    if (win->_title_wgt == RT_NULL)
        return;

    /* begin drawing */
    dc = rtgui_dc_begin_drawing(RTGUI_WIDGET(win->_title_wgt));
    if (dc == RT_NULL)
        return;

    /* get rect */
    rtgui_widget_get_rect(RTGUI_WIDGET(win->_title_wgt), &rect);

    /* draw border */
    if (!(win->style & RTGUI_WIN_STYLE_NO_BORDER))
    {
        rect.x2 -= 1;
        rect.y2 -= 1;
        RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = RTGUI_RGB(212, 208, 200);
        rtgui_dc_draw_hline(dc, rect.x1, rect.x2, rect.y1);
        rtgui_dc_draw_vline(dc, rect.x1, rect.y1, rect.y2);

        RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = white;
        rtgui_dc_draw_hline(dc, rect.x1 + 1, rect.x2 - 1, rect.y1 + 1);
        rtgui_dc_draw_vline(dc, rect.x1 + 1, rect.y1 + 1, rect.y2 - 1);

        RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = RTGUI_RGB(128, 128, 128);
        rtgui_dc_draw_hline(dc, rect.x1 + 1, rect.x2 - 1, rect.y2 - 1);
        rtgui_dc_draw_vline(dc, rect.x2 - 1, rect.y1 + 1, rect.y2);

        RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = RTGUI_RGB(64, 64, 64);
        rtgui_dc_draw_hline(dc, rect.x1, rect.x2, rect.y2);
        rtgui_dc_draw_vline(dc, rect.x2, rect.y1, rect.y2 + 1);

        /* shrink border */
        rtgui_rect_inflate(&rect, -WINTITLE_BORDER_SIZE);
    }

    /* draw title */
    if (!(win->style & RTGUI_WIN_STYLE_NO_TITLE))
    {
        rt_uint16_t index;
        rt_uint16_t r, g, b, delta;

#define RGB_FACTOR  4
        if (win->flag & RTGUI_WIN_FLAG_ACTIVATE)
        {
            r = 10 << RGB_FACTOR;
            g = 36 << RGB_FACTOR;
            b = 106 << RGB_FACTOR;
            delta = (150 << RGB_FACTOR) / (rect.x2 - rect.x1);
        }
        else
        {
            r = 128 << RGB_FACTOR;
            g = 128 << RGB_FACTOR;
            b = 128 << RGB_FACTOR;
            delta = (64 << RGB_FACTOR) / (rect.x2 - rect.x1);
        }

        for (index = rect.x1; index < rect.x2 + 1; index ++)
        {
            RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = RTGUI_RGB((r>>RGB_FACTOR),
                    (g>>RGB_FACTOR),
                    (b>>RGB_FACTOR));
            rtgui_dc_draw_vline(dc, index, rect.y1, rect.y2);
            r += delta;
            g += delta;
            b += delta;
        }
#undef RGB_FACTOR

        if (win->flag & RTGUI_WIN_FLAG_ACTIVATE)
        {
            RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = white;
        }
        else
        {
            RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = RTGUI_RGB(212, 208, 200);
        }

        rect.x1 += 4;
        rect.y1 += 2;
        rect.y2 = rect.y1 + WINTITLE_CB_HEIGHT;
        rtgui_dc_draw_text(dc, win->title, &rect);

        if (win->style & RTGUI_WIN_STYLE_CLOSEBOX)
        {
            /* get close button rect */
            rtgui_rect_t box_rect = {0, 0, WINTITLE_CB_WIDTH, WINTITLE_CB_HEIGHT};
            rtgui_rect_move_to_align(&rect, &box_rect, RTGUI_ALIGN_CENTER_VERTICAL | RTGUI_ALIGN_RIGHT);
            box_rect.x1 -= 3;
            box_rect.x2 -= 3;
            rtgui_dc_fill_rect(dc, &box_rect);

            /* draw close box */
            if (win->flag & RTGUI_WIN_FLAG_CB_PRESSED)
            {
                rtgui_dc_draw_border(dc, &box_rect, RTGUI_BORDER_SUNKEN);
                RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = red;
                rtgui_dc_draw_word(dc, box_rect.x1, box_rect.y1 + 6, 7, close_byte);
            }
            else
            {
                rtgui_dc_draw_border(dc, &box_rect, RTGUI_BORDER_RAISE);
                RTGUI_WIDGET_FOREGROUND(win->_title_wgt) = black;
                rtgui_dc_draw_word(dc, box_rect.x1 - 1, box_rect.y1 + 5, 7, close_byte);
            }
        }
    }

    rtgui_dc_end_drawing(dc, 1);
}
