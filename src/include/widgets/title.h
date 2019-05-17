/*
 * File      : title.h
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
 * 2019-05-15     onelife      Refactor
 */

#ifndef __RTGUI_TITLE__
#define __RTGUI_TITLE__

/* Includes ------------------------------------------------------------------*/
#include "./widget.h"

/* Exported defines ----------------------------------------------------------*/
DECLARE_CLASS_TYPE(win_title);
/** Gets the type of a title */
#define RTGUI_WIN_TITLE_TYPE        (RTGUI_TYPE(win_title))
/** Casts the object to an rtgui_win_title */
#define RTGUI_WIN_TITLE(obj)        \
    (RTGUI_OBJECT_CAST((obj), RTGUI_WIN_TITLE_TYPE, rtgui_win_title_t))
/** Checks if the object is an rtgui_win_title */
#define RTGUI_IS_WIN_TITLE(obj)     \
    (RTGUI_OBJECT_CHECK_TYPE((obj), RTGUI_WIN_TITLE_TYPE))

/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_win_title {
    struct rtgui_widget parent;
} rtgui_win_title_t;

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rtgui_win_title_t *rtgui_win_title_create(struct rtgui_win *window);
void rtgui_win_title_destroy(rtgui_win_title_t *win_t);
rt_bool_t rtgui_win_tile_event_handler(struct rtgui_obj *obj,
    rtgui_evt_generic_t *evt);

#endif /* __RTGUI_TITLE__ */
