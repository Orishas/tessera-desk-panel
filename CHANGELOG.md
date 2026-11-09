# Changelog

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
