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

#include "tjpgd/tjpgd.h"

#include "include/rtgui.h"
#include "include/image.h"
#include "include/blit.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "IMG_JPG"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/***************************************************************************//**
 * @addtogroup TJpgDec
 * @{
 ******************************************************************************/

/* Private typedef -----------------------------------------------------------*/
struct rtgui_image_jpeg {
    rtgui_filerw_t *filerw;
    rtgui_dc_t *dc;
    rt_uint16_t dst_x, dst_y;
    rt_uint16_t dst_w, dst_h;
    rt_bool_t is_loaded;
    rt_uint8_t byte_per_pixel;
    JDEC tjpgd;                     /* jpeg object */
    void *buf;
    rt_uint8_t *pixels;
};

/* Private define ------------------------------------------------------------*/
#define TJPGD_WORKING_BUFFER_SIZE   (1 * 1024)
#define display_fmt(dc)             (rtgui_dc_get_pixel_format(dc))

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static rt_bool_t jpeg_check(rtgui_filerw_t *file);
static rt_bool_t jpeg_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load);
static void jpeg_unload(rtgui_image_t *img);
static void jpeg_blit(rtgui_image_t *img, rtgui_dc_t *dc,
    rtgui_rect_t *dst_rect);

/* Private variables ---------------------------------------------------------*/
rtgui_image_engine_t jpeg_engine = {
    "jpeg",
    {RT_NULL},
    jpeg_check,
    jpeg_load,
    jpeg_unload,
    jpeg_blit
};

rtgui_image_engine_t jpg_engine = {
    "jpg",
    {RT_NULL},
    jpeg_check,
    jpeg_load,
    jpeg_unload,
    jpeg_blit
};

/* Private functions ---------------------------------------------------------*/
void rtgui_image_jpeg_init(void) {
    /* register jpeg on image system */
    rtgui_image_register_engine(&jpeg_engine);
    /* register jpg on image system */
    rtgui_image_register_engine(&jpg_engine);
}

static rt_uint16_t tjpgd_in_func(JDEC *jdec, rt_uint8_t *buff,
    rt_uint16_t ndata) {
    rtgui_filerw_t *file = *(rtgui_filerw_t **)jdec->device;

    if (buff == RT_NULL)
        return rtgui_filerw_seek(file, ndata, RTGUI_FILE_SEEK_CUR);
    return rtgui_filerw_read(file, (void *)buff, 1, ndata);
}

static rt_uint16_t tjpgd_out_func(JDEC *jdec, void *bitmap, JRECT *rect) {
    struct rtgui_image_jpeg *jpeg = (struct rtgui_image_jpeg *)jdec->device;
    rt_uint16_t w, h, y;
    rt_uint16_t rectWidth;          /* Width of source rectangular (bytes) */
    rt_uint8_t *src, *dst;

    /* Copy the decompressed RGB rectangular to the frame buffer */
    rectWidth = (rect->right - rect->left + 1) * jpeg->byte_per_pixel;
    src = (rt_uint8_t *)bitmap;

    if (jpeg->is_loaded) {
        rt_uint16_t imageWidth;     /* Width of image (bytes) */

        imageWidth = (jdec->width >> jdec->scale) * jpeg->byte_per_pixel;
        dst = jpeg->pixels + rect->top * imageWidth + \
              rect->left * jpeg->byte_per_pixel;
        /* Left-top of destination rectangular */
        for (h = rect->top; h <= rect->bottom; h++) {
            rt_memcpy(dst, src, rectWidth);
            src += rectWidth;
            dst += imageWidth;      /* Next line */
        }
    } else {
        /* We decompress from top to bottom if the block is beyond the right
         * boundary, just continue to next block. However, if the block is
         * beyond the bottom boundary, we don't need to decompress the rest. */
        if (rect->left > jpeg->dst_w) return 1;
        if (rect->top  > jpeg->dst_h) return 0;

        w = _MIN(rect->right, jpeg->dst_w);
        w = w - rect->left + 1;
        h = _MIN(rect->bottom, jpeg->dst_h);
        h = h - rect->top + 1;

        for (y = 0; y < h; y++) {
            jpeg->dc->engine->blit_line(jpeg->dc,
                jpeg->dst_x + rect->left, jpeg->dst_x + rect->left + w,
                jpeg->dst_y + rect->top + y,
                src);
            src += rectWidth;
        }
    }
    return 1;                       /* Continue to decompress */
}

static rt_bool_t jpeg_check(rtgui_filerw_t *file) {
    rt_uint8_t soi[2];

    rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET);
    rtgui_filerw_read(file, &soi, 2, 1);
    rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET);

    /* check SOI==FFD8 */
    if (soi[0] == 0xff && soi[1] == 0xd8) return RT_TRUE;

    return RT_FALSE;
}

//TODO(onelife): load body when blit?
//TODO(onelife): scale
static rt_bool_t jpeg_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load) {
    struct rtgui_image_jpeg *jpeg;
    JRESULT ret;
    rt_err_t err;

    err = RT_EOK;

    do {
        jpeg = rtgui_malloc(sizeof(struct rtgui_image_jpeg));
        if (!jpeg) {
            err = -RT_ENOMEM;
            break;
        }

        jpeg->filerw = file;
        jpeg->is_loaded = load;
        jpeg->buf = RT_NULL;
        jpeg->pixels = RT_NULL;

        jpeg->buf = rtgui_malloc(TJPGD_WORKING_BUFFER_SIZE);
        if (!jpeg->buf) {
            err = -RT_ENOMEM;
            LOG_E("no mem to work (%d)", TJPGD_WORKING_BUFFER_SIZE);
            break;
        }

        if (-1 == rtgui_filerw_seek(jpeg->filerw, 0, RTGUI_FILE_SEEK_SET)) {
            err = -RT_EIO;
            break;
        }

        ret = jd_prepare(&jpeg->tjpgd, tjpgd_in_func, jpeg->buf,
            TJPGD_WORKING_BUFFER_SIZE, (void *)jpeg);
        if (JDR_OK != ret) {
            if (JDR_FMT3 == ret) {
                LOG_E("bad format, fmt3");
            }
            err = -RT_ERROR;
            break;
        }

        #if JD_FORMAT
            /* RGR565 */
            jpeg->byte_per_pixel = 2;
        #else
            /* RGB888 */
            jpeg->byte_per_pixel = 3;
        #endif

        img->w = (rt_uint16_t)jpeg->tjpgd.width;
        img->h = (rt_uint16_t)jpeg->tjpgd.height;
        /* set image private data and engine */
        img->data = jpeg;
        img->engine = &jpeg_engine;

        if (load) {
            jpeg->pixels = rtgui_malloc(
                jpeg->byte_per_pixel * img->w * img->h);
            if (!jpeg->pixels) {
                err = -RT_ENOMEM;
                LOG_E("no mem to load (%d)",
                    jpeg->byte_per_pixel * img->w * img->h);
                break;
            }

            ret = jd_decomp(&jpeg->tjpgd, tjpgd_out_func, 0);
            if (JDR_OK != ret) {
                err = -RT_ERROR;
                break;
            }

            rtgui_filerw_close(jpeg->filerw);
            jpeg->filerw = RT_NULL;

            #if !JD_FORMAT && (GUIENGINE_RGB888_PIXEL_BITS == 32)
            {
                /* RGB888 */
                rt_uint8_t *dstp = pixels, *srcp = jpeg->pixels;
                rt_uint8_t *pixels = rtgui_malloc(4 * img->w * img->h);
                if (!pixels) {
                    err = -RT_ENOMEM;
                    break;
                }

                while (srcp < \
                    jpeg->pixels + jpeg->byte_per_pixel * img->w * img->h) {
                    *dstp++ = *(srcp + 2);
                    *dstp++ = *(srcp + 1);
                    *dstp++ = *(srcp);
                    *dstp++ = 255;
                    srcp += 3;
                }
                rtgui_free(jpeg->pixels);
                jpeg->pixels = pixels;
                pixels = RT_NULL;
                jpeg->byte_per_pixel = 4;
            }
            #endif
        }
    } while (0);

    if (jpeg) {
        if (jpeg->buf) {
            rtgui_free(jpeg->buf);
            jpeg->buf = RT_NULL;
        }
        if (RT_EOK != err) {
            if (jpeg->pixels) rtgui_free(jpeg->pixels);
            rtgui_free(jpeg);
            LOG_E("load err");
        }
    }

    return RT_EOK == err;
}

static void jpeg_unload(rtgui_image_t *img) {
    struct rtgui_image_jpeg *jpeg;

    if (!img) return;

    jpeg = (struct rtgui_image_jpeg *)img->data;
    if (jpeg) {
        if (jpeg->pixels) rtgui_free(jpeg->pixels);
        if (jpeg->buf) rtgui_free(jpeg->buf);
        if (jpeg->filerw) rtgui_filerw_close(jpeg->filerw);
        rtgui_free(jpeg);
        LOG_D("JPG unload");
    }
}

static void jpeg_blit(rtgui_image_t *img, rtgui_dc_t *dc,
    rtgui_rect_t *dst_rect) {
    struct rtgui_image_jpeg *jpeg;
    rt_uint16_t y, w, h, xoff, yoff;

    jpeg = (struct rtgui_image_jpeg *)img->data;
    RT_ASSERT(img != RT_NULL || dc != RT_NULL || dst_rect != RT_NULL || \
        jpeg != RT_NULL);

    /* this dc is not visible */
    if (!rtgui_dc_get_visible(dc)) return;
    if (!jpeg->pixels) return;

    jpeg->dc = dc;
    xoff = 0;
    if (dst_rect->x1 < 0) {
        xoff = -dst_rect->x1;
        dst_rect->x1 = 0;
    }
    yoff = 0;
    if (dst_rect->y1 < 0) {
        yoff = -dst_rect->y1;
        dst_rect->y1 = 0;
    }
    if ((xoff >= img->w) || (yoff >= img->h)) return;

    /* the minimum rect */
    w = _MIN(img->w - xoff, RECT_W (*dst_rect));
    h = _MIN(img->h - yoff, RECT_H(*dst_rect));

    #if JD_FORMAT
    if (RTGRAPHIC_PIXEL_FORMAT_RGB565 == display_fmt(dc)) {
    #else
    if (RTGRAPHIC_PIXEL_FORMAT_RGB888 == display_fmt(dc)) {
    #endif
        rt_uint16_t imageWidth = img->w * jpeg->byte_per_pixel;
        rt_uint8_t *src = jpeg->pixels + yoff * imageWidth + \
                          xoff * jpeg->byte_per_pixel;

        for (y = 0; y < h; y++) {
            dc->engine->blit_line(dc, dst_rect->x1, dst_rect->x1 + w,
                dst_rect->y1 + y, src);
            src += imageWidth;
        }
    } else if (RTGUI_DC_BUFFER == dc->type) {
        /* mismatch format is only supported DC buffer */
        rtgui_blit_info_t info = { 0 };
        struct rtgui_dc_buffer *buffer;

        buffer = (struct rtgui_dc_buffer*)dc;

        info.src = jpeg->pixels + yoff * img->w * jpeg->byte_per_pixel + \
                   xoff * jpeg->byte_per_pixel;
        info.src_h = RECT_H(*dst_rect);
        info.src_w = RECT_W(*dst_rect);
        #if JD_FORMAT
        info.src_fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        #else
        info.src_fmt = RTGRAPHIC_PIXEL_FORMAT_RGB888;
        #endif
        info.src_pitch = img->w * jpeg->byte_per_pixel;
        info.src_skip = info.src_pitch - info.src_w * jpeg->byte_per_pixel;

        info.dst = rtgui_dc_buffer_get_pixel(
            RTGUI_DC(buffer)) + dst_rect->y1 * buffer->pitch + \
            dst_rect->x1 * rtgui_color_get_bpp(buffer->pixel_format);
        info.dst_h = RECT_H(*dst_rect);
        info.dst_w = RECT_W(*dst_rect);
        info.dst_fmt = buffer->pixel_format;
        info.dst_pitch = buffer->pitch;
        info.dst_skip = info.dst_pitch - info.dst_w * \
                        rtgui_color_get_bpp(buffer->pixel_format);
        info.a = 255;

        rtgui_blit(&info);
    } else {
        LOG_E("mismatch fmt");
    }
}

#endif /* GUIENGINE_IMAGE_JPEG */
