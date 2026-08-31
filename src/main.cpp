// ATOM Matrix + ATOM Mate for toio (VL53L0X ToF センサ)
// 距離を sensor_msgs/Range として micro-ROS (Wi-Fi UDP) で publish する。
// Wi-Fi / agent の設定は NVS に保存され、未設定時または起動時にボタンを
// 押していると設定モード(SoftAP + 設定ページ)に入る。
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
#include <sensor_msgs/msg/laser_scan.h>
#include <sensor_msgs/msg/range.h>

#include "app_config.h"
#include "config_portal.h"
#include "settings.h"
#include "toio_range_logic.h"

#if PUBLISH_MODE != PUBLISH_MODE_RANGE && PUBLISH_MODE != PUBLISH_MODE_SCAN
#error "PUBLISH_MODE must be PUBLISH_MODE_RANGE or PUBLISH_MODE_SCAN"
#endif

// ATOM Mate for toio の I2C ピン(ATOM デフォルトの G26/G32 とは異なる)
static const int I2C_SDA_PIN = 25;
static const int I2C_SCL_PIN = 21;

// ATOM Matrix 内蔵 5x5 LED マトリクス
static const int LED_PIN = 27;
static const int NUM_LEDS = 25;
static CRGB leds[NUM_LEDS];

// ATOM Matrix 前面の押しボタン(押下で LOW)
static const int BUTTON_PIN = 39;

static Settings settings;

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
#if PUBLISH_MODE == PUBLISH_MODE_SCAN
static sensor_msgs__msg__LaserScan scan_msg;
static float scan_ranges[TOIO_SCAN_NUM_READINGS];
#else
static sensor_msgs__msg__Range range_msg;
#endif

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

// 設定モード: 青い光が 5x5 の外周を回る
static void config_mode_led_tick() {
  static const uint8_t ring[16] = {0, 1, 2, 3, 4, 9, 14, 19, 24, 23, 22, 21, 20, 15, 10, 5};
  static unsigned long last = 0;
  static uint8_t head = 0;
  if (millis() - last < 80) return;
  last = millis();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < 4; ++i) {
    uint8_t idx = ring[(head + 16 - i) % 16];
    leds[idx] = CRGB(0, 0, 255 >> i);  // 先頭が明るく、尾が暗い
  }
  FastLED.show();
  head = (head + 1) % 16;
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
#if PUBLISH_MODE == PUBLISH_MODE_SCAN
  fill_stamp(&scan_msg.header.stamp);
  // 検出なし(無効測定)は +Inf で表現。単一点センサのため全ビームに同一距離を設定する。
  const float scan_r = toio_range_to_meters(last_range_mm, range_valid);
  for (size_t i = 0; i < scan_msg.ranges.size; ++i) {
    scan_msg.ranges.data[i] = scan_r;
  }
  rcl_publish(&publisher, &scan_msg, NULL);
#else
  fill_stamp(&range_msg.header.stamp);
  // 検出なし(無効測定)は +Inf で表現
  range_msg.range = toio_range_to_meters(last_range_mm, range_valid);
  rcl_publish(&publisher, &range_msg, NULL);
#endif
}

static void set_frame_id(std_msgs__msg__Header *header) {
  static char frame_id_buf[] = FRAME_ID;
  header->frame_id.data = frame_id_buf;
  header->frame_id.size = strlen(frame_id_buf);
  header->frame_id.capacity = sizeof(frame_id_buf);
}

#if PUBLISH_MODE == PUBLISH_MODE_SCAN
static void init_msg() {
  sensor_msgs__msg__LaserScan__init(&scan_msg);
  set_frame_id(&scan_msg.header);
  // 単一点センサを FoV 分の複数ビーム LaserScan として配信する(angle_increment 非ゼロ)
  const toio_scan_params_t p = toio_scan_params(PUBLISH_PERIOD_MS);
  scan_msg.angle_min = p.angle_min;
  scan_msg.angle_max = p.angle_max;
  scan_msg.angle_increment = p.angle_increment;
  scan_msg.time_increment = p.time_increment;
  scan_msg.scan_time = p.scan_time;
  scan_msg.range_min = p.range_min;
  scan_msg.range_max = p.range_max;
  for (uint32_t i = 0; i < p.num_readings; ++i) {
    scan_ranges[i] = 0.0f;
  }
  scan_msg.ranges.data = scan_ranges;
  scan_msg.ranges.size = p.num_readings;
  scan_msg.ranges.capacity = p.num_readings;
}
#else
static void init_msg() {
  sensor_msgs__msg__Range__init(&range_msg);
  set_frame_id(&range_msg.header);
  range_msg.radiation_type = sensor_msgs__msg__Range__INFRARED;
  range_msg.field_of_view = TOIO_FOV_RAD;  // VL53L0X の FoV 約25°
  range_msg.min_range = TOIO_RANGE_MIN_M;
  range_msg.max_range = TOIO_RANGE_MAX_M;
  range_msg.range = 0.0f;
}
#endif

static bool create_entities() {
  allocator = rcl_get_default_allocator();
  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "atom_toio_range_node", settings.ros_namespace, &support));
#if PUBLISH_MODE == PUBLISH_MODE_SCAN
  RCCHECK(rclc_publisher_init_best_effort(
      &publisher, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, LaserScan),
      SCAN_TOPIC_NAME));
#else
  RCCHECK(rclc_publisher_init_best_effort(
      &publisher, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
      RANGE_TOPIC_NAME));
#endif
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
  Serial.println("atom_mate_toio_ros2 " FW_VERSION);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(20);

  // 設定が無い、または起動時にボタンが押されていれば設定モードへ(戻ってこない)
  pinMode(BUTTON_PIN, INPUT);
  bool configured = settings_load(&settings);
  bool button_held = digitalRead(BUTTON_PIN) == LOW;
  if (!configured || button_held) {
    Serial.println(configured ? "Button held at boot: entering config mode"
                              : "No settings found: entering config mode");
    config_portal_run(&settings, config_mode_led_tick);
  }

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
  Serial.printf("Connecting to Wi-Fi \"%s\" (agent %s:%u, namespace \"%s\")\n",
                settings.ssid, settings.agent_ip, settings.agent_port,
                settings.ros_namespace);
  IPAddress agent_ip;
  agent_ip.fromString(settings.agent_ip);
  set_microros_wifi_transports(settings.ssid, settings.password, agent_ip,
                               settings.agent_port);
  Serial.print("Wi-Fi connected: ");
  Serial.println(WiFi.localIP());

  init_msg();
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
