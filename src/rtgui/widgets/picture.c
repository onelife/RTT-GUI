/*
 * File      : picture.c
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
 * 2019-08-19     onelife      first version
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/image.h"
#include "include/widgets/container.h"
#include "include/widgets/picture.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "WGT_PIC"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_W                      LOG_E
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void _picture_constructor(void *obj);
static void _picture_destructor(void *obj);
static rt_bool_t _picture_event_handler(void *obj, rtgui_evt_generic_t *evt);
static void _theme_draw_picture(rtgui_picture_t *pic);

/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    picture,
    CLASS_METADATA(widget),
    _picture_constructor,
    _picture_destructor,
    _picture_event_handler,
    sizeof(rtgui_picture_t));

/* Private functions ---------------------------------------------------------*/
static void _picture_constructor(void *obj) {
    rtgui_picture_t *pic = obj;

    WIDGET_TEXTALIGN(pic) = RTGUI_ALIGN_CENTER;
    pic->path = RT_NULL;
    pic->image = RT_NULL;
}

static void _picture_destructor(void *obj) {
    rtgui_picture_t *pic = obj;

    if (pic->path) rtgui_free(pic->path);
    pic->path = RT_NULL;
    if (pic->image) rtgui_image_destroy(pic->image);
    pic->image = RT_NULL;
}

static rt_bool_t _picture_event_handler(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_picture_t *pic = obj;
    rt_bool_t done = RT_FALSE;

    EVT_LOG("[PicEVT] %s @%p from %s", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name);

    switch (evt->base.type) {
    case RTGUI_EVENT_PAINT:
        _theme_draw_picture(pic);
        break;

    default:
        if (SUPER_CLASS_HANDLER(picture))
            done = SUPER_CLASS_HANDLER(picture)(pic, evt);
        break;
    }

    EVT_LOG("[PicEVT] %s @%p from %s done %d", rtgui_event_text(evt), evt,
        evt->base.origin->mb->parent.parent.name, done);
    return done;
}

static void _theme_draw_picture(rtgui_picture_t *pic) {
    do {
        rtgui_dc_t *dc;
        rtgui_rect_t rect1, rect2;

        dc = rtgui_dc_begin_drawing(TO_WIDGET(pic));
        if (!dc) {
            LOG_W("no dc");
            break;
        }

        rtgui_widget_get_rect(TO_WIDGET(pic), &rect1);
        rtgui_dc_fill_rect(dc, &rect1);

        if (pic->image) {
            rtgui_image_get_rect(pic->image, &rect2);
            rtgui_rect_move_align(&rect1, &rect2, pic->align);
            LOG_D("draw picture (%d,%d)-(%d, %d)", rect2.x1, rect2.y1, rect2.x2,
                rect2.y2);
            rtgui_image_blit(pic->image, dc, &rect2);
        }

        rtgui_dc_end_drawing(dc, RT_TRUE);
        LOG_D("draw picture done");
    } while (0);
}

/* Public functions ----------------------------------------------------------*/
rt_err_t *rtgui_picture_init(rtgui_picture_t *pic, rt_uint32_t align,
    rt_bool_t resize) {
    pic->path = RT_NULL;
    pic->image = RT_NULL;
    pic->align = align;
    pic->resize = resize;

    return RT_EOK;
}
RTM_EXPORT(rtgui_picture_init);

rtgui_picture_t *rtgui_create_picture(rtgui_container_t *cntr,
    rtgui_evt_hdl_t hdl, rtgui_rect_t *rect, const char *path,
    rt_uint32_t align, rt_bool_t resize) {
    rtgui_picture_t *pic = RT_NULL;

    do {
        pic = CREATE_INSTANCE(picture, hdl);
        if (!pic) break;
        if (rtgui_picture_init(pic, align, resize)) {
            DELETE_INSTANCE(pic);
            break;
        }
        rtgui_picture_set_path(pic, path);
        if (rect)
            rtgui_widget_set_rect(TO_WIDGET(pic), rect);
        if (cntr)
            rtgui_container_add_child(cntr, TO_WIDGET(pic));
    } while (0);

    return pic;
}

void rtgui_picture_set_path(rtgui_picture_t *pic, const char *path) {
    if (pic->path) {
        /* check if changed */
        if (!rt_strcmp(path, pic->path)) return;
        rtgui_free(pic->path);
        pic->path = RT_NULL;
    }
    if (pic->image) {
        rtgui_image_destroy(pic->image);
        pic->image = RT_NULL;
    }

    if (path) {
        pic->image = rtgui_image_create(path, pic->resize ? 0 : -1, RT_NULL);
        if (!pic->image) return;
        pic->path = rt_strdup(path);
        LOG_D("pic path: %s", pic->path);
    }

    /* update widget */
    rtgui_widget_update(TO_WIDGET(pic));
}
RTM_EXPORT(rtgui_picture_set_path);

RTGUI_MEMBER_GETTER(rtgui_picture_t, picture, char*, path);
