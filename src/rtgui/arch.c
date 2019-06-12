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
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"
#include "include/arch.h"
#include "include/app.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                    RTGUI_LOG_LEVEL
# define LOG_TAG                    "GUI_ARC"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)     rt_kprintf(format "\n", ##args)
# define LOG_D                      LOG_E
#endif /* RT_USING_ULOG */

/* Private function prototype ------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#ifndef RTGUI_EVENT_LOG
# define rtgui_log_event(tgt, evt)
#endif
// #define RTGUI_MEM_TRACE
#define SYNC_ACK_NUMBER             (1)

/* Private variables ---------------------------------------------------------*/
static struct rt_mutex _screen_lock;
static struct rt_mailbox ack_sync;
static rt_uint8_t ack_pool[SYNC_ACK_NUMBER];
static rtgui_rect_t _mainwin_rect;

/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/
rt_err_t rtgui_system_init(void) {
    rt_err_t ret;

    do {
        ret = rt_mutex_init(&_screen_lock, "screen", RT_IPC_FLAG_FIFO);
        if (RT_EOK != ret) break;
        ret = rt_mb_init(&ack_sync, "ack", ack_pool, SYNC_ACK_NUMBER,
            RT_IPC_FLAG_FIFO);
        if (RT_EOK != ret) break;
        ret = rtgui_system_image_init();
        if (RT_EOK != ret) break;
        ret = rtgui_font_system_init();
        if (RT_EOK != ret) break;
        ret = rtgui_topwin_init();
        if (RT_EOK != ret) break;
        ret = rtgui_server_init();
        if (RT_EOK != ret) break;
        /* use h/w rect for main window */
        rtgui_graphic_driver_get_rect(rtgui_graphic_driver_get_default(),
            &_mainwin_rect);
    } while (0);

    return ret;
}
INIT_ENV_EXPORT(rtgui_system_init);

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
        evt->timer.base.origin = RT_NULL;
        evt->timer.timer = timer;
        ret = rtgui_request(timer->app, evt, RT_WAITING_NO);
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
#endif /* RTGUI_MEM_TRACE */

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

    return ptr;
}
RTM_EXPORT(rtgui_malloc);

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

    return new_ptr;
}
RTM_EXPORT(rtgui_realloc);

void rtgui_free(void *ptr) {
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

/***************************************************************************//**
 * Event Log
 ******************************************************************************/
#ifdef RTGUI_EVENT_LOG
const char *rtgui_event_text(rtgui_evt_generic_t *evt) {
    switch (evt->base.type) {
    case RTGUI_EVENT_APP_CREATE:    return "<App>Create";
    case RTGUI_EVENT_APP_DESTROY:   return "<App>Destory";
    case RTGUI_EVENT_APP_ACTIVATE:  return "<App>Active";
    /* WM event */
    case RTGUI_EVENT_SET_WM:        return "<WM>Set";
    /* win event */
    case RTGUI_EVENT_WIN_CREATE:    return "<Win>Create";
    case RTGUI_EVENT_WIN_DESTROY:   return "<Win>Destory";
    case RTGUI_EVENT_WIN_SHOW:      return "<Win>Show";
    case RTGUI_EVENT_WIN_HIDE:      return "<Win>Hide";
    case RTGUI_EVENT_WIN_MOVE:      return "<Win>Move";
    case RTGUI_EVENT_WIN_RESIZE:    return "<Win>Resize";
    case RTGUI_EVENT_WIN_CLOSE:     return "<Win>Close";
    case RTGUI_EVENT_WIN_ACTIVATE:  return "<Win>Active";
    case RTGUI_EVENT_WIN_DEACTIVATE:    return "<Win>Deactive";
    case RTGUI_EVENT_WIN_UPDATE_END:    return "<Win>UpdateEnd";
    /* sent after window setup and before the app setup */
    case RTGUI_EVENT_WIN_MODAL_ENTER:   return "<Win>ModelEnter";
    /* widget event */
    case RTGUI_EVENT_SHOW:          return "<EVT>Show";
    case RTGUI_EVENT_HIDE:          return "<EVT>Hide";
    case RTGUI_EVENT_PAINT:         return "<EVT>Paint";
    case RTGUI_EVENT_FOCUSED:       return "<EVT>Focus";
    case RTGUI_EVENT_SCROLLED:      return "<EVT>Scroll";
    case RTGUI_EVENT_RESIZE:        return "<EVT>Resize";
    case RTGUI_EVENT_SELECTED:      return "<EVT>Select";
    case RTGUI_EVENT_UNSELECTED:    return "<EVT>Unselect";
    case RTGUI_EVENT_MV_MODEL:      return "<EVT>MvModel";
    case RTGUI_EVENT_UPDATE_TOPLVL: return "<EVT>UpdateTop";
    case RTGUI_EVENT_UPDATE_BEGIN:  return "<EVT>UpdateBegin";
    case RTGUI_EVENT_UPDATE_END:    return "<EVT>UpdateEnd";
    case RTGUI_EVENT_MONITOR_ADD:   return "<EVT>MonitorAdd";
    case RTGUI_EVENT_MONITOR_REMOVE:    return "<EVT>MonitorRemove";
    case RTGUI_EVENT_TIMER:         return "<EVT>Timer";
    /* virtual paint event (server -> client) */
    case RTGUI_EVENT_VPAINT_REQ:    return "<EVT>VPaintReq";
    /* clip rect information */
    case RTGUI_EVENT_CLIP_INFO:     return "<EVT>ClipInfo";
    /* mouse and keyboard event */
    case RTGUI_EVENT_MOUSE_MOTION:  return "<Mouse>Motion";
    case RTGUI_EVENT_MOUSE_BUTTON:  return "<Mouse>Button";
    case RTGUI_EVENT_KBD:           return "<KBD>";
    case RTGUI_EVENT_TOUCH:         return "<Touch>";
    case RTGUI_EVENT_GESTURE:       return "<Gesture>";
    case WBUS_NOTIFY_EVENT:         return "<WBUS>";
    /* user command (at bottom) */
    case RTGUI_EVENT_COMMAND:       return "<CMD>";
    default:                        return "<?>";
    }
}

static void rtgui_log_event(rtgui_app_t* tgt, rtgui_evt_generic_t *evt) {
    // if ((evt->base.type == RTGUI_EVENT_TIMER) ||
    //     (evt->base.type == RTGUI_EVENT_UPDATE_BEGIN) ||
    //     (evt->base.type == RTGUI_EVENT_MOUSE_MOTION) ||
    //     (evt->base.type == RTGUI_EVENT_UPDATE_END)) {
    //     /* don't dump timer event */
    //     return;
    // }

    rt_kprintf("[%-*.s ===> %-*.s]%-*.s",
        10, evt->base.origin ? (char *)evt->base.origin->name : "NULL",
        10, tgt->name, 10, rtgui_event_text(evt));

    switch (evt->base.type) {
    case RTGUI_EVENT_APP_CREATE:
    case RTGUI_EVENT_APP_DESTROY:
    case RTGUI_EVENT_APP_ACTIVATE:
        rt_kprintf(": %s", evt->app_create.app->name);
        break;

    case RTGUI_EVENT_PAINT:
        rt_kprintf(": %s",
            evt->paint.wid ? evt->paint.wid->title : "???");
        break;

    case RTGUI_EVENT_KBD:
        rt_kprintf(": <%s> %s",
            RTGUI_KBD_IS_UP(&evt->kbd) ? "up" : "down",
            evt->kbd.wid ? evt->kbd.wid->title : "???");
        break;

    case RTGUI_EVENT_CLIP_INFO:
        rt_kprintf(": %s",
            evt->clip_info.wid ? evt->clip_info.wid->title : "???");
        break;

    case RTGUI_EVENT_WIN_CREATE:
        rt_kprintf(": (%03d, %03d)-(%03d, %03d) %s @%p",
            TO_WIDGET(evt->win_create.wid)->extent.x1,
            TO_WIDGET(evt->win_create.wid)->extent.y1,
            TO_WIDGET(evt->win_create.wid)->extent.x2,
            TO_WIDGET(evt->win_create.wid)->extent.y2,
            evt->win_create.wid->title, evt->win_create.wid);
        break;

    case RTGUI_EVENT_UPDATE_END:
        rt_kprintf(": (%03d, %03d)-(%03d, %03d)",
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
        rt_kprintf(": %s",
            evt->win_base.wid ? evt->win_base.wid->title : "???");
        break;

    case RTGUI_EVENT_WIN_MOVE:
        rt_kprintf(": (%03d, %03d) %s",
            evt->win_move.x, evt->win_move.y,
            evt->win_move.wid ? evt->win_move.wid->title : "???");
        break;

    case RTGUI_EVENT_WIN_RESIZE:
    rt_kprintf(": (%03d, %03d)-(%03d, %03d) %s",
        TO_WIDGET(evt->win_resize.wid)->extent.x1,
        TO_WIDGET(evt->win_resize.wid)->extent.y1,
        TO_WIDGET(evt->win_resize.wid)->extent.x2,
        TO_WIDGET(evt->win_resize.wid)->extent.y2,
        evt->win_resize.wid ? evt->win_resize.wid->title : "???");
        break;

    case RTGUI_EVENT_MOUSE_BUTTON:
    case RTGUI_EVENT_MOUSE_MOTION:
        rt_kprintf("(%03d, %03d) <%s %s> %s", evt->mouse.x, evt->mouse.y,
            (evt->mouse.button & RTGUI_MOUSE_BUTTON_LEFT) ? "L" : "R",
            (evt->mouse.button & RTGUI_MOUSE_BUTTON_DOWN) ? "down" : "up",
            evt->mouse.wid ? evt->mouse.wid->title : "???");
        break;

    case RTGUI_EVENT_MONITOR_ADD:
        rt_kprintf(": (%03d, %03d)-(%03d, %03d) %s",
            evt->monitor.rect.x1, evt->monitor.rect.y1,
            evt->monitor.rect.x2, evt->monitor.rect.y2,
            evt->monitor.wid ? evt->monitor.wid->title : "???");
        break;

    default:
        break;
    }
    rt_kprintf("\n");
}
#endif /* RTGUI_EVENT_LOG */

/***************************************************************************//**
 * Server/Client API
 ******************************************************************************/
rt_err_t rtgui_request(rtgui_app_t* tgt, rtgui_evt_generic_t *evt,
    rt_int32_t timeout) {
    rt_err_t ret;

    rtgui_log_event(tgt, evt);

    evt->base.ack = RT_NULL;
    ret = rt_mb_send_wait(tgt->mb, (rt_ubase_t)evt, timeout);
    if (ret) {
        LOG_E("tx evt %d to %s err [%d]", evt->base.type, tgt->name, ret);
        RTGUI_FREE_EVENT(evt);
    }

    #ifdef RTGUI_EVENT_LOG
        LOG_I("[EVT] Request by MB");
    #endif
    return ret;
}
RTM_EXPORT(rtgui_request);

rt_err_t rtgui_request_sync(rtgui_app_t* dst, rtgui_evt_generic_t *evt) {
    rt_ubase_t ack;
    rt_err_t ret;

    rtgui_log_event(dst, evt);
    ret = RT_EOK;

    do {
        evt->base.ack = &ack_sync;
        ret = rt_mb_send(dst->mb, (rt_ubase_t)evt);
        if (ret) {
            LOG_E("tx sync %d err %d", evt->base.type, ret);
            RTGUI_FREE_EVENT(evt);
            break;
        }
        #ifdef RTGUI_EVENT_LOG
            LOG_I("[EVT] Sync request by MB");
        #endif

        ret = rt_mb_recv(&ack_sync, &ack, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("rx ack err %d", ret);
            break;
        }
        if (ack != RT_EOK) {
            LOG_E("ack err %d", ack);
            ret = -RT_ERROR;
        }
        #ifdef RTGUI_EVENT_LOG
            LOG_I("[EVT] Sync response %d", ack);
        #endif
    } while (0);

    return ret;
}
RTM_EXPORT(rtgui_request_sync);

rt_err_t rtgui_response(rtgui_evt_generic_t *evt, rt_uint32_t val) {
    if (!evt->base.ack) {
        LOG_W("no ack return");
        return RT_EOK;
    }
    return rt_mb_send(evt->base.ack, val);
}
RTM_EXPORT(rtgui_response);

rt_err_t rtgui_wait(rtgui_app_t *tgt, rtgui_evt_generic_t **evt,
    rt_int32_t timeout) {
    rt_err_t ret;

    ret = rt_mb_recv(tgt->mb, (rt_ubase_t *)evt, timeout);
    #ifdef RTGUI_EVENT_LOG
        LOG_I("[EVT] Got response");
    #endif

    return ret;
}
RTM_EXPORT(rtgui_wait);

rt_err_t rtgui_recv_filter(rtgui_app_t *tgt, rt_uint32_t type,
    rtgui_evt_generic_t **evt) {
    rt_err_t ret;

    do {
        rtgui_evt_generic_t *_evt;
        ret = rtgui_wait(tgt, &_evt, RT_WAITING_FOREVER);
        if (ret) {
            LOG_E("filter rx err [%d]", ret);
            break;
        }
        if (type == _evt->base.type) {
            *evt = _evt;
            break;
        } else {
            if (EVENT_HANDLER(tgt)) {
                (void)EVENT_HANDLER(tgt)(tgt, _evt);
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

