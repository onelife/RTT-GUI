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
#include "../include/rtgui.h"
#include "../include/dc.h"
#include "../include/widgets/widget.h"
#include "../include/widgets/window.h"
#include "../include/widgets/container.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_WGT"
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
/* Public functions ----------------------------------------------------------*/
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
    rtgui_region_init_with_extents(&wgt->clip, &wgt->extent);

    wgt->on_focus_in   = RT_NULL;
    wgt->on_focus_out  = RT_NULL;

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
        done = rtgui_widget_onupdate_toplvl(obj, evt);
        break;

    case RTGUI_EVENT_PAINT:
    default:
        if (SUPER_HANDLER(wgt)) {
            done = SUPER_HANDLER(wgt)(wgt, evt);
        }
        break;
    }

    EVT_LOG("[WdgEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

void rtgui_widget_set_rect(rtgui_widget_t *wgt, const rtgui_rect_t *rect) {
    if (!wgt || !rect) return;

    /* update extent rectangle */
    wgt->extent = *rect;
    /* set the visiable extern as extern */
    wgt->extent_visiable = *rect;
    if (IS_CONTAINER(wgt)) {
        /* re-do layout */
        rtgui_container_layout(TO_CONTAINER(wgt));
    }

    /* reset min width and height */
    wgt->min_width  = RECT_W(wgt->extent);
    wgt->min_height = RECT_H(wgt->extent);

    /* it's not empty, fini it */
    if (rtgui_region_not_empty(&(wgt->clip))) {
        rtgui_region_uninit(&(wgt->clip));
    }

    /* reset clip info */
    rtgui_region_init_with_extents(&(wgt->clip), rect);
    if (wgt->parent && wgt->toplevel) {
        if ((void*)wgt->parent == (void*)wgt->toplevel) {
            rtgui_win_update_clip(wgt->toplevel);
        } else {
            /* update widget clip */
            rtgui_widget_update_clip(wgt->parent);
        }
    }

    /* move to a logic position if it's a container widget */
    if (IS_CONTAINER(wgt)) {
        int delta_x, delta_y;

        delta_x = rect->x1 - wgt->extent.x1;
        delta_y = rect->y1 - wgt->extent.y1;

        rtgui_widget_move_to_logic(wgt, delta_x, delta_y);
    }
}
RTM_EXPORT(rtgui_widget_set_rect);

void rtgui_widget_set_rectangle(rtgui_widget_t *wgt, int x, int y, int width, int height)
{
    rtgui_rect_t rect;

    rect.x1 = x;
    rect.y1 = y;
    rect.x2 = x + width;
    rect.y2 = y + height;

    rtgui_widget_set_rect(wgt, &rect);
}
RTM_EXPORT(rtgui_widget_set_rectangle);

void rtgui_widget_set_parent(rtgui_widget_t *wgt, rtgui_widget_t *parent)
{
    /* set parent and toplevel widget */
    wgt->parent = parent;
}
RTM_EXPORT(rtgui_widget_set_parent);

void rtgui_widget_get_extent(rtgui_widget_t *wgt, rtgui_rect_t *rect)
{
    RT_ASSERT(wgt != RT_NULL);
    RT_ASSERT(rect != RT_NULL);

    *rect = wgt->extent;
}
RTM_EXPORT(rtgui_widget_get_extent);

void rtgui_widget_set_minsize(rtgui_widget_t *wgt, int width, int height)
{
    RT_ASSERT(wgt != RT_NULL);
    wgt->min_width = width;
    wgt->min_height = height;
}
RTM_EXPORT(rtgui_widget_set_minsize);

void rtgui_widget_set_minwidth(rtgui_widget_t *wgt, int width)
{
    RT_ASSERT(wgt != RT_NULL);

    wgt->min_width = width;
}
RTM_EXPORT(rtgui_widget_set_minwidth);

void rtgui_widget_set_minheight(rtgui_widget_t *wgt, int height)
{
    RT_ASSERT(wgt != RT_NULL);

    wgt->min_height = height;
}
RTM_EXPORT(rtgui_widget_set_minheight);

static void _widget_move(rtgui_widget_t* wgt, int dx, int dy) {
    rt_slist_t *node;
    rtgui_widget_t *child, *parent;

    rtgui_rect_move(&(wgt->extent), dx, dy);

    /* handle visiable extent */
    wgt->extent_visiable = wgt->extent;
    parent = wgt->parent;
    /* we should find out the none-transparent parent */
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;
    if (wgt->parent)
        rtgui_rect_intersect(&(wgt->parent->extent_visiable), &(wgt->extent_visiable));

    /* reset clip info */
    rtgui_region_init_with_extents(&(wgt->clip), &(wgt->extent));

    /* move each child */
    if (IS_CONTAINER(wgt))
    {
        rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children))
        {
            child = rt_slist_entry(node, rtgui_widget_t, sibling);

            _widget_move(child, dx, dy);
        }
    }
}

/*
 * This function moves widget and its children to a logic point
 */
void rtgui_widget_move_to_logic(rtgui_widget_t *wgt, int dx, int dy)
{
    rtgui_rect_t rect;
    rtgui_widget_t *parent;

    if (wgt == RT_NULL)
        return;

    /* give clip of this widget back to parent */
    parent = wgt->parent;
    if (parent != RT_NULL)
    {
        /* get the parent rect, even if it's a transparent parent. */
        rect = parent->extent_visiable;
    }

    /* we should find out the none-transparent parent */
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;
    if (parent) {
        /* reset clip info */
        rtgui_region_init_with_extents(&(wgt->clip), &(wgt->extent));
        rtgui_region_intersect_rect(&(wgt->clip), &(wgt->clip), &rect);

        /* give back the extent */
        rtgui_region_union(&(parent->clip), &(parent->clip), &(wgt->clip));
    }

    /* move this widget (and its children if it's a container) to destination point */
    _widget_move(wgt, dx, dy);
    /* update this widget */
    rtgui_widget_update_clip(wgt);
}
RTM_EXPORT(rtgui_widget_move_to_logic);

void rtgui_widget_get_rect(rtgui_widget_t *wgt, rtgui_rect_t *rect)
{
    RT_ASSERT(wgt != RT_NULL);

    if (rect != RT_NULL)
    {
        rect->x1 = rect->y1 = 0;
        rect->x2 = wgt->extent.x2 - wgt->extent.x1;
        rect->y2 = wgt->extent.y2 - wgt->extent.y1;
    }
}
RTM_EXPORT(rtgui_widget_get_rect);

/**
 * set widget draw style
 */
void rtgui_widget_set_border(rtgui_widget_t *wgt, rt_uint32_t style)
{
    RT_ASSERT(wgt != RT_NULL);

    wgt->border_style = style;
    switch (style)
    {
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
        wgt->border = 2;
        break;
    default:
        wgt->border = 2;
        break;
    }
}
RTM_EXPORT(rtgui_widget_set_border);

void rtgui_widget_set_onfocus(rtgui_widget_t *wgt, rtgui_evt_hdl_t handler)
{
    RT_ASSERT(wgt != RT_NULL);

    wgt->on_focus_in = handler;
}
RTM_EXPORT(rtgui_widget_set_onfocus);

void rtgui_widget_set_onunfocus(rtgui_widget_t *wgt, rtgui_evt_hdl_t handler)
{
    RT_ASSERT(wgt != RT_NULL);

    wgt->on_focus_out = handler;
}
RTM_EXPORT(rtgui_widget_set_onunfocus);

/**
 * @brief Focuses the widget. The focused widget is the widget which can receive the keyboard events
 * @param wgt a widget
 * @note The widget has to be attached to a toplevel widget, otherwise it will have no effect
 */
void rtgui_widget_focus(rtgui_widget_t *wgt) {
    rtgui_widget_t *old_focus;

    RT_ASSERT(wgt != RT_NULL);
    RT_ASSERT(wgt->toplevel != RT_NULL);
    if (!IS_WIDGET_FLAG(wgt, FOCUSABLE) || IS_WIDGET_FLAG(wgt, DISABLE))
        return;

    old_focus = TO_WIN(wgt->toplevel)->focused;
    if (old_focus == wgt) return; /* it's the same focused widget */

    /* unfocused the old widget */
    if (RT_NULL != old_focus) {
        rtgui_widget_unfocus(old_focus);
    }

    WIDGET_FLAG_SET(wgt, FOCUS);
    TO_WIN(wgt->toplevel)->focused = wgt;

    /* invoke on focus in call back */
    if (RT_NULL != wgt->on_focus_in) {
        wgt->on_focus_in(TO_OBJECT(wgt), RT_NULL);
    }
    LOG_D("focus %s", wgt->_super.cls->name);
}
RTM_EXPORT(rtgui_widget_focus);

/**
 * @brief Unfocused the widget
 * @param wgt a widget
 */
void rtgui_widget_unfocus(rtgui_widget_t *wgt) {
    rt_slist_t *node;

    if (!wgt->toplevel || !IS_WIDGET_FLAG(wgt, FOCUS)) return;

    WIDGET_FLAG_CLEAR(wgt, FOCUS);
    if (wgt->on_focus_out)
        wgt->on_focus_out(TO_OBJECT(wgt), RT_NULL);
    TO_WIN(wgt->toplevel)->focused = RT_NULL;

    LOG_D("unfocus %d", wgt->_super.id);
    /* Ergodic constituent widget, make child loss of focus */
    if (!IS_CONTAINER(wgt)) return;

    rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);
        if (!IS_WIDGET_FLAG(child, SHOWN)) continue;

        rtgui_widget_unfocus(child);
    }
}
RTM_EXPORT(rtgui_widget_unfocus);

void rtgui_widget_point_to_device(rtgui_widget_t *wgt, rtgui_point_t *point) {
    RT_ASSERT(wgt != RT_NULL);

    if (point) {
        point->x += wgt->extent.x1;
        point->y += wgt->extent.y1;
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

void rtgui_widget_point_to_logic(rtgui_widget_t *wgt, rtgui_point_t *point) {
    RT_ASSERT(wgt != RT_NULL);

    if (point) {
        point->x -= wgt->extent.x1;
        point->y -= wgt->extent.y1;
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

rtgui_win_t *rtgui_widget_get_toplevel(rtgui_widget_t *_wgt) {
    rtgui_widget_t *wgt = _wgt;

    RT_ASSERT(wgt != RT_NULL);

    if (wgt->toplevel)
        return wgt->toplevel;

    LOG_W("wgt->toplevel not properly set");
    /* get the toplevel widget */
    while (wgt->parent)
        wgt = wgt->parent;
    /* set toplevel */
    wgt->toplevel = TO_WIN(wgt);

    return TO_WIN(wgt);
}
RTM_EXPORT(rtgui_widget_get_toplevel);

rt_bool_t rtgui_widget_onupdate_toplvl(rtgui_obj_t *obj,
    rtgui_evt_generic_t *evt) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);

    wgt->toplevel = evt->update_toplvl.toplvl;
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onupdate_toplvl);

/*
 * This function updates the clip info of widget
 */
void rtgui_widget_update_clip(rtgui_widget_t *wgt) {
    rtgui_rect_t rect;
    rt_slist_t *node;
    rtgui_widget_t *parent;

    /* no widget or widget is hide, no update clip */
    if (!wgt || !IS_WIDGET_FLAG(wgt, SHOWN) || !wgt->parent || \
        rtgui_widget_is_in_animation(wgt))
        return;

    parent = wgt->parent;
    /* reset visiable extent */
    wgt->extent_visiable = wgt->extent;
    rtgui_rect_intersect(&(parent->extent_visiable), &(wgt->extent_visiable));

    rect = parent->extent_visiable;
    /* reset clip to extent */
    rtgui_region_reset(&(wgt->clip), &(wgt->extent));
    /* limit widget extent in parent extent */
    rtgui_region_intersect_rect(&(wgt->clip), &(wgt->clip), &rect);

    /* get the no transparent parent */
    while (parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;
    if (parent) {
        /* give my clip back to parent */
        rtgui_region_union(&(parent->clip), &(parent->clip), &(wgt->clip));

        /* subtract widget clip in parent clip */
        if (!IS_WIDGET_FLAG(wgt, TRANSPARENT) && IS_CONTAINER(parent))
            rtgui_region_subtract_rect(&(parent->clip), &(parent->clip),
                &(wgt->extent_visiable));
    }

    /*
     * note: since the layout widget introduction, the sibling widget should not intersect.
     */

    /* if it's a container object, update the clip info of children */
    if (IS_CONTAINER(wgt))
    {
        rtgui_widget_t *child;
        rt_slist_for_each(node, &(TO_CONTAINER(wgt)->children))
        {
            child = rt_slist_entry(node, rtgui_widget_t, sibling);

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

rt_bool_t rtgui_widget_onshow(rtgui_obj_t *obj,
    rtgui_evt_generic_t *evt) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    (void)evt;

    if (!IS_WIDGET_FLAG(wgt, SHOWN))
        return RT_FALSE;
    if (!IS_WIDGET_FLAG(wgt, TRANSPARENT)) {
        rtgui_widget_clip_parent(wgt);
        return RT_TRUE;
    }
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onshow);

rt_bool_t rtgui_widget_onhide(rtgui_obj_t *obj,
    rtgui_evt_generic_t *evt) {
    rtgui_widget_t *wgt = TO_WIDGET(obj);
    (void)evt;

    if (!IS_WIDGET_FLAG(wgt, SHOWN))
        return RT_FALSE;
    rtgui_widget_clip_return(wgt);
    return RT_FALSE;
}
RTM_EXPORT(rtgui_widget_onhide);

rtgui_color_t rtgui_widget_get_parent_foreground(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent;

    /* get parent widget */
    parent = wgt->parent;
    if (parent == RT_NULL)
        return WIDGET_GET_FOREGROUND(wgt);

    while (parent->parent && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    /* get parent's color */
    return WIDGET_GET_FOREGROUND(parent);
}
RTM_EXPORT(rtgui_widget_get_parent_foreground);

rtgui_color_t rtgui_widget_get_parent_background(rtgui_widget_t *wgt)
{
    rtgui_widget_t *parent;

    /* get parent widget */
    parent = wgt->parent;
    if (parent == RT_NULL)
        return WIDGET_GET_BACKGROUND(wgt);

    while (parent->parent != RT_NULL && IS_WIDGET_FLAG(parent, TRANSPARENT))
        parent = parent->parent;

    /* get parent's color */
    return WIDGET_GET_BACKGROUND(parent);
}
RTM_EXPORT(rtgui_widget_get_parent_background);

void rtgui_widget_clip_parent(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent = wgt->parent;

    /* get the no transparent parent */
    while (parent) {
        if (!IS_WIDGET_FLAG(parent, TRANSPARENT)) break;
        parent = parent->parent;
    }
    /* clip the widget extern from parent */
    if (parent) {
        rtgui_region_subtract(&(parent->clip), &(parent->clip), &(wgt->clip));
    }
}
RTM_EXPORT(rtgui_widget_clip_parent);

void rtgui_widget_clip_return(rtgui_widget_t *wgt) {
    rtgui_widget_t *parent = wgt->parent;

    /* get the no transparent parent */
    while (parent) {
        if (!IS_WIDGET_FLAG(parent, TRANSPARENT)) break;
        parent = parent->parent;
    }
    /* give clip back to parent */
    if (parent) {
        rtgui_region_union(&(parent->clip), &(parent->clip), &(wgt->clip));
    }
}
RTM_EXPORT(rtgui_widget_clip_return);

void rtgui_widget_update(rtgui_widget_t *wgt) {
    do {
        rtgui_evt_generic_t *evt;

        if (!wgt) return;
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

rtgui_widget_t *rtgui_widget_get_next_sibling(rtgui_widget_t *wgt)
{
    rtgui_widget_t *sibling = RT_NULL;

    if (wgt->sibling.next != RT_NULL)
    {
        sibling = rt_slist_entry(wgt->sibling.next, rtgui_widget_t, sibling);
    }

    return sibling;
}
RTM_EXPORT(rtgui_widget_get_next_sibling);

rtgui_widget_t *rtgui_widget_get_prev_sibling(rtgui_widget_t *wgt)
{
    rt_slist_t *node;
    rtgui_widget_t *sibling, *parent;

    node = RT_NULL;
    sibling = RT_NULL;
    parent = wgt->parent;
    if (parent != RT_NULL)
    {
        rt_slist_for_each(node, &(TO_CONTAINER(parent)->children))
        {
            if (node->next == &(wgt->sibling))
                break;
        }
    }

    if (node != RT_NULL)
        sibling = rt_slist_entry(node, rtgui_widget_t, sibling);

    return sibling;
}
RTM_EXPORT(rtgui_widget_get_prev_sibling);

rt_bool_t rtgui_widget_is_in_animation(rtgui_widget_t *wgt) {
    /* check the visible of widget */
    while (wgt != RT_NULL) {
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
    else if ((RTGUI_IS_LABEL(wgt) == RT_TRUE) || (RTGUI_IS_BUTTON(wgt) == RT_TRUE))
        rt_kprintf(":%s ", RTGUI_LABEL(wgt)->text);

    rt_kprintf("extent(%d, %d) - (%d, %d)\n", wgt->extent.x1,
               wgt->extent.y1, wgt->extent.x2, wgt->extent.y2);
    // rtgui_region_dump(&(wgt->clip));
}
#endif

