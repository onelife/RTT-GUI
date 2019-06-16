/*
 * File      : window.h
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
 * 2009-10-04     Bernard      first version
 * 2010-05-03     Bernard      add win close function
 * 2019-05-15     onelife      refactor
 */
#ifndef __RTGUI_WINDOW_H__
#define __RTGUI_WINDOW_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "../rtgui.h"

/* Exported defines ----------------------------------------------------------*/
#define RTGUI_WIN_MAGIC                     0xA5A55A5A

#define TITLE_HEIGHT                        20
#define TITLE_CB_WIDTH                      16
#define TITLE_CB_HEIGHT                     16
#define TITLE_BORDER_SIZE                   2

#define CREATE_WIN_INSTANCE(obj, hdl, parent, title, style, rect) \
    do {                                    \
        obj = (rtgui_win_t *)CREATE_INSTANCE(win, hdl); \
        if (!obj) break;                    \
        if (rtgui_win_init(obj, parent, title, rect, style)) \
            DELETE_INSTANCE(obj);           \
    } while (0)

#define CREATE_MAIN_WIN(obj, hdl, parent, title, style) \
    do {                                    \
        rtgui_rect_t rect;                  \
        rtgui_get_mainwin_rect(&rect);      \
        CREATE_WIN_INSTANCE(obj, hdl, parent, title, style, &rect); \
    } while (0)

#define DELETE_WIN_INSTANCE(obj)            rtgui_win_uninit(obj)

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_win_init(rtgui_win_t *win, rtgui_win_t *parent,
    const char *title, rtgui_rect_t *rect, rt_uint16_t style);
void rtgui_win_uninit(rtgui_win_t *win);

rt_err_t rtgui_win_show(rtgui_win_t *win, rt_bool_t is_modal);
rt_err_t rtgui_win_enter_modal(rtgui_win_t *win);

void rtgui_win_hide(rtgui_win_t *win);
void rtgui_win_end_modal(rtgui_win_t *win, rtgui_modal_code_t modal);
rt_err_t rtgui_win_activate(rtgui_win_t *win);
rt_bool_t rtgui_win_is_activated(rtgui_win_t *win);

void rtgui_win_move(rtgui_win_t *win, int x, int y);

/* reset extent of window */
void rtgui_win_set_rect(rtgui_win_t *win, rtgui_rect_t *rect);
void rtgui_win_update_clip(rtgui_win_t *win);

MEMBER_SETTER_PROTOTYPE(rtgui_win_t, win, rtgui_evt_hdl_t, on_activate);
MEMBER_SETTER_PROTOTYPE(rtgui_win_t, win, rtgui_evt_hdl_t, on_deactivate);
MEMBER_SETTER_PROTOTYPE(rtgui_win_t, win, rtgui_evt_hdl_t, on_close);
MEMBER_SETTER_PROTOTYPE(rtgui_win_t, win, rtgui_evt_hdl_t, on_key);
MEMBER_GETTER_PROTOTYPE(rtgui_win_t, win, char*, title);

void rtgui_win_event_loop(rtgui_win_t *wnd);

void rtgui_win_set_title(rtgui_win_t *win, const char *title);

rtgui_dc_t *rtgui_win_get_drawing(rtgui_win_t * win);

void rtgui_theme_draw_win(rtgui_title_t *wint);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_WINDOW_H__ */
