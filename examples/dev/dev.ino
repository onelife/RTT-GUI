/***************************************************************************//**
 * @file    dev.ino
 * @brief   Arduino RT-Thread library "dev" example
 * @author  onelife <onelife.real[at]gmail.com>
 ******************************************************************************/
#include <rtt.h>
#include <rttgui.h>

static rt_bool_t show_demo(rtgui_win_t *win) {
  rtgui_dc_t *dc;
  rtgui_rect_t rect;

  dc = rtgui_dc_begin_drawing(TO_WIDGET(win));
  if (!dc) {
    rt_kprintf("no dc\n");
    return RT_FALSE;
  }
  rtgui_dc_get_rect(dc, &rect);
  rt_kprintf("*** rect (%d, %d) (%d, %d)\n", rect.x1, rect.y1, rect.x2, rect.y2);

  /* draw circular */
  {
    rt_int16_t x, y, r;

    x = rect.x2 / 2 / 2;
    y = rect.y2 / 2 / 2;
    r = x > y ? y : x;
    r -= y / 10;
    rtgui_dc_draw_circle(dc, x, y, r);
  }

  /* draw image */
  {
    rtgui_image_t *img;
    rtgui_rect_t draw_rect;

    // img = rtgui_image_create_from_mem("png", _picture_png, sizeof(_picture_png), 0, RT_TRUE);
    // img = rtgui_image_create_from_file("png", "/pic/logo.png", 0, RT_TRUE);
    // img = rtgui_image_create_from_file("bmp", "/pic/test_565.bmp", 0, RT_FALSE);
    img = rtgui_image_create_from_file("jpg", "/pic/test9.jpg", 0, RT_FALSE);
    if (img) {
      draw_rect.x1 = 10;
      draw_rect.y1 = 10;
      draw_rect.x2 = draw_rect.x1 + img->w;
      draw_rect.y2 = draw_rect.y1 + img->h;
      rt_kprintf("*** draw (%d, %d) (%d, %d)\n", draw_rect.x1, draw_rect.y1,
        draw_rect.x2, draw_rect.y2);
      rtgui_image_blit(img, dc, &draw_rect);
      rtgui_image_destroy(img);
    } else {
      rt_kprintf("*** load img filed\n");
    }
  }

  /* draw text */
  {
    rtgui_color_t fc;
    rtgui_rect_t draw_rect;
    const char text_buf[] = "哈罗 RT-Thread!";

    fc = RTGUI_DC_FC(dc);
    RTGUI_DC_FC(dc) = RED;
    draw_rect.x1 = rect.x2 * 2 / 5;
    draw_rect.y1 = rect.y2 * 3 / 4;
    draw_rect.x2 = rect.x2;
    draw_rect.y2 = rect.y2;
    rtgui_dc_draw_text(dc, text_buf, &draw_rect);
    RTGUI_DC_FC(dc) = fc;
  }

  rtgui_dc_end_drawing(dc, RT_TRUE);

  return RT_TRUE;
}

rt_bool_t dc_event_handler(void *obj, rtgui_evt_generic_t *evt) {
  rt_bool_t done = RT_FALSE;
  rt_bool_t do_paint = RT_FALSE;
  rtgui_win_t *win = TO_WIN(obj);

  rt_kprintf("*** handle %x\n", evt->base.type);
  do_paint = (evt->base.type == RTGUI_EVENT_PAINT);

  if (DEFAULT_HANDLER(win)) {
    done = DEFAULT_HANDLER(win)(TO_OBJECT(win), evt);
  }

  if (do_paint) {
    rt_kprintf("*** show_demo\n");
    done = show_demo(win);
  }

  return done;
}

static void rt_gui_demo_entry(void *param) {
  rtgui_win_t *main_win;
  rtgui_app_t *app;
  (void)param;

  /* create gui app */
  CREATE_APP_INSTANCE(app, RT_NULL, "gui_demo");
  if (!app) {
    rt_kprintf("create app failed\n");
    return;
  }
  rt_kprintf("*** create app ok\n");

  main_win = CREATE_MAIN_WIN(dc_event_handler, "UiWindow",
    RTGUI_WIN_STYLE_DEFAULT);
  if (!main_win) {
    rtgui_app_uninit(app);
    rt_kprintf("create mainwin failed\n");
    return;
  }
  rt_kprintf("*** create mainwin ok\n");
  rtgui_win_show(main_win, RT_FALSE);

  rtgui_app_run(app);
  rt_kprintf("*** app_run done\n");
  DELETE_WIN_INSTANCE(main_win);
  rtgui_app_uninit(app);
}


// RT-Thread function called by "RT_T.begin()"
void rt_setup(void) {
  rt_thread_t tid;

  tid = rt_thread_create(
          "mygui",
          rt_gui_demo_entry, RT_NULL,
          2048, 25, 10);

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
