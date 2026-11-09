#include "desk_panel.h"

namespace esphome::desk_panel {

namespace ui = tessera::ui;
using tessera::Font;

// Scaffold: the tile says what it needs until the next tasks draw it.
class DeskTile : public tessera::Tile {
 public:
  explicit DeskTile(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override {
    note_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_width(note_, c.width);
    lv_obj_set_style_text_align(note_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(note_, 0, (c.height - ui::line_height(Font::BODY)) / 2);
    ui::set_text(note_, plugin_->text("choose_select"));
  }
  void on_state(JsonObjectConst data) override {}
  void on_theme() override { ui::set_color(note_, theme::MUTED); }

 private:
  const tessera::Plugin *plugin_;
  lv_obj_t *note_{};
};

class ConfirmCard : public tessera::Card {
 public:
  void open(const tessera::CardContext &c) override {}
};

void DeskPanel::setup() {
  add_tile("desk", [this]() { return new DeskTile(this); });
  add_card("confirm", []() { return new ConfirmCard(); });
}

}  // namespace esphome::desk_panel
