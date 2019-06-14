/***************************************************************************//**
 * @file    PicShow.ino
 * @brief   Arduino RT-Thread library "PicShow" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "APP_PIC"
#include <log.h>

#define CMD_PIC_SHOW  0xFF01
#define PIC_DIR       "/pic"

struct picShow_info {
  char* path;
  const char* format;
};

const char bmp[] = "bmp";
const char jpeg[] = "jpeg";
rtgui_app_t *picShow;
DIR* dir = RT_NULL;
struct dirent* dirent = RT_NULL;


static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt) {
  struct picShow_info *info = (struct picShow_info *)evt->command.type;
  rtgui_dc_t *dc;
  rtgui_rect_t rect;
  rtgui_image_t *img;
  rt_bool_t done;

  rt_kprintf("picShow handle %x\n", evt->base.type);
  done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj)) {
      done = DEFAULT_HANDLER(obj)(obj, evt);
    }

    if ((evt->base.type != RTGUI_EVENT_COMMAND) || \
        (evt->command.command_id != CMD_PIC_SHOW)) {
      break;
    }

    dc = rtgui_dc_begin_drawing(TO_WIDGET(obj));
    if (!dc) {
      rt_kprintf("No DC!\n");
      done = RT_FALSE;
      break;
    }
    rtgui_dc_get_rect(dc, &rect);
    rt_kprintf("Loading %s\n", info->path);

    img = rtgui_image_create_from_file(info->format, info->path, RT_FALSE);
    if (img) {
        rtgui_image_blit(img, dc, &rect);
        rtgui_image_destroy(img);
    } else {
      rt_kprintf("Load %s filed!\n", info->path);
    }
    done = RT_TRUE;

    rtgui_dc_end_drawing(dc, RT_TRUE);
  } while (0);

  return done;
}

static void picShow_entry(void *param) {
  rtgui_win_t *win;
  (void)param;

  /* create app */
  picShow = rtgui_app_create("PicShow", RT_NULL);
  if (!picShow) {
    rt_kprintf("Create app failed!\n");
    return;
  }

  /* create win */
  win = rtgui_mainwin_create(RT_NULL, picShow_handler, "PicWin",
    RTGUI_WIN_STYLE_NO_BORDER | RTGUI_WIN_STYLE_NO_TITLE);
  if (!win) {
    rtgui_app_destroy(picShow);
    rt_kprintf("Create main win failed!\n");
    return;
  }

  /* create lable in window */
  // label = rtgui_label_create("Photo Frame Demo");
  // if (label == RT_NULL)
  // {
  //     rt_kprintf("Create lable failed!\n");
  //     return;
  // }

  // RTGUI_WIDGET_TEXTALIGN(RTGUI_WIDGET(label)) = RTGUI_ALIGN_LEFT;
  // RTGUI_WIDGET_BACKGROUND(RTGUI_WIDGET(label)) = white;
  // RTGUI_WIDGET_FOREGROUND(RTGUI_WIDGET(label)) = blue;

  //   rect2.x1 = rect1.x1;
  //   rect2.y1 = rect1.y1;
  //   rect2.x2 = rect1.x2;
  //   rect2.y2 = 15;
  // rtgui_widget_set_rect(RTGUI_WIDGET(label), &rect2);
  //   rtgui_object_set_event_handler(RTGUI_OBJECT(label), photo_lable_event_handler);
  // rtgui_container_add_child(RTGUI_CONTAINER(window), RTGUI_WIDGET(label));

  rtgui_win_show(win, RT_FALSE);
  rtgui_app_run(picShow);

  rtgui_win_destroy(win);
  rtgui_app_destroy(picShow);
}


// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid = rt_thread_create(
    "picShow", picShow_entry, RT_NULL, 2048, 25, 10);
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
  struct picShow_info info;
  char path[20];
  rtgui_evt_generic_t *evt;

  while (RT_TRUE) {
    if (!dir) {
      dir = opendir(PIC_DIR);
      dirent = readdir(dir);
    } else if (!dirent) {
      closedir(dir);
      dir = opendir(PIC_DIR);
      dirent = readdir(dir);
    } else {
      dirent = readdir(dir);
    }

    info.path = path;
    rt_sprintf(path, PIC_DIR "/%s", dirent->d_name);
    if (rt_strstr(path, ".bmp") || rt_strstr(path, ".BMP")) {
      info.format = bmp;
      rt_kprintf("BMP: %s\n", path);
      break;
    } else if (rt_strstr(path, ".jpg") || rt_strstr(path, ".JPG")) {
      info.format = jpeg;
      rt_kprintf("JPEG: %s\n", path);
      break;
    } else {
      rt_kprintf("Skip: %s\n", path);
    }
  }

  while (!picShow || (picShow->state_flag & RTGUI_APP_FLAG_EXITED)) {
    rt_kprintf("Waiting app\n");
    rt_thread_sleep(100);
  }

  RTGUI_CREATE_EVENT(evt, COMMAND, RT_WAITING_FOREVER);
  evt->command.wid = RT_NULL;
  evt->command.type = (rt_int32_t)&info;
  evt->command.command_id = CMD_PIC_SHOW;

  rtgui_request(picShow, evt, RT_WAITING_FOREVER);
  rt_thread_sleep(1000);
}
