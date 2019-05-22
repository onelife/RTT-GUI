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
 */
#ifndef __TO_CONTAINER_H__
#define __TO_CONTAINER_H__

#include "./widget.h"
#include "./box.h"

#ifdef __cplusplus
extern "C" {
#endif

RTGUI_CLASS_PROTOTYPE(container);
/** Gets the type of a container */
#define _CONTAINER_METADATA                 CLASS_METADATA(container)
/** Casts the object to an rtgui_container */
#define TO_CONTAINER(obj)                   \
    RTGUI_CAST(obj, _CONTAINER_METADATA, rtgui_container_t)
/** Checks if the object is an rtgui_container */
#define IS_CONTAINER(obj)                   \
    IS_INSTANCE((obj), _CONTAINER_METADATA)

void rtgui_container_destroy(rtgui_container_t *container);

/* set layout box */
void rtgui_container_set_box(struct rtgui_container *container, rtgui_box_t *box);
void rtgui_container_layout(struct rtgui_container *container);

void rtgui_container_add_child(rtgui_container_t *container, rtgui_widget_t *child);
void rtgui_container_remove_child(rtgui_container_t *container, rtgui_widget_t *child);
void rtgui_container_destroy_children(rtgui_container_t *container);
rtgui_widget_t *rtgui_container_get_first_child(rtgui_container_t *container);

rt_bool_t rtgui_container_dispatch_mouse_event(rtgui_container_t *container,
    rtgui_evt_generic_t *event);

rtgui_obj_t* rtgui_container_get_object(struct rtgui_container *container, rt_uint32_t id);
#ifdef __cplusplus
}
#endif

#endif
