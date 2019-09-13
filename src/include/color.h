/*
 * File      : color.h
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
 * 2012-01-24     onelife      add mono color support
 */
#ifndef __RTGUI_COLOR_H__
#define __RTGUI_COLOR_H__

#include "include/rtgui.h"
#include "include/blit.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The color used in the GUI:
 *
 *         bit        bit
 * RGB565  15 R,G,B   0
 * BGR565  15 B,G,R   0
 * RGB888  23 R,G,B   0
 * ARGB888 31 A,R,G,B 0
 * RGBA888 31 R,G,B,A 0
 * ABGR888 31 A,B,G,R 0
 *
 * The rtgui_color is defined as ARGB888.
 *        bit31 A,R,G,B bit0
 */
#define RTGUI_ARGB(a, r, g, b)  \
        ((rtgui_color_t)(((rt_uint8_t)(b)|\
        (((unsigned long)(rt_uint8_t)(g))<<8))|\
        (((unsigned long)(rt_uint8_t)(r))<<16)|\
        (((unsigned long)(rt_uint8_t)(a))<<24)))
#define RTGUI_RGB(r, g, b)  RTGUI_ARGB(255, (r), (g), (b))

#define RTGUI_RGB_A(c)  (((rt_uint32_t)(c) >> 24) & 0xff)
#define RTGUI_RGB_R(c)  (((rt_uint32_t)(c) >> 16) & 0xff)
#define RTGUI_RGB_G(c)  (((rt_uint32_t)(c) >> 8)  & 0xff)
#define RTGUI_RGB_B(c)  ((rt_uint32_t)(c) & 0xff)

extern const rtgui_color_t default_foreground;
extern const rtgui_color_t default_background;
extern const rtgui_color_t selected_color;
extern const rtgui_color_t disable_foreground;

/* it's better use these color definitions */
#define RED                RTGUI_RGB(0xff, 0x00, 0x00)
#define GREEN              RTGUI_RGB(0x00, 0xff, 0x00)
#define BLUE               RTGUI_RGB(0x00, 0x00, 0xff)
#define BLACK              RTGUI_RGB(0x00, 0x00, 0x00)
#define WHITE              RTGUI_RGB(0xff, 0xff, 0xff)
#define HIGH_LIGHT         RTGUI_RGB(0xfc, 0xfc, 0xfc)
#define DARK_GREY          RTGUI_RGB(0x7f, 0x7f, 0x7f)
#define LIGHT_GREY         RTGUI_RGB(0xc0, 0xc0, 0xc0)

#ifdef  TRANSPARENT
# undef  TRANSPARENT
#endif
#define TRANSPARENT        RTGUI_ARGB(0, 0, 0, 0)

extern const rtgui_color_t red;
extern const rtgui_color_t green;
extern const rtgui_color_t blue;
extern const rtgui_color_t black;
extern const rtgui_color_t white;
extern const rtgui_color_t high_light;
extern const rtgui_color_t dark_grey;
extern const rtgui_color_t light_grey;

extern const rt_uint8_t* rtgui_blit_expand_byte[9];

/* xxx <= RGB */
#ifdef RTGUI_BIG_ENDIAN_OUTPUT
# define RGB565_FROM_RGB(pixel, r, g, b) {                              \
    pixel = ((g >> 2) << 13) | ((b >> 3) << 8) | ((r >> 3) << 3) | (g >> 5); \
}
#else
# define RGB565_FROM_RGB(pixel, r, g, b) {                              \
    pixel = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);              \
}
#endif

#define BGR565_FROM_RGB(pixel, r, g, b) {                               \
    pixel = ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3);              \
}

#define RGB555_FROM_RGB(pixel, r, g, b) {                               \
    pixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);              \
}

#ifdef RTGUI_USING_RGB888_AS_32BIT
# define RGB888_FROM_RGB(pixel, r, g, b) {                              \
    pixel = (r << 16) | (g << 8) | b;                                   \
}
#else
# define RGB888_FROM_RGB(pixel, r, g, b) {                              \
    rt_uint8_t *p = &pixel;                                             \
    *p++ = r;                                                           \
    *p++ = g;                                                           \
    *p = b;                                                             \
}
#endif

#define ARGB8888_FROM_RGBA(pixel, r, g, b, a) {                         \
    pixel = (a << 24) | (r << 16) | (g << 8) | b;                       \
}
#define RGBA8888_FROM_RGBA(pixel, r, g, b, a)                           \
    pixel = (r << 24) | (g << 16) | (b << 8) | a;                       \
}
#define ABGR8888_FROM_RGBA(pixel, r, g, b, a) {                         \
    pixel = (a << 24) | (b << 16) | (g << 8) | r;                       \
}
#define BGRA8888_FROM_RGBA(pixel, r, g, b, a) {                         \
    pixel = (b << 24) | (g << 16) | (r << 8) | a;                       \
}
#define ARGB2101010_FROM_RGBA(pixel, r, g, b, a) {                      \
    r = r ? ((r << 2) | 0x3) : 0;                                       \
    g = g ? ((g << 2) | 0x3) : 0;                                       \
    b = b ? ((b << 2) | 0x3) : 0;                                       \
    a = (a * 3) / 255;                                                  \
    pixel = (a << 30) | (r << 20) | (g << 10) | b;                      \
}

/* RGB <= xxx */
#define RGB_FROM_RGB565(pixel, r, g, b) {                               \
    r = rtgui_blit_expand_byte[3][((pixel & 0xF800)>>11)];              \
    g = rtgui_blit_expand_byte[2][((pixel & 0x07E0)>>5)];               \
    b = rtgui_blit_expand_byte[3][(pixel & 0x001F)];                    \
}
#define RGB_FROM_BGR565(pixel, r, g, b) {                               \
    b = rtgui_blit_expand_byte[3][((pixel & 0xF800)>>11)];              \
    g = rtgui_blit_expand_byte[2][((pixel & 0x07E0)>>5)];               \
    r = rtgui_blit_expand_byte[3][(pixel & 0x001F)];                    \
}
#define RGB_FROM_RGB555(pixel, r, g, b) {                               \
    r = rtgui_blit_expand_byte[3][((pixel & 0x7C00)>>10)];              \
    g = rtgui_blit_expand_byte[3][((pixel & 0x03E0)>>5)];               \
    b = rtgui_blit_expand_byte[3][(pixel & 0x001F)];                    \
}

#ifdef RTGUI_USING_RGB888_AS_32BIT
# define RGB_FROM_RGB888(pixel, r, g, b) {                              \
    r = ((pixel & 0xFF0000)>>16);                                       \
    g = ((pixel & 0xFF00)>>8);                                          \
    b = (pixel & 0xFF);                                                 \
}
#else
# define RGB_FROM_RGB888(pixel, r, g, b) {                              \
    rt_uint8_t *p = &pixel;                                             \
    r = *p++;                                                           \
    g = *p++;                                                           \
    b = *p;                                                             \
}
#endif

#define RGBA_FROM_RGBA8888(pixel, r, g, b, a) {                         \
    r = (pixel >> 24);                                                  \
    g = ((pixel >> 16) & 0xFF);                                         \
    b = ((pixel >> 8) & 0xFF);                                          \
    a = (pixel & 0xFF);                                                 \
}
#define RGBA_FROM_ARGB8888(pixel, r, g, b, a) {                         \
    r = ((pixel >> 16) & 0xFF);                                         \
    g = ((pixel >> 8) & 0xFF);                                          \
    b = (pixel & 0xFF);                                                 \
    a = (pixel >> 24);                                                  \
}
#define RGBA_FROM_ABGR8888(pixel, r, g, b, a) {                         \
    r = (pixel & 0xFF);                                                 \
    g = ((pixel >> 8) & 0xFF);                                          \
    b = ((pixel >> 16) & 0xFF);                                         \
    a = (pixel >> 24);                                                  \
}
#define RGBA_FROM_BGRA8888(pixel, r, g, b, a) {                         \
    r = ((pixel >> 8) & 0xFF);                                          \
    g = ((pixel >> 16) & 0xFF);                                         \
    b = (pixel >> 24);                                                  \
    a = (pixel & 0xFF);                                                 \
}
#define RGBA_FROM_ARGB2101010(pixel, r, g, b, a) {                      \
    r = ((pixel >> 22) & 0xFF);                                         \
    g = ((pixel >> 12) & 0xFF);                                         \
    b = ((pixel >> 2) & 0xFF);                                          \
    a = rtgui_blit_expand_byte[6][(pixel >> 30)];                       \
}


/*
 * RTGUI default color format: ARGB
 * AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB
 * 31                                    0
 */

/* RGB -> mono */
rt_inline rtgui_color_t rtgui_color_to_mono(rtgui_color_t c) {
    return (RTGUI_RGB_R(c) | RTGUI_RGB_G(c) | RTGUI_RGB_B(c)) ? 0x01 : 0x00;
}

/* mono -> RGB */
rt_inline rtgui_color_t rtgui_color_from_mono(rtgui_color_t pixel) {
    return pixel ? white : black;
}

/* convert rtgui color to RRRRRGGGGGGBBBBB */
rt_inline rt_uint16_t rtgui_color_to_565(rtgui_color_t c) {
    rt_uint16_t pixel;

    RGB565_FROM_RGB(pixel, RTGUI_RGB_R(c), RTGUI_RGB_G(c), RTGUI_RGB_B(c));
    return pixel;
}

rt_inline rtgui_color_t rtgui_color_from_565(rt_uint16_t pixel) {
    rt_uint16_t r, g, b;

    r = (pixel >> 11) & 0x1f;
    g = (pixel >> 5)  & 0x3f;
    b = pixel & 0x1f;

    return (b * 255 / 31 + ((g * 255 / 63) << 8) + ((r * 255 / 31) << 16));
}

/* convert rtgui color to BBBBBGGGGGGRRRRR */
rt_inline rt_uint16_t rtgui_color_to_565p(rtgui_color_t c) {
    return (rt_uint16_t)(((RTGUI_RGB_B(c) >> 3) << 11) | \
                         ((RTGUI_RGB_G(c) >> 2) << 5)  | \
                         (RTGUI_RGB_R(c) >> 3));
}

rt_inline rtgui_color_t rtgui_color_from_565p(rt_uint16_t pixel) {
    rt_uint8_t r, g, b;

    r = pixel & 0x1f;
    g = (pixel >> 5) & 0x3f;
    b = (pixel >> 11) & 0x1f;

    return (b * 255 / 31 + ((g * 255 / 63) << 8) + ((r * 255 / 31) << 16));
}

/* convert rtgui color to RGB */
rt_inline rt_uint32_t rtgui_color_to_888(rtgui_color_t c) {
    return (RTGUI_RGB_R(c) << 16 | RTGUI_RGB_G(c) << 8 | RTGUI_RGB_B(c));
}

rt_inline rtgui_color_t rtgui_color_from_888(rt_uint32_t pixel) {
    return RTGUI_RGB(((pixel >> 16) & 0xff), ((pixel >> 8) & 0xff),
        (pixel & 0xff));
}

#ifdef __cplusplus
}
#endif

#endif

