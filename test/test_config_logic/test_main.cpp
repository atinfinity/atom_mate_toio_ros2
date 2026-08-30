// toio_config_logic の単体テスト(native 環境: pio test -e native)
#include <unity.h>

#include "toio_config_logic.h"

void setUp(void) {}
void tearDown(void) {}

// --- SSID / パスワード ---

void test_ssid_must_not_be_empty(void) {
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_SSID_EMPTY, toio_config_validate_ssid(""));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_ssid("a"));
}

void test_ssid_length_limit_is_32(void) {
  char s[40];
  memset(s, 'x', sizeof(s));
  s[32] = '\0';
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_ssid(s));
  s[32] = 'x';
  s[33] = '\0';
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_SSID_TOO_LONG, toio_config_validate_ssid(s));
}

void test_password_may_be_empty_but_not_over_63(void) {
  char s[70];
  memset(s, 'x', sizeof(s));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_password(""));
  s[63] = '\0';
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_password(s));
  s[63] = 'x';
  s[64] = '\0';
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_PASSWORD_TOO_LONG, toio_config_validate_password(s));
}

// --- IPv4 ---

void test_ipv4_valid_addresses(void) {
  uint8_t o[4];
  TEST_ASSERT_TRUE(toio_config_parse_ipv4("192.168.1.100", o));
  TEST_ASSERT_EQUAL_UINT8(192, o[0]);
  TEST_ASSERT_EQUAL_UINT8(168, o[1]);
  TEST_ASSERT_EQUAL_UINT8(1, o[2]);
  TEST_ASSERT_EQUAL_UINT8(100, o[3]);
  TEST_ASSERT_TRUE(toio_config_parse_ipv4("0.0.0.0", o));
  TEST_ASSERT_TRUE(toio_config_parse_ipv4("255.255.255.255", o));
  TEST_ASSERT_TRUE(toio_config_parse_ipv4("10.0.0.1", o));
}

void test_ipv4_invalid_addresses(void) {
  uint8_t o[4];
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168.1", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168.1.100.1", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168.1.256", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168..1", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168.1.100 ", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("192.168.01.1", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("abc", o));
  TEST_ASSERT_FALSE(toio_config_parse_ipv4("1.2.3.", o));
}

// --- ポート ---

void test_port_range(void) {
  uint16_t p = 0;
  TEST_ASSERT_TRUE(toio_config_parse_port("8888", &p));
  TEST_ASSERT_EQUAL_UINT16(8888, p);
  TEST_ASSERT_TRUE(toio_config_parse_port("1", &p));
  TEST_ASSERT_TRUE(toio_config_parse_port("65535", &p));
  TEST_ASSERT_EQUAL_UINT16(65535, p);
  TEST_ASSERT_FALSE(toio_config_parse_port("0", &p));
  TEST_ASSERT_FALSE(toio_config_parse_port("65536", &p));
  TEST_ASSERT_FALSE(toio_config_parse_port("", &p));
  TEST_ASSERT_FALSE(toio_config_parse_port("-1", &p));
  TEST_ASSERT_FALSE(toio_config_parse_port("80a", &p));
  TEST_ASSERT_FALSE(toio_config_parse_port("99999999999", &p));
}

// --- namespace ---

void test_namespace_valid(void) {
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_namespace(""));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_namespace("robot1"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_namespace("/robot1"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_namespace("lab/robot_1"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK, toio_config_validate_namespace("_x"));
}

void test_namespace_invalid(void) {
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("/"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("robot1/"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("a//b"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("1robot"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("a/1b"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("robot-1"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("robot 1"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID, toio_config_validate_namespace("ロボ"));
}

// --- まとめて検証 ---

void test_validate_all_reports_first_error(void) {
  TEST_ASSERT_EQUAL(TOIO_CONFIG_OK,
                    toio_config_validate("home", "secret", "192.168.1.100", "8888", ""));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_SSID_EMPTY,
                    toio_config_validate("", "secret", "bad", "0", "1x"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_IP_INVALID,
                    toio_config_validate("home", "", "bad", "0", "1x"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_PORT_INVALID,
                    toio_config_validate("home", "", "10.0.0.1", "0", "1x"));
  TEST_ASSERT_EQUAL(TOIO_CONFIG_ERR_NAMESPACE_INVALID,
                    toio_config_validate("home", "", "10.0.0.1", "8888", "1x"));
}

void test_error_messages_are_non_empty_for_errors(void) {
  TEST_ASSERT_EQUAL_STRING("", toio_config_error_message(TOIO_CONFIG_OK));
  TEST_ASSERT_TRUE(strlen(toio_config_error_message(TOIO_CONFIG_ERR_IP_INVALID)) > 0);
  TEST_ASSERT_TRUE(strlen(toio_config_error_message(TOIO_CONFIG_ERR_NAMESPACE_INVALID)) > 0);
}

// --- パスワードの維持 ---

void test_empty_password_with_same_ssid_keeps_saved_password(void) {
  TEST_ASSERT_EQUAL_STRING("old-secret",
                           toio_config_resolve_password("home", "", "home", "old-secret"));
}

void test_empty_password_with_different_ssid_becomes_empty(void) {
  TEST_ASSERT_EQUAL_STRING("", toio_config_resolve_password("other", "", "home", "old-secret"));
}

void test_new_password_overrides_saved_password(void) {
  TEST_ASSERT_EQUAL_STRING("new", toio_config_resolve_password("home", "new", "home", "old"));
}

void test_empty_password_without_saved_settings_stays_empty(void) {
  TEST_ASSERT_EQUAL_STRING("", toio_config_resolve_password("home", "", "", ""));
}

// --- AP 名 ---

void test_ap_name_uses_last_two_mac_bytes(void) {
  const uint8_t mac[6] = {0x24, 0x6f, 0x28, 0xab, 0xcd, 0x0f};
  char name[32];
  toio_config_ap_name(mac, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("AtomToio-CD0F", name);
}

void test_ap_name_truncates_to_buffer(void) {
  const uint8_t mac[6] = {0, 0, 0, 0, 0x12, 0x34};
  char name[6];
  toio_config_ap_name(mac, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("AtomT", name);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_ssid_must_not_be_empty);
  RUN_TEST(test_ssid_length_limit_is_32);
  RUN_TEST(test_password_may_be_empty_but_not_over_63);
  RUN_TEST(test_ipv4_valid_addresses);
  RUN_TEST(test_ipv4_invalid_addresses);
  RUN_TEST(test_port_range);
  RUN_TEST(test_namespace_valid);
  RUN_TEST(test_namespace_invalid);
  RUN_TEST(test_validate_all_reports_first_error);
  RUN_TEST(test_error_messages_are_non_empty_for_errors);
  RUN_TEST(test_empty_password_with_same_ssid_keeps_saved_password);
  RUN_TEST(test_empty_password_with_different_ssid_becomes_empty);
  RUN_TEST(test_new_password_overrides_saved_password);
  RUN_TEST(test_empty_password_without_saved_settings_stays_empty);
  RUN_TEST(test_ap_name_uses_last_two_mac_bytes);
  RUN_TEST(test_ap_name_truncates_to_buffer);
  return UNITY_END();
}
