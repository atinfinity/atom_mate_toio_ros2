// 実行時設定(NVS に保存)の読み書き
#pragma once

#include <stdint.h>

#include "toio_config_logic.h"

struct Settings {
  char ssid[TOIO_CONFIG_SSID_MAX + 1];
  char password[TOIO_CONFIG_PASSWORD_MAX + 1];
  char agent_ip[16];  // "255.255.255.255" + NUL
  uint16_t agent_port;
  char ros_namespace[TOIO_CONFIG_NAMESPACE_MAX + 1];
};

// NVS から読み込む。保存済みの設定が無ければ false(既定値を入れて返す)
bool settings_load(Settings *s);

// NVS に保存する
void settings_save(const Settings *s);
