#include "desk_panel.h"
#include "desk_tile.h"

namespace esphome::desk_panel {

class ConfirmCard : public tessera::Card {
 public:
  void open(const tessera::CardContext &c) override {}
};

void DeskPanel::setup() {
  add_tile("desk", [this]() { return new DeskTile(this); });
  add_card("confirm", []() { return new ConfirmCard(); });
}

}  // namespace esphome::desk_panel
