// ATOM Matrix + ATOM Mate for toio (VL53L0X ToF センサ)
// 距離を sensor_msgs/Range として micro-ROS (Wi-Fi UDP) で publish する。
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <FastLED.h>
#include <Adafruit_VL53L0X.h>

#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <sensor_msgs/msg/range.h>

#include "config.h"
#include "toio_range_logic.h"

// ATOM Mate for toio の I2C ピン(ATOM デフォルトの G26/G32 とは異なる)
static const int I2C_SDA_PIN = 25;
static const int I2C_SCL_PIN = 21;

// ATOM Matrix 内蔵 5x5 LED マトリクス
static const int LED_PIN = 27;
static const int NUM_LEDS = 25;
static CRGB leds[NUM_LEDS];

static Adafruit_VL53L0X lox;
static bool sensor_ok = false;
static uint16_t last_range_mm = 0;
static bool range_valid = false;

// micro-ROS エンティティ
static rcl_allocator_t allocator;
static rclc_support_t support;
static rcl_node_t node;
static rcl_publisher_t publisher;
static rcl_timer_t timer;
static rclc_executor_t executor;
static sensor_msgs__msg__Range range_msg;

enum AgentState {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED,
};
static AgentState agent_state = WAITING_AGENT;

#define RCCHECK(fn)                  \
  {                                  \
    rcl_ret_t rc = fn;               \
    if (rc != RCL_RET_OK) {          \
      return false;                  \
    }                                \
  }

#define EXECUTE_EVERY_N_MS(MS, X)          \
  do {                                     \
    static int64_t init = -1;              \
    if (init == -1) init = millis();       \
    if ((int64_t)millis() - init > (MS)) { \
      X;                                   \
      init = millis();                     \
    }                                      \
  } while (0)

static void set_status_led(const CRGB &color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

static void fill_stamp(builtin_interfaces__msg__Time *stamp) {
  int64_t ms = rmw_uros_epoch_synchronized() ? rmw_uros_epoch_millis()
                                             : (int64_t)millis();
  toio_range_stamp_from_ms(ms, &stamp->sec, &stamp->nanosec);
}

static void timer_callback(rcl_timer_t *timer_handle, int64_t last_call_time) {
  (void)last_call_time;
  if (timer_handle == NULL) {
    return;
  }
  fill_stamp(&range_msg.header.stamp);
  // 検出なし(無効測定)は +Inf で表現
  range_msg.range = toio_range_to_meters(last_range_mm, range_valid);
  rcl_publish(&publisher, &range_msg, NULL);
}

static void init_range_msg() {
  sensor_msgs__msg__Range__init(&range_msg);
  static char frame_id_buf[] = FRAME_ID;
  range_msg.header.frame_id.data = frame_id_buf;
  range_msg.header.frame_id.size = strlen(frame_id_buf);
  range_msg.header.frame_id.capacity = sizeof(frame_id_buf);
  range_msg.radiation_type = sensor_msgs__msg__Range__INFRARED;
  range_msg.field_of_view = 0.44f;  // VL53L0X の FoV 約25°
  range_msg.min_range = 0.03f;
  range_msg.max_range = 2.0f;
  range_msg.range = 0.0f;
}

static bool create_entities() {
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "atom_toio_range_node", ROS_NAMESPACE, &support));
  RCCHECK(rclc_publisher_init_best_effort(
      &publisher, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
      TOPIC_NAME));
  RCCHECK(rclc_timer_init_default2(
      &timer, &support, RCL_MS_TO_NS(PUBLISH_PERIOD_MS), timer_callback, true));
  executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  // header.stamp 用にエージェントと時刻同期
  rmw_uros_sync_session(1000);
  return true;
}

static void destroy_entities() {
  rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
  (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  rcl_publisher_fini(&publisher, &node);
  rcl_timer_fini(&timer);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

static void update_sensor() {
  if (!sensor_ok) {
    range_valid = false;
    return;
  }
  if (lox.isRangeComplete()) {
    uint16_t mm = lox.readRange();
    if (toio_range_is_invalid(mm)) {
      range_valid = false;
    } else {
      last_range_mm = mm;
      range_valid = true;
    }
  }
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
  set_status_led(CRGB::Red);  // 赤: Wi-Fi 接続中

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (lox.begin(VL53L0X_I2C_ADDR, false, &Wire)) {
    lox.startRangeContinuous(PUBLISH_PERIOD_MS);
    sensor_ok = true;
    Serial.println("VL53L0X initialized");
  } else {
    Serial.println("ERROR: VL53L0X not found");
  }

  // Wi-Fi 接続 + micro-ROS UDP トランスポート設定(接続完了までブロック)
  IPAddress agent_ip;
  agent_ip.fromString(AGENT_IP);
  set_microros_wifi_transports((char *)WIFI_SSID, (char *)WIFI_PASSWORD,
                               agent_ip, AGENT_PORT);
  Serial.print("Wi-Fi connected: ");
  Serial.println(WiFi.localIP());

  init_range_msg();
  set_status_led(CRGB::Yellow);  // 黄: agent 待ち
}

void loop() {
  update_sensor();

  switch (agent_state) {
    case WAITING_AGENT:
      EXECUTE_EVERY_N_MS(500, {
        if (RMW_RET_OK == rmw_uros_ping_agent(100, 1)) {
          agent_state = AGENT_AVAILABLE;
        }
      });
      break;

    case AGENT_AVAILABLE:
      if (create_entities()) {
        agent_state = AGENT_CONNECTED;
        set_status_led(CRGB::Green);  // 緑: publish 中
        Serial.println("micro-ROS agent connected");
      } else {
        destroy_entities();
        agent_state = WAITING_AGENT;
        set_status_led(CRGB::Yellow);
      }
      break;

    case AGENT_CONNECTED:
      EXECUTE_EVERY_N_MS(2000, {
        if (RMW_RET_OK != rmw_uros_ping_agent(100, 3)) {
          agent_state = AGENT_DISCONNECTED;
        }
      });
      if (agent_state == AGENT_CONNECTED) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
      }
      break;

    case AGENT_DISCONNECTED:
      destroy_entities();
      agent_state = WAITING_AGENT;
      set_status_led(CRGB::Yellow);
      Serial.println("micro-ROS agent disconnected");
      break;
  }
}
