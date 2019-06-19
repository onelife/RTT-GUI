/*
 * File      : font.c
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
 * 2013-08-31     Bernard      remove the default font setting.
 *                             (which set by theme)
 * 2019-06-03     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/font/font.h"
#include "include/dc.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_FNT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static rt_slist_t _font_list;
static rtgui_font_t *_default_font;

/* Imported variables --------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_font_system_init(void) {
    rt_err_t ret;

    rt_slist_init(&(_font_list));
    _default_font = RT_NULL;
    ret = RT_EOK;

    do {
        #ifdef GUIENGINE_USING_FONT12
            ret = rtgui_font_system_add_font(&rtgui_font_asc12);
            if (RT_EOK != ret) break;
            _default_font = &rtgui_font_asc12;
            LOG_D("add asc12");

        # ifdef GUIENGINE_USING_FONTHZ
            ret = rtgui_font_system_add_font(&rtgui_font_hz12);
            if (RT_EOK != ret) break;
            _default_font = &rtgui_font_hz12;
            LOG_D("add hz12");
        # endif
        #endif

        #ifdef GUIENGINE_USING_FONT16
            ret = rtgui_font_system_add_font(&rtgui_font_asc16);
            if (RT_EOK != ret) break;
            _default_font = &rtgui_font_asc16;
            LOG_D("add asc16");

        # ifdef GUIENGINE_USING_FONTHZ
            ret = rtgui_font_system_add_font(&rtgui_font_hz16);
            if (RT_EOK != ret) break;
            _default_font = &rtgui_font_hz16;
            LOG_D("add hz16");
        # endif
        #endif
    } while (0);

    if (_default_font) {
        LOG_D("_default_font %s%d", _default_font->family,
            _default_font->height);
    } else {
        ret = -RT_ERROR;
        LOG_E("no font!");
    }

    return ret;
}

void rtgui_font_close_all(void) {
    rt_slist_t *node;
    rtgui_font_t *font;

    rt_slist_for_each(node, &_font_list) {
        font = rt_slist_entry(node, rtgui_font_t, list);
        if (font->engine->font_close) {
            font->engine->font_close(font);
        }
    }
}

rt_err_t rtgui_font_system_add_font(rtgui_font_t *font) {
    rt_err_t ret = RT_EOK;

    rt_slist_init(&(font->list));
    rt_slist_append(&_font_list, &(font->list));
    /* init font */
    if (font->engine->font_init) {
        ret = font->engine->font_init(font);
    }
    return ret;
}
RTM_EXPORT(rtgui_font_system_add_font);

void rtgui_font_system_remove_font(rtgui_font_t *font) {
    if (font->engine->font_close) {
        font->engine->font_close(font);
    }
    rt_slist_remove(&_font_list, &(font->list));
}
RTM_EXPORT(rtgui_font_system_remove_font);

rtgui_font_t *rtgui_font_default(void) {
    return _default_font;
}

void rtgui_font_set_defaut(rtgui_font_t *font) {
    _default_font = font;
    LOG_D("set font %d", font->height);
}

rtgui_font_t *rtgui_font_refer(const char *family, rt_uint16_t height) {
    rt_slist_t *node;
    rtgui_font_t *font;

    /* search font */
    rt_slist_for_each(node, &_font_list)     {
        font = rt_slist_entry(node, rtgui_font_t, list);
        if (!rt_strcasecmp(font->family, family) && (font->height == height)) {
            font->refer_count++;
            return font;
        }
    }
    return RT_NULL;
}
RTM_EXPORT(rtgui_font_refer);

void rtgui_font_derefer(rtgui_font_t *font) {
    RT_ASSERT(font != RT_NULL);
    font->refer_count--;
    /* no refer, remove font */
    if (!font->refer_count) rtgui_font_system_remove_font(font);
}
RTM_EXPORT(rtgui_font_derefer);

/* draw a text */
void rtgui_font_draw(rtgui_font_t *font, rtgui_dc_t *dc, const char *text,
    rt_ubase_t len, rtgui_rect_t *rect) {
    rtgui_font_t *ascii, *non_ascii;
    rtgui_rect_t text_rect;

    RT_ASSERT(font != RT_NULL);

    rtgui_font_get_metrics(font, text, &text_rect);
    rtgui_rect_move_align(rect, &text_rect, RTGUI_DC_TEXTALIGN(dc));

    if (!rt_strcasecmp(font->family, "asc")) {
        ascii = font;
        #ifdef GUIENGINE_USING_FONTHZ
            non_ascii = rtgui_font_refer("hz", font->height);
        #else
            non_ascii = RT_NULL;
        #endif
    } else {
        ascii = rtgui_font_refer("asc", font->height);
        non_ascii = font;
    }

    do {
        rt_uint8_t idx = 0;

        if (ascii && ascii->engine->font_open) {
            if (RT_EOK != ascii->engine->font_open(ascii)) break;
        }
        if (non_ascii && non_ascii->engine->font_open) {
            if (RT_EOK != non_ascii->engine->font_open(non_ascii)) break;
        }

        while ((text_rect.x1 < text_rect.x2) && (idx < len)) {
            /* get font data */
            rt_uint8_t *utf8 = (rt_uint8_t *)text + idx;
            rt_uint8_t size = UTF8_SIZE(*utf8);
            rt_uint16_t code;
            rtgui_font_t *_font;

            if (IS_ASCII(utf8, size) && ascii) {
                code = UTF8_TO_UNICODE(utf8, size);
                _font = ascii;
            } else {
                code = UnicodeToGB2312(UTF8_TO_UNICODE(utf8, size));
                 _font = non_ascii;
            }

            /* draw a char */
            text_rect.x1 += rtgui_font_draw_char(_font, dc, code, &text_rect);
            idx += size;
        }

        if (ascii && ascii->engine->font_close) {
            ascii->engine->font_close(ascii);
        }
        if (non_ascii && non_ascii->engine->font_close) {
            non_ascii->engine->font_close(non_ascii);
        }
    } while (0);

    if (!rt_strcasecmp(font->family, "asc")) {
        if (non_ascii) rtgui_font_derefer(non_ascii);
    } else {
        if (ascii) rtgui_font_derefer(ascii);
    }
}

rt_uint8_t rtgui_font_draw_char(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect) {
    RT_ASSERT(font != RT_NULL);

    if (font->engine && font->engine->font_draw_char)
        return font->engine->font_draw_char(font, dc, code, rect);
    return 0;
}
RTM_EXPORT(rtgui_font_draw_char);

rt_uint8_t rtgui_font_get_width(rtgui_font_t *font, const char *text) {
    RT_ASSERT(font != RT_NULL);

    if (font->engine && font->engine->font_get_width)
        return font->engine->font_get_width(font, text);
    else
        return 0;
}
RTM_EXPORT(rtgui_font_get_width);

rt_uint32_t rtgui_font_get_string_width(rtgui_font_t *font, const char *text) {
    rt_uint8_t *utf8 = (rt_uint8_t *)text;
    rtgui_font_t *ascii, *non_ascii;
    rt_uint8_t sz;
    int w;

    RT_ASSERT(font != RT_NULL);

    if (!rt_strcasecmp(font->family, "asc")) {
        ascii = font;
        #ifdef GUIENGINE_USING_FONTHZ
            non_ascii = rtgui_font_refer("hz", font->height);
        #else
            non_ascii = RT_NULL;
        #endif
    } else {
        ascii = rtgui_font_refer("asc", font->height);
        non_ascii = font;
    }

    w = 0;
    while (*utf8) {
        sz = UTF8_SIZE(*utf8);
        if (IS_ASCII(utf8, sz) && ascii) {
            w += rtgui_font_get_width(ascii, (const char *)utf8);
        } else {
            w += rtgui_font_get_width(non_ascii, (const char *)utf8);
        }
        utf8 += sz;
    }

    if (!rt_strcasecmp(font->family, "asc")) {
        if (non_ascii) rtgui_font_derefer(non_ascii);
    } else {
        if (ascii) rtgui_font_derefer(ascii);
    }

    return w;
}
RTM_EXPORT(rtgui_font_get_string_width);

void rtgui_font_get_metrics(rtgui_font_t *font, const char *text,
    rtgui_rect_t *rect) {
    rect->x1 = 0;
    rect->x2 = rtgui_font_get_string_width(font, text);
    rect->y1 = 0;
    rect->y2 = font->height;
}
RTM_EXPORT(rtgui_font_get_metrics);
