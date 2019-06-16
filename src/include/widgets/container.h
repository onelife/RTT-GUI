/*
 * File      : container.h
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
 * 2019-06-15     onelife      refactor
 */
#ifndef __TO_CONTAINER_H__
#define __TO_CONTAINER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "./widget.h"
#include "./box.h"

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rt_bool_t rtgui_container_dispatch_mouse_event(rtgui_container_t *container,
    rtgui_evt_generic_t *event);
void rtgui_container_add_child(rtgui_container_t *cntr, rtgui_widget_t *child);
void rtgui_container_remove_child(rtgui_container_t *cntr,
    rtgui_widget_t *child);
rtgui_widget_t *rtgui_container_get_first_child(rtgui_container_t *cntr);
void rtgui_container_set_box(rtgui_container_t *container, rtgui_box_t *box);
void rtgui_container_layout(rtgui_container_t *container);
rtgui_obj_t* rtgui_container_get_object(rtgui_container_t *cntr,
    rt_uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* __TO_CONTAINER_H__ */
