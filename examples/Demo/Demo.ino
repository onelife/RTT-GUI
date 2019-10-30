/***************************************************************************//**
 * @file    Demo.ino
 * @brief   RTT-GUI library "Demo" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

static rt_bool_t show_demo(rtgui_win_t *win) {
  rtgui_dc_t *dc;

  dc = rtgui_dc_begin_drawing(TO_WIDGET(win));
  if (!dc) {
    rt_kprintf("no dc\n");
    return RT_FALSE;
  }

  /* draw image */
  {
    rtgui_image_t *img;
    rtgui_rect_t draw_rect;

    img = rtgui_image_create_from_file("bmp", "/pic/logo.bmp", 0, RT_FALSE);
    if (img) {
      draw_rect.x1 = 10;
      draw_rect.y1 = 10;
      draw_rect.x2 = draw_rect.x1 + img->w;
      draw_rect.y2 = draw_rect.y1 + img->h;
      rtgui_image_blit(img, dc, &draw_rect);
      rtgui_image_destroy(img);
    } else {
      rt_kprintf("*** load img filed\n");
    }
  }

  /* draw pie shape */
  {
    rtgui_color_t fc;

    fc = RTGUI_DC_FC(dc);
    RTGUI_DC_FC(dc) = GREEN;
    rtgui_dc_fill_pie(dc, 100, 150, 50, 30, 270);
    RTGUI_DC_FC(dc) = fc;
  }

  /* draw text */
  {
    rtgui_color_t fc;
    rtgui_rect_t rect;
    const char text_buf[] = "哈罗 Arduino!";

    fc = RTGUI_DC_FC(dc);

    RTGUI_DC_FC(dc) = RED;
    rtgui_dc_get_rect(dc, &rect);
    rect.x1 = rect.x2 * 1 / 5;
    rect.y1 = rect.y2 * 3 / 4;
    rtgui_dc_draw_text(dc, text_buf, &rect);

    RTGUI_DC_FC(dc) = fc;
  }

  rtgui_dc_end_drawing(dc, RT_TRUE);

  return RT_TRUE;
}

rt_bool_t win_event_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;

  if (DEFAULT_HANDLER(obj)) {
    done = DEFAULT_HANDLER(obj)(obj, evt);
  }

  if (IS_EVENT_TYPE(evt, PAINT)) {
    done = show_demo(TO_WIN(obj));
  }

  return done;
}

static void rt_gui_demo_entry(void *param) {
  rtgui_app_t *app;
  rtgui_win_t *main_win;
  (void)param;

  /* create gui app */
  CREATE_APP_INSTANCE(app, RT_NULL, "Demo App");
  if (!app) {
    rt_kprintf("create app failed\n");
    return;
  }

  main_win = CREATE_MAIN_WIN(win_event_handler, "Demo Window",
    RTGUI_WIN_STYLE_DEFAULT);
  if (!main_win) {
    rtgui_app_uninit(app);
    rt_kprintf("create mainwin failed\n");
    return;
  }

  do {
    rtgui_box_t *sizer;
    rtgui_label_t* label;
    rtgui_button_t *btn;

    sizer = CREATE_BOX_INSTANCE(main_win, RTGUI_HORIZONTAL, 0);
    if (!sizer) {
      rt_kprintf("Create sizer failed!\n");
      break;
    }

    label = CREATE_LABEL_INSTANCE(main_win, RT_NULL, RT_NULL, "Label Demo");
    if (!label) {
      rt_kprintf("Create label failed!\n");
      break;
    }
    WIDGET_TEXTALIGN_SET(label, CENTER_HORIZONTAL);
    WIDGET_ALIGN_SET(label, STRETCH);

    btn = CREATE_BUTTON_INSTANCE(main_win, RT_NULL, NORMAL, "Button Demo");
    if (!btn) {
      rt_kprintf("Create button failed!\n");
      break;
    }
    WIDGET_ALIGN_SET(btn, STRETCH);

    rtgui_box_layout(sizer);
    rtgui_win_show(main_win, RT_FALSE);
    rtgui_app_run(app);
  } while (0);

  DELETE_WIN_INSTANCE(main_win);
  rtgui_app_uninit(app);
}


// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid;

  tid = rt_thread_create(
    "demo_gui", rt_gui_demo_entry, RT_NULL,
    CONFIG_APP_STACK_SIZE, CONFIG_APP_PRIORITY, CONFIG_APP_TIMESLICE);

  if (tid) {
    rt_thread_startup(tid);
  } else {
    rt_kprintf("create thread failed\n");
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
