// ESP32 + MCP2515 OBD-II CAN logger
//
// Boot sequence:
//   1. Bring up SPI, the MCP2515 and the SD card.
//   2. Loopback self-test: the MCP2515 sends a frame to itself without touching the
//      bus. If this fails, the SPI wiring or crystal setting is wrong, and nothing is
//      ever transmitted onto the vehicle.
//   3. Normal mode: poll each PID in turn (request on 0x7DF, answer on 0x7E8–0x7EF)
//      and append every result, including timeouts and errors, to a CSV on the SD card.
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <mcp2515.h>

#include "config.h"
#include "obd2.h"

namespace {

MCP2515* can = nullptr;
File logFile;
uint32_t linesSinceFlush = 0;

const uint8_t kPollList[] = {
    obd2::kEngineRpm,    obd2::kVehicleSpeed, obd2::kCoolantTemp,
    obd2::kThrottlePosition, obd2::kIntakeAirTemp, obd2::kMafRate,
};

void halt(const char* why) {
  Serial.printf("FATAL: %s\n", why);
  while (true) delay(1000);
}

bool loopbackSelfTest() {
  can->setLoopbackMode();
  can_frame tx{};
  tx.can_id = obd2::kFunctionalRequestId;
  tx.can_dlc = 8;
  obd2::buildRequest(obd2::kEngineRpm, tx.data);
  if (can->sendMessage(&tx) != MCP2515::ERROR_OK) return false;

  const uint32_t start = millis();
  can_frame rx{};
  while (millis() - start < 100) {
    if (can->readMessage(&rx) == MCP2515::ERROR_OK) {
      return rx.can_id == tx.can_id && rx.can_dlc == 8 &&
             memcmp(rx.data, tx.data, 8) == 0;
    }
  }
  return false;
}

// Only accept frames 0x7E8–0x7EF so unrelated bus traffic never reaches the CPU.
void configureResponseFilters() {
  can->setFilterMask(MCP2515::MASK0, false, 0x7F8);
  can->setFilter(MCP2515::RXF0, false, obd2::kResponseIdFirst);
  can->setFilter(MCP2515::RXF1, false, obd2::kResponseIdFirst);
  can->setFilterMask(MCP2515::MASK1, false, 0x7F8);
  for (auto f : {MCP2515::RXF2, MCP2515::RXF3, MCP2515::RXF4, MCP2515::RXF5}) {
    can->setFilter(f, false, obd2::kResponseIdFirst);
  }
}

File openNextLogFile() {
  char path[20];
  for (int i = 0; i < 1000; ++i) {
    snprintf(path, sizeof(path), "/log_%03d.csv", i);
    if (!SD.exists(path)) {
      File f = SD.open(path, FILE_WRITE);
      if (f) {
        f.println("ms,pid,name,value,unit,ecu,raw_hex,status");
        Serial.printf("Logging to %s\n", path);
      }
      return f;
    }
  }
  return File();
}

void writeLine(const char* line) {
  Serial.println(line);
  if (!logFile) return;
  logFile.println(line);
  if (++linesSinceFlush >= SD_FLUSH_EVERY_N_LINES) {
    logFile.flush();  // bounds data lost on power cut to the last N lines
    linesSinceFlush = 0;
  }
}

void pollPid(uint8_t pid) {
  const obd2::PidInfo* info = obd2::lookupPid(pid);
  can_frame tx{};
  tx.can_id = obd2::kFunctionalRequestId;
  tx.can_dlc = 8;
  obd2::buildRequest(pid, tx.data);

  char line[128];
  const uint32_t sentAt = millis();
  if (can->sendMessage(&tx) != MCP2515::ERROR_OK) {
    snprintf(line, sizeof(line), "%lu,0x%02X,%s,,,,,tx_error",
             (unsigned long)sentAt, pid, info->name);
    writeLine(line);
    return;
  }

  can_frame rx{};
  while (millis() - sentAt < RESPONSE_TIMEOUT_MS) {
    if (can->readMessage(&rx) != MCP2515::ERROR_OK) continue;

    obd2::Reading r;
    obd2::ParseStatus st = obd2::parseResponse(rx.can_id, rx.data, rx.can_dlc, pid, &r);
    if (st == obd2::ParseStatus::kPidMismatch) continue;  // late answer to an older request

    char raw[3 * 8 + 1] = {0};
    for (uint8_t i = 0; i < rx.can_dlc && i < 8; ++i) {
      snprintf(raw + 3 * i, 4, "%02X%s", rx.data[i], i + 1 < rx.can_dlc ? " " : "");
    }
    if (st == obd2::ParseStatus::kOk) {
      snprintf(line, sizeof(line), "%lu,0x%02X,%s,%.2f,%s,%u,%s,ok",
               (unsigned long)millis(), pid, info->name, r.value, info->unit, r.ecu, raw);
    } else {
      snprintf(line, sizeof(line), "%lu,0x%02X,%s,,,,%s,%s",
               (unsigned long)millis(), pid, info->name, raw, obd2::statusString(st));
    }
    writeLine(line);
    return;
  }

  snprintf(line, sizeof(line), "%lu,0x%02X,%s,,,,,timeout",
           (unsigned long)millis(), pid, info->name);
  writeLine(line);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 OBD-II CAN logger");

  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);  // keep SD deselected while the MCP2515 initializes

  can = new MCP2515(PIN_CAN_CS);
  if (can->reset() != MCP2515::ERROR_OK) halt("MCP2515 reset failed (check SPI wiring/CS)");
  if (can->setBitrate(CAN_BITRATE, CAN_CRYSTAL) != MCP2515::ERROR_OK) halt("setBitrate failed");

  if (!loopbackSelfTest()) halt("MCP2515 loopback self-test failed");
  Serial.println("Loopback self-test passed");

  configureResponseFilters();
  can->setNormalMode();

  if (SD.begin(PIN_SD_CS)) {
    logFile = openNextLogFile();
  } else {
    Serial.println("WARNING: SD init failed, logging to serial only");
  }
}

void loop() {
  for (uint8_t pid : kPollList) {
    pollPid(pid);
    delay(INTER_REQUEST_GAP_MS);
  }
}
