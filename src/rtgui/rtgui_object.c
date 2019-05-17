/*
 * File      : rtgui_obj.c
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

#include "../include/rtgui_object.h"
#include "../include/rtgui_system.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_OBJ"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */


static void _rtgui_object_constructor(rtgui_obj_t *object) {
    if (!object)
        return;

    object->flag = RTGUI_OBJECT_FLAG_VALID;
    object->id   = (rt_ubase_t)object;
}

/* Destroys the object */
static void _rtgui_object_destructor(rtgui_obj_t *object) {
    /* Any valid objest should both have valid flag _and_ valid type. Only use
     * flag is not enough because the chunk of memory may be reallocted to other
     * object and thus the flag will become valid. */
    object->flag = RTGUI_OBJECT_FLAG_NONE;
    object->type = RT_NULL;
}

DEFINE_CLASS_TYPE(
    object, "object",
    RT_NULL,
    _rtgui_object_constructor,
    _rtgui_object_destructor,
    sizeof(struct rtgui_obj));
RTM_EXPORT(_rtgui_object);

void rtgui_type_object_construct(const rtgui_type_t *type,
    rtgui_obj_t *obj) {
    /* construct from parent to children */
    if (RT_NULL != type->parent) {
        rtgui_type_object_construct(type->parent, obj);
    }

    if (type->constructor) {
        type->constructor(obj);
    }
}

void rtgui_type_destructors_call(const rtgui_type_t *type,
    rtgui_obj_t *object) {
    /* destruct from children to parent */
    if (type->destructor) {
        type->destructor(object);
    }

    if (type->parent) {
        rtgui_type_destructors_call(type->parent, object);
    }
}

rt_bool_t rtgui_type_inherits_from(const rtgui_type_t *type,
    const rtgui_type_t *parent) {
    const rtgui_type_t *t;

    t = type;
    while (t)     {
        if (t == parent) return RT_TRUE;
        t = t->parent;
    }
    return RT_FALSE;
}

const rtgui_type_t *rtgui_type_parent_type_get(const rtgui_type_t *type) {
    return type->parent;
}

const char *rtgui_type_name_get(const rtgui_type_t *type) {
    if (!type) return RT_NULL;

    return type->name;
}

#ifdef RTGUI_OBJECT_TRACE
    struct rtgui_obj_info {
        rt_uint32_t objs_num;
        rt_uint32_t alloc_size;
        rt_uint32_t max_alloc;
    };
    struct rtgui_obj_info obj_info = {0, 0, 0};
#endif

/**
 * @brief Creates a new object: it calls the corresponding constructors
 * (from the constructor of the base class to the constructor of the more
 * derived class) and then sets the values of the given properties
 *
 * @param type the type of object to create
 * @return the created object
 */
rtgui_obj_t *rtgui_object_create(const rtgui_type_t *type) {
    rtgui_obj_t *new_obj;

    if (!type) return RT_NULL;

    new_obj = rtgui_malloc(type->size);
    if (!new_obj) {
        LOG_E("create %s mem err", type->name);
        return RT_NULL;
    }

    new_obj->type = type;
    rtgui_type_object_construct(type, new_obj);
    LOG_D("create %s", type->name);

    #ifdef RTGUI_OBJECT_TRACE
        obj_info.objs_num++;
        obj_info.alloc_size += type->size;
        if (obj_info.alloc_size > obj_info.max_alloc) {
            obj_info.max_alloc = obj_info.alloc_size;
        }
    #endif

    return new_obj;
}
RTM_EXPORT(rtgui_object_create);

/**
 * @brief Destroys the object.
 *
 * The object destructors will be called in inherited type order.
 *
 * @param obj the object to destroy
 */
void rtgui_object_destroy(rtgui_obj_t *obj) {
    if (!obj || (obj->flag & RTGUI_OBJECT_FLAG_STATIC)) return;

    #ifdef RTGUI_OBJECT_TRACE
        obj_info.objs_num --;
        obj_info.alloc_size -= obj->type->size;
    #endif
    RT_ASSERT(obj->type != RT_NULL);
    LOG_D("destroy %d", obj->type);

    /* call destructor */
    rtgui_type_destructors_call(obj->type, obj);

    /* release object */
    rtgui_free(obj);
}
RTM_EXPORT(rtgui_object_destroy);

/**
 * @brief Checks if the object can be cast to the specified type.
 *
 * If the object doesn't inherit from the specified type, a warning
 * is displayed in the console but the object is returned anyway.
 *
 * @param object the object to cast
 * @param type the type to which we cast the object
 * @return Returns the object
 */
rtgui_obj_t *rtgui_object_check_cast(rtgui_obj_t *obj,
    const rtgui_type_t *type, const char *func, int line) {
    if (!obj) return RT_NULL;

    if (!rtgui_type_inherits_from(obj->type, type)) {
        LOG_E("%s[%d] Invalid cast from \"%s\" to \"%s\"", func, line,
            rtgui_type_name_get(obj->type), rtgui_type_name_get(type));
    }

    return obj;
}
RTM_EXPORT(rtgui_object_check_cast);


/**
 * @brief Gets the type of the object
 *
 * @param object an object
 * @return the type of the object (RT_NULL on failure)
 */
const rtgui_type_t *rtgui_object_object_type_get(rtgui_obj_t *obj) {
    if (!obj) return RT_NULL;

    return obj->type;
}
RTM_EXPORT(rtgui_object_object_type_get);

void rtgui_object_set_event_handler(struct rtgui_obj *obj,
    rtgui_evt_hdl_p hdr) {
    RT_ASSERT(obj != RT_NULL);

    obj->event_handler = hdr;
}
RTM_EXPORT(rtgui_object_set_event_handler);

rt_bool_t rtgui_object_event_handler(struct rtgui_obj *obj,
    union rtgui_evt_generic *evt) {
    (void)obj;
    (void)evt;

    return RT_FALSE;
}
RTM_EXPORT(rtgui_object_event_handler);

void rtgui_object_set_id(struct rtgui_obj *obj, rt_uint32_t id) {
    #ifdef RTGUI_USING_ID_CHECK
        struct rtgui_obj *_obj = rtgui_get_self_object(id);
        RT_ASSERT(!_obj);
    #endif

    obj->id = id;
}
RTM_EXPORT(rtgui_object_set_id);

rt_uint32_t rtgui_object_get_id(struct rtgui_obj *obj) {
    return obj->id;
}
RTM_EXPORT(rtgui_object_get_id);
