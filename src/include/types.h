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
#define CLASS_METADATA(name)                (&_rtgui_##name)
#define CLASS_PROTOTYPE(name)               \
    extern const struct rtgui_class _rtgui_##name
#define RTGUI_CLASS(name, _super, ctor, dtor, hdl, size) \
    const struct rtgui_class _rtgui_##name = { \
        _super,                             \
        (rtgui_constructor_t)(ctor),        \
        (rtgui_destructor_t)(dtor),         \
        (rtgui_evt_hdl_t)(hdl),             \
        #name,                              \
        size,                               \
    }
#define CREATE_INSTANCE(name, hdl)          \
    rtgui_create_instance(CLASS_METADATA(name), (rtgui_evt_hdl_t)(hdl))
#define DELETE_INSTANCE(obj)                rtgui_delete_instance(obj)

#ifdef GUIENGIN_USING_CAST_CHECK
# define CAST_(obj, cls, _type)             \
    ((_type *)rtgui_object_cast_check(obj, cls, __FUNCTION__, __LINE__))
#else
# define CAST_(obj, cls, _type)             ((_type *)(obj))
#endif

#define SUPER_(obj)                         (obj->_super)
#define CLASS_(ins)                         rtgui_class_of((void *)ins)
#define IS_SUBCLASS(cls, _super)            rtgui_is_subclass_of(cls, _super)
#define IS_INSTANCE(ins, _cls)              \
    rtgui_is_subclass_of(((rtgui_obj_t *)ins)->cls, _cls)

#define OBJECT_HANDLER(obj)                 TO_OBJECT((void *)obj)->evt_hdl
#define DEFAULT_HANDLER(obj)                CLASS_(obj)->evt_hdl
#define EVENT_HANDLER(obj)                  \
    (OBJECT_HANDLER(obj) ? OBJECT_HANDLER(obj) : DEFAULT_HANDLER(obj))
#define SUPER_HANDLER(obj)                  \
    (CLASS_(obj)->_super ? CLASS_(obj)->_super->evt_hdl : RT_NULL)

#define SETTER_PROTOTYPE(name, _type)       \
    void rtgui_set_##name(_type val)
#define GETTER_PROTOTYPE(name, _type)       \
    _type rtgui_get_##name(void)
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
    void rtgui_set_##name(_type *val) {         \
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

#define APP_SETTER(mname)                   rtgui_app_set_##mname
#define WIDGET_SETTER(mname)                rtgui_widget_set_##mname
#define WIDGET_GETTER(mname)                rtgui_widget_get_##mname



#define WIDGET_GET_ALIGN(w)                 (TO_WIDGET(w)->align)
#define WIDGET_GET_BORDER(w)                (TO_WIDGET(w)->border)
#define WIDGET_GET_BORDER_STYLE(w)          (TO_WIDGET(w)->border_style)
#define WIDGET_GET_FOREGROUND(w)            (TO_WIDGET(w)->gc.foreground)
#define WIDGET_GET_BACKGROUND(w)            (TO_WIDGET(w)->gc.background)
#define WIDGET_GET_TEXTALIGN(w)             (TO_WIDGET(w)->gc.textalign)
#define WIDGET_GET_FONT(w)                  (TO_WIDGET(w)->gc.font)
#define WIDGET_GET_DC(w)                    ((rtgui_dc_t *)&((w)->dc_type))

#define WIDGET_GET_FLAG(w)                  (TO_WIDGET(w)->flag)
#define WIDGET_FLAG_CLEAR(w, fname)         WIDGET_GET_FLAG(w) &= ~RTGUI_WIDGET_FLAG_##fname
#define WIDGET_FLAG_SET(w, fname)           WIDGET_GET_FLAG(w) |= RTGUI_WIDGET_FLAG_##fname
#define IS_WIDGET_FLAG(w, fname)            (WIDGET_GET_FLAG(w) & RTGUI_WIDGET_FLAG_##fname)

#define WIN_GET_FLAG(w)                     (TO_WIN(w)->flag)
#define WIN_FLAG_CLEAR(w, fname)            WIN_GET_FLAG(w) &= ~RTGUI_WIN_FLAG_##fname
#define WIN_FLAG_SET(w, fname)              WIN_GET_FLAG(w) |= RTGUI_WIN_FLAG_##fname
#define IS_WIN_FLAG(w, fname)               (WIN_GET_FLAG(w) & RTGUI_WIN_FLAG_##fname)

#define WIN_GET_STYLE(w)                    (TO_WIN(w)->style)
#define WIN_STYLE_CLEAR(w, sname)           WIN_GET_STYLE(w) &= ~RTGUI_WIN_STYLE_##sname
#define WIN_STYLE_SET(w, sname)             WIN_GET_STYLE(w) |= RTGUI_WIN_STYLE_##sname
#define IS_WIN_STYLE(w, sname)              (WIN_GET_STYLE(w) & RTGUI_WIN_STYLE_##sname)

#define APP_GET_FLAG(a)                     (TO_APP(a)->flag)
#define APP_FLAG_CLEAR(a, fname)            APP_GET_FLAG(a) &= ~RTGUI_APP_FLAG_##fname
#define APP_FLAG_SET(a, fname)              APP_GET_FLAG(a) |= RTGUI_APP_FLAG_##fname
#define IS_APP_FLAG(a, fname)               (APP_GET_FLAG(a) & RTGUI_APP_FLAG_##fname)

#define TOPWIN_GET_FLAG(t)                  (t->flag)
#define TOPWIN_FLAG_CLEAR(t, fname)         TOPWIN_GET_FLAG(t) &= ~RTGUI_TOPWIN_FLAG_##fname
#define TOPWIN_FLAG_SET(t, fname)           TOPWIN_GET_FLAG(t) |= RTGUI_TOPWIN_FLAG_##fname
#define IS_TOPWIN_FLAG(t, fname)            (TOPWIN_GET_FLAG(t) & RTGUI_TOPWIN_FLAG_##fname)

/* Exported types ------------------------------------------------------------*/
struct rtgui_graphic_driver;
typedef struct rtgui_graphic_driver rtgui_graphic_driver_t;

typedef struct rtgui_rect rtgui_rect_t;
typedef struct rtgui_point rtgui_point_t;
typedef struct rtgui_line rtgui_line_t;
typedef struct rtgui_region_data rtgui_region_data_t;
typedef struct rtgui_region rtgui_region_t;

typedef struct rtgui_font_engine rtgui_font_engine_t;
typedef struct rtgui_fnt_font rtgui_fnt_font_t;
typedef struct rtgui_bmp_font rtgui_bmp_font_t;
typedef struct rtgui_font rtgui_font_t;

struct rtgui_image_info;
struct rtgui_blit_info;
struct rtgui_filerw;
typedef struct rtgui_filerw rtgui_filerw_t;
typedef rt_uint32_t rtgui_color_t;
typedef struct rtgui_image_engine rtgui_image_engine_t;
typedef struct rtgui_image_palette rtgui_image_palette_t;
typedef struct rtgui_image rtgui_image_t;
typedef struct rtgui_image_info rtgui_image_info_t;
typedef struct rtgui_blit_info rtgui_blit_info_t;

typedef struct rtgui_gc rtgui_gc_t;
typedef struct rtgui_dc rtgui_dc_t;

typedef struct rtgui_class rtgui_class_t;
typedef struct rtgui_obj rtgui_obj_t;
typedef struct rtgui_container rtgui_container_t;
typedef struct rtgui_box rtgui_box_t;
typedef struct rtgui_widget rtgui_widget_t;
typedef struct rtgui_title rtgui_title_t;
typedef struct rtgui_win rtgui_win_t;
typedef struct rtgui_app rtgui_app_t;
typedef struct rtgui_timer rtgui_timer_t;
typedef struct rtgui_topwin rtgui_topwin_t;

typedef struct rtgui_evt_base rtgui_evt_base_t;
typedef struct rtgui_event_timer rtgui_event_timer_t;
typedef union rtgui_evt_generic rtgui_evt_generic_t;

typedef void (*rtgui_constructor_t)(void *obj);
typedef void (*rtgui_destructor_t)(rtgui_class_t *obj);
typedef rt_bool_t (*rtgui_evt_hdl_t)(void *obj, rtgui_evt_generic_t *evt);
typedef void (*rtgui_idle_hdl_t)(rtgui_obj_t *obj, rtgui_evt_generic_t *evt);
typedef void (*rtgui_timeout_hdl_t)(rtgui_timer_t *timer, void *parameter);
typedef void (*rtgui_hook_t)(void);

/* coordinate point */
struct rtgui_point {
    rt_int16_t x, y;
};

/* line segment */
struct rtgui_line {
    rtgui_point_t start, end;
};

/* rectangle structure */
struct rtgui_rect {
    rt_int16_t x1, y1, x2, y2;
};

/* region*/
struct rtgui_region_data {
    rt_uint32_t size;
    rt_uint32_t numRects;
    /* XXX: And why, exactly, do we have this bogus struct definition? */
    /* rtgui_rect_t rects[size]; in memory but not explicitly declared */
};

struct rtgui_region {
    rtgui_rect_t extents;
    rtgui_region_data_t *data;
};

typedef enum {
    RTGUI_REGION_STATUS_FAILURE,
    RTGUI_REGION_STATUS_SUCCESS
} rtgui_region_status_t;

/* font */
struct rtgui_font_engine {
    rt_err_t (*font_init)(rtgui_font_t *font);
    rt_err_t (*font_open)(rtgui_font_t *font);
    void (*font_close)(rtgui_font_t *font);
    rt_uint8_t (*font_draw_char)(rtgui_font_t *font, rtgui_dc_t *dc,
    rt_uint16_t code, rtgui_rect_t *rect);
    rt_uint8_t (*font_get_width)(rtgui_font_t *font, const char *text);
};

struct rtgui_fnt_font {
    void *data;
    const rt_uint16_t *offset;
    const rt_uint8_t *width;
    #ifdef RTGUI_USING_FONT_FILE
        const char *fname;
        int fd;
    #endif
};

struct rtgui_bmp_font {
    void *data;
    #ifdef RTGUI_USING_FONT_FILE
        const char *fname;
        int fd;
    #endif
};

struct rtgui_font {
    char *family;                           /* font name */
    const rtgui_font_engine_t *engine;      /* font engine */
    const rt_uint16_t height;               /* font height */
    const rt_uint16_t width;                /* font width */
    const rt_uint16_t start;                /* start unicode */
    const rt_uint16_t end;                  /* end unicode */
    const rt_uint16_t dft;                  /* default unicode */
    const rt_uint16_t size;                 /* char size in byte */
    void *data;                             /* font private data */
    rt_uint32_t refer_count;                /* refer count */
    rt_slist_t list;                        /* the font list */
};

/* image */
struct rtgui_image_engine {
    const char *name;
    rt_slist_t list;
    /* image engine function */
    rt_bool_t (*image_check)(rtgui_filerw_t *file);
    rt_bool_t (*image_load)(rtgui_image_t *image, rtgui_filerw_t *file,
        rt_bool_t load);
    void (*image_unload)(rtgui_image_t *image);
    void (*image_blit)(rtgui_image_t *image, rtgui_dc_t *dc,
        rtgui_rect_t *rect);
};

struct rtgui_image_palette {
    rtgui_color_t *colors;
    rt_uint32_t ncolors;
};

struct rtgui_image {
    /* image metrics */
    rt_uint16_t w, h;
    /* image engine */
    const rtgui_image_engine_t *engine;
    /* image palette */
    rtgui_image_palette_t *palette;
    /* image private data */
    void *data;
};

/* device context/drawable canvas */
struct rtgui_dc {
    /* type of device context */
    rt_uint32_t type;
    /* dc engine */
    const struct rtgui_dc_engine *engine;
};

/* graphic context */
struct rtgui_gc {
    rtgui_color_t foreground, background;
    rt_uint16_t textstyle;
    rt_uint16_t textalign;
    rtgui_font_t *font;
};

/* class metadata */
struct rtgui_class {
    const struct rtgui_class *_super;
    rtgui_constructor_t constructor;
    rtgui_destructor_t destructor;
    rtgui_evt_hdl_t evt_hdl;
    char *name;
    rt_size_t size;
};

/* object */
typedef enum rtgui_obj_flag {
    RTGUI_OBJECT_FLAG_NONE                  = 0x0000,
    RTGUI_OBJECT_FLAG_STATIC                = 0x0001,
    RTGUI_OBJECT_FLAG_DISABLED              = 0x0002,
    /* when created, set to valid
       when deleted, clear valid */
    RTGUI_OBJECT_FLAG_VALID                 = 0xAB00,
} rtgui_obj_flag_t;

struct rtgui_obj {
    rt_uint32_t _super;
    const rtgui_class_t *cls;
    rtgui_evt_hdl_t evt_hdl;
    rtgui_obj_flag_t flag;
    rt_ubase_t id;
};

/* widget */
typedef enum rtgui_widget_flag {
    RTGUI_WIDGET_FLAG_DEFAULT               = 0x0000,
    RTGUI_WIDGET_FLAG_SHOWN                 = 0x0001,
    RTGUI_WIDGET_FLAG_DISABLE               = 0x0002,
    RTGUI_WIDGET_FLAG_FOCUS                 = 0x0004,
    RTGUI_WIDGET_FLAG_TRANSPARENT           = 0x0008,
    RTGUI_WIDGET_FLAG_FOCUSABLE             = 0x0010,
    RTGUI_WIDGET_FLAG_DC_VISIBLE            = 0x0100,
    RTGUI_WIDGET_FLAG_IN_ANIM               = 0x0200,
} rtgui_win_widget_t;

struct rtgui_widget {
    rtgui_obj_t _super;                     /* super class */
    rtgui_widget_t *parent;                 /* parent widget */
    rtgui_win_t *toplevel;                  /* parent window */
    rt_slist_t sibling;                     /* children and sibling */
    rtgui_win_widget_t flag;
    rt_int32_t align;
    rt_uint16_t border;
    rt_uint16_t border_style;
    rt_int16_t min_width, min_height;
    rtgui_rect_t extent;
    rtgui_rect_t extent_visiable;           /* including children */
    rtgui_region_t clip;
    rtgui_evt_hdl_t on_focus;
    rtgui_evt_hdl_t on_unfocus;
    rtgui_gc_t gc;                          /* graphic context */
    rt_ubase_t dc_type;                     /* hardware device context */
    const struct rtgui_dc_engine *dc_engine;  // TODO(onelife): struct rtgui_dc
    rt_uint32_t user_data;
};

/* container */
struct rtgui_container {
    rtgui_widget_t _super;
    rtgui_box_t *layout_box;
    rt_slist_t children;
};

/* box sizer */
struct rtgui_box {
    rtgui_obj_t _super;
    rt_uint16_t orient;
    rt_uint16_t border_size;
    rtgui_container_t *container;
};

/* title */
struct rtgui_title {
    rtgui_widget_t _super;
};

/* window */
typedef enum rtgui_win_style {
    RTGUI_WIN_STYLE_NO_FOCUS                = 0x0001,  /* non-focused window */
    RTGUI_WIN_STYLE_NO_TITLE                = 0x0002,  /* no title window */
    RTGUI_WIN_STYLE_NO_BORDER               = 0x0004,  /* no border window */
    RTGUI_WIN_STYLE_CLOSEBOX                = 0x0008,  /* window has the close button */
    RTGUI_WIN_STYLE_MINIBOX                 = 0x0010,  /* window has the mini button */
    RTGUI_WIN_STYLE_DESTROY_ON_CLOSE        = 0x0020,  /* window is destroyed when closed */
    RTGUI_WIN_STYLE_ONTOP                   = 0x0040,  /* window is in the top layer */
    RTGUI_WIN_STYLE_ONBTM                   = 0x0080,  /* window is in the bottom layer */
    RTGUI_WIN_STYLE_MAINWIN                 = 0x0106,  /* window is a main window */
    RTGUI_WIN_STYLE_DEFAULT                 = RTGUI_WIN_STYLE_CLOSEBOX | RTGUI_WIN_STYLE_MINIBOX,
} rtgui_win_style_t;

typedef enum rtgui_win_flag {
    RTGUI_WIN_FLAG_INIT                     = 0x00,
    RTGUI_WIN_FLAG_MODAL                    = 0x01,
    RTGUI_WIN_FLAG_CLOSED                   = 0x02,
    RTGUI_WIN_FLAG_ACTIVATE                 = 0x04,
    RTGUI_WIN_FLAG_IN_MODAL                 = 0x08,  /* (modaled by other) */
    RTGUI_WIN_FLAG_CONNECTED                = 0x10,
    /* connected to server */
    /* window is event_key dispatcher(dispatch it to the focused widget in
     * current window) _and_ a key handler(it should be able to handle keys
     * such as ESC). Both of dispatching and handling are in the same
     * function(_win_event_handler). So we have to distinguish between the
     * two modes.
     *
     * If this flag is set, we are in key-handling mode.
     */
    RTGUI_WIN_FLAG_HANDLE_KEY               = 0x20,
    RTGUI_WIN_FLAG_CB_PRESSED               = 0x40,
} rtgui_win_flag_t;

typedef enum rtgui_modal_code {
    RTGUI_MODAL_OK,
    RTGUI_MODAL_CANCEL,
    RTGUI_MODAL_MAX                         = 0xFFFF,
} rtgui_modal_code_t;

struct rtgui_win {
    rtgui_container_t _super;               /* super class */
    rtgui_win_t *parent;                    /* parent window */
    rtgui_app_t *app;
    char *title;
    rtgui_win_style_t style;
    rtgui_win_flag_t flag;
    rtgui_modal_code_t modal;
    rt_base_t update;                       /* update count */
    rt_base_t drawing;                      /* drawing count */
    rtgui_rect_t drawing_rect;
    rtgui_rect_t outer_extent;
    rtgui_region_t outer_clip;
    rtgui_title_t *_title;
    rtgui_widget_t *focused;
    rtgui_widget_t *last_mouse;             /* last mouse event handler */
    rtgui_evt_hdl_t on_activate;
    rtgui_evt_hdl_t on_deactivate;
    rtgui_evt_hdl_t on_close;
    /* the key is sent to the focused widget by default. If the focused widget
     * and all of it's parents didn't handle the key event, it will be handled
     * by @func on_key
     *
     * If you want to handle key event on your own, it's better to overload
     * this function other than handle EVENT_KBD in evt_hdl.
     */
    rtgui_evt_hdl_t on_key;
    void *user_data;
    /* PRIVATE */
    rt_err_t (*_do_show)(rtgui_win_t *win);
    rt_uint16_t _ref_count;                 /* app _ref_cnt */
    rt_uint32_t _magic;                     /* 0xA5A55A5A */
};

/* app */
typedef enum rtgui_app_flag {
    RTGUI_APP_FLAG_INIT                     = 0x04,
    RTGUI_APP_FLAG_EXITED                   = RTGUI_APP_FLAG_INIT,
    RTGUI_APP_FLAG_SHOWN                    = 0x08,
    RTGUI_APP_FLAG_KEEP                     = 0x80,
} rtgui_app_flag_t;

struct rtgui_app {
    rtgui_obj_t _super;
    rt_thread_t tid;
    rtgui_win_t *main_win;
    char *name;
    rtgui_app_flag_t flag;
    rt_mailbox_t mb;
    rtgui_image_t *icon;
    rtgui_idle_hdl_t on_idle;
    void *user_data;
    /* PRIVATE */
    rt_ubase_t ref_cnt;
    rt_ubase_t win_cnt;
    rt_ubase_t act_cnt;                     /* activate count */
    rt_base_t exit_code;
};

/* timer */
typedef enum rtgui_timer_state {
    RTGUI_TIMER_ST_INIT,
    RTGUI_TIMER_ST_RUNNING,
    RTGUI_TIMER_ST_DESTROY_PENDING,
} rtgui_timer_state_t;

struct rtgui_timer {
    rtgui_app_t* app;
    struct rt_timer timer;
    int pending_cnt;                        /* #(pending event) */
    rtgui_timer_state_t state;
    rtgui_timeout_hdl_t timeout;
    void *user_data;
};

/* top window (server side) */
typedef enum rtgui_topwin_flag {
    RTGUI_TOPWIN_FLAG_INIT                  = 0x0000,
    RTGUI_TOPWIN_FLAG_ACTIVATE              = 0x0001,
    RTGUI_TOPWIN_FLAG_NO_FOCUS              = 0x0002,
    /* window is hidden by default */
    RTGUI_TOPWIN_FLAG_SHOWN                 = 0x0004,
    /* window is modaled by other window */
    RTGUI_TOPWIN_FLAG_DONE_MODAL            = 0x0008,
    /* window is modaling other window */
    RTGUI_TOPWIN_FLAG_IN_MODAL              = 0x0100,
    RTGUI_TOPWIN_FLAG_ONTOP                 = 0x0200,
    RTGUI_TOPWIN_FLAG_ONBTM                 = 0x0400,
} rtgui_topwin_flag_t;

struct rtgui_topwin {
    rtgui_app_t *app;
    rtgui_win_t *wid;                       /* window id */
    rtgui_rect_t extent;
    rtgui_topwin_flag_t flag;
    rtgui_topwin_t *parent;
    /* we need to iterate the topwin list with normal order (get target window)
     * and reverse order (painting). */
    rt_list_t list;
    rt_list_t children;
    rtgui_title_t *title;                   // TODO: ?
    /* event mask */
    rt_uint32_t mask;                       // TODO: ?
    rt_slist_t monitor_list;                /* monitor rect list */
};

/* event */
typedef enum rtgui_evt_type {
    /* app event */
    RTGUI_EVENT_APP_CREATE                  = 0x0000,
    RTGUI_EVENT_APP_DESTROY,
    RTGUI_EVENT_APP_ACTIVATE,
    /* WM event */
    RTGUI_EVENT_SET_WM,
    /* win event */
    RTGUI_EVENT_WIN_CREATE                  = 0x1000,
    RTGUI_EVENT_WIN_DESTROY,
    RTGUI_EVENT_WIN_SHOW,
    RTGUI_EVENT_WIN_HIDE,
    RTGUI_EVENT_WIN_MOVE,
    RTGUI_EVENT_WIN_RESIZE,
    RTGUI_EVENT_WIN_CLOSE,
    RTGUI_EVENT_WIN_ACTIVATE,
    RTGUI_EVENT_WIN_DEACTIVATE,
    RTGUI_EVENT_WIN_UPDATE_END,
    /* sent after window setup and before the app setup */
    RTGUI_EVENT_WIN_MODAL_ENTER,
    /* widget event */
    RTGUI_EVENT_SHOW                        = 0x2000,
    RTGUI_EVENT_HIDE,
    RTGUI_EVENT_PAINT,
    RTGUI_EVENT_FOCUSED,
    RTGUI_EVENT_SCROLLED,
    RTGUI_EVENT_RESIZE,
    RTGUI_EVENT_SELECTED,
    RTGUI_EVENT_UNSELECTED,
    RTGUI_EVENT_MV_MODEL,
    RTGUI_EVENT_UPDATE_TOPLVL               = 0x3000,
    RTGUI_EVENT_UPDATE_BEGIN,
    RTGUI_EVENT_UPDATE_END,
    RTGUI_EVENT_MONITOR_ADD,
    RTGUI_EVENT_MONITOR_REMOVE,
    RTGUI_EVENT_TIMER,
    /* virtual paint event (server -> client) */
    RTGUI_EVENT_VPAINT_REQ,
    /* clip rect information */
    RTGUI_EVENT_CLIP_INFO,
    /* mouse and keyboard event */
    RTGUI_EVENT_MOUSE_MOTION                = 0x4000,
    RTGUI_EVENT_MOUSE_BUTTON,
    RTGUI_EVENT_KBD,
    RTGUI_EVENT_TOUCH,
    RTGUI_EVENT_GESTURE,
    WBUS_NOTIFY_EVENT                       = 0x5000,
    /* user command (at bottom) */
    RTGUI_EVENT_COMMAND                     = 0xF000,
} rtgui_evt_type_t;

/* base event */
struct rtgui_evt_base {
    rtgui_evt_type_t type;
    rt_uint16_t user;   // TODO(onelife): Not used?
    rtgui_app_t *origin;
    rt_mailbox_t ack;
};

/* app event */
struct rtgui_evt_app {
    rtgui_evt_base_t base;
    rtgui_app_t *app;
};

/* window manager event  */
struct rtgui_event_set_wm {
    rtgui_evt_base_t base;
    rtgui_app_t *app;
};

/* window event */
#define _RTGUI_EVENT_WIN_ELEMENTS           \
    rtgui_evt_base_t base;                \
    rtgui_win_t *wid;

struct rtgui_event_win {
    _RTGUI_EVENT_WIN_ELEMENTS;
};

struct rtgui_event_win_create {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rtgui_win_t *parent_window;
};

struct rtgui_event_win_move {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rt_int16_t x, y;
};

struct rtgui_event_win_resize {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rtgui_rect_t rect;
};

struct rtgui_event_win_update_end {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rtgui_rect_t rect;
};

#define rtgui_event_win_destroy             rtgui_event_win
#define rtgui_event_win_show                rtgui_event_win
#define rtgui_event_win_hide                rtgui_event_win
#define rtgui_event_win_activate            rtgui_event_win
#define rtgui_event_win_deactivate          rtgui_event_win
#define rtgui_event_win_close               rtgui_event_win
#define rtgui_event_win_modal_enter         rtgui_event_win

/* other window event */
struct rtgui_event_update_begin {
    rtgui_evt_base_t base;
    /* the update rect */
    rtgui_rect_t rect;
};

struct rtgui_evt_update_end {
    rtgui_evt_base_t base;
    /* the update rect */
    rtgui_rect_t rect;
};

struct rtgui_event_monitor {
    _RTGUI_EVENT_WIN_ELEMENTS;
    /* the monitor rect */
    rtgui_rect_t rect;
};

struct rtgui_event_paint {
    _RTGUI_EVENT_WIN_ELEMENTS;
    /* rect to be updated */
    rtgui_rect_t rect;
};

struct rtgui_event_clip_info {
    _RTGUI_EVENT_WIN_ELEMENTS;
    /* the number of rects */
    /*rt_uint32_t num_rect;*/
    /* rtgui_rect_t *rects */
};

#define rtgui_event_show rtgui_evt_base
#define rtgui_event_hide rtgui_evt_base

struct rtgui_event_update_toplvl {
    rtgui_evt_base_t base;
    rtgui_win_t *toplvl;
};

struct rtgui_event_timer {
    rtgui_evt_base_t base;
    struct rtgui_timer *timer;
};

// TODO(onelife): ??
struct rt_completion {
    void *hi;
};

struct rtgui_event_vpaint_req {
    _RTGUI_EVENT_WIN_ELEMENTS;
    struct rtgui_event_vpaint_req *origin;
    struct rt_completion *cmp;
    rtgui_dc_t* buffer;
};

/* gesture event */
enum rtgui_gesture_type {
    RTGUI_GESTURE_NONE                      = 0x0000,
    RTGUI_GESTURE_TAP                       = 0x0001,
    /* Usually used to zoom in and out. */
    RTGUI_GESTURE_LONGPRESS                 = 0x0002,
    RTGUI_GESTURE_DRAG_HORIZONTAL           = 0x0004,
    RTGUI_GESTURE_DRAG_VERTICAL             = 0x0008,
    RTGUI_GESTURE_DRAG                      = (RTGUI_GESTURE_DRAG_HORIZONTAL | \
                                               RTGUI_GESTURE_DRAG_VERTICAL),
    /* PINCH, DRAG finished. */
    RTGUI_GESTURE_FINISH                    = 0x8000,
    /* The corresponding gesture should be canceled. */
    RTGUI_GESTURE_CANCEL                    = 0x4000,
    RTGUI_GESTURE_TYPE_MASK                 = 0x0FFF,
};

struct rtgui_event_gesture {
    _RTGUI_EVENT_WIN_ELEMENTS;
    enum rtgui_gesture_type type;
    rt_uint32_t act_cnt;               /* window activate count */
};

/* mouse and keyboard event */
struct rtgui_event_mouse {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rt_uint16_t x, y;
    rt_uint16_t button;
    rt_tick_t ts;                           /* timestamp */
    /* id of touch session(from down to up). Different session should have
     * different id. id should never be 0. */
    rt_uint32_t id;
    rt_uint32_t act_cnt;               /* window activate count */
};

struct rtgui_event_kbd {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rt_uint32_t act_cnt;               /* window activate count */
    rt_uint16_t type;                       /* key down or up */
    rt_uint16_t key;                        /* current key */
    rt_uint16_t mod;                        /* current key modifiers */
    rt_uint16_t unicode;                    /* translated character */
};

/* touch event: handled by server */
struct rtgui_event_touch {
    rtgui_evt_base_t base;
    rt_uint16_t x, y;
    rt_uint16_t up_down;
    rt_uint16_t resv;
};

/* user command event */
#ifndef GUIENGINE_CMD_STRING_MAX
# define GUIENGINE_CMD_STRING_MAX            16
#endif
struct rtgui_event_command {
    _RTGUI_EVENT_WIN_ELEMENTS;
    rt_int32_t type;
    rt_int32_t command_id;
    char command_string[GUIENGINE_CMD_STRING_MAX];
};

/* widget event */
struct rtgui_event_scrollbar {
    rtgui_evt_base_t base;
    rt_uint8_t event;
};

struct rtgui_event_focused {
    rtgui_evt_base_t base;
    rtgui_widget_t *widget;
};

struct rtgui_event_resize {
    rtgui_evt_base_t base;
    rt_int16_t x, y;
    rt_int16_t w, h;
};

typedef enum rtgui_event_model_mode {
    RTGUI_MV_DATA_ADDED,
    RTGUI_MV_DATA_CHANGED,
    RTGUI_MV_DATA_DELETED,
} rtgui_event_model_mode_t;

struct rtgui_event_mv_model {
    rtgui_evt_base_t base;
    struct rtgui_mv_model *model;
    struct rtgui_mv_view  *view;
    rt_size_t first_data_changed_idx;
    rt_size_t last_data_changed_idx;
};


/* generic event */
union rtgui_evt_generic {
    struct rtgui_evt_base base;
    struct rtgui_evt_app app_create;
    struct rtgui_evt_app app_destroy;
    struct rtgui_evt_app app_activate;
    struct rtgui_event_set_wm set_wm;
    struct rtgui_event_win win_base;
    struct rtgui_event_win_create win_create;
    struct rtgui_event_win_move win_move;
    struct rtgui_event_win_resize win_resize;
    struct rtgui_event_win_destroy win_destroy;
    struct rtgui_event_win_show win_show;
    struct rtgui_event_win_hide win_hide;
    struct rtgui_event_win_update_end win_update;
    struct rtgui_event_win_activate win_activate;
    struct rtgui_event_win_deactivate win_deactivate;
    struct rtgui_event_win_close win_close;
    struct rtgui_event_win_modal_enter win_modal_enter;
    struct rtgui_event_update_begin update_begin;
    struct rtgui_evt_update_end update_end;
    struct rtgui_event_monitor monitor;
    struct rtgui_event_paint paint;
    struct rtgui_event_update_toplvl update_toplvl;
    struct rtgui_event_vpaint_req vpaint_req;
    struct rtgui_event_clip_info clip_info;
    struct rtgui_event_timer timer;
    struct rtgui_event_mouse mouse;
    struct rtgui_event_kbd kbd;
    struct rtgui_event_touch touch;
    struct rtgui_event_gesture gesture;
    struct rtgui_event_scrollbar scrollbar;
    struct rtgui_event_focused focused;
    struct rtgui_event_resize resize;
    struct rtgui_event_mv_model model;
    struct rtgui_event_command command;
};

/* other types */
enum RTGUI_MARGIN_STYLE {
    RTGUI_MARGIN_LEFT                       = 0x01,
    RTGUI_MARGIN_RIGHT                      = 0x02,
    RTGUI_MARGIN_TOP                        = 0x04,
    RTGUI_MARGIN_BOTTOM                     = 0x08,
    RTGUI_MARGIN_ALL                        = RTGUI_MARGIN_LEFT | \
                                              RTGUI_MARGIN_RIGHT | \
                                              RTGUI_MARGIN_TOP | \
                                              RTGUI_MARGIN_BOTTOM,
};

enum RTGUI_BORDER_STYLE {
    RTGUI_BORDER_NONE                       = 0,
    RTGUI_BORDER_SIMPLE,
    RTGUI_BORDER_RAISE,
    RTGUI_BORDER_SUNKEN,
    RTGUI_BORDER_BOX,
    RTGUI_BORDER_STATIC,
    RTGUI_BORDER_EXTRA,
    RTGUI_BORDER_UP,
    RTGUI_BORDER_DOWN,
    RTGUI_BORDER_DEFAULT_WIDTH              = 2,
    RTGUI_WIDGET_DEFAULT_MARGIN             = 3,
};

/* Blend mode */
enum RTGUI_BLENDMODE {
    RTGUI_BLENDMODE_NONE                    = 0x00,
    RTGUI_BLENDMODE_BLEND,
    RTGUI_BLENDMODE_ADD,
    RTGUI_BLENDMODE_MOD,
};

enum RTGUI_ORIENTATION {
    RTGUI_HORIZONTAL                        = 0x01,
    RTGUI_VERTICAL                          = 0x02,
    RTGUI_ORIENTATION_BOTH                  = RTGUI_HORIZONTAL | RTGUI_VERTICAL,
};

enum RTGUI_ALIGN {
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
};

enum RTGUI_TEXTSTYLE {
    RTGUI_TEXTSTYLE_NORMAL                  = 0x00,
    RTGUI_TEXTSTYLE_DRAW_BACKGROUND         = 0x01,
    RTGUI_TEXTSTYLE_SHADOW                  = 0x02,
    RTGUI_TEXTSTYLE_OUTLINE                 = 0x04,
};

#undef _RTGUI_EVENT_WIN_ELEMENTS

/* Exported constants --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* __RTGUI_TYPES_H__ */
