/*
 * File      : image.h
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
#ifndef __RTGUI_IMAGE_H__
#define __RTGUI_IMAGE_H__

#include "include/types.h"
#include "include/dc.h"
#include "include/region.h"
#include "include/filerw.h"

#ifdef __cplusplus
extern "C" {
#endif

/* init rtgui image system */
rt_err_t rtgui_system_image_init(void);

#if defined(GUIENGINE_USING_DFS_FILERW)
rtgui_image_engine_t *rtgui_image_get_engine_by_filename(const char *fn);
rtgui_image_t *rtgui_image_create_from_file(const char *type, const char *filename, rt_bool_t load);
rtgui_image_t *rtgui_image_create(const char *filename, rt_bool_t load);
#endif

rtgui_image_t *rtgui_image_create_from_mem(const char *type, const rt_uint8_t *data, rt_size_t length, rt_bool_t load);
void rtgui_image_destroy(rtgui_image_t *image);

/* get image's rect */
void rtgui_image_get_rect(rtgui_image_t *image, rtgui_rect_t *rect);

/* register an image engine */
rt_err_t rtgui_image_register_engine(rtgui_image_engine_t *engine);

/* blit an image on DC */
void rtgui_image_blit(rtgui_image_t *image, rtgui_dc_t *dc, rtgui_rect_t *rect);
rtgui_image_palette_t *rtgui_image_palette_create(rt_uint32_t ncolors);

#ifdef __cplusplus
}
#endif

#endif

