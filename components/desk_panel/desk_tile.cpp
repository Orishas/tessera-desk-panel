#include "desk_tile.h"
#include "logo.h"
#include "parts.h"
#include "esphome/core/hal.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace esphome::desk_panel {

using tessera::Font;
namespace ui = tessera::ui;

// How long a tapped source stays chosen before Home Assistant has said so, in milliseconds.
static constexpr uint32_t PENDING_MS = 15000;
// Icons of Tessera's set.
static constexpr uint32_t ICON_EYE = 0xF0208;
static constexpr uint32_t ICON_POWER = 0xF0425;
static constexpr uint32_t ICON_AUTO = 0xF18F2;
static constexpr uint32_t ICON_MONITOR = 0xF0379;

// "status_1" ... "auto_4": the id of an option, and of its entity in on_state's data.
struct OptionId {
  char text[12];
  OptionId(const char *stem, int i) { snprintf(text, sizeof(text), "%s_%d", stem, i + 1); }
};

static tessera::Text option(const tessera::TileContext &c, const char *id) {
  return roomy::text(std::string(c.options[id] | ""));
}

void DeskTile::create(const tessera::TileContext &c) {
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;
  rows_ = c.rows;
  entity_ = roomy::text(std::string(c.entity ? c.entity : ""));
  script_ = option(c, "script");
  picture_ = option(c, "picture");
  power_ = option(c, "power");
  device_ = option(c, "device");
  for (int i = 0; i < WITH_OPTIONS; ++i) {
    status_[i] = option(c, OptionId("status", i).text);
    auto_[i] = option(c, OptionId("auto", i).text);
  }

  for (int i = 0; i < MAX_SOURCES; ++i) {
    sblock_[i] = key_block(c.parent);
    // The logo, the word, the dot and the Auto mark lie in their key's block, so they move and colour with it.
    // The border is always there, in the block's own colour: the frame of the chosen source moves nothing.
    lv_obj_set_style_border_width(sblock_[i], ui::px(2), 0);
    lv_obj_set_style_border_opa(sblock_[i], LV_OPA_COVER, 0);
    slogo_[i] = lv_image_create(sblock_[i]);
    lv_obj_set_style_image_recolor_opa(slogo_[i], LV_OPA_COVER, 0);
    lv_obj_remove_flag(slogo_[i], LV_OBJ_FLAG_CLICKABLE);
    sword_[i] = ui::label(sblock_[i], Font::BODY, theme::INK_SOFT);
    lv_obj_set_style_text_align(sword_[i], LV_TEXT_ALIGN_CENTER, 0);
    sdot_[i] = ui::block(sblock_[i], theme::ACCENT);
    lv_obj_set_style_radius(sdot_[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(sdot_[i], ui::mm(2), ui::mm(2));
    if (i < WITH_OPTIONS) {   // only these sources have an Auto option
      sauto_[i] = ui::label(sblock_[i], Font::ICON_SMALL, theme::INK_SOFT);
      ui::set_text(sauto_[i], ui::icon(ICON_AUTO));
    }
    show(sblock_[i], false);
  }
  for (int k = 0; k < C_COUNT; ++k) {
    cblock_[k] = key_block(c.parent);
    cicon_[k] = ui::label(cblock_[k], Font::ICON_SMALL, theme::INK);
    cword_[k] = ui::label(cblock_[k], Font::BODY, theme::INK_SOFT);
    lv_obj_set_style_text_align(cword_[k], LV_TEXT_ALIGN_CENTER, 0);
    show(cblock_[k], false);
  }
  cdot_ = ui::block(cblock_[C_DEVICE], theme::ACCENT);
  lv_obj_set_style_radius(cdot_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_size(cdot_, ui::mm(2), ui::mm(2));
  note_ = ui::label(c.parent, Font::BODY, theme::MUTED);
  lv_obj_set_width(note_, width_);
  lv_obj_set_style_text_align(note_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(note_, 0, (height_ - ui::line_height(Font::BODY)) / 2);
  build();
}

void DeskTile::on_state(JsonObjectConst data) {
  chosen_ = roomy::text(std::string(data["state"] | ""));
  int count = 0;
  for (JsonVariantConst v : data["attributes"]["options"].as<JsonArrayConst>()) {
    if (count >= MAX_SOURCES) break;
    const char *s = v.as<const char *>();
    // An option that does not fit is left out: cut short, a tap would send a value the select does not have.
    if (!s || !*s || strlen(s) >= sizeof(src_[0].value)) continue;
    Source &src = src_[count++];
    snprintf(src.value, sizeof(src.value), "%s", s);
    snprintf(src.logo, sizeof(src.logo), "%s", guess_logo(s).c_str());
  }
  JsonObjectConst e = data["entities"];
  const auto is_on = [&e](const char *id) { return strcmp(e[id]["state"] | "", "on") == 0; };
  for (int i = 0; i < WITH_OPTIONS; ++i) {
    status_on_[i] = is_running(e[OptionId("status", i).text]["state"] | "");
    auto_on_[i] = is_on(OptionId("auto", i).text);
  }
  picture_on_ = is_on("picture");
  power_on_ = is_on("power");
  device_on_ = is_on("device");
  device_name_ = roomy::text(std::string(e["device"]["name"] | ""));
  // Home Assistant has the tapped option now, or the option is gone: the key follows the select again.
  if (pending_ >= count || (pending_ >= 0 && roomy::same(chosen_, std::string(src_[pending_].value)))) pending_ = -1;
  // The device is there: "Starting ..." ends with this state, not a second later.
  if (pulsing_ && !starting_.active(esphome::millis(), device_on_)) pulsing_ = false;
  if (starting_src_ >= count ||
      (starting_src_ >= 0 && !src_starting_.active(esphome::millis(), status_on_[starting_src_])))
    starting_src_ = -1;
  toggles_.clear();   // Home Assistant has spoken: the keys show its states

  bool has_status[WITH_OPTIONS];
  for (int i = 0; i < WITH_OPTIONS; ++i) has_status[i] = !status_[i].empty();
  count_ = count;
  view_ = choose_view(!power_.empty(), power_on_, status_on_, has_status);
  refresh();
}

void DeskTile::refresh() {
  if (!built_ || needs_relayout(laid_, laid_view_, have(), view_)) build();
  else paint();
}

void DeskTile::on_tick(uint32_t) {
  const uint32_t now = esphome::millis();
  if (pending_ >= 0 && now - pending_at_ > PENDING_MS) {
    pending_ = -1;
    refresh();   // the chosen source is the select's again: its Auto key may come or go
  }
  // No answer to a switch: its key shows what Home Assistant last said. Each switch waits for itself.
  if (toggles_.expire(now)) paint();
  if (!pulsing_ && starting_src_ < 0) return;
  // "Starting ...": the dot of what starts blinks once a second, painted here (no animation of its own). Counted
  // from the tap, so it blinks on a screen whose clock is not set too.
  if (pulsing_) {
    if (starting_.active(now, device_on_)) {
      pulse_lit_ = starting_.lit(now);
    } else {
      pulsing_ = false;   // the device is there, or it took too long: the key shows its state again
      paint();
    }
  }
  if (starting_src_ >= 0 && !src_starting_.active(now, status_on_[starting_src_])) starting_src_ = -1;
  paint_dots();
}

void DeskTile::on_tap() {
  const tessera::Point p = tessera::tap_point();
  if (p.x < 0 || entity_.empty()) return;
  const Key k = hit(layout_, p.x, p.y);
  const int ki = int(k);
  if (ki >= int(Key::SOURCE0) && ki < int(Key::SOURCE0) + MAX_SOURCES) return tap_source(ki);
  switch (k) {
    case Key::PICTURE: return tap_switch(picture_, picture_on_);
    case Key::POWER: return tap_switch(power_, power_on_);
    case Key::AUTO: {
      const int a = auto_index();
      if (a >= 0) tap_switch(auto_[a], auto_on_[a]);
      return;
    }
    case Key::DEVICE: return tap_device();
    default: return;
  }
}

int DeskTile::active() const {
  if (pending_ >= 0) return pending_;
  for (int i = 0; i < count_; ++i)
    if (roomy::same(chosen_, std::string(src_[i].value))) return i;
  return -1;
}

int DeskTile::auto_index() const {
  const int i = active();
  return i >= 0 && i < WITH_OPTIONS && !auto_[i].empty() ? i : -1;
}

Have DeskTile::have() const {
  return Have{count_, !picture_.empty(), !power_.empty(), auto_index() >= 0, !device_.empty(), rows_};
}

DeskTile::Ctl DeskTile::ctl_of(Key k) {
  switch (k) {
    case Key::PICTURE: return C_PICTURE;
    case Key::POWER: return C_POWER;
    case Key::AUTO: return C_AUTO;
    default: return C_DEVICE;
  }
}

void DeskTile::build() {
  built_ = true;
  const Have h = have();
  laid_ = h;
  laid_view_ = view_;
  const int gap = ui::px(8);
  layout_ = view_ == View::REST ? layout_rest(width_, height_, h, gap) : layout_work(width_, height_, h, gap);
  for (bool &s : shown_src_) s = false;
  for (bool &s : shown_ctl_) s = false;
  if (!entity_.empty() && count_ > 0) {
    for (int i = 0; i < layout_.count; ++i) {
      const Slot &s = layout_.slots[i];
      const int k = int(s.key);
      if (k >= int(Key::SOURCE0) && k < int(Key::SOURCE0) + MAX_SOURCES) {
        place_source(k, s.r);
        shown_src_[k] = true;
      } else {
        place_ctl(ctl_of(s.key), s.r);
        shown_ctl_[ctl_of(s.key)] = true;
      }
    }
  }
  for (int i = 0; i < MAX_SOURCES; ++i) show(sblock_[i], shown_src_[i]);
  for (int k = 0; k < C_COUNT; ++k) show(cblock_[k], shown_ctl_[k]);
  paint();
}

void DeskTile::place_source(int i, const Rect &r) {
  lv_obj_set_pos(sblock_[i], r.x, r.y);
  lv_obj_set_size(sblock_[i], r.w, r.h);
  const bool rest = view_ == View::REST;
  const int pad = ui::px(8);
  const int lh = ui::line_height(Font::BODY);
  // REST: a large logo, 45 % of the key's shorter side. WORK: as large as the key leaves beside its word. Never
  // more than LOGO_MOST, which bounds its mask.
  const int room = rest ? std::min(r.w, r.h) * 45 / 100 : std::min(r.w - 2 * pad, r.h - lh - 3 * pad);
  const int side = std::max(0, std::min(room, LOGO_MOST));
  const uint8_t *alpha = src_[i].logo[0] && side > 0 ? logo_alpha(src_[i].logo) : nullptr;
  const int b = ui::px(2);   // the block's border: its children sit inside it
  const int top = (r.h - (alpha ? side + pad : 0) - lh) / 2 - b;
  if (alpha) {
    // The mask at the key's size, made again only when the size or the logo changed.
    if (made_side_[i] != side || made_logo_[i] != alpha) {
      // The image lets go of the old mask before it is made again; set anew below, it reads the new size.
      lv_image_set_src(slogo_[i], nullptr);
      mask_[i].assign(size_t(side) * side, 0);
      resample(alpha, side, mask_[i].data());
      lv_image_dsc_t &d = image_[i];
      d = lv_image_dsc_t{};
      d.header.magic = LV_IMAGE_HEADER_MAGIC;
      d.header.cf = LV_COLOR_FORMAT_A8;
      d.header.w = side;
      d.header.h = side;
      d.header.stride = side;
      d.data = mask_[i].data();
      d.data_size = mask_[i].size();
      made_side_[i] = side;
      made_logo_[i] = alpha;
      lv_image_set_src(slogo_[i], &image_[i]);
      lv_obj_invalidate(slogo_[i]);
    }
    lv_obj_set_pos(slogo_[i], (r.w - side) / 2 - b, top);
    lv_obj_set_size(slogo_[i], side, side);
  }
  show(slogo_[i], alpha != nullptr);
  lv_obj_set_pos(sword_[i], 0, alpha ? top + side + pad : top);
  lv_obj_set_width(sword_[i], r.w - 2 * b);
  ui::set_text(sword_[i], src_[i].value);
  const int inset = ui::px(8), dot = ui::mm(2);
  lv_obj_set_pos(sdot_[i], r.w - inset - dot - b, inset - b);
  if (sauto_[i]) lv_obj_set_pos(sauto_[i], inset - b, inset - b);   // top left; the dot is top right
  // The mark shows only where it touches neither the logo nor the word: a small key goes without.
  const int mark_end = inset - b + ui::line_height(Font::ICON_SMALL);
  mark_fits_[i] = mark_end <= top || (alpha && mark_end <= (r.w - side) / 2 - b);
}

void DeskTile::place_ctl(Ctl k, const Rect &r) {
  lv_obj_set_pos(cblock_[k], r.x, r.y);
  lv_obj_set_size(cblock_[k], r.w, r.h);
  const int ih = ui::line_height(Font::ICON_SMALL), wh = ui::line_height(Font::BODY), pad = ui::px(4);
  // The device's dot is top right, as a source's; its icon and word keep clear of it on both sides.
  const int m = k == C_DEVICE ? ui::px(8) + ui::mm(2) + pad : 0;
  if (k == C_DEVICE) lv_obj_set_pos(cdot_, r.w - ui::px(8) - ui::mm(2), ui::px(8));
  if (r.h < ih + pad + wh + 2 * pad) {
    // Too low for the icon over the word (the device's row over the whole width): the two side by side, meeting a
    // little left of the middle so a short word balances the icon.
    const int split = r.w / 2 - ih / 2;
    lv_obj_set_width(cicon_[k], split - pad);
    lv_obj_set_style_text_align(cicon_[k], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(cicon_[k], 0, (r.h - ih) / 2);
    lv_obj_set_width(cword_[k], r.w - split - pad - m);
    lv_obj_set_style_text_align(cword_[k], LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(cword_[k], split + pad, (r.h - wh) / 2);
    return;
  }
  const int top = (r.h - ih - pad - wh) / 2;
  lv_obj_set_width(cicon_[k], r.w - 2 * m);
  lv_obj_set_style_text_align(cicon_[k], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cicon_[k], m, top);
  lv_obj_set_width(cword_[k], r.w - 2 * m);
  lv_obj_set_style_text_align(cword_[k], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cword_[k], m, top + ih + pad);
}

void DeskTile::paint() {
  const int on = active();
  for (int i = 0; i < MAX_SOURCES; ++i) {
    if (!shown_src_[i]) continue;
    const bool chosen = source_lit(view_, i, pending_, on);
    ui::set_fill(sblock_[i], tessera::Tone::NORMAL, chosen ? theme::ACCENT : theme::TRACK);
    // REST: a frame around the source the monitor shows when it comes on.
    const bool framed = source_framed(view_, i, pending_, on);
    set_style_color(sblock_[i], LV_STYLE_BORDER_COLOR, ui::color(chosen || framed ? theme::ACCENT : theme::TRACK));
    ui::set_color(sword_[i], chosen ? theme::ON_ACCENT : theme::INK_SOFT);
    set_style_color(slogo_[i], LV_STYLE_IMAGE_RECOLOR, ui::color(chosen ? theme::ON_ACCENT : theme::INK));
    ui::set_fill(sdot_[i], tessera::Tone::NORMAL, chosen ? theme::ON_ACCENT : theme::ACCENT);
    // The Auto mark, in both views: this source turns the monitor on by itself.
    if (sauto_[i]) ui::set_color(sauto_[i], chosen ? theme::ON_ACCENT : theme::INK_SOFT);
    show(sauto_[i], i < WITH_OPTIONS && mark_fits_[i] && !auto_[i].empty() && auto_on_[i]);
  }
  const int a = auto_index();
  const bool auto_on = a >= 0 && auto_on_[a];
  // The keys stay grey, the accent is the chosen source's: a switch that is off has a muted icon and says so.
  struct Look { bool off; uint32_t icon; std::string word; };
  const Look looks[C_COUNT] = {
      {!picture_on_, ICON_EYE, plugin_->text(picture_on_ ? "picture" : "picture_off")},
      {false, ICON_MONITOR, plugin_->text("monitor")},
      {!auto_on, ICON_AUTO, plugin_->text(auto_on ? "auto" : "auto_off")},
      // The device says on with its dot, as a source does, and "Starting ..." with the dot blinking.
      {false, ICON_POWER,
       pulsing_ ? plugin_->text("starting") : device_name_.empty() ? std::string("") : roomy::plain(device_name_)},
  };
  for (int k = 0; k < C_COUNT; ++k) {
    if (!shown_ctl_[k]) continue;
    ui::set_fill(cblock_[k], tessera::Tone::NORMAL, theme::TRACK);
    ui::set_text(cicon_[k], ui::icon(looks[k].icon));
    ui::set_color(cicon_[k], looks[k].off ? theme::MUTED : theme::INK);
    ui::set_text(cword_[k], looks[k].word);
    ui::set_color(cword_[k], looks[k].off ? theme::MUTED : theme::INK_SOFT);
  }
  ui::set_fill(cdot_, tessera::Tone::NORMAL, theme::ACCENT);
  paint_dots();
  // Nothing to show: say what is missing.
  const bool empty = entity_.empty() || count_ == 0;
  ui::set_text(note_, empty ? plugin_->text("choose_select") : std::string(""));
  ui::set_color(note_, theme::MUTED);
  show(note_, empty);
}

// A dot shows while its device runs, and every other second while it starts.
void DeskTile::paint_dots() {
  for (int i = 0; i < MAX_SOURCES; ++i) {
    if (!shown_src_[i]) continue;
    const bool running = i < WITH_OPTIONS && !status_[i].empty() && status_on_[i];
    show(sdot_[i], running || (i == starting_src_ && src_starting_.lit(esphome::millis())));
  }
  show(cdot_, shown_ctl_[C_DEVICE] && (pulsing_ ? pulse_lit_ : device_on_));
}

void DeskTile::tap_source(int i) {
  if (i >= count_) return;
  const std::string value = src_[i].value;
  bool sent;
  if (!script_.empty()) {
    // The option under both names, so the script's field may be called either: a script takes what it has no field
    // for as a plain variable.
    JsonDocument d;
    d["variables"]["source"] = value;
    d["variables"]["quelle"] = value;
    sent = tessera::action("script.turn_on", roomy::plain(script_), d);
  } else {
    const std::string entity = roomy::plain(entity_);
    JsonDocument d;
    d["option"] = value;
    sent = tessera::action(entity.rfind("select.", 0) == 0 ? "select.select_option" : "input_select.select_option",
                           entity, d);
  }
  if (sent) {
    pending_ = i;
    pending_at_ = esphome::millis();
    // Its device is off: the dot blinks until it is there. Without a status option nothing says so, so nothing blinks.
    starting_src_ = -1;
    if (i < WITH_OPTIONS && !status_[i].empty() && !status_on_[i]) {
      src_starting_.begin(pending_at_);
      starting_src_ = i;
    }
    refresh();   // another source may bring or take away the Auto key
  }
}

// A switch or an input boolean, turned the other way; the key shows it at once and Home Assistant confirms. Without
// an answer within CONFIRM_MS the key goes back (on_tick), each switch by itself.
void DeskTile::tap_switch(const tessera::Text &entity, bool &on) {
  const std::string e = roomy::plain(entity);
  if (e.empty()) return;
  if (tessera::action(switch_service(e.c_str(), !on), e)) {
    toggles_.flip(on, esphome::millis());
    paint();
  }
}

// Off: turn it on and pulse until it is there. On: ask first, on the card. Starting: nothing, it is on its way.
void DeskTile::tap_device() {
  if (starting_.active(esphome::millis(), device_on_)) return;
  const std::string e = roomy::plain(device_);
  if (e.empty()) return;
  if (device_on_) {
    const std::string name = device_name_.empty() ? std::string("") : roomy::plain(device_name_);
    tessera::open_card(plugin_->plugin_id(), "confirm", e, tile_, name);
    return;
  }
  if (tessera::action(switch_service(e.c_str(), true), e)) {
    starting_.begin(esphome::millis());
    pulsing_ = true;
    pulse_lit_ = true;
    paint();
  }
}

}  // namespace esphome::desk_panel
