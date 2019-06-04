/*
 * File      : rtgui_system.c
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
 * 2016-03-23     Bernard      fix the default font initialization issue.
 * 2019-05-15     onelife      refactor and rename to "arch.c"
 */

#include "include/rtgui.h"
#include "include/arch.h"
#include "include/app.h"
#include "include/image.h"
#include "include/font/font.h"
#include "include/types.h"
#include "include/widgets/window.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    LOG_LVL_DBG
// # define LOG_LVL                   LOG_LVL_INFO
# define LOG_TAG                    "GUI_SYS"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

// #define RTGUI_MEM_TRACE

#define SYNC_ACK_NUMBER             (1)

static struct rt_mailbox ack_sync;
static rt_uint8_t ack_pool[SYNC_ACK_NUMBER];

struct rt_mempool *rtgui_event_pool = RT_NULL;

const rtgui_color_t default_foreground = RTGUI_RGB(0x00, 0x00, 0x00);
const rtgui_color_t default_background = RTGUI_RGB(212, 208, 200);

static rtgui_rect_t _mainwin_rect;
static struct rt_mutex _screen_lock;


rt_err_t rtgui_system_server_init(void) {
    rt_err_t ret;

    do {
        ret = rt_mutex_init(&_screen_lock, "screen", RT_IPC_FLAG_FIFO);
        if (RT_EOK != ret) break;
        ret = rt_mb_init(&ack_sync, "ack", ack_pool, SYNC_ACK_NUMBER,
            RT_IPC_FLAG_FIFO);
        if (RT_EOK != ret) break;

        /* init image */
        ret = rtgui_system_image_init();
        if (RT_EOK != ret) break;
        /* init font */
        ret = rtgui_font_system_init();
        if (RT_EOK != ret) break;
        /* init rtgui server */
        rtgui_topwin_init();  // TODO:return err
        rtgui_server_init();
        /* use driver rect for main window */
        rtgui_graphic_driver_get_rect(rtgui_graphic_driver_get_default(),
            &_mainwin_rect);
    } while (0);

    return ret;
}
INIT_ENV_EXPORT(rtgui_system_server_init);

/************************************************************************/
/* RTGUI Timer                                                          */
/************************************************************************/
static void rtgui_time_out(void *param) {
    rtgui_evt_generic_t *evt;
    rt_err_t ret;
    rtgui_timer_t *timer;
    timer = (rtgui_timer_t *)param;

    if (RTGUI_TIMER_ST_RUNNING != timer->state) return;

    /* send RTGUI_EVENT_TIMER */
    evt = (rtgui_evt_generic_t *)rt_mp_alloc(rtgui_event_pool, RT_WAITING_NO);
    if (evt) {
        /* not in thread context */
        evt->timer.base.type = RTGUI_EVENT_TIMER;
        evt->timer.base.sender = RT_NULL;
        evt->timer.timer = timer;
        ret = rtgui_send(timer->app, evt, RT_WAITING_NO);
        if (ret) {
            LOG_E("timer evt err [%d]", ret);
            return;
        }
    } else {
        LOG_E("get mp err");
        return;
    }

    timer->pending_cnt++;
}

rtgui_timer_t *rtgui_timer_create(rt_int32_t time, rt_int32_t flag,
    rtgui_timeout_hdl_t timeout, void *param) {
    rtgui_timer_t *timer;

    timer = (rtgui_timer_t *)rtgui_malloc(sizeof(rtgui_timer_t));
    timer->app = rtgui_app_self();
    timer->timeout = timeout;
    timer->pending_cnt = 0;
    timer->state = RTGUI_TIMER_ST_INIT;
    timer->user_data = param;

    /* init rt-thread timer */
    rt_timer_init(&(timer->timer), "rtgui", rtgui_time_out, timer, time,
        (rt_uint8_t)flag);

    return timer;
}
RTM_EXPORT(rtgui_timer_create);

void rtgui_timer_destory(rtgui_timer_t *timer) {
    RT_ASSERT(timer != RT_NULL);

    /* stop timer firstly */
    rtgui_timer_stop(timer);
    if ((0 != timer->pending_cnt) && (0 != timer->app->ref_cnt)) {
        timer->state = RTGUI_TIMER_ST_DESTROY_PENDING;
    } else {
        /* detach rt-thread timer */
        rt_timer_detach(&(timer->timer));
        rtgui_free(timer);
    }
}
RTM_EXPORT(rtgui_timer_destory);

void rtgui_timer_set_timeout(rtgui_timer_t *timer, rt_int32_t time) {
    RT_ASSERT(timer != RT_NULL);

    /* start rt-thread timer */
    if (timer->state == RTGUI_TIMER_ST_RUNNING) {
        rtgui_timer_stop(timer);
        rt_timer_control(&timer->timer, RT_TIMER_CTRL_SET_TIME, &time);
        rtgui_timer_start(timer);
    } else {
        rt_timer_control(&timer->timer, RT_TIMER_CTRL_SET_TIME, &time);
    }
}
RTM_EXPORT(rtgui_timer_set_timeout);

void rtgui_timer_start(rtgui_timer_t *timer) {
    RT_ASSERT(timer != RT_NULL);

    /* start rt-thread timer */
    timer->state = RTGUI_TIMER_ST_RUNNING;
    rt_timer_start(&(timer->timer));
}
RTM_EXPORT(rtgui_timer_start);

void rtgui_timer_stop(rtgui_timer_t *timer) {
    RT_ASSERT(timer != RT_NULL);

    /* stop rt-thread timer */
    timer->state = RTGUI_TIMER_ST_INIT;
    rt_timer_stop(&(timer->timer));
}
RTM_EXPORT(rtgui_timer_stop);

/************************************************************************/
/* RTGUI Memory Management                                              */
/************************************************************************/
#ifdef RTGUI_MEM_TRACE
struct rtgui_mem_info {
    rt_uint32_t allocated_size;
    rt_uint32_t max_allocated;
};
struct rtgui_mem_info mem_info;

#define MEMTRACE_MAX        4096
#define MEMTRACE_HASH_SIZE  256

struct rti_memtrace_item {
    void       *mb_ptr;     /* memory block pointer */
    rt_uint32_t mb_len;     /* memory block length */

    struct rti_memtrace_item *next;
};
struct rti_memtrace_item trace_list[MEMTRACE_MAX];
struct rti_memtrace_item *item_hash[MEMTRACE_HASH_SIZE];
struct rti_memtrace_item *item_free;

rt_bool_t rti_memtrace_inited = 0;
void rti_memtrace_init(void) {
    struct rti_memtrace_item *item;
    rt_uint32_t index;

    rt_memset(trace_list, 0, sizeof(trace_list));
    rt_memset(item_hash, 0, sizeof(item_hash));

    item_free = &trace_list[0];
    item = &trace_list[0];

    for (index = 1; index < MEMTRACE_HASH_SIZE; index ++) {
        item->next = &trace_list[index];
        item = item->next;
    }

    item->next = RT_NULL;
}

void rti_malloc_hook(void *ptr, rt_uint32_t len) {
    rt_uint32_t index;
    struct rti_memtrace_item *item;

    if (RT_NULL == item_free) return;

    mem_info.allocated_size += len;
    if (mem_info.max_allocated < mem_info.allocated_size) {
        mem_info.max_allocated = mem_info.allocated_size;
    }

    /* lock context */
    item = item_free;
    item_free = item->next;

    item->mb_ptr = ptr;
    item->mb_len = len;
    item->next   = RT_NULL;

    /* get hash item index */
    index = ((rt_uint32_t)ptr) % MEMTRACE_HASH_SIZE;
    if (RT_NULL != item_hash[index]) {
        /* add to list */
        item->next = item_hash[index];
        item_hash[index] = item;
    } else {
        /* set list header */
        item_hash[index] = item;
    }
    /* unlock context */
}

void rti_free_hook(void *ptr) {
    rt_uint32_t index;
    struct rti_memtrace_item *item;

    /* get hash item index */
    index = ((rt_uint32_t)ptr) % MEMTRACE_HASH_SIZE;
    if (RT_NULL != item_hash[index]) {
        item = item_hash[index];
        if (item->mb_ptr == ptr) {
            /* delete item from list */
            item_hash[index] = item->next;
        } else {
            /* find ptr in list */
            while (item->next != RT_NULL && item->next->mb_ptr != ptr)
                item = item->next;

            /* delete item from list */
            if (item->next != RT_NULL) {
                struct rti_memtrace_item *i;

                i = item->next;
                item->next = item->next->next;

                item = i;
            } else {
                /* not found */
                return;
            }
        }

        /* reduce allocated size */
        mem_info.allocated_size -= item->mb_len;

        /* clear item */
        rt_memset(item, 0, sizeof(struct rti_memtrace_item));

        /* add item to the free list */
        item->next = item_free;
        item_free = item;
    }
}
#endif

//#define DEBUG_MEMLEAK

#undef rtgui_malloc
void *rtgui_malloc(rt_size_t size) {
    void *ptr;

    ptr = rt_malloc(size);
    #ifdef RTGUI_MEM_TRACE
        if (!rti_memtrace_inited) {
            rti_memtrace_init();
            rti_memtrace_inited = 1;
        }
        if (RT_NULL != ptr) rti_malloc_hook(ptr, size);
    #endif

    #ifdef DEBUG_MEMLEAK
        LOG_D("alloc %p (%d) on %p %.*s\n", ptr, size,
            __builtin_return_address(0), RT_NAME_MAX, rt_thread_self()->name);
    #endif

    return ptr;
}
RTM_EXPORT(rtgui_malloc);

#undef rtgui_realloc
void *rtgui_realloc(void *ptr, rt_size_t size) {
    void *new_ptr;

    #ifdef RTGUI_MEM_TRACE
        new_ptr = rtgui_malloc(size);
        if ((RT_NULL != new_ptr) && (RT_NULL != ptr)) {
            rt_memcpy(new_ptr, ptr, size);
            rtgui_free(ptr);
        }
    #else
        new_ptr = rt_realloc(ptr, size);
    #endif

    #ifdef DEBUG_MEMLEAK
        LOG_D("realloc %p to %p (%d) on %p %*.s\n", ptr, new_ptr, size,
            __builtin_return_address(0), RT_NAME_MAX, rt_thread_self()->name);
    #endif

    return new_ptr;
}
RTM_EXPORT(rtgui_realloc);

#undef rtgui_free
void rtgui_free(void *ptr) {
    #ifdef DEBUG_MEMLEAK
        LOG_D("dealloc %p on %p\n", ptr, __builtin_return_address(0));
    #endif

    #ifdef RTGUI_MEM_TRACE
        if (RT_NULL != ptr) rti_free_hook(ptr);
    #endif

    rt_free(ptr);
}
RTM_EXPORT(rtgui_free);


#if defined(RTGUI_MEM_TRACE) && defined(RT_USING_FINSH)
# include "components/finsh/finsh.h"

void list_guimem(void) {
    rt_kprintf("Current Used: %d, Maximal Used: %d\n",
        mem_info.allocated_size, mem_info.max_allocated);
}
FINSH_FUNCTION_EXPORT(list_guimem, display memory information);
#endif

/************************************************************************/
/* RTGUI Event Dump                                                     */
/************************************************************************/
#ifdef RTGUI_EVENT_DEBUG
    static void rtgui_event_dump(rtgui_app_t* app,
        rtgui_evt_generic_t *evt) {
        char *sender;

        // if ((evt->base.type == RTGUI_EVENT_TIMER) ||
        //     (evt->base.type == RTGUI_EVENT_UPDATE_BEGIN) ||
        //     (evt->base.type == RTGUI_EVENT_MOUSE_MOTION) ||
        //     (evt->base.type == RTGUI_EVENT_UPDATE_END)) {
        //     /* don't dump timer event */
        //     return;
        // }

        if (!evt->base.sender) {
            sender = "NULL";
        } else {
            sender = (char *)evt->base.sender->name;
        }

        if (evt->base.type >= RTGUI_EVENT_COMMAND) {
            rt_kprintf("[%-*.s] (%s) => [%-*.s]\n",  RT_NAME_MAX, sender,
                "CMD ",  RT_NAME_MAX, app->name);
            return;
        } else {
            rt_kprintf("[%-*.s] (%04x) => [%-*.s] { ", RT_NAME_MAX, sender,
                evt->base.type, RT_NAME_MAX, app->name);
        }

        switch (evt->base.type) {
        case RTGUI_EVENT_APP_CREATE:
        case RTGUI_EVENT_APP_DESTROY:
        case RTGUI_EVENT_APP_ACTIVATE:
            rt_kprintf("<app> %s", evt->app_create.app->name);
            break;

        case RTGUI_EVENT_PAINT:
            if (evt->paint.wid) {
                rt_kprintf("<win> %s", evt->paint.wid->title);
            }
            break;

        case RTGUI_EVENT_KBD:
            if (evt->kbd.wid) {
                rt_kprintf("<win> %s - %s", evt->kbd.wid->title,
                    RTGUI_KBD_IS_UP(&evt->kbd) ? "up" : "down");
            }
            break;

        case RTGUI_EVENT_CLIP_INFO:
            if (evt->clip_info.wid) {
                rt_kprintf("<win> %s", evt->clip_info.wid->title);
            }
            break;

        case RTGUI_EVENT_WIN_CREATE:
            rt_kprintf("<win> %s @%p (x1:%03d, y1:%03d, x2:%03d, y2:%03d)",
                evt->win_create.wid->title, evt->win_create.wid,
                TO_WIDGET(evt->win_create.wid)->extent.x1,
                TO_WIDGET(evt->win_create.wid)->extent.y1,
                TO_WIDGET(evt->win_create.wid)->extent.x2,
                TO_WIDGET(evt->win_create.wid)->extent.y2);
            break;

        case RTGUI_EVENT_UPDATE_END:
            rt_kprintf("(x1:%03d, y1:%03d, x2:%03d, y2:%03d)",
                evt->update_end.rect.x1, evt->update_end.rect.y1,
                evt->update_end.rect.x2, evt->update_end.rect.y2);
            break;

        case RTGUI_EVENT_WIN_ACTIVATE:
        case RTGUI_EVENT_WIN_DESTROY:
        case RTGUI_EVENT_WIN_CLOSE:
        case RTGUI_EVENT_WIN_DEACTIVATE:
        case RTGUI_EVENT_WIN_SHOW:
        case RTGUI_EVENT_WIN_HIDE:
        case RTGUI_EVENT_WIN_MODAL_ENTER:
            if (evt->win_base.wid) {
                rt_kprintf("<win> %s", evt->win_base.wid->title);
            }
            break;

        case RTGUI_EVENT_WIN_MOVE:
            if (evt->win_move.wid) {
                rt_kprintf("<win> %s move (%03d, %03d)",
                    evt->win_move.wid->title, evt->win_move.x, evt->win_move.y);
            }
            break;

        case RTGUI_EVENT_WIN_RESIZE:
            if (evt->win_resize.wid) {
                rt_kprintf("<win> %s resize (x1:%d, y1:%d, x2:%d, y2:%d)",
                    evt->win_resize.wid->title,
                    TO_WIDGET(evt->win_resize.wid)->extent.x1,
                    TO_WIDGET(evt->win_resize.wid)->extent.y1,
                    TO_WIDGET(evt->win_resize.wid)->extent.x2,
                    TO_WIDGET(evt->win_resize.wid)->extent.y2);
            }
            break;

        case RTGUI_EVENT_MOUSE_BUTTON:
        case RTGUI_EVENT_MOUSE_MOTION:
            rt_kprintf("<mouse> (%03d, %03d) %s %s", evt->mouse.x, evt->mouse.y,
                (evt->mouse.button & RTGUI_MOUSE_BUTTON_LEFT) ? "L" : "R",
                (evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) ? "down" : "up");
            if (evt->mouse.wid) {
                rt_kprintf(" on <win> %s", evt->mouse.wid->title);
            }
            break;

        case RTGUI_EVENT_MONITOR_ADD:
            if (RT_NULL != evt->monitor.wid) {
                rt_kprintf("<win> %s monitor (x1:%d, y1:%d, x2:%d, y2:%d)",
                    evt->monitor.wid->title,
                    evt->monitor.rect.x1, evt->monitor.rect.y1,
                    evt->monitor.rect.x2, evt->monitor.rect.y2);
            }
            break;

        default:
            break;
        }
        rt_kprintf(" }\n");
    }

#else /* RTGUI_EVENT_DEBUG */
# define rtgui_event_dump(app, evt)

#endif /* RTGUI_EVENT_DEBUG */

/************************************************************************/
/* RTGUI IPC APIs                                                       */
/************************************************************************/
rt_err_t rtgui_send(rtgui_app_t* app, rtgui_evt_generic_t *evt,
    rt_int32_t timeout) {
    rt_err_t ret;

    rtgui_event_dump(app, evt);

    evt->base.ack = RT_NULL;
    ret = rt_mb_send_wait(app->mb, (rt_ubase_t)evt, timeout);
    if (ret) {
        if (RTGUI_EVENT_TIMER != evt->base.type) {
            LOG_E("tx evt %d to %s err %d", evt->base.type, app->name, ret);
            rt_mp_free(evt);
        }
    }

    return ret;
}
RTM_EXPORT(rtgui_send);

rt_err_t rtgui_send_sync(rtgui_app_t* dst, rtgui_evt_generic_t *evt) {
    rt_ubase_t ack;
    rt_err_t ret;

    rtgui_event_dump(dst, evt);
    ret = RT_EOK;

    do {
        evt->base.ack = &ack_sync;
        ret = rt_mb_send(dst->mb, (rt_ubase_t)evt);
        if (ret) {
            if (RTGUI_EVENT_TIMER != evt->base.type) {
                LOG_E("tx sync %d err %d", evt->base.type, ret);
                rt_mp_free(evt);
            }
            break;
        }

        ret = rt_mb_recv(&ack_sync, &ack, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("rx ack err %d", ret);
            break;
        }
        if (ack != RT_EOK) {
            LOG_E("ack err %d", ack);
            ret = -RT_ERROR;
        }
        LOG_D("ack %d", ack);
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_send_sync);

rt_err_t rtgui_ack(rtgui_evt_generic_t *evt, rt_uint32_t val) {
    if (!evt->base.ack) {
        LOG_W("no ack return");
        return RT_EOK;
    }
    return rt_mb_send(evt->base.ack, val);
}
RTM_EXPORT(rtgui_ack);

rt_err_t rtgui_recv(rtgui_app_t *app, rtgui_evt_generic_t **evt,
    rt_int32_t timeout) {
    return rt_mb_recv(app->mb, (rt_ubase_t *)evt, timeout);
}
RTM_EXPORT(rtgui_recv);

rt_err_t rtgui_recv_filter(rtgui_app_t *app, rt_uint32_t type,
    rtgui_evt_generic_t **evt) {
    rt_err_t ret;

    do {
        rtgui_evt_generic_t *_evt;
        ret = rtgui_recv(app, &_evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("filter rx err [%d]", ret);
            break;
        }
        if (type == _evt->base.type) {
            *evt = _evt;
            break;
        } else {
            if (EVENT_HANDLER(app)) {
                (void)EVENT_HANDLER(app)(app, _evt);
            }
        }
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_recv_filter);

void rtgui_set_mainwin_rect(rtgui_rect_t *rect) {
    _mainwin_rect = *rect;
}
RTM_EXPORT(rtgui_set_mainwin_rect);

void rtgui_get_mainwin_rect(rtgui_rect_t *rect) {
    *rect = _mainwin_rect;
}
RTM_EXPORT(rtgui_get_mainwin_rect);

void rtgui_get_screen_rect(rtgui_rect_t *rect) {
    rtgui_graphic_driver_get_rect(rtgui_graphic_driver_get_default(), rect);
}
RTM_EXPORT(rtgui_get_screen_rect);

void rtgui_screen_lock(rt_int32_t timeout) {
    rt_mutex_take(&_screen_lock, timeout);
}
RTM_EXPORT(rtgui_screen_lock);

void rtgui_screen_unlock(void) {
    rt_mutex_release(&_screen_lock);
}
RTM_EXPORT(rtgui_screen_unlock);

int rtgui_screen_lock_freeze(void) {
    int hold = 0;

    if (_screen_lock.owner == rt_thread_self())
    {
        int index;

        index = hold = _screen_lock.hold;
        while (index --) rt_mutex_release(&_screen_lock);
    }

    return hold;
}
RTM_EXPORT(rtgui_screen_lock_freeze);

void rtgui_screen_lock_thaw(int value)
{
    while (value--) rt_mutex_take(&_screen_lock, RT_WAITING_FOREVER);
}
RTM_EXPORT(rtgui_screen_lock_thaw);

