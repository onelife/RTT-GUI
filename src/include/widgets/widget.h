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

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _WIDGET_METADATA                    CLASS_METADATA(widget)
#define IS_WIDGET(obj)                      IS_INSTANCE(obj, _WIDGET_METADATA)
#define TO_WIDGET(obj)                      CAST_(obj, _WIDGET_METADATA, rtgui_widget_t)

#define WIDGET_ALIGN(w)                     (TO_WIDGET(w)->align)
#define WIDGET_ALIGN_CLEAR(w, fname)        WIDGET_ALIGN(w) &= ~RTGUI_ALIGN_##fname
#define WIDGET_ALIGN_SET(w, fname)          WIDGET_ALIGN(w) |= RTGUI_ALIGN_##fname
#define IS_WIDGET_ALIGN(w, fname)           (WIDGET_ALIGN(w) & RTGUI_ALIGN_##fname)

#define WIDGET_BORDER(w)                    (TO_WIDGET(w)->border)
#define WIDGET_BORDER_STYLE(w)              (TO_WIDGET(w)->border_style)
#define WIDGET_FOREGROUND(w)                (TO_WIDGET(w)->gc.foreground)
#define WIDGET_BACKGROUND(w)                (TO_WIDGET(w)->gc.background)
#define WIDGET_FONT(w)                      (TO_WIDGET(w)->gc.font)
#define WIDGET_TEXTALIGN(w)                 (TO_WIDGET(w)->gc.textalign)
#define WIDGET_TEXTALIGN_CLEAR(w, fname)    WIDGET_TEXTALIGN(w) &= ~RTGUI_ALIGN_##fname
#define WIDGET_TEXTALIGN_SET(w, fname)      WIDGET_TEXTALIGN(w) |= RTGUI_ALIGN_##fname
#define IS_WIDGET_TEXTALIGN(w, fname)      (WIDGET_TEXTALIGN(w) & RTGUI_ALIGN_##fname)
#define WIDGET_DC(w)                        ((rtgui_dc_t *)&((w)->dc_type))

#define WIDGET_FLAG(w)                      (TO_WIDGET(w)->flag)
#define WIDGET_FLAG_CLEAR(w, fname)         WIDGET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_##fname
#define WIDGET_FLAG_SET(w, fname)           WIDGET_FLAG(w) |= RTGUI_WIDGET_FLAG_##fname
#define IS_WIDGET_FLAG(w, fname)            (WIDGET_FLAG(w) & RTGUI_WIDGET_FLAG_##fname)

#define WIDGET_SETTER(mname)                rtgui_widget_set_##mname
#define WIDGET_GETTER(mname)                rtgui_widget_get_##mname

/* Exported types ------------------------------------------------------------*/
typedef enum rtgui_widget_flag {
    RTGUI_WIDGET_FLAG_DEFAULT               = 0x0000,
    RTGUI_WIDGET_FLAG_SHOWN                 = 0x0001,
    RTGUI_WIDGET_FLAG_DISABLE               = 0x0002,
    RTGUI_WIDGET_FLAG_FOCUS                 = 0x0004,
    RTGUI_WIDGET_FLAG_TRANSPARENT           = 0x0008,
    RTGUI_WIDGET_FLAG_FOCUSABLE             = 0x0010,
    RTGUI_WIDGET_FLAG_DC_VISIBLE            = 0x0100,
    RTGUI_WIDGET_FLAG_IN_ANIM               = 0x0200,
} rtgui_widget_flag_t;

struct rtgui_widget {
    rtgui_obj_t _super;                     /* super class */
    rtgui_widget_t *parent;                 /* parent widget */
    rtgui_win_t *toplevel;                  /* parent window */
    rt_slist_t sibling;                     /* children and sibling */
    rt_uint32_t flag;
    rt_uint32_t align;
    rt_uint16_t border;
    rt_uint16_t border_style;
    rt_int16_t min_width, min_height;
    rtgui_rect_t extent;
    rtgui_rect_t extent_visiable;           /* including children */
    rtgui_region_t clip;
    rtgui_evt_hdl_t on_focus;
    rtgui_evt_hdl_t on_unfocus;
    rtgui_gc_t gc;                          /* graphic context */
    rt_ubase_t dc_type;                     /* hardware device context */
    const rtgui_dc_engine_t *dc_engine;     // TODO(onelife): struct rtgui_dc
    rt_uint32_t user_data;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(widget);

#undef __RTGUI_WIDGET_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, wgt, rtgui_widget_t*, parent);
rtgui_win_t *rtgui_widget_get_toplevel(rtgui_widget_t *wgt);
void rtgui_widget_set_border(rtgui_widget_t *wgt, rt_uint32_t style);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rt_int16_t, min_width);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rt_int16_t, min_height);
void rtgui_widget_set_minsize(rtgui_widget_t *wgt, rt_int16_t w, rt_int16_t h);
MEMBER_GETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_rect_t, extent);
void rtgui_widget_set_rect(rtgui_widget_t *wgt, const rtgui_rect_t *rect);
void rtgui_widget_set_rectangle(rtgui_widget_t *wgt, rt_int16_t x, rt_int16_t y,
    rt_int16_t w, rt_int16_t h);
void rtgui_widget_get_rect(rtgui_widget_t *wgt, rtgui_rect_t *rect);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_focus);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_unfocus);

rtgui_widget_t *rtgui_widget_get_next_sibling(rtgui_widget_t *wgt);
rtgui_widget_t *rtgui_widget_get_prev_sibling(rtgui_widget_t *wgt);
rtgui_color_t rtgui_widget_get_parent_foreground(rtgui_widget_t *wgt);
rtgui_color_t rtgui_widget_get_parent_background(rtgui_widget_t *wgt);

void rtgui_widget_clip_parent(rtgui_widget_t *wgt);
void rtgui_widget_clip_return(rtgui_widget_t *wgt);

/* get the physical position of a logic point on widget */
void rtgui_widget_point_to_device(rtgui_widget_t *wgt, rtgui_point_t *point);
/* get the physical position of a logic rect on widget */
void rtgui_widget_rect_to_device(rtgui_widget_t *wgt, rtgui_rect_t *rect);
/* get the logic position of a physical point on widget */
void rtgui_widget_point_to_logic(rtgui_widget_t *wgt, rtgui_point_t *point);
/* get the logic position of a physical rect on widget */
void rtgui_widget_rect_to_logic(rtgui_widget_t *wgt, rtgui_rect_t *rect);
/* move widget and its children to a logic point */
void rtgui_widget_move_to_logic(rtgui_widget_t *wgt, int dx, int dy);

void rtgui_widget_focus(rtgui_widget_t *wgt);
void rtgui_widget_unfocus(rtgui_widget_t *wgt);
void rtgui_widget_update_clip(rtgui_widget_t *wgt);
void rtgui_widget_show(rtgui_widget_t *wgt);
void rtgui_widget_hide(rtgui_widget_t *wgt);
void rtgui_widget_update(rtgui_widget_t *wgt);
rt_bool_t rtgui_widget_onshow(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_onhide(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_onpaint(rtgui_obj_t *obj, rtgui_evt_generic_t *evt);
rt_bool_t rtgui_widget_onupdate_toplvl(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_is_in_animation(rtgui_widget_t *wgt);
/* dump widget information */
void rtgui_widget_dump(rtgui_widget_t *wgt);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_WIDGET_H__ */
