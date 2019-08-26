/***************************************************************************//**
 * @file    FileBrowserDesign.ino
 * @brief   RTT-GUI library "FileBrowserDesign" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "APP_FBD"
#include <log.h>

#define PATH_SEPARATOR  '/'
#define PATH_LEN_MAX    (128)


static rt_bool_t fileBrowser_on_close(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t fileBrowser_on_file(void *obj, rtgui_evt_generic_t *evt);

static char design[] = "\
APP: {\n\
  PARAM: {\n\
    // name, handler\n\
    \"FileBrowser\", NULL,\n\
  },\n\
\n\
  MWIN: {\n\
    PARAM: {\n\
      // title, handler\n\
      \"FileBrowser\", NULL,\n\
    },\n\
\n\
    FILELIST: {\n\
      PARAM: {\n\
        // size, handler, dir\n\
        DEFAULT, on_file, \"/\",\n\
      },\n\
    },\n\
  },\n\
\n\
  WIN: {\n\
    PARAM: {\n\
      // title, size, handler, on_close handler\n\
      \"FileWin\", DEFAULT, NULL, on_close,\n\
    },\n\
    ID: 0,\n\
\n\
    BOX: {\n\
      PARAM: {\n\
        // orient, border size\n\
        // orient: 1, HORIZONTAL; 2, VERTICAL\n\
        1, 0,\n\
      },\n\
      ID: 1,\n\
    },\n\
\n\
    PICTURE: {\n\
      PARAM: {\n\
        // size, handler, path, align, auto-resize\n\
        // align: 0x00, LEFT / TOP; 0x01, CENTER_HORIZONTAL; 0x02, RIGHT; 0x08, CENTER_VERTICAL; 0x04, BOTTOM\n\
        // resize: 0, False; 1, True\n\
        NULL, NULL, NULL, 0x01, 1,\n\
      },\n\
      // RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND\n\
      ALIGN: 0x30,\n\
      ID: 2,\n\
    },\n\
  },\n\
}\n\
";
static uint32_t offset = 0;

static rtgui_win_t *fileWin;
static rtgui_picture_t *pic;

static design_contex_t ctx;
static const hdl_tbl_entr_t hdl_tbl[] = {
  { "on_file",  (void *)fileBrowser_on_file },
  { "on_close", (void *)fileBrowser_on_close },
  { RT_NULL,    RT_NULL },
};
static obj_tbl_entr_t obj_tbl[3];


static rt_err_t design_read(char **buf, rt_uint32_t *sz) {
  if (offset >= sizeof(design)) {
    *sz = 0;
    return -RT_EEMPTY;
  }

  *buf = design;
  *sz = sizeof(design);
  offset += *sz;
  return RT_EOK;
}

static rt_bool_t fileBrowser_on_close(void *obj, rtgui_evt_generic_t *evt) {
  (void)obj;
  (void)evt;
  (void)rtgui_win_show(ctx.win, RT_FALSE);

  return RT_TRUE;
}

static rt_bool_t fileBrowser_on_file(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;
  (void)evt;

  do {
    rtgui_filelist_t *filelist = (rtgui_filelist_t *)obj;
    rtgui_list_item_t *item = &SUPER_(filelist).items[SUPER_(filelist).current];
    char *ptr;
    rt_uint8_t len, len2;
    char path[PATH_LEN_MAX];

    if ((ptr = rt_strstr(item->name, ".bmp")) || \
        (ptr = rt_strstr(item->name, ".BMP"))) {
    } else if ((ptr = rt_strstr(item->name, ".jpg")) || \
               (ptr = rt_strstr(item->name, ".JPG"))) {
    } else {
      break;
    }

    len = rt_strlen(filelist->cur_dir);
    len2 = ptr - item->name + 4;
    rt_strncpy(path, filelist->cur_dir, len);
    if (path[len - 1] != PATH_SEPARATOR)
      path[len++] = PATH_SEPARATOR;
    rt_strncpy(path + len, item->name, len2);
    path[len + len2] = '\x00';

    PICTURE_SETTER(path)(pic, path);
    rtgui_win_show(fileWin, RT_TRUE);
    done = RT_TRUE;
  } while (0);

  return done;
}

static void fileBrowser_entry(void *param) {
  rtgui_box_t *sizer;
  (void)param;

  if (RT_EOK != rtgui_design_init(&ctx, design_read, hdl_tbl, obj_tbl, 3)) {
    LOG_E("Design init failed!");
    return;
  }
  if (RT_EOK != rtgui_design_parse(&ctx)) {
    LOG_E("Design parsing failed!");
    return;
  }
  fileWin = (rtgui_win_t *)obj_tbl[0];
  sizer = (rtgui_box_t *)obj_tbl[1];
  pic = (rtgui_picture_t *)obj_tbl[2];
  rtgui_box_layout(sizer);


  rtgui_win_show(ctx.win, RT_FALSE);
  rtgui_app_run(ctx.app);

  DELETE_WIN_INSTANCE(fileWin);
  DELETE_WIN_INSTANCE(ctx.win);
  rtgui_app_uninit(ctx.app);
}

// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid = rt_thread_create(
    "FileBrowser", fileBrowser_entry, RT_NULL, 2048, 25, 10);
  if (tid) {
    rt_thread_startup(tid);
  } else {
    LOG_E("Create thread failed!");
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
