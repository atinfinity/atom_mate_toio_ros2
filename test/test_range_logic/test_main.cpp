// toio_range_logic の単体テスト(native 環境: pio test -e native)
#include <unity.h>

#include "toio_range_logic.h"

void setUp(void) {}
void tearDown(void) {}

// --- toio_range_is_invalid ---

void test_valid_measurement_is_not_invalid(void) {
  TEST_ASSERT_FALSE(toio_range_is_invalid(0));
  TEST_ASSERT_FALSE(toio_range_is_invalid(123));
  TEST_ASSERT_FALSE(toio_range_is_invalid(8189));
}

void test_out_of_range_values_are_invalid(void) {
  TEST_ASSERT_TRUE(toio_range_is_invalid(8190));
  TEST_ASSERT_TRUE(toio_range_is_invalid(8191));
  TEST_ASSERT_TRUE(toio_range_is_invalid(65535));
}

// --- toio_range_to_meters ---

void test_mm_is_converted_to_meters(void) {
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.123f, toio_range_to_meters(123, true));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 2.0f, toio_range_to_meters(2000, true));
  TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, toio_range_to_meters(0, true));
}

void test_invalid_measurement_becomes_infinity(void) {
  float r = toio_range_to_meters(123, false);
  TEST_ASSERT_TRUE(isinf(r));
  TEST_ASSERT_TRUE(r > 0);
}

// --- toio_range_stamp_from_ms ---

void test_stamp_splits_ms_into_sec_and_nanosec(void) {
  int32_t sec = 0;
  uint32_t nanosec = 0;
  toio_range_stamp_from_ms(1723400000123LL, &sec, &nanosec);
  TEST_ASSERT_EQUAL_INT32(1723400000, sec);
  TEST_ASSERT_EQUAL_UINT32(123000000u, nanosec);
}

void test_stamp_under_one_second(void) {
  int32_t sec = 0;
  uint32_t nanosec = 0;
  toio_range_stamp_from_ms(999, &sec, &nanosec);
  TEST_ASSERT_EQUAL_INT32(0, sec);
  TEST_ASSERT_EQUAL_UINT32(999000000u, nanosec);
}

void test_stamp_exact_second_has_zero_nanosec(void) {
  int32_t sec = 0;
  uint32_t nanosec = 0;
  toio_range_stamp_from_ms(5000, &sec, &nanosec);
  TEST_ASSERT_EQUAL_INT32(5, sec);
  TEST_ASSERT_EQUAL_UINT32(0u, nanosec);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_valid_measurement_is_not_invalid);
  RUN_TEST(test_out_of_range_values_are_invalid);
  RUN_TEST(test_mm_is_converted_to_meters);
  RUN_TEST(test_invalid_measurement_becomes_infinity);
  RUN_TEST(test_stamp_splits_ms_into_sec_and_nanosec);
  RUN_TEST(test_stamp_under_one_second);
  RUN_TEST(test_stamp_exact_second_has_zero_nanosec);
  return UNITY_END();
}
