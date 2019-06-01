/*
 * File      : image_jpg.c
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
 * 2012-01-24     onelife      add TJpgDec (Tiny JPEG Decompressor) support
 * 2019-06-01     onelife      keep TJpgDec only
 */
/* Includes ------------------------------------------------------------------*/
#include "../include/rtgui.h"

#ifdef GUIENGINE_IMAGE_JPEG

#include "tjpgd1a/tjpgd.h"

#include "include/rtgui.h"
#include "include/image.h"
#include "include/blit.h"

/***************************************************************************//**
 * @addtogroup TJpgDec
 * @{
 ******************************************************************************/

/* Private typedef -----------------------------------------------------------*/
struct rtgui_image_jpeg
{
    rtgui_filerw_t *filerw;
    rtgui_dc_t *dc;
    rt_uint16_t dst_x, dst_y;
    rt_uint16_t dst_w, dst_h;
    rt_bool_t is_loaded;
    rt_uint8_t byte_per_pixel;

    JDEC tjpgd;                     /* jpeg structure */
    void *pool;
    rt_uint8_t *pixels;
};

/* Private define ------------------------------------------------------------*/
#define TJPGD_WORKING_BUFFER_SIZE   (32 * 1024)

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static rt_bool_t rtgui_image_jpeg_check(rtgui_filerw_t *file);
static rt_bool_t rtgui_image_jpeg_load(rtgui_image_t *image, rtgui_filerw_t *file, rt_bool_t load);
static void rtgui_image_jpeg_unload(rtgui_image_t *image);
static void rtgui_image_jpeg_blit(rtgui_image_t *image,
                                  rtgui_dc_t *dc, rtgui_rect_t *dst_rect);

/* Private variables ---------------------------------------------------------*/
rtgui_image_engine_t rtgui_image_jpeg_engine =
{
    "jpeg",
    {RT_NULL},
    rtgui_image_jpeg_check,
    rtgui_image_jpeg_load,
    rtgui_image_jpeg_unload,
    rtgui_image_jpeg_blit
};

rtgui_image_engine_t rtgui_image_jpg_engine =
{
    "jpg",
    {RT_NULL},
    rtgui_image_jpeg_check,
    rtgui_image_jpeg_load,
    rtgui_image_jpeg_unload,
    rtgui_image_jpeg_blit
};

/* Private functions ---------------------------------------------------------*/
void rtgui_image_jpeg_init()
{
    /* register jpeg on image system */
    rtgui_image_register_engine(&rtgui_image_jpeg_engine);
    /* register jpg on image system */
    rtgui_image_register_engine(&rtgui_image_jpg_engine);
}

static UINT tjpgd_in_func(JDEC *jdec, BYTE *buff, UINT ndata)
{
    rtgui_filerw_t *file = *(rtgui_filerw_t **)jdec->device;

    if (buff == RT_NULL)
    {
        return rtgui_filerw_seek(file, ndata, RTGUI_FILE_SEEK_CUR);
    }

    return rtgui_filerw_read(file, (void *)buff, 1, ndata);
}

static UINT tjpgd_out_func(JDEC *jdec, void *bitmap, JRECT *rect)
{
    struct rtgui_image_jpeg *jpeg = (struct rtgui_image_jpeg *)jdec->device;
    rt_uint16_t w, h, y;
    rt_uint16_t rectWidth;               /* Width of source rectangular (bytes) */
    rt_uint8_t *src, *dst;

    /* Copy the decompressed RGB rectangular to the frame buffer */
    rectWidth = (rect->right - rect->left + 1) * jpeg->byte_per_pixel;
    src = (rt_uint8_t *)bitmap;

    if (jpeg->is_loaded == RT_TRUE)
    {
        rt_uint16_t imageWidth;          /* Width of image (bytes) */

        imageWidth = (jdec->width >> jdec->scale) * jpeg->byte_per_pixel;
        dst = jpeg->pixels + rect->top * imageWidth + rect->left * jpeg->byte_per_pixel;
        /* Left-top of destination rectangular */
        for (h = rect->top; h <= rect->bottom; h++)
        {
            rt_memcpy(dst, src, rectWidth);
            src += rectWidth;
            dst += imageWidth;           /* Next line */
        }
    }
    else
    {
        /* we decompress from top to bottom if the block is beyond the right
         * boundary, just continue to next block. However, if the block is
         * beyond the bottom boundary, we don't need to decompress the rest. */
        if (rect->left > jpeg->dst_w) return 1;
        if (rect->top  > jpeg->dst_h) return 0;

        w = _MIN(rect->right, jpeg->dst_w);
        w = w - rect->left + 1;
        h = _MIN(rect->bottom, jpeg->dst_h);
        h = h - rect->top + 1;

        for (y = 0; y < h; y++)
        {
            jpeg->dc->engine->blit_line(jpeg->dc,
                                        jpeg->dst_x + rect->left, jpeg->dst_x + rect->left + w,
                                        jpeg->dst_y + rect->top + y,
                                        src);
            src += rectWidth;
        }
    }
    return 1;                           /* Continue to decompress */
}

static rt_bool_t rtgui_image_jpeg_check(rtgui_filerw_t *file)
{
    rt_uint8_t soi[2];
    rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET);
    rtgui_filerw_read(file, &soi, 2, 1);
    rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET);

    /* check SOI==FFD8 */
    if (soi[0] == 0xff && soi[1] == 0xd8) return RT_TRUE;

    return RT_FALSE;
}

static rt_bool_t rtgui_image_jpeg_load(rtgui_image_t *image, rtgui_filerw_t *file, rt_bool_t load)
{
    rt_bool_t res = RT_FALSE;
    struct rtgui_image_jpeg *jpeg;
    JRESULT ret;
    struct rtgui_graphic_driver *hw_driver;

    do
    {
        jpeg = (struct rtgui_image_jpeg *)rtgui_malloc(sizeof(struct rtgui_image_jpeg));
        if (jpeg == RT_NULL) break;

        jpeg->filerw = file;
        jpeg->is_loaded = load;
        jpeg->pixels = RT_NULL;

        /* allocate memory pool */
        jpeg->pool = rtgui_malloc(TJPGD_WORKING_BUFFER_SIZE);
        if (jpeg->pool == RT_NULL)
        {
            rt_kprintf("TJPGD err: no mem (%d)\n", TJPGD_WORKING_BUFFER_SIZE);
            break;
        }

        if (rtgui_filerw_seek(jpeg->filerw, 0, RTGUI_FILE_SEEK_SET) == -1)
        {
            break;
        }

        ret = jd_prepare(&jpeg->tjpgd, tjpgd_in_func, jpeg->pool,
                         TJPGD_WORKING_BUFFER_SIZE, (void *)jpeg);
        if (ret != JDR_OK)
        {
            if (ret == JDR_FMT3)
            {
                rt_kprintf("TJPGD: not supported format\n");
            }
            break;
        }

        /* use RGB565 format */
        hw_driver = rtgui_graphic_driver_get_default();
        if (hw_driver->pixel_format == RTGRAPHIC_PIXEL_FORMAT_RGB565)
        {
            jpeg->tjpgd.format = 1;
            jpeg->byte_per_pixel = 2;
        }
        /* else use RGB888 format */
        else
        {
            jpeg->tjpgd.format = 0;
            jpeg->byte_per_pixel = 3;
        }

        image->w = (rt_uint16_t)jpeg->tjpgd.width;
        image->h = (rt_uint16_t)jpeg->tjpgd.height;
        /* set image private data and engine */
        image->data = jpeg;
        image->engine = &rtgui_image_jpeg_engine;

        if (jpeg->is_loaded == RT_TRUE)
        {
            rt_uint8_t *pixels = RT_NULL;

            if (jpeg->tjpgd.format == 0 && GUIENGINE_RGB888_PIXEL_BITS == 32)
                pixels = (rt_uint8_t *)rtgui_malloc(4 * image->w * image->h);

            jpeg->pixels = (rt_uint8_t *)rtgui_malloc(jpeg->byte_per_pixel * image->w * image->h);
            if (jpeg->pixels == RT_NULL)
            {
                rt_kprintf("TJPGD err: no mem to load (%d)\n",
                           jpeg->byte_per_pixel * image->w * image->h);
                break;
            }

            ret = jd_decomp(&jpeg->tjpgd, tjpgd_out_func, 0);
            if (ret != JDR_OK) break;

            rtgui_filerw_close(jpeg->filerw);
            jpeg->filerw = RT_NULL;

            if (jpeg->tjpgd.format == 0 && pixels)
            {
                rt_uint8_t *dstp = pixels, *srcp = jpeg->pixels;

                while (srcp < jpeg->pixels + jpeg->byte_per_pixel * image->w * image->h)
                {
                    *dstp++ = *(srcp + 2);
                    *dstp++ = *(srcp + 1);
                    *dstp++ = *(srcp);
                    *dstp++ = 255;
                    srcp += 3;
                }

                rtgui_free(jpeg->pixels);
                jpeg->pixels = pixels;
                jpeg->byte_per_pixel = 4;
            }
        }
        res = RT_TRUE;
    }
    while (0);

    if (jpeg && (!res || jpeg->is_loaded))
    {
        rtgui_free(jpeg->pool);
        jpeg->pool = RT_NULL;
    }

    if (!res)
    {
        if (jpeg)
            rtgui_free(jpeg->pixels);
        rtgui_free(jpeg);

        image->data   = RT_NULL;
        image->engine = RT_NULL;
    }

    /* create jpeg image successful */
    return res;
}

static void rtgui_image_jpeg_unload(rtgui_image_t *image)
{
    if (image != RT_NULL)
    {
        struct rtgui_image_jpeg *jpeg;

        jpeg = (struct rtgui_image_jpeg *) image->data;
        RT_ASSERT(jpeg != RT_NULL);

        if (jpeg->is_loaded == RT_TRUE)
        {
            rtgui_free(jpeg->pixels);
        }
        else
        {
            rtgui_free(jpeg->pool);
            rtgui_filerw_close(jpeg->filerw);
        }
        rtgui_free(jpeg);
    }
}

static void rtgui_image_jpeg_blit(rtgui_image_t *image,
                                  rtgui_dc_t *dc, rtgui_rect_t *dst_rect)
{
    rt_uint16_t y, w, h, xoff, yoff;
    struct rtgui_image_jpeg *jpeg;

    jpeg = (struct rtgui_image_jpeg *) image->data;
    RT_ASSERT(image != RT_NULL || dc != RT_NULL || dst_rect != RT_NULL || jpeg != RT_NULL);

    /* this dc is not visible */
    if (rtgui_dc_get_visible(dc) != RT_TRUE)
        return;
    jpeg->dc = dc;

    xoff = 0;
    if (dst_rect->x1 < 0)
    {
        xoff = -dst_rect->x1;
        dst_rect->x1 = 0;
    }
    yoff = 0;
    if (dst_rect->y1 < 0)
    {
        yoff = -dst_rect->y1;
        dst_rect->y1 = 0;
    }

    if (xoff >= image->w || yoff >= image->h)
        return;

    /* the minimum rect */
    w = _MIN(image->w - xoff, RECT_W (*dst_rect));
    h = _MIN(image->h - yoff, RECT_H(*dst_rect));

    if (jpeg->pixels)
    {
        if ((rtgui_dc_get_pixel_format(dc) == RTGRAPHIC_PIXEL_FORMAT_RGB888 && jpeg->tjpgd.format == 0) ||
                (rtgui_dc_get_pixel_format(dc) == RTGRAPHIC_PIXEL_FORMAT_RGB565 && jpeg->tjpgd.format == 1) ||
                (rtgui_dc_get_pixel_format(dc) == RTGRAPHIC_PIXEL_FORMAT_ARGB888 && jpeg->tjpgd.format == 0 && GUIENGINE_RGB888_PIXEL_BITS == 32))
        {
            rt_uint16_t imageWidth = image->w * jpeg->byte_per_pixel;
            rt_uint8_t *src = jpeg->pixels + yoff * imageWidth + xoff * jpeg->byte_per_pixel;

            for (y = 0; y < h; y++)
            {
                dc->engine->blit_line(dc,
                                      dst_rect->x1, dst_rect->x1 + w,
                                      dst_rect->y1 + y,
                                      src);
                src += imageWidth;
            }
        }
        /* if the format is not match, only support DC buffer */
        else if (dc->type == RTGUI_DC_BUFFER)
        {
            rtgui_blit_info_t info = { 0 };
            struct rtgui_dc_buffer *buffer;

            buffer = (struct rtgui_dc_buffer*)dc;

            info.src = jpeg->pixels + yoff * image->w * jpeg->byte_per_pixel + xoff * jpeg->byte_per_pixel;
            info.src_h = RECT_H(*dst_rect);
            info.src_w = RECT_W(*dst_rect);
            info.src_fmt = (jpeg->tjpgd.format == 0 ? RTGRAPHIC_PIXEL_FORMAT_RGB888 : RTGRAPHIC_PIXEL_FORMAT_RGB565);
            info.src_pitch = image->w * jpeg->byte_per_pixel;
            info.src_skip = info.src_pitch - info.src_w * jpeg->byte_per_pixel;

            info.dst = rtgui_dc_buffer_get_pixel(RTGUI_DC(buffer)) + dst_rect->y1 * buffer->pitch +
                       dst_rect->x1 * rtgui_color_get_bpp(buffer->pixel_format);
            info.dst_h = RECT_H(*dst_rect);
            info.dst_w = RECT_W(*dst_rect);
            info.dst_fmt = buffer->pixel_format;
            info.dst_pitch = buffer->pitch;
            info.dst_skip = info.dst_pitch - info.dst_w * rtgui_color_get_bpp(buffer->pixel_format);

            info.a = 255;

            rtgui_blit(&info);
        }
    }
}

#endif /* GUIENGINE_IMAGE_JPEG */
