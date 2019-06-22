/*
 * File      : widget.c
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
 * 2010-06-26     Bernard      add user_data to widget structure
 * 2013-10-07     Bernard      remove the win_check in update_clip.
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/dc.h"
#include "include/widgets/widget.h"
#include "include/widgets/window.h"
#include "include/widgets/container.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_WGT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void _widget_constructor(void *obj);
static void _widget_destructor(void *obj);
static rt_bool_t _widget_event_handler(void *obj, rtgui_evt_generic_t *evt);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    widget,
    CLASS_METADATA(object),
    _widget_constructor,
    _widget_destructor,
    _widget_event_handler,
    sizeof(rtgui_widget_t));
RTM_EXPORT(_rtgui_widget);

/* Private functions ---------------------------------------------------------*/
static void _widget_constructor(void *obj) {
    rtgui_widget_t *wgt = obj;

    /* init widget*/
    wgt->parent = RT_NULL;
    wgt->toplevel = RT_NULL;
    rt_slist_init(&(wgt->sibling));

    wgt->flag = RTGUI_WIDGET_FLAG_SHOWN;
    wgt->align = RTGUI_ALIGN_LEFT | RTGUI_ALIGN_TOP;
    // border, border_style
    wgt->min_width = wgt->min_height = 0;
    rt_memset(&(wgt->extent), 0x0, sizeof(wgt->extent));
    rt_memset(&(wgt->extent_visiable), 0x0, sizeof(wgt->extent_visiable));
    rtgui_region_init_with_extent(&wgt->clip, &wgt->extent);

    wgt->on_focus = RT_NULL;
    wgt->on_unfocus  = RT_NULL;

    wgt->gc.foreground = default_foreground;
    wgt->gc.background = default_background;
    wgt->gc.textstyle = RTGUI_TEXTSTYLE_NORMAL;
    wgt->gc.textalign = RTGUI_ALIGN_LEFT | RTGUI_ALIGN_TOP;
    wgt->gc.font = rtgui_font_default();
    rtgui_dc_client_init(wgt);

    wgt->user_data = 0;
}

static void _widget_destructor(void *obj) {
    rtgui_widget_t *wgt = obj;

    /* remove from child list */
    if (wgt->parent && IS_CONTAINER(wgt->parent)) {
        rt_slist_remove(&(TO_CONTAINER(wgt->parent)->children),
            &(wgt->sibling));
        wgt->parent = RT_NULL;
    }

    /* uninit clip region */
    rtgui_region_uninit(&(wgt->clip));
}

static rt_bool_t _widget_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    rt_bool_t done = RT_FALSE;

    EVT_LOG("[WdgEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_SHOW:
        done = rtgui_widget_onshow(obj, evt);
        break;

    case RTGUI_EVENT_HIDE:
        done = rtgui_widget_onhide(obj, evt);
        break;

    case RTGUI_EVENT_UPDATE_TOPLVL:
        done = rtgui_widget_onupdate_toplvl(obj, evt->update_toplvl.toplvl);
        break;

    case RTGUI_EVENT_PAINT:
    default:
        if (SUPER_CLASS_HANDLER(widget))
            done = SUPER_CLASS_HANDLER(widget)(wgt, evt);
        break;
    }

    EVT_LOG("[WdgEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static void _widget_move(rtgui_widget_t* wgt, int dx, int dy) {
    rt_slist_t *node;
    rtgui_widget_t *parent, *child;

    rtgui_rect_move(&(wgt->extent), dx, dy);

    /* handle visiable extent */
    wgt->extent_visiable = wgt->extent;

    /* get the parent without transparent */
    parent = wgt->parent;
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    if (wgt->parent)
        rtgui_rect_intersect(&(wgt->parent->extent_visiable),
            &(wgt->extent_visiable));

    /* reset clip */
    rtgui_region_init_with_extent(&(wgt->clip), &(wgt->extent));

    /* move children */
    if (IS_CONTAINER(wgt)) {
        rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children)) {
            child = rt_slist_entry(node, rtgui_widget_t, sibling);
            _widget_move(child, dx, dy);
        }
    }
}

/* Public functions ----------------------------------------------------------*/
RTGUI_MEMBER_SETTER(rtgui_widget_t, widget, rtgui_widget_t*, parent);

rtgui_win_t *rtgui_widget_get_toplevel(rtgui_widget_t *wgt) {
    if (wgt->toplevel) return wgt->toplevel;

    LOG_W("toplevel not set");

    /* get the topmost */
    while (wgt->parent)
        wgt = wgt->parent;
    /* set toplevel */
    wgt->toplevel = TO_WIN(wgt);
    return wgt->toplevel;
}
RTM_EXPORT(rtgui_widget_get_toplevel);

void rtgui_widget_set_border(rtgui_widget_t *wgt, rt_uint32_t style) {
    RT_ASSERT(wgt != RT_NULL);
    wgt->border_style = style;

    switch (style) {
    case RTGUI_BORDER_NONE:
        wgt->border = 0;
        break;

    case RTGUI_BORDER_SIMPLE:
    case RTGUI_BORDER_UP:
    case RTGUI_BORDER_DOWN:
        wgt->border = 1;
        break;

    case RTGUI_BORDER_STATIC:
    case RTGUI_BORDER_RAISE:
    case RTGUI_BORDER_SUNKEN:
    case RTGUI_BORDER_BOX:
    case RTGUI_BORDER_EXTRA:
    default:
        wgt->border = 2;
        break;
    }
}
RTM_EXPORT(rtgui_widget_set_border);

RTGUI_MEMBER_SETTER(rtgui_widget_t, widget, int, min_width);

RTGUI_MEMBER_SETTER(rtgui_widget_t, widget, int, min_height);

void rtgui_widget_set_minsize(rtgui_widget_t *wgt, int width, int height) {
    RT_ASSERT(wgt != RT_NULL);
    WIDGET_SETTER(min_width)(wgt, width);
    WIDGET_SETTER(min_height)(wgt, height);
}
RTM_EXPORT(rtgui_widget_set_minsize);

RTGUI_MEMBER_GETTER(rtgui_widget_t, widget, rtgui_rect_t, extent);

void rtgui_widget_set_rect(rtgui_widget_t *wgt, const rtgui_rect_t *rect) {
    if (!wgt || !rect) return;

    /* set extent and extent_visiable */
    wgt->extent = *rect;
    wgt->extent_visiable = *rect;
    if (IS_CONTAINER(wgt))
        rtgui_container_layout(TO_CONTAINER(wgt));

    /* reset min_width and min_height */
    wgt->min_width  = RECT_W(wgt->extent);
    wgt->min_height = RECT_H(wgt->extent);

    /* reset clip */
    if (rtgui_region_not_empty(&(wgt->clip)))
        rtgui_region_uninit(&(wgt->clip));
    rtgui_region_init_with_extent(&(wgt->clip), rect);
    if (wgt->parent && wgt->toplevel) {
        if (wgt->parent == TO_WIDGET(wgt->toplevel))
            rtgui_win_update_clip(wgt->toplevel);
        else
            rtgui_widget_update_clip(wgt->parent);
    }

    /* move to logic position if container */
    if (IS_CONTAINER(wgt)) {
        int delta_x = rect->x1 - wgt->extent.x1;
        int delta_y = rect->y1 - wgt->extent.y1;
        rtgui_widget_move_to_logic(wgt, delta_x, delta_y);
    }
}
RTM_EXPORT(rtgui_widget_set_rect);

void rtgui_widget_set_rectangle(rtgui_widget_t *wgt, int x, int y,
    int width, int height) {
    rtgui_rect_t rect = {
        .x1 = x,
        .y1 = y,
        .x2 = x + width,
        .y2 = y + height,
    };
    rtgui_widget_set_rect(wgt, &rect);
}
RTM_EXPORT(rtgui_widget_set_rectangle);

void rtgui_widget_get_rect(rtgui_widget_t *wgt, rtgui_rect_t *rect) {
    RT_ASSERT(wgt != RT_NULL);
    if (rect) {
        rect->x1 = rect->y1 = 0;
        rect->x2 = wgt->extent.x2 - wgt->extent.x1;
        rect->y2 = wgt->extent.y2 - wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_get_rect);

RTGUI_MEMBER_SETTER(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_focus);

RTGUI_MEMBER_SETTER(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_unfocus);

rtgui_widget_t *rtgui_widget_get_next_sibling(rtgui_widget_t *wgt) {
    rtgui_widget_t *sibling;

    sibling = RT_NULL;
    if (wgt->sibling.next)
        sibling = rt_slist_entry(wgt->sibling.next, rtgui_widget_t, sibling);

    return sibling;
}
RTM_EXPORT(rtgui_widget_get_next_sibling);

rtgui_widget_t *rtgui_widget_get_prev_sibling(rtgui_widget_t *wgt) {
    rt_slist_t *node;

    if (!wgt->parent) return RT_NULL;

    rt_slist_for_each(node, &(TO_CONTAINER(wgt->parent)->children))
        if (node->next == &(wgt->sibling)) break;

    if (node) return rt_slist_entry(node, rtgui_widget_t, sibling);
    return RT_NULL;
}
RTM_EXPORT(rtgui_widget_get_prev_sibling);

rtgui_color_t rtgui_widget_get_parent_foreground(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent;

    parent = wgt->parent;
    if (!parent) return WIDGET_FOREGROUND(wgt);

    /* get root */
    while (parent->parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    return WIDGET_FOREGROUND(parent);
}
RTM_EXPORT(rtgui_widget_get_parent_foreground);

rtgui_color_t rtgui_widget_get_parent_background(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent;

    parent = wgt->parent;
    if (!parent) return WIDGET_BACKGROUND(wgt);

    /* get root */
    while (parent->parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    return WIDGET_BACKGROUND(parent);
}
RTM_EXPORT(rtgui_widget_get_parent_background);

void rtgui_widget_clip_parent(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent = wgt->parent;
    /* get the parent without transparent */
    while (parent) {
        if (!IS_WIDGET_FLAG(parent, TRANSPARENT)) break;
        parent = parent->parent;
    }
    /* clip the widget extern from parent */
    if (parent)
        rtgui_region_subtract(&(parent->clip), &(wgt->clip));
}
RTM_EXPORT(rtgui_widget_clip_parent);

void rtgui_widget_clip_return(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent = wgt->parent;
    /* get the parent without transparent */
    while (parent) {
        if (!IS_WIDGET_FLAG(parent, TRANSPARENT)) break;
        parent = parent->parent;
    }
    /* give clip back to parent */
    if (parent)
        rtgui_region_union(&(parent->clip), &(wgt->clip));
}
RTM_EXPORT(rtgui_widget_clip_return);

/*
 * This function moves widget and its children to a logic point
 */
void rtgui_widget_move_to_logic(rtgui_widget_t *wgt, int dx, int dy) {
    rtgui_rect_t rect;
    rtgui_widget_t *parent;

    if (wgt == RT_NULL)
        return;

    /* give clip of this widget back to parent */
    parent = wgt->parent;
    /* get the parent rect, even if it's a transparent parent. */
    if (parent)
        rect = parent->extent_visiable;

    /* we should find out the none-transparent parent */
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;
    if (parent) {
        /* reset clip info */
        rtgui_region_init_with_extent(&(wgt->clip), &(wgt->extent));
        rtgui_region_intersect_rect(&(wgt->clip), &rect);

        /* give back the extent */
        rtgui_region_union(&(parent->clip), &(wgt->clip));
    }

    /* move this widget (and its children if it's a container) to destination 
       point */
    _widget_move(wgt, dx, dy);
    /* update this widget */
    rtgui_widget_update_clip(wgt);
}
RTM_EXPORT(rtgui_widget_move_to_logic);

void rtgui_widget_point_to_device(rtgui_widget_t *wgt, rtgui_point_t *pt) {
    RT_ASSERT(wgt != RT_NULL);
    if (pt) {
        pt->x += wgt->extent.x1;
        pt->y += wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_point_to_device);

void rtgui_widget_rect_to_device(rtgui_widget_t *wgt, rtgui_rect_t *rect) {
    RT_ASSERT(wgt != RT_NULL);
    if (rect) {
        rect->x1 += wgt->extent.x1;
        rect->x2 += wgt->extent.x1;
        rect->y1 += wgt->extent.y1;
        rect->y2 += wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_rect_to_device);

void rtgui_widget_point_to_logic(rtgui_widget_t *wgt, rtgui_point_t *pt) {
    RT_ASSERT(wgt != RT_NULL);

    if (pt) {
        pt->x -= wgt->extent.x1;
        pt->y -= wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_point_to_logic);

void rtgui_widget_rect_to_logic(rtgui_widget_t *wgt, rtgui_rect_t *rect) {
    RT_ASSERT(wgt != RT_NULL);
    if (rect) {
        rect->x1 -= wgt->extent.x1;
        rect->x2 -= wgt->extent.x1;
        rect->y1 -= wgt->extent.y1;
        rect->y2 -= wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_rect_to_logic);

void rtgui_widget_focus(rtgui_widget_t *wgt) {
    /* A focused widget will receive keyboard events */
    RT_ASSERT(wgt != RT_NULL);
    /* no effect if no toplevel */
    RT_ASSERT(wgt->toplevel != RT_NULL);

    if (!IS_WIDGET_FLAG(wgt, FOCUSABLE) || IS_WIDGET_FLAG(wgt, DISABLE)) return;
    if (wgt->toplevel->focused == wgt) return;

    if (wgt->toplevel->focused)
        rtgui_widget_unfocus(wgt->toplevel->focused);
    WIDGET_FLAG_SET(wgt, FOCUS);
    wgt->toplevel->focused = wgt;

    if (wgt->on_focus)
        wgt->on_focus(TO_OBJECT(wgt), RT_NULL);

    LOG_D("focus %s", wgt->_super.cls->name);
}
RTM_EXPORT(rtgui_widget_focus);

void rtgui_widget_unfocus(rtgui_widget_t *wgt) {
    rt_slist_t *node;

    if (!wgt->toplevel || !IS_WIDGET_FLAG(wgt, FOCUS)) return;

    if (wgt->on_unfocus)
        wgt->on_unfocus(TO_OBJECT(wgt), RT_NULL);
    WIDGET_FLAG_CLEAR(wgt, FOCUS);
    wgt->toplevel->focused = RT_NULL;

    LOG_D("unfocus %s", wgt->_super.cls->name);
    if (!IS_CONTAINER(wgt)) return;

    rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);
        if (!IS_WIDGET_FLAG(child, SHOWN)) continue;
        rtgui_widget_unfocus(child);
    }
}
RTM_EXPORT(rtgui_widget_unfocus);

/*
 * This function updates the clip info of widget
 */
void rtgui_widget_update_clip(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent;
    rt_slist_t *node;

    if (!wgt || !IS_WIDGET_FLAG(wgt, SHOWN) || !wgt->parent || \
        rtgui_widget_is_in_animation(wgt))
        return;

    parent = wgt->parent;

    /* update extent_visiable */
    wgt->extent_visiable = wgt->extent;
    rtgui_rect_intersect(&(parent->extent_visiable), &(wgt->extent_visiable));

    /* update clip, limit widget extent in parent extent */
    rtgui_region_reset(&(wgt->clip), &(wgt->extent));
    rtgui_region_intersect_rect(&(wgt->clip), &(parent->extent_visiable));

    /* get the parent without transparent */
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    if (parent) {
        /* return clip to parent */
        rtgui_region_union(&(parent->clip), &(wgt->clip));
        /* subtract widget clip in parent clip */
        if (!IS_WIDGET_FLAG(wgt, TRANSPARENT) && IS_CONTAINER(parent))
            rtgui_region_subtract_rect(&(parent->clip), &(wgt->extent_visiable));
    }

    /*
     * note: since the layout widget introduction, the sibling widget should 
       not intersect.
     */

    /* update children's clip */
    if (IS_CONTAINER(wgt)) {
        rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children)) {
            rtgui_widget_t *child = \
                rt_slist_entry(node, rtgui_widget_t, sibling);
            rtgui_widget_update_clip(child);
        }
    }
}
RTM_EXPORT(rtgui_widget_update_clip);

void rtgui_widget_show(rtgui_widget_t *wgt) {
    do {
        rtgui_evt_generic_t *evt;

        if (!wgt) break;
        if (IS_WIDGET_FLAG(wgt, SHOWN)) break;
        WIDGET_FLAG_SET(wgt, SHOWN);
        LOG_D("unhide %s", wgt->_super.cls->name);
        if (!wgt->toplevel) break;
        rtgui_widget_update_clip(wgt);
        if (!EVENT_HANDLER(wgt)) break;

        /* send RTGUI_EVENT_SHOW */
        RTGUI_CREATE_EVENT(evt, SHOW, RT_WAITING_FOREVER);
        if (!evt) break;
        (void)EVENT_HANDLER(wgt)(wgt, evt);
        RTGUI_FREE_EVENT(evt);
    } while (0);
}
RTM_EXPORT(rtgui_widget_show);

void rtgui_widget_hide(rtgui_widget_t *wgt) {
    do {
        rtgui_evt_generic_t *evt;

        if (!wgt) break;
        if (!IS_WIDGET_FLAG(wgt, SHOWN)) break;
        if (!wgt->toplevel) break;
        if (!EVENT_HANDLER(wgt)) break;

        /* send RTGUI_EVENT_HIDE */
        RTGUI_CREATE_EVENT(evt, HIDE, RT_WAITING_FOREVER);
        if (!evt) break;
        (void)EVENT_HANDLER(wgt)(wgt, evt);
        RTGUI_FREE_EVENT(evt);

        WIDGET_FLAG_CLEAR(wgt, SHOWN);
        LOG_D("hide %s", wgt->_super.cls->name);
    } while (0);
}
RTM_EXPORT(rtgui_widget_hide);

void rtgui_widget_update(rtgui_widget_t *wgt) {
    do {
        rtgui_evt_generic_t *evt;

        if (!wgt) break;
        if (!EVENT_HANDLER(wgt) || IS_WIDGET_FLAG(wgt, IN_ANIM)) break;

        /* send RTGUI_EVENT_PAINT */
        RTGUI_CREATE_EVENT(evt, PAINT, RT_WAITING_FOREVER);
        if (!evt) break;
        LOG_I("update");
        evt->paint.wid = RT_NULL;
        (void)EVENT_HANDLER(wgt)(wgt, evt);
        RTGUI_FREE_EVENT(evt);
    } while (0);
}
RTM_EXPORT(rtgui_widget_update);

rt_bool_t rtgui_widget_onshow(rtgui_obj_t *obj, void *param) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    (void)param;

    if (!IS_WIDGET_FLAG(wgt, SHOWN)) return RT_FALSE;
    if (wgt->parent && !IS_WIDGET_FLAG(wgt, TRANSPARENT))
        rtgui_widget_clip_parent(wgt);
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onshow);

rt_bool_t rtgui_widget_onhide(rtgui_obj_t *obj, void *param) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    (void)param;

    if (!IS_WIDGET_FLAG(wgt, SHOWN)) return RT_FALSE;
    if (wgt->parent)
        rtgui_widget_clip_return(wgt);
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onhide);

rt_bool_t rtgui_widget_onupdate_toplvl(rtgui_obj_t *obj, void *param) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    rtgui_win_t *win = param;

    wgt->toplevel = win;
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onupdate_toplvl);

rt_bool_t rtgui_widget_is_in_animation(rtgui_widget_t *wgt) {
    while (wgt) {
        if (IS_WIDGET_FLAG(wgt, IN_ANIM)) return RT_TRUE;
        wgt = wgt->parent;
    }
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_is_in_animation);

#ifdef RTGUI_WIDGET_DEBUG
#include <rtgui/widgets/label.h>
#include <rtgui/widgets/button.h>

void rtgui_widget_dump(rtgui_widget_t *wgt)
{
    rtgui_obj_t *obj;

    obj = TO_OBJECT(wgt);
    rt_kprintf("wgt cls: %s ", obj->cls->name);

    if (IS_WIN(wgt) == RT_TRUE)
        rt_kprintf(":%s ", TO_WIN(wgt)->title);
    else if ((IS_LABEL(wgt) == RT_TRUE) || (IS_BUTTON(wgt) == RT_TRUE))
        rt_kprintf(":%s ", TO_LABEL(wgt)->text);

    rt_kprintf("extent(%d, %d) - (%d, %d)\n", wgt->extent.x1,
               wgt->extent.y1, wgt->extent.x2, wgt->extent.y2);
    // rtgui_region_dump(&(wgt->clip));
}
#endif

