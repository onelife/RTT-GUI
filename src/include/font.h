/*
 * File      : font.h
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
#ifndef __RTGUI_FONT_H__
#define __RTGUI_FONT_H__

/* Includes ------------------------------------------------------------------*/
#include "./rtgui.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/tree.h"

/* Exported defines ----------------------------------------------------------*/
/* used by stract font */
#define FONT_BMP_DATA_BEGIN
#define FONT_BMP_DATA_END

/* Exported types ------------------------------------------------------------*/
struct rtgui_font_engine {
    /* font engine function */
    void (*font_init)(rtgui_font_t *font);
    void (*font_load)(rtgui_font_t *font);
    void (*font_draw_text)(rtgui_font_t *font, rtgui_dc_t *dc, const char *text,
        rt_ubase_t len, rtgui_rect_t *rect);
    void (*font_get_metrics)(rtgui_font_t *font, const char *text,
        rtgui_rect_t *rect);
};

/* bitmap font engine */
struct rtgui_font_bitmap {
    const rt_uint8_t  *bmp;         /* bitmap font data */
    rt_uint8_t  _len;         /* each character width, NULL for fixed font */
    const rt_uint32_t *offset;      /* offset for each character */

    rt_uint16_t width;              /* font width  */
    rt_uint16_t height;             /* font height */

    rt_uint16_t _start;
    rt_uint16_t _end;   //TODO: const
    const rt_uint16_t _dft;
    const rt_uint16_t size;
};


SPLAY_HEAD(cache_tree, hz_cache);

struct hz_cache {
    SPLAY_ENTRY(hz_cache) hz_node;
    rt_uint16_t hz_id;
};

struct rtgui_hz_file_font {
    struct cache_tree cache_root;
    rt_uint16_t cache_size;
    /* font size */
    rt_uint16_t font_size;
    rt_uint16_t font_data_size;
    /* file descriptor */
    int fd;
    /* font file name */
    const char *font_fn;
};

/* Exported constants --------------------------------------------------------*/
extern const rtgui_font_engine_t bmp_font_engine;
extern const rtgui_font_engine_t rtgui_hz_file_font_engine;
extern const rtgui_font_engine_t hz_bmp_font_engine;

#ifdef GUIENGINE_USING_FONT12
extern const rtgui_font_t rtgui_font_asc12;
# ifdef GUIENGINE_USING_FONTHZ
extern const rtgui_font_t rtgui_font_hz12;
# endif
#endif
#ifdef GUIENGINE_USING_FONT16
extern const rtgui_font_t rtgui_font_asc16;
# ifdef GUIENGINE_USING_FONTHZ
extern const rtgui_font_t rtgui_font_hz16;
# endif
#endif

/* Exported functions ------------------------------------------------------- */
void rtgui_font_system_init(void);
void rtgui_font_fd_uninstall(void);
void rtgui_font_system_add_font(rtgui_font_t *font);
void rtgui_font_system_remove_font(rtgui_font_t *font);
rtgui_font_t *rtgui_font_default(void);
void rtgui_font_set_defaut(rtgui_font_t *font);

rtgui_font_t *rtgui_font_refer(const char *family, rt_uint16_t height);
void rtgui_font_derefer(rtgui_font_t *font);

/* draw a text */
void rtgui_font_draw(rtgui_font_t *font, rtgui_dc_t *dc, const char *text, rt_ubase_t len, rtgui_rect_t *rect);
int  rtgui_font_get_string_width(rtgui_font_t *font, const char *text);
void rtgui_font_get_metrics(rtgui_font_t *font, const char *text, rtgui_rect_t *rect);

#ifdef GUIENGINE_USING_FONTHZ
rt_uint16_t UnicodeToGB2312(rt_uint16_t unicode);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_FONT_H__ */
