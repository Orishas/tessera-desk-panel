#pragma once
// The tile's decisions, without LVGL and without Tessera, so the host tests (tests/run.sh) can check them: which view
// shows, what "running" means, where every key sits and which key a point hits, and when "Starting ..." pulses.
#include <cstdint>

namespace esphome::desk_panel {

enum class View { REST, WORK };
// SOURCE0 + k is the k-th source (k < 6).
enum class Key : int { NONE = -1, SOURCE0 = 0, PICTURE = 10, POWER, AUTO, DEVICE, MOOD0, MOOD1 };
struct Rect { int x, y, w, h; };
struct Slot { Key key; Rect r; };
struct Layout { Slot slots[16]; int count = 0; };

static constexpr int MAX_SOURCES = 6;
static constexpr int REST_SOURCES = 4;
static constexpr uint32_t STARTING_MS = 120000;

// A state that says the device runs: on, home, playing, paused or idle. standby, unavailable and the rest do not.
bool is_running(const char *state);
// REST while the monitor's power is off; without a power switch, while no source with a status option runs.
View choose_view(bool has_power, bool power_on, const bool running[3], const bool has_status[3]);

// What the person has, for laying out. autokey: the chosen source has an Auto switch. tall: the tile is three rows.
struct Have { int sources; bool picture, power, autokey, device; int moods; bool tall; };
Layout layout_rest(int w, int h, const Have &have, int gap);
Layout layout_work(int w, int h, const Have &have, int gap);
Key hit(const Layout &l, int x, int y);
// The layout must be made again: another view, another number of sources, or Auto that comes or goes.
bool needs_relayout(const Have &was, View was_view, const Have &now, View now_view);
// Whether source i is marked: in REST only the key just tapped (an answer to the finger), in WORK the chosen one.
bool source_lit(View view, int i, int pending, int active);

// The device key's "Starting ...": from a tap that turned the device on until it reports on, at most STARTING_MS.
struct Starting {
  uint32_t since_ms = 0;
  bool on = false;
  void begin(uint32_t now_ms);
  bool active(uint32_t now_ms, bool device_on);   // ends itself
  bool lit(uint32_t epoch) const;                  // the pulse: lit on even seconds
};

}  // namespace esphome::desk_panel
