#pragma once

#include <atomic>
#include <cstdint>

#include "esp_err.h"

namespace printsphere {

struct PowerSnapshot {
  bool available = false;
  bool battery_present = false;
  uint8_t battery_percent = 0;
  bool charging = false;
  bool usb_present = false;
  float temperature_c = 0.0f;
};

class PmuManager {
 public:
  esp_err_t initialize();
  PowerSnapshot sample() const;
  bool is_ready() const { return initialized_; }
  bool has_power_menu_request() const {
    return power_menu_requested_.load(std::memory_order_relaxed);
  }
  bool consume_power_menu_request() {
    return power_menu_requested_.exchange(false, std::memory_order_relaxed);
  }
  esp_err_t power_off();

 private:
#if defined(PRINTSPHERE_HW_VARIANT_AMOLED_1_75)
  static void power_key_poll_task(void* context);
#endif
  std::atomic<bool> power_menu_requested_{false};
  bool initialized_ = false;
};

}  // namespace printsphere
