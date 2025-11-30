#include <lvgl.h>
#include <Arduino.h>

#include "ui_main.h"
#include "dbc_decoder.h"   // per dbc_get_state()

// Puntatori agli oggetti LVGL che vogliamo aggiornare
static lv_obj_t *label_tte      = nullptr;
static lv_obj_t *label_ttf      = nullptr;
static lv_obj_t *label_bms_v    = nullptr;
static lv_obj_t *label_inv_v    = nullptr;
static lv_obj_t *label_delta_v  = nullptr;
static lv_obj_t *label_batt_age = nullptr;
static lv_obj_t *label_bus_age  = nullptr;

// ----------------------------------------------------
// Helper: formatta decimi di volt come "xx.x V"
// ----------------------------------------------------
static void set_label_decivolt(lv_obj_t *label, const char *prefix, uint16_t deciv)
{
  uint16_t v_int  = deciv / 10;
  uint16_t v_frac = deciv % 10;

  char buf[32];
  snprintf(buf, sizeof(buf), "%s%u.%u V", prefix, (unsigned)v_int, (unsigned)v_frac);
  lv_label_set_text(label, buf);
}

// ----------------------------------------------------
// FUNZIONE PUBBLICA: aggiorna la UI dai dati DBC
// (la chiameremo dal loop() ogni 1 secondo)
// ----------------------------------------------------
void ui_main_update()
{
  const DbcState &s = dbc_get_state();
  uint32_t now_ms = millis();

  // --- Battery time ---
  {
    char buf[48];
    snprintf(buf, sizeof(buf), "Time to Empty: %u min", s.batt_timeToEmpty_min);
    lv_label_set_text(label_tte, buf);

    snprintf(buf, sizeof(buf), "Time to Full:  %u min", s.batt_timeToFull_min);
    lv_label_set_text(label_ttf, buf);

    if (s.batt_lastUpdate_ms > 0) {
      uint32_t age_s = (now_ms - s.batt_lastUpdate_ms) / 1000;
      snprintf(buf, sizeof(buf), "Last update: %lus ago", (unsigned long)age_s);
      lv_label_set_text(label_batt_age, buf);
    } else {
      lv_label_set_text(label_batt_age, "Last update: n/a");
    }
  }

  // --- DC bus ---
  {
    set_label_decivolt(label_bms_v,   "BMS:  ", s.bms_v_dc_deciv);
    set_label_decivolt(label_inv_v,   "INV:  ", s.inv_v_dc_deciv);
    set_label_decivolt(label_delta_v, "ΔV:   ", s.delta_v_dc_deciv);

    char buf[48];
    if (s.vcu_bus_lastUpdate_ms > 0) {
      uint32_t age_s = (now_ms - s.vcu_bus_lastUpdate_ms) / 1000;
      snprintf(buf, sizeof(buf), "Last update: %lus ago", (unsigned long)age_s);
      lv_label_set_text(label_bus_age, buf);
    } else {
      lv_label_set_text(label_bus_age, "Last update: n/a");
    }
  }
}

// ----------------------------------------------------
// Creazione "card" estetica
// ----------------------------------------------------
static lv_obj_t *create_card(lv_obj_t *parent, const char *title_text)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_style_pad_all(card, 12, 0);
  lv_obj_set_style_pad_row(card, 6, 0);
  lv_obj_set_style_radius(card, 12, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_opa(card, LV_OPA_20, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
  lv_obj_set_style_shadow_width(card, 12, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *title = lv_label_create(card);
  lv_label_set_text(title, title_text);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_opa(title, LV_OPA_80, 0);

  return card;
}

// ----------------------------------------------------
// Init UI
// ----------------------------------------------------
void ui_main_init()
{
  Serial.println("[ui_main] Creo UI principale DBC");

  lv_obj_t *scr = lv_scr_act();

  lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(scr, 10, 0);
  lv_obj_set_style_pad_row(scr, 10, 0);

  // Titolo
  lv_obj_t *header = lv_label_create(scr);
  lv_label_set_text(header, "VCU Realtime Status");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_opa(header, LV_OPA_90, 0);

  // Spaziatore
  lv_obj_t *spacer = lv_obj_create(scr);
  lv_obj_remove_style_all(spacer);
  lv_obj_set_size(spacer, LV_SIZE_CONTENT, 4);

  // Card 1
  lv_obj_t *card_batt = create_card(scr, "Battery Time");

  label_tte = lv_label_create(card_batt);
  lv_label_set_text(label_tte, "Time to Empty: -- min");

  label_ttf = lv_label_create(card_batt);
  lv_label_set_text(label_ttf, "Time to Full: -- min");

  label_batt_age = lv_label_create(card_batt);
  lv_label_set_text(label_batt_age, "Last update: n/a");
  lv_obj_set_style_text_opa(label_batt_age, LV_OPA_60, 0);
  lv_obj_set_style_text_font(label_batt_age, &lv_font_montserrat_14, 0);

  // Card 2
  lv_obj_t *card_bus = create_card(scr, "DC Bus");

  label_bms_v = lv_label_create(card_bus);
  lv_label_set_text(label_bms_v, "BMS: --.- V");

  label_inv_v = lv_label_create(card_bus);
  lv_label_set_text(label_inv_v, "INV: --.- V");

  label_delta_v = lv_label_create(card_bus);
  lv_label_set_text(label_delta_v, "ΔV: --.- V");

  label_bus_age = lv_label_create(card_bus);
  lv_label_set_text(label_bus_age, "Last update: n/a");
  lv_obj_set_style_text_opa(label_bus_age, LV_OPA_60, 0);
  lv_obj_set_style_text_font(label_bus_age, &lv_font_montserrat_14, 0);

  Serial.println("[ui_main] UI DBC pronta");
}