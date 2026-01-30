#pragma once

#include <Arduino.h>
#include "can_port.h"   // per la struct CanFrame

// Stato globale decodificato dal "DBC"
struct DbcState
{
  // ================== VCU_Display_Status (0x1088A0F1) ==================
  uint8_t  soc_percent           = 0;   // BMS_SOC [% 0..100]
  uint16_t remaining_time_s      = 0;   // Remaining time [s]
  uint8_t  msm_state             = 0;   // 0=standby,1=carica,2=scarica
  int8_t   max_batt_temp_c       = 0;   // [°C]
  int8_t   max_inv_temp_c        = 0;   // [°C]
  int16_t  bms_p_dc_w            = 0;   // [W]
  uint32_t status_lastUpdate_ms  = 0;   // millis ultima ricezione valida

  // ================== VCU_Display_Status_2 (0x1088A1F1) ==================
  uint16_t grid_v_ac_deciv       = 0;   // INV_GRID_V_AC [0.1 V]
  int16_t  inv_p_ac_w            = 0;   // INV_P_AC [W]
  uint32_t status2_lastUpdate_ms = 0;   // millis ultima ricezione valida
};

// Gestisce un frame CAN secondo il nostro "DBC"
// - se il messaggio è riconosciuto, lo decodifica, aggiorna lo stato e stampa sulla seriale
// - se non è riconosciuto, lo ignora (silenzio)
void dbc_handle_frame(const CanFrame &frame);

// Accesso in sola lettura allo stato decodificato
const DbcState &dbc_get_state();