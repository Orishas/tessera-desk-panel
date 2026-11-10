# The Home Assistant side: a desk with an LG OLED as its monitor

The tile only shows states and sends taps. Everything that happens after a tap is Home Assistant's: this page shows
one way to build it, taken from a running desk with an LG C2 42" as the monitor and four sources (a PC, a Mac, a PS5
and a Switch 2). The entity names are examples; yours will differ. Take the parts you need: the tile works with the
select alone.

## The idea

```
 tile                          Home Assistant                              devices
 ───────────────────────────   ─────────────────────────────────────────   ─────────────────────
 source key        ──────────► script "show source" (field: source)  ───►  wake the device, wake
                               └─ sets input_select.desk_source            the TV, choose its input
 which key is lit  ◄────────── input_select.desk_source
 dot on a source   ◄────────── binary_sensor / media_player of the device
 Picture key       ◄─────────► switch.desk_monitor_picture (template) ──►  TV screen off / on
 Monitor key       ◄─────────► switch.desk_monitor (template)         ──►  TV off / on
 device key        ◄─────────► switch.desk_pc (template)              ──►  Wake-on-LAN / shut down
 Auto key          ◄─────────► input_boolean.<source>_auto
                               └─ read by the presence automation
```

Three rules keep it simple:

1. **One select holds the chosen source**, and nothing else does. The tile shows it, the script sets it, the
   automation reads it. When someone changes the input with the TV's remote, an automation writes it back.
2. **One script shows a source.** The tile, a button on the desk and the presence automation all call the same script,
   so they cannot disagree about what "show the Mac" means.
3. **A template switch for everything the tile switches.** The tile turns a switch on or off and shows its state. A
   template switch puts a script behind "on" and "off" and decides itself what "on" means.

## What you need

| What | Here | Why |
|---|---|---|
| The TV in Home Assistant | The **LG webOS TV** integration: `media_player.lg_c2` and its switch `switch.lg_c2_screen` | The media player turns the TV on and off and chooses the input. The screen switch turns only the picture off: the TV stays on the network and is back in a second. |
| A way to turn the TV on | The **Wake on LAN** integration, and on the TV the setting that lets it be turned on over the network | A TV that is off does not hear Home Assistant. See "Turning the TV on" below. |
| Whether each device runs | `binary_sensor.pc`, `binary_sensor.mac`, `media_player.ps5` | The dots on the tile, and the automation's decisions. A ping sensor, the PlayStation integration or a companion app on the computer all do. |
| A way to start and stop the PC | `button.wake_pc` (Wake on LAN) and `button.pc_shutdown` (a companion app on the PC such as HASS.Agent) | The device key. |
| Presence at the desk, optional | `binary_sensor.desk_presence` | Only for the automation that turns the monitor off when you leave. |

Name the TV's inputs on the TV itself (its input settings). The names you give them there are the ones in the media
player's `source_list`, and the ones `media_player.select_source` takes. Look them up in **Developer tools > States**.
An LG may rename an input by itself when a device introduces itself over HDMI-CEC ("Mac" one day, "MAC" the next):
the script below does not care about upper and lower case for that reason.

## 1. The select

**Settings > Devices & services > Helpers > Create helper > Dropdown.** Its options are the source keys, in the order
they show on the tile. A name with `PC`, `Mac`, `PS`, `Switch`, `Xbox` or `Linux` in it gets a logo.

```yaml
input_select:
  desk_source:
    name: Desk monitor source
    options: [PC, Mac, PS5, Switch 2]
```

## 2. Turning the TV on

An LG that is off can only be woken with a magic packet. The webOS integration has a trigger for exactly this: it
fires when something calls `media_player.turn_on` on the TV. So every other part of the setup simply turns the media
player on.

```yaml
automation:
  - alias: Desk - wake the LG
    triggers:
      - trigger: webostv.turn_on
        entity_id: media_player.lg_c2
    actions:
      - action: wake_on_lan.send_magic_packet
        data:
          mac: "aa:bb:cc:dd:ee:ff"   # the TV's
```

## 3. The scripts

**Wake the monitor.** The one way to make the picture visible: turn the TV on when it is off, then turn the screen
on when only the picture was off. Everything else calls this.

```yaml
script:
  desk_monitor_wake:
    alias: Desk - wake the monitor
    mode: single
    max_exceeded: silent
    sequence:
      - if:
          - condition: not
            conditions:
              - condition: state
                entity_id: media_player.lg_c2
                state: "on"
        then:
          - action: media_player.turn_on
            target: { entity_id: media_player.lg_c2 }
          - wait_template: "{{ is_state('media_player.lg_c2', 'on') }}"
            timeout: "00:01:00"
            continue_on_timeout: true
      - if:
          - condition: state
            entity_id: switch.lg_c2_screen
            state: "off"
        then:
          - action: switch.turn_on
            continue_on_error: true
            target: { entity_id: switch.lg_c2_screen }
```

**Show a source.** The script the tile runs (its **Script** option). The tile hands it the tapped option in the
field `source`. It sets the select first, so the tile lights the key at once, then does the slow part: wake the
device, wake the monitor, wait until the TV knows its inputs, choose the input.

```yaml
  desk_show_source:
    alias: Desk - show a source
    mode: restart          # a second tap on another source wins
    fields:
      source:
        name: Source
        required: true
        selector:
          select:
            options: [PC, Mac, PS5, Switch 2]
    sequence:
      - action: input_select.select_option
        target: { entity_id: input_select.desk_source }
        data: { option: "{{ source }}" }
      # A computer that is off is started first.
      - if:
          - "{{ source == 'PC' }}"
          - condition: state
            entity_id: binary_sensor.pc
            state: "off"
        then:
          - action: button.press
            target: { entity_id: button.wake_pc }
      - action: script.desk_monitor_wake
      # A TV that has just come on has no inputs for a moment.
      - wait_template: >-
          {{ is_state('media_player.lg_c2', 'on') and state_attr('media_player.lg_c2', 'source') is not none }}
        timeout: "00:00:30"
        continue_on_timeout: true
      # The TV's input whose name holds the word, whatever its case; chosen only when it is not showing already.
      - variables:
          word: "{{ {'PC': 'pc', 'Mac': 'mac', 'PS5': 'ps5', 'Switch 2': 'switch'}[source] }}"
          input: >-
            {{ (state_attr('media_player.lg_c2', 'source_list') or [])
               | select('search', '(?i)' ~ word) | first | default('') }}
      - if:
          - "{{ input != '' and not is_state_attr('media_player.lg_c2', 'source', input) }}"
        then:
          - action: media_player.select_source
            continue_on_error: true
            target: { entity_id: media_player.lg_c2 }
            data: { source: "{{ input }}" }
      # Whatever goes with a source: here the PC's speakers.
      - action: "switch.turn_{{ 'on' if source == 'PC' else 'off' }}"
        target: { entity_id: switch.pc_speaker }
```

The tile sends the option as `source` and as `quelle`, so a script may call its field either.

**Turn the monitor off.** For the Monitor key: the TV off, and what belongs to it.

```yaml
  desk_monitor_off:
    alias: Desk - monitor off
    sequence:
      - action: media_player.turn_off
        target: { entity_id: media_player.lg_c2 }
      - action: switch.turn_off
        target: { entity_id: switch.pc_speaker }
```

**Shut the PC down.** For the device key. It waits until the PC is really off before it turns off what hangs on it.

```yaml
  desk_pc_off:
    alias: Desk - PC off
    mode: single
    max_exceeded: silent
    sequence:
      - if:
          - condition: state
            entity_id: binary_sensor.pc
            state: "on"
        then:
          - action: button.press
            target: { entity_id: button.pc_shutdown }
      - wait_template: "{{ is_state('binary_sensor.pc', 'off') }}"
        timeout: "00:02:00"
        continue_on_timeout: true
      - action: switch.turn_off
        target: { entity_id: switch.pc_speaker }
```

## 4. The template switches

**Helpers > Create helper > Template > Switch**, or in YAML as here. Each has a template that says when it is on, and
what to do for on and off. They start their script with `script.turn_on`, which returns at once: the tile's key does
not wait for a script that takes a minute.

```yaml
template:
  - switch:
      # The Picture key: on while the TV is on and shows a picture.
      - name: Desk monitor picture
        state: "{{ is_state('media_player.lg_c2', 'on') and is_state('switch.lg_c2_screen', 'on') }}"
        turn_on:
          - action: script.turn_on
            target: { entity_id: script.desk_monitor_wake }
        turn_off:
          - action: switch.turn_off
            target: { entity_id: switch.lg_c2_screen }

      # The Monitor key, which also chooses the tile's view: off shows the large start keys.
      - name: Desk monitor
        state: "{{ is_state('media_player.lg_c2', 'on') }}"
        turn_on:
          - action: script.turn_on
            target: { entity_id: script.desk_monitor_wake }
        turn_off:
          - action: script.turn_on
            target: { entity_id: script.desk_monitor_off }

      # The device key. Its name is the word on the key.
      - name: PC
        state: "{{ is_state('binary_sensor.pc', 'on') }}"
        turn_on:
          - action: button.press
            target: { entity_id: button.wake_pc }
        turn_off:
          - action: script.turn_on
            target: { entity_id: script.desk_pc_off }
```

Why two switches for one TV: turning the picture off is what you want when you leave the desk for a few minutes (it
is back at once, and an OLED that shows nothing does not age); turning the TV off is for the evening.

## 5. The tile's options

| Option of the tile | Entity |
|---|---|
| The tile's entity | `input_select.desk_source` |
| Script | `script.desk_show_source` |
| State of the 1st to 4th source | `binary_sensor.pc`, `binary_sensor.mac`, `media_player.ps5`, and nothing for a console that tells Home Assistant nothing |
| Auto of the 1st to 4th source | `input_boolean.pc_auto`, `input_boolean.mac_auto`, `input_boolean.ps5_auto` (below) |
| Picture switch | `switch.desk_monitor_picture` |
| Monitor switch | `switch.desk_monitor` |
| Device | `switch.pc` |

## 6. Optional: the monitor follows you

"Auto" on the tile is a plain toggle helper per source (**Helpers > Toggle**). The tile only switches it and shows
it; what it means is this automation's business: *when I sit down, may this source turn the monitor on by itself?*

The automation has four jobs:

- **You sit down:** show the chosen source, if its Auto is on and its device runs.
- **You are away for 3 minutes:** turn only the picture off.
- **You are away for 15 minutes:** turn the TV off.
- **Someone changed the input on the TV** (remote, HDMI-CEC): write it back to the select, so the tile is right again.

```yaml
automation:
  - alias: Desk - the monitor follows presence
    mode: queued
    triggers:
      - trigger: state
        entity_id: binary_sensor.desk_presence
        to: "on"
        for: { seconds: 3 }
        id: arrived
      - trigger: state
        entity_id: binary_sensor.desk_presence
        to: "off"
        for: { minutes: 3 }
        id: left
      - trigger: state
        entity_id: binary_sensor.desk_presence
        to: "off"
        for: { minutes: 15 }
        id: left_long
      - trigger: state
        entity_id: media_player.lg_c2
        attribute: source
        for: { seconds: 15 }     # not while the script is still switching
        id: input_changed
    variables:
      chosen: "{{ states('input_select.desk_source') }}"
      # Per source: its Auto toggle and the entity that says its device runs.
      sources:
        PC: { auto: input_boolean.pc_auto, runs: binary_sensor.pc }
        Mac: { auto: input_boolean.mac_auto, runs: binary_sensor.mac }
        PS5: { auto: input_boolean.ps5_auto, runs: media_player.ps5 }
    actions:
      - choose:
          - conditions:
              - condition: trigger
                id: arrived
              - "{{ is_state('switch.desk_monitor_picture', 'off') }}"
              - "{{ chosen in sources }}"
              - "{{ is_state(sources[chosen].auto, 'on') }}"
              - "{{ states(sources[chosen].runs) in ['on', 'playing', 'idle', 'paused'] }}"
            sequence:
              - action: script.desk_show_source
                data: { source: "{{ chosen }}" }
          - conditions:
              - condition: trigger
                id: left
              - condition: state
                entity_id: switch.desk_monitor_picture
                state: "on"
            sequence:
              - action: switch.turn_off
                target: { entity_id: switch.desk_monitor_picture }
          - conditions:
              - condition: trigger
                id: left_long
              - condition: state
                entity_id: media_player.lg_c2
                state: "on"
            sequence:
              - action: script.desk_monitor_off
          - conditions:
              - condition: trigger
                id: input_changed
              - condition: state
                entity_id: media_player.lg_c2
                state: "on"
              - condition: state
                entity_id: script.desk_show_source
                state: "off"      # the script itself is not the one changing it
            sequence:
              - variables:
                  now_showing: "{{ state_attr('media_player.lg_c2', 'source') or '' }}"
                  # The option whose word is in the input's name, as in the script.
                  match: >-
                    {% set words = {'PC': 'pc', 'Mac': 'mac', 'PS5': 'ps5', 'Switch 2': 'switch'} %}
                    {% set found = namespace(option='') %}
                    {% for option, word in words.items() if now_showing is search('(?i)' ~ word) %}
                      {% set found.option = option %}
                    {% endfor %}
                    {{ found.option }}
              - if:
                  - "{{ match != '' and match != chosen }}"
                then:
                  - action: input_select.select_option
                    target: { entity_id: input_select.desk_source }
                    data: { option: "{{ match }}" }
```

The desk this comes from does a little more in the same automation: when the chosen Mac or PS5 goes off it falls back
to the PC if that runs (never the other way: nothing starts a computer by itself), and a light behind the monitor
follows the picture.

## Good to know

- **A key that goes back.** The tile shows a switch the other way at once and waits five seconds for Home Assistant to
  agree. An LG that was off takes longer than that to come on: the Monitor key goes back for a moment, then follows
  the TV once it is there.
- **Test each part on its own first.** Run the scripts from **Developer tools > Actions** with the field filled in,
  and flip the template switches on a dashboard, before you give them to the tile. Then a problem is either Home
  Assistant's or the tile's, not both.
- **`mode: restart`** on the script that shows a source matters: without it a second tap while the first is still
  waiting for the TV would be dropped.
- **A media player in `standby` counts as off** for the tile's dot (a PS5 in rest mode).
- The YAML here is the running setup with its names changed and its special cases left out. Read it as a plan, and
  check each entity and each input name against your own.
