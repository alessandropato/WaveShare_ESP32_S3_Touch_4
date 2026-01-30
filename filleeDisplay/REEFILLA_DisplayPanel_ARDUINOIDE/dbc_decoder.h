#pragma once

#include <Arduino.h>
#include "can_port.h"   // per la struct CanFrame

// Stato globale decodificato dal "DBC"
struct DbcState
{
  // ================== VCU_Display_Status (0x1088A0F1) ==================
  int16_t  soc_tot_percent       = -11; // SOC_TOT [%]
  int16_t  soc_active_percent    = -11; // SOC_ACTIVE [%]
  int32_t  time_to_full_s        = -11; // TimeToFull [s]
  int32_t  time_to_empty_s       = -11; // TimeToEmpty [s]
  int16_t  main_state            = -11; // Main state machine enum
  uint32_t status_lastUpdate_ms  = 0;   // millis ultima ricezione valida

  // ================== VCU_Display_Status_2 (0x1088A1F1) ==================
  int32_t  inv_p_ac_w[3]         = { -11, -11, -11 }; // INV_P_AC_VECT[*] [W]
  uint32_t status2_lastUpdate_ms = 0;   // millis ultima ricezione valida
};

// Gestisce un frame CAN secondo il nostro "DBC"
// - se il messaggio è riconosciuto, lo decodifica, aggiorna lo stato e stampa sulla seriale
// - se non è riconosciuto, lo ignora (silenzio)
void dbc_handle_frame(const CanFrame &frame);

// Accesso in sola lettura allo stato decodificato
const DbcState &dbc_get_state();
