/*
 * File      : list.h
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
#ifndef __RTGUI_LIST_H__
#define __RTGUI_LIST_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _LIST_METADATA                      CLASS_METADATA(list)
#define IS_LIST(obj)                        IS_INSTANCE(obj, _LIST_METADATA)
#define TO_LIST(obj)                        CAST_(obj, _LIST_METADATA, rtgui_list_t)

#define CREATE_LIST_INSTANCE(obj, hdl, items, cnt, rect) \
    do {                                    \
        obj = (rtgui_list_t *)CREATE_INSTANCE(list, hdl); \
        if (!obj) break;                    \
        rtgui_widget_set_rect(TO_WIDGET(obj), rect); \
        _rtgui_list_set_items(obj, items, cnt); \
    } while (0)

#define DELETE_LIST_INSTANCE(obj)           DELETE_INSTANCE(obj)

#define LIST_SETTER(mname)                  rtgui_list_set_##mname

/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_list_item rtgui_list_item_t;
typedef struct rtgui_list rtgui_list_t;

struct rtgui_list_item {
    char *name;
    rtgui_image_t *image;
    // void *data;
};

struct rtgui_list {
    rtgui_widget_t _super;
    /* PRIVATE */
    rtgui_list_item_t *items;
    rt_uint16_t page_sz;                    /* #items per page */
    rt_uint16_t count;                      /* #items */
    rt_int32_t current;                     /* current item */
    rtgui_evt_hdl_t on_item;
};


/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(list);

#undef __RTGUI_LIST_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
void _rtgui_list_set_items(rtgui_list_t *list, rtgui_list_item_t *items,
    rt_uint16_t count);
void rtgui_list_set_items(rtgui_list_t *list, rtgui_list_item_t *items,
    rt_uint16_t count);
void rtgui_list_set_current(rtgui_list_t *list, rt_int32_t idx);
MEMBER_SETTER_PROTOTYPE(rtgui_list_t, list, rtgui_evt_hdl_t, on_item);

void _theme_draw_selected(rtgui_dc_t *dc, rtgui_rect_t *rect);

#endif /* IMPORT_TYPES */

#endif /* __RTGUI_LIST_H__ */

