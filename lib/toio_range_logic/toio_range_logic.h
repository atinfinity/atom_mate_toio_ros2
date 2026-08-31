// ハードウェア非依存の純粋ロジック(native 環境の単体テスト対象)
#pragma once

#include <math.h>
#include <stdint.h>

// VL53L0X の測定レンジ [m](Range の min/max_range、LaserScan の range_min/max 共通)
#define TOIO_RANGE_MIN_M 0.03f
#define TOIO_RANGE_MAX_M 2.0f

// VL53L0X の視野角(FoV)[rad] 約 25°(Range の field_of_view と共通)
#define TOIO_FOV_RAD 0.44f
// LaserScan のビーム数。単一点センサだが、FoV を等分した複数ビームとして
// 表現することで angle_increment を非ゼロにし、規約に沿った有効な
// LaserScan にする(2 未満だと angle_increment が定義できない)。
#define TOIO_SCAN_NUM_READINGS 7

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
  uint32_t num_readings;  // ranges 配列の要素数(= ビーム数)
} toio_scan_params_t;

// 単一点 ToF センサを、FoV を等分した複数ビームの LaserScan として表現する
// パラメータ。全ビームに同じ測定距離を入れることで、センサ正面の FoV 内を
// 一様に占有する障害物として近似する。angle_increment を非ゼロにするのが要点で、
// これにより RViz や laser_geometry・SLAM 等が規約どおりに扱える有効なスキャンになる
// (angle_increment=0 かつ angle_min==angle_max の退化スキャンは描画・処理されない)。
inline toio_scan_params_t toio_scan_params(uint32_t publish_period_ms) {
  toio_scan_params_t p;
  p.num_readings = TOIO_SCAN_NUM_READINGS;
  p.angle_min = -TOIO_FOV_RAD / 2.0f;
  p.angle_max = TOIO_FOV_RAD / 2.0f;
  // num_readings >= 2 前提。ビーム間の角度 = FoV /(ビーム数 - 1)。
  p.angle_increment = TOIO_FOV_RAD / (float)(TOIO_SCAN_NUM_READINGS - 1);
  p.time_increment = 0.0f;
  p.scan_time = publish_period_ms / 1000.0f;
  p.range_min = TOIO_RANGE_MIN_M;
  p.range_max = TOIO_RANGE_MAX_M;
  return p;
}
