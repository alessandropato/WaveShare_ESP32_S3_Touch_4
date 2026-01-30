#include "dbc_decoder.h"
#include <limits.h>

// ----------------------
// ID / configurazione DBC
// ----------------------

// ========================= VCU_Display_Status (0x1088A0F1) =========================
// B0   : SOC_TOT [% 0..100, 0xFF=invalid]
// B1   : SOC_ACTIVE [% 0..100, 0xFF=invalid]
// B2-3 : TimeToFull  [0.1 min, uint16]
// B4-5 : TimeToEmpty [0.1 min, uint16]
// B6   : MainStateMachineState (enum)
// B7   : Reserved
static const uint32_t DBC_ID_VCU_DISPLAY_STATUS   = 0x1088A0F1UL;
static const bool     DBC_VCU_DISPLAY_STATUS_EXTD = true;

// ========================= VCU_Display_Status_2 (0x1088A1F1) =========================
// B0-1: INV_P_AC_VECT[0] (0.1 kW, int16)
// B2-3: INV_P_AC_VECT[1] (0.1 kW, int16)
// B4-5: INV_P_AC_VECT[2] (0.1 kW, int16)
// B6-7: Reserved
static const uint32_t DBC_ID_VCU_DISPLAY_STATUS2   = 0x1088A1F1UL;
static const bool     DBC_VCU_DISPLAY_STATUS2_EXTD = true;

// Stato globale
static DbcState g_dbc_state;
static constexpr int32_t DBC_NO_DATA = -11;

// ----------------------
// Helper per LE 16 bit
// ----------------------
static uint16_t read_le_u16(const uint8_t *d)
{
  return (uint16_t)d[0] | ((uint16_t)d[1] << 8);
}

static int16_t read_le_i16(const uint8_t *d)
{
  return (int16_t)((uint16_t)d[0] | ((uint16_t)d[1] << 8));
}

// ----------------------
// Decoder VCU_Display_Status (0x1088A0F1)
// ----------------------
static void decode_vcu_display_status(const CanFrame &frame)
{
  if (frame.dlc < 8) {
    Serial.println("[DBC] VCU_Display_Status: DLC < 8, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  auto decode_percent = [](uint8_t raw) -> int16_t {
    return (raw == 0xFFU) ? static_cast<int16_t>(DBC_NO_DATA)
                          : static_cast<int16_t>(raw);
  };

  auto decode_time_s = [](uint16_t raw) -> int32_t {
    if (raw == 0xFFFFU) {
      return DBC_NO_DATA;
    }
    // 0.1 min -> 6 seconds
    return static_cast<int32_t>(raw) * 6;
  };

  g_dbc_state.soc_tot_percent    = decode_percent(d[0]);
  g_dbc_state.soc_active_percent = decode_percent(d[1]);
  uint16_t raw_ttf = read_le_u16(&d[2]);
  uint16_t raw_tte = read_le_u16(&d[4]);
  uint8_t raw_state = d[6];

  g_dbc_state.time_to_full_s  = decode_time_s(raw_ttf);
  g_dbc_state.time_to_empty_s = decode_time_s(raw_tte);
  g_dbc_state.main_state      = (raw_state == 0xFFU)
                                   ? static_cast<int16_t>(DBC_NO_DATA)
                                   : static_cast<int16_t>(raw_state);
  g_dbc_state.status_lastUpdate_ms = frame.timestamp_ms;

  Serial.printf(
      "[DBC] Status: SOC_TOT=%d%%, SOC_ACTIVE=%d%%, TTF=%.1f min, TTE=%.1f min, STATE=%d\n",
      (int)g_dbc_state.soc_tot_percent,
      (int)g_dbc_state.soc_active_percent,
      (g_dbc_state.time_to_full_s > 0) ? (g_dbc_state.time_to_full_s / 60.0f) : -1.0f,
      (g_dbc_state.time_to_empty_s > 0) ? (g_dbc_state.time_to_empty_s / 60.0f) : -1.0f,
      (int)g_dbc_state.main_state
  );
}

// ----------------------
// Decoder VCU_Display_Status_2 (0x1088A1F1)
// ----------------------
static void decode_vcu_display_status2(const CanFrame &frame)
{
  if (frame.dlc < 6) {
    Serial.println("[DBC] VCU_Display_Status_2: DLC < 6, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  for (int i = 0; i < 3; ++i) {
    int16_t raw_deci_kw = read_le_i16(&d[i * 2]);
    if (raw_deci_kw == INT16_MIN) {
      g_dbc_state.inv_p_ac_w[i] = DBC_NO_DATA;
    } else {
      g_dbc_state.inv_p_ac_w[i] = static_cast<int32_t>(raw_deci_kw) * 100; // 0.1 kW -> W
    }
  }

  g_dbc_state.status2_lastUpdate_ms = frame.timestamp_ms;

  Serial.printf(
      "[DBC] Status2: P_AC = [%.1f, %.1f, %.1f] kW\n",
      g_dbc_state.inv_p_ac_w[0] / 1000.0f,
      g_dbc_state.inv_p_ac_w[1] / 1000.0f,
      g_dbc_state.inv_p_ac_w[2] / 1000.0f
  );
}

// ----------------------
// Entry point DBC
// ----------------------
void dbc_handle_frame(const CanFrame &frame)
{
  if (frame.rtr) {
    return; // niente RTR per ora
  }

  // VCU_Display_Status
  if (frame.extended == DBC_VCU_DISPLAY_STATUS_EXTD &&
      frame.id       == DBC_ID_VCU_DISPLAY_STATUS)
  {
    decode_vcu_display_status(frame);
    return;
  }

  // VCU_Display_Status_2
  if (frame.extended == DBC_VCU_DISPLAY_STATUS2_EXTD &&
      frame.id       == DBC_ID_VCU_DISPLAY_STATUS2)
  {
    decode_vcu_display_status2(frame);
    return;
  }

  // Altri messaggi: ignorati in silenzio
}

// Stato globale in sola lettura
const DbcState &dbc_get_state()
{
  return g_dbc_state;
}
