#pragma once

#include <Arduino.h>
#include "can_port.h"   // per la struct CanFrame

// Stato globale decodificato dal "DBC"
struct DbcState
{
  // Messaggio 0x108890F1 - BattTime
  uint16_t batt_timeToEmpty_min   = 0;  // minuti
  uint16_t batt_timeToFull_min    = 0;  // minuti
  uint32_t batt_lastUpdate_ms     = 0;  // millis ultima ricezione valida

  // Messaggio 0x108810F1 - VCU_BUS_DC_MSG
  uint16_t bms_v_dc_deciv         = 0;  // decimi di volt (es: 523 -> 52.3 V)
  uint16_t inv_v_dc_deciv         = 0;  // decimi di volt
  uint16_t delta_v_dc_deciv       = 0;  // decimi di volt
  uint32_t vcu_bus_lastUpdate_ms  = 0;  // millis ultima ricezione valida
};

// Gestisce un frame CAN secondo il nostro "DBC"
// - se il messaggio è riconosciuto, lo decodifica, aggiorna lo stato e stampa sulla seriale
// - se non è riconosciuto, lo ignora (silenzio)
void dbc_handle_frame(const CanFrame &frame);

// Accesso in sola lettura allo stato decodificato
const DbcState &dbc_get_state();