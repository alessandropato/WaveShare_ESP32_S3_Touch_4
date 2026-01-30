#include <Arduino.h>
#include "can_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "dbc_decoder.h"    // usa la logica DBC

// ----------------------------------------------------
// CONFIGURAZIONE CAN (TWAI)
// ----------------------------------------------------

// Pin CAN della Waveshare ESP32-S3 4" (TJA1051 on-board)
#define CAN_RX_PIN  GPIO_NUM_0   // CANRX
#define CAN_TX_PIN  GPIO_NUM_6   // CANTX

// Bitrate: 250 kbit/s
#define CAN_TIMING   TWAI_TIMING_CONFIG_250KBITS()

// Dimensione coda interna dei messaggi (tutti quelli che passano da qui)
#define CAN_RX_QUEUE_LEN  64

// ----------------------------------------------------
// STATE INTERNI
// ----------------------------------------------------

static QueueHandle_t s_can_rx_queue       = nullptr;
static bool          s_can_driver_installed = false;
static bool          s_can_started          = false;
static TaskHandle_t  s_can_rx_task_handle   = nullptr;

// ----------------------------------------------------
// TASK DI RICEZIONE CAN
// ----------------------------------------------------

static void can_rx_task(void *arg)
{
  (void)arg;
  Serial.println("[can_port] RX task avviato");

  while (true)
  {
    twai_message_t msg;
    esp_err_t res = twai_receive(&msg, pdMS_TO_TICKS(1000));

    if (res == ESP_OK)
    {
      // Popoliamo il nostro CanFrame "pulito"
      CanFrame frame{};
      frame.id           = msg.identifier;
      frame.extended     = msg.extd;
      frame.rtr          = msg.rtr;
      frame.dlc          = msg.data_length_code;
      frame.timestamp_ms = millis();
      memset(frame.data, 0, sizeof(frame.data));
      memcpy(frame.data, msg.data, msg.data_length_code);

      // 1) Passiamo il frame al DBC: se è un messaggio noto, lo decodifica e stampa
      dbc_handle_frame(frame);

      // 2) Mettiamo il frame in coda (se vuoi usarlo altrove o loggarlo)
      if (s_can_rx_queue)
      {
        xQueueSend(s_can_rx_queue, &frame, 0);   // non blocca se la coda è piena
      }
    }
    else if (res == ESP_ERR_TIMEOUT)
    {
      // Nessun messaggio entro timeout: ok, non facciamo nulla
    }
    else
    {
      Serial.printf("[can_port] twai_receive errore: %d\n", (int)res);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

// ----------------------------------------------------
// INIZIALIZZAZIONE DRIVER CAN (TWAI)
// ----------------------------------------------------

bool can_port_init()
{
  if (s_can_driver_installed)
  {
    Serial.println("[can_port] Driver già installato");
    return true;
  }

  Serial.println("[can_port] Inizializzo TWAI (CAN) in NORMAL a 250 kbit/s");

  // Modalità NORMAL per dare ACK ma senza trasmettere (non chiamiamo mai twai_transmit)
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);

  // Timing (250 kbit/s)
  twai_timing_config_t t_config = CAN_TIMING;

  // Filtro HW: accetta tutto; filtriamo a livello DBC
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t res = twai_driver_install(&g_config, &t_config, &f_config);
  if (res != ESP_OK)
  {
    Serial.printf("[can_port] twai_driver_install fallita, err = %d\n", (int)res);
    return false;
  }

  s_can_driver_installed = true;

  // Coda per i frame (per ora mettiamo tutto; il DBC filtra per ID)
  s_can_rx_queue = xQueueCreate(CAN_RX_QUEUE_LEN, sizeof(CanFrame));
  if (!s_can_rx_queue)
  {
    Serial.println("[can_port] ERRORE: impossibile creare la coda RX");
    return false;
  }

  Serial.println("[can_port] Driver installato con successo");
  return true;
}

// ----------------------------------------------------
// AVVIO DRIVER + TASK RX
// ----------------------------------------------------

bool can_port_start()
{
  if (!s_can_driver_installed)
  {
    Serial.println("[can_port] ERRORE: can_port_init() non chiamata");
    return false;
  }

  if (s_can_started)
  {
    Serial.println("[can_port] CAN già avviata");
    return true;
  }

  esp_err_t res = twai_start();
  if (res != ESP_OK)
  {
    Serial.printf("[can_port] twai_start fallita, err = %d\n", (int)res);
    return false;
  }

  s_can_started = true;
  Serial.println("[can_port] TWAI avviata, creo task RX");

  BaseType_t task_res = xTaskCreatePinnedToCore(
      can_rx_task,
      "can_rx_task",
      4096,        // stack
      nullptr,
      5,           // priorità
      &s_can_rx_task_handle,
      0            // core 0
  );

  if (task_res != pdPASS)
  {
    Serial.println("[can_port] ERRORE: impossibile creare can_rx_task");
    return false;
  }

  return true;
}

// ----------------------------------------------------
// API PER LEGGERE UN MESSAGGIO DALLA CODA
// ----------------------------------------------------

bool can_port_get_frame(CanFrame &out, uint32_t timeout_ms)
{
  if (!s_can_rx_queue)
    return false;

  TickType_t to_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xQueueReceive(s_can_rx_queue, &out, to_ticks) == pdTRUE)
  {
    return true;
  }
  return false;
}