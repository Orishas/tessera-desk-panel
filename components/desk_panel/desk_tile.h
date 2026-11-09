#pragma once
#include "esphome/components/smart_display/plugin_api.h"
#include "model.h"

namespace esphome::desk_panel {

// The desk: with the monitor off (REST) large keys that start a source, with it on (WORK) the sources in a row, the
// monitor's keys, Auto and the device under them. Two moods at the bottom of a tall tile. The decisions are model.h's.
class DeskTile : public tessera::Tile {
 public:
  explicit DeskTile(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override;
  void on_state(JsonObjectConst data) override;
  void on_tick(uint32_t epoch) override;
  void on_theme() override { paint(); }
  void on_tap() override;

 private:
  struct Source {
    char value[65];
    char logo[33];
  };
  // A key that is not a source: a block with an icon over a word.
  enum Ctl { C_PICTURE, C_POWER, C_AUTO, C_DEVICE, C_MOOD0, C_MOOD1, C_COUNT };

  void build();   // lays out the current view and shows the keys it has
  void paint();   // fills, colours and words, without laying out again
  void refresh(); // build() when the layout must change (needs_relayout), else paint()
  int active() const;        // the chosen source: the one just tapped, else the select's option; -1 none
  int auto_index() const;    // the chosen source's Auto switch, -1 none
  Have have() const;
  static Ctl ctl_of(Key k);
  void place_source(int i, const Rect &r);
  void place_ctl(Ctl k, const Rect &r);
  void tap_source(int i);
  void tap_switch(const tessera::Text &entity, bool &on);
  void tap_device();

  const tessera::Plugin *plugin_;
  lv_obj_t *parent_{};
  int width_ = 0, height_ = 0, tile_ = -1;
  bool tall_ = true;
  int rows_ = 3;
  tessera::Text entity_, script_, picture_, power_, device_, chosen_;
  tessera::Text status_[3], auto_[3], mood_[2];
  tessera::Text device_name_, mood_name_[2];

  Source src_[MAX_SOURCES]{};
  int count_ = 0, pending_ = -1, mood_chosen_ = -1;
  uint32_t pending_at_ = 0;
  bool built_ = false;
  Have laid_{};            // what the current layout was made for
  View laid_view_ = View::REST;
  bool status_on_[3]{}, auto_on_[3]{};
  bool picture_on_ = false, power_on_ = false, device_on_ = false;
  View view_ = View::REST;
  Layout layout_;
  Starting starting_;
  bool pulsing_ = false;

  lv_obj_t *sblock_[MAX_SOURCES]{}, *slogo_[MAX_SOURCES]{}, *sword_[MAX_SOURCES]{}, *sdot_[MAX_SOURCES]{},
      *sauto_[MAX_SOURCES]{};
  tessera::List<uint8_t> mask_[MAX_SOURCES];
  lv_image_dsc_t image_[MAX_SOURCES]{};
  int made_side_[MAX_SOURCES]{};
  const uint8_t *made_logo_[MAX_SOURCES]{};
  bool shown_src_[MAX_SOURCES]{};
  lv_obj_t *cblock_[C_COUNT]{}, *cicon_[C_COUNT]{}, *cword_[C_COUNT]{};
  bool shown_ctl_[C_COUNT]{};
  lv_obj_t *note_{};
};

}  // namespace esphome::desk_panel
