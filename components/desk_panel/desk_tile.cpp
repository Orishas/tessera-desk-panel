#include "desk_tile.h"
#include "logo.h"
#include "logos.h"
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

static void show(lv_obj_t *o, bool on) {
  if (!o) return;
  if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *key_block(lv_obj_t *parent) {
  lv_obj_t *b = ui::block(parent, theme::TRACK);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}

static std::string option(const tessera::TileContext &c, const char *id) { return c.options[id] | ""; }

void DeskTile::create(const tessera::TileContext &c) {
  parent_ = c.parent;
  width_ = c.width;
  height_ = c.height;
  tile_ = c.tile;
  tall_ = c.rows >= 3;
  rows_ = c.rows;
  entity_ = roomy::text(std::string(c.entity ? c.entity : ""));
  script_ = roomy::text(option(c, "script"));
  picture_ = roomy::text(option(c, "picture"));
  power_ = roomy::text(option(c, "power"));
  device_ = roomy::text(option(c, "device"));
  char id[12];
  for (int i = 0; i < 3; ++i) {
    snprintf(id, sizeof(id), "status_%d", i + 1);
    status_[i] = roomy::text(option(c, id));
    snprintf(id, sizeof(id), "auto_%d", i + 1);
    auto_[i] = roomy::text(option(c, id));
  }

  for (int i = 0; i < MAX_SOURCES; ++i) {
    sblock_[i] = key_block(c.parent);
    // The logo, the word, the dot and the "A" lie in their key's block, so they move and colour with it.
    slogo_[i] = lv_image_create(sblock_[i]);
    lv_obj_set_style_image_recolor_opa(slogo_[i], LV_OPA_COVER, 0);
    lv_obj_remove_flag(slogo_[i], LV_OBJ_FLAG_CLICKABLE);
    sword_[i] = ui::label(sblock_[i], Font::BODY, theme::INK_SOFT);
    lv_obj_set_style_text_align(sword_[i], LV_TEXT_ALIGN_CENTER, 0);
    sdot_[i] = ui::block(sblock_[i], theme::ACCENT);
    lv_obj_set_style_radius(sdot_[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(sdot_[i], ui::mm(2), ui::mm(2));
    if (i < 3) {   // only the first three sources have an Auto option
      sauto_[i] = ui::label(sblock_[i], Font::BODY, theme::MUTED);
      ui::set_text(sauto_[i], "A");
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
  char id[12];
  for (int i = 0; i < 3; ++i) {
    snprintf(id, sizeof(id), "status_%d", i + 1);
    status_on_[i] = is_running(e[id]["state"] | "");
    snprintf(id, sizeof(id), "auto_%d", i + 1);
    auto_on_[i] = std::string(e[id]["state"] | "") == "on";
  }
  picture_on_ = std::string(e["picture"]["state"] | "") == "on";
  power_on_ = std::string(e["power"]["state"] | "") == "on";
  device_on_ = std::string(e["device"]["state"] | "") == "on";
  device_name_ = roomy::text(std::string(e["device"]["name"] | ""));
  // Home Assistant has the tapped option now, or the option is gone: the key follows the select again.
  if (pending_ >= count || (pending_ >= 0 && roomy::same(chosen_, std::string(src_[pending_].value)))) pending_ = -1;
  // The device is there: "Starting ..." ends with this state, not a second later.
  if (pulsing_ && !starting_.active(esphome::millis(), device_on_)) pulsing_ = false;

  bool has_status[3];
  for (int i = 0; i < 3; ++i) has_status[i] = !status_[i].empty();
  count_ = count;
  view_ = choose_view(!power_.empty(), power_on_, status_on_, has_status);
  refresh();
}

void DeskTile::refresh() {
  if (!built_ || needs_relayout(laid_, laid_view_, have(), view_)) build();
  else paint();
}

void DeskTile::on_tick(uint32_t) {
  if (pending_ >= 0 && esphome::millis() - pending_at_ > PENDING_MS) {
    pending_ = -1;
    refresh();   // the chosen source is the select's again: its Auto key may come or go
  }
  if (!pulsing_) return;
  if (!starting_.active(esphome::millis(), device_on_)) {
    pulsing_ = false;   // the device is there, or it took too long: the key shows its state again
    paint();
    return;
  }
  // "Starting ...": the device key pulses once a second, painted here (no animation of its own). Counted from the
  // tap, so it pulses on a screen whose clock is not set too.
  const bool lit = starting_.lit(esphome::millis());
  pulse_lit_ = lit;
  if (!shown_ctl_[C_DEVICE]) return;
  ui::set_fill(cblock_[C_DEVICE], tessera::Tone::NORMAL, lit ? theme::ACCENT : theme::TRACK);
  ui::set_color(cicon_[C_DEVICE], lit ? theme::ON_ACCENT : theme::INK);
  ui::set_color(cword_[C_DEVICE], lit ? theme::ON_ACCENT : theme::INK_SOFT);
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
  return i >= 0 && i < 3 && !auto_[i].empty() ? i : -1;
}

Have DeskTile::have() const {
  return Have{count_, !picture_.empty(), !power_.empty(), auto_index() >= 0, !device_.empty(), tall_, rows_};
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
  // REST: a large logo, 45 % of the key's shorter side. WORK: as large as the key leaves beside its word.
  const int side = std::max(0, rest ? std::min(r.w, r.h) * 45 / 100 : std::min(r.w - 2 * pad, r.h - lh - 3 * pad));
  const uint8_t *alpha = src_[i].logo[0] && side > 0 ? logo_alpha(src_[i].logo) : nullptr;
  const int top = (r.h - (alpha ? side + pad : 0) - lh) / 2;
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
    lv_obj_set_pos(slogo_[i], (r.w - side) / 2, top);
    lv_obj_set_size(slogo_[i], side, side);
  }
  show(slogo_[i], alpha != nullptr);
  lv_obj_set_pos(sword_[i], 0, alpha ? top + side + pad : top);
  lv_obj_set_width(sword_[i], r.w);
  ui::set_text(sword_[i], src_[i].value);
  const int inset = ui::px(8), dot = ui::mm(2);
  lv_obj_set_pos(sdot_[i], r.w - inset - dot, inset);
  if (sauto_[i]) lv_obj_set_pos(sauto_[i], inset, inset);   // top left; the dot is top right
}

void DeskTile::place_ctl(Ctl k, const Rect &r) {
  lv_obj_set_pos(cblock_[k], r.x, r.y);
  lv_obj_set_size(cblock_[k], r.w, r.h);
  const int ih = ui::line_height(Font::ICON_SMALL), wh = ui::line_height(Font::BODY), pad = ui::px(4);
  if (r.h < ih + pad + wh + 2 * pad) {
    // Too low for the icon over the word (the device's row over the whole width): the two side by side, meeting a
    // little left of the middle so a short word balances the icon.
    const int split = r.w / 2 - ih / 2;
    lv_obj_set_width(cicon_[k], split - pad);
    lv_obj_set_style_text_align(cicon_[k], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(cicon_[k], 0, (r.h - ih) / 2);
    lv_obj_set_width(cword_[k], r.w - split - pad);
    lv_obj_set_style_text_align(cword_[k], LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_pos(cword_[k], split + pad, (r.h - wh) / 2);
    return;
  }
  const int top = (r.h - ih - pad - wh) / 2;
  lv_obj_set_width(cicon_[k], r.w);
  lv_obj_set_style_text_align(cicon_[k], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cicon_[k], 0, top);
  lv_obj_set_width(cword_[k], r.w);
  lv_obj_set_style_text_align(cword_[k], LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(cword_[k], 0, top + ih + pad);
}

void DeskTile::paint() {
  const int on = active();
  const bool rest = view_ == View::REST;
  for (int i = 0; i < MAX_SOURCES; ++i) {
    if (!shown_src_[i]) continue;
    const bool chosen = source_lit(view_, i, pending_, on);
    ui::set_fill(sblock_[i], tessera::Tone::NORMAL, chosen ? theme::ACCENT : theme::TRACK);
    ui::set_color(sword_[i], chosen ? theme::ON_ACCENT : theme::INK_SOFT);
    lv_obj_set_style_image_recolor(slogo_[i], ui::color(chosen ? theme::ON_ACCENT : theme::INK), 0);
    const bool running = i < 3 && !status_[i].empty() && status_on_[i];
    ui::set_fill(sdot_[i], tessera::Tone::NORMAL, chosen ? theme::ON_ACCENT : theme::ACCENT);
    show(sdot_[i], running);
    if (sauto_[i]) ui::set_color(sauto_[i], theme::MUTED);
    show(sauto_[i], rest && i < 3 && !auto_[i].empty() && auto_on_[i]);
  }
  const int a = auto_index();
  const bool starting = pulsing_;
  struct Look { bool accent; uint32_t icon; std::string word; };
  const Look looks[C_COUNT] = {
      // Picture and monitor stay grey: in WORK the monitor is on, and a row of blue keys says nothing.
      {false, ICON_EYE, plugin_->text("picture")},
      {false, ICON_POWER, plugin_->text("monitor")},
      {a >= 0 && auto_on_[a], ICON_AUTO, plugin_->text("auto")},
      // While it starts the key keeps this second's pulse, whatever else made the tile paint.
      {starting ? pulse_lit_ : device_on_, ICON_POWER,
       starting ? plugin_->text("starting") : device_name_.empty() ? std::string("") : roomy::plain(device_name_)},
  };
  for (int k = 0; k < C_COUNT; ++k) {
    if (!shown_ctl_[k]) continue;
    const bool accent = looks[k].accent;
    ui::set_fill(cblock_[k], tessera::Tone::NORMAL, accent ? theme::ACCENT : theme::TRACK);
    ui::set_text(cicon_[k], ui::icon(looks[k].icon));
    // The picture's key says "off" by fading: grey like the others, its icon and word muted.
    const bool faded = k == C_PICTURE && !picture_on_;
    ui::set_color(cicon_[k], accent ? theme::ON_ACCENT : faded ? theme::SUBTLE : theme::INK);
    ui::set_text(cword_[k], looks[k].word);
    ui::set_color(cword_[k], accent ? theme::ON_ACCENT : faded ? theme::SUBTLE : theme::INK_SOFT);
  }
  // Nothing to show: say what is missing.
  const bool empty = entity_.empty() || count_ == 0;
  ui::set_text(note_, empty ? plugin_->text("choose_select") : std::string(""));
  ui::set_color(note_, theme::MUTED);
  show(note_, empty);
}

void DeskTile::tap_source(int i) {
  if (i >= count_) return;
  const std::string value = src_[i].value;
  bool sent;
  if (!script_.empty()) {
    JsonDocument d;
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
    refresh();   // another source may bring or take away the Auto key
  }
}

// A switch or an input boolean, turned the other way; the key shows it at once and Home Assistant confirms.
void DeskTile::tap_switch(const tessera::Text &entity, bool &on) {
  const std::string e = roomy::plain(entity);
  if (e.empty()) return;
  const bool boolean = e.rfind("input_boolean.", 0) == 0;
  const char *service = on ? (boolean ? "input_boolean.turn_off" : "switch.turn_off")
                           : (boolean ? "input_boolean.turn_on" : "switch.turn_on");
  if (tessera::action(service, e)) {
    on = !on;
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
  const bool boolean = e.rfind("input_boolean.", 0) == 0;
  if (tessera::action(boolean ? "input_boolean.turn_on" : "switch.turn_on", e)) {
    starting_.begin(esphome::millis());
    pulsing_ = true;
    pulse_lit_ = true;
    paint();
  }
}

}  // namespace esphome::desk_panel
