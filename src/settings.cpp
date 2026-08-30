#include "settings.h"

#include <Preferences.h>
#include <string.h>

static const char *NVS_NAMESPACE = "toio";
static const uint8_t SETTINGS_FORMAT = 1;

static void copy_str(char *dst, size_t dst_size, const String &src) {
  strncpy(dst, src.c_str(), dst_size - 1);
  dst[dst_size - 1] = '\0';
}

bool settings_load(Settings *s) {
  memset(s, 0, sizeof(*s));
  strncpy(s->agent_ip, "192.168.1.100", sizeof(s->agent_ip) - 1);
  s->agent_port = 8888;

  Preferences prefs;
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    return false;
  }
  bool configured = prefs.getUChar("fmt", 0) == SETTINGS_FORMAT;
  if (configured) {
    copy_str(s->ssid, sizeof(s->ssid), prefs.getString("ssid", ""));
    copy_str(s->password, sizeof(s->password), prefs.getString("pass", ""));
    copy_str(s->agent_ip, sizeof(s->agent_ip), prefs.getString("ip", s->agent_ip));
    s->agent_port = prefs.getUShort("port", s->agent_port);
    copy_str(s->ros_namespace, sizeof(s->ros_namespace), prefs.getString("ns", ""));
  }
  prefs.end();
  return configured && s->ssid[0] != '\0';
}

void settings_save(const Settings *s) {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putString("ssid", s->ssid);
  prefs.putString("pass", s->password);
  prefs.putString("ip", s->agent_ip);
  prefs.putUShort("port", s->agent_port);
  prefs.putString("ns", s->ros_namespace);
  prefs.putUChar("fmt", SETTINGS_FORMAT);
  prefs.end();
}
