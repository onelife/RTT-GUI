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
 */

#include "../include/dc.h"
#include "../include/rtgui_system.h"
#include "../include/rtgui_app.h"
#include "../include/widgets/container.h"
#include "../include/widgets/window.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_CNT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

static void _rtgui_container_constructor(rtgui_container_t *cntr) {
    /* init cntr */
    rtgui_object_set_event_handler(
        RTGUI_OBJECT(cntr), rtgui_container_event_handler);

    rtgui_list_init(&(cntr->children));
    cntr->layout_box = RT_NULL;

    RTGUI_WIDGET(cntr)->flag |= RTGUI_WIDGET_FLAG_FOCUSABLE;
}

static void _rtgui_container_destructor(rtgui_container_t *cntr) {
    rtgui_container_destroy_children(cntr);

    if (cntr->layout_box) {
        rtgui_object_destroy(RTGUI_OBJECT(cntr->layout_box));
    }
}

DEFINE_CLASS_TYPE(container, "container",
                  RTGUI_PARENT_TYPE(widget),
                  _rtgui_container_constructor,
                  _rtgui_container_destructor,
                  sizeof(struct rtgui_container));
RTM_EXPORT(_rtgui_container);

rt_bool_t rtgui_container_dispatch_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    rt_bool_t done;

    done = RT_FALSE;
    do {
        struct rtgui_list_node *node;
        rtgui_evt_type_t mask;

        if (!(cntr->parent.flag & RTGUI_WIDGET_FLAG_SHOWN)) {
            done = RT_TRUE;
            break;
        }

        mask = evt->base.type & RTGUI_EVENT_PAINT;
        rtgui_list_foreach(node, &(cntr->children)) {
            rtgui_widget_t *wgt = \
                rtgui_list_entry(node, struct rtgui_widget, sibling);

            if (mask && !EXTENTCHECK(&cntr->parent.extent, &wgt->extent)) {
            // if ((wgt->extent.x1 > _wgt->extent.x2) || \
            //     (wgt->extent.x2 < _wgt->extent.x1) || \
            //     (wgt->extent.y1 > _wgt->extent.y2) || \
            //     (wgt->extent.y2 < _wgt->extent.y1)) {
                evt->base.type &= ~mask;
            }

            if (RTGUI_OBJECT(wgt)->event_handler) {
                done = RTGUI_OBJECT(wgt)->event_handler(
                    RTGUI_OBJECT(wgt), evt);
                if (done) break;
            }
        }
    } while (0);

    return done;
}
RTM_EXPORT(rtgui_container_dispatch_event);

/* broadcast means that the return value of event handlers will be ignored. The
 * events will always reach every child.*/
rt_bool_t rtgui_container_broadcast_event(struct rtgui_container *cntr,
    union rtgui_evt_generic *evt) {
    struct rtgui_list_node *node;

    rtgui_list_foreach(node, &(cntr->children)) {
        struct rtgui_widget *wgt = \
            rtgui_list_entry(node, struct rtgui_widget, sibling);

        if (RTGUI_OBJECT(wgt)->event_handler) {
            return RTGUI_OBJECT(wgt)->event_handler(RTGUI_OBJECT(wgt), evt);
        }
    }

    return RT_FALSE;
}
RTM_EXPORT(rtgui_container_broadcast_event);

rt_bool_t rtgui_container_dispatch_mouse_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    struct rtgui_widget *last_focus;
    struct rtgui_list_node *node;
    rt_bool_t done;

    last_focus = RTGUI_WIDGET(cntr)->toplevel->focused_widget;
    done = RT_FALSE;

    rtgui_list_foreach(node, &(cntr->children)) {
        struct rtgui_widget *wgt = \
            rtgui_list_entry(node, struct rtgui_widget, sibling);

        if (!rtgui_rect_contains_point(
            &(wgt->extent), evt->mouse.x, evt->mouse.y)) {
            if ((last_focus != wgt) && RTGUI_WIDGET_IS_FOCUSABLE(wgt)) {
                rtgui_widget_focus(wgt);
            }
            if (RTGUI_OBJECT(wgt)->event_handler) {
                done = RTGUI_OBJECT(wgt)->event_handler(RTGUI_OBJECT(wgt), evt);
                if (done) break;
            }
        }
    }

    return done;
}
RTM_EXPORT(rtgui_container_dispatch_mouse_event);

rt_bool_t rtgui_container_event_handler(struct rtgui_obj *obj,
    union rtgui_evt_generic *evt) {
    struct rtgui_container *cntr;
    struct rtgui_widget *wgt;
    rt_bool_t done;

    if (!obj || !evt) return RT_FALSE;
    cntr = RTGUI_CONTAINER(obj);
    wgt = RTGUI_WIDGET(obj);
    done = RT_FALSE;

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
    {
        struct rtgui_dc *dc;
        struct rtgui_rect rect;

        dc = rtgui_dc_begin_drawing(wgt);
        if (!dc) {
            break;
        }
        rtgui_widget_get_rect(wgt, &rect);
        /* fill cntr with background */
        rtgui_dc_fill_rect(dc, &rect);
        /* paint on each child */
        done = rtgui_container_dispatch_event(cntr, evt);
        rtgui_dc_end_drawing(dc, 1);
    }
    break;

    case RTGUI_EVENT_KBD:
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        /* handle in child wgt */
        done = rtgui_container_dispatch_mouse_event(cntr, evt);
        break;

    case RTGUI_EVENT_SHOW:
        (void)rtgui_widget_onshow(obj, evt);
        done = rtgui_container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_HIDE:
        (void)rtgui_widget_onhide(obj, evt);
        done = rtgui_container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_COMMAND:
        done = rtgui_container_dispatch_event(cntr, evt);
        break;

    case RTGUI_EVENT_UPDATE_TOPLVL:
        /* call parent handler */
        (void)rtgui_widget_onupdate_toplvl(obj, evt);
        /* update the children */
        done = rtgui_container_broadcast_event(cntr, evt);
        break;

    case RTGUI_EVENT_RESIZE:
        /* re-layout cntr */
        rtgui_container_layout(cntr);
        done = RT_TRUE;
        break;

    default:
        /* call parent wgt event handler */
        done = rtgui_widget_event_handler(obj, evt);
        break;
    }

    if (done && evt) {
        LOG_D("free %p", evt);
        rt_mp_free(evt);
        evt = RT_NULL;
    }
    return done;
}
RTM_EXPORT(rtgui_container_event_handler);

rtgui_container_t *rtgui_container_create(void)
{
    struct rtgui_container *cntr;

    /* allocate cntr */
    cntr = (struct rtgui_container *) rtgui_widget_create(RTGUI_CONTAINER_TYPE);
    return cntr;
}
RTM_EXPORT(rtgui_container_create);

void rtgui_container_destroy(rtgui_container_t *cntr) {
    rtgui_widget_destroy(RTGUI_WIDGET(cntr));
}
RTM_EXPORT(rtgui_container_destroy);

/*
 * This function will add a child to a cntr wgt
 * Note: this function will not change the wgt layout
 * the layout is the responsibility of layout wgt, such as box.
 */
void rtgui_container_add_child(rtgui_container_t *cntr,
    rtgui_widget_t *child) {
    RT_ASSERT(cntr != RT_NULL);
    RT_ASSERT(child != RT_NULL);
    RT_ASSERT(child->parent == RT_NULL);

    if (child->parent == RTGUI_WIDGET(cntr)) return;

    /* set parent and toplevel wgt */
    child->parent = RTGUI_WIDGET(cntr);
    /* put wgt to parent's children list */
    rtgui_list_append(&(cntr->children), &(child->sibling));

    /* update children toplevel */
    if (RTGUI_WIDGET(cntr)->toplevel) {
        union rtgui_evt_generic *evt;
        struct rtgui_win *top;

        /* send RTGUI_EVENT_UPDATE_TOPLVL */
        evt = rtgui_malloc(sizeof(struct rtgui_event_update_toplvl));
        if (!evt) {
            LOG_E("evt mem err");
            return;
        }
        RTGUI_EVENT_UPDATE_TOPLVL_INIT(&evt->update_toplvl);
        evt->update_toplvl.toplvl = RTGUI_WIDGET(cntr)->toplevel;
        rtgui_object_handle(RTGUI_OBJECT(cntr), evt);
        rtgui_free(evt);

        /* update window clip */
        top = RTGUI_WIDGET(cntr)->toplevel;
        if ((top->flag & RTGUI_WIN_FLAG_CONNECTED) && \
            (RTGUI_WIDGET(top)->flag & RTGUI_WIDGET_FLAG_SHOWN)) {
            rtgui_win_update_clip(RTGUI_WIN(RTGUI_WIDGET(cntr)->toplevel));
        }
    }
}
RTM_EXPORT(rtgui_container_add_child);

/* remove a child to wgt */
void rtgui_container_remove_child(rtgui_container_t *cntr,
    rtgui_widget_t *child) {
    rtgui_widget_unfocus(child);

    /* remove wgt from parent's children list */
    rtgui_list_remove(&(cntr->children), &(child->sibling));
    /* set parent and toplevel wgt */
    child->parent = RT_NULL;
    child->toplevel = RT_NULL;

    /* update window clip */
    if (RTGUI_WIDGET(cntr)->toplevel) {
        rtgui_win_update_clip(RTGUI_WIN(RTGUI_WIDGET(cntr)->toplevel));
    }
}
RTM_EXPORT(rtgui_container_remove_child);

/* destroy all children of cntr */
void rtgui_container_destroy_children(rtgui_container_t *cntr)
{
    struct rtgui_list_node *node;

    if (cntr == RT_NULL)
        return;

    node = cntr->children.next;
    while (node != RT_NULL)
    {
        rtgui_widget_t *child = rtgui_list_entry(node, rtgui_widget_t, sibling);

        if (RTGUI_IS_CONTAINER(child))
        {
            /* break parent firstly */
            child->parent = RT_NULL;

            /* destroy children of child */
            rtgui_container_destroy_children(RTGUI_CONTAINER(child));
        }

        /* remove wgt from parent's children list */
        rtgui_list_remove(&(cntr->children), &(child->sibling));

        /* set parent and toplevel wgt */
        child->parent = RT_NULL;

        /* destroy object and remove from parent */
        rtgui_object_destroy(RTGUI_OBJECT(child));

        node = cntr->children.next;
    }

    cntr->children.next = RT_NULL;

    /* update wgt clip */
    rtgui_win_update_clip(RTGUI_WIN(RTGUI_WIDGET(cntr)->toplevel));
}
RTM_EXPORT(rtgui_container_destroy_children);

rtgui_widget_t *rtgui_container_get_first_child(rtgui_container_t *cntr)
{
    rtgui_widget_t *child = RT_NULL;

    RT_ASSERT(cntr != RT_NULL);

    if (cntr->children.next != RT_NULL)
    {
        child = rtgui_list_entry(cntr->children.next, rtgui_widget_t, sibling);
    }

    return child;
}
RTM_EXPORT(rtgui_container_get_first_child);

void rtgui_container_set_box(rtgui_container_t *cntr, struct rtgui_box *box)
{
    if (cntr == RT_NULL || box  == RT_NULL)
        return;

    cntr->layout_box = box;
    box->container = cntr;
}
RTM_EXPORT(rtgui_container_set_box);

void rtgui_container_layout(struct rtgui_container *cntr) {
    if (!cntr || !cntr->layout_box) return;
    rtgui_box_layout(cntr->layout_box);
}
RTM_EXPORT(rtgui_container_layout);

struct rtgui_obj* rtgui_container_get_object(struct rtgui_container *cntr,
                                                rt_uint32_t id)
{
    struct rtgui_list_node *node;

    rtgui_list_foreach(node, &(cntr->children))
    {
        struct rtgui_obj *o;
        o = RTGUI_OBJECT(rtgui_list_entry(node, struct rtgui_widget, sibling));

        if (o->id == id)
            return o;

        if (RTGUI_IS_CONTAINER(o))
        {
            struct rtgui_obj *obj;

            obj = rtgui_container_get_object(RTGUI_CONTAINER(o), id);
            if (obj)
                return obj;
        }
    }

    return RT_NULL;
}
RTM_EXPORT(rtgui_container_get_object);

