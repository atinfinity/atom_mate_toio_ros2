// ハードウェア非依存の純粋ロジック(native 環境の単体テスト対象)
#pragma once

#include <math.h>
#include <stdint.h>

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
