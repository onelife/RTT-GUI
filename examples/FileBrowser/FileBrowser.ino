/***************************************************************************//**
 * @file    FileBrowser.ino
 * @brief   Arduino RT-Thread library "FileBrowser" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "APP_FB "
#include <log.h>

#define PATH_SEPARATOR                  '/'
#define MAX_PATH_LENGTH                 (128)

const char bmp[] = "bmp";
const char jpeg[] = "jpeg";
const char* format = RT_NULL;

rtgui_app_t *fileBrs;
rtgui_win_t *mainWin;
rtgui_win_t *fileWin;
char path[MAX_PATH_LENGTH];


static rt_bool_t fileBrowser_on_close(void *obj, rtgui_evt_generic_t *evt) {
  (void)obj;
  (void)evt;
  (void)rtgui_win_show(mainWin, RT_FALSE);

  return RT_TRUE;
}

static rt_bool_t fileWin_handler(void *obj, rtgui_evt_generic_t *evt) {
  rtgui_dc_t *dc;
  rtgui_rect_t rect;
  rtgui_image_t *img;
  rt_bool_t done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, PAINT)) break;
    done = RT_FALSE;
    rt_kprintf("*** Load %s: %s\n", format, path);

    dc = rtgui_dc_begin_drawing(TO_WIDGET(obj));
    if (!dc) {
      rt_kprintf("No DC!\n");
      break;
    }

    rtgui_dc_get_rect(dc, &rect);

    img = rtgui_image_create_from_file(format, path, RT_FALSE);
    if (img) {
        rtgui_image_blit(img, dc, &rect);
        rtgui_image_destroy(img);
    } else {
      rt_kprintf("Load %s filed!\n", path);
    }

    rtgui_dc_end_drawing(dc, RT_TRUE);
    done = RT_TRUE;
  } while (0);

  return done;
}

static rt_bool_t fileBrowser_on_file(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;
  (void)evt;

  do {
    rtgui_filelist_t *filelist = (rtgui_filelist_t *)obj;
    rtgui_list_item_t *item = &SUPER_(filelist).items[SUPER_(filelist).current];
    char *ptr;
    rt_uint8_t len, len2;

    format = RT_NULL;
    if ((ptr = rt_strstr(item->name, ".bmp")) || \
      (ptr = rt_strstr(item->name, ".BMP")))
      format = bmp;
    else if ((ptr = rt_strstr(item->name, ".jpg")) || \
      (ptr = rt_strstr(item->name, ".JPG")))
      format = jpeg;

    if (!format) break;

    len = rt_strlen(filelist->cur_dir);
    len2 = ptr - item->name + 4;
    rt_strncpy(path, filelist->cur_dir, len);
    if (path[len - 1] != PATH_SEPARATOR)
      path[len++] = PATH_SEPARATOR;
    rt_strncpy(path + len, item->name, len2);
    path[len + len2] = '\x00';

    rtgui_win_show(fileWin, RT_TRUE);
    done = RT_TRUE;
  } while (0);

  return done;
}

static void fileBrowser_entry(void *param) {
  rtgui_rect_t rect;
  rtgui_filelist_t *filelist;
  (void)param;

  /* create app */
  CREATE_APP_INSTANCE(fileBrs, RT_NULL, "FileBrowser");
  if (!fileBrs) {
    rt_kprintf("Create app failed!\n");
    return;
  }

  /* create win */
  mainWin = CREATE_MAIN_WIN(RT_NULL, RT_NULL, "FileBrowser",
    RTGUI_WIN_STYLE_MAINWIN);
  if (!mainWin) {
    rtgui_app_uninit(fileBrs);
    rt_kprintf("Create mainWin failed!\n");
    return;
  }

  rtgui_get_screen_rect(&rect);

  /* filelist */
  filelist = CREATE_FILELIST_INSTANCE(mainWin, fileBrowser_on_file, &rect, "/");
  if (!filelist) {
      rt_kprintf("Create filelist failed!\n");
      return;
  }

  /* fileWin */
  fileWin = CREATE_WIN_INSTANCE(RT_NULL, fileWin_handler, &rect, "PicWin",
    RTGUI_WIN_STYLE_DEFAULT);
  if (!fileWin) {
      rt_kprintf("Create fileWin failed!\n");
      return;
  }
  WIN_SETTER(on_close)(fileWin, fileBrowser_on_close);  

  rtgui_win_show(mainWin, RT_FALSE);
  rtgui_app_run(fileBrs);

  DELETE_WIN_INSTANCE(mainWin);
  rtgui_app_uninit(fileBrs);
}

// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid = rt_thread_create(
    "FileBrowser", fileBrowser_entry, RT_NULL, 2048, 25, 10);
  if (tid) {
    rt_thread_startup(tid);
  } else {
    rt_kprintf("Create thread failed!\n");
  }
}

void setup() {
  RT_T.begin();
  // no code here as RT_T.begin() never return
}

// this function will be called by "Arduino" thread
void loop() {
  // may put some code here that will be run repeatedly
}
