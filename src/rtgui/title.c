/*
 * File      : title.c
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
 * 2019-05-15     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "../include/rtgui_system.h"
//#include <rtgui/rtgui_theme.h>
//#include "../server/mouse.h"
#include "../include/widgets/window.h"
#include "../include/widgets/title.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_TTL"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototypes -----------------------------------------------*/
static void _win_title_constructor(void *obj);
static rt_bool_t _win_tile_event_handler(void *obj, rtgui_evt_generic_t *evt);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    win_title,
    CLASS_METADATA(widget),
    _win_title_constructor,
    RT_NULL,
    _win_tile_event_handler,
    sizeof(rtgui_win_title_t));

/* Private functions ---------------------------------------------------------*/
static void _win_title_constructor(void *obj) {
    rtgui_win_title_t *win_t = obj;
    TO_WIDGET(win_t)->flag = RTGUI_WIDGET_FLAG_DEFAULT;
    RTGUI_WIDGET_TEXTALIGN(win_t) = RTGUI_ALIGN_CENTER_VERTICAL;
}

static rt_bool_t _win_tile_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    struct rtgui_win_title *win_t;
    rtgui_widget_t *wgt;
    rt_bool_t done;

    win_t = TO_WIN_TITLE(obj);
    wgt = TO_WIDGET(obj);
    if (!wgt->toplevel) {
        LOG_E("%s no toplevel");
        return RT_FALSE;
    }
    done = RT_FALSE;

    LOG_D("title rx %x from %s", evt->base.type, evt->base.sender->mb->parent.parent.name);
    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        rtgui_theme_draw_win(win_t);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (wgt->toplevel->style & RTGUI_WIN_STYLE_CLOSEBOX) {
            if (evt->mouse.button & RTGUI_MOUSE_BUTTON_LEFT) {
                rtgui_rect_t rect;

                /* get close button rect (device value) */
                rect.x1 = wgt->extent.x2 - WINTITLE_BORDER_SIZE - 3 - \
                    WINTITLE_CB_WIDTH;
                rect.y1 = wgt->extent.y1 + WINTITLE_BORDER_SIZE + 3;
                rect.x2 = rect.x1 + WINTITLE_CB_WIDTH;
                rect.y2 = rect.y1 + WINTITLE_CB_HEIGHT;

                if (evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) {
                    if (rtgui_rect_contains_point(
                        &rect, evt->mouse.x, evt->mouse.y)) {
                        wgt->toplevel->flag |= RTGUI_WIN_FLAG_CB_PRESSED;
                        rtgui_theme_draw_win(win_t);

                #ifdef RTGUI_USING_WINMOVE
                    } else {
                        rtgui_winrect_set(wgt->toplevel);
                #endif
                    }
                } else if (evt->mouse.button & RTGUI_MOUSE_BUTTON_UP) {
                    if ((wgt->toplevel->flag & RTGUI_WIN_FLAG_CB_PRESSED) && \
                        (rtgui_rect_contains_point(
                            &rect, evt->mouse.x, evt->mouse.y))) {
                        (void)rtgui_win_close(wgt->toplevel);
                    } else {
                        wgt->toplevel->flag &= ~RTGUI_WIN_FLAG_CB_PRESSED;
                        rtgui_theme_draw_win(win_t);
                        #ifdef RTGUI_USING_WINMOVE
                            /* Reset the window movement state machine. */
                            (void)rtgui_winrect_moved_done(RT_NULL, RT_NULL);
                        #endif
                    }
                }
            }
        } else if (evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) {
            #ifdef RTGUI_USING_WINMOVE
                rtgui_winrect_set(wgt->toplevel);
            #endif
        }
        done = RT_TRUE;
        break;

    default:
        if (SUPER_HANDLER(win_t)) {
            done = SUPER_HANDLER(win_t)(win_t, evt);
        }
        break;
    }

    LOG_D("title done %d", done);
    if (done && evt) {
        if (!evt->base.ack) {
            LOG_W("title free %p", evt);
            rt_mp_free(evt);
            evt = RT_NULL;
        }
    }
    return done;
}

/* Public functions ----------------------------------------------------------*/
rtgui_win_title_t *rtgui_win_title_create(rtgui_win_t *win, rtgui_evt_hdl_t evt_hdl) {
    rtgui_win_title_t *win_t;

    win_t = (rtgui_win_title_t *)RTGUI_CREATE_INSTANCE(win_title, evt_hdl);
    if (win_t) TO_WIDGET(win_t)->toplevel = win;

    return win_t;
}

void rtgui_win_title_destroy(rtgui_win_title_t *win_t) {
    return rtgui_widget_destroy(TO_WIDGET(win_t));
}
