#include "logo.h"
#include "logos.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace esphome::desk_panel {

// The logo a word stands for, when the person gave none: "PC" is Windows, "Mac" Apple, and so on.
std::string guess_logo(const std::string &label) {
  std::string l;
  for (char ch : label) l += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  auto has = [&l](const char *w) { return l.find(w) != std::string::npos; };
  if (has("mac") || has("apple")) return "apple";
  if (has("ps") || has("playstation")) return "playstation";
  if (has("switch") || has("nintendo")) return "nintendo-switch";
  if (has("xbox")) return "xbox";
  if (has("pc") || has("windows")) return "windows";
  if (has("linux")) return "linux";
  return "";
}

// The mask of a logo by its name (logos.h), or none.
const uint8_t *logo_alpha(const char *name) {
  for (const Logo &l : LOGOS)
    if (strcmp(l.name, name) == 0) return l.alpha;
  return nullptr;
}

// A mask of LOGO_SIZE pixels a side, made `side` pixels a side: each pixel the mean of the source pixels it covers
// (bilinear when it grows). LVGL then draws it as it is, without a transform of its own.
void resample(const uint8_t *src, int side, uint8_t *dst) {
  for (int y = 0; y < side; ++y)
    for (int x = 0; x < side; ++x) {
      const float x0 = float(x) * LOGO_SIZE / side, x1 = float(x + 1) * LOGO_SIZE / side;
      const float y0 = float(y) * LOGO_SIZE / side, y1 = float(y + 1) * LOGO_SIZE / side;
      float sum = 0, area = 0;
      for (int sy = int(y0); sy < int(std::ceil(y1)) && sy < LOGO_SIZE; ++sy)
        for (int sx = int(x0); sx < int(std::ceil(x1)) && sx < LOGO_SIZE; ++sx) {
          const float w = (std::min(x1, float(sx + 1)) - std::max(x0, float(sx))) *
                          (std::min(y1, float(sy + 1)) - std::max(y0, float(sy)));
          sum += w * src[sy * LOGO_SIZE + sx];
          area += w;
        }
      dst[y * side + x] = area > 0 ? uint8_t(sum / area + 0.5f) : 0;
    }
}

}  // namespace esphome::desk_panel
