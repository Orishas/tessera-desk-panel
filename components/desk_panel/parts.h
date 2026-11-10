#pragma once
// The LVGL parts the tile and the card both make, and setters that leave a part alone when nothing changes: LVGL
// draws a part again on every style it is given and on every flag set, whether or not the value is new.
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::desk_panel {

// A key's block: no padding, no scrolling; what lies in it is placed by hand.
inline lv_obj_t *key_block(lv_obj_t *parent) {
  lv_obj_t *b = tessera::ui::block(parent, theme::TRACK);
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}

inline void show(lv_obj_t *o, bool on) {
  if (!o || lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN) == !on) return;
  if (on) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
}

// A colour of the part's own style (its border, an image's recolour), set only when it is another one.
inline void set_style_color(lv_obj_t *o, lv_style_prop_t prop, lv_color_t color) {
  if (!o) return;
  lv_style_value_t was;
  if (lv_obj_get_local_style_prop(o, prop, &was, 0) == LV_STYLE_RES_FOUND && lv_color_eq(was.color, color)) return;
  lv_style_value_t now;
  now.color = color;
  lv_obj_set_local_style_prop(o, prop, now, 0);
}

}  // namespace esphome::desk_panel
