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
#include "include/image.h"

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
    rt_int32_t scale, rt_bool_t load);
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
    rt_uint8_t bits_per_pixel;
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
#define display()                   (rtgui_get_gfx_device())

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
    rt_int32_t scale, rt_bool_t load_body) {
    rtgui_image_bmp_t *bmp;
    rt_uint8_t *lineBuf;
    rt_err_t err;

    do {
        rt_uint8_t temp[32];
        rt_uint32_t ncolors = 0;
        rt_uint32_t headerSize;
        rt_uint16_t bits_per_pixel;
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
        bmp->pixel_offset = ((rt_uint32_t)temp[10] <<  0) | \
                            ((rt_uint32_t)temp[11] <<  8) | \
                            ((rt_uint32_t)temp[12] << 16) | \
                            ((rt_uint32_t)temp[13] << 24);
        /* get BMP header size */
        headerSize = ((rt_uint32_t)temp[14] <<  0) | \
                     ((rt_uint32_t)temp[15] <<  8) | \
                     ((rt_uint32_t)temp[16] << 16) | \
                     ((rt_uint32_t)temp[17] << 24);
        err = -RT_EIO;
        if (12 == headerSize) {
            /* BITMAPCOREHEADER */
            if (8 != rtgui_filerw_read(file, temp, 1, 8)) break;
            /* get image size */
            bmp->w = ((rt_uint32_t)temp[0] << 0) | \
                     ((rt_uint32_t)temp[1] << 8);
            bmp->h = ((rt_uint32_t)temp[2] << 0) | \
                     ((rt_uint32_t)temp[3] << 8);
            /* get bits per pixel */
            bits_per_pixel = ((rt_uint32_t)temp[6] << 0) | \
                             ((rt_uint32_t)temp[7] << 8);
        } else {
            /* BITMAPINFOHEADER and later */
            if (32 != rtgui_filerw_read(file, temp, 1, 32)) break;
            /* get image size */
            bmp->w = ((rt_uint32_t)temp[0] <<  0) | \
                     ((rt_uint32_t)temp[1] <<  8) | \
                     ((rt_uint32_t)temp[2] << 16) | \
                     ((rt_uint32_t)temp[3] << 24);
            bmp->h = ((rt_uint32_t)temp[4] <<  0) | \
                     ((rt_uint32_t)temp[5] <<  8) | \
                     ((rt_uint32_t)temp[6] << 16) | \
                     ((rt_uint32_t)temp[7] << 24);
            /* get bits per pixel */
            bits_per_pixel = ((rt_uint32_t)temp[10] << 0) | \
                             ((rt_uint32_t)temp[11] << 8);
            /* The 16bpp and 32bpp images are always stored uncompressed */
            if ((16 != bits_per_pixel) && (32 != bits_per_pixel)) {
                /* check compression */
                rt_uint32_t comp = ((rt_uint32_t)temp[12] <<  0) | \
                                   ((rt_uint32_t)temp[13] <<  8) | \
                                   ((rt_uint32_t)temp[14] << 16) | \
                                   ((rt_uint32_t)temp[15] << 24);
                if (comp != BI_RGB) {
                    err = -RT_ERROR;
                    LOG_E("bad BMP comp %d", comp);
                    break;
                }
                LOG_D("comp %d", comp);
            }
            /* get number of colors */
            ncolors = ((rt_uint32_t)temp[28] <<  0) | \
                      ((rt_uint32_t)temp[29] <<  8) | \
                      ((rt_uint32_t)temp[30] << 16) | \
                      ((rt_uint32_t)temp[31] << 24);
        }

        /* check bits per pixel */
        if (bits_per_pixel > 32) {
            err = -RT_ERROR;
            LOG_E("bad BMP bitPP %d", bits_per_pixel);
            break;
        }
        LOG_D("bitPP %d", bits_per_pixel);
        bmp->bits_per_pixel = (rt_uint8_t)bits_per_pixel;

        /* load palette */
        if (!ncolors) {
            ncolors = 1 << bmp->bits_per_pixel;
        }
        if (bmp->bits_per_pixel <= 8) {
            if (rtgui_filerw_seek(
                file, 14 + headerSize, RTGUI_FILE_SEEK_SET) < 0) {
                break;
            }
            img->palette = bmp_load_palette(file, ncolors, loadAlpha);
            if (!img->palette) break;
        }

        /* get scale */
        if (scale == 0) {
            while (scale < BMP_MAX_SCALING_FACTOR) {
                if (display()->width > (bmp->w >> scale)) break;
                scale++;
            }
        } else if (scale < 0) {
            scale = 0;
        }
        if (scale >= BMP_MAX_SCALING_FACTOR) {
            scale = BMP_MAX_SCALING_FACTOR;
        }

        /* set bmp info */
        bmp->scale = scale;
        if (1 == bmp->bits_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_MONO;
            bmp->pitch = (bmp->w + 7) >> 3;
        } else if (2 == bmp->bits_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB2I;
            bmp->pitch = (bmp->w + 3) >> 2;
        } else if (4 == bmp->bits_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB4I;
            bmp->pitch = (bmp->w + 1) >> 1;
        } else if (8 == bmp->bits_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB8I;
            bmp->pitch = bmp->w;
        } else if (16 == bmp->bits_per_pixel) {
            /* assume RGB565 */
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
            bmp->pitch = bmp->w << 1;
        } else if (24 == bmp->bits_per_pixel) {
            bmp->pixel_format = RTGRAPHIC_PIXEL_FORMAT_BGR888;
            bmp->pitch = bmp->w * _BIT2BYTE(bmp->bits_per_pixel);
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
                    dst_rect->y1 + (h - 1 - y), lineBuf2);

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

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_image_bmp_init(void) {
    /* register bmp engine */
    return rtgui_image_register_engine(&bmp_engine);
}


#if defined(RT_USING_DFS) && defined(RT_USING_FINSH)
#include "components/finsh/finsh.h"
#define WRITE_BUFFER_SIZE           (512)

#pragma pack(push)
#pragma pack(2)
struct bmp_header {
    /* The Win32 BMP file header (14 bytes) */
    rt_uint16_t bfType;
    rt_uint32_t bfSize;
    rt_uint16_t bfReserved1;
    rt_uint16_t bfReserved2;
    rt_uint32_t bfOffBits;

    /* The Win32 BITMAPINFOHEADER struct (40 bytes) */
    rt_uint32_t biSize;
    rt_int32_t  biWidth;
    rt_int32_t  biHeight;
    rt_uint16_t biPlanes;
    rt_uint16_t biBitCount;
    rt_uint32_t biCompression;
    rt_uint32_t biSizeImage;
    rt_int32_t  biXPelsPerMeter;
    rt_int32_t  biYPelsPerMeter;
    rt_uint32_t biClrUsed;
    rt_uint32_t biClrImportant;
};
#pragma pack(pop)

static void bmp_set_header(struct bmp_header *hdr, rt_uint32_t w, rt_uint32_t h,
    rt_uint8_t bits_per_pixel) {
    rt_uint32_t img_size = w * h * _BIT2BYTE(bits_per_pixel);

    /* BMP header */
    hdr->bfType = 0x4d42;           /* "BM" */
    hdr->bfSize = sizeof(struct bmp_header) + 12 + img_size;
    hdr->bfReserved1 = 0;
    hdr->bfReserved2 = 0;
    hdr->bfOffBits = sizeof(struct bmp_header) + 12;
    /* DIB header (BITMAPV2INFOHEADER: BITMAPINFOHEADER + RGB bit masks) */
    hdr->biSize = 40;
    hdr->biWidth = w;
    hdr->biHeight = h;
    hdr->biPlanes = 1;              /* Fixed */
    hdr->biBitCount = bits_per_pixel;
    hdr->biCompression = BI_BITFIELDS;
    hdr->biSizeImage = img_size;
    hdr->biXPelsPerMeter = 0;
    hdr->biYPelsPerMeter = 0;
    hdr->biClrUsed = 0;
    hdr->biClrImportant = 0;
}

/* Grab screen and save as BMP file */
rt_err_t screenshot(const char *filename) {
    int file = -1;
    rt_uint8_t *buf = RT_NULL;
    rt_bool_t locked = RT_FALSE;
    rt_err_t ret = RT_EOK;

    do {
        struct bmp_header *hdr;
        rt_uint32_t w = display()->width;
        rt_uint32_t h = display()->height;

        buf = rtgui_malloc(WRITE_BUFFER_SIZE);
        if (RT_NULL == buf) {
            LOG_E("no mem");
            break;
        }
        file = open(filename, O_CREAT | O_WRONLY);
        if (file < 0) {
            ret = -RT_EIO;
            LOG_E("bad file");
            break;
        }

        /* header */
        hdr = (struct bmp_header *)buf;
        bmp_set_header(hdr, w, h, display()->bits_per_pixel);
        if (write(file, buf, sizeof(struct bmp_header)) < 0) {
            ret = -RT_EIO;
            LOG_E("bad write");
            break;
        }

        if (BI_BITFIELDS == hdr->biCompression) {
            rt_uint32_t *mask = (rt_uint32_t *)buf;

            *(mask++) = 0x0000F800;     /* Red Mask */
            *(mask++) = 0x000007E0;     /* Green Mask */
            *(mask++) = 0x0000001F;     /* Blue Mask */
            if (write(file, buf, 12) < 0) {
                ret = -RT_EIO;
                LOG_E("bad write");
                break;
            }
        }

        rtgui_screen_lock(RT_WAITING_FOREVER);
        locked = RT_TRUE;

        if (display()->framebuffer) {
            if (write(file, display()->framebuffer,
                w * h * _BIT2BYTE(display()->bits_per_pixel)) < 0) {
                ret = -RT_EIO;
                LOG_E("bad write");
                break;
            }
        } else {
            rt_uint32_t len, i;
            rtgui_color_t pixel;
            rt_uint32_t x;
            rt_int32_t y;

            rt_kprintf("\n00%%\r");
            if (RTGRAPHIC_PIXEL_FORMAT_RGB565 == display()->pixel_format) {
                rt_uint16_t *c;

                len = WRITE_BUFFER_SIZE / sizeof(rt_uint16_t);
                i = 0;
                c = (rt_uint16_t *)buf;

                for (y = h - 1; y >= 0; y--) {
                    rt_kprintf("%02d%%\r", (h - 1 - y) * 100 / h);
                    for (x = 0; x < w; x++) {
                        display()->ops->get_pixel(&pixel, x, y);
                        if (i >= len) {
                            if (write(file, buf, sizeof(rt_uint16_t) * i) < 0) {
                                ret = -RT_EIO;
                                LOG_E("bad write");
                                break;
                            }
                            i = 0;
                        }
                        *(c + i++) = ((pixel & 0x00f80000) >> 8) | \
                                     ((pixel & 0x0000fc00) >> 5) | \
                                     ((pixel & 0x000000f8) >> 3);
                    }
                    if (RT_EOK != ret) break;
                }
                if (RT_EOK != ret) break;
                if (i) {
                    if (write(file, buf, sizeof(rt_uint16_t) * i) < 0) {
                        ret = -RT_EIO;
                        LOG_E("bad write");
                        break;
                    }
                }
            } else {
                /* TODO */
                ret = -RT_ERROR;
                LOG_E("not implemented");
                break;
            }
            rt_kprintf("Done %s\n", filename);
        }
    } while (0);

    if (locked) rtgui_screen_unlock();
    if (buf) rtgui_free(buf);
    if (file >= 0) {
        fsync(file);
        close(file);
    }

    return ret;
}
FINSH_FUNCTION_EXPORT(screenshot, usage: screenshot(filename));

int prtscn(int argc, char **argv) {
    if (argc != 2) {
        rt_kprintf("\nusage: prtscn(filename)\n");
        return -1;
    }
    return screenshot(argv[1]);
}

#endif /* defined(RT_USING_DFS) && defined(RT_USING_FINSH) */

#endif /* GUIENGINE_IMAGE_BMP */
