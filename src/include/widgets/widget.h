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
#include "../rtgui.h"
#include "../region.h"
// #include "../color.h"
// #include "../font/font.h"

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, wgt, rtgui_widget_t*, parent);
rtgui_win_t *rtgui_widget_get_toplevel(rtgui_widget_t *wgt);
void rtgui_widget_set_border(rtgui_widget_t *widget, rt_uint32_t style);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, int, min_width);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, int, min_height);
void rtgui_widget_set_minsize(rtgui_widget_t *widget, int width, int height);
MEMBER_GETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_rect_t, extent);
void rtgui_widget_set_rect(rtgui_widget_t *widget, const rtgui_rect_t *rect);
void rtgui_widget_set_rectangle(rtgui_widget_t *widget, int x, int y,
    int width, int height);
void rtgui_widget_get_rect(rtgui_widget_t *widget, rtgui_rect_t *rect);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_focus);
MEMBER_SETTER_PROTOTYPE(rtgui_widget_t, widget, rtgui_evt_hdl_t, on_unfocus);

rtgui_widget_t *rtgui_widget_get_next_sibling(rtgui_widget_t *widget);
rtgui_widget_t *rtgui_widget_get_prev_sibling(rtgui_widget_t *widget);
rtgui_color_t rtgui_widget_get_parent_foreground(rtgui_widget_t *widget);
rtgui_color_t rtgui_widget_get_parent_background(rtgui_widget_t *widget);

void rtgui_widget_clip_parent(rtgui_widget_t *widget);
void rtgui_widget_clip_return(rtgui_widget_t *widget);

/* get the physical position of a logic point on widget */
void rtgui_widget_point_to_device(rtgui_widget_t *widget, rtgui_point_t *point);
/* get the physical position of a logic rect on widget */
void rtgui_widget_rect_to_device(rtgui_widget_t *widget, rtgui_rect_t *rect);
/* get the logic position of a physical point on widget */
void rtgui_widget_point_to_logic(rtgui_widget_t *widget, rtgui_point_t *point);
/* get the logic position of a physical rect on widget */
void rtgui_widget_rect_to_logic(rtgui_widget_t *widget, rtgui_rect_t *rect);
/* move widget and its children to a logic point */
void rtgui_widget_move_to_logic(rtgui_widget_t *widget, int dx, int dy);

void rtgui_widget_focus(rtgui_widget_t *widget);
void rtgui_widget_unfocus(rtgui_widget_t *widget);
void rtgui_widget_update_clip(rtgui_widget_t *widget);
void rtgui_widget_show(rtgui_widget_t *widget);
void rtgui_widget_hide(rtgui_widget_t *widget);
void rtgui_widget_update(rtgui_widget_t *widget);
rt_bool_t rtgui_widget_onshow(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_onhide(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_onpaint(rtgui_obj_t *object, rtgui_evt_base_t *event);
rt_bool_t rtgui_widget_onupdate_toplvl(rtgui_obj_t *obj, void *param);
rt_bool_t rtgui_widget_is_in_animation(rtgui_widget_t *widget);
/* dump widget information */
void rtgui_widget_dump(rtgui_widget_t *widget);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_WIDGET_H__ */
