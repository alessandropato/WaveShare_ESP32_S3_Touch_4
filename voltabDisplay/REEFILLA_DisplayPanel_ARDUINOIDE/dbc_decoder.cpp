#include "dbc_decoder.h"

// ----------------------
// ID / configurazione DBC
// ----------------------

// ========================= VCU_Display_Status (0x1088A0F1) =========================
// B0   : BMS_SOC [% interi, 0..100]
// B1-2 : Remaining time [s, uint16] (TimeToFull se in carica, TimeToEmpty se in scarica)
// B3   : MSM debounced state (0=standby,1=carica,2=scarica)
// B4   : Max battery temperature [°C, int8]
// B5   : Max inverter temperature [°C, int8]
// B6-7 : BMS_P_DC [W, int16]
static const uint32_t DBC_ID_VCU_DISPLAY_STATUS   = 0x1088A0F1UL;
static const bool     DBC_VCU_DISPLAY_STATUS_EXTD = true;

// ========================= VCU_Display_Status_2 (0x1088A1F1) =========================
// B0-1: INV_GRID_V_AC [0.1 V, uint16]
// B2-3: INV_P_AC [W, int16]
// B4-7: RESERVED
static const uint32_t DBC_ID_VCU_DISPLAY_STATUS2   = 0x1088A1F1UL;
static const bool     DBC_VCU_DISPLAY_STATUS2_EXTD = true;

// Stato globale
static DbcState g_dbc_state;

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
  if (frame.dlc < 7) {
    Serial.println("[DBC] VCU_Display_Status: DLC < 7, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  uint8_t  soc_percent      = d[0];
  uint16_t remaining_time_s = read_le_u16(&d[1]);
  uint8_t  msm_state        = d[3];
  int8_t   max_batt_temp_c  = (int8_t)d[4];
  int8_t   max_inv_temp_c   = (int8_t)d[5];
  int16_t  bms_p_dc_w       = read_le_i16(&d[6]);

  g_dbc_state.soc_percent          = soc_percent;
  g_dbc_state.remaining_time_s     = remaining_time_s;
  g_dbc_state.msm_state            = msm_state;
  g_dbc_state.max_batt_temp_c      = max_batt_temp_c;
  g_dbc_state.max_inv_temp_c       = max_inv_temp_c;
  g_dbc_state.bms_p_dc_w           = bms_p_dc_w;
  g_dbc_state.status_lastUpdate_ms = frame.timestamp_ms;

  // Log leggibile
  const char *mode_str = "unknown";
  switch (msm_state) {
    case 0: mode_str = "standby";  break;
    case 1: mode_str = "charge";   break;
    case 2: mode_str = "discharge";break;
  }

  float remaining_min = remaining_time_s / 60.0f;

  Serial.printf(
      "[DBC] Status: SOC=%u%%, Rem=%.1f min, Mode=%s, T_batt=%dC, T_inv=%dC, P_DC=%d W\n",
      (unsigned)soc_percent,
      remaining_min,
      mode_str,
      (int)max_batt_temp_c,
      (int)max_inv_temp_c,
      (int)bms_p_dc_w
  );
}

// ----------------------
// Decoder VCU_Display_Status_2 (0x1088A1F1)
// ----------------------
static void decode_vcu_display_status2(const CanFrame &frame)
{
  if (frame.dlc < 4) {
    Serial.println("[DBC] VCU_Display_Status_2: DLC < 4, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  uint16_t grid_v_ac_deciv = read_le_u16(&d[0]);  // 0.1 V
  int16_t  inv_p_ac_w      = read_le_i16(&d[2]);  // [W]

  g_dbc_state.grid_v_ac_deciv       = grid_v_ac_deciv;
  g_dbc_state.inv_p_ac_w            = inv_p_ac_w;
  g_dbc_state.status2_lastUpdate_ms = frame.timestamp_ms;

  float grid_v = grid_v_ac_deciv / 10.0f;

  Serial.printf(
      "[DBC] Status2: V_grid=%.1f V, P_AC=%d W\n",
      grid_v,
      (int)inv_p_ac_w
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