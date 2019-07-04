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

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

// #ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define TOPWIN_FLAG(t)                      (t->flag)
#define TOPWIN_FLAG_CLEAR(t, fname)         TOPWIN_FLAG(t) &= ~RTGUI_TOPWIN_FLAG_##fname
#define TOPWIN_FLAG_SET(t, fname)           TOPWIN_FLAG(t) |= RTGUI_TOPWIN_FLAG_##fname
#define IS_TOPWIN_FLAG(t, fname)            (TOPWIN_FLAG(t) & RTGUI_TOPWIN_FLAG_##fname)

/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_topwin rtgui_topwin_t;

typedef enum rtgui_topwin_flag {
    RTGUI_TOPWIN_FLAG_INIT                  = 0x0000,
    RTGUI_TOPWIN_FLAG_ACTIVATE              = 0x0001,
    RTGUI_TOPWIN_FLAG_NO_FOCUS              = 0x0002,
    RTGUI_TOPWIN_FLAG_SHOWN                 = 0x0004,   /* by default */
    RTGUI_TOPWIN_FLAG_DONE_MODAL            = 0x0008,   /* modaled by other */
    RTGUI_TOPWIN_FLAG_IN_MODAL              = 0x0100,   /* modaling other */
    RTGUI_TOPWIN_FLAG_ONTOP                 = 0x0200,
    RTGUI_TOPWIN_FLAG_ONBTM                 = 0x0400,
} rtgui_topwin_flag_t;

struct rtgui_topwin {
    rtgui_app_t *app;
    rtgui_win_t *wid;                       /* window id */
    rtgui_rect_t extent;
    rtgui_topwin_flag_t flag;
    rtgui_topwin_t *parent;
    /* "topwin" list will be iterated with normal (when access) and reverse 
        (when painting) order.
     */
    rt_list_t list;
    rt_list_t children;
    rt_slist_t monitor_list;                /* monitor rect list */
};

/* Exported constants --------------------------------------------------------*/

// #undef __RTGUI_TOPWIN_H__
// #else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_topwin_add(rtgui_app_t *app, rtgui_win_t *win,
    rtgui_win_t *parent);
rt_err_t rtgui_topwin_remove(rtgui_win_t *win);
rt_err_t rtgui_topwin_activate(rtgui_topwin_t *win);
rt_err_t rtgui_topwin_activate_win(rtgui_win_t *win);
/* show a window */
rt_err_t rtgui_topwin_show(rtgui_win_t *win);
/* hide a window */
rt_err_t rtgui_topwin_hide(rtgui_win_t *win);
/* move a window */
rt_err_t rtgui_topwin_move(rtgui_win_t *win, rt_int16_t x, rt_int16_t y);
/* resize a window */
void rtgui_topwin_resize(rtgui_win_t *win, rtgui_rect_t *r);
/* a window is entering modal mode */
rt_err_t rtgui_topwin_modal_enter(rtgui_win_t *win);
/* get window at (x, y) */
rtgui_topwin_t *rtgui_topwin_get_with_modal_at(int x, int y);
rtgui_topwin_t *rtgui_topwin_get_at(int x, int y);
/* monitor rect */
void rtgui_topwin_append_monitor_rect(rtgui_win_t *win, rtgui_rect_t *rect);
void rtgui_topwin_remove_monitor_rect(rtgui_win_t *win, rtgui_rect_t *rect);
/* get the topwin that is currently focused */
rtgui_topwin_t *rtgui_topwin_get_focus(void);
/* get the topwin which app I belong */
rtgui_app_t *rtgui_topwin_get_focus_app(void);
rtgui_topwin_t *rtgui_topwin_get_shown(void);
rtgui_win_t* rtgui_topwin_get_shown_win(void);
rtgui_win_t* rtgui_topwin_get_next_shown_win(void);

// #endif /* IMPORT_TYPES */

#endif /* __RTGUI_TOPWIN_H__ */
