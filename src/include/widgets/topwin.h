/*
 * File      : topwin.h
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
#ifndef __RTGUI_TOPWIN_H__
#define __RTGUI_TOPWIN_H__

#include "../rtgui.h"
#include "../region.h"
#include "./title.h"

/* add or remove a top win */
rt_err_t rtgui_topwin_add(struct rtgui_event_win_create *event);
rt_err_t rtgui_topwin_remove(rtgui_win_t *wid);
rt_err_t rtgui_topwin_activate(struct rtgui_event_win_activate *event);
rt_err_t rtgui_topwin_activate_topwin(rtgui_topwin_t *win);

/* show a window */
rt_err_t rtgui_topwin_show(struct rtgui_event_win *event);
/* hide a window */
rt_err_t rtgui_topwin_hide(struct rtgui_event_win *event);
/* move a window */
rt_err_t rtgui_topwin_move(rtgui_evt_generic_t *event);
/* resize a window */
void rtgui_topwin_resize(rtgui_win_t *wid, rtgui_rect_t *r);
/* a window is entering modal mode */
rt_err_t rtgui_topwin_modal_enter(struct rtgui_event_win_modal_enter *event);

/* get window at (x, y) */
rtgui_topwin_t *rtgui_topwin_get_wnd(int x, int y);
rtgui_topwin_t *rtgui_topwin_get_wnd_no_modaled(int x, int y);

//void rtgui_topwin_deactivate_win(rtgui_topwin_t* win);

/* window title */
void rtgui_topwin_title_ondraw(rtgui_topwin_t *win);
void rtgui_topwin_title_onmouse(rtgui_topwin_t *win, struct rtgui_event_mouse *event);

/* monitor rect */
void rtgui_topwin_append_monitor_rect(rtgui_win_t *wid, rtgui_rect_t *rect);
void rtgui_topwin_remove_monitor_rect(rtgui_win_t *wid, rtgui_rect_t *rect);

/* get the topwin that is currently focused */
rtgui_topwin_t *rtgui_topwin_get_focus(void);

/* get the topwin which app I belong */
rtgui_app_t *rtgui_topwin_app_get_focus(void);

rtgui_topwin_t *rtgui_topwin_get_topmost_window_shown_all(void);

#endif

