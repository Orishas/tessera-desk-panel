# Schreibtisch (`desk_panel`) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A full-page Tessera tile that switches between a "Ruhe" and an "Arbeit" view by the monitor's power, with
source keys, status dots, Auto, a PC power key with a confirm card, and two mood keys.

**Architecture:** The pure decisions live in `model.h/.cpp`, which has no LVGL and no Tessera. That covers which view
shows, what "running" means, the key rectangles and which key a point hits, and when "Startet …" pulses. Host unit
tests in `tests/` cover it. `desk_tile.cpp` draws and acts, using the model. `confirm_card.cpp` is the card.
`logo.h/.cpp` holds the logo helpers taken from `source_keys`.

**Tech Stack:** C++17, ESPHome external component, Tessera plugin API 0.24 (LVGL 9 via `tessera::ui`), host tests
with `c++ -std=c++17` and plain `assert`, the bench from `../tessera-plugins/tools`.

**Spec:** `docs/superpowers/specs/2026-11-09-desk-panel-design.md` (read it first; this plan does not repeat it)

## Global Constraints

- Manifest `api: "0.24"`, `id: desk_panel`, tile id `desk`, card id `confirm`, `sizes: { min: 2x2, max: 2x3 }`,
  `domains: [input_select, select]`, `attributes: [options]`, `boards: any`, `license: MIT`, `maintainer: Orishas`,
  `stage: beta`.
- **Exactly these 12 options, in this order:**
  1. `script`
  2. `status_1`, `status_2`, `status_3`
  3. `auto_1`, `auto_2`, `auto_3`
  4. `picture`, `power`, `device`
  5. `mood_1`, `mood_2`
- **The option kinds:**
  - `status_*`: `kind: entity` with domains `[binary_sensor, media_player, switch, device_tracker]`.
  - `auto_*`: domains `[input_boolean, switch]`.
  - `picture` and `power`: `[switch]`.
  - `device`: `[switch, input_boolean]`.
  - `mood_*`: `[scene, script]`.
  - `script`: `[script]`.
- `permissions.home_assistant_actions`: `script.turn_on`, `input_select.select_option`, `select.select_option`,
  `switch.turn_on`, `switch.turn_off`, `input_boolean.turn_on`, `input_boolean.turn_off`, `scene.turn_on`.
- The script field is always `quelle`: `{variables: {quelle: <option>}}`.
- "Running" means the state is `on`, `home`, `playing`, `paused` or `idle`. Everything else, `standby` included,
  counts as off.
- "Startet …" lasts 120 s after the ⏻ key turned the device on, and ends at once when `device` reports `on`. It
  pulses by `on_tick` only: no `lv_anim`, no timer.
- No `on_hold` (tiles have none).
- Every word on the glass comes from `translations/{en,de}.json`.
- Commits: author `Orishas <6690749+Orishas@users.noreply.github.com>`. Every message ends with
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- After every change: `python3 ../tessera-plugins/tools/check.py .` prints `desk_panel: ok`.
- Bench boards: `hosyond40`, `jc8012p4a1`, `cyd` (the Guition does not build on the bench). Run the venv python
  `../tessera-plugins/tools/.cache/preview-venv/bin/python`.

## Review Focus

1. **A select with more than 4 options.** Ruhe shows only the first 4; Arbeit shows up to 6. Nothing overlaps. This
   is tested in Task 2, `test_layout_rest_caps_at_four`.
2. **The chosen option is not in the list** (it was renamed in HA). No key is ACCENT, and nothing crashes. This is a
   bench case in Task 3, `preview.yaml` "Unknown choice".
3. **An entity option points to an entity that is `unavailable`.** The status dot is off, the Auto key is off and
   the ⏻ key reads off. Covered in Task 2 by `test_running_states` (`unavailable` → false).
4. **A tap in the gap between keys.** It does nothing: `hit` returns `Key::NONE`. Tested in Task 2,
   `test_hit_gap_is_none`.
5. **⏻ tapped again while "Startet …".** It sends nothing a second time and does not open the card (the device is
   still off). Tested in Task 2, `test_starting_blocks_retap`.

---

### Task 1: Scaffold that builds and checks

**Files:**
- Create: `tessera-plugin.yaml`, `plugin.yaml`, `LICENSE`, `README.md`, `CHANGELOG.md`, `.gitignore` (`.preview/`,
  `tests/run`)
- Create: `components/desk_panel/__init__.py`, `desk_panel.h`, `desk_panel.cpp`
- Copy from `../tessera-source-keys`: `components/source_keys/logos.h` → `components/desk_panel/logos.h`, and
  `tools/make_logos.py` and `tools/svg/`. In the copied `logos.h` and `make_logos.py`, change the namespace
  `source_keys` to `desk_panel`.
- Create: `translations/en.json`, `translations/de.json`

**Interfaces:**
- Produces: `class DeskPanel : public Component, public tessera::Plugin` in namespace `esphome::desk_panel`, whose
  `setup()` calls `add_tile("desk", …)` and `add_card("confirm", …)`.
- Produces: text keys
  - screen: `choose_select`, `picture`, `monitor`, `auto`, `starting`, `shutdown_q` ("{name} herunterfahren?"),
    `cancel`, `shutdown`
  - app: `name`, `summary`, `tile_name`, `tile_example`, `card_name`, plus a label and a `_hint` for every option

- [ ] **Step 1: Write the manifest as the Global Constraints say**
  - `memory: 4200`: 6 source blocks, 4 middle keys, 2 moods and 1 device, each a block with a label and an icon;
    6 logo images; 6 dots; 3 "A" labels.
  - `cards: [{ id: confirm, name: card_name }]`
  - `__init__.py` and `.h` as in `source_keys`, renamed.
  - The tile draws only the `choose_select` note for now.
- [ ] **Step 2: Run** `python3 ../tessera-plugins/tools/check.py .`. Expected: `desk_panel: ok`.
- [ ] **Step 3: Commit** with the message `Scaffold desk_panel`.

### Task 2: The model, tested on the host

**Files:**
- Create: `components/desk_panel/model.h`, `components/desk_panel/model.cpp`, `components/desk_panel/logo.h`,
  `components/desk_panel/logo.cpp`
  - `logo.*` take `guess_logo`, `logo_alpha` and `resample` from `source_keys`, unchanged.
- Create: `tests/test_model.cpp`, `tests/run.sh`
  - `run.sh` runs: `c++ -std=c++17 -Wall -Werror -I components/desk_panel tests/test_model.cpp components/desk_panel/model.cpp components/desk_panel/logo.cpp -o tests/run && tests/run`

**Interfaces (Produces, all in `namespace esphome::desk_panel`, `model.h` has no LVGL and no Tessera include):**

```cpp
enum class View { REST, WORK };
enum class Key : int { NONE = -1, SOURCE0 = 0 /* .. SOURCE0+5 */, PICTURE = 10, POWER, AUTO, DEVICE, MOOD0, MOOD1 };
struct Rect { int x, y, w, h; };
struct Slot { Key key; Rect r; };
struct Layout { Slot slots[16]; int count = 0; };

bool is_running(const char *state);                       // Global Constraints list; nullptr -> false
View choose_view(bool has_power, bool power_on, const bool running[3], const bool has_status[3]);
// What the person has, for laying out:
struct Have { int sources; bool picture, power, autokey, device; int moods; bool tall; };  // tall = 3 rows
Layout layout_rest(int w, int h, const Have &have, int gap);
Layout layout_work(int w, int h, const Have &have, int gap);
Key hit(const Layout &l, int x, int y);
struct Starting {                                          // the ⏻ key's "Startet …"
  uint32_t since_ms = 0; bool on = false;
  void begin(uint32_t now_ms);
  bool active(uint32_t now_ms, bool device_on);            // clears itself after 120000 ms or once device_on
  bool lit(uint32_t epoch) const;                           // pulse phase: true on even seconds
};
```

**Layout rules** (the spec's sketch, made exact):
- **Margin:** none, since the card has its padding.
- **REST, tall:**
  - The rows split the height 2 : 2 : 1.
  - Sources take a 2-column grid over rows 1 and 2, at most 4.
  - Row 3 holds moods and device, in equal widths.
- **REST, not tall:**
  - Sources fill the 2×2 grid.
  - With 3 or fewer sources, the free cell takes the device.
- **WORK, tall:**
  - The rows split the height 2 : 1.5 : 1.
  - Row 1 holds the sources, at most 6, in equal widths.
  - Row 2 holds picture, power, auto and device, as far as `have` has them, in equal widths.
  - Row 3 holds the moods.
- **WORK, not tall:** rows 1 and 2 only, with the heights 2 : 1.5.
- Keys of one row have `gap` between them, and rows have `gap` between them.

- [ ] **Step 1: Write the failing tests** in `tests/test_model.cpp`:
  - `test_running_states`
    - true: `on`, `home`, `playing`, `paused`, `idle`
    - false: `off`, `standby`, `unavailable`, `unknown`, `""`, `nullptr`
  - `test_view_follows_power`
    - power set: `power_on` false → REST, true → WORK, whatever the running flags say
  - `test_view_without_power`
    - power not set: REST when every `has_status` entry is not running, WORK if any is running
  - `test_layout_rest_tall_four`
    - 480×480, 4 sources, 2 moods, device, gap 8 → 7 slots
    - SOURCE0 starts at (0,0) and is 236 px wide
    - DEVICE is the last slot and its right edge is 480
  - `test_layout_rest_caps_at_four`
    - 6 sources → the source slots are exactly SOURCE0 to SOURCE3
  - `test_layout_work_no_auto`
    - `autokey` false → row 2 has 3 slots (PICTURE, POWER, DEVICE) of equal width (±1)
  - `test_hit_gap_is_none`
    - a point in the gap between SOURCE0 and SOURCE1 → `Key::NONE`
    - the center of each slot → its key
  - `test_starting_ends_on_device_on`
    - `begin(1000)`; `active(5000, false)` is true; `active(6000, true)` is false and stays false
  - `test_starting_times_out`
    - `begin(0)`; `active(119999, false)` is true; `active(120000, false)` is false
  - `test_starting_blocks_retap`
    - while active, the tile's rule "⏻ tap does nothing when starting" holds
    - expressed as `active(...)` being true; Task 4 calls it before acting
  - `test_guess_logo`
    - `"PC"` → `windows`, `"Mac"` → `apple`, `"PS5"` → `playstation`, `"Switch 2"` → `nintendo-switch`, `"Radio"` → `""`
- [ ] **Step 2: Run** `sh tests/run.sh`. Expected: it does not compile, because `model.h` is missing.
- [ ] **Step 3: Implement** `model.cpp` and `logo.cpp` to the signatures above.
- [ ] **Step 4: Run** `sh tests/run.sh`. Expected: `all model tests passed`, exit code 0.
- [ ] **Step 5: Run** `check.py`, expecting `ok`. **Commit** with the message `Model: views, layout, hit, starting`.

### Task 3: The tile draws both views

**Files:**
- Create: `components/desk_panel/desk_tile.h`, `desk_tile.cpp` (class `DeskTile : public tessera::Tile`)
- Modify: `desk_panel.cpp`, to register `DeskTile`
- Create: `preview.yaml`

**Interfaces:**
- Consumes: Task 2's `choose_view`, `layout_rest`, `layout_work`, `is_running`, `guess_logo`, `logo_alpha`,
  `resample`.
- Produces, for Task 4:
  - `DeskTile` members `Layout layout_`, `View view_`, `bool status_on_[3]`, `auto_on_[3]`, `picture_on_`,
    `power_on_`, `device_on_`, `int mood_chosen_ = -1`
  - `Starting starting_`
  - `void paint()`, which re-applies fills and colours without re-layout
  - `void build()`, which lays out and creates or hides blocks for the current view

- [ ] **Step 1: Write `preview.yaml` first.**
  - Entity `input_select.monitor_source` with the options `PC, Mac, PS5, Switch 2`.
  - Status entities, auto `input_boolean`s, the picture and power switches, `switch.pc` (device), and
    `scene.focus` and `scene.off` (friendly names "Fokus" and "Aus").
  - Tiles, each at 2x3 unless noted:
    1. "Ruhe": power off, PC status on, PC and Mac auto on.
    2. "Arbeit": power on, choice Mac, Mac auto on.
    3. "Arbeit Switch": choice `Switch 2`.
    4. "Unknown choice": choice `Xbox`.
    5. "Six sources": 6 options.
    6. "Bare": the select only.
    7. "Small": 2x2, both views.
    8. "No entity".
  - `cards: [{ card: confirm, entity: switch.pc }]`
- [ ] **Step 2: Run** `preview.py . --strict --board hosyond40`. Expected: warnings or blank tiles, because nothing
  is drawn yet.
- [ ] **Step 3: Implement `create`, `on_state`, `on_theme`, `build` and `paint`.**
  - `create`:
    - reads the options into `tessera::Text` (PSRAM).
    - makes the most blocks either view needs, once: 6 sources, 4 middle keys, 2 moods, 1 device. Each block gets a
      logo image, an icon label and a word label. Each source also gets a dot block and an "A" label.
  - `on_state`:
    - reads `state` and `attributes.options`.
    - reads `data["entities"][<option>]["state"]` for the status, auto, picture, power and device options.
    - reads the friendly names of `device` and the moods.
    - calls `build()` when the view or the source count changed, and `paint()` otherwise.
  - **Logo sizes:**
    - REST: 45 % of the key's shorter side.
    - WORK: as `source_keys` draws a source key.
    - Resample only when the side changed, kept per key as in `source_keys`.
  - **Dot:**
    - `ui::mm(2)`, round (`lv_obj_set_style_radius` to `LV_RADIUS_CIRCLE`).
    - Top right with an inset of `ui::px(8)`.
    - ACCENT, or INK on an ACCENT key. Hidden when the source is not running or has no status option.
  - **"A":**
    - `Font::BODY`, `theme::MUTED`, bottom right, REST only.
    - Shown when that source's auto is on.
  - **Icons:**
    - picture `0xF0208` (eye), power `0xF0425`, auto `0xF18F2`, device `0xF0425`.
    - Moods get no icon, only a word.
  - **Words:**
    - Use `plugin_->text(...)`.
    - The device word is the friendly name of `device`.
    - It shows `starting` while `starting_.active`.
- [ ] **Step 4: Run** the bench on `hosyond40`, `jc8012p4a1` and `cyd`. Expected: it ends without a warning.
  - Open every `tile-*` picture, light and dark.
  - Ruhe shows 4 big logos with dots and "A", and the moods and ⏻ at the bottom.
  - Arbeit shows the row, Bild · Monitor · Auto · ⏻, and the moods.
  - "Arbeit Switch" has no Auto.
  - "Unknown choice" has no ACCENT source.
  - "Six sources" shows 4 in Ruhe and 6 in Arbeit.
- [ ] **Step 5: Run** `check.py`, expecting `ok`. **Commit** with the message `Tile: Ruhe and Arbeit views`.

### Task 4: Taps, "Startet …" and the confirm card

**Files:**
- Modify: `desk_tile.cpp` (`on_tap`, `on_tick`)
- Create: `components/desk_panel/confirm_card.h`, `confirm_card.cpp` (class `ConfirmCard : public tessera::Card`)
- Modify: `desk_panel.cpp`, to register the card
- Modify: `preview.yaml`, to add the tile "Starting", which is like "Ruhe" with `switch.pc` off. The bench's tap on it
  records the action.

**Interfaces:**
- Consumes: Task 2's `hit` and `Starting`, and Task 3's members, `paint()` and `build()`.
- Produces: the card id `confirm`, opened with
  `tessera::open_card(plugin_id(), "confirm", <device entity>, tile_)` and a title of the device's friendly name.

- [ ] **Step 1: Implement `DeskTile::on_tap`.**
  - `hit(layout_, tap_point())`, then act by the spec's tap table.
  - **Source:** with a script, `tessera::action("script.turn_on", script, {variables: {quelle: <option>}})`; without
    one, `select_option` on the tile's entity, as in `source_keys`. Mark the key chosen at once (pending 15 s, as in
    `source_keys`).
  - **PICTURE, POWER and AUTO:** turn on or off by the known state, with the domain from the entity's prefix. Flip the
    known state at once and call `paint()`. AUTO acts on `auto_[index of chosen source]`.
  - **MOOD:** `scene.turn_on` or `script.turn_on` by prefix. Set `mood_chosen_` and call `paint()`.
  - **DEVICE:**
    - If `starting_.active(millis(), device_on_)`, do nothing.
    - Else if the device is on, open `confirm`.
    - Else turn it on, call `starting_.begin(millis())` and `paint()`.
- [ ] **Step 2: Implement `on_tick`.**
  - If `starting_.active(millis(), device_on_)`, set the ⏻ fill to ACCENT when `starting_.lit(epoch)` and to TRACK
    otherwise, with the word `starting`.
  - Once it is no longer active, call `paint()` once.
- [ ] **Step 3: Implement `ConfirmCard`.**
  - `open`:
    - The question `shutdown_q` with `{name}` replaced by the title, in `Font::BODY_LARGE`, centred in the upper
      half.
    - Two blocks side by side in the lower half: `cancel` on TRACK, `shutdown` on ACCENT, each at least `ui::mm(12)`
      high.
  - `on_tap`:
    - On `cancel`: `tessera::close_card()`.
    - On `shutdown`: `turn_off` on the entity (by prefix), then `close_card()`.
    - Anywhere else: nothing.
- [ ] **Step 4: Run** the bench on all three boards. Expected: no warning.
  - `taps[]` of "Starting" lists `switch.turn_on` on `switch.pc`. The tap picture shows "Startet …".
  - `card-confirm-light.png` and `card-confirm-dark.png` show the question and both buttons.
  - `tap-*` of "Arbeit" lists an action, which tap depends on where the bench taps.
- [ ] **Step 5: Run** `sh tests/run.sh` and `check.py`, expecting both green. **Commit** with the message
  `Taps, Startet, confirm card`.

### Task 5: Docs, store picture, publish

**Files:**
- Modify: `README.md`, with set-up steps like those of `source_keys`: the 12 options, that the script field is
  `quelle`, that the view follows the monitor's power switch, and the logos list.
- Modify: `CHANGELOG.md`, with `0.1.0 - 2026-11-09`.
- Create: `store.png`, made as for `source_keys` from the bench's Ruhe and Arbeit pictures, with
  `../tessera-source-keys` `scratchpad/mk.py`'s approach: `store_image.picture(...)` with our own composed images.

- [ ] **Step 1: Write** the README and CHANGELOG, then run the bench once more on all three boards with `--strict`.
  Expected: no warning.
- [ ] **Step 2: Make** `store.png` at 1280×720 and look at it.
- [ ] **Step 3: Commit** with the message `README, changelog, store picture`.
- [ ] **Step 4: Publish.**
  - `gh repo create Orishas/tessera-desk-panel --public --source . --push`.
  - Expected: the repo URL is printed, and `git log origin/main -1` equals the local HEAD.

### Task 6: Home Assistant (independent of Tasks 1–5)

Use the ha-mcp tools with the current BestPracticeKey and `MandatoryBPS=false`.

- [ ] **Step 1: Change `switch.buro_pc`**, the template helper with the entry `01M6ZTPFS5REGWMMKWQFWBNF78`, through its
  options flow.
  - `turn_on` becomes `[{action: button.press, target: {entity_id: button.wake_on_lan_c8_7f_54_6a_46_fe}}, {action: switch.turn_on, target: {entity_id: switch.pc_speaker}}]`.
  - `turn_off` and `value_template` stay as they are.
  - **Verify:** read the entry back. The state is unchanged, and `script.buro_pc_einschalten` is untouched (same
    `config_hash` `34b81d7cd3d5c01d`).
- [ ] **Step 2: Create `scene.buro_fokus` and `scene.buro_aus`.**
  - `scene.buro_fokus`:
    - `light.deckenleuchte_buro_weiss` on at brightness 100 % and `color_temp_kelvin` 4000.
    - `light.deckenleuchte_buro_ring` off.
    - `light.filamentlampe_buro` off.
  - `scene.buro_aus`: all three lights off.
  - Ask the user before activating either: it changes real lights.
  - **Verify:** read both configs back.

### Task 7: On the screen

- [ ] **Step 1: Add the plugin in Tessera.** Plugins → Mit Link hinzufügen → `https://github.com/Orishas/tessera-desk-panel`
  → Plugin lesen.
  - The user ticks the consent and confirms the build for Desk Control. Wait for the build.
- [ ] **Step 2: Swap the tile on page 1.**
  - Put `desk_panel.desk` at 2x3 on page 1 in place of the `source_keys` tile, Ambilight and Lautsprecher. Ask the user
    first; the speaker and Ambilight tiles move to page 2.
  - Set the 12 options as the spec's "Bei dir" column says, with the auto switches copied from the old tile's
    inspector.
  - Save and send.
- [ ] **Step 3: Verify.**
  - `sensor.esp_screens_desk_control` lists the tile.
  - With the user at the screen, check: both views (turn the monitor off and on), the dots, a source tap, the Auto
    tap and ⏻ on.
  - ⏻ off (shutdown) only with the user's explicit OK.
