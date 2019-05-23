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

#include "../include/rtgui.h"
#include "../include/dc.h"
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


static void _container_constructor(void *obj);
static void _container_destructor(void *obj);
static rt_bool_t _container_event_handler(void *obj,
    rtgui_evt_generic_t *evt);
static rt_bool_t rtgui_container_dispatch_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt);


RTGUI_CLASS(
    container,
    CLASS_METADATA(widget),
    _container_constructor,
    _container_destructor,
    _container_event_handler,
    sizeof(struct rtgui_container));
RTM_EXPORT(_rtgui_container);


static void _container_constructor(void *obj) {
    rtgui_container_t *cntr = obj;

    rt_slist_init(&(cntr->children));
    cntr->layout_box = RT_NULL;

    TO_WIDGET(cntr)->flag |= RTGUI_WIDGET_FLAG_FOCUSABLE;
}

static void _container_destructor(void *obj) {
    rtgui_container_t *cntr = obj;
    rtgui_container_destroy_children(cntr);

    if (cntr->layout_box) {
        DELETE_INSTANCE(cntr->layout_box);
    }
}

static rt_bool_t rtgui_container_dispatch_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    rt_bool_t done;

    done = RT_FALSE;
    do {
        rt_slist_t *node;
        rtgui_evt_type_t mask;

        if (!(cntr->_super.flag & RTGUI_WIDGET_FLAG_SHOWN)) {
            done = RT_TRUE;
            break;
        }

        mask = evt->base.type & RTGUI_EVENT_PAINT;
        rt_slist_for_each(node, &(cntr->children)) {
            rtgui_widget_t *wgt = \
                rt_slist_entry(node, struct rtgui_widget, sibling);

            if (mask && !EXTENTCHECK(&cntr->_super.extent, &wgt->extent)) {
            // if ((wgt->extent.x1 > _wgt->extent.x2) || \
            //     (wgt->extent.x2 < _wgt->extent.x1) || \
            //     (wgt->extent.y1 > _wgt->extent.y2) || \
            //     (wgt->extent.y2 < _wgt->extent.y1)) {
                evt->base.type &= ~mask;
            }

            if (EVENT_HANDLER(wgt)) {
                done = EVENT_HANDLER(wgt)(wgt, evt);
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
    rtgui_evt_generic_t *evt) {
    rt_slist_t *node;

    rt_slist_for_each(node, &(cntr->children)) {
        struct rtgui_widget *wgt = \
            rt_slist_entry(node, struct rtgui_widget, sibling);

        if (EVENT_HANDLER(wgt)) {
            return EVENT_HANDLER(wgt)(wgt, evt);
        }
    }

    return RT_FALSE;
}
RTM_EXPORT(rtgui_container_broadcast_event);

rt_bool_t rtgui_container_dispatch_mouse_event(rtgui_container_t *cntr,
    rtgui_evt_generic_t *evt) {
    struct rtgui_widget *last_focus;
    rt_slist_t *node;
    rt_bool_t done;

    last_focus = TO_WIDGET(cntr)->toplevel->focused_widget;
    done = RT_FALSE;

    rt_slist_for_each(node, &(cntr->children)) {
        struct rtgui_widget *wgt = \
            rt_slist_entry(node, struct rtgui_widget, sibling);

        if (!rtgui_rect_contains_point(
            &(wgt->extent), evt->mouse.x, evt->mouse.y)) {
            if ((last_focus != wgt) && RTGUI_WIDGET_IS_FOCUSABLE(wgt)) {
                rtgui_widget_focus(wgt);
            }
            if (EVENT_HANDLER(wgt)) {
                done =EVENT_HANDLER(wgt)(wgt, evt);
                if (done) break;
            }
        }
    }

    return done;
}
RTM_EXPORT(rtgui_container_dispatch_mouse_event);

static rt_bool_t _container_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    struct rtgui_container *cntr;
    struct rtgui_widget *wgt;
    rt_bool_t done;

    if (!obj || !evt) return RT_FALSE;
    cntr = TO_CONTAINER(obj);
    wgt = TO_WIDGET(obj);
    done = RT_FALSE;

    LOG_I("cntr rx %x (%p) from %s", evt->base.type, evt, evt->base.sender->mb->parent.parent.name);
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
        /* call _super handler */
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
        if (SUPER_HANDLER(cntr)) {
            done = SUPER_HANDLER(cntr)(cntr, evt);
        }
        break;
    }

    LOG_D("cntr done %d", done);
    return done;
}

void rtgui_container_destroy(rtgui_container_t *cntr) {
    rtgui_widget_destroy(TO_WIDGET(cntr));
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

    if (child->parent == TO_WIDGET(cntr)) return;

    /* set parent and toplevel wgt */
    child->parent = TO_WIDGET(cntr);
    /* put wgt to parent's children list */
    rt_slist_append(&(cntr->children), &(child->sibling));

    /* update children toplevel */
    if (TO_WIDGET(cntr)->toplevel) {
        rtgui_win_t *top;

        if (EVENT_HANDLER(cntr)) {
            rtgui_evt_generic_t *evt;

            /* send RTGUI_EVENT_UPDATE_TOPLVL */
            evt = (rtgui_evt_generic_t *)rt_mp_alloc(
                rtgui_event_pool, RT_WAITING_FOREVER);
            if (evt) {
                RTGUI_EVENT_UPDATE_TOPLVL_INIT(&evt->update_toplvl);
                evt->update_toplvl.toplvl = TO_WIDGET(cntr)->toplevel;
                (void)EVENT_HANDLER(cntr)(cntr, evt);
                rt_mp_free(evt);
            } else {
                LOG_E("get mp err");
                return;
            }
        }

        /* update window clip */
        top = TO_WIDGET(cntr)->toplevel;
        if ((top->flag & RTGUI_WIN_FLAG_CONNECTED) && \
            (TO_WIDGET(top)->flag & RTGUI_WIDGET_FLAG_SHOWN)) {
            rtgui_win_update_clip(TO_WIN(TO_WIDGET(cntr)->toplevel));
        }
    }
}
RTM_EXPORT(rtgui_container_add_child);

/* remove a child to wgt */
void rtgui_container_remove_child(rtgui_container_t *cntr,
    rtgui_widget_t *child) {
    rtgui_widget_unfocus(child);

    /* remove wgt from parent's children list */
    rt_slist_remove(&(cntr->children), &(child->sibling));
    /* set parent and toplevel wgt */
    child->parent = RT_NULL;
    child->toplevel = RT_NULL;

    /* update window clip */
    if (TO_WIDGET(cntr)->toplevel) {
        rtgui_win_update_clip(TO_WIN(TO_WIDGET(cntr)->toplevel));
    }
}
RTM_EXPORT(rtgui_container_remove_child);

/* destroy all children of cntr */
void rtgui_container_destroy_children(rtgui_container_t *cntr)
{
    rt_slist_t *node;

    if (cntr == RT_NULL)
        return;

    node = cntr->children.next;
    while (node != RT_NULL)
    {
        rtgui_widget_t *child = rt_slist_entry(node, rtgui_widget_t, sibling);

        if (IS_CONTAINER(child))
        {
            /* break parent firstly */
            child->parent = RT_NULL;

            /* destroy children of child */
            rtgui_container_destroy_children(TO_CONTAINER(child));
        }

        /* remove wgt from parent's children list */
        rt_slist_remove(&(cntr->children), &(child->sibling));

        /* set parent and toplevel wgt */
        child->parent = RT_NULL;

        /* destroy object and remove from parent */
        DELETE_INSTANCE(child);

        node = cntr->children.next;
    }

    cntr->children.next = RT_NULL;

    /* update wgt clip */
    rtgui_win_update_clip(TO_WIN(TO_WIDGET(cntr)->toplevel));
}
RTM_EXPORT(rtgui_container_destroy_children);

rtgui_widget_t *rtgui_container_get_first_child(rtgui_container_t *cntr)
{
    rtgui_widget_t *child = RT_NULL;

    RT_ASSERT(cntr != RT_NULL);

    if (cntr->children.next != RT_NULL)
    {
        child = rt_slist_entry(cntr->children.next, rtgui_widget_t, sibling);
    }

    return child;
}
RTM_EXPORT(rtgui_container_get_first_child);

void rtgui_container_set_box(rtgui_container_t *cntr, rtgui_box_t *box)
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

rtgui_obj_t* rtgui_container_get_object(struct rtgui_container *cntr,
                                                rt_uint32_t id)
{
    rt_slist_t *node;

    rt_slist_for_each(node, &(cntr->children))
    {
        rtgui_obj_t *o;
        o = TO_OBJECT(rt_slist_entry(node, struct rtgui_widget, sibling));

        if (o->id == id)
            return o;

        if (IS_CONTAINER(o))
        {
            rtgui_obj_t *obj;

            obj = rtgui_container_get_object(TO_CONTAINER(o), id);
            if (obj)
                return obj;
        }
    }

    return RT_NULL;
}
RTM_EXPORT(rtgui_container_get_object);

