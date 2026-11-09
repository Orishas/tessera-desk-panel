// Host tests of model.h and logo.h: sh tests/run.sh
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "model.h"
#include "logo.h"

using namespace esphome::desk_panel;

static const Slot *find(const Layout &l, Key k) {
  for (int i = 0; i < l.count; ++i)
    if (l.slots[i].key == k) return &l.slots[i];
  return nullptr;
}
static int count_sources(const Layout &l) {
  int n = 0;
  for (int i = 0; i < l.count; ++i)
    if (int(l.slots[i].key) >= int(Key::SOURCE0) && int(l.slots[i].key) < int(Key::SOURCE0) + 6) ++n;
  return n;
}
static Have full(int sources, bool tall = true) { return Have{sources, true, true, true, true, 2, tall}; }

static void test_running_states() {
  for (const char *s : {"on", "home", "playing", "paused", "idle"}) assert(is_running(s));
  for (const char *s : {"off", "standby", "unavailable", "unknown", ""}) assert(!is_running(s));
  assert(!is_running(nullptr));
}

static void test_view_follows_power() {
  const bool all[3] = {true, true, true}, none[3] = {false, false, false}, has[3] = {true, true, true};
  assert(choose_view(true, false, all, has) == View::REST);
  assert(choose_view(true, true, none, has) == View::WORK);
}

static void test_view_without_power() {
  const bool none[3] = {false, false, false}, one[3] = {false, true, false}, has[3] = {true, true, true};
  const bool only_first[3] = {true, false, false};
  assert(choose_view(false, false, none, has) == View::REST);
  assert(choose_view(false, false, one, has) == View::WORK);
  // A running flag of a source without a status option does not count.
  assert(choose_view(false, false, one, only_first) == View::REST);
}

static void test_layout_rest_tall_four() {
  const Layout l = layout_rest(480, 480, full(4), 8);
  assert(l.count == 7);
  const Slot *s0 = find(l, Key::SOURCE0);
  assert(s0 && s0->r.x == 0 && s0->r.y == 0 && s0->r.w == 236);
  assert(l.slots[l.count - 1].key == Key::DEVICE);
  const Rect d = l.slots[l.count - 1].r;
  assert(d.x + d.w == 480);
}

static void test_layout_rest_caps_at_four() {
  const Layout l = layout_rest(480, 480, full(6), 8);
  assert(count_sources(l) == 4);
  for (int k = 0; k < 4; ++k) assert(find(l, Key(int(Key::SOURCE0) + k)));
}

static void test_layout_work_no_auto() {
  Have h = full(4);
  h.autokey = false;
  const Layout l = layout_work(480, 480, h, 8);
  const Slot *p = find(l, Key::PICTURE), *w = find(l, Key::POWER), *d = find(l, Key::DEVICE);
  assert(p && w && d && !find(l, Key::AUTO));
  assert(p->r.y == w->r.y && w->r.y == d->r.y);
  assert(p->r.w - w->r.w <= 1 && w->r.w - p->r.w <= 1 && d->r.w - p->r.w <= 1 && p->r.w - d->r.w <= 1);
  assert(count_sources(layout_work(480, 480, full(6), 8)) == 6);
}

static void test_hit_gap_is_none() {
  const Layout l = layout_rest(480, 480, full(4), 8);
  const Slot *s0 = find(l, Key::SOURCE0);
  assert(hit(l, s0->r.x + s0->r.w + 3, s0->r.y + s0->r.h / 2) == Key::NONE);
  for (int i = 0; i < l.count; ++i) {
    const Rect r = l.slots[i].r;
    assert(hit(l, r.x + r.w / 2, r.y + r.h / 2) == l.slots[i].key);
  }
  assert(hit(l, -5, -5) == Key::NONE);
}

static void test_starting_ends_on_device_on() {
  Starting s;
  s.begin(1000);
  assert(s.active(5000, false));
  assert(!s.active(6000, true));
  assert(!s.active(7000, false));
}

static void test_starting_times_out() {
  Starting s;
  s.begin(0);
  assert(s.active(119999, false));
  assert(!s.active(120000, false));
}

static void test_starting_blocks_retap() {
  Starting s;
  assert(!s.active(10, false));   // not begun: a tap may act
  s.begin(10);
  assert(s.active(20, false));    // begun, device still off: the tile ignores a second tap
  assert(s.lit(2) && !s.lit(3));
}

static void test_guess_logo() {
  assert(strcmp(guess_logo("PC").c_str(), "windows") == 0);
  assert(strcmp(guess_logo("Mac").c_str(), "apple") == 0);
  assert(strcmp(guess_logo("PS5").c_str(), "playstation") == 0);
  assert(strcmp(guess_logo("Switch 2").c_str(), "nintendo-switch") == 0);
  assert(guess_logo("Radio").empty());
}

int main() {
  test_running_states();
  test_view_follows_power();
  test_view_without_power();
  test_layout_rest_tall_four();
  test_layout_rest_caps_at_four();
  test_layout_work_no_auto();
  test_hit_gap_is_none();
  test_starting_ends_on_device_on();
  test_starting_times_out();
  test_starting_blocks_retap();
  test_guess_logo();
  puts("all model tests passed");
  return 0;
}
