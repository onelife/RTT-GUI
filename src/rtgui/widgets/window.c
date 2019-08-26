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
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/widgets/container.h"
#include "include/widgets/window.h"
#include "include/app/app.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_WIN"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void _win_constructor(void *obj);
static void _win_destructor(void *obj);
static rt_bool_t _win_event_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_err_t _win_do_show(rtgui_win_t *win);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    win,
    CLASS_METADATA(container),
    _win_constructor,
    _win_destructor,
    _win_event_handler,
    sizeof(rtgui_win_t));

/* Private functions ---------------------------------------------------------*/
static void _win_constructor(void *obj) {
    rtgui_win_t *win = obj;

    /* set super fields */
    TO_WIDGET(obj)->toplevel = win;

    /* init win */
    win->parent = RT_NULL;
    win->app = rtgui_app_self();
    win->style = RTGUI_WIN_STYLE_DEFAULT;
    win->flag = RTGUI_WIN_FLAG_INIT;
    // win->modal = RTGUI_MODAL_OK;
    win->update = 0;
    win->drawing = 0;
    // drawing_rect, outer_extent, outer_clip
    win->title = RT_NULL;
    win->_title = RT_NULL;
    win->focused = RT_NULL;
    win->last_mouse = RT_NULL;
    win->on_activate = RT_NULL;
    win->on_deactivate = RT_NULL;
    win->on_close = RT_NULL;
    win->on_key = RT_NULL;
    win->user_data = RT_NULL;
    /* PRIVATE */
    win->_do_show = _win_do_show;
    // _ref_count, _magic

    /* hide win */
    WIDGET_FLAG_CLEAR(win, SHOWN);
}

static void _win_destructor(void *obj) {
    rtgui_win_t *win = obj;

    if (IS_WIN_FLAG(win, CONNECTED)) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_WIN_DESTROY */
        RTGUI_CREATE_EVENT(evt, WIN_DESTROY, RT_WAITING_FOREVER);
        if (!evt) return;
        evt->win_destroy.wid = win;
        if (rtgui_send_request_sync(evt)) return;
    }

    if (win->_title) {
        DELETE_INSTANCE(win->_title);
    }
    if (win->title) {
        rtgui_free(win->title);
        win->title = RT_NULL;
    }
    rtgui_region_uninit(&win->outer_clip);
    win->drawing = 0;
}

static rt_err_t _win_create_in_server(rtgui_win_t *win) {
    rt_err_t ret;

    ret = RT_EOK;

    do {
        rtgui_evt_generic_t *evt;

        if (IS_WIN_FLAG(win, CONNECTED)) break;

        /* send RTGUI_EVENT_WIN_CREATE */
        RTGUI_CREATE_EVENT(evt, WIN_CREATE, RT_WAITING_FOREVER);
        if (!evt) {
            ret = -RT_ENOMEM;
            break;
        }
        evt->win_create.parent_window = win->parent;
        evt->win_create.wid = win;
        ret = rtgui_send_request_sync(evt);
        if (ret) break;
        WIN_FLAG_SET(win, CONNECTED);
    } while (0);

    return ret;
}

static rt_err_t _win_do_show(rtgui_win_t *win) {
    rt_err_t ret;

    if (!win) return -RT_ERROR;

    WIN_FLAG_CLEAR(win, CLOSED);
    WIN_FLAG_CLEAR(win, CB_PRESSED);

    do {
        rtgui_evt_generic_t *evt;

        /* if it does not register into server, create it in server */
        if (!IS_WIN_FLAG(win, CONNECTED)) {
            ret = _win_create_in_server(win);
            if (ret) break;
        }
        /* set window unhidden before notify the server */
        rtgui_widget_show(TO_WIDGET(win));

        /* send RTGUI_EVENT_WIN_SHOW */
        RTGUI_CREATE_EVENT(evt, WIN_SHOW, RT_WAITING_FOREVER);
        if (!evt) {
            ret = -RT_ENOMEM;
            break;
        }
        evt->win_show.wid = win;
        ret = rtgui_send_request_sync(evt);
        if (ret) {
            /* It could not be shown if a _super window is hidden. */
            rtgui_widget_hide(TO_WIDGET(win));
            LOG_E("show %s err [%d]", win->title, ret);
            break;
        }

        if (!win->focused) {
            rtgui_widget_focus(TO_WIDGET(win));
        }
        /* set main window */
        if (!win->app->main_win)
            APP_SETTER(main_win)(win->app, win);

        if (IS_WIN_FLAG(win, MODAL)) {
            ret = rtgui_win_enter_modal(win);
        }
    } while (0);

    return ret;
}

static rt_bool_t _win_do_close(rtgui_win_t *win, rt_bool_t force) {
    rt_bool_t done = RT_TRUE;

    do {
        if (win->on_close) {
            done = win->on_close(TO_OBJECT(win), RT_NULL);
            if (!done && !force) break;
        }
        rtgui_win_hide(win);
        WIN_FLAG_SET(win, CLOSED);

        if (IS_WIN_STYLE(win, DESTROY_ON_CLOSE))
            /* will check if MODAL, then call rtgui_win_end_modal */
            DELETE_WIN_INSTANCE(win);

        if (IS_WIN_FLAG(win, MODAL))
            /* will clear RTGUI_WIN_FLAG_MODAL */
            rtgui_win_end_modal(win, RTGUI_MODAL_OK);
    } while (0);

    LOG_D("_win_do_close %d", done);
    return done;
}

static rt_bool_t _win_ondraw(rtgui_win_t *win, rtgui_evt_generic_t *evt) {
    if (SUPER_CLASS_HANDLER(win)) {
        LOG_I("ondraw, wid %p", evt->paint.wid);
        evt->paint.wid = RT_NULL;
        return SUPER_CLASS_HANDLER(win)(win, evt);
    }
    return RT_FALSE;
}

static rt_bool_t _win_handle_mouse_btn(rtgui_win_t *win,
    rtgui_evt_generic_t *evt) {
    /* check whether has widget which handled mouse event before.
     *
     * Note #1: that the widget should have already received mouse down
     * event and we should only feed the mouse up event to it here.
     *
     * Note #2: the widget is responsible to clean up
     * last_mouse on mouse up event(but not overwrite other
     * widgets). If not, it will receive two mouse up events.
     */
    if (win->last_mouse && IS_MOUSE_EVENT_BUTTON(evt, UP)) {
        if (TO_OBJECT(win->last_mouse)->evt_hdl(
                TO_OBJECT(win->last_mouse), evt)) {
            /* clean last mouse event handled widget */
            win->last_mouse = RT_NULL;
            return RT_TRUE;
        }
    }

    /** if a widget will destroy the window in the evt_hdl(or in
     * on_* callbacks), it should return RT_TRUE. Otherwise, it will
     * crash the app.
     *
     * TODO: add it in the doc
     */
    return rtgui_container_dispatch_mouse_event(TO_CONTAINER(win), evt);
}

static rt_bool_t _win_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_win_t *win = TO_WIN(obj);
    rt_bool_t done = RT_TRUE;

    EVT_LOG("[WinEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_WIN_SHOW:
        _win_do_show(win);
        break;

    case RTGUI_EVENT_WIN_HIDE:
        rtgui_win_hide(win);
        break;

    case RTGUI_EVENT_WIN_CLOSE:
        (void)_win_do_close(win, RT_FALSE);
        /* do not broadcast WIN_CLOSE event */
        break;

    case RTGUI_EVENT_WIN_MOVE:
        rtgui_win_move(win, evt->win_move.x, evt->win_move.y);
        break;

    case RTGUI_EVENT_WIN_ACTIVATE:
        if (IS_WIN_FLAG(win, IN_MODAL) || !IS_WIDGET_FLAG(win, SHOWN)) break;

        WIN_FLAG_SET(win, ACTIVATE);
        /* There are many cases where the paint event will follow this activate
         * event and just repaint the title is not a big deal. So just repaint
         * the title if there is one. If you want to update the content of the
         * window, do it in the on_activate callback.*/
        if (win->_title) rtgui_widget_update(TO_WIDGET(win->_title));
        if (win->on_activate) win->on_activate(TO_OBJECT(obj), evt);
        break;

    case RTGUI_EVENT_WIN_DEACTIVATE:
        WIN_FLAG_CLEAR(win, ACTIVATE);
        /* No paint event follow the deactive event. So we have to update
         * the title manually to reflect the change. */
        if (win->_title) rtgui_widget_update(TO_WIDGET(win->_title));
        if (win->on_deactivate) win->on_deactivate(TO_OBJECT(obj), evt);
        break;

    case RTGUI_EVENT_WIN_UPDATE_END:
        break;

    case RTGUI_EVENT_CLIP_INFO:
        /* update win clip */
        rtgui_win_update_clip(win);
        break;

    case RTGUI_EVENT_PAINT:
        if (win->_title) rtgui_widget_update(TO_WIDGET(win->_title));
        (void)_win_ondraw(win, evt);
        break;

    #ifdef GUIENGIN_USING_VFRAMEBUFFER
        case RTGUI_EVENT_VPAINT_REQ:
            evt->vpaint_req.origin->buffer = rtgui_win_get_drawing(win);
            rt_completion_done(evt->vpaint_req.origin->cmp);
            break;
    #endif

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (rtgui_rect_contains_point(
            &TO_WIDGET(win)->extent, evt->mouse.x, evt->mouse.y)) {
            done = _win_handle_mouse_btn(win, evt);
        } else if (win->_title) {
            rtgui_obj_t *tobj = TO_OBJECT(win->_title);
            done = EVENT_HANDLER(tobj)(tobj, evt);
        }
        break;

    case RTGUI_EVENT_MOUSE_MOTION:
        done = rtgui_container_dispatch_mouse_event(TO_CONTAINER(win), evt);
        break;

    case RTGUI_EVENT_KBD:
        /* we should dispatch key event firstly */
        if (!IS_WIN_FLAG(win, HANDLE_KEY)) {
            rtgui_widget_t *wgt = win->focused;

            /* we should dispatch the key event just once. Once entered the
             * dispatch mode, we should swtich to key handling mode. */
            WIN_FLAG_SET(win, HANDLE_KEY);
            /* dispatch the key event */
            while (wgt) {
                if (EVENT_HANDLER(wgt)) {
                    done = EVENT_HANDLER(wgt)(wgt, evt);
                    if (done) break;
                }
                wgt = wgt->parent;
            }
            WIN_FLAG_CLEAR(win, HANDLE_KEY);
        } else if (!win->on_key) {
            /* in key handling mode(it may reach here in
             * win->focused->evt_hdl call) */
            done = win->on_key(TO_OBJECT(win), evt);
        }
        break;

    case RTGUI_EVENT_COMMAND:
    default:
        if (SUPER_CLASS_HANDLER(win))
            done = SUPER_CLASS_HANDLER(win)(win, evt);
        break;
    }

    EVT_LOG("[WinEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static rt_err_t rtgui_win_init(rtgui_win_t *win, rtgui_win_t *parent,
    const char *title, rtgui_rect_t *rect, rt_uint16_t style) {
    rt_err_t ret = -RT_ERROR;

    do {
        rtgui_rect_t fixed = *rect;

        win->parent = parent;
        if (title) {
            win->title = rt_strdup(title);
        }
        win->style = style;

        if (!IS_WIN_STYLE(win, NO_BORDER))
            rtgui_rect_inflate(&fixed, -TITLE_DEFAULT_BORDER);
        if (!IS_WIN_STYLE(win, NO_TITLE))
            fixed.y1 += TITLE_DEFAULT_HEIGHT;

        rtgui_widget_set_rect(TO_WIDGET(win), &fixed);
        rtgui_region_init_with_extent(&win->outer_clip, rect);
        win->outer_extent = *rect;
        LOG_D("win rect (%d,%d)-(%d,%d)", fixed.x1, fixed.y1, fixed.x2,
            fixed.y2);

        if (!IS_WIN_STYLE(win, NO_TITLE)) {
            /* create title */
            win->_title = TO_TITLE(CREATE_INSTANCE(title, RT_NULL));
            if (!win->_title) {
                LOG_E("create title %s err", title);
                ret = -RT_ENOMEM;
                break;
            }
            TO_WIDGET(win->_title)->toplevel = win;
            rtgui_widget_set_rect(TO_WIDGET(win->_title), rect);
            LOG_D("title rect (%d,%d)-(%d,%d)", rect->x1, rect->y1, rect->x2,
                rect->y2);
            /* always show title */
            rtgui_widget_show(TO_WIDGET(win->_title));
        }

        ret = _win_create_in_server(win);
        if (ret) break;

        win->app->win_cnt++;
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("win init %s err %d", title, ret);
    }
    return ret;
}

/* Public functions ----------------------------------------------------------*/
RTGUI_MEMBER_SETTER(rtgui_win_t, win, rtgui_evt_hdl_t, on_activate);
RTGUI_MEMBER_SETTER(rtgui_win_t, win, rtgui_evt_hdl_t, on_deactivate);
RTGUI_MEMBER_SETTER(rtgui_win_t, win, rtgui_evt_hdl_t, on_close);
RTGUI_MEMBER_SETTER(rtgui_win_t, win, rtgui_evt_hdl_t, on_key);
RTGUI_MEMBER_GETTER(rtgui_win_t, win, char*, title);

rtgui_win_t *rtgui_create_win(rtgui_win_t *parent, rtgui_evt_hdl_t hdl,
    rtgui_rect_t *rect, const char *title, rt_uint16_t style) {
    rtgui_win_t *win;

    do {
        rtgui_rect_t rect_;

        win = CREATE_INSTANCE(win, hdl);
        if (!win) break;
        if (!rect) {
            rtgui_get_mainwin_rect(&rect_);
            rect = &rect_;
        }
        if (rtgui_win_init(win, parent, title, rect, style))
            DELETE_INSTANCE(win);
    } while (0);

    return win;
}

void rtgui_win_uninit(rtgui_win_t *win) {
    win->_magic = 0;
    if (!IS_WIN_FLAG(win, CLOSED)) {
        (void)_win_do_close(win, RT_TRUE);
        /* if DESTROY_ON_CLOSE, then the win is already deleted */
        if (IS_WIN_STYLE(win, DESTROY_ON_CLOSE)) return;
    }

    if (IS_WIN_FLAG(win, MODAL)) {
        LOG_W("uninit MODAL");
        /* set the DESTROY_ON_CLOSE flag so the win will be destroyed in
         * next event_loop? */
        WIN_STYLE_SET(win, DESTROY_ON_CLOSE);
        rtgui_win_end_modal(win, RTGUI_MODAL_CANCEL);
    } else {
        win->app->win_cnt--;
        LOG_D("win_cnt %d", win->app->win_cnt);
        if (!win->app->win_cnt && !IS_APP_FLAG(win->app, KEEP))
            rtgui_app_exit(rtgui_app_self(), RT_EOK);
        DELETE_INSTANCE(win);
    }
}
RTM_EXPORT(rtgui_win_uninit);

rt_err_t rtgui_win_enter_modal(rtgui_win_t *win) {
    rt_err_t ret;

    do {
        rtgui_evt_generic_t *evt;
        rt_base_t exit_code;

        /* send RTGUI_EVENT_WIN_MODAL_ENTER */
        RTGUI_CREATE_EVENT(evt, WIN_MODAL_ENTER, RT_WAITING_FOREVER);
        if (!evt) {
            ret = -RT_ENOMEM;
            break;
        }
        evt->win_modal_enter.wid = win;
        ret = rtgui_send_request_sync(evt);
        if (ret) break;

        LOG_D("enter modal %s", win->title);
        WIN_FLAG_SET(win, MODAL);
        win->_ref_count = win->app->ref_cnt + 1;
        exit_code = rtgui_app_run(win->app);
        LOG_I("modal %s exit %d", win->title, exit_code);

        if (IS_WIN_STYLE(win, DESTROY_ON_CLOSE)) {
            DELETE_WIN_INSTANCE(win);
        } else {
            WIN_FLAG_CLEAR(win, MODAL);
            rtgui_win_hide(win);
        }
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_win_enter_modal);

rt_err_t rtgui_win_show(rtgui_win_t *win, rt_bool_t is_modal) {
    WIDGET_FLAG_SET(win, SHOWN);
    win->_magic = RTGUI_WIN_MAGIC;
    if (is_modal) WIN_FLAG_SET(win, MODAL);

    if (win->_do_show) return win->_do_show(win);
    return _win_do_show(win);
}
RTM_EXPORT(rtgui_win_show);

void rtgui_win_end_modal(rtgui_win_t *win, rtgui_modal_code_t modal) {
    rt_uint32_t cnt = 0;

    if (!win || !IS_WIN_FLAG(win, MODAL)) return;

    while (win->_ref_count < win->app->ref_cnt) {
        rtgui_app_exit(win->app, RT_EOK);
        cnt++;
        if (cnt >= 1000) {
            LOG_E("call rtgui_app_exit() x%d!", cnt);
            RT_ASSERT(cnt < 1000);
        }
    }

    rtgui_app_exit(win->app, modal);
    WIN_FLAG_CLEAR(win, MODAL);
}
RTM_EXPORT(rtgui_win_end_modal);

void rtgui_win_hide(rtgui_win_t *win) {
    rtgui_evt_generic_t *evt;

    RT_ASSERT(win != RT_NULL);
    if (!IS_WIDGET_FLAG(win, SHOWN) || !IS_WIN_FLAG(win, CONNECTED))
        return;

    /* send RTGUI_EVENT_WIN_HIDE */
    RTGUI_CREATE_EVENT(evt, WIN_HIDE, RT_WAITING_FOREVER);
    if (!evt) return;
    evt->win_hide.wid = win;
    if (rtgui_send_request_sync(evt)) return;

    rtgui_widget_hide(TO_WIDGET(win));
    WIN_FLAG_CLEAR(win, ACTIVATE);
}
RTM_EXPORT(rtgui_win_hide);

rt_err_t rtgui_win_activate(rtgui_win_t *win) {
    rt_err_t ret;

    do {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_WIN_ACTIVATE */
        RTGUI_CREATE_EVENT(evt, WIN_ACTIVATE, RT_WAITING_FOREVER);
        if (!evt) {
            ret = -RT_ENOMEM;
            break;
        }
        evt->win_activate.wid = win;
        ret = rtgui_send_request_sync(evt);
        if (ret) break;
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_win_activate);

void rtgui_win_move(rtgui_win_t *win, int x, int y) {
    rtgui_widget_t *wgt;
    int dx, dy;

    if (RT_NULL == win) return;

    if (win->_title) {
        wgt = TO_WIDGET(win->_title);
        dx = x - wgt->extent.x1;
        dy = y - wgt->extent.y1;
        rtgui_widget_move_to_logic(wgt, dx, dy);

        wgt = TO_WIDGET(win);
        rtgui_widget_move_to_logic(wgt, dx, dy);
    } else {
        wgt = TO_WIDGET(win);
        dx = x - wgt->extent.x1;
        dy = y - wgt->extent.y1;
        rtgui_widget_move_to_logic(wgt, dx, dy);
    }
    rtgui_rect_move(&win->outer_extent, dx, dy);

    if (IS_WIN_FLAG(win, CONNECTED)) {
        rtgui_evt_generic_t *evt;

        rtgui_widget_hide(TO_WIDGET(win));

        /* send RTGUI_EVENT_WIN_MOVE */
        RTGUI_CREATE_EVENT(evt, WIN_MOVE, RT_WAITING_FOREVER);
        if (!evt) return;
        evt->win_move.wid = win;
        evt->win_move.x = x;
        evt->win_move.y = y;
        if (rtgui_send_request_sync(evt)) return;
    }
    rtgui_widget_show(TO_WIDGET(win));
}
RTM_EXPORT(rtgui_win_move);

void rtgui_win_update_clip(rtgui_win_t *win) {
    rtgui_container_t *cntr;
    rt_slist_t *node;

    if (RT_NULL == win) return;
    if (IS_WIN_FLAG(win, CLOSED)) return;

    if (win->_title) {
        rtgui_rect_t fixed = win->outer_extent;

        if (!IS_WIN_STYLE(win, NO_BORDER))
            rtgui_rect_inflate(&fixed, -TITLE_DEFAULT_BORDER);
        fixed.y1 += TITLE_DEFAULT_HEIGHT;

        rtgui_widget_set_rect(TO_WIDGET(win->_title), &win->outer_extent);
        TO_WIDGET(win)->extent = fixed;
    } else {
        TO_WIDGET(win)->extent = win->outer_extent;
    }
    rtgui_region_intersect_rect(&TO_WIDGET(win)->clip, &win->outer_clip,
        &TO_WIDGET(win)->extent);

    /* update the clip info of each child */
    cntr = TO_CONTAINER(win);
    rt_slist_for_each(node, &(cntr->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);

        rtgui_widget_update_clip(child);
    }
}
RTM_EXPORT(rtgui_win_update_clip);

void rtgui_win_set_rect(rtgui_win_t *win, rtgui_rect_t *rect) {
    if (!win || !rect) return;
    TO_WIDGET(win)->extent = *rect;

    if (IS_WIN_FLAG(win, CONNECTED)) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_WIN_RESIZE */
        RTGUI_CREATE_EVENT(evt, WIN_RESIZE, RT_WAITING_FOREVER);
        if (evt) {
            evt->win_resize.wid = win;
            evt->win_resize.rect = *rect;
            rtgui_send_request(evt, RT_WAITING_FOREVER);
        }
    }
}
RTM_EXPORT(rtgui_win_set_rect);

void rtgui_win_set_title(rtgui_win_t *win, const char *title) {
    /* modify in local side */
    if (win->title) {
        rtgui_free(win->title);
        win->title = RT_NULL;
    }
    if (title) {
        win->title = rt_strdup(title);
    }
}
RTM_EXPORT(rtgui_win_set_title);


#ifdef GUIENGIN_USING_VFRAMEBUFFER

rtgui_dc_t *rtgui_win_get_drawing(rtgui_win_t * win) {
    rtgui_dc_t *dc;
    rtgui_rect_t rect;

    if (rtgui_app_self() == RT_NULL)
        return RT_NULL;
    if (!win || !IS_WIN_FLAG(win, CONNECTED))
        return RT_NULL;

    if (win->app == rtgui_app_self()) {
        /* under the same app context */
        rtgui_region_t region;
        rtgui_region_t clip_region;

        rtgui_region_init_empty(&clip_region);
        rtgui_region_copy(&clip_region, &win->outer_clip);

        rtgui_gfx_driver_vmode_enter();

        rtgui_gfx_get_rect(RT_NULL, &rect);
        region.data = RT_NULL;
        region.extents.x1 = rect.x1;
        region.extents.y1 = rect.y1;
        region.extents.x2 = rect.x2;
        region.extents.y2 = rect.y2;

        /* remove clip */
        rtgui_region_reset(&win->outer_clip,
                           &TO_WIDGET(win)->extent);
        rtgui_region_intersect(&win->outer_clip, &region);
        rtgui_win_update_clip(win);
        /* use virtual framebuffer */
        rtgui_widget_update(TO_WIDGET(win));

        /* get the extent of widget */
        rect = WIDGET_GETTER(extent)(TO_WIDGET(win));

        dc = rtgui_graphic_driver_get_rect_buffer(RT_NULL, &rect);

        rtgui_gfx_driver_vmode_exit();

        /* restore the clip information of window */
        rtgui_region_reset(&TO_WIDGET(win)->clip,
                           &TO_WIDGET(win)->extent);
        rtgui_region_intersect(&(TO_WIDGET(win)->clip), &clip_region);
        rtgui_region_uninit(&region);
        rtgui_region_uninit(&clip_region);

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
            ret = rtgui_request(win->app, evt, RT_WAITING_FOREVER);
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
#endif /* GUIENGIN_USING_VFRAMEBUFFER */
