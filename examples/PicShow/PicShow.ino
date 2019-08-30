/***************************************************************************//**
 * @file    PicShow.ino
 * @brief   RTT-GUI library "PicShow" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "APP_PIC"
#include <log.h>

// #define USING_SMALL_DISPLAY

#define PATH_LEN_MAX  (20)
#define PIC_DIR       "/pic"
#define TIMER_TICKS   (1 * RT_TICK_PER_SECOND)
#define TIMER_COUNT   (5)


static rtgui_timer_t *picTmr;
static rt_uint8_t tmrCnt = 0;
static rt_bool_t timeout = RT_TRUE;
static rt_bool_t pause = RT_FALSE;

static rtgui_app_t *picShow;
#ifndef USING_SMALL_DISPLAY
static rtgui_label_t* label;
static rtgui_progress_t *bar;
#endif
static rtgui_picture_t *pic;

static DIR* dir = RT_NULL;
static struct dirent* dirent = RT_NULL;


static rt_bool_t get_pic(char *path) {
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

  if (rt_strstr(dirent->d_name, ".bmp") || \
      rt_strstr(dirent->d_name, ".BMP")) {
    rt_sprintf(path, PIC_DIR "/%s", dirent->d_name);
    LOG_I("BMP: %s", path);
    return RT_TRUE;
  } else if (rt_strstr(dirent->d_name, ".jpg") || \
             rt_strstr(dirent->d_name, ".JPG")) {
    rt_sprintf(path, PIC_DIR "/%s", dirent->d_name);
    LOG_I("JPEG: %s", path);
    return RT_TRUE;
  } else {
    LOG_I("Skip: %s (%s)", path, dirent->d_name);
    return RT_FALSE;
  }
}

static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt) {
  char *path = RT_FALSE;
  rt_bool_t done = RT_FALSE;

  do {
    /* before load image */
    if (IS_EVENT_TYPE(evt, PAINT)) {
      path = PICTURE_GETTER(path)(TO_PICTURE(obj));

      #ifndef USING_SMALL_DISPLAY
      if (path) {
        /* show label */
        WIDGET_FLAG_SET(label, SHOWN);
        rtgui_label_set_text(label, path);
        /* hide bar */
        WIDGET_FLAG_CLEAR(bar, SHOWN);
      }
      #endif
    }

    /* call default handler */
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    /* handle key */
    if (IS_EVENT_TYPE(evt, KBD) && IS_KBD_EVENT_TYPE(evt, UP)) {
      if (IS_KBD_EVENT_KEY(evt, a)) {
        char path[PATH_LEN_MAX];
        rt_uint8_t retry = 0;

        while (!get_pic(path) && (retry < 10)) {
          rt_thread_sleep(20);
          continue;
        }

        if (retry < 10) {
          rtgui_timer_stop(picTmr);
          timeout = RT_FALSE;
          tmrCnt = 0;
          PICTURE_SETTER(path)(pic, path);
        }
        done = RT_TRUE;
      } else if (IS_KBD_EVENT_KEY(evt, b)) {
        if (pause) {
          LOG_I("loop");
          rtgui_timer_start(picTmr);
        } else {
          LOG_I("pause");
          rtgui_timer_stop(picTmr);
          timeout = RT_FALSE;
          tmrCnt = 0;
        }
        pause = !pause;
        done = RT_TRUE;
      }
      break;
    }

    if (!IS_EVENT_TYPE(evt, PAINT) || !path)
      break;

    /* after load image */
    done = RT_FALSE;

    rtgui_timer_stop(picTmr);
    timeout = RT_FALSE;
    tmrCnt = 0;

    if (!pause) {
      #ifndef USING_SMALL_DISPLAY
      /* show bar */
      WIDGET_FLAG_SET(bar, SHOWN);
      rtgui_progress_set_value(bar, 0);
      /* hide label */
      WIDGET_FLAG_CLEAR(label, SHOWN);
      #endif
      rtgui_timer_start(picTmr);
    }
  } while (0);

  return done;
}

#ifndef USING_SMALL_DISPLAY
static rt_bool_t picShow_btn1_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;

  do {
    char path[PATH_LEN_MAX];
    rt_uint8_t retry;

    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, MOUSE_BUTTON) || !IS_MOUSE_EVENT_BUTTON(evt, UP))
      break;

    retry = 0;
    while (!get_pic(path) && (retry < 10)) {
      rt_thread_sleep(20);
      continue;
    }

    if (retry < 10) {
      rtgui_timer_stop(picTmr);
      timeout = RT_FALSE;
      tmrCnt = 0;
      PICTURE_SETTER(path)(pic, path);
    }
    done = RT_TRUE;
  } while (0);

  return done;
}

static rt_bool_t picShow_btn2_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, MOUSE_BUTTON) || !IS_MOUSE_EVENT_BUTTON(evt, UP))
      break;

    if (IS_BUTTON_FLAG(obj, PRESS)) {
      LOG_I("%s pressed", MEMBER_GETTER(button, text)(obj));
      MEMBER_SETTER(button, text)(obj, "Loop");
      rtgui_timer_stop(picTmr);
      timeout = RT_FALSE;
      tmrCnt = 0;
      pause = RT_TRUE;
    } else {
      LOG_I("%s unpressed", MEMBER_GETTER(button, text)(obj));
      MEMBER_SETTER(button, text)(obj, "Pause");
      pause = RT_FALSE;
      /* show bar */
      WIDGET_FLAG_SET(bar, SHOWN);
      rtgui_progress_set_value(bar, 0);
      /* hide image */
      WIDGET_FLAG_CLEAR(label, SHOWN);
      rtgui_timer_start(picTmr);
    }

    done = RT_TRUE;
  } while (0);

  return done;
}
#endif /* USING_SMALL_DISPLAY */

static void picShow_timeout(rtgui_timer_t *tmr, void *param) {
  (void)param;
  tmrCnt++;
  #ifndef USING_SMALL_DISPLAY
  rtgui_progress_set_value(bar, (rt_uint16_t)(
    (rt_uint32_t)tmrCnt * 100 / TIMER_COUNT));
  #endif
  if (tmrCnt >= TIMER_COUNT) {
    rtgui_timer_stop(tmr);
    tmrCnt = 0;
    timeout = RT_TRUE;
  }
}

static void picShow_entry(void *param) {
  rtgui_win_t *win;
  #ifndef USING_SMALL_DISPLAY
  rtgui_container *cntr;
  rtgui_box_t *sizer;
  rtgui_button_t *btn1, *btn2;
  #endif
  rtgui_rect_t rect;
  (void)param;

  /* create app */
  CREATE_APP_INSTANCE(picShow, RT_NULL, "PicShow");
  if (!picShow) {
    LOG_E("Create app failed!");
    return;
  }

  picTmr = rtgui_timer_create(TIMER_TICKS, RT_TIMER_FLAG_PERIODIC,
    picShow_timeout, RT_NULL);
  if (!picTmr) {
    LOG_E("Create timer failed!");
    return;
  }

  /* create win */
  win = CREATE_MAIN_WIN(RT_NULL, "PicWin", RTGUI_WIN_STYLE_MAINWIN);
  if (!win) {
    rtgui_app_uninit(picShow);
    LOG_E("Create mainWin failed!");
    return;
  }
  #ifndef USING_SMALL_DISPLAY
  sizer = CREATE_BOX_INSTANCE(win, RTGUI_VERTICAL, 0);

  /* label */
  label = CREATE_LABEL_INSTANCE(win, RT_NULL, RT_NULL, "PicShow Example");
  if (!label) {
      LOG_E("Create label failed!");
      return;
  }
  WIDGET_BACKGROUND(TO_WIDGET(label)) = white;
  WIDGET_FOREGROUND(TO_WIDGET(label)) = blue;
  WIDGET_SETTER(min_height)(TO_WIDGET(label), 15);
  WIDGET_ALIGN(label) = RTGUI_ALIGN_EXPAND;

  /* progress bar */
  bar = CREATE_PROGRESS_INSTANCE(win, RT_NULL, RT_NULL, RTGUI_HORIZONTAL, 100);
  if (!bar) {
      LOG_E("Create progress failed!");
      return;
  }
  WIDGET_SETTER(min_height)(TO_WIDGET(bar), 15);
  WIDGET_ALIGN(bar) = RTGUI_ALIGN_EXPAND;
  WIDGET_FLAG_CLEAR(bar, SHOWN);
  #endif /* USING_SMALL_DISPLAY */

  /* picture */
  #ifdef USING_SMALL_DISPLAY
  rtgui_get_screen_rect(&rect);
  pic = CREATE_PICTURE_INSTANCE(win, picShow_handler, &rect, RT_NULL,
    CENTER_HORIZONTAL, RT_TRUE);
  if (!pic) {
      LOG_E("Create picture failed!");
      return;
  }
  #else /* USING_SMALL_DISPLAY */
  pic = CREATE_PICTURE_INSTANCE(win, picShow_handler, RT_NULL, RT_NULL,
    CENTER_HORIZONTAL, RT_TRUE);
  if (!pic) {
      LOG_E("Create picture failed!");
      return;
  }
  WIDGET_ALIGN(pic) = RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND;

  /* buttons in container */
  cntr = CREATE_CONTAINER_INSTANCE(win, RT_NULL, RT_NULL);
  WIDGET_SETTER(min_height)(TO_WIDGET(cntr), 30);
  WIDGET_ALIGN(cntr) = RTGUI_ALIGN_EXPAND;
  (void)CREATE_BOX_INSTANCE(cntr, RTGUI_HORIZONTAL, 0);
  btn1 = CREATE_BUTTON_INSTANCE(cntr, picShow_btn1_handler, NORMAL, "Next");
  WIDGET_ALIGN(btn1) = RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND;
  btn2 = CREATE_BUTTON_INSTANCE(cntr, picShow_btn2_handler, PUSH, "Pause");
  WIDGET_ALIGN(btn2) = RTGUI_ALIGN_STRETCH | RTGUI_ALIGN_EXPAND;
  rtgui_box_layout(sizer);
  /* "bar" has no size as it is hiding. set the same as "label". */
  rtgui_widget_get_rect(TO_WIDGET(label), &rect);
  rtgui_widget_set_rect(TO_WIDGET(bar), &rect);
  #endif /* USING_SMALL_DISPLAY */

  rtgui_win_show(win, RT_FALSE);
  rtgui_app_run(picShow);

  DELETE_WIN_INSTANCE(win);
  rtgui_app_uninit(picShow);
}

// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid = rt_thread_create(
    "picShow", picShow_entry, RT_NULL,
    CONFIG_APP_STACK_SIZE, CONFIG_APP_PRIORITY, CONFIG_APP_TIMESLICE);
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
  char path[PATH_LEN_MAX];

  while (!picShow || IS_APP_FLAG(picShow, EXITED)) {
    LOG_I("Waiting app");
    rt_thread_sleep(100);
  }
  /* let the window show first */
  rt_thread_sleep(20);

  while (RT_TRUE) {
    if (!timeout) {
      rt_thread_sleep(100);
      continue;
    }

    while (!get_pic(path)) {
      rt_thread_sleep(20);
      continue;
    }
    LOG_I("In loop: %s", path);

    PICTURE_SETTER(path)(pic, path);
    timeout = RT_FALSE;
  }
}
