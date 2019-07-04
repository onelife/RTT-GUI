/*
 * File      : font_fnt.c
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
 * 2019-06-03     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/font/font.h"

#ifdef RTGUI_USING_FONT_FILE
# ifndef RT_USING_DFS
#  error "Please enable RT_USING_DFS for RTGUI_USING_FONT_FILE"
# endif
# include "components/dfs/include/dfs_posix.h"
#endif

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "FNT_FNT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
#ifdef RTGUI_USING_FONT_FILE
static rt_err_t fnt_font_open(rtgui_font_t *font);
static void fnt_font_close(rtgui_font_t *font);
#endif
static rt_uint8_t fnt_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect);
static rt_uint8_t fnt_font_get_width(rtgui_font_t *font, const char *utf8);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Imported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
const rtgui_font_engine_t fnt_font_engine = {
    .font_init = RT_NULL,
    #ifdef RTGUI_USING_FONT_FILE
        .font_open = fnt_font_open,
        .font_close = fnt_font_close,
    #else
        .font_open = RT_NULL,
        .font_close = RT_NULL,
    #endif
    .font_draw_char = fnt_font_draw_char,
    .font_get_width = fnt_font_get_width,
};

#ifdef GUIENGINE_USING_FONT12
#include "include/font/asc12font.h"

# ifdef RTGUI_USING_FONT_FILE
    static rtgui_fnt_font_t _asc12 = {
        .data = RT_NULL,
        .offset = _sysfont_offset,
        .width = _sysfont_width,
        .fname = "/font/asc12.fnt",
        .fd = -1,
    };
# else
    static rtgui_fnt_font_t _asc12 = {
        .data = (void *)_font_bits,
        .offset = _sysfont_offset,
        .width = _sysfont_width
    };
# endif

    rtgui_font_t rtgui_font_asc12 = {
        .family = "asc",
        .engine = &fnt_font_engine,
        .height = 12,
        .width = 13,                /* max width */
        .start = 0x20,
        .end = 0xFF,
        .dft = 0x3F,
        .size = 26,                 /* max size */
        .data = &_asc12,
        .refer_count = 1,
        .list = { RT_NULL },
    };
#endif /* GUIENGINE_USING_FONT12 */

/* Private functions ---------------------------------------------------------*/
#ifdef RTGUI_USING_FONT_FILE
static rt_err_t fnt_font_open(rtgui_font_t *font) {
    rtgui_fnt_font_t *fnt_font;
    rt_err_t ret;

    fnt_font = font->data;
    ret = RT_EOK;

    do {
        if (!fnt_font->fname) {
            if (!fnt_font->data) {
                /* no in file and no in ROM */
                ret = -RT_ERROR;
            }
            break;
        }
        fnt_font->fd = open(fnt_font->fname, O_RDONLY);
        if (fnt_font->fd < 0) {
            ret = -RT_EIO;
            break;
        }
        fnt_font->data = rtgui_malloc(font->size);
        if (!fnt_font->data) {
            ret = -RT_ENOMEM;
            break;
        }
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("open %s err [%d]", fnt_font->fname, ret);
    } else if (fnt_font->fname) {
        LOG_D("open %s", fnt_font->fname);
    }
    return ret;
}

static void fnt_font_close(rtgui_font_t *font) {
    rtgui_fnt_font_t *fnt_font;

    fnt_font = font->data;

    if (fnt_font->fd >= 0) {
        close(fnt_font->fd);
        fnt_font->fd = -1;
        if (fnt_font->data) {
            rtgui_free(fnt_font->data);
            fnt_font->data = RT_NULL;
        }
        LOG_D("closed %s", fnt_font->fname);
    }
}
#endif

static const rt_uint8_t *_fnt_font_get_data(rtgui_font_t *font,
    rt_uint16_t code) {
    rtgui_fnt_font_t *fnt_font;
    rt_uint32_t offset, size;

    fnt_font = font->data;
    offset = fnt_font->offset[code - font->start];
    size = fnt_font->width[code - font->start] * 2;

    #ifdef RTGUI_USING_FONT_FILE
        if (fnt_font->fname) {
            do {
                if (fnt_font->fd < 0) {
                    LOG_E("no fd %s", fnt_font->fname);
                    break;
                }
                if (lseek(fnt_font->fd, offset, SEEK_SET) < 0) {
                    LOG_E("seek %s err", fnt_font->fname);
                    break;
                }
                if (size != (rt_uint32_t)read(
                    fnt_font->fd, fnt_font->data, size)) {
                    LOG_E("read %s err", fnt_font->fname);
                    break;
                }
            } while (0);
            return fnt_font->data;
        } else {
    #endif

        return fnt_font->data + offset;

    #ifdef RTGUI_USING_FONT_FILE
        }
    #endif
}

static rt_uint8_t fnt_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect) {
    rtgui_fnt_font_t *fnt_font;
    rtgui_color_t bc;
    rt_uint16_t style;
    const rt_uint8_t *data;
    rt_uint8_t fnt_w, w, h, oft, sft, col;

    fnt_font = font->data;
    bc = RTGUI_DC_BC(dc);
    style = rtgui_dc_get_gc(dc)->textstyle;

    if ((code < font->start) || (code > font->end)) {
        code = font->dft;
        LOG_D("used dft code %x", code);
    }
    data = _fnt_font_get_data(font, code);

    fnt_w = fnt_font->width[code - font->start];
    w = _MIN(RECT_W(*rect), fnt_w);
    h = _MIN(RECT_H(*rect), font->height);

    for (col = 0, oft = 0; col < (w << 1); col++) {
        /* draw half column */
        for (sft = 0; sft < ((col > w) ? 4 : 8); sft++) {
            rt_uint16_t y_off = ((col > w) ? (8 + sft) : sft);

            if (y_off >= h) continue;
            if (data[oft] & (1 << sft)) {
                rtgui_dc_draw_point(dc, rect->x1 + (col % w), rect->y1 + y_off);
            } else if (style & RTGUI_TEXTSTYLE_DRAW_BACKGROUND) {
                rtgui_dc_draw_color_point(
                    dc, rect->x1 + (col % w), rect->y1 + y_off, bc);
            }
        }

        if ((w < fnt_w) && ((w - 1) == col)) {
            oft += fnt_w - w + 1;
        } else {
            oft++;
        }
    }

    return w;
}

static rt_uint8_t fnt_font_get_width(rtgui_font_t *font, const char *utf8) {
    rtgui_fnt_font_t *fnt_font;
    rt_uint8_t sz;
    rt_uint16_t code;

    fnt_font = font->data;
    sz = UTF8_SIZE(*utf8);
    code = UTF8_TO_UNICODE(utf8, sz);
    if ((code < font->start) || (code > font->end)) {
        code = font->dft;
        LOG_D("used dft code %x", code);
    }

    return fnt_font->width[code - font->start];
}

/* Public functions ----------------------------------------------------------*/
