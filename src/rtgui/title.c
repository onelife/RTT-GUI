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
 * 2019-05-15     onelife      Refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "../include/rtgui_system.h"
//#include <rtgui/rtgui_theme.h>
//#include "../server/mouse.h"
#include "../include/widgets/window.h"
#include "../include/widgets/title.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_TTL"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototypes -----------------------------------------------*/
static void _rtgui_win_title_constructor(rtgui_win_title_t *win_t);
static void _rtgui_win_title_deconstructor(rtgui_win_title_t *win_t);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
DEFINE_CLASS_TYPE(
    win_title, "win_title",
    RTGUI_PARENT_TYPE(widget),
    _rtgui_win_title_constructor,
    _rtgui_win_title_deconstructor,
    sizeof(rtgui_win_title_t));

/* Private functions ---------------------------------------------------------*/
static void _rtgui_win_title_constructor(rtgui_win_title_t *win_t) {
    RTGUI_WIDGET(win_t)->flag = RTGUI_WIDGET_FLAG_DEFAULT;
    RTGUI_WIDGET_TEXTALIGN(win_t) = RTGUI_ALIGN_CENTER_VERTICAL;

    rtgui_object_set_event_handler(RTGUI_OBJECT(win_t),
        rtgui_win_tile_event_handler);
}

static void _rtgui_win_title_deconstructor(rtgui_win_title_t *win_t) {
    (void)win_t;
}

/* Public functions ----------------------------------------------------------*/
rtgui_win_title_t *rtgui_win_title_create(struct rtgui_win *win) {
    rtgui_win_title_t *win_t;

    win_t = (rtgui_win_title_t *)rtgui_widget_create(RTGUI_WIN_TITLE_TYPE);
    if (win_t) RTGUI_WIDGET(win_t)->toplevel = win;

    return win_t;
}

void rtgui_win_title_destroy(rtgui_win_title_t *win_t) {
    return rtgui_widget_destroy(RTGUI_WIDGET(win_t));
}

rt_bool_t rtgui_win_tile_event_handler(struct rtgui_obj *obj,
    rtgui_evt_generic_t *evt) {
    struct rtgui_win_title *win_t;
    rtgui_widget_t *wgt;
    rt_bool_t done;

    if (!obj || !evt) return RT_FALSE;

    win_t = RTGUI_WIN_TITLE(obj);
    wgt = RTGUI_WIDGET(obj);
    if (!wgt->toplevel) {
        LOG_E("%s no toplevel");
        return RT_FALSE;
    }
    done = RT_FALSE;

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
        done = rtgui_widget_event_handler(obj, evt);
        break;
    }

    if (done && evt) {
        LOG_D("free %p", evt);
        rt_mp_free(evt);
        evt = RT_NULL;
    }
    return done;
}
