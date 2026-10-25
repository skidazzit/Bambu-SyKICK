#pragma once

#include <cstdint>
#include <array>
#include "esp_err.h"

namespace printsphere {

// Preserves the v13 face/menu option order. Stored separately from printer
// profiles so the UI port cannot overwrite existing Wi-Fi/printer settings.
enum class V13FaceMode : uint8_t { kRandomize = 0, kMood = 1, kSingle = 2 };
enum class V13Face : uint8_t {
  kIdle = 0, kSad, kHappy, kGlossy, kSleepy, kSurprised
};
enum class V13IdleFace : uint8_t {
  kIdle = 0, kSad, kHappy, kGlossy, kSleepy, kSurprised, kSadSleepy
};
enum class V13FaceColor : uint8_t {
  kCyan = 0, kBlue, kGreen, kOrange, kPurple, kRed, kWhite, kPink, kYellow
};

struct V13MenuPreferences {
  V13FaceMode face_mode = V13FaceMode::kRandomize;
  V13Face main_face = V13Face::kHappy;
  V13IdleFace idle_face = V13IdleFace::kSleepy;
  // v13 menu: 10s, 30s, 1m, 2m, 5m, 10m, Never.
  uint8_t idle_delay_choice = 2;
  V13FaceColor face_color = V13FaceColor::kCyan;
  bool match_heart_color = false;
  uint8_t brightness = 80;
  uint8_t idle_brightness = 20;
  uint8_t volume = 50;
  uint8_t language = 0;  // 0 English, 1 Spanish, 2 French
  uint8_t time_zone_choice = 1;
  bool clock_24_hour = false;
  bool show_clock_on_home = false;
  // Order of six Control Center links; Sort Menu and About stay last.
  std::array<uint8_t, 6> menu_order{{0, 1, 2, 3, 4, 5}};

  void clamp();
};

// The caller initializes NVS first. A missing namespace returns defaults.
esp_err_t load_v13_menu_preferences(V13MenuPreferences* prefs);
esp_err_t save_v13_menu_preferences(const V13MenuPreferences& prefs);

}  // namespace printsphere
