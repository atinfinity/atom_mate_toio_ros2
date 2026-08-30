// 実行時設定(Wi-Fi / micro-ros-agent / namespace)のバリデーション。
// ハードウェア非依存の純粋ロジック(native 環境の単体テスト対象)。
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// 各項目の最大長(NUL を含まない)
#define TOIO_CONFIG_SSID_MAX 32
#define TOIO_CONFIG_PASSWORD_MAX 63
#define TOIO_CONFIG_NAMESPACE_MAX 63

typedef enum {
  TOIO_CONFIG_OK = 0,
  TOIO_CONFIG_ERR_SSID_EMPTY,
  TOIO_CONFIG_ERR_SSID_TOO_LONG,
  TOIO_CONFIG_ERR_PASSWORD_TOO_LONG,
  TOIO_CONFIG_ERR_IP_INVALID,
  TOIO_CONFIG_ERR_PORT_INVALID,
  TOIO_CONFIG_ERR_NAMESPACE_INVALID,
} toio_config_error_t;

// Wi-Fi SSID: 1〜32 文字
inline toio_config_error_t toio_config_validate_ssid(const char *ssid) {
  size_t n = strlen(ssid);
  if (n == 0) return TOIO_CONFIG_ERR_SSID_EMPTY;
  if (n > TOIO_CONFIG_SSID_MAX) return TOIO_CONFIG_ERR_SSID_TOO_LONG;
  return TOIO_CONFIG_OK;
}

// Wi-Fi パスワード: 0(オープン)または 8〜63 文字は WPA の仕様だが、
// WEP や特殊な AP もあるため長さ上限のみチェックする
inline toio_config_error_t toio_config_validate_password(const char *password) {
  if (strlen(password) > TOIO_CONFIG_PASSWORD_MAX) return TOIO_CONFIG_ERR_PASSWORD_TOO_LONG;
  return TOIO_CONFIG_OK;
}

// IPv4 のドット区切り 10 進表記を解析する。成功時 true、octets に 4 バイトを格納
inline bool toio_config_parse_ipv4(const char *s, uint8_t octets[4]) {
  int part = 0;
  int value = -1;  // -1 = まだ数字を読んでいない
  for (const char *p = s;; ++p) {
    char c = *p;
    if (c >= '0' && c <= '9') {
      if (value == -1) value = 0;
      // "01" のような先頭ゼロは拒否(曖昧さを避ける)
      if (value == 0 && p > s && *(p - 1) == '0') return false;
      value = value * 10 + (c - '0');
      if (value > 255) return false;
    } else if (c == '.' || c == '\0') {
      if (value == -1) return false;  // 空のオクテット
      if (part >= 4) return false;
      octets[part++] = (uint8_t)value;
      value = -1;
      if (c == '\0') break;
    } else {
      return false;
    }
  }
  return part == 4;
}

inline toio_config_error_t toio_config_validate_ip(const char *ip) {
  uint8_t o[4];
  return toio_config_parse_ipv4(ip, o) ? TOIO_CONFIG_OK : TOIO_CONFIG_ERR_IP_INVALID;
}

// ポート番号文字列: 1〜65535 の 10 進整数。成功時 *port に格納
inline bool toio_config_parse_port(const char *s, uint16_t *port) {
  if (*s == '\0') return false;
  long v = 0;
  for (const char *p = s; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    v = v * 10 + (*p - '0');
    if (v > 65535) return false;
  }
  if (v < 1) return false;
  *port = (uint16_t)v;
  return true;
}

inline toio_config_error_t toio_config_validate_port(const char *s) {
  uint16_t p;
  return toio_config_parse_port(s, &p) ? TOIO_CONFIG_OK : TOIO_CONFIG_ERR_PORT_INVALID;
}

// ROS 2 の namespace。空(名前空間なし)を許可。それ以外は
// "/" 区切りのトークン列で、各トークンは [A-Za-z_][A-Za-z0-9_]* 。
// 先頭の "/" は任意(rcl 側で補われる)。末尾の "/" と "//" は不可。
inline toio_config_error_t toio_config_validate_namespace(const char *ns) {
  size_t n = strlen(ns);
  if (n == 0) return TOIO_CONFIG_OK;
  if (n > TOIO_CONFIG_NAMESPACE_MAX) return TOIO_CONFIG_ERR_NAMESPACE_INVALID;
  const char *p = ns;
  if (*p == '/') ++p;
  if (*p == '\0') return TOIO_CONFIG_ERR_NAMESPACE_INVALID;  // "/" のみ
  bool token_start = true;
  for (; *p; ++p) {
    char c = *p;
    bool alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    bool digit = (c >= '0' && c <= '9');
    if (c == '/') {
      if (token_start) return TOIO_CONFIG_ERR_NAMESPACE_INVALID;  // "//"
      token_start = true;
      continue;
    }
    if (token_start) {
      if (!alpha) return TOIO_CONFIG_ERR_NAMESPACE_INVALID;  // 数字始まり等
      token_start = false;
    } else if (!alpha && !digit) {
      return TOIO_CONFIG_ERR_NAMESPACE_INVALID;
    }
  }
  if (token_start) return TOIO_CONFIG_ERR_NAMESPACE_INVALID;  // 末尾 "/"
  return TOIO_CONFIG_OK;
}

// 全項目をまとめて検証し、最初に見つかったエラーを返す
inline toio_config_error_t toio_config_validate(const char *ssid, const char *password,
                                                const char *ip, const char *port,
                                                const char *ns) {
  toio_config_error_t e;
  if ((e = toio_config_validate_ssid(ssid)) != TOIO_CONFIG_OK) return e;
  if ((e = toio_config_validate_password(password)) != TOIO_CONFIG_OK) return e;
  if ((e = toio_config_validate_ip(ip)) != TOIO_CONFIG_OK) return e;
  if ((e = toio_config_validate_port(port)) != TOIO_CONFIG_OK) return e;
  if ((e = toio_config_validate_namespace(ns)) != TOIO_CONFIG_OK) return e;
  return TOIO_CONFIG_OK;
}

// エラーコード → 設定ページに表示する日本語メッセージ
inline const char *toio_config_error_message(toio_config_error_t e) {
  switch (e) {
    case TOIO_CONFIG_OK: return "";
    case TOIO_CONFIG_ERR_SSID_EMPTY: return "Wi-Fi SSID を入力してください";
    case TOIO_CONFIG_ERR_SSID_TOO_LONG: return "Wi-Fi SSID は 32 文字以内で入力してください";
    case TOIO_CONFIG_ERR_PASSWORD_TOO_LONG: return "Wi-Fi パスワードは 63 文字以内で入力してください";
    case TOIO_CONFIG_ERR_IP_INVALID: return "agent の IP アドレスは 192.168.1.100 のような形式で入力してください";
    case TOIO_CONFIG_ERR_PORT_INVALID: return "agent のポートは 1〜65535 の整数で入力してください";
    case TOIO_CONFIG_ERR_NAMESPACE_INVALID:
      return "namespace は英字か _ で始まり、英数字と _ と / だけを使ってください(例: robot1)";
  }
  return "";
}

// 設定モード時の AP 名: "AtomToio-XXXX"(XXXX = MAC アドレス下位 2 バイトの 16 進大文字)
inline void toio_config_ap_name(const uint8_t mac[6], char *out, size_t out_size) {
  static const char hex[] = "0123456789ABCDEF";
  const char prefix[] = "AtomToio-";
  size_t i = 0;
  for (const char *p = prefix; *p && i + 1 < out_size; ++p) out[i++] = *p;
  for (int b = 4; b < 6 && i + 2 < out_size; ++b) {
    out[i++] = hex[mac[b] >> 4];
    out[i++] = hex[mac[b] & 0x0f];
  }
  out[i] = '\0';
}
