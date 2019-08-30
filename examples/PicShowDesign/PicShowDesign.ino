/***************************************************************************//**
 * @file    PicShow.ino
 * @brief   RTT-GUI library "PicShow" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "APP_PID"
#include <log.h>

#define PATH_LEN_MAX  (20)
#define PIC_DIR       "/pic"
#define DESIGN_FILE   "/design/PicShow.gui"
#define TIMER_COUNT   (5)


static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_btn1_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_btn2_handler(void *obj, rtgui_evt_generic_t *evt);
static void picShow_timeout(rtgui_timer_t *tmr, void *param);

static rtgui_timer_t *picTmr;
static rt_uint8_t tmrCnt = 0;
static rt_bool_t timeout = RT_TRUE;
static rt_bool_t pause = RT_FALSE;

static rtgui_label_t* label;
static rtgui_progress_t *bar;
static rtgui_picture_t *pic;

static DIR* dir = RT_NULL;
static struct dirent* dirent = RT_NULL;

static char buf[512];
static int designFile = -1;

static design_contex_t ctx;
static const hdl_tbl_entr_t hdl_tbl[] = {
  { "pic",    (void *)picShow_handler },
  { "btn1",   (void *)picShow_btn1_handler },
  { "btn2",   (void *)picShow_btn2_handler },
  { "timer",  (void *)picShow_timeout },
  { RT_NULL,  RT_NULL },
};
static obj_tbl_entr_t obj_tbl[5];


static rt_err_t design_read(char **_buf, rt_uint32_t *sz) {
  *sz = (rt_uint32_t)read(designFile, buf, sizeof(buf));

  *_buf = buf;
  *sz = sizeof(buf);
  return RT_EOK;
}

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

      if (path) {
        /* show label */
        WIDGET_FLAG_SET(label, SHOWN);
        rtgui_label_set_text(label, path);
        /* hide bar */
        WIDGET_FLAG_CLEAR(bar, SHOWN);
      }
    }

    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, PAINT) || !path)
      break;

    /* after load image */
    done = RT_FALSE;

    rtgui_timer_stop(picTmr);
    timeout = RT_FALSE;
    tmrCnt = 0;

    if (!pause) {
      /* show bar */
      WIDGET_FLAG_SET(bar, SHOWN);
      rtgui_progress_set_value(bar, 0);
      /* hide image */
      WIDGET_FLAG_CLEAR(label, SHOWN);
      rtgui_timer_start(picTmr);
    }
  } while (0);

  return done;
}

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

static void picShow_timeout(rtgui_timer_t *tmr, void *param) {
  (void)param;
  tmrCnt++;
  rtgui_progress_set_value(bar, (rt_uint16_t)(
    (rt_uint32_t)tmrCnt * 100 / TIMER_COUNT));
  if (tmrCnt >= TIMER_COUNT) {
    rtgui_timer_stop(tmr);
    tmrCnt = 0;
    timeout = RT_TRUE;
  }
}

static void picShow_entry(void *param) {
  rtgui_box_t *sizer;
  rtgui_rect_t rect;
  (void)param;

  designFile = open(DESIGN_FILE, O_RDONLY);
  if (designFile < 0) {
    LOG_E("Open %s failed!", DESIGN_FILE);
    return;
  }

  if (RT_EOK != rtgui_design_init(&ctx, design_read, hdl_tbl, obj_tbl, 5)) {
    close(designFile);
    LOG_E("Design init failed!");
    return;
  }
  if (RT_EOK != rtgui_design_parse(&ctx)) {
    close(designFile);
    LOG_E("Design parsing failed!");
    return;
  }
  close(designFile);
  picTmr = (rtgui_timer_t *)obj_tbl[0];
  sizer = (rtgui_box_t *)obj_tbl[1];
  label = (rtgui_label_t *)obj_tbl[2];
  bar = (rtgui_progress_t *)obj_tbl[3];
  pic = (rtgui_picture_t *)obj_tbl[4];
  WIDGET_FLAG_CLEAR(bar, SHOWN);
  rtgui_box_layout(sizer);
  /* "bar" has no size as it is hiding. set the same as "label". */
  rtgui_widget_get_rect(TO_WIDGET(label), &rect);
  rtgui_widget_set_rect(TO_WIDGET(bar), &rect);


  rtgui_win_show(ctx.win, RT_FALSE);
  rtgui_app_run(ctx.app);

  DELETE_WIN_INSTANCE(ctx.win);
  rtgui_app_uninit(ctx.app);
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

  while (!ctx.app || IS_APP_FLAG(ctx.app, EXITED)) {
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
    LOG_I("In loop TX: %s", path);

    PICTURE_SETTER(path)(pic, path);
    timeout = RT_FALSE;
  }
}
