/*
 * File      : fileview.c
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
 * 2010-01-06     Bernard      first version
 * 2019-07-04     onelife      refactor
 */
/* Includes ------------------------------------------------------------------*/
#include "include/rtgui.h"

#ifndef RT_USING_DFS
# error "Please enable RT_USING_DFS for filelist"
#endif
#include "components/dfs/include/dfs_posix.h"

#include "include/image.h"
#include "include/widgets/container.h"
#include "include/widgets/list.h"
#include "include/widgets/filelist.h"

#ifdef RT_USING_ULOG
# define LOG_LVL                        RTGUI_LOG_LEVEL
# define LOG_TAG                        "WGT_FLS"
# include "components/utilities/ulog/ulog.h"
#else /* RT_USING_ULOG */
# define LOG_E(format, args...)         rt_kprintf(format "\n", ##args)
# define LOG_W                          LOG_E
# define LOG_D                          LOG_E
#endif /* RT_USING_ULOG */

#define PATH_SEPARATOR                  '/'
#define MAX_PATH_LENGTH                 (128)

/* Private function prototype ------------------------------------------------*/
static void _filelist_constructor(void *obj);
static void _filelist_destructor(void *obj);
static void _filelist_clear_items(rtgui_filelist_t *filelist);
static rt_bool_t _filelist_on_item(void *obj, rtgui_evt_generic_t *evt);

/* Private typedef -----------------------------------------------------------*/
struct fileview_contex {
    rt_uint32_t count;
    rtgui_image_t *file_img;
    rtgui_image_t *folder_img;
};

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTGUI_CLASS(
    filelist,
    CLASS_METADATA(list),
    _filelist_constructor,
    _filelist_destructor,
    RT_NULL,
    sizeof(rtgui_filelist_t));

static const char * const file_xpm[] = {
    "16 16 21 1",
    "  c None",
    ". c #999999",
    "+ c #818181",
    "@ c #FFFFFF",
    "# c #ECECEC",
    "$ c #EAEAEA",
    "% c #EBEBEB",
    "& c #EDEDED",
    "* c #F0F0F0",
    "= c #C4C4C4",
    "- c #C5C5C5",
    "; c #C6C6C6",
    "> c #C7C7C7",
    ", c #EEEEEE",
    "' c #EDEDE5",
    ") c #EDEDE6",
    "! c #EFEFEF",
    "~ c #C8C8C8",
    "{ c #F1F1F1",
    "] c #F2F2F2",
    "^ c #959595",
    ".++++++++++++   ",
    "+@@@@@@@@@@@@+  ",
    "+@#$$%%%##&*@+  ",
    "+@$=--;;;;>*@+  ",
    "+@$%%###&&,*@+  ",
    "+@%-;;;;;;>*@+  ",
    "+@%%##&&'#,*@+  ",
    "+@%;;;;,,),*@+  ",
    "+@##&&,,!!!*@+  ",
    "+@#;;;>>~~~*@+  ",
    "+@#&,,!!*{{{@+  ",
    "+@&;>>~~~{{]@+  ",
    "+@&&,!!**{]]@+  ",
    "+@@@@@@@@@@@@+  ",
    "^++++++++++++^  ",
    "                "
};

static const char * const folder_xpm[] = {
    "16 16 121 2",
    "  c None",
    ". c #D9B434",
    "+ c #E1C25E",
    "@ c #E2C360",
    "# c #E2C35F",
    "$ c #DBB63C",
    "% c #DAB336",
    "& c #FEFEFD",
    "* c #FFFFFE",
    "= c #FFFEFE",
    "- c #FFFEFD",
    "; c #FBF7EA",
    "> c #E4C76B",
    ", c #E3C76B",
    "' c #E6CD79",
    ") c #E5CA74",
    "! c #DAAF35",
    "~ c #FEFCF7",
    "{ c #F8E48E",
    "] c #F5DE91",
    "^ c #F5E09F",
    "/ c #F6E1AC",
    "( c #FEFBEF",
    "_ c #FEFDF4",
    ": c #FEFCF3",
    "< c #FEFCF1",
    "[ c #FEFBEE",
    "} c #FFFDFA",
    "| c #DAAF36",
    "1 c #DAAA36",
    "2 c #FDFAF1",
    "3 c #F5DE94",
    "4 c #F4DC93",
    "5 c #F2D581",
    "6 c #EDCA6A",
    "7 c #EACB6C",
    "8 c #EFD385",
    "9 c #EFD280",
    "0 c #EFD07A",
    "a c #EECF76",
    "b c #EECF72",
    "c c #FBF7E9",
    "d c #DAAE34",
    "e c #DAAB35",
    "f c #FBF6E8",
    "g c #EFD494",
    "h c #EECE88",
    "i c #E9C173",
    "j c #F6E9C9",
    "k c #FEFCF2",
    "l c #FEFCF0",
    "m c #DAAB36",
    "n c #DAA637",
    "o c #FFFDF8",
    "p c #FFFDF6",
    "q c #FFFCF5",
    "r c #FCF6D8",
    "s c #F8E694",
    "t c #F7E385",
    "u c #F6DF76",
    "v c #F5DB68",
    "w c #F4D85C",
    "x c #FCF4D7",
    "y c #DAA435",
    "z c #DAA136",
    "A c #FEFCF6",
    "B c #FCF2C8",
    "C c #FBEFB9",
    "D c #FAECAC",
    "E c #F9E89C",
    "F c #F7E38B",
    "G c #F6E07C",
    "H c #F6DC6C",
    "I c #F5D95D",
    "J c #F4D64F",
    "K c #F3D344",
    "L c #FCF3D0",
    "M c #DA9F35",
    "N c #DA9A36",
    "O c #FDFAF2",
    "P c #FAEDB3",
    "Q c #F9E9A4",
    "R c #F8E695",
    "S c #F7E285",
    "T c #F6DE76",
    "U c #F5DB65",
    "V c #F4D757",
    "W c #F3D449",
    "X c #F2D13B",
    "Y c #F1CE30",
    "Z c #FBF2CC",
    "` c #DA9835",
    " . c #DA9435",
    ".. c #FEFAEF",
    "+. c #F9E9A1",
    "@. c #F8E591",
    "#. c #F7E181",
    "$. c #F6DE72",
    "%. c #F5DA63",
    "&. c #F4D754",
    "*. c #F3D347",
    "=. c #F2D039",
    "-. c #F1CD2E",
    ";. c #F0CB26",
    ">. c #FBF2CA",
    ",. c #D98E33",
    "'. c #FAF0DC",
    "). c #F4DDA7",
    "!. c #F4DB9E",
    "~. c #F3DA96",
    "{. c #F3D88E",
    "]. c #F3D786",
    "^. c #F2D47F",
    "/. c #F2D379",
    "(. c #F1D272",
    "_. c #F1D06C",
    ":. c #F1CF69",
    "<. c #F8EAC2",
    "[. c #D8882D",
    "}. c #D8872D",
    "|. c #D8862C",
    "                                ",
    "                                ",
    "                                ",
    "  . + @ @ @ # $                 ",
    "  % & * = - * ; > , , , ' )     ",
    "  ! ~ { ] ^ / ( _ : < ( [ } |   ",
    "  1 2 3 4 5 6 7 8 9 0 a b c d   ",
    "  e f g h i j k : k l ( [ * m   ",
    "  n * o p q : r s t u v w x y   ",
    "  z A B C D E F G H I J K L M   ",
    "  N O P Q R S T U V W X Y Z `   ",
    "   ...+.@.#.$.%.&.*.=.-.;.>. .  ",
    "  ,.'.).!.~.{.].^./.(._.:.<.,.  ",
    "    [.}.[.[.[.[.[.[.[.[.}.[.|.  ",
    "                                ",
    "                                "
};

static const rtgui_list_item_t _folder_act[] = {
    #ifdef RTGUI_USING_FONTHZ
        {"打开", RT_NULL},
        {"选择", RT_NULL},
        {"取消", RT_NULL}
    #else
        {"Open", RT_NULL},
        {"Select", RT_NULL},
        {"Cancel", RT_NULL}
    #endif
};

static struct fileview_contex _contex = {
    .count = 0,
    .file_img = RT_NULL,
    .folder_img = RT_NULL,
};

/* Private functions ---------------------------------------------------------*/
static void _filelist_constructor(void *obj) {
    rtgui_filelist_t *filelist = obj;

    LIST_SETTER(on_item)(TO_LIST(obj), _filelist_on_item);

    filelist->cur_dir = RT_NULL;
    // filelist->pattern = RT_NULL;
    filelist->on_file = RT_NULL;

    if (!_contex.count) {
        _contex.file_img = rtgui_image_create_from_mem("xpm",
            (const rt_uint8_t *)file_xpm, sizeof(file_xpm), -1, RT_TRUE);
        _contex.folder_img = rtgui_image_create_from_mem("xpm",
            (const rt_uint8_t *)folder_xpm, sizeof(folder_xpm), -1, RT_TRUE);
    }
    _contex.count++;
}

static void  _filelist_destructor(void *obj) {
    rtgui_filelist_t *filelist = obj;

    _filelist_clear_items(filelist);
    if (filelist->cur_dir) {
        rtgui_free(filelist->cur_dir);
        filelist->cur_dir = RT_NULL;
    }

    _contex.count--;
    if (!_contex.count) {
        rtgui_image_destroy(_contex.file_img);
        _contex.file_img = RT_NULL;
        rtgui_image_destroy(_contex.folder_img);
        _contex.folder_img = RT_NULL;
    }
}

static void _filelist_clear_items(rtgui_filelist_t *filelist) {
    rt_uint32_t idx;

    for (idx = 0; idx < SUPER_(filelist).count; idx++)
        rtgui_free(SUPER_(filelist).items[idx].name);
    rtgui_free(SUPER_(filelist).items);
    SUPER_(filelist).items = RT_NULL;
}

static rt_bool_t _filelist_on_item(void *obj, rtgui_evt_generic_t *evt) {
    rtgui_filelist_t *filelist = obj;
    char *path = RT_NULL;
    rt_bool_t done = RT_FALSE;
    (void)evt;

    do {
        rtgui_list_item_t *item = \
            &SUPER_(filelist).items[SUPER_(filelist).current];

        if (_contex.folder_img != item->image) {
            /* is a file */
            if (filelist->on_file)
                (void)filelist->on_file(obj, RT_NULL);
            break;
        }

        /* is current dir */
        if (!rt_strcmp(item->name, ".")) break;

        path = rtgui_malloc(MAX_PATH_LENGTH);
        if (!path) break;

        if (!rt_strcmp(item->name, "..")) {
            /* is parent dir */
            char *ptr;
            rt_uint8_t len;

            ptr = filelist->cur_dir + rt_strlen(filelist->cur_dir) - 1;
            while (ptr >= filelist->cur_dir) {
                if (PATH_SEPARATOR == *ptr) break;
                ptr--;
            }
            if (ptr < filelist->cur_dir) break;

            len = ptr - filelist->cur_dir + 1;
            rt_strncpy(path, filelist->cur_dir, len);
            path[len] = '\x00';
        } else {
            /* is subdir */
            if (filelist->cur_dir[rt_strlen(filelist->cur_dir) - 1] != \
                PATH_SEPARATOR) {
                rt_snprintf(path, MAX_PATH_LENGTH, "%s%c%s",
                    filelist->cur_dir, PATH_SEPARATOR, item->name);
            } else {
                rt_snprintf(path, MAX_PATH_LENGTH, "%s%s",
                    filelist->cur_dir, item->name);
            }
        }
        LOG_D("go dir %s", path);
        rtgui_filelist_set_dir(filelist, path);
    } while (0);

    if (path) rtgui_free(path);
    return done;
}

/* Public functions ----------------------------------------------------------*/
rtgui_filelist_t *rtgui_create_filelist(rtgui_container_t *cntr,
    rtgui_evt_hdl_t hdl, rtgui_rect_t *rect, const char *dir) {
    rtgui_filelist_t *filelist;

    do {
        filelist = CREATE_INSTANCE(filelist, SUPER_CLASS_HANDLER(filelist));
        if (!filelist) break;
        if (rect)
            rtgui_widget_set_rect(TO_WIDGET(filelist), rect);
        rtgui_filelist_set_dir(filelist, dir);
        FILELIST_SETTER(on_file)(filelist, hdl);
        if (cntr)
        rtgui_container_add_child(cntr, TO_WIDGET(filelist));
    } while (0);

    return filelist;
}

void rtgui_filelist_set_dir(rtgui_filelist_t *filelist, const char *dir_) {
    RT_ASSERT(filelist != RT_NULL);

    _filelist_clear_items(filelist);

    do {
        rt_bool_t is_root;
        char *fullpath;
        DIR *dir;
        struct dirent *dirent;
        rtgui_list_item_t *items;
        rt_uint16_t cnt, idx;
        struct stat info;

        is_root = !(dir_[0] != '/' || dir_[1] != '\x00');
        if (filelist->cur_dir)
            rtgui_free(filelist->cur_dir);
        filelist->cur_dir = rt_strdup(dir_);

        if (!dir_) break;
        dir = opendir(dir_);
        if (!dir) break;
        /* get items count */
        cnt = 0;
        while (RT_NULL != (dirent = readdir(dir))) {
            /* "readdir()" may not return ".." */
            if (!rt_strcmp(dirent->d_name, "..")) continue;
            if (!rt_strcmp(dirent->d_name, ".")) continue;
            cnt++;
        }
        closedir(dir);

        /* for ".." */
        if (!is_root) cnt++;

        /* allocate memory */
        items = rtgui_malloc(sizeof(rtgui_list_item_t) * cnt);
        if (!items) break;

        idx = 0;
        if (!is_root) {
            items[idx].name = rt_strdup("..");
            items[idx].image = _contex.folder_img;
            idx++;
        }

        /* get info */
        fullpath = rtgui_malloc(MAX_PATH_LENGTH);
        dir = opendir(dir_);
        if (!dir) break;

        while ((RT_NULL != (dirent = readdir(dir))) && \
               (idx < cnt)) {
            if (!rt_strcmp(dirent->d_name, "..")) continue;
            if (!rt_strcmp(dirent->d_name, ".")) continue;

            /* build info */
            if (fullpath) {
                rt_memset(&info, 0x00, sizeof(struct stat));
                if (dir_[rt_strlen(dir_) - 1] != PATH_SEPARATOR) {
                    rt_snprintf(fullpath, MAX_PATH_LENGTH, "%s%c%s",
                        dir_, PATH_SEPARATOR, dirent->d_name);
                } else {
                    rt_snprintf(fullpath, MAX_PATH_LENGTH, "%s%s",
                        dir_, dirent->d_name);
                }
                stat(fullpath, &info);
            }

            if (info.st_mode & S_IFDIR) {
                items[idx].image = _contex.folder_img;
                items[idx].name = rt_strdup(dirent->d_name);
            } else {
                items[idx].image = _contex.file_img;
                if (!fullpath) {
                    items[idx].name = rt_strdup(dirent->d_name);
                } else {
                    rt_uint32_t sz = info.st_size;

                    if (sz < (1 << 10)) {
                        rt_snprintf(fullpath, MAX_PATH_LENGTH, "%-16s  %4dB",
                            dirent->d_name, sz);
                    } else if (sz < (1 << 20)) {
                        sz >>= 10;
                        rt_snprintf(fullpath, MAX_PATH_LENGTH, "%-16s %4dKB",
                            dirent->d_name, sz);
                    } else {
                        sz >>= 20;
                        rt_snprintf(fullpath, MAX_PATH_LENGTH, "%-16s %4dMB",
                            dirent->d_name, sz);
                    }
                    items[idx].name = rt_strdup(fullpath);
                }
            }
            idx++;
        }

        closedir(dir);
        rtgui_free(fullpath);
        /* notes
           The following line triggers "PAINT" event.
           It is too early, when called by CREATE_FILELIST_INSTANCE.
         */
        LIST_SETTER(items)(TO_LIST(filelist), items, cnt);
    } while (0);
}
RTM_EXPORT(rtgui_filelist_set_dir);

RTGUI_MEMBER_GETTER(rtgui_filelist_t, filelist, char*, cur_dir);

RTGUI_MEMBER_SETTER(rtgui_filelist_t, filelist, rtgui_evt_hdl_t, on_file);
