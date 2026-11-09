#include "desk_panel.h"
#include "confirm_card.h"
#include "desk_tile.h"

namespace esphome::desk_panel {

void DeskPanel::setup() {
  add_tile("desk", [this]() { return new DeskTile(this); });
  add_card("confirm", [this]() { return new ConfirmCard(this); });
}

}  // namespace esphome::desk_panel
