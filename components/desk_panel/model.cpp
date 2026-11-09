#include "model.h"
#include <cstring>
#include <initializer_list>

namespace esphome::desk_panel {

bool is_running(const char *state) {
  if (state == nullptr) return false;
  for (const char *s : {"on", "home", "playing", "paused", "idle"})
    if (strcmp(state, s) == 0) return true;
  return false;
}

View choose_view(bool has_power, bool power_on, const bool running[3], const bool has_status[3]) {
  if (has_power) return power_on ? View::WORK : View::REST;
  for (int i = 0; i < 3; ++i)
    if (has_status[i] && running[i]) return View::WORK;
  return View::REST;
}

static void add(Layout &l, Key k, int x, int y, int w, int h) {
  if (l.count < int(sizeof(l.slots) / sizeof(l.slots[0]))) l.slots[l.count++] = Slot{k, Rect{x, y, w, h}};
}

// n keys side by side over w with gap between them, the widths within a pixel of each other; the row ends at w.
static void row(Layout &l, const Key *keys, int n, int y, int w, int h, int gap) {
  for (int i = 0; i < n; ++i) {
    const int x = i * (w + gap) / n, next = (i + 1) * (w + gap) / n;
    add(l, keys[i], x, y, next - gap - x, h);
  }
}

// Row heights in the parts given, gap between the rows; the last row takes the rest.
static void heights(int h, int gap, const int *parts, int n, int *out) {
  int sum = 0, used = 0;
  for (int i = 0; i < n; ++i) sum += parts[i];
  const int avail = h - (n - 1) * gap;
  for (int i = 0; i < n; ++i) {
    out[i] = i == n - 1 ? avail - used : avail * parts[i] / sum;
    used += out[i];
  }
}

static Key source(int k) { return Key(int(Key::SOURCE0) + k); }

Layout layout_rest(int w, int h, const Have &have, int gap) {
  Layout l;
  const int n = have.sources < REST_SOURCES ? have.sources : REST_SOURCES;
  const int cw = (w - gap) / 2;
  Key bottom[3];
  int m = 0;
  for (int i = 0; i < have.moods && i < 2; ++i) bottom[m++] = Key(int(Key::MOOD0) + i);
  if (have.device) bottom[m++] = Key::DEVICE;
  // A tall tile keeps its third row only for keys to put there; without, the sources take the height.
  const bool third = have.tall && m > 0;
  int rh[3];
  if (third) {
    const int parts[3] = {2, 2, 1};
    heights(h, gap, parts, 3, rh);
  } else {
    const int parts[2] = {1, 1};
    heights(h, gap, parts, 2, rh);
  }
  auto cell = [&](Key k, int i) {
    const int c = i % 2, r = i / 2;
    add(l, k, c ? cw + gap : 0, r ? rh[0] + gap : 0, c ? w - cw - gap : cw, rh[r]);
  };
  for (int k = 0; k < n; ++k) cell(source(k), k);
  if (third) {
    row(l, bottom, m, rh[0] + rh[1] + 2 * gap, w, rh[2], gap);
  } else if (have.device && n < REST_SOURCES) {
    cell(Key::DEVICE, n);
  }
  return l;
}

Layout layout_work(int w, int h, const Have &have, int gap) {
  Layout l;
  const int n = have.sources < MAX_SOURCES ? have.sources : MAX_SOURCES;
  const bool third = have.tall && have.moods > 0;
  int rh[3];
  if (third) {
    const int parts[3] = {4, 3, 2};
    heights(h, gap, parts, 3, rh);
  } else {
    const int parts[2] = {4, 3};
    heights(h, gap, parts, 2, rh);
  }
  Key keys[MAX_SOURCES];
  for (int k = 0; k < n; ++k) keys[k] = source(k);
  row(l, keys, n, 0, w, rh[0], gap);
  Key mid[4];
  int m = 0;
  if (have.picture) mid[m++] = Key::PICTURE;
  if (have.power) mid[m++] = Key::POWER;
  if (have.autokey) mid[m++] = Key::AUTO;
  if (have.device) mid[m++] = Key::DEVICE;
  row(l, mid, m, rh[0] + gap, w, rh[1], gap);
  if (third) {
    Key moods[2];
    int c = 0;
    for (int i = 0; i < have.moods && i < 2; ++i) moods[c++] = Key(int(Key::MOOD0) + i);
    row(l, moods, c, rh[0] + rh[1] + 2 * gap, w, rh[2], gap);
  }
  return l;
}

Key hit(const Layout &l, int x, int y) {
  for (int i = 0; i < l.count; ++i) {
    const Rect &r = l.slots[i].r;
    if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return l.slots[i].key;
  }
  return Key::NONE;
}

void Starting::begin(uint32_t now_ms) {
  since_ms = now_ms;
  on = true;
}

bool Starting::active(uint32_t now_ms, bool device_on) {
  if (on && (device_on || now_ms - since_ms >= STARTING_MS)) on = false;
  return on;
}

bool Starting::lit(uint32_t epoch) const { return epoch % 2 == 0; }

}  // namespace esphome::desk_panel
