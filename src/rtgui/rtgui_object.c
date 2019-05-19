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

static void _object_constructor(void *obj);
static void _object_destructor(void *obj);


#ifdef RTGUI_OBJECT_TRACE
    struct rtgui_obj_info {
        rt_uint32_t objs_num;
        rt_uint32_t alloc_size;
        rt_uint32_t max_alloc;
    };
    struct rtgui_obj_info obj_info = {0, 0, 0};
#endif

RTGUI_CLASS(
    object,
    RT_NULL,
    _object_constructor,
    _object_destructor,
    sizeof(rtgui_obj_t));
RTM_EXPORT(_rtgui_object);


static void _object_constructor(void *_obj) {
    rtgui_obj_t *obj = _obj;

    if (!obj) return;
    obj->_super = RT_NULL;
    obj->cls = RT_NULL;
    obj->evt_hdl = RT_NULL;
    obj->flag = RTGUI_OBJECT_FLAG_VALID;
    obj->id = (rt_ubase_t)obj;
}

static void _object_destructor(void *_obj) {
    rtgui_obj_t *obj = _obj;

    /* Any valid objest should both have valid flag _and_ valid cls. Only use
     * flag is not enough because the chunk of memory may be reallocted to other
     * object and thus the flag will become valid. */
    obj->cls = RT_NULL;
    obj->flag = RTGUI_OBJECT_FLAG_NONE;
    obj->evt_hdl = RT_NULL;
}


static void _construct_instance(const rtgui_type_t *cls, void *obj) {
    /* call super constructor first */
    if (cls->_super) _construct_instance(cls->_super, obj);
    if (cls->constructor) cls->constructor(obj);
}

static void _deconstruct_instance(const rtgui_type_t *cls, void *obj) {
    /* call self destructor first */
    if (cls->destructor) cls->destructor(obj);
    if (cls->_super) _deconstruct_instance(cls->_super, obj);
}

void *rtgui_create_instance(const rtgui_type_t *cls) {
    rtgui_obj_t *_new;

    _new = rtgui_malloc(cls->size);
    if (!_new) {
        LOG_E("create ins %s err", cls->name);
        return RT_NULL;
    }

    _new->cls = cls;
    _construct_instance(cls, _new);
    LOG_D("create ins %s", cls->name);

    #ifdef RTGUI_OBJECT_TRACE
        obj_info.objs_num++;
        obj_info.alloc_size += cls->size;
        if (obj_info.alloc_size > obj_info.max_alloc) {
            obj_info.max_alloc = obj_info.alloc_size;
        }
    #endif

    return _new;
}

void rtgui_delete_instance(void *_obj) {
    rtgui_obj_t *obj = _obj;

    // if (!obj || (obj->flag & RTGUI_OBJECT_FLAG_STATIC)) return;

    #ifdef RTGUI_OBJECT_TRACE
        obj_info.objs_num--;
        obj_info.alloc_size -= obj->cls->size;
    #endif

    LOG_D("delete ins %d", obj->cls->name);
    _deconstruct_instance(obj->cls, obj);
    rtgui_free(obj);
}

rt_bool_t rtgui_is_subclass_of(const rtgui_type_t *cls,
    const rtgui_type_t *_super) {
    while (cls) {
        if (cls == _super) return RT_TRUE;
        cls = cls->_super;
    }
    return RT_FALSE;
}

const rtgui_type_t *rtgui_class_of(void *_obj) {
    rtgui_obj_t *obj = _obj;
    return obj->cls;
}


/**
 * @brief Checks if the object can be cast to the specified cls.
 *
 * If the object doesn't inherit from the specified cls, a warning
 * is displayed in the console but the object is returned anyway.
 *
 * @param object the object to cast
 * @param cls the cls to which we cast the object
 * @return Returns the object
 */
void *rtgui_object_cast_check(void *_obj, const rtgui_type_t *cls,
    const char *func, int line) {
    rtgui_obj_t *obj = _obj;

    if (!obj) return RT_NULL;

    if (!rtgui_is_subclass_of(obj->cls, cls)) {
        LOG_E("%s[%d] Invalid cast from \"%s\" to \"%s\"", func, line,
            obj->cls->name, cls->name);
    }
    return _obj;
}
RTM_EXPORT(rtgui_object_cast_check);


/**
 * @brief Gets the cls of the object
 *
 * @param object an object
 * @return the cls of the object (RT_NULL on failure)
 */
const rtgui_type_t *rtgui_object_type_get(void *_obj) {
    rtgui_obj_t *obj = _obj;

    if (!obj) return RT_NULL;
    return obj->cls;
}
RTM_EXPORT(rtgui_object_type_get);

void rtgui_object_event_handler_set(void *_obj, rtgui_evt_hdl_t hdl) {
    rtgui_obj_t *obj = _obj;

    obj->evt_hdl = hdl;
}
RTM_EXPORT(rtgui_object_event_handler_set);

rt_bool_t rtgui_object_event_handler(rtgui_obj_t *obj,
    rtgui_evt_generic_t *evt) {
    (void)obj;
    (void)evt;

    return RT_FALSE;
}
RTM_EXPORT(rtgui_object_event_handler);

// void rtgui_object_set_id(rtgui_obj_t *obj, rt_uint32_t id) {
//     #ifdef RTGUI_USING_ID_CHECK
//         rtgui_obj_t *_obj = rtgui_get_self_object(id);
//         RT_ASSERT(!_obj);
//     #endif

//     obj->id = id;
// }
// RTM_EXPORT(rtgui_object_set_id);

// rt_uint32_t rtgui_object_get_id(rtgui_obj_t *obj) {
//     return obj->id;
// }
// RTM_EXPORT(rtgui_object_get_id);
