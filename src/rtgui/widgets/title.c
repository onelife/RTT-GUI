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
#include "include/widgets/title.h"
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
static void _theme_draw_title(rtgui_title_t *title);

/* Private typedef -----------------------------------------------------------*/
#define RGB_FACTOR                  (4)

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    title,
    CLASS_METADATA(widget),
    _title_constructor,
    RT_NULL,
    _title_event_handler,
    sizeof(rtgui_title_t));

static const rt_uint8_t close_byte[14] = {
    0x06, 0x18, 0x03, 0x30, 0x01, 0xE0, 0x00,
    0xC0, 0x01, 0xE0, 0x03, 0x30, 0x06, 0x18
};

/* Private functions ---------------------------------------------------------*/
static void _title_constructor(void *obj) {
    rtgui_title_t *title_ = obj;

    TO_WIDGET(title_)->flag = RTGUI_WIDGET_FLAG_DEFAULT;
    WIDGET_TEXTALIGN(title_) = RTGUI_ALIGN_CENTER_VERTICAL;
}

static rt_bool_t _title_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_title_t *title_;
    rtgui_win_t *win;
    rt_bool_t done;

    EVT_LOG("[TitEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);
    title_ = TO_TITLE(obj);
    win = TO_WIDGET(obj)->toplevel;
    if (!win) {
        LOG_E("[TitEVT] %s no toplevel");
        return RT_FALSE;
    }
    done = RT_FALSE;

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        _theme_draw_title(title_);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (IS_WIN_STYLE(win, CLOSEBOX)) {
            if (evt->mouse.button & MOUSE_BUTTON_LEFT) {
                rtgui_rect_t rect;

                rtgui_widget_get_rect(TO_WIDGET(title_), &rect);
                if (!IS_WIN_STYLE(win, NO_BORDER))
                    rtgui_rect_inflate(&rect, -TITLE_DEFAULT_BORDER);

                /* get close button rect (device value) */
                rect.x2 -= 3;
                rect.x1 = rect.x2 - TITLE_CLOSE_BUTTON_WIDTH;
                rect.y1 += \
                    (TITLE_DEFAULT_HEIGHT - TITLE_CLOSE_BUTTON_HEIGHT) >> 1;
                rect.y2 = rect.y1 + TITLE_CLOSE_BUTTON_HEIGHT;
                // make it easier to trigger
                rtgui_rect_inflate(&rect, 2);

                if (IS_MOUSE_EVENT_BUTTON(evt, DOWN)) {
                    if (rtgui_rect_contains_point(
                        &rect, evt->mouse.x, evt->mouse.y)) {
                        WIN_FLAG_SET(win, CB_PRESSED);
                        _theme_draw_title(title_);

                #ifdef RTGUI_USING_WINMOVE
                    } else {
                        rtgui_winmove_start(win);
                #endif
                    }
                } else if (IS_MOUSE_EVENT_BUTTON(evt, UP)) {
                    if (IS_WIN_FLAG(win, CB_PRESSED) && \
                        (rtgui_rect_contains_point(
                            &rect, evt->mouse.x, evt->mouse.y))) {
                        RTGUI_EVENT_REINIT(evt, WIN_CLOSE);
                        evt->win_close.wid = win;
                        (void)EVENT_HANDLER(win)(win, evt);
                    } else {
                        WIN_FLAG_CLEAR(win, CB_PRESSED);
                        _theme_draw_title(title_);
                        #ifdef RTGUI_USING_WINMOVE
                            /* Reset the window movement state machine. */
                            (void)rtgui_winmove_done(RT_NULL, RT_NULL);
                        #endif
                    }
                }
            }
        } else if (evt->mouse.button & MOUSE_BUTTON_DOWN) {
            #ifdef RTGUI_USING_WINMOVE
                rtgui_winmove_start(win);
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

/* title drawing */
static void _theme_draw_title(rtgui_title_t *title) {
    rtgui_win_t *win;
    rtgui_dc_t *dc;

    if (!title) return;

    win = TO_WIDGET(title)->toplevel;
    if (!win->_title) {
        LOG_E("no _title");
        return;
    }

    /* begin drawing */
    dc = rtgui_dc_begin_drawing(TO_WIDGET(win->_title));
    if (!dc) {
        LOG_E("no dc");
        return;
    }

    do {
        rtgui_rect_t box_rect = {
            0, 0, TITLE_CLOSE_BUTTON_WIDTH, TITLE_CLOSE_BUTTON_HEIGHT,
        };
        rtgui_rect_t rect;

        rtgui_widget_get_rect(TO_WIDGET(win->_title), &rect);

        /* draw border */
        if (!IS_WIN_STYLE(win, NO_BORDER)) {
            // LOG_D("draw border (%d,%d)-(%d,%d)", rect.x1, rect.y1, rect.x2,
            //     rect.y2);
            WIDGET_FOREGROUND(win->_title) = RTGUI_RGB(212, 208, 200);
            rtgui_dc_draw_hline(dc, rect.x1, rect.x2 - 1, rect.y1);
            rtgui_dc_draw_vline(dc, rect.x1, rect.y1 + 1, rect.y2 - 2);

            WIDGET_FOREGROUND(win->_title) = white;
            rtgui_dc_draw_hline(dc, rect.x1 + 1, rect.x2 - 2, rect.y1 + 1);
            rtgui_dc_draw_vline(dc, rect.x1 + 1, rect.y1 + 2, rect.y2 - 3);

            WIDGET_FOREGROUND(win->_title) = RTGUI_RGB(128, 128, 128);
            rtgui_dc_draw_hline(dc, rect.x1 + 1, rect.x2 - 2, rect.y2 - 2);
            rtgui_dc_draw_vline(dc, rect.x2 - 2, rect.y1 + 2, rect.y2 - 2);

            WIDGET_FOREGROUND(win->_title) = RTGUI_RGB(64, 64, 64);
            rtgui_dc_draw_hline(dc, rect.x1, rect.x2 - 1, rect.y2 - 1);
            rtgui_dc_draw_vline(dc, rect.x2 - 1, rect.y1 + 1, rect.y2 - 2);

            /* shrink border */
            rtgui_rect_inflate(&rect, -TITLE_DEFAULT_BORDER);
        }

        /* draw title */
        if (!IS_WIN_STYLE(win, NO_TITLE)) {
            rt_uint16_t idx, r, g, b, delta;

            LOG_D("draw title (%d,%d)-(%d,%d)", rect.x1, rect.y1, rect.x2,
                rect.y2);
            if (IS_WIN_FLAG(win, ACTIVATE)) {
                r = 10 << RGB_FACTOR;
                g = 36 << RGB_FACTOR;
                b = 106 << RGB_FACTOR;
                delta = (150 << RGB_FACTOR) / RECT_W(rect);
            } else {
                r = 128 << RGB_FACTOR;
                g = 128 << RGB_FACTOR;
                b = 128 << RGB_FACTOR;
                delta = (64 << RGB_FACTOR) / RECT_W(rect);
            }

            for (idx = rect.x1; idx <= rect.x2; idx++) {
                WIDGET_FOREGROUND(win->_title) = RTGUI_RGB(
                    (r >> RGB_FACTOR), (g >> RGB_FACTOR), (b >> RGB_FACTOR));
                rtgui_dc_draw_vline(dc, idx, rect.y1,
                    rect.y1 + TITLE_DEFAULT_HEIGHT);
                r += delta;
                g += delta;
                b += delta;
            }

            if (IS_WIN_FLAG(win, ACTIVATE))
                WIDGET_FOREGROUND(win->_title) = white;
            else
                WIDGET_FOREGROUND(win->_title) = RTGUI_RGB(212, 208, 200);

            rect.x1 += 4;
            rect.y1 += 2;
            rect.y2 = rect.y1 + TITLE_CLOSE_BUTTON_HEIGHT;
            rtgui_dc_draw_text(dc, win->title, &rect);

            if (!IS_WIN_STYLE(win, CLOSEBOX)) break;

            /* get close button rect */
            rtgui_rect_move_align(&rect, &box_rect,
                RTGUI_ALIGN_CENTER_VERTICAL | RTGUI_ALIGN_RIGHT);
            box_rect.x1 -= 3;
            box_rect.x2 -= 3;
            // LOG_D("box (%d,%d)-(%d,%d)", box_rect.x1, box_rect.y1, box_rect.x2,
            //     box_rect.y2);

            /* draw close button */
            rtgui_dc_fill_rect(dc, &box_rect);
            if (IS_WIN_FLAG(win, CB_PRESSED)) {
                rtgui_dc_draw_border(dc, &box_rect, RTGUI_BORDER_SUNKEN);
                WIDGET_FOREGROUND(win->_title) = red;
                rtgui_dc_draw_word(dc, box_rect.x1, box_rect.y1 + 6, 7,
                    close_byte);
            } else {
                rtgui_dc_draw_border(dc, &box_rect, RTGUI_BORDER_RAISE);
                WIDGET_FOREGROUND(win->_title) = black;
                rtgui_dc_draw_word(dc, box_rect.x1 - 1, box_rect.y1 + 5, 7,
                    close_byte);
            }
        }
    } while (0);

    rtgui_dc_end_drawing(dc, RT_TRUE);
    LOG_W("draw theme done");
}

/* Public functions ----------------------------------------------------------*/
