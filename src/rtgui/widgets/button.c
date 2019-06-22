/*
 * File      : button.c
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
 * 2019-06-18     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/region.h"
#include "include/dc.h"
#include "include/images/image.h"
#include "include/widgets/button.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_BTN"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void _button_constructor(void *obj);
static void _button_destructor(void *obj);
static rt_bool_t _button_event_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t _button_on_unfocus(void *obj, rtgui_evt_generic_t *evt);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    button,
    CLASS_METADATA(label),
    _button_constructor,
    _button_destructor,
    _button_event_handler,
    sizeof(rtgui_button_t));

/* Private functions ---------------------------------------------------------*/
static void _button_constructor(void *obj) {
    rtgui_button_t *btn = obj;

    WIDGET_FOREGROUND(btn) = default_foreground;
    WIDGET_BACKGROUND(btn) = default_background;
    WIDGET_TEXTALIGN(btn) = \
        RTGUI_ALIGN_CENTER_HORIZONTAL | RTGUI_ALIGN_CENTER_VERTICAL;

    WIDGET_FLAG_SET(btn, FOCUSABLE);
    WIDGET_SETTER(on_unfocus)(TO_WIDGET(btn), _button_on_unfocus);

    btn->flag = RTGUI_BUTTON_FLAG_TYPE_NORMAL;
    btn->press_img = RT_NULL;
    btn->unpress_img = RT_NULL;
    btn->on_button = RT_NULL;
}

static void _button_destructor(void *obj) {
    rtgui_button_t *btn = obj;

    if (btn->press_img) {
        rtgui_image_destroy(btn->press_img);
        btn->press_img = RT_NULL;
    }

    if (btn->unpress_img) {
        rtgui_image_destroy(btn->unpress_img);
        btn->unpress_img = RT_NULL;
    }
}

static rt_bool_t _button_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_button_t *btn = obj;
    rt_bool_t done = RT_FALSE;

    EVT_LOG("[BtnEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        rtgui_theme_draw_button(btn);
        break;

    case RTGUI_EVENT_KBD:
        if (!IS_WIDGET_FLAG(btn, SHOWN) || IS_WIDGET_FLAG(btn, DISABLE))
            break;
        if (!IS_KBD_EVENT_KEY(evt, RETURN) && !IS_KBD_EVENT_KEY(evt, SPACE))
            break;

        if (IS_KBD_EVENT_TYPE(evt, DOWN))
            BUTTON_FLAG_SET(btn, PRESS);
        else
            BUTTON_FLAG_CLEAR(btn, PRESS);

        rtgui_widget_update(TO_WIDGET(btn));

        if (!IS_BUTTON_FLAG(btn, PRESS) && btn->on_button)
            (void)btn->on_button(TO_OBJECT(btn), evt);
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
        if (!IS_WIDGET_FLAG(btn, SHOWN) || IS_WIDGET_FLAG(btn, DISABLE))
            break;

        if (!rtgui_rect_contains_point(&(TO_WIDGET(btn)->extent),
            evt->mouse.x, evt->mouse.y)) {
            /* not on this btn */
            BUTTON_FLAG_CLEAR(btn, PRESS);
            LOG_D("unpress btn");
            rtgui_widget_update(TO_WIDGET(btn));
            break;
        }
        done = RT_TRUE;

        if (IS_BUTTON_FLAG(btn, TYPE_PUSH)) {
            if (!IS_MOUSE_EVENT_BUTTON(evt, UP)) break;

            if (IS_BUTTON_FLAG(btn, PRESS))
                BUTTON_FLAG_CLEAR(btn, PRESS);
            else
                BUTTON_FLAG_SET(btn, PRESS);

            LOG_D("push btn press: %d", IS_BUTTON_FLAG(btn, PRESS));
            rtgui_widget_update(TO_WIDGET(btn));
            if (btn->on_button)
                (void)btn->on_button(TO_OBJECT(btn), evt);
        } else {
            rtgui_win_t *win;
            rt_bool_t do_call;

            if (!IS_MOUSE_EVENT_BUTTON(evt, LEFT)) break;

            win = TO_WIDGET(btn)->toplevel;
            /* we need to decide whether the callback will be invoked
             * before the flag has changed. Moreover, we cannot invoke
             * it directly here, because the button might be destroyed
             * in the callback. If that happens, program will crash on
             * the following code. We need to make sure that the
             * callbacks are invoke at the very last step. */
            do_call = IS_BUTTON_FLAG(btn, PRESS) && \
                      IS_MOUSE_EVENT_BUTTON(evt, UP);

            /* if the button will handle the mouse up event here, it
             * should not be the last_mouse. Take care that
             * don't overwrite other widgets. */
            if (IS_MOUSE_EVENT_BUTTON(evt, DOWN)) {
                BUTTON_FLAG_SET(btn, PRESS);
                win->last_mouse = TO_WIDGET(btn);
            } else {
                BUTTON_FLAG_CLEAR(btn, PRESS);
                if (win->last_mouse == TO_WIDGET(btn))
                    win->last_mouse = RT_NULL;
            }

            LOG_W("btn press: %d", IS_BUTTON_FLAG(btn, PRESS));
            rtgui_widget_update(TO_WIDGET(btn));
            if (do_call && btn->on_button)
                btn->on_button(TO_WIDGET(btn), evt);
        }
        break;

    default:
        if (SUPER_CLASS_HANDLER(button))
            done = SUPER_CLASS_HANDLER(button)(btn, evt);
        break;
    }

    EVT_LOG("[BtnEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static rt_bool_t _button_on_unfocus(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_dc_t *dc;
    rt_bool_t done = RT_FALSE;
    (void)evt;

    do {
        rtgui_widget_t *wgt;
        rtgui_gc_t *gc;
        rtgui_rect_t rect;
        rtgui_color_t fc;

        wgt = TO_WIDGET(obj);
        dc = rtgui_dc_begin_drawing(wgt);
        if (!dc) {
            LOG_E("no dc");
            break;
        }

        done = RT_TRUE;
        if (IS_WIDGET_FLAG(wgt, FOCUS)) break;

        rtgui_widget_get_rect(wgt, &rect);

        /* only clear focus rect */
        rtgui_rect_inflate(&rect, -BORDER_SIZE_DEFAULT);
        gc = rtgui_dc_get_gc(dc);
        fc = gc->foreground;
        gc->foreground = gc->background;
        rtgui_dc_draw_focus_rect(dc, &rect);
        gc->foreground = fc;

    } while (0);

    rtgui_dc_end_drawing(dc, RT_TRUE);
    return done;
}

/* Public functions ----------------------------------------------------------*/
RTGUI_MEMBER_SETTER(rtgui_button_t, button, rtgui_image_t*, press_img);
RTGUI_MEMBER_SETTER(rtgui_button_t, button, rtgui_image_t*, unpress_img);
RTGUI_MEMBER_SETTER(rtgui_button_t, button, rtgui_evt_hdl_t, on_button);

void rtgui_theme_draw_button(rtgui_button_t *btn) {
    do {
        struct rtgui_dc *dc;
        struct rtgui_rect rect;

        dc = rtgui_dc_begin_drawing(TO_WIDGET(btn));
        if (!dc) {
            LOG_E("no dc");
            break;
        }

        /* get widget rect */
        rtgui_widget_get_rect(TO_WIDGET(btn), &rect);
        LOG_W("draw button (%d,%d)-(%d, %d)", rect.x1, rect.y1, rect.x2,
            rect.y2);

        /* draw image or border */
        rtgui_dc_fill_rect(dc, &rect);
        if (IS_BUTTON_FLAG(btn, PRESS)) {
            if (btn->press_img) {
                rtgui_rect_t rect_img;
                rect_img.x1 = 0;
                rect_img.y1 = 0;
                rect_img.x2 = btn->unpress_img->w;
                rect_img.y2 = btn->unpress_img->h;
                rtgui_rect_move_align(&rect, &rect_img, RTGUI_ALIGN_CENTER);
                rtgui_image_blit(btn->press_img, dc, &rect_img);
            } else {
                rtgui_dc_draw_border(dc, &rect, RTGUI_BORDER_SUNKEN);
            }
        } else {
            if (btn->unpress_img) {
                rtgui_rect_t rect_img;
                rect_img.x1 = 0;
                rect_img.y1 = 0;
                rect_img.x2 = btn->unpress_img->w;
                rect_img.y2 = btn->unpress_img->h;
                rtgui_rect_move_align(&rect, &rect_img, RTGUI_ALIGN_CENTER);
                rtgui_image_blit(btn->unpress_img, dc, &rect_img);
            } else {
                rtgui_dc_draw_border(dc, &rect, RTGUI_BORDER_RAISE);
            }
        }

        if (IS_WIDGET_FLAG(btn, FOCUS)) {
            rtgui_color_t fc;
            /* reset foreground and get default rect */
            rtgui_widget_get_rect(TO_WIDGET(btn), &rect);
            rtgui_rect_inflate(&rect, -BORDER_SIZE_DEFAULT);

            fc = WIDGET_FOREGROUND(btn);

            WIDGET_FOREGROUND(btn) = black;
            rtgui_dc_draw_focus_rect(dc, &rect);

            WIDGET_FOREGROUND(btn) = fc;
        }

        if (!btn->press_img) {
            /* reset foreground and get default rect */
            rtgui_widget_get_rect(TO_WIDGET(btn), &rect);
            /* remove border */
            rtgui_rect_inflate(&rect, -BORDER_SIZE_DEFAULT);

            if (IS_WIDGET_FLAG(btn, DISABLE)) {
                rtgui_color_t fc;

                fc = WIDGET_FOREGROUND(btn);

                /* draw disable text */
                WIDGET_FOREGROUND(btn) = white;
                rtgui_rect_move(&rect, 1, 1);
                rtgui_dc_draw_text(dc,
                    MEMBER_GETTER(label, text)(TO_LABEL(btn)),
                    &rect);

                WIDGET_FOREGROUND(btn) = dark_grey;
                rtgui_rect_move(&rect, -1, -1);
                rtgui_dc_draw_text(dc,
                     MEMBER_GETTER(label, text)(TO_LABEL(btn)),
                     &rect);

                WIDGET_FOREGROUND(btn) = fc;
            } else {
                /* draw text */
                rtgui_dc_draw_text(dc,
                    MEMBER_GETTER(label, text)(TO_LABEL(btn)),
                    &rect);
            }
        }

        rtgui_dc_end_drawing(dc, RT_TRUE);
        LOG_W("draw button done");
    } while (0);
}
