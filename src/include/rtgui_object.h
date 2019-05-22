/*
 * File      : rtgui_obj.h
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
#ifndef __RTGUI_OBJECT_H__
#define __RTGUI_OBJECT_H__

#include "./rtgui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* rtgui object */
void *rtgui_create_instance(const rtgui_type_t *cls, rtgui_evt_hdl_t evt_hdl);
void rtgui_delete_instance(void *_obj);
rt_bool_t rtgui_is_subclass_of(const rtgui_type_t *cls,
    const rtgui_type_t *_super);
const rtgui_type_t *rtgui_class_of(void *_obj);
void *rtgui_object_cast_check(void *_obj, const rtgui_type_t *cls,
    const char *func, int line);
const rtgui_type_t *rtgui_object_type_get(void *_obj);

#ifdef GUIENGIN_USING_CAST_CHECK
# define RTGUI_CAST(obj, cls, _type)         \
    ((_type *)rtgui_object_cast_check(obj, cls, __FUNCTION__, __LINE__))
#else
# define RTGUI_CAST(obj, cls, _type)        ((_type *)(obj))
#endif

#define IS_INSTANCE(ins, _cls)              \
    rtgui_is_subclass_of(((rtgui_obj_t *)ins)->cls, _cls)


#define _OBJECT_METADATA                    CLASS_METADATA(object)
#define TO_OBJECT(obj)                      \
    RTGUI_CAST(obj, _OBJECT_METADATA, rtgui_obj_t)
#define IS_OBJECT(obj)                      IS_INSTANCE(obj, _OBJECT_METADATA)

RTGUI_CLASS_PROTOTYPE(object);

// void rtgui_object_set_id(rtgui_obj_t *obj, rt_uint32_t id);
// rt_uint32_t rtgui_object_get_id(rtgui_obj_t *obj);
// rtgui_obj_t* rtgui_get_object(rtgui_app_t *app, rt_uint32_t id);
// rtgui_obj_t* rtgui_get_self_object(rt_uint32_t id);

#ifdef __cplusplus
}
#endif

#endif

