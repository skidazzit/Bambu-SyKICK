#include "printsphere/v13_menu_preferences.hpp"

#include <algorithm>
#include <cstdio>
#include "nvs.h"

namespace printsphere {
namespace {
constexpr char kNamespace[] = "sykick_v13_ui";

uint8_t read_u8(nvs_handle_t handle, const char* key, uint8_t fallback) {
  uint8_t value = fallback;
  return nvs_get_u8(handle, key, &value) == ESP_OK ? value : fallback;
}

esp_err_t save_u8(nvs_handle_t handle, const char* key, uint8_t value) {
  return nvs_set_u8(handle, key, value);
}
}  // namespace

void V13MenuPreferences::clamp() {
  face_mode = static_cast<V13FaceMode>(
      std::min<uint8_t>(static_cast<uint8_t>(face_mode), 2));
  main_face = static_cast<V13Face>(
      std::min<uint8_t>(static_cast<uint8_t>(main_face), 5));
  idle_face = static_cast<V13IdleFace>(
      std::min<uint8_t>(static_cast<uint8_t>(idle_face), 6));
  idle_delay_choice = std::min<uint8_t>(idle_delay_choice, 6);
  face_color = static_cast<V13FaceColor>(
      std::min<uint8_t>(static_cast<uint8_t>(face_color), 8));
  brightness = std::clamp<uint8_t>(brightness, 10, 100);
  idle_brightness = std::min<uint8_t>(idle_brightness, 50);
  volume = std::min<uint8_t>(volume, 100);
  language = std::min<uint8_t>(language, 2);
  bool seen[6] = {};
  for (uint8_t value : menu_order) {
    if (value >= 6 || seen[value]) {
      menu_order = {{0, 1, 2, 3, 4, 5}};
      break;
    }
    seen[value] = true;
  }
}

esp_err_t load_v13_menu_preferences(V13MenuPreferences* prefs) {
  if (prefs == nullptr) return ESP_ERR_INVALID_ARG;
  *prefs = V13MenuPreferences{};
  nvs_handle_t handle = 0;
  const esp_err_t open_result = nvs_open(kNamespace, NVS_READONLY, &handle);
  if (open_result == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
  if (open_result != ESP_OK) return open_result;

  prefs->face_mode = static_cast<V13FaceMode>(
      read_u8(handle, "face_mode", static_cast<uint8_t>(prefs->face_mode)));
  prefs->main_face = static_cast<V13Face>(
      read_u8(handle, "main_face", static_cast<uint8_t>(prefs->main_face)));
  prefs->idle_face = static_cast<V13IdleFace>(
      read_u8(handle, "idle_face", static_cast<uint8_t>(prefs->idle_face)));
  prefs->idle_delay_choice = read_u8(handle, "face_delay", prefs->idle_delay_choice);
  prefs->face_color = static_cast<V13FaceColor>(
      read_u8(handle, "face_color", static_cast<uint8_t>(prefs->face_color)));
  prefs->match_heart_color = read_u8(handle, "heart_match", 0) != 0;
  prefs->brightness = read_u8(handle, "brightness", prefs->brightness);
  prefs->idle_brightness = read_u8(handle, "idle_dim", prefs->idle_brightness);
  prefs->volume = read_u8(handle, "volume", prefs->volume);
  prefs->language = read_u8(handle, "language", prefs->language);
  prefs->time_zone_choice = read_u8(handle, "time_zone", prefs->time_zone_choice);
  prefs->clock_24_hour = read_u8(handle, "clock_24h", 0) != 0;
  prefs->show_clock_on_home = read_u8(handle, "home_clock", 0) != 0;
  for (uint8_t i = 0; i < prefs->menu_order.size(); ++i) {
    char key[12] = {};
    std::snprintf(key, sizeof(key), "menu_ord_%u", static_cast<unsigned>(i));
    prefs->menu_order[i] = read_u8(handle, key, i);
  }
  nvs_close(handle);
  prefs->clamp();
  return ESP_OK;
}

esp_err_t save_v13_menu_preferences(const V13MenuPreferences& input) {
  V13MenuPreferences prefs = input;
  prefs.clamp();
  nvs_handle_t handle = 0;
  esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &handle);
  if (err != ESP_OK) return err;
  const struct Entry { const char* key; uint8_t value; } entries[] = {
      {"face_mode", static_cast<uint8_t>(prefs.face_mode)},
      {"main_face", static_cast<uint8_t>(prefs.main_face)},
      {"idle_face", static_cast<uint8_t>(prefs.idle_face)},
      {"face_delay", prefs.idle_delay_choice},
      {"face_color", static_cast<uint8_t>(prefs.face_color)},
      {"heart_match", static_cast<uint8_t>(prefs.match_heart_color)},
      {"brightness", prefs.brightness},
      {"idle_dim", prefs.idle_brightness},
      {"volume", prefs.volume},
      {"language", prefs.language},
      {"time_zone", prefs.time_zone_choice},
      {"clock_24h", static_cast<uint8_t>(prefs.clock_24_hour)},
      {"home_clock", static_cast<uint8_t>(prefs.show_clock_on_home)},
  };
  for (const auto& entry : entries) {
    err = save_u8(handle, entry.key, entry.value);
    if (err != ESP_OK) break;
  }
  if (err == ESP_OK) {
    for (uint8_t i = 0; i < prefs.menu_order.size(); ++i) {
      char key[12] = {};
      std::snprintf(key, sizeof(key), "menu_ord_%u", static_cast<unsigned>(i));
      err = save_u8(handle, key, prefs.menu_order[i]);
      if (err != ESP_OK) break;
    }
  }
  if (err == ESP_OK) err = nvs_commit(handle);
  nvs_close(handle);
  return err;
}

}  // namespace printsphere
