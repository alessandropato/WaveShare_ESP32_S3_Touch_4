#pragma once

#include <ESP_Panel_Library.h>

// Inizializza il pannello Waveshare (display + touch + backlight).
// Ritorna true se tutto ok.
bool panel_port_init();

// Getter globali sugli oggetti del pannello
ESP_PanelLcd*       panel_port_get_lcd();
ESP_PanelTouch*     panel_port_get_touch();
ESP_PanelBacklight* panel_port_get_backlight();
ESP_Panel*          panel_port_get_panel();
