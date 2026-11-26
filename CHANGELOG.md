# Changelog

## 0.8.4
- Fix: the frost protection lambda in the example YAML did not compile (`id()` of the climate
  is a pointer).
- The example YAML loads the tag `v0.8.4`.

## 0.8.3
- New switch "Frost Protection Auto Off" (off by default): when frost protection has switched
  the heater on, the ESP switches it off again once the room reaches 5 °C. Saves gas; the heater
  cycles instead. Status then shows `Done HH:MM · x.x °C`. If the heating is changed by hand
  (mode or set point), the ESP leaves it alone.
- The example YAML loads the tag `v0.8.3`.

## 0.8.2
- Shorter "Frost Protection Status" text so it fits a dashboard tile: `Ein 16:16 · 4.4 °C`
  (EN: `On 16:16 · 4.4 °C`) instead of `25.11. 16:16: eingeschaltet bei 4.4 °C`. The date is
  still visible in Home Assistant as the time the entity last changed.
- The example YAML loads the tag `v0.8.2`.

## 0.8.1
- Frost protection threshold can now be set from 0 °C (was 3 °C); default stays 5 °C.
- The example YAML loads the tag `v0.8.1`.

## 0.8.0
- Frost protection on the ESP, also without Home Assistant: new entities "Frost Protection"
  (switch, off by default in the example YAML), "Frost Protection Threshold" (3–10 °C, default
  5 °C) and "Frost Protection Status". With the heater off and the room temperature below the
  threshold for 10 minutes, the ESP sets the heater to HEAT 5 °C. A heating set by hand is never
  changed. Built in the example YAML (template switch/number/text sensor plus an interval).
- The example YAML loads the tag `v0.8.0`.

## 0.7.8
- `startup_delay` is now on by default: after a restart the ESP waits `supervision_timeout` + 8 s
  (28 s by default) before it connects. Until then the panel still holds the old link and refuses
  a new one – with panel software 3.5.38 it then refused every attempt until power-cycled.
  Confirmed by a tester (issue #1): with the wait the ESP reconnects after an OTA update by
  itself. `startup_delay: 0s` turns it off.
- `L1Bat.Current` (×0.01 A, + = charging) confirmed on a NE315 Gen2.
- README: check "WiFi Signal" first on radio timeouts (better than −70 dBm).

## 0.7.7
- Test options for panels that refuse the ESP after every restart until power-cycled (status 133 /
  0x3e, issue #1): `startup_delay` waits after boot before the first connection, so the panel has
  dropped the old link; `disconnect_on_shutdown` ends the link cleanly before a planned restart
  (OTA, restart button). Both off by default; the example YAML lists them as comments.
- Nordelettronica NE315 Gen2 (thanks to a tester): the example YAML adds leisure battery state of
  charge (`L1Bat.SoC`), current (`L1Bat.Current`, 0.01 A, + = charging), nominal capacity
  (`L1Bat.NomCapacity`) and D+ (`PowerSupply.DPlus`). Other units leave them "unknown".

## 0.7.6
- Fix: `lang/de.yaml` and `lang/en.yaml` contained the key `gas_level` twice (heater gas stage
  and gas bottle level), so ESPHome 2026.11 rejected them ("Duplicate key"). The gas bottle
  names now use their own keys `gas_bottle_level`, `gas_bottle_contents`,
  `gas_bottle_temperature` and `gas_bottle_label`. Reported in issue #1 – thanks!
- New check `tests/check_lang.py`: duplicate or missing keys in the language files.

## 0.7.5
- Up to two gas bottles: new option `instance:` (1, 2, ...) for every entity. It picks the
  n-th bus device that publishes the topic, in ascending address order, so two Truma
  LevelControl sensors work without looking up their addresses. The example YAML now has
  "Truma Gas Bottle 1" and "2" with level, contents, temperature and the bottle's label
  from the panel (`GasBtl.Name`), which shows which bottle is which.

## 0.7.4
- Gas bottles (Truma LevelControl, untested): the example YAML adds a device "Truma Gas" with
  level (%), contents (kg) and bottle temperature (`GasBtl.FillLevelP`, `FillLevelW`,
  `Temperature`), with names in `lang/de.yaml` and `lang/en.yaml`. Two bottles need an
  `address:` per sensor (see README).
- The log now shows the ESP-IDF version under the component version (helps with bug reports).

## 0.7.3
- ESPHome 2026.11 or later is required: from 2026.11 every sub-device needs `relation:`. The example
  YAML sets `relation: via` (heater, power system and panel are separate devices reached through
  the ESP), which keeps the existing device and entity IDs in Home Assistant.

## 0.7.2
- Reconnect after a link loss during login: the ESP-IDF Bluetooth stack can drop the panel key
  from RAM ("Deleting unpaired device"), although it is still in flash. The ESP then tried to
  pair from scratch and the panel refused ("pairing not supported", 0x52) until the ESP was
  restarted by hand. The ESP now restarts itself in this case (at most twice in a row) and
  reconnects with the stored key.

## 0.7.1
- Room, water and panel temperature no longer alternate. The heater and the panel send each
  temperature twice, about 0.3 s apart, and the first message is 0.4 °C higher (checked against
  a separate thermometer: the second value is the right one). The ESP now holds every update
  for 0.8 s and passes on only the last value of such a pair. All entities therefore update
  0.8 s later than before.

## 0.7.0 – first public beta
- Diagnostics: new button `type: log_raw` logs every parameter message in full for 2 minutes
  (used to track down the room/water temperature that alternates between two values).
- Entities grouped into ESPHome sub-devices: "Truma Heater" (Combi: heating and hot water),
  "Truma Power" (EBL), "Truma Panel". Connection and diagnostics stay on the ESP itself.
- One example YAML (`truma-inetx.yaml`) instead of two: the language is chosen with one line,
  which loads `lang/de.yaml` or `lang/en.yaml` (entity names, device names, status texts).
- The example YAML loads component and language file from the tag `v0.7.0`, so an update on
  `main` never changes a working installation by surprise.
- Known issues: room and water temperature alternate between two values about 0.4-0.5 °C
  apart; some status values (battery, tank sensors, energy mode) are not decoded yet;
  ESP32-C3/S3/C6 are not supported.

## 0.6.5
- Panel clock: the panel ignores writes to `TimeAndDate.SystemTime`. The ESP now writes the
  time as text (`TimeAndDate.Time` "HH:MM", `TimeAndDate.Date` "DD.MM.YY"), checks the panel's
  next minute report and stops the daily attempts if the panel does not take it.

## 0.6.4
- Panel clock: the ESP writes the Home Assistant time 15 s after connecting and then once a day
  (option `time_sync`, default on) and checks the time the panel displays.
- New button `type: sync_time` ("Set Panel Clock").
- Subscribes to `TimeAndDate`, so "Panel Time" updates live.

## 0.6.3
- The panel rejects connection intervals of 30-50 ms ("unacceptable connection interval").
  The ESP now keeps ESPHome's interval (8.75-11.25 ms) and only raises the supervision timeout.
- Timer entities removed from the example YAML (`format: timer` still works if you add them).
- New diagnostic sensor `type: link_params` ("Radio Link"): interval, latency and supervision
  timeout actually in use, read back from the Bluetooth stack.

## 0.6.2
- After connecting, the ESP asks for a supervision timeout of 20 s (new option
  `supervision_timeout`). Previously the link dropped after 8 s without a packet.

## 0.6.1
- New option `language: en | de` for the texts the ESP produces (connection status,
  "Last Disconnect", timer schedules). The German example YAML sets `de`, the English one `en`.

## 0.6.0
- Pairing code removed (the panel always pairs without a code).
- Sensors bound to fixed device addresses; room and water temperature no longer flip.
- `climate`: new option `appliance_address`.
- `text_sensor`: `format: timer` (readable timers) and `format: firmware`.
- Example YAML with all known parameters: 12 V main switch, air circulation, panel
  temperature, target temperatures, gas level, battery and tank state, display brightness
  and timeout, screensaver, firmware versions, panel clock, 6 timers.
- Example YAML in English (`truma-inetx.en.yaml`) and German (`truma-inetx.de.yaml`).
- Electric heating element removed from the example (Combi E only).

## 0.5.0
- The ESP never closes a working connection itself; if something goes wrong only the Truma
  session is restarted (the panel does not accept a new connection after a disconnect).
- New sensor "Last Disconnect" (time, reason, duration).

## 0.4.0
- Version number as a sensor, in the log and in the device info.
- Experiment with a pairing code (passkey).
