#include "obd2.h"

namespace obd2 {

namespace {

const PidInfo kPidTable[] = {
    {kPidsSupported01_20, "pids_supported_01_20", "bitmask", 4},
    {kCoolantTemp,        "coolant_temp",         "degC",    1},
    {kEngineRpm,          "engine_rpm",           "rpm",     2},
    {kVehicleSpeed,       "vehicle_speed",        "km/h",    1},
    {kIntakeAirTemp,      "intake_air_temp",      "degC",    1},
    {kMafRate,            "maf_rate",             "g/s",     2},
    {kThrottlePosition,   "throttle_position",    "%",       1},
};

}  // namespace

const PidInfo* lookupPid(uint8_t pid) {
  for (const PidInfo& info : kPidTable) {
    if (info.pid == pid) return &info;
  }
  return nullptr;
}

void buildRequest(uint8_t pid, uint8_t out[8]) {
  out[0] = 0x02;              // single frame, 2 payload bytes follow
  out[1] = kModeCurrentData;  // service 01
  out[2] = pid;
  for (int i = 3; i < 8; ++i) out[i] = kPadByte;
}

float decodeValue(uint8_t pid, const uint8_t* b) {
  const float A = b[0];
  const float B = b[1];
  switch (pid) {
    case kEngineRpm:        return (A * 256.0f + B) / 4.0f;
    case kVehicleSpeed:     return A;
    case kCoolantTemp:      return A - 40.0f;
    case kIntakeAirTemp:    return A - 40.0f;
    case kThrottlePosition: return A * 100.0f / 255.0f;
    case kMafRate:          return (A * 256.0f + B) / 100.0f;
    case kPidsSupported01_20:
      // Bitmask doesn't fit a float losslessly; callers should use Reading::raw.
      return 0.0f;
    default:                return 0.0f;
  }
}

ParseStatus parseResponse(uint32_t canId, const uint8_t* data, uint8_t dlc,
                          uint8_t expectedPid, Reading* out) {
  if (canId < kResponseIdFirst || canId > kResponseIdLast) {
    return ParseStatus::kNotAResponseId;
  }
  if (dlc < 3) return ParseStatus::kBadLength;

  const uint8_t pci = data[0];
  if ((pci & 0xF0) != 0x00) return ParseStatus::kBadLength;  // not a single frame
  const uint8_t len = pci & 0x0F;
  if (len < 2 || len > 7 || len + 1 > dlc) return ParseStatus::kBadLength;

  if (data[1] == kNegativeResponse) return ParseStatus::kNegativeResponse;
  if (data[1] != (kModeCurrentData | kPositiveResponse)) return ParseStatus::kWrongMode;
  if (data[2] != expectedPid) return ParseStatus::kPidMismatch;

  const PidInfo* info = lookupPid(data[2]);
  if (info == nullptr) return ParseStatus::kUnknownPid;

  // len counts mode + PID + data bytes
  const uint8_t dataLen = len - 2;
  if (dataLen < info->dataBytes) return ParseStatus::kBadLength;

  out->pid = data[2];
  out->ecu = static_cast<uint8_t>(canId - kResponseIdFirst);
  out->rawLen = info->dataBytes;
  for (uint8_t i = 0; i < 4; ++i) out->raw[i] = (i < info->dataBytes) ? data[3 + i] : 0;
  out->value = decodeValue(out->pid, &data[3]);
  return ParseStatus::kOk;
}

const char* statusString(ParseStatus s) {
  switch (s) {
    case ParseStatus::kOk:               return "ok";
    case ParseStatus::kNotAResponseId:   return "not_a_response_id";
    case ParseStatus::kBadLength:        return "bad_length";
    case ParseStatus::kNegativeResponse: return "negative_response";
    case ParseStatus::kWrongMode:        return "wrong_mode";
    case ParseStatus::kPidMismatch:      return "pid_mismatch";
    case ParseStatus::kUnknownPid:       return "unknown_pid";
  }
  return "?";
}

}  // namespace obd2
