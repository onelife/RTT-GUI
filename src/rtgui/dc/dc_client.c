/*
 * File      : dc_client.c
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
 * 2010-08-09     Bernard      rename hardware dc to client dc
 * 2010-09-13     Bernard      fix _dc_client_blit_line issue, which found
 *                             by appele
 * 2010-09-14     Bernard      fix vline and hline coordinate issue
 * 2019-06-18     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    " DC_CLT"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define display()                           (rtgui_get_gfx_device())
#define dc_set_foreground(c)                dc->gc.foreground = c
#define dc_set_background(c)                dc->gc.background = c
#define _int_swap(x, y)                     \
    do {                                    \
        x ^= y;                             \
        y ^= x;                             \
        x ^= y;                             \
    } while (0)

/* Private function prototypes -----------------------------------------------*/
static void _dc_client_draw_point(rtgui_dc_t *dc, int x, int y);
static void _dc_client_draw_color_point(rtgui_dc_t *dc, int x, int y,
    rtgui_color_t color);
static void _dc_client_draw_hline(rtgui_dc_t *dc, int x1, int x2, int y);
static void _dc_client_draw_vline(rtgui_dc_t *dc, int x, int y1, int y2);
static void _dc_client_fill_rect(rtgui_dc_t *dc, rtgui_rect_t *rect);
static void _dc_client_blit_line(rtgui_dc_t *self, int x1, int x2, int y,
    rt_uint8_t *line_data);
static void _dc_client_blit(rtgui_dc_t *dc, struct rtgui_point *dc_point,
    rtgui_dc_t *dest, rtgui_rect_t *rect);
static rt_bool_t _dc_client_uninit(rtgui_dc_t *dc);

/* Private variables ---------------------------------------------------------*/
const rtgui_dc_engine_t dc_client_engine = {
    _dc_client_draw_point,
    _dc_client_draw_color_point,
    _dc_client_draw_vline,
    _dc_client_draw_hline,
    _dc_client_fill_rect,
    _dc_client_blit_line,
    _dc_client_blit,
    _dc_client_uninit,
};

/* Private functions ---------------------------------------------------------*/
static void _dc_client_draw_point(rtgui_dc_t *self, int x, int y) {
    return _dc_client_draw_color_point(self, x, y, RT_NULL);
}

static void _dc_client_draw_color_point(rtgui_dc_t *self, int x, int y,
    rtgui_color_t color) {
    rtgui_widget_t *owner;
    rtgui_rect_t rect;

    if (!self || !rtgui_dc_get_visible(self)) return;
    owner = rt_container_of(self, rtgui_widget_t, dc_type);

    x += owner->extent.x1;
    y += owner->extent.y1;

    if (rtgui_region_contains_point(&(owner->clip), x, y, &rect)) {
        if (color)
            display()->ops->set_pixel(&color, x, y);
        else
            display()->ops->set_pixel(&(owner->gc.foreground), x, y);
    }
}

static void _dc_client_draw_vline(rtgui_dc_t *self, int x, int y1, int y2) {
    rtgui_widget_t *owner;
    rtgui_rect_t *rect;

    if (!self || !rtgui_dc_get_visible(self)) return;
    owner = rt_container_of(self, rtgui_widget_t, dc_type);

    x  += owner->extent.x1;
    y1 += owner->extent.y1;
    y2 += owner->extent.y1;
    if (y1 > y2) _int_swap(y1, y2);

    if (rtgui_region_is_flat(&(owner->clip))) {
        rect = &(owner->clip.extents);
        if (!IS_VL_INTERSECT(rect, x, y1, y2)) return;

        if (y1 < rect->y1) y1 = rect->y1;
        if (y2 > rect->y2) y2 = rect->y2;
        display()->ops->draw_vline(&(owner->gc.foreground), x, y1, y2);
    } else {
        register rt_uint32_t idx;
        register rt_base_t draw_y1, draw_y2;

        for (idx = 1; idx <= rtgui_region_num_rects(&(owner->clip)); idx++) {
            rect = ((rtgui_rect_t *)(owner->clip.data + idx));
            draw_y1 = y1;
            draw_y2 = y2;
            if (!IS_VL_INTERSECT(rect, x, y1, y2)) continue;

            if (rect->y1 > y1) draw_y1 = rect->y1;
            if (rect->y2 < y2) draw_y2 = rect->y2;
            display()->ops->draw_vline(&(owner->gc.foreground), x, draw_y1,
                draw_y2);
        }
    }
}

static void _dc_client_draw_hline(rtgui_dc_t *self, int x1, int x2, int y) {
    rtgui_widget_t *owner;
    rtgui_rect_t *rect;

    if (!self || !rtgui_dc_get_visible(self)) return;
    owner = rt_container_of(self, rtgui_widget_t, dc_type);

    x1 += owner->extent.x1;
    x2 += owner->extent.x1;
    y  += owner->extent.y1;
    if (x1 > x2) _int_swap(x1, x2);

    if (rtgui_region_is_flat(&(owner->clip))) {
        rect = &(owner->clip.extents);
        if (!IS_HL_INTERSECT(rect, x1, x2, y)) return;

        if (rect->x1 > x1) x1 = rect->x1;
        if (rect->x2 < x2) x2 = rect->x2;
        display()->ops->draw_hline(&(owner->gc.foreground), x1, x2, y);
    } else {
        register rt_uint32_t idx;
        register rt_base_t draw_x1, draw_x2;

        for (idx = 1; idx <= rtgui_region_num_rects(&(owner->clip)); idx++) {
            rect = ((rtgui_rect_t *)(owner->clip.data + idx));
            draw_x1 = x1;
            draw_x2 = x2;
            if (!IS_HL_INTERSECT(rect, x1, x2, y)) continue;

            if (rect->x1 > x1) draw_x1 = rect->x1;
            if (rect->x2 < x2) draw_x2 = rect->x2;
            display()->ops->draw_hline(&(owner->gc.foreground), draw_x1,
                draw_x2, y);
        }
    }
}

static void _dc_client_fill_rect(rtgui_dc_t *self, rtgui_rect_t *rect) {
    register rt_base_t idx;
    rtgui_widget_t *owner;
    rtgui_color_t fc;

    if (!self || !rtgui_dc_get_visible(self)) return;
    owner = rt_container_of(self, rtgui_widget_t, dc_type);

    fc = owner->gc.foreground;
    owner->gc.foreground = owner->gc.background;

    /* fill rect */
    for (idx = rect->y1; idx < rect->y2; idx++)
        _dc_client_draw_hline(self, rect->x1, rect->x2, idx);

    owner->gc.foreground = fc;
}

static void _dc_client_blit_line(rtgui_dc_t *self, int x1, int x2, int y,
    rt_uint8_t *line_data) {
    rtgui_widget_t *owner;
    rtgui_rect_t *rect;
    rt_base_t offset;

    if (!self || !rtgui_dc_get_visible(self)) return;
    owner = rt_container_of(self, rtgui_widget_t, dc_type);

    /* convert logic to device */
    x1 += owner->extent.x1;
    x2 += owner->extent.x1;
    y  += owner->extent.y1;
    if (x1 > x2) _int_swap(x1, x2);

    if (rtgui_region_is_flat(&(owner->clip))) {
        rect = &(owner->clip.extents);
        if (!IS_HL_INTERSECT(rect, x1, x2, y)) return;

        if (rect->x1 > x1) x1 = rect->x1;
        if (rect->x2 < x2) x2 = rect->x2;
        /* patch note:
         * We need to adjust the offset when update widget clip!
         * Of course at ordinary times for 0. General */
        offset = (owner->clip.extents.x1 - owner->extent.x1) * \
            _BIT2BYTE(display()->bits_per_pixel);
        /* draw hline */
        display()->ops->draw_raw_hline(line_data + offset, x1, x2, y);
    } else {
        register rt_ubase_t idx;
        register rt_base_t draw_x1, draw_x2;

        for (idx = 1; idx <= rtgui_region_num_rects(&(owner->clip)); idx++) {
            rect = ((rtgui_rect_t *)(owner->clip.data + idx));
            draw_x1 = x1;
            draw_x2 = x2;
            if (!IS_HL_INTERSECT(rect, x1, x2, y)) continue;

            if (rect->x1 > x1) draw_x1 = rect->x1;
            if (rect->x2 < x2) draw_x2 = rect->x2;
            offset = (draw_x1 - x1) * _BIT2BYTE(display()->bits_per_pixel);
            display()->ops->draw_raw_hline(line_data + offset, draw_x1,
                draw_x2, y);
        }
    }
}

static void _dc_client_blit(rtgui_dc_t *dc, struct rtgui_point *dc_point,
    rtgui_dc_t *dest, rtgui_rect_t *rect) {
    (void)dc;
    (void)dc_point;
    (void)dest;
    (void)rect;
    /* no blit in hardware dc? */
    LOG_E("no client dc blit");
}

static rt_bool_t _dc_client_uninit(rtgui_dc_t *dc) {
    if (!dc || (dc->type != RTGUI_DC_CLIENT)) return RT_FALSE;
    return RT_TRUE;
}

/* Public functions ----------------------------------------------------------*/
void rtgui_dc_client_init(rtgui_widget_t *owner) {
    rtgui_dc_t *dc;

    dc = WIDGET_DC(owner);
    dc->type = RTGUI_DC_CLIENT;
    dc->engine = &dc_client_engine;
}

rtgui_dc_t *rtgui_dc_client_create(rtgui_widget_t *owner) {
    /* adjudge owner */
    if (!owner || !owner->toplevel) return RT_NULL;

    return WIDGET_DC(owner);
}
