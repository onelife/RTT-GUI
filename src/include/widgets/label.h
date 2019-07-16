/*
 * File      : label.h
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
 * 2019-06-18     onelife      refactor
 */
#ifndef __RTGUI_LABEL_H__
#define __RTGUI_LABEL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _LABEL_METADATA                     CLASS_METADATA(label)
#define IS_LABEL(obj)                       IS_INSTANCE(obj, _LABEL_METADATA)
#define TO_LABEL(obj)                       CAST_(obj, _LABEL_METADATA, rtgui_label_t)

#define CREATE_LABEL_INSTANCE(parent, hdl, rect, text) \
    rtgui_create_label(TO_CONTAINER(parent), hdl, rect, text)
#define DELETE_LABEL_INSTANCE(obj)          DELETE_INSTANCE(obj)

#define LABEL_GETTER(mname)                 rtgui_label_get_##mname

/* Exported types ------------------------------------------------------------*/
struct rtgui_label {
    rtgui_widget_t _super;
    char *text;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(label);

#undef __RTGUI_LABEL_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rt_err_t *rtgui_label_init(rtgui_label_t *lab, const char *text);
rtgui_label_t *rtgui_create_label(rtgui_container_t *cntr, rtgui_evt_hdl_t hdl,
    rtgui_rect_t *rect, const char *text);
void rtgui_label_set_text(rtgui_label_t *lab, const char *text);
MEMBER_GETTER_PROTOTYPE(rtgui_label_t, label, char*, text);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_LABEL_H__ */
