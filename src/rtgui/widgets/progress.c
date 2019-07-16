/*
 * File      : progress.c
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
 * 2019-07-02     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/widgets/container.h"
#include "include/widgets/progress.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_PGB"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
static void _progress_constructor(void *obj);
static rt_bool_t _progress_event_handler(void *obj, rtgui_evt_generic_t *evt);
static void _theme_draw_progress(rtgui_progress_t *bar);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    progress,
    CLASS_METADATA(widget),
    _progress_constructor,
    RT_NULL,
    _progress_event_handler,
    sizeof(rtgui_progress_t));

/* Private functions ---------------------------------------------------------*/
static void _progress_constructor(void *obj) {
    rtgui_progress_t *bar = obj;
    rtgui_rect_t rect = {
        0, 0, PROGRESS_WIDTH_DEFAULT, PROGRESS_HEIGHT_DEFAULT
    };

    rtgui_widget_set_rect(TO_WIDGET(bar), &rect);
    WIDGET_TEXTALIGN(bar) = RTGUI_ALIGN_CENTER_HORIZONTAL | \
                            RTGUI_ALIGN_CENTER_VERTICAL;
    bar->orient = RTGUI_HORIZONTAL;
    bar->range = PROGRESS_RANGE_DEFAULT;
    bar->value = 0;
}

static rt_bool_t _progress_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_progress_t *bar = obj;
    rt_bool_t done = RT_FALSE;

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        _theme_draw_progress(bar);
        break;

    default:
        if (SUPER_CLASS_HANDLER(progress))
            done = SUPER_CLASS_HANDLER(progress)(obj, evt);
        break;
    }

    return done;
}

static void _theme_draw_progress(rtgui_progress_t *bar) {
    rtgui_dc_t *dc;

    dc = rtgui_dc_begin_drawing(TO_WIDGET(bar));
    if (!dc) {
        LOG_E("no dc");
        return;
    }

    do {
        rtgui_rect_t rect;
        rtgui_color_t bc;
        rt_uint16_t val;

        rtgui_widget_get_rect(TO_WIDGET(bar), &rect);
        LOG_W("draw progress (%d,%d)-(%d, %d)", rect.x1, rect.y1, rect.x2,
            rect.y2);

        bc = WIDGET_BACKGROUND(bar);
        WIDGET_BACKGROUND(bar) = RTGUI_RGB(212, 208, 200);
        rtgui_dc_draw_border(dc, &rect, RTGUI_BORDER_SUNKEN);
        WIDGET_BACKGROUND(bar) = bc;

        if (!bar->range) break;

        rect.x2++;
        rect.y2++;
        rtgui_rect_inflate(&rect, -2);
        rect.x2--;
        rect.y2--;

        if (RTGUI_VERTICAL == bar->orient) {
            /* vertical bar grows from bottom to top */
            val = (rt_uint16_t)((rt_uint32_t)RECT_H(rect) * \
                (bar->range - bar->value) / bar->range);

            rect.y1 += val;
            WIDGET_BACKGROUND(bar) = WIDGET_FOREGROUND(bar);
            rtgui_dc_fill_rect(dc, &rect);
            rect.y1 -= val;
            rect.y2 = val;
            WIDGET_BACKGROUND(bar) = bc;
            rtgui_dc_fill_rect(dc, &rect);
        } else {
            /* horizontal bar grows from left to right */
            val = (rt_uint16_t)((rt_uint32_t)RECT_W(rect) * \
                (bar->range - bar->value) / bar->range);

            rect.x2 -= val;
            WIDGET_BACKGROUND(bar) = WIDGET_FOREGROUND(bar);
            rtgui_dc_fill_rect(dc, &rect);
            rect.x1 = rect.x2;
            rect.x2 += val;
            WIDGET_BACKGROUND(bar) = bc;
            rtgui_dc_fill_rect(dc, &rect);
        }
    } while (0);

    rtgui_dc_end_drawing(dc, RT_TRUE);
    LOG_W("draw progress done");
}

/* Public functions ----------------------------------------------------------*/
rtgui_progress_t *rtgui_create_progress(rtgui_container_t *cntr,
    rtgui_evt_hdl_t hdl, rtgui_rect_t *rect, rtgui_orient_t orient,
    rt_uint16_t range) {
    rtgui_progress_t *bar;

    do {
        bar = CREATE_INSTANCE(progress, hdl);
        if (!bar) break;
        bar->orient = orient;
        bar->range = range;
        if (rect)
            rtgui_widget_set_rect(TO_WIDGET(bar), rect);
        if (cntr)
            rtgui_container_add_child(cntr, TO_WIDGET(bar));
    } while (0);

    return bar;
}

void rtgui_progress_set_range(rtgui_progress_t *bar, rt_uint16_t range) {
    if (bar->range != range) {
        bar->range = range;
        rtgui_widget_update(TO_WIDGET(bar));
    }
}
RTM_EXPORT(rtgui_progress_set_range);

RTGUI_MEMBER_GETTER(rtgui_progress_t, progress, rt_uint16_t, range);

void rtgui_progress_set_value(rtgui_progress_t *bar, rt_uint16_t value) {
    if (bar->value != value) {
        bar->value = value;
        rtgui_widget_update(TO_WIDGET(bar));
    }
}
RTM_EXPORT(rtgui_progress_set_value);

RTGUI_MEMBER_GETTER(rtgui_progress_t, progress, rt_uint16_t, value);
