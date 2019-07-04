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
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _WIN_METADATA                       CLASS_METADATA(win)
#define IS_WIN(obj)                         IS_INSTANCE((obj), _WIN_METADATA)
#define TO_WIN(obj)                         CAST_(obj, _WIN_METADATA, rtgui_win_t)

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

#define WIN_FLAG(w)                         (TO_WIN(w)->flag)
#define WIN_FLAG_CLEAR(w, fname)            WIN_FLAG(w) &= ~RTGUI_WIN_FLAG_##fname
#define WIN_FLAG_SET(w, fname)              WIN_FLAG(w) |= RTGUI_WIN_FLAG_##fname
#define IS_WIN_FLAG(w, fname)               (WIN_FLAG(w) & RTGUI_WIN_FLAG_##fname)

#define WIN_STYLE(w)                        (TO_WIN(w)->style)
#define WIN_STYLE_CLEAR(w, sname)           WIN_STYLE(w) &= ~RTGUI_WIN_STYLE_##sname
#define WIN_STYLE_SET(w, sname)             WIN_STYLE(w) |= RTGUI_WIN_STYLE_##sname
#define IS_WIN_STYLE(w, sname)              (WIN_STYLE(w) & RTGUI_WIN_STYLE_##sname)

#define RTGUI_WIN_MAGIC                     0xA5A55A5A

/* Exported types ------------------------------------------------------------*/
typedef enum rtgui_win_style {
    RTGUI_WIN_STYLE_NO_FOCUS                = 0x0001,  /* non-focused */
    RTGUI_WIN_STYLE_NO_TITLE                = 0x0002,  /* no title */
    RTGUI_WIN_STYLE_NO_BORDER               = 0x0004,  /* no border */
    RTGUI_WIN_STYLE_CLOSEBOX                = 0x0008,  /* close button */
    RTGUI_WIN_STYLE_MINIBOX                 = 0x0010,  /* minimize button */
    RTGUI_WIN_STYLE_DESTROY_ON_CLOSE        = 0x0020,  /* destroy if closed */
    RTGUI_WIN_STYLE_ONTOP                   = 0x0040,  /* at top layer */
    RTGUI_WIN_STYLE_ONBTM                   = 0x0080,  /* at bottom layer */
    RTGUI_WIN_STYLE_MAINWIN                 = 0x0106,  /* main window */
    RTGUI_WIN_STYLE_DEFAULT                 = RTGUI_WIN_STYLE_CLOSEBOX | \
                                              RTGUI_WIN_STYLE_MINIBOX,
} rtgui_win_style_t;

typedef enum rtgui_win_flag {
    RTGUI_WIN_FLAG_INIT                     = 0x00,
    RTGUI_WIN_FLAG_MODAL                    = 0x01,
    RTGUI_WIN_FLAG_CLOSED                   = 0x02,
    RTGUI_WIN_FLAG_ACTIVATE                 = 0x04,
    RTGUI_WIN_FLAG_IN_MODAL                 = 0x08,  /* modaled by other */
    RTGUI_WIN_FLAG_CONNECTED                = 0x10,  /* with server */
    /* "window" handles or dispatchs KBD event to its focused widget by function
        "_win_event_handler()".
        RTGUI_WIN_FLAG_HANDLE_KEY indicates key-handling mode.
     */
    RTGUI_WIN_FLAG_HANDLE_KEY               = 0x20,
    RTGUI_WIN_FLAG_CB_PRESSED               = 0x40,
} rtgui_win_flag_t;

typedef enum rtgui_modal_code {
    RTGUI_MODAL_OK,
    RTGUI_MODAL_CANCEL,
    RTGUI_MODAL_MAX                         = 0xFFFF,
} rtgui_modal_code_t;

struct rtgui_win {
    rtgui_container_t _super;               /* super class */
    rtgui_win_t *parent;                    /* parent window */
    rtgui_app_t *app;
    char *title;
    rtgui_win_style_t style;
    rtgui_win_flag_t flag;
    rtgui_modal_code_t modal;
    rt_base_t update;                       /* update count */
    rt_base_t drawing;                      /* drawing count */
    rtgui_rect_t drawing_rect;
    rtgui_rect_t outer_extent;
    rtgui_region_t outer_clip;
    rtgui_title_t *_title;
    rtgui_widget_t *focused;
    rtgui_widget_t *last_mouse;             /* last mouse event handler */
    rtgui_evt_hdl_t on_activate;
    rtgui_evt_hdl_t on_deactivate;
    rtgui_evt_hdl_t on_close;
    /* KBD event is sent to the focused widget. In case a KBD event is either
        handked by the focused widget nor it's parents, it finally is handled by
        function "on_key()".
     */
    rtgui_evt_hdl_t on_key;
    void *user_data;
    /* PRIVATE */
    rt_err_t (*_do_show)(rtgui_win_t *win);
    rt_uint16_t _ref_count;                 /* app _ref_cnt */
    rt_uint32_t _magic;                     /* 0xA5A55A5A */
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(win);

#undef __RTGUI_WINDOW_H__
#else /* IMPORT_TYPES */

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

void rtgui_win_set_title(rtgui_win_t *win, const char *title);
rtgui_dc_t *rtgui_win_get_drawing(rtgui_win_t * win);
void rtgui_theme_draw_win(rtgui_title_t *wint);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_WINDOW_H__ */
