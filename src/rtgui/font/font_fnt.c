/*
 * File      : font_fnt.c
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
 */

/*
* rockbox fnt font engine
*/
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/font_fnt.h"
#ifdef RT_USING_DFS
# include "components/dfs/include/dfs_posix.h"
#endif

/* Private function prototype ------------------------------------------------*/
static void rtgui_fnt_font_draw_text(rtgui_font_t *font, rtgui_dc_t *dc,
    const char *text, rt_ubase_t len, rtgui_rect_t *rect);
static void rtgui_fnt_font_get_metrics(rtgui_font_t *font, const char *text,
    rtgui_rect_t *rect);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
#ifdef GUIENGINE_USING_FONT12
#include "include/font/asc12font.h"

    static const struct fnt_font _asc12 = {
        {
            "RB12",
            13,
            12,
            10,
            0,
            32,
            32,
            224,
            2864,
            224,
            224
        },
        _font_bits,         /* bits */
        _sysfont_offset,    /* offset */
        _sysfont_width      /* width */
    };
#endif /* GUIENGINE_USING_FONT12 */

/* Exported constants --------------------------------------------------------*/
const rtgui_font_engine_t fnt_font_engine = {
    RT_NULL,
    RT_NULL,
    rtgui_fnt_font_draw_text,
    rtgui_fnt_font_get_metrics
};

#ifdef GUIENGINE_USING_FONT12
    const rtgui_font_t rtgui_font_asc12 = {
        "asc",              /* family */
        12,                 /* height */
        1,                  /* refer count */
        &fnt_font_engine,   /* font engine */
        (void *)&_asc12,    /* font private data */
        { RT_NULL },
    };
#endif /* GUIENGINE_USING_FONT12 */

/* Private functions ---------------------------------------------------------*/
static void rtgui_fnt_font_draw_text(rtgui_font_t *font, rtgui_dc_t *dc,
    const char *text, rt_ubase_t len, rtgui_rect_t *rect) {
    struct fnt_font *fnt;
    rtgui_rect_t text_rect;
    int ch, i, j, c, width;
    rt_uint32_t position;
    rt_uint8_t *data_ptr;

    RT_ASSERT(font->data != RT_NULL);
    fnt = font->data;

    rtgui_font_get_metrics(font, text, &text_rect);
    rtgui_rect_move_to_align(rect, &text_rect, RTGUI_DC_TEXTALIGN(dc));

    while (len) {
        /* get character */
        ch = *text;
        /* NOTE: we only support asc character right now */
        if (ch > 0x80) {
            text += 1;
            len -= 1;
            continue;
        }

        /* get position and width */
        if (!fnt->offset) {
            width = fnt->header.max_width;
            position = (ch - fnt->header._start) * width * \
                       ((fnt->header.height + 7) / 8);
        } else {
            width = fnt->width[ch - fnt->header._start];
            position = fnt->offset[ch - fnt->header._start];
        }

        /* draw a character */
        data_ptr = (rt_uint8_t*)&fnt->bits[position];
        for (i = 0; i < width; i ++) { /* x */
            for (j = 0; j < 8; j ++) { /* y */
                for (c = 0; c < (fnt->header.height + 7)/8; c ++) {
                    /* check drawable region */
                    if (((text_rect.x1 + i) > text_rect.x2) || \
                        ((text_rect.y1 + c * 8 + j) > text_rect.y2))
                        continue;

                    if (data_ptr[i + c * width] & (1 << j))
                        rtgui_dc_draw_point(
                            dc, text_rect.x1 + i, text_rect.y1 + c * 8 + j);
                }
            }
        }

        text_rect.x1 += width;
        text += 1;
        len -= 1;
    }
}

static void rtgui_fnt_font_get_metrics(rtgui_font_t *font, const char *text,
    rtgui_rect_t *rect) {
    struct fnt_font *fnt;
    int ch;

    RT_ASSERT(font->data != RT_NULL);
    fnt = font->data;

    rt_memset(rect, 0x00, sizeof(rtgui_rect_t));
    rect->y2 = fnt->header.height;

    while (*text) {
        if (!fnt->width) {
            /* fixed width font */
            rect->x2 += fnt->header.max_width;
        } else {
            ch = *text;
            /* NOTE: we only support asc character right now */
            if (ch > 0x80) {
                text += 1;
                continue;
            }
            rect->x2 += fnt->width[ch - fnt->header._start];
        }
        text += 1;
    }
}

/* Public functions ----------------------------------------------------------*/
rtgui_font_t *rtgui_fnt_font_create(const char* filename,
    const char* font_family) {
    int fd = -1, file_len = 0;
    rtgui_font_t *font = RT_NULL;
    struct rtgui_fnt_header *fnt_header = RT_NULL;

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0) return RT_NULL;

    file_len = lseek(fd, 0, SEEK_END);
    lseek(fd, (file_len - sizeof(struct rtgui_fnt_header)), SEEK_SET);

    /* read fnt header */
    fnt_header = (struct rtgui_fnt_header*)rtgui_malloc(
        sizeof(struct rtgui_fnt_header));
    if (fnt_header) {
        if (read(fd, fnt_header, sizeof(struct rtgui_fnt_header)) != \
            sizeof(struct rtgui_fnt_header)) {
            rtgui_free(fnt_header);
            close(fd);
            return RT_NULL;
        }
    }

    /* crc */
    {
        rt_uint8_t *data = (rt_uint8_t *)fnt_header;
        rt_uint16_t crc_h = data[0];
        rt_uint16_t crc_l = data[1];
        rt_uint16_t crc;
        rt_uint8_t i ;

        for (i = 2; i < 22; i += 2) {
            crc_h ^= data[i];
            crc_l ^= data[i + 1];
        }

        crc = (crc_h << 8) + crc_l;

        if ((crc != fnt_header->crc16) || (fnt_header->w == 0) || \
            (fnt_header->h == 0)) {
            rtgui_free(fnt_header);
            close(fd);
            return RT_NULL;
        }
    }

    /* load font */
    {
        rtgui_font_bitmap_t *asc = (rtgui_font_bitmap_t*)rtgui_malloc(
            sizeof(rtgui_font_bitmap_t));
        struct rtgui_hz_file_font *hz = NULL;
        rtgui_font_t *asc_font = (rtgui_font_t*)rtgui_malloc(
            sizeof(rtgui_font_t));
        rtgui_font_t *hz_font = NULL;

        #ifdef GUIENGINE_USING_HZ_FILE
            hz = (struct rtgui_hz_file_font*)rtgui_malloc(
                sizeof(struct rtgui_hz_file_font));
            hz_font = (rtgui_font_t*)rtgui_malloc(sizeof(rtgui_font_t));

            /* load hz */
            if (hz && hz_font && fnt_header->asc_offset != 0) {
                struct cache_tree _cache_root = { NULL };

                hz->cache_root = _cache_root;
                hz->cache_size = 0;
                hz->font_size = fnt_header->h;
                hz->font_data_size = (fnt_header->h + 7) / 8 * fnt_header->h;
                hz->fd = -1;
                hz->font_fn = rt_strdup(filename);

                hz_font->family = rt_strdup(font_family);
                hz_font->height = fnt_header->h;
                hz_font->refer_count = 1;
                hz_font->engine = &rtgui_hz_file_font_engine;
                hz_font->data = (void *)hz;
            } else {
                rtgui_free(hz);
                rtgui_free(hz_font);
                hz_font = RT_NULL;
            }
        #endif

        /* load asc */
        if (asc && asc_font) {
            asc->bmp = (const rt_uint8_t *)rtgui_malloc(fnt_header->asc_length);
            if (asc->bmp) {
                asc->_len = 0;
                asc->offset = RT_NULL;
                asc->width = fnt_header->w / 2;
                asc->height = fnt_header->h;
                asc->_start = 0x00;
                asc->_end = 0xFF;

                lseek(fd, fnt_header->asc_offset, SEEK_SET);
                if (fnt_header->asc_length != (rt_uint32_t) \
                    read(fd, (void*)asc->bmp, fnt_header->asc_length)) {
                    rtgui_free((void*)asc->bmp);
                    rtgui_free(asc);
                    rtgui_free(asc_font);
                } else {
                    if (hz_font)
                        asc_font->family = rt_strdup("asc");
                    else
                        asc_font->family = rt_strdup(font_family);
                    asc_font->height = fnt_header->h;
                    asc_font->refer_count = 1;
                    asc_font->engine = &bmp_font_engine;
                    asc_font->data = (void *)asc;

                    rtgui_font_system_add_font(asc_font);
                }
            }
        }

        close(fd);

        if (hz_font) {
            rtgui_font_system_add_font(hz_font);
            font = hz_font;
        } else if (asc_font) {
            font = asc_font;
        }
    }

    return font;
}

rtgui_font_t *rtgui_hz_fnt_font_create(const char* filename,
    const char* font_family, rt_uint8_t font_size) {
        rtgui_font_t *font = RT_NULL;

    #ifdef GUIENGINE_USING_HZ_FILE
        int fd = -1;

        fd = open(filename, O_RDONLY, 0);
        if (fd < 0) return RT_NULL;

        close(fd);

        /* load font */
        {
            struct rtgui_hz_file_font *hz = (struct rtgui_hz_file_font*) \
                rtgui_malloc(sizeof(struct rtgui_hz_file_font));
            rtgui_font_t *hz_font = (rtgui_font_t*)rtgui_malloc(
                sizeof(rtgui_font_t));

            /* load hz */
            if (hz && hz_font) {
                struct cache_tree _cache_root = { NULL };

                hz->cache_root = _cache_root;
                hz->cache_size = 0;
                hz->font_size = font_size;
                hz->font_data_size = (font_size + 7) / 8 * font_size;
                hz->fd = -1;
                hz->font_fn = rt_strdup(filename);

                hz_font->family = rt_strdup(font_family);
                hz_font->height = font_size;
                hz_font->refer_count = 1;
                hz_font->engine = &rtgui_hz_file_font_engine;
                hz_font->data = (void *)hz;

                rtgui_font_system_add_font(hz_font);
                font = hz_font;
            } else {
                rtgui_free(hz);
                rtgui_free(hz_font);
            }
        }
    #endif

    return font;
}

rtgui_font_t *rtgui_asc_fnt_font_create(const char* filename,
    const char* font_family, rt_uint8_t font_size) {
    int fd = -1, file_len = 0;
    rtgui_font_t *font = RT_NULL;

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0) return RT_NULL;

    file_len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    /* load font */
    {
        rtgui_font_bitmap_t *asc = (rtgui_font_bitmap_t*)rtgui_malloc(
            sizeof(rtgui_font_bitmap_t));
        rtgui_font_t *asc_font = (rtgui_font_t*)rtgui_malloc(
            sizeof(rtgui_font_t));

        /* load asc */
        if (asc && asc_font) {
            asc->bmp = (const rt_uint8_t *)rtgui_malloc(file_len);
            if (asc->bmp) {
                asc->_len = 0;
                asc->offset = RT_NULL;
                asc->width = font_size / 2;
                asc->height = font_size;
                asc->_start = 0x00;
                asc->_end = 0xFF;

                if (file_len != read(fd, (void*)asc->bmp, file_len)) {
                    rtgui_free((void*)asc->bmp);
                    rtgui_free(asc);
                    rtgui_free(asc_font);
                } else {
                    asc_font->family = rt_strdup("asc");
                    asc_font->height = font_size;
                    asc_font->refer_count = 1;
                    asc_font->engine = &bmp_font_engine;
                    asc_font->data = (void *)asc;

                    rtgui_font_system_add_font(asc_font);
                    font = asc_font;
                }
            }
        }
        close(fd);
    }

    return font;
}

#ifdef GUIENG_USING_FNT_FILE

rt_inline int readbyte(int fd, unsigned char *cp)
{
    unsigned char buf[1];

    if (read(fd, buf, 1) != 1)
        return 0;
    *cp = buf[0];
    return 1;
}

rt_inline int readshort(int fd, unsigned short *sp)
{
    unsigned char buf[2];

    if (read(fd, buf, 2) != 2)
        return 0;
    *sp = buf[0] | (buf[1] << 8);
    return 1;
}

rt_inline int readlong(int fd, rt_uint32_t *lp)
{
    unsigned char buf[4];

    if (read(fd, buf, 4) != 4)
        return 0;
    *lp = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    return 1;
}

rt_inline int readstr(int fd, char *buf, int count)
{
    return read(fd, buf, count);
}

rtgui_font_t *fnt_font_create(const char* filename, const char* font_family)
{
    int fd = -1;
    rt_uint32_t index;
    rtgui_font_t *font = RT_NULL;
    struct fnt_font *fnt = RT_NULL;
    struct fnt_header *fnt_header;

    fd = open(filename, O_RDONLY, 0);
    if (fd < 0)
    {
        goto __exit;
    }

    font = (rtgui_font_t*) rtgui_malloc (sizeof(rtgui_font_t));
    if (font == RT_NULL) goto __exit;
    fnt = (struct fnt_font*) rtgui_malloc (sizeof(struct fnt_font));
    if (fnt == RT_NULL) goto __exit;
    rt_memset(fnt, 0x00, sizeof(struct fnt_font));
    font->data = (void*)fnt;

    fnt_header = &(fnt->header);
    if (readstr(fd, (char *)fnt_header->version, 4) != 4) goto __exit;
    if (!readshort(fd, &fnt_header->max_width)) goto __exit;
    if (!readshort(fd, &fnt_header->height)) goto __exit;
    if (!readshort(fd, &fnt_header->ascent)) goto __exit;
    if (!readshort(fd, &fnt_header->depth)) goto __exit;

    if (!readlong(fd, &fnt_header->_start)) goto __exit;
    if (!readlong(fd, &fnt_header->default_char)) goto __exit;
    if (!readlong(fd, &fnt_header->size)) goto __exit;
    if (!readlong(fd, &fnt_header->nbits)) goto __exit;
    if (!readlong(fd, &fnt_header->noffset)) goto __exit;
    if (!readlong(fd, &fnt_header->nwidth)) goto __exit;

    fnt->bits = (MWIMAGEBITS*) rtgui_malloc (fnt_header->nbits * sizeof(MWIMAGEBITS));
    if (fnt->bits == RT_NULL) goto __exit;
    /* read data */
    if (readstr(fd, (char *)&(fnt->bits[0]), fnt_header->nbits) != fnt_header->nbits) goto __exit;

    /* check boundary */
    if (fnt_header->nbits & 0x01)
    {
        rt_uint16_t pad;
        readshort(fd, &pad);
        pad = pad; /* skip warning */
    }

    if (fnt_header->noffset != 0)
    {
        fnt->offset = rtgui_malloc (fnt_header->noffset * sizeof(rt_uint32_t));
        if (fnt->offset == RT_NULL) goto __exit;

        for (index = 0; index < fnt_header->noffset; index ++)
        {
            if (!readshort(fd, (unsigned short *)&(fnt->offset[index]))) goto __exit;
        }
    }

    if (fnt_header->nwidth != 0)
    {
        fnt->width = rtgui_malloc (fnt_header->nwidth * sizeof(rt_uint8_t));
        if (fnt->width == RT_NULL) goto __exit;

        for (index = 0; index < fnt_header->nwidth; index ++)
        {
            if (!readbyte(fd, (unsigned char *)&(fnt->width[index]))) goto __exit;
        }
    }

    close(fd);

    font->family = rt_strdup(font_family);
    font->height = fnt->header.height;
    font->refer_count = 0;
    font->engine = &fnt_font_engine;

    /* add to system */
    rtgui_font_system_add_font(font);

    return font;

__exit:
    if (fd >= 0) close(fd);
    if (fnt != RT_NULL)
    {
        if (fnt->bits != RT_NULL) rtgui_free((void *)fnt->bits);
        if (fnt->offset != RT_NULL) rtgui_free((void *)fnt->offset);
        if (fnt->width != RT_NULL) rtgui_free((void *)fnt->width);

        rtgui_free(fnt);
    }

    if (font != RT_NULL)
    {
        rtgui_free(font);
    }
    return RT_NULL;
}
#endif /* GUIENG_USING_FNT_FILE */

