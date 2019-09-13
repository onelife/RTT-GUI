/*
 * File      : driver.h
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
#ifndef __RTGUI_DRIVER_H__
#define __RTGUI_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_gfx_driver rtgui_gfx_driver_t;

/* graphic driver operations */
struct rtgui_graphic_driver_ops {
    void (*set_pixel)(rtgui_color_t *c, int x, int y);
    void (*get_pixel)(rtgui_color_t *c, int x, int y);
    void (*draw_hline)(rtgui_color_t *c, int x1, int x2, int y);
    void (*draw_vline)(rtgui_color_t *c, int x , int y1, int y2);
    void (*draw_raw_hline)(rt_uint8_t *pixels, int x1, int x2, int y);
};

/* graphic extension operations */
struct rtgui_graphic_ext_ops {
    void (*draw_line)(rtgui_color_t *c, int x1, int y1, int x2, int y2);
    void (*draw_rect)(rtgui_color_t *c, int x1, int y1, int x2, int y2);
    void (*fill_rect)(rtgui_color_t *c, int x1, int y1, int x2, int y2);
    void (*draw_circle)(rtgui_color_t *c, int x, int y, int r);
    void (*fill_circle)(rtgui_color_t *c, int x, int y, int r);
    void (*draw_ellipse)(rtgui_color_t *c, int x, int y, int rx, int ry);
    void (*fill_ellipse)(rtgui_color_t *c, int x, int y, int rx, int ry);
};

struct rtgui_gfx_driver {
    /* pixel format and byte per pixel */
    rt_uint8_t pixel_format;
    rt_uint8_t bits_per_pixel;
    rt_uint16_t pitch;
    /* screen width and height */
    rt_uint16_t width;
    rt_uint16_t height;
    /* framebuffer address and ops */
    rt_uint8_t *framebuffer;
    struct rt_device* device;
    const struct rtgui_graphic_driver_ops *ops;
    const struct rtgui_graphic_ext_ops *ext_ops;
};

#ifdef RTGUI_USING_HW_CURSOR
    enum rtgui_cursor_type {
        RTGUI_CURSOR_ARROW,
        RTGUI_CURSOR_HAND,
    };
#endif

/* Exported constants --------------------------------------------------------*/

#undef __RTGUI_DRIVER_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_set_gfx_device(rt_device_t dev);
REFERENCE_GETTER_PROTOTYPE(gfx_device, rtgui_gfx_driver_t);
void rtgui_gfx_get_rect(const rtgui_gfx_driver_t *driver, rtgui_rect_t *rect);
void rtgui_gfx_update_screen(const rtgui_gfx_driver_t *driver, rtgui_rect_t *rect);
rt_uint8_t *rtgui_gfx_get_framebuffer(const rtgui_gfx_driver_t *driver);

#ifdef CONFIG_TOUCH_DEVICE_NAME
    rt_err_t rtgui_set_touch_device(rt_device_t dev);
    GETTER_PROTOTYPE(touch_device, rt_device_t);
#endif /* CONFIG_TOUCH_DEVICE_NAME */

#ifdef CONFIG_KEY_DEVICE_NAME
    rt_err_t rtgui_set_key_device(rt_device_t dev);
    GETTER_PROTOTYPE(key_device, rt_device_t);
#endif /* CONFIG_KEY_DEVICE_NAME */

#ifdef RTGUI_USING_HW_CURSOR
    void rtgui_cursor_set_device(const char* device_name);
    void rtgui_cursor_set_position(rt_uint16_t x, rt_uint16_t y);
    void rtgui_cursor_set_image(enum rtgui_cursor_type type);
#endif /* RTGUI_USING_HW_CURSOR */

#endif /* IMPORT_TYPES */

#endif /* __RTGUI_DRIVER_H__ */
