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
 * 2019-05-15     onelife      Refactor
 */
#ifndef __RTGUI_WINDOW_H__
#define __RTGUI_WINDOW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../rtgui.h"
#include "../dc.h"
#include "./widget.h"
#include "./box.h"

DECLARE_CLASS_TYPE(win);
/** Gets the type of a win */
#define RTGUI_WIN_TYPE       (RTGUI_TYPE(win))
/** Casts the object to an rtgui_win */
#define RTGUI_WIN(obj)       (RTGUI_OBJECT_CAST((obj), RTGUI_WIN_TYPE, rtgui_win_t))
/** Checks if the object is an rtgui_win */
#define RTGUI_IS_WIN(obj)    (RTGUI_OBJECT_CHECK_TYPE((obj), RTGUI_WIN_TYPE))

#define RTGUI_WIN_STYLE_NO_FOCUS            0x0001  /* non-focused window            */
#define RTGUI_WIN_STYLE_NO_TITLE            0x0002  /* no title window               */
#define RTGUI_WIN_STYLE_NO_BORDER           0x0004  /* no border window              */
#define RTGUI_WIN_STYLE_CLOSEBOX            0x0008  /* window has the close button   */
#define RTGUI_WIN_STYLE_MINIBOX             0x0010  /* window has the mini button    */

#define RTGUI_WIN_STYLE_DESTROY_ON_CLOSE    0x0020  /* window is destroyed when closed */
#define RTGUI_WIN_STYLE_ONTOP               0x0040  /* window is in the top layer    */
#define RTGUI_WIN_STYLE_ONBTM               0x0080  /* window is in the bottom layer */
#define RTGUI_WIN_STYLE_MAINWIN             0x0106  /* window is a main window       */

#define RTGUI_WIN_MAGIC						0xA5A55A5A		/* win magic flag */

#define RTGUI_WIN_STYLE_DEFAULT     (RTGUI_WIN_STYLE_CLOSEBOX | RTGUI_WIN_STYLE_MINIBOX)

#define WINTITLE_HEIGHT         20
#define WINTITLE_CB_WIDTH       16
#define WINTITLE_CB_HEIGHT      16
#define WINTITLE_BORDER_SIZE    2

rtgui_win_t *rtgui_win_create(rtgui_win_t *parent_window, const char *title,
                              rtgui_rect_t *rect, rt_uint16_t style);
rtgui_win_t *rtgui_mainwin_create(rtgui_win_t *parent_window, const char *title, rt_uint16_t style);

void rtgui_win_destroy(rtgui_win_t *win);

rt_err_t rtgui_win_init(rtgui_win_t *win, rtgui_win_t *parent_window,
                   const char *title,
                   rtgui_rect_t *rect,
                   rt_uint16_t style);
rt_err_t rtgui_win_fini(rtgui_win_t* win);

/** Close window.
 *
 * @param win the window you want to close
 *
 * @return RT_TRUE if the window is closed. RT_FALSE if not. If the onclose
 * callback returns RT_FALSE, the window won't be closed.
 *
 * \sa rtgui_win_set_onclose .
 */
rt_bool_t rtgui_win_close(rtgui_win_t *win);

rt_err_t rtgui_win_show(rtgui_win_t *win, rt_bool_t is_modal);
rt_err_t rtgui_win_do_show(rtgui_win_t *win);
rt_err_t rtgui_win_enter_modal(rtgui_win_t *win);

void rtgui_win_hide(rtgui_win_t *win);
void rtgui_win_end_modal(rtgui_win_t *win, rtgui_modal_code_t modal_code);
rt_err_t rtgui_win_activate(rtgui_win_t *win);
rt_bool_t rtgui_win_is_activated(rtgui_win_t *win);

void rtgui_win_move(rtgui_win_t *win, int x, int y);

/* reset extent of window */
void rtgui_win_set_rect(rtgui_win_t *win, rtgui_rect_t *rect);
void rtgui_win_update_clip(rtgui_win_t *win);

void rtgui_win_set_onactivate(rtgui_win_t *win, rtgui_evt_hdl_t handler);
void rtgui_win_set_ondeactivate(rtgui_win_t *win, rtgui_evt_hdl_t handler);
void rtgui_win_set_onclose(rtgui_win_t *win, rtgui_evt_hdl_t handler);
void rtgui_win_set_onkey(rtgui_win_t *win, rtgui_evt_hdl_t handler);

rt_bool_t rtgui_win_event_handler(rtgui_obj_t *win,
    rtgui_evt_generic_t *event);

void rtgui_win_event_loop(rtgui_win_t *wnd);

void rtgui_win_set_title(rtgui_win_t *win, const char *title);
char *rtgui_win_get_title(rtgui_win_t *win);

struct rtgui_dc *rtgui_win_get_drawing(rtgui_win_t * win);

rtgui_win_t* rtgui_win_get_topmost_shown(void);
rtgui_win_t* rtgui_win_get_next_shown(void);

void rtgui_theme_draw_win(struct rtgui_win_title *wint);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_WINDOW_H__ */
