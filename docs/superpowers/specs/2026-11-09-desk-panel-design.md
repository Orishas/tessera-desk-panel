# Schreibtisch (`desk_panel`): Entwurf

Stand: 2026-11-09 · Repo: `Orishas/tessera-desk-panel` · Plugin-API `0.24`

> Seit 0.2.0 gibt es die Stimmungstasten (`mood_1`, `mood_2`) nicht mehr. Was dieser Entwurf dazu sagt, gilt nur
> bis 0.1.2; in der Ruhe-Ansicht liegt die Gerätetaste seitdem über die ganze Breite.

## Ziel

Eine ganzseitige Kachel für Desk Control, die von selbst zeigt, was am Schreibtisch gerade dran ist:

- Ist der Monitor aus, zeigt sie große Tasten, mit denen du eine Quelle startest.
- Ist der Monitor an, zeigt sie die Quellen mit Statuspunkten und die Monitor-Tasten.
- In beiden Ansichten gibt es Licht-Stimmungen und eine Taste, die den PC ein- und ausschaltet, ohne den Eingang zu
  wechseln.
- Pro Quelle (außer Switch 2) lässt sich ihr Auto-Schalter an- und ausschalten.

Sie ersetzt auf Seite 1 die Kachel `source_keys`. `source_keys` bleibt als eigenes Plugin unverändert.

**Erfolg heißt:**
- Ein Blick zeigt, welche Geräte laufen.
- Ein Tipp startet eine Quelle.
- Der PC lässt sich an- und ausschalten, ohne dass sich Quelle oder Eingang ändern.
- Das Herunterfahren fragt vorher nach.

## Rahmen, den Tessera vorgibt

- Höchstens 12 Optionen pro Kachel, alle zusammen höchstens 1280 Byte JSON.
- Den Zustand eines fremden Geräts sieht die Kachel nur, wenn es eine Entity-Option ist (`kind: entity`, API 0.15).
- Eine Kachel kennt **kein** `on_hold`. Halten öffnet Tesseras Karte der Kachel. Alles läuft deshalb über Tipps.
- Kein `lv_anim` und kein eigener Timer. Was sich bewegt (das Pulsieren), zeichnet `on_tick` einmal pro Sekunde.
- Logos: die eingebauten aus `source_keys` (`logos.h`, MDI, Apache 2.0). Sie werden aus den Namen der Optionen geraten.

## Kachel `desk`

- `domains: [input_select, select]`, `attributes: [options]`. Die Entität der Kachel ist die Quellen-Auswahl
  (`input_select.buro_monitor_quelle`).
- `sizes: { min: 2x2, max: 2x3 }`. Auf Desk Control (2×3) füllt sie die ganze Seite.

### Optionen (12 von 12)

| id | kind | Bei dir | Wozu |
|---|---|---|---|
| `script` | entity `[script]` | `script.buro_quelle_anzeigen` | Ein Tipp auf eine Quelle ruft es mit der Quelle im festen Feld `quelle` auf. Ohne Skript wählt die Kachel die Option direkt. |
| `status_1` … `status_3` | entity `[binary_sensor, media_player, switch, device_tracker]` | PC `binary_sensor.jt_desktop_status`, Mac `binary_sensor.mac_mini`, PS5 `media_player.playstation_5` | Statuspunkt der 1., 2. und 3. Quelle (Reihenfolge der Optionen). „Läuft“ heißt: Zustand `on`, `home`, `playing`, `paused` oder `idle` (eine PS5 im Ruhemodus meldet `standby` und gilt als aus). Leer heißt: kein Punkt. |
| `auto_1` … `auto_3` | entity `[input_boolean, switch]` | PC, Mac, PS5: dieselben Auto-Schalter wie heute in `source_keys` | Auto-Schalter der 1., 2. und 3. Quelle. Leer heißt: keine Auto-Taste und kein „A“ für diese Quelle. |
| `picture` | entity `[switch]` | derselbe Schalter wie heute in `source_keys` | Taste „Bild“ |
| `power` | entity `[switch]` | wie heute in `source_keys` | Taste „Monitor“. **Sie entscheidet auch die Ansicht.** |
| `device` | entity `[switch, input_boolean]` | `switch.buro_pc` | Die ⏻-Taste. Ihr Wort und der Titel der Karte sind der Name des Schalters in Home Assistant („PC“). |
| `mood_1`, `mood_2` | entity `[scene, script]` | `scene.buro_fokus`, `scene.buro_aus` | Stimmungstasten. Das Wort kommt vom Namen der Szene. |

Den Namen des Geräts und der Stimmungen liefert Home Assistant über `data["entities"][id]`. Fehlt eine Option,
verschwindet die zugehörige Taste, und die übrigen teilen sich ihren Platz.

### Zwei Ansichten

Die Ansicht wählt `power`:
- **Ruhe:** `power` ist aus. Ist `power` nicht gesetzt, gilt: alle eingetragenen Statusgeräte sind aus.
- **Arbeit:** sonst.

Bei einem Wechsel baut `on_state` die Ansicht neu auf. Zwischen den Ansichten wird nicht animiert.

```
 Ruhe (Monitor aus)            Arbeit (Monitor an)
┌────────────┬────────────┐   ┌──────────────────────────┐
│  ⊞ PC •    │   Mac •   │   │ ⊞•   •  ◯   ◯           │  Quellen
├────────────┼────────────┤   ├──────┬───────┬──────┬─────┤
│  PS5 •    │  Switch   │   │ Bild │Monitor│ Auto │ ⏻ PC│  Monitor + Gerät
├────────┬───┴────┬───────┤   ├──────┴─────┬─┴──────┴─────┤
│ Fokus  │  Aus   │ ⏻ PC  │   │   Fokus    │    Aus      │  Stimmungen
└────────┴────────┴───────┘   └────────────┴─────────────┘
```

**Ruhe:**
- Die Quellen liegen als große Tasten im Raster, höchstens 4 (2×2). Jede zeigt ihr Logo groß, das Wort darunter und
  oben rechts den Statuspunkt. Ist ihr Auto-Schalter an, steht unten rechts ein kleines „A“ (INK, gedämpft).
- Eine 5. oder 6. Quelle zeigt die Ruhe-Ansicht nicht.
- Unten: die Stimmungen und die ⏻-Taste.

**Arbeit:**
- Oben: die Quellenreihe wie in `source_keys`. Die gewählte Quelle ist ACCENT, Statuspunkte gibt es auch hier.
- Mitte: Bild, Monitor, Auto und ⏻. „Auto“ gilt für die gewählte Quelle und ist ACCENT, wenn ihr Auto-Schalter an ist. Hat die gewählte Quelle keinen (Switch 2), entfällt die Taste und die anderen drei teilen sich die Reihe.
- Unten: die Stimmungen.

Auf einer 2×2-Kachel entfällt die Stimmungsreihe. In der Ruhe-Ansicht steht die ⏻-Taste dann in der Mitte unten
zwischen den Quellen, falls Platz ist, sonst entfällt sie.

**Der Statuspunkt** ist ein kleiner Kreis, `ui::mm(2)`:
- **an:** ACCENT. Ist die Taste selbst ACCENT, dann INK.
- **aus:** gar nicht gezeichnet.

### Was ein Tipp tut

`on_tap` ermittelt mit `tessera::tap_point()` die Taste.

| Taste | Tipp |
|---|---|
| Quelle | Wie `source_keys`: mit Skript `script.turn_on` mit `{variables: {quelle: <Option>}}`, sonst `select_option`. Die Taste ist sofort ACCENT. |
| Bild, Monitor | `turn_on` oder `turn_off`, je nach bekanntem Zustand; der neue Zustand ist sofort zu sehen. |
| Stimmung | `scene.turn_on` oder `script.turn_on`. Die Taste ist danach ACCENT, bis eine andere Stimmung getippt wird oder die Kachel neu entsteht. Szenen haben keinen „aktiv“-Zustand; die Auswahl merkt sich nur die Kachel. |
| Auto | `turn_on` oder `turn_off` auf den Auto-Schalter der gewählten Quelle (`input_boolean.*` oder `switch.*`); sofort zu sehen. |
| ⏻, Gerät aus | `turn_on` auf `device`, danach „Startet …“ (unten) |
| ⏻, Gerät an | öffnet die Karte `confirm` |

**„Startet …“:** Nach dem Einschalten zählt die Kachel 120 Sekunden. In dieser Zeit wechselt `on_tick` jede Sekunde
die Füllung der ⏻-Taste zwischen ACCENT und normal, und die Taste zeigt „Startet …“. Es endet, sobald `device` `on`
meldet oder die Zeit um ist.

`switch.buro_pc` zeigt schon an, wenn der PC erreichbar ist. Damit endet das Pulsieren genau dann, wenn er da ist.

### Karte `confirm`

- Titel: der Name von `device`, also „PC“.
- Inhalt: Frage „PC herunterfahren?“ und zwei große Knöpfe nebeneinander: links **Abbrechen** (normal), rechts
  **Herunterfahren** (Rolle für Gefahr/Warnung, sonst ACCENT).
- Abbrechen und die Zurück-Taste schließen die Karte.
- Herunterfahren: `turn_off` auf `device`, dann schließt die Karte.
- Auf die Karte in Home Assistant („PC fährt herunter“, `script.buro_pc_ausschalten`) wartet die Kachel nicht.

### Rechte (`permissions.home_assistant_actions`)

`script.turn_on`, `input_select.select_option`, `select.select_option`, `switch.turn_on`, `switch.turn_off`,
`input_boolean.turn_on`, `input_boolean.turn_off`, `scene.turn_on`

### Texte

`translations/en.json` und `de.json`. Alle Wörter auf dem Display kommen von dort:
- „Bild“, „Monitor“, „Auto“, „Startet …“, „herunterfahren?“, „Abbrechen“, „Herunterfahren“
- „Auswahl wählen“ für eine Kachel ohne Entität

## Home Assistant (neben dem Plugin)

1. **`switch.buro_pc`** (Template-Helper `01M6ZTPFS5REGWMMKWQFWBNF78`):
   - **Ein** wird neu: nur `button.press` auf `button.wake_on_lan_c8_7f_54_6a_46_fe` und `switch.turn_on` auf
     `switch.pc_speaker`.
   - **Aus** bleibt `script.buro_pc_ausschalten`.
   - `script.buro_pc_einschalten` bleibt unverändert. Der Zigbee-Taster und `pc_on_off_new` nutzen es weiter.
2. **`scene.buro_fokus`:**
   - Deckenleuchte weiß an, hell und neutral.
   - Ring und Filamentlampe aus.
   - Die Werte stimmen wir beim Bauen ab.
3. **`scene.buro_aus`:** Deckenleuchte weiß, Ring und Filamentlampe aus. Das Ambilight folgt weiter seiner Automatik.

## Repo

```
tessera-plugin.yaml   plugin.yaml   LICENSE (MIT)   README.md   CHANGELOG.md   preview.yaml   store.png
components/desk_panel/{__init__.py, desk_panel.h, desk_panel.cpp, logos.h}
translations/{en,de}.json
tools/make_logos.py + tools/svg/ (aus source_keys übernommen)
```

`logos.h` und das Raten der Logos kommen aus `source_keys`. `desk_panel.cpp` wird neu geschrieben, in klaren Teilen:
- Optionen lesen
- Zustand übernehmen (`on_state`)
- Ruhe zeichnen
- Arbeit zeichnen
- Tipp zuordnen
- Karte

## Prüfen

1. `python3 ../tessera-plugins/tools/check.py .` meldet `desk_panel: ok`.
2. Der Prüfstand `preview.py . --strict` läuft ohne Warnung auf `hosyond40`, `jc8012p4a1` und `cyd` (Guition baut auf
   dem Prüfstand nicht).
   - `preview.yaml` hat die Fälle: Ruhe (mit „A“), Arbeit (Auto an), Arbeit mit Switch 2 gewählt (ohne Auto), „Startet …“, 2×2, ohne Statusgeräte, ohne Stimmungen und
     „keine Entität“.
   - Die Bilder schaue ich an, hell und dunkel.
3. `store.png` wird aus den Prüfstand-Bildern gebaut, wie bei `source_keys`.
4. Gerät:
   - Repo veröffentlichen, das Plugin in Tessera hinzufügen und den Firmware-Build abwarten.
   - Die Kachel auf Seite 1 gegen `source_keys` tauschen.
   - Dann zusammen am Display prüfen: beide Ansichten, Punkte, Quelle tippen, ⏻ an.
   - ⏻ aus, also das Herunterfahren, nur mit deinem OK.

## Nicht dabei (bewusst)

- Die Seite von selbst wechseln per Automation (Idee 1 außerhalb der Kachel). Die Kachel wechselt ihre Ansicht selbst.
- Halten auf Tasten. Die API kann es für Kacheln nicht.
- Ein frei wählbarer Feldname für das Skript (fest `quelle`, damit die 12 Optionen reichen).
