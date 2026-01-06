# Arduino Rheinturm Lichtzeitpegel Uhr

Nachbau des digitalen **Lichtzeitpegels** des Düsseldorfer Rheinturms mit **WS2812B-LEDs (60 LEDs)**, **Arduino UNO** und **RTC (DS3231/DS1307)**.

Dieses Projekt ist ein eigenständiger Nachbau mit eigener Implementierung und Dokumentation, funktional inspiriert von öffentlich beschriebenen Projekten (siehe Quellen).

---

## Überblick

Die Uhr zeigt die Zeit vertikal in **Dezimalstellen** (Stunden, Minuten, Sekunden) und visualisiert jede Stelle als **unäre Balkenanzeige**:
„So viele Bullaugen leuchten, wie der jeweilige Ziffernwert angibt.“

Zusätzlich gibt es:

* **Orange Trenner-Leuchten** (Grundglimmen + Akzent bei voller Minute/Stunde)
* **Rotes Funkfeuer** (blinkend)

---

## Funktionsprinzip

Beispiel **23:19:04**

| Stelle          | Wert | Darstellung |
| --------------- | ---: | ----------- |
| Sekunden Einer  |    4 | 4/9 LEDs an |
| Sekunden Zehner |    0 | 0/5 LEDs an |
| Minuten Einer   |    9 | 9/9 LEDs an |
| Minuten Zehner  |    1 | 1/5 LEDs an |
| Stunden Einer   |    3 | 3/9 LEDs an |
| Stunden Zehner  |    2 | 2/2 LEDs an |

**Trenner (orange):**

* glimmen immer schwach
* **volle Minute (s == 0):** alle Trenner kurz hell
* **volle Stunde (m == 0):** Stunden-Trenner eine Minute lang hell

**Funkfeuer (rot):**

* blinkt **1 Hz** synchron zur Sekunde

---

## Teileliste (BOM)

| Komponente           |      Menge | Hinweis                                 |
| -------------------- | ---------: | --------------------------------------- |
| Arduino Uno          |          1 | Steuerung                               |
| WS2812B LED Strip    | 1× 60 LEDs | 1 m / 60 LEDs                           |
| RTC Modul            |          1 | **DS3231 empfohlen**, DS1307 kompatibel |
| Netzteil 5V          |          1 | **≥ 3A empfohlen**                      |
| Elko 1000 µF / ≥ 10V |          1 | zwischen 5V/GND am Strip-Eingang        |
| Widerstand 330–470 Ω |          1 | in Reihe in die Datenleitung (DIN)      |
| Jumper-Kabel         |    diverse | je nach Aufbau                          |

**Bezugsquelle (meine Bestellung):**
roboter-bausatz.de: [https://www.roboter-bausatz.de/](https://www.roboter-bausatz.de/) ([Roboter-Bausatz.de][1])

---

## Verkabelung

### RTC (I²C) an Arduino UNO

| RTC Pin | Arduino Uno |
| ------- | ----------- |
| SDA     | A4          |
| SCL     | A5          |
| VCC     | 5V          |
| GND     | GND         |

Hinweis:

* DS1307 benötigt typischerweise eine CR2032, damit die Zeit stromlos erhalten bleibt.
* DS3231 ist in der Praxis deutlich genauer/stabiler.

### WS2812B Strip

| Strip | Arduino / Netzteil                                      |
| ----- | ------------------------------------------------------- |
| +5V   | +5V vom Netzteil                                        |
| GND   | GND vom Netzteil **und** Arduino-GND (gemeinsame Masse) |
| DIN   | Arduino **D6** über **330–470 Ω**                       |

**Stützkondensator (empfohlen):**

* Elko **1000 µF** direkt am Strip-Eingang zwischen **+5V** und **GND** (Polung beachten).

---

## Stromversorgung

WS2812B kann pro LED theoretisch bis zu **60 mA** ziehen.
60 LEDs → theoretisch **3,6 A** bei Vollweiß.

In diesem Projekt wird die Helligkeit softwareseitig begrenzt (**~30%**), daher reicht ein **5V/3A Netzteil** in der Praxis meist aus.

Wichtig:

* **Arduino-GND und Netzteil-GND müssen verbunden sein**, sonst ist das Datensignal instabil.

---

## Software / Bibliotheken

Arduino IDE → **Sketch → Bibliothek einbinden → Bibliotheken verwalten**

Benötigt:

1. **FastLED**
2. **RTClib (Adafruit)**
3. **Wire** (Standard)

Hinweis RTC-Typ:

* Standard im Code: `RTC_DS3231`
* Wenn du DS1307 nutzt: im Code auf `RTC_DS1307` umstellen.

---

## LED-Mapping (60 LEDs, LED0 unten → LED59 oben)

Diese Version bildet das Original sehr nah nach.
Um oben wieder **5 Bullaugen** zu ermöglichen, nutzen wir je Funkfeuer nur **1 LED**.

| Bereich          | Index | Länge | Funktion / Farbe      |
| ---------------- | ----- | ----: | --------------------- |
| Dummy unten      | 0–10  |    11 | gelb (gedimmt)        |
| Sekunden Einer   | 11–19 |     9 | warmweiß (unär)       |
| Trenner Sekunden | 20    |     1 | orange (dim + Akzent) |
| Sekunden Zehner  | 21–25 |     5 | warmweiß (unär)       |
| Funkfeuer 1      | 26    |     1 | rot blinkend          |
| Minuten Einer    | 27–35 |     9 | warmweiß (unär)       |
| Trenner Minuten  | 36    |     1 | orange (dim + Akzent) |
| Minuten Zehner   | 37–41 |     5 | warmweiß (unär)       |
| Funkfeuer 2      | 42    |     1 | rot blinkend          |
| Stunden Einer    | 43–51 |     9 | warmweiß (unär)       |
| Trenner Stunden  | 52    |     1 | orange (dim + Akzent) |
| Stunden Zehner   | 53–54 |     2 | warmweiß (unär)       |
| Dummy oben       | 55–59 |     5 | gelb (gedimmt)        |

---

## Installation & Upload

1. RTC-Batterie einsetzen (falls DS1307/DS3231-Modul Batteriefach hat)
2. Verkabelung herstellen (gemeinsame Masse!)
3. Bibliotheken installieren
4. Sketch öffnen
5. Board/Port auswählen
6. Upload durchführen

Optional:

* Wenn RTC „lost power“ meldet, setzt der Sketch die Zeit auf die Compile-Zeit.

---

## Troubleshooting

**1) Nichts leuchtet**

* DIN am falschen Strip-Ende (DOUT statt DIN)
* keine gemeinsame Masse (Arduino-GND nicht am Netzteil/Strip-GND)
* Datenpin falsch (Code nutzt D6)
* Strip bekommt keine 5V

**2) Flackern/Resets**

* Netzteil zu schwach / Leitungen zu dünn
* Elko fehlt
* ggf. zusätzliche Einspeisung am Strip-Ende („Power Injection“)

**3) RTC wird nicht erkannt**

* SDA/SCL vertauscht
* falscher RTC-Typ im Code (DS3231 vs DS1307)
* VCC/GND nicht korrekt

---

## Lizenz

MIT License – siehe `LICENSE`.

---

[1]: https://www.roboter-bausatz.de/"Roboter Bausatz, 3D-Drucker und DIY- Elektronik Shop in ..."
[2]: https://starthardware.org/arduino-rheinturm-lichtzeitpegel/"Arduino Rheinturm –Lichtzeitpegel - StartHardware" Inspiration
[3]: https://de.wikipedia.org/wiki/Lichtzeitpegel"Lichtzeitpegel"
