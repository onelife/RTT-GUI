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

#define CMD_PIC_SHOW  0xFF01
#define CMD_PIC_DELAY 0xFF02
#define PIC_DIR       "/pic"
#define DESIGN_FILE   "/design/PicShow.gui"
#define TIMER_TICKS   (1 * RT_TICK_PER_SECOND)
#define TIMER_COUNT   (5)

struct picShow_info {
  char* path;
  const char* format;
};

static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_label_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_progress_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_btn1_handler(void *obj, rtgui_evt_generic_t *evt);
static rt_bool_t picShow_btn2_handler(void *obj, rtgui_evt_generic_t *evt);
static void picShow_timeout(rtgui_timer_t *tmr, void *param);

static const char bmp[] = "bmp";
static const char jpeg[] = "jpeg";

static rtgui_timer_t *picTmr;
static rt_uint8_t tmrCnt = 0;
static rt_bool_t timeout = RT_TRUE;
static rt_bool_t pause = RT_FALSE;

static rtgui_progress_t *bar;
static rtgui_box_t *sizer;
static DIR* dir = RT_NULL;
static struct dirent* dirent = RT_NULL;
static char path[20];
static char buf[512];
static int designFile = -1;

static design_contex_t ctx;
static const hdl_tbl_entr_t hdl_tbl[] = {
  { "picwin",   (void *)picShow_handler },
  { "lab",      (void *)picShow_label_handler },
  { "progress", (void *)picShow_progress_handler },
  { "btn1",     (void *)picShow_btn1_handler },
  { "btn2",     (void *)picShow_btn2_handler },
  { "timer",    (void *)picShow_timeout },
  { RT_NULL,    RT_NULL },
};
static obj_tbl_entr_t obj_tbl[3];


static rt_err_t design_read(char **_buf, rt_uint32_t *sz) {
  *sz = (rt_uint32_t)read(designFile, buf, sizeof(buf));

  *_buf = buf;
  *sz = sizeof(buf);
  return RT_EOK;
}

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
    LOG_I("BMP: %s", path);
    return RT_TRUE;
  } else if (rt_strstr(path, ".jpg") || rt_strstr(path, ".JPG")) {
    info->format = jpeg;
    LOG_I("JPEG: %s", path);
    return RT_TRUE;
  } else {
    LOG_I("Skip: %s (%s)", path, dirent->d_name);
    return RT_FALSE;
  }
}

static rt_bool_t picShow_handler(void *obj, rtgui_evt_generic_t *evt) {
  struct picShow_info *info = (struct picShow_info *)evt->command.type;
  rtgui_dc_t *dc;
  rtgui_rect_t rect;
  rtgui_image_t *img;
  rtgui_evt_generic_t *dlyEvt;
  rt_bool_t done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, COMMAND) || \
        (evt->command.command_id != CMD_PIC_SHOW))
      break;
    done = RT_FALSE;


    if (picTmr) rtgui_timer_stop(picTmr);
    timeout = RT_FALSE;
    tmrCnt = 0;

    dc = rtgui_dc_begin_drawing(TO_WIDGET(obj));
    if (!dc) {
      LOG_E("No DC!");
      break;
    }

    rtgui_dc_get_rect(dc, &rect);
    rect.y1 += 15;
    rect.y2 -= 30;

    img = rtgui_image_create_from_file(info->format, info->path, RT_FALSE);
    if (img) {
        rtgui_image_blit(img, dc, &rect);
        rtgui_image_destroy(img);
    } else {
      LOG_E("Load %s filed!", info->path);
    }
    rtgui_dc_end_drawing(dc, RT_TRUE);

    RTGUI_CREATE_EVENT(dlyEvt, COMMAND, RT_WAITING_FOREVER);
    dlyEvt->command.wid = RT_NULL;
    dlyEvt->command.type = 0;
    dlyEvt->command.command_id = CMD_PIC_DELAY;
    rtgui_request(ctx.app, dlyEvt, RT_WAITING_FOREVER);

    if (!pause) {
      rtgui_timer_start(picTmr);
    }
  } while (0);

  return done;
}

static rt_bool_t picShow_label_handler(void *obj, rtgui_evt_generic_t *evt) {
  struct picShow_info *info = (struct picShow_info *)evt->command.type;
  rt_bool_t done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, COMMAND)) break;
    done = RT_FALSE;

    LOG_I("In label handler: %x", evt->command.command_id);
    if (evt->command.command_id == CMD_PIC_SHOW) {
      WIDGET_FLAG_SET(obj, SHOWN);
      rtgui_label_set_text(TO_LABEL(obj), info->path);
    } else if (evt->command.command_id == CMD_PIC_DELAY) {
      WIDGET_FLAG_CLEAR(obj, SHOWN);
    }
  } while (0);

  return done;
}

static rt_bool_t picShow_progress_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;

  do {
    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, COMMAND)) break;
    done = RT_FALSE;

    LOG_I("In progress handler: %x", evt->command.command_id);
    if (evt->command.command_id == CMD_PIC_SHOW) {
      WIDGET_FLAG_CLEAR(obj, SHOWN);
    } else if (evt->command.command_id == CMD_PIC_DELAY) {
      WIDGET_FLAG_SET(obj, SHOWN);
      rtgui_progress_set_value(TO_PROGRESS(obj), 0);
    }
  } while (0);

  return done;
}

static rt_bool_t picShow_btn1_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;

  do {
    static struct picShow_info info;

    if (DEFAULT_HANDLER(obj))
      done = DEFAULT_HANDLER(obj)(obj, evt);

    if (!IS_EVENT_TYPE(evt, MOUSE_BUTTON) || !IS_MOUSE_EVENT_BUTTON(evt, UP))
      break;

    if (!get_pic(&info)) {
      LOG_D("BTN1 got: %s", info.path);
    } else {
      rtgui_evt_generic_t *picEvt;

      RTGUI_CREATE_EVENT(picEvt, COMMAND, RT_WAITING_FOREVER);
      picEvt->command.wid = RT_NULL;
      picEvt->command.type = (rt_int32_t)&info;
      picEvt->command.command_id = CMD_PIC_SHOW;
      rtgui_request(ctx.app, picEvt, RT_WAITING_FOREVER);
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
  (void)param;

  designFile = open(DESIGN_FILE, O_RDONLY);
  if (designFile < 0) {
    LOG_E("Open %s failed!", DESIGN_FILE);
    return;
  }

  if (RT_EOK != rtgui_design_init(&ctx, design_read, hdl_tbl, obj_tbl, 3)) {
    close(designFile);
    LOG_E("Design init failed!");
  }
  if (RT_EOK != rtgui_design_parse(&ctx)) {
    close(designFile);
    LOG_E("Design parsing failed!");
  }
  close(designFile);
  picTmr = (rtgui_timer_t *)obj_tbl[0];
  bar = (rtgui_progress_t *)obj_tbl[1];
  sizer = (rtgui_box_t *)obj_tbl[2];
  LOG_I("picTmr @%p", picTmr);
  LOG_I("bar @%p", bar);
  LOG_I("sizer @%p", sizer);
  rtgui_box_layout(sizer);


  rtgui_win_show(ctx.win, RT_FALSE);
  rtgui_app_run(ctx.app);

  DELETE_WIN_INSTANCE(ctx.win);
  rtgui_app_uninit(ctx.app);
}

// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid = rt_thread_create(
    "picShow", picShow_entry, RT_NULL, 2048, 25, 10);
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
  struct picShow_info info;
  rtgui_evt_generic_t *evt;

  while (!ctx.app || IS_APP_FLAG(ctx.app, EXITED)) {
    LOG_I("Waiting app");
    rt_thread_sleep(100);
  }

  while (RT_TRUE) {
    if (!timeout) {
      rt_thread_sleep(100);
      continue;
    }

    while (!get_pic(&info)) {
      rt_thread_sleep(100);
      continue;
    }
    LOG_I("In loop TX: %s", info.path);

    RTGUI_CREATE_EVENT(evt, COMMAND, RT_WAITING_FOREVER);
    evt->command.wid = RT_NULL;
    evt->command.type = (rt_int32_t)&info;
    evt->command.command_id = CMD_PIC_SHOW;
    rtgui_request(ctx.app, evt, RT_WAITING_FOREVER);
    timeout = RT_FALSE;
  }
}
