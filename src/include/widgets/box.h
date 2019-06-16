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


#define CREATE_BOX_INSTANCE(obj, orient, border_sz) \
    do {                                    \
        obj = (rtgui_box_t *)CREATE_INSTANCE(box, RT_NULL); \
        if (obj) {                          \
            box->orient = orient;           \
            box->border_size = border_sz;   \
        }                                   \
    } while (0)


void rtgui_box_layout(rtgui_box_t *box);
void rtgui_box_layout_rect(rtgui_box_t *box, rtgui_rect_t *rect);

#ifdef __cplusplus
}
#endif

#endif
