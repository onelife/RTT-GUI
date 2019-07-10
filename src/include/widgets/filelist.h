/*
 * File      : filelist.h
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
 * 2010-01-06     Bernard      first version
 * 2019-07-04     onelife      refactor
 */
#ifndef __RTGUI_FILELIST_H__
#define __RTGUI_FILELIST_H__

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define _FILELIST_METADATA                  CLASS_METADATA(filelist)
#define IS_FILELIST(obj)                    IS_INSTANCE(obj, _FILELIST_METADATA)
#define TO_FILELIST(obj)                    CAST_(obj, _FILELIST_METADATA, rtgui_filelist_t)

#define CREATE_FILELIST_INSTANCE(obj, dir, rect) \
    do {                                    \
        obj = (rtgui_filelist_t *)CREATE_INSTANCE(filelist, SUPER_CLASS_HANDLER(filelist)); \
        if (!obj) break;                    \
        rtgui_widget_set_rect(TO_WIDGET(obj), rect); \
        rtgui_filelist_set_dir(obj, dir);   \
    } while (0)

#define DELETE_FILELIST_INSTANCE(obj)       DELETE_INSTANCE(obj)

#define FILELIST_SETTER(mname)              rtgui_filelist_set_##mname

/* Exported types ------------------------------------------------------------*/
typedef struct rtgui_filelist rtgui_filelist_t;

struct rtgui_filelist {
    rtgui_list_t _super;
    /* PRIVATE */
    char *cur_dir;
    // char *pattern;
    rtgui_evt_hdl_t on_file;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(filelist);

#undef __RTGUI_FILELIST_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
void rtgui_filelist_set_dir(rtgui_filelist_t *filelist, const char *dir);
MEMBER_GETTER_PROTOTYPE(rtgui_filelist_t, filelist, char*, cur_dir);
MEMBER_SETTER_PROTOTYPE(rtgui_filelist_t, filelist, rtgui_evt_hdl_t, on_file);

#endif /* IMPORT_TYPES */

#endif /* __RTGUI_FILELIST_H__ */
