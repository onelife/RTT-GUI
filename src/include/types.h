/*
 * File      : types.h
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
 * 2019-05-17     onelife      move typedef here
 */
#ifndef __RTGUI_TYPES_H__
#define __RTGUI_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "include/rtthread.h"

/* Exported defines ----------------------------------------------------------*/
#define OBJECT_HANDLER(obj)                 TO_OBJECT((void *)obj)->evt_hdl
#define DEFAULT_HANDLER(obj)                CLASS_(obj)->evt_hdl
#define EVENT_HANDLER(obj)                  (OBJECT_HANDLER(obj) ? OBJECT_HANDLER(obj) : DEFAULT_HANDLER(obj))
#define SUPER_HANDLER(obj)                  (CLASS_(obj)->_super ? CLASS_(obj)->_super->evt_hdl : RT_NULL)
#define SUPER_CLASS_HANDLER(name)           (SUPER_CLASS(name)->evt_hdl)

#define SETTER_PROTOTYPE(name, _type)       void rtgui_set_##name(_type val)
#define GETTER_PROTOTYPE(name, _type)       _type rtgui_get_##name(void)
#define SETTER_GETTER_PROTOTYPE(name, _type) \
    SETTER_PROTOTYPE(name, _type);          \
    GETTER_PROTOTYPE(name, _type)
#define RTGUI_SETTER(name, _type, reso)     \
    void rtgui_set_##name(_type val) {      \
        reso = val;                         \
    }                                       \
    RTM_EXPORT(rtgui_set_##name)
#define RTGUI_GETTER(name, _type, reso)     \
    _type rtgui_get_##name(void) {          \
        return reso;                        \
    }                                       \
    RTM_EXPORT(rtgui_get_##name)
#define RTGUI_SETTER_GETTER(name, _type, reso) \
    RTGUI_SETTER(name, _type, reso);        \
    RTGUI_GETTER(name, _type, reso)

#define STRUCT_SETTER_PROTOTYPE(name, _type) \
    void rtgui_set_##name(_type *val)
#define STRUCT_GETTER_PROTOTYPE(name, _type) \
    void rtgui_get_##name(_type *val)
#define STRUCT_SETTER_GETTER_PROTOTYPE(name, _type) \
    STRUCT_SETTER_PROTOTYPE(name, _type);   \
    STRUCT_GETTER_PROTOTYPE(name, _type)
#define RTGUI_STRUCT_SETTER(name, _type, reso) \
    void rtgui_set_##name(_type *val) {     \
        reso = *val;                        \
    }                                       \
    RTM_EXPORT(rtgui_set_##name)
#define RTGUI_STRUCT_GETTER(name, _type, reso)     \
    void rtgui_get_##name(_type *val) {     \
        *val = reso;                        \
    }                                       \
    RTM_EXPORT(rtgui_get_##name)
#define RTGUI_STRUCT_SETTER_GETTER(name, _type, reso) \
    RTGUI_STRUCT_SETTER(name, _type, reso);        \
    RTGUI_STRUCT_GETTER(name, _type, reso)

#define REFERENCE_SETTER_PROTOTYPE(name, _type) \
    _type *rtgui_set_##name(void)
#define REFERENCE_GETTER_PROTOTYPE(name, _type) \
    _type *rtgui_get_##name(void)
#define RTGUI_REFERENCE_SETTER(name, _type, ref) \
    void rtgui_set_##name(_type *val) {     \
        ref = val;                          \
    }                                       \
    RTM_EXPORT(_type *rtgui_set_##name)
#define RTGUI_REFERENCE_GETTER(name, _type, ref) \
    _type *rtgui_get_##name(void) {         \
        return ref;                         \
    }
    // RTM_EXPORT(_type *rtgui_get_##name)
#define GETTER(name)                        rtgui_get_##name

#define MEMBER_SETTER_PROTOTYPE(ctype, cname, mtype, mname) \
    void rtgui_##cname##_set_##mname(ctype *obj, mtype val)
#define MEMBER_GETTER_PROTOTYPE(ctype, cname, mtype, mname) \
    mtype rtgui_##cname##_get_##mname(ctype *obj)
#define MEMBER_SETTER_GETTER_PROTOTYPE(ctype, cname, mtype, mname) \
    MEMBER_SETTER_PROTOTYPE(ctype, cname, mtype, mname); \
    MEMBER_GETTER_PROTOTYPE(ctype, cname, mtype, mname)
#define RTGUI_MEMBER_SETTER(ctype, cname, mtype, mname) \
    void rtgui_##cname##_set_##mname(ctype *obj, mtype val) { \
        if (obj) obj->mname = val;          \
    }                                       \
    RTM_EXPORT(rtgui_##cname##_set_##mname)
#define RTGUI_MEMBER_GETTER(ctype, cname, mtype, mname) \
    mtype rtgui_##cname##_get_##mname(ctype *obj) { \
        RT_ASSERT(obj != RT_NULL);          \
        return obj->mname;                  \
    }                                       \
    RTM_EXPORT(rtgui_##cname##_get_##mname)
#define RTGUI_MEMBER_SETTER_GETTER(ctype, cname, mtype, mname) \
    RTGUI_MEMBER_SETTER(ctype, cname, mtype, mname); \
    RTGUI_MEMBER_GETTER(ctype, cname, mtype, mname)
#define MEMBER_GETTER(cname, mname)          rtgui_##cname##_get_##mname
#define MEMBER_SETTER(cname, mname)          rtgui_##cname##_set_##mname

/* Exported types ------------------------------------------------------------*/
typedef rt_uint32_t rtgui_color_t;
typedef struct rtgui_point rtgui_point_t;
typedef struct rtgui_line rtgui_line_t;

/* gc */
typedef struct rtgui_gc rtgui_gc_t;

/* dc */
typedef struct rtgui_dc_engine rtgui_dc_engine_t;
typedef struct rtgui_dc rtgui_dc_t;

/* classes */
typedef struct rtgui_class rtgui_class_t;
typedef struct rtgui_obj rtgui_obj_t;
typedef struct rtgui_container rtgui_container_t;
typedef struct rtgui_box rtgui_box_t;
typedef struct rtgui_widget rtgui_widget_t;
typedef struct rtgui_title rtgui_title_t;
typedef struct rtgui_label rtgui_label_t;
typedef struct rtgui_button rtgui_button_t;
typedef struct rtgui_progress rtgui_progress_t;
typedef struct rtgui_picture rtgui_picture_t;
typedef struct rtgui_list_item rtgui_list_item_t;
typedef struct rtgui_list rtgui_list_t;
typedef struct rtgui_filelist rtgui_filelist_t;
typedef struct rtgui_win rtgui_win_t;
typedef struct rtgui_app rtgui_app_t;

/* event */
typedef struct rtgui_key rtgui_key_t;
typedef struct rtgui_touch rtgui_touch_t;
typedef struct rtgui_event_timer rtgui_event_timer_t;
typedef union rtgui_evt_generic rtgui_evt_generic_t;

/* timer */
typedef struct rtgui_timer rtgui_timer_t;

/* functions */
typedef void (*rtgui_constructor_t)(void *obj);
typedef void (*rtgui_destructor_t)(rtgui_class_t *obj);
typedef rt_bool_t (*rtgui_evt_hdl_t)(void *obj, rtgui_evt_generic_t *evt);
typedef void (*rtgui_idle_hdl_t)(rtgui_obj_t *obj, rtgui_evt_generic_t *evt);
typedef void (*rtgui_timeout_hdl_t)(rtgui_timer_t *timer, void *param);
typedef void (*rtgui_hook_t)(void);


/* point */
struct rtgui_point {
    rt_int16_t x, y;
};

/* line */
struct rtgui_line {
    rtgui_point_t start, end;
};

#define IMPORT_TYPES
#include "include/driver.h"
#include "include/region.h"
#include "include/font/font.h"
#include "include/image.h"
#undef IMPORT_TYPES

/* graphic context */
struct rtgui_gc {
    rtgui_color_t foreground;
    rtgui_color_t background;
    rt_uint16_t textstyle;
    rt_uint16_t textalign;
    rtgui_font_t *font;
};

typedef enum rtgui_orient {
    RTGUI_HORIZONTAL                        = 0x01,
    RTGUI_VERTICAL                          = 0x02,
    RTGUI_ORIENTATION_BOTH                  = RTGUI_HORIZONTAL | RTGUI_VERTICAL,
} rtgui_orient_t;

enum rtgui_margin {
    RTGUI_MARGIN_LEFT                       = 0x01,
    RTGUI_MARGIN_RIGHT                      = 0x02,
    RTGUI_MARGIN_TOP                        = 0x04,
    RTGUI_MARGIN_BOTTOM                     = 0x08,
    RTGUI_MARGIN_ALL                        = RTGUI_MARGIN_LEFT     | \
                                              RTGUI_MARGIN_RIGHT    | \
                                              RTGUI_MARGIN_TOP      | \
                                              RTGUI_MARGIN_BOTTOM,
};

enum rtgui_border {
    RTGUI_BORDER_NONE                       = 0x00,
    RTGUI_BORDER_SIMPLE,
    RTGUI_BORDER_RAISE,
    RTGUI_BORDER_SUNKEN,
    RTGUI_BORDER_BOX,
    RTGUI_BORDER_STATIC,
    RTGUI_BORDER_EXTRA,
    RTGUI_BORDER_UP,
    RTGUI_BORDER_DOWN,
};

typedef enum rtgui_alignment {
    RTGUI_ALIGN_NOT                         = 0x00,
    RTGUI_ALIGN_CENTER_HORIZONTAL           = 0x01,
    RTGUI_ALIGN_LEFT                        = RTGUI_ALIGN_NOT,
    RTGUI_ALIGN_TOP                         = RTGUI_ALIGN_NOT,
    RTGUI_ALIGN_RIGHT                       = 0x02,
    RTGUI_ALIGN_BOTTOM                      = 0x04,
    RTGUI_ALIGN_CENTER_VERTICAL             = 0x08,
    RTGUI_ALIGN_CENTER                      = RTGUI_ALIGN_CENTER_HORIZONTAL | \
                                              RTGUI_ALIGN_CENTER_VERTICAL,
    RTGUI_ALIGN_EXPAND                      = 0x10,
    RTGUI_ALIGN_STRETCH                     = 0x20,
    RTGUI_ALIGN_TTF_SIZE                    = 0x40,
} rtgui_alignment_t;

enum rtgui_text_style {
    RTGUI_TEXTSTYLE_NORMAL                  = 0x00,
    RTGUI_TEXTSTYLE_DRAW_BACKGROUND         = 0x01,
    RTGUI_TEXTSTYLE_SHADOW                  = 0x02,
    RTGUI_TEXTSTYLE_OUTLINE                 = 0x04,
};

typedef enum rtgui_blend_mode {
    RTGUI_BLENDMODE_NONE                    = 0x00,
    RTGUI_BLENDMODE_BLEND,
    RTGUI_BLENDMODE_ADD,
    RTGUI_BLENDMODE_MOD,
} rtgui_blend_mode_t;

#define IMPORT_TYPES
#include "include/dc.h"

#include "include/class.h"
#include "include/widgets/widget.h"
#include "include/widgets/container.h"
#include "include/widgets/box.h"
#include "include/widgets/title.h"
#include "include/widgets/label.h"
#include "include/widgets/button.h"
#include "include/widgets/progress.h"
#include "include/widgets/picture.h"
#include "include/widgets/list.h"
#include "include/widgets/filelist.h"
#include "include/widgets/window.h"
#include "include/app/app.h"
#undef IMPORT_TYPES
#include "include/event.h"

/* timer */
typedef enum rtgui_timer_state {
    RTGUI_TIMER_ST_INIT,
    RTGUI_TIMER_ST_RUNNING,
    RTGUI_TIMER_ST_DESTROY_PENDING,
} rtgui_timer_state_t;

struct rtgui_timer {
    rtgui_app_t* app;
    struct rt_timer timer;
    rtgui_timer_state_t state;
    rtgui_timeout_hdl_t timeout;
    rt_int32_t pending_cnt;                 /* #(pending event) */
    void *user_data;
};

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_TYPES_H__ */
