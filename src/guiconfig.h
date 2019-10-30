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
#ifndef RT_THREAD_PRIORITY_MAX
# define RT_THREAD_PRIORITY_MAX             (32)
#endif


/* User Config */

/* RT-Thread Device Name */
#define CONFIG_GUI_DEVICE_NAME              "ILI9341"   // RGB565
#define CONFIG_TOUCH_DEVICE_NAME            "FT6206"
// #define CONFIG_GUI_DEVICE_NAME              "SSD1331"   // RGB565
// #define CONFIG_KEY_DEVICE_NAME              "BTN"
// #define CONFIG_GUI_DEVICE_NAME              "SSD1306"   // MONO

/* Color */
#define CONFIG_USING_MONO                   (0)
#define CONFIG_USING_RGB565                 (1)
#define CONFIG_USING_RGB565P                (0)
#define CONFIG_USING_RGB888                 (0)

/* Image Decoder */
#define CONFIG_USING_IMAGE_XPM              (1)
#define CONFIG_USING_IMAGE_BMP              (1)
#define CONFIG_USING_IMAGE_JPEG             (1)
#define CONFIG_USING_IMAGE_PNG              (0)

/* Font */
#define CONFIG_USING_FONT_12                (0)
#define CONFIG_USING_FONT_16                (1)
#define CONFIG_USING_FONT_HZ                (0)
#define CONFIG_USING_FONT_FILE              (1)

/* APP */
#define CONFIG_APP_PRIORITY                 (RTGUI_SERVER_PRIORITY + (RT_THREAD_PRIORITY_MAX >> 3))
#define CONFIG_APP_TIMESLICE                (RTGUI_SERVER_TIMESLICE)
#define CONFIG_APP_STACK_SIZE               (3 * 512)


/* Debug Config */

#define RTGUI_LOG_LEVEL                     (LOG_LVL_INFO)
#define RTGUI_CASTING_CHECK
// #define RTGUI_LOG_EVENT
// #define RTGUI_OBJECT_TRACE
// #define RTGUI_USING_CURSOR


/* Event Config */

#define RTGUI_MB_SIZE                       (16)
#define RTGUI_EVENT_POOL_NUMBER             (32)
#define RTGUI_EVENT_RESEND_DELAY            (RT_TICK_PER_SECOND / 50)


/* System Config */

#define RTGUI_SERVER_PRIORITY               ((RT_THREAD_PRIORITY_MAX >> 1) + (RT_THREAD_PRIORITY_MAX >> 3))
#define RTGUI_SERVER_TIMESLICE              (15)
#define RTGUI_SERVER_STACK_SIZE             (2 * 512)
#if (CONFIG_USING_MONO)
# define RTGUI_USING_FRAMEBUFFER
#endif


/* Color Config */

#define RTGUI_BIG_ENDIAN_OUTPUT
#ifdef RTGUI_USING_RGB888_AS_32BIT
# define RTGUI_RGB888_PIXEL_BITS 32
#else
# define RTGUI_RGB888_PIXEL_BITS 24
#endif


/* File Operation Config */

#define RTGUI_USING_DFS_FILERW


/* External Library Config*/

/* JPEG */
#define CONFIG_JPEG_BUFFER_SIZE             (4 * 1024)
#define CONFIG_JPEG_OUTPUT_RGB565           (1)

/* LodePNG */
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ERROR_TEXT
#define LODEPNG_NO_COMPILE_ALLOCATORS
#define LODEPNG_NO_COMPILE_CPP

#endif /* __GUICONFIG_H__ */
