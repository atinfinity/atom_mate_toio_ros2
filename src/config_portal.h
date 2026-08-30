// 設定モード: SoftAP を立てて設定ページ(captive portal)を提供する
#pragma once

#include "settings.h"

// AP を起動し、設定ページで保存されるまで戻らない(保存後に再起動する)。
// led_tick は待機中に定期的に呼ばれる(LED アニメーション用)。
void config_portal_run(const Settings *current, void (*led_tick)());
