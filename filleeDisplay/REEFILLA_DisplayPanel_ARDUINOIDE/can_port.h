#pragma once

#include <Arduino.h>
#include "driver/twai.h"

// Frame CAN "pulito" usato dal resto del codice
struct CanFrame
{
  uint32_t id;
  bool     extended;
  bool     rtr;
  uint8_t  dlc;
  uint8_t  data[8];
  uint32_t timestamp_ms;
};

// Inizializza il driver CAN (TWAI) a 250 kbit/s
bool can_port_init();

// Avvia il driver e crea il task RX
bool can_port_start();

// Legge un messaggio dalla coda interna (solo messaggi filtrati/gestiti)
// timeout_ms = 0 -> non blocca
bool can_port_get_frame(CanFrame &out, uint32_t timeout_ms = 0);