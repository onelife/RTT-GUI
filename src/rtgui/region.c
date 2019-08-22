/*
 * File      : region.c
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
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/region.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_RGN"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
// #define GOOD(rgn)                           RT_ASSERT(rtgui_region16_valid(rgn))
#define GOOD(rgn)

/* not a region */
#define IS_REGION_INVALID(rgn)              ((rgn)->data == &_bad_region_data)
#define REGION_NO_RECT(rgn)                 \
    ((rgn)->data && !(rgn)->data->numRects)
#define REGION_DATA_NUM_RECTS(rgn)          \
    ((rgn)->data ? (rgn)->data->numRects : 1)
#define REGION_DATA_SIZE(rgn)               \
    ((rgn)->data ? (rgn)->data->size : 0)
#define REGION_RECTS_PTR(rgn)               ((rtgui_rect_t *)((rgn)->data + 1))
#define REGION_GET_RECTS(rgn)               \
    ((rgn)->data ? REGION_RECTS_PTR(rgn) : &(rgn)->extents)
#define REGION_RECT_AT(rgn, i)              (&REGION_RECTS_PTR(rgn)[i])
#define REGION_END_RECT(rgn)                \
    (REGION_GET_RECTS(rgn) + REGION_DATA_NUM_RECTS(rgn))
#define REGION_LAST_RECT(rgn)               (REGION_END_RECT(rgn) - 1)
#define REGION_SIZE_OF_N_RECTS(n)           \
    (sizeof(rtgui_region_data_t) + ((n) * sizeof(rtgui_rect_t)))

/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static rtgui_rect_t _null_rect = {
    0, 0, 0, 0
};
static rtgui_region_data_t _null_region_data = {
    0, 0
};
static rtgui_region_data_t  _bad_region_data = {
    0, 0
};

/*
 * The functions in this file implement the Region abstraction used extensively
 * throughout the X11 sample server. A Region is simply a set of disjoint
 * (non-overlapping) rectangles, plus an "extent" rectangle which is the
 * smallest single rectangle that contains all the non-overlapping rectangles.
 *
 * A Region is implemented as a "y-x-banded" array of rectangles.  This array
 * imposes two degrees of order.  First, all rectangles are sorted by top side
 * y coordinate first (y1), and then by left side x coordinate (x1).
 *
 * Furthermore, the rectangles are grouped into "bands".  Each rectangle in a
 * band has the same top y coordinate (y1), and each has the same bottom y
 * coordinate (y2).  Thus all rectangles in a band differ only in their left
 * and right side (x1 and x2).  Bands are implicit in the array of rectangles:
 * there is no separate list of band start pointers.
 *
 * The y-x band representation does not RTGUI_MINimize rectangles.  In particular,
 * if a rectangle vertically crosses a band (the rectangle has scanlines in
 * the y1 to y2 area spanned by the band), then the rectangle may be broken
 * down into two or more smaller rectangles stacked one atop the other.
 *
 *  -----------                              -----------
 *  |         |                              |         |             band 0
 *  |         |  --------                    -----------  --------
 *  |         |  |      |  in y-x banded     |         |  |      |   band 1
 *  |         |  |      |  form is           |         |  |      |
 *  -----------  |      |                    -----------  --------
 *               |      |                                 |      |   band 2
 *               --------                                 --------
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible: no two rectangles within a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course).
 *
 * Adam de Boor wrote most of the original region code.  Joel McCormack
 * substantially modified or rewrote most of the core arithmetic routines, and
 * added rtgui_region_validate in order to support several speed improvements to
 * rtgui_region_validateTree.  Bob Scheifler changed the representation to be more
 * compact when empty or a single rectangle, and did a bunch of gratuitous
 * reformatting. Carl Worth did further gratuitous reformatting while re-merging
 * the server and client region code into libpixregion.
 */

/* Private functions ---------------------------------------------------------*/
#define allocData(n)                        \
    rtgui_malloc(REGION_SIZE_OF_N_RECTS(n))

#define freeData(rgn)                       \
    if ((rgn)->data && (rgn)->data->size) { \
        rtgui_free((rgn)->data);            \
    }

#define RECTALLOC_BAIL(rgn, n, bail) \
if (!(rgn)->data || (((rgn)->data->numRects + (n)) > (rgn)->data->size)) \
    if (!_alloc_rects(rgn, n)) { goto bail; }

#define _realloc_rects(rgn, n)              \
    if (!(rgn)->data || (((rgn)->data->numRects + (n)) > (rgn)->data->size)) { \
        if (!_alloc_rects(rgn, n)) {         \
            return FAILURE;                 \
        }                                   \
    }

#define _append_rect(end, _x1, _y1, _x2, _y2) { \
    end->x1 = _x1;                          \
    end->y1 = _y1;                          \
    end->x2 = _x2;                          \
    end->y2 = _y2;                          \
    end++;                                  \
}

#define _append_new_rect(rgn, end, _x1, _y1, _x2, _y2) { \
    if (!(rgn)->data || ((rgn)->data->numRects == (rgn)->data->size)) { \
        if (SUCCESS != _alloc_rects(rgn, 1)) return FAILURE; \
        end = REGION_END_RECT(rgn);         \
    }                                       \
    _append_rect(end, _x1, _y1, _x2, _y2);  \
    rgn->data->numRects++;                  \
    RT_ASSERT(rgn->data->numRects <= rgn->data->size); \
}

#define _resize_data(rgn, num)              \
    if (((num) < ((rgn)->data->size >> 1)) && ((rgn)->data->size > 50)) { \
        rtgui_region_data_t *newData = (rtgui_region_data_t *)\
            rtgui_realloc((rgn)->data, REGION_SIZE_OF_N_RECTS(num)); \
        if (newData) {                      \
            newData->size = (num);          \
            (rgn)->data = newData;          \
        }                                   \
    }

static r_op_status_t _invalid(rtgui_region_t *region) {
    freeData(region);
    region->extents = _null_rect;
    region->data = &_bad_region_data;
    return FAILURE;
}

static r_op_status_t _alloc_rects(rtgui_region_t *region,
    rt_uint32_t n) {
    r_op_status_t ret;

    ret = FAILURE;

    do {
        rtgui_region_data_t *data;

        if (!region->data) {
            n++;
            region->data = allocData(n);
            if (!region->data) break;
            region->data->numRects = 1;
            *REGION_RECTS_PTR(region) = region->extents;

        } else if (!region->data->size) {
            region->data = allocData(n);
            if (!region->data) break;
            region->data->numRects = 0;

        } else {
            if (n == 1) {
                n = region->data->numRects;
                if (n > 500) /* XXX pick numbers out of a hat */
                    n = 250;
            }
            n += region->data->numRects;
            data = (rtgui_region_data_t *)rtgui_realloc(
                region->data, REGION_SIZE_OF_N_RECTS(n));
            if (!data) break;
            region->data = data;
        }

        region->data->size = n;
        ret = SUCCESS;
    } while (0);

    if (SUCCESS != ret)
        return _invalid(region);
    return ret;
}

/* Public functions ----------------------------------------------------------*/
void rtgui_region_init_empty(rtgui_region_t *region) {
    region->extents = _null_rect;
    region->data = &_null_region_data;
}
RTM_EXPORT(rtgui_region_init_empty);

void rtgui_region_init(rtgui_region_t *region,
    rt_uint16_t x, rt_uint16_t y, rt_uint16_t width, rt_uint16_t height) {
    region->extents.x1 = x;
    region->extents.y1 = y;
    region->extents.x2 = x + width;
    region->extents.y2 = y + height;
    region->data = RT_NULL;
}

void rtgui_region_init_with_extent(rtgui_region_t *region,
    const rtgui_rect_t *extent) {
    region->extents = *extent;
    region->data = RT_NULL;
}

void rtgui_region_uninit(rtgui_region_t *region) {
    GOOD(region);
    freeData(region);
}
RTM_EXPORT(rtgui_region_uninit);

rt_uint32_t rtgui_region_num_rects(rtgui_region_t *region) {
    return REGION_DATA_NUM_RECTS(region);
}

rtgui_rect_t *rtgui_region_rects(rtgui_region_t *region) {
    return REGION_GET_RECTS(region);
}

r_op_status_t rtgui_region_copy(rtgui_region_t *dst, rtgui_region_t *src) {
    r_op_status_t ret;

    GOOD(dst); GOOD(src);
    ret = SUCCESS;

    do {
        if (dst == src) break;

        dst->extents = src->extents;
        if (!src->data || !src->data->size) {
            freeData(dst);
            dst->data = src->data;
            break;
        }

        if (!dst->data || (dst->data->size < src->data->numRects)) {
            freeData(dst);
            dst->data = allocData(src->data->numRects);
            if (!dst->data) {
                ret = FAILURE;
                break;
            }
            dst->data->size = src->data->numRects;
        }
        dst->data->numRects = src->data->numRects;
        rt_memmove(REGION_RECTS_PTR(dst), REGION_RECTS_PTR(src),
            dst->data->numRects * sizeof(rtgui_rect_t));
    } while (0);

    if (SUCCESS != ret)
        return _invalid(dst);
    return ret;
}
RTM_EXPORT(rtgui_region_copy);

/*======================================================================
 *      Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * _coalesce --
 *  Attempt to merge the boxes in the current band with those in the
 *  previous one.  We are guaranteed that the current band extends to
 *  the end of the rects array.  Used only by region_op.
 *
 * Results:
 *  The new index for the previous band.
 *
 * Side Effects:
 *  If coalescing takes place:
 *      - rectangles in the previous band will have their y2 fields
 *        altered.
 *      - rgn->data->numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
rt_inline rt_uint32_t _coalesce(
    rtgui_region_t *rgn,            /* Region to coalesce */
    rt_uint32_t prvStart,           /* Index of start of previous band */
    rt_uint32_t curStart) {         /* Index of start of current band */
    rtgui_rect_t *pPrvBox;          /* Current box in previous band */
    rtgui_rect_t *pCurBox;          /* Current box in current band */
    rt_uint32_t numRects;           /* Number rectangles in both bands */
    rt_int16_t y2;                  /* Bottom of current band */

    /*
     * Figure out how many rectangles are in the band.
     */
    numRects = curStart - prvStart;
    RT_ASSERT(numRects == (rgn->data->numRects - curStart));

    if (!numRects) return curStart;

    /*
     * The bands may only be coalesced if the bottom of the previous
     * matches the top scanline of the current.
     */
    pPrvBox = REGION_RECT_AT(rgn, prvStart);
    pCurBox = REGION_RECT_AT(rgn, curStart);
    if (pPrvBox->y2 != pCurBox->y1) return curStart;

    /*
     * Make sure the bands have boxes in the same places. This
     * assumes that boxes have been added in such a way that they
     * cover the most area possible. I.e. two boxes in a band must
     * have some horizontal space between them.
     */
    y2 = pCurBox->y2;

    do {
        if ((pPrvBox->x1 != pCurBox->x1) || (pPrvBox->x2 != pCurBox->x2))
            return curStart;
        pPrvBox++;
        pCurBox++;
        numRects--;
    } while (numRects);

    /*
     * The bands may be merged, so set the bottom y of each box
     * in the previous band to the bottom y of the current band.
     */
    numRects = curStart - prvStart;
    rgn->data->numRects -= numRects;
    do {
        pPrvBox--;
        pPrvBox->y2 = y2;
        numRects--;
    } while (numRects);
    return prvStart;
}

/* Quicky macro to avoid trivial reject procedure calls to _coalesce */
#define coalesce(dstRgn, prvStart, curStart) \
    if ((curStart - prvStart) != (dstRgn->data->numRects - curStart)) { \
        if ((0 != prvStart) || (0 != curStart)) { \
            LOG_W("coalesce mismatch");     \
            prvStart = curStart;            \
        }                                   \
    } else {                                \
        prvStart = _coalesce(dstRgn, prvStart, curStart); \
    }

/*-
 *-----------------------------------------------------------------------
 * _expend_verticaly
 *  Handle a non-overlapping band for the union and subtract operations.
 *      Just adds the (top/bottom-clipped) rectangles into the region.
 *      Doesn't have to check for subsumption or anything.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  rgn->data->numRects is incremented and the rectangles overwritten
 *  with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */
rt_inline r_op_status_t _expend_verticaly(rtgui_region_t *rgn, rtgui_rect_t *r,
    rtgui_rect_t *last_r, rt_int16_t y1, rt_int16_t y2) {
    rt_uint32_t num;
    rtgui_rect_t *end;

    RT_ASSERT(y1 < y2);
    RT_ASSERT(last_r != r);

    num = last_r - r;
    _realloc_rects(rgn, num);
    end = REGION_END_RECT(rgn);
    rgn->data->numRects += num;

    do {
        RT_ASSERT(r->x1 < r->x2);
        _append_rect(end, r->x1, y1, r->x2, y2);
        r++;
    } while (r != last_r);

    return SUCCESS;
}


/*  input:  r, last_r
    output: r_stop, r0_y1 */
#define _find_next(r, r_stop, last_r, r0_y1) { \
    r0_y1 = r->y1;                          \
    r_stop = r + 1;                         \
    while ((r_stop != last_r) && (r_stop->y1 == r0_y1)) { \
        r_stop++;                           \
    }                                       \
}

#define _append_rects(dstRgn, r, last_r) {  \
    rt_uint32_t num = last_r - r;           \
    if (num) {                              \
        _realloc_rects(dstRgn, num);        \
        rt_memmove(REGION_END_RECT(dstRgn), r, num * sizeof(rtgui_rect_t)); \
        dstRgn->data->numRects += num;      \
    }                                       \
}

/*-
 *-----------------------------------------------------------------------
 * region_op --
 *  Apply an operation to two regions. Called by rtgui_region_union, rtgui_region_inverse,
 *  rtgui_region_subtract, rtgui_region_intersect....  Both regions MUST have at least one
 *      rectangle, and cannot be the same object.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *  The new region is overwritten.
 *  pOverlap set to SUCCESS if func ever returns SUCCESS.
 *
 * Notes:
 *  The idea behind this function is to view the two regions as sets.
 *  Together they cover a rectangle of area that this function divides
 *  into horizontal bands where points are covered only by one region
 *  or by both. For the first case, the nonOverlapFunc is called with
 *  each the band and the band's upper and lower extents. For the
 *  second, the func is called to process the entire band. It
 *  is responsible for clipping the rectangles in the band, though
 *  this function provides the boundaries.
 *  At the end of each band, the new region is coalesced, if possible,
 *  to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */

typedef r_op_status_t (*overlapFunc)(
    rtgui_region_t *region,
    rtgui_rect_t *r1,
    rtgui_rect_t *end1,
    rtgui_rect_t *r2,
    rtgui_rect_t *end2,
    rt_int16_t y1,
    rt_int16_t y2,
    int *pOverlap);

static r_op_status_t region_op(
    rtgui_region_t *dstRgn,         /* destination rgn */
    rtgui_region_t *rgn1,           /* source rgn1 */
    rtgui_region_t *rgn2,           /* source rgn2 */
    overlapFunc func,               /* function to processing overlap */
    rt_bool_t merge_b1,             /* if merge free space to band1 */
    rt_bool_t merge_b2,             /* if merge free space to band2 */
    int *pOverlap) {                /* param of func */
    rtgui_region_data_t *backup;    /* backup of previous data of dstRgn */
    rtgui_rect_t *r1;
    rtgui_rect_t *r2;
    rt_uint32_t num1;
    rt_uint32_t num2;
    rtgui_rect_t *end1;             /* the end of rect in rgn1 */
    rtgui_rect_t *end2;             /* the end of rect in rgn2 */
    rt_uint32_t prvStart;          /* previous coalesce starting index */
    rt_uint32_t curStart;           /* current coalesce starting index */

    rt_int16_t top_y;
    rt_int16_t lower_y1;
    rt_int16_t r1_start_y1;         /* rgn1 search start y1 */
    rt_int16_t r2_start_y1;         /* rgn2 search start y1 */
    rtgui_rect_t *r1_stop;          /* rgn1 search end r */
    rtgui_rect_t *r2_stop;          /* rgn2 search end r */

    if (IS_REGION_INVALID(rgn1) || IS_REGION_INVALID(rgn2))
        return _invalid(dstRgn);

    /*
     * Initialization:
     *  set r1, r2, end1 and end2 appropriately, save the rectangles
     * of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1      = REGION_GET_RECTS(rgn1);
    num1    = REGION_DATA_NUM_RECTS(rgn1);
    end1    = REGION_END_RECT(rgn1);
    r2      = REGION_GET_RECTS(rgn2);
    num2    = REGION_DATA_NUM_RECTS(rgn2);
    end2    = REGION_END_RECT(rgn2);
    RT_ASSERT(num1 != 0);
    RT_ASSERT(num2 != 0);

    if (((dstRgn == rgn1) && num1) || ((dstRgn == rgn2) && num2) || \
        !dstRgn->data) {
        backup = dstRgn->data;
        dstRgn->data = &_null_region_data;
    } else {
        backup = RT_NULL;
    }

    if (dstRgn->data && dstRgn->data->size) {
        /* clear dstRgn->data */
        dstRgn->data->numRects = 0;
    }

    /* guess the final size */
    num1 = _MAX(num1, num2) << 1;
    _realloc_rects(dstRgn, num1);

    /*
     * Initialize top_y.
     *  In the upcoming loop, top_y and lower_y1 serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *  In the case of a non-overlapping band (only one of the regions
     * has points in the band), top_y is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * lower_y1 is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *  For an overlapping band (where the two regions intersect), lower_y1 clips
     * the top of the rectangles of both regions and top_y clips the bottoms.
     */
    top_y = _MIN(r1->y1, r2->y1);

    /*
     * prvStart serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. _coalesce, above.
     * In the beginning, there is no previous band, so prvStart == curStart
     * (curStart is set later on, of course, but the first band will always
     * start at index 0). prvStart and curStart must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prvStart = 0;

    do {
        RT_ASSERT(r1 != end1);
        RT_ASSERT(r2 != end2);

        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1_stop and r2_stop serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
        _find_next(r1, r1_stop, end1, r1_start_y1);
        _find_next(r2, r2_stop, end2, r2_start_y1);

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1_start_y1 < r2_start_y1) {
            /* rgn1 r on top */
            if (merge_b1) {
                rt_int16_t top;
                rt_int16_t btm;

                /* get space top and bottom */
                top = _MAX(r1_start_y1, top_y);
                btm = _MIN(r1->y2, r2_start_y1);
                if (top < btm) {
                    /* has space */
                    curStart = dstRgn->data->numRects;
                    _expend_verticaly(dstRgn, r1, r1_stop, top, btm);
                    coalesce(dstRgn, prvStart, curStart);
                }
            }
            lower_y1 = r2_start_y1;
        } else if (r2_start_y1 < r1_start_y1) {
            /* region2 r on top */
            if (merge_b2) {
                rt_int16_t top;
                rt_int16_t btm;

                /* get space top and bottom */
                top = _MAX(r2_start_y1, top_y);
                btm = _MIN(r2->y2, r1_start_y1);
                if (top < btm) {
                    /* has space */
                    curStart = dstRgn->data->numRects;
                    _expend_verticaly(dstRgn, r2, r2_stop, top, btm);
                    coalesce(dstRgn, prvStart, curStart);
                }
            }
            lower_y1 = r1_start_y1;
        } else {
            lower_y1 = r1_start_y1;
        }

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if top_y > lower_y1
         */
        top_y = _MIN(r1->y2, r2->y2);
        if (top_y > lower_y1) {
            /* overlap */
            curStart = dstRgn->data->numRects;
            if (SUCCESS != func(dstRgn, r1, r1_stop, r2, r2_stop, lower_y1,
                top_y, pOverlap))
                return FAILURE;
            coalesce(dstRgn, prvStart, curStart);
        }

        /*
         * If we've finished with a band (y2 == top_y) we skip forward
         * in the region to the next band.
         */
        if (r1->y2 == top_y) r1 = r1_stop;
        if (r2->y2 == top_y) r2 = r2_stop;
    } while (r1 != end1 && r2 != end2);

    /*
     * Deal with whichever region (if any) still has rectangles left.
     *
     * We only need to worry about banding and coalescing for the very first
     * band left.  After that, we can just group all remaining boxes,
     * regardless of how many bands, into one final append to the list.
     */

    if ((r1 != end1) && merge_b1) {
        rt_int16_t top;

        _find_next(r1, r1_stop, end1, r1_start_y1);

        /* get space top */
        top = _MAX(r1_start_y1, top_y);
        if (top < r1->y2) {
            /* has space */
            curStart = dstRgn->data->numRects;
            _expend_verticaly(dstRgn, r1, r1_stop, top, r1->y2);
            coalesce(dstRgn, prvStart, curStart);
            /* append the rest */
            _append_rects(dstRgn, r1_stop, end1);
        }
    } else if ((r2 != end2) && merge_b2) {
        rt_int16_t top;

        _find_next(r2, r2_stop, end2, r2_start_y1);

        /* get space top */
        top = _MAX(r2_start_y1, top_y);
        if (top < r2->y2) {
            curStart = dstRgn->data->numRects;
            _expend_verticaly(dstRgn, r2, r2_stop, top, r2->y2);
            coalesce(dstRgn, prvStart, curStart);
            /* append the rest */
            _append_rects(dstRgn, r2_stop, end2);
        }
    }

    if (backup)
        rtgui_free(backup);

    num2 = dstRgn->data->numRects;
    if (0 == num2) {
        freeData(dstRgn);
        dstRgn->data = &_null_region_data;
    } else if (1 == num2) {
        dstRgn->extents = *REGION_RECTS_PTR(dstRgn);
        freeData(dstRgn);
        dstRgn->data = RT_NULL;
    } else {
        _resize_data(dstRgn, num2);
    }

    return SUCCESS;
}

/*-
 *-----------------------------------------------------------------------
 * _set_extents --
 *  Reset the extents of a region to what they should be. Called by
 *  rtgui_region_subtract and rtgui_region_intersect as they can't figure it out along the
 *  way or do so easily, as rtgui_region_union can.
 *
 * Results:
 *  None.
 *
 * Side Effects:
 *  The region's 'extents' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
static void _set_extents(rtgui_region_t *rgn) {
    rtgui_rect_t *rect, *last;

    if (!rgn->data) return;
    if (!rgn->data->size) {
        rgn->extents.x2 = rgn->extents.x1;
        rgn->extents.y2 = rgn->extents.y1;
        return;
    }

    rect = REGION_RECTS_PTR(rgn);
    last = REGION_LAST_RECT(rgn);

    /*
     * Since rect is the first rectangle in the region, it must have the
     * smallest y1 and since last is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from rect and last, resp., as GOOD things to initialize them
     * to...
     */
    rgn->extents.x1 = rect->x1;
    rgn->extents.y1 = rect->y1;
    rgn->extents.x2 = last->x2;
    rgn->extents.y2 = last->y2;
    RT_ASSERT(rgn->extents.y1 < rgn->extents.y2);

    while (rect <= last) {
        if (rect->x1 < rgn->extents.x1)
            rgn->extents.x1 = rect->x1;
        if (rect->x2 > rgn->extents.x2)
            rgn->extents.x2 = rect->x2;
        rect++;
    }
    RT_ASSERT(rgn->extents.x1 < rgn->extents.x2);
}

/*======================================================================
 *      Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * _intersect_func --
 *  Handle an overlapping band for rtgui_region_intersect.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *  Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
static r_op_status_t _intersect_func(
    rtgui_region_t *rgn,
    rtgui_rect_t *r1,
    rtgui_rect_t *end1,
    rtgui_rect_t *r2,
    rtgui_rect_t *end2,
    rt_int16_t y1,
    rt_int16_t y2,
    int *notUsed) {
    rtgui_rect_t *end;
    rt_int16_t x1, x2;
    (void)notUsed;

    RT_ASSERT(y1 < y2);
    RT_ASSERT(r1 != end1 && r2 != end2);

    end = REGION_END_RECT(rgn);

    do {
        x1 = _MAX(r1->x1, r2->x1);
        x2 = _MIN(r1->x2, r2->x2);

        /*
         * If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         */
        if (x1 < x2) _append_new_rect(rgn, end, x1, y1, x2, y2);

        /*
         * Advance the pointer(s) with the leftmost right side, since the next
         * rectangle on that list may still overlap the other region's
         * current rectangle.
         */
        if (r1->x2 == x2) r1++;
        if (r2->x2 == x2) r2++;
    } while ((r1 != end1) && (r2 != end2));

    return SUCCESS;
}

r_op_status_t rtgui_region_intersect(rtgui_region_t *dstRgn,
    rtgui_region_t *rgn1, rtgui_region_t *rgn2) {
    GOOD(rgn1); GOOD(rgn2); GOOD(dstRgn);

    /* check for trivial reject */
    if (REGION_NO_RECT(rgn1) || REGION_NO_RECT(rgn2) || \
        !IS_R_INTERSECT(&rgn1->extents, &rgn2->extents)) {
        /* covers about 20% of all cases */
        freeData(dstRgn);
        dstRgn->extents.x2 = dstRgn->extents.x1;
        dstRgn->extents.y2 = dstRgn->extents.y1;
        if (IS_REGION_INVALID(rgn1) || IS_REGION_INVALID(rgn2)) {
            dstRgn->data = &_bad_region_data;
            return FAILURE;
        } else {
            dstRgn->data = &_null_region_data;
        }
    } else if (!rgn1->data && !rgn2->data) {
        /* Covers about 80% of cases that aren't trivially rejected */
        dstRgn->extents.x1 = _MAX(rgn1->extents.x1, rgn2->extents.x1);
        dstRgn->extents.y1 = _MAX(rgn1->extents.y1, rgn2->extents.y1);
        dstRgn->extents.x2 = _MIN(rgn1->extents.x2, rgn2->extents.x2);
        dstRgn->extents.y2 = _MIN(rgn1->extents.y2, rgn2->extents.y2);
        freeData(dstRgn);
        dstRgn->data = RT_NULL;
    } else if (!rgn2->data && IS_R_INSIDE(&rgn2->extents, &rgn1->extents)) {
        return rtgui_region_copy(dstRgn, rgn1);
    } else if (!rgn1->data && IS_R_INSIDE(&rgn1->extents, &rgn2->extents)) {
        return rtgui_region_copy(dstRgn, rgn2);
    } else if (rgn1 == rgn2) {
        return rtgui_region_copy(dstRgn, rgn1);
    } else {
        /* General purpose intersection */
        int notUsed;

        if (SUCCESS != region_op(dstRgn, rgn1, rgn2, _intersect_func, RT_FALSE,
            RT_FALSE, &notUsed))
            return FAILURE;
        _set_extents(dstRgn);
    }

    GOOD(dstRgn);
    return SUCCESS;
}
RTM_EXPORT(rtgui_region_intersect);

r_op_status_t rtgui_region_intersect_rect(rtgui_region_t *dstRgn,
    rtgui_region_t *rgn1, rtgui_rect_t *rect) {
    rtgui_region_t rgn;

    rgn.data = RT_NULL;
    rgn.extents.x1 = rect->x1;
    rgn.extents.y1 = rect->y1;
    rgn.extents.x2 = rect->x2;
    rgn.extents.y2 = rect->y2;
    return rtgui_region_intersect(dstRgn, rgn1, &rgn);
}
RTM_EXPORT(rtgui_region_intersect_rect);

/*======================================================================
 *      Region Union
 *====================================================================*/

#define _merge_left(rgn, r, end, _x1, _y1, _x2, _y2) { \
    if (r->x1 <= _x2) {                     \
        /* merge */                         \
        if (r->x1 < _x2) *pOverlap = SUCCESS; \
        x2 = _MAX(_x2, r->x2);              \
    } else {                                \
        /* add current rect then restart */ \
        _append_new_rect(rgn, end, _x1, _y1, _x2, _y2); \
        _x1 = r->x1;                        \
        _x2 = r->x2;                        \
    }                                       \
}

/*-
 *-----------------------------------------------------------------------
 * _union_func --
 *  Handle an overlapping band for the union operation. Picks the
 *  left-most rectangle each time and merges it into the region.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *  region is overwritten.
 *  pOverlap is set to SUCCESS if any boxes overlap.
 *
 *-----------------------------------------------------------------------
 */
static r_op_status_t _union_func(
    rtgui_region_t *rgn,
    rtgui_rect_t *r1,
    rtgui_rect_t *end1,
    rtgui_rect_t *r2,
    rtgui_rect_t *end2,
    rt_int16_t y1,
    rt_int16_t y2,
    int *pOverlap) {
    rtgui_rect_t *end;
    rt_int16_t x1, x2;

    RT_ASSERT(y1 < y2);
    RT_ASSERT((r1 != end1) && (r2 != end2));

    end = REGION_END_RECT(rgn);

    /* Start off current rectangle */
    if (r1->x1 < r2->x1) {
        x1 = r1->x1;
        x2 = r1->x2;
        r1++;
    } else {
        x1 = r2->x1;
        x2 = r2->x2;
        r2++;
    }

    do {
        if (r1->x1 < r2->x1) {
            _merge_left(rgn, r1, end, x1, y1, x2, y2);
            r1++;
        } else {
            _merge_left(rgn, r2, end, x1, y1, x2, y2);
            r2++;
        }
    } while ((r1 != end1) && (r2 != end2));

    /* Finish off whoever (if any) is left */
    if (r1 != end1) {
        do {
            _merge_left(rgn, r1, end, x1, y1, x2, y2); 
            r1++;
        } while (r1 != end1);
    } else if (r2 != end2) {
        do {
            _merge_left(rgn, r2, end, x1, y1, x2, y2);
            r2++;
        } while (r2 != end2);
    }

    /* add current rect */
    _append_new_rect(rgn, end, x1, y1, x2, y2);
    return SUCCESS;
}

/* Convenience function for performing union of region with a single rectangle */
r_op_status_t rtgui_region_union(rtgui_region_t *dstRgn, rtgui_region_t *rgn1,
    rtgui_region_t *rgn2) {
    int notUsed;
    /* Return SUCCESS if some overlap between rgn1, rgn2 */

    GOOD(rgn1); GOOD(rgn2); GOOD(dstRgn);
    /*  checks all the simple cases */
    /*
     * Region 1 and 2 are the same
     */
    if (rgn1 == rgn2) {
        return rtgui_region_copy(dstRgn, rgn1);
    }

    /*
     * Region 1 is empty
     */
    if (REGION_NO_RECT(rgn1)) {
        if (IS_REGION_INVALID(rgn1))
            return _invalid(dstRgn);
        if (dstRgn != rgn2)
            return rtgui_region_copy(dstRgn, rgn2);
        return SUCCESS;
    }

    /*
     * Region 2 is empty
     */
    if (REGION_NO_RECT(rgn2)) {
        if (IS_REGION_INVALID(rgn2))
            return _invalid(dstRgn);
        if (dstRgn != rgn1)
            return rtgui_region_copy(dstRgn, rgn1);
        return SUCCESS;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if (!rgn1->data && IS_R_INSIDE(&rgn1->extents, &rgn2->extents)) {
        if (dstRgn != rgn1)
            return rtgui_region_copy(dstRgn, rgn1);
        return SUCCESS;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if (!rgn2->data && IS_R_INSIDE(&rgn2->extents, &rgn1->extents)) {
        if (dstRgn != rgn2)
            return rtgui_region_copy(dstRgn, rgn2);
        return SUCCESS;
    }

    if (SUCCESS != region_op(dstRgn, rgn1, rgn2, _union_func, RT_TRUE, RT_TRUE,
        &notUsed))
        return FAILURE;

    dstRgn->extents.x1 = _MIN(rgn1->extents.x1, rgn2->extents.x1);
    dstRgn->extents.y1 = _MIN(rgn1->extents.y1, rgn2->extents.y1);
    dstRgn->extents.x2 = _MAX(rgn1->extents.x2, rgn2->extents.x2);
    dstRgn->extents.y2 = _MAX(rgn1->extents.y2, rgn2->extents.y2);

    GOOD(dstRgn);
    return SUCCESS;
}

r_op_status_t rtgui_region_union_rect(rtgui_region_t *dst, rtgui_region_t *src,
    rtgui_rect_t *rect) {
    rtgui_region_t rgn;

    rgn.data = RT_NULL;
    rgn.extents.x1 = rect->x1;
    rgn.extents.y1 = rect->y1;
    rgn.extents.x2 = rect->x2;
    rgn.extents.y2 = rect->y2;

    return rtgui_region_union(dst, src, &rgn);
}

/*======================================================================
 *      Batch Rectangle Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * rtgui_region_append --
 *
 *      "Append" the rgn rectangles onto the end of dstRgn, maintaining
 *      knowledge of YX-banding when it's easy.  Otherwise, dstRgn just
 *      becomes a non-y-x-banded random collection of rectangles, and not
 *      yet a true region.  After a sequence of appends, the caller must
 *      call rtgui_region_validate to ensure that a valid region is constructed.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *      dstRgn is modified if rgn has rectangles.
 *
 */
r_op_status_t rtgui_region_append(rtgui_region_t *dstRgn, rtgui_region_t *rgn) {
    rt_uint32_t num1, num2, size;
    rtgui_rect_t *rect1, *rect2;
    rt_bool_t prepend;

    if (IS_REGION_INVALID(rgn)) return _invalid(dstRgn);

    if (!rgn->data && (dstRgn->data == &_null_region_data)) {
        dstRgn->extents = rgn->extents;
        dstRgn->data = RT_NULL;
        return SUCCESS;
    }

    num2 = REGION_DATA_NUM_RECTS(rgn);
    if (!num2) return SUCCESS;

    prepend = RT_FALSE;

    num1 = REGION_DATA_NUM_RECTS(dstRgn);
    if (!num1 && (num2 < 200)) {
        size = 200; /* XXX pick numbers out of a hat */
    } else {
        size = num2;
    }
    _realloc_rects(dstRgn, size);

    rect1 = REGION_GET_RECTS(rgn);
    if (!num1) {
        dstRgn->extents = rgn->extents;
    } else if (dstRgn->extents.x2 > dstRgn->extents.x1) {
        rtgui_rect_t *first, *last;

        first = rect1;
        last = REGION_LAST_RECT(dstRgn);
        if ((first->y1 > last->y2) || ((first->y1 == last->y1) && \
            (first->y2 == last->y2) && (first->x1 > last->x2))) {
            /* rgn first below or at right of dstRgn last => extend dstRgn */
            dstRgn->extents.x1 = _MIN(dstRgn->extents.x1, rgn->extents.x1);
            dstRgn->extents.x2 = _MAX(dstRgn->extents.x2, rgn->extents.x2);
            dstRgn->extents.y2 = rgn->extents.y2;
        } else {
            first = REGION_RECTS_PTR(dstRgn);
            last = REGION_LAST_RECT(rgn);

            if ((first->y1 > last->y2) || ((first->y1 == last->y1) && \
                (first->y2 == last->y2) && (first->x1 > last->x2))) {
                /* dstRgn first below or at right of rgn last => extend dstRgn */
                prepend = RT_TRUE;
                dstRgn->extents.x1 = _MIN(dstRgn->extents.x1, rgn->extents.x1);
                dstRgn->extents.x2 = _MAX(dstRgn->extents.x2, rgn->extents.x2);
                dstRgn->extents.y1 = rgn->extents.y1;
            } else {
                dstRgn->extents.x2 = dstRgn->extents.x1;
            }
        }
    }

    if (prepend) {
        rect2 = REGION_RECT_AT(dstRgn, num2);
        if (1 == num1) {
            *rect2 = *REGION_RECTS_PTR(dstRgn);
        } else {
            rt_memmove(rect2, REGION_RECTS_PTR(dstRgn),
                num1 * sizeof(rtgui_rect_t));
        }
        rect2 = REGION_RECTS_PTR(dstRgn);
    } else {
        rect2 = REGION_RECTS_PTR(dstRgn) + num1;
    }

    if (1 == num2) {
        *rect2 = *rect1;
    } else {
        rt_memmove(rect2, rect1, num2 * sizeof(rtgui_rect_t));
    }
    dstRgn->data->numRects += num2;

    return SUCCESS;
}

#define _rect_swap(i, j) {                  \
    rtgui_rect_t tmp = rects[i];            \
    rects[i] = rects[j];                    \
    rects[j] = tmp;                         \
}

static void _quick_sort_rects(rtgui_rect_t rects[], rt_uint32_t num) {
    /* sort from top to bottom, left to right */
    rt_int16_t x1, y1;
    rt_uint32_t i, j;

    /* always called with num > 1 */
    RT_ASSERT(num > 1);

    do {
        if (num == 2) {
            if ((rects[0].y1 > rects[1].y1) || \
                ((rects[0].y1 == rects[1].y1) && (rects[0].x1 > rects[1].x1)))
                _rect_swap(0, 1);
            return;
        }

        /* Choose partition element, stick in location 0 */
        _rect_swap(0, num >> 1);
        y1 = rects[0].y1;
        x1 = rects[0].x1;

        /* Partition array */
        i = 0;
        j = num;
        do {
            do {
                i++;
                if (!((rects[i].y1 < y1) || \
                      ((rects[i].y1 == y1) && (rects[i].x1 < x1))))
                    break;
            } while (i < num);

            do {
                j--;
                if (!((rects[i].y1 > y1) || \
                      ((rects[i].y1 == y1) && (rects[i].x1 > x1))))
                    break;
            } while (1);

            if (i < j) _rect_swap(i, j);
        } while (i < j);

        /* move partition element back to middle */
        _rect_swap(0, j);

        /* sort [j + 1] ~ [num - 1] */
        if (num - (j + 1) > 1)
            _quick_sort_rects(&rects[j + 1], num - (j + 1));
        /* sort [0] ~ [j] */
        num = j;
    } while (num > 1);
}

/*-
 *-----------------------------------------------------------------------
 * rtgui_region_validate --
 *
 *      Take a ``region'' which is a non-y-x-banded random collection of
 *      rectangles, and compute a nice region which is the union of all the
 *      rectangles.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *      The passed-in ``region'' may be modified.
 *  pOverlap set to SUCCESS if any retangles overlapped, else FAILURE;
 *
 * Strategy:
 *      Step 1. Sort the rectangles into ascending order with primary key y1
 *      and secondary key x1.
 *
 *      Step 2. Split the rectangles into the RTGUI_MINimum number of proper y-x
 *      banded regions.  This may require horizontally merging
 *      rectangles, and vertically coalescing bands.  With any luck,
 *      this step in an identity transformation (ala the Box widget),
 *      or a coalescing into 1 box (ala Menus).
 *
 *  Step 3. Merge the separate regions down to a single region by calling
 *      rtgui_region_union.  Maximize the work each rtgui_region_union call does by using
 *      a binary merge.
 *
 *-----------------------------------------------------------------------
 */
r_op_status_t rtgui_region_validate(
    rtgui_region_t *badreg,
    int *pOverlap) {
    /* Descriptor for regions under construction  in Step 2. */
    typedef struct {
        rtgui_region_t rgn;
        rt_uint32_t prvStart;
        rt_uint32_t curStart;
    } RegionInfo;

    rt_uint32_t num2;   /* Original num2 for badreg     */
    RegionInfo *ri;     /* Array of current regions         */
    int numRI;      /* Number of entries used in ri     */
    int sizeRI;     /* Number of entries available in ri    */
    int i;      /* Index into rects             */
    int j;      /* Index into ri                */
    RegionInfo *rit;       /* &ri[j]                    */
    rtgui_region_t   *rgn;        /* ri[j].rgn              */
    rtgui_rect_t   *rect;        /* Current box in rects         */
    rtgui_rect_t   *riBox;      /* Last box in ri[j].rgn            */
    rtgui_region_t   *hreg;       /* ri[j_half].rgn             */
    r_op_status_t ret = SUCCESS;

    *pOverlap = FAILURE;
    if (!badreg->data) {
        GOOD(badreg);
        return SUCCESS;
    }
    num2 = badreg->data->numRects;
    if (!num2) {
        if (IS_REGION_INVALID(badreg))
            return FAILURE;
        GOOD(badreg);
        return SUCCESS;
    }
    if (badreg->extents.x1 < badreg->extents.x2) {
        if ((num2) == 1) {
            freeData(badreg);
            badreg->data = RT_NULL;
        } else {
            _resize_data(badreg, num2);
        }
        GOOD(badreg);
        return SUCCESS;
    }

    /* Step 1: Sort the rects array into ascending (y1, x1) order */
    _quick_sort_rects(REGION_RECTS_PTR(badreg), num2);

    /* Step 2: Scatter the sorted array into the RTGUI_MINimum number of regions */

    /* Set up the first region to be the first rectangle in badreg */
    /* Note that step 2 code will never overflow the ri[0].rgn rects array */
    ri = (RegionInfo *) rtgui_malloc(4 * sizeof(RegionInfo));
    if (!ri) return _invalid(badreg);
    sizeRI = 4;
    numRI = 1;
    ri[0].prvStart = 0;
    ri[0].curStart = 0;
    ri[0].rgn = *badreg;
    rect = REGION_RECTS_PTR(&ri[0].rgn);
    ri[0].rgn.extents = *rect;
    ri[0].rgn.data->numRects = 1;

    /* Now scatter rectangles into the RTGUI_MINimum set of valid regions.  If the
       next rectangle to be added to a region would force an existing rectangle
       in the region to be split up in order to maintain y-x banding, just
       forget it.  Try the next region.  If it doesn't fit cleanly into any
       region, make a new one. */

    for (i = num2; --i > 0;)
    {
        rect++;
        /* Look for a region to append box to */
        for (j = numRI, rit = ri; --j >= 0; rit++)
        {
            rgn = &rit->rgn;
            riBox = REGION_LAST_RECT(rgn);

            if (rect->y1 == riBox->y1 && rect->y2 == riBox->y2)
            {
                /* box is in same band as riBox.  Merge or append it */
                if (rect->x1 <= riBox->x2)
                {
                    /* Merge it with riBox */
                    if (rect->x1 < riBox->x2) *pOverlap = SUCCESS;
                    if (rect->x2 > riBox->x2) riBox->x2 = rect->x2;
                }
                else
                {
                    RECTALLOC_BAIL(rgn, 1, bail);
                    *REGION_END_RECT(rgn) = *rect;
                    rgn->data->numRects++;
                }
                goto NextRect;   /* So sue me */
            }
            else if (rect->y1 >= riBox->y2)
            {
                /* Put box into new band */
                if (rgn->extents.x2 < riBox->x2) rgn->extents.x2 = riBox->x2;
                if (rgn->extents.x1 > rect->x1)   rgn->extents.x1 = rect->x1;
                coalesce(rgn, rit->prvStart, rit->curStart);
                rit->curStart = rgn->data->numRects;
                RECTALLOC_BAIL(rgn, 1, bail);
                *REGION_END_RECT(rgn) = *rect;
                rgn->data->numRects++;
                goto NextRect;
            }
            /* Well, this region was inappropriate.  Try the next one. */
        } /* for j */

        /* Uh-oh.  No regions were appropriate.  Create a new one. */
        if (sizeRI == numRI)
        {
            /* Oops, allocate space for new region information */
            sizeRI <<= 1;
            rit = (RegionInfo *) rtgui_realloc(ri, sizeRI * sizeof(RegionInfo));
            if (!rit)
                goto bail;
            ri = rit;
            rit = &ri[numRI];
        }
        numRI++;
        rit->prvStart = 0;
        rit->curStart = 0;
        rit->rgn.extents = *rect;
        rit->rgn.data = (rtgui_region_data_t *)RT_NULL;
        if (!_alloc_rects(&rit->rgn, (i + numRI) / numRI)) /* MUST force allocation */
            goto bail;

    NextRect:
        ;
    } /* for i */

    /* Make a final pass over each region in order to coalesce and set
       extents.x2 and extents.y2 */

    for (j = numRI, rit = ri; --j >= 0; rit++)
    {
        rgn = &rit->rgn;
        riBox = REGION_LAST_RECT(rgn);
        rgn->extents.y2 = riBox->y2;
        if (rgn->extents.x2 < riBox->x2) rgn->extents.x2 = riBox->x2;
        coalesce(rgn, rit->prvStart, rit->curStart);
        if (rgn->data->numRects == 1) /* keep unions happy below */
        {
            freeData(rgn);
            rgn->data = (rtgui_region_data_t *)RT_NULL;
        }
    }

    /* Step 3: Union all regions into a single region */
    while (numRI > 1)
    {
        int half = numRI / 2;
        for (j = numRI & 1; j < (half + (numRI & 1)); j++)
        {
            rgn = &ri[j].rgn;
            hreg = &ri[j + half].rgn;
            if (!region_op(rgn, rgn, hreg, _union_func, SUCCESS, SUCCESS, pOverlap))
                ret = FAILURE;
            if (hreg->extents.x1 < rgn->extents.x1)
                rgn->extents.x1 = hreg->extents.x1;
            if (hreg->extents.y1 < rgn->extents.y1)
                rgn->extents.y1 = hreg->extents.y1;
            if (hreg->extents.x2 > rgn->extents.x2)
                rgn->extents.x2 = hreg->extents.x2;
            if (hreg->extents.y2 > rgn->extents.y2)
                rgn->extents.y2 = hreg->extents.y2;
            freeData(hreg);
        }
        numRI -= half;
    }
    *badreg = ri[0].rgn;
    rtgui_free(ri);
    GOOD(badreg);
    return ret;

    bail:

    for (i = 0; i < numRI; i++)
        freeData(&ri[i].rgn);
    rtgui_free(ri);

    return _invalid(badreg);
}

/*======================================================================
 *            Region Subtraction
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * _subtract_func --
 *  Overlapping band subtraction. x is the left-most point not yet
 *  checked.
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *  region may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */
static r_op_status_t _subtract_func(
    rtgui_region_t *rgn,
    rtgui_rect_t *r1,
    rtgui_rect_t *end1,
    rtgui_rect_t *r2,
    rtgui_rect_t *end2,
    rt_int16_t y1,
    rt_int16_t y2,
    int *notUsed) {
    rtgui_rect_t *end;
    rt_int16_t x;
    (void)notUsed;

    RT_ASSERT(y1 < y2);
    RT_ASSERT((r1 != end1) && (r2 != end2));

    end = REGION_END_RECT(rgn);
    x = r1->x1;

    do {
        if (r2->x2 <= x) {
            /*
             * Subtrahend entirely to left of minuend: go to next subtrahend.
             */
            r2++;
        } else if (r2->x1 <= x) {
            /*
             * Subtrahend preceeds minuend: nuke left edge of minuend.
             */
            x = r2->x2;
            if (x >= r1->x2) {
                /*
                 * Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend.
                 */
                r1++;
                if (r1 != end1)
                    x = r1->x1;
            } else {
                /*
                 * Subtrahend now used up since it doesn't extend beyond
                 * minuend
                 */
                r2++;
            }
        } else if (r2->x1 < r1->x2) {
            /*
             * Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend.
             */
            RT_ASSERT(x < r2->x1);
            _append_new_rect(rgn, end, x, y1, r2->x1, y2);

            x = r2->x2;
            if (x >= r1->x2) {
                /*
                 * Minuend used up: advance to new...
                 */
                r1++;
                if (r1 != end1)
                    x = r1->x1;
            } else {
                /*
                 * Subtrahend used up
                 */
                r2++;
            }
        } else {
            /*
             * Minuend used up: add any remaining piece before advancing.
             */
            if (r1->x2 > x)
                _append_new_rect(rgn, end, x, y1, r1->x2, y2);
            r1++;
            if (r1 != end1)
                x = r1->x1;
        }
    } while ((r1 != end1) && (r2 != end2));

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != end1) {
        RT_ASSERT(x < r1->x2);
        _append_new_rect(rgn, end, x, y1, r1->x2, y2);
        r1++;
        if (r1 != end1)
            x = r1->x1;
    }
    return SUCCESS;
}

/*-
 *-----------------------------------------------------------------------
 * rtgui_region_subtract --
 *  Subtract regS from regM and leave the result in regD.
 *  S stands for subtrahend, M for minuend and D for difference.
 *  M - S = D
 *
 * Results:
 *  SUCCESS if successful.
 *
 * Side Effects:
 *  regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */
r_op_status_t rtgui_region_subtract(rtgui_region_t *regD, rtgui_region_t *regM,
    rtgui_region_t *regS) {
    int notUsed;

    GOOD(regM); GOOD(regS); GOOD(regD);

    /* check for trivial rejects */
    if (REGION_NO_RECT(regM) || REGION_NO_RECT(regS) ||
            !IS_R_INTERSECT(&regM->extents, &regS->extents))
    {
        if (IS_REGION_INVALID(regS)) return _invalid(regD);
        return rtgui_region_copy(regD, regM);
    }
    else if (regM == regS)
    {
        freeData(regD);
        regD->extents.x2 = regD->extents.x1;
        regD->extents.y2 = regD->extents.y1;
        regD->data = &_null_region_data;
        return SUCCESS;
    }

    /* Add those rectangles in region 1 that aren't in region 2,
       do yucky substraction for overlaps, and
       just throw away rectangles in region 2 that aren't in region 1 */
    if (SUCCESS != region_op(regD, regM, regS, _subtract_func, RT_TRUE,
        RT_FALSE, &notUsed))
        return FAILURE;

    /*
     * Can't alter RegD's extents before we call region_op because
     * it might be one of the source regions and region_op depends
     * on the extents of those regions being unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to exaRTGUI_MINe fewer rectangles.
     */
    _set_extents(regD);

    GOOD(regD);
    return SUCCESS;
}

r_op_status_t rtgui_region_subtract_rect(rtgui_region_t *regD,
    rtgui_region_t *regM, rtgui_rect_t *rect) {
    rtgui_region_t rgn;

    rgn.data = RT_NULL;
    rgn.extents.x1 = rect->x1;
    rgn.extents.y1 = rect->y1;
    rgn.extents.x2 = rect->x2;
    rgn.extents.y2 = rect->y2;
    return rtgui_region_subtract(regD, regM, &rgn);
}

/*======================================================================
 *      Region Inversion
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * rtgui_region_inverse --
 *  Take a region and a box and return a region that is everything
 *  in the box but not in the region. The careful reader will note
 *  that this is the same as subtracting the region from the box...
 *
 * Results:
 *  SUCCESS.
 *
 * Side Effects:
 *  dstRgn is overwritten.
 *
 *-----------------------------------------------------------------------
 */
#if 0
r_op_status_t
rtgui_region_inverse(rtgui_region_t *dstRgn,       /* Destination region */
                     rtgui_region_t *rgn1,         /* Region to invert */
                     rtgui_rect_t *invRect)        /* Bounding box for inversion */
{
    rtgui_region_t    invReg;       /* Quick and dirty region made from the
                     * bounding box */
    int   overlap;  /* result ignored */

    GOOD(rgn1);
    GOOD(dstRgn);
    /* check for trivial rejects */
    if (REGION_NO_RECT(rgn1) || !IS_R_INTERSECT(invRect, &rgn1->extents))
    {
        if (IS_REGION_INVALID(rgn1)) return _invalid(dstRgn);
        dstRgn->extents = *invRect;
        freeData(dstRgn);
        dstRgn->data = (rtgui_region_data_t *)RT_NULL;
        return SUCCESS;
    }

    /* Add those rectangles in region 1 that aren't in region 2,
       do yucky substraction for overlaps, and
       just throw away rectangles in region 2 that aren't in region 1 */
    invReg.extents = *invRect;
    invReg.data = (rtgui_region_data_t *)RT_NULL;
    if (!region_op(dstRgn, &invReg, rgn1, _subtract_func, SUCCESS, FAILURE, &overlap))
        return FAILURE;

    /*
     * Can't alter dstRgn's extents before we call region_op because
     * it might be one of the source regions and region_op depends
     * on the extents of those regions being unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to exaRTGUI_MINe fewer rectangles.
     */
    _set_extents(dstRgn);
    GOOD(dstRgn);
    return SUCCESS;
}
#endif

/*
 *   RectIn(region, rect)
 *   This routine takes a pointer to a region and a pointer to a box
 *   and deterRTGUI_MINes if the box is outside/inside/partly inside the region.
 *
 *   The idea is to travel through the list of rectangles trying to cover the
 *   passed box with them. Anytime a piece of the rectangle isn't covered
 *   by a band of rectangles, partOut is set SUCCESS. Any time a rectangle in
 *   the region covers part of the box, partIn is set SUCCESS. The process ends
 *   when either the box has been completely covered (we reached a band that
 *   doesn't overlap the box, partIn is SUCCESS and partOut is false), the
 *   box has been partially covered (partIn == partOut == SUCCESS -- because of
 *   the banding, the first time this is true we know the box is only
 *   partially in the region) or is outside the region (we reached a band
 *   that doesn't overlap the box at all and partIn is false)
 */

rt_bool_t rtgui_region_contains_rect(rtgui_region_t *rgn, rtgui_rect_t *rect) {
    rt_int16_t x, y;
    rtgui_rect_t *pbox, *pboxEnd;
    int partIn, partOut;
    int num2;

    GOOD(rgn);
    num2 = REGION_DATA_NUM_RECTS(rgn);
    /* useful optimization */
    if (!num2 || !IS_R_INTERSECT(&rgn->extents, rect))
        return(RTGUI_REGION_OUT);

    if (num2 == 1)
    {
        /* We know that it must be rgnIN or rgnPART */
        if (IS_R_INSIDE(&rgn->extents, rect))
            return(RTGUI_REGION_IN);
        else
            return(RTGUI_REGION_PART);
    }

    partOut = FAILURE;
    partIn = FAILURE;

    /* (x,y) starts at upper left of rect, moving to the right and down */
    x = rect->x1;
    y = rect->y1;

    /* can stop when both partOut and partIn are SUCCESS, or we reach rect->y2 */
    for (pbox = REGION_RECTS_PTR(rgn), pboxEnd = pbox + num2;
            pbox != pboxEnd;
            pbox++)
    {

        if (pbox->y2 <= y)
            continue;    /* getting up to speed or skipping remainder of band */

        if (pbox->y1 > y)
        {
            partOut = SUCCESS;      /* missed part of rectangle above */
            if (partIn || (pbox->y1 >= rect->y2))
                break;
            y = pbox->y1;        /* x guaranteed to be == rect->x1 */
        }

        if (pbox->x2 <= x)
            continue;            /* not far enough over yet */

        if (pbox->x1 > x)
        {
            partOut = SUCCESS;      /* missed part of rectangle to left */
            if (partIn)
                break;
        }

        if (pbox->x1 < rect->x2)
        {
            partIn = SUCCESS;      /* definitely overlap */
            if (partOut)
                break;
        }

        if (pbox->x2 >= rect->x2)
        {
            y = pbox->y2;        /* finished with this band */
            if (y >= rect->y2)
                break;
            x = rect->x1;       /* reset x out to left again */
        }
        else
        {
            /*
             * Because boxes in a band are maximal width, if the first box
             * to overlap the rectangle doesn't completely cover it in that
             * band, the rectangle must be partially out, since some of it
             * will be uncovered in that band. partIn will have been set true
             * by now...
             */
            partOut = SUCCESS;
            break;
        }
    }

    return(partIn ? ((y < rect->y2) ? RTGUI_REGION_PART : RTGUI_REGION_IN) : RTGUI_REGION_OUT);
}

/* rtgui_region_translate (rgn, x, y)
   translates in place
*/
void rtgui_region_translate(rtgui_region_t *rgn, int x, int y) 
{
    int x1, x2, y1, y2;
    int nbox;
    rtgui_rect_t *pbox;

    GOOD(rgn);
    rgn->extents.x1 = x1 = rgn->extents.x1 + x;
    rgn->extents.y1 = y1 = rgn->extents.y1 + y;
    rgn->extents.x2 = x2 = rgn->extents.x2 + x;
    rgn->extents.y2 = y2 = rgn->extents.y2 + y;
    if (((x1 - INT16_MIN) | (y1 - INT16_MIN) | (INT16_MAX - x2) | (INT16_MAX - y2)) >= 0)
    {
        if (rgn->data && rgn->data->numRects)
        {
            nbox = rgn->data->numRects;
            for (pbox = REGION_RECTS_PTR(rgn); nbox--; pbox++)
            {
                pbox->x1 += x;
                pbox->y1 += y;
                pbox->x2 += x;
                pbox->y2 += y;
            }
        }
        return;
    }
    if (((x2 - INT16_MIN) | (y2 - INT16_MIN) | (INT16_MAX - x1) | (INT16_MAX - y1)) <= 0)
    {
        rgn->extents.x2 = rgn->extents.x1;
        rgn->extents.y2 = rgn->extents.y1;
        freeData(rgn);
        rgn->data = &_null_region_data;
        return;
    }
    if (x1 < INT16_MIN)
        rgn->extents.x1 = INT16_MIN;
    else if (x2 > INT16_MAX)
        rgn->extents.x2 = INT16_MAX;
    if (y1 < INT16_MIN)
        rgn->extents.y1 = INT16_MIN;
    else if (y2 > INT16_MAX)
        rgn->extents.y2 = INT16_MAX;

    if (rgn->data && rgn->data->numRects)
    {
        rtgui_rect_t *pboxout;

        nbox = rgn->data->numRects;
        for (pboxout = pbox = REGION_RECTS_PTR(rgn); nbox--; pbox++)
        {
            pboxout->x1 = x1 = pbox->x1 + x;
            pboxout->y1 = y1 = pbox->y1 + y;
            pboxout->x2 = x2 = pbox->x2 + x;
            pboxout->y2 = y2 = pbox->y2 + y;
            if (((x2 - INT16_MIN) | (y2 - INT16_MIN) |
                    (INT16_MAX - x1) | (INT16_MAX - y1)) <= 0)
            {
                rgn->data->numRects--;
                continue;
            }
            if (x1 < INT16_MIN)
                pboxout->x1 = INT16_MIN;
            else if (x2 > INT16_MAX)
                pboxout->x2 = INT16_MAX;
            if (y1 < INT16_MIN)
                pboxout->y1 = INT16_MIN;
            else if (y2 > INT16_MAX)
                pboxout->y2 = INT16_MAX;
            pboxout++;
        }
        if (pboxout != pbox)
        {
            if (rgn->data->numRects == 1)
            {
                rgn->extents = *REGION_RECTS_PTR(rgn);
                freeData(rgn);
                rgn->data = (rtgui_region_data_t *)RT_NULL;
            }
            else
                _set_extents(rgn);
        }
    }
}

void rtgui_region_reset(rtgui_region_t *rgn, rtgui_rect_t *rect) {
    GOOD(rgn);
    freeData(rgn);

    rtgui_region_init_with_extent(rgn, rect);
}
RTM_EXPORT(rtgui_region_reset);

/* box is "return" value */
rt_bool_t rtgui_region_contains_point(rtgui_region_t *rgn, int x, int y,
    rtgui_rect_t *box) {
    rtgui_rect_t *pbox, *pboxEnd;
    int num2;

    GOOD(rgn);
    num2 = REGION_DATA_NUM_RECTS(rgn);
    if (!num2 || !IS_P_INSIDE(&rgn->extents, x, y))
        return RT_FALSE;

    if (num2 == 1) {
        *box = rgn->extents;
        return RT_TRUE;
    }

    for (pbox = REGION_RECTS_PTR(rgn), pboxEnd = pbox + num2;
         pbox != pboxEnd;
         pbox++) {
        if (y >= pbox->y2)
            continue;       /* not there yet */
        if ((y < pbox->y1) || (x < pbox->x1))
            break;          /* missed it */
        if (x >= pbox->x2)
            continue;       /* not there yet */
        *box = *pbox;
        return RT_TRUE;
    }

    return RT_FALSE;
}

rt_bool_t rtgui_region_not_empty(rtgui_region_t *rgn) {
    GOOD(rgn);

    return (!REGION_NO_RECT(rgn));
}

void rtgui_region_empty(rtgui_region_t *rgn) {
    GOOD(rgn);
    freeData(rgn);

    rgn->extents = _null_rect;
    rgn->data = &_null_region_data;
}

rtgui_rect_t *rtgui_region_extents(rtgui_region_t *rgn) {
    GOOD(rgn);
    return(&rgn->extents);
}

#define RTGUI_REGION_TRACE

#ifdef RTGUI_REGION_TRACE
void rtgui_region_dump(rtgui_region_t *rgn)
{
    int num;
    int i;
    rtgui_rect_t *rects;

    num = REGION_DATA_NUM_RECTS(rgn);
    rects = REGION_GET_RECTS(rgn);
    rt_kprintf("extents: (%d,%d) (%d,%d)\n",
               rgn->extents.x1, rgn->extents.y1,
               rgn->extents.x2, rgn->extents.y2);

    for (i = 0; i < num; i++)
    {
        rt_kprintf("box[%d]: (%d,%d) (%d,%d)\n", i,
                   rects[i].x1, rects[i].y1,
                   rects[i].x2, rects[i].y2);
    }
}
RTM_EXPORT(rtgui_region_dump);

void rtgui_region_draw_clip(rtgui_region_t *rgn, rtgui_dc_t *dc)
{
    int i;
    int num;
    int x, y;
    rtgui_rect_t *rects;
    rtgui_color_t fc;
    char text[32];

    fc = RTGUI_DC_FC(dc);
    RTGUI_DC_FC(dc) = RED;

    num = REGION_DATA_NUM_RECTS(rgn);
    rects = REGION_GET_RECTS(rgn);

    x = rgn->extents.x1;
    y = rgn->extents.y1;

    for (i = 0; i < num; i++)
    {
        rtgui_rect_t rect;

        rect = rects[i];

        rtgui_rect_move(&rect, -x, -y);
        rtgui_dc_draw_rect(dc, &rect);

        rt_snprintf(text, sizeof(text) - 1, "%d", i);
        rtgui_dc_draw_text(dc, text, &rect);
    }

    RTGUI_DC_FC(dc) = fc;
}
RTM_EXPORT(rtgui_region_draw_clip);
#endif

rt_bool_t rtgui_region_is_flat(rtgui_region_t *rgn) {
    return (1 == REGION_DATA_NUM_RECTS(rgn));
}
RTM_EXPORT(rtgui_region_is_flat);

void rtgui_rect_move(rtgui_rect_t *rect, int x, int y) {
    rect->x1 += x;
    rect->x2 += x;
    rect->y1 += y;
    rect->y2 += y;
}
RTM_EXPORT(rtgui_rect_move);

void rtgui_rect_move_to_point(rtgui_rect_t *rect, int x, int y) {
    int mx, my;

    mx = x - rect->x1;
    my = y - rect->y1;

    rect->x1 += mx;
    rect->x2 += mx;
    rect->y1 += my;
    rect->y2 += my;
}
RTM_EXPORT(rtgui_rect_move_to_point);

void rtgui_rect_move_align(const rtgui_rect_t *rect, rtgui_rect_t *to,
    int align) {
    int dw, dh;

    /* get delta width and height */
    dw = RECT_W(*rect) - RECT_W(*to);
    dh = RECT_H(*rect) - RECT_H(*to);
    if (dw < 0) dw = 0;
    if (dh < 0) dh = 0;

    /* move to insider of rect */
    rtgui_rect_move_to_point(to, rect->x1, rect->y1);

    /* limited the destination rect to source rect */
    if (dw == 0) to->x2 = rect->x2;
    if (dh == 0) to->y2 = rect->y2;

    if (align & RTGUI_ALIGN_RIGHT) {
        /* align to right */
        to->x1 += dw;
        to->x2 += dw;
    } else if (align & RTGUI_ALIGN_CENTER_HORIZONTAL) {
        /* align to center horizontal */
        to->x1 += (dw >> 1);
        to->x2 += (dw >> 1);
    }

    if (align & RTGUI_ALIGN_BOTTOM) {
        /* align to bottom */
        to->y1 += dh;
        to->y2 += dh;
    } else if (align & RTGUI_ALIGN_CENTER_VERTICAL) {
        /* align to center vertical */
        to->y1 += (dh >> 1);
        to->y2 += (dh >> 1);
    }
}
RTM_EXPORT(rtgui_rect_move_align);

void rtgui_rect_inflate(rtgui_rect_t *rect, int d) {
    rect->x1 -= d;
    rect->x2 += d;
    rect->y1 -= d;
    rect->y2 += d;
}
RTM_EXPORT(rtgui_rect_inflate);

/* put the intersect of src rect and dest rect to dest */
void rtgui_rect_intersect(rtgui_rect_t *src, rtgui_rect_t *dest) {
    if (dest->x1 < src->x1) dest->x1 = src->x1;
    if (dest->y1 < src->y1) dest->y1 = src->y1;
    if (dest->x2 > src->x2) dest->x2 = src->x2;
    if (dest->y2 > src->y2) dest->y2 = src->y2;
}
RTM_EXPORT(rtgui_rect_intersect);

/* union src rect into dest rect */
void rtgui_rect_union(rtgui_rect_t *src, rtgui_rect_t *dest) {
    if (rtgui_rect_is_empty(dest)) {
        *dest = *src;
        return;
    }

    if (dest->x1 > src->x1) dest->x1 = src->x1;
    if (dest->y1 > src->y1) dest->y1 = src->y1;
    if (dest->x2 < src->x2) dest->x2 = src->x2;
    if (dest->y2 < src->y2) dest->y2 = src->y2;
}
RTM_EXPORT(rtgui_rect_union);

rt_bool_t rtgui_rect_contains_point(const rtgui_rect_t *rect, int x, int y) {
    return IS_P_INSIDE(rect, x, y);
}
RTM_EXPORT(rtgui_rect_contains_point);

rt_bool_t rtgui_rect_contains_rect(const rtgui_rect_t *rect1,
    const rtgui_rect_t *rect2) {
    return (IS_P_INSIDE(rect1, rect2->x1, rect2->y1) && \
            IS_P_INSIDE(rect1, rect2->x1, rect2->y2) && \
            IS_P_INSIDE(rect1, rect2->x2, rect2->y1) && \
            IS_P_INSIDE(rect1, rect2->x2, rect2->y2) );
}
RTM_EXPORT(rtgui_rect_contains_rect);

rt_bool_t rtgui_rect_is_intersect(const rtgui_rect_t *rect1,
    const rtgui_rect_t *rect2) {
    if (
        IS_P_INSIDE(rect1, rect2->x1, rect2->y1) || \
        IS_P_INSIDE(rect1, rect2->x1, rect2->y2) || \
        IS_P_INSIDE(rect1, rect2->x2, rect2->y1) || \
        IS_P_INSIDE(rect1, rect2->x2, rect2->y2)) {
        return RT_TRUE;
    } else if (
        IS_P_INSIDE(rect2, rect1->x1, rect1->y1) || \
        IS_P_INSIDE(rect2, rect1->x1, rect1->y2) || \
        IS_P_INSIDE(rect2, rect1->x2, rect1->y1) || \
        IS_P_INSIDE(rect2, rect1->x2, rect1->y2)) {
        return RT_TRUE;
    } else if (IS_R_CROSS(rect1, rect2)) {
        return RT_TRUE;
    } else if (IS_R_CROSS(rect2, rect1)) {
        return RT_TRUE;
    }
    return RT_FALSE;
}
RTM_EXPORT(rtgui_rect_is_intersect);

rt_bool_t rtgui_rect_is_equal(const rtgui_rect_t *rect1,
    const rtgui_rect_t *rect2) {
    return (*((rt_uint32_t *)rect1) == *((rt_uint32_t *)rect2) && \
            *((rt_uint32_t *)rect1 + 1) == *((rt_uint32_t *)rect2 + 1));
}
RTM_EXPORT(rtgui_rect_is_equal);

rt_bool_t rtgui_rect_is_empty(const rtgui_rect_t *rect)
{
    if (rtgui_rect_is_equal(rect, &_null_rect) == RT_EOK) return RT_TRUE;
    return RT_FALSE;
}
RTM_EXPORT(rtgui_rect_is_empty);

rtgui_rect_t *rtgui_rect_set(rtgui_rect_t *rect, int x, int y, int w, int h)
{
    RT_ASSERT(rect != RT_NULL);

    rect->x1 = x;
    rect->y1 = y;
    rect->x2 = x + w;
    rect->y2 = y + h;

    return rect;
}
RTM_EXPORT(rtgui_rect_set);

