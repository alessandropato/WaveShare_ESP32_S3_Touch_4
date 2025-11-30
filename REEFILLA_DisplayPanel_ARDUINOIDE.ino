#include <Arduino.h>
#include "panel_port.h"
#include "lv_port.h"
#include "ui_main.h"
#include "can_port.h"

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("===== ESP32-S3 4\" PANEL - LVGL + CAN =====");

  if (!panel_port_init()) {
    Serial.println("ERRORE: panel_port_init() fallita, mi fermo.");
    while (true) {
      delay(1000);
    }
  }

  lv_port_init(panel_port_get_lcd(), panel_port_get_touch());
  ui_main_init();

  // ---- CAN ----
  if (!can_port_init()) {
    Serial.println("ERRORE: can_port_init() fallita");
  } else {
    if (!can_port_start()) {
      Serial.println("ERRORE: can_port_start() fallita");
    } else {
      Serial.println("CAN avviata in LISTEN_ONLY");
    }
  }

  Serial.println("Setup completato: UI + CAN pronte.");}

void loop()
{
  // il tuo lv_conf.h usa LV_TICK_CUSTOM con millis()
  lv_timer_handler();
  delay(5);
}