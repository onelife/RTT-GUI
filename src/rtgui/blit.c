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
