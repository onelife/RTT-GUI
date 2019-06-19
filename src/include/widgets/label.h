/*
 * File      : label.h
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
#ifndef __RTGUI_LABEL_H__
#define __RTGUI_LABEL_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

/* Exported defines ----------------------------------------------------------*/
#define CREATE_LABEL_INSTANCE(obj, hdl, text) \
    do {                                    \
        obj = (rtgui_label_t *)CREATE_INSTANCE(label, hdl); \
        if (!obj) break;                    \
        if (rtgui_label_init(obj, text))    \
            DELETE_INSTANCE(obj);           \
    } while (0)

#define DELETE_LABEL_INSTANCE(obj)          DELETE_INSTANCE(obj)

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rt_err_t *rtgui_label_init(rtgui_label_t *lab, const char *text);
void rtgui_label_set_text(rtgui_label_t *lab, const char *text);
MEMBER_GETTER_PROTOTYPE(rtgui_label_t, label, char*, text);
void rtgui_theme_draw_label(rtgui_label_t *lab);

#endif /* __RTGUI_LABEL_H__ */
