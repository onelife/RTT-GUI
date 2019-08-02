/*
 * File      : image_xpm.c
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
 * 2019-06-17     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef GUIENGINE_IMAGE_XPM

#include "include/image.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "IMG_XPM"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
static rt_bool_t xpm_check(rtgui_filerw_t *file);
static rt_bool_t xpm_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load);
static void xpm_unload(rtgui_image_t *img);
static void xpm_blit(rtgui_image_t *img, rtgui_dc_t *dc, rtgui_rect_t *rect);

/* Private define ------------------------------------------------------------*/
// #define XPM_MAGIC_LEN               9
#define HASH_TABLE_INIT_SIZE        (1 << 8)
#define HASH_TABLE_KEY_SIZE         (3)

/* Private typedef -----------------------------------------------------------*/
struct rgb_item {
    char *name;
    rt_uint8_t r;
    rt_uint8_t g;
    rt_uint8_t b;
};
/* Hash table to look up colors from pixel strings */
struct hash_entry {
    char name[HASH_TABLE_KEY_SIZE];
    rtgui_color_t color;
    struct hash_entry *next;
};

struct color_hash {
    struct hash_entry **table;
    struct hash_entry *entries;     /* array of all entries */
    struct hash_entry *next_free;
    rt_uint32_t size;
    rt_uint32_t num;
};

/* Private variables ---------------------------------------------------------*/
rtgui_image_engine_t xpm_engine = {
    "xpm",
    { RT_NULL },
    xpm_check,
    xpm_load,
    xpm_unload,
    xpm_blit,
};

/* Private functions ---------------------------------------------------------*/
static rt_uint32_t _str2int(const char *str, rt_uint32_t rt_strlen,
    rt_uint32_t *p) {
    rt_uint32_t idx;

    *p = 0;
    /* skip the leading non-number */
    for (idx = 0; idx < rt_strlen; idx++)
        if (('0' <= str[idx]) && (str[idx] <= '9')) break;

    for (; idx < rt_strlen; idx++) {
        if ((str[idx] < '0') || ('9' < str[idx])) break;
        *p = (*p) * 10 + (str[idx] - '0');
    }
    return idx;
}

static rt_uint32_t _hex2int(const char *str) {
    rt_uint32_t idx, num;

    num = 0;
    for (idx = 0; idx < 2; idx++) {
        if        (('0' <= str[idx]) && (str[idx] <= '9')) {
            num += str[idx] - '0';
        } else if (('a' <= str[idx]) && (str[idx] <= 'f')) {
            num += str[idx] - 'a' + 10;
        } else if (('A' <= str[idx]) && (str[idx] <= 'F')) {
            num += str[idx] - 'A' + 10;
        } else {
            LOG_E("hex2int err: %c", str[idx]);
            return 0;
        }
        if (idx < 1)
            num *= 16;
    }
    return num;
}

static rt_uint32_t hash_func(const char *name, rt_uint32_t name_sz,
    rt_uint32_t size) {
    rt_uint32_t hash = 0;
    while (name_sz-- > 0)
        hash = hash * 33 + *name++;
    return hash & (size - 1);
}

static void delete_color_table(struct color_hash *hash) {
    if (!hash) return;

    if (hash->entries) rtgui_free(hash->entries);
    if (hash->table) rtgui_free(hash->table);
    if (hash) rtgui_free(hash);

    hash = RT_NULL;
}

static struct color_hash *create_color_table(rt_uint32_t num) {
    struct color_hash *hash;

    do {
        hash = rtgui_malloc(sizeof(struct color_hash));
        if (!hash) break;

        /* table size is power of 2 */
        hash->size = HASH_TABLE_INIT_SIZE;
        while (hash->size < num)
            hash->size <<= 1;
        hash->num = num;

        hash->table = rtgui_malloc(hash->size * sizeof(void *));
        if (!hash->table) break;
        rt_memset(hash->table, 0x00, hash->size * sizeof(void *));
        hash->entries = rtgui_malloc(num * sizeof(struct hash_entry));
        if (!hash->entries) break;
        hash->next_free = hash->entries;

        return hash;
    } while (0);

    delete_color_table(hash);
    return RT_NULL;
}

static void add_color_hash(struct color_hash *hash, char *name,
    rt_uint32_t name_sz, rtgui_color_t *color) {
    rt_uint32_t index;
    struct hash_entry *entry;

    entry = hash->next_free++;
    index = hash_func(name, name_sz, hash->size);

    /* init entry */
    rt_strncpy(entry->name, name, name_sz);
    entry->name[name_sz] = 0x00;
    entry->color = *color;
    entry->next = hash->table[index];
    hash->table[index] = entry;
}

static void get_color_hash(struct color_hash *hash, const char *name,
    rt_uint32_t name_sz, rtgui_color_t *c) {
    struct hash_entry *entry;

    entry = hash->table[hash_func(name, name_sz, hash->size)];
    while (entry) {
        if (rt_memcmp(name, entry->name, name_sz)) {
            entry = entry->next;
            continue;
        }
        *c = entry->color;
        break;
    }
}

static rt_bool_t xpm_check(rtgui_filerw_t *file) {
    (void)file;
    return RT_TRUE;

    #if 0
    rt_uint8_t buffer[XPM_MAGIC_LEN];
    rt_size_t start;
    rt_bool_t result;

    result = RT_FALSE;

    start = rtgui_filerw_tell(file);

    /* seek to the begining of file */
    if (start != 0) rtgui_filerw_seek(file, 0, SEEK_SET);
    rtgui_filerw_read(file, &buffer[0], XPM_MAGIC_LEN, 1);

    if (rt_memcmp(buffer, "/* XPM */", (rt_ubase_t)XPM_MAGIC_LEN) == 0)
        result = RT_TRUE;

    rtgui_filerw_seek(file, start, SEEK_SET);

    return result;
    #endif
}

static rt_bool_t xpm_load(rtgui_image_t *img, rtgui_filerw_t *file,
    rt_bool_t load) {
    struct color_hash *palette = RT_NULL;
    rt_err_t err;
    (void)load;     // always load?

    if (!img) return RT_FALSE;
    err = RT_EOK;
    LOG_D("load xpm");

    do {
        char **xpm;
        rt_uint32_t len, idx, i;
        rt_uint32_t w, h, ncolors, name_sz;
        rtgui_color_t *ptr;

        img->data = RT_NULL;
        xpm = (char **)rtgui_filerw_mem_getdata(file);
        if (!xpm) {
            LOG_E("read file err");
            err = -RT_EIO;
            break;
        }
        len = rt_strlen(xpm[0]);

        /* get info */
        idx = _str2int(xpm[0],          len,        &w) + 1;
        idx += _str2int(xpm[0] + idx,   len - idx,  &h) + 1;
        idx += _str2int(xpm[0] + idx,   len - idx,  &ncolors) + 1;
        idx += _str2int(xpm[0] + idx,   len - idx,  &name_sz) + 1;
        if (name_sz > HASH_TABLE_KEY_SIZE) {
            LOG_E("name too long %d", name_sz);
            err = -RT_ERROR;
            break;
        }

        /* set image info */
        img->w = w;
        img->h = h;
        img->engine = &xpm_engine;

        palette = create_color_table(ncolors);
        if (!palette) {
            LOG_E("create palette err");
            err = -RT_ENOMEM;
            break;
        }

        /* load palette */
        for (idx = 1; idx <= ncolors; idx++) {
            rtgui_color_t c;

            len = rt_strlen(xpm[idx]);
            for (i = name_sz; i < len; i++)
                if (xpm[idx][i] == 'c') break;
            if (xpm[idx][i] != 'c') {
                LOG_E("load palette err");
                err = -RT_ERROR;
                break;
            }

            i += 2;
            if (xpm[idx][i] == '#')
                c = RTGUI_ARGB(0, _hex2int(&xpm[idx][i + 1]),
                    _hex2int(&xpm[idx][i + 3]), _hex2int(&xpm[idx][i + 5]));
            else /*if (!rt_strcasecmp(&xpm[idx][i], "None"))*/
                c = RTGUI_RGB(0, 0, 0);

            /* add to palette */
            add_color_hash(palette, xpm[idx], name_sz, &c);
        }
        if (RT_EOK != err) break;

        img->data = rtgui_malloc(img->h * img->w * sizeof(rtgui_color_t));
        if (!img->data) {
            LOG_E("no mem to load");
            err = -RT_ENOMEM;
            break;
        }

        /* load image */
        ptr = (rtgui_color_t *)img->data;
        for (idx = 0; idx < img->h; idx++)
            for (i = 0; i < img->w; i++, ptr++)
                get_color_hash(palette,
                    &xpm[ncolors + 1 + idx][i * name_sz], name_sz, ptr);
    } while (0);

    delete_color_table(palette);
    rtgui_filerw_close(file);

    if ((RT_EOK != err) && img->data)
        rtgui_free(img->data);

    return (RT_EOK == err);
}

static void xpm_unload(rtgui_image_t *img) {
    if (!img || !img->data) return;
    /* release data */
    rtgui_free(img->data);
    img->data = RT_NULL;
}

static void xpm_blit(rtgui_image_t *img, rtgui_dc_t *dc, rtgui_rect_t *rect) {
    rt_uint16_t x, y, w, h;
    rtgui_color_t *ptr;

    if (!img || !dc || !rect || !img->data) return;

    /* the minimum rect */
    w = _MIN(img->w, RECT_W(*rect));
    h = _MIN(img->h, RECT_H(*rect));

    /* draw each point within dc */
    for (y = 0; y < h; y ++) {
        ptr = (rtgui_color_t *)img->data + img->w * y;
        for (x = 0; x < w; x++, ptr++) {
            if (RTGUI_RGB_A(*ptr) != 0xff)
                rtgui_dc_draw_color_point(dc, rect->x1 + x, rect->y1 + y, *ptr);
        }
    }
}

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_image_xpm_init(void) {
    /* register xpm engine */
    return rtgui_image_register_engine(&xpm_engine);
}

#endif /* GUIENGINE_IMAGE_XPM */
