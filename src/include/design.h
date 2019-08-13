/*
 * File      : design.h
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
 * 2019-08-06     onelife      first version
 */
#ifndef __RTGUI_DESIGN_H__
#define __RTGUI_DESIGN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

/* Exported defines ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef rt_err_t (*design_read_t)(char **buf, rt_uint32_t *sz);
typedef struct hdl_tbl_entr hdl_tbl_entr_t;
typedef void* obj_tbl_entr_t;
typedef struct design_contex design_contex_t;

struct hdl_tbl_entr {
    const char *name;
    void *hdl;
};

struct design_contex {
    design_read_t read;
    const hdl_tbl_entr_t *hdl_tbl;
    obj_tbl_entr_t *obj_tbl;
    rt_uint32_t obj_sz;
    char *buf;
    rt_uint32_t pos;
    rt_uint32_t len;
    rtgui_app_t *app;
    rtgui_win_t *win;
};

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
rt_err_t rtgui_design_init(design_contex_t *ctx, design_read_t read,
    const hdl_tbl_entr_t *hdl_tbl, obj_tbl_entr_t *obj_tbl, rt_uint32_t obj_sz);
rt_err_t rtgui_design_parse(design_contex_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_DESIGN_H__ */

