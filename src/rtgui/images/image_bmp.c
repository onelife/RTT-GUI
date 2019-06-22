/*
 * File      : image_bmp.c
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
 * 2012-01-24     onelife      Reimplement to improve efficiency and add
 *  features. The new decoder uses configurable fixed size working buf and
 *  provides scaledown function.
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef GUIENGINE_IMAGE_BMP

#include "include/blit.h"
#include "include/images/image.h"
#include "include/images/image_bmp.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "IMG_BMP"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
static rt_bool_t bmp_check(rtgui_filerw_t *file);
static rt_bool_t bmp_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load);
static void bmp_unload(rtgui_image_t *img);
static void bmp_blit(rtgui_image_t *img, rtgui_dc_t *dc, rtgui_rect_t *rect);

/* Private typedef -----------------------------------------------------------*/
typedef struct rtgui_image_bmp {
    rt_bool_t is_loaded;
    rt_uint8_t *pixels;
    rtgui_filerw_t *file;
    rt_uint32_t pixel_offset;
    rt_uint32_t w, h;
    rt_uint8_t scale;
    rt_uint8_t bit_per_pixel;
    rt_uint8_t pixel_format;
    rt_uint32_t pitch;
    rt_uint8_t pad;
} rtgui_image_bmp_t;

/* Private define ------------------------------------------------------------*/
/* Compression methods */
#ifndef BI_RGB
# define BI_RGB                     (0)
# define BI_RLE8                    (1)
# define BI_RLE4                    (2)
# define BI_BITFIELDS               (3)
#endif
/* In multiple of 12 and bigger than 48 */
#define BMP_WORKING_BUFFER_SIZE     (384)
#define BMP_MAX_SCALING_FACTOR      (10)
#define display()                   (rtgui_get_graphic_device())

/* Private variables ---------------------------------------------------------*/
rtgui_image_engine_t bmp_engine = {
    "bmp",
    { RT_NULL },
    bmp_check,
    bmp_load,
    bmp_unload,
    bmp_blit,
};

/* Private functions ---------------------------------------------------------*/
static rt_bool_t bmp_check(rtgui_filerw_t *file) {
    rt_uint8_t buf[2];
    rt_bool_t is_bmp = RT_FALSE;

    do {
        if (!file) break;

        /* Prepare to decode */
        if (rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET) < 0) break;
        if (rtgui_filerw_read(file, buf, 1, 2) != 2) break;

        /* Read file type */
        if ((buf[0] != 'B') || (buf[1] != 'M')) break;
        is_bmp = RT_TRUE;
    } while (0);

    LOG_D("is_bmp %d", is_bmp);
    return is_bmp;
}

static rtgui_image_palette_t *bmp_load_palette(rtgui_filerw_t *file,
    rt_uint32_t ncolors, rt_bool_t loadAlpha) {
    rtgui_image_palette_t *palette;
    rt_err_t err;

    err = RT_EOK;

    do {
        rt_uint32_t i;
        rt_uint8_t temp[4];

        palette = rtgui_image_palette_create(ncolors);
        if (!palette) {
            err = -RT_ENOMEM;
            break;
        }
        palette->ncolors = ncolors;
        for (i = 0; i < ncolors; i++) {
            if (4 != rtgui_filerw_read(file, temp, 1, 4)) {
                err = -RT_EIO;
                break;
            }
            if (loadAlpha) {
                palette->colors[i] = \
                    RTGUI_ARGB(temp[3], temp[2], temp[1], temp[0]);
            } else {
                palette->colors[i] = RTGUI_RGB(temp[2], temp[1], temp[0]);
            }
        }
    } while (0);

    if (RT_EOK != err) {
        if (palette) {
            rtgui_free(palette);
            palette = RT_NULL;
        }
        LOG_E("load palette err [%d]", err);
    }

    LOG_D("load palette [%d]", palette->ncolors);
    return palette;
}

static rt_bool_t bmp_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load_body) {
    rtgui_image_bmp_t *bmp;
    rt_uint8_t *lineBuf;
    rt_err_t err;

    do {
        rt_uint8_t temp[32];
        rt_uint32_t ncolors = 0;
        rt_uint8_t scale = 0;
        rt_uint32_t headerSize;
        rt_bool_t loadAlpha = RT_FALSE;   /* in palette */
        lineBuf = RT_NULL;

        bmp = rtgui_malloc(sizeof(rtgui_image_bmp_t));
        if (!bmp) {
            err = -RT_ENOMEM;
            LOG_E("no mem for struct");
            break;
        }
        bmp->is_loaded = RT_FALSE;
        bmp->pixels = RT_NULL;
        bmp->file = file;

        /* read header */
        err = -RT_EIO;
        if (rtgui_filerw_seek(file, 0, RTGUI_FILE_SEEK_SET) < 0) break;
        /* file header (14-byte) + 4-byte of DIB header */
        if (rtgui_filerw_read(file, temp, 1, 18) != 18) break;
        /* check format */
        err = -RT_ERROR;
        if ((temp[0] != 'B') || (temp[1] != 'M')) break;
        /* get pixel array offset */
        bmp->pixel_offset = *(rt_uint32_t *)&temp[10];
        /* get BMP header size */
        headerSize = *(rt_uint32_t *)&temp[14];

        err = -RT_EIO;
        if (12 == headerSize) {
            /* BITMAPCOREHEADER */
            if (8 != rtgui_filerw_read(file, temp, 1, 8)) break;
            /* get image size */
            bmp->w = (rt_uint32_t)*(rt_uint16_t *)&temp[0];
            bmp->h = (rt_uint32_t)*(rt_uint16_t *)&temp[2];
            /* get bits per pixel */
            bmp->bit_per_pixel = (rt_uint8_t)*(rt_uint16_t *)&temp[6];
            LOG_D("bitPP %d", bmp->bit_per_pixel);
        } else {
            /* BITMAPINFOHEADER and later */
            if (32 != rtgui_filerw_read(file, temp, 1, 32)) break;
            /* get image size */
            bmp->w = *(rt_uint32_t *)&temp[0];
            bmp->h = *(rt_uint32_t *)&temp[4];
            /* get bits per pixel */
            bmp->bit_per_pixel = (rt_uint8_t)*(rt_uint16_t *)&temp[10];
            LOG_D("bitPP %d", bmp->bit_per_pixel);
            if (bmp->bit_per_pixel > 32) {
                err = -RT_ERROR;
                LOG_E("bad BMP bitPP %d",  bmp->bit_per_pixel);
                break;
            }
            /* The 16bpp and 32bpp images are always stored uncompressed */
            if ((16 != bmp->bit_per_pixel) && (32 != bmp->bit_per_pixel)) {
                /* check compression */
                rt_uint32_t comp = *(rt_uint32_t *)&temp[12];
                if (comp != BI_RGB) {
                    err = -RT_ERROR;
                    LOG_E("bad BMP comp %d", comp);
                    break;
                }
                LOG_D("comp %d", comp);
            }
            /* get number of colors */
            ncolors = *(rt_uint32_t *)&temp[28];
        }

        /* load palette */
        if (!ncolors) {
            ncolors = 1 << bmp->bit_per_pixel;
        }
        if (bmp->bit_per_pixel <= 8) {
            if (rtgui_filerw_seek(
                file, 14 + headerSize, RTGUI_FILE_SEEK_SET) < 0) {
                break;
            }
            img->palette = bmp_load_palette(file, ncolors, loadAlpha);
            if (!img->palette) break;
        }

        /* get scale */
        while (scale < BMP_MAX_SCALING_FACTOR) {
            if (display()->width > (bmp->w >> scale)) break;
            scale++;
        }
        if (scale >= BMP_MAX_SCALING_FACTOR) {
            scale = BMP_MAX_SCALING_FACTOR;
        }

        /* set bmp info */
        bmp->scale = scale;
        if (1 == bmp->bit_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_MONO;
            bmp->pitch = (bmp->w + 7) >> 3;
        } else if (2 == bmp->bit_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB2I;
            bmp->pitch = (bmp->w + 3) >> 2;
        } else if (4 == bmp->bit_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB4I;
            bmp->pitch = (bmp->w + 1) >> 1;
        } else if (8 == bmp->bit_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB8I;
            bmp->pitch = bmp->w;
        } else if (16 == bmp->bit_per_pixel) {
            /* assume RGB565 */
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
            bmp->pitch = bmp->w << 1;
        } else if (24 == bmp->bit_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB888;
            bmp->pitch = bmp->w * _BIT2BYTE(bmp->bit_per_pixel);
        } else {
            err = -RT_ERROR;
            LOG_E("bad BMP format");
            break;
        }
        bmp->pad = (bmp->pitch % 4) ? (4 - (bmp->pitch % 4)) : 0;
        LOG_I("scale %d, pitch %d", bmp->scale, bmp->pitch);

        /* set image info */
        img->w = (rt_uint16_t)(bmp->w >> scale);
        img->h = (rt_uint16_t)(bmp->h >> scale);
        img->engine = &bmp_engine;
        img->data = bmp;

        err = RT_EOK;
        if (load_body) {
            rtgui_blit_line_func2 blit_line;
            rt_uint16_t y;
            rt_uint8_t *dst;

            /* prepare to decode */
            blit_line = rtgui_get_blit_line_func(
                bmp->pixel_format, display()->pixel_format);
            if (!blit_line) {
                err = -RT_ERROR;
                LOG_E("no blit func");
                break;
            }
            lineBuf = rtgui_malloc(bmp->pitch);
            if (!lineBuf) {
                err = -RT_ENOMEM;
                LOG_E("no mem to read");
                break;
            }
            bmp->pixels = rtgui_malloc(img->h * display()->pitch);
            if (!bmp->pixels) {
                err = -RT_ENOMEM;
                LOG_E("no mem to load");
                break;
            }
            if (rtgui_filerw_seek(
                file, bmp->pixel_offset, RTGUI_FILE_SEEK_SET) < 0) {
                break;
            }

            /* start to decode (the image is upside down) */
            for (y = 0; y < img->h; y++) {
                dst = bmp->pixels + (img->h - y - 1) * display()->pitch;
                /* read a line */
                if (bmp->pitch != (rt_uint32_t)rtgui_filerw_read(
                    file, lineBuf, 1, bmp->pitch)) {
                    LOG_E("read data failed");
                    err = -RT_EIO;
                    break;
                }

                /* process the line */
                blit_line(dst, lineBuf, bmp->pitch, bmp->scale, img->palette);

                /* skip padding bytes  */
                if (bmp->pad) {
                    if (rtgui_filerw_seek(
                        file, bmp->pad, RTGUI_FILE_SEEK_CUR) < 0) {
                        err = -RT_EIO;
                        break;
                    }
                }

                /* scale y */
                if (bmp->scale) {
                    if (rtgui_filerw_seek(file,
                        (bmp->pitch + bmp->pad) * ((1 << bmp->scale) - 1),
                        RTGUI_FILE_SEEK_CUR) < 0) {
                        err = -RT_EIO;
                        break;
                    }
                }
            } /* for (y = 0; y < img->, y++) */
            if (RT_EOK != err) break;

            /* Close file */
            rtgui_filerw_close(bmp->file);
            bmp->file = RT_NULL;
            bmp->is_loaded = RT_TRUE;
        }
    } while (0);

    /* release memory */
    if (lineBuf) rtgui_free(lineBuf);
    if ((RT_EOK != err) && bmp) {
        if (img->palette) rtgui_free(img->palette);
        if (bmp->pixels) rtgui_free(bmp->pixels);
        rtgui_free(bmp);
        LOG_E("load err");
    }

    return RT_EOK == err;
}

static void bmp_unload(rtgui_image_t *img) {
    rtgui_image_bmp_t *bmp;

    if (img) {
        bmp = (rtgui_image_bmp_t *)img->data;
        /* Release memory */
        if (bmp->pixels) rtgui_free(bmp->pixels);
        if (bmp->file) rtgui_filerw_close(bmp->file);
        rtgui_free(bmp);
        LOG_D("BMP unload");
    }
}

static void bmp_blit(rtgui_image_t *img, rtgui_dc_t *dc,
    rtgui_rect_t *dst_rect) {
    rtgui_image_bmp_t *bmp;
    rt_uint8_t *lineBuf, *lineBuf2;
    rt_err_t err;

    if (!img || !dc || !dst_rect || !img->data) return;

    bmp = (rtgui_image_bmp_t *)img->data;
    lineBuf = lineBuf2 = RT_NULL;
    err = RT_EOK;

    do {
        rt_uint16_t w, h;

        /* the minimum rect */
        w = _MIN(img->w, RECT_W(*dst_rect));
        h = _MIN(img->h, RECT_H(*dst_rect));
        LOG_D("bmp_blit w %d, h %d, load %d", w, h, bmp->is_loaded);

        if (!bmp->is_loaded) {
            rtgui_blit_line_func2 blit_line;
            rt_uint16_t y;

            /* prepare to decode */
            blit_line = rtgui_get_blit_line_func(
                bmp->pixel_format, display()->pixel_format);
            if (!blit_line) {
                err = -RT_ERROR;
                LOG_E("no blit func");
                break;
            }
            lineBuf = rtgui_malloc(bmp->pitch);
            if (!lineBuf) {
                LOG_E("no mem to read");
                break;
            }
            lineBuf2 = rtgui_malloc(display()->pitch);
            if (!lineBuf2) {
                err = -RT_ENOMEM;
                LOG_E("no mem to decode");
                break;
            }

            if (rtgui_filerw_seek(
                bmp->file, bmp->pixel_offset, RTGUI_FILE_SEEK_SET) < 0) {
                err = -RT_EIO;
                break;
            }
            /* Due to the image is upside down, we may start drawing from the
             * middle if the image is higher than the dst_rect in y-axis. */
            if (img->h > RECT_H(*dst_rect)) {
                rt_uint16_t delta_h = img->h - RECT_H(*dst_rect);

                if (rtgui_filerw_seek(bmp->file,
                    delta_h * (bmp->pitch + bmp->pad) * (1 << bmp->scale),
                    RTGUI_FILE_SEEK_CUR) < 0) {
                    err = -RT_EIO;
                    break;
                }
            }

            /* start to decode (the image is upside down) */
            for (y = 0; y < h; y++) {
                /* read a line */
                if (bmp->pitch != (rt_uint32_t)rtgui_filerw_read(
                    bmp->file, lineBuf, 1, bmp->pitch)) {
                    LOG_E("read data failed");
                    err = -RT_EIO;
                    break;
                }

                /* process the line */
                blit_line(lineBuf2, lineBuf, bmp->pitch, bmp->scale,
                    img->palette);

                /* output the line */
                dc->engine->blit_line(dc, dst_rect->x1, dst_rect->x1 + w - 1,
                    dst_rect->y1 + (h - y + 1), lineBuf2);

                /* skip padding bytes  */
                if (bmp->pad) {
                    if (rtgui_filerw_seek(
                        bmp->file, bmp->pad, RTGUI_FILE_SEEK_CUR) < 0) {
                        err = -RT_EIO;
                        break;
                    }
                }

                /* scale y */
                if (bmp->scale) {
                    if (rtgui_filerw_seek(bmp->file,
                        (bmp->pitch + bmp->pad) * ((1 << bmp->scale) - 1),
                        RTGUI_FILE_SEEK_CUR) < 0) {
                        err = -RT_EIO;
                        break;
                    }
                }
            } /* for (y = 0; y < h; y++) */
            if (RT_EOK != err) break;
        } else { /* (!bmp->is_loaded) */
            rt_uint16_t y;
            rt_uint8_t *ptr;

            /* output the image */
            for (y = 0; y < h; y++) {
                ptr = bmp->pixels + (y * display()->pitch);
                dc->engine->blit_line(dc, dst_rect->x1, dst_rect->x1 + w + 1,
                    dst_rect->y1 + y, ptr);
            }
        }
    } while (0);

    if (lineBuf) rtgui_free(lineBuf);
    if (lineBuf2) rtgui_free(lineBuf2);
    if (RT_EOK != err) {
        LOG_E("blit err");
    }
}

/*
 * config BMP header.
 */
void rtgui_image_bmp_header_cfg(struct rtgui_image_bmp_header *bhr,
    rt_int32_t w, rt_int32_t h, rt_uint16_t bits_per_pixel) {
    int image_size = w * h * _BIT2BYTE(bits_per_pixel);
    int header_size = sizeof(struct rtgui_image_bmp_header);

    bhr->bfType = 0x4d42; /* BM */
    bhr->bfSize = header_size + image_size; /* data size */
    bhr->bfReserved1 = 0;
    bhr->bfReserved2 = 0;
    bhr->bfOffBits = header_size;

    bhr->biSize = 40; /* sizeof BITMAPINFOHEADER */
    bhr->biWidth = w;
    bhr->biHeight = h;
    bhr->biPlanes = 1;
    bhr->biBitCount = bits_per_pixel;
    bhr->biCompression = BI_BITFIELDS;
    bhr->biSizeImage = image_size;
    bhr->biXPelsPerMeter = 0;
    bhr->biYPelsPerMeter = 0;
    bhr->biClrUsed = 0;
    bhr->biClrImportant = 0;
    if (bhr->biBitCount == 16 && bhr->biCompression == BI_BITFIELDS) {
        bhr->bfSize += 12;
        bhr->bfOffBits += 12;
    }
}

#ifdef GUIENGINE_USING_DFS_FILERW
# define WRITE_CLUSTER_SIZE  2048

void bmp_align_write(rtgui_filerw_t *file, char *dest,
    char *src, rt_int32_t len, rt_int32_t *count) {
    rt_int32_t len_bak = len;

    while (len) {
        if (*count >= WRITE_CLUSTER_SIZE) {
            rtgui_filerw_write(file, dest, 1, WRITE_CLUSTER_SIZE);
            *count = 0;
        }
        *(dest + *count) = *(src + (len_bak - len));
        len--;
        (*count)++;
    }
}

/*
 * Grab screen and save as a BMP file
 * MACRO RGB_CONVERT_TO_BGR: If the pixel of colors is BGR mode, defined it.
 */
void screenshot(const char *filename) {
    rtgui_filerw_t *file;
    int w, h, i, pitch;
    rt_uint16_t *src;
    rt_uint32_t mask;
    struct rtgui_image_bmp_header bhr;
    rtgui_gfx_driver_t *grp = display();
    #ifdef RGB_CONVERT_TO_BGR
        int j;
        rt_uint16_t *line_buf;
        rt_uint16_t color, tmp;
    #endif
    char *pixel_buf;
    rt_int32_t write_count = 0;

    file = rtgui_filerw_create_file(filename, "wb");
    if (!file) {
        rt_kprintf("create file failed\n");
        return;
    }

    w = grp->width;
    h = grp->height;

    pitch = w * sizeof(rt_uint16_t);
    #ifdef RGB_CONVERT_TO_BGR
        line_buf = rtgui_malloc(pitch);
        if (!line_buf) {
            rt_kprintf("no memory!\n");
            return;
        }
    #endif
    pixel_buf = rtgui_malloc(WRITE_CLUSTER_SIZE);
    if (!pixel_buf) {
        rt_kprintf("no memory!\n");
    #ifdef RGB_CONVERT_TO_BGR
            rtgui_free(line_buf);
    #endif
        return;
    }

    rtgui_image_bmp_header_cfg(&bhr, w, h, grp->bits_per_pixel);
    bmp_align_write(file, pixel_buf, (char *)&bhr,
        sizeof(struct rtgui_image_bmp_header), &write_count);

    if (BI_BITFIELDS == bhr.biCompression) {
        mask = 0xF800; /* Red Mask */
        bmp_align_write(file, pixel_buf, (char *)&mask, 4, &write_count);
        mask = 0x07E0; /* Green Mask */
        bmp_align_write(file, pixel_buf, (char *)&mask, 4, &write_count);
        mask = 0x001F; /* Blue Mask */
        bmp_align_write(file, pixel_buf, (char *)&mask, 4, &write_count);
    }
    rtgui_screen_lock(RT_WAITING_FOREVER);
    if (grp->framebuffer) {
        src = (rt_uint16_t *)grp->framebuffer;
        src += w * h;
        for (i = 0; i < h; i++) {
            src -= w;
            #ifdef RGB_CONVERT_TO_BGR
                for (j = 0; j < w; j++)
                {
                    tmp = *(src + j);
                    color  = (tmp & 0x001F) << 11;
                    color += (tmp & 0x07E0);
                    color += (tmp & 0xF800) >> 11;

                    *(line_buf + i) = color;
                }
                bmp_align_write(file, pixel_buf, (char *)line_buf, pitch, &write_count);
            #else
                bmp_align_write(file, pixel_buf, (char *)src, pitch, &write_count);
            #endif
        }
    } else {
        rtgui_color_t pixel_color;
        rt_uint16_t write_color;
        int x;

        for (i = h - 1; i >= 0; i--) {
            x = 0;
            if ((i % 10) == 0) rt_kprintf(">", i);
            while (x < w) {
                grp->ops->get_pixel(&pixel_color, x, i);
                write_color = rtgui_color_to_565p(pixel_color);
                bmp_align_write(file, pixel_buf, (char *)&write_color,
                    sizeof(rt_uint16_t), &write_count);
                x++;
            }
        }
    }
    /* write The tail of the last */
    if (write_count < WRITE_CLUSTER_SIZE) {
        rtgui_filerw_write(file, pixel_buf, 1, write_count);
    }
    rtgui_screen_unlock();
    #ifdef RGB_CONVERT_TO_BGR
        rtgui_free(line_buf);
    #endif
    rtgui_free(pixel_buf);
    rt_kprintf("bmp create succeed.\n");
    rtgui_filerw_close(file);
}

#ifdef RT_USING_FINSH
# include "components/finsh/finsh.h"
FINSH_FUNCTION_EXPORT(screenshot, usage: screenshot(filename));
# endif
#endif

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_image_bmp_init(void) {
    /* register bmp engine */
    return rtgui_image_register_engine(&bmp_engine);
}

#endif /* GUIENGINE_IMAGE_BMP */
