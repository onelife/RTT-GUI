/*
 * File      : rtgui_app.h
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
 * 2012-01-13     Grissiom     first version
 * 2019-05-15     onelife      refactor and rename to "app.h"
 */

#ifndef __RTGUI_CLASS_H__
#define __RTGUI_CLASS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifdef IMPORT_TYPES

/* Exported defines ----------------------------------------------------------*/
#define CLASS_METADATA(name)                (&_rtgui_##name)
#define CLASS_PROTOTYPE(name)               extern const struct rtgui_class _rtgui_##name
#define RTGUI_CLASS(name, _super, ctor, dtor, hdl, size) \
    const struct rtgui_class _rtgui_##name = { \
        _super,                             \
        (rtgui_constructor_t)(ctor),        \
        (rtgui_destructor_t)(dtor),         \
        (rtgui_evt_hdl_t)(hdl),             \
        #name,                              \
        size,                               \
    }
#define CREATE_INSTANCE(name, hdl)          rtgui_create_instance(CLASS_METADATA(name), (rtgui_evt_hdl_t)(hdl))
#define DELETE_INSTANCE(obj)                rtgui_delete_instance(obj)

#ifdef RTGUI_CASTING_CHECK
# define CAST_(obj, cls, _type)             ((_type *)rtgui_object_cast_check(obj, cls, __FUNCTION__, __LINE__))
#else
# define CAST_(obj, cls, _type)             ((_type *)(obj))
#endif

#define SUPER_(obj)                         (obj->_super)
#define CLASS_(ins)                         rtgui_class_of((void *)ins)
#define IS_SUBCLASS(cls, _super)            rtgui_is_subclass_of(cls, _super)
#define IS_INSTANCE(ins, _cls)              rtgui_is_subclass_of(((rtgui_obj_t *)ins)->cls, _cls)
#define SUPER_CLASS(name)                   (CLASS_METADATA(name)->_super)

/* object is the base class of all other classes */
#define _OBJECT_METADATA                    CLASS_METADATA(object)
#define IS_OBJECT(obj)                      IS_INSTANCE(obj, _OBJECT_METADATA)
#define TO_OBJECT(obj)                      CAST_(obj, _OBJECT_METADATA, rtgui_obj_t)

/* Exported types ------------------------------------------------------------*/
/* class metadata */
struct rtgui_class {
    const struct rtgui_class *_super;
    rtgui_constructor_t constructor;
    rtgui_destructor_t destructor;
    rtgui_evt_hdl_t evt_hdl;
    char *name;
    rt_size_t size;
};

typedef enum rtgui_obj_flag {
    RTGUI_OBJECT_FLAG_NONE                  = 0x0000,
    RTGUI_OBJECT_FLAG_STATIC                = 0x0001,
    RTGUI_OBJECT_FLAG_DISABLED              = 0x0002,
    RTGUI_OBJECT_FLAG_VALID                 = 0xBEEF,
} rtgui_obj_flag_t;

/* object */
struct rtgui_obj {
    rt_uint32_t _super;
    const rtgui_class_t *cls;
    rtgui_evt_hdl_t evt_hdl;
    rtgui_obj_flag_t flag;
    rt_ubase_t id;
};

/* Exported constants --------------------------------------------------------*/
CLASS_PROTOTYPE(object);

#undef __RTGUI_CLASS_H__
#else /* IMPORT_TYPES */

/* Exported functions ------------------------------------------------------- */
void *rtgui_create_instance(const rtgui_class_t *cls, rtgui_evt_hdl_t evt_hdl);
void rtgui_delete_instance(void *_obj);
rt_bool_t rtgui_is_subclass_of(const rtgui_class_t *cls,
    const rtgui_class_t *_super);
const rtgui_class_t *rtgui_class_of(void *_obj);
void *rtgui_object_cast_check(void *_obj, const rtgui_class_t *cls,
    const char *func, int line);

#endif /* IMPORT_TYPES */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_CLASS_H__ */
