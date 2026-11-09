# Desk panel

One full-page tile for a desk. With the monitor off it shows large keys that start a source. With the monitor on it
shows the sources in a row with a dot for each device that runs, the monitor's keys, an **Auto** key for the chosen
source and a key for a device such as a PC. Two mood keys sit at the bottom.

- **A source key** runs your script with the option in the field `quelle`. Without a script it chooses the option
  of the select directly.
- **The device key** turns the device on and pulses "Starting …" until the device is there (at most two minutes).
  While the device is on, the key asks first, on a card: Cancel or Shut down.
- **The view follows the monitor switch.** Off is the large start keys, on is the row. Without a monitor switch,
  the view shows the row while any source with a state runs.

## Set up

1. In Tessera, open **Plugins**, choose **Add with a link** and paste `https://github.com/Orishas/tessera-desk-panel`.
2. Add the plugin to a screen. The screen builds once.
3. In **Layout**, place the **Desk panel** tile from the library's Plugins group. It is 2x3 (a whole page of a 2x3
   grid), or 2x2 without the moods, or 2x1 with the sources alone. The editor starts it at 1x1: drag its right edge, then
   its bottom edge.
4. In the inspector, choose the select whose options are the sources (such as `input_select.buro_monitor_quelle`).
5. Choose what you have, all optional:

   | Option | What |
   |---|---|
   | **Script** | Runs with the option as `quelle` (the script's field must be called so). |
   | **State of the 1st, 2nd and 3rd source** | A device that is on while that source runs (`binary_sensor`, `media_player`, `switch` or `device_tracker`). Its key gets a dot. A media player in `standby` counts as off. |
   | **Auto of the 1st, 2nd and 3rd source** | A switch or input boolean that says whether that source turns the monitor on by itself. The Auto key switches the chosen source's. The large keys show an "A" while it is on. |
   | **Picture switch**, **Monitor switch** | The monitor's keys. The monitor switch also chooses the view. |
   | **Device** | A switch for a device such as a PC. Its key carries the switch's name. |
   | **1st and 2nd mood** | A scene or a script. The key carries its name. |

The order of the sources is the order of the select's options. The 1st state and the 1st Auto belong to its first
option.

Logos, guessed from the option's name: `windows` (PC), `apple` (Mac), `playstation` (PS), `nintendo-switch` (Switch),
`xbox`, `linux`. A key without a logo shows its word.

## Good to know

- The screen must be allowed to perform Home Assistant actions (Tessera turns it on when it adds the screen).
- Up to six sources: the row shows all six, and the large keys the first four.
- A mood stays marked until another is tapped or the tile is made again: a scene has no state that says it is on.
- The logos are Material Design Icons (Apache 2.0), the ones Home Assistant uses too; the brands belong to their owners.
  `tools/make_logos.py` makes `components/desk_panel/logos.h` from `tools/svg/`.
