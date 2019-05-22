/*
 * File      : rtgui_server.h
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
 */
#ifndef __RTGUI_SERVER_H__
#define __RTGUI_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

// #include "include/rtservice.h"
#include "./rtgui.h"

/* RTGUI server definitions */
/* top win manager init */
void rtgui_topwin_init(void);
void rtgui_server_init(void);

void rtgui_server_install_show_win_hook(void (*hk)(void));
void rtgui_server_install_act_win_hook(void (*hk)(void));

/* post an event to server */
rt_err_t rtgui_server_post_event(rtgui_evt_generic_t *evt);
rt_err_t rtgui_server_post_event_sync(rtgui_evt_generic_t *evt);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_SERVER_H__ */
