/*
 * File      : box.h
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
 * 2019-05-18     onelife      refactor
 */
#ifndef __TO_BOX_H__
#define __TO_BOX_H__

#include "../rtgui.h"
#include "./widget.h"
#include "./container.h"

#ifdef __cplusplus
extern "C" {
#endif

RTGUI_CLASS_PROTOTYPE(box);

/** Gets the type of a box */
#define _BOX_METADATA                       CLASS_METADATA(box)
/** Casts the object to an rtgui_box */
#define TO_BOX(obj)                         \
    RTGUI_CAST(obj, _BOX_METADATA, rtgui_box_t)
/** Checks if the object is an rtgui_box */
#define IS_BOX(obj)                         IS_INSTANCE(obj, _BOX_METADATA)

rtgui_box_t *rtgui_box_create(int orientation, int border_size);
void rtgui_box_destroy(rtgui_box_t *box);

void rtgui_box_layout(rtgui_box_t *box);
void rtgui_box_layout_rect(rtgui_box_t *box, struct rtgui_rect *rect);

#ifdef __cplusplus
}
#endif

#endif
