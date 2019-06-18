/*
 * File      : mouse.c
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
#include "include/driver.h"
#include "include/app/mouse.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "APP_MOS"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
#ifdef RTGUI_USING_MOUSE_CURSOR
    static void rtgui_cursor_restore(void);
    static void rtgui_cursor_save(void);
    static void rtgui_cursor_show(void);
#endif
#ifdef RTGUI_USING_WINMOVE
    static void rtgui_winrect_restore(void);
    static void rtgui_winrect_save(void);
    static void rtgui_winrect_show(void);
#endif

/* Private typedef -----------------------------------------------------------*/
struct rtgui_cursor {
    /* screen byte per pixel */
    rt_uint16_t byte_pp;
    /* screen pitch */
    rt_uint16_t screen_pitch;
    /* current cursor x and y */
    rt_uint16_t cx, cy;
    #ifdef RTGUI_USING_MOUSE_CURSOR
        /* cursor pitch */
        rt_uint16_t cursor_pitch;
        /* show cursor and show cursor count */
        rt_bool_t show_cursor;
        rt_base_t show_cursor_count;
        /* cursor rect info */
        rtgui_rect_t rect;
        /* cursor image and saved cursor */
        rtgui_image_t *cursor_image;
        rt_uint8_t *cursor_saved;
    #endif /* RTGUI_USING_MOUSE_CURSOR */
    #ifdef RTGUI_USING_WINMOVE
        /* move window rect and border */
        rtgui_win_t *win;
        rtgui_rect_t win_rect;
        rt_uint8_t *win_left, *win_right;
        rt_uint8_t *win_top, *win_bottom;
        rt_bool_t win_rect_show, win_rect_has_saved;
    #endif /* RTGUI_USING_WINMOVE */
};
typedef struct rtgui_cursor rtgui_cursor_t;

/* Private define ------------------------------------------------------------*/
#define WIN_MOVE_BORDER                     4
#define display()                           (rtgui_get_graphic_device())

/* Private variables ---------------------------------------------------------*/
static rtgui_cursor_t *_cursor;
#ifdef RTGUI_USING_MOUSE_CURSOR
    static const char *cursor_xpm[] = {
        "16 16 35 1",
        "  c None",
        ". c #A0B8D0",
        "+ c #F0F0F0",
        "@ c #FFFFFF",
        "# c #F0F8F0",
        "$ c #A0B0D0",
        "% c #90A8C0",
        "& c #A0B0C0",
        "* c #E0E8F0",
        "= c #8090B0",
        "- c #D0D8E0",
        "; c #7080A0",
        "> c #90A0B0",
        ", c #FFF8FF",
        "' c #F0F8FF",
        ") c #607090",
        "! c #8098B0",
        "~ c #405060",
        "{ c #405070",
        "] c #506070",
        "^ c #607080",
        "/ c #708090",
        "( c #7088A0",
        "_ c #D0D0E0",
        ": c #607890",
        "< c #C0D0E0",
        "[ c #C0C8D0",
        "} c #506880",
        "| c #5F778F",
        "1 c #D0D8F0",
        "2 c #506080",
        "3 c #C0C8E0",
        "4 c #A0A8C0",
        "5 c #405870",
        "6 c #5F6F8F",
        "   .            ",
        "   ..           ",
        "   .+.          ",
        "   .@#$         ",
        "   $@@+%        ",
        "   &@@@*=       ",
        "   %@@@@-;      ",
        "   >@@,''-)     ",
        "   !,''+)~{]    ",
        "   ='-^*/       ",
        "   (_{:<[^      ",
        "   ;} |:12      ",
        "   /   )345     ",
        "       6}${     ",
        "        5{      ",
        "                "
    };
    struct rt_mutex cursor_lock;
#endif

/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_mouse_init(void) {
    rt_err_t ret;

    ret = -RT_ENOMEM;

    do {
        const rtgui_graphic_driver_t *disp = display();

        if (_cursor) rtgui_mouse_uninit();

        _cursor = rtgui_malloc(sizeof(rtgui_cursor_t));
        if (!_cursor) break;

        /* init cursor */
        rt_memset(_cursor, 0, sizeof(rtgui_cursor_t));
        _cursor->byte_pp = _BIT2BYTE(disp->bits_per_pixel);
        _cursor->screen_pitch = _cursor->byte_pp * disp->width;

        #ifdef RTGUI_USING_MOUSE_CURSOR
            /* init cursor_lock */
            rt_mutex_init(&cursor_lock, "cursor", RT_IPC_FLAG_FIFO);
            /* cursor image */
            _cursor->cursor_image = rtgui_image_create_from_mem("xpm",
                (const rt_uint8_t *)cursor_xpm, sizeof(cursor_xpm), RT_TRUE);
            if (!_cursor->cursor_image) {
                LOG_E("cursor img mem err");
                break;
            }
            /* init rect */
            _cursor->rect.x1 = _cursor->rect.y1 = 0;
            _cursor->rect.x2 = _cursor->cursor_image->w;
            _cursor->rect.y2 = _cursor->cursor_image->h;
            _cursor->cursor_pitch = _cursor->cursor_image->w * _cursor->byte_pp;
            _cursor->show_cursor = RT_TRUE;
            _cursor->show_cursor_count = 0;
            _cursor->cursor_saved = rtgui_malloc(
                _cursor->cursor_pitch * _cursor->cursor_image->h);
            if (!_cursor->cursor_saved) break;
        #endif /* RTGUI_USING_MOUSE_CURSOR */

        #ifdef RTGUI_USING_WINMOVE
            /* init window move */
            _cursor->win_rect_has_saved = RT_FALSE;
            _cursor->win_rect_show = RT_FALSE;
            _cursor->win_left = rtgui_malloc(
                _cursor->byte_pp * disp->height * WIN_MOVE_BORDER);
            if (!_cursor->win_left) break;
            _cursor->win_right = rtgui_malloc(
                _cursor->byte_pp * disp->height * WIN_MOVE_BORDER);
            if (!_cursor->win_right) break;
            _cursor->win_top = rtgui_malloc(
                _cursor->byte_pp * disp->width  * WIN_MOVE_BORDER);
            if (!_cursor->win_top) break;
            _cursor->win_bottom = rtgui_malloc(
                _cursor->byte_pp * disp->width  * WIN_MOVE_BORDER);
            if (!_cursor->win_bottom) break;
        #endif /* RTGUI_USING_WINMOVE */

        ret = RT_EOK;
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("mouse init err");
        rtgui_mouse_uninit();
    }
    return ret;
}

void rtgui_mouse_uninit(void) {
    if (!_cursor) return;

    #ifdef RTGUI_USING_MOUSE_CURSOR
        rt_mutex_detach(&cursor_lock);
        if (_cursor->cursor_image)
            rtgui_image_destroy(_cursor->cursor_image);
        if (_cursor->cursor_saved)
            rtgui_free(_cursor->cursor_saved);
    #endif

    #ifdef RTGUI_USING_WINMOVE
        if (_cursor->win_left) rtgui_free(_cursor->win_left);
        if (_cursor->win_right) rtgui_free(_cursor->win_right);
        if (_cursor->win_top) rtgui_free(_cursor->win_top);
        if (_cursor->win_bottom) rtgui_free(_cursor->win_bottom);
    #endif

    rtgui_free(_cursor);
    _cursor = RT_NULL;
}

void rtgui_mouse_moveto(rt_uint32_t x, rt_uint32_t y) {
    #ifdef RTGUI_USING_MOUSE_CURSOR
        rt_mutex_take(&cursor_lock, RT_WAITING_FOREVER);
    #endif

    if ((x != _cursor->cx) || (y != _cursor->cy)) {
        #ifdef RTGUI_USING_WINMOVE
            if (_cursor->win_rect_show) {
                if (_cursor->win_rect_has_saved == RT_TRUE) {
                    rtgui_winrect_restore();
                }

                #ifdef RTGUI_USING_MOUSE_CURSOR
                    rtgui_mouse_hide_cursor();
                #endif

                /* move winrect */
                rtgui_rect_move(&(_cursor->win_rect),
                    x - _cursor->cx, y - _cursor->cy);
                rtgui_winrect_save();

                /* move current cursor */
                _cursor->cx = x;
                _cursor->cy = y;

                /* show cursor */
                #ifdef RTGUI_USING_MOUSE_CURSOR
                    rtgui_mouse_show_cursor();
                #endif
                /* show winrect */
                rtgui_winrect_show();
            } else {
        #endif /* RTGUI_USING_WINMOVE */

        #ifdef RTGUI_USING_MOUSE_CURSOR
            rtgui_mouse_hide_cursor();
        #endif
        /* move current cursor */
        _cursor->cx = x;
        _cursor->cy = y;

        /* show cursor */
        #ifdef RTGUI_USING_MOUSE_CURSOR
            rtgui_mouse_show_cursor();
        #endif

        #ifdef RTGUI_USING_WINMOVE
            }
        #endif

        #ifdef RTGUI_USING_HW_CURSOR
            rtgui_cursor_set_position(_cursor->cx, _cursor->cy);
        #endif
    }

    #ifdef RTGUI_USING_MOUSE_CURSOR
        rt_mutex_release(&cursor_lock);
    #endif
}

void rtgui_mouse_set_position(int x, int y)
{
    /* move current cursor */
    _cursor->cx = x;
    _cursor->cy = y;

#ifdef RTGUI_USING_HW_CURSOR
    rtgui_cursor_set_position(_cursor->cx, _cursor->cy);
#endif
}

#ifdef RTGUI_USING_MOUSE_CURSOR
void rtgui_mouse_set_cursor_enable(rt_bool_t enable)
{
    _cursor->show_cursor = enable;
}

/* set current cursor image */
void rtgui_mouse_set_cursor(rtgui_image_t *cursor)
{
}

void rtgui_mouse_get_cursor_rect(rtgui_rect_t *rect)
{
    if (rect != RT_NULL)
    {
        *rect = _cursor->rect;
    }
}

void rtgui_mouse_show_cursor()
{
    if (_cursor->show_cursor == RT_FALSE)
        return;

    _cursor->show_cursor_count ++;
    if (_cursor->show_cursor_count == 1)
    {
        /* save show mouse area */
        rtgui_cursor_save();

        /* show mouse cursor */
        rtgui_cursor_show();
    }
}

void rtgui_mouse_hide_cursor(void) {
    if (_cursor->show_cursor == RT_FALSE)
        return;

    if (_cursor->show_cursor_count == 1) {
        /* display the cursor coverage area */
        rtgui_cursor_restore();
    }
    _cursor->show_cursor_count--;
}

rt_bool_t rtgui_mouse_is_intersect(rtgui_rect_t *r)
{
    return rtgui_rect_is_intersect(&(_cursor->rect), r) == RT_EOK ? RT_TRUE : RT_FALSE;
}

/* display the saved cursor area to screen */
static void rtgui_cursor_restore()
{
    rt_base_t idx, height, cursor_pitch;
    rt_uint8_t *cursor_ptr, *fb_ptr;

    fb_ptr = rtgui_graphic_driver_get_framebuffer(RT_NULL) + _cursor->cy * _cursor->screen_pitch
             + _cursor->cx * _cursor->byte_pp;
    cursor_ptr = _cursor->cursor_saved;

    height = (_cursor->cy + _cursor->cursor_image->h <
              rtgui_get_graphic_device()->height) ? _cursor->cursor_image->h :
             rtgui_get_graphic_device()->height - _cursor->cy;

    cursor_pitch = (_cursor->cx + _cursor->cursor_image->w <
                    rtgui_get_graphic_device()->width) ? _cursor->cursor_pitch :
                   (rtgui_get_graphic_device()->width - _cursor->cx) * _cursor->byte_pp;

    for (idx = 0; idx < height; idx ++)
    {
        rt_memcpy(fb_ptr, cursor_ptr, cursor_pitch);

        fb_ptr += _cursor->screen_pitch;
        cursor_ptr += _cursor->cursor_pitch;
    }
}

/* save the cursor coverage area from screen */
static void rtgui_cursor_save()
{
    rt_base_t idx, height, cursor_pitch;
    rt_uint8_t *cursor_ptr, *fb_ptr;

    fb_ptr = rtgui_graphic_driver_get_framebuffer(RT_NULL) + _cursor->cy * _cursor->screen_pitch +
             _cursor->cx * _cursor->byte_pp;
    cursor_ptr = _cursor->cursor_saved;

    height = (_cursor->cy + _cursor->cursor_image->h <
              rtgui_get_graphic_device()->height) ? _cursor->cursor_image->h :
             rtgui_get_graphic_device()->height - _cursor->cy;

    cursor_pitch = (_cursor->cx + _cursor->cursor_image->w <
                    rtgui_get_graphic_device()->width) ? _cursor->cursor_pitch :
                   (rtgui_get_graphic_device()->width - _cursor->cx) * _cursor->byte_pp;

    for (idx = 0; idx < height; idx ++)
    {
        rt_memcpy(cursor_ptr, fb_ptr, cursor_pitch);

        fb_ptr += _cursor->screen_pitch;
        cursor_ptr += _cursor->cursor_pitch;
    }
}

static void rtgui_cursor_show()
{
    // FIXME: the prototype of set_pixel is using int so we have to use int
    // as well. Might be uniformed with others in the future
    int x, y;
    rtgui_color_t *ptr;
    rtgui_rect_t rect;
    void (*set_pixel)(rtgui_color_t * c, int x, int y);

    ptr = (rtgui_color_t *) _cursor->cursor_image->data;
    set_pixel = rtgui_get_graphic_device()->ops->set_pixel;

    rtgui_mouse_get_cursor_rect(&rect);
    rtgui_rect_move(&rect, _cursor->cx, _cursor->cy);

    /* draw each point */
    for (y = rect.y1; y < rect.y2; y ++)
    {
        for (x = rect.x1; x < rect.x2; x++)
        {
            /* not alpha */
            if ((*ptr >> 24) != 255)
            {
                set_pixel(ptr, x, y);
            }

            /* move to next color buffer */
            ptr ++;
        }
    }

    /* update rect */
    rtgui_graphic_driver_screen_update(rtgui_get_graphic_device(), &rect);
}
#endif

#ifdef RTGUI_USING_WINMOVE
void rtgui_winrect_set(rtgui_win_t *win)
{
    /* set win rect show */
    _cursor->win_rect_show = RT_TRUE;

    /* set win rect */
    _cursor->win_rect =
        win->_title == RT_NULL ?
        TO_WIDGET(win)->extent :
        TO_WIDGET(win->_title)->extent;

    _cursor->win = win;
}

rt_bool_t rtgui_winrect_moved_done(rtgui_rect_t *winrect,
    rtgui_win_t **win) {
    rt_bool_t moved = RT_FALSE;

    /* restore winrect */
    if (_cursor->win_rect_has_saved) {
        rtgui_winrect_restore();
        moved = RT_TRUE;
    }

    /* clear win rect show */
    _cursor->win_rect_show = RT_FALSE;
    _cursor->win_rect_has_saved = RT_FALSE;
    /* return win rect */
    if (winrect) *winrect = _cursor->win_rect;
    if (win) *win = _cursor->win;

    return moved;
}

rt_bool_t rtgui_winrect_is_moved()
{
    return _cursor->win_rect_show;
}

/* show winrect */
static void rtgui_winrect_show()
{
    rt_uint16_t x, y;
    rtgui_color_t c;
    rtgui_rect_t screen_rect, win_rect, win_rect_inner;
    void (*set_pixel)(rtgui_color_t * c, int x, int y);

    c = black;
    set_pixel = rtgui_get_graphic_device()->ops->set_pixel;

    win_rect = _cursor->win_rect;
    win_rect_inner = win_rect;
    rtgui_rect_inflate(&win_rect_inner, -WIN_MOVE_BORDER);

    rtgui_graphic_driver_get_rect(rtgui_get_graphic_device(),
                                  &screen_rect);
    rtgui_rect_intersect(&screen_rect, &win_rect);
    rtgui_rect_intersect(&screen_rect, &win_rect_inner);

    /* draw left */
    for (y = win_rect.y1; y < win_rect.y2; y ++)
    {
        for (x = win_rect.x1; x < win_rect_inner.x1; x++)
            if ((x + y) & 0x01) set_pixel(&c, x, y);
    }

    /* draw right */
    for (y = win_rect.y1; y < win_rect.y2; y ++)
    {
        for (x = win_rect_inner.x2; x < win_rect.x2; x++)
            if ((x + y) & 0x01) set_pixel(&c, x, y);
    }

    /* draw top border */
    for (y = win_rect.y1; y < win_rect_inner.y1; y ++)
    {
        for (x = win_rect_inner.x1; x < win_rect_inner.x2; x++)
            if ((x + y) & 0x01) set_pixel(&c, x, y);
    }

    /* draw bottom border */
    for (y = win_rect_inner.y2; y < win_rect.y2; y ++)
    {
        for (x = win_rect_inner.x1; x < win_rect_inner.x2; x++)
            if ((x + y) & 0x01) set_pixel(&c, x, y);
    }

    /* update rect */
    rtgui_graphic_driver_screen_update(rtgui_get_graphic_device(), &win_rect);
}

#define display_direct_memcpy(src, dest, src_pitch, dest_pitch, height, len)    \
    for (idx = 0; idx < height; idx ++)     \
    {                                       \
        rt_memcpy(dest, src, len);             \
        src  += src_pitch;                  \
        dest += dest_pitch;                 \
    }

static void rtgui_winrect_restore()
{
    rt_uint8_t *winrect_ptr, *fb_ptr, *driver_fb;
    int winrect_pitch, idx;
    rtgui_rect_t screen_rect, win_rect;

    driver_fb = rtgui_graphic_driver_get_framebuffer(RT_NULL);
    win_rect = _cursor->win_rect;

    rtgui_graphic_driver_get_rect(rtgui_get_graphic_device(),
                                  &screen_rect);
    rtgui_rect_intersect(&screen_rect, &win_rect);

    /* restore winrect left */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             win_rect.x1 * _cursor->byte_pp;
    winrect_ptr = _cursor->win_left;
    winrect_pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
    display_direct_memcpy(winrect_ptr, fb_ptr, winrect_pitch, _cursor->screen_pitch,
                          (win_rect.y2 - win_rect.y1), winrect_pitch);

    /* restore winrect right */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             (win_rect.x2 - WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_right;
    winrect_pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
    display_direct_memcpy(winrect_ptr, fb_ptr, winrect_pitch, _cursor->screen_pitch,
                          (win_rect.y2 - win_rect.y1), winrect_pitch);

    /* restore winrect top */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_top;
    winrect_pitch = (win_rect.x2 - win_rect.x1 - 2 * WIN_MOVE_BORDER) * _cursor->byte_pp;
    display_direct_memcpy(winrect_ptr, fb_ptr, winrect_pitch, _cursor->screen_pitch,
                          WIN_MOVE_BORDER, winrect_pitch);

    /* restore winrect bottom */
    fb_ptr = driver_fb + (win_rect.y2 - WIN_MOVE_BORDER) * _cursor->screen_pitch +
             (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_bottom;
    display_direct_memcpy(winrect_ptr, fb_ptr, winrect_pitch, _cursor->screen_pitch,
                          WIN_MOVE_BORDER, winrect_pitch);
}

static void rtgui_winrect_save()
{
    rt_uint8_t *winrect_ptr, *fb_ptr, *driver_fb;
    int winrect_pitch, idx;
    rtgui_rect_t screen_rect, win_rect;

    driver_fb = rtgui_graphic_driver_get_framebuffer(RT_NULL);
    win_rect = _cursor->win_rect;

    rtgui_graphic_driver_get_rect(rtgui_get_graphic_device(),
                                  &screen_rect);
    rtgui_rect_intersect(&screen_rect, &win_rect);

    /* set winrect has saved */
    _cursor->win_rect_has_saved = RT_TRUE;

    /* save winrect left */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             win_rect.x1 * _cursor->byte_pp;
    winrect_ptr = _cursor->win_left;
    winrect_pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
    display_direct_memcpy(fb_ptr, winrect_ptr, _cursor->screen_pitch, winrect_pitch,
                          (win_rect.y2 - win_rect.y1), winrect_pitch);

    /* save winrect right */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             (win_rect.x2 - WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_right;
    winrect_pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
    display_direct_memcpy(fb_ptr, winrect_ptr, _cursor->screen_pitch, winrect_pitch,
                          (win_rect.y2 - win_rect.y1), winrect_pitch);

    /* save winrect top */
    fb_ptr = driver_fb + win_rect.y1 * _cursor->screen_pitch +
             (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_top;
    winrect_pitch = (win_rect.x2 - win_rect.x1 - 2 * WIN_MOVE_BORDER) * _cursor->byte_pp;
    display_direct_memcpy(fb_ptr, winrect_ptr, _cursor->screen_pitch, winrect_pitch,
                          WIN_MOVE_BORDER, winrect_pitch);

    /* save winrect bottom */
    fb_ptr = driver_fb + (win_rect.y2 - WIN_MOVE_BORDER) * _cursor->screen_pitch +
             (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
    winrect_ptr = _cursor->win_bottom;
    display_direct_memcpy(fb_ptr, winrect_ptr, _cursor->screen_pitch, winrect_pitch,
                          WIN_MOVE_BORDER, winrect_pitch);
}
#endif

void rtgui_mouse_monitor_append(rt_slist_t *head, rtgui_rect_t *rect)
{
    struct rtgui_mouse_monitor *mmonitor;

    /* check parameters */
    if (head == RT_NULL || rect == RT_NULL) return;

    /* create a mouse monitor node */
    mmonitor = (struct rtgui_mouse_monitor *) rtgui_malloc(sizeof(struct rtgui_mouse_monitor));
    if (mmonitor == RT_NULL) return; /* no memory */

    /* set mouse monitor node */
    mmonitor->rect = *rect;
    rt_slist_init(&(mmonitor->list));

    /* append to list */
    rt_slist_append(head, &(mmonitor->list));
}

void rtgui_mouse_monitor_remove(rt_slist_t *head, rtgui_rect_t *rect)
{
    rt_slist_t *node;
    struct rtgui_mouse_monitor *mmonitor;

    /* check parameters */
    if (head == RT_NULL || rect == RT_NULL) return;

    for (node = head->next; node != RT_NULL; node = node->next)
    {
        mmonitor = rt_slist_entry(node, struct rtgui_mouse_monitor, list);
        if (mmonitor->rect.x1 == rect->x1 &&
                mmonitor->rect.x2 == rect->x2 &&
                mmonitor->rect.y1 == rect->y1 &&
                mmonitor->rect.y2 == rect->y2)
        {
            /* found node */
            rt_slist_remove(head, node);
            rtgui_free(mmonitor);

            return ;
        }
    }
}

rt_bool_t rtgui_mouse_monitor_contains_point(rt_slist_t *head, int x, int y)
{
    rt_slist_t *node;

    /* check parameter */
    if (head == RT_NULL) return RT_FALSE;

    rt_slist_for_each(node, head)
    {
        struct rtgui_mouse_monitor *monitor = rt_slist_entry(node,
                                              struct rtgui_mouse_monitor, list);

        if (rtgui_rect_contains_point(&(monitor->rect), x, y) == RT_EOK)
        {
            return RT_TRUE;
        }
    }

    return RT_FALSE;
}
