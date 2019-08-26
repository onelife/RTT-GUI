/*
 * File      : design.c
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
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/design.h"
#include "include/widgets/box.h"
#include "include/widgets/container.h"
#include "include/widgets/label.h"
#include "include/widgets/button.h"
#include "include/widgets/progress.h"
#include "include/widgets/picture.h"
#include "include/widgets/list.h"
#include "include/widgets/filelist.h"
#include "include/widgets/window.h"
#include "include/app/app.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_DSN"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
enum object_token_type {
  OBJ_UNKNOWN = 0,
  OBJ_TIMER,
  OBJ_APP,
  OBJ_MAIN_WINDOW,
  OBJ_WINDOW,
  OBJ_CONTAINER,
  OBJ_BOX,
  OBJ_LABEL,
  OBJ_BUTTON,
  OBJ_PROGRESS,
  OBJ_PICTURE,
  //OBJ_LIST,
  OBJ_FILELIST,
  ATTR_ID,
  ATTR_BACKGROUND,
  ATTR_FOREGROUND,
  ATTR_MIN_HEIGHT,
  ATTR_MIN_WIDTH,
  ATTR_ALIGN,
};

/* Private define ------------------------------------------------------------*/
#define TOKEN_MAX_SIZE              (10)
#define STRING_MAX_SIZE             (32)
#define NUMBER_MAX_SIZE             (10)

/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static char _get_next(design_contex_t *ctx) {
    if (ctx->pos >= ctx->len) {
        if (RT_EOK != ctx->read(&ctx->buf, &ctx->len)) {
            ctx->len = 0;
            return 0x00;
        }
        if (!ctx->buf || !ctx->len) {
            ctx->len = 0;
            return 0x00;
        }
        ctx->pos = 0;
        // LOG_W("|read| %d", ctx->len);
        // LOG_HEX("read", 16, ctx->buf, ctx->len);
    }
    return ctx->buf[ctx->pos++];
}

static void _skip_white_space(design_contex_t *ctx) {
    char next;
    rt_bool_t done = RT_FALSE;

    do {
        next = _get_next(ctx);
        if (!next) break;

        switch (next) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            break;
        default:
            ctx->pos--;
            done = RT_TRUE;
            break;
        }
    } while (!done);
}

static void _skip_until(design_contex_t *ctx, char target) {
    char next;

    do {
        next = _get_next(ctx);
        if (!next) break;

        if (target == next) break;
    } while (1);
}

static rt_bool_t _skip_none_token(design_contex_t *ctx) {
    char next;
    rt_bool_t done = RT_FALSE;

    do {
        _skip_white_space(ctx);
        next = _get_next(ctx);
        // LOG_D("|skip bf| \"%c\"", next);
        if (!next) break;

        /* comment */
        if ('/' == next) {
            next = _get_next(ctx);
            if ('/' == next) {
                LOG_D("|skip| Comment");
                _skip_until(ctx, '\n');
            } else {
                LOG_E("|skip| unexpect \"/\"");
                break;
            }
        } else {
            done = RT_TRUE;
        }
    } while (!done);

    if (done) ctx->pos--;
    return done;
}

static rt_uint8_t _parse_token(design_contex_t *ctx, char *buf,
    rt_uint32_t size, char ending) {
    char next;
    rt_uint8_t len;

    len = 0;
    do {
        next = _get_next(ctx);
        if (!next) break;
        /* copy content */
        if (ending != next)
            buf[len++] = next;
        else
            break;
    } while (len <= size);

    buf[len] = '\0';
    return len;
}

static rt_bool_t _parse_string(design_contex_t *ctx, char *buf) {
    do {
        char next;
        rt_uint8_t len;

        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;

        /* the preceding mark */
        next = _get_next(ctx);
        if ('N' == next) {
            /* check if NULL */
            len = _parse_token(ctx, buf, STRING_MAX_SIZE, ',');
            if (!len) break;
            if (rt_strncmp("ULL", buf, 3)) {
                LOG_E("|string| expect \"NULL\" got \"N%s\"", buf);
                break;
            }
            *buf = '\0';
            return RT_TRUE;
        } else if ('"' != next) {
            LOG_E("|string| expect \"\"\" got \"%c\"", next);
            break;
        }

        /* the string */
        len = _parse_token(ctx, buf, STRING_MAX_SIZE, '"');
        if (!len) break;
        LOG_D("|string| %s", buf);

        /* if string oversize, skip the rest */
        if ('"' != ctx->buf[ctx->pos - 1])
            _skip_until(ctx, '"');

        /* skip succeeding white space and comment */
        if (!_skip_none_token(ctx)) break;

        /* the separation mark */
        next = _get_next(ctx);
        if (',' != next) {
            LOG_E("|string| expect \",\" got \"%c\"", next);
            break;
        }

        return RT_TRUE;
    } while (0);

    LOG_E("|string| err");
    return RT_FALSE;
}

static rt_bool_t _parse_handler(design_contex_t *ctx, void **hdl) {
    do {
        rt_uint8_t len;
        char name[STRING_MAX_SIZE + 1];
        const hdl_tbl_entr_t *entry;

        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;

        /* the handler name */
        len = _parse_token(ctx, name, STRING_MAX_SIZE, ',');
        if (!len) break;
        LOG_D("|handler| %s", name);

        *hdl = RT_NULL;
        if (rt_strncmp("NULL", name, 4)) {
            for (entry = ctx->hdl_tbl; entry->name != RT_NULL; entry++) {
                if (!rt_strncmp(entry->name, name, len)) {
                    *hdl = entry->hdl;
                    break;
                }
            }
            if (!*hdl) {
                LOG_E("|handler| unknown %s", name);
                break;
            }
        }
        return RT_TRUE;
    } while (0);

    LOG_E("|handler| err");
    return RT_FALSE;
}

static rt_bool_t _parse_number(design_contex_t *ctx, rt_uint32_t *num) {
    do {
        char buf[NUMBER_MAX_SIZE + 1];
        rt_uint8_t base, len, i;
        rt_bool_t err;

        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;

        /* the number string */
        len = _parse_token(ctx, buf, NUMBER_MAX_SIZE, ',');
        if (!len) break;
        // LOG_D("|number| <-%s", buf);

        if ((buf[0] == '0') && ((buf[1] == 'x') || (buf[1] == 'X'))) {
            base = 16;
        } else {
            base = 10;
        }

        *num = 0;
        err = RT_FALSE;
        if (10 == base) {
            for (i = 0; i < len; i++) {
                if ((buf[i] >= '0') && (buf[i] <= '9')) {
                    *num = (*num) * 10 + (buf[i] - '0');
                } else {
                    err = RT_TRUE;
                    LOG_E("|number| bad %s", buf);
                    break;
                }
            }
            if (err) break;
        } else {
            for (i = 2; i < len; i++) {
                if ((buf[i] >= '0') && (buf[i] <= '9')) {
                    *num = (*num) * 16 + (buf[i] - '0');
                } else if ((buf[i] >= 'a') && (buf[i] <= 'f')) {
                    *num = (*num) * 16 + (buf[i] - 'a' + 10);
                } else if ((buf[i] >= 'A') && (buf[i] <= 'F')) {
                    *num = (*num) * 16 + (buf[i] - 'A' + 10);
                } else {
                    err = RT_TRUE;
                    LOG_E("|number| bad %s", buf);
                    break;
                }
            }
            if (err) break;
        }

        LOG_D("|number| %d", *num);
        return RT_TRUE;
    } while (0);

    LOG_E("|number| err");
    return RT_FALSE;
}

static rt_bool_t _parse_size(design_contex_t *ctx, rtgui_rect_t *size) {
    do {
        char next;
        char buf[NUMBER_MAX_SIZE + 1];
        rt_uint8_t len;

        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;

        next = _get_next(ctx);
        if ('N' == next) {
            /* check if NULL */
            len = _parse_token(ctx, buf, NUMBER_MAX_SIZE, ',');
            if (!len) break;
            if (rt_strncmp("ULL", buf, 3)) {
                LOG_E("|size| expect \"NULL\" got \"N%s\"", buf);
                break;
            }
            size = RT_NULL;
        } else if ('D' == next) {
            /* check if DEFAULT */
            len = _parse_token(ctx, buf, NUMBER_MAX_SIZE, ',');
            if (!len) break;
            if (rt_strncmp("EFAULT", buf, 6)) {
                LOG_E("|size| expect \"DEFAULT\" got \"D%s\"", buf);
                break;
            }
            rtgui_get_screen_rect(size);
        } else if ('[' != next) {
        /* the preceding mark */
            LOG_E("|size| expect \"[\" got \"%c\"", next);
            break;
        } else {
            rt_uint32_t num;

            if (!_parse_number(ctx, &num)) break;
            size->x1 = num & 0x0000ffff;
            if (!_parse_number(ctx, &num)) break;
            size->y1 = num & 0x0000ffff;
            if (!_parse_number(ctx, &num)) break;
            size->x2 = num & 0x0000ffff;
            if (!_parse_number(ctx, &num)) break;
            size->y2 = num & 0x0000ffff;

            /* skip preceding white space and comment */
            if (!_skip_none_token(ctx)) break;
            next = _get_next(ctx);
            if (']' != next) {
                LOG_E("|size| expect \"]\" got \"%c\"", next);
                break;
            }

            /* skip preceding white space and comment */
            if (!_skip_none_token(ctx)) break;
            next = _get_next(ctx);
            if (',' != next) {
                LOG_E("|size| expect \",\" got \"%c\"", next);
                break;
            }
        }

        if (!size) {
            LOG_D("|size| (NULL)");
        } else {
            LOG_D("|size| (%d,%d)-(%d,%d)", size->x1, size->y1, size->x2,
                size->y2);
        }
        return RT_TRUE;
    } while (0);

    LOG_E("|size| err");
    return RT_FALSE;
}

static enum object_token_type _get_object_type(char *buf) {
    /* process tockens:
        - APP
        - MWIN
        - WIN
        - CONTAINER
        - BOX
        - LABEL
        - BUTTON
        - PROGRESS
        - FILELIST
        - ID
        - COLOR
        - ALIGN
    */
    if (!rt_strncmp("ID", buf, 2))          return ATTR_ID;
    if (!rt_strncmp("APP", buf, 3))         return OBJ_APP;
    if (!rt_strncmp("WIN", buf, 3))         return OBJ_WINDOW;
    if (!rt_strncmp("BOX", buf, 3))         return OBJ_BOX;
    if (!rt_strncmp("MWIN", buf, 4))        return OBJ_MAIN_WINDOW;
    if (!rt_strncmp("TIMER", buf, 5))       return OBJ_TIMER;
    if (!rt_strncmp("LABEL", buf, 5))       return OBJ_LABEL;
    if (!rt_strncmp("ALIGN", buf, 5))       return ATTR_ALIGN;
    if (!rt_strncmp("BUTTON", buf, 6))      return OBJ_BUTTON;
    if (!rt_strncmp("PICTURE", buf, 7))     return OBJ_PICTURE;
    if (!rt_strncmp("PROGRESS", buf, 8))    return OBJ_PROGRESS;
    if (!rt_strncmp("FILELIST", buf, 8))    return OBJ_FILELIST;
    if (!rt_strncmp("CONTAINER", buf, 9))   return OBJ_CONTAINER;
    if (!rt_strncmp("MIN_WIDTH", buf, 9))   return ATTR_MIN_WIDTH;
    if (!rt_strncmp("BACKGROUND", buf, 10)) return ATTR_BACKGROUND;
    if (!rt_strncmp("FOREGROUND", buf, 10)) return ATTR_FOREGROUND;
    if (!rt_strncmp("MIN_HEIGHT", buf, 10)) return ATTR_MIN_HEIGHT;
    return OBJ_UNKNOWN;
}

static rt_bool_t _parse_object(design_contex_t *ctx, void *parent) {
    do {
        char next;
        rt_uint8_t len;
        char token[TOKEN_MAX_SIZE + 1];
        enum object_token_type type;
        rt_bool_t is_attr;
        rt_bool_t err;
        void *self;

        /*--- object name ---*/
        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;

        /* the object name */
        len = _parse_token(ctx, token, TOKEN_MAX_SIZE, ':');
        if (!len) break;
        // LOG_D("|object| %s", token);

        /* get object type */
        type = _get_object_type(token);
        if (OBJ_UNKNOWN == type) break;

        /* process attribute */
        is_attr = RT_FALSE;
        if (ATTR_ID == type) {
            rt_uint32_t num;

            if (!_parse_number(ctx, &num)) break;
            if (num >= ctx->obj_sz) {
                LOG_E("|object| ID overflow");
                break;
            }
            ctx->obj_tbl[num] = parent;
            LOG_D("|ID| %d", num);
            is_attr = RT_TRUE;
        } else if ((ATTR_BACKGROUND == type) || (ATTR_FOREGROUND == type)) {
            rtgui_color_t color;

            if (!_parse_number(ctx, &color)) break;
            if (ATTR_BACKGROUND == type) {
                WIDGET_BACKGROUND(TO_WIDGET(parent)) = color;
                LOG_D("|BACKGROUND| 0x%04x", color);
            } else {
                WIDGET_FOREGROUND(TO_WIDGET(parent)) = color;
                LOG_D("|FOREGROUND| 0x%04x", color);
            }
            is_attr = RT_TRUE;
        } else if ((ATTR_MIN_HEIGHT == type) || (ATTR_MIN_WIDTH == type)) {
            rt_uint32_t num;

            if (!_parse_number(ctx, &num)) break;
            num &= 0x0000ffff;
            if (ATTR_MIN_HEIGHT == type) {
                WIDGET_SETTER(min_height)(TO_WIDGET(parent), (rt_int16_t)num);
                LOG_D("|MIN_HEIGHT| %d", num);
            } else {
                WIDGET_SETTER(min_width)(TO_WIDGET(parent), (rt_int16_t)num);
                LOG_D("|MIN_WIDTH| %d", num);
            }
            is_attr = RT_TRUE;
        } else if (ATTR_ALIGN == type) {
            rt_uint32_t align;

            if (!_parse_number(ctx, &align)) break;
            WIDGET_ALIGN(parent) = align;
            LOG_D("|ALIGN| 0x%04x", align);
            is_attr = RT_TRUE;
        }
        if (is_attr) {
            /* put back a "," in buf */
            ctx->buf[--ctx->pos] = ',';
            return RT_TRUE;
        }

        /*--- { ---*/
        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;
        next = _get_next(ctx);
        if ('{' != next) {
            LOG_E("|object| expect \"{\" got \"%c\"", next);
            break;
        }

        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;
        len = _parse_token(ctx, token, TOKEN_MAX_SIZE, ':');
        if (!len) break;

        /*--- PARAM ---*/
        if (rt_strncmp("PARAM", token, 5)) {
            LOG_E("|object| expect \"PARAM\" got \"%s\"", token);
            break;
        }

        /*--- { ---*/
        /* skip leading white space and comment */
        if (!_skip_none_token(ctx)) break;
        next = _get_next(ctx);
        if ('{' != next) {
            LOG_E("|object| expect \"{\" got \"%c\"", next);
            break;
        }

        /* create object */
        self = RT_NULL;
        if (OBJ_TIMER == type) {
            rtgui_timeout_hdl_t hdl;
            rt_uint32_t num1, num2;
            rtgui_timer_t *tmr;

            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_number(ctx, &num1)) break;
            if (!_parse_number(ctx, &num2)) break;

            num1 &= 0x000000ff;
            tmr = rtgui_timer_create((rt_int32_t)num2, (rt_uint8_t)num1, hdl,
                RT_NULL);
            if (!tmr) {
                LOG_E("|TIMER| Create failed!");
                break;
            }
            LOG_D("|TIMER| %p", tmr);
            self = (void *)tmr;
        } else if (OBJ_APP == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_evt_hdl_t hdl;

            if (ctx->app) {
                LOG_E("|APP| Duplicate!");
                break;
            }

            if (!_parse_string(ctx, str)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;

            CREATE_APP_INSTANCE(ctx->app, hdl, str);
            if (!ctx->app) {
                LOG_E("|APP| Create failed!");
                break;
            }
            LOG_D("|APP| %p", ctx->app);
        } else if (OBJ_MAIN_WINDOW == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_evt_hdl_t hdl;

            if (!ctx->app) {
                LOG_E("|MWIN| No APP!");
                break;
            }
            if (ctx->win) {
                LOG_E("|MWIN| Duplicate!");
                break;
            }

            if (!_parse_string(ctx, str)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;

            ctx->win = CREATE_MAIN_WIN(hdl, str, RTGUI_WIN_STYLE_MAINWIN);
            if (!ctx->win) {
                rtgui_app_uninit(ctx->app);
                LOG_E("|MWIN| Create failed!");
                break;
            }
            LOG_D("|MWIN| %p", ctx->win);
            self = (void *)ctx->win;
        } else if (OBJ_WINDOW == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl, on_close;
            rtgui_win_t *win;

            if (!_parse_string(ctx, str)) break;
            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_handler(ctx, (void**)&on_close)) break;

            win = CREATE_WIN_INSTANCE(parent, hdl, &rect, str,
                RTGUI_WIN_STYLE_DEFAULT);
            if (!win) {
                LOG_E("|WIN| Create failed!");
                break;
            }
            if (on_close) WIN_SETTER(on_close)(win, on_close);  
            LOG_D("|WIN| %p -> %p", win, parent);
            self = (void *)win;
        } else if (OBJ_CONTAINER == type) {
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl;
            rtgui_container_t *cntr;

            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;

            cntr = CREATE_CONTAINER_INSTANCE(parent, hdl, &rect);
            if (!cntr) {
                LOG_E("|CONTAINER| Create failed!");
                break;
            }
            LOG_D("|CONTAINER| %p -> %p", cntr, parent);
            self = (void *)cntr;
        } else if (OBJ_BOX == type) {
            rt_uint32_t num1, num2;
            rtgui_box_t *box;

            if (!_parse_number(ctx, &num1)) break;
            if (!_parse_number(ctx, &num2)) break;

            num1 &= 0x0000ffff;
            num2 &= 0x0000ffff;
            box = CREATE_BOX_INSTANCE(parent, (rt_uint16_t)num1,
                (rt_uint16_t)num2);
            if (!box) {
                LOG_E("|BOX| Create failed!");
                break;
            }
            LOG_D("|BOX| %p -> %p", box, parent);
            self = (void *)box;
        } else if (OBJ_LABEL == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl;
            rtgui_label_t *lab;

            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_string(ctx, str)) break;

            lab = CREATE_LABEL_INSTANCE(parent, hdl, &rect, str);
            if (!lab) {
                LOG_E("|LABEL| Create failed!");
                break;
            }
            LOG_D("|LABEL| %p -> %p", lab, parent);
            self = (void *)lab;
        } else if (OBJ_BUTTON == type) {
            char str[STRING_MAX_SIZE + 1];
            rt_uint32_t num;
            rtgui_evt_hdl_t hdl;
            rtgui_button_t *btn;

            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_number(ctx, &num)) break;
            if (!_parse_string(ctx, str)) break;

            // num &= 0x0000ffff;
            btn = rtgui_create_button(TO_CONTAINER(parent), hdl,
                (rtgui_button_flag_t)num, str);
            if (!btn) {
                LOG_E("|BUTTON| Create failed!");
                break;
            }
            LOG_D("|BUTTON| %p -> %p", btn, parent);
            self = (void *)btn;
        } else if (OBJ_PROGRESS == type) {
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl;
            rt_uint32_t num1, num2;
            rtgui_progress_t *bar;

            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_number(ctx, &num1)) break;
            if (!_parse_number(ctx, &num2)) break;

            num1 &= 0x0000ffff;
            num2 &= 0x0000ffff;
            bar = CREATE_PROGRESS_INSTANCE(parent, hdl, &rect,
                (rtgui_orient_t)num1, (rt_uint16_t)num2);
            if (!bar) {
                LOG_E("|PROGRESS| Create failed!");
                break;
            }
            LOG_D("|PROGRESS| %p -> %p", bar, parent);
            self = (void *)bar;
        } else if (OBJ_PICTURE == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl;
            rt_uint32_t num1, num2;
            rtgui_picture_t *pic;

            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_string(ctx, str)) break;
            if (!_parse_number(ctx, &num1)) break;
            if (!_parse_number(ctx, &num2)) break;

            pic = rtgui_create_picture(TO_CONTAINER(parent), hdl, &rect,
                str[0] == '\0' ? RT_NULL : str, num1, !(num2 == 0));
            if (!pic) {
                LOG_E("|PICTURE| Create failed!");
                break;
            }
            LOG_D("|PICTURE| %p -> %p", pic, parent);
            self = (void *)pic;
        } else if (OBJ_FILELIST == type) {
            char str[STRING_MAX_SIZE + 1];
            rtgui_rect_t rect;
            rtgui_evt_hdl_t hdl;
            rtgui_filelist_t *filelist;

            if (!_parse_size(ctx, &rect)) break;
            if (!_parse_handler(ctx, (void**)&hdl)) break;
            if (!_parse_string(ctx, str)) break;

            filelist = CREATE_FILELIST_INSTANCE(parent, hdl, &rect, str);
            if (!filelist) {
                LOG_E("|FILELIST| Create failed!");
                break;
            }
            LOG_D("|FILELIST| %p -> %p", filelist, parent);
            self = (void *)filelist;
        } else {
            /* should not be here */
            break;
        }

        /*--- } ---*/
        /* skip succeeding white space and comment */
        if (!_skip_none_token(ctx)) break;
        next = _get_next(ctx);
        if ('}' != next) {
            LOG_E("|object| expect \"}\" got \"%c\"", next);
            break;
        }

        /*--- , or } ---*/
        err = RT_FALSE;
        do {
            /* skip succeeding white space and comment */
            if (!_skip_none_token(ctx)) break;
            next = _get_next(ctx);
            if ('}' == next) {
                LOG_D("|object| done1");
                break;
            }
            if (',' != next) {
                LOG_E("|object| expect \",\" got \"%c\"", next);
                err = RT_TRUE;
                break;
            }

            /* check if has sub object */
            /* skip succeeding white space and comment */
            if (!_skip_none_token(ctx)) break;
            next = _get_next(ctx);
            if (!next) {
                LOG_E("|object| incompleted");
                err = RT_TRUE;
                break;
            }
            if ('}' == next) {
                LOG_D("|object| done2");
                break;
            }
            LOG_D("|object| has sub");
            /* return the byte */
            ctx->pos--;
            err = !_parse_object(ctx, self);
            if (err) break;
        } while (1);
        if (err) break;

        return RT_TRUE;
    } while (0);

    LOG_E("|object| err");
    return RT_FALSE;
}

/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_design_init(design_contex_t *ctx, design_read_t read,
    const hdl_tbl_entr_t *hdl_tbl, obj_tbl_entr_t *obj_tbl, rt_uint32_t obj_sz) {

    if (!read) return -RT_EINVAL;

    ctx->read = read;
    ctx->hdl_tbl = hdl_tbl;
    ctx->obj_tbl = obj_tbl;
    ctx->obj_sz = obj_sz;
    ctx->buf = RT_NULL;
    ctx->pos = 0;
    ctx->len = 0;
    ctx->app = RT_NULL;
    ctx->win = RT_NULL;

    return RT_EOK;
}

rt_err_t rtgui_design_parse(design_contex_t *ctx) {
    char next;

    if (!ctx) return -RT_EINVAL;

    do {
        if (!_parse_object(ctx, RT_NULL)) break;
        LOG_W("obj +1");

        /* skip succeeding white space and comment */
        (void)_skip_none_token(ctx);
        next = _get_next(ctx);
        if (',' != next) {
            LOG_I("Parsing done");
            return RT_EOK;
        }
    } while (1);

    LOG_E("Parsing err");
    return -RT_ERROR;
}


