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
 * 2019-08-21     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#ifdef RTGUI_USING_CURSOR
#include "include/image.h"
#endif
#include "include/app/mouse.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "SRV_MOS"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
#ifdef RTGUI_USING_CURSOR
    static void _cursor_rect_restore(void);
    static void _cursor_rect_save(void);
    static void _cursor_draw(void);
#endif
#ifdef RTGUI_USING_WINMOVE
    static void rtgui_winmove_restore(void);
    static void rtgui_winmove_save(void);
    static void rtgui_winmove_show(void);
#endif

/* Private typedef -----------------------------------------------------------*/
struct rtgui_cursor {
    /* screen byte per pixel */
    rt_uint16_t byte_pp;
    /* current cursor x and y */
    rt_uint16_t cx, cy;

    #ifdef RTGUI_USING_CURSOR
        /* cursor rect */
        rtgui_rect_t rect;
        /* show cursor and show cursor count */
        rt_bool_t show_cursor;
        rt_base_t cursor_count;
        /* cursor image and saved cursor */
        rtgui_image_t *cursor_image;
        void *rect_copy;
    #endif /* RTGUI_USING_CURSOR */

    #ifdef RTGUI_USING_WINMOVE
        /* move window rect and border */
        rtgui_win_t *win;
        rt_bool_t win_moving, has_win_copy;
        rtgui_rect_t win_rect;
        rt_uint8_t *win_left, *win_right;
        rt_uint8_t *win_top, *win_bottom;
    #endif /* RTGUI_USING_WINMOVE */
};
typedef struct rtgui_cursor rtgui_cursor_t;

/* Private define ------------------------------------------------------------*/
#define WIN_MOVE_BORDER                     (4)
#define display()                           (rtgui_get_gfx_device())
#define display_pitch                       \
    (display()->width * _BIT2BYTE(display()->bits_per_pixel))

/* Private variables ---------------------------------------------------------*/
static rtgui_cursor_t *_cursor;
#ifdef RTGUI_USING_CURSOR
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
#ifdef RTGUI_USING_CURSOR

/* display the saved cursor area to screen */
static void _cursor_rect_restore(void) {
    rt_uint8_t *fb;
    rt_uint16_t h_max;

    h_max = _MIN(_cursor->cy + _cursor->cursor_image->h, display()->height);

    fb = rtgui_gfx_get_framebuffer(RT_NULL);
    if (fb) {
        rt_uint8_t *ptr;
        rt_uint16_t pitch, len, y;

        ptr = _cursor->rect_copy;
        pitch = _cursor->cursor_image->w * _cursor->byte_pp;
        len = (display()->width > (_cursor->cx + _cursor->cursor_image->w)) ? \
            _cursor->cursor_image->w : display()->width - 1 - _cursor->cx;
        len *= _cursor->byte_pp;
        fb += _cursor->cy * display_pitch + _cursor->cx * _cursor->byte_pp;

        for (y = _cursor->cy; y < h_max; y++) {
            rt_memcpy(fb, ptr, len);
            fb += display_pitch;
            ptr += pitch;
        }
    } else {
        #if 0 //TODO(onelife): rect_copy needs to be updated
        rtgui_color_t *pixel;
        rt_uint16_t w_max, x, y;

        pixel = _cursor->rect_copy;
        w_max = _MIN(_cursor->cx + _cursor->cursor_image->w, display()->width);

        for (y = _cursor->cy; y < h_max; y++)
            for (x = _cursor->cx; x < w_max; x++, pixel++)
                display()->ops->set_pixel(pixel, x, y);
        #endif
    }
}

/* save the cursor coverage area from screen */
static void _cursor_rect_save(void) {
    rt_uint8_t *fb;
    rt_uint16_t h_max;

    h_max = _MIN(_cursor->cy + _cursor->cursor_image->h, display()->height);

    fb = rtgui_gfx_get_framebuffer(RT_NULL);
    if (fb) {
        rt_uint8_t *ptr;
        rt_uint16_t pitch, len, y;

        ptr = _cursor->rect_copy;
        pitch = _cursor->cursor_image->w * _cursor->byte_pp;
        len = (display()->width > (_cursor->cx + _cursor->cursor_image->w)) ? \
            _cursor->cursor_image->w : display()->width - 1 - _cursor->cx;
        len *= _cursor->byte_pp;
        fb += _cursor->cy * display_pitch + _cursor->cx * _cursor->byte_pp;

        for (y = _cursor->cy; y < h_max; y++) {
            rt_memcpy(ptr, fb, len);
            fb += display_pitch;
            ptr += pitch;
        }
    } else {
        #if 0 //TODO(onelife): rect_copy needs to be updated
        rtgui_color_t *pixel;
        rt_uint16_t w_max, x, y;

        pixel = _cursor->rect_copy;
        w_max = _MIN(_cursor->cx + _cursor->cursor_image->w, display()->width);

        for (y = _cursor->cy; y < h_max; y++)
            for (x = _cursor->cx; x < w_max; x++, pixel++)
                display()->ops->get_pixel(pixel, x, y);
        #endif
    }
}

static void _cursor_draw(void) {
    rtgui_rect_t rect;
    rtgui_color_t *pixel;
    rt_uint16_t x, y;

    rtgui_cursor_get_rect(&rect);
    rtgui_rect_move(&rect, _cursor->cx, _cursor->cy);
    pixel = (rtgui_color_t *)_cursor->cursor_image->data;
    LOG_D("cursor @ (%d,%d)-(%d,%d)", rect.x1, rect.y1, rect.x2, rect.y2);

    /* draw */
    for (y = rect.y1; y < rect.y2; y++)
        for (x = rect.x1; x < rect.x2; x++, pixel++)
            if (0xff != RTGUI_RGB_A(*pixel))
                display()->ops->set_pixel(pixel, x, y);

    /* update rect */
    rtgui_gfx_update_screen(display(), &rect);
}

#endif /* RTGUI_USING_CURSOR */

#ifdef RTGUI_USING_WINMOVE
    #define fb_memcpy(src, dst, src_pitch, dst_pitch, height, len) { \
        rt_uint16_t y;                              \
        for (y = 0; y < height; y++) {              \
            rt_memcpy(dst, src, len);               \
            src += src_pitch;                       \
            dst += dst_pitch;                       \
        }                                           \
    }

    /* show winrect */
    static void rtgui_winmove_show(void) {
        rtgui_color_t pixel;
        rtgui_rect_t win_rect, win_rect_inner, screen_rect;
        rt_uint16_t x, y;

        pixel = black;
        win_rect = _cursor->win_rect;
        win_rect_inner = win_rect;
        rtgui_rect_inflate(&win_rect_inner, -WIN_MOVE_BORDER);
        rtgui_gfx_get_rect(display(), &screen_rect);
        rtgui_rect_intersect(&screen_rect, &win_rect);
        rtgui_rect_intersect(&screen_rect, &win_rect_inner);

        /* draw left */
        for (y = win_rect.y1; y < win_rect.y2; y ++)
            for (x = win_rect.x1; x < win_rect_inner.x1; x++)
                if ((x + y) & 0x01)
                    display()->ops->set_pixel(&pixel, x, y);

        /* draw right */
        for (y = win_rect.y1; y < win_rect.y2; y ++)
            for (x = win_rect_inner.x2; x < win_rect.x2; x++)
                if ((x + y) & 0x01)
                    display()->ops->set_pixel(&pixel, x, y);

        /* draw top border */
        for (y = win_rect.y1; y < win_rect_inner.y1; y ++)
            for (x = win_rect_inner.x1; x < win_rect_inner.x2; x++)
                if ((x + y) & 0x01)
                    display()->ops->set_pixel(&pixel, x, y);

        /* draw bottom border */
        for (y = win_rect_inner.y2; y < win_rect.y2; y ++)
            for (x = win_rect_inner.x1; x < win_rect_inner.x2; x++)
                if ((x + y) & 0x01)
                    display()->ops->set_pixel(&pixel, x, y);

        /* update rect */
        rtgui_gfx_update_screen(display(), &win_rect);
    }

    static void rtgui_winmove_restore(void) {
        rt_uint8_t *fb, *fb_ptr, *win_ptr;
        rtgui_rect_t win_rect, screen_rect;
        rt_uint16_t pitch;

        fb = rtgui_gfx_get_framebuffer(RT_NULL);
        if (!fb) return;

        win_rect = _cursor->win_rect;
        rtgui_gfx_get_rect(display(), &screen_rect);
        rtgui_rect_intersect(&screen_rect, &win_rect);

        /* restore win left */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 win_rect.x1 * _cursor->byte_pp;
        win_ptr = _cursor->win_left;
        pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
        fb_memcpy(
            win_ptr, fb_ptr,
            pitch, display_pitch,
            RECT_H(win_rect), pitch);

        /* restore winrect right */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 (win_rect.x2 - WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_right;
        pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
        fb_memcpy(
            win_ptr, fb_ptr,
            pitch, display_pitch,
            RECT_H(win_rect), pitch);

        /* restore winrect top */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_top;
        pitch = (RECT_W(win_rect) - WIN_MOVE_BORDER * 2) * _cursor->byte_pp;
        fb_memcpy(
            win_ptr, fb_ptr,
            pitch, display_pitch,
            WIN_MOVE_BORDER, pitch);

        /* restore winrect bottom */
        fb_ptr = fb + (win_rect.y2 - WIN_MOVE_BORDER) * display_pitch + \
                 (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_bottom;
        fb_memcpy(
            win_ptr, fb_ptr,
            pitch, display_pitch,
            WIN_MOVE_BORDER, pitch);
    }

    static void rtgui_winmove_save(void) {
        rt_uint8_t *fb, *fb_ptr, *win_ptr;
        rtgui_rect_t win_rect, screen_rect;
        rt_uint16_t pitch;

        fb = rtgui_gfx_get_framebuffer(RT_NULL);
        if (!fb) return;

        win_rect = _cursor->win_rect;
        rtgui_gfx_get_rect(display(), &screen_rect);
        rtgui_rect_intersect(&screen_rect, &win_rect);

        /* set winrect has saved */
        _cursor->has_win_copy = RT_TRUE;

        /* save winrect left */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 win_rect.x1 * _cursor->byte_pp;
        win_ptr = _cursor->win_left;
        pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
        fb_memcpy(
            fb_ptr, win_ptr,
            display_pitch, pitch,
            RECT_H(win_rect), pitch);

        /* save winrect right */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 (win_rect.x2 - WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_right;
        pitch = WIN_MOVE_BORDER * _cursor->byte_pp;
        fb_memcpy(
            fb_ptr, win_ptr,
            display_pitch, pitch,
            RECT_H(win_rect), pitch);

        /* save winrect top */
        fb_ptr = fb + win_rect.y1 * display_pitch + \
                 (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_top;
        pitch = (RECT_W(win_rect) - WIN_MOVE_BORDER * 2) * _cursor->byte_pp;
        fb_memcpy(
            fb_ptr, win_ptr,
            display_pitch, pitch,
            WIN_MOVE_BORDER, pitch);

        /* save winrect bottom */
        fb_ptr = fb + (win_rect.y2 - WIN_MOVE_BORDER) * display_pitch + \
                 (win_rect.x1 + WIN_MOVE_BORDER) * _cursor->byte_pp;
        win_ptr = _cursor->win_bottom;
        fb_memcpy(
            fb_ptr, win_ptr,
            display_pitch, pitch,
            WIN_MOVE_BORDER, pitch);
    }
#endif /* RTGUI_USING_WINMOVE */

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_cursor_init(void) {
    rt_err_t ret = RT_EOK;

    do {
        if (_cursor) rtgui_cursor_uninit();

        _cursor = rtgui_malloc(sizeof(rtgui_cursor_t));
        if (!_cursor) {
            ret = -RT_ENOMEM;
            break;
        }

        rt_memset(_cursor, 0x00, sizeof(rtgui_cursor_t));
        #if 0 //TODO(onelife): rect_copy needs to be updated
        if (rtgui_gfx_get_framebuffer(RT_NULL))
            _cursor->byte_pp = _BIT2BYTE(display()->bits_per_pixel);
        else
            _cursor->byte_pp = 4;   /* ARGB format */
        #else
        _cursor->byte_pp = _BIT2BYTE(display()->bits_per_pixel);
        #endif

        #ifdef RTGUI_USING_CURSOR
            /* init lock */
            rt_mutex_init(&cursor_lock, "cursor", RT_IPC_FLAG_FIFO);

            /* load image */
            _cursor->cursor_image = rtgui_image_create_from_mem(
                "xpm", (const rt_uint8_t *)cursor_xpm, sizeof(cursor_xpm),
                -1, RT_TRUE);
            if (!_cursor->cursor_image) {
                ret = -RT_ENOMEM;
                LOG_E("cursor img no mem");
                break;
            }

            /* init rect */
            _cursor->rect.x1 = _cursor->rect.y1 = 0;
            _cursor->rect.x2 = _cursor->cursor_image->w;
            _cursor->rect.y2 = _cursor->cursor_image->h;
            _cursor->show_cursor = RT_TRUE;
            _cursor->cursor_count = 0;
            _cursor->rect_copy = rtgui_malloc(_cursor->cursor_image->h * \
                _cursor->cursor_image->w * _cursor->byte_pp);
            if (!_cursor->rect_copy) {
                ret = -RT_ENOMEM;
                break;
            }
        #endif /* RTGUI_USING_CURSOR */

        #ifdef RTGUI_USING_WINMOVE
            /* init window move */
            _cursor->has_win_copy = RT_FALSE;
            _cursor->win_moving = RT_FALSE;
            _cursor->win_left = rtgui_malloc(
                display()->height * WIN_MOVE_BORDER * _cursor->byte_pp);
            _cursor->win_right = rtgui_malloc(
                display()->height * WIN_MOVE_BORDER * _cursor->byte_pp);
            _cursor->win_top = rtgui_malloc(
                display()->width * WIN_MOVE_BORDER * _cursor->byte_pp);
            _cursor->win_bottom = rtgui_malloc(
                display()->width * WIN_MOVE_BORDER * _cursor->byte_pp);
            if (!_cursor->win_left || !_cursor->win_right || \
                !_cursor->win_top  || !_cursor->win_bottom) {
                ret = -RT_ENOMEM;
                break;
            }
        #endif /* RTGUI_USING_WINMOVE */
    } while (0);

    if (RT_EOK != ret) {
        LOG_E("mouse init err");
        rtgui_cursor_uninit();
    }
    return ret;
}

void rtgui_cursor_uninit(void) {
    if (!_cursor) return;

    #ifdef RTGUI_USING_CURSOR
        rt_mutex_detach(&cursor_lock);
        if (_cursor->cursor_image)  rtgui_image_destroy(_cursor->cursor_image);
        if (_cursor->rect_copy)     rtgui_free(_cursor->rect_copy);
    #endif

    #ifdef RTGUI_USING_WINMOVE
        if (_cursor->win_left)      rtgui_free(_cursor->win_left);
        if (_cursor->win_right)     rtgui_free(_cursor->win_right);
        if (_cursor->win_top)       rtgui_free(_cursor->win_top);
        if (_cursor->win_bottom)    rtgui_free(_cursor->win_bottom);
    #endif

    rtgui_free(_cursor);
    _cursor = RT_NULL;
}

void rtgui_cursor_move(rt_uint16_t x, rt_uint16_t y) {
    #ifdef RTGUI_USING_CURSOR
        rt_mutex_take(&cursor_lock, RT_WAITING_FOREVER);
    #endif

    if ((x != _cursor->cx) || (y != _cursor->cy)) {
        #ifdef RTGUI_USING_WINMOVE
            if (_cursor->win_moving) {
                if (_cursor->has_win_copy)
                    rtgui_winmove_restore();

                #ifdef RTGUI_USING_CURSOR
                    rtgui_cursor_hide();
                #endif
                /* move win */
                rtgui_rect_move(
                    &(_cursor->win_rect), x - _cursor->cx, y - _cursor->cy);
                rtgui_winmove_save();
                /* move current cursor */
                _cursor->cx = x;
                _cursor->cy = y;
                /* show cursor */
                #ifdef RTGUI_USING_CURSOR
                    rtgui_cursor_show();
                #endif
                /* show win */
                rtgui_winmove_show();
            } else
        #endif /* RTGUI_USING_WINMOVE */
        {
            #ifdef RTGUI_USING_CURSOR
                rtgui_cursor_hide();
            #endif
            _cursor->cx = x;
            _cursor->cy = y;
            #ifdef RTGUI_USING_CURSOR
                rtgui_cursor_show();
            #endif
        }

        #ifdef RTGUI_USING_HW_CURSOR
            rtgui_cursor_set_position(_cursor->cx, _cursor->cy);
        #endif
    }

    #ifdef RTGUI_USING_CURSOR
        rt_mutex_release(&cursor_lock);
    #endif
}

void rtgui_cursor_set(rt_uint16_t x, rt_uint16_t y) {
    _cursor->cx = x;
    _cursor->cy = y;

    #ifdef RTGUI_USING_HW_CURSOR
        rtgui_cursor_set_position(_cursor->cx, _cursor->cy);
    #endif
}

void rtgui_monitor_append(rt_slist_t *list, rtgui_rect_t *rect) {
    if (!list || !rect) return;

    do {
        rtgui_monitor_t *mo;

        /* create */
        mo = rtgui_malloc(sizeof(rtgui_monitor_t));
        if (!mo) {
            LOG_E("no mem");
            break;
        }

        /* set */
        mo->rect = *rect;
        rt_slist_init(&(mo->list));

        /* append to list */
        rt_slist_append(list, &(mo->list));
    } while (0);
}

void rtgui_monitor_remove(rt_slist_t *list, rtgui_rect_t *rect) {
    if (!list || !rect) return;

    do {
        rt_slist_t *node;
        rtgui_monitor_t *mo;

        rt_slist_for_each(node, list) {
            mo = rt_slist_entry(node, rtgui_monitor_t, list);
            if ((mo->rect.x1 == rect->x1) && (mo->rect.x2 == rect->x2) && \
                (mo->rect.y1 == rect->y1) && (mo->rect.y2 == rect->y2)) {
                /* found node */
                rt_slist_remove(list, node);
                rtgui_free(mo);
                break;
            }
        }
    } while (0);
}

rt_bool_t rtgui_monitor_contain_point(rt_slist_t *list, int x, int y) {
    rt_bool_t ret = RT_FALSE;

    do {
        rt_slist_t *node;
        rtgui_monitor_t *mo;

        if (!list) break;

        rt_slist_for_each(node, list) {
            mo = rt_slist_entry(node, rtgui_monitor_t, list);
            if (rtgui_rect_contains_point(&(mo->rect), x, y)) {
                ret = RT_TRUE;
                break;
            }
        }
    } while (0);

    return ret;
}

#ifdef RTGUI_USING_CURSOR
    void rtgui_cursor_enable(rt_bool_t enable) {
        _cursor->show_cursor = enable;
    }

    void rtgui_cursor_set_image(rtgui_image_t *img) {
        // TODO
        (void)img;
    }

    void rtgui_cursor_get_rect(rtgui_rect_t *rect) {
        if (!rect) return;
        *rect = _cursor->rect;
    }

    void rtgui_cursor_show(void) {
        if (!_cursor->show_cursor) return;

        _cursor->cursor_count++;
        LOG_D("->cursor cnt %d", _cursor->cursor_count);

        if (1 == _cursor->cursor_count) {
            _cursor_rect_save();
            _cursor_draw();
        }
    }

    void rtgui_cursor_hide(void) {
        if (!_cursor->show_cursor) return;

        if (1 == _cursor->cursor_count)
            _cursor_rect_restore();

        _cursor->cursor_count--;
        LOG_D("<-cursor cnt %d", _cursor->cursor_count);
    }

    rt_bool_t rtgui_mouse_is_intersect(rtgui_rect_t *rect) {
        return rtgui_rect_is_intersect(&(_cursor->rect), rect);
    }
#endif /* RTGUI_USING_CURSOR */

#ifdef RTGUI_USING_WINMOVE
    void rtgui_winmove_start(rtgui_win_t *win) {
        _cursor->win_moving = RT_TRUE;
        _cursor->win_rect = (win->_title) ? \
            TO_WIDGET(win->_title)->extent : TO_WIDGET(win)->extent;
        _cursor->win = win;
    }

    rt_bool_t rtgui_winmove_done(rtgui_rect_t *rect, rtgui_win_t **win) {
        rt_bool_t ret = RT_FALSE;

        if (_cursor->has_win_copy) {
            rtgui_winmove_restore();
            ret = RT_TRUE;
        }
        _cursor->has_win_copy = RT_FALSE;
        _cursor->win_moving = RT_FALSE;

        if (rect)
            *rect = _cursor->win_rect;
        if (win)
            *win = _cursor->win;

        return ret;
    }

    rt_bool_t rtgui_winmove_is_moving(void) {
        return _cursor->win_moving;
    }
#endif /* RTGUI_USING_WINMOVE */
