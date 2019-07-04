/*
 * File      : title.h
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
 * 2019-05-18     onelife      refactor
 */
#ifndef __RTGUI_TITLE_H__
#define __RTGUI_TITLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _TITLE_METADATA                     CLASS_METADATA(title)
#define IS_TITLE(obj)                       IS_INSTANCE((obj), _TITLE_METADATA)
#define TO_TITLE(obj)                       CAST_(obj, _TITLE_METADATA, rtgui_title_t)

#define TITLE_HEIGHT                        (20)
#define TITLE_BORDER_SIZE                   (2)
#define TITLE_CLOSE_BUTTON_WIDTH            (16)
#define TITLE_CLOSE_BUTTON_HEIGHT           (16)

/* Exported types ------------------------------------------------------------*/
struct rtgui_title {
    rtgui_widget_t _super;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(title);

#undef __RTGUI_TITLE_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_TITLE_H__ */
