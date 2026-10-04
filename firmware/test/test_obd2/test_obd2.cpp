// Host-side unit tests for the OBD-II protocol layer.
// Run with:  pio test -e native
#include <unity.h>

#include "obd2.h"

using namespace obd2;

void setUp() {}
void tearDown() {}

static void test_build_rpm_request() {
  uint8_t frame[8];
  buildRequest(kEngineRpm, frame);
  const uint8_t expected[8] = {0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, frame, 8);
}

static void test_decode_rpm() {
  // 0x1A 0xF8 -> (26*256 + 248)/4 = 1726 rpm
  const uint8_t resp[8] = {0x04, 0x41, 0x0C, 0x1A, 0xF8, 0x00, 0x00, 0x00};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kOk, parseResponse(0x7E8, resp, 8, kEngineRpm, &r));
  TEST_ASSERT_EQUAL_FLOAT(1726.0f, r.value);
  TEST_ASSERT_EQUAL_UINT8(0, r.ecu);
}

static void test_decode_speed_and_temps() {
  const uint8_t b50[1] = {50};
  TEST_ASSERT_EQUAL_FLOAT(50.0f, decodeValue(kVehicleSpeed, b50));
  const uint8_t b130[1] = {130};
  TEST_ASSERT_EQUAL_FLOAT(90.0f, decodeValue(kCoolantTemp, b130));
  const uint8_t b0[1] = {0};
  TEST_ASSERT_EQUAL_FLOAT(-40.0f, decodeValue(kIntakeAirTemp, b0));
}

static void test_decode_throttle_and_maf() {
  const uint8_t full[1] = {255};
  TEST_ASSERT_EQUAL_FLOAT(100.0f, decodeValue(kThrottlePosition, full));
  const uint8_t maf[2] = {0x01, 0x2C};  // 300 / 100 = 3.00 g/s
  TEST_ASSERT_EQUAL_FLOAT(3.0f, decodeValue(kMafRate, maf));
}

static void test_second_ecu_id() {
  const uint8_t resp[8] = {0x03, 0x41, 0x0D, 0x3C, 0x00, 0x00, 0x00, 0x00};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kOk, parseResponse(0x7E9, resp, 8, kVehicleSpeed, &r));
  TEST_ASSERT_EQUAL_UINT8(1, r.ecu);
  TEST_ASSERT_EQUAL_FLOAT(60.0f, r.value);
}

static void test_rejects_non_response_id() {
  const uint8_t resp[8] = {0x04, 0x41, 0x0C, 0x1A, 0xF8, 0, 0, 0};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kNotAResponseId,
                    parseResponse(0x7DF, resp, 8, kEngineRpm, &r));
}

static void test_rejects_negative_response() {
  // 7F 01 12 = service 01 rejected, subFunctionNotSupported
  const uint8_t resp[8] = {0x03, 0x7F, 0x01, 0x12, 0, 0, 0, 0};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kNegativeResponse,
                    parseResponse(0x7E8, resp, 8, kEngineRpm, &r));
}

static void test_rejects_pid_mismatch() {
  const uint8_t resp[8] = {0x03, 0x41, 0x0D, 0x3C, 0, 0, 0, 0};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kPidMismatch,
                    parseResponse(0x7E8, resp, 8, kEngineRpm, &r));
}

static void test_rejects_truncated_frame() {
  // Claims 4 bytes (mode, pid, A, B) but RPM data is cut short by the length byte
  const uint8_t resp[8] = {0x03, 0x41, 0x0C, 0x1A, 0, 0, 0, 0};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kBadLength,
                    parseResponse(0x7E8, resp, 8, kEngineRpm, &r));
}

static void test_rejects_multi_frame() {
  // First frame of a multi-frame ISO-TP message (PCI high nibble = 1)
  const uint8_t resp[8] = {0x10, 0x14, 0x49, 0x02, 0x01, 0x57, 0x56, 0x47};
  Reading r;
  TEST_ASSERT_EQUAL(ParseStatus::kBadLength,
                    parseResponse(0x7E8, resp, 8, kEngineRpm, &r));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_build_rpm_request);
  RUN_TEST(test_decode_rpm);
  RUN_TEST(test_decode_speed_and_temps);
  RUN_TEST(test_decode_throttle_and_maf);
  RUN_TEST(test_second_ecu_id);
  RUN_TEST(test_rejects_non_response_id);
  RUN_TEST(test_rejects_negative_response);
  RUN_TEST(test_rejects_pid_mismatch);
  RUN_TEST(test_rejects_truncated_frame);
  RUN_TEST(test_rejects_multi_frame);
  return UNITY_END();
}
