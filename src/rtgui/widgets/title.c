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
#include "include/rtgui.h"
#include "include/region.h"
#include "include/widgets/window.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_TTL"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
static void _title_constructor(void *obj);
static rt_bool_t _title_event_handler(void *obj, rtgui_evt_generic_t *evt);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    title,
    CLASS_METADATA(widget),
    _title_constructor,
    RT_NULL,
    _title_event_handler,
    sizeof(rtgui_title_t));

/* Private functions ---------------------------------------------------------*/
static void _title_constructor(void *obj) {
    rtgui_title_t *title_ = obj;

    TO_WIDGET(title_)->flag = RTGUI_WIDGET_FLAG_DEFAULT;
    WIDGET_TEXTALIGN(title_) = RTGUI_ALIGN_CENTER_VERTICAL;
}

static rt_bool_t _title_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_title_t *title_;
    rtgui_widget_t *wgt;
    rt_bool_t done;

    EVT_LOG("[TitEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);
    title_ = TO_TITLE(obj);
    wgt = TO_WIDGET(obj);
    if (!wgt->toplevel) {
        LOG_E("[TitEVT] %s no toplevel");
        return RT_FALSE;
    }
    done = RT_FALSE;

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        rtgui_theme_draw_win(title_);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (IS_WIN_STYLE(wgt->toplevel, CLOSEBOX)) {
            if (evt->mouse.button & RTGUI_MOUSE_BUTTON_LEFT) {
                rtgui_rect_t rect;

                /* get close button rect (device value) */
                rect.x1 = wgt->extent.x2 - TITLE_BORDER_SIZE - 3 - \
                    TITLE_CB_WIDTH;
                rect.y1 = wgt->extent.y1 + TITLE_BORDER_SIZE + 3;
                rect.x2 = rect.x1 + TITLE_CB_WIDTH;
                rect.y2 = rect.y1 + TITLE_CB_HEIGHT;

                if (evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) {
                    if (rtgui_rect_contains_point(
                        &rect, evt->mouse.x, evt->mouse.y)) {
                        WIN_FLAG_SET(wgt->toplevel, CB_PRESSED);
                        rtgui_theme_draw_win(title_);

                #ifdef RTGUI_USING_WINMOVE
                    } else {
                        rtgui_winrect_set(wgt->toplevel);
                #endif
                    }
                } else if (evt->mouse.button & RTGUI_MOUSE_BUTTON_UP) {
                    if (IS_WIN_FLAG(wgt->toplevel, CB_PRESSED) && \
                        (rtgui_rect_contains_point(
                            &rect, evt->mouse.x, evt->mouse.y))) {
                        RTGUI_EVENT_REINIT(evt, WIN_CLOSE);
                        evt->win_close.wid = wgt->toplevel;
                        (void)EVENT_HANDLER(wgt->toplevel)(wgt->toplevel, evt);
                    } else {
                        WIN_FLAG_CLEAR(wgt->toplevel, CB_PRESSED);
                        rtgui_theme_draw_win(title_);
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
        if (SUPER_CLASS_HANDLER(title))
            done = SUPER_CLASS_HANDLER(title)(title_, evt);
        break;
    }

    EVT_LOG("[TitEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

/* Public functions ----------------------------------------------------------*/
