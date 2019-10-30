/*
 * File      : topwin.c
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
 * 2009-10-16     Bernard      first version
 * 2012-02-25     Grissiom     rewrite topwin implementation
 * 2019-05-15     onelife      Refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/app/mouse.h"
#include "include/app/topwin.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "SRV_TOP"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/*
 * windows tree in the server side.
 *
 * This list is divided into two parts. The first part is the shown list, in
 * which all the windows have the RTGUI_TOPWIN_FLAG_SHOWN flag set. Second part is the
 * hidden items, in which all the windows don't have RTGUI_TOPWIN_FLAG_SHOWN flag.
 *
 * The active window is the one that would receive kbd events. It should always
 * be in the first tree. The order of this list is the order of the windows.
 * Top window can always clip the window beneath it when the two
 * overlapping. Child window can always clip it's parent. Slibing windows can
 * clip each other with the same rule as this list. Each child list is the same
 * as _topwin_list. This forms the hierarchy tree structure of all
 * windows.
 *
 * Thus, the left most leaf of the tree is the top most window and the right
 * most root node is the bottom window.  The hidden part have no specific
 * order.
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define get_topwin_by_list(ptr)     rt_container_of(ptr, rtgui_topwin_t, list)
#define rt_list_foreach(node, list, _dir) \
    for ((node) = (list)->_dir; (node) != list; (node) = (node)->_dir)
#define IS_ROOT(top)                (top->parent == RT_NULL)

/* Private function prototypes -----------------------------------------------*/
static void _topwin_update_clip(void);
static void topwin_redraw(rtgui_rect_t *rect);
static void _topwin_activate_next_with_flag(rtgui_topwin_flag_t flag);

/* Private variables ---------------------------------------------------------*/
static rt_list_t _topwin_list = RT_LIST_OBJECT_INIT(_topwin_list);

/* Private functions ---------------------------------------------------------*/
static rtgui_topwin_t *_topwin_search_win_in_list(rtgui_win_t *win,
    rt_list_t *list) {
    rt_list_t *node;
    rtgui_topwin_t *top;

    /* the action is tend to operate on the top most window. So we search in a
     * depth first order.
     */
    rt_list_foreach(node, list, next) {
        top = (rtgui_topwin_t *)rt_list_entry(node, rtgui_topwin_t, list);
        if (win == top->wid) return top;

        // LOG_D("top->children %p", &top->children);
        top = _topwin_search_win_in_list(win, &top->children);
        if (top) return top;
    }

    return RT_NULL;
}

static rtgui_topwin_t *_topwin_get_root(rtgui_topwin_t *top) {
    rtgui_topwin_t *root = top;

    while (root && !IS_ROOT(root)) {
        root = root->parent;
    }

    LOG_D("root %s", root->wid->title);
    return root;
}

static rtgui_topwin_t *_topwin_get_deepest_shown_child(rtgui_topwin_t *top) {
    while (!(rt_list_isempty(&top->children)) && \
           IS_TOPWIN_FLAG(get_topwin_by_list(top->children.next), SHOWN)) {
        top = get_topwin_by_list(top->children.next);
    }
    return top;
}

static rt_bool_t _topwin_get_with_flag(rtgui_topwin_t *top,
    rtgui_topwin_flag_t flag) {
    switch (flag) {
    case RTGUI_TOPWIN_FLAG_ONTOP:
        return IS_TOPWIN_FLAG(top, ONTOP);
    case RTGUI_TOPWIN_FLAG_ONBTM:
        return IS_TOPWIN_FLAG(top, ONBTM);
    default:
        return RT_FALSE;
    }
}

static rtgui_topwin_t *_topwin_get_shown_with_flag(rtgui_topwin_flag_t flag) {
    rt_list_t *node;

    rt_list_foreach(node, &_topwin_list, next) {
        rtgui_topwin_t *top = get_topwin_by_list(node);

        /* reach the hidden region no window shown in current layer */
        if (!IS_TOPWIN_FLAG(top, SHOWN)) return RT_NULL;
        if (_topwin_get_with_flag(top, flag))
            return _topwin_get_deepest_shown_child(top);
    }

    /* no window in current layer is shown */
    return RT_NULL;
}

static rtgui_topwin_t* _topwin_get_next_shown(rtgui_topwin_t *top) {
    if (!top->parent) {
        /* move to next */
        if ((top->list.next != &_topwin_list) && \
            IS_TOPWIN_FLAG(get_topwin_by_list(top->list.next), SHOWN)) {
            top = _topwin_get_deepest_shown_child(
                get_topwin_by_list(top->list.next));
        } else {
            top = RT_NULL;
        }
    } else {
        if ((top->list.next != &top->parent->children) && \
            IS_TOPWIN_FLAG(get_topwin_by_list(top->list.next), SHOWN)) {
            /* move to next */
            top = _topwin_get_deepest_shown_child(
                get_topwin_by_list(top->list.next));
        } else {
            top = top->parent;
        }
    }

    return top;
}

/* a hidden parent will hide it's children. Top list window can be shown at
 * any time. */
static rt_bool_t _is_topwin_shown(rtgui_topwin_t *top) {
    rtgui_topwin_t *parent;

    for (parent = top->parent; parent != RT_NULL; parent = parent->parent) {
        if (!IS_TOPWIN_FLAG(parent, SHOWN)) return RT_FALSE;
    }
    return RT_TRUE;
}

static void _topwin_get_union_region(rtgui_topwin_t *top,
    rtgui_region_t *region) {
    rt_list_t *node;

    rt_list_foreach(node, &top->children, next) {
        _topwin_get_union_region(get_topwin_by_list(node), region);
    }
    rtgui_region_union_rect(region, region, &top->extent);
}

/* The return value of this function is the next node in tree.
 *
 * As we freed the node in this function, it would be a null reference error of
 * the caller iterate the tree normally.
 */
static rt_list_t *_topwin_free(rtgui_topwin_t *top) {
    rt_list_t *node, *next;

    node = top->children.next;
    while (node != &top->children) {
        node = _topwin_free(get_topwin_by_list(node));
    }

    next = top->list.next;
    rt_list_remove(&top->list);

    /* free the monitor rect list, topwin node and title */
    while (top->monitor_list.next != RT_NULL) {
        rtgui_monitor_t *monitor = rt_container_of(
            top->monitor_list.next, rtgui_monitor_t, list);

        top->monitor_list.next = top->monitor_list.next->next;
        rtgui_free(monitor);
    }

    rtgui_free(top);
    return next;
}

/* neither deactivate the old focus nor change _topwin_list.
 * Suitable to be called when the first item is the window to be activated
 * already. */
static void _topwin_activate(rtgui_topwin_t *top) {
    rtgui_evt_generic_t *evt;

    if (IS_TOPWIN_FLAG(top, NO_FOCUS)) return;

    #ifdef RTGUI_USING_CURSOR
        if (top->wid->drawing > 0) rtgui_cursor_hide();
    #endif

    /* send RTGUI_EVENT_WIN_ACTIVATE */
    RTGUI_CREATE_EVENT(evt, WIN_ACTIVATE, RT_WAITING_FOREVER);
    if (!evt) return;
    evt->win_activate.wid = top->wid;
    if (rtgui_request(top->app, evt, RT_WAITING_FOREVER)) return;

    TOPWIN_FLAG_SET(top, ACTIVATE);
    LOG_D("_active %s", top->wid->title);
}

/* activate next window in the same layer as flag. The flag has many other
 * infomations but we only use the ONTOP/ONBTM */
static void _topwin_activate_next_with_flag(rtgui_topwin_flag_t flag) {
    rtgui_topwin_t *top;

    top = _topwin_get_shown_with_flag(flag);
    if (!top) return;

    _topwin_activate(top);
}

/* this function does not update the clip (to avoid doubel clipping). So if the
 * tree has changed, make sure it has already updated outside. */
static void _topwin_deactivate(rtgui_topwin_t *top) {
    rtgui_evt_generic_t *evt;

    #ifdef RTGUI_USING_CURSOR
        if (top->wid->drawing > 0) rtgui_cursor_show();
    #endif

    /* send RTGUI_EVENT_WIN_DEACTIVATE */
    RTGUI_CREATE_EVENT(evt, WIN_DEACTIVATE, RT_WAITING_FOREVER);
    if (!evt) return;
    evt->win_deactivate.wid = top->wid;
    if (rtgui_request(top->app, evt, RT_WAITING_FOREVER)) return;

    TOPWIN_FLAG_CLEAR(top, ACTIVATE);
    LOG_D("deactive %s", top->wid->title);
}

/* Return 1 on the tree is truely moved. If the tree is already in position,
 * return 0. */
static rt_bool_t _topwin_move_to_top(rtgui_topwin_t *top) {
    rtgui_topwin_t *root;
    rt_list_t *node;

    /* move the whole list */
    root = _topwin_get_root(top);
    RT_ASSERT(root != RT_NULL);

    /* insert node to _topwin_list */
    if (IS_TOPWIN_FLAG(top, ONTOP)) {
        if (get_topwin_by_list(_topwin_list.next) == top) return RT_FALSE;
        rt_list_remove(&root->list);
        rt_list_insert_after(&_topwin_list, &(root->list));
        return RT_TRUE;
    }

    if (IS_TOPWIN_FLAG(top, ONBTM)) {
        /* insert before the fisrt bottom layer window or hidden window */
        rtgui_topwin_t *b_top = RT_NULL;

        rt_list_foreach(node, &_topwin_list, next) {
            b_top = get_topwin_by_list(node);
            if (IS_TOPWIN_FLAG(b_top, ONBTM) || !IS_TOPWIN_FLAG(b_top, SHOWN))
                break;
        }
    } else {
        /* insert before the fisrt shown normal layer window */
        rtgui_topwin_t *n_top = RT_NULL;

        rt_list_foreach(node, &_topwin_list, next) {
            n_top = get_topwin_by_list(node);
            if (!IS_TOPWIN_FLAG(n_top, ONTOP) && IS_TOPWIN_FLAG(n_top, SHOWN))
                break;
        }
    }

    if (get_topwin_by_list(node) == root) return RT_FALSE;
    rt_list_remove(&root->list);
    rt_list_insert_before(node, &(root->list));
    return RT_TRUE;
}

static void _topwin_move_to_front(rtgui_topwin_t *top) {
    rt_list_t *list;

    if (!top->parent)
        list = &_topwin_list;
    else
        list = &top->parent->children;
    rt_list_remove(&top->list);
    rt_list_insert_after(list, &top->list);
}

/* 1. moving the whole tree(the root of the tree) to the front
   2. moving topwin to the front of it's siblings. */
static rt_bool_t _topwin_move_to_top_front(rtgui_topwin_t *top) {
    rt_bool_t moved;

    moved = _topwin_move_to_top(top);
    /* root win is aleady moved by _topwin_move_to_top */
    if (!IS_ROOT(top))
        _topwin_move_to_front(top);

    return moved;
}

static void _rtgui_topwin_draw_tree(rtgui_topwin_t *top,
    rtgui_evt_generic_t *evt) {
    rt_list_t *node;

    rt_list_foreach(node, &top->children, next) {
        if (!IS_TOPWIN_FLAG(get_topwin_by_list(node), SHOWN))
            break;
        _rtgui_topwin_draw_tree(get_topwin_by_list(node), evt);
    }

    evt->paint.wid = top->wid;
    rtgui_request(top->app, evt, RT_WAITING_FOREVER);
    LOG_D("draw %s", top->wid->title);
}

/* map func to the topwin tree in preorder.
 *
 * Remember that we are in a embedded system so write the @param func memory
 * efficiently.
 */
rt_inline void _topwin_map_func(rtgui_topwin_t *top,
    void (*func)(rtgui_topwin_t *)) {
    rt_list_t *child;

    func(top);
    rt_list_foreach(child, &top->children, next) {
        _topwin_map_func(get_topwin_by_list(child), func);
    }
}

rt_inline void _topwin_mark_hidden(rtgui_topwin_t *top) {
    TOPWIN_FLAG_CLEAR(top, SHOWN);
    WIDGET_FLAG_CLEAR(top->wid, SHOWN);
}

rt_inline void _topwin_mark_shown(rtgui_topwin_t *top) {
    if (IS_TOPWIN_FLAG(top, SHOWN)) return;
    TOPWIN_FLAG_SET(top, SHOWN);

    LOG_D("mark shown %s", top->wid->title);
    if (!IS_WIDGET_FLAG(top->wid, SHOWN))
        rtgui_widget_show(TO_WIDGET(top->wid));
}

static void _topwin_clear_modal(rtgui_topwin_t *top) {
    while (!IS_ROOT(top)) {
        rt_list_t *node;

        rt_list_foreach(node, &top->parent->children, next) {
            TOPWIN_FLAG_CLEAR(get_topwin_by_list(node), DONE_MODAL);
            WIN_FLAG_CLEAR(get_topwin_by_list(node)->wid, IN_MODAL);
            if (IS_TOPWIN_FLAG(get_topwin_by_list(node), IN_MODAL))
                goto _done;
        }
        top = top->parent;
    }
_done:
    /* clear the modal flag of the root window */
    TOPWIN_FLAG_CLEAR(top, DONE_MODAL);
    WIN_FLAG_CLEAR(top->wid, IN_MODAL);
}

static rtgui_topwin_t *_topwin_get_focus(rt_list_t *list) {
    rt_list_t *node;

    rt_list_foreach(node, list, next) {
        rtgui_topwin_t *top = get_topwin_by_list(node);
        if (IS_TOPWIN_FLAG(top, ACTIVATE)) return top;

        top = _topwin_get_focus(&top->children);
        if (top) return top;
    }

    return RT_NULL;
}

static rtgui_topwin_t *_topwin_get_at(rt_list_t *list, int x, int y,
    rt_bool_t no_modal) {
    rt_list_t *node;
    rtgui_topwin_t *ret;

    RT_ASSERT(list != RT_NULL);
    ret = RT_NULL;

    rt_list_foreach(node, list, next) {
        rtgui_topwin_t *top, *child;

        top = get_topwin_by_list(node);
        if (!IS_TOPWIN_FLAG(top, SHOWN)) break;

        child = _topwin_get_at(&top->children, x, y, no_modal);
        if (child) {
            ret = child;
            break;
        }

        if (no_modal && IS_TOPWIN_FLAG(top, DONE_MODAL)) break;

        if (rtgui_rect_contains_point(&(top->extent), x, y)) {
            ret = top;
            break;
        }
    }

    return ret;
}

/* clip region from topwin, and the windows beneath it. */
rt_inline void _rtgui_topwin_clip_to_region(rtgui_topwin_t *top,
    rtgui_region_t *region) {

    rtgui_region_reset(&top->wid->outer_clip, &top->wid->outer_extent);
    rtgui_region_intersect(&top->wid->outer_clip, &top->wid->outer_clip,
        region);
}

static void _topwin_update_clip(void) {
    rtgui_region_t region;
    rtgui_topwin_t *top;

    if (rt_list_isempty(&_topwin_list) || \
        !IS_TOPWIN_FLAG(get_topwin_by_list(_topwin_list.next), SHOWN)) {
        LOG_E("can't show");
        return;
    }

    rtgui_region_init(
        &region, 0, 0,
        rtgui_get_gfx_device()->width,
        rtgui_get_gfx_device()->height);

    /* from top to bottom. */
    top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_ONTOP);
    /* 0 is normal layer */
    if (!top) {
        top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_INIT);
    }
    if (!top) {
        top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_ONBTM);
    }


    while (top) {
        rtgui_evt_generic_t *evt;

        LOG_D("update clip top %s (%s)", top->wid->title, top->wid->app->name);
        /* send RTGUI_EVENT_CLIP_INFO */
        RTGUI_CREATE_EVENT(evt, CLIP_INFO, RT_WAITING_FOREVER);
        if (!evt) break;
        evt->clip_info.wid = top->wid;

        /* clip the topwin */
        _rtgui_topwin_clip_to_region(top, &region);
        /* update available region */
        rtgui_region_subtract_rect(&region, &region, &top->extent);

        if (RT_EOK != rtgui_request(top->app, evt, RT_WAITING_FOREVER)) {
            LOG_E("active %s err", top->wid->title);
            break;
        }

        top = _topwin_get_next_shown(top);
    }

    rtgui_region_uninit(&region);
}

static void _topwin_redraw(rt_list_t *list, rtgui_rect_t *rect) {
    rt_list_t *node;

    /* skip the hidden windows */
    rt_list_foreach(node, list, prev) {
        if (IS_TOPWIN_FLAG(get_topwin_by_list(node), SHOWN)) break;
    }

    for (; node != list; node = node->prev) {
        rtgui_topwin_t *top = get_topwin_by_list(node);

        // FIXME: intersect with clip?
        // if (!rtgui_rect_is_intersect(rect, &(top->extent))) {
        //     evt->paint.wid = top->wid;
            // < XY do !!! >
            // rtgui_request(top->app, evt);
        // }
        _topwin_redraw(&top->children, rect);
    }
}

static void topwin_redraw(rtgui_rect_t *rect) {
    _topwin_redraw(&_topwin_list, rect);
}

/* Public functions ----------------------------------------------------------*/
/* add a window to window list[hide] */
rt_err_t rtgui_topwin_add(rtgui_app_t *app, rtgui_win_t *win,
    rtgui_win_t *parent) {
    rtgui_topwin_t *top;
    rt_err_t ret;

    ret = RT_EOK;

    do {
        top = rtgui_malloc(sizeof(rtgui_topwin_t));
        if (!top) {
            LOG_E("create topwin mem err");
            ret = -RT_ENOMEM;
            break;
        }

        LOG_D("insert0 [%p] (%s)", win, app->name);
        top->app = app;
        top->wid = win;

        if (win->_title) {
            top->extent = TO_WIDGET(win->_title)->extent;
        } else {
            top->extent = TO_WIDGET(win)->extent;
        }

        top->flag = RTGUI_TOPWIN_FLAG_INIT;
        if (IS_WIN_STYLE(win, NO_FOCUS)) {
            TOPWIN_FLAG_SET(top, NO_FOCUS);
        }
        if (IS_WIN_STYLE(win, ONTOP)) {
            TOPWIN_FLAG_SET(top, ONTOP);
        }
        if (IS_WIN_STYLE(win, ONBTM)) {
            TOPWIN_FLAG_SET(top, ONBTM);
        }

        if (!parent) {
            top->parent = RT_NULL;
            LOG_D("insert as top [%p] (%s)", win, app->name);
            rt_list_insert_before(&_topwin_list, &top->list);
        } else {
            top->parent = _topwin_search_win_in_list(parent, &_topwin_list);
            if (!top->parent) {
                /* parent does not exist. Orphan window? */
                ret = -RT_ERROR;
                break;
            }
            LOG_D("insert as child [%p] (%s)", win, app->name);
            rt_list_insert_before(&top->parent->children, &top->list);
        }

        rt_list_init(&top->children);
        rt_slist_init(&top->monitor_list);
    } while (0);

    if (RT_EOK != ret) {
        rtgui_free(top);
    }

    return ret;
}

rt_err_t rtgui_topwin_remove(rtgui_win_t *win) {
    rtgui_topwin_t *top;

    /* find the topwin node */
    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return -RT_ERROR;

    /* remove the root from _topwin_list will remove the whole tree from
     * _topwin_list. */
    rt_list_remove(&top->list);

    if (top == rtgui_topwin_get_focus())
        _topwin_activate_next_with_flag(top->flag);

    if (IS_TOPWIN_FLAG(top, SHOWN)) {
        rtgui_region_t region;

        rtgui_region_init_empty(&region);
        _topwin_update_clip();
        /* redraw */
        _topwin_get_union_region(top, &region);
        topwin_redraw(rtgui_region_extents(&region));

        rtgui_region_uninit(&region);
    }

    (void)_topwin_free(top);
    return RT_EOK;
}

rt_err_t rtgui_topwin_activate(rtgui_topwin_t *top) {
    rt_err_t ret;

    ret = RT_EOK;

    do {
        rtgui_evt_generic_t *evt;
        rt_bool_t no_focus, moved;
        rtgui_topwin_t *focus;

        if (!IS_TOPWIN_FLAG(top, SHOWN)) {
            LOG_E("can't show");
            ret = -RT_ERROR;
            break;
        }

        no_focus = IS_TOPWIN_FLAG(top, NO_FOCUS);

        if (!no_focus) {
            if (IS_TOPWIN_FLAG(top, ACTIVATE)) break;
            focus = rtgui_topwin_get_focus();
            /* if topwin has the focus, it should have RTGUI_TOPWIN_FLAG_ACTIVATE
             * set and returned above. */
            RT_ASSERT(focus != top);
        } else {
            /* just raise and show, no other effects. Active window is the one
             * which will receive kbd events. So a no-focus window can only be
             * "raised" but not "activated".
             */
            focus = RT_NULL;
        }

        moved = _topwin_move_to_top_front(top);
        LOG_D("moved %d", moved);
        /* clip before active the window, so we could get right boarder region. */
        _topwin_update_clip();

        if (!no_focus) {
            if (focus) {
                /* deactivate last focused window firstly, otherwise it will make
                 * the new window invisible. */
                _topwin_deactivate(focus);
            }
            _topwin_activate(top);
        }

        /* create RTGUI_EVENT_PAINT */
        RTGUI_CREATE_EVENT(evt, PAINT, RT_WAITING_FOREVER);
        if (!evt) {
            ret = -RT_ENOMEM;
            break;
        }

        /* send RTGUI_EVENT_PAINT */
        if (moved)
            _rtgui_topwin_draw_tree(_topwin_get_root(top), evt);
        else
            _rtgui_topwin_draw_tree(top, evt);
    } while (0);

    LOG_D("active %s, ret %d", top->wid->title, ret);
    return ret;
}

/* activate a win means:
 * - deactivate the last focus win if any
 * - bring the window to the front of it's siblings
 * - activate the win
 */
rt_err_t rtgui_topwin_activate_win(rtgui_win_t *win) {
    rtgui_topwin_t *top;

    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return -RT_ERROR;

    return rtgui_topwin_activate(top);
}

rt_err_t rtgui_topwin_show(rtgui_win_t *win) {
    rtgui_topwin_t *top;

    LOG_D("search topwin %s [%p] (%s)", win->title, win, win->app->name);
    top = _topwin_search_win_in_list(win, &_topwin_list);
    /* no such a window recorded */
    if (!top) {
        LOG_E("no topwin");
        return -RT_ERROR;
    }

    /* child windows could only be shown iif the parent is shown */
    if (!_is_topwin_shown(top)) {
        LOG_E("can't show");
        return -RT_ERROR;
    }

    _topwin_map_func(top, _topwin_mark_shown);
    return rtgui_topwin_activate(top);
}

/* hide a window */
rt_err_t rtgui_topwin_hide(rtgui_win_t *win) {
    rtgui_topwin_t *top;
    rtgui_topwin_t *focus;
    rt_list_t *list;

    /* find in show list */
    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return -RT_ERROR;
    if (!IS_TOPWIN_FLAG(top, SHOWN)) return RT_EOK;
    focus = rtgui_topwin_get_focus();

    _topwin_map_func(top, _topwin_mark_hidden);

    if (!top->parent)
        list = &_topwin_list;
    else
        list = &top->parent->children;

    rt_list_remove(&top->list);
    rt_list_insert_before(list, &top->list);

    /* update clip info */
    _topwin_update_clip();

    if (IS_TOPWIN_FLAG(top, IN_MODAL)) {
        TOPWIN_FLAG_CLEAR(top, IN_MODAL);
        _topwin_clear_modal(top);
    }

    if (focus == top)
        _topwin_activate_next_with_flag(top->flag);

    TOPWIN_FLAG_CLEAR(top, ACTIVATE);
    /* redraw the rect */
    topwin_redraw(&(top->extent));

    return RT_EOK;
}

/* move top window */
rt_err_t rtgui_topwin_move(rtgui_win_t *win, rt_int16_t x, rt_int16_t y) {
    rt_err_t ret;

    ret = RT_EOK;

    do {
        rtgui_topwin_t *top;
        rt_int16_t dx, dy;
        rtgui_rect_t rect;
        rt_slist_t *node;

        /* find in show list */
        top = _topwin_search_win_in_list(win, &_topwin_list);
        if (!top || !IS_TOPWIN_FLAG(top, SHOWN)) {
            LOG_E("can't show");
            ret = -RT_ERROR;
            break;
        }

        /* get the delta move x, y */
        dx = x - top->extent.x1;
        dy = y - top->extent.y1;
        rect = top->extent;
        /* move window rect */
        rtgui_rect_move(&(top->extent), dx, dy);

        /* move the monitor rect list */
        rt_slist_for_each(node, &(top->monitor_list)) {
            rtgui_monitor_t *monitor;
            monitor = rt_slist_entry(node, rtgui_monitor_t, list);
            rtgui_rect_move(&(monitor->rect), dx, dy);
        }

        /* update windows clip info */
        _topwin_update_clip();
        /* update last window coverage area */
        topwin_redraw(&rect);

        if (!rtgui_rect_is_intersect(&rect, &(top->extent))) {
            /* last rect is not intersect with moved rect, re-paint window */
            rtgui_evt_generic_t *evt;

            /* send RTGUI_EVENT_PAINT */
            RTGUI_CREATE_EVENT(evt, PAINT, RT_WAITING_FOREVER);
            if (!evt) {
                ret = -RT_ENOMEM;
                break;
            }
            evt->paint.wid = top->wid;
            ret = rtgui_request(top->app, evt, RT_WAITING_FOREVER);
            if (ret) break;
        }
    } while (0);

    return ret;
}

/*
 * resize a top win
 * Note: currently, only support resize hidden window
 */
void rtgui_topwin_resize(rtgui_win_t *win, rtgui_rect_t *rect) {
    rtgui_topwin_t *top;
    rtgui_region_t region;

    /* find in show list */
    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top || !!IS_TOPWIN_FLAG(top, SHOWN)) return;

    /* get current region */
    rtgui_region_init_with_extent(&region, &top->extent);
    /* union the new rect so this is the region we should redraw */
    rtgui_region_union_rect(&region, &region, rect);
    top->extent = *rect;

    /* update windows clip info */
    _topwin_update_clip();

    /* update old window coverage area */
    topwin_redraw(rtgui_region_extents(&region));

    rtgui_region_uninit(&region);
}

/* a window enter modal mode will modal all the sibling window and parent
 * window all along to the root window. If a root window modals, there is
 * nothing to do here.*/
rt_err_t rtgui_topwin_modal_enter(rtgui_win_t *win) {
    rtgui_topwin_t *top, *parent;

    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return -RT_ERROR;
    if (IS_ROOT(top)) return RT_EOK;

    parent = top->parent;
    /* modal window should be on top already */
    RT_ASSERT(get_topwin_by_list(parent->children.next) == top);

    while (parent) {
        rt_list_t *node;

        rt_list_foreach(node, &parent->children, next) {
            TOPWIN_FLAG_SET(get_topwin_by_list(node), DONE_MODAL);
            WIN_FLAG_SET(get_topwin_by_list(node)->wid, IN_MODAL);
        }

        TOPWIN_FLAG_SET(parent, DONE_MODAL);
        WIN_FLAG_SET(parent->wid, IN_MODAL);
        parent = parent->parent;
    }

    TOPWIN_FLAG_CLEAR(top, DONE_MODAL);
    WIN_FLAG_CLEAR(top->wid, IN_MODAL);
    TOPWIN_FLAG_SET(top, IN_MODAL);

    return RT_EOK;
}

rtgui_topwin_t *rtgui_topwin_get_with_modal_at(int x, int y) {
    return _topwin_get_at(&_topwin_list, x, y, RT_FALSE);
}

rtgui_topwin_t *rtgui_topwin_get_at(int x, int y) {
    return _topwin_get_at(&_topwin_list, x, y, RT_TRUE);
}

void rtgui_topwin_append_monitor_rect(rtgui_win_t *win, rtgui_rect_t *rect) {
    rtgui_topwin_t *top;

    /* parameters check */
    if (!win || !rect) return;

    /* find topwin */
    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return;
    /* append rect to top window monitor rect list */
    rtgui_monitor_append(&(top->monitor_list), rect);
}

void rtgui_topwin_remove_monitor_rect(rtgui_win_t *win,
    rtgui_rect_t *rect) {
    rtgui_topwin_t *top;

    /* parameters check */
    if (!win || !rect) return;

    /* find topwin */
    top = _topwin_search_win_in_list(win, &_topwin_list);
    if (!top) return;
    /* remove rect from top window monitor rect list */
    rtgui_monitor_remove(&(top->monitor_list), rect);
}

rtgui_topwin_t *rtgui_topwin_get_focus(void) {
    return _topwin_get_focus(&_topwin_list);
}

rtgui_app_t *rtgui_topwin_get_focus_app(void) {
    rtgui_topwin_t *top;

    top = rtgui_topwin_get_focus();
    if (top) {
        return top->app;
    }
    return RT_NULL;
}

rtgui_topwin_t *rtgui_topwin_get_shown(void) {
    rtgui_topwin_t *top;

    top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_ONTOP);
    /* 0 is normal layer */
    if (!top)
        top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_INIT);
    if (!top)
        top = _topwin_get_shown_with_flag(RTGUI_TOPWIN_FLAG_ONBTM);

    return top;
}

rtgui_win_t* rtgui_topwin_get_shown_win(void) {
    rtgui_topwin_t *top = rtgui_topwin_get_shown();
    if (!top) return RT_NULL;
    return top->wid;
}

rtgui_win_t* rtgui_topwin_get_next_shown_win(void) {
    rtgui_topwin_t *top = rtgui_topwin_get_shown();
    if (!top) return RT_NULL;

    top = _topwin_get_next_shown(top);
    if (!top) return RT_NULL;
    return top->wid;
}

#if 0
rtgui_obj_t* rtgui_topwin_get_object(rtgui_topwin_t *top,
    rtgui_app_t *app, rt_uint32_t id) {
    rtgui_obj_t *object;
    rt_list_t *node;

    object = TO_OBJECT(top->wid);
    if (object->id == id) return object;

    object = rtgui_container_get_object(TO_CONTAINER(object), id);
    if (object) return object;

    rt_list_foreach(node, &top->children, next) {
        rtgui_topwin_t *top_;

        top_ = get_topwin_by_list(node);
        if (top_->app != app) continue;

        object = rtgui_topwin_get_object(top_, app, id);
        if (object) return object;
    }

    return RT_NULL;
}

rtgui_obj_t* rtgui_get_object(rtgui_app_t *app, rt_uint32_t id) {
    rtgui_obj_t *obj;
    rt_list_t *node;

    obj = TO_OBJECT(app);
    if (obj->id == id) return obj;

    rt_list_foreach(node, &_topwin_list, next) {
        rtgui_topwin_t *top;

        top = get_topwin_by_list(node);
        if (top->app != app) continue;

        obj = rtgui_topwin_get_object(top, app, id);
        if (obj) return obj;
    }
    return RT_NULL;
}
RTM_EXPORT(rtgui_get_object);

rtgui_obj_t* rtgui_get_self_object(rt_uint32_t id) {
    return rtgui_get_object(rtgui_app_self(), id);
}
RTM_EXPORT(rtgui_get_self_object);
#endif

#ifdef RT_USING_FINSH
#include "components/finsh/finsh.h"

static void _topwin_dump(rtgui_topwin_t *top) {
    rt_list_t *node;

    rt_kprintf("0x%p: %s, 0x%x, %c%c ",
        top, top->wid->title, top->flag,
        IS_TOPWIN_FLAG(top, SHOWN) ? 'S' : 'H',
        IS_TOPWIN_FLAG(top, DONE_MODAL) ? 'm' : \
            IS_TOPWIN_FLAG(top, IN_MODAL) ? 'M' : ' ');

    rt_kprintf("(");
    rt_list_foreach(node, &top->children, next) {
        _topwin_dump(get_topwin_by_list(node));
    }
    rt_kprintf(")");
}

void dump_tree() {
    rt_list_t *node;

    rt_list_foreach(node, &_topwin_list, next) {
        _topwin_dump(get_topwin_by_list(node));
        rt_kprintf("\n");
    }
}
FINSH_FUNCTION_EXPORT(dump_tree, dump rtgui topwin tree)

#endif /* RT_USING_FINSH */
