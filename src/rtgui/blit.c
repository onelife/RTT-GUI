/*
 * File      : blit.h
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
 * 2012-01-24     onelife      add one more blit table which exchanges the
 *                             positions of R and B color components in output
 * 2013-10-04     Bernard      porting SDL software render to RT-Thread GUI
 * 2019-05-29     onelife      refactor 
 */

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "include/rtgui.h"
#include "include/blit.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "IMG_BLT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

#if (CONFIG_USING_MONO)
/* RGB888 / BGR888 -> MONO blending */
static void blit_line_rgb888_to_mono(rt_uint8_t *dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint8_t *end = src + len;
    rt_uint8_t step = _BIT2BYTE(RTGUI_RGB888_PIXEL_BITS) << scale;
    rt_uint8_t bit;
    (void)palette;

    for (bit = 0, *dst = 0x00; src < end; src += step, bit++) {
        if (bit >= 8) {
            bit = 0;
            dst++;
            *dst = 0x00;
        }
        if (((rt_uint16_t)*(src + 0) + *(src + 1) + *(src + 2)) >= 0x017f)
            *dst |= 0x01 << bit;
    }
}

/* RGB565 -> MONO blending */
static void blit_line_rgb565_to_mono(rt_uint8_t *dst, rt_uint8_t *_src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *src = (rt_uint16_t *)_src;
    rt_uint8_t *end = _src + len;
    rt_uint8_t step = 1 << scale;
    rt_uint8_t bit;
    (void)palette;

    for (bit = 0, *dst = 0x00; src < (rt_uint16_t *)end; src += step, bit++) {
        if (bit >= 8) {
            bit = 0;
            dst++;
            *dst = 0x00;
        }
        if (((rt_uint16_t)((*src & 0xf800) >> 11) + \
            ((*src & 0x07c0) >> 6) + \
            (*src & 0x001f)) >= 0x2e)
            *dst |= 0x01 << bit;
    }
}

/* RGB8I -> MONO blending */
static void blit_line_gray8i_to_mono(rt_uint8_t *dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint8_t *end = src + len;
    rt_uint8_t step = 1 << scale;
    rt_uint8_t bit;
    (void)palette;

    for (bit = 0, *dst = 0x00; src < end; src += step, bit++) {
        if (bit >= 8) {
            bit = 0;
            dst++;
            *dst = 0x00;
        }
        if (*src >= ((1 << 7) - 1))
            *dst |= 0x01 << bit;
    }
}

/* RGB4I -> MONO blending */
static void blit_line_gray4i_to_mono(rt_uint8_t *dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;
    rt_uint8_t bit;
    (void)palette;

    if (scale > 1) {
        shift_step = 8;
        step = 1 << (scale - 1);
    } else {
        shift_step = 1 << (2 + scale);
        step = 1;
    }

    for (bit = 0, *dst = 0x00; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step, bit++) {
            if (bit >= 8) {
                bit = 0;
                dst++;
                *dst = 0x00;
            }
            if (*src >= ((1 << 3) - 1))
                *dst |= 0x01 << bit;
        }
}

/* RGB2I -> MONO blending */
static void blit_line_gray2i_to_mono(rt_uint8_t *dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;
    rt_uint8_t bit;
    (void)palette;

    if (scale > 2) {
        shift_step = 8;
        step = 1 << (scale - 2);
    } else {
        shift_step = 1 << (1 + scale);
        step = 1;
    }

    for (bit = 0, *dst = 0x00; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step, bit++) {
            if (bit >= 8) {
                bit = 0;
                dst++;
                *dst = 0x00;
            }
            if (*src >= 1)
                *dst |= 0x01 << bit;
        }
}

/* MONO -> MONO blending */
static void blit_line_mono_to_mono(rt_uint8_t *dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;
    rt_uint8_t bit;

    if (scale > 3) {
        shift_step = 8;
        step = 1 << (scale - 3);
    } else {
        shift_step = 1 << scale;
        step = 1;
    }

    for (bit = 0, *dst = 0x00; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step, bit++) {
            if (bit >= 8) {
                bit = 0;
                dst++;
                *dst = 0x00;
            }
            if (palette->colors[1] != black) {
                if (*src & (1 << (7 - shift))) {
                    *dst |= 0x01 << bit;
                }
            } else {
                if (!(*src & (1 << (7 - shift)))) {
                    *dst |= 0x01 << bit;
                }
            }
        }
}
#endif

#if (CONFIG_USING_RGB565)
/* RGB888 -> RGB565 blending */
static void blit_line_rgb888_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step = _BIT2BYTE(RTGUI_RGB888_PIXEL_BITS) << scale;
    rt_uint32_t srcR, srcG, srcB;
    (void)palette;

    for ( ; src < end; src += step, dst++) {
        RGB_FROM_RGB888(*src, srcR, srcG, srcB);
        RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
    }
}

/* BGR888 -> RGB565 blending */
static void blit_line_bgr888_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step = _BIT2BYTE(RTGUI_RGB888_PIXEL_BITS) << scale;
    rt_uint32_t srcR, srcG, srcB;
    (void)palette;

    for ( ; src < end; src += step, dst++) {
        RGB_FROM_RGB888(*src, srcB, srcG, srcR);
        RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
    }
}

/* RGB565 -> RGB565 blending */
static void blit_line_rgb565_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint16_t *src = (rt_uint16_t *)_src;
    rt_uint8_t *end = _src + len;
    rt_uint8_t step = 1 << scale;
    (void)palette;

    for ( ; src < (rt_uint16_t *)end; src += step) {
        #ifdef RTGUI_BIG_ENDIAN_OUTPUT
            *dst++ = (*src << 8) | (*src >> 8);
        #else
            *dst++ = *src;
        #endif
    }
}

/* RGB8I -> RGB565 blending */
static void blit_line_gray8i_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step = 1 << scale;
    rtgui_color_t color;

    for ( ; src < end; src += step) {
        color = palette->colors[*src];
        RGB565_FROM_RGB(*dst++, RTGUI_RGB_R(color), RTGUI_RGB_G(color),
            RTGUI_RGB_B(color));
    }
}

/* RGB4I -> RGB565 blending */
static void blit_line_gray4i_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;
    rtgui_color_t color;

    if (scale > 1) {
        shift_step = 8;
        step = 1 << (scale - 1);
    } else {
        shift_step = 1 << (2 + scale);
        step = 1;
    }

    for ( ; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step) {
            color = \
                palette->colors[(*src & (0x0F << (4 - shift))) >> (4 - shift)];
            RGB565_FROM_RGB(*dst++, RTGUI_RGB_R(color), RTGUI_RGB_G(color),
                RTGUI_RGB_B(color));
        }
}

/* RGB2I -> RGB565 blending */
static void blit_line_gray2i_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;
    rtgui_color_t color;

    if (scale > 2) {
        shift_step = 8;
        step = 1 << (scale - 2);
    } else {
        shift_step = 1 << (1 + scale);
        step = 1;
    }

    // TODO(onelife): test
    for ( ; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step) {
            color = \
                palette->colors[(*src & (0x03 << (6 - shift))) >> (6 - shift)];
            RGB565_FROM_RGB(*dst++, RTGUI_RGB_R(color), RTGUI_RGB_G(color),
                RTGUI_RGB_B(color));
        }
}

/* MONO -> RGB565 blending */
static void blit_line_mono_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step, shift_step, shift;

    if (scale > 3) {
        shift_step = 8;
        step = 1 << (scale - 3);
    } else {
        shift_step = 1 << scale;
        step = 1;
    }

    for ( ; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step)
            if (*src & (1 << (7 - shift))) {
                RGB565_FROM_RGB(*dst++,
                    RTGUI_RGB_R(palette->colors[1]),
                    RTGUI_RGB_G(palette->colors[1]),
                    RTGUI_RGB_B(palette->colors[1]));
            } else {
                RGB565_FROM_RGB(*dst++,
                    RTGUI_RGB_R(palette->colors[0]),
                    RTGUI_RGB_G(palette->colors[0]),
                    RTGUI_RGB_B(palette->colors[0]));
            }
}

#endif /* CONFIG_USING_RGB565 */

rtgui_blit_line_func rtgui_get_blit_line_func(rt_uint8_t src_fmt,
    rt_uint8_t dst_fmt) {
    switch (dst_fmt) {
    #if (CONFIG_USING_MONO)
    case RTGRAPHIC_PIXEL_FORMAT_MONO:
        switch (src_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        case RTGRAPHIC_PIXEL_FORMAT_BGR888:
            return blit_line_rgb888_to_mono;

        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            return blit_line_rgb565_to_mono;

        case RTGRAPHIC_PIXEL_FORMAT_RGB8I:
            return blit_line_gray8i_to_mono;

        case RTGRAPHIC_PIXEL_FORMAT_RGB4I:
            return blit_line_gray4i_to_mono;

        case RTGRAPHIC_PIXEL_FORMAT_RGB2I:
            return blit_line_gray2i_to_mono;

        case RTGRAPHIC_PIXEL_FORMAT_MONO:
            return blit_line_mono_to_mono;

        default:
            return RT_NULL;
        }
    #endif /* RTGRAPHIC_PIXEL_FORMAT_MONO */

    #if (CONFIG_USING_RGB565)
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        switch (src_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            return blit_line_rgb888_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_BGR888:
            return blit_line_bgr888_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            return blit_line_rgb565_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_RGB8I:
            return blit_line_gray8i_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_RGB4I:
            return blit_line_gray4i_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_RGB2I:
            return blit_line_gray2i_to_rgb565;

        case RTGRAPHIC_PIXEL_FORMAT_MONO:
            return blit_line_mono_to_rgb565;

        default:
            return RT_NULL;
        }
    #endif /* CONFIG_USING_RGB565 */

    default:
        return RT_NULL;
    }

    return RT_NULL;
}


#ifdef RTGUI_USING_DC_BUFFER

/* 3 bpp to 1 bpp */
static void rtgui_blit_line_3_1(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len) {
    len = len / 3;
    while (len)
    {
        *_dst = (rt_uint8_t)(((*_src & 0x00E00000) >> 16) |
                                ((*(_src + 1) & 0x0000E000) >> 11) |
                                ((*(_src + 2) & 0x000000C0) >> 6));

        _src += 3;
        _dst ++;
        len --;
    }
    return;
}

/* 4 bpp to 1 bpp */
static void rtgui_blit_line_4_1(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    struct _color
    {
        rt_uint8_t r, g, b, a;
    } *c;

    c = (struct _color *)_src;
    while (len-- > 0)
    {
        *_dst = (c->r & 0xe0) | (c->g & 0xc0) >> 3 | (c->b & 0xe0) >> 5 ;

        c ++;
        _dst ++;
    }
}

/* 3 bpp to 2 bpp */
static void rtgui_blit_line_3_2(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    rt_uint16_t *dst;

    dst = (rt_uint16_t *)_dst;
    len = len / 3;
    while (len)
    {
        *dst = (((*(_src + 2) << 8) & 0x0000F800) |
                ((*(_src + 1) << 3) & 0x000007E0)     |
                ((*_src >> 3) & 0x0000001F));

        _src += 3;
        dst ++;
        len --;
    }

    return;
}

/* 4 bpp to 2 bpp (ARGB --> RGB565) */
static void rtgui_blit_line_4_2(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    int width = len/4;
    rt_uint32_t *srcp = (rt_uint32_t *) _src;
    rt_uint16_t *dstp = (rt_uint16_t *) _dst;

    /* *INDENT-OFF* */
    DUFFS_LOOP4(
    {
        rt_uint32_t s = *srcp;
        unsigned alpha = s >> 27; /* downscale alpha to 5 bits */
        /* FIXME: Here we special-case opaque alpha since the
           compositioning used (>>8 instead of /0xFFU) doesn't handle
           it correctly. Also special-case alpha=0 for speed?
           Benchmark this! */
        if(alpha)
        {
            if(alpha == (0xFFU >> 3))
            {
                *dstp = (rt_uint16_t)((s >> 8 & 0xf800) + (s >> 5 & 0x7e0) + (s >> 3  & 0x1f));
            }
            else
            {
                rt_uint32_t d = *dstp;
                /*
                 * convert source and destination to G0RAB65565
                 * and blend all components at the same time
                 */
                s = ((s & 0xfc00) << 11) + (s >> 8 & 0xf800)
                    + (s >> 3 & 0x1f);
                d = (d | d << 16) & 0x07e0f81f;
                d += (s - d) * alpha >> 5;
                d &= 0x07e0f81f;
                *dstp = (rt_uint16_t)(d | d >> 16);
            }
        }
        srcp++;
        dstp++;
    }, width);
}


#define HI  1
#define LO  0

/* Special optimized blit for RGB 5-6-5 --> RGBA 8-8-8-8 */
static const rt_uint32_t RGB565_RGBA8888_LUT[512] =
{
    0x000000ff, 0x00000000, 0x000008ff, 0x00200000,
    0x000010ff, 0x00400000, 0x000018ff, 0x00610000,
    0x000020ff, 0x00810000, 0x000029ff, 0x00a10000,
    0x000031ff, 0x00c20000, 0x000039ff, 0x00e20000,
    0x000041ff, 0x08000000, 0x00004aff, 0x08200000,
    0x000052ff, 0x08400000, 0x00005aff, 0x08610000,
    0x000062ff, 0x08810000, 0x00006aff, 0x08a10000,
    0x000073ff, 0x08c20000, 0x00007bff, 0x08e20000,
    0x000083ff, 0x10000000, 0x00008bff, 0x10200000,
    0x000094ff, 0x10400000, 0x00009cff, 0x10610000,
    0x0000a4ff, 0x10810000, 0x0000acff, 0x10a10000,
    0x0000b4ff, 0x10c20000, 0x0000bdff, 0x10e20000,
    0x0000c5ff, 0x18000000, 0x0000cdff, 0x18200000,
    0x0000d5ff, 0x18400000, 0x0000deff, 0x18610000,
    0x0000e6ff, 0x18810000, 0x0000eeff, 0x18a10000,
    0x0000f6ff, 0x18c20000, 0x0000ffff, 0x18e20000,
    0x000400ff, 0x20000000, 0x000408ff, 0x20200000,
    0x000410ff, 0x20400000, 0x000418ff, 0x20610000,
    0x000420ff, 0x20810000, 0x000429ff, 0x20a10000,
    0x000431ff, 0x20c20000, 0x000439ff, 0x20e20000,
    0x000441ff, 0x29000000, 0x00044aff, 0x29200000,
    0x000452ff, 0x29400000, 0x00045aff, 0x29610000,
    0x000462ff, 0x29810000, 0x00046aff, 0x29a10000,
    0x000473ff, 0x29c20000, 0x00047bff, 0x29e20000,
    0x000483ff, 0x31000000, 0x00048bff, 0x31200000,
    0x000494ff, 0x31400000, 0x00049cff, 0x31610000,
    0x0004a4ff, 0x31810000, 0x0004acff, 0x31a10000,
    0x0004b4ff, 0x31c20000, 0x0004bdff, 0x31e20000,
    0x0004c5ff, 0x39000000, 0x0004cdff, 0x39200000,
    0x0004d5ff, 0x39400000, 0x0004deff, 0x39610000,
    0x0004e6ff, 0x39810000, 0x0004eeff, 0x39a10000,
    0x0004f6ff, 0x39c20000, 0x0004ffff, 0x39e20000,
    0x000800ff, 0x41000000, 0x000808ff, 0x41200000,
    0x000810ff, 0x41400000, 0x000818ff, 0x41610000,
    0x000820ff, 0x41810000, 0x000829ff, 0x41a10000,
    0x000831ff, 0x41c20000, 0x000839ff, 0x41e20000,
    0x000841ff, 0x4a000000, 0x00084aff, 0x4a200000,
    0x000852ff, 0x4a400000, 0x00085aff, 0x4a610000,
    0x000862ff, 0x4a810000, 0x00086aff, 0x4aa10000,
    0x000873ff, 0x4ac20000, 0x00087bff, 0x4ae20000,
    0x000883ff, 0x52000000, 0x00088bff, 0x52200000,
    0x000894ff, 0x52400000, 0x00089cff, 0x52610000,
    0x0008a4ff, 0x52810000, 0x0008acff, 0x52a10000,
    0x0008b4ff, 0x52c20000, 0x0008bdff, 0x52e20000,
    0x0008c5ff, 0x5a000000, 0x0008cdff, 0x5a200000,
    0x0008d5ff, 0x5a400000, 0x0008deff, 0x5a610000,
    0x0008e6ff, 0x5a810000, 0x0008eeff, 0x5aa10000,
    0x0008f6ff, 0x5ac20000, 0x0008ffff, 0x5ae20000,
    0x000c00ff, 0x62000000, 0x000c08ff, 0x62200000,
    0x000c10ff, 0x62400000, 0x000c18ff, 0x62610000,
    0x000c20ff, 0x62810000, 0x000c29ff, 0x62a10000,
    0x000c31ff, 0x62c20000, 0x000c39ff, 0x62e20000,
    0x000c41ff, 0x6a000000, 0x000c4aff, 0x6a200000,
    0x000c52ff, 0x6a400000, 0x000c5aff, 0x6a610000,
    0x000c62ff, 0x6a810000, 0x000c6aff, 0x6aa10000,
    0x000c73ff, 0x6ac20000, 0x000c7bff, 0x6ae20000,
    0x000c83ff, 0x73000000, 0x000c8bff, 0x73200000,
    0x000c94ff, 0x73400000, 0x000c9cff, 0x73610000,
    0x000ca4ff, 0x73810000, 0x000cacff, 0x73a10000,
    0x000cb4ff, 0x73c20000, 0x000cbdff, 0x73e20000,
    0x000cc5ff, 0x7b000000, 0x000ccdff, 0x7b200000,
    0x000cd5ff, 0x7b400000, 0x000cdeff, 0x7b610000,
    0x000ce6ff, 0x7b810000, 0x000ceeff, 0x7ba10000,
    0x000cf6ff, 0x7bc20000, 0x000cffff, 0x7be20000,
    0x001000ff, 0x83000000, 0x001008ff, 0x83200000,
    0x001010ff, 0x83400000, 0x001018ff, 0x83610000,
    0x001020ff, 0x83810000, 0x001029ff, 0x83a10000,
    0x001031ff, 0x83c20000, 0x001039ff, 0x83e20000,
    0x001041ff, 0x8b000000, 0x00104aff, 0x8b200000,
    0x001052ff, 0x8b400000, 0x00105aff, 0x8b610000,
    0x001062ff, 0x8b810000, 0x00106aff, 0x8ba10000,
    0x001073ff, 0x8bc20000, 0x00107bff, 0x8be20000,
    0x001083ff, 0x94000000, 0x00108bff, 0x94200000,
    0x001094ff, 0x94400000, 0x00109cff, 0x94610000,
    0x0010a4ff, 0x94810000, 0x0010acff, 0x94a10000,
    0x0010b4ff, 0x94c20000, 0x0010bdff, 0x94e20000,
    0x0010c5ff, 0x9c000000, 0x0010cdff, 0x9c200000,
    0x0010d5ff, 0x9c400000, 0x0010deff, 0x9c610000,
    0x0010e6ff, 0x9c810000, 0x0010eeff, 0x9ca10000,
    0x0010f6ff, 0x9cc20000, 0x0010ffff, 0x9ce20000,
    0x001400ff, 0xa4000000, 0x001408ff, 0xa4200000,
    0x001410ff, 0xa4400000, 0x001418ff, 0xa4610000,
    0x001420ff, 0xa4810000, 0x001429ff, 0xa4a10000,
    0x001431ff, 0xa4c20000, 0x001439ff, 0xa4e20000,
    0x001441ff, 0xac000000, 0x00144aff, 0xac200000,
    0x001452ff, 0xac400000, 0x00145aff, 0xac610000,
    0x001462ff, 0xac810000, 0x00146aff, 0xaca10000,
    0x001473ff, 0xacc20000, 0x00147bff, 0xace20000,
    0x001483ff, 0xb4000000, 0x00148bff, 0xb4200000,
    0x001494ff, 0xb4400000, 0x00149cff, 0xb4610000,
    0x0014a4ff, 0xb4810000, 0x0014acff, 0xb4a10000,
    0x0014b4ff, 0xb4c20000, 0x0014bdff, 0xb4e20000,
    0x0014c5ff, 0xbd000000, 0x0014cdff, 0xbd200000,
    0x0014d5ff, 0xbd400000, 0x0014deff, 0xbd610000,
    0x0014e6ff, 0xbd810000, 0x0014eeff, 0xbda10000,
    0x0014f6ff, 0xbdc20000, 0x0014ffff, 0xbde20000,
    0x001800ff, 0xc5000000, 0x001808ff, 0xc5200000,
    0x001810ff, 0xc5400000, 0x001818ff, 0xc5610000,
    0x001820ff, 0xc5810000, 0x001829ff, 0xc5a10000,
    0x001831ff, 0xc5c20000, 0x001839ff, 0xc5e20000,
    0x001841ff, 0xcd000000, 0x00184aff, 0xcd200000,
    0x001852ff, 0xcd400000, 0x00185aff, 0xcd610000,
    0x001862ff, 0xcd810000, 0x00186aff, 0xcda10000,
    0x001873ff, 0xcdc20000, 0x00187bff, 0xcde20000,
    0x001883ff, 0xd5000000, 0x00188bff, 0xd5200000,
    0x001894ff, 0xd5400000, 0x00189cff, 0xd5610000,
    0x0018a4ff, 0xd5810000, 0x0018acff, 0xd5a10000,
    0x0018b4ff, 0xd5c20000, 0x0018bdff, 0xd5e20000,
    0x0018c5ff, 0xde000000, 0x0018cdff, 0xde200000,
    0x0018d5ff, 0xde400000, 0x0018deff, 0xde610000,
    0x0018e6ff, 0xde810000, 0x0018eeff, 0xdea10000,
    0x0018f6ff, 0xdec20000, 0x0018ffff, 0xdee20000,
    0x001c00ff, 0xe6000000, 0x001c08ff, 0xe6200000,
    0x001c10ff, 0xe6400000, 0x001c18ff, 0xe6610000,
    0x001c20ff, 0xe6810000, 0x001c29ff, 0xe6a10000,
    0x001c31ff, 0xe6c20000, 0x001c39ff, 0xe6e20000,
    0x001c41ff, 0xee000000, 0x001c4aff, 0xee200000,
    0x001c52ff, 0xee400000, 0x001c5aff, 0xee610000,
    0x001c62ff, 0xee810000, 0x001c6aff, 0xeea10000,
    0x001c73ff, 0xeec20000, 0x001c7bff, 0xeee20000,
    0x001c83ff, 0xf6000000, 0x001c8bff, 0xf6200000,
    0x001c94ff, 0xf6400000, 0x001c9cff, 0xf6610000,
    0x001ca4ff, 0xf6810000, 0x001cacff, 0xf6a10000,
    0x001cb4ff, 0xf6c20000, 0x001cbdff, 0xf6e20000,
    0x001cc5ff, 0xff000000, 0x001ccdff, 0xff200000,
    0x001cd5ff, 0xff400000, 0x001cdeff, 0xff610000,
    0x001ce6ff, 0xff810000, 0x001ceeff, 0xffa10000,
    0x001cf6ff, 0xffc20000, 0x001cffff, 0xffe20000,
};

static void rtgui_blit_line_2_3(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    rt_uint16_t *src;
    rt_uint32_t *dst;

    src = (rt_uint16_t *)_src;
    dst = (rt_uint32_t *)_dst;

    len = len / 2;
    while (len)
    {
        *dst++ = RGB565_RGBA8888_LUT[src[LO] * 2] + RGB565_RGBA8888_LUT[src[HI] * 2 + 1];
        len--;
        src ++;
    }
}

/* convert 4bpp to 3bpp */
static void rtgui_blit_line_4_3(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    len = len / 4;
    while (len)
    {
        *_dst++ = *_src++;
        *_dst++ = *_src++;
        *_dst++ = *_src++;
        _src ++;
        len --;
    }
}


static void rtgui_blit_line_2_4(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
}

/* convert 3bpp to 4bpp */
static void rtgui_blit_line_3_4(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    len = len / 4;
    while (len)
    {
        *_dst++ = *_src++;
        *_dst++ = *_src++;
        *_dst++ = *_src++;
        *_dst++ = 0;
        len --;
    }
}

static const rtgui_blit_line_func_ _blit_table[4][4] = {
    /* 1_1, 2_1, 3_1, 4_1 */
    {RT_NULL, RT_NULL, rtgui_blit_line_3_1, rtgui_blit_line_4_1 },
    /* 1_2, 2_2, 3_2, 4_2 */
    {RT_NULL, RT_NULL, rtgui_blit_line_3_2, rtgui_blit_line_4_2 },
    /* 1_3, 2_3, 3_3, 4_3 */
    {RT_NULL, rtgui_blit_line_2_3, RT_NULL, rtgui_blit_line_4_3 },
    /* 1_4, 2_4, 3_4, 4_4 */
    {RT_NULL, rtgui_blit_line_2_4, rtgui_blit_line_3_4, RT_NULL },
};

rtgui_blit_line_func_ rtgui_blit_line_get(rt_uint8_t dst_bpp,
    rt_uint8_t src_bpp) {
    RT_ASSERT(dst_bpp > 0 && dst_bpp < 5);
    RT_ASSERT(src_bpp > 0 && src_bpp < 5);

    return _blit_table[dst_bpp - 1][src_bpp - 1];
}


/* RGB565 -> RGB565 blending with alpha */
static void blit_rgb565_to_rgb565_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        rt_uint16_t *src = (rt_uint16_t *)info->src;
        rt_uint32_t src_skip = info->src_skip >> 1;
        rt_uint16_t *dst = (rt_uint16_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 1;

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                /* not do alpha blend */
                if (0xF8 == (alpha & 0xF8)) {
                    *dst = *src;
                } else {
                    rt_uint32_t dstR, dstG, dstB;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    RGB_FROM_RGB565(*src, srcR, srcG, srcB);
                    RGB_FROM_RGB565(*dst, dstR, dstG, dstB);
                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                    RGB565_FROM_RGB(*dst, dstR, dstG, dstB);
                }
                src++;
                dst++;
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* RGB565 -> RGB888 blending with alpha */
static void blit_rgb565_to_rgb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        rt_uint16_t *src = (rt_uint16_t *)info->src;
        rt_uint32_t src_skip = info->src_skip >> 1;
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            rt_uint32_t *dst = (rt_uint32_t *)info->dst;
            rt_uint32_t dst_skip = info->dst_skip >> 2;
        #else
            rt_uint8_t *dst = (rt_uint8_t *)info->dst;
            rt_uint32_t dst_skip = info->dst_skip;
        #endif

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                /* not do alpha blend */
                if (0xF8 == (alpha & 0xF8)) {
                    RGB_FROM_RGB565(*src, srcR, srcG, srcB);
                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                        dst++;
                    #else
                        *dst++ = (rt_uint8_t)srcR;
                        *dst++ = (rt_uint8_t)srcG;
                        *dst++ = (rt_uint8_t)srcB;
                    #endif
                } else {
                    rt_uint32_t dstR, dstG, dstB;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    RGB_FROM_RGB565(*src, srcR, srcG, srcB);
                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                    #else
                        dstR = *dst;
                        dstG = *(dst + 1);
                        dstB = *(dst + 2);
                    #endif
                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB888_FROM_RGB(*dst, dstR, dstG, dstB);
                        dst++;
                    #else
                        *dst++ = (rt_uint8_t)dstR;
                        *dst++ = (rt_uint8_t)dstG;
                        *dst++ = (rt_uint8_t)dstB;
                    #endif
                }
                src++;
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* RGB565 -> ARGB888 blending with alpha */
static void blit_rgb565_to_argb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        rt_uint16_t *src = (rt_uint16_t *)info->src;
        rt_uint32_t src_skip = info->src_skip >> 1;
        rt_uint32_t *dst = (rt_uint32_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 2;

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                /* not do alpha blend */
                if (0xF8 == (alpha & 0xF8)) {
                    RGB_FROM_RGB565(*src, srcR, srcG, srcB);
                    ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, 0xFFU);
                } else {
                    rt_uint32_t dstR, dstG, dstB, dstA;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    RGB_FROM_RGB565(*src, srcR, srcG, srcB);
                    RGBA_FROM_ARGB8888(*dst, dstR, dstG, dstB, dstA);
                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                    dstA = alpha + ((0xFFU - alpha) * dstA) / 0xFFU;
                    ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, dstA);
                }
                src++;
                dst++;
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* RGB888 -> RGB565 blending with alpha */
static void blit_rgb888_to_rgb565_alpha(rtgui_blit_info_t *info) {
    unsigned int alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            rt_uint32_t *src = (rt_uint32_t *)info->src;
            rt_uint32_t src_skip = info->src_skip >> 2;
        #else
            rt_uint8_t *src = (rt_uint8_t *)info->src;
            rt_uint32_t src_skip = info->src_skip;
        #endif
        rt_uint16_t *dst = (rt_uint16_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 1;

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                /* not do alpha blend */
                if (0xF8 == (alpha & 0xF8)) {
                    RGB_FROM_RGB888(*src, srcR, srcG, srcB);
                    RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
                } else {
                    rt_uint32_t dstR, dstG, dstB;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    RGB_FROM_RGB888(*src, srcR, srcG, srcB);
                    RGB_FROM_RGB565(*dst, dstR, dstG, dstB);
                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                    RGB565_FROM_RGB(*dst, dstR, dstG, dstB);
                }

                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    src++;
                #else
                    src += 3;
                #endif
                dst++;
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* RGB888 -> RGB888 blending with alpha */
static void blit_rgb888_to_rgb888_alpha(rtgui_blit_info_t *info) {
    unsigned int alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            rt_uint32_t *src = (rt_uint32_t *)info->src;
            rt_uint32_t src_skip = info->src_skip >> 2;
            rt_uint32_t *dst = (rt_uint32_t *)info->dst;
            rt_uint32_t dst_skip = info->dst_skip >> 2;
        #else
            rt_uint8_t *src = (rt_uint8_t *)info->src;
            rt_uint32_t src_skip = info->src_skip;
            rt_uint8_t *dst = (rt_uint8_t *)info->dst;
            rt_uint32_t dst_skip = info->dst_skip;
        #endif

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                /* not do alpha blend */
                if (0xF8 == (alpha & 0xF8)) {
                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        *dst++ = *src++;
                    #else
                        *dst++ = *src++;
                        *dst++ = *src++;
                        *dst++ = *src++;
                    #endif
                } else {
                    rt_uint32_t dstR, dstG, dstB;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB_FROM_RGB888(*src, srcR, srcG, srcB);
                        RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                    #else
                        srcR = *src++;
                        srcG = *src++;
                        srcB = *src++;
                        dstR = *dst;
                        dstG = *(dst + 1);
                        dstB = *(dst + 2);
                    #endif

                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;

                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB888_FROM_RGB(*dst, dstR, dstG, dstB);
                        src++;
                        dst++;
                    #else
                        *dst++ = dstR;
                        *dst++ = dstG;
                        *dst++ = dstB;
                    #endif
                }
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* RGB888 -> ARGB888 blending with alpha */
static void blit_rgb888_to_argb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t alpha = info->a;

    if (alpha >> 3) {
        rt_uint32_t srcR, srcG, srcB;
        rt_uint32_t height = info->dst_h;
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            rt_uint32_t *src = (rt_uint32_t *)info->src;
            rt_uint32_t src_skip = info->src_skip >> 2;
        #else
            rt_uint8_t *src = (rt_uint8_t *)info->src;
            rt_uint32_t src_skip = info->src_skip;
        #endif
        rt_uint32_t *dst = (rt_uint32_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 2;

        while (height--) {
            rt_uint32_t width = info->dst_w;

            while (width--) {
                if (0xF8 == (alpha & 0xF8)) {
                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        *dst = (*src | 0xFF000000);
                        src++;
                    #else
                        srcR = *src++;
                        srcG = *src++;
                        srcB = *src++;
                        ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, alpha);
                    #endif
                } else {
                    rt_uint32_t dstR, dstG, dstB, dstA;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    RGBA_FROM_ARGB8888(*dst, dstR, dstG, dstB, dstA);

                    #ifdef RTGUI_USING_RGB888_AS_32BIT
                        RGB_FROM_RGB888(*src, srcR, srcG, srcB);
                        src++;
                    #else
                        srcR = *src++;
                        srcG = *src++;
                        srcB = *src++;
                    #endif

                    if (dstA) {
                        dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                        dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                        dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                        dstA = alpha + ((0xFFU - alpha) * dstA) / 0xFFU;
                        ARGB8888_FROM_RGBA(*dst, dstR, dstG, dstB, dstA);
                    } else {
                        ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, alpha);
                    }
                }
                dst++;
            }
            src += src_skip;
            dst += dst_skip;
        }
    }
}

/* ARGB888 -> RGB565 blending with alpha */
static void blit_argb888_to_rgb565_alpha(rtgui_blit_info_t * info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint32_t *src = (rt_uint32_t *)info->src;
    rt_uint32_t src_skip = info->src_skip >> 2;
    rt_uint16_t *dst = (rt_uint16_t *)info->dst;
    rt_uint32_t dst_skip = info->dst_skip >> 1;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            RGBA_FROM_ARGB8888(*src, srcR, srcG, srcB, srcA);
            if (0xFFU != info->a) {
                srcA = (srcA * info->a + 128) / 0xFFU;
            }

            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGB_FROM_RGB565(*dst, dstR, dstG, dstB);
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                RGB565_FROM_RGB(*dst, dstR, dstG, dstB);
            }
            src++;
            dst++;
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* ARGB888 -> RGB888 blending with alpha */
static void blit_argb888_to_rgb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint32_t *src = (rt_uint32_t *)info->src;
    rt_uint32_t src_skip = info->src_skip >> 2;
    #ifdef RTGUI_USING_RGB888_AS_32BIT
        rt_uint32_t *dst = (rt_uint32_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 2;
    #else
        rt_uint8_t *dst = (rt_uint8_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip;
    #endif

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            RGBA_FROM_ARGB8888(*src, srcR, srcG, srcB, srcA);
            if (0xFFU != info->a) {
                srcA = (srcA * info->a + 128) / 0xFFU;
            }

            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                    dst++;
                #else
                    *dst++ = (rt_uint8_t)srcR;
                    *dst++ = (rt_uint8_t)srcG;
                    *dst++ = (rt_uint8_t)srcB;
                #endif
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    dst++;
                #else
                    dst += 3;
                #endif
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, dstR, dstG, dstB);
                    dst++;
                #else
                    *dst++ = dstR;
                    *dst++ = dstG;
                    *dst++ = dstB;
                #endif
            }
            src++;
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* ARGB888 -> ARGB888 blending with alpha */
static void blit_argb888_to_argb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint32_t *src = (rt_uint32_t *)info->src;
    rt_uint32_t src_skip = info->src_skip >> 2;
    rt_uint32_t *dst = (rt_uint32_t *)info->dst;
    rt_uint32_t dst_skip = info->dst_skip >> 2;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            RGBA_FROM_ARGB8888(*src, srcR, srcG, srcB, srcA);
            if (0xFFU != info->a) {
                srcA = (srcA * info->a + 128) / 0xFFU;
            }

            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, 0xFFU);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB, dstA;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGBA_FROM_ARGB8888(*dst, dstR, dstG, dstB, dstA);
                if (dstA) {
                    dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                    dstA = srcA + ((0xFFU - srcA) * dstA) / 0xFFU;
                    ARGB8888_FROM_RGBA(*dst, dstR, dstG, dstB, dstA);
                } else {
                    ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, srcA);
                }
            }
            src++;
            dst++;
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* alpha -> RGB565 blending with alpha */
static void blit_a_to_rgb565_alpha(rtgui_blit_info_t * info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint8_t *src = info->src;
    rt_uint16_t *dst = (rt_uint16_t *)info->dst;
    rt_uint32_t src_skip = info->src_skip;
    rt_uint32_t dst_skip = info->dst_skip >> 1;

    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            if (0xFFU != info->a) {
                srcA = ((*src) * info->a + 128) / 0xFFU;
            } else {
                srcA = (*src);
            }

            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGB_FROM_RGB565(*dst, dstR, dstG, dstB);
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                RGB565_FROM_RGB(*dst, dstR, dstG, dstB);
            }
            src++;
            dst++;
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* alpha -> RGB888 blending with alpha */
static void blit_a_to_rgb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint8_t *src = info->src;
    #ifdef RTGUI_USING_RGB888_AS_32BIT
        rt_uint32_t *dst = (rt_uint32_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 2;
    #else
        rt_uint8_t *dst = (rt_uint8_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip;
    #endif
    rt_uint32_t src_skip = info->src_skip;

    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            if (0xFFU != info->a) {
                srcA = ((*src) * info->a + 128) / 0xFFU;
            } else {
                srcA = (*src);
            }
            src++;
            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                    dst++;
                #else
                    *dst++ = srcR;
                    *dst++ = srcG;
                    *dst++ = srcB;
                #endif
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    dst++;
                #else
                    dst += 3;
                #endif
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, dstR, dstG, dstB);
                    dst++;
                #else
                    *dst++ = dstR;
                    *dst++ = dstG;
                    *dst++ = dstB;
                #endif
            }
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* alpha -> ARGB888 blending with alpha */
static void blit_a_to_argb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint8_t *src = info->src;
    rt_uint32_t *dst = (rt_uint32_t *)info->dst;
    rt_uint32_t src_skip = info->src_skip;
    rt_uint32_t dst_skip = info->dst_skip >> 2;

    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            if (0xFFU != info->a) {
                srcA = ((*src) * info->a + 128) / 0xFFU;
            } else {
                srcA = (*src);
            }

            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, 0xFFU);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB, dstA;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGBA_FROM_ARGB8888(*dst, dstR, dstG, dstB, dstA);
                if (dstA) {
                    dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                    dstA = srcA + ((0xFFU - srcA) * dstA) / 0xFFU;
                    ARGB8888_FROM_RGBA(*dst, dstR, dstG, dstB, dstA);
                } else {
                    ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, srcA);
                }
            }
            src++;
            dst++;
        }
        src += src_skip;
        dst += dst_skip;
    }
}

/* alpha color -> RGB565 blending with alpha */
static void blit_color_to_rgb565_alpha(rtgui_blit_info_t * info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint16_t *dst = (rt_uint16_t *)info->dst;
    rt_uint32_t dst_skip = info->dst_skip >> 1;

    srcA = info->a;
    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                RGB565_FROM_RGB(*dst, srcR, srcG, srcB);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGB_FROM_RGB565(*dst, dstR, dstG, dstB);
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                RGB565_FROM_RGB(*dst, dstR, dstG, dstB);
            }
            dst++;
        }
        dst += dst_skip;
    }
}

/* alpha color -> RGB888 blending with alpha */
static void blit_color_to_rgb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    #ifdef RTGUI_USING_RGB888_AS_32BIT
        rt_uint32_t *dst = (rt_uint32_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip >> 2;
    #else
        rt_uint8_t *dst = (rt_uint8_t *)info->dst;
        rt_uint32_t dst_skip = info->dst_skip;
    #endif

    srcA = info->a;
    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--) {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                    dst++;
                #else
                    *dst++ = srcR;
                    *dst++ = srcG;
                    *dst++ = srcB;
                #endif
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef RTGUI_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, dstR, dstG, dstB);
                    dst++;
                #else
                    *dst++ = dstR;
                    *dst++ = dstG;
                    *dst++ = dstB;
                #endif
            }
        }
        dst += dst_skip;
    }
}

/* alpha color -> ARGB888 blending with alpha */
static void blit_color_to_argb888_alpha(rtgui_blit_info_t *info) {
    rt_uint32_t srcR, srcG, srcB, srcA;
    rt_uint32_t height = info->dst_h;
    rt_uint32_t *dst = (rt_uint32_t *)info->dst;
    rt_uint32_t dst_skip = info->dst_skip >> 2;

    srcA = info->a;
    srcR = info->r;
    srcG = info->g;
    srcB = info->b;

    while (height--)  {
        rt_uint32_t width = info->dst_w;

        while (width--) {
            /* not do alpha blend */
            if (0xF8 == (srcA & 0xF8)) {
                ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, 0xFFU);
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
            } else {
                rt_uint32_t dstR, dstG, dstB, dstA;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                RGBA_FROM_ARGB8888(*dst, dstR, dstG, dstB, dstA);
                if (dstA) {
                    dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                    dstA = srcA + ((0xFFU - srcA) * dstA) / 0xFFU;
                    ARGB8888_FROM_RGBA(*dst, dstR, dstG, dstB, dstA);
                } else {
                    ARGB8888_FROM_RGBA(*dst, srcR, srcG, srcB, srcA);
                }
            }
            dst++;
        }
        dst += dst_skip;
    }
}

void rtgui_blit(rtgui_blit_info_t *info) {
    if (!info->src_h || !info->src_w || !info->dst_h || !info->dst_w)
        return;

    /* We only use the dst_w in the low level drivers. So adjust the info right
     * here. Note the origin is always (0, 0). */
    if (info->src_w < info->dst_w) {
        info->dst_w = info->src_w;
        info->dst_skip = info->dst_pitch - \
            info->dst_w * rtgui_color_get_bpp(info->dst_fmt);
    } else if (info->src_w > info->dst_w) {
        info->src_skip = info->src_pitch - \
            info->dst_w * rtgui_color_get_bpp(info->src_fmt);
    }

    if (info->dst_h > info->src_h) {
        info->dst_h = info->src_h;
    }

    if (info->src_fmt == RTGRAPHIC_PIXEL_FORMAT_RGB565) {
        switch (info->dst_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            blit_rgb565_to_rgb565_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            blit_rgb565_to_rgb888_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            blit_rgb565_to_argb888_alpha(info);
            break;
        }
    } else if (info->src_fmt == RTGRAPHIC_PIXEL_FORMAT_RGB888) {
        switch (info->dst_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            blit_rgb888_to_rgb565_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            blit_rgb888_to_rgb888_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            blit_rgb888_to_argb888_alpha(info);
            break;
        }
    } else if (info->src_fmt == RTGRAPHIC_PIXEL_FORMAT_ARGB888) {
        switch (info->dst_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            blit_argb888_to_rgb565_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            blit_argb888_to_rgb888_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            blit_argb888_to_argb888_alpha(info);
            break;
        }
    } else if (info->src_fmt == RTGRAPHIC_PIXEL_FORMAT_ALPHA) {
        switch (info->dst_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            blit_a_to_rgb565_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            blit_a_to_rgb888_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            blit_a_to_argb888_alpha(info);
            break;
        }
    } else if (info->src_fmt == RTGRAPHIC_PIXEL_FORMAT_COLOR) {
        switch (info->dst_fmt) {
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            blit_color_to_rgb565_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            blit_color_to_rgb888_alpha(info);
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            blit_color_to_argb888_alpha(info);
            break;
        }
    }
}
RTM_EXPORT(rtgui_blit);

#endif /* RTGUI_USING_DC_BUFFER */
