#include <lvgl.h>
#include <Arduino.h>

#include "ui_main.h"
#include "dbc_decoder.h"   // per dbc_get_state()

// -----------------------
// Oggetti LVGL
// -----------------------

// Card centrale + SOC
static lv_obj_t *central_card    = nullptr;
static lv_obj_t *soc_bar         = nullptr;
static lv_obj_t *label_soc_title = nullptr;   // "SoC"
static lv_obj_t *label_soc_big   = nullptr;   // valore numerico

// Card angoli
static lv_obj_t *card_tl         = nullptr; // top-left
static lv_obj_t *card_tr         = nullptr; // top-right
static lv_obj_t *card_bl         = nullptr; // bottom-left
static lv_obj_t *card_br         = nullptr; // bottom-right

// Label nelle card
// Top-left
static lv_obj_t *label_mode      = nullptr;
static lv_obj_t *label_v_grid    = nullptr;

// Top-right
static lv_obj_t *label_rem_time  = nullptr;

// Bottom-left
static lv_obj_t *label_temp_batt = nullptr;
static lv_obj_t *label_temp_inv  = nullptr;

// Bottom-right
static lv_obj_t *label_p_dc      = nullptr;
static lv_obj_t *label_p_ac      = nullptr;

// ----------------------------------------------------
// Helper: stringa modalità MSM
// ----------------------------------------------------
static const char *mode_to_str(uint8_t msm_state)
{
  switch (msm_state)
  {
    case 0: return "Standby";
    case 1: return "Charging";
    case 2: return "Discharging";
    default: return "Unknown";
  }
}

// ----------------------------------------------------
// FUNZIONE PUBBLICA: aggiorna la UI dai dati DBC
// ----------------------------------------------------
void ui_main_update()
{
  const DbcState &s = dbc_get_state();

  // ---------- SOC centrale ----------
  {
    uint8_t soc = s.soc_percent;
    if (soc > 100) soc = 100;

    if (soc_bar) {
      lv_bar_set_value(soc_bar, soc, LV_ANIM_OFF);

      lv_color_t col;
      if (soc < 10) {
        col = lv_palette_main(LV_PALETTE_RED);
      } else if (soc < 40) {
        col = lv_palette_main(LV_PALETTE_YELLOW);
      } else {
        col = lv_palette_main(LV_PALETTE_GREEN);
      }

      // colore barra
      lv_obj_set_style_bg_color(soc_bar, col, LV_PART_INDICATOR);

      // bordo card centrale
      if (central_card) {
        lv_obj_set_style_border_color(central_card, col, 0);
      }

      // colore testi SOC
      if (label_soc_big) {
        lv_obj_set_style_text_color(label_soc_big, col, 0);
      }
      if (label_soc_title) {
        lv_obj_set_style_text_color(label_soc_title, col, 0);
      }
    }

    if (label_soc_big) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%u %%", (unsigned)soc);
      lv_label_set_text(label_soc_big, buf);
    }
  }

  // ---------- Top-left: mode + grid ----------
  {
    char buf[64];

    snprintf(buf, sizeof(buf), "Mode: %s", mode_to_str(s.msm_state));
    lv_label_set_text(label_mode, buf);

    float v_grid = s.grid_v_ac_deciv / 10.0f;
    snprintf(buf, sizeof(buf), "Grid: %.1f V", v_grid);
    lv_label_set_text(label_v_grid, buf);
  }

  // ---------- Top-right: remaining time ----------
  {
    char buf[64];
    float rem_min = s.remaining_time_s / 60.0f;
    snprintf(buf, sizeof(buf), "Remaining: %.1f min", rem_min);
    lv_label_set_text(label_rem_time, buf);
  }

  // ---------- Bottom-left: temperature ----------
  {
    char buf[64];

    snprintf(buf, sizeof(buf), "Batt Temp: %d C", (int)s.max_batt_temp_c);
    lv_label_set_text(label_temp_batt, buf);

    snprintf(buf, sizeof(buf), "Inv Temp:  %d C", (int)s.max_inv_temp_c);
    lv_label_set_text(label_temp_inv, buf);
  }

  // ---------- Bottom-right: potenze ----------
  {
    char buf[64];

    snprintf(buf, sizeof(buf), "P_DC: %d W", (int)s.bms_p_dc_w);
    lv_label_set_text(label_p_dc, buf);

    snprintf(buf, sizeof(buf), "P_AC: %d W", (int)s.inv_p_ac_w);
    lv_label_set_text(label_p_ac, buf);
  }
}

// ----------------------------------------------------
// Helper: crea una card rettangolare con bordi arrotondati
// contenuto centrato, bordo parametrico
// ----------------------------------------------------
static lv_obj_t *create_card(lv_obj_t *parent,
                             lv_coord_t w,
                             lv_coord_t h,
                             lv_color_t border_col)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_border_width(card, 6, 0);         // ⬅️ bordo spesso per tutte
  lv_obj_set_style_border_opa(card, LV_OPA_80, 0);
  lv_obj_set_style_border_color(card, border_col, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
  lv_obj_set_style_shadow_width(card, 12, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_set_style_pad_row(card, 6, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(card, lv_color_make(20, 20, 20), 0);  // card scura

  // flex column + centrato
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(card,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  // testo bianco di default sulle card
  lv_obj_set_style_text_color(card, lv_color_white(), 0);

  return card;
}

// ----------------------------------------------------
// Inizializzazione UI principale
// ----------------------------------------------------
void ui_main_init()
{
  Serial.println("[ui_main] Creo UI principale DBC (layout card)");

  lv_obj_t *scr = lv_scr_act();

  // Sfondo nero
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // Dimensioni card
  const lv_coord_t card_corner_w = 200;
  const lv_coord_t card_corner_h = 110;
  const lv_coord_t margin        = 10;

  // ---------------- CARD CENTRALE ----------------
  // colore iniziale del bordo (poi sovrascritto in funzione del SoC)
  central_card = create_card(scr, 260, 160, lv_palette_main(LV_PALETTE_GREY));
  lv_obj_align(central_card, LV_ALIGN_CENTER, 0, 0);

  // Ordine dentro la card centrale:
  //   1) "SoC"
  //   2) percentuale
  //   3) barra

  // Label "SoC" (grande)
  label_soc_title = lv_label_create(central_card);
  lv_label_set_text(label_soc_title, "SoC");
  lv_obj_set_style_text_font(label_soc_title, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_align(label_soc_title, LV_TEXT_ALIGN_CENTER, 0);

  // Label SOC percentuale
  label_soc_big = lv_label_create(central_card);
  lv_label_set_text(label_soc_big, "-- %");
  lv_obj_set_style_text_font(label_soc_big, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_align(label_soc_big, LV_TEXT_ALIGN_CENTER, 0);

  // Barra SOC in basso dentro la card
  soc_bar = lv_bar_create(central_card);
  lv_obj_set_width(soc_bar, lv_pct(100));
  lv_obj_set_height(soc_bar, 10);
  lv_bar_set_range(soc_bar, 0, 100);
  lv_bar_set_value(soc_bar, 0, LV_ANIM_OFF);

  // background barra (grigio scuro), indicatore colorato via update
  lv_obj_set_style_bg_color(soc_bar, lv_color_make(60, 60, 60), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(soc_bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(soc_bar, 6, LV_PART_MAIN);
  lv_obj_set_style_radius(soc_bar, 6, LV_PART_INDICATOR);

  // ---------------- 4 CARD AGLI ANGOLI ----------------

  // Top-left: Mode + Grid (blu)
  card_tl = create_card(scr, card_corner_w, card_corner_h,
                        lv_palette_main(LV_PALETTE_BLUE));
  lv_obj_align(card_tl, LV_ALIGN_TOP_LEFT, margin, margin);

  label_mode = lv_label_create(card_tl);
  lv_label_set_text(label_mode, "Mode: ---");
  lv_obj_set_style_text_align(label_mode, LV_TEXT_ALIGN_CENTER, 0);

  label_v_grid = lv_label_create(card_tl);
  lv_label_set_text(label_v_grid, "Grid: --.- V");
  lv_obj_set_style_text_align(label_v_grid, LV_TEXT_ALIGN_CENTER, 0);

  // Top-right: Remaining (teal)
  card_tr = create_card(scr, card_corner_w, card_corner_h,
                        lv_palette_main(LV_PALETTE_TEAL));
  lv_obj_align(card_tr, LV_ALIGN_TOP_RIGHT, -margin, margin);

  label_rem_time = lv_label_create(card_tr);
  lv_label_set_text(label_rem_time, "Remaining: --.- min");
  lv_obj_set_style_text_align(label_rem_time, LV_TEXT_ALIGN_CENTER, 0);

  // Bottom-left: Temperature (amber)
  card_bl = create_card(scr, card_corner_w, card_corner_h,
                        lv_palette_main(LV_PALETTE_AMBER));
  lv_obj_align(card_bl, LV_ALIGN_BOTTOM_LEFT, margin, -margin);

  label_temp_batt = lv_label_create(card_bl);
  lv_label_set_text(label_temp_batt, "Batt Temp: -- C");
  lv_obj_set_style_text_align(label_temp_batt, LV_TEXT_ALIGN_CENTER, 0);

  label_temp_inv = lv_label_create(card_bl);
  lv_label_set_text(label_temp_inv, "Inv Temp:  -- C");
  lv_obj_set_style_text_align(label_temp_inv, LV_TEXT_ALIGN_CENTER, 0);

  // Bottom-right: Power (purple)
  card_br = create_card(scr, card_corner_w, card_corner_h,
                        lv_palette_main(LV_PALETTE_PURPLE));
  lv_obj_align(card_br, LV_ALIGN_BOTTOM_RIGHT, -margin, -margin);

  label_p_dc = lv_label_create(card_br);
  lv_label_set_text(label_p_dc, "P_DC: --- W");
  lv_obj_set_style_text_align(label_p_dc, LV_TEXT_ALIGN_CENTER, 0);

  label_p_ac = lv_label_create(card_br);
  lv_label_set_text(label_p_ac, "P_AC: --- W");
  lv_obj_set_style_text_align(label_p_ac, LV_TEXT_ALIGN_CENTER, 0);

  Serial.println("[ui_main] UI card pronta (bordo spesso, SoC grande)");
}