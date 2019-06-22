/*
 * File      : cntr.c
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
 * 2010-09-24     Bernard      fix cntr destroy issue
 * 2019-06-15     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/dc.h"
#include "include/widgets/container.h"
#include "include/widgets/window.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_CNT"
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
static void _container_constructor(void *obj);
static void _container_destructor(void *obj);
static rt_bool_t _container_event_handler(void *obj,
    rtgui_evt_generic_t *evt);
static rt_bool_t _container_dispatch_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    container,
    CLASS_METADATA(widget),
    _container_constructor,
    _container_destructor,
    _container_event_handler,
    sizeof(rtgui_container_t));
RTM_EXPORT(_rtgui_container);

/* Private functions ---------------------------------------------------------*/
static void _container_constructor(void *obj) {
    rtgui_container_t *cntr = obj;

    /* set super fields */
    WIDGET_FLAG_SET(obj, FOCUSABLE);

    cntr->layout_box = RT_NULL;
    rt_slist_init(&(cntr->children));
}

static void _container_destruct_children(rtgui_container_t *cntr) {
    rt_slist_t *node = cntr->children.next;

    while (node != RT_NULL) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);

        /* unset parent */
        child->parent = RT_NULL;

        if (IS_CONTAINER(child))
            _container_destruct_children(TO_CONTAINER(child));

        /* remove from child list */
        rt_slist_remove(&(cntr->children), &(child->sibling));
        DELETE_INSTANCE(child);

        node = cntr->children.next;
    }
    cntr->children.next = RT_NULL;

    /* update clip region */
    rtgui_win_update_clip(TO_WIN(TO_WIDGET(cntr)->toplevel));
}

static void _container_destructor(void *obj) {
    rtgui_container_t *cntr = obj;

    _container_destruct_children(cntr);

    if (cntr->layout_box)
        DELETE_INSTANCE(cntr->layout_box);
}

static rt_bool_t _container_dispatch_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    rt_slist_t *node;
    rt_bool_t is_paint, done;

    /* check if not shown */
    if (!IS_WIDGET_FLAG(cntr, SHOWN))
        return RT_TRUE;

    is_paint = IS_EVENT_TYPE(evt, PAINT);
    done = RT_FALSE;

    /* call children's event handler, return once handled */
    rt_slist_for_each(node, &(cntr->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);
        /* check if intercepted for paint event */
        if (is_paint && \
            !IS_R_INTERSECT(&(TO_WIDGET(cntr)->extent), &child->extent))
            continue;
        if (EVENT_HANDLER(child)) {
            done = EVENT_HANDLER(child)(child, evt);
            if (done) break;
        }
    }

    return done;
}

static rt_bool_t _container_broadcast_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    rt_slist_t *node;

    /* call every child's event handler and ignore return value */
    rt_slist_for_each(node, &(cntr->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);
        if (EVENT_HANDLER(child))
            (void)EVENT_HANDLER(child)(child, evt);
    }
    return RT_FALSE;
}

rt_bool_t rtgui_container_dispatch_mouse_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    rtgui_widget_t *focus;
    rt_slist_t *node;
    rt_bool_t done;

    focus = TO_WIDGET(cntr)->toplevel->focused;
    done = RT_FALSE;
    LOG_D("cntr mouse");

    /* call children's event handler, return once handled */
    rt_slist_for_each(node, &(cntr->children)) {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);
        /* check if hit the child */
        if (!rtgui_rect_contains_point(&(child->extent), evt->mouse.x,
            evt->mouse.y))
            continue;
        if ((focus != child) && IS_WIDGET_FLAG(child, FOCUSABLE))
            rtgui_widget_focus(child);
        if (EVENT_HANDLER(child)) {
            done = EVENT_HANDLER(child)(child, evt);
            if (done) break;
        }
    }
    return done;
}
RTM_EXPORT(rtgui_container_dispatch_mouse_event);

static rt_bool_t _container_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_container_t *cntr;
    rtgui_widget_t *wgt;
    rt_bool_t done;

    EVT_LOG("[CntrEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    cntr = TO_CONTAINER(obj);
    wgt = TO_WIDGET(obj);
    done = RT_FALSE;

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
    {
        rtgui_dc_t *dc;
        rtgui_rect_t rect;

        dc = rtgui_dc_begin_drawing(wgt);
        if (!dc) break;
        rtgui_widget_get_rect(wgt, &rect);
        /* fill cntr with background */
        rtgui_dc_fill_rect(dc, &rect);
        /* inform children but not mark done in order to give chance to sibling
           widgets */
        (void)_container_dispatch_event(cntr, evt);
        rtgui_dc_end_drawing(dc, RT_TRUE);
    }
    break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        /* inform children mouse event */
        done = rtgui_container_dispatch_mouse_event(cntr, evt);
        break;

    case RTGUI_EVENT_KBD:
        break;

    case RTGUI_EVENT_SHOW:
        (void)rtgui_widget_onshow(obj, RT_NULL);
        done = _container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_HIDE:
        (void)rtgui_widget_onhide(obj, RT_NULL);
        done = _container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_COMMAND:
        done = _container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_UPDATE_TOPLVL:
        /* call super handler */
        (void)rtgui_widget_onupdate_toplvl(obj, evt->update_toplvl.toplvl);
        /* inform children */
        done = _container_broadcast_event(cntr, evt);
        break;

    case RTGUI_EVENT_RESIZE:
        /* re-layout cntr */
        rtgui_container_layout(cntr);
        done = RT_TRUE;
        break;

    default:
        if (SUPER_CLASS_HANDLER(container))
            done = SUPER_CLASS_HANDLER(container)(cntr, evt);
        break;
    }

    EVT_LOG("[CntrEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

/* Public functions ----------------------------------------------------------*/
void rtgui_container_add_child(rtgui_container_t *cntr, rtgui_widget_t *child) {
    rtgui_widget_t *parent;

    RT_ASSERT(child->parent == RT_NULL);
    parent = TO_WIDGET(cntr);
    if (child->parent == parent) return;

    /* add to children list, no layout */
    rt_slist_append(&(cntr->children), &(child->sibling));
    child->parent = parent;

    if (!parent->toplevel) return;

    /* update children's toplevel */
    // TODO(onelife): why send to all children?
    if (EVENT_HANDLER(cntr)) {
        rtgui_evt_generic_t *evt;

        /* send RTGUI_EVENT_UPDATE_TOPLVL */
        RTGUI_CREATE_EVENT(evt, UPDATE_TOPLVL, RT_WAITING_FOREVER);
        if (!evt) return;
        evt->update_toplvl.toplvl = parent->toplevel;
        (void)EVENT_HANDLER(cntr)(cntr, evt);
        RTGUI_FREE_EVENT(evt);
    }

    /* update window clip */
    if (IS_WIN_FLAG(parent->toplevel, CONNECTED) && \
        IS_WIDGET_FLAG(parent->toplevel, SHOWN))
        rtgui_win_update_clip(parent->toplevel);
}
RTM_EXPORT(rtgui_container_add_child);

void rtgui_container_remove_child(rtgui_container_t *cntr,
    rtgui_widget_t *child) {
    rtgui_widget_unfocus(child);
    /* remove from children list */
    rt_slist_remove(&(cntr->children), &(child->sibling));
    child->parent = RT_NULL;
    child->toplevel = RT_NULL;

    /* update window clip */
    if (TO_WIDGET(cntr)->toplevel)
        rtgui_win_update_clip(TO_WIDGET(cntr)->toplevel);
}
RTM_EXPORT(rtgui_container_remove_child);

rtgui_widget_t *rtgui_container_get_first_child(rtgui_container_t *cntr) {
    if (cntr->children.next)
        return rt_slist_entry(cntr->children.next, rtgui_widget_t, sibling);
    return RT_NULL;
}
RTM_EXPORT(rtgui_container_get_first_child);

void rtgui_container_set_box(rtgui_container_t *cntr, rtgui_box_t *box) {
    if (!cntr || !box) return;
    cntr->layout_box = box;
    box->container = cntr;
}
RTM_EXPORT(rtgui_container_set_box);

void rtgui_container_layout(rtgui_container_t *cntr) {
    if (!cntr || !cntr->layout_box) return;
    rtgui_box_layout(cntr->layout_box);
}
RTM_EXPORT(rtgui_container_layout);

rtgui_obj_t* rtgui_container_get_object(rtgui_container_t *cntr, rt_uint32_t id) {
    rt_slist_t *node;

    rt_slist_for_each(node, &(cntr->children)) {
        rtgui_widget_t *wgt = rt_slist_entry(node, rtgui_widget_t, sibling);
        if (TO_OBJECT(wgt)->id == id) return TO_OBJECT(wgt);

        if (IS_CONTAINER(wgt)) {
            rtgui_obj_t *obj = \
                rtgui_container_get_object(TO_CONTAINER(wgt), id);
            if (obj) return obj;
        }
    }
    return RT_NULL;
}
RTM_EXPORT(rtgui_container_get_object);

