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


/* RGB888 -> RGB565 blending */
static void blit_line_rgb888_to_rgb565(rt_uint8_t *_dst, rt_uint8_t *src,
    rt_uint32_t len, rt_uint8_t scale, rtgui_image_palette_t *palette) {
    rt_uint16_t *dst = (rt_uint16_t *)_dst;
    rt_uint8_t *end = src + len;
    rt_uint8_t step = _BIT2BYTE(GUIENGINE_RGB888_PIXEL_BITS) << scale;
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
    rt_uint8_t step = _BIT2BYTE(GUIENGINE_RGB888_PIXEL_BITS) << scale;
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
    //  test
    if (scale > 3) {
        shift_step = 8;
        step = 1 << (scale - 3);
    } else {
        shift_step = 1 << scale;
        step = 1;
    }

    for ( ; src < end; src += step)
        for (shift = 0; shift < 8; shift += shift_step)
            if (*src & (0x01 << (7 - shift))) {
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

rtgui_blit_line_func2 rtgui_get_blit_line_func(rt_uint8_t src_fmt,
    rt_uint8_t dst_fmt) {

    switch (dst_fmt) {
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

static void rtgui_blit_line_1_3(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
    return;
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

/* Special optimized blit for RGB 5-6-5 --> ARGB 8-8-8-8 */
static const rt_uint32_t RGB565_ARGB8888_LUT[512] =
{
    0x00000000, 0xff000000, 0x00000008, 0xff002000,
    0x00000010, 0xff004000, 0x00000018, 0xff006100,
    0x00000020, 0xff008100, 0x00000029, 0xff00a100,
    0x00000031, 0xff00c200, 0x00000039, 0xff00e200,
    0x00000041, 0xff080000, 0x0000004a, 0xff082000,
    0x00000052, 0xff084000, 0x0000005a, 0xff086100,
    0x00000062, 0xff088100, 0x0000006a, 0xff08a100,
    0x00000073, 0xff08c200, 0x0000007b, 0xff08e200,
    0x00000083, 0xff100000, 0x0000008b, 0xff102000,
    0x00000094, 0xff104000, 0x0000009c, 0xff106100,
    0x000000a4, 0xff108100, 0x000000ac, 0xff10a100,
    0x000000b4, 0xff10c200, 0x000000bd, 0xff10e200,
    0x000000c5, 0xff180000, 0x000000cd, 0xff182000,
    0x000000d5, 0xff184000, 0x000000de, 0xff186100,
    0x000000e6, 0xff188100, 0x000000ee, 0xff18a100,
    0x000000f6, 0xff18c200, 0x000000ff, 0xff18e200,
    0x00000400, 0xff200000, 0x00000408, 0xff202000,
    0x00000410, 0xff204000, 0x00000418, 0xff206100,
    0x00000420, 0xff208100, 0x00000429, 0xff20a100,
    0x00000431, 0xff20c200, 0x00000439, 0xff20e200,
    0x00000441, 0xff290000, 0x0000044a, 0xff292000,
    0x00000452, 0xff294000, 0x0000045a, 0xff296100,
    0x00000462, 0xff298100, 0x0000046a, 0xff29a100,
    0x00000473, 0xff29c200, 0x0000047b, 0xff29e200,
    0x00000483, 0xff310000, 0x0000048b, 0xff312000,
    0x00000494, 0xff314000, 0x0000049c, 0xff316100,
    0x000004a4, 0xff318100, 0x000004ac, 0xff31a100,
    0x000004b4, 0xff31c200, 0x000004bd, 0xff31e200,
    0x000004c5, 0xff390000, 0x000004cd, 0xff392000,
    0x000004d5, 0xff394000, 0x000004de, 0xff396100,
    0x000004e6, 0xff398100, 0x000004ee, 0xff39a100,
    0x000004f6, 0xff39c200, 0x000004ff, 0xff39e200,
    0x00000800, 0xff410000, 0x00000808, 0xff412000,
    0x00000810, 0xff414000, 0x00000818, 0xff416100,
    0x00000820, 0xff418100, 0x00000829, 0xff41a100,
    0x00000831, 0xff41c200, 0x00000839, 0xff41e200,
    0x00000841, 0xff4a0000, 0x0000084a, 0xff4a2000,
    0x00000852, 0xff4a4000, 0x0000085a, 0xff4a6100,
    0x00000862, 0xff4a8100, 0x0000086a, 0xff4aa100,
    0x00000873, 0xff4ac200, 0x0000087b, 0xff4ae200,
    0x00000883, 0xff520000, 0x0000088b, 0xff522000,
    0x00000894, 0xff524000, 0x0000089c, 0xff526100,
    0x000008a4, 0xff528100, 0x000008ac, 0xff52a100,
    0x000008b4, 0xff52c200, 0x000008bd, 0xff52e200,
    0x000008c5, 0xff5a0000, 0x000008cd, 0xff5a2000,
    0x000008d5, 0xff5a4000, 0x000008de, 0xff5a6100,
    0x000008e6, 0xff5a8100, 0x000008ee, 0xff5aa100,
    0x000008f6, 0xff5ac200, 0x000008ff, 0xff5ae200,
    0x00000c00, 0xff620000, 0x00000c08, 0xff622000,
    0x00000c10, 0xff624000, 0x00000c18, 0xff626100,
    0x00000c20, 0xff628100, 0x00000c29, 0xff62a100,
    0x00000c31, 0xff62c200, 0x00000c39, 0xff62e200,
    0x00000c41, 0xff6a0000, 0x00000c4a, 0xff6a2000,
    0x00000c52, 0xff6a4000, 0x00000c5a, 0xff6a6100,
    0x00000c62, 0xff6a8100, 0x00000c6a, 0xff6aa100,
    0x00000c73, 0xff6ac200, 0x00000c7b, 0xff6ae200,
    0x00000c83, 0xff730000, 0x00000c8b, 0xff732000,
    0x00000c94, 0xff734000, 0x00000c9c, 0xff736100,
    0x00000ca4, 0xff738100, 0x00000cac, 0xff73a100,
    0x00000cb4, 0xff73c200, 0x00000cbd, 0xff73e200,
    0x00000cc5, 0xff7b0000, 0x00000ccd, 0xff7b2000,
    0x00000cd5, 0xff7b4000, 0x00000cde, 0xff7b6100,
    0x00000ce6, 0xff7b8100, 0x00000cee, 0xff7ba100,
    0x00000cf6, 0xff7bc200, 0x00000cff, 0xff7be200,
    0x00001000, 0xff830000, 0x00001008, 0xff832000,
    0x00001010, 0xff834000, 0x00001018, 0xff836100,
    0x00001020, 0xff838100, 0x00001029, 0xff83a100,
    0x00001031, 0xff83c200, 0x00001039, 0xff83e200,
    0x00001041, 0xff8b0000, 0x0000104a, 0xff8b2000,
    0x00001052, 0xff8b4000, 0x0000105a, 0xff8b6100,
    0x00001062, 0xff8b8100, 0x0000106a, 0xff8ba100,
    0x00001073, 0xff8bc200, 0x0000107b, 0xff8be200,
    0x00001083, 0xff940000, 0x0000108b, 0xff942000,
    0x00001094, 0xff944000, 0x0000109c, 0xff946100,
    0x000010a4, 0xff948100, 0x000010ac, 0xff94a100,
    0x000010b4, 0xff94c200, 0x000010bd, 0xff94e200,
    0x000010c5, 0xff9c0000, 0x000010cd, 0xff9c2000,
    0x000010d5, 0xff9c4000, 0x000010de, 0xff9c6100,
    0x000010e6, 0xff9c8100, 0x000010ee, 0xff9ca100,
    0x000010f6, 0xff9cc200, 0x000010ff, 0xff9ce200,
    0x00001400, 0xffa40000, 0x00001408, 0xffa42000,
    0x00001410, 0xffa44000, 0x00001418, 0xffa46100,
    0x00001420, 0xffa48100, 0x00001429, 0xffa4a100,
    0x00001431, 0xffa4c200, 0x00001439, 0xffa4e200,
    0x00001441, 0xffac0000, 0x0000144a, 0xffac2000,
    0x00001452, 0xffac4000, 0x0000145a, 0xffac6100,
    0x00001462, 0xffac8100, 0x0000146a, 0xffaca100,
    0x00001473, 0xffacc200, 0x0000147b, 0xfface200,
    0x00001483, 0xffb40000, 0x0000148b, 0xffb42000,
    0x00001494, 0xffb44000, 0x0000149c, 0xffb46100,
    0x000014a4, 0xffb48100, 0x000014ac, 0xffb4a100,
    0x000014b4, 0xffb4c200, 0x000014bd, 0xffb4e200,
    0x000014c5, 0xffbd0000, 0x000014cd, 0xffbd2000,
    0x000014d5, 0xffbd4000, 0x000014de, 0xffbd6100,
    0x000014e6, 0xffbd8100, 0x000014ee, 0xffbda100,
    0x000014f6, 0xffbdc200, 0x000014ff, 0xffbde200,
    0x00001800, 0xffc50000, 0x00001808, 0xffc52000,
    0x00001810, 0xffc54000, 0x00001818, 0xffc56100,
    0x00001820, 0xffc58100, 0x00001829, 0xffc5a100,
    0x00001831, 0xffc5c200, 0x00001839, 0xffc5e200,
    0x00001841, 0xffcd0000, 0x0000184a, 0xffcd2000,
    0x00001852, 0xffcd4000, 0x0000185a, 0xffcd6100,
    0x00001862, 0xffcd8100, 0x0000186a, 0xffcda100,
    0x00001873, 0xffcdc200, 0x0000187b, 0xffcde200,
    0x00001883, 0xffd50000, 0x0000188b, 0xffd52000,
    0x00001894, 0xffd54000, 0x0000189c, 0xffd56100,
    0x000018a4, 0xffd58100, 0x000018ac, 0xffd5a100,
    0x000018b4, 0xffd5c200, 0x000018bd, 0xffd5e200,
    0x000018c5, 0xffde0000, 0x000018cd, 0xffde2000,
    0x000018d5, 0xffde4000, 0x000018de, 0xffde6100,
    0x000018e6, 0xffde8100, 0x000018ee, 0xffdea100,
    0x000018f6, 0xffdec200, 0x000018ff, 0xffdee200,
    0x00001c00, 0xffe60000, 0x00001c08, 0xffe62000,
    0x00001c10, 0xffe64000, 0x00001c18, 0xffe66100,
    0x00001c20, 0xffe68100, 0x00001c29, 0xffe6a100,
    0x00001c31, 0xffe6c200, 0x00001c39, 0xffe6e200,
    0x00001c41, 0xffee0000, 0x00001c4a, 0xffee2000,
    0x00001c52, 0xffee4000, 0x00001c5a, 0xffee6100,
    0x00001c62, 0xffee8100, 0x00001c6a, 0xffeea100,
    0x00001c73, 0xffeec200, 0x00001c7b, 0xffeee200,
    0x00001c83, 0xfff60000, 0x00001c8b, 0xfff62000,
    0x00001c94, 0xfff64000, 0x00001c9c, 0xfff66100,
    0x00001ca4, 0xfff68100, 0x00001cac, 0xfff6a100,
    0x00001cb4, 0xfff6c200, 0x00001cbd, 0xfff6e200,
    0x00001cc5, 0xffff0000, 0x00001ccd, 0xffff2000,
    0x00001cd5, 0xffff4000, 0x00001cde, 0xffff6100,
    0x00001ce6, 0xffff8100, 0x00001cee, 0xffffa100,
    0x00001cf6, 0xffffc200, 0x00001cff, 0xffffe200
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

static void rtgui_blit_line_1_4(rt_uint8_t *_dst, rt_uint8_t *_src,
    rt_uint32_t len)
{
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

static const rtgui_blit_line_func _blit_table[4][4] = {
    /* 1_1, 2_1, 3_1, 4_1 */
    {RT_NULL, RT_NULL, rtgui_blit_line_3_1, rtgui_blit_line_4_1 },
    /* 1_2, 2_2, 3_2, 4_2 */
    {RT_NULL, RT_NULL, rtgui_blit_line_3_2, rtgui_blit_line_4_2 },
    /* 1_3, 2_3, 3_3, 4_3 */
    {rtgui_blit_line_1_3, rtgui_blit_line_2_3, RT_NULL, rtgui_blit_line_4_3 },
    /* 1_4, 2_4, 3_4, 4_4 */
    {rtgui_blit_line_1_4, rtgui_blit_line_2_4, rtgui_blit_line_3_4, RT_NULL },
};

rtgui_blit_line_func rtgui_blit_line_get(rt_uint8_t dst_bpp,
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
        #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                        RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                    #else
                        dstR = *dst;
                        dstG = *(dst + 1);
                        dstB = *(dst + 2);
                    #endif
                    dstR = ((srcR * alpha) + (inverse_alpha * dstR)) >> 8;
                    dstG = ((srcG * alpha) + (inverse_alpha * dstG)) >> 8;
                    dstB = ((srcB * alpha) + (inverse_alpha * dstB)) >> 8;
                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
        #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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

                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
        #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                        *dst++ = *src++;
                    #else
                        *dst++ = *src++;
                        *dst++ = *src++;
                        *dst++ = *src++;
                    #endif
                } else {
                    rt_uint32_t dstR, dstG, dstB;
                    rt_uint32_t inverse_alpha = 0xFFU - alpha;

                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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

                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
        #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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

                    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                    dst++;
                #else
                    *dst++ = (rt_uint8_t)srcR;
                    *dst++ = (rt_uint8_t)srcG;
                    *dst++ = (rt_uint8_t)srcB;
                #endif
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    dst++;
                #else
                    dst += 3;
                #endif
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    RGB888_FROM_RGB(*dst, srcR, srcG, srcB);
                    dst++;
                #else
                    *dst++ = srcR;
                    *dst++ = srcG;
                    *dst++ = srcB;
                #endif
            } else if (0x00 == (srcA & 0xF8)) {
                /* keep original pixel data */
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    dst++;
                #else
                    dst += 3;
                #endif
            } else {
                rt_uint32_t dstR, dstG, dstB;
                rt_uint32_t inverse_alpha = 0xFFU - srcA;

                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
    #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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

                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
                    RGB_FROM_RGB888(*dst, dstR, dstG, dstB);
                #else
                    dstR = *dst;
                    dstG = *(dst + 1);
                    dstB = *(dst + 2);
                #endif
                dstR = ((srcR * srcA) + (inverse_alpha * dstR)) >> 8;
                dstG = ((srcG * srcA) + (inverse_alpha * dstG)) >> 8;
                dstB = ((srcB * srcA) + (inverse_alpha * dstB)) >> 8;
                #ifdef GUIENGINE_USING_RGB888_AS_32BIT
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

#ifdef RTGUI_USING_DC_BUFFER

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

#endif /* RTGUI_USING_DC_BUFFER */
