/*
 * File      : box.c
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
 * 2019-05-18     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/widgets/container.h"
#include "include/widgets/box.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_BOX"
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
static void _box_constructor(void *obj);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    box,
    CLASS_METADATA(object),
    _box_constructor,
    RT_NULL,
    RT_NULL,
    sizeof(rtgui_box_t));

/* Private functions ---------------------------------------------------------*/
static void _box_constructor(void *obj) {
    rtgui_box_t *box = obj;

    /* set proper of control */
    box->orient = RTGUI_HORIZONTAL;
    box->border_sz = WIDGET_DEFAULT_BORDER;
    box->container = RT_NULL;
}

static void rtgui_box_layout_vertical(rtgui_box_t *box,
    rtgui_rect_t *extent) {
    rt_slist_t *node;
    rt_uint32_t stretch_cnt;
    rt_int32_t total_size, space, box_size;
    rt_int32_t next_x, next_y;

    /* find spaces */
    stretch_cnt = 0;
    total_size = box->border_sz;
    LOG_D("layout_vertical");

    rt_slist_for_each(node, &(box->container->children)) {
        rtgui_widget_t *wgt = rt_slist_entry(node, rtgui_widget_t, sibling);

        if (!IS_WIDGET_FLAG(wgt, SHOWN)) continue;
        if (IS_WIDGET_ALIGN(wgt, STRETCH))
            stretch_cnt++;
        else
            total_size += wgt->min_height;
        total_size += box->border_sz;
    }
    // LOG_W("stretch_cnt %d", stretch_cnt);
    // LOG_W("total_size %d", total_size);
    // LOG_W("extent %d %d %d %d", extent->x1, extent->y1, extent->x2, extent->y2);

    /* calculate the height for each spaces */
    if (stretch_cnt && (RECT_H(*extent) > total_size))
        space = (RECT_H(*extent) - total_size) / stretch_cnt;
    else
        space = 0;


    /* init (x, y) and box width */
    next_x = extent->x1 + box->border_sz;
    next_y = extent->y1 + box->border_sz;
    box_size = RECT_W(*extent) - box->border_sz * 2;
    // LOG_W("next_x %d", next_x);
    // LOG_W("next_y %d", next_y);
    // LOG_W("box_size %d", box_size);

    /* layout each widget */
    rt_slist_for_each(node, &(box->container->children)) {
        rtgui_widget_t *wgt = rt_slist_entry(node, rtgui_widget_t, sibling);
        rtgui_rect_t *rect;
        rtgui_evt_generic_t *evt;

        if (!IS_WIDGET_FLAG(wgt, SHOWN)) continue;

        /* get extent of widget */
        rect = &(wgt->extent);
        /* reset rect */
        rtgui_rect_init(rect, 0, 0, wgt->min_width, wgt->min_height);
        /* left in default */
        rtgui_rect_move(rect, next_x, next_y);

        /* expand on horizontal */
        if (IS_WIDGET_ALIGN(wgt, EXPAND))
            rect->x2 = rect->x1 + (rt_int16_t)box_size - 1;
        /* center */
        if (IS_WIDGET_ALIGN(wgt, CENTER_VERTICAL)) {
            rt_uint32_t mid;

            mid = box_size - RECT_W(*rect);
            mid = mid / 2;

            rect->x1 = next_x + mid;
            rect->x2 = next_x + box_size - mid - 1;
        } else if (IS_WIDGET_ALIGN(wgt, RIGHT)) {
            /* right */
            rect->x1 = next_x + box_size - RECT_W(*rect);
            rect->x2 = next_x + box_size - 1;
        }

        if (IS_WIDGET_ALIGN(wgt, STRETCH))
            rect->y2 = rect->y1 + space - 1;

        LOG_D("rect (%d,%d)-(%d,%d)", rect->x1, rect->y1, rect->x2, rect->y2);
        /* send RTGUI_EVENT_RESIZE */
        RTGUI_CREATE_EVENT(evt, RESIZE, RT_WAITING_FOREVER);
        if (evt) {
            /* process resize event */
            evt->resize.x = rect->x1;
            evt->resize.y = rect->y1;
            evt->resize.w = RECT_W(*rect);
            evt->resize.h = RECT_H(*rect);
            (void)EVENT_HANDLER(wgt)(wgt, evt);
            RTGUI_FREE_EVENT(evt);
        } else {
            LOG_W("V RESIZE evt err");
            continue;
        }

        /* point to next height */
        next_y = rect->y2 + box->border_sz;

        if (IS_CONTAINER(wgt)) {
            rtgui_container_t *cntr = TO_CONTAINER(wgt);

            if (cntr->layout_box)
                rtgui_box_layout(cntr->layout_box);
        }
    }
}

static void rtgui_box_layout_horizontal(rtgui_box_t *box,
    rtgui_rect_t *extent) {
    rt_slist_t *node;
    rt_uint32_t stretch_cnt;
    rt_int32_t total_size, space, box_size;
    rt_int32_t next_x, next_y;

    /* find spaces */
    stretch_cnt = 0;
    total_size = box->border_sz;
    LOG_D("layout_horizontal");

    rt_slist_for_each(node, &(box->container->children)) {
        rtgui_widget_t *wgt = rt_slist_entry(node, rtgui_widget_t, sibling);

        if (!IS_WIDGET_FLAG(wgt, SHOWN)) continue;
        if (IS_WIDGET_ALIGN(wgt, STRETCH))
            stretch_cnt++;
        else
            total_size += wgt->min_width;
        total_size += box->border_sz;
    }

    /* calculate the width for each spaces */
    if (stretch_cnt)
        space = (RECT_W(*extent) - total_size) / stretch_cnt;
    else
        space = 0;
    /* init (x, y) and box height */
    next_x = extent->x1 + box->border_sz;
    next_y = extent->y1 + box->border_sz;
    box_size = RECT_H(*extent) - box->border_sz * 2;

    /* layout each widget */
    rt_slist_for_each(node, &(box->container->children)) {
        rtgui_widget_t *wgt = rt_slist_entry(node, rtgui_widget_t, sibling);
        rtgui_evt_generic_t *evt;
        rtgui_rect_t *rect;

        if (!IS_WIDGET_FLAG(wgt, SHOWN)) continue;

        /* get extent of widget */
        rect = &(wgt->extent);
        /* reset rect */
        rtgui_rect_init(rect, 0, 0, wgt->min_width, wgt->min_height);
        /* top in default */
        rtgui_rect_move(rect, next_x, next_y);

        /* expand on vertical */
        if (IS_WIDGET_ALIGN(wgt, EXPAND))
            rect->y2 = rect->y1 + (rt_int16_t)box_size - 1;
        /* center */
        if (IS_WIDGET_ALIGN(wgt, CENTER_HORIZONTAL)) {
            rt_uint32_t mid;

            mid = box_size - RECT_H(*rect);
            mid = mid / 2;

            rect->y1 = next_y + mid;
            rect->y2 = next_y + box_size - mid - 1;
        } else if (IS_WIDGET_ALIGN(wgt, RIGHT)) {
            /* right */
            rect->y1 = next_y + box_size - RECT_H(*rect);
            rect->y2 = next_y + box_size - 1;
        }

        if (IS_WIDGET_ALIGN(wgt, STRETCH))
            rect->x2 = rect->x1 + space - 1;

        LOG_D("rect (%d,%d)-(%d,%d)", rect->x1, rect->y1, rect->x2, rect->y2);
        /* send RTGUI_EVENT_RESIZE */
        RTGUI_CREATE_EVENT(evt, RESIZE, RT_WAITING_FOREVER);
        if (evt) {
            /* process resize event */
            evt->resize.x = rect->x1;
            evt->resize.y = rect->y1;
            evt->resize.w = RECT_W(*rect);
            evt->resize.h = RECT_H(*rect);
            (void)EVENT_HANDLER(wgt)(wgt, evt);
            RTGUI_FREE_EVENT(evt);
        } else {
            LOG_W("H RESIZE evt err");
            continue;
        }

        /* point to next width */
        next_x = rect->x2 + box->border_sz;

        if (IS_CONTAINER(wgt)) {
            rtgui_container_t *cntr = TO_CONTAINER(wgt);

            if (cntr->layout_box)
                rtgui_box_layout(cntr->layout_box);
        }
    }
}

/* Public functions ----------------------------------------------------------*/
rtgui_box_t *rtgui_create_box(rtgui_container_t *cntr, rt_uint16_t orient,
    rt_uint16_t size) {
    rtgui_box_t *box;

    do {
        box = CREATE_INSTANCE(box, RT_NULL);
        if (!box) break;
        box->orient = orient;
        box->border_sz = size;
        rtgui_container_set_box(TO_CONTAINER(cntr), box);
    } while (0);

    return box;
}

void rtgui_box_layout(rtgui_box_t *box) {
    rtgui_rect_t extent;
    RT_ASSERT(box != RT_NULL);

    if (box->container == RT_NULL) return;

    extent = WIDGET_GETTER(extent)(TO_WIDGET(box->container));
    LOG_D("box (%d,%d)-(%d,%d)", extent.x1, extent.y1, extent.x2, extent.y2);

    if (box->orient & RTGUI_VERTICAL)
        rtgui_box_layout_vertical(box, &extent);
    else
        rtgui_box_layout_horizontal(box, &extent);

    /* update box and its children clip */
    if (IS_WIDGET_FLAG(box->container, SHOWN))
        rtgui_widget_update_clip(TO_WIDGET(box->container));
}
RTM_EXPORT(rtgui_box_layout);

void rtgui_box_layout_rect(rtgui_box_t *box, rtgui_rect_t *rect) {
    RT_ASSERT(box != RT_NULL);

    if (box->container == RT_NULL) return;

    if (box->orient & RTGUI_VERTICAL)
        rtgui_box_layout_vertical(box, rect);
    else
        rtgui_box_layout_horizontal(box, rect);

    /* update box and its children clip */
    if (IS_WIDGET_FLAG(box->container, SHOWN))
        rtgui_widget_update_clip(TO_WIDGET(box->container));
}
RTM_EXPORT(rtgui_box_layout_rect);

