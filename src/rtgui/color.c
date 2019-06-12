/*
 * File      : color.c
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
#include "rtconfig.h"
#include "../include/color.h"

const rtgui_color_t default_foreground = RTGUI_RGB(0x00, 0x00, 0x00);
const rtgui_color_t default_background = RTGUI_RGB(212, 208, 200);

const rtgui_color_t red     = RTGUI_RGB(0xff, 0x00, 0x00);
const rtgui_color_t green   = RTGUI_RGB(0x00, 0xff, 0x00);
const rtgui_color_t blue    = RTGUI_RGB(0x00, 0x00, 0xff);
const rtgui_color_t black   = RTGUI_RGB(0x00, 0x00, 0x00);
const rtgui_color_t white   = RTGUI_RGB(0xff, 0xff, 0xff);

const rtgui_color_t high_light  = RTGUI_RGB(0xfc, 0xfc, 0xfc);
const rtgui_color_t dark_grey   = RTGUI_RGB(0x7f, 0x7f, 0x7f);
const rtgui_color_t light_grey  = RTGUI_RGB(0xc0, 0xc0, 0xc0);

static const rt_uint8_t pixel_bits_table[] = {
    1,      /* mono */
    2,      /* 4 level for gray */
    4,      /* 16 level for gray */
    8,      /* RGB332 */
    12,     /* RGB444 */
    16,     /* RGB565 */
    16,     /* BGR565 */
    18,     /* RGB666 */
    GUIENGINE_RGB888_PIXEL_BITS, /* RGB888 */
    32,     /* ARGB888 */
};

rt_uint8_t rtgui_color_get_bits(rt_uint8_t pixel_format)
{
    if (pixel_format <= RTGRAPHIC_PIXEL_FORMAT_ARGB888)
        return pixel_bits_table[pixel_format];

    /* use 32 as the default */
    return 32;
}
RTM_EXPORT(rtgui_color_get_bits);

rt_uint8_t rtgui_color_get_bpp(rt_uint8_t pixel_format) {
    rt_uint8_t bpp = 4;

    if (pixel_format <= RTGRAPHIC_PIXEL_FORMAT_ARGB888) {
        bpp = _BIT2BYTE(pixel_bits_table[pixel_format]);
    }

    return bpp;
}
RTM_EXPORT(rtgui_color_get_bpp);
