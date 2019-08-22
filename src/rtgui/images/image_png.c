/*
 * File      : image_png.c
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
 * 2010-09-15     Bernard      first version
 * 2019-07-15     onelife      keep LodePNG only
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef GUIENGINE_IMAGE_PNG

#include "include/image.h"
#include "include/blit.h"
#include "lodepng/lodepng.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "IMG_PNG"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/***************************************************************************//**
 * @addtogroup LodePNG
 * @{
 ******************************************************************************/

/* Private typedef -----------------------------------------------------------*/
struct rtgui_image_png {
    rt_bool_t is_loaded;
    rtgui_filerw_t *file;
    rt_uint32_t in_size;
    LodePNGColorType colorType;
    rt_uint32_t bitDepth;
    rt_uint8_t *pixels;
    rt_uint8_t pixel_format;
    rt_uint32_t pitch;
};

/* Private define ------------------------------------------------------------*/
#define display()                   (rtgui_get_gfx_device())

/* Private function prototypes -----------------------------------------------*/
static rt_bool_t png_check(rtgui_filerw_t *file);
static rt_bool_t png_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_int32_t scale, rt_bool_t load_body);
static void png_unload(rtgui_image_t *img);
static void png_blit(rtgui_image_t *img, rtgui_dc_t *dc, rtgui_rect_t *rect);

/* Private variables ---------------------------------------------------------*/
rtgui_image_engine_t png_engine = {
    "png",
    { RT_NULL },
    png_check,
    png_load,
    png_unload,
    png_blit,
};

/* Private functions ---------------------------------------------------------*/
static rt_bool_t png_check(rtgui_filerw_t *file) {
    rt_uint8_t magic[4];
    rt_bool_t is_png = RT_FALSE;

    do {
        if (!file) break;
        if (rtgui_filerw_seek(file, 0, SEEK_SET) < 0) break;
        if (rtgui_filerw_read(file, magic, 1, 4) != 4) break;

        is_png = magic[0] == 0x89 && magic[1] == 'P' && \
                 magic[2] == 'N' && magic[3] == 'G';
    } while (0);

    LOG_D("is_png %d", is_png);
    return is_png;
}

static rt_bool_t png_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_int32_t scale, rt_bool_t load_body) {
    /* LodePNG:
       - Do decoding in one call, use a lot of memory
       - Do allocate output buffer itself 
       - Do not support scale
     */
    struct rtgui_image_png *png;
    rt_uint8_t *buf;
    rt_err_t err;
    (void)scale;

    buf = RT_NULL;
    err = RT_EOK;

    do {

        png = rtgui_malloc(sizeof(struct rtgui_image_png));
        if (!png) {
            err = -RT_ENOMEM;
            LOG_E("no mem for struct");
            break;
        }
        png->is_loaded = RT_FALSE;
        png->file = file;
        png->in_size = 0;
        png->pixels = RT_NULL;

        if (rtgui_filerw_seek(png->file, 0, SEEK_END) < 0) {
            err = -RT_EIO;
            break;
        }
        png->in_size = rtgui_filerw_tell(png->file);
        if (rtgui_filerw_seek(png->file, 0, SEEK_SET) < 0) {
            err = -RT_EIO;
            break;
        }

        switch (display()->pixel_format) {
        case RTGRAPHIC_PIXEL_FORMAT_MONO:
            png->colorType = LCT_GREY;
            png->bitDepth = 1;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB2I:
            png->colorType = LCT_GREY;
            png->bitDepth = 2;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB4I:
            png->colorType = LCT_GREY;
            png->bitDepth = 4;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB8I:
            png->colorType = LCT_GREY;
            png->bitDepth = 8;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB565:
            png->colorType = LCT_RGB;
            png->bitDepth = 8;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_RGB888:
            png->colorType = LCT_RGB;
            png->bitDepth = 8;
            break;
        case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
            png->colorType = LCT_RGBA;
            png->bitDepth = 8;
            break;
        default:
            LOG_E("bad output format");
            err = -RT_EINVAL;
            break;
        }
        if (RT_EOK != err) break;
        // LOG_D("colorType %x, bitDepth %d", png->colorType, png->bitDepth);

        /* set image info */
        img->engine = &png_engine;
        img->data = png;

        if (load_body) {
            rt_uint32_t w, h;
            rt_uint32_t ret;

            buf = rtgui_malloc(png->in_size);
            if (!buf) {
                err = -RT_ENOMEM;
                LOG_E("no mem to load (%d)", png->in_size);
                break;
            }
            if (png->in_size != (rt_uint32_t)\
                rtgui_filerw_read(png->file, buf, 1, png->in_size)) {
                err = -RT_EIO;
                break;
            }
            ret = lodepng_decode_memory(&png->pixels, &w, &h, buf, png->in_size,
                png->colorType, png->bitDepth);
            if (ret) {
                png->pixels = RT_NULL;
                err = -RT_ERROR;
                LOG_E("lodepng err %d", ret);
            }

            switch (display()->pixel_format) {
            case RTGRAPHIC_PIXEL_FORMAT_MONO:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_MONO;
                png->pitch = (w + 7) >> 3;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_RGB2I:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB2I;
                png->pitch = (w + 3) >> 2;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_RGB4I:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB4I;
                png->pitch = (w + 1) >> 1;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_RGB8I:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB8I;
                png->pitch = w;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_RGB565:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB888;
                png->pitch = w * 3;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_RGB888:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB888;
                png->pitch = w * 3;
                break;
            case RTGRAPHIC_PIXEL_FORMAT_ARGB888:
                png->pixel_format = RTGRAPHIC_PIXEL_FORMAT_ARGB888;
                png->pitch = w << 2;
                break;
            default:
                LOG_E("bad output format");
                err = -RT_EINVAL;
                break;
            }
            if (RT_EOK != err) break;

            img->w = (rt_uint16_t)w;
            img->h = (rt_uint16_t)h;
            png->is_loaded = RT_TRUE;
            LOG_D("img size %dx%d", w, h);
        } else {
            img->w = 0;
            img->h = 0;
        }
    } while (0);

    if (buf) rtgui_free(buf);
    if (RT_EOK != err) {
        if (png->pixels) {
            rtgui_free(png->pixels);
            png->pixels = RT_NULL;
        }
        rtgui_free(png);
        LOG_E("load err %d", err);
    }

    return RT_EOK == err;
}

static void png_unload(rtgui_image_t *img) {
    struct rtgui_image_png *png;

    if (!img) return;
    png = (struct rtgui_image_png *)img->data;
    if (png) {
        if (png->pixels) rtgui_free(png->pixels);
        if (png->file) rtgui_filerw_close(png->file);
        rtgui_free(png);
        LOG_D("PNG unload");
    }
}

static void png_blit(rtgui_image_t *img, rtgui_dc_t *dc, rtgui_rect_t *rect) {
    struct rtgui_image_png *png;

    if (!img || !dc || !rect || !img->data) return;

    png = (struct rtgui_image_png *)img->data;

    do {
        rt_uint16_t w, h;
        rt_uint32_t pitch;

        w = _MIN(img->w, RECT_W(*rect));
        h = _MIN(img->h, RECT_H(*rect));
        pitch = w * _BIT2BYTE(display()->bits_per_pixel);

        if (!png->is_loaded) {
            LOG_W("PNG is not loaded!");
            break;
        } else {
            rt_uint16_t y;
            rt_uint8_t *ptr;

            if (png->pixel_format != display()->pixel_format) {
                rtgui_blit_line_func2 blit_line;
                rt_uint8_t *src, *dst;

                blit_line = rtgui_get_blit_line_func(png->pixel_format,
                    display()->pixel_format);
                if (!blit_line) {
                    LOG_E("no blit func");
                    break;
                }

                /* process the image */
                src = png->pixels;
                dst = png->pixels;
                for (y = 0; y < h; y++) {
                    blit_line(dst, src, png->pitch, 0, RT_NULL);
                    src += png->pitch;
                    dst += pitch;
                }
            }

            /* output the image */
            for (y = 0; y < h; y++) {
                ptr = png->pixels + y * pitch;
                dc->engine->blit_line(dc, rect->x1, rect->x1 + w - 1,
                    rect->y1 + y, ptr);
            }
        }
    }  while (0);
}

/* Public functions ----------------------------------------------------------*/
void* lodepng_malloc(size_t size) {
    // LOG_I("png malloc %d", size);
    return rtgui_malloc(size);
}

void* lodepng_realloc(void* ptr, size_t new_size) {
    // LOG_I("png realloc %p %d", ptr, new_size);
    return rtgui_realloc(ptr, new_size);
}

void lodepng_free(void* ptr) {
    // LOG_I("png free %p", ptr);
    return rtgui_free(ptr);
}

rt_err_t rtgui_image_png_init(void) {
    /* register png */
    return rtgui_image_register_engine(&png_engine);
}

#endif /* GUIENGINE_IMAGE_PNG */
