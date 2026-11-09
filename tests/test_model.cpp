// Host tests of model.h and logo.h: sh tests/run.sh
#include <algorithm>
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
static Have full(int sources, bool tall = true) { return Have{sources, true, true, true, true, tall}; }

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
  assert(l.count == 5);   // four sources two by two, the device under them over the whole width
  const Slot *s0 = find(l, Key::SOURCE0);
  assert(s0 && s0->r.x == 0 && s0->r.y == 0 && s0->r.w == 236);
  assert(l.slots[l.count - 1].key == Key::DEVICE);
  const Rect d = l.slots[l.count - 1].r;
  assert(d.x == 0 && d.w == 480 && d.y + d.h == 480);
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
  assert(s.lit(10) && s.lit(1009) && !s.lit(1010) && s.lit(2010));   // counted from the tap, not the clock
}

static void test_guess_logo() {
  assert(strcmp(guess_logo("PC").c_str(), "windows") == 0);
  assert(strcmp(guess_logo("Mac").c_str(), "apple") == 0);
  assert(strcmp(guess_logo("PS5").c_str(), "playstation") == 0);
  assert(strcmp(guess_logo("Switch 2").c_str(), "nintendo-switch") == 0);
  assert(guess_logo("Radio").empty());
}

static void test_layout_without_bottom_fills() {
  const Have bare{4, false, false, false, false, true};
  const Layout r = layout_rest(480, 480, bare, 8);
  int bottom = 0;
  for (int i = 0; i < r.count; ++i) bottom = r.slots[i].r.y + r.slots[i].r.h > bottom ? r.slots[i].r.y + r.slots[i].r.h : bottom;
  assert(bottom == 480);
  const Layout w = layout_work(480, 480, Have{4, true, true, false, false, true}, 8);
  bottom = 0;
  for (int i = 0; i < w.count; ++i) bottom = w.slots[i].r.y + w.slots[i].r.h > bottom ? w.slots[i].r.y + w.slots[i].r.h : bottom;
  assert(bottom == 480);
  // WORK without a key for the second row: the sources take the height, no empty band.
  const Layout s = layout_work(480, 480, bare, 8);
  assert(s.count == 4);
  for (int i = 0; i < s.count; ++i) assert(s.slots[i].r.y == 0 && s.slots[i].r.h == 480);
}

static void test_relayout_on_auto_change() {
  const Have mac{4, true, true, true, true, true}, sw{4, true, true, false, true, true};
  assert(needs_relayout(mac, View::WORK, sw, View::WORK));
  assert(needs_relayout(sw, View::WORK, mac, View::WORK));
  assert(!needs_relayout(mac, View::WORK, mac, View::WORK));
  assert(needs_relayout(mac, View::WORK, mac, View::REST));
  Have six = mac;
  six.sources = 6;
  assert(needs_relayout(mac, View::WORK, six, View::WORK));
}

static void test_layout_empty_without_sources() {
  const Have none{0, true, true, true, true, true};
  assert(layout_rest(480, 480, none, 8).count == 0);
  assert(layout_work(480, 480, none, 8).count == 0);
  assert(hit(layout_work(480, 480, none, 8), 240, 400) == Key::NONE);
}

static void test_source_lit_rest_pending() {
  assert(source_lit(View::REST, 2, 2, 1));     // REST: the tapped key answers at once
  assert(!source_lit(View::REST, 1, -1, 1));   // REST: the chosen one is not marked
  assert(source_lit(View::WORK, 1, -1, 1));    // WORK: the chosen one
  assert(!source_lit(View::WORK, 0, -1, -1));  // WORK: unknown choice marks none
}

static void test_layout_one_row_only_sources() {
  Have one{4, true, true, true, true, false};
  one.rows = 1;
  const Layout r = layout_rest(480, 160, one, 8), w = layout_work(480, 160, one, 8);
  assert(r.count == 4 && w.count == 4);
  for (int i = 0; i < 4; ++i) {
    assert(r.slots[i].r.y == 0 && r.slots[i].r.h == 160);
    assert(w.slots[i].r.y == 0 && w.slots[i].r.h == 160);
  }
  assert(r.slots[3].r.x + r.slots[3].r.w == 480);
}

static void test_layout_two_rows_fits_everything() {
  Have two{4, true, true, true, true, false};
  two.rows = 2;
  const Layout r = layout_rest(480, 310, two, 8);
  // REST: the four sources side by side on top, the device under them over the whole width.
  const Slot *s0 = find(r, Key::SOURCE0), *s3 = find(r, Key(3));
  const Slot *d = find(r, Key::DEVICE);
  assert(r.count == 5 && s0 && s3 && d);
  assert(s0->r.y == 0 && s3->r.y == 0 && s3->r.x + s3->r.w == 480);
  assert(d->r.y > s0->r.y + s0->r.h && d->r.x == 0 && d->r.w == 480 && d->r.y + d->r.h == 310);
  // REST without a device: two by two over the whole height.
  Have plain = two;
  plain.device = false;
  const Layout g = layout_rest(480, 310, plain, 8);
  assert(g.count == 4 && find(g, Key(3))->r.y + find(g, Key(3))->r.h == 310);
  // WORK: two rows, sources and the four keys.
  const Layout w = layout_work(480, 310, two, 8);
  assert(w.count == 8 && find(w, Key::DEVICE) && find(w, Key::AUTO));
  int bottom = 0;
  for (int i = 0; i < w.count; ++i) bottom = std::max(bottom, w.slots[i].r.y + w.slots[i].r.h);
  assert(bottom == 310);
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
  test_layout_without_bottom_fills();
  test_relayout_on_auto_change();
  test_layout_empty_without_sources();
  test_source_lit_rest_pending();
  test_layout_one_row_only_sources();
  test_layout_two_rows_fits_everything();
  puts("all model tests passed");
  return 0;
}
