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
 * 2019-06-03     onelife      refactor
 */
#ifndef __FONT_H__
#define __FONT_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported defines ----------------------------------------------------------*/
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
#define IS_ASCII(utf, sz)           \
    ((sz > 2)  ? 0 : (              \
     (sz == 1) ? 1 : (              \
     (utf[0] <= 0xC3) ? 1 : 0)))

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
extern const rtgui_font_engine_t fnt_font_engine;
extern const rtgui_font_engine_t bmp_font_engine;

/* Exported variables --------------------------------------------------------*/
#ifdef GUIENGINE_USING_FONT12
extern rtgui_font_t rtgui_font_asc12;
# ifdef GUIENGINE_USING_FONTHZ
extern rtgui_font_t rtgui_font_hz12;
# endif
#endif
#ifdef GUIENGINE_USING_FONT16
extern rtgui_font_t rtgui_font_asc16;
# ifdef GUIENGINE_USING_FONTHZ
extern rtgui_font_t rtgui_font_hz16;
# endif
#endif

/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_font_system_init(void);
void rtgui_font_close_all(void);
rt_err_t rtgui_font_system_add_font(rtgui_font_t *font);
void rtgui_font_system_remove_font(rtgui_font_t *font);
rtgui_font_t *rtgui_font_default(void);
void rtgui_font_set_defaut(rtgui_font_t *font);
rtgui_font_t *rtgui_font_refer(const char *family, rt_uint16_t height);
void rtgui_font_derefer(rtgui_font_t *font);

void rtgui_font_draw(rtgui_font_t *font, rtgui_dc_t *dc, const char *text,
    rt_ubase_t len, rtgui_rect_t *rect);
rt_uint8_t rtgui_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect);
rt_uint8_t rtgui_font_get_width(rtgui_font_t *font, const char *text);
rt_uint32_t rtgui_font_get_string_width(rtgui_font_t *font, const char *text);
void rtgui_font_get_metrics(rtgui_font_t *font, const char *text,
    rtgui_rect_t *rect);

#ifdef GUIENGINE_USING_FONTHZ
rt_uint16_t UnicodeToGB2312(rt_uint16_t unicode);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __FONT_H__ */
