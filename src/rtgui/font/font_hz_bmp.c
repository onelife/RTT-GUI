/*
 * File      : font_hz_bmp.c
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
 * 2010-09-15     Bernard      first version
 */
/* Includes ------------------------------------------------------------------*/
#include "include/font.h"

#ifdef GUIENGINE_USING_HZ_BMP
#include "include/dc.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "FNT_HZB"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */


/* Private function prototype ------------------------------------------------*/
static void rtgui_hz_bitmap_font_draw_text(rtgui_font_t *font, rtgui_dc_t *dc,
    const char *text, rt_ubase_t len, rtgui_rect_t *rect);
static void rtgui_hz_bitmap_font_get_metrics(rtgui_font_t *font,
    const char *text, rtgui_rect_t *rect);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define UTF8_SIZE(byte0)                    \
    (((byte0 & 0xF0) == 0xF0) ? 4 : (       \
     ((byte0 & 0xF0) == 0xE0) ? 3 : (       \
     ((byte0 & 0xF0) == 0xC0) ? 2 : (       \
     1))))
#define UTF8_TO_UNICODE(utf, sz)            \
    (rt_uint16_t)                           \
    ((sz == 1) ? (utf[0] & 0x7F) : (        \
     (sz == 2) ? (((rt_uint16_t)(utf[0] & 0x1F) << 6) | (utf[1] & 0x3F)) : ( \
     (sz == 3) ? (((rt_uint16_t)(utf[0] & 0x1F) << 12) | ((rt_uint16_t)(utf[1] & 0x3F) << 6) | (utf[2] & 0x3F)) : ( \
     (((rt_uint16_t)(utf[0] & 0x1F) << 18) | ((rt_uint16_t)(utf[1] & 0x3F) << 12) | ((rt_uint16_t)(utf[2] & 0x3F) << 6) | (utf[3] & 0x3F))))))
#define CONVERT_COLOR(fmt, c)            \
    ((fmt = RTGRAPHIC_PIXEL_FORMAT_MONO) ? rtgui_color_to_mono(c) : ( \
     (fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565) ? rtgui_color_to_565(c) : ( \
     (fmt = RTGRAPHIC_PIXEL_FORMAT_BGR565) ? rtgui_color_to_565p(c) : ( \
     (fmt = RTGRAPHIC_PIXEL_FORMAT_RGB888) ? rtgui_color_to_888(c) : ( \
     0)))))
#define hw_drv()                    (rtgui_graphic_driver_get_default())

/* Private variables ---------------------------------------------------------*/
#ifdef GUIENGINE_USING_FONT12
    #include "include/font/hz12font.h"

    static const rtgui_font_bitmap_t _hz12 = {
        .bmp = hz12_font,
        ._len = 2,
        .offset = RT_NULL,
        .width = 12,
        .height = 12,
        ._start = 0xA1A1,
        ._end = 0xF7FE,
        ._dft = 0xA3BF - 0xA1A1,
        .size = 18,
    };
#endif /* GUIENGINE_USING_FONT12 */
#ifdef GUIENGINE_USING_FONT16
    #include "include/font/hz16font.h"

    static const rtgui_font_bitmap_t _hz16 = {
        .bmp = hz16_font,
        ._len = 2,
        .offset = RT_NULL,
        .width = 16,
        .height = 16,
        ._start = 0xA1A1,
        ._end = 0xF7FE,
        ._dft = 0xA3BF - 0xA1A1,
        .size = 32,
    };
#endif /* GUIENGINE_USING_FONT16 */

/* Imported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
const rtgui_font_engine_t hz_bmp_font_engine = {
    RT_NULL,
    RT_NULL,
    rtgui_hz_bitmap_font_draw_text,
    rtgui_hz_bitmap_font_get_metrics
};
#ifdef GUIENGINE_USING_FONT12
    const rtgui_font_t rtgui_font_hz12 = {
        .family = "hz",
        .height = 12,
        .refer_count = 1,
        .engine = &hz_bmp_font_engine,
        .data = &_hz12,
        .list = { RT_NULL },
    };
#endif /* GUIENGINE_USING_FONT12 */
#ifdef GUIENGINE_USING_FONT16
    const rtgui_font_t rtgui_font_hz16 = {
        .family = "hz",
        .height = 16,
        .refer_count = 1,
        .engine = &hz_bmp_font_engine,
        .data = &_hz16,
        .list = { RT_NULL },
    };
#endif /* GUIENGINE_USING_FONT16 */

/* Private functions ---------------------------------------------------------*/
rt_inline const rt_uint8_t *_bmp_font_get_data(rtgui_font_bitmap_t *bmp_font,
    rt_uint16_t code) {
    rt_uint32_t offset;

    offset = code - bmp_font->_start;

    offset = (94 * (offset >> 8) + (offset & 0x00ff)) * bmp_font->size;
    LOG_E("code %x offset %d %d", code, offset, bmp_font->size);

    return bmp_font->bmp + offset;
}

static void _bmp_font_draw_text(rtgui_font_bitmap_t *bmp_font, rtgui_dc_t *dc,
    const char *_utf8, rt_ubase_t len, rtgui_rect_t *rect) {
    rtgui_color_t fc, bc;
    rt_uint16_t style;
    rt_uint8_t *lineBuf;
    rt_uint32_t h, x, y;

    RT_ASSERT(bmp_font != RT_NULL);

    fc = RTGUI_DC_FC(dc);
    bc = RTGUI_DC_BC(dc);
    /* get text style */
    style = rtgui_dc_get_gc(dc)->textstyle;

    /* drawing height */
    h = _MIN(RECT_H(*rect), bmp_font->height);

    lineBuf = RT_NULL;

    do {
        rt_uint8_t hw_bytePP = _BIT2BYTE(hw_drv()->bits_per_pixel);
        rt_uint32_t bufSize = bmp_font->width * hw_bytePP;
        rt_uint8_t idx;

        lineBuf = rtgui_malloc(bufSize);
        if (!lineBuf) {
            LOG_E("no mem to disp");
            break;
        }

        idx = 0;
        x = rect->x1;
        y = rect->y1;
        while ((x < rect->x2) && (idx < len)) {
            /* get font data */
            rt_uint8_t *utf8 = (rt_uint8_t *)_utf8 + idx;
            LOG_E("utf %x %x %x %x", utf8[0], utf8[1], utf8[2], utf8[3]);
            rt_uint8_t size = UTF8_SIZE(*utf8);
            rt_uint16_t code = UnicodeToGB2312(UTF8_TO_UNICODE(utf8, size));
            // LOG_E("size %d, code %x", size, code);
            const rt_uint8_t *data = _bmp_font_get_data(bmp_font, code);
            rt_int8_t oft = -1;
            rt_uint8_t sft = 0;
            rt_uint8_t lst_sft = 0;
            rt_uint8_t bit, line;
            // rtgui_color_t c;

            /* draw a char */
            for (line = 0; line < h; line++) {
                rt_uint32_t w;

                if ((x + bmp_font->width) > rect->x2) {
                    w = rect->x2 - x;
                } else {
                    w = bmp_font->width;
                }

                /* draw a line */
                for (bit = 0; bit < w; bit++) {
                    sft = (bit + lst_sft) % 8;
                    if (!sft) {
                        // LOG_E("oft %d data %x", oft, data[oft+1]);
                        oft++;
                    }
                    if (data[oft] & (1 << (7 - sft))) {
                        rtgui_dc_draw_point(dc, x + bit, y + line);
                        LOG_E("line %d bit %d, x %d, y %d", line, bit, x + bit, y + line);
                    } else if (style & RTGUI_TEXTSTYLE_DRAW_BACKGROUND) {
                        rtgui_dc_draw_color_point(dc, x + bit, y + line, bc);
                    }
                }
                lst_sft = sft + 1;
                    // c = (rtgui_color_t)CONVERT_COLOR(hw_drv()->pixel_format, fc);
                    // rt_memcpy(lineBuf + idx, &c, hw_bytePP);
                    // [j] = CONVERT_COLOR
            }
            idx += size;
            x += bmp_font->width;
        }
    } while (0);

    rtgui_free(lineBuf);
}

static void rtgui_hz_bitmap_font_draw_text(rtgui_font_t *font, rtgui_dc_t *dc,
    const char *utf8, rt_ubase_t length, rtgui_rect_t *rect) {
    rt_uint32_t len;
    rtgui_font_t *efont;
    rtgui_font_bitmap_t *bmp_font = (rtgui_font_bitmap_t *)(font->data);
    rtgui_rect_t text_rect;

    RT_ASSERT(dc != RT_NULL);

    rtgui_font_get_metrics(rtgui_dc_get_gc(dc)->font, utf8, &text_rect);
    rtgui_rect_move_to_align(rect, &text_rect, RTGUI_DC_TEXTALIGN(dc));

    /* get English font */
    efont = rtgui_font_refer("asc", bmp_font->height);
    if (!efont) efont = rtgui_font_default(); /* use system default font */

    while (length > 0) {
        len = 0;
        while (((rt_uint8_t)*(utf8 + len) < 0x80) && *(utf8 + len) && \
            (len < length)) {
            len++;
        }
        /* draw text with English font */
        if (len > 0) {
            rtgui_font_draw(efont, dc, utf8, len, &text_rect);
            utf8 += len;
            length -= len;
        }

        len = 0;
        while (((rt_uint8_t)*(utf8 + len) >= 0x80) && (len < length)) len++;
        if (len > 0) {
            _bmp_font_draw_text(bmp_font, dc, utf8, len, &text_rect);
            utf8 += len;
            length -= len;
        }
    }

    rtgui_font_derefer(efont);
}

static void rtgui_hz_bitmap_font_get_metrics(rtgui_font_t *font,
    const char *utf8, rtgui_rect_t *rect) {
    rtgui_font_bitmap_t *bmp_font;

    RT_ASSERT(font->data != RT_NULL);
    bmp_font = (rtgui_font_bitmap_t *)(font->data);

    /* set metrics rect */
    rect->x1 = rect->y1 = 0;
    /* Chinese font is always fixed font */
    rect->x2 = (rt_int16_t)(bmp_font->width * rt_strlen((const char *)utf8));
    rect->y2 = bmp_font->height;
}

/* Public functions ----------------------------------------------------------*/

#endif /* GUIENGINE_USING_HZ_BMP */
