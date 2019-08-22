/*
 * File      : region.h
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
#ifndef __RTGUI_REGION_H__
#define __RTGUI_REGION_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
/*  true if rect r1 and r2 are overlap */
#define IS_R_INTERSECT(r1, r2)      \
    (  !((r1)->x2 <= (r2)->x1)  ||  \
        ((r1)->x1 >= (r2)->x2)  ||  \
        ((r1)->y2 <= (r2)->y1)  ||  \
        ((r1)->y1 >= (r2)->y2)  )

/* true if rect contains point (x, y) */
#define IS_P_INSIDE(r, x, y)    \
    (   ((r)->x2 >  (x))    &&  \
        ((r)->x1 <= (x))    &&  \
        ((r)->y2 >  (y))    &&  \
        ((r)->y1 <= (y))    )

/* true if vline intersect rect */
#define IS_VL_INTERSECT(r, x, y1, y2) \
    (  !((r)->x1 >  ( x))   ||  \
        ((r)->x2 <= ( x))   ||  \
        ((r)->y2 <= (y1))   ||  \
        ((r)->y1 >  (y2))   )

/* true if hline intersect rect */
#define IS_HL_INTERSECT(r, x1, x2, y) \
    (  !((r)->y1 >  ( y))   ||  \
        ((r)->y2 <= ( y))   ||  \
        ((r)->x2 <= (x1))   ||  \
        ((r)->x1 >  (x2))   )

/* true if rect r1 contains rect r2 */
#define IS_R_INSIDE(r1, r2)         \
    (   ((r1)->x1 <= (r2)->x1)  &&  \
        ((r1)->x2 >= (r2)->x2)  &&  \
        ((r1)->y1 <= (r2)->y1)  &&  \
        ((r1)->y2 >= (r2)->y2)  )

/* true if rect r1 and rect r2 make cross */
#define IS_R_CROSS(r1, r2)          \
    (   ((r1)->x1 <= (r2)->x1)  &&  \
        ((r1)->x2 >= (r2)->x2)  &&  \
        ((r1)->y1 >= (r2)->y1)  &&  \
        ((r1)->y2 <= (r2)->y2)  )

/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_rect rtgui_rect_t;
typedef struct rtgui_region_data rtgui_region_data_t;
typedef struct rtgui_region rtgui_region_t;

typedef enum {
    FAILURE,
    SUCCESS
} r_op_status_t;

typedef enum {
    RTGUI_REGION_OUT,
    RTGUI_REGION_IN,
    RTGUI_REGION_PART,
} r_contain_status_t;

struct rtgui_rect {
    rt_int16_t x1, y1, x2, y2;
};

struct rtgui_region_data {
    rt_uint32_t size;                       /* total */
    rt_uint32_t numRects;                   /* used */
    // rtgui_rect_t rects[size];
};

struct rtgui_region {
    rtgui_rect_t extents;
    rtgui_region_data_t *data;
};

/* Exported constants --------------------------------------------------------*/

#undef __RTGUI_REGION_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
void rtgui_region_init_empty(rtgui_region_t *region);
void rtgui_region_init(rtgui_region_t *region,
    rt_uint16_t x, rt_uint16_t y, rt_uint16_t width, rt_uint16_t height);
void rtgui_region_init_with_extent(rtgui_region_t *region,
    const rtgui_rect_t *extent);
void rtgui_region_uninit(rtgui_region_t *region);
rt_uint32_t rtgui_region_num_rects(rtgui_region_t *region);
rtgui_rect_t *rtgui_region_rects(rtgui_region_t *region);
r_op_status_t rtgui_region_copy(rtgui_region_t *dst, rtgui_region_t *src);

void rtgui_region_translate(rtgui_region_t *region, int x, int y);

r_op_status_t rtgui_region_intersect(rtgui_region_t *dstRgn,
    rtgui_region_t *rgn1, rtgui_region_t *rgn2);
r_op_status_t rtgui_region_intersect_rect(rtgui_region_t *dstRgn,
    rtgui_region_t *reg1, rtgui_rect_t *rect);
r_op_status_t rtgui_region_union(rtgui_region_t *dstRgn, rtgui_region_t *rgn1,
    rtgui_region_t *rgn2);
r_op_status_t rtgui_region_union_rect(rtgui_region_t *dst, rtgui_region_t *src,
    rtgui_rect_t *rect);
r_op_status_t rtgui_region_subtract(rtgui_region_t *regD, rtgui_region_t *regM,
    rtgui_region_t *regS);
r_op_status_t rtgui_region_subtract_rect(rtgui_region_t *regD,
    rtgui_region_t *regM, rtgui_rect_t *rect);

rt_bool_t rtgui_region_contains_point(rtgui_region_t *region, int x, int y, rtgui_rect_t *box);
rt_bool_t rtgui_region_contains_rect(rtgui_region_t *rgn, rtgui_rect_t *rect);

rt_bool_t rtgui_region_not_empty(rtgui_region_t *region);
rtgui_rect_t *rtgui_region_extents(rtgui_region_t *region);

r_op_status_t rtgui_region_append(rtgui_region_t *dest, rtgui_region_t *region);
r_op_status_t rtgui_region_validate(rtgui_region_t *badreg, int *pOverlap);

void rtgui_region_reset(rtgui_region_t *region, rtgui_rect_t *rect);
void rtgui_region_empty(rtgui_region_t *region);
void rtgui_region_dump(rtgui_region_t *region);
void rtgui_region_draw_clip(rtgui_region_t *region, rtgui_dc_t *dc);
rt_bool_t rtgui_region_is_flat(rtgui_region_t *region);

/* rect functions */
void rtgui_rect_move(rtgui_rect_t *rect, int x, int y);
void rtgui_rect_move_to_point(rtgui_rect_t *rect, int x, int y);
void rtgui_rect_move_align(const rtgui_rect_t *rect, rtgui_rect_t *to, int align);
void rtgui_rect_inflate(rtgui_rect_t *rect, int d);
void rtgui_rect_intersect(rtgui_rect_t *src, rtgui_rect_t *dest);
rt_bool_t rtgui_rect_contains_point(const rtgui_rect_t *rect, int x, int y);
rt_bool_t rtgui_rect_contains_rect(const rtgui_rect_t *rect1,
    const rtgui_rect_t *rect2);
rt_bool_t rtgui_rect_is_intersect(const rtgui_rect_t *rect1, const rtgui_rect_t *rect2);
rt_bool_t rtgui_rect_is_equal(const rtgui_rect_t *rect1, const rtgui_rect_t *rect2);
rtgui_rect_t *rtgui_rect_set(rtgui_rect_t *rect, int x, int y, int w, int h);
rt_bool_t rtgui_rect_is_empty(const rtgui_rect_t *rect);
void rtgui_rect_union(rtgui_rect_t *src, rtgui_rect_t *dest);

rt_inline void rtgui_rect_init(rtgui_rect_t* rect, int x, int y, int width,
    int height) {
    rect->x1 = x;
    rect->y1 = y;
    rect->x2 = x + width;
    rect->y2 = y + height;
}

#if 0
#define RTGUI_RECT(rect, x, y, w, h)	\
	do { \
	rect.x1 = x; rect.y1 = y; \
	rect.x2 = (x) + (w); rect.y2 = (y) + (h); \
	} while (0)
#endif

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_REGION_H__ */
