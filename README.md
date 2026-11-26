# Truma iNet X → ESPHome (direct BLE connection)

🇬🇧 English · 🇩🇪 [Deutsche Version](README.de.md)

ESPHome component for a **classic ESP32**: the ESP pairs with the Truma iNet X panel by
itself and keeps the Bluetooth connection open permanently. Home Assistant only talks to
the ESP via the ESPHome API over WiFi.

No Home Assistant integration (`truma_inetx`) and no Bluetooth proxy needed.

> **Status: public beta 0.8.6 (November 2026) – testers wanted!**
> Tested with an iNet X Pro panel (software 3.4.27), a Combi 6 heater and a Schaudt EBL; by testers
> also with panel software 3.5.38, Combi D 4 GEN2, Aventa compact and a Nordelettronica NE315 Gen2.
> With a **classic ESP32** (ESP32-D0WD) the connection is stable (long-term tests of 30 hours
> and more). The ESP32-C3 is not supported, see [Hardware](#hardware).
>
> **Please report your experience** in the [Issues](https://github.com/Moormerlander/esphome-truma-inetx/issues):
> panel type and software, heater/AC, ESP32 board, how long the connection holds
> ("Last Disconnect" sensor) and, if something fails, the ESPHome log.

## Quick start

### Requirements
- A **classic ESP32**, e.g. "ESP32 DevKit" or "ESP32-WROOM-32". **Not** a C3, S3 or C6 (see [Hardware](#hardware)).
- Home Assistant with the **ESPHome add-on** (ESPHome Builder), version 2026.11 or later.
- A USB cable and Chrome or Edge for the first flash.

### 1. Clean up
- If you used the "Truma iNet X (BLE)" HA integration before: **delete** it, otherwise two devices compete for the panel.
- On the panel, under *Connected smartphones*, delete old entries, especially the one of your former Bluetooth proxy. The panel only stores a few devices.

### 2. Create the device in the ESPHome Builder
1. **+ New Device** → give it a name, e.g. `truma-inetx` → choose **ESP32**.
2. Finish the wizard. It creates a small starter YAML with an **API key**: copy it.
3. Click **Edit** on the device, replace the whole content with [`truma-inetx.yaml`](truma-inetx.yaml) and save.
4. **Language:** at the top, `packages: language:` points to `lang/de.yaml`. Change it to `lang/en.yaml` for English. The language file sets all entity names, the device names and the status texts.

### 3. Add the secrets
In the ESPHome Builder open **Secrets** (top right) and add, following [`secrets.example.yaml`](secrets.example.yaml):

```yaml
wifi_ssid: "My-WiFi"
wifi_password: "…"
api_encryption_key: "…"     # the key from step 2
ota_password: "…"           # any password
fallback_password: "…"      # any password, at least 8 characters
```

### 4. First flash over USB
1. Connect the ESP to your computer via USB.
2. On the device choose **Install → Plug into this computer**. ESPHome fetches the component from GitHub by itself.
3. From now on all updates go over WiFi.

### 5. Add it to Home Assistant
- HA reports a newly discovered ESPHome device under *Settings → Devices & services*: **Add**, and enter the API key if asked.
- You then get the ESP itself plus the devices **Truma Heater**, **Truma Power** and **Truma Panel**.

### 6. Place the ESP
As close to the panel as possible (a few metres, line of sight), with permanent power (12 V → 5 V USB adapter).

### 7. Pair the panel
1. On the panel start **"Add new device"**.
2. Right after that press **"Pair Panel"** on the ESP in HA.
3. Wait until "Connection Status" shows **"Connected"**. No code is needed. "Something went wrong" on the panel can be ignored.
4. Check: the panel lists **exactly one** entry for the ESP, with a Bluetooth symbol.

### Good to know
- **After a firmware update** the ESP usually reconnects by itself. If not: power-cycle the panel briefly, do not pair again.
- **Pair again** only if the status says "Panel rejects this device", and delete **all** old entries on the panel first.

---

## Key findings from real-world testing

1. **The ESP must appear exactly once in the panel's device list.** The panel adds a *new*
   entry on every pairing and does not replace the old one. With two entries for the same
   MAC address, the panel's Bluetooth module discards the pairing after one or two hours
   ("Panel rejects this device: pair again").
   **Always delete all old entries on the panel before pairing again.**
2. **After a connection has been closed, the panel does not accept a new one** – neither
   from the ESP nor from the Truma app – until the panel has been power-cycled (Bluetooth
   error `0x3e`). That is why, since 0.5.0, the ESP **never** closes a working connection
   itself. If something goes wrong it only restarts the Truma session over the existing
   radio link, also when you press "Reconnect".
3. **Every flash closes the connection.** Since 0.6.3 the ESP usually reconnects by itself.
   If not: power-cycle the panel briefly. The pairing survives that, no need to pair again.
4. The panel always pairs **without a code** ("Just Works"). Its Bluetooth module is a
   separate device on the Truma bus (address 0x0601, `BleDeviceManagement`).
5. While the ESP is connected, the panel shows "To pair further devices please close other
   iNet X apps". It only allows one app connection at a time.
6. Several devices report the same values (e.g. `AirHeating.Temp`). Without `address:` the
   value flips back and forth – always bind sensors to one device (heater 0x0201,
   panel 0x0101, power distribution unit 0x0405).

### Re-pairing step by step

1. Power-cycle the panel (only needed if there was a connection before or you just flashed).
2. On the panel, under *Connected smartphones*, delete **all** entries of the ESP.
3. In Home Assistant press "Remove Pairing".
4. On the panel start "Add new device", right after that press "Pair Panel" in Home Assistant.
5. Check: the panel shows **exactly one** entry with a Bluetooth symbol.

### Diagnostic sensors

- "Last Disconnect": time, reason (HCI code) and duration of the last connection
- "BLE Free Slots", "BLE Module State": state of the panel's Bluetooth module
- "Firmware Version": version of this component
- "Radio Link": connection interval and supervision timeout actually in use

---

## Why this is more stable

The Home Assistant integration works, but the connection runs through many layers it does
not control. Its own documentation lists the causes:

| Problem with HA + proxy | With the dedicated ESP |
|---|---|
| HA picks **on every connection attempt** whether to use the proxy or the local adapter. But the pairing (bond) exists on only *one* of them. | There is exactly one central, and it holds the bond. |
| The panel uses a **constantly changing Bluetooth address (RPA)**. Whether the host can resolve it depends on kernel and BlueZ (regression since Linux 6.19). | The ESP stores the panel's identity key (IRK) when pairing and resolves the address itself (AES, as in the Bluetooth specification). |
| The proxy shares its connection slots with other BLE devices (battery BMS, sensors …). | The ESP does nothing else. |
| BlueZ writes without timeout, hanging disconnects, reload issues in HA. | Every stage has its own timeout. |
| Power saving / restarts of the HA host interrupt the connection. | The ESP keeps the connection while HA restarts. The values are back immediately. |

What remains are quirks of the **panel itself**:
- After a closed connection the panel does not accept a new one until it is power-cycled
  (see "Key findings").
- After a restart the internal bus is "deaf" for about 90 s. The ESP retries automatically.

---

## Hardware

- **Classic ESP32** (ESP32-WROOM / "ESP32 DevKit", `board: esp32dev`)

> **Important:** With the **ESP32-C3** (and probably S3/C6) pairing and data work, but
> **reconnecting** after a drop does not. The Bluetooth stack of these chips looks up bonded
> devices through address resolution in the radio controller, and that does not find the
> panel (measured on 14 Nov 2026). The classic ESP32 resolves the address in software, just
> like the ESPHome Bluetooth proxy.
- USB power from the vehicle's 12 V system (12 V → 5 V USB adapter)
- **Placement:** as close to the panel as possible (a few metres, line of sight).

---

## Pairing in detail

1. Leave the panel on its **normal screen**.
2. In Home Assistant press **"Pair Panel"**. The ESP is now in pairing mode for 120 s (see
   sensor "Connection Status").
3. **Now** put the panel into **add-device mode** (Truma app or panel menu).
4. Wait. The log shows `Paired with the Truma panel`, then `Connected`.

Notes:
- At the end the panel may show **"Something went wrong"**. This only refers to Truma's app
  handshake and can be ignored. What counts is the Bluetooth symbol next to the entry on the
  panel and the sensor "Panel Connected".
- **On a campsite:** in pairing mode the ESP only connects to a panel that advertises
  *without a name*. Only a panel in add-device mode does that. Other panels in normal
  operation send their name and are skipped. A panel that refuses pairing is ignored for the
  rest of the pairing window.
- After pairing, the ESP **only connects to this one panel** (address resolution with the
  stored IRK).
- Status **"Panel rejects this device: pair again"**: the panel has forgotten the ESP. Clear
  the device list on the panel and pair again.

---

## Configuration

```yaml
truma_inetx:
  id: truma_hub
  time_id: ha_time          # optional: time for the panel
  user_name: "Home Assistant"
  pairing_timeout: 120s
  log_parameters: false     # true = log every value change
  language: en              # en | de: language of status, "Last Disconnect" and timer texts
  supervision_timeout: 20s  # how long the link survives without hearing the panel (2-32 s)
  time_sync: true           # set the panel clock from time_id after connecting and daily
  startup_delay: 28s        # wait after boot before connecting (default: supervision_timeout + 8 s)
  disconnect_on_shutdown: false  # test option: disconnect cleanly before a planned restart
```

**After a restart the ESP waits before it connects** (`startup_delay`, by default
`supervision_timeout` + 8 s, i.e. 28 s). Until then the panel still holds the link from before
the restart, and a connection attempt is refused (log: `status=133` or `0x3e`) – with panel
software 3.5.38 the panel then refused every attempt until it was without power. Confirmed by
a tester: with the wait it reconnects after an OTA update by itself. `startup_delay: 0s` turns
it off. `disconnect_on_shutdown: true` (ends the link cleanly before a restart) stays as a
test option; so far it has not been needed.

**Power distribution unit NE315 Gen2** (Nordelettronica, address 0x0405) also reports the
leisure battery's state of charge (`L1Bat.SoC`, %), current (`L1Bat.Current`, 0.01 A,
+ = charging), nominal capacity (`L1Bat.NomCapacity`, Ah) and D+ (`PowerSupply.DPlus`). The
example YAML has these; with other units they stay "unknown".

The bus behind the panel is generic. Every value is a `topic` + `param` and comes from a
device with a bus address (panel 0x0101, heater 0x0201, power distribution unit 0x04xx,
gas bottles 0x06xx …). Without `address:` the device that reported the value last is used.
With `address: 0x0603` you pin it, e.g. with two gas bottles.

**Which values do I have?** Press the button "Dump All Parameters to Log". The log then
lists every parameter of every device with its current value. Alternatively
`log_parameters: true`.

| Platform | Options |
|---|---|
| `sensor` | `topic`, `param`, `address`. Raw number, scale with `filters: - multiply: 0.1`. Lists report their length. |
| `binary_sensor` | `topic`/`param`/`address` and optionally `on_values: [1]` (default: anything ≠ 0), **or** `type: connected \| paired \| pairing_mode` |
| `text_sensor` | `topic`/`param`/`address` with optional `format: timer` (readable timer) **or** `format: firmware` + `address` (firmware version of a device), **or** `type: status \| version \| last_disconnect \| link_params` |
| `number` | `topic`, `param`, `address`, `min_value`, `max_value`, `step`, `scale` (displayed = value × scale; temperatures: `0.1`) |
| `select` | `topic`, `param`, `address`, `options: {0: "Eco", 1: "Comfort"}` |
| `switch` | `topic`, `param`, `address`, `on_value` (1), `off_value` (0) |
| `button` | `type: pair \| unpair \| reconnect \| dump \| log_raw \| measure \| sync_time \| write` (for `write`: `topic`, `param`, `value`, `address`; `log_raw` logs every message in full for 2 min) |
| `climate` | `supported_modes: [HEAT, FAN_ONLY, COOL, AUTO, DRY]`, `appliance_address: 0x0201` (heater whose values are shown) |

How the thermostat entity works:
- The mode is `RoomClimate.Mode` on the panel.
- Depending on the mode, the target temperature is `AirHeating.TgtTemp`, `AirCooling.TgtTemp`
  (air conditioner) or `RoomClimate.TgtTemp` (auto).
- The current temperature is `AirHeating.Temp`.

Writes go to the device that reports the value. Only `RoomClimate` goes to the panel.

### Frost protection

The ESP itself watches for frost – also when Home Assistant or the WiFi is down (it only
needs the link to the panel). With **"Frost Protection"** on, the heater off and the room
temperature below **"Frost Protection Threshold"** (default 5 °C) for 10 minutes, it sets the
heater to **HEAT 5 °C** (the lowest set point of the panel) and notes it in **"Frost Protection
Status"**. A heating set by hand is never changed. Off by default in the example YAML.

**"Frost Protection Auto Off"** (off by default, only together with "Frost Protection"): when the
heater is on **HEAT with 5 °C**, the ESP switches it off again once the room reaches 5 °C – no
matter who set it, also after a restart. 5 °C counts as frost mode; a higher set point is real
heating and is never touched. This saves gas because the room
stays colder on average – in return the heater cycles on and off more often in cold nights. The
status then shows e.g. `Done 03:40 · 5.0 °C`.

Turn it off when the vehicle is winterized and the heater should stay off. The ESP cannot warn
you when the panel link, the gas or the battery fails – do that in Home Assistant, e.g. a
notification when "Panel Connected" is off for 10 minutes or "Fault" is on.

### Gas bottles (Truma LevelControl)

If one or two Truma LevelControl sensors are registered at the iNet X panel (the level
shows on the panel), the panel passes their values on. The example YAML has two devices,
"Truma Gas Bottle 1" and "2", each with level (%), contents (kg), temperature and the
label the bottle has at the panel (e.g. "Links"). `instance: 1` / `instance: 2` picks the
first / second bottle by bus address, so no address has to be looked up. With one bottle
or none, the other entities simply stay "unknown".

Re-pairing a LevelControl at the panel can swap bottle 1 and 2; the "Label" entity shows
which is which. Tested by a user with two LevelControl sensors (Combi 6): level and
contents match the panel (e.g. 96 % = 10.6 kg on an 11 kg bottle). Reports with other
setups are welcome as an issue.

### Truma Aventa Compact Plus 2nd gen (air conditioner)

[`aventa-compact-plus-2.example.yaml`](aventa-compact-plus-2.example.yaml) is a
complete configuration for the Aventa Compact Plus 2nd gen at address `0x0801`: climate with
heat, cool, auto, fan and dry, fan level, ambient light (on/off and brightness), mains
connection and diagnostics. Heating through the Aventa needs v0.8.6 or newer. Contributed and
tested by a user (Aventa firmware 1.10.5, panel 3.5.38) – thanks!

### Custom actions in Home Assistant

```yaml
api:
  actions:
    - action: truma_write
      variables:
        topic: string
        param: string
        value: int
      then:
        - lambda: 'id(truma_hub).write_param(topic, param, value);'
```

---

## How it works internally

1. **Find:** the panel advertises the service UUID `fc310002-…`. Its address changes
   constantly and is checked against the IRK stored when pairing.
2. **Connect & encrypt:** encryption (bond) first, then notifications on CMD and DATA_R.
   Upstream showed that the other way round makes the panel abort.
3. **Session:**
   - registration (the panel assigns a bus address)
   - subscribe to topics
   - send identity and time
   - parameter request to every bus device (only panel and heater answer a broadcast)
   - trigger tank measurement
4. **Keep alive:** values arrive as push messages. Tanks are measured every minute.
5. **Watchdog:** if no data arrives for 90 s, the session is restarted over the existing
   radio link. The ESP never closes the radio link itself.

Transport: every message goes through `InitDataTransfer → Ready → Data → DataAck`.
If an ack is missing, the session is restarted. The ack carries no identifier, so matching
would no longer be reliable afterwards (measured upstream).

---

## Tests

```bash
cd tests
g++ -std=c++17 -I../components/truma_inetx test_core.cpp ../components/truma_inetx/truma_core.cpp -lcrypto -o test_core
./test_core        # -v for a verbose log
```

What the tests cover:
- CBOR (RFC 8949 test vectors)
- frame building (byte-identical to the Python reference)
- RPA resolution (test vector from the Bluetooth specification)
- a complete session against a simulated panel: fragmented replies at MTU 23, devices
  without ack, writes with echo, rejected writes, stall watchdog, stray Ready/Ack packets

---

## Troubleshooting

- Log level `DEBUG` (or `logs: { truma_inetx: VERBOSE }`). The sensor "Connection Status"
  shows the current stage.
- "Last Disconnect" shows a radio timeout (0x08): the ESP32 shares one radio between WiFi
  and Bluetooth. **Check "WiFi Signal" first: it should be better than −70 dBm.** A tester
  had drops every 1–2 hours at −81 dBm (4G router plus WireGuard); with a strong signal the
  link then ran for days without a drop. Set a **fixed WiFi channel** on your router (automatic channel changes make
  the ESP rescan), or use an ESP32 board with Ethernet (e.g. WT32-ETH01, Olimex ESP32-POE)
  so Bluetooth has the radio to itself.
- Permanent status 0x3E / `connect failed` after a disconnect or after flashing:
  power-cycle the panel. Only if the status then says "Panel rejects this device", pair again
  (see "Re-pairing step by step").
- `the panel assigned no address` right after a panel restart: normal, goes away after
  about 1–2 minutes.

---

## License & thanks

- Protocol knowledge and measurements: [rpodgorny/hass-truma-inetx](https://github.com/rpodgorny/hass-truma-inetx) (GPL-3.0)
- Original reverse engineering: [daaaaan/truma-inetx-ble](https://github.com/daaaaan/truma-inetx-ble)

The C++ code is a reimplementation; no code was taken from the unlicensed upstream protocol
module. Because logic and measurements come from the GPL project, this project is released
under **GPL-3.0** as well.

"Truma" and "iNet X" are trademarks of Truma Gerätetechnik GmbH & Co. KG. This project is
not affiliated with Truma.
