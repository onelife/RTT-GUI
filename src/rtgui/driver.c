/*
 * File      : drv.c
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
 * 2009-10-04     Bernard      first version
 * 2019-05-23     onelife      rename to "driver.c"
 * 2019-06-21     onelife      add touch device support
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#if defined(CONFIG_TOUCH_DEVICE_NAME) || defined(CONFIG_KEY_DEVICE_NAME)
# include "components/arduino/drv_common.h"
#endif

#ifdef RT_USING_ULOG
# define LOG_LVL                        RTGUI_LOG_LEVEL
# define LOG_TAG                        "GUI_DRV"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)         rt_kprintf(format "\n", ##args)
# define LOG_D                          LOG_E
#endif /* RT_USING_ULOG */

#ifdef GUIENGIN_USING_VFRAMEBUFFER
# ifndef RTGUI_VFB_PIXEL_FMT
#  define RTGUI_VFB_PIXEL_FMT           RTGRAPHIC_PIXEL_FORMAT_RGB565
# endif
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define graphic_ops()                   ((struct rtgui_graphic_driver_ops *)\
    ((rtgui_get_gfx_device()->device)->user_data))

/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static rtgui_gfx_driver_t _gfx_drv;
static rtgui_gfx_driver_t *_cur_drv = &_gfx_drv;
#ifdef GUIENGIN_USING_VFRAMEBUFFER
    static rtgui_gfx_driver_t _vfb_drv = {0};
#endif
#ifdef CONFIG_TOUCH_DEVICE_NAME
    static rt_bool_t _touch_done = RT_FALSE;
    static rt_device_t _touch;
    RTGUI_GETTER(touch_device, rt_device_t, _touch);
#endif
#ifdef CONFIG_KEY_DEVICE_NAME
    static rt_device_t _key;
    RTGUI_GETTER(key_device, rt_device_t, _key);
#endif

/* Private functions ---------------------------------------------------------*/
static void _draw_raw_hline(rt_uint8_t *pixels, int x1, int x2, int y) {
    if (x2 > x1)
        graphic_ops()->draw_raw_hline(pixels, x1, x2, y);
    else
        graphic_ops()->draw_raw_hline(pixels, x2, x2, y);
}

#if (CONFIG_USING_MONO)
static void _mono_set_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel = rtgui_color_to_mono(*c);
    graphic_ops()->set_pixel(&pixel, x, y);
}

static void _mono_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    graphic_ops()->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_mono(pixel);
}

static void _mono_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t pixel = rtgui_color_to_mono(*c);
    graphic_ops()->draw_hline(&pixel, x1, x2, y);
}

static void _mono_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = rtgui_color_to_mono(*c);
    graphic_ops()->draw_vline(&pixel, x, y1, y2);
}

static const struct rtgui_graphic_driver_ops _mono_ops = {
    _mono_set_pixel,
    _mono_get_pixel,
    _mono_draw_hline,
    _mono_draw_vline,
    _draw_raw_hline,
};
#endif /* CONFIG_USING_MONO */

#if (CONFIG_USING_RGB565)
static void _rgb565_set_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    graphic_ops()->set_pixel(&pixel, x, y);
}

static void _rgb565_get_pixel(rtgui_color_t *c, int x, int y) {
    graphic_ops()->get_pixel(c, x, y);
    // *c = rtgui_color_from_565((rt_uint16_t)pixel);
}

static void _rgb565_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    graphic_ops()->draw_hline(&pixel, x1, x2, y);
}

static void _rgb565_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    graphic_ops()->draw_vline(&pixel, x, y1, y2);
}

static const struct rtgui_graphic_driver_ops _rgb565_ops = {
    _rgb565_set_pixel,
    _rgb565_get_pixel,
    _rgb565_draw_hline,
    _rgb565_draw_vline,
    _draw_raw_hline,
};
#endif /* CONFIG_USING_RGB565 */

#if (CONFIG_USING_RGB565P)
static void _rgb565p_set_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565p(*c);
    graphic_ops()->set_pixel(&pixel, x, y);
}

static void _rgb565p_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    graphic_ops()->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_565p((rt_uint16_t)pixel);
}

static void _rgb565p_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565p(*c);
    graphic_ops()->draw_hline(&pixel, x1, x2, y);
}

static void _rgb565p_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565p(*c);
    graphic_ops()->draw_vline(&pixel, x, y1, y2);
}

static const struct rtgui_graphic_driver_ops _rgb565p_ops = {
    _rgb565p_set_pixel,
    _rgb565p_get_pixel,
    _rgb565p_draw_hline,
    _rgb565p_draw_vline,
    _draw_raw_hline,
};
#endif /* CONFIG_USING_RGB565P */

#if (CONFIG_USING_RGB888)
static void _rgb888_set_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_888(*c);
    graphic_ops()->set_pixel(&pixel, x, y);
}

static void _rgb888_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    graphic_ops()->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_888((rt_uint32_t)pixel);
}

static void _rgb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t pixel = rtgui_color_to_888(*c);
    graphic_ops()->draw_hline(&pixel, x1, x2, y);
}

static void _rgb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_888(*c);
    graphic_ops()->draw_vline(&pixel, x, y1, y2);
}

static const struct rtgui_graphic_driver_ops _rgb888_ops = {
    _rgb888_set_pixel,
    _rgb888_get_pixel,
    _rgb888_draw_hline,
    _rgb888_draw_vline,
    _draw_raw_hline,
};
#endif /* CONFIG_USING_RGB888 */

static const struct rtgui_graphic_driver_ops *_get_pixel_ops(rt_uint32_t fmt) {
    switch (fmt) {

    #if (CONFIG_USING_MONO)
    case RTGRAPHIC_PIXEL_FORMAT_MONO:
        return &_mono_ops;
    #endif

    #if (CONFIG_USING_RGB565)
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        return &_rgb565_ops;
    #endif

    #if (CONFIG_USING_RGB565P)
    case RTGRAPHIC_PIXEL_FORMAT_RGB565P:
        return &_rgb565p_ops;
    #endif

    #if (CONFIG_USING_RGB888)
    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        return &_rgb888_ops;
    #endif

    default:
        return RT_NULL;
    }
}


#ifdef GUIENGIN_USING_VFRAMEBUFFER

#define PIXEL(x, y)     ((rt_uint8_t **)display()->framebuffer)[y / 8][x]

#if (CONFIG_USING_MONO)
static void _frame_mono_set_pixel(rtgui_color_t *c, int x, int y) {
    if (*c == white)
        PIXEL(x, y) &= ~(1 << (y % 8));
    else
        PIXEL(x, y) |=  (1 << (y % 8));
}

static void _frame_mono_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_gfx_driver_t *drv = rtgui_get_gfx_device();

    if (PIXEL(x, y) & (1 << (y % 8)))
        *c = black;
    else
        *c = white;
}

static void _frame_mono_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    int x;

    if (*c == white) {
        for (x = x1; x < x2; x++)
            PIXEL(x, y) &= ~(1 << (y % 8));
    } else {
        for (x = x1; x < x2; x++)
            PIXEL(x, y) |= (1 << (y % 8));
    }
}

static void _frame_mono_draw_vline(rtgui_color_t *c, int x , int y1, int y2) {
    int y;

    if (*c == white) {
        for (y = y1; y < y2; y++)
            PIXEL(x, y) &= ~(1 << (index % 8));
    } else {
        for (y = y1; y < y2; y++)
            PIXEL(x, y) |= (1 << (index % 8));
    }
}

static void _frame_mono_draw_raw_hline(rt_uint8_t *pixels, int x1, int x2,
    int y) {
    int x;

    for (x = x1; x < x2; x++) {
        if (pixels[x / 8] && (1 << (x % 8)))
            PIXEL(x, y) |= (1 << (y % 8));
        else
            PIXEL(x, y) &= ~(1 << (y % 8));
    }
}

#undef PIXEL

const struct rtgui_graphic_driver_ops _frame_mono_ops = {
    _frame_mono_set_pixel,
    _frame_mono_get_pixel,
    _frame_mono_draw_hline,
    _frame_mono_draw_vline,
    _frame_mono_draw_raw_hline,
};

#endif /* CONFIG_USING_MONO */

#define PIXEL(x, y)     (rtgui_get_gfx_device()->framebuffer + \
                         rtgui_get_gfx_device()->pitch * y + \
                         _BIT2BYTE(rtgui_get_gfx_device()->bits_per_pixel) * x)

static void _frame_draw_raw_hline(rt_uint8_t *pixels, int x1, int x2, int y) {
    rt_uint8_t *ptr = (rt_uint8_t *)PIXEL(x1, y);

    rt_memcpy(ptr, pixels,
        (x2 - x1) * _BIT2BYTE(rtgui_get_gfx_device()->bits_per_pixel));
}

#if (CONFIG_USING_RGB565)
static void _frame_rgb565_set_pixel(rtgui_color_t *c, int x, int y) {
    *(rt_uint16_t *)PIXEL(x, y) = rtgui_color_to_565(*c);
}

static void _frame_rgb565_get_pixel(rtgui_color_t *c, int x, int y) {
    rt_uint16_t pixel = *(rt_uint16_t *)PIXEL(x, y);
    *c = rtgui_color_from_565(pixel);
}

static void _frame_rgb565_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rt_uint16_t pixel = rtgui_color_to_565(*c);
    rt_uint16_t *ptr = (rt_uint16_t *)PIXEL(x1, y);
    int x;

    for (x = x1; x < x2; x++, ptr++)
        *ptr = pixel;
}

static void _frame_rgb565_draw_vline(rtgui_color_t *c, int x , int y1, int y2) {
    rt_uint16_t pixel = rtgui_color_to_565(*c);
    rt_uint16_t *ptr = (rt_uint16_t *)PIXEL(x, y1);
    int y;

    for (y = y1; y < y2; y++, ptr += rtgui_get_gfx_device()->width)
        *ptr = pixel;
}

const struct rtgui_graphic_driver_ops _frame_rgb565_ops = {
    _frame_rgb565_set_pixel,
    _frame_rgb565_get_pixel,
    _frame_rgb565_draw_hline,
    _frame_rgb565_draw_vline,
    _frame_draw_raw_hline,
};
#endif /* CONFIG_USING_RGB565 */

#if (CONFIG_USING_RGB565P)
static void _frame_rgb565p_set_pixel(rtgui_color_t *c, int x, int y) {
    *(rt_uint16_t *)PIXEL(x, y) = rtgui_color_to_565p(*c);
}

static void _frame_rgb565p_get_pixel(rtgui_color_t *c, int x, int y) {
    rt_uint16_t pixel = *(rt_uint16_t *)PIXEL(x, y);
    *c = rtgui_color_from_565p(pixel);
}

static void _frame_rgb565p_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rt_uint16_t pixel = rtgui_color_to_565p(*c);
    rt_uint16_t *ptr = (rt_uint16_t *)PIXEL(x1, y);
    int x;

    for (x = x1; x < x2; x++, ptr++)
        *ptr = pixel;
}

static void _frame_rgb565p_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rt_uint16_t pixel = rtgui_color_to_565p(*c);
    rt_uint16_t *ptr = (rt_uint16_t *)PIXEL(x, y1);
    int y;

    for (y = y1; y < y2; y++, ptr += rtgui_get_gfx_device()->width)
        *ptr = pixel;
}

const struct rtgui_graphic_driver_ops _frame_rgb565p_ops = {
    _frame_rgb565p_set_pixel,
    _frame_rgb565p_get_pixel,
    _frame_rgb565p_draw_hline,
    _frame_rgb565p_draw_vline,
    _frame_draw_raw_hline,
};
#endif /* CONFIG_USING_RGB565P */

#if (CONFIG_USING_RGB888)
static void _frame_rgb888_set_pixel(rtgui_color_t *c, int x, int y) {
    #ifdef RTGUI_USING_RGB888_AS_32BIT
        *(rtgui_color_t *)PIXEL(x, y) = *c;
    #else
        *(rtgui_color_t *)PIXEL(x, y) = *c & 0x00ffffff;
    #endif
}

static void _frame_rgb888_get_pixel(rtgui_color_t *c, int x, int y) {
    #ifdef RTGUI_USING_RGB888_AS_32BIT
        *c = *(rtgui_color_t *)PIXEL(x, y);
    #else
        *c = *(rtgui_color_t *)PIXEL(x, y) & 0x00ffffff;
    #endif
}

static void _frame_rgb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t *ptr = (rtgui_color_t *)PIXEL(x1, y);
    int x;

    for (x = x1; x < x2; x++, ptr++)
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            *ptr = *c;
        #else
            *ptr = *c & 0x00ffffff;
        #endif
}

static void _frame_rgb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t *ptr = (rtgui_color_t *)PIXEL(x, y1);
    int y;

    for (y = y1; y < y2; y++, ptr += rtgui_get_gfx_device()->width)
        #ifdef RTGUI_USING_RGB888_AS_32BIT
            *ptr = *c;
        #else
            *ptr = *c & 0x00ffffff;
        #endif
}

const struct rtgui_graphic_driver_ops _frame_rgb888_ops = {
    _frame_rgb888_set_pixel,
    _frame_rgb888_get_pixel,
    _frame_rgb888_draw_hline,
    _frame_rgb888_draw_vline,
    _frame_draw_raw_hline,
};
#endif /* CONFIG_USING_RGB888 */

#if (CONFIG_USING_ARGB888)
static void _frame_argb888_set_pixel(rtgui_color_t *c, int x, int y) {
    *(rtgui_color_t *)PIXEL(x, y) = *c;
}

static void _frame_argb888_get_pixel(rtgui_color_t *c, int x, int y) {
    *(rtgui_color_t *)PIXEL(x, y) = *c;
}

static void _frame_argb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t *ptr = (rtgui_color_t *)PIXEL(x1, y);
    int x;

    for (x = x1; x < x2; x++, ptr++)
        *ptr = *c;
}

static void _frame_argb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t *ptr = (rtgui_color_t *)PIXEL(x, y1);
    int y;

    for (y = y1; y < y2; y++, ptr += rtgui_get_gfx_device()->width)
        *ptr = *c;
}

const struct rtgui_graphic_driver_ops _frame_argb888_ops = {
    _frame_argb888_set_pixel,
    _frame_argb888_get_pixel,
    _frame_argb888_draw_hline,
    _frame_argb888_draw_vline,
    _frame_draw_raw_hline,
};
#endif /* CONFIG_USING_ARGB888 */

#undef PIXEL

static const struct rtgui_graphic_driver_ops *_get_frame_ops(rt_uint32_t fmt) {
    switch (fmt) {

    #if (CONFIG_USING_MONO)
    case RTGRAPHIC_PIXEL_FORMAT_MONO:
        return &_frame_mono_ops;
    #endif

    case RTGRAPHIC_PIXEL_FORMAT_GRAY4:
    case RTGRAPHIC_PIXEL_FORMAT_GRAY16:
        return RT_NULL;

    #if (CONFIG_USING_RGB565)
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        return &_frame_rgb565_ops;
    #endif

    #if (CONFIG_USING_RGB565P)
    case RTGRAPHIC_PIXEL_FORMAT_RGB565P:
        return &_frame_rgb565p_ops;
    #endif

    #if (CONFIG_USING_RGB888)
    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        return &_frame_rgb888_ops;
    #endif

    #if (CONFIG_USING_ARGB888)
    case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
        return &_frame_argb888_ops;
    #endif

    default:
        return RT_NULL;
    }
}

static void _gfx_driver_vmode_init(void) {
    if ((_vfb_drv.width  == _gfx_drv.width) && \
        (_vfb_drv.height == _gfx_drv.height))
        return;

    if (_vfb_drv.framebuffer) rtgui_free(_vfb_drv.framebuffer);

    _vfb_drv.device = RT_NULL;
    _vfb_drv.width = _gfx_drv.width;
    _vfb_drv.height = _gfx_drv.height;
    _vfb_drv.pixel_format = RTGUI_VFB_PIXEL_FMT;
    _vfb_drv.bits_per_pixel = rtgui_color_get_bits(RTGUI_VFB_PIXEL_FMT);
    _vfb_drv.pitch = _gfx_drv.width * _BIT2BYTE(_vfb_drv.bits_per_pixel);
    _vfb_drv.framebuffer = rtgui_malloc(_vfb_drv.height * _vfb_drv.pitch);
    rt_memset(_vfb_drv.framebuffer, 0x00, _vfb_drv.height * _vfb_drv.pitch);
    _vfb_drv.ext_ops = RT_NULL;
    _vfb_drv.ops = _get_frame_ops(_vfb_drv.pixel_format);
}


void rtgui_gfx_driver_vmode_enter(void) {
    rtgui_screen_lock(RT_WAITING_FOREVER);
    _cur_drv = &_vfb_drv;
}
RTM_EXPORT(rtgui_gfx_driver_vmode_enter);

void rtgui_gfx_driver_vmode_exit(void) {
    _cur_drv = &_gfx_drv;
    rtgui_screen_unlock();
}
RTM_EXPORT(rtgui_gfx_driver_vmode_exit);

rt_bool_t rtgui_gfx_driver_is_vmode(void) {
    return (_cur_drv == &_vfb_drv);
}
RTM_EXPORT(rtgui_gfx_driver_is_vmode);

rtgui_dc_t *rtgui_gfx_driver_get_buffer(const rtgui_gfx_driver_t *drv,
    rtgui_rect_t *_rect) {
    rtgui_rect_t rect;
    rt_int16_t w, h;
    struct rtgui_dc_buffer *buf;
    rt_uint8_t *pixel, *dst;

    if (!drv)
        drv = _cur_drv;

    rtgui_gfx_get_rect(drv, &rect);
    if (_rect)
        rtgui_rect_intersect(_rect, &rect);

    w = RECT_W(rect);
    h = RECT_H(rect);
    if ((w <= 1) || (h <= 1) || !drv->framebuffer) return RT_NULL;

    /* create buffer DC */
    buf = (struct rtgui_dc_buffer *)rtgui_dc_buffer_create_pixformat(
        drv->pixel_format, w, h);
    if (!buf) return RT_NULL;

    /* get source pixel */
    pixel = (rt_uint8_t *)drv->framebuffer + \
            rect.y1 * drv->pitch + \
            rect.x1 * rtgui_color_get_bpp(drv->pixel_format);
    dst = buf->pixel;

    do {
        rt_memcpy(dst, pixel, buf->pitch);
        dst += buf->pitch;
        pixel += drv->pitch;
        h--;
    } while (h > 0);

    return (rtgui_dc_t *)buf;
}
RTM_EXPORT(rtgui_gfx_driver_get_buffer);

#endif /* GUIENGIN_USING_VFRAMEBUFFER */


/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_set_gfx_device(rt_device_t dev) {
    struct rt_device_graphic_info info;
    rt_err_t ret;

    do {
        ret = rt_device_open(dev, 0);
        if (RT_EOK != ret) break;

        /* get info */
        ret = rt_device_control(dev, RTGRAPHIC_CTRL_GET_INFO, &info);
        if (RT_EOK != ret) break;

        /* check if not init */
        if (!_gfx_drv.width || !_gfx_drv.height) {
            rtgui_rect_t rect;

            rtgui_get_mainwin_rect(&rect);
            if (!rect.x2 || !rect.y2) {
                /* reset main window rect */
                rtgui_rect_init(&rect, 0, 0, info.width, info.height);
                rtgui_set_mainwin_rect(&rect);
            }
        }

        /* init gfx drv */
        _gfx_drv.device = dev;
        _gfx_drv.width = info.width;
        _gfx_drv.height = info.height;
        _gfx_drv.pixel_format = info.pixel_format;
        _gfx_drv.bits_per_pixel = info.bits_per_pixel;
        _gfx_drv.pitch = _gfx_drv.width * _BIT2BYTE(_gfx_drv.bits_per_pixel);
        _gfx_drv.framebuffer = info.framebuffer;

        /* get extent ops */
        ret = rt_device_control(dev, RTGRAPHIC_CTRL_GET_EXT, &_gfx_drv.ext_ops);
        if (RT_EOK != ret) {
            _gfx_drv.ext_ops = RT_NULL;
            ret = RT_EOK;
        }

        /* get ops */
        if (!_gfx_drv.framebuffer) {
            _gfx_drv.ops = _get_pixel_ops(_gfx_drv.pixel_format);
        } else {
            #ifdef GUIENGIN_USING_VFRAMEBUFFER
                _gfx_drv.ops = _get_frame_ops(_gfx_drv.pixel_format);
            #else
                _gfx_drv.ops = RT_NULL;
            #endif
        }
        if (!_gfx_drv.ops) {
            LOG_E("no gfx ops");
            ret = -RT_ERROR;
            break;
        }

        #ifdef GUIENGIN_USING_VFRAMEBUFFER
            _gfx_driver_vmode_init();
        #endif

        #ifdef RTGUI_USING_HW_CURSOR
            rtgui_cursor_set_image(RTGUI_CURSOR_ARROW);
        #endif
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_set_gfx_device);

RTGUI_REFERENCE_GETTER(gfx_device, rtgui_gfx_driver_t, _cur_drv);

void rtgui_gfx_get_rect(const rtgui_gfx_driver_t *drv, rtgui_rect_t *rect) {
    RT_ASSERT(rect != RT_NULL);

    if (!drv)
        drv = _cur_drv;

    rect->x1 = rect->y1 = 0;
    rect->x2 = drv->width - 1;
    rect->y2 = drv->height - 1;
}
RTM_EXPORT(rtgui_gfx_get_rect);

void rtgui_gfx_update_screen(const rtgui_gfx_driver_t *drv,
    rtgui_rect_t *rect) {
    if (drv->device)  {
        struct rt_device_rect_info info;

        if ((rect->x1 >= drv->width) || (rect->y1 >= drv->height) || \
            (rect->x2 <= 0) || (rect->y2 <= 0))
            return;

        info.x = rect->x1 > 0 ? rect->x1 : 0;
        info.y = rect->y1 > 0 ? rect->y1 : 0;

        info.width = rect->x2 > drv->width ? drv->width : rect->x2;
        info.height = rect->y2 > drv->height ? drv->height : rect->y2;

        info.width -= info.x;
        info.height -= info.y;

        rt_device_control(drv->device, RTGRAPHIC_CTRL_RECT_UPDATE, &info);
    }
}
RTM_EXPORT(rtgui_gfx_update_screen);

void rtgui_gfx_set_framebuffer(void *fb) {
    if (_cur_drv)
        _cur_drv->framebuffer = fb;
    else
        _gfx_drv.framebuffer = fb;
}

rt_uint8_t *rtgui_gfx_get_framebuffer(const rtgui_gfx_driver_t *drv) {
    if (!drv)
        drv = _cur_drv;

    return drv->framebuffer;
}
RTM_EXPORT(rtgui_gfx_get_framebuffer);


#ifdef CONFIG_TOUCH_DEVICE_NAME
static void touch_available(void) {
    /* call by interrupt, send touch event to server */
    // TODO(onelife): add a lock to prevent interrupting e.g. sd reading?
    rt_size_t num;
    rtgui_touch_t *data;
    rtgui_evt_generic_t *evt;

    num = rt_device_read(_touch, 0, &data, 1);
    /* skip moving event otherwise eat up resources */
    if ((num > 1) || (data->type == RTGUI_TOUCH_MOTION) || \
        (data->type == RTGUI_TOUCH_NONE))
        return;

    /* get only one lift up event */
    if (num)                _touch_done = RT_FALSE;
    else if (_touch_done)   return;
    else                    _touch_done = RT_TRUE;

    /* send RTGUI_EVENT_TOUCH */
    RTGUI_CREATE_EVENT(evt, TOUCH, RT_WAITING_NO);
    if (!evt) return;
    rt_memcpy(&evt->touch.data, data, sizeof(rtgui_touch_t));
    (void)rtgui_send_request(evt, RT_WAITING_NO);
}

rt_err_t rtgui_set_touch_device(rt_device_t dev) {
    rt_err_t ret;

    do {
        ret = rt_device_open(dev, 0);
        if (RT_EOK != ret) break;
        /* set touch indicator */
        ret = rt_device_control(dev, RT_DEVICE_CTRL_SET_RX_INDICATOR,
            touch_available);
        _touch = dev;
    } while (0);

    return ret;
}
#endif /* CONFIG_TOUCH_DEVICE_NAME */

#ifdef CONFIG_KEY_DEVICE_NAME
static void key_available(void) {
    /* call by interrupt, send touch event to server */
    rt_size_t len;

    do {
        rtgui_key_t data;
        rtgui_evt_generic_t *evt;

        len = rt_device_read(_key, 0, &data, 1);
        if (!len) break;

        /* send RTGUI_EVENT_TOUCH */
        RTGUI_CREATE_EVENT(evt, KBD, RT_WAITING_NO);
        if (!evt) break;
        rt_memcpy(&evt->kbd.data, &data, sizeof(rtgui_key_t));
        (void)rtgui_send_request(evt, RT_WAITING_NO);
    } while (len > 0);
}

rt_err_t rtgui_set_key_device(rt_device_t dev) {
    rt_err_t ret;

    do {
        ret = rt_device_open(dev, 0);
        if (RT_EOK != ret) break;
        /* set touch indicator */
        ret = rt_device_control(dev, RT_DEVICE_CTRL_SET_RX_INDICATOR,
            key_available);
        _key = dev;
    } while (0);

    return ret;
}
#endif /* CONFIG_KEY_DEVICE_NAME */


#ifdef RTGUI_USING_HW_CURSOR
void rtgui_cursor_set_position(rt_uint16_t x, rt_uint16_t y) {
    rt_uint32_t value;

    if (_cur_drv->device) {
        value = (x << 16 | y);
        rt_device_control(_gfx_drv.device, RT_DEVICE_CTRL_CURSOR_SET_POSITION,
            &value);
    }
}

void rtgui_cursor_set_image(enum rtgui_cursor_type type) {
    rt_uint32_t value;

    if (_cur_drv->device) {
        value = type;
        rt_device_control(_gfx_drv.device, RT_DEVICE_CTRL_CURSOR_SET_TYPE,
            &value);
    }
}
#endif /* RTGUI_USING_HW_CURSOR */
