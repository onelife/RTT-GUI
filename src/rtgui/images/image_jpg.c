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
#include "include/rtgui.h"

#ifdef GUIENGINE_IMAGE_JPEG

#include "include/blit.h"
#include "include/images/image.h"
#include "tjpgd/tjpgd.h"

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
    rt_bool_t is_loaded;
    rt_bool_t is_blit;
    rt_uint8_t *pixels;
    rtgui_filerw_t *file;
    void *buf;
    JDEC tjpgd;                     /* jpeg object */
    rt_uint8_t byte_PP;
    rt_uint8_t pixel_format;
    rt_uint32_t pitch;
    rtgui_dc_t *dc;
    rt_uint16_t dst_x, dst_y;
    rt_uint16_t dst_w, dst_h;
};

/* Private define ------------------------------------------------------------*/
#define JPEG_BUF_SIZE               CONFIG_JPEG_BUFFER_SIZE
#define JPEG_MAX_SCALING_FACTOR     (3)
#define JPEG_MAX_OUTPUT_WIDTH       (16)
#define display()                   (rtgui_get_graphic_device())

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static rt_bool_t jpeg_check(rtgui_filerw_t *file);
static rt_bool_t jpeg_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load_body);
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
static rt_uint16_t tjpgd_in_func(JDEC *jdec, rt_uint8_t *buff,
    rt_uint16_t ndata) {
    struct rtgui_image_jpeg *jpeg = jdec->device;

    if (!buff) {
        if (rtgui_filerw_seek(jpeg->file, ndata, RTGUI_FILE_SEEK_CUR) < 0) {
            return 0;
        } else {
            return ndata;
        }
    } else {
        return rtgui_filerw_read(jpeg->file, (void *)buff, 1, ndata);
    }
}

static rt_uint16_t tjpgd_out_func(JDEC *jdec, void *bitmap, JRECT *rect) {
    struct rtgui_image_jpeg *jpeg = jdec->device;
    rt_uint16_t w, h;
    rt_uint16_t sz;
    rt_uint8_t *src, *dst;

    /* Copy the decompressed RGB rectangular to the frame buffer */
    src = (rt_uint8_t *)bitmap;
    w = rect->right - rect->left + 1;
    sz = w * jpeg->byte_PP;

    RT_ASSERT(w <= JPEG_MAX_OUTPUT_WIDTH);

    if (jpeg->is_blit) {
        rt_uint16_t y;

        /* We decompress from top to bottom if the block is beyond the right
         * boundary, just continue to next block. However, if the block is
         * beyond the bottom boundary, we don't need to decompress the rest. */
        if (rect->left >= jpeg->dst_w) return 1;
        if (rect->top  >= jpeg->dst_h) return 0;

        w = _MIN(w, jpeg->dst_w - rect->left);
        h = _MIN(rect->bottom, jpeg->dst_h);
        h = h - rect->top + 1;


        for (y = 0; y < h; y++) {
            #if JD_FORMAT && defined(RTGUI_BIG_ENDIAN_OUTPUT)
                rt_uint16_t *_src = (rt_uint16_t *)src;
                rt_uint16_t *_dst = (rt_uint16_t *)jpeg->pixels;
                rt_uint16_t x;

                for (x = 0; x < w; x++) {
                    *_dst++ = (*_src << 8) | (*_src >> 8);
                    _src++;
                }
                jpeg->dc->engine->blit_line(jpeg->dc,
                    jpeg->dst_x + rect->left, jpeg->dst_x + rect->left + w - 1,
                    jpeg->dst_y + rect->top + y, jpeg->pixels);
            #else
                jpeg->dc->engine->blit_line(jpeg->dc,
                    jpeg->dst_x + rect->left, jpeg->dst_x + rect->left + w - 1,
                    jpeg->dst_y + rect->top + y, src);
            #endif
            src += sz;
        }
    } else {
        dst = jpeg->pixels + rect->top * jpeg->pitch + \
              rect->left * jpeg->byte_PP;
        /* Left-top of destination rectangular */
        for (h = rect->top; h <= rect->bottom; h++) {
            /* process a line */
            #if !JD_FORMAT && (GUIENGINE_RGB888_PIXEL_BITS == 32)
                rt_uint16_t p;

                for (p = 0; p < w; p++) {
                    *dst++ = *(src + 2);
                    *dst++ = *(src + 1);
                    *dst++ = *(src);
                    *dst++ = 0xff;
                    src += 3;
                }
            #else
                rt_memcpy(dst, src, sz);
                dst += jpeg->pitch;
                src += sz;
            #endif
        }
    }
    /* Continue to decompress */
    return 1;
}

static rt_bool_t jpeg_check(rtgui_filerw_t *file) {
    rt_uint8_t soi[2];
    rt_bool_t is_jpg = RT_FALSE;

    do {
        if (!file) break;
        if (rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET) < 0) break;
        if (rtgui_filerw_read(file, soi, 1, 2) != 2) break;

        /* check if SOI == FFD8 */
        if ((soi[0] != 0xff) || (soi[1] != 0xd8)) break;
        is_jpg = RT_TRUE;
    } while (0);

    LOG_D("is_jpg %d", is_jpg);
    return is_jpg;
}

static rt_bool_t jpeg_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load_body) {
    struct rtgui_image_jpeg *jpeg;
    rt_err_t err;

    err = RT_EOK;

    do {
        JRESULT ret;
        rt_uint8_t scale = 0;

        jpeg = rtgui_malloc(sizeof(struct rtgui_image_jpeg));
        if (!jpeg) {
            err = -RT_ENOMEM;
            LOG_E("no mem for struct");
            break;
        }
        jpeg->is_loaded = RT_FALSE;
        jpeg->is_blit = RT_FALSE;
        jpeg->pixels = RT_NULL;
        jpeg->file = file;

        jpeg->buf = rtgui_malloc(JPEG_BUF_SIZE);
        if (!jpeg->buf) {
            err = -RT_ENOMEM;
            LOG_E("no mem to work (%d)", JPEG_BUF_SIZE);
            break;
        }

        if (rtgui_filerw_seek(jpeg->file, 0, RTGUI_FILE_SEEK_SET) < 0) {
            err = -RT_EIO;
            break;
        }

        ret = jd_prepare(&jpeg->tjpgd, tjpgd_in_func, jpeg->buf, JPEG_BUF_SIZE,
            (void *)jpeg);
        if (JDR_OK != ret) {
            err = -RT_ERROR;
            LOG_E("jd_prepare %d", ret);
            break;
        }

        /* get scale */
        while (scale < JPEG_MAX_SCALING_FACTOR) {
            if (display()->width > (jpeg->tjpgd.width >> scale)) break;
            scale++;
        }
        if (scale >= JPEG_MAX_SCALING_FACTOR) {
            scale = JPEG_MAX_SCALING_FACTOR;
        }

        /* set JPEG info */
        #if JD_FORMAT
            /* RGR565 */
            jpeg->byte_PP = 2;
            jpeg->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        #else
            /* RGB888 */
            #if (GUIENGINE_RGB888_PIXEL_BITS == 32)
                jpeg->byte_PP = 4;
            #else
                jpeg->byte_PP = 3;
            #endif
            jpeg->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB888;
        #endif
        jpeg->pitch = (jpeg->tjpgd.width >> scale) * jpeg->byte_PP;

        /* set image info */
        img->w = (rt_uint16_t)(jpeg->tjpgd.width >> scale);
        img->h = (rt_uint16_t)(jpeg->tjpgd.height >> scale);
        img->engine = &jpeg_engine;
        img->data = jpeg;

        if (load_body) {
            jpeg->pixels = rtgui_malloc(img->w * img->h * jpeg->byte_PP);
            if (!jpeg->pixels) {
                err = -RT_ENOMEM;
                LOG_E("no mem to load (%d)", img->w * img->h * jpeg->byte_PP);
                break;
            }

            ret = jd_decomp(&jpeg->tjpgd, tjpgd_out_func, scale);
            if (JDR_OK != ret) {
                err = -RT_ERROR;
                LOG_E("jd_decomp %d", ret);
                break;
            }

            rtgui_free(jpeg->buf);
            jpeg->buf = RT_NULL;
            rtgui_filerw_close(jpeg->file);
            jpeg->file = RT_NULL;
            jpeg->is_loaded = RT_TRUE;
        } else {
            /* save for blit */
            jpeg->tjpgd.scale = scale;
        }
    } while (0);

    if (jpeg) {
        if (load_body && jpeg->buf) {
            rtgui_free(jpeg->buf);
            jpeg->buf = RT_NULL;
        }
        if (RT_EOK != err) {
            if (jpeg->pixels) {
                rtgui_free(jpeg->pixels);
                jpeg->pixels = RT_NULL;
            }
            rtgui_free(jpeg);
            LOG_E("load err %d", err);
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
        if (jpeg->file) rtgui_filerw_close(jpeg->file);
        rtgui_free(jpeg);
        LOG_D("JPG unload");
    }
}

static void jpeg_blit(rtgui_image_t *img, rtgui_dc_t *dc,
    rtgui_rect_t *dst_rect) {
    struct rtgui_image_jpeg *jpeg;

    if (!img || !dc || !dst_rect || !img->data) return;

    jpeg = (struct rtgui_image_jpeg *)img->data;
    if (jpeg->pixel_format != display()->pixel_format) {
        LOG_E("pixel_format mismatch");
        return;
    }

    do {
        rt_uint16_t w, h;

        w = _MIN(img->w, RECT_W(*dst_rect));
        h = _MIN(img->h, RECT_H(*dst_rect));

        if (!jpeg->is_loaded) {
            JRESULT ret;

            jpeg->is_blit = RT_TRUE;
            jpeg->dc = dc;
            jpeg->dst_x = dst_rect->x1;
            jpeg->dst_y = dst_rect->y1;
            jpeg->dst_w = w;
            jpeg->dst_h = h;

            #ifdef RTGUI_BIG_ENDIAN_OUTPUT
            jpeg->pixels = rtgui_malloc(JPEG_MAX_OUTPUT_WIDTH * jpeg->byte_PP);
            if (!jpeg->pixels) {
                LOG_E("no mem to load (%d)",
                    JPEG_MAX_OUTPUT_WIDTH * jpeg->byte_PP);
                break;
            }
            #endif

            ret = jd_decomp(&jpeg->tjpgd, tjpgd_out_func, jpeg->tjpgd.scale);
            if (JDR_OK != ret) {
                LOG_E("jd_decomp %d", ret);
                break;
            }
        } else { /* (!jpeg->is_loaded) */
            rt_uint16_t y;
            rt_uint8_t *ptr;

            /* output the image */
            for (y = 0; y < h; y++) {
                ptr = jpeg->pixels + (y * jpeg->pitch);
                dc->engine->blit_line(dc, dst_rect->x1, dst_rect->x1 + w + 1,
                    dst_rect->y1 + y, ptr);
            }
        }
    }  while (0);
}

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_image_jpeg_init(void) {
    rt_err_t ret;

    /* register jpeg */
    ret = rtgui_image_register_engine(&jpeg_engine);
    if (RT_EOK != ret) return ret;
    /* register jpg */
    ret = rtgui_image_register_engine(&jpg_engine);

    return ret;
}

#endif /* GUIENGINE_IMAGE_JPEG */
