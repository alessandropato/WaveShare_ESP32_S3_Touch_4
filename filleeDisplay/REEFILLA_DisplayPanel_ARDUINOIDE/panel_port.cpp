#include <Arduino.h>
#include "panel_port.h"

// Oggetti statici visibili solo in questo file
static ESP_Panel        *s_panel      = nullptr;
static ESP_PanelLcd     *s_lcd        = nullptr;
static ESP_PanelTouch   *s_touch      = nullptr;
static ESP_PanelBacklight *s_backlight = nullptr;

bool panel_port_init()
{
  Serial.println("[panel_port] Creo oggetto ESP_Panel");
  s_panel = new ESP_Panel();

  Serial.println("[panel_port] panel->init()...");
  if (!s_panel->init()) {
    Serial.println("[panel_port] ERRORE: panel->init() ha fallito.");
    return false;
  }

  Serial.println("[panel_port] panel->begin()...");
  if (!s_panel->begin()) {
    Serial.println("[panel_port] ERRORE: panel->begin() ha fallito.");
    return false;
  }

  s_lcd       = s_panel->getLcd();
  s_touch     = s_panel->getTouch();
  s_backlight = s_panel->getBacklight();

  if (!s_lcd) {
    Serial.println("[panel_port] ERRORE: lcd == nullptr (config errata).");
    return false;
  }

  if (s_backlight) {
    Serial.println("[panel_port] Accendo retroilluminazione.");
    s_backlight->on();
  }

  uint16_t w = s_panel->getLcdWidth();
  uint16_t h = s_panel->getLcdHeight();
  Serial.printf("[panel_port] Pannello %u x %u\n", w, h);

  if (s_touch) {
    Serial.println("[panel_port] Touch presente.");
  } else {
    Serial.println("[panel_port] Touch NON presente (touch == nullptr)");
  }

  return true;
}

// Getter
ESP_PanelLcd* panel_port_get_lcd()
{
  return s_lcd;
}

ESP_PanelTouch* panel_port_get_touch()
{
  return s_touch;
}

ESP_PanelBacklight* panel_port_get_backlight()
{
  return s_backlight;
}

ESP_Panel* panel_port_get_panel()
{
  return s_panel;
}
