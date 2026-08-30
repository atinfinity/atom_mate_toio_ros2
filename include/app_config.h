// ビルド時に固定する設定。Wi-Fi / agent / namespace は実行時に設定モード
// (SoftAP + 設定ページ)で与えるため、ここには含まれない。
#pragma once

// 配信モード(platformio.ini の build_flags で選択。デフォルトは Range)
//   0 = PUBLISH_MODE_RANGE: sensor_msgs/Range を RANGE_TOPIC_NAME に配信
//   1 = PUBLISH_MODE_SCAN:  sensor_msgs/LaserScan(1ビーム)を SCAN_TOPIC_NAME に配信
#define PUBLISH_MODE_RANGE 0
#define PUBLISH_MODE_SCAN 1
#ifndef PUBLISH_MODE
#define PUBLISH_MODE PUBLISH_MODE_RANGE
#endif

#define RANGE_TOPIC_NAME "toio/range"
#define SCAN_TOPIC_NAME "toio/scan"
#define FRAME_ID "toio_range_link"

// publish 周期 [ms](100ms = 10Hz)
#define PUBLISH_PERIOD_MS 100

// ファームウェアのバージョン表示(CI がタグ名を -DFW_VERSION で埋め込む)
#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif
