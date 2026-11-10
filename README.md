# Desk panel

One full-page tile for a desk. With the monitor off it shows large keys that start a source. With the monitor on it
shows the sources in a row with a dot for each device that runs, the monitor's keys, an **Auto** key for the chosen
source and a key for a device such as a PC.

- **A source key** runs your script with the option in the field `quelle`. Without a script it chooses the option
  of the select directly.
- **The device key** turns the device on and says "Starting …" with a blinking dot until the device is there (at most two minutes). A dot on the key says the device is on.
  While the device is on, the key asks first, on a card: Cancel or Shut down.
- **The view follows the monitor switch.** Off is the large start keys, on is the row. Without a monitor switch,
  the view shows the row while any source with a state runs.

## Set up

1. In Tessera, open **Plugins**, choose **Add with a link** and paste `https://github.com/Orishas/tessera-desk-panel`.
2. Add the plugin to a screen. The screen builds once.
3. In **Layout**, place the **Desk panel** tile from the library's Plugins group and make it 2x2 (everything) or 2x3
   on a grid that offers it; 2x1 shows the sources alone. A 2x3 grid keeps its whole page for Tessera's own tiles,
   so there 2x2 is the largest. The editor starts it at 1x1: drag its right edge, then
   its bottom edge.
4. In the inspector, choose the select whose options are the sources (such as `input_select.buro_monitor_quelle`).
5. Choose what you have, all optional:

   | Option | What |
   |---|---|
   | **Script** | Runs with the option as `quelle` (the script's field must be called so). |
   | **State of the 1st, 2nd and 3rd source** | A device that is on while that source runs (`binary_sensor`, `media_player`, `switch` or `device_tracker`). Its key gets a dot. A media player in `standby` counts as off. |
   | **Auto of the 1st, 2nd and 3rd source** | A switch or input boolean that says whether that source turns the monitor on by itself. The Auto key switches the chosen source's. A source key shows a small Auto mark while it is on. |
   | **Picture switch**, **Monitor switch** | The monitor's keys. The monitor switch also chooses the view. |
   | **Device** | A switch for a device such as a PC. Its key carries the switch's name. |

The order of the sources is the order of the select's options. The 1st state and the 1st Auto belong to its first
option.

Logos, guessed from the option's name: `windows` (PC), `apple` (Mac), `playstation` (PS), `nintendo-switch` (Switch),
`xbox`, `linux`. A key without a logo shows its word.

## Good to know

- The screen must be allowed to perform Home Assistant actions (Tessera turns it on when it adds the screen).
- Up to six sources, with the monitor on or off.
- On a screen turned in its settings (Rotation 90°, 180° or 270°) that turns its picture in software, such as the
  Guition 4", a tap on this tile is lost with firmware 0.60.0: the pressed card takes two slow frames, and the firmware
  counts the tap as a hold, which a plugin's tile does not get. It works with Rotation at 0°.
- The logos are Material Design Icons (Apache 2.0), the ones Home Assistant uses too; the brands belong to their owners.
  `tools/make_logos.py` makes `components/desk_panel/logos.h` from `tools/svg/`.
