/*
 * File      : progress.h
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
#ifndef __RTGUI_PROGRESS_H__
#define __RTGUI_PROGRESS_H__
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

/* Exported defines ----------------------------------------------------------*/
#define PROGRESS_WIDTH_DEFAULT              (100)
#define PROGRESS_HEIGHT_DEFAULT             (20)
#define PROGRESS_RANGE_DEFAULT              (100)

#define CREATE_PROGRESS_INSTANCE(obj, hdl, _orient, _range, rect) \
    do {                                    \
        obj = (rtgui_progress_t *)CREATE_INSTANCE(progress, hdl); \
        if (!obj) break;                    \
        obj->orient = _orient;              \
        obj->range = _range;                \
        if (rect != RT_NULL)                \
            rtgui_widget_set_rect(TO_WIDGET(obj), rect); \
    } while (0)

#define DELETE_PROGRESS_INSTANCE(obj)       DELETE_INSTANCE(obj)

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_progress_t, progress, rt_uint16_t, range);
MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_progress_t, progress, rt_uint16_t, value);
void rtgui_theme_draw_progress(rtgui_progress_t *bar);

#endif /* __RTGUI_PROGRESS_H__ */
