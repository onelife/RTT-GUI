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

// #include "include/rtservice.h"

#include "../include/rtgui.h"
#include "../include/widgets/topwin.h"
#include "../include/widgets/mouse.h"
#include "../include/image.h"
//#include <rtgui/rtgui_theme.h>
#include "../include/widgets/window.h"
#include "../include/widgets/container.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_TOP"
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
 * which all the windows have the WINTITLE_SHOWN flag set. Second part is the
 * hidden items, in which all the windows don't have WINTITLE_SHOWN flag.
 *
 * The active window is the one that would receive kbd events. It should always
 * be in the first tree. The order of this list is the order of the windows.
 * Top window can always clip the window beneath it when the two
 * overlapping. Child window can always clip it's parent. Slibing windows can
 * clip each other with the same rule as this list. Each child list is the same
 * as _rtgui_topwin_list. This forms the hierarchy tree structure of all
 * windows.
 *
 * Thus, the left most leaf of the tree is the top most window and the right
 * most root node is the bottom window.  The hidden part have no specific
 * order.
 */
#define get_topwin_from_list(list_entry) (rt_list_entry((list_entry), rtgui_topwin_t, list))
#define rt_list_foreach(node, list, _dir)  \
    for ((node) = (list)->_dir; (node) != list; (node) = (node)->_dir)

#define IS_ROOT_WIN(topwin) ((topwin)->parent == RT_NULL)

static rt_list_t _rtgui_topwin_list = RT_LIST_OBJECT_INIT(_rtgui_topwin_list);
static struct rt_semaphore _rtgui_topwin_lock;

static void rtgui_topwin_update_clip(void);
static void rtgui_topwin_redraw(rtgui_rect_t *rect);
static void _rtgui_topwin_activate_next(enum rtgui_topwin_flag);

void rtgui_topwin_init(void) {
    /* initialize semaphore */
    rt_sem_init(&_rtgui_topwin_lock, "wintree", 1, RT_IPC_FLAG_FIFO);
}

static rtgui_topwin_t *rtgui_topwin_search_in_list(
    rtgui_win_t *win, struct rt_list_node *list) {
    /* TODO: use a cache to speed up the search. */
    rt_list_t *node;
    rtgui_topwin_t *topwin;

    /* the action is tend to operate on the top most window. So we search in a
     * depth first order.
     */
    rt_list_foreach(node, list, next) {
        topwin = (rtgui_topwin_t *)rt_list_entry(node, rtgui_topwin_t, list);
        /* is this node? */
        if (topwin->wid == win) return topwin;

        LOG_I("topwin->child_list %p", &topwin->child_list);
        topwin = rtgui_topwin_search_in_list(win, &topwin->child_list);
        if (topwin != RT_NULL) return topwin;
    }

    return RT_NULL;
}

/* add a window to window list[hide] */
rt_err_t rtgui_topwin_add(struct rtgui_event_win_create *event) {
    rtgui_topwin_t *topwin;
    rt_err_t ret;

    do {
        topwin = rtgui_malloc(sizeof(rtgui_topwin_t));
        if (topwin == RT_NULL) {
            LOG_E("create topwin failed");
            ret = -RT_ERROR;
            break;
        }

        topwin->wid = event->wid;
        if (event->wid->_title_wgt) {
            topwin->extent = TO_WIDGET(event->wid->_title_wgt)->extent;
        } else {
            topwin->extent = TO_WIDGET(event->wid)->extent;
        }
        topwin->app = event->base.sender;
        LOG_D("insert0 [%p] (%s)", event->wid, event->base.sender->name);

        if (RT_NULL == event->parent_window) {
            LOG_D("insert as top [%p] (%s)", topwin->wid, topwin->app->name);
            topwin->parent = RT_NULL;
            rt_list_insert_before(&_rtgui_topwin_list, &topwin->list);
        } else {
            topwin->parent = rtgui_topwin_search_in_list(
                event->parent_window, &_rtgui_topwin_list);
            if (RT_NULL == topwin->parent) {
                /* parent does not exist. Orphan window? */
                ret = -RT_ERROR;
                break;
            }
            LOG_D("insert as child [%p] (%s)", topwin->wid, topwin->app->name);
            rt_list_insert_before(&topwin->parent->child_list, &topwin->list);
        }

        rt_list_init(&topwin->child_list);
        topwin->flag = WINTITLE_INIT;
        if (event->base.user & RTGUI_WIN_STYLE_NO_FOCUS) {
            topwin->flag |= WINTITLE_NOFOCUS;
        }
        if (event->base.user & RTGUI_WIN_STYLE_ONTOP) {
            topwin->flag |= WINTITLE_ONTOP;
        }
        if (event->base.user & RTGUI_WIN_STYLE_ONBTM) {
            topwin->flag |= WINTITLE_ONBTM;
        }
        topwin->title = RT_NULL;
        rt_slist_init(&topwin->monitor_list);

        ret = RT_EOK;
    } while (0);

    if (RT_EOK != ret) {
        rtgui_free(topwin);
    }

    return ret;
}

static rtgui_topwin_t *_rtgui_topwin_get_root_win(
    rtgui_topwin_t *topwin) {
    rtgui_topwin_t *parent;

    RT_ASSERT(topwin != RT_NULL);

    parent = topwin;
    while (parent && !IS_ROOT_WIN(parent)) {
        parent = parent->parent;
    }
    LOG_D("root %s", parent->wid->title);
    return parent;
}

static rtgui_topwin_t *_rtgui_topwin_get_topmost_child_shown(
    rtgui_topwin_t *topwin) {
    RT_ASSERT(topwin != RT_NULL);

    while (!(rt_list_isempty(&topwin->child_list)) && \
           (get_topwin_from_list(topwin->child_list.next)->flag & \
            WINTITLE_SHOWN)) {
        topwin = get_topwin_from_list(topwin->child_list.next);
    }
    return topwin;
}

static rt_bool_t _rtgui_topwin_in_layer(rtgui_topwin_t *topwin,
    enum rtgui_topwin_flag flag) {
    return ((WINTITLE_ONTOP | WINTITLE_ONBTM) & topwin->flag) == \
           ((WINTITLE_ONTOP | WINTITLE_ONBTM) & flag);
}

/* find the topmost window shown in the layer set by flag. The flag has many
 * other infomations but we only use the ONTOP/ONBTM */
rtgui_topwin_t *rtgui_topwin_get_topmost_window_shown(
    enum rtgui_topwin_flag flag) {
    rt_list_t *node;

    rt_list_foreach(node, &_rtgui_topwin_list, next) {
        rtgui_topwin_t *topwin = get_topwin_from_list(node);

        /* reach the hidden region no window shown in current layer */
        if (!(topwin->flag & WINTITLE_SHOWN)) {
            return RT_NULL;
        }
        if (_rtgui_topwin_in_layer(topwin, flag)) {
            return _rtgui_topwin_get_topmost_child_shown(topwin);
        }
    }
    /* no window in current layer is shown */
    return RT_NULL;
}

rtgui_topwin_t *rtgui_topwin_get_topmost_window_shown_all(void)
{
    rtgui_topwin_t *top;

    top = rtgui_topwin_get_topmost_window_shown(WINTITLE_ONTOP);
    /* 0 is normal layer */
    if (top == RT_NULL)
        top = rtgui_topwin_get_topmost_window_shown(WINTITLE_INIT);
    if (top == RT_NULL)
        top = rtgui_topwin_get_topmost_window_shown(WINTITLE_ONBTM);

    return top;
}

rtgui_win_t* rtgui_win_get_topmost_shown(void)
{
    rtgui_topwin_t *top;

    top = rtgui_topwin_get_topmost_window_shown_all();
    if (!top)
        return RT_NULL;
    return top->wid;
}

static rtgui_topwin_t* _rtgui_topwin_get_next_shown(rtgui_topwin_t *top)
{
    /* move to next sibling tree */
    if (top->parent == RT_NULL)
    {
        if (top->list.next != &_rtgui_topwin_list &&
                get_topwin_from_list(top->list.next)->flag & WINTITLE_SHOWN)
            top = _rtgui_topwin_get_topmost_child_shown(get_topwin_from_list(top->list.next));
        else
            return RT_NULL;
    }
    /* move to next slibing topwin */
    else if (top->list.next != &top->parent->child_list &&
             get_topwin_from_list(top->list.next)->flag & WINTITLE_SHOWN)
    {
        top = _rtgui_topwin_get_topmost_child_shown(get_topwin_from_list(top->list.next));
    }
    /* level up */
    else
    {
        top = top->parent;
    }

    return top;
}

rtgui_win_t* rtgui_win_get_next_shown(void)
{
    rtgui_topwin_t *top;

    top = rtgui_topwin_get_topmost_window_shown_all();
    if (!top)
        return RT_NULL;

    top = _rtgui_topwin_get_next_shown(top);
    if (!top)
        return RT_NULL;

    return top->wid;
}

/* a hidden parent will hide it's children. Top level window can be shown at
 * any time. */
static rt_bool_t _rtgui_topwin_could_show(rtgui_topwin_t *topwin) {
    rtgui_topwin_t *parent;

    RT_ASSERT(topwin != RT_NULL);

    for (parent = topwin->parent; parent != RT_NULL; parent = parent->parent) {
        if (!(parent->flag & WINTITLE_SHOWN)) return RT_FALSE;
    }
    return RT_TRUE;
}

static void _rtgui_topwin_union_region_tree(rtgui_topwin_t *topwin,
        struct rtgui_region *region)
{
    rt_list_t *node;

    RT_ASSERT(topwin != RT_NULL);

    rt_list_foreach(node, &topwin->child_list, next)
    _rtgui_topwin_union_region_tree(get_topwin_from_list(node), region);

    rtgui_region_union_rect(region, region, &topwin->extent);
}

/* The return value of this function is the next node in tree.
 *
 * As we freed the node in this function, it would be a null reference error of
 * the caller iterate the tree normally.
 */
static rt_list_t *_rtgui_topwin_free_tree(rtgui_topwin_t *topwin)
{
    rt_list_t *node, *next_node;

    RT_ASSERT(topwin != RT_NULL);

    node = topwin->child_list.next;
    while (node != &topwin->child_list)
        node = _rtgui_topwin_free_tree(get_topwin_from_list(node));

    next_node = topwin->list.next;
    rt_list_remove(&topwin->list);

    /* free the monitor rect list, topwin node and title */
    while (topwin->monitor_list.next != RT_NULL)
    {
        struct rtgui_mouse_monitor *monitor = \
        rt_slist_entry(topwin->monitor_list.next, struct rtgui_mouse_monitor, list);

        topwin->monitor_list.next = topwin->monitor_list.next->next;
        rtgui_free(monitor);
    }

    rtgui_free(topwin);
    return next_node;
}

rt_err_t rtgui_topwin_remove(rtgui_win_t *wid)
{
    rtgui_topwin_t *topwin, *old_focus;
    struct rtgui_region region;

    /* find the topwin node */
    topwin = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);

    if (topwin == RT_NULL) return -RT_ERROR;

    rtgui_region_init(&region);

    old_focus = rtgui_topwin_get_focus();

    /* remove the root from _rtgui_topwin_list will remove the whole tree from
     * _rtgui_topwin_list. */
    rt_list_remove(&topwin->list);

    if (old_focus == topwin)
    {
        _rtgui_topwin_activate_next(topwin->flag);
    }

    if (topwin->flag & WINTITLE_SHOWN)
    {
        rtgui_topwin_update_clip();
        /* redraw the old rect */
        _rtgui_topwin_union_region_tree(topwin, &region);
        rtgui_topwin_redraw(rtgui_region_extents(&region));
    }

    rtgui_region_fini(&region);
    _rtgui_topwin_free_tree(topwin);

    return RT_EOK;
}

/* neither deactivate the old focus nor change _rtgui_topwin_list.
 * Suitable to be called when the first item is the window to be activated
 * already. */
static void _rtgui_topwin_only_activate(rtgui_topwin_t *topwin) {
    rtgui_evt_generic_t *evt;

    RT_ASSERT(topwin != RT_NULL);
    if (topwin->flag & WINTITLE_NOFOCUS) return;

    /* send RTGUI_EVENT_WIN_ACTIVATE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
        rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        rt_err_t ret;

        RTGUI_EVENT_INIT(evt, WIN_ACTIVATE);
        evt->win_activate.wid = topwin->wid;
        ret = rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("_active %s err [%d]", topwin->wid->title, ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }

    topwin->flag |= WINTITLE_ACTIVATE;
    LOG_D("_active %s", topwin->wid->title);
}

/* activate next window in the same layer as flag. The flag has many other
 * infomations but we only use the ONTOP/ONBTM */
static void _rtgui_topwin_activate_next(enum rtgui_topwin_flag flag) {
    rtgui_topwin_t *topwin;

    topwin = rtgui_topwin_get_topmost_window_shown(flag);
    if (!topwin) return;

    _rtgui_topwin_only_activate(topwin);
}

/* this function does not update the clip(to avoid doubel clipping). So if the
 * tree has changed, make sure it has already updated outside. */
static void _rtgui_topwin_deactivate(rtgui_topwin_t *topwin) {
    rtgui_evt_generic_t *evt;

    RT_ASSERT(topwin != RT_NULL);
    RT_ASSERT(topwin->app != RT_NULL);

    /* send RTGUI_EVENT_WIN_DEACTIVATE */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
        rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        rt_err_t ret;
        RTGUI_EVENT_INIT(evt, WIN_DEACTIVATE);
        evt->win_deactivate.wid = topwin->wid;
        ret = rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("deactive %s err [%d]", topwin->wid->title, ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }

    topwin->flag &= ~WINTITLE_ACTIVATE;
    LOG_E("deactive %s", topwin->wid->title);
}

/* Return 1 on the tree is truely moved. If the tree is already in position,
 * return 0. */
static rt_uint32_t _rtgui_topwin_move_whole_tree2top(rtgui_topwin_t *topwin)
{
    rtgui_topwin_t *topparent;

    RT_ASSERT(topwin != RT_NULL);

    /* move the whole tree */
    topparent = _rtgui_topwin_get_root_win(topwin);
    RT_ASSERT(topparent != RT_NULL);

    /* add node to show list */
    if (topwin->flag & WINTITLE_ONTOP)
    {
        if (get_topwin_from_list(_rtgui_topwin_list.next) == topwin)
            return 0;
        rt_list_remove(&topparent->list);
        rt_list_insert_after(&_rtgui_topwin_list, &(topparent->list));
    }
    else if (topwin->flag & WINTITLE_ONBTM)
    {
        /* botton layer window, before the fisrt bottom window or hidden window. */
        rt_list_t *node;
        rtgui_topwin_t *ntopwin = RT_NULL;

        rt_list_foreach(node, &_rtgui_topwin_list, next)
        {
            ntopwin = get_topwin_from_list(node);
            if ((ntopwin->flag & WINTITLE_ONBTM)
                    || !(ntopwin->flag & WINTITLE_SHOWN))
                break;
        }
        if (get_topwin_from_list(node) == topparent)
            return 0;
        rt_list_remove(&topparent->list);
        rt_list_insert_before(node, &(topparent->list));
    }
    else
    {
        /* normal layer window, before the fisrt shown normal layer window. */
        rtgui_topwin_t *ntopwin = RT_NULL;
        rt_list_t *node;

        rt_list_foreach(node, &_rtgui_topwin_list, next)
        {
            ntopwin = get_topwin_from_list(node);
            if (!((ntopwin->flag & WINTITLE_ONTOP)
                    && (ntopwin->flag & WINTITLE_SHOWN)))
                break;
        }
        if (get_topwin_from_list(node) == topparent)
            return 0;
        rt_list_remove(&topparent->list);
        rt_list_insert_before(node, &(topparent->list));
    }
    return 1;
}

static void _rtgui_topwin_raise_in_sibling(rtgui_topwin_t *topwin)
{
    rt_list_t *win_level;

    RT_ASSERT(topwin != RT_NULL);

    if (topwin->parent == RT_NULL)
        win_level = &_rtgui_topwin_list;
    else
        win_level = &topwin->parent->child_list;
    rt_list_remove(&topwin->list);
    rt_list_insert_after(win_level, &topwin->list);
}

/* it will do 2 things. One is moving the whole tree(the root of the tree) to
 * the front and the other is moving topwin to the front of it's siblings. */
static rt_uint32_t _rtgui_topwin_raise_tree_from_root(rtgui_topwin_t *topwin) {
    rt_uint32_t moved;

    RT_ASSERT(topwin != RT_NULL);

    moved = _rtgui_topwin_move_whole_tree2top(topwin);
    /* root win is aleady moved by _rtgui_topwin_move_whole_tree2top */
    if (!IS_ROOT_WIN(topwin)) _rtgui_topwin_raise_in_sibling(topwin);

    return moved;
}

/* activate a win means:
 * - deactivate the old focus win if any
 * - raise the window to the front of it's siblings
 * - activate a win
 */
rt_err_t rtgui_topwin_activate(struct rtgui_event_win_activate *event)
{
    rtgui_topwin_t *topwin;

    RT_ASSERT(event);

    topwin = rtgui_topwin_search_in_list(event->wid, &_rtgui_topwin_list);
    if (topwin == RT_NULL)
        return -RT_ERROR;

    return rtgui_topwin_activate_topwin(topwin);
}

static void _rtgui_topwin_draw_tree(rtgui_topwin_t *topwin,
    rtgui_evt_generic_t *evt) {
    rt_list_t *node;

    rt_list_foreach(node, &topwin->child_list, next) {
        if (!(get_topwin_from_list(node)->flag & WINTITLE_SHOWN)) {
            break;
        }
        _rtgui_topwin_draw_tree(get_topwin_from_list(node), evt);
    }

    evt->paint.wid = topwin->wid;
    rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
    LOG_D("draw %s", topwin->wid->title);
}

rt_err_t rtgui_topwin_activate_topwin(rtgui_topwin_t *topwin) {
    rt_err_t ret;

    do {
        rtgui_evt_generic_t *evt;
        rt_uint32_t moved;
        rtgui_topwin_t *last_topwin;

        if (!topwin || !(topwin->flag & WINTITLE_SHOWN)) {
            LOG_E("can't show");
            ret = -RT_ERROR;
            break;
        }

        /* send RTGUI_EVENT_PAINT */
        evt = (rtgui_evt_generic_t *)rt_mp_alloc(
            rtgui_event_pool, RT_WAITING_FOREVER);
        if (!evt) {
            LOG_E("get mp err");
            ret = -RT_ENOMEM;
            break;
        }

        RTGUI_EVENT_INIT(evt, PAINT);
        evt->win_activate.wid = topwin->wid;
        ret = rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("active %s err [%d]", topwin->wid->title, ret);
            break;
        }

        if (topwin->flag & WINTITLE_NOFOCUS) {
            /* just raise and show, no other effects. Active window is the one
             * which will receive kbd events. So a no-focus window can only be
             * "raised" but not "activated".
             */
            moved = _rtgui_topwin_raise_tree_from_root(topwin);
            rtgui_topwin_update_clip();
            /* send RTGUI_EVENT_PAINT */
            if (moved) {
                _rtgui_topwin_draw_tree(
                    _rtgui_topwin_get_root_win(topwin), evt);
            } else {
                _rtgui_topwin_draw_tree(topwin, evt);
            }
            ret = RT_EOK;
            break;
        }

        if (topwin->flag & WINTITLE_ACTIVATE) {
            ret = RT_EOK;
            break;
        }

        last_topwin = rtgui_topwin_get_focus();
        /* if topwin has the focus, it should have WINTITLE_ACTIVATE set and
         * returned above. */
        RT_ASSERT(last_topwin != topwin);

        moved = _rtgui_topwin_raise_tree_from_root(topwin);
        /* clip before active the window, so we could get right boarder region. */
        rtgui_topwin_update_clip();

        if (last_topwin) {
            /* deactivate the old focus window firstly, otherwise it will make the new
             * window invisible. */
            /* send RTGUI_EVENT_WIN_DEACTIVATE */
            _rtgui_topwin_deactivate(last_topwin);
        }
        _rtgui_topwin_only_activate(topwin);

        /* send RTGUI_EVENT_PAINT */
        if (moved) {
            _rtgui_topwin_draw_tree(
                _rtgui_topwin_get_root_win(topwin), evt);
        } else {
            _rtgui_topwin_draw_tree(topwin, evt);
        }
    } while (0);

    LOG_D("active %s", topwin->wid->title);
    return ret;
}

/* map func to the topwin tree in preorder.
 *
 * Remember that we are in a embedded system so write the @param func memory
 * efficiently.
 */
rt_inline void _rtgui_topwin_preorder_map(rtgui_topwin_t *topwin,
    void (*func)(rtgui_topwin_t *)) {
    rt_list_t *child;

    RT_ASSERT(topwin != RT_NULL);
    RT_ASSERT(func != RT_NULL);

    func(topwin);

    rt_list_foreach(child, &topwin->child_list, next)
    _rtgui_topwin_preorder_map(get_topwin_from_list(child), func);
}

rt_inline void _rtgui_topwin_mark_hidden(rtgui_topwin_t *topwin)
{
    topwin->flag &= ~WINTITLE_SHOWN;
    RTGUI_WIDGET_HIDE(topwin->wid);
}

rt_inline void _rtgui_topwin_mark_shown(rtgui_topwin_t *topwin) {
    if (topwin->flag & WINTITLE_SHOWN) return;
    topwin->flag |= WINTITLE_SHOWN;

    LOG_D("mark show %s", topwin->wid->title);
    if (RTGUI_WIDGET_IS_HIDE(topwin->wid)) {
        rtgui_widget_show(TO_WIDGET(topwin->wid));
    }
}

rt_err_t rtgui_topwin_show(struct rtgui_event_win *evt) {
    rtgui_topwin_t *topwin;
    rtgui_win_t *wid = evt->wid;

    LOG_D("search topwin %s [%p] (%s)", wid->title, wid, wid->app->name);
    topwin = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);
    /* no such a window recorded */
    if (RT_NULL == topwin) {
        LOG_E("no topwin");
        return -RT_ERROR;
    }

    /* child windows could only be shown iif the parent is shown */
    if (!_rtgui_topwin_could_show(topwin)) {
        LOG_E("can't show");
        return -RT_ERROR;
    }

    _rtgui_topwin_preorder_map(topwin, _rtgui_topwin_mark_shown);
    return rtgui_topwin_activate_topwin(topwin);
}

static void _rtgui_topwin_clear_modal_tree(rtgui_topwin_t *topwin) {
    while (!IS_ROOT_WIN(topwin)) {
        rt_list_t *node;

        rt_list_foreach(node, &topwin->parent->child_list, next) {
            get_topwin_from_list(node)->flag &= ~WINTITLE_MODALED;
            get_topwin_from_list(node)->wid->flag &= ~RTGUI_WIN_FLAG_UNDER_MODAL;
            if (get_topwin_from_list(node)->flag & WINTITLE_MODALING) {
                goto _out;
            }
        }
        topwin = topwin->parent;
    }
_out:
    /* clear the modal flag of the root window */
    topwin->flag &= ~WINTITLE_MODALED;
    topwin->wid->flag &= ~RTGUI_WIN_FLAG_UNDER_MODAL;
}

/* hide a window */
rt_err_t rtgui_topwin_hide(struct rtgui_event_win *evt) {
    rtgui_topwin_t *topwin;
    rtgui_topwin_t *last_topwin;
    rtgui_win_t    *wid;
    rt_list_t *containing_list;

    if (!evt)
        return -RT_ERROR;

    wid = evt->wid;
    /* find in show list */
    topwin = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);
    if (topwin == RT_NULL)
    {
        return -RT_ERROR;
    }
    if (!(topwin->flag & WINTITLE_SHOWN))
    {
        return RT_EOK;
    }

    last_topwin = rtgui_topwin_get_focus();

    _rtgui_topwin_preorder_map(topwin, _rtgui_topwin_mark_hidden);

    if (topwin->parent == RT_NULL)
        containing_list = &_rtgui_topwin_list;
    else
        containing_list = &topwin->parent->child_list;

    rt_list_remove(&topwin->list);
    rt_list_insert_before(containing_list, &topwin->list);

    /* update clip info */
    rtgui_topwin_update_clip();

    if (topwin->flag & WINTITLE_MODALING)
    {
        topwin->flag &= ~WINTITLE_MODALING;
        _rtgui_topwin_clear_modal_tree(topwin);
    }

    if (last_topwin == topwin)
    {
        _rtgui_topwin_activate_next(topwin->flag);
    }

    topwin->flag &= ~WINTITLE_ACTIVATE;

    /* redraw the old rect */
    rtgui_topwin_redraw(&(topwin->extent));

    return RT_EOK;
}

/* move top window */
rt_err_t rtgui_topwin_move(rtgui_evt_generic_t *evt) {
    rt_err_t ret;

    do {
        rtgui_topwin_t *topwin;
        rt_int16_t dx, dy;
        rtgui_rect_t last_rect;
        rt_slist_t *node;

        /* find in show list */
        topwin = rtgui_topwin_search_in_list(
            evt->win_move.wid, &_rtgui_topwin_list);
        if (!topwin || !(topwin->flag & WINTITLE_SHOWN)) {
            LOG_E("can't show");
            ret = -RT_ERROR;
            break;
        }

        /* get the delta move x, y */
        dx = evt->win_move.x - topwin->extent.x1;
        dy = evt->win_move.y - topwin->extent.y1;
        last_rect = topwin->extent;
        /* move window rect */
        rtgui_rect_move(&(topwin->extent), dx, dy);

        /* move the monitor rect list */
        rt_slist_for_each(node, &(topwin->monitor_list)) {
            struct rtgui_mouse_monitor *monitor;
            monitor = rt_slist_entry(node, struct rtgui_mouse_monitor, list);
            rtgui_rect_move(&(monitor->rect), dx, dy);
        }

        /* update windows clip info */
        rtgui_topwin_update_clip();
        /* update last window coverage area */
        rtgui_topwin_redraw(&last_rect);

        if (!rtgui_rect_is_intersect(&last_rect, &(topwin->extent))) {
            /* last rect is not intersect with moved rect, re-paint window */
            rtgui_evt_generic_t *evt;

            /* send RTGUI_EVENT_PAINT */
            evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
            if (evt) {
                RTGUI_EVENT_INIT(evt, PAINT);
                evt->paint.wid = topwin->wid;
                ret = rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
                if (ret) {
                    LOG_E("paint %s err [%d]", topwin->wid->title, ret);
                    break;
                }
            } else {
                LOG_E("get mp err");
                ret = -RT_ENOMEM;
                break;
            }
        }
    } while (0);

    return ret;
}

/*
 * resize a top win
 * Note: currently, only support resize hidden window
 */
void rtgui_topwin_resize(rtgui_win_t *wid, rtgui_rect_t *rect)
{
    rtgui_topwin_t *topwin;
    struct rtgui_region region;

    /* find in show list */
    topwin = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);
    if (topwin == RT_NULL ||
            !(topwin->flag & WINTITLE_SHOWN))
        return;

    /* record the old rect */
    rtgui_region_init_with_extents(&region, &topwin->extent);
    /* union the new rect so this is the region we should redraw */
    rtgui_region_union_rect(&region, &region, rect);

    topwin->extent = *rect;

    /* update windows clip info */
    rtgui_topwin_update_clip();

    /* update old window coverage area */
    rtgui_topwin_redraw(rtgui_region_extents(&region));

    rtgui_region_fini(&region);
}

static rtgui_topwin_t *_rtgui_topwin_get_focus_from_list(rt_list_t *list)
{
    rt_list_t *node;

    RT_ASSERT(list != RT_NULL);

    rt_list_foreach(node, list, next)
    {
        rtgui_topwin_t *child = get_topwin_from_list(node);
        if (child->flag & WINTITLE_ACTIVATE)
            return child;

        child = _rtgui_topwin_get_focus_from_list(&child->child_list);
        if (child != RT_NULL)
            return child;
    }

    return RT_NULL;
}

rtgui_topwin_t *rtgui_topwin_get_focus(void)
{
    return _rtgui_topwin_get_focus_from_list(&_rtgui_topwin_list);
}

rtgui_app_t *rtgui_topwin_app_get_focus(void)
{
    rtgui_app_t *topwin_app = RT_NULL;
    rtgui_topwin_t *topwin = rtgui_topwin_get_focus();

    if (topwin)
    {
        topwin_app = topwin->app;
    }

    return topwin_app;
}

static rtgui_topwin_t *_rtgui_topwin_get_wnd_from_tree(rt_list_t *list,
        int x, int y,
        rt_bool_t exclude_modaled)
{
    rt_list_t *node;
    rtgui_topwin_t *topwin, *target;

    RT_ASSERT(list != RT_NULL);

    rt_list_foreach(node, list, next)
    {
        topwin = get_topwin_from_list(node);
        if (!(topwin->flag & WINTITLE_SHOWN))
            break;

        /* if higher window have this point, return it */
        target = _rtgui_topwin_get_wnd_from_tree(&topwin->child_list, x, y, exclude_modaled);
        if (target != RT_NULL)
            return target;

        if (exclude_modaled && (topwin->flag & WINTITLE_MODALED))
            break;

        if (rtgui_rect_contains_point(&(topwin->extent), x, y) == RT_EOK)
        {
            return topwin;
        }
    }

    return RT_NULL;
}

rtgui_topwin_t *rtgui_topwin_get_wnd(int x, int y)
{
    return _rtgui_topwin_get_wnd_from_tree(&_rtgui_topwin_list, x, y, RT_FALSE);
}

rtgui_topwin_t *rtgui_topwin_get_wnd_no_modaled(int x, int y)
{
    return _rtgui_topwin_get_wnd_from_tree(&_rtgui_topwin_list, x, y, RT_TRUE);
}

/* clip region from topwin, and the windows beneath it. */
rt_inline void _rtgui_topwin_clip_to_region(rtgui_topwin_t *topwin,
        struct rtgui_region *region)
{
    RT_ASSERT(region != RT_NULL);
    RT_ASSERT(topwin != RT_NULL);

    rtgui_region_reset(&topwin->wid->outer_clip, &topwin->wid->outer_extent);
    rtgui_region_intersect(&topwin->wid->outer_clip, &topwin->wid->outer_clip, region);
}

static void rtgui_topwin_update_clip(void) {
    rtgui_topwin_t *topwin;
    rtgui_evt_generic_t *evt;
    rt_err_t ret;
    /* Note that the region is a "female die", that means it's the region you
     * can paint to, not the region covered by others.
     */
    struct rtgui_region region_available;

    if (rt_list_isempty(&_rtgui_topwin_list) || \
        !(get_topwin_from_list(_rtgui_topwin_list.next)->flag & \
          WINTITLE_SHOWN)) {
        LOG_E("can't show");
        return;
    }

    rtgui_region_init_rect(
        &region_available, 0, 0,
        rtgui_graphic_driver_get_default()->width,
        rtgui_graphic_driver_get_default()->height);

    /* from top to bottom. */
    topwin = rtgui_topwin_get_topmost_window_shown(WINTITLE_ONTOP);
    /* 0 is normal layer */
    if (!topwin) {
        topwin = rtgui_topwin_get_topmost_window_shown(WINTITLE_INIT);
    }
    if (!topwin) {
        topwin = rtgui_topwin_get_topmost_window_shown(WINTITLE_ONBTM);
    }

    /* send RTGUI_EVENT_CLIP_INFO */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(
        rtgui_event_pool, RT_WAITING_FOREVER);
    if (!evt) {
        LOG_E("get mp err");
        return;
    }
    RTGUI_EVENT_INIT(evt, CLIP_INFO);

    while (topwin) {
        LOG_D("topwin %s (%s)", topwin->wid->title, topwin->wid->app->name);
        evt->clip_info.wid = topwin->wid;

        /* clip the topwin */
        _rtgui_topwin_clip_to_region(topwin, &region_available);
        /* update available region */
        rtgui_region_subtract_rect(
            &region_available, &region_available, &topwin->extent);

        ret = rtgui_send(topwin->app, evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("active %s err [%d]", topwin->wid->title, ret);
            return;
        }

        topwin = _rtgui_topwin_get_next_shown(topwin);
    }

    rtgui_region_fini(&region_available);
}

static void _rtgui_topwin_redraw_tree(rt_list_t *list, rtgui_rect_t *rect,
    rtgui_evt_generic_t *evt) {
    rt_list_t *node;

    RT_ASSERT(list != RT_NULL);
    RT_ASSERT(rect != RT_NULL);
    RT_ASSERT(evt != RT_NULL);

    /* skip the hidden windows */
    rt_list_foreach(node, list, prev) {
        if (get_topwin_from_list(node)->flag & WINTITLE_SHOWN) break;
    }

    for (; node != list; node = node->prev) {
        rtgui_topwin_t *topwin = get_topwin_from_list(node);

        //FIXME: intersect with clip?
        if (!rtgui_rect_is_intersect(rect, &(topwin->extent))) {
            evt->paint.wid = topwin->wid;
            // < XY do !!! >
            //rtgui_send(topwin->app, evt);
        }
        _rtgui_topwin_redraw_tree(&topwin->child_list, rect, evt);
    }
}

static void rtgui_topwin_redraw(rtgui_rect_t *rect) {
    rtgui_evt_generic_t *evt;

    /* send RTGUI_EVENT_PAINT */
    evt = rt_mp_alloc(rtgui_event_pool, RT_WAITING_FOREVER);
    if (evt) {
        RTGUI_EVENT_INIT(evt, PAINT);
        evt->paint.wid = RT_NULL;
        _rtgui_topwin_redraw_tree(&_rtgui_topwin_list, rect, evt);
        rt_mp_free(evt);
    } else {
        LOG_E("get mp err");
        return;
    }
}

/* a window enter modal mode will modal all the sibling window and parent
 * window all along to the root window. If a root window modals, there is
 * nothing to do here.*/
rt_err_t rtgui_topwin_modal_enter(struct rtgui_event_win_modal_enter *event) {
    rtgui_topwin_t *topwin, *parent_top;
    rt_list_t *node;

    topwin = rtgui_topwin_search_in_list(event->wid, &_rtgui_topwin_list);
    if (topwin == RT_NULL) return -RT_ERROR;
    if (IS_ROOT_WIN(topwin)) return RT_EOK;

    parent_top = topwin->parent;

    /* modal window should be on top already */
    RT_ASSERT(get_topwin_from_list(parent_top->child_list.next) == topwin);

    do {
        rt_list_foreach(node, &parent_top->child_list, next) {
            get_topwin_from_list(node)->flag |= WINTITLE_MODALED;
            get_topwin_from_list(node)->wid->flag |= RTGUI_WIN_FLAG_UNDER_MODAL;
        }

        parent_top->flag |= WINTITLE_MODALED;
        parent_top->wid->flag |= RTGUI_WIN_FLAG_UNDER_MODAL;
        parent_top = parent_top->parent;
    } while (parent_top);

    topwin->flag &= ~WINTITLE_MODALED;
    topwin->wid->flag &= ~RTGUI_WIN_FLAG_UNDER_MODAL;
    topwin->flag |= WINTITLE_MODALING;

    return RT_EOK;
}

void rtgui_topwin_append_monitor_rect(rtgui_win_t *wid,
    rtgui_rect_t *rect) {
    rtgui_topwin_t *win;

    /* parameters check */
    if (!wid || !rect) return;

    /* find topwin */
    win = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);
    if (!win) return;
    /* append rect to top window monitor rect list */
    rtgui_mouse_monitor_append(&(win->monitor_list), rect);
}

void rtgui_topwin_remove_monitor_rect(rtgui_win_t *wid,
    rtgui_rect_t *rect) {
    rtgui_topwin_t *win;

    /* parameters check */
    if (wid == RT_NULL || rect == RT_NULL)
        return;

    /* find topwin */
    win = rtgui_topwin_search_in_list(wid, &_rtgui_topwin_list);
    if (win == RT_NULL)
        return;

    /* remove rect from top window monitor rect list */
    rtgui_mouse_monitor_remove(&(win->monitor_list), rect);
}

static rtgui_obj_t* _get_obj_in_topwin(rtgui_topwin_t *topwin,
    rtgui_app_t *app, rt_uint32_t id) {
    rtgui_obj_t *object;
    rt_list_t *node;

    object = TO_OBJECT(topwin->wid);
    if (object->id == id) return object;

    object = rtgui_container_get_object(TO_CONTAINER(object), id);
    if (object) return object;

    rt_list_foreach(node, &topwin->child_list, next) {
        rtgui_topwin_t *topwin;

        topwin = get_topwin_from_list(node);
        if (topwin->app != app) continue;

        object = _get_obj_in_topwin(topwin, app, id);
        if (object) return object;
    }

    return RT_NULL;
}

// rtgui_obj_t* rtgui_get_object(rtgui_app_t *app, rt_uint32_t id) {
//     rtgui_obj_t *obj;
//     rt_list_t *node;

//     obj = TO_OBJECT(app);
//     if (obj->id == id) return obj;

//     rt_list_foreach(node, &_rtgui_topwin_list, next) {
//         rtgui_topwin_t *topwin;

//         topwin = get_topwin_from_list(node);
//         if (topwin->app != app) continue;

//         obj = _get_obj_in_topwin(topwin, app, id);
//         if (obj) return obj;
//     }
//     return RT_NULL;
// }
// RTM_EXPORT(rtgui_get_object);

// rtgui_obj_t* rtgui_get_self_object(rt_uint32_t id) {
//     return rtgui_get_object(rtgui_app_self(), id);
// }
// RTM_EXPORT(rtgui_get_self_object);

static void _rtgui_topwin_dump(rtgui_topwin_t *topwin) {
    rt_kprintf("0x%p:%s,0x%x,%c%c",
        topwin, topwin->wid->title, topwin->flag,
        topwin->flag & WINTITLE_SHOWN ? 'S' : 'H',
        topwin->flag & WINTITLE_MODALED ? 'm' :
        topwin->flag & WINTITLE_MODALING ? 'M' : ' ');
}

static void _rtgui_topwin_dump_tree(rtgui_topwin_t *topwin) {
    rt_list_t *node;

    _rtgui_topwin_dump(topwin);

    rt_kprintf("(");
    rt_list_foreach(node, &topwin->child_list, next) {
        _rtgui_topwin_dump_tree(get_topwin_from_list(node));
    }
    rt_kprintf(")");
}

static void rtgui_topwin_dump_tree(void) {
    rt_list_t *node;

    rt_list_foreach(node, &_rtgui_topwin_list, next) {
        _rtgui_topwin_dump_tree(get_topwin_from_list(node));
        rt_kprintf("\n");
    }
}


#ifdef RT_USING_FINSH
#include "components/finsh/finsh.h"

void dump_tree() {
    rtgui_topwin_dump_tree();
}
FINSH_FUNCTION_EXPORT(dump_tree, dump rtgui topwin tree)

#endif /* RT_USING_FINSH */
