#include "config_portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "app_config.h"

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);

static WebServer server(80);
static DNSServer dns;
static Settings pending;
static bool saved = false;

static const char PAGE_HEAD[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ja"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ATOM Mate for toio 設定</title>
<style>
body{font-family:-apple-system,"Hiragino Sans","Noto Sans JP",sans-serif;max-width:480px;margin:1.5rem auto;padding:0 1rem;line-height:1.6;color:#222}
h1{font-size:1.3rem}label{display:block;margin-top:1rem;font-weight:600}
input{width:100%;box-sizing:border-box;font-size:1rem;padding:.5rem;border:1px solid #bbb;border-radius:6px}
small{color:#666}button{margin-top:1.5rem;width:100%;font-size:1.05rem;padding:.7rem;background:#1976d2;color:#fff;border:0;border-radius:6px}
.err{background:#fdecea;border:1px solid #f5c2c0;color:#b3261e;padding:.6rem .8rem;border-radius:6px}
.ver{color:#888;font-size:.8rem}
</style></head><body>
<h1>ATOM Mate for toio 設定</h1>
<p class="ver">ファームウェア: )HTML";

static const char PAGE_FORM[] PROGMEM = R"HTML(
<form method="POST" action="/save">
<label>Wi-Fi SSID<input name="ssid" maxlength="32" required value="%SSID%"><small>2.4GHz 帯のみ対応</small></label>
<label>Wi-Fi パスワード<input name="pass" type="password" maxlength="63" placeholder="(再入力してください)"></label>
<label>micro-ros-agent の IP アドレス<input name="ip" inputmode="decimal" placeholder="192.168.1.100" required value="%IP%"><small>agent を動かす PC 自身の LAN 上の IP</small></label>
<label>micro-ros-agent のポート<input name="port" inputmode="numeric" required value="%PORT%"></label>
<label>ROS 2 namespace(任意)<input name="ns" maxlength="63" placeholder="空欄でよい。複数台なら robot1 など" value="%NS%"></label>
<button type="submit">保存して再起動</button>
</form></body></html>)HTML";

static const char PAGE_SAVED[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ja"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>保存しました</title>
<style>body{font-family:-apple-system,"Hiragino Sans","Noto Sans JP",sans-serif;max-width:480px;margin:1.5rem auto;padding:0 1rem;line-height:1.6}</style>
</head><body><h1>保存しました</h1>
<p>ATOM Matrix を再起動します。LED が 赤 → 黄 → 緑 と変われば接続完了です。</p>
<p>この Wi-Fi(AtomToio-XXXX)は消えるので、元の Wi-Fi に戻してください。</p></body></html>)HTML";

static String html_escape(const char *s) {
  String out;
  for (; *s; ++s) {
    switch (*s) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += *s;
    }
  }
  return out;
}

static void send_form(const Settings *s, const char *error) {
  String page = FPSTR(PAGE_HEAD);
  page += FW_VERSION;
  page += "</p>";
  if (error && *error) {
    page += "<p class=\"err\">";
    page += html_escape(error);
    page += "</p>";
  }
  String form = FPSTR(PAGE_FORM);
  form.replace("%SSID%", html_escape(s->ssid));
  form.replace("%IP%", html_escape(s->agent_ip));
  form.replace("%PORT%", String(s->agent_port));
  form.replace("%NS%", html_escape(s->ros_namespace));
  page += form;
  server.send(200, "text/html; charset=utf-8", page);
}

static void handle_root() { send_form(&pending, nullptr); }

static void handle_save() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  String ip = server.arg("ip");
  String port = server.arg("port");
  String ns = server.arg("ns");
  ssid.trim();
  ip.trim();
  port.trim();
  ns.trim();

  // 入力値をそのまま画面に戻せるように先に控える(パスワードは戻さない)
  strncpy(pending.ssid, ssid.c_str(), sizeof(pending.ssid) - 1);
  strncpy(pending.agent_ip, ip.c_str(), sizeof(pending.agent_ip) - 1);
  strncpy(pending.ros_namespace, ns.c_str(), sizeof(pending.ros_namespace) - 1);

  toio_config_error_t e =
      toio_config_validate(ssid.c_str(), pass.c_str(), ip.c_str(), port.c_str(), ns.c_str());
  if (e != TOIO_CONFIG_OK) {
    send_form(&pending, toio_config_error_message(e));
    return;
  }
  uint16_t port_num = 0;
  toio_config_parse_port(port.c_str(), &port_num);

  strncpy(pending.password, pass.c_str(), sizeof(pending.password) - 1);
  pending.agent_port = port_num;
  settings_save(&pending);
  saved = true;
  server.send(200, "text/html; charset=utf-8", FPSTR(PAGE_SAVED));
}

// captive portal の検出 URL や未知のパスは設定ページへ誘導する
static void handle_not_found() {
  server.sendHeader("Location", String("http://") + AP_IP.toString() + "/", true);
  server.send(302, "text/plain", "");
}

void config_portal_run(const Settings *current, void (*led_tick)()) {
  pending = *current;
  pending.password[0] = '\0';
  saved = false;

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ap_name[32];
  toio_config_ap_name(mac, ap_name, sizeof(ap_name));

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  WiFi.softAP(ap_name);
  dns.start(53, "*", AP_IP);

  server.on("/", HTTP_GET, handle_root);
  server.on("/save", HTTP_POST, handle_save);
  server.onNotFound(handle_not_found);
  server.begin();

  Serial.printf("Config mode: connect to Wi-Fi \"%s\" and open http://%s/\n", ap_name,
                AP_IP.toString().c_str());

  unsigned long saved_at = 0;
  for (;;) {
    dns.processNextRequest();
    server.handleClient();
    if (led_tick) led_tick();
    if (saved) {
      if (saved_at == 0) {
        saved_at = millis();
      } else if (millis() - saved_at > 1500) {  // 応答を送り切ってから再起動
        Serial.println("Settings saved, restarting");
        ESP.restart();
      }
    }
    delay(5);
  }
}
