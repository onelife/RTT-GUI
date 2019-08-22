/*
 * File      : mouse.h
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
 */
#ifndef __RTGUI_MOUSE_H__
#define __RTGUI_MOUSE_H__
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_monitor rtgui_monitor_t;

struct rtgui_monitor {
    rtgui_rect_t rect;
    rt_slist_t list;
};

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_cursor_init(void);
void rtgui_cursor_uninit(void);
void rtgui_cursor_move(rt_uint16_t x, rt_uint16_t y);
void rtgui_cursor_set(rt_uint16_t x, rt_uint16_t y);
void rtgui_monitor_append(rt_slist_t *list, rtgui_rect_t *rect);
void rtgui_monitor_remove(rt_slist_t *list, rtgui_rect_t *rect);
rt_bool_t rtgui_monitor_contain_point(rt_slist_t *list, int x, int y);

# ifdef RTGUI_USING_CURSOR
    void rtgui_cursor_enable(rt_bool_t enable);
    void rtgui_cursor_set_image(rtgui_image_t *img);
    void rtgui_cursor_get_rect(rtgui_rect_t *rect);
    void rtgui_cursor_show(void);
    void rtgui_cursor_hide(void);
    rt_bool_t rtgui_mouse_is_intersect(rtgui_rect_t *rect);
# endif
# ifdef RTGUI_USING_WINMOVE
    rt_bool_t rtgui_winmove_is_moving(void);
    void rtgui_winmove_start(rtgui_win_t *win);
    rt_bool_t rtgui_winmove_done(rtgui_rect_t *winrect, rtgui_win_t **win);
# endif

#endif /* __RTGUI_MOUSE_H__ */
