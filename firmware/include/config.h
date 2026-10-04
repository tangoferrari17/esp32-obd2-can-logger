// config.h — pin map and tunables. Change these to match YOUR wiring before flashing.
#pragma once

#include <stdint.h>

// ---- Shared SPI bus (ESP32 VSPI defaults) ----
constexpr int PIN_SPI_SCK  = 18;
constexpr int PIN_SPI_MISO = 19;
constexpr int PIN_SPI_MOSI = 23;

// ---- Chip selects (one per SPI device) ----
constexpr int PIN_CAN_CS = 5;   // MCP2515 CS
constexpr int PIN_SD_CS  = 13;  // microSD CS

// ---- MCP2515 ----
// Most blue MCP2515/TJA1050 modules carry an 8 MHz crystal (read the can on the board:
// "8.000" = 8 MHz). Using the wrong value here silently produces the wrong bitrate and
// nothing on the bus will answer.
#define CAN_CRYSTAL MCP_8MHZ
#define CAN_BITRATE CAN_500KBPS  // ISO 15765-4 on passenger cars is 500 kbit/s

// ---- Polling ----
constexpr uint32_t RESPONSE_TIMEOUT_MS = 100;  // J1979 P2 max is 50 ms; leave margin
constexpr uint32_t INTER_REQUEST_GAP_MS = 10;
constexpr uint32_t SD_FLUSH_EVERY_N_LINES = 20;
