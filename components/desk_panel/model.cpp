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

View choose_view(bool has_power, bool power_on, const bool running[WITH_OPTIONS],
                 const bool has_status[WITH_OPTIONS]) {
  if (has_power) return power_on ? View::WORK : View::REST;
  for (int i = 0; i < WITH_OPTIONS; ++i)
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

// A tile one row high (2x1): the sources in a row, nothing else fits there.
static Layout one_row(int w, int h, int n, int gap) {
  Layout l;
  Key keys[MAX_SOURCES];
  for (int k = 0; k < n; ++k) keys[k] = source(k);
  row(l, keys, n, 0, w, h, gap);
  return l;
}

Layout layout_rest(int w, int h, const Have &have, int gap) {
  Layout l;
  if (have.sources <= 0) return l;   // nothing shows, so nothing takes a tap
  const int n = have.sources < MAX_SOURCES ? have.sources : MAX_SOURCES;
  if (have.rows == 1) return one_row(w, h, n, gap);
  const Key device[1] = {Key::DEVICE};
  Key keys[MAX_SOURCES];
  for (int k = 0; k < n; ++k) keys[k] = source(k);
  // Two rows high (2x2, the largest a plugin's tile gets on a 2x3 grid) with a device: the sources side by side on
  // top, the device's key under them over the whole width.
  if (!have.tall() && have.device) {
    const int parts[2] = {2, 1};
    int two[2];
    heights(h, gap, parts, 2, two);
    row(l, keys, n, 0, w, two[0], gap);
    row(l, device, 1, two[0] + gap, w, two[1], gap);
    return l;
  }
  // Two rows of sources: two in a row up to four, three with five or six; the second row's keys share the width.
  // A tall tile has a third row for the device; without a device the sources take the height.
  const bool third = have.tall() && have.device;
  const int cols = n <= 4 ? 2 : 3, first = n < cols ? n : cols;
  int rh[3];
  if (third) {
    const int parts[3] = {2, 2, 1};
    heights(h, gap, parts, 3, rh);
  } else {
    const int parts[2] = {1, 1};
    heights(h, gap, parts, 2, rh);
  }
  row(l, keys, first, 0, w, rh[0], gap);
  if (n > first) row(l, keys + first, n - first, rh[0] + gap, w, rh[1], gap);
  if (third) row(l, device, 1, rh[0] + rh[1] + 2 * gap, w, rh[2], gap);
  return l;
}

Layout layout_work(int w, int h, const Have &have, int gap) {
  Layout l;
  if (have.sources <= 0) return l;   // nothing shows, so nothing takes a tap
  const int n = have.sources < MAX_SOURCES ? have.sources : MAX_SOURCES;
  if (have.rows == 1) return one_row(w, h, n, gap);
  Key mid[4];
  int m = 0;
  if (have.picture) mid[m++] = Key::PICTURE;
  if (have.power) mid[m++] = Key::POWER;
  if (have.autokey) mid[m++] = Key::AUTO;
  if (have.device) mid[m++] = Key::DEVICE;
  if (m == 0) return one_row(w, h, n, gap);   // no key for a second row: the sources take the height
  const int parts[2] = {4, 3};
  int rh[2];
  heights(h, gap, parts, 2, rh);
  Key keys[MAX_SOURCES];
  for (int k = 0; k < n; ++k) keys[k] = source(k);
  row(l, keys, n, 0, w, rh[0], gap);
  row(l, mid, m, rh[0] + gap, w, rh[1], gap);
  return l;
}

Key hit(const Layout &l, int x, int y) {
  for (int i = 0; i < l.count; ++i) {
    const Rect &r = l.slots[i].r;
    if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return l.slots[i].key;
  }
  return Key::NONE;
}

bool needs_relayout(const Have &was, View was_view, const Have &now, View now_view) {
  return was_view != now_view || !(was == now);
}

bool source_lit(View view, int i, int pending, int active) {
  return view == View::REST ? i == pending : i == active;
}

bool source_framed(View view, int i, int pending, int active) {
  return view == View::REST && pending < 0 && i >= 0 && i == active;
}

const char *switch_service(const char *entity, bool on) {
  const bool boolean = entity != nullptr && strncmp(entity, "input_boolean.", 14) == 0;
  if (boolean) return on ? "input_boolean.turn_on" : "input_boolean.turn_off";
  return on ? "switch.turn_on" : "switch.turn_off";
}

bool unconfirmed(uint32_t now_ms, uint32_t at_ms) { return now_ms - at_ms >= CONFIRM_MS; }

void Starting::begin(uint32_t now_ms) {
  since_ms = now_ms;
  on = true;
}

bool Starting::active(uint32_t now_ms, bool device_on) {
  if (on && (device_on || now_ms - since_ms >= STARTING_MS)) on = false;
  return on;
}

bool Starting::lit(uint32_t now_ms) const { return (now_ms - since_ms) / 1000 % 2 == 0; }

}  // namespace esphome::desk_panel
