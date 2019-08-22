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

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/filerw.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_image_engine rtgui_image_engine_t;
typedef struct rtgui_image_palette rtgui_image_palette_t;
typedef struct rtgui_image rtgui_image_t;

struct rtgui_image_engine {
    const char *name;
    rt_slist_t list;
    /* image engine function */
    rt_bool_t (*image_check)(rtgui_filerw_t *file);
    rt_bool_t (*image_load)(rtgui_image_t *image, rtgui_filerw_t *file,
        rt_int32_t scale, rt_bool_t load);
    void (*image_unload)(rtgui_image_t *image);
    void (*image_blit)(rtgui_image_t *image, rtgui_dc_t *dc,
        rtgui_rect_t *rect);
};

struct rtgui_image_palette {
    rtgui_color_t *colors;
    rt_uint32_t ncolors;
};

struct rtgui_image {
    /* image metrics */
    rt_uint16_t w, h;
    /* image engine */
    const rtgui_image_engine_t *engine;
    /* image palette */
    rtgui_image_palette_t *palette;
    /* image private data */
    void *data;
};

/* Exported constants --------------------------------------------------------*/

#undef __RTGUI_IMAGE_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_system_image_init(void);

#ifdef GUIENGINE_USING_DFS_FILERW
rtgui_image_engine_t *rtgui_image_get_engine_by_filename(const char *fn);
rtgui_image_t *rtgui_image_create_from_file(const char *type, const char *fn,
    rt_int32_t scale, rt_bool_t load);
rtgui_image_t *rtgui_image_create(const char *fn, rt_int32_t scale,
    rt_bool_t load);
#endif
rtgui_image_t *rtgui_image_create_from_mem(const char *type,
    const rt_uint8_t *data, rt_size_t size, rt_int32_t scale, rt_bool_t load);
void rtgui_image_destroy(rtgui_image_t *image);

/* get image's rect */
void rtgui_image_get_rect(rtgui_image_t *image, rtgui_rect_t *rect);

/* register an image engine */
rt_err_t rtgui_image_register_engine(rtgui_image_engine_t *engine);

/* blit an image on DC */
void rtgui_image_blit(rtgui_image_t *image, rtgui_dc_t *dc, rtgui_rect_t *rect);
rtgui_image_palette_t *rtgui_image_palette_create(rt_uint32_t ncolors);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_IMAGE_H__ */

