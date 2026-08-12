// ハードウェア非依存の純粋ロジック(native 環境の単体テスト対象)
#pragma once

#include <math.h>
#include <stdint.h>

// VL53L0X の測定レンジ [m](Range の min/max_range、LaserScan の range_min/max 共通)
#define TOIO_RANGE_MIN_M 0.03f
#define TOIO_RANGE_MAX_M 2.0f

// VL53L0X の out-of-range 値(8190/8191)以上は無効測定とみなす
inline bool toio_range_is_invalid(uint16_t mm) {
  return mm >= 8190;
}

// 測定値[mm] → sensor_msgs/Range の range[m]。無効時は +Inf(検出なし)
inline float toio_range_to_meters(uint16_t mm, bool valid) {
  return valid ? mm / 1000.0f : INFINITY;
}

// ミリ秒時刻 → ROS の header.stamp(sec / nanosec)
inline void toio_range_stamp_from_ms(int64_t ms, int32_t *sec, uint32_t *nanosec) {
  *sec = (int32_t)(ms / 1000);
  *nanosec = (uint32_t)((ms % 1000) * 1000000LL);
}

// scan モードの sensor_msgs/LaserScan 固定フィールド(micro-ROS 非依存)
typedef struct {
  float angle_min;
  float angle_max;
  float angle_increment;
  float time_increment;
  float scan_time;
  float range_min;
  float range_max;
} toio_scan_params_t;

// 単一点センサを正面方向 1 ビームの LaserScan として表現するパラメータ
inline toio_scan_params_t toio_scan_params(uint32_t publish_period_ms) {
  toio_scan_params_t p;
  p.angle_min = 0.0f;
  p.angle_max = 0.0f;
  p.angle_increment = 0.0f;
  p.time_increment = 0.0f;
  p.scan_time = publish_period_ms / 1000.0f;
  p.range_min = TOIO_RANGE_MIN_M;
  p.range_max = TOIO_RANGE_MAX_M;
  return p;
}
