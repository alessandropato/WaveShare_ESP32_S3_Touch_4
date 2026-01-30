#include <lvgl.h>
#include <Arduino.h>
#include <ctype.h>
#include <string.h>

#include "ui_main.h"
#include "dbc_decoder.h"   // per dbc_get_state()

// -----------------------
// Oggetti LVGL
// -----------------------

// SoC gauge
static lv_obj_t *soc_arc_bg        = nullptr;
static lv_obj_t *soc_arc_fg        = nullptr;
static lv_obj_t *label_soc_value   = nullptr;
static lv_obj_t *label_soc_percent = nullptr;
static lv_obj_t *label_soc_state   = nullptr;

// Time remaining
static lv_obj_t *label_time_title = nullptr;
static lv_obj_t *label_time_value = nullptr;
static lv_obj_t *label_time_unit  = nullptr;

// Icone stato (placeholder)
static lv_obj_t *icon_warn = nullptr;
static lv_obj_t *icon_stop = nullptr;

// Line info (3 colonne in basso)
static lv_obj_t *label_line_name[3]  = { nullptr, nullptr, nullptr };
static lv_obj_t *label_line_value[3] = { nullptr, nullptr, nullptr };
static lv_obj_t *label_line_unit[3]  = { nullptr, nullptr, nullptr };

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
  uint8_t soc = s.soc_percent;
  if (soc > 100) soc = 100;

  lv_color_t accent_col;
  if (soc < 10) {
    accent_col = lv_palette_main(LV_PALETTE_RED);
  } else if (soc < 40) {
    accent_col = lv_palette_main(LV_PALETTE_YELLOW);
  } else {
    accent_col = lv_palette_main(LV_PALETTE_BLUE);
  }

  if (soc_arc_fg) {
    lv_arc_set_value(soc_arc_fg, soc);
    lv_obj_set_style_arc_color(soc_arc_fg, accent_col, LV_PART_INDICATOR);
  }

  if (label_soc_value) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)soc);
    lv_label_set_text(label_soc_value, buf);
  }

  if (label_soc_percent) {
    lv_label_set_text(label_soc_percent, "%");
  }

  if (label_soc_state) {
    const char *mode = mode_to_str(s.msm_state);
    char buf[32];
    size_t len = strlen(mode);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    for (size_t i = 0; i < len; ++i) {
      buf[i] = toupper((unsigned char)mode[i]);
    }
    buf[len] = '\0';
    lv_label_set_text(label_soc_state, buf);
  }

  // ---------- Tempo rimanente ----------
  if (label_time_value) {
    char buf[16];
    uint32_t total_sec = s.remaining_time_s;
    uint32_t minutes = total_sec / 60U;
    uint32_t seconds = total_sec % 60U;
    snprintf(buf, sizeof(buf), "%u:%02u", (unsigned)minutes, (unsigned)seconds);
    lv_label_set_text(label_time_value, buf);
  }

  if (label_time_unit) {
    lv_label_set_text(label_time_unit, "min");
  }

  // ---------- Linee inferiori (placeholder: valori attuali) ----------
  int line_values[3];
  line_values[0] = (int)s.bms_p_dc_w;
  line_values[1] = (int)s.inv_p_ac_w;
  line_values[2] = (int)s.bms_p_dc_w - (int)s.inv_p_ac_w;

  for (int i = 0; i < 3; ++i) {
    if (label_line_value[i]) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%d", line_values[i]);
      lv_label_set_text(label_line_value[i], buf);
    }
    if (label_line_unit[i]) {
      lv_label_set_text(label_line_unit[i], "W");
    }
  }
}

// ----------------------------------------------------
// Inizializzazione UI principale
// ----------------------------------------------------
void ui_main_init()
{
  Serial.println("[ui_main] Creo UI principale stile single-panel");

  lv_obj_t *scr = lv_scr_act();

  // Sfondo teal pieno
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x009EA8), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // --------- Blocco tempo rimanente (alto sinistra) ---------
  lv_obj_t *time_block = lv_obj_create(scr);
  lv_obj_remove_style_all(time_block);
  lv_obj_set_size(time_block, 220, 100);
  lv_obj_align(time_block, LV_ALIGN_TOP_LEFT, 20, 12);
  lv_obj_set_flex_flow(time_block, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(time_block,
                        LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(time_block, 6, 0);
  lv_obj_move_foreground(time_block);

  label_time_title = lv_label_create(time_block);
  lv_label_set_text(label_time_title, "TIME REMAINING");
  lv_obj_set_style_text_color(label_time_title, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_time_title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(label_time_title, LV_TEXT_ALIGN_CENTER, 0);

  label_time_value = lv_label_create(time_block);
  lv_label_set_text(label_time_value, "--:--");
  lv_obj_set_style_text_color(label_time_value, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_time_value, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_align(label_time_value, LV_TEXT_ALIGN_CENTER, 0);

  label_time_unit = lv_label_create(time_block);
  lv_label_set_text(label_time_unit, "min");
  lv_obj_set_style_text_color(label_time_unit, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_time_unit, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_align(label_time_unit, LV_TEXT_ALIGN_CENTER, 0);

  // --------- Icone stato (alto destra) ---------
  icon_warn = lv_obj_create(scr);
  lv_obj_remove_style_all(icon_warn);
  lv_obj_set_size(icon_warn, 46, 46);
  lv_obj_set_style_radius(icon_warn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(icon_warn, lv_color_hex(0xFFC857), 0);
  lv_obj_set_style_bg_opa(icon_warn, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(icon_warn, 12, 0);
  lv_obj_set_style_shadow_opa(icon_warn, LV_OPA_30, 0);
  lv_obj_align(icon_warn, LV_ALIGN_TOP_RIGHT, -80, 20);
  lv_obj_t *warn_label = lv_label_create(icon_warn);
  lv_label_set_text(warn_label, "!");
  lv_obj_set_style_text_color(warn_label, lv_color_black(), 0);
  lv_obj_center(warn_label);

  icon_stop = lv_obj_create(scr);
  lv_obj_remove_style_all(icon_stop);
  lv_obj_set_size(icon_stop, 46, 46);
  lv_obj_set_style_radius(icon_stop, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(icon_stop, lv_color_hex(0xF05454), 0);
  lv_obj_set_style_bg_opa(icon_stop, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(icon_stop, 12, 0);
  lv_obj_set_style_shadow_opa(icon_stop, LV_OPA_30, 0);
  lv_obj_align(icon_stop, LV_ALIGN_TOP_RIGHT, -20, 20);
  lv_obj_t *stop_label = lv_label_create(icon_stop);
  lv_label_set_text(stop_label, "X");
  lv_obj_set_style_text_color(stop_label, lv_color_white(), 0);
  lv_obj_center(stop_label);

  // --------- Gauge circolare ---------
  const lv_coord_t arc_size = 260;

  soc_arc_bg = lv_arc_create(scr);
  lv_obj_set_size(soc_arc_bg, arc_size, arc_size);
  lv_arc_set_range(soc_arc_bg, 0, 100);
  lv_arc_set_rotation(soc_arc_bg, 270);
  lv_arc_set_bg_angles(soc_arc_bg, 20, 340);
  lv_arc_set_value(soc_arc_bg, 100);
  lv_obj_align(soc_arc_bg, LV_ALIGN_CENTER, 0, -10);
  lv_obj_clear_flag(soc_arc_bg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(soc_arc_bg, 26, LV_PART_MAIN);
  lv_obj_set_style_arc_color(soc_arc_bg, lv_color_hex(0x007E81), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(soc_arc_bg, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(soc_arc_bg, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(soc_arc_bg, LV_OPA_TRANSP, LV_PART_KNOB);

  soc_arc_fg = lv_arc_create(scr);
  lv_obj_set_size(soc_arc_fg, arc_size, arc_size);
  lv_arc_set_range(soc_arc_fg, 0, 100);
  lv_arc_set_rotation(soc_arc_fg, 270);
  lv_arc_set_bg_angles(soc_arc_fg, 20, 340);
  lv_arc_set_value(soc_arc_fg, 0);
  lv_obj_align(soc_arc_fg, LV_ALIGN_CENTER, 0, -10);
  lv_obj_clear_flag(soc_arc_fg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(soc_arc_fg, 26, LV_PART_MAIN);
  lv_obj_set_style_arc_color(soc_arc_fg, lv_color_hex(0x00585B), LV_PART_MAIN);
  lv_obj_set_style_arc_width(soc_arc_fg, 26, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(soc_arc_fg, lv_palette_lighten(LV_PALETTE_CYAN, 2), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(soc_arc_fg, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(soc_arc_fg, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_move_foreground(soc_arc_fg);

  // --------- Testi centrali ---------
  label_soc_value = lv_label_create(scr);
  lv_label_set_text(label_soc_value, "--");
  lv_obj_set_style_text_font(label_soc_value, &lv_font_montserrat_32, 0);
  lv_obj_set_style_transform_zoom(label_soc_value, 384, 0);  // ~1.5x
  lv_obj_set_style_text_color(label_soc_value, lv_color_white(), 0);
  lv_obj_align(label_soc_value, LV_ALIGN_CENTER, 0, -40);

  label_soc_percent = lv_label_create(scr);
  lv_label_set_text(label_soc_percent, "%");
  lv_obj_set_style_text_font(label_soc_percent, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(label_soc_percent, lv_color_white(), 0);
  lv_obj_align(label_soc_percent, LV_ALIGN_CENTER, 0, 0);

  label_soc_state = lv_label_create(scr);
  lv_label_set_text(label_soc_state, "STATE");
  lv_obj_set_style_text_color(label_soc_state, lv_color_white(), 0);
  lv_obj_align(label_soc_state, LV_ALIGN_CENTER, 0, 40);

  // --------- Fascia inferiore con 3 colonne ---------
  lv_obj_t *bottom_row = lv_obj_create(scr);
  lv_obj_remove_style_all(bottom_row);
  lv_obj_set_size(bottom_row, lv_pct(100), 140);
  lv_obj_align(bottom_row, LV_ALIGN_BOTTOM_MID, 0, 8);
  lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom_row,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(bottom_row, 30, 0);
  lv_obj_set_style_pad_left(bottom_row, 20, 0);
  lv_obj_set_style_pad_right(bottom_row, 20, 0);

  const char *line_titles[3] = { "LINE 1", "LINE 2", "LINE 3" };
  for (int i = 0; i < 3; ++i) {
    lv_obj_t *col = lv_obj_create(bottom_row);
    lv_obj_remove_style_all(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_text_align(col, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(col, lv_color_white(), 0);
    lv_obj_set_style_pad_all(col, 0, 0);
    lv_obj_set_style_pad_row(col, 6, 0);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, lv_pct(100));

    label_line_name[i] = lv_label_create(col);
    lv_label_set_text(label_line_name[i], line_titles[i]);

    label_line_value[i] = lv_label_create(col);
    lv_label_set_text(label_line_value[i], "----");
    lv_obj_set_style_text_font(label_line_value[i], &lv_font_montserrat_32, 0);

    label_line_unit[i] = lv_label_create(col);
    lv_label_set_text(label_line_unit[i], "W");
  }

  Serial.println("[ui_main] UI stile gauge pronta");
}
