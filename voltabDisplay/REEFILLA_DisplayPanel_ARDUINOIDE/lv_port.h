#pragma once

#include <lvgl.h>
#include <ESP_Panel_Library.h>

// Inizializza LVGL (display + input) usando gli oggetti del pannello.
void lv_port_init(ESP_PanelLcd *lcd, ESP_PanelTouch *touch);
