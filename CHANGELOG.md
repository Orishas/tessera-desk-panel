# Changelog

## 0.4.0 - 2026-11-10
- The fourth source can have a state and an Auto switch too (two new options). A tile has at most twelve options, so
  the fifth and sixth source stay without.
- The script gets the option in the field `source` as well as in `quelle`: call your script's field either.
- A switch that Home Assistant does not answer goes back by itself, also when you tapped another switch meanwhile.
  Before, only the last one tapped went back.
- The card that asks before shutting down stays open when the screen could not send the action, instead of closing
  as if it had.
- A logo is at most 160 pixels a side: on a very large key it no longer takes memory without end.
- The repository checks itself on every push (the model's tests and Tessera's `check.py`).

## 0.3.1 - 2026-11-10
- The tile draws a key again only when its look changes: an update from Home Assistant that changes nothing on the
  tile no longer redraws the source keys.
- Nothing else changes for you: the code was reviewed and tidied.

## 0.3.0 - 2026-11-09
- The accent is the chosen source's alone. The other keys stay grey: the device says on with a dot, as a source
  does, and a switch that is off has a muted icon and says "Picture off" or "Auto off".
- With the monitor off, a frame marks the source it shows when it comes on.
- A tapped source whose device is off blinks its dot until the device is there (at most two minutes).
- The monitor's key has a monitor as its icon, not the device key's power sign.
- The Auto mark is an icon, and shows with the monitor on too.
- A switch that Home Assistant does not answer within five seconds goes back to what it was.
- With the monitor off all six sources show, not the first four.

## 0.2.0 - 2026-11-09
- The two mood keys are gone, with their options and the permission `scene.turn_on`. With the monitor off the
  device's key takes the whole width under the sources.
- "Starting …" ends with the state that says the device is on, keeps its pulse when something else changes, and
  pulses on a screen whose clock is not set.
- With the monitor on and no key for the second row, the sources take the tile's height.
- An option of the select longer than 64 bytes is left out instead of being sent cut short.
- A key too low for its icon over its word (the device's key on a small screen) shows the two side by side instead
  of cutting the word.

## 0.1.2 - 2026-11-09
- On 2x2 everything fits: with the monitor off the sources side by side and the moods and the device under them,
  with it on the moods in a row of their own. On a 2x3 grid Tessera keeps the whole page for its own tiles, so 2x2
  is the largest a plugin's tile gets there.

## 0.1.1 - 2026-11-09
- The tile can be 2x1 too, with the sources in a row. The editor starts a new tile at 1x1 and grows it one way at a
  time, so it now reaches 2x3 by 2x1 and 2x2.

## 0.1.0 - 2026-11-09
- First version.
