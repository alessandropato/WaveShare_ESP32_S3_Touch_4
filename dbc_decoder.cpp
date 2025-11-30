#include "dbc_decoder.h"

// ----------------------
// ID / configurazione DBC
// ----------------------

// Messaggio 1: BattTime
// ID messaggio: 0x108890F1 (frame CAN esteso).
// Decodifica: primi 2 byte = timeToEmpty in minuti (little-endian),
//             successivi 2 byte = timeToFull in minuti;
//             ultimi 4 byte non usati.
static const uint32_t DBC_ID_BATT_TIME   = 0x108890F1UL;
static const bool     DBC_BATT_TIME_EXTD = true;

// Messaggio 2: VCU_BUS_DC_MSG
// ID 0x108810F1 (esteso), 8 byte little-endian:
//
// B0-B1: tensione lato batteria (BMS_V_DC) scalata 0.1 V → uint16.
// B2-B3: tensione lato inverter (INV_V_DC) scalata 0.1 V → uint16.
// B4-B5: delta assoluto tra i due valori (|BMS_V_DC - INV_V_DC|) 0.1 V → uint16.
// B6-B7: riservati, forzati a 0xFFFF (li ignoriamo).
static const uint32_t DBC_ID_VCU_BUS_DC   = 0x108810F1UL;
static const bool     DBC_VCU_BUS_DC_EXTD = true;

// Stato globale
static DbcState g_dbc_state;

// ----------------------
// Helper per leggere LE 16 bit
// ----------------------
static uint16_t read_le_u16(const uint8_t *d)
{
  return (uint16_t)d[0] | ((uint16_t)d[1] << 8);
}

// ----------------------
// Decoder messaggio BattTime (0x108890F1)
// ----------------------
static void decode_batt_time(const CanFrame &frame)
{
  if (frame.dlc < 4) {
    Serial.println("[DBC] BattTime: DLC < 4, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  uint16_t timeToEmpty = read_le_u16(&d[0]);  // minuti
  uint16_t timeToFull  = read_le_u16(&d[2]);  // minuti

  // Aggiorna stato globale
  g_dbc_state.batt_timeToEmpty_min = timeToEmpty;
  g_dbc_state.batt_timeToFull_min  = timeToFull;
  g_dbc_state.batt_lastUpdate_ms   = frame.timestamp_ms;

  // Stampa decodificata
  Serial.printf("[DBC] BattTime: TTE=%u min, TTF=%u min\n",
                (unsigned)timeToEmpty,
                (unsigned)timeToFull);
}

// ----------------------
// Decoder messaggio VCU_BUS_DC_MSG (0x108810F1)
// ----------------------
static void decode_vcu_bus_dc(const CanFrame &frame)
{
  if (frame.dlc < 6) {
    Serial.println("[DBC] VCU_BUS_DC_MSG: DLC < 6, frame ignorato");
    return;
  }

  const uint8_t *d = frame.data;

  uint16_t bms_v_dc_deciv   = read_le_u16(&d[0]); // 0.1 V
  uint16_t inv_v_dc_deciv   = read_le_u16(&d[2]); // 0.1 V
  uint16_t delta_v_dc_deciv = read_le_u16(&d[4]); // 0.1 V

  // Aggiorna stato globale
  g_dbc_state.bms_v_dc_deciv        = bms_v_dc_deciv;
  g_dbc_state.inv_v_dc_deciv        = inv_v_dc_deciv;
  g_dbc_state.delta_v_dc_deciv      = delta_v_dc_deciv;
  g_dbc_state.vcu_bus_lastUpdate_ms = frame.timestamp_ms;

  // Convertiamo in volt per stampa leggibile
  float bms_v  = bms_v_dc_deciv   / 10.0f;
  float inv_v  = inv_v_dc_deciv   / 10.0f;
  float delta  = delta_v_dc_deciv / 10.0f;

  Serial.printf("[DBC] VCU_BUS_DC: BMS=%.1f V, INV=%.1f V, DELTA=%.1f V\n",
                bms_v, inv_v, delta);
}

// ----------------------
// Entry point DBC
// ----------------------
void dbc_handle_frame(const CanFrame &frame)
{
  // Ignoriamo RTR per ora
  if (frame.rtr) {
    return;
  }

  // Messaggio BattTime
  if (frame.extended == DBC_BATT_TIME_EXTD &&
      frame.id       == DBC_ID_BATT_TIME)
  {
    decode_batt_time(frame);
    return;
  }

  // Messaggio VCU_BUS_DC_MSG
  if (frame.extended == DBC_VCU_BUS_DC_EXTD &&
      frame.id       == DBC_ID_VCU_BUS_DC)
  {
    decode_vcu_bus_dc(frame);
    return;
  }

  // Tutti gli altri messaggi CAN: ignorati (nessuna stampa)
}

// Stato globale in sola lettura
const DbcState &dbc_get_state()
{
  return g_dbc_state;
}