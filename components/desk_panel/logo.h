#pragma once
// The logos: which one a word stands for, its mask, and the mask at another size. Taken from source_keys.
#include <cstdint>
#include <string>

namespace esphome::desk_panel {

// "PC" is windows, "Mac" apple, "PS5" playstation, "Switch 2" nintendo-switch, ...; "" for none.
std::string guess_logo(const std::string &label);
// The LOGO_SIZE mask of a logo by its name (logos.h), or nullptr.
const uint8_t *logo_alpha(const char *name);
// The LOGO_SIZE mask src made side x side into dst (area mean, bilinear when it grows).
void resample(const uint8_t *src, int side, uint8_t *dst);

}  // namespace esphome::desk_panel
