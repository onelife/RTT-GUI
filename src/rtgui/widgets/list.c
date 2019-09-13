/*
 * File      : list.c
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
 * 2010-01-06     Bernard      first version
 * 2019-07-04     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/image.h"
#include "include/widgets/container.h"
#include "include/widgets/list.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                        RTGUI_LOG_LEVEL
# define LOG_TAG                        "WGT_LST"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)         rt_kprintf(format "\n", ##args)
# define LOG_W                          LOG_E
# define LOG_D                          LOG_E
#endif /* RT_USING_ULOG */

#define BEGINNING_MARGIN                (5)
#define ROW_MARGIN                      (2)
#define COLUMN_MARGIN                   (2)

/* Private function prototype ------------------------------------------------*/
static void _list_constructor(void *obj);
static rt_bool_t _list_event_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t _list_on_unfocus(void *obj, rtgui_evt_generic_t *evt);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    list,
    CLASS_METADATA(widget),
    _list_constructor,
    RT_NULL,
    _list_event_handler,
    sizeof(rtgui_list_t));

/* Private functions ---------------------------------------------------------*/
static void _list_constructor(void *obj) {
    rtgui_list_t *list = obj;

    WIDGET_BACKGROUND(obj) = white;
    WIDGET_TEXTALIGN(obj) = RTGUI_ALIGN_CENTER_VERTICAL;

    WIDGET_FLAG_SET(obj, FOCUSABLE);
    WIDGET_SETTER(on_unfocus)(TO_WIDGET(obj), _list_on_unfocus);

    list->items = RT_NULL;
    list->page_sz = 1;
    list->count = 0;
    list->current = -1;
    list->on_item = RT_NULL;
}

static rt_bool_t _list_on_unfocus(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_widget_t *wgt = obj;
    (void)evt;

    if (!IS_WIDGET_FLAG(wgt, FOCUS)) {
        rtgui_color_t fc;
        rtgui_dc_t *dc;
        rtgui_rect_t rect;

        dc = rtgui_dc_begin_drawing(wgt);
        if (!dc) {
            LOG_E("no dc");
            return RT_FALSE;
        }

        rtgui_widget_get_rect(wgt, &rect);
        fc = RTGUI_DC_FC(dc);
        RTGUI_DC_FC(dc) = RTGUI_DC_BC(dc);
        rtgui_dc_draw_focus_rect(dc, &rect);
        RTGUI_DC_FC(dc) = fc;

        rtgui_dc_end_drawing(dc, RT_TRUE);
    }

    return RT_TRUE;
}

static void _list_draw_item(rtgui_dc_t *dc, const rtgui_list_item_t *item,
    rtgui_rect_t *rect) {
    rtgui_rect_t img_rect;

    /* offset row margin */
    rect->x1 += BEGINNING_MARGIN;
    if (item->image) {
        rtgui_image_get_rect(item->image, &img_rect);
        rtgui_rect_move_align(rect, &img_rect, RTGUI_ALIGN_CENTER_VERTICAL);
        rtgui_image_blit(item->image, dc, &img_rect);
        /* offset column margin */
        rect->x1 += item->image->w + COLUMN_MARGIN;
    }
    rtgui_dc_draw_text(dc, item->name, rect);

    /* reset rect */
    if (item->image)
        rect->x1 -= item->image->w + COLUMN_MARGIN;
    rect->x1 -= BEGINNING_MARGIN;
}

static void _list_draw(void *obj) {
    rt_uint16_t item_h = _theme_selected_item_height() + ROW_MARGIN;
    rtgui_list_t *list = obj;
    rtgui_widget_t *wgt = obj;
    rt_uint16_t idx, end;
    rtgui_dc_t *dc;
    rtgui_rect_t rect;

    if (list->current < 0)
        idx = 0;
    else
        idx = list->current - (list->current % list->page_sz);
    end = _MIN(idx + list->page_sz + 1, list->count - 1);
    LOG_D("idx %d end %d", idx, end);

    dc = rtgui_dc_begin_drawing(wgt);
    if (!dc) {
        LOG_E("no dc");
        return;
    }

    rtgui_widget_get_rect(wgt, &rect);
    rtgui_dc_fill_rect(dc, &rect);
    /* draw focused border */
    if (IS_WIDGET_FLAG(wgt, FOCUS))
        rtgui_dc_draw_focus_rect(dc, &rect);

    rect.x1 += 1;
    rect.x2 -= 1;
    rect.y1 += ROW_MARGIN;
    rect.y2 = rect.y1 + item_h;
    /* draw itmes */
    for (; idx <= end; idx++) {
        if (idx == list->current)
            _theme_draw_selected(dc, &rect);
        _list_draw_item(dc, &list->items[idx], &rect);
        rect.y1 += item_h;
        rect.y2 += item_h;
    }

    rtgui_dc_end_drawing(dc, RT_TRUE);
}

static void _list_draw_current_item(rtgui_dc_t *dc, void *obj,
    rt_int32_t last_idx) {
    rt_uint16_t item_h = _theme_selected_item_height() + ROW_MARGIN;
    rtgui_list_t *list = obj;
    rtgui_widget_t *wgt = obj;
    rtgui_rect_t rect, item_rect;

    if (last_idx < 0)
        last_idx = 0;

    /* if not in same page then update all */
    if ((last_idx / list->page_sz) != (list->current / list->page_sz))
        return rtgui_widget_update(wgt);

    rtgui_widget_get_rect(wgt, &rect);
    /* last item rect */
    item_rect.x1 = rect.x1 + 1;
    item_rect.x2 = rect.x2 - 1;
    item_rect.y1 = rect.y1 + ROW_MARGIN + (last_idx % list->page_sz) * item_h;
    item_rect.y2 = item_rect.y1 + item_h;
    /* unfocuse last item */
    rtgui_dc_fill_rect(dc, &item_rect);
    _list_draw_item(dc, &list->items[last_idx], &item_rect);

    /* current item rect */
    item_rect.y1 = rect.y1 + ROW_MARGIN + \
                   (list->current % list->page_sz) * item_h;
    item_rect.y2 = item_rect.y1 + item_h;
    /* focuse current item */
    _theme_draw_selected(dc, &item_rect);
    _list_draw_item(dc, &list->items[list->current], &item_rect);
}

static rt_bool_t _list_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_list_t *list = obj;
    rt_uint16_t item_h = _theme_selected_item_height() + ROW_MARGIN;
    rt_bool_t done = RT_FALSE;

    EVT_LOG("[LisEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        _list_draw(obj);
        break;

    case RTGUI_EVENT_RESIZE:
        /* recalculate page size */
        list->page_sz = evt->resize.h / item_h;
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    {
        rtgui_widget_t *wgt = obj;
        rtgui_rect_t rect;
        rt_uint16_t idx;
        rtgui_dc_t *dc;

        /* get physical extent */
        rtgui_widget_get_rect(wgt, &rect);
        rtgui_widget_rect_to_device(wgt, &rect);

        if (!list->count || \
            !rtgui_rect_contains_point(&rect, evt->mouse.x, evt->mouse.y))
            break;

        idx = (evt->mouse.y - rect.y1) / item_h;
        LOG_D("idx %d, current %d", idx, list->current);
        LOG_D("count %d, page_sz %d", list->count, list->page_sz);

        /* set focus */
        rtgui_widget_focus(wgt);
        rtgui_widget_get_rect(wgt, &rect);

        dc = rtgui_dc_begin_drawing(wgt);
        if (!dc) {
            LOG_E("no dc");
            break;
        }

        /* draw focused border */
        if (IS_WIDGET_FLAG(wgt, FOCUS)) {
            rtgui_dc_draw_focus_rect(dc, &rect);
        }

        if ((idx < list->count) && (idx <= list->page_sz)) {
            if (IS_MOUSE_EVENT_BUTTON(evt, DOWN)) {
                rt_int32_t last_idx = list->current;
                rt_int32_t new_idx = list->current - \
                                     (list->current % list->page_sz) + idx;
                if (new_idx < list->count) {
                    list->current = new_idx;
                    LOG_D("current -> %d", list->current);
                    _list_draw_current_item(dc, obj, last_idx);
                }
            } else {
                if (list->on_item)
                    (void)list->on_item(obj, RT_NULL);
            }
        }

        rtgui_dc_end_drawing(dc, RT_TRUE);
        done = RT_TRUE;
        break;
    }

    case RTGUI_EVENT_KBD:
    {
        rt_int16_t last_idx = list->current;
        rt_int32_t cur_item = -1;
        rtgui_dc_t *dc;

        if (!list->count || !IS_KBD_EVENT_TYPE(evt, DOWN)) break;

        if (IS_KBD_EVENT_KEY(evt, RETURN)) {
            if (list->on_item)
                (void)list->on_item(obj, RT_NULL);
            done = RT_TRUE;
            break;
        } else if (IS_KBD_EVENT_KEY(evt, LEFT)) {
            cur_item = list->current - list->page_sz;
        } else if (IS_KBD_EVENT_KEY(evt, RIGHT)) {
            cur_item = list->current + list->page_sz;
        } else if (IS_KBD_EVENT_KEY(evt, UP)) {
            cur_item = list->current - 1;
        } else if (IS_KBD_EVENT_KEY(evt, DOWN)) {
            cur_item = list->current + 1;
        } else {
            break;
        }

        if (cur_item < 0)
            list->current = 0;
        else if (cur_item >= list->count)
            list->current = list->count - 1;
        else
            list->current = cur_item;

        dc = rtgui_dc_begin_drawing(TO_WIDGET(obj));
        if (!dc) {
            LOG_E("no dc");
            break;
        }

        _list_draw_current_item(dc, obj, last_idx);

        rtgui_dc_end_drawing(dc, RT_TRUE);
        done = RT_TRUE;
        break;
    }

    default:
        if (SUPER_CLASS_HANDLER(list))
            done = SUPER_CLASS_HANDLER(list)(obj, evt);
        break;
    }

    EVT_LOG("[LisEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static void _list_set_items(rtgui_list_t *list, rtgui_list_item_t *items,
    rt_uint16_t count) {
    rt_uint16_t item_h = _theme_selected_item_height() + 2;
    rtgui_rect_t rect;

    rtgui_widget_get_rect(TO_WIDGET(list), &rect);
    list->page_sz = RECT_H(rect) / item_h;
    if (!list->page_sz)
        list->page_sz = 1;
    list->items = items;
    list->count = count;
}

/* Public functions ----------------------------------------------------------*/
rtgui_list_t *rtgui_create_list(rtgui_container_t *cntr, rtgui_evt_hdl_t hdl,
    rtgui_rect_t *rect, rtgui_list_item_t *items, rt_uint16_t count) {
    rtgui_list_t *list;

    do {
        list = CREATE_INSTANCE(list, hdl);
        if (!list) break;
        if (rect)
            rtgui_widget_set_rect(TO_WIDGET(list), rect);
        _list_set_items(list, items, count);
        if (cntr)
            rtgui_container_add_child(cntr, TO_WIDGET(list));
    } while (0);

    return list;
}

void rtgui_list_set_items(rtgui_list_t *list, rtgui_list_item_t *items,
    rt_uint16_t count) {
    _list_set_items(list, items, count);
    list->current = -1;
    rtgui_widget_update(TO_WIDGET(list));
}
RTM_EXPORT(rtgui_list_set_items);

void rtgui_list_set_current(rtgui_list_t *list, rt_int32_t idx) {
    if (idx >= list->count)
        return;

    if (idx != list->current) {
        rt_int32_t last_idx = list->current;
        rtgui_dc_t *dc;

        list->current = idx;
        dc = rtgui_dc_begin_drawing(TO_WIDGET(list));
        if (!dc) {
            LOG_E("no dc");
            return;
        }

        _list_draw_current_item(dc, list, last_idx);

        rtgui_dc_end_drawing(dc, RT_TRUE);
    }
    if (list->on_item)
        (void)list->on_item(TO_OBJECT(list), RT_NULL);
}
RTM_EXPORT(rtgui_list_set_current);

RTGUI_MEMBER_SETTER(rtgui_list_t, list, rtgui_evt_hdl_t, on_item);

void _theme_draw_selected(rtgui_dc_t *dc, rtgui_rect_t *rect) {
    rtgui_color_t bc;
    rt_uint16_t idx;

    bc = RTGUI_DC_FC(dc);
    RTGUI_DC_FC(dc) = selected_color;

    rtgui_dc_draw_vline(dc, rect->x1 + 2, rect->y1 + 2, rect->y2 - 2);
    rtgui_dc_draw_vline(dc, rect->x2 - 2, rect->y1 + 2, rect->y2 - 2);

    for (idx = rect->y1 + 1; idx <= rect->y2 - 1; idx++)
        rtgui_dc_draw_hline(dc, rect->x1 + 3, rect->x2 - 3, idx);

    RTGUI_DC_FC(dc) = bc;
}
