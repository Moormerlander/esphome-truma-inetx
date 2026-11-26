#pragma once

// Entity classes. Each is only compiled when its ESPHome platform is in use.

#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/core/version.h"
#include "truma_inetx.h"

#include <cmath>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_CLIMATE
#include "esphome/components/climate/climate.h"
#endif

namespace esphome {
namespace truma_inetx {

#ifdef USE_SENSOR
// Numeric value as-is (use ESPHome filters such as `multiply: 0.1` to scale).
// Arrays publish their length, e.g. the number of active error codes.
class TrumaSensor : public sensor::Sensor, public TrumaListener {
 public:
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    if (v.is_number()) {
      this->publish_state(static_cast<float>(v.as_double()));
    } else if (v.type == truma_inetx_proto::CborValue::ARRAY) {
      this->publish_state(static_cast<float>(v.items.size()));
    }
  }
};
#endif

#ifdef USE_BINARY_SENSOR
class TrumaBinarySensor : public binary_sensor::BinarySensor, public TrumaListener {
 public:
  void add_on_value(int32_t v) { this->on_values_.push_back(v); }
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    if (v.type == truma_inetx_proto::CborValue::ARRAY) {
      this->publish_state(!v.items.empty());
      return;
    }
    if (!v.is_number())
      return;
    const int64_t i = v.as_int();
    if (this->on_values_.empty()) {
      this->publish_state(i != 0);
      return;
    }
    bool on = false;
    for (auto x : this->on_values_)
      on |= x == i;
    this->publish_state(on);
  }
  void on_status() override {
    switch (this->status_type_) {
      case STATUS_CONNECTED:
        this->publish_state(this->parent_->is_connected());
        break;
      case STATUS_PAIRED:
        this->publish_state(this->parent_->is_paired());
        break;
      case STATUS_PAIRING_MODE:
        this->publish_state(this->parent_->is_pairing());
        break;
      default:
        break;
    }
  }

 protected:
  std::vector<int32_t> on_values_;
};
#endif

#ifdef USE_TEXT_SENSOR
enum TextFormat : uint8_t { TEXT_RAW = 0, TEXT_TIMER = 1, TEXT_FIRMWARE = 2 };

class TrumaTextSensor : public text_sensor::TextSensor, public TrumaListener {
 public:
  void set_format(uint8_t f) { this->format_ = f; }

  bool wants(uint16_t src, const std::string &topic, const std::string &param) const override {
    if (this->format_ == TEXT_FIRMWARE)
      return topic == "Identify" && (param == "SwMaj" || param == "SwMin" || param == "SwBgFx") &&
             (this->address_ == 0 || this->address_ == src);
    return TrumaListener::wants(src, topic, param);
  }

  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    std::string out;
    switch (this->format_) {
      case TEXT_TIMER:
        out = format_timer(v, this->parent_->german());
        break;
      case TEXT_FIRMWARE:
        out = this->format_firmware_(src);
        if (out.empty())
          return;
        break;
      default:
        out = v.type == truma_inetx_proto::CborValue::TEXT ? v.s : v.to_string();
        break;
    }
    if (out != this->state)
      this->publish_state(out);
  }
  void refresh() override {
    if (this->format_ == TEXT_FIRMWARE) {
      std::string out = this->format_firmware_(this->address_);
      if (!out.empty() && out != this->state)
        this->publish_state(out);
      return;
    }
    TrumaListener::refresh();
  }

  // TimerConfig.TimerN: {"id","name","symbol","start","end","wd":[7 x 0/1]}.
  // wd is taken as Monday..Sunday.
  static std::string format_timer(const truma_inetx_proto::CborValue &v, bool german) {
    using truma_inetx_proto::CborValue;
    if (v.type != CborValue::MAP)
      return v.to_string();
    const CborValue *id = v.get("id");
    const CborValue *name = v.get("name");
    const CborValue *start = v.get("start");
    const CborValue *end = v.get("end");
    const CborValue *wd = v.get("wd");
    const bool unused = (id == nullptr || (id->is_number() && id->as_int() == 0)) &&
                        (name == nullptr || (name->type == CborValue::TEXT && name->s == "DefaultTimer"));
    if (unused)
      return german ? "nicht eingerichtet" : "not set up";
    std::string out;
    if (name != nullptr && name->type == CborValue::TEXT && !name->s.empty())
      out = name->s + ": ";
    out += (start != nullptr && start->type == CborValue::TEXT) ? start->s : "?";
    out += "-";
    out += (end != nullptr && end->type == CborValue::TEXT) ? end->s : "?";
    if (wd != nullptr && wd->type == CborValue::ARRAY) {
      static const char *const DAYS_DE[7] = {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};
      static const char *const DAYS_EN[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
      const char *const *DAYS = german ? DAYS_DE : DAYS_EN;
      std::string days;
      int n = 0;
      for (size_t k = 0; k < wd->items.size() && k < 7; k++) {
        if (wd->items[k].is_number() && wd->items[k].as_int() != 0) {
          if (!days.empty())
            days += " ";
          days += DAYS[k];
          n++;
        }
      }
      out += n == 7 ? (german ? ", taeglich" : ", daily") : n == 0 ? (german ? ", keine Tage" : ", no days") : ", " + days;
    }
    return out;
  }

  void on_status() override {
    if (this->status_type_ == STATUS_TEXT && this->parent_->status_text() != this->state)
      this->publish_state(this->parent_->status_text());
    else if (this->status_type_ == STATUS_VERSION && this->state != TRUMA_INETX_VERSION)
      this->publish_state(TRUMA_INETX_VERSION);
    else if (this->status_type_ == STATUS_LAST_DISCONNECT && this->state != this->parent_->last_disconnect())
      this->publish_state(this->parent_->last_disconnect());
    else if (this->status_type_ == STATUS_LINK_PARAMS && this->state != this->parent_->link_params())
      this->publish_state(this->parent_->link_params());
  }

 protected:
  std::string format_firmware_(uint16_t src) const {
    const auto *maj = this->parent_->get_value("Identify", "SwMaj", src);
    const auto *min = this->parent_->get_value("Identify", "SwMin", src);
    const auto *fx = this->parent_->get_value("Identify", "SwBgFx", src);
    if (maj == nullptr || min == nullptr || !maj->is_number() || !min->is_number())
      return "";
    std::string out = std::to_string(maj->as_int()) + "." + std::to_string(min->as_int());
    if (fx != nullptr && fx->is_number())
      out += "." + std::to_string(fx->as_int());
    return out;
  }
  uint8_t format_{TEXT_RAW};
};
#endif

#ifdef USE_NUMBER
// Displayed value = wire value * scale (temperatures: scale 0.1).
class TrumaNumber : public number::Number, public TrumaListener {
 public:
  void set_scale(float s) { this->scale_ = s; }
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    if (v.is_number())
      this->publish_state(static_cast<float>(v.as_double() * this->scale_));
  }

 protected:
  void control(float value) override {
    // No optimistic state: the device confirms by publishing the new value.
    this->parent_->write_param(this->topic_, this->param_, static_cast<int32_t>(lroundf(value / this->scale_)),
                               this->address_);
  }
  float scale_{1.0f};
};
#endif

#ifdef USE_SELECT
class TrumaSelect : public select::Select, public TrumaListener {
 public:
  void set_values(const std::vector<int32_t> &values) { this->values_ = values; }
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    if (!v.is_number())
      return;
    const auto &options = this->traits.get_options();
    for (size_t k = 0; k < this->values_.size() && k < options.size(); k++) {
      if (this->values_[k] == v.as_int()) {
        this->publish_state(std::string(options[k]));
        return;
      }
    }
    ESP_LOGW("truma_inetx", "%s.%s: value %lld has no option", this->topic_.c_str(), this->param_.c_str(),
             static_cast<long long>(v.as_int()));
  }

 protected:
  void control(const std::string &value) override {
    const auto &options = this->traits.get_options();
    for (size_t k = 0; k < this->values_.size() && k < options.size(); k++) {
      if (value == options[k]) {
        this->parent_->write_param(this->topic_, this->param_, this->values_[k], this->address_);
        return;
      }
    }
  }
  std::vector<int32_t> values_;
};
#endif

#ifdef USE_SWITCH
class TrumaSwitch : public switch_::Switch, public TrumaListener {
 public:
  void set_on_off(int32_t on, int32_t off) {
    this->on_ = on;
    this->off_ = off;
  }
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override {
    if (v.is_number())
      this->publish_state(v.as_int() == this->on_);
  }

 protected:
  void write_state(bool state) override {
    this->parent_->write_param(this->topic_, this->param_, state ? this->on_ : this->off_, this->address_);
  }
  int32_t on_{1};
  int32_t off_{0};
};
#endif

#ifdef USE_BUTTON
enum ButtonType : uint8_t { BTN_PAIR, BTN_UNPAIR, BTN_RECONNECT, BTN_DUMP, BTN_MEASURE, BTN_WRITE, BTN_SYNC_TIME, BTN_LOG_RAW };

class TrumaButton : public button::Button {
 public:
  void set_parent(TrumaInetX *p) { this->parent_ = p; }
  void set_type(uint8_t t) { this->type_ = t; }
  void set_write(const std::string &topic, const std::string &param, int32_t value, uint16_t address) {
    this->topic_ = topic;
    this->param_ = param;
    this->value_ = value;
    this->address_ = address;
  }

 protected:
  void press_action() override {
    switch (this->type_) {
      case BTN_PAIR:
        this->parent_->start_pairing();
        break;
      case BTN_UNPAIR:
        this->parent_->unpair();
        break;
      case BTN_RECONNECT:
        this->parent_->reconnect();
        break;
      case BTN_DUMP:
        this->parent_->dump_bus();
        break;
      case BTN_MEASURE:
        this->parent_->request_measurements();
        break;
      case BTN_WRITE:
        this->parent_->write_param(this->topic_, this->param_, this->value_, this->address_);
        break;
      case BTN_SYNC_TIME:
        this->parent_->sync_panel_time();
        break;
      case BTN_LOG_RAW:
        this->parent_->log_raw();
        break;
    }
  }
  TrumaInetX *parent_{nullptr};
  uint8_t type_{BTN_RECONNECT};
  std::string topic_;
  std::string param_;
  int32_t value_{1};
  uint16_t address_{0};
};
#endif

#ifdef USE_CLIMATE
// Room climate: mode from the panel's RoomClimate.Mode, set point and reading
// from the appliance that serves the running mode.
//
// RoomClimate.Mode: 0 off, 1 auto, 2 cool, 3 heat, 4 heat (variant),
// 5 fan only, 6 dry. Temperatures are tenths of a degree on the wire.
class TrumaClimate : public climate::Climate, public TrumaListener {
 public:
  void add_supported_mode(climate::ClimateMode m) { this->modes_.push_back(m); }
  // Bus address of the heater whose readings to show (0 = any). Several
  // devices report AirHeating.Temp; without this the value flips between them.
  void set_appliance_address(uint16_t a) { this->appliance_ = a; }

  bool wants(uint16_t src, const std::string &topic, const std::string &param) const override {
    return topic == "RoomClimate" || topic == "AirHeating" || topic == "AirCooling" || topic == "AirCirculation";
  }
  void on_value(uint16_t src, const truma_inetx_proto::CborValue &v) override { this->recompute_(); }
  void refresh() override { this->recompute_(); }

  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2025, 11, 0)
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
#else
    traits.set_supports_current_temperature(true);
    traits.set_supports_action(true);
#endif
    traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
    for (auto m : this->modes_)
      traits.add_supported_mode(m);
    traits.set_visual_min_temperature(5);
    traits.set_visual_max_temperature(30);
    traits.set_visual_target_temperature_step(0.5f);
    traits.set_visual_current_temperature_step(0.1f);
    return traits;
  }

 protected:
  static climate::ClimateMode to_mode_(int64_t raw) {
    switch (raw) {
      case 1:
        return climate::CLIMATE_MODE_AUTO;
      case 2:
        return climate::CLIMATE_MODE_COOL;
      case 3:
      case 4:
        return climate::CLIMATE_MODE_HEAT;
      case 5:
        return climate::CLIMATE_MODE_FAN_ONLY;
      case 6:
        return climate::CLIMATE_MODE_DRY;
      default:
        return climate::CLIMATE_MODE_OFF;
    }
  }
  int32_t from_mode_(climate::ClimateMode m) {
    switch (m) {
      case climate::CLIMATE_MODE_AUTO:
        return 1;
      case climate::CLIMATE_MODE_COOL:
        return 2;
      case climate::CLIMATE_MODE_HEAT:
        return this->appliance_ == 0x0801 ? 4 : 3;
      case climate::CLIMATE_MODE_FAN_ONLY:
        return 5;
      case climate::CLIMATE_MODE_DRY:
        return 6;
      default:
        return 0;
    }
  }
  // Which topic holds the set point for a mode.
  static const char *setpoint_topic_(climate::ClimateMode m) {
    switch (m) {
      case climate::CLIMATE_MODE_COOL:
      case climate::CLIMATE_MODE_DRY:
        return "AirCooling";
      case climate::CLIMATE_MODE_AUTO:
        return "RoomClimate";
      default:
        return "AirHeating";
    }
  }
  bool active_(const char *topic) const {
    const truma_inetx_proto::CborValue *v = this->parent_->get_value(topic, "Active");
    return v != nullptr && v->is_number() && v->as_int() == 1;
  }
  void recompute_() {
    const truma_inetx_proto::CborValue *mode = this->parent_->get_value("RoomClimate", "Mode", truma_inetx_proto::DEV_PANEL);
    if (mode == nullptr)
      mode = this->parent_->get_value("RoomClimate", "Mode");
    if (mode != nullptr && mode->is_number())
      this->mode = to_mode_(mode->as_int());

    const char *sp_topic = setpoint_topic_(this->mode);
    const uint16_t sp_addr = strcmp(sp_topic, "RoomClimate") == 0 ? 0 : this->appliance_;
    const truma_inetx_proto::CborValue *tgt = this->parent_->get_value(sp_topic, "TgtTemp", sp_addr);
    if (tgt == nullptr && this->mode != climate::CLIMATE_MODE_HEAT)
      tgt = this->parent_->get_value("AirHeating", "TgtTemp", this->appliance_);
    if (tgt != nullptr && tgt->is_number())
      this->target_temperature = static_cast<float>(tgt->as_double() / 10.0);

    const truma_inetx_proto::CborValue *cur = nullptr;
    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_DRY)
      cur = this->parent_->get_value("AirCooling", "Temp");
    if (cur == nullptr)
      cur = this->parent_->get_value("AirHeating", "Temp", this->appliance_);
    if (cur != nullptr && cur->is_number())
      this->current_temperature = static_cast<float>(cur->as_double() / 10.0);

    switch (this->mode) {
      case climate::CLIMATE_MODE_OFF:
        this->action = climate::CLIMATE_ACTION_OFF;
        break;
      case climate::CLIMATE_MODE_HEAT:
        this->action = this->active_("AirHeating") ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_COOL:
        this->action = this->active_("AirCooling") ? climate::CLIMATE_ACTION_COOLING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_DRY:
        this->action = this->active_("AirCooling") ? climate::CLIMATE_ACTION_DRYING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        this->action = this->active_("AirCirculation") ? climate::CLIMATE_ACTION_FAN : climate::CLIMATE_ACTION_IDLE;
        break;
      default:
        this->action = this->active_("AirCooling")   ? climate::CLIMATE_ACTION_COOLING
                       : this->active_("AirHeating") ? climate::CLIMATE_ACTION_HEATING
                                                     : climate::CLIMATE_ACTION_IDLE;
        break;
    }
    this->publish_state();
  }

  void control(const climate::ClimateCall &call) override {
    climate::ClimateMode target_mode = this->mode;
    if (call.get_mode().has_value()) {
      target_mode = *call.get_mode();
      this->parent_->write_param("RoomClimate", "Mode", from_mode_(target_mode), truma_inetx_proto::DEV_PANEL);
    }
    if (call.get_target_temperature().has_value()) {
      const float t = *call.get_target_temperature();
      this->parent_->write_param(setpoint_topic_(target_mode), "TgtTemp", static_cast<int32_t>(lroundf(t * 10.0f)));
    }
    // No optimistic publish: the panel/heater echo the new values.
  }

  std::vector<climate::ClimateMode> modes_;
  uint16_t appliance_{0};
};
#endif

}  // namespace truma_inetx
}  // namespace esphome
