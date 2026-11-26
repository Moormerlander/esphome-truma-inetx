# Truma iNet X → ESPHome (direkte BLE-Verbindung)

🇬🇧 [English version](README.md) · 🇩🇪 Deutsch

ESPHome-Komponente für einen **klassischen ESP32**: Der ESP koppelt sich selbst
mit dem Truma iNet X Panel und hält die Bluetooth-Verbindung dauerhaft. Home Assistant
spricht nur noch per ESPHome-API über WLAN mit dem ESP.

Damit entfallen die HA-Integration `truma_inetx` und der Bluetooth-Proxy.

> **Status: öffentliche Beta 0.8.6 (November 2026) – Tester gesucht!**
> Getestet an einem iNet X Pro Panel (Software 3.4.27) mit Combi 6 und Schaudt EBL, von Testern
> auch mit Panel-Software 3.5.38, Combi D 4 GEN2, Aventa compact und Nordelettronica NE315 Gen2. Mit einem
> **klassischen ESP32** (ESP32-D0WD) läuft die Verbindung stabil (Langzeittests über 30 Stunden).
> Der ESP32-C3 wird nicht unterstützt, siehe [Hardware](#hardware).
>
> **Bitte berichte deine Erfahrungen** in den [Issues](https://github.com/Moormerlander/esphome-truma-inetx/issues):
> Panel-Typ und -Software, Heizung/Klimaanlage, ESP32-Board, wie lange die Verbindung hält
> (Sensor „Letzte Trennung“) und, falls etwas nicht klappt, das ESPHome-Log.

## Schnellstart

### Voraussetzungen
- Ein **klassischer ESP32**, z. B. „ESP32 DevKit“ oder „ESP32-WROOM-32“. **Kein** C3, S3 oder C6 (siehe [Hardware](#hardware)).
- Home Assistant mit dem **ESPHome-Add-on** (ESPHome Builder), ab Version 2026.11.
- Ein USB-Kabel und für das erste Flashen Chrome oder Edge.

### 1. Aufräumen
- War vorher die HA-Integration „Truma iNet X (BLE)“ im Einsatz: in HA **löschen**, sonst konkurrieren zwei Geräte um das Panel.
- Am Panel unter *Verbundene Smartphones* die alten Einträge löschen, vor allem den des früheren Bluetooth-Proxys. Das Panel speichert nur wenige Geräte.

### 2. Gerät im ESPHome Builder anlegen
1. **+ New Device** → Namen vergeben, z. B. `truma-inetx` → als Typ **ESP32** wählen.
2. Den Assistenten abschließen. Er erzeugt eine kleine Start-YAML mit einem **API-Schlüssel**: diesen kopieren.
3. Beim Gerät auf **Edit** klicken, den ganzen Inhalt durch [`truma-inetx.yaml`](truma-inetx.yaml) ersetzen und speichern.
4. **Sprache:** Ganz oben bei `packages: language:` steht `lang/de.yaml`. Für Englisch durch `lang/en.yaml` ersetzen. Die Sprachdatei stellt alle Entitätsnamen, die Gerätenamen und die Statustexte um.

### 3. Secrets eintragen
Im ESPHome Builder oben rechts **Secrets** öffnen und nach der Vorlage [`secrets.example.yaml`](secrets.example.yaml) ergänzen:

```yaml
wifi_ssid: "Mein-WLAN"
wifi_password: "…"
api_encryption_key: "…"     # der Schlüssel aus Schritt 2
ota_password: "…"           # frei wählbar
fallback_password: "…"      # frei wählbar, mindestens 8 Zeichen
```

### 4. Erstes Flashen per USB
1. Den ESP per USB an den Computer anschließen.
2. Beim Gerät **Install → Plug into this computer**. ESPHome lädt die Komponente dabei selbst von GitHub.
3. Ab jetzt laufen alle Updates per WLAN.

### 5. In Home Assistant hinzufügen
- HA meldet unter *Einstellungen → Geräte & Dienste* ein neu gefundenes ESPHome-Gerät: **Hinzufügen**, falls verlangt den API-Schlüssel eingeben.
- Danach gibt es den ESP selbst sowie die Geräte **Truma Heizung**, **Truma Bordnetz** und **Truma Panel**.

### 6. ESP platzieren
Möglichst nah am Panel (wenige Meter, Sichtlinie) und dauerhaft mit Strom versorgen (12 V → 5 V USB-Adapter).

### 7. Panel koppeln
1. Am Panel **„Neues Gerät hinzufügen“** starten.
2. Direkt danach in HA beim ESP **„Panel koppeln“** drücken.
3. Warten, bis der „Verbindungsstatus“ **„Verbunden“** zeigt. Einen Code braucht es nicht. „Etwas ist schiefgelaufen“ am Panel kann man ignorieren.
4. Kontrollieren: am Panel steht **genau ein** Eintrag des ESP mit Bluetooth-Symbol.

### Gut zu wissen
- **Nach einem Firmware-Update** verbindet sich der ESP meist von selbst wieder. Falls nicht: Panel kurz stromlos machen, nicht neu koppeln.
- **Neu koppeln** nur, wenn der Status „Panel lehnt den ESP ab“ zeigt, und vorher **alle** alten Einträge am Panel löschen.

---

## Wichtigste Erkenntnisse aus dem Praxistest

1. **Am Panel darf der ESP nur genau einmal in der Geräteliste stehen.** Das Panel legt
   bei jeder Kopplung einen *neuen* Eintrag an und ersetzt den alten nicht. Stehen zwei
   Einträge mit derselben MAC drin, verwirft das Bluetooth-Modul des Panels nach ein bis
   zwei Stunden die Kopplung („Panel rejects this device: pair again“).
   **Vor jedem Neu-Koppeln alle alten Einträge am Panel löschen.**
2. **Das Panel nimmt nach einer getrennten Verbindung keine neue an** – weder vom ESP noch
   von der Truma-App –, bis das Panel stromlos gemacht wurde (Bluetooth-Fehler `0x3e`).
   Deshalb trennt der ESP ab 0.5.0 eine laufende Verbindung **nie** selbst. Bei Problemen
   startet er nur die Truma-Sitzung über die bestehende Funkverbindung neu, auch beim
   Button „Neu verbinden“.
3. **Jedes Flashen trennt die Verbindung.** Seit 0.6.3 verbindet sich der ESP danach meist
   von selbst wieder. Falls nicht: Panel kurz stromlos machen. Die Kopplung übersteht das,
   neu koppeln ist nicht nötig.
4. Das Panel koppelt immer **ohne Code** („Just Works“). Das Bluetooth-Modul ist ein eigenes
   Gerät auf dem Truma-Bus (Adresse 0x0601, `BleDeviceManagement`).
5. Solange der ESP verbunden ist, zeigt das Panel „Zur Kopplung weiterer Geräte bitte andere
   iNet X Apps schliessen“. Es lässt nur eine App-Verbindung gleichzeitig zu.
6. Mehrere Geräte melden dieselben Werte (z. B. `AirHeating.Temp`). Ohne `address:` springt
   der Wert hin und her – Sensoren deshalb immer an ein Gerät binden (Heizung 0x0201,
   Panel 0x0101, Elektroblock 0x0405).

### Neu koppeln Schritt für Schritt

1. Panel stromlos machen und wieder einschalten (nur nötig, wenn vorher eine Verbindung
   bestand oder geflasht wurde).
2. Am Panel unter *Verbundene Smartphones* **alle** Einträge des ESP löschen.
3. In HA „Kopplung löschen“ drücken.
4. Am Panel „Neues Gerät hinzufügen“ starten, direkt danach in HA „Panel koppeln“.
5. Kontrollieren: am Panel steht **genau ein** Eintrag mit Bluetooth-Symbol.

### Diagnose-Sensoren

- „Letzte Trennung“: Zeitpunkt, Grund (HCI-Code) und Dauer der letzten Verbindung
- „BLE freie Plätze“, „BLE-Modul Zustand“: Zustand des Bluetooth-Moduls im Panel
- „Firmware-Version“: Version dieser Komponente
- „Funkverbindung“: tatsächlich verwendetes Verbindungsintervall und Funk-Timeout

---

## Warum das stabiler sein sollte

Die HA-Integration funktioniert, aber die Verbindung führt über viele Ebenen, die sie
selbst nicht kontrolliert. Ihre eigene Doku beschreibt die Ursachen:

| Problem mit HA + Proxy | Mit dem dedizierten ESP |
|---|---|
| HA wählt **bei jedem Verbindungsaufbau neu**, ob es über den Proxy oder den lokalen Adapter geht. Die Kopplung (Bond) existiert aber nur auf *einem* davon. | Es gibt genau einen Central, und der hat den Bond. |
| Das Panel nutzt eine **ständig wechselnde Bluetooth-Adresse (RPA)**. Ob der Host die auflösen kann, hängt von Kernel und BlueZ ab (Regression ab Linux 6.19). | Der ESP speichert beim Koppeln den Identitätsschlüssel (IRK) des Panels und löst die Adresse selbst auf (AES, wie in der BT-Spezifikation). |
| Der Proxy teilt seine Verbindungs-Slots mit anderen BLE-Geräten (Batterie-BMS, Sensoren …). | Der ESP macht nichts anderes. |
| BlueZ-Schreibzugriffe ohne Timeout, hängende Disconnects, Reload-Probleme in HA. | Jede Stufe hat ihr eigenes Timeout, und der ESP verbindet sich selbstständig neu. |
| Stromsparen/Neustarts des HA-Hosts unterbrechen die Verbindung. | Der ESP hält die Verbindung auch, wenn HA neu startet. Die Werte sind sofort wieder da. |

Was bleibt, sind Eigenheiten des **Panels selbst**:
- Nach einer getrennten Verbindung nimmt das Panel keine neue an, bis es stromlos gemacht
  wurde (siehe „Wichtigste Erkenntnisse“).
- Nach einem Neustart ist der interne Bus ca. 90 s „taub“. Der ESP wiederholt automatisch.

---

## Hardware

- **Klassischer ESP32** (ESP32-WROOM / „ESP32 DevKit“, `board: esp32dev`)

> **Wichtig:** Mit dem **ESP32-C3** (und vermutlich S3/C6) klappen Koppeln und Datenverbindung,
> aber **keine Wiederverbindung** nach einem Abbruch. Der Bluetooth-Stack dieser Chips sucht
> gekoppelte Geräte über die Adressauflösung im Funkchip, und das findet das Panel nicht
> (gemessen am 14.11.2026). Der klassische ESP32 löst die Adresse in Software auf, genau wie
> der ESPHome-Bluetooth-Proxy, mit dem die Wiederverbindung laut Original-Projekt funktioniert.
- USB-Stromversorgung aus dem Bordnetz (12 V → 5 V USB-Adapter)
- **Platzierung:** möglichst nah am Panel (wenige Meter, Sichtlinie). Entfernung zeigt sich
  als Verbindungsabbrüche.


---

## Koppeln im Detail

1. Panel im **normalen Bildschirm** lassen.
2. In Home Assistant den Button **„Panel koppeln“** drücken. Der ESP ist jetzt 120 s im
   Kopplungsmodus (siehe Sensor „Verbindungsstatus“).
3. **Jetzt** das Panel in den **Gerät-hinzufügen-Modus** setzen (Truma-App oder Panel-Menü).
4. Warten. Im Log erscheint `Paired with the Truma panel`, danach `Connected`.

Hinweise:
- Das Panel zeigt am Ende evtl. **„Etwas ist schiefgelaufen“**. Das bezieht sich nur auf
  den App-Handshake von Truma und kann ignoriert werden. Maßgeblich sind ein grüner Balken
  über dem Bluetooth-Symbol am Panel und der Sensor „Panel verbunden“.
- **Auf dem Campingplatz:** Im Kopplungsmodus verbindet sich der ESP nur mit einem Panel,
  das *ohne Namen* wirbt. Das tut nur ein Panel im Gerät-hinzufügen-Modus. Fremde Panels
  im Normalbetrieb senden ihren Namen und werden übersprungen. Ein Panel, das die
  Kopplung verweigert, wird für den Rest des Kopplungsfensters ignoriert.
- Nach dem Koppeln verbindet sich der ESP **nur noch mit diesem einen Panel**
  (Adressauflösung über den gespeicherten IRK).
- Status **„Panel rejects this device: pair again“**: Das Panel hat den ESP vergessen.
  Geräteliste am Panel leeren und neu koppeln.

---

## Konfiguration

```yaml
truma_inetx:
  id: truma_hub
  time_id: ha_time          # optional: Uhrzeit fürs Panel
  user_name: "Home Assistant"
  pairing_timeout: 120s
  log_parameters: false     # true = jede Wertänderung ins Log
  language: de              # en | de: Sprache von Status, „Letzte Trennung“ und Zeitschaltuhren
  supervision_timeout: 20s  # so lange überlebt die Verbindung ohne Lebenszeichen vom Panel (2–32 s)
  time_sync: true           # Panel-Uhr nach dem Verbinden und täglich aus time_id stellen
  startup_delay: 28s        # nach dem Start so lange mit dem Verbinden warten (Standard: supervision_timeout + 8 s)
  disconnect_on_shutdown: false  # Testschalter: vor einem geplanten Neustart sauber trennen
```

**Nach einem Neustart wartet der ESP, bevor er sich verbindet** (`startup_delay`,
standardmäßig `supervision_timeout` + 8 s, also 28 s). So lange hält das Panel noch die
Verbindung von vor dem Neustart, und ein Verbindungsversuch wird abgelehnt (Log: `status=133`
oder `0x3e`) – mit Panel-Software 3.5.38 lehnte das Panel danach jeden Versuch ab, bis es
stromlos war. Von einem Tester bestätigt: Mit der Wartezeit verbindet sich der ESP nach einem
OTA-Update von selbst wieder. `startup_delay: 0s` schaltet das ab.
`disconnect_on_shutdown: true` (trennt vor einem Neustart sauber) bleibt als Testschalter;
gebraucht wurde er bisher nicht.

**Elektroblock NE315 Gen2** (Nordelettronica, Adresse 0x0405) meldet zusätzlich Ladezustand
(`L1Bat.SoC`, %), Strom (`L1Bat.Current`, 0,01 A, + = Laden) und Nennkapazität
(`L1Bat.NomCapacity`, Ah) der Aufbaubatterie sowie D+ (`PowerSupply.DPlus`). Die
Beispiel-YAML enthält diese Werte; bei anderen Geräten bleiben sie „Unbekannt“.

Der Bus hinter dem Panel ist generisch. Jeder Wert ist `topic` + `param` und kommt von einem
Gerät mit Busadresse (Panel 0x0101, Heizung 0x0201, Elektroblock 0x04xx, Gasflaschen 0x06xx …).
Ohne `address:` wird das Gerät genommen, das den Wert zuletzt gemeldet hat. Mit
`address: 0x0603` legst du es fest, z. B. bei zwei Gasflaschen.

**Welche Werte gibt es bei mir?** Button „Alle Parameter ins Log“ drücken. Dann steht
jeder Parameter jedes Geräts mit aktuellem Wert im Log. Alternativ `log_parameters: true`.

| Plattform | Optionen |
|---|---|
| `sensor` | `topic`, `param`, `address`. Zahlenwert roh, skalieren mit `filters: - multiply: 0.1`. Listen liefern ihre Länge. |
| `binary_sensor` | `topic`/`param`/`address` und optional `on_values: [1]` (Standard: alles ≠ 0), **oder** `type: connected \| paired \| pairing_mode` |
| `text_sensor` | `topic`/`param`/`address` mit optional `format: timer` (Zeitschaltuhr lesbar) **oder** `format: firmware` + `address` (Firmware-Version eines Geräts), **oder** `type: status \| version \| last_disconnect \| link_params` |
| `number` | `topic`, `param`, `address`, `min_value`, `max_value`, `step`, `scale` (Anzeige = Wert × scale; Temperaturen: `0.1`) |
| `select` | `topic`, `param`, `address`, `options: {0: "Eco", 1: "Comfort"}` |
| `switch` | `topic`, `param`, `address`, `on_value` (1), `off_value` (0) |
| `button` | `type: pair \| unpair \| reconnect \| dump \| log_raw \| measure \| sync_time \| write` (bei `write`: `topic`, `param`, `value`, `address`; `log_raw` schreibt 2 min lang jede Meldung komplett ins Log) |
| `climate` | `supported_modes: [HEAT, FAN_ONLY, COOL, AUTO, DRY]`, `appliance_address: 0x0201` (Heizung, deren Werte angezeigt werden) |

Die Thermostat-Entität funktioniert so:
- Der Modus ist `RoomClimate.Mode` am Panel.
- Die Solltemperatur liegt je nach Modus in `AirHeating.TgtTemp`, `AirCooling.TgtTemp`
  (Klimaanlage) oder `RoomClimate.TgtTemp` (Auto).
- Die Isttemperatur ist `AirHeating.Temp`.

Schreibzugriffe gehen an das Gerät, das den Wert meldet. Nur `RoomClimate` geht ans Panel.

### Frostschutz

Der ESP passt selbst auf Frost auf – auch wenn Home Assistant oder das WLAN ausfällt (er braucht
nur die Verbindung zum Panel). Ist **„Frostschutz“** an, die Heizung aus und die Raumtemperatur
10 Minuten lang unter der **„Frostschutz-Schwelle“** (Standard 5 °C), stellt er die Heizung auf
**Heizen 5 °C** (der niedrigste Sollwert des Panels) und vermerkt das im **„Frostschutz Status“**.
Eine von Hand eingestellte Heizung wird nie verändert. In der Beispiel-YAML standardmäßig aus.

**„Frostschutz Auto-Aus“** (standardmäßig aus, wirkt nur zusammen mit „Frostschutz“): Steht die
Heizung auf **Heizen mit 5 °C**, schaltet der ESP sie wieder aus, sobald es innen 5 °C hat – egal
wer sie so eingestellt hat, auch nach einem Neustart. 5 °C gilt als Frostschutzbetrieb; ein
höherer Sollwert ist echtes Heizen und bleibt unangetastet. Das spart Gas, weil der Innenraum im
Mittel kälter bleibt – dafür springt die Heizung in kalten Nächten öfter an und aus. Der Status
zeigt dann z. B. `Fertig 03:40 · 5.0 °C`.

Beim Einwintern, wenn die Heizung aus bleiben soll, ausschalten. Warnen, wenn die Verbindung zum
Panel, das Gas oder die Batterie ausfällt, kann der ESP nicht – das gehört in Home Assistant,
z. B. eine Benachrichtigung, wenn „Panel verbunden“ 10 Minuten aus ist oder „Störung“ an.

### Gasflaschen (Truma LevelControl)

Sind eine oder zwei Truma LevelControl am iNet X Panel angemeldet (der Füllstand
erscheint im Panel), reicht das Panel die Werte weiter. Die Beispiel-YAML hat zwei
Geräte, „Truma Gasflasche 1“ und „2“, jeweils mit Füllstand (%), Inhalt (kg), Temperatur
und der Bezeichnung der Flasche am Panel (z. B. „Links“). `instance: 1` / `instance: 2`
wählt die erste bzw. zweite Flasche nach Busadresse, eine Adresse muss also niemand
heraussuchen. Mit nur einer oder keiner Flasche bleiben die übrigen Entitäten einfach
„Unbekannt“.

Wird eine LevelControl am Panel neu angemeldet, können Flasche 1 und 2 die Plätze
tauschen; die Entität „Bezeichnung“ zeigt, welche welche ist. Von einem Nutzer mit zwei
LevelControl (Combi 6) getestet: Füllstand und Inhalt stimmen mit dem Panel überein
(z. B. 96 % = 10,6 kg bei einer 11-kg-Flasche). Rückmeldungen mit anderen Ausstattungen
gern als Issue.

### Truma Aventa Compact Plus 2. Generation (Klimaanlage)

[`aventa-compact-plus-2.example.yaml`](aventa-compact-plus-2.example.yaml) ist eine
komplette Konfiguration für das Aventa Compact Plus 2. Gen. unter Adresse `0x0801`: Klima mit
Heizen, Kühlen, Automatik, Lüften und Entfeuchten, Lüfterstufe, Ambientebeleuchtung (Ein/Aus und
Helligkeit), Netzanschluss und Diagnose. Heizen über das Aventa braucht v0.8.6 oder neuer.
Beigesteuert und getestet von einem Nutzer (Aventa-Firmware 1.10.5, Panel 3.5.38) – danke!

### Eigene Aktionen in Home Assistant

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

## Wie es intern funktioniert

1. **Finden:** Das Panel wirbt mit der Service-UUID `fc310002-…`. Seine Adresse wechselt
   ständig. Sie wird gegen den beim Koppeln gespeicherten IRK geprüft.
2. **Verbinden & verschlüsseln:** Erst Verschlüsselung (Bond), dann Notifications
   auf CMD und DATA_R. Upstream hat gezeigt: andersherum bricht das Panel ab.
3. **Session:**
   - Registrierung (Panel vergibt eine Busadresse)
   - Topics abonnieren
   - Identität und Uhrzeit senden
   - Parameter-Abfrage an jedes Busgerät (nur Panel und Heizung antworten auf Broadcast)
   - Tankmessung anstoßen
4. **Halten:** Werte kommen als Push. Tanks werden jede Minute neu gemessen.
5. **Überwachung:** Kommen 90 s lang keine Daten, wird die Sitzung über die bestehende
   Funkverbindung neu gestartet. Die Funkverbindung selbst trennt der ESP nie.

Transport: Jede Nachricht läuft über `InitDataTransfer → Ready → Daten → DataAck`.
Bleibt ein Ack aus, wird die Sitzung neu gestartet. Das Ack trägt keine Kennung,
deshalb wäre die Zuordnung danach nicht mehr sicher (upstream gemessen).

---

## Tests

```bash
cd tests
g++ -std=c++17 -I../components/truma_inetx test_core.cpp ../components/truma_inetx/truma_core.cpp -lcrypto -o test_core
./test_core        # -v für ausführliches Log
```

Was die Tests abdecken:
- CBOR (RFC-8949-Testvektoren)
- Frame-Aufbau (byte-identisch zur Python-Referenz)
- RPA-Auflösung (Testvektor aus der Bluetooth-Spezifikation)
- eine komplette Session gegen ein simuliertes Panel: fragmentierte Antworten bei MTU 23,
  Geräte ohne Ack, Schreiben mit Echo, abgelehnte Writes, Stall-Watchdog, verirrte
  Ready/Ack-Pakete

---

## Bei Problemen

- Log-Level `DEBUG` (oder `logs: { truma_inetx: VERBOSE }`). Der Sensor „Verbindungsstatus“
  zeigt die aktuelle Stufe.
- „Letzte Trennung“ zeigt einen Funk-Timeout (0x08): Der ESP32 teilt sich ein Funkteil
  zwischen WLAN und Bluetooth. **Zuerst „WLAN-Signal“ prüfen: besser als −70 dBm.** Ein Tester
  hatte bei −81 dBm (4G-Router plus WireGuard) alle 1–2 Stunden einen Abbruch; mit starkem
  Signal lief die Verbindung danach tagelang ohne Abbruch. Im Router einen **festen WLAN-Kanal** einstellen (automatische
  Kanalwechsel lassen den ESP neu suchen) oder ein ESP32-Board mit LAN verwenden
  (z. B. WT32-ETH01, Olimex ESP32-POE), dann hat Bluetooth das Funkteil für sich allein.
- Dauerhaft Status 0x3E / `connect failed` nach einer Trennung oder nach dem Flashen:
  Panel stromlos machen. Nur wenn der Status danach „Panel lehnt den ESP ab“ zeigt, neu koppeln
  (siehe „Neu koppeln Schritt für Schritt“).
- `the panel assigned no address` direkt nach Panel-Neustart: normal, verschwindet nach ca. 1–2 Minuten.

---

## Lizenz & Dank

- Protokollwissen und Messungen: [rpodgorny/hass-truma-inetx](https://github.com/rpodgorny/hass-truma-inetx) (GPL-3.0)
- Ursprüngliches Reverse Engineering: [daaaaan/truma-inetx-ble](https://github.com/daaaaan/truma-inetx-ble)

Der C++-Code ist eine Neuimplementierung, es wurde kein Code aus dem unlizenzierten
Upstream-Protokollmodul übernommen. Weil Logik und Messwissen aus dem GPL-Projekt stammen,
sollte dieses Projekt ebenfalls unter **GPL-3.0** veröffentlicht werden.

„Truma“ und „iNet X“ sind Marken der Truma Gerätetechnik GmbH & Co. KG. Dieses Projekt ist
nicht mit Truma verbunden.
