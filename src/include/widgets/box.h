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
#ifndef __RTGUI_BOX_H__
#define __RTGUI_BOX_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _BOX_METADATA                       CLASS_METADATA(box)
#define IS_BOX(obj)                         IS_INSTANCE(obj, _BOX_METADATA)
#define TO_BOX(obj)                         CAST_(obj, _BOX_METADATA, rtgui_box_t)

#define CREATE_BOX_INSTANCE(obj, ort, sz)   \
    do {                                    \
        obj = (rtgui_box_t *)CREATE_INSTANCE(box, RT_NULL); \
        if (obj) {                          \
            box->orient = ort;              \
            box->border_sz = sz;            \
        }                                   \
    } while (0)

/* Exported types ------------------------------------------------------------*/
struct rtgui_box {
    rtgui_obj_t _super;
    rt_uint16_t orient;
    rt_uint16_t border_sz;
    rtgui_container_t *container;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(box);

#undef __RTGUI_BOX_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
void rtgui_box_layout(rtgui_box_t *box);
void rtgui_box_layout_rect(rtgui_box_t *box, rtgui_rect_t *rect);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_BOX_H__ */
