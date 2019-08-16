/*
 * File      : rtgui.h
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
 * 2009-10-04     Bernard      first version
 * 2019-05-17     onelife      refactor
 */
#ifndef __RTGUI_H__
#define __RTGUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTGUI_RESTRICT                      __restrict
#ifdef _MSC_VER
# define RTGUI_PURE
#else
# define RTGUI_PURE                         __attribute__((pure))
#endif

/* Includes ------------------------------------------------------------------*/
#include "guiconfig.h"
#include "include/types.h"
#include "include/region.h"
#include "include/class.h"
#include "include/widgets/widget.h"
#include "include/arch.h"
#include "include/dc.h"
#include "include/driver.h"
#include "include/color.h"

/* Exported defines ----------------------------------------------------------*/
#define _MIN(x, y)                          (((x) < (y)) ? (x) : (y))
#define _MAX(x, y)                          (((x) > (y)) ? (x) : (y))

#define _BIT2BYTE(bits)                     ((bits + 7) >> 3)
#define _ABS(x)                             (((x) >= 0) ? (x) : -(x))

#define RECT_W(r)                           ((r).x2 - (r).x1 + 1)
#define RECT_H(r)                           ((r).y2 - (r).y1 + 1)
#define IS_RECT_NO_SIZE(r)                  \
    (((r).x1 == (r).x2) && ((r).y1 == (r).y2))
#define RECT_CLEAR(r)                       \
    rt_memset(&r, 0x00, sizeof(rtgui_rect_t))

#define WIDGET_DEFAULT_BORDER               (2)
#define WIDGET_DEFAULT_MARGIN               (3)
#define SELECTED_ITEM_HEIGHT                (25)

/* Global variables ----------------------------------------------------------*/
extern struct rt_mempool *rtgui_event_pool;
extern rtgui_point_t rtgui_empty_point;

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/* server */
REFERENCE_GETTER_PROTOTYPE(server, rtgui_app_t);
SETTER_PROTOTYPE(server_show_win_hook, rtgui_hook_t);
SETTER_PROTOTYPE(server_act_win_hook, rtgui_hook_t);
rt_err_t rtgui_server_init(void);
rt_err_t rtgui_send_request(rtgui_evt_generic_t *evt, rt_int32_t timeout);
rt_err_t rtgui_send_request_sync(rtgui_evt_generic_t *evt);

/* theme */
#define _theme_selected_item_height()   (rt_uint16_t)SELECTED_ITEM_HEIGHT

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_H__ */
