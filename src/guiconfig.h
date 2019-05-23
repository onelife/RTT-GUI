/*
 * File      : guiconfig.h
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
 * 2009-10-16     Bernard      first version
 * 2010-02-08     Bernard      move some RTGUI options to bsp
 * 2019-05-23     onelife      rename to "guiconfig.h"
 */

#ifndef __GUICONFIG_H__
#define __GUICONFIG_H__

#define CONFIG_GUI_DEVICE_NAME              "ILI9341"


#define RTGUI_MQ_SIZE                       (16)
#define RTGUI_MB_SIZE                       (16)
#define RTGUI_EVENT_POOL_NUMBER             (32)
#define RTGUI_EVENT_RESEND_DELAY            (RT_TICK_PER_SECOND / 50)

#define RTGUI_LOG_LEVEL                     (LOG_LVL_DBG)
#define GUIENGIN_USING_CAST_CHECK
// #define RTGUI_OBJECT_TRACE

#ifndef GUIENGINE_SVR_THREAD_PRIORITY
# define GUIENGINE_SVR_THREAD_PRIORITY      (20)
#endif
#ifndef GUIENGINE_SVR_THREAD_TIMESLICE
# define GUIENGINE_SVR_THREAD_TIMESLICE     (10)
#endif
#ifndef GUIENGIN_SVR_THREAD_STACK_SIZE
# define GUIENGIN_SVR_THREAD_STACK_SIZE      (4 * 1024 )
#endif

// #define GUIENGIN_APP_THREAD_PRIORITY        25
// #define GUIENGIN_APP_THREAD_TIMESLICE       10
// #define GUIENGIN_APP_THREAD_STACK_SIZE      4 * 1024

#ifdef RT_USING_DFS
# ifndef GUIENGINE_USING_DFS_FILERW
#  define GUIENGINE_USING_DFS_FILERW
# endif
#else
# ifdef GUIENGINE_USING_DFS_FILERW
#   undef GUIENGINE_USING_DFS_FILERW
# endif
# ifdef GUIENGINE_USING_HZ_FILE
#  undef GUIENGINE_USING_HZ_FILE
# endif
#endif


#define RTGUI_EVENT_DEBUG



// #define GUIENGIN_USING_CAST_CHECK

// #define GUIENGIN_USING_DESKTOP_WINDOW

// #define GUIENGIN_USING_CALIBRATION

// #define GUIENGIN_USING_VFRAMEBUFFER

#ifndef PKG_USING_RGB888_PIXEL_BITS_32
# ifndef PKG_USING_RGB888_PIXEL_BITS_24
#  define PKG_USING_RGB888_PIXEL_BITS_32
#  define PKG_USING_RGB888_PIXEL_BITS 32
# endif
#endif

#ifdef DEBUG_MEMLEAK
# define rtgui_malloc     rt_malloc
# define rtgui_realloc    rt_realloc
# define rtgui_free       rt_free
#endif

#endif

