// scan モード(LaserScan)向けロジックの単体テスト(native 環境: pio test -e native)
#include <unity.h>

#include "toio_range_logic.h"

void setUp(void) {}
void tearDown(void) {}

// --- toio_scan_params ---

void test_scan_is_single_front_beam(void) {
  const toio_scan_params_t p = toio_scan_params(100);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, p.angle_min);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, p.angle_max);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, p.angle_increment);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, p.time_increment);
}

void test_scan_time_matches_publish_period(void) {
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.1f, toio_scan_params(100).scan_time);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.2f, toio_scan_params(200).scan_time);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, toio_scan_params(1000).scan_time);
}

void test_scan_range_bounds_match_sensor_spec(void) {
  const toio_scan_params_t p = toio_scan_params(100);
  TEST_ASSERT_EQUAL_FLOAT(TOIO_RANGE_MIN_M, p.range_min);
  TEST_ASSERT_EQUAL_FLOAT(TOIO_RANGE_MAX_M, p.range_max);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.03f, p.range_min);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 2.0f, p.range_max);
}

// --- ranges[0] に入る値(共通の変換ロジックとの結合) ---

void test_scan_beam_value_for_valid_measurement(void) {
  float ranges[1];
  ranges[0] = toio_range_to_meters(456, true);
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.456f, ranges[0]);
}

void test_scan_beam_value_for_invalid_measurement_is_infinity(void) {
  float ranges[1];
  ranges[0] = toio_range_to_meters(8190, !toio_range_is_invalid(8190));
  TEST_ASSERT_TRUE(isinf(ranges[0]));
  TEST_ASSERT_TRUE(ranges[0] > 0);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_scan_is_single_front_beam);
  RUN_TEST(test_scan_time_matches_publish_period);
  RUN_TEST(test_scan_range_bounds_match_sensor_spec);
  RUN_TEST(test_scan_beam_value_for_valid_measurement);
  RUN_TEST(test_scan_beam_value_for_invalid_measurement_is_infinity);
  return UNITY_END();
}
