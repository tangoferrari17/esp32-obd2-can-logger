// obd2.h — OBD-II (SAE J1979) Mode 01 request building and response decoding
// over ISO 15765-4 CAN, single-frame messages only.
//
// Deliberately hardware-independent: no Arduino, no MCP2515. That keeps the
// protocol logic unit-testable on a laptop (`pio test -e native`) before any of
// it is sent onto a real vehicle bus.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace obd2 {

// ISO 15765-4, 11-bit identifiers
constexpr uint32_t kFunctionalRequestId = 0x7DF;  // broadcast to all ECUs
constexpr uint32_t kResponseIdFirst     = 0x7E8;  // ECU #1 (usually the engine)
constexpr uint32_t kResponseIdLast      = 0x7EF;  // ECU #8

constexpr uint8_t kModeCurrentData   = 0x01;
constexpr uint8_t kPositiveResponse  = 0x40;  // response mode = request mode + 0x40
constexpr uint8_t kNegativeResponse  = 0x7F;
constexpr uint8_t kPadByte           = 0x00;  // J1979 allows 0x00 or 0x55; 0x00 is common

// Standard PIDs this logger understands.
enum Pid : uint8_t {
  kPidsSupported01_20 = 0x00,
  kCoolantTemp        = 0x05,
  kEngineRpm          = 0x0C,
  kVehicleSpeed       = 0x0D,
  kIntakeAirTemp      = 0x0F,
  kMafRate            = 0x10,
  kThrottlePosition   = 0x11,
};

struct PidInfo {
  uint8_t     pid;
  const char* name;
  const char* unit;
  uint8_t     dataBytes;  // number of data bytes (A, B, ...) in the response
};

// Returns metadata for a supported PID, or nullptr if the PID isn't known.
const PidInfo* lookupPid(uint8_t pid);

// Builds the 8-byte single-frame request: [0x02, 0x01, PID, pad x5].
// Byte 0 is the ISO-TP PCI byte: high nibble 0 = single frame, low nibble = length (2).
void buildRequest(uint8_t pid, uint8_t out[8]);

enum class ParseStatus {
  kOk,
  kNotAResponseId,     // CAN ID outside 0x7E8–0x7EF
  kBadLength,          // DLC or ISO-TP length makes no sense
  kNegativeResponse,   // ECU replied 0x7F (request rejected)
  kWrongMode,          // not a Mode 01 positive response
  kPidMismatch,        // response is for a different PID than expected
  kUnknownPid,         // PID not in the decode table
};

struct Reading {
  uint8_t  pid;
  uint8_t  ecu;    // 0–7, derived from response ID (0x7E8 -> 0)
  float    value;  // engineering units, see PidInfo::unit
  uint8_t  raw[4]; // raw data bytes A..D as received
  uint8_t  rawLen;
};

// Validates and decodes one received CAN frame as the answer to `expectedPid`.
ParseStatus parseResponse(uint32_t canId, const uint8_t* data, uint8_t dlc,
                          uint8_t expectedPid, Reading* out);

// Applies the J1979 scaling formula for a PID. Exposed separately for testing.
float decodeValue(uint8_t pid, const uint8_t* bytes);

const char* statusString(ParseStatus s);

}  // namespace obd2
