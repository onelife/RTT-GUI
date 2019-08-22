/*
 * File      : picture.h
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
 * 2019-08-19     onelife      first version
 */
#ifndef __RTGUI_PICTURE_H__
#define __RTGUI_PICTURE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _PICTURE_METADATA                   CLASS_METADATA(picture)
#define IS_PICTURE(obj)                     IS_INSTANCE(obj, _PICTURE_METADATA)
#define TO_PICTURE(obj)                     CAST_(obj, _PICTURE_METADATA, rtgui_picture_t)

#define CREATE_PICTURE_INSTANCE(parent, hdl, rect, path, align, resize) \
    rtgui_create_picture(TO_CONTAINER(parent), hdl, rect, path, RTGUI_ALIGN_##align, resize)
#define DELETE_PICTURE_INSTANCE(obj)        DELETE_INSTANCE(obj)

#define PICTURE_SETTER(mname)               rtgui_picture_set_##mname
#define PICTURE_GETTER(mname)               rtgui_picture_get_##mname

/* Exported types ------------------------------------------------------------*/
struct rtgui_picture {
    rtgui_widget_t _super;
    char *path;
    rtgui_image_t *image;
    rt_uint32_t align;
    rt_bool_t resize;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(picture);

#undef __RTGUI_PICTURE_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rt_err_t *rtgui_picture_init(rtgui_picture_t *pic, rt_uint32_t align,
    rt_bool_t resize);
rtgui_picture_t *rtgui_create_picture(rtgui_container_t *cntr,
    rtgui_evt_hdl_t hdl, rtgui_rect_t *rect, const char *path,
    rt_uint32_t align, rt_bool_t resize);
void rtgui_picture_set_path(rtgui_picture_t *pic, const char *path);
MEMBER_GETTER_PROTOTYPE(rtgui_picture_t, picture, char*, path);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_PICTURE_H__ */
