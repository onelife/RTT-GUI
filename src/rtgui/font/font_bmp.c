/*
 * File      : font_bmp.c
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
 * 2019-06-03     onelife      make a generic BMP font engine
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#if (CONFIG_USING_FONT_FILE)
# ifndef RT_USING_DFS
#  error "Please enable RT_USING_DFS for CONFIG_USING_FONT_FILE"
# endif
# include "components/dfs/include/dfs_posix.h"
#endif

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "FNT_BMP"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
#if (CONFIG_USING_FONT_FILE)
static rt_err_t bmp_font_open(rtgui_font_t *font);
static void bmp_font_close(rtgui_font_t *font);
#endif
static rt_uint8_t bmp_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect);
static rt_uint8_t bmp_font_get_width(rtgui_font_t *font, const char *utf8);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Imported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
const rtgui_font_engine_t bmp_font_engine = {
    .font_init = RT_NULL,
    #if (CONFIG_USING_FONT_FILE)
        .font_open = bmp_font_open,
        .font_close = bmp_font_close,
    #else
        .font_open = RT_NULL,
        .font_close = RT_NULL,
    #endif
    .font_draw_char = bmp_font_draw_char,
    .font_get_width = bmp_font_get_width,
};

#if (CONFIG_USING_FONT_12)
# if (CONFIG_USING_FONT_HZ)
#  if (CONFIG_USING_FONT_FILE)
    static rtgui_bmp_font_t _hz12 = {
        .data = RT_NULL,
        .fname = "/font/hz12.fnt",
        .fd = -1,
    };
#  else
#   include "include/font/hz12font.h"
    static rtgui_bmp_font_t _hz12 = {
        .data = (void *)hz12_font,
    };
#  endif

    rtgui_font_t rtgui_font_hz12 = {
        .family = "hz",
        .engine = &bmp_font_engine,
        .height = 12,
        .width = 12,
        .start = 0xA1A1,
        .end = 0xF7FE,
        .dft = 0xA3BF,
        .size = 18,
        .data = &_hz12,
        .refer_count = 1,
        .list = { RT_NULL },
    };
# endif /* CONFIG_USING_FONT_HZ */
#endif /* CONFIG_USING_FONT_12 */

#if (CONFIG_USING_FONT_16)
# if (CONFIG_USING_FONT_FILE)
    static rtgui_bmp_font_t _asc16 = {
        .data = RT_NULL,
        .fname = "/font/asc16.fnt",
        .fd = -1,
    };
# else
#   include "include/font/asc16font.h"
    static rtgui_bmp_font_t _asc16 = {
        .data = (void *)asc16_font,
    };
# endif

    rtgui_font_t rtgui_font_asc16 = {
        .family = "asc",
        .engine = &bmp_font_engine,
        .height = 16,
        .width = 8,
        .start = 0x00,
        .end = 0xFF,
        .dft = 0x3F,
        .size = 16,
        .data = &_asc16,
        .refer_count = 1,
        .list = { RT_NULL },
    };

# if (CONFIG_USING_FONT_HZ)
#  if (CONFIG_USING_FONT_FILE)
    static rtgui_bmp_font_t _hz16 = {
        .data = RT_NULL,
        .fname = "/font/hz16.fnt",
        .fd = -1,
    };
#  else
#   include "include/font/hz16font.h"
    static rtgui_bmp_font_t _hz16 = {
        .data = (void *)hz16_font,
    };
#  endif

    rtgui_font_t rtgui_font_hz16 = {
        .family = "hz",
        .engine = &bmp_font_engine,
        .height = 16,
        .width = 16,
        .start = 0xA1A1,
        .end = 0xF7FE,
        .dft = 0xA3BF,
        .size = 32,
        .data = &_hz16,
        .refer_count = 1,
        .list = { RT_NULL },
    };
# endif /* CONFIG_USING_FONT_HZ */
#endif /* CONFIG_USING_FONT_16 */

/* Private functions ---------------------------------------------------------*/
#if (CONFIG_USING_FONT_FILE)
static rt_err_t bmp_font_open(rtgui_font_t *font) {
    rtgui_bmp_font_t *bmp_fnt;
    rt_err_t ret;

    bmp_fnt = font->data;
    ret = RT_EOK;

    do {
        if (!bmp_fnt->fname) {
            if (!bmp_fnt->data) {
                /* no in file and no in ROM */
                ret = -RT_ERROR;
            }
            break;
        }
        bmp_fnt->fd = open(bmp_fnt->fname, O_RDONLY);
        if (bmp_fnt->fd < 0) {
            ret = -RT_EIO;
            break;
        }
        bmp_fnt->data = rtgui_malloc(font->size);
        if (!bmp_fnt->data) {
            ret = -RT_ENOMEM;
            break;
        }
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("open %s err [%d]", bmp_fnt->fname, ret);
    } else if (bmp_fnt->fname) {
        // LOG_D("open %s", bmp_fnt->fname);
    }
    return ret;
}

static void bmp_font_close(rtgui_font_t *font) {
    rtgui_bmp_font_t *bmp_fnt;

    bmp_fnt = font->data;

    if (bmp_fnt->fd >= 0) {
        close(bmp_fnt->fd);
        bmp_fnt->fd = -1;
        if (bmp_fnt->data) {
            rtgui_free(bmp_fnt->data);
            bmp_fnt->data = RT_NULL;
        }
        // LOG_D("closed %s", bmp_fnt->fname);
    }
}
#endif /* CONFIG_USING_FONT_FILE */

static const rt_uint8_t *_bmp_font_get_data(rtgui_font_t *font,
    rt_uint16_t code) {
    rtgui_bmp_font_t *bmp_fnt;
    rt_uint32_t offset;

    bmp_fnt = font->data;
    offset = code - font->start;
    if (!rt_strcasecmp(font->family, "hz")) {
        offset = (94 * (offset >> 8) + (offset & 0x00ff)) * font->size;
    } else {
        offset *= font->size;
    }

    #if (CONFIG_USING_FONT_FILE)
        if (bmp_fnt->fname) {
            do {
                if (bmp_fnt->fd < 0) {
                    LOG_E("no fd %s", bmp_fnt->fname);
                    break;
                }
                if (lseek(bmp_fnt->fd, offset, SEEK_SET) < 0) {
                    LOG_E("seek %s err", bmp_fnt->fname);
                    break;
                }
                if (font->size != (rt_uint16_t)read(
                    bmp_fnt->fd, bmp_fnt->data, font->size)) {
                    LOG_E("read %s err", bmp_fnt->fname);
                    break;
                }
            } while (0);
            return bmp_fnt->data;
        } else {
    #endif

        return bmp_fnt->data + offset;

    #if (CONFIG_USING_FONT_FILE)
        }
    #endif
}

static rt_uint8_t bmp_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect) {
    rtgui_color_t bc;
    rt_uint16_t style;
    const rt_uint8_t *data;
    rt_int8_t oft;
    rt_uint8_t w, h, sft, lst_sft, bit, line;

    bc = RTGUI_DC_BC(dc);
    style = rtgui_dc_get_gc(dc)->textstyle;

    if ((code < font->start) || (code > font->end)) {
        code = font->dft;
        LOG_D("used dft code %x", code);
    }
    data = _bmp_font_get_data(font, code);

    w = _MIN(RECT_W(*rect), font->width);
    h = _MIN(RECT_H(*rect), font->height);
    oft = -1;
    sft = 0;
    lst_sft = 0;

    for (line = 0; line < h; line++) {
        /* draw a line */
        for (bit = 0; bit < w; bit++) {
            sft = (bit + lst_sft) % 8;
            if (!sft) oft++;
            if (data[oft] & (1 << (7 - sft))) {
                rtgui_dc_draw_point(dc, rect->x1 + bit, rect->y1 + line);
            } else if (style & RTGUI_TEXTSTYLE_DRAW_BACKGROUND) {
                rtgui_dc_draw_color_point(
                    dc, rect->x1 + bit, rect->y1 + line, bc);
            }
        }
        lst_sft = sft + 1;
    }

    return w;
}

static rt_uint8_t bmp_font_get_width(rtgui_font_t *font, const char *utf8) {
    /* fix width for BMP font*/
    (void)utf8;
    return font->width;
}

/* Public functions ----------------------------------------------------------*/
