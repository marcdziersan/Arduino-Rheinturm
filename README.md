# Arduino Rheinturm Lichtpegel Uhr

Nachbau des digitalen Lichtzeitpegels des Düsseldorfer Rheinturms mit WS2812-LEDs und DS1307-RTC

## Übersicht

Dieses Projekt bildet den berühmten **Lichtzeitpegel des Düsseldorfer Rheinturms** mithilfe eines Arduino und adressierbarer LEDs nach. Die Uhr zeigt die Zeit vertikal in **dezimale Einzelstellen** (Stunden, Minuten, Sekunden), getrennt durch rote Trennlampen.

Das Projekt basiert funktional auf dem öffentlich beschriebenen Vorbildprojekt von **StartHardware.org (Stefan Hermann)**.
Diese Dokumentation erläutert Schaltung, Aufbau, Funktionsprinzip, benötigte Komponenten und die Softwarelogik in einer zusammenhängenden, klar strukturierten Form.

---

## Funktionsprinzip

Die Rheinturmuhr teilt die Uhrzeit in ihre Dezimalstellen und visualisiert jede Stelle durch eine **stacked unary representation**:

Beispiel:
**23:19:04 →**

Von unten nach oben:

| Abschnitt         | Anzeige    |
| ----------------- | ---------- |
| Sekunden (Einer)  | 4 Leuchten |
| Sekunden (Zehner) | 0 Leuchten |
| Trennleuchte      | Rot        |
| Minuten (Einer)   | 9 Leuchten |
| Minuten (Zehner)  | 1 Leuchte  |
| Trennleuchte      | Rot        |
| Stunden (Einer)   | 3 Leuchten |
| Stunden (Zehner)  | 2 Leuchten |

Zwischen den Blöcken befinden sich unbenutzte LEDs sowie die roten Trenner.

---

## Komponentenliste (BOM – Bill of Materials)

| Komponente           | Menge          | Beschreibung                                 |
| -------------------- | -------------- | -------------------------------------------- |
| Arduino Uno          | 1              | Mikrocontroller für die Steuerung            |
| WS2812B LED-Streifen | 1× 51 LEDs     | Mindestanzahl: 51 LEDs                       |
| DS1307 RTC-Modul     | 1              | Echtzeituhr mit I²C                          |
| Elko 1000 µF / 16 V  | 1              | Stützkondensator an der 5V-Versorgung        |
| Widerstand 300–500 Ω | 1              | Serienwiderstand für das Datensignal         |
| 5 V Netzteil         | ≥3 A empfohlen | Versorgung der LEDs                          |
| Jumper-Kabel         | diverse        | Aufbau auf Breadboard oder Direktverdrahtung |

---

## Schaltung / Verkabelung

### 1. Real Time Clock (DS1307)

| DS1307 | Arduino Uno |
| ------ | ----------- |
| SDA    | A4          |
| SCL    | A5          |
| VCC    | 5V          |
| GND    | GND         |

> Hinweis: Der DS1307 benötigt eine **CR2032-Batterie**, damit die Zeit im stromlosen Zustand gespeichert bleibt.

---

### 2. WS2812B LED-Streifen

| LED-Streifen | Arduino / Netzteil                            |
| ------------ | --------------------------------------------- |
| +5V          | 5V Netzteil                                   |
| GND          | Gemeinsames GND (Netzteil + Arduino!)         |
| DIN          | Arduino Pin 6 über 300–500 Ω Serienwiderstand |

### 3. Stützkondensator

Zwischen **+5V** und **GND** des LED-Streifens:

```
+5V ----||---- GND
         1000 µF Elko
```

Polung beachten (– an GND).

---

## Stromversorgung

Jede WS2812B kann bis zu **60 mA** aufnehmen.
51 LEDs → maximal ca. **3.06 A**.

Ein **stabilisiertes 5-V-Netzteil mit ≥3 A** wird empfohlen.

**GND vom Arduino und GND des Netzteils müssen verbunden werden**, damit das Datensignal ein gemeinsames Bezugspotenzial hat.

---

## Software – Bibliotheken

Über **Sketch → Bibliothek verwalten** installieren:

1. **Adafruit NeoPixel**
2. **RTC by Makuna**
3. **Wire (Standardbibliothek)**

---

## Softwareprinzip (funktionsorientierte Erklärung)

Die Software gliedert sich in folgende Schritte:

1. **Initialisierung der RTC**

   * Kommunikation prüfen
   * Zeit abrufen
   * Im Fehlerfall: RTC-Zeit auf Kompilierzeit setzen

2. **Generieren der LED-Zeitstruktur**

   * Sekunde, Minute, Stunde werden in Einer- und Zehnerstelle zerlegt
   * LED-Indexbereiche exakt den Abschnitten zugeordnet
   * LED-Muster über Arrays umgesetzt
     (0 = aus, 1 = weiß, 2 = Abstand, 3 = Trennleuchte rot)

3. **Ausgabe der Farben**

   * WS2812-Farbwerte setzen
   * Rote Trenner optional blinkend (per millis())

---

## LED-Indexbelegung (Schema 51 LEDs)

Von **unten (Index 0)** nach oben **(Index 50)**:

| Bereich           | Index | Funktion               |
| ----------------- | ----- | ---------------------- |
| Sekunden – Einer  | 0–9   | LED 0 = niedrigste LED |
| Sekunden – Zehner | 10–15 | Zehnerdarstellung      |
| Abstand           | 16    | unbeleuchtet           |
| Minuten – Einer   | 17–26 | Minuten-Einer          |
| Minuten – Zehner  | 27–32 | Minuten-Zehner         |
| Abstand           | 33    | unbeleuchtet           |
| Stunden – Einer   | 34–42 | Stunden-Einer          |
| Stunden – Zehner  | 43–50 | Stunden-Zehner         |

Im Code werden zur besseren Formatierung zusätzliche Abstandswerte verwendet.

---

## Installation & Upload

1. Arduino Uno anschließen
2. RTC-Batterie einsetzen
3. Bibliotheken installieren
4. Sketch öffnen
5. Board & Port auswählen
6. Upload durchführen
7. Serielle Konsole öffnen (115200 Baud) zur Kontrolle

Die Uhrzeit wird bei jedem Start automatisch angezeigt.

---

## Troubleshooting (häufige Fehlerquellen)

### 1. Obere LEDs bleiben dunkel

→ WS2812-Datenstörung

* schlechter DIN-Kontakt
* keine gemeinsame Masse
* fehlender oder falscher Serienwiderstand
* zu lange Datenleitung

### 2. Farben verschoben, LED reagiert falsch

→ Timingfehler

* Streifen ist kein WS2812B-kompatibler Typ
* SK6812 kann funktionieren, aber nicht immer zuverlässig
* Datenleitung zu lang → kürzen

### 3. RTC zeigt falsche Zeit

* Batterie leer / fehlt
* DS1307 ist empfindlich gegenüber Spannungsunterbrechungen
* DS3231 als hochwertigere Alternative möglich

### 4. Alle LEDs leuchten weiß

→ Datenleitung verliert Signal vollständig

* GND fehlt
* Netzteilinstabil
* Data-Pin falsch verdrahtet

---

## Erweiterungsmöglichkeiten

* NTP-Zeitsynchronisation mit ESP8266/ESP32
* größere LED-Zahl für maßstäbliche Modelle
* Plexiglaszylinder als Turmschaft
* RGB-Animationen
* DCF77-Zeitbasis

---

## Lizenzhinweis

Dieses Projekt basiert funktional auf einer öffentlich beschriebenen Anleitung von **StartHardware.org (Stefan Hermann)**.

Dies ist eine **eigene technische Dokumentation**, die:

* keine Textteile kopiert
* keine urheberrechtlich geschützten Originalpassagen verwendet
* keine nicht lizenzierte Codekopie enthält

Der Originalcode ist urheberrechtlich geschützt und wird daher **nicht in diese README eingebettet**, kann aber über die Originalquelle bezogen werden.

---

## Quellen / Referenzen

* StartHardware.org – Arduino Rheinturm-Uhr
* WS2812B Datasheet
* DS1307 Datasheet
* Adafruit NeoPixel Best Practices
