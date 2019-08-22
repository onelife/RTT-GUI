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

#ifndef CONFIG_ARDUINO
# define CONFIG_ARDUINO
#endif

#define CONFIG_GUI_DEVICE_NAME              "ILI9341"
#define CONFIG_TOUCH_DEVICE_NAME            "FT6206"

#define CONFIG_JPEG_BUFFER_SIZE             (4 * 1024) 
#define CONFIG_JPEG_OUTPUT_RGB565           (1)
#define RTGUI_BIG_ENDIAN_OUTPUT
#define CONFIG_USING_FONT_FILE              (1)

#define RTGUI_MQ_SIZE                       (32)
#define RTGUI_MB_SIZE                       (32)
#define RTGUI_EVENT_POOL_NUMBER             (64)
#define RTGUI_EVENT_RESEND_DELAY            (RT_TICK_PER_SECOND / 50)


#define RTGUI_LOG_LEVEL                     (LOG_LVL_DBG)
#define RTGUI_CASTING_CHECK
// #define RTGUI_OBJECT_TRACE
#define RTGUI_LOG_EVENT
#define RTGUI_USING_CURSOR
// #define RTGUI_USING_DC_BUFFER

/* Font */
// #define GUIENGINE_USING_FONT12
#define GUIENGINE_USING_FONT16
#define GUIENGINE_USING_FONTHZ
#if CONFIG_USING_FONT_FILE
# define RTGUI_USING_FONT_FILE
#endif

/* Image Decoder */
#define GUIENGINE_IMAGE_XPM
#define GUIENGINE_IMAGE_BMP
#define GUIENGINE_IMAGE_JPEG
// #define GUIENGINE_IMAGE_PNG

/* LodePNG */
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ERROR_TEXT
#define LODEPNG_NO_COMPILE_ALLOCATORS
#define LODEPNG_NO_COMPILE_CPP


#define GUIENGINE_USING_DFS_FILERW

#ifndef GUIENGINE_SVR_THREAD_PRIORITY
# define GUIENGINE_SVR_THREAD_PRIORITY      (20)
#endif
#ifndef GUIENGINE_SVR_THREAD_TIMESLICE
# define GUIENGINE_SVR_THREAD_TIMESLICE     (10)
#endif
#ifndef GUIENGIN_SVR_THREAD_STACK_SIZE
# define GUIENGIN_SVR_THREAD_STACK_SIZE     (4 * 1024)
#endif

// #define GUIENGIN_APP_THREAD_PRIORITY        25
// #define GUIENGIN_APP_THREAD_TIMESLICE       10
// #define GUIENGIN_APP_THREAD_STACK_SIZE      4 * 1024

// #define GUIENGIN_USING_DESKTOP_WINDOW
// #define GUIENGIN_USING_CALIBRATION
// #define GUIENGIN_USING_VFRAMEBUFFER

#ifndef GUIENGINE_RGB888_PIXEL_BITS
# ifdef GUIENGINE_USING_RGB888_AS_32BIT
#  define GUIENGINE_RGB888_PIXEL_BITS 32
# else
#  define GUIENGINE_RGB888_PIXEL_BITS 24
# endif
#endif

#endif /* __GUICONFIG_H__ */
