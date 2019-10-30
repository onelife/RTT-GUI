/*
 * File      : label.c
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
#include "include/font/font.h"
#include "include/widgets/container.h"
#include "include/widgets/label.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_LAB"
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
static void _label_constructor(void *obj);
static void _label_destructor(void *obj);
static rt_bool_t _label_event_handler(void *obj, rtgui_evt_generic_t *evt);
static void _theme_draw_label(rtgui_label_t *lab);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    label,
    CLASS_METADATA(widget),
    _label_constructor,
    _label_destructor,
    _label_event_handler,
    sizeof(rtgui_label_t));

/* Private functions ---------------------------------------------------------*/
static void _label_constructor(void *obj) {
    rtgui_label_t *lab = obj;

    WIDGET_TEXTALIGN(lab) = RTGUI_ALIGN_LEFT | RTGUI_ALIGN_CENTER_VERTICAL;
    lab->text = RT_NULL;
}

static void _label_destructor(void *obj) {
    rtgui_label_t *lab = obj;

    if (lab->text) rtgui_free(lab->text);
    lab->text = RT_NULL;
}

static rt_bool_t _label_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_label_t *lab = obj;
    rt_bool_t done = RT_FALSE;

    EVT_LOG("[LabEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        _theme_draw_label(lab);
        break;

    default:
        if (SUPER_CLASS_HANDLER(label))
            done = SUPER_CLASS_HANDLER(label)(lab, evt);
        break;
    }

    EVT_LOG("[LabEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static void _theme_draw_label(rtgui_label_t *lab) {
    do {
        rtgui_dc_t *dc;
        rtgui_rect_t rect;

        dc = rtgui_dc_begin_drawing(TO_WIDGET(lab));
        if (!dc) {
            LOG_W("no dc");
            break;
        }

        rtgui_widget_get_rect(TO_WIDGET(lab), &rect);
        LOG_D("draw label (%d,%d)-(%d,%d)", rect.x1, rect.y1, rect.x2,
            rect.y2);
        rtgui_dc_fill_rect(dc, &rect);
        rtgui_dc_draw_text(dc, LABEL_GETTER(text)(lab), &rect);

        rtgui_dc_end_drawing(dc, RT_TRUE);
        LOG_D("draw label done");
    } while (0);
}

/* Public functions ----------------------------------------------------------*/
rt_err_t *rtgui_label_init(rtgui_label_t *lab, const char *text) {
    rtgui_rect_t rect;

    /* set min size */
    rtgui_font_get_metrics(rtgui_font_default(), text, &rect);
    TO_WIDGET(lab)->min_width  = RECT_W(rect);
    TO_WIDGET(lab)->min_height = RECT_H(rect);
    /* set text */
    lab->text = (char *)rt_strdup(text);

    return RT_EOK;
}
RTM_EXPORT(rtgui_label_init);

rtgui_label_t *rtgui_create_label(rtgui_container_t *cntr, rtgui_evt_hdl_t hdl,
    rtgui_rect_t *rect, const char *text) {
    rtgui_label_t *lab;

    do {
        lab = CREATE_INSTANCE(label, hdl);
        if (!lab) break;
        if (rtgui_label_init(lab, text)) {
            DELETE_INSTANCE(lab);
            break;
        }
        if (rect)
            rtgui_widget_set_rect(TO_WIDGET(lab), rect);
        if (cntr)
            rtgui_container_add_child(cntr, TO_WIDGET(lab));
    } while (0);

    return lab;
}

void rtgui_label_set_text(rtgui_label_t *lab, const char *text) {
    if (lab->text) {
        /* check if changed */
        if (!rt_strcmp(text, lab->text)) return;
        rtgui_free(lab->text);
        lab->text = RT_NULL;
    }

    if (text)
        lab->text = rt_strdup(text);

    /* update widget */
    rtgui_widget_update(TO_WIDGET(lab));
}
RTM_EXPORT(rtgui_label_set_text);

RTGUI_MEMBER_GETTER(rtgui_label_t, label, char*, text);
