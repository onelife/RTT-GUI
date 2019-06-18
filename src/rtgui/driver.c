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
 */

#include "../include/rtgui.h"
#include "../include/driver.h"
#include "../include/region.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_DRV"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */


#define rt_graphix_ops(device)      \
    ((struct rtgui_graphic_driver_ops *)(device->user_data))
#define gfx_device                  (rtgui_get_graphic_device()->device)
#define gfx_device_ops              rt_graphix_ops(gfx_device)


const struct rtgui_graphic_driver_ops *rtgui_pixel_device_get_ops(
    int pixel_format);
const struct rtgui_graphic_driver_ops *rtgui_framebuffer_get_ops(
    int pixel_format);

static rtgui_graphic_driver_t _driver;
static rtgui_graphic_driver_t *_current_driver = &_driver;
RTGUI_REFERENCE_GETTER(graphic_device, rtgui_graphic_driver_t, _current_driver);


#ifdef GUIENGIN_USING_VFRAMEBUFFER
# include "../include/dc.h"

# ifndef RTGUI_VFB_PIXEL_FMT
#  define RTGUI_VFB_PIXEL_FMT       RTGRAPHIC_PIXEL_FORMAT_RGB565
# endif

static rtgui_graphic_driver_t _vfb_driver = {0};

static void _graphic_driver_vmode_init(void) {
    if (_vfb_driver.width != _driver.width || _vfb_driver.height != _driver.height)
    {
        if (_vfb_driver.framebuffer != RT_NULL) rtgui_free((void*)_vfb_driver.framebuffer);

        _vfb_driver.device = RT_NULL;
        _vfb_driver.pixel_format = RTGUI_VFB_PIXEL_FMT;
        _vfb_driver.bits_per_pixel = rtgui_color_get_bits(RTGUI_VFB_PIXEL_FMT);
        _vfb_driver.width  = _driver.width;
        _vfb_driver.height = _driver.height;
        _vfb_driver.pitch  = _driver.width * _BIT2BYTE(_vfb_driver.bits_per_pixel);
        _vfb_driver.framebuffer = rtgui_malloc(_vfb_driver.height * _vfb_driver.pitch);
        rt_memset(_vfb_driver.framebuffer, 0, _vfb_driver.height * _vfb_driver.pitch);
        _vfb_driver.ext_ops = RT_NULL;
        _vfb_driver.ops = rtgui_framebuffer_get_ops(_vfb_driver.pixel_format);
    }
}

void rtgui_graphic_driver_vmode_enter(void)
{
    rtgui_screen_lock(RT_WAITING_FOREVER);
    _current_driver = &_vfb_driver;
}
RTM_EXPORT(rtgui_graphic_driver_vmode_enter);

void rtgui_graphic_driver_vmode_exit(void)
{
    _current_driver = &_driver;
    rtgui_screen_unlock();
}
RTM_EXPORT(rtgui_graphic_driver_vmode_exit);

rt_bool_t rtgui_graphic_driver_is_vmode(void) {
    return (_current_driver == &_vfb_driver);
}
RTM_EXPORT(rtgui_graphic_driver_is_vmode);

rtgui_dc_t*
rtgui_graphic_driver_get_rect_buffer(const rtgui_graphic_driver_t *drv,
                                     rtgui_rect_t *r)
{
    int w, h;
    struct rtgui_dc_buffer *buffer;
    rt_uint8_t *pixel, *dst;
    rtgui_rect_t src, rect;

    /* use virtual framebuffer in default */
    if (drv == RT_NULL) drv = _current_driver;

    if (r == RT_NULL)
    {
        rtgui_graphic_driver_get_rect(drv, &rect);
    }
    else
    {
        rtgui_graphic_driver_get_rect(drv, &src);
        rect = *r;
        rtgui_rect_intersect(&src, &rect);
    }

    w = RECT_W (rect);
    h = RECT_H(rect);
    if (!(w && h) || drv->framebuffer == RT_NULL)
        return RT_NULL;

    /* create buffer DC */
    buffer = (struct rtgui_dc_buffer*)rtgui_dc_buffer_create_pixformat(drv->pixel_format, w, h);
    if (buffer == RT_NULL)
        return (rtgui_dc_t*)buffer;

    /* get source pixel */
    pixel = (rt_uint8_t*)drv->framebuffer
            + rect.y1 * drv->pitch
            + rect.x1 * rtgui_color_get_bpp(drv->pixel_format);

    dst = buffer->pixel;

    while (h--)
    {
        rt_memcpy(dst, pixel, buffer->pitch);

        dst += buffer->pitch;
        pixel += drv->pitch;
    }

    return (rtgui_dc_t*)buffer;
}
RTM_EXPORT(rtgui_graphic_driver_get_rect_buffer);
#else /* GUIENGIN_USING_VFRAMEBUFFER */
rt_bool_t rtgui_graphic_driver_is_vmode(void) {
    return RT_FALSE;
}
RTM_EXPORT(rtgui_graphic_driver_is_vmode);
#endif /* GUIENGIN_USING_VFRAMEBUFFER */

void rtgui_graphic_driver_get_rect(const rtgui_graphic_driver_t *drv,
    rtgui_rect_t *rect) {
    RT_ASSERT(rect != RT_NULL);

    /* use default drv */
    if (RT_NULL == drv) drv = _current_driver;

    rect->x1 = rect->y1 = 0;
    rect->x2 = drv->width;
    rect->y2 = drv->height;
}
RTM_EXPORT(rtgui_graphic_driver_get_rect);

rt_err_t rtgui_graphic_set_device(rt_device_t dev) {
    struct rt_device_graphic_info info;
    struct rtgui_graphic_ext_ops *ext_ops;
    rt_err_t ret;

    do {
        ret = rt_device_open(dev, 0);
        if (RT_EOK != ret) break;

        /* get framebuffer address */
        ret = rt_device_control(dev, RTGRAPHIC_CTRL_GET_INFO, &info);
        if (RT_EOK != ret) break;

        /* if the first set graphic device */
        if (_driver.width == 0 || _driver.height == 0) {
            rtgui_rect_t rect;

            rtgui_get_mainwin_rect(&rect);
            if (rect.x2 == 0 || rect.y2 == 0) {
                rtgui_rect_init(&rect, 0, 0, info.width, info.height);
                /* re-set main-window */
                rtgui_set_mainwin_rect(&rect);
            }
        }

        /* initialize framebuffer drv */
        _driver.device = dev;
        _driver.pixel_format = info.pixel_format;
        _driver.bits_per_pixel = info.bits_per_pixel;
        _driver.width = info.width;
        _driver.height = info.height;
        _driver.pitch = _driver.width * _BIT2BYTE(_driver.bits_per_pixel);
        _driver.framebuffer = info.framebuffer;

        /* get graphic extension operations */
        ret = rt_device_control(dev, RTGRAPHIC_CTRL_GET_EXT, &ext_ops);
        if (RT_EOK != ret) {
            _driver.ext_ops = ext_ops;
            ret = RT_EOK;
        }

        if (info.framebuffer) {
            /* is a frame buffer device */
            _driver.ops = rtgui_framebuffer_get_ops(_driver.pixel_format);
        } else {
            /* is a pixel device */
            _driver.ops = rtgui_pixel_device_get_ops(_driver.pixel_format);
        }

        #ifdef RTGUI_USING_HW_CURSOR
            /* set default cursor image */
            rtgui_cursor_set_image(RTGUI_CURSOR_ARROW);
        #endif

        #ifdef GUIENGIN_USING_VFRAMEBUFFER
            _graphic_driver_vmode_init();
        #endif
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_graphic_set_device);

/* screen update */
void rtgui_graphic_driver_screen_update(
    const rtgui_graphic_driver_t *drv, rtgui_rect_t *rect) {
    if (drv->device)  {
        struct rt_device_rect_info rect_info;

        if (rect->x1 >= drv->width || rect->y1 >= drv->height)
            return;
        if (rect->x2 <= 0 || rect->y2 <= 0)
            return;

        rect_info.x = rect->x1 > 0 ? rect->x1 : 0;
        rect_info.y = rect->y1 > 0 ? rect->y1 : 0;

        rect_info.width = rect->x2 > drv->width ? drv->width : rect->x2;
        rect_info.height = rect->y2 > drv->height ? drv->height : rect->y2;

        rect_info.width -= rect_info.x;
        rect_info.height -= rect_info.y;

        rt_device_control(drv->device, RTGRAPHIC_CTRL_RECT_UPDATE, &rect_info);
    }
}
RTM_EXPORT(rtgui_graphic_driver_screen_update);

void rtgui_graphic_driver_set_framebuffer(void *fb)
{
    if (_current_driver)
        _current_driver->framebuffer = fb;
    else
        _driver.framebuffer = fb;
}

/* get video frame buffer */
rt_uint8_t *rtgui_graphic_driver_get_framebuffer(const rtgui_graphic_driver_t *drv)
{
    if (drv == RT_NULL) drv = _current_driver;

    return (rt_uint8_t *)drv->framebuffer;
}
RTM_EXPORT(rtgui_graphic_driver_get_framebuffer);

/*
 * FrameBuffer type drv
 */
#define GET_PIXEL(dst, x, y, type)  \
    (type *)((rt_uint8_t*)((dst)->framebuffer) + (y) * (dst)->pitch + (x) * _BIT2BYTE((dst)->bits_per_pixel))

static void _rgb565_set_pixel(rtgui_color_t *c, int x, int y)
{
    *GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint16_t) = rtgui_color_to_565(*c);
}

static void _rgb565_get_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint16_t pixel;

    pixel = *GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint16_t);

    /* get pixel from color */
    *c = rtgui_color_from_565(pixel);
}

static void _rgb565_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    int index;
    rt_uint16_t pixel;
    rt_uint16_t *pixel_ptr;

    /* get pixel from color */
    pixel = rtgui_color_to_565(*c);

    /* get pixel pointer in framebuffer */
    pixel_ptr = GET_PIXEL(rtgui_get_graphic_device(), x1, y, rt_uint16_t);

    for (index = x1; index < x2; index ++)
    {
        *pixel_ptr = pixel;
        pixel_ptr ++;
    }
}

static void _rgb565_draw_vline(rtgui_color_t *c, int x , int y1, int y2)
{
    rtgui_graphic_driver_t *drv;
    rt_uint8_t *dst;
    rt_uint16_t pixel;
    int index;

    drv = rtgui_get_graphic_device();
    pixel = rtgui_color_to_565(*c);
    dst = GET_PIXEL(drv, x, y1, rt_uint8_t);
    for (index = y1; index < y2; index ++)
    {
        *(rt_uint16_t *)dst = pixel;
        dst += drv->pitch;
    }
}

static void _rgb565p_set_pixel(rtgui_color_t *c, int x, int y)
{
    *GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint16_t) = rtgui_color_to_565p(*c);
}

static void _rgb565p_get_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint16_t pixel;

    pixel = *GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint16_t);

    /* get pixel from color */
    *c = rtgui_color_from_565p(pixel);
}

static void _rgb565p_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    int index;
    rt_uint16_t pixel;
    rt_uint16_t *pixel_ptr;

    /* get pixel from color */
    pixel = rtgui_color_to_565p(*c);

    /* get pixel pointer in framebuffer */
    pixel_ptr = GET_PIXEL(rtgui_get_graphic_device(), x1, y, rt_uint16_t);

    for (index = x1; index < x2; index ++)
    {
        *pixel_ptr = pixel;
        pixel_ptr ++;
    }
}

static void _rgb565p_draw_vline(rtgui_color_t *c, int x , int y1, int y2)
{
    rtgui_graphic_driver_t *drv;
    rt_uint8_t *dst;
    rt_uint16_t pixel;
    int index;

    drv = rtgui_get_graphic_device();
    pixel = rtgui_color_to_565p(*c);
    dst = GET_PIXEL(drv, x, y1, rt_uint8_t);
    for (index = y1; index < y2; index ++)
    {
        *(rt_uint16_t *)dst = pixel;
        dst += drv->pitch;
    }
}

static void _rgb888_set_pixel(rtgui_color_t *c, int x, int y)
{
#ifdef GUIENGINE_USING_RGB888_AS_32BIT
    *GET_PIXEL(rtgui_get_graphic_device(), x, y, rtgui_color_t) = *c;
#else
    rt_uint8_t *pixel = GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint8_t);

    *pixel = (*c >> 16) & 0xFF;
    pixel++;
    *pixel = (*c >> 8) & 0xFF;
    pixel++;
    *pixel = (*c) & 0xFF;
#endif // GUIENGINE_USING_RGB888_AS_32BIT
}

static void _rgb888_get_pixel(rtgui_color_t *c, int x, int y)
{
#ifdef GUIENGINE_USING_RGB888_AS_32BIT
    *c = ((rtgui_color_t)*GET_PIXEL(rtgui_get_graphic_device(), x, y, rtgui_color_t) & 0xFFFFFF) + 0xFF000000;
#else
    rt_uint8_t *pixel = GET_PIXEL(rtgui_get_graphic_device(), x, y, rt_uint8_t);

    *c = 0xFF000000;
    *c = (*pixel << 16);
    pixel++;
    *c = (*pixel << 8);
    pixel++;
    *c = (*pixel);
#endif // GUIENGINE_USING_RGB888_AS_32BIT 
}

static void _rgb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
#ifdef GUIENGINE_USING_RGB888_AS_32BIT
    int index;
    rtgui_color_t *pixel_ptr;

    /* get pixel pointer in framebuffer */
    pixel_ptr = GET_PIXEL(rtgui_get_graphic_device(), x1, y, rtgui_color_t);

    for (index = x1; index < x2; index++)
    {
        *pixel_ptr = *c;
        pixel_ptr++;
    }
#else
    int index;
    rt_uint8_t *pixel_ptr;

    /* get pixel pointer in framebuffer */
    pixel_ptr = GET_PIXEL(rtgui_get_graphic_device(), x1, y, rt_uint8_t);

    for (index = x1; index < x2; index++)
    {
        *pixel_ptr = (*c >> 16) & 0xFF;
        pixel_ptr++;
        *pixel_ptr = (*c >> 8) & 0xFF;
        pixel_ptr++;
        *pixel_ptr = (*c) & 0xFF;
        pixel_ptr++;
    }
#endif // GUIENGINE_USING_RGB888_AS_32BIT 
}

static void _rgb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2)
{
#ifdef GUIENGINE_USING_RGB888_AS_32BIT
    rtgui_graphic_driver_t *drv;
    rtgui_color_t *dst;
    int index;

    drv = rtgui_get_graphic_device();
    dst = GET_PIXEL(drv, x, y1, rtgui_color_t);
    for (index = y1; index < y2; index++)
    {
        *dst = *c;
        dst += drv->width;
    }
#else
    rtgui_graphic_driver_t *drv;
    rt_uint8_t *dst;
    int index;

    drv = rtgui_get_graphic_device();
    dst = GET_PIXEL(drv, x, y1, rt_uint8_t);
    for (index = y1; index < y2; index++)
    {
        *dst = (*c >> 16) & 0xFF;
        dst++;
        *dst = (*c >> 8) & 0xFF;
        dst++;
        *dst = (*c) & 0xFF;
        dst++;
        dst += drv->width;
    }
#endif // GUIENGINE_USING_RGB888_AS_32BIT 
}

static void _argb888_set_pixel(rtgui_color_t *c, int x, int y)
{
    *GET_PIXEL(rtgui_get_graphic_device(), x, y, rtgui_color_t) = *c;
}

static void _argb888_get_pixel(rtgui_color_t *c, int x, int y)
{
    *c = (rtgui_color_t)*GET_PIXEL(rtgui_get_graphic_device(), x, y, rtgui_color_t);
}

static void _argb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    int index;
    rtgui_color_t *pixel_ptr;

    /* get pixel pointer in framebuffer */
    pixel_ptr = GET_PIXEL(rtgui_get_graphic_device(), x1, y, rtgui_color_t);

    for (index = x1; index < x2; index++)
    {
        *pixel_ptr = *c;
        pixel_ptr++;
    }
}

static void _argb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2)
{
    rtgui_graphic_driver_t *drv;
    rtgui_color_t *dst;
    int index;

    drv = rtgui_get_graphic_device();
    dst = GET_PIXEL(drv, x, y1, rtgui_color_t);
    for (index = y1; index < y2; index++)
    {
        *dst = *c;
        dst += drv->width;
    }
}

/* draw raw hline */
static void framebuffer_draw_raw_hline(rt_uint8_t *pixels, int x1, int x2, int y)
{
    rtgui_graphic_driver_t *drv;
    rt_uint8_t *dst;

    drv = rtgui_get_graphic_device();
    dst = GET_PIXEL(drv, x1, y, rt_uint8_t);
    rt_memcpy(dst, pixels,
           (x2 - x1) * _BIT2BYTE(drv->bits_per_pixel));
}

const struct rtgui_graphic_driver_ops _framebuffer_rgb565_ops =
{
    _rgb565_set_pixel,
    _rgb565_get_pixel,
    _rgb565_draw_hline,
    _rgb565_draw_vline,
    framebuffer_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _framebuffer_rgb565p_ops =
{
    _rgb565p_set_pixel,
    _rgb565p_get_pixel,
    _rgb565p_draw_hline,
    _rgb565p_draw_vline,
    framebuffer_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _framebuffer_rgb888_ops =
{
    _rgb888_set_pixel,
    _rgb888_get_pixel,
    _rgb888_draw_hline,
    _rgb888_draw_vline,
    framebuffer_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _framebuffer_argb888_ops =
{
    _argb888_set_pixel,
    _argb888_get_pixel,
    _argb888_draw_hline,
    _argb888_draw_vline,
    framebuffer_draw_raw_hline,
};

#define FRAMEBUFFER (drv->framebuffer)
#define MONO_PIXEL(framebuffer, x, y) \
    ((rt_uint8_t**)(framebuffer))[y/8][x]

static void _mono_set_pixel(rtgui_color_t *c, int x, int y)
{
    rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();

    if (*c == white)
        MONO_PIXEL(FRAMEBUFFER, x, y) &= ~(1 << (y % 8));
    else
        MONO_PIXEL(FRAMEBUFFER, x, y) |= (1 << (y % 8));
}

static void _mono_get_pixel(rtgui_color_t *c, int x, int y)
{
    rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();

    if (MONO_PIXEL(FRAMEBUFFER, x, y) & (1 << (y % 8)))
        *c = black;
    else
        *c = white;
}

static void _mono_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    int index;
    rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();

    if (*c == white)
        for (index = x1; index < x2; index ++)
        {
            MONO_PIXEL(FRAMEBUFFER, index, y) &= ~(1 << (y % 8));
        }
    else
        for (index = x1; index < x2; index ++)
        {
            MONO_PIXEL(FRAMEBUFFER, index, y) |= (1 << (y % 8));
        }
}

static void _mono_draw_vline(rtgui_color_t *c, int x , int y1, int y2)
{
    rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();
    int index;

    if (*c == white)
        for (index = y1; index < y2; index ++)
        {
            MONO_PIXEL(FRAMEBUFFER, x, index) &= ~(1 << (index % 8));
        }
    else
        for (index = y1; index < y2; index ++)
        {
            MONO_PIXEL(FRAMEBUFFER, x, index) |= (1 << (index % 8));
        }
}

/* draw raw hline */
static void _mono_draw_raw_hline(rt_uint8_t *pixels, int x1, int x2, int y)
{
    rtgui_graphic_driver_t *drv = rtgui_get_graphic_device();
    int index;

    for (index = x1; index < x2; index ++)
    {
        if (pixels[index / 8] && (1 << (index % 8)))
            MONO_PIXEL(FRAMEBUFFER, index, y) |= (1 << (y % 8));
        else
            MONO_PIXEL(FRAMEBUFFER, index, y) &= ~(1 << (y % 8));
    }
}

const struct rtgui_graphic_driver_ops _framebuffer_mono_ops = {
    _mono_set_pixel,
    _mono_get_pixel,
    _mono_draw_hline,
    _mono_draw_vline,
    _mono_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops *rtgui_framebuffer_get_ops(
    int pixel_format) {
    switch (pixel_format) {
    case RTGRAPHIC_PIXEL_FORMAT_MONO:
        return &_framebuffer_mono_ops;
    case RTGRAPHIC_PIXEL_FORMAT_GRAY4:
        break;
    case RTGRAPHIC_PIXEL_FORMAT_GRAY16:
        break;
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        return &_framebuffer_rgb565_ops;
    case RTGRAPHIC_PIXEL_FORMAT_RGB565P:
        return &_framebuffer_rgb565p_ops;
    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        return &_framebuffer_rgb888_ops;
    case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
        return &_framebuffer_argb888_ops;
    default:
        RT_ASSERT(0);
        break;
    }

    return RT_NULL;
}

/*
 * Pixel type drv
 */
static void _pixel_mono_set_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint8_t pixel;

    pixel = rtgui_color_to_mono(*c);
    gfx_device_ops->set_pixel((char *)&pixel, x, y);
}

static void _pixel_rgb565p_set_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint16_t pixel;

    pixel = rtgui_color_to_565p(*c);
    gfx_device_ops->set_pixel((char *)&pixel, x, y);
}

static void _pixel_rgb565_set_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    gfx_device_ops->set_pixel(&pixel, x, y);
}

static void _pixel_rgb888_set_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint32_t pixel;

    pixel = rtgui_color_to_888(*c);
    gfx_device_ops->set_pixel((char *)&pixel, x, y);
}

static void _pixel_mono_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    gfx_device_ops->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_mono((rt_uint8_t)pixel);
}

static void _pixel_rgb565p_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    gfx_device_ops->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_565p((rt_uint16_t)pixel);
}

static void _pixel_rgb565_get_pixel(rtgui_color_t *c, int x, int y) {
    rtgui_color_t pixel;

    gfx_device_ops->get_pixel(&pixel, x, y);
    *c = rtgui_color_from_565((rt_uint16_t)pixel);
}

static void _pixel_rgb888_get_pixel(rtgui_color_t *c, int x, int y)
{
    rt_uint32_t pixel;

    gfx_device_ops->get_pixel((char *)&pixel, x, y);
    *c = rtgui_color_from_888(pixel);
}

static void _pixel_mono_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    rt_uint8_t pixel;

    pixel = rtgui_color_to_mono(*c);
    gfx_device_ops->draw_hline((char *)&pixel, x1, x2, y);
}

static void _pixel_rgb565p_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    rt_uint16_t pixel;

    pixel = rtgui_color_to_565p(*c);
    gfx_device_ops->draw_hline((char *)&pixel, x1, x2, y);
}

static void _pixel_rgb565_draw_hline(rtgui_color_t *c, int x1, int x2, int y) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    gfx_device_ops->draw_hline(&pixel, x1, x2, y);
}

static void _pixel_rgb888_draw_hline(rtgui_color_t *c, int x1, int x2, int y)
{
    rt_uint32_t pixel;

    pixel = rtgui_color_to_888(*c);
    gfx_device_ops->draw_hline((char *)&pixel, x1, x2, y);
}

static void _pixel_mono_draw_vline(rtgui_color_t *c, int x, int y1, int y2)
{
    rt_uint8_t pixel;

    pixel = rtgui_color_to_mono(*c);
    gfx_device_ops->draw_vline((char *)&pixel, x, y1, y2);
}

static void _pixel_rgb565p_draw_vline(rtgui_color_t *c, int x, int y1, int y2)
{
    rt_uint16_t pixel;

    pixel = rtgui_color_to_565p(*c);
    gfx_device_ops->draw_vline((char *)&pixel, x, y1, y2);
}

static void _pixel_rgb565_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_565(*c);
    gfx_device_ops->draw_vline(&pixel, x, y1, y2);
}

static void _pixel_rgb888_draw_vline(rtgui_color_t *c, int x, int y1, int y2) {
    rtgui_color_t pixel = (rtgui_color_t)rtgui_color_to_888(*c);
    gfx_device_ops->draw_vline(&pixel, x, y1, y2);
}

static void _pixel_draw_raw_hline(rt_uint8_t *pixels, int x1, int x2, int y) {
    if (x2 > x1)
        gfx_device_ops->draw_raw_hline(pixels, x1, x2, y);
    else
        gfx_device_ops->draw_raw_hline(pixels, x2, x2, y);
}

const struct rtgui_graphic_driver_ops _pixel_mono_ops = {
    _pixel_mono_set_pixel,
    _pixel_mono_get_pixel,
    _pixel_mono_draw_hline,
    _pixel_mono_draw_vline,
    _pixel_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _pixel_rgb565p_ops =
{
    _pixel_rgb565p_set_pixel,
    _pixel_rgb565p_get_pixel,
    _pixel_rgb565p_draw_hline,
    _pixel_rgb565p_draw_vline,
    _pixel_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _pixel_rgb565_ops =
{
    _pixel_rgb565_set_pixel,
    _pixel_rgb565_get_pixel,
    _pixel_rgb565_draw_hline,
    _pixel_rgb565_draw_vline,
    _pixel_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops _pixel_rgb888_ops =
{
    _pixel_rgb888_set_pixel,
    _pixel_rgb888_get_pixel,
    _pixel_rgb888_draw_hline,
    _pixel_rgb888_draw_vline,
    _pixel_draw_raw_hline,
};

const struct rtgui_graphic_driver_ops *rtgui_pixel_device_get_ops(int pixel_format)
{
    switch (pixel_format)
    {
    case RTGRAPHIC_PIXEL_FORMAT_MONO:
        return &_pixel_mono_ops;

    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        return &_pixel_rgb565_ops;

    case RTGRAPHIC_PIXEL_FORMAT_RGB565P:
        return &_pixel_rgb565p_ops;

    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        return &_pixel_rgb888_ops;
    }

    return RT_NULL;
}

/*
 * Hardware cursor
 */

#ifdef RTGUI_USING_HW_CURSOR
void rtgui_cursor_set_position(rt_uint16_t x, rt_uint16_t y)
{
    rt_uint32_t value;

    if (_current_driver->device != RT_NULL)
    {
        value = (x << 16 | y);
        rt_device_control(_driver.device, RT_DEVICE_CTRL_CURSOR_SET_POSITION, &value);
    }
}

void rtgui_cursor_set_image(enum rtgui_cursor_type type)
{
    rt_uint32_t value;

    if (_current_driver->device != RT_NULL)
    {
        value = type;
        rt_device_control(_driver.device, RT_DEVICE_CTRL_CURSOR_SET_TYPE, &value);
    }
};
#endif

