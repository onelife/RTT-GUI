/*
 * File      : button.h
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
 * 2019-06-18     onelife      refactor
 */
#ifndef __RTGUI_BUTTON_H__
#define __RTGUI_BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

/**
 * @defgroup rtgui_button
 * @{
 */

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _BUTTON_METADATA                    CLASS_METADATA(button)
#define IS_BUTTON(obj)                      IS_INSTANCE(obj, _BUTTON_METADATA)
#define TO_BUTTON(obj)                      CAST_(obj, _BUTTON_METADATA, rtgui_button_t)

#define CREATE_BUTTON_INSTANCE(obj, hdl, _type, text) \
    do {                                    \
        obj = (rtgui_button_t *)CREATE_INSTANCE(button, hdl); \
        if (!obj) break;                    \
        if (rtgui_label_init(TO_LABEL(obj), text)) { \
            DELETE_INSTANCE(obj);           \
            break;                          \
        }                                   \
        WIDGET_TEXTALIGN(obj) = RTGUI_ALIGN_CENTER; \
        BUTTON_FLAG_SET(obj, _type);        \
    } while (0)

#define DELETE_BUTTON_INSTANCE(obj)         DELETE_INSTANCE(obj)

#define BUTTON_FLAG(b)                      (TO_BUTTON(b)->flag)
#define BUTTON_FLAG_CLEAR(b, fname)         BUTTON_FLAG(b) &= ~RTGUI_BUTTON_FLAG_##fname
#define BUTTON_FLAG_SET(b, fname)           BUTTON_FLAG(b) |= RTGUI_BUTTON_FLAG_##fname
#define IS_BUTTON_FLAG(b, fname)            (BUTTON_FLAG(b) & RTGUI_BUTTON_FLAG_##fname)

/* Exported types ------------------------------------------------------------*/
typedef enum rtgui_button_flag {
    RTGUI_BUTTON_FLAG_TYPE_NORMAL           = 0x00,
    RTGUI_BUTTON_FLAG_TYPE_PUSH             = 0x10,
    RTGUI_BUTTON_FLAG_PRESS                 = 0x01,
    RTGUI_BUTTON_FLAG_DEFAULT               = 0x02,
} rtgui_button_flag_t;

struct rtgui_button {
    rtgui_label_t _super;
    rt_uint16_t flag;
    rtgui_image_t *press_img;
    rtgui_image_t *unpress_img;
    rtgui_evt_hdl_t on_button;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(button);

#undef __RTGUI_BUTTON_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
#define rtgui_button_get_text(obj)          \
    MEMBER_GETTER(label, text)(TO_LABEL(obj))
#define rtgui_button_set_text(obj, _text)   \
    MEMBER_SETTER(label, text)(TO_LABEL(obj), _text)
MEMBER_SETTER_PROTOTYPE(rtgui_button_t, button, rtgui_image_t*, press_img);
MEMBER_SETTER_PROTOTYPE(rtgui_button_t, button, rtgui_image_t*, unpress_img);
/** 
 * If the btn is a push button, the callback will be invoked every
 * time the btn got "pushed", i.e., both pressed down @em and pressed
 * up.  If the button is a normal button, the callback will be invoked when
 * the btn got "clicked", i.e., when pressed up @em after pressed
 * down.
 *
 * @param btn the btn that the callback will be setted on.
 * @param evt_hdl the callback function.
 */
MEMBER_SETTER_PROTOTYPE(rtgui_button_t, button, rtgui_evt_hdl_t, on_button);

#endif /* IMPORT_TYPES */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_BUTTON_H__ */
