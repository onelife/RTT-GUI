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


#include "include/rtthread.h"
#include "./rtgui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* rtgui object */
#define RTGUI_CONTAINER_OF(obj, type, member) \
    ((type *)((char *)(obj) - (unsigned long)(&((type *)0)->member)))
#define RTGUI_CONSTRUCTOR(constructor) ((rtgui_constructor_t)(constructor))
#define RTGUI_DESTRUCTOR(destructor) ((rtgui_destructor_t)(destructor))

/* pre-definition */
struct rtgui_app;




#define RTGUI_TYPE(type)            (_rtgui_##type##_get_type())
#define RTGUI_PARENT_TYPE(type)     (const rtgui_type_t *)(&_rtgui_##type)

#define DECLARE_CLASS_TYPE(type)    \
    const rtgui_type_t *_rtgui_##type##_get_type(void); \
    extern const struct rtgui_type _rtgui_##type


#define DEFINE_CLASS_TYPE(type, name, parent, constructor, destructor, size) \
    const struct rtgui_type _rtgui_##type = { \
        name, \
        parent, \
        RTGUI_CONSTRUCTOR(constructor), \
        RTGUI_DESTRUCTOR(destructor), \
        size, \
    }; \
    const rtgui_type_t *_rtgui_##type##_get_type(void) { \
        return &_rtgui_##type; \
    } \
    RTM_EXPORT(_rtgui_##type##_get_type)


void rtgui_type_object_construct(const rtgui_type_t *type, rtgui_obj_t *obj);
void rtgui_type_destructors_call(const rtgui_type_t *type, rtgui_obj_t *obj);
rt_bool_t rtgui_type_inherits_from(const rtgui_type_t *type, const rtgui_type_t *parent);
const rtgui_type_t *rtgui_type_parent_type_get(const rtgui_type_t *type);
const char *rtgui_type_name_get(const rtgui_type_t *type);
const rtgui_type_t *rtgui_object_object_type_get(rtgui_obj_t *obj);

#ifdef GUIENGIN_USING_CAST_CHECK
# define RTGUI_OBJECT_CAST(obj, type, c_type) \
    ((c_type *)rtgui_object_check_cast( \
        (rtgui_obj_t *)(obj), (type), __FUNCTION__, __LINE__))
#else
# define RTGUI_OBJECT_CAST(obj, type, c_type) ((c_type *)(obj))
#endif

#define RTGUI_OBJECT_CHECK_TYPE(_obj, _type) \
    (rtgui_type_inherits_from(((rtgui_obj_t *)(_obj))->type, (_type)))

DECLARE_CLASS_TYPE(object);
/** Gets the type of an object */
#define RTGUI_OBJECT_TYPE       RTGUI_TYPE(object)
/** Casts the object to an rtgui_obj_t */
#define RTGUI_OBJECT(obj)       (RTGUI_OBJECT_CAST((obj), RTGUI_OBJECT_TYPE, rtgui_obj_t))
/** Checks if the object is an rtgui_Object */
#define RTGUI_IS_OBJECT(obj)    (RTGUI_OBJECT_CHECK_TYPE((obj), RTGUI_OBJECT_TYPE))


rtgui_obj_t *rtgui_object_create(const rtgui_type_t *type);
void rtgui_object_destroy(rtgui_obj_t *object);

/* set the event handler of object */
void rtgui_object_set_event_handler(rtgui_obj_t *object,
    rtgui_evt_hdl_t handler);
/* object default event handler */
rt_bool_t rtgui_object_event_handler(rtgui_obj_t *object,
    rtgui_evt_generic_t *event);

/** handle @param event on @param object's own event handler
 *
 * If the @param object does not have an event handler, which means the object
 * does not interested in any event, it will return RT_FALSE. Otherwise, the
 * return code of that handler is returned.
 */
rt_inline rt_bool_t rtgui_object_handle(rtgui_obj_t *obj,
    rtgui_evt_generic_t *evt) {
    if (obj->event_handler) {
        return obj->event_handler(obj, evt);
    }
    return RT_FALSE;
}

rtgui_obj_t *rtgui_object_check_cast(rtgui_obj_t *object,
    const rtgui_type_t *type, const char *func, int line);
const rtgui_type_t *rtgui_object_object_type_get(rtgui_obj_t *object);

void rtgui_object_set_id(rtgui_obj_t *obj, rt_uint32_t id);
rt_uint32_t rtgui_object_get_id(rtgui_obj_t *obj);
rtgui_obj_t* rtgui_get_object(struct rtgui_app *app, rt_uint32_t id);
rtgui_obj_t* rtgui_get_self_object(rt_uint32_t id);

#ifdef __cplusplus
}
#endif

#endif

