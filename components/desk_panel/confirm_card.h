#pragma once
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::desk_panel {

// "Shut down PC?": Cancel on the left, Shut down on the right. Opened by the desk's device key while the device is on.
class ConfirmCard : public tessera::Card {
 public:
  explicit ConfirmCard(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void open(const tessera::CardContext &c) override;
  void on_state(JsonObjectConst data) override;
  void on_theme() override { paint(); }
  void on_tap() override;

 private:
  void paint();
  const tessera::Plugin *plugin_;
  tessera::Text entity_, name_;
  int width_ = 0, height_ = 0, keys_y_ = 0, key_w_ = 0, key_h_ = 0, gap_ = 0;
  lv_obj_t *question_{}, *cancel_{}, *cancel_word_{}, *off_{}, *off_word_{};
};

}  // namespace esphome::desk_panel
