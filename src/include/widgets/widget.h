/*
 * File      : widget.h
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
 */
#ifndef __RTGUI_WIDGET_H__
#define __RTGUI_WIDGET_H__

#include "../rtgui.h"
// #include "../list.h"
#include "../region.h"
#include "../color.h"
#include "../font.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTGUI_WIDGET_FLAG_DEFAULT       0x0000
#define RTGUI_WIDGET_FLAG_SHOWN         0x0001
#define RTGUI_WIDGET_FLAG_DISABLE       0x0002
#define RTGUI_WIDGET_FLAG_FOCUS         0x0004
#define RTGUI_WIDGET_FLAG_TRANSPARENT   0x0008
#define RTGUI_WIDGET_FLAG_FOCUSABLE     0x0010
#define RTGUI_WIDGET_FLAG_DC_VISIBLE    0x0100
#define RTGUI_WIDGET_FLAG_IN_ANIM       0x0200

/* rtgui widget attribute */
#define RTGUI_WIDGET_FOREGROUND(w)      (TO_WIDGET(w)->gc.foreground)
#define RTGUI_WIDGET_BACKGROUND(w)      (TO_WIDGET(w)->gc.background)
#define RTGUI_WIDGET_TEXTALIGN(w)       (TO_WIDGET(w)->gc.textalign)
#define RTGUI_WIDGET_FONT(w)            (TO_WIDGET(w)->gc.font)
#define RTGUI_WIDGET_FLAG(w)            (TO_WIDGET(w)->flag)
#define RTGUI_WIDGET_ALIGN(w)           (TO_WIDGET(w)->align)
#define RTGUI_WIDGET_BORDER(w)          (TO_WIDGET(w)->border)
#define RTGUI_WIDGET_BORDER_STYLE(w)    (TO_WIDGET(w)->border_style)

#define RTGUI_WIDGET_UNHIDE(w)          RTGUI_WIDGET_FLAG(w) |= RTGUI_WIDGET_FLAG_SHOWN
#define RTGUI_WIDGET_HIDE(w)            RTGUI_WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_SHOWN
#define RTGUI_WIDGET_IS_HIDE(w)         (!(RTGUI_WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_SHOWN))

#define RTGUI_WIDGET_ENABLE(w)          RTGUI_WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_DISABLE
#define RTGUI_WIDGET_DISABLE(w)         RTGUI_WIDGET_FLAG(w) |= RTGUI_WIDGET_FLAG_DISABLE
#define RTGUI_WIDGET_IS_ENABLE(w)       (!((RTGUI_WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_DISABLE)))

#define RTGUI_WIDGET_UNFOCUS(w)         RTGUI_WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_FOCUS
#define RTGUI_WIDGET_FOCUS(w)           RTGUI_WIDGET_FLAG(w) |= RTGUI_WIDGET_FLAG_FOCUS
#define RTGUI_WIDGET_IS_FOCUSED(w)      (RTGUI_WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_FOCUS)

#define RTGUI_WIDGET_IS_FOCUSABLE(w)    (RTGUI_WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_FOCUSABLE)
#define RTGUI_WIDGET_SET_UNFOCUSABLE(w) RTGUI_WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_FOCUSABLE

#define RTGUI_WIDGET_IS_DC_VISIBLE(w)   (RTGUI_WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_DC_VISIBLE)
#define RTGUI_WIDGET_DC_SET_VISIBLE(w)  (RTGUI_WIDGET_FLAG(w) |= RTGUI_WIDGET_FLAG_DC_VISIBLE)
#define RTGUI_WIDGET_DC_SET_UNVISIBLE(w) (RTGUI_WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_DC_VISIBLE)
#define RTGUI_WIDGET_DC(w)              ((struct rtgui_dc*)&((w)->dc_type))

RTGUI_CLASS_PROTOTYPE(widget);

/** Gets the type of a widget */
#define _WIDGET_METADATA                    CLASS_METADATA(widget)
/** Casts the object to a rtgui_widget */
#define TO_WIDGET(obj)                      \
    RTGUI_CAST(obj, _WIDGET_METADATA, rtgui_widget_t)
/** Check if the object is a rtgui_widget */
#define IS_WIDGET(obj)                      IS_INSTANCE(obj, _WIDGET_METADATA)

// rtgui_widget_t *rtgui_widget_create(const rtgui_type_t *widget_type);
void rtgui_widget_destroy(rtgui_widget_t *widget);

/* focus and unfocus */
void rtgui_widget_focus(rtgui_widget_t *widget);
void rtgui_widget_unfocus(rtgui_widget_t *widget);

/* event handler for each command */
void rtgui_widget_set_onfocus(rtgui_widget_t *widget, rtgui_evt_hdl_t handler);
void rtgui_widget_set_onunfocus(rtgui_widget_t *widget, rtgui_evt_hdl_t handler);

/* get and set rect of widget */
void rtgui_widget_get_rect(rtgui_widget_t *widget, rtgui_rect_t *rect);
void rtgui_widget_set_border(rtgui_widget_t *widget, rt_uint32_t style);
void rtgui_widget_set_rect(rtgui_widget_t *widget, const rtgui_rect_t *rect);
void rtgui_widget_set_rectangle(rtgui_widget_t *widget, int x, int y, int width, int height);
void rtgui_widget_get_extent(rtgui_widget_t *widget, rtgui_rect_t *rect);

void rtgui_widget_set_minsize(rtgui_widget_t *widget, int width, int height);
void rtgui_widget_set_minwidth(rtgui_widget_t *widget, int width);
void rtgui_widget_set_minheight(rtgui_widget_t *widget, int height);

void rtgui_widget_set_parent(rtgui_widget_t *widget, rtgui_widget_t *_super);

/* get the physical position of a logic point on widget */
void rtgui_widget_point_to_device(rtgui_widget_t *widget, rtgui_point_t *point);
/* get the physical position of a logic rect on widget */
void rtgui_widget_rect_to_device(rtgui_widget_t *widget, rtgui_rect_t *rect);

/* get the logic position of a physical point on widget */
void rtgui_widget_point_to_logic(rtgui_widget_t *widget, rtgui_point_t *point);
/* get the logic position of a physical rect on widget */
void rtgui_widget_rect_to_logic(rtgui_widget_t *widget, rtgui_rect_t *rect);

void rtgui_widget_clip_parent(rtgui_widget_t *widget);
void rtgui_widget_clip_return(rtgui_widget_t *widget);

/* move widget and its children to a logic point */
void rtgui_widget_move_to_logic(rtgui_widget_t *widget, int dx, int dy);

/* update the clip info of widget */
void rtgui_widget_update_clip(rtgui_widget_t *widget);

/* get the toplevel widget of widget */
rtgui_win_t *rtgui_widget_get_toplevel(rtgui_widget_t *widget);
rt_bool_t rtgui_widget_onupdate_toplvl(rtgui_obj_t *object,
    rtgui_evt_generic_t *event);

void rtgui_widget_show(rtgui_widget_t *widget);
rt_bool_t rtgui_widget_onshow(rtgui_obj_t *object,
    rtgui_evt_generic_t *event);
void rtgui_widget_hide(rtgui_widget_t *widget);
rt_bool_t rtgui_widget_onhide(rtgui_obj_t *object,
    rtgui_evt_generic_t *event);
void rtgui_widget_update(rtgui_widget_t *widget);
rt_bool_t rtgui_widget_onpaint(rtgui_obj_t *object, rtgui_evt_base_t *event);

/* get _super color */
rtgui_color_t rtgui_widget_get_parent_foreground(rtgui_widget_t *widget);
rtgui_color_t rtgui_widget_get_parent_background(rtgui_widget_t *widget);

/* get the next sibling of widget */
rtgui_widget_t *rtgui_widget_get_next_sibling(rtgui_widget_t *widget);
/* get the prev sibling of widget */
rtgui_widget_t *rtgui_widget_get_prev_sibling(rtgui_widget_t *widget);

rt_bool_t rtgui_widget_is_in_animation(rtgui_widget_t *widget);

/* dump widget information */
void rtgui_widget_dump(rtgui_widget_t *widget);

#ifdef __cplusplus
}
#endif

#endif

