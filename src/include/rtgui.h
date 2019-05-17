/*
 * File      : rtgui.h
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
#ifndef __RT_GUI_H__
#define __RT_GUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTGUI_RESTRICT      __restrict
#ifdef _MSC_VER
# define RTGUI_PURE
#else
/* GCC and MDK share the same attributes.
 * TODO: IAR attributes. */
# define RTGUI_PURE __attribute__((pure))
#endif

#include "./rtgui_config.h"
#include "./types.h"
#include "./event.h"
#include "./list.h"
#include "./rtgui_object.h"

#define _UI_MIN(x, y)                       (((x)<(y))?(x):(y))
#define _UI_MAX(x, y)                       (((x)>(y))?(x):(y))
#define _UI_BITBYTES(bits)                  ((bits + 7)/8)
#define _UI_ABS(x)                          ((x)>=0? (x):-(x))

#define rtgui_rect_width(r)                 ((r).x2 - (r).x1)
#define rtgui_rect_height(r)                ((r).y2 - (r).y1)

extern struct rt_mempool *rtgui_event_pool;

extern rtgui_point_t rtgui_empty_point;

#ifdef __cplusplus
}
#endif

#endif

