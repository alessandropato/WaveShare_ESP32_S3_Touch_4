#include <lvgl.h>
#include <Arduino.h>
#include "ui_main.h"

// Callback evento bottone
static void ui_btn_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {
    Serial.println("[ui_main] Bottone cliccato!");
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, "Clicked!");
  }
}

void ui_main_init()
{
  Serial.println("[ui_main] Creo UI principale (Hello + Button)");

  // Label principale
  lv_obj_t *main_label = lv_label_create(lv_scr_act());
  lv_label_set_text(main_label, "Hello LVGL");
  lv_obj_center(main_label);

  // Bottone
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 160, 60);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_add_event_cb(btn, ui_btn_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Tap me");
  lv_obj_center(btn_label);
}
