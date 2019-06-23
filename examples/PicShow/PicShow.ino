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
#define TIMER_TICKS   (5 * RT_TICK_PER_SECOND)

struct picShow_info {
  char* path;
  const char* format;
};

static struct rt_timer picTmr;
static rt_bool_t timeout = RT_TRUE;
static rt_bool_t pause = RT_FALSE;

const char bmp[] = "bmp";
const char jpeg[] = "jpeg";
rtgui_app_t *picShow;
DIR* dir = RT_NULL;
struct dirent* dirent = RT_NULL;
char path[20];


static rt_bool_t get_pic(struct picShow_info *info) {
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

  info->path = path;
  rt_sprintf(path, PIC_DIR "/%s", dirent->d_name);
  if (rt_strstr(path, ".bmp") || rt_strstr(path, ".BMP")) {
    info->format = bmp;
    rt_kprintf("BMP: %s\n", path);
    return RT_TRUE;
  } else if (rt_strstr(path, ".jpg") || rt_strstr(path, ".JPG")) {
    info->format = jpeg;
    rt_kprintf("JPEG: %s\n", path);
    return RT_TRUE;
  } else {
    rt_kprintf("Skip: %s (%s)\n", path, dirent->d_name);
    return RT_FALSE;
  }
}

static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt) {
  struct picShow_info *info = (struct picShow_info *)evt->command.type;
  rtgui_dc_t *dc;
  rtgui_rect_t rect;
  rtgui_image_t *img;
  rt_bool_t done;

  done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, COMMAND) || \
        (evt->command.command_id != CMD_PIC_SHOW))
      break;

    dc = rtgui_dc_begin_drawing(TO_WIDGET(obj));
    if (!dc) {
      rt_kprintf("No DC!\n");
      done = RT_FALSE;
      break;
    }

    rtgui_dc_get_rect(dc, &rect);
    rect.y1 += 15;
    rect.y2 -= 30;

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

static rt_bool_t picShow_label_handler(void *obj, rtgui_evt_generic_t *evt) {
  struct picShow_info *info = (struct picShow_info *)evt->command.type;
  rt_bool_t done;

  done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, COMMAND) || \
        (evt->command.command_id != CMD_PIC_SHOW))
      break;

    rtgui_label_set_text(TO_LABEL(obj), info->path);
    done = RT_TRUE;
  } while (0);

  return done;
}

static rt_bool_t picShow_btn1_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done;

  done = RT_FALSE;

  do {
    static struct picShow_info info;

    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, MOUSE_BUTTON) || !IS_MOUSE_EVENT_BUTTON(evt, UP))
      break;

    if (!get_pic(&info)) {
      rt_kprintf("+++ got %s\n", info.path);
    } else {
      rtgui_evt_generic_t *picEvt;

      RTGUI_CREATE_EVENT(picEvt, COMMAND, RT_WAITING_FOREVER);
      picEvt->command.wid = RT_NULL;
      picEvt->command.type = (rt_int32_t)&info;
      picEvt->command.command_id = CMD_PIC_SHOW;
      rtgui_request(picShow, picEvt, RT_WAITING_FOREVER);
      timeout = RT_FALSE;
      if (!pause) rt_timer_start(&picTmr);
    }
    done = RT_TRUE;
  } while (0);

  return done;
}

static rt_bool_t picShow_btn2_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done;

  done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, MOUSE_BUTTON) || !IS_MOUSE_EVENT_BUTTON(evt, UP))
      break;

    if (IS_BUTTON_FLAG(obj, PRESS)) {
      rt_kprintf("+++ %s pressed\n", MEMBER_GETTER(button, text)(obj));
      MEMBER_SETTER(button, text)(obj, "开始");
      rt_timer_stop(&picTmr);
      timeout = RT_FALSE;
      pause = RT_TRUE;
    } else {
      rt_kprintf("+++ %s unpressed\n", MEMBER_GETTER(button, text)(obj));
      MEMBER_SETTER(button, text)(obj, "暂停");
      pause = RT_FALSE;
      rt_timer_start(&picTmr);
    }

    done = RT_TRUE;
  } while (0);

  return done;
}

static void picShow_entry(void *param) {
  rtgui_rect_t rect;
  rtgui_win_t *win;
  rtgui_label_t* label;
  rtgui_container *cntr;
  rtgui_box_t *box;
  rtgui_button_t *btn1, *btn2;
  (void)param;

  /* create app */
  CREATE_APP_INSTANCE(picShow, RT_NULL, "PicShow");
  if (!picShow) {
    rt_kprintf("Create app failed!\n");
    return;
  }

  /* create win */
  CREATE_MAIN_WIN(win, picShow_handler, RT_NULL,
    "PicWin", RTGUI_WIN_STYLE_NO_BORDER | RTGUI_WIN_STYLE_NO_TITLE);
  if (!win) {
    rtgui_app_uninit(picShow);
    rt_kprintf("Create main win failed!\n");
    return;
  }

  /* create label in window */
  CREATE_LABEL_INSTANCE(label, picShow_label_handler, "PicShow Example");
  if (!label) {
      rt_kprintf("Create label failed!\n");
      return;
  }
  WIDGET_BACKGROUND(TO_WIDGET(label)) = white;
  WIDGET_FOREGROUND(TO_WIDGET(label)) = blue;

  rtgui_get_screen_rect(&rect);
  rect.y2 = 15;
  rtgui_widget_set_rect(TO_WIDGET(label), &rect);
  rtgui_container_add_child(TO_CONTAINER(win), TO_WIDGET(label));

  cntr = TO_CONTAINER(CREATE_INSTANCE(container, RT_NULL));
  CREATE_BOX_INSTANCE(box, RTGUI_HORIZONTAL, 1);
  rtgui_container_set_box(cntr, box);

  CREATE_BUTTON_INSTANCE(btn1, picShow_btn1_handler, TYPE_NORMAL, "下一张");
  WIDGET_ALIGN(btn1) = RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND;
  rtgui_container_add_child(cntr, TO_WIDGET(btn1));
  CREATE_BUTTON_INSTANCE(btn2, picShow_btn2_handler, TYPE_PUSH, "暂停");
  WIDGET_ALIGN(btn2) = RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND;
  rtgui_container_add_child(cntr, TO_WIDGET(btn2));

  rtgui_get_screen_rect(&rect);
  rect.y1 = rect.y2 - 30;
  rtgui_widget_set_rect(TO_WIDGET(cntr), &rect);  
  rtgui_container_add_child(TO_CONTAINER(win), TO_WIDGET(cntr));
  rtgui_container_layout(cntr);

  rtgui_win_show(win, RT_FALSE);
  rtgui_app_run(picShow);

  DELETE_WIN_INSTANCE(win);
  rtgui_app_uninit(picShow);
}

static void picShow_timeout(void *param) {
    (void)param;
    timeout = RT_TRUE;
}

// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_timer_init(
    &picTmr, "picTmr",
    picShow_timeout, RT_NULL,
    TIMER_TICKS, RT_TIMER_FLAG_ONE_SHOT);

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
  rtgui_evt_generic_t *evt;

  while (!picShow || IS_APP_FLAG(picShow, EXITED)) {
    rt_kprintf("Waiting app\n");
    rt_thread_sleep(100);
  }

  while (RT_TRUE) {
    if (!timeout) {
      rt_thread_sleep(100);
      continue;
    }

    while (!get_pic(&info)) {
      rt_kprintf("+++ got %s\n", info.path);
      rt_thread_sleep(100);
      continue;
    }

    RTGUI_CREATE_EVENT(evt, COMMAND, RT_WAITING_FOREVER);
    evt->command.wid = RT_NULL;
    evt->command.type = (rt_int32_t)&info;
    evt->command.command_id = CMD_PIC_SHOW;
    rtgui_request(picShow, evt, RT_WAITING_FOREVER);
    timeout = RT_FALSE;
    rt_timer_start(&picTmr);
  }
}
