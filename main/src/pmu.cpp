#include "printsphere/pmu.hpp"

#if defined(PRINTSPHERE_HW_VARIANT_AMOLED_1_75)

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

#include "bsp/esp32_s3_touch_amoled_1_75.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace printsphere {

namespace {

constexpr char kTag[] = "printsphere.pmu";
constexpr uint32_t kI2cTimeoutMs = 1000;
constexpr int64_t kPowerMenuHoldUs = 2000000;
constexpr uint32_t kPowerKeyPollMs = 80;

XPowersPMU s_pmu;
i2c_master_dev_handle_t s_pmu_device = nullptr;
esp_io_expander_handle_t s_power_key_expander = nullptr;
bool s_power_key_input_ready = false;

int pmu_register_read(uint8_t, uint8_t reg_addr, uint8_t* data, uint8_t len) {
  if (s_pmu_device == nullptr) {
    return -1;
  }

  const esp_err_t err =
      i2c_master_transmit_receive(s_pmu_device, &reg_addr, 1, data, len, kI2cTimeoutMs);
  return err == ESP_OK ? 0 : -1;
}

int pmu_register_write_byte(uint8_t, uint8_t reg_addr, uint8_t* data, uint8_t len) {
  if (s_pmu_device == nullptr) {
    return -1;
  }

  uint8_t buffer[17] = {};
  if (len + 1 > sizeof(buffer)) {
    return -1;
  }

  buffer[0] = reg_addr;
  for (uint8_t index = 0; index < len; ++index) {
    buffer[index + 1] = data[index];
  }

  const esp_err_t err = i2c_master_transmit(s_pmu_device, buffer, len + 1, kI2cTimeoutMs);
  return err == ESP_OK ? 0 : -1;
}

}  // namespace

esp_err_t PmuManager::initialize() {
  if (initialized_) {
    return ESP_OK;
  }

  ESP_RETURN_ON_ERROR(bsp_i2c_init(), kTag, "bsp_i2c_init failed");

  i2c_master_bus_handle_t bus_handle = bsp_i2c_get_handle();
  if (bus_handle == nullptr) {
    return ESP_FAIL;
  }

  if (s_pmu_device == nullptr) {
    i2c_device_config_t device_config = {};
    device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device_config.device_address = AXP2101_SLAVE_ADDRESS;
    device_config.scl_speed_hz = 400000;
    device_config.scl_wait_us = 0;
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus_handle, &device_config, &s_pmu_device), kTag,
                        "i2c_master_bus_add_device failed");
  }

  if (!s_pmu.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte)) {
    ESP_LOGE(kTag, "PMU begin failed");
    return ESP_FAIL;
  }

  s_pmu.enableVbusVoltageMeasure();
  s_pmu.enableBattVoltageMeasure();
  s_pmu.enableSystemVoltageMeasure();
  s_pmu.enableTemperatureMeasure();
  s_pmu.disableTSPinMeasure();

  s_pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  s_pmu.clearIrqStatus();
  // The PMU also reports a PWR-key hold. Use it as a second input path.
  s_pmu.setIrqLevelTime(XPOWERS_AXP2101_IRQ_TIME_2S);
  if (!s_pmu.enableIRQ(XPOWERS_AXP2101_PKEY_LONG_IRQ)) {
    ESP_LOGW(kTag, "Could not enable AXP2101 long-press fallback");
  }
  s_pmu.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
  s_pmu.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_400MA);
  s_pmu.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
  s_pmu.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  // Keep the hardware emergency shutdown available on an 8-second PWR hold.
  if (!s_pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_8S)) {
    ESP_LOGW(kTag, "Could not configure 8-second hardware power-off");
  }

  initialized_ = true;
  s_power_key_expander = bsp_io_expander_init();
  s_power_key_input_ready = s_power_key_expander != nullptr &&
      esp_io_expander_set_dir(s_power_key_expander, IO_EXPANDER_PIN_NUM_4,
                              IO_EXPANDER_INPUT) == ESP_OK;
  ESP_LOGI(kTag, "PWR monitor v2: EXIO4=%s, AXP2101 long-key IRQ enabled",
           s_power_key_input_ready ? "ready" : "unavailable");
  // Run even if EXIO4 is unavailable; the independent PMU IRQ can still work.
  if (xTaskCreate(&PmuManager::power_key_poll_task, "sykick_pwr_key", 4096, this,
                  tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
    ESP_LOGE(kTag, "PWR key monitor task could not start");
  }
  ESP_LOGI(kTag, "PMU ready (AXP2101)");
  return ESP_OK;
}

void PmuManager::power_key_poll_task(void* context) {
  auto* manager = static_cast<PmuManager*>(context);
  bool first_release_seen = false;
  bool last_pressed = false;
  bool hold_reported = false;
  int64_t pressed_since_us = 0;
  int64_t last_good_sample_us = esp_timer_get_time();
  int64_t last_error_log_us = 0;
  uint32_t loop_counter = 0;
  for (;;) {
    const int64_t now_us = esp_timer_get_time();
    if (s_power_key_input_ready) {
      uint32_t levels = 0;
      const esp_err_t result = esp_io_expander_get_level(
          s_power_key_expander, IO_EXPANDER_PIN_NUM_4, &levels);
      if (result != ESP_OK) {
        // Brief bus delays are common under touch/audio load. Allow a gap
        // shorter than 400ms instead of resetting a valid hold every time.
        if (now_us - last_good_sample_us > 400000) {
          pressed_since_us = 0;
        }
        if (now_us - last_error_log_us > 5000000) {
          ESP_LOGW(kTag, "PWR EXIO4 read failed: %s", esp_err_to_name(result));
          last_error_log_us = now_us;
        }
      } else {
        last_good_sample_us = now_us;
        const bool pressed = (levels & IO_EXPANDER_PIN_NUM_4) != 0;
        if (pressed != last_pressed) {
          ESP_LOGI(kTag, "PWR EXIO4 state: %s (raw=0x%02lx)",
                   pressed ? "PRESSED" : "RELEASED", static_cast<unsigned long>(levels));
          last_pressed = pressed;
        }
        if (!pressed) {
          first_release_seen = true;
          pressed_since_us = 0;
          hold_reported = false;
        } else if (first_release_seen && !hold_reported) {
          if (pressed_since_us == 0) {
            pressed_since_us = now_us;
          } else if (now_us - pressed_since_us >= kPowerMenuHoldUs) {
            hold_reported = true;
            manager->power_menu_requested_.store(true, std::memory_order_relaxed);
            ESP_LOGI(kTag, "PWR held 2 seconds on EXIO4; opening menu");
          }
        }
      }
    }

    // AXP2101 latches a long-key event at 2 seconds. Read only INTSTS2
    // and require a successful I2C transfer, avoiding false presses on errors.
    if ((++loop_counter & 1U) == 0U) {
      uint8_t irq_reg = XPOWERS_AXP2101_INTSTS2;
      uint8_t irq_bits = 0;
      const esp_err_t irq_err = i2c_master_transmit_receive(
          s_pmu_device, &irq_reg, 1, &irq_bits, 1, kI2cTimeoutMs);
      constexpr uint8_t kLongKeyBit = static_cast<uint8_t>(
          XPOWERS_AXP2101_PKEY_LONG_IRQ >> 8);
      if (irq_err == ESP_OK && (irq_bits & kLongKeyBit) != 0) {
        const uint8_t clear_irq[] = {XPOWERS_AXP2101_INTSTS2, kLongKeyBit};
        const esp_err_t clear_err = i2c_master_transmit(
            s_pmu_device, clear_irq, sizeof(clear_irq), kI2cTimeoutMs);
        if (clear_err == ESP_OK) {
          manager->power_menu_requested_.store(true, std::memory_order_relaxed);
          ESP_LOGI(kTag, "PWR held 2 seconds (AXP2101 IRQ); opening menu");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(kPowerKeyPollMs));
  }
}

esp_err_t PmuManager::power_off() {
  if (!initialized_) return ESP_ERR_INVALID_STATE;
  ESP_LOGI(kTag, "Requesting AXP2101 shutdown");
  s_pmu.shutdown();
  // If power remains (for example, USB still supplies power), report it.
  vTaskDelay(pdMS_TO_TICKS(500));
  return ESP_FAIL;
}

PowerSnapshot PmuManager::sample() const {
  PowerSnapshot snapshot;
  if (!initialized_) {
    return snapshot;
  }

  snapshot.available = true;
  snapshot.battery_present = s_pmu.isBatteryConnect();
  snapshot.usb_present = s_pmu.isVbusIn() || s_pmu.isVbusGood();
  snapshot.charging = s_pmu.isCharging();
  snapshot.temperature_c = s_pmu.getTemperature();

  if (snapshot.battery_present) {
    snapshot.battery_percent = static_cast<uint8_t>(s_pmu.getBatteryPercent());
  }

  return snapshot;
}

}  // namespace printsphere

#elif defined(PRINTSPHERE_HW_VARIANT_LCD_2_8C)

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

namespace printsphere {

namespace {

constexpr char kTag[] = "printsphere.pmu";
constexpr adc_unit_t kBatteryAdcUnit = ADC_UNIT_1;
constexpr adc_channel_t kBatteryAdcChannel = ADC_CHANNEL_3;
constexpr adc_atten_t kBatteryAdcAtten = ADC_ATTEN_DB_12;
constexpr float kBatteryDividerScale = 3.0f;
constexpr float kBatteryVoltageOffset = 0.980952f;
constexpr float kBatteryPresentMinVoltage = 3.20f;
constexpr float kBatteryPresentMaxVoltage = 4.18f;
constexpr float kUsbOnlyArtifactVoltage = 4.24f;
constexpr uint32_t kDiagLogIntervalMs = 5000U;
constexpr uint32_t kUsbArtifactConfirmSamples = 1U;
constexpr uint32_t kBatteryPresentConfirmSamples = 12U;
constexpr uint32_t kNoBatteryConfirmSamples = 6U;

enum PowerSenseState {
  kSenseUnknown = 0,
  kSenseUsbArtifact = 1,
  kSenseBatteryPresent = 2,
  kSenseNoBattery = 3,
};

adc_oneshot_unit_handle_t s_adc_handle = nullptr;
adc_cali_handle_t s_adc_cali_handle = nullptr;
bool s_adc_cali_enabled = false;
uint32_t s_last_diag_log_ms = 0;
int s_last_diag_state = -1;
PowerSenseState s_stable_state = kSenseUnknown;
uint32_t s_usb_artifact_count = 0;
uint32_t s_battery_present_count = 0;
uint32_t s_no_battery_count = 0;

uint8_t battery_percent_from_voltage(float voltage) {
  if (voltage <= 3.30f) {
    return 0;
  }
  if (voltage >= 4.20f) {
    return 100;
  }
  const float normalized = (voltage - 3.30f) / (4.20f - 3.30f);
  return static_cast<uint8_t>(normalized * 100.0f + 0.5f);
}

const char* power_sense_state_name(int state) {
  switch (state) {
    case kSenseUsbArtifact:
      return "usb-artifact";
    case kSenseBatteryPresent:
      return "battery";
    case kSenseNoBattery:
      return "no-battery";
    case kSenseUnknown:
    default:
      return "unknown";
  }
}

void maybe_log_power_diag(int raw, int millivolts, float battery_voltage, int state, int inst_state,
                          const PowerSnapshot& snapshot) {
  const uint32_t now_ms = esp_log_timestamp();
  if (state != s_last_diag_state || (now_ms - s_last_diag_log_ms) >= kDiagLogIntervalMs) {
    ESP_LOGI(kTag,
             "PMU diag: raw=%d adc_mv=%d batt_v=%.3f state=%s inst=%s battery_present=%d usb_present=%d charging=%d pct=%u",
             raw, millivolts, battery_voltage, power_sense_state_name(state),
             power_sense_state_name(inst_state),
             snapshot.battery_present ? 1 : 0, snapshot.usb_present ? 1 : 0,
             snapshot.charging ? 1 : 0, snapshot.battery_percent);
    s_last_diag_log_ms = now_ms;
    s_last_diag_state = state;
  }
}

PowerSenseState instantaneous_power_state(float battery_voltage) {
  if (battery_voltage >= kUsbOnlyArtifactVoltage) {
    return kSenseUsbArtifact;
  }
  if (battery_voltage >= kBatteryPresentMinVoltage &&
      battery_voltage <= kBatteryPresentMaxVoltage) {
    return kSenseBatteryPresent;
  }
  return kSenseNoBattery;
}

PowerSenseState resolve_stable_power_state(PowerSenseState inst_state) {
  if (inst_state == kSenseUsbArtifact) {
    s_usb_artifact_count++;
    s_battery_present_count = 0;
    s_no_battery_count = 0;
    if (s_usb_artifact_count >= kUsbArtifactConfirmSamples) {
      s_stable_state = kSenseUsbArtifact;
    }
    return s_stable_state;
  }

  if (inst_state == kSenseBatteryPresent) {
    s_battery_present_count++;
    s_usb_artifact_count = 0;
    s_no_battery_count = 0;
    if (s_battery_present_count >= kBatteryPresentConfirmSamples) {
      s_stable_state = kSenseBatteryPresent;
    }
    return s_stable_state;
  }

  s_no_battery_count++;
  s_usb_artifact_count = 0;
  s_battery_present_count = 0;
  if (s_no_battery_count >= kNoBatteryConfirmSamples) {
    s_stable_state = kSenseNoBattery;
  }
  return s_stable_state;
}

}  // namespace

esp_err_t PmuManager::initialize() {
  if (initialized_) {
    return ESP_OK;
  }

  const adc_oneshot_unit_init_cfg_t unit_config = {
      .unit_id = kBatteryAdcUnit,
      .clk_src = static_cast<adc_oneshot_clk_src_t>(0),
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  if (adc_oneshot_new_unit(&unit_config, &s_adc_handle) != ESP_OK) {
    ESP_LOGW(kTag, "ADC unit init failed; battery telemetry disabled");
    return ESP_OK;
  }

  const adc_oneshot_chan_cfg_t channel_config = {
      .atten = kBatteryAdcAtten,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  if (adc_oneshot_config_channel(s_adc_handle, kBatteryAdcChannel, &channel_config) != ESP_OK) {
    ESP_LOGW(kTag, "ADC channel config failed; battery telemetry disabled");
    return ESP_OK;
  }

  const adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = kBatteryAdcUnit,
      .chan = kBatteryAdcChannel,
      .atten = kBatteryAdcAtten,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };
  if (adc_cali_create_scheme_curve_fitting(&cali_config, &s_adc_cali_handle) == ESP_OK) {
    s_adc_cali_enabled = true;
  }

  initialized_ = true;
  ESP_LOGI(kTag, "Battery ADC ready");
  return ESP_OK;
}

PowerSnapshot PmuManager::sample() const {
  PowerSnapshot snapshot;
  if (!initialized_ || s_adc_handle == nullptr) {
    return snapshot;
  }

  int raw = 0;
  if (adc_oneshot_read(s_adc_handle, kBatteryAdcChannel, &raw) != ESP_OK) {
    return snapshot;
  }

  int millivolts = 0;
  if (s_adc_cali_enabled) {
    if (adc_cali_raw_to_voltage(s_adc_cali_handle, raw, &millivolts) != ESP_OK) {
      return snapshot;
    }
  } else {
    millivolts = raw;
  }

  const float battery_voltage =
      ((static_cast<float>(millivolts) * kBatteryDividerScale) / 1000.0f) / kBatteryVoltageOffset;
  const PowerSenseState inst_state = instantaneous_power_state(battery_voltage);
  const PowerSenseState stable_state = resolve_stable_power_state(inst_state);

  snapshot.available = true;
  snapshot.battery_present = stable_state == kSenseBatteryPresent;
  snapshot.usb_present = stable_state == kSenseUsbArtifact;
  snapshot.charging = false;
  snapshot.temperature_c = 0.0f;
  snapshot.battery_percent =
      snapshot.battery_present ? battery_percent_from_voltage(battery_voltage) : 0;
  maybe_log_power_diag(raw, millivolts, battery_voltage, stable_state, inst_state, snapshot);

  return snapshot;
}

esp_err_t PmuManager::power_off() {
  return ESP_ERR_NOT_SUPPORTED;
}

}  // namespace printsphere

#else
#error "Unknown PrintSphere hardware variant"
#endif
