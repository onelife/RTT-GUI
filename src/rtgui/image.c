/*
 * File      : image.c
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
 * 2012-01-24     onelife      add TJpgDec (Tiny JPEG Decompressor) support
 * 2012-08-29     amsl         add Image zoom interface.
 */

#include "include/rtgui.h"
#include "include/image.h"
#include "include/image_hdc.h"
#include "include/image_container.h"
#ifdef GUIENGINE_IMAGE_BMP
# include "include/image_bmp.h"
#endif

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_IMG"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

#ifdef GUIENGINE_IMAGE_XPM
extern void rtgui_image_xpm_init(void);
#endif
#ifdef GUIENGINE_IMAGE_JPEG
extern rt_err_t rtgui_image_jpeg_init(void);
#endif
#if defined(GUIENGINE_IMAGE_PNG) || defined(GUIENGINE_IMAGE_LODEPNG)
extern void rtgui_image_png_init(void);
#endif

static rt_slist_t _rtgui_system_image_list = {RT_NULL};

/* initialize rtgui image system */
rt_err_t rtgui_system_image_init(void) {
    rt_err_t ret = RT_EOK;

    do {
        #ifdef GUIENGINE_USING_HDC
            /* always support HDC image */
            rtgui_image_hdc_init();
        #endif
        #ifdef GUIENGINE_IMAGE_XPM
            rtgui_image_xpm_init();
            LOG_D("XPM init");
        #endif
        #ifdef GUIENGINE_IMAGE_BMP
            ret = rtgui_image_bmp_init();
            if (RT_EOK != ret) break;
            LOG_D("BMP init");
        #endif
        #ifdef GUIENGINE_IMAGE_JPEG
            ret = rtgui_image_jpeg_init();
            if (RT_EOK != ret) break;
            LOG_D("JPEG init");
        #endif
        #if defined(GUIENGINE_IMAGE_PNG) || defined(GUIENGINE_IMAGE_LODEPNG)
            rtgui_image_png_init();
        #endif
        #ifdef GUIENGINE_IMAGE_CONTAINER
            /* initialize image container */
            rtgui_system_image_container_init();
        #endif
    } while (0);

    return ret;
}

static rtgui_image_engine_t *rtgui_image_get_engine(const char *type) {
    rt_slist_t *node;

    rt_slist_for_each(node, &_rtgui_system_image_list) {
        rtgui_image_engine_t *engine;

        engine = rt_slist_entry(node, rtgui_image_engine_t, list);
        if (!rt_strncmp(engine->name, type, rt_strlen(engine->name)))
            return engine;
    }

    LOG_E("no engine for %s", type);
    return RT_NULL;
}

#ifdef GUIENGINE_USING_DFS_FILERW

rtgui_image_engine_t *rtgui_image_get_engine_by_filename(const char *fn) {
    rt_slist_t *node;
    const char *ext;

    ext = fn + rt_strlen(fn);
    while (ext != fn)  {
        if (*ext == '.') {
            ext++;
            break;
        }
        ext--;
    }
    if (ext == fn) return RT_NULL;  /* no ext */

    rt_slist_for_each(node, &_rtgui_system_image_list) {
        rtgui_image_engine_t *engine;

        engine = rt_slist_entry(node, rtgui_image_engine_t, list);
        if (!rt_strncmp(engine->name, ext, rt_strlen(engine->name)))
            return engine;
    }

    return RT_NULL;
}
RTM_EXPORT(rtgui_image_get_engine_by_filename);

rtgui_image_t *rtgui_image_create_from_file(const char *type,
    const char *filename, rt_bool_t load) {
    rtgui_filerw_t *filerw;
    rtgui_image_engine_t *engine;
    rtgui_image_t *image = RT_NULL;

    /* create filerw context */
    filerw = rtgui_filerw_create_file(filename, "rb");
    if (!filerw) return RT_NULL;

    /* get image engine */
    engine = rtgui_image_get_engine(type);
    if (!engine) {
        /* close filerw context */
        rtgui_filerw_close(filerw);
        return RT_NULL;
    }

    if (engine->image_check(filerw)) {
        image = (rtgui_image_t *)rtgui_malloc(sizeof(rtgui_image_t));
        if (!image) {
            /* close filerw context */
            rtgui_filerw_close(filerw);
            return RT_NULL;
        }

        image->palette = RT_NULL;
        if (!engine->image_load(image, filerw, load)) {
            /* close filerw context */
            rtgui_filerw_close(filerw);
            return RT_NULL;
        }
        /* set image engine */
        image->engine = engine;
    } else {
        rtgui_filerw_close(filerw);
    }

    return image;
}
RTM_EXPORT(rtgui_image_create_from_file);

rtgui_image_t *rtgui_image_create(const char *filename, rt_bool_t load)
{
    rtgui_filerw_t *filerw;
    rtgui_image_engine_t *engine;
    rtgui_image_t *image = RT_NULL;

    /* create filerw context */
    filerw = rtgui_filerw_create_file(filename, "rb");
    if (filerw == RT_NULL)
    {
        //rt_kprintf("create filerw failed!\n");
        return RT_NULL;
    }

    /* get image engine */
    engine = rtgui_image_get_engine_by_filename(filename);
    if (engine == RT_NULL)
    {
        rt_kprintf("no engine for file: %s\n", filename);
        /* close filerw context */
        rtgui_filerw_close(filerw);
        return RT_NULL;
    }

    if (engine->image_check(filerw) == RT_TRUE)
    {
        image = (rtgui_image_t *) rtgui_malloc(sizeof(rtgui_image_t));
        if (image == RT_NULL)
        {
            rt_kprintf("out of memory\n");
            /* close filerw context */
            rtgui_filerw_close(filerw);
            return RT_NULL;
        }

        image->palette = RT_NULL;
        if (engine->image_load(image, filerw, load) != RT_TRUE)
        {
            rt_kprintf("load image:%s failed!\n", filename);
            /* close filerw context */
            rtgui_filerw_close(filerw);
            return RT_NULL;
        }

        /* set image engine */
        image->engine = engine;
    }
    else
    {
        rt_kprintf("image:%s check failed!\n", filename);
        rtgui_filerw_close(filerw);
    }

    return image;
}
RTM_EXPORT(rtgui_image_create);

#endif /* GUIENGINE_USING_DFS_FILERW */

rtgui_image_t *rtgui_image_create_from_mem(const char *type,
    const rt_uint8_t *data, rt_size_t length, rt_bool_t load) {
    rtgui_filerw_t *filerw;
    rtgui_image_engine_t *engine;
    rtgui_image_t *image = RT_NULL;

    /* create filerw context */
    filerw = rtgui_filerw_create_mem(data, length);
    if (filerw == RT_NULL) return RT_NULL;

    /* get image engine */
    engine = rtgui_image_get_engine(type);
    if (engine == RT_NULL) {
        /* close filerw context */
        rtgui_filerw_close(filerw);
        LOG_E("no engine for %s", type);
        return RT_NULL;
    }

    if (engine->image_check(filerw) == RT_TRUE) {
        image = (rtgui_image_t *) rtgui_malloc(sizeof(rtgui_image_t));
        if (image == RT_NULL) {
            /* close filerw context */
            rtgui_filerw_close(filerw);
            return RT_NULL;
        }

        image->palette = RT_NULL;
        if (engine->image_load(image, filerw, load) != RT_TRUE) {
            /* close filerw context */
            rtgui_filerw_close(filerw);
            LOG_E("%s load err", type);
            return RT_NULL;
        }

        /* set image engine */
        image->engine = engine;
    } else {
        rtgui_filerw_close(filerw);
        LOG_E("%s check err", type);
    }

    return image;
}
RTM_EXPORT(rtgui_image_create_from_mem);

void rtgui_image_destroy(rtgui_image_t *image)
{
    RT_ASSERT(image != RT_NULL);

    image->engine->image_unload(image);
    if (image->palette != RT_NULL)
        rtgui_free(image->palette);
    rtgui_free(image);
}
RTM_EXPORT(rtgui_image_destroy);

/* register an image engine */
rt_err_t rtgui_image_register_engine(rtgui_image_engine_t *engine) {
    if (!engine) return -RT_EINVAL;

    rt_slist_append(&_rtgui_system_image_list, &(engine->list));
    return RT_EOK;
}
RTM_EXPORT(rtgui_image_register_engine);

void rtgui_image_blit(rtgui_image_t *img, rtgui_dc_t *dc,
    rtgui_rect_t *rect) {
    rtgui_rect_t dc_rect;

    RT_ASSERT(dc != RT_NULL);

    if (!rtgui_dc_get_visible(dc)) return;

    rtgui_dc_get_rect(dc, &dc_rect);

    /* use rect of DC */
    if (!rect) {
        rect = &dc_rect;
    } else {
        /* Don't modify x1, y1, they are handled in engine->image_blit. */
        if (rect->x1 > dc_rect.x2) return;
        if (rect->y1 > dc_rect.y2) return;
        if (rect->x2 > dc_rect.x2) rect->x2 = dc_rect.x2;
        if (rect->y2 > dc_rect.y2) rect->y2 = dc_rect.y2;
    }

    if (img && img->engine) {
        /* use img engine to blit */
        img->engine->image_blit(img, dc, rect);
    }
}
RTM_EXPORT(rtgui_image_blit);

rtgui_image_palette_t *rtgui_image_palette_create(rt_uint32_t ncolors) {
    rtgui_image_palette_t *palette = RT_NULL;

    if (ncolors > 0) {
        palette = rtgui_malloc(
            sizeof(rtgui_image_palette_t) + sizeof(rtgui_color_t) * ncolors);
        if (palette)
            palette->colors = (rtgui_color_t *)(palette + 1);
    }

    return palette;
}
RTM_EXPORT(rtgui_image_palette_create);

void rtgui_image_get_rect(rtgui_image_t *img, rtgui_rect_t *rect)
{
    RT_ASSERT(img != RT_NULL);
    RT_ASSERT(rect  != RT_NULL);

    rect->x1 = 0;
    rect->y1 = 0;
    rect->x2 = img->w;
    rect->y2 = img->h;
}
RTM_EXPORT(rtgui_image_get_rect);
