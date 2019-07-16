/*
 * File      : progress.h
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
 * 2019-07-02     onelife      refactor
 */
#ifndef __RTGUI_PROGRESS_H__
#define __RTGUI_PROGRESS_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _PROGRESS_METADATA                  CLASS_METADATA(progress)
#define IS_PROGRESS(obj)                    IS_INSTANCE(obj, _PROGRESS_METADATA)
#define TO_PROGRESS(obj)                    CAST_(obj, _PROGRESS_METADATA, rtgui_progress_t)

#define CREATE_PROGRESS_INSTANCE(parent, hdl, rect, ort, rg) \
    rtgui_create_progress(TO_CONTAINER(parent), hdl, rect, ort, rg)
#define DELETE_PROGRESS_INSTANCE(obj)       DELETE_INSTANCE(obj)

#define PROGRESS_WIDTH_DEFAULT              (100)
#define PROGRESS_HEIGHT_DEFAULT             (20)
#define PROGRESS_RANGE_DEFAULT              (100)

/* Exported types ------------------------------------------------------------*/
struct rtgui_progress {
    rtgui_widget_t _super;
    rtgui_orient_t orient;
    rt_uint16_t range;
    rt_uint16_t value;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(progress);

#undef __RTGUI_PROGRESS_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rtgui_progress_t *rtgui_create_progress(rtgui_container_t *cntr,
    rtgui_evt_hdl_t hdl, rtgui_rect_t *rect, rtgui_orient_t orient,
    rt_uint16_t range);
MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_progress_t, progress, rt_uint16_t, range);
MEMBER_SETTER_GETTER_PROTOTYPE(rtgui_progress_t, progress, rt_uint16_t, value);

#endif /* IMPORT_TYPES */

#endif /* __RTGUI_PROGRESS_H__ */
