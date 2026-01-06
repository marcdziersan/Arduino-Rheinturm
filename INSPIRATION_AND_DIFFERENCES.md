# Inspiration und Abgrenzung (Provenance)

Dieses Dokument beschreibt, welche öffentlichen Projekte die **Idee** zu diesem Repository inspiriert haben und – wichtiger – **wodurch sich unsere Implementierung** in Aufbau, Mapping, Bibliotheken und Verhalten unterscheidet. Ziel ist Transparenz und ein klarer Nachweis, dass es sich um eine **eigenständige Umsetzung** handelt und nicht um ein Copy/Paste-Derivat.

---

## 1) Inspiration (Ideen-/Konzept-Ebene)

Der Düsseldorfer Rheinturm „Lichtzeitpegel“ wurde von vielen Maker:innen nachgebaut. Öffentliche Beschreibungen und Beispielsketche (u. a. verbreitete Ansätze, wie sie auch auf StartHardware vorgestellt werden) dienten als **konzeptionelle Inspiration** für:

- Darstellung der Uhrzeit als vertikale Anzeige je Dezimalstelle (unär / „so viele Lichter wie der Wert“)
- Einsatz von Trennern/Marker-Lichtern
- Verwendung einer RTC (DS1307/DS3231) und adressierbarer LEDs (WS2812-Familie)

Dieses Repository enthält **keine eingebetteten oder übernommenen Quelltexte** aus fremden Projekten. Die Implementierung hier ist **eigenständig** entwickelt und iterativ erarbeitet worden.

---

## 2) Ziel und Scope dieses Repositories

Dieses Repository verfolgt bewusst eine „**turm-treue**“ Umsetzung auf einem **60-LED-Strip**:

- Dummy-/„ohne Funktion“-Zonen unten und oben (Bullaugen ohne Anzeige-Funktion)
- dedizierte Trenner-Logik (Grundglimmen + Akzent bei Minute/Stunde)
- dedizierte Funkfeuer-Logik (Blinken als separater Bereich)
- Warmweiß-Abstimmung der Zeitanzeige
- Helligkeitsbegrenzung und optionales Stromlimit für stabile Versorgung

Ziel ist nicht ein minimaler Demo-Aufbau, sondern eine Darstellung, die sich in Struktur und Wirkung **näher am Rheinturm** orientiert – angepasst auf marktübliche Hardware (UNO + 60 LEDs).

---

## 3) Wesentliche Unterschiede zu typischen Inspirations-Sketches

### A) LED-Anzahl und physisches Mapping
Viele Inspirations-Sketche nutzen **51 LEDs** (oder ähnliche Minimalzahlen) und bilden den Turm damit vereinfacht ab.

Unsere Version nutzt **60 LEDs** und bildet bewusst zusätzliche Zonen ab:
- unten eine Dummy-Zone (11 LEDs)
- oben eine Dummy-Zone (5 LEDs)
- Sekunden/Minuten/Stunden-Segmente nach Turm-Logik
- Trenner und Funkfeuer als explizite Indizes

Ergebnis: Das Layout nutzt das Band vollständig und wirkt „turmähnlicher“.

---

### B) Funkfeuer („Funkfeuer-Bullaugen“) als eigene Logik
Viele Inspirationscodes modellieren Trenner (rot blinkend) über eine generische „Trenner“-Kategorie im Layout.

Unsere Implementierung modelliert Funkfeuer **explizit** und verwendet je Funkfeuer **nur 1 LED** (statt 2), um bei 60 LEDs die oberen Dummy-Bullaugen **wieder vollständig** abbilden zu können. Das ist eine bewusst getroffene Mapping-Entscheidung für ein turm-treues Gesamtbild.

---

### C) Architektur: Segment-Ranges statt Masken-Array
Inspiration-Sketche arbeiten häufig so:
- Ein statisches Layout-Array definiert „aus / Zeit / Abstand / Trenner“
- Dieses Array wird in jedem Loop zurückgesetzt und dann überschrieben

Unsere Implementierung arbeitet segmentbasiert:
- benannte Bereiche (Start/Length) je Segment
- wiederverwendbare Render-Funktionen (`setUnary`, `fillRange`)
- klare Trennung: „Zeit berechnen“ vs. „Segmente rendern“

Ergebnis: besser wartbar, leichter auditierbar und einfacher auf andere LED-Längen skalierbar.

---

### D) Bibliotheken (LED + RTC) und daraus resultierende Implementierungsunterschiede
Typische Inspirationscodes:
- LEDs: **Adafruit_NeoPixel**
- RTC: **Makuna Rtc** (`RtcDS1307`)

Unser Code:
- LEDs: **FastLED**
- RTC: **Adafruit RTClib** (DS3231 oder DS1307 umschaltbar)

Das sind unterschiedliche APIs und führen zu einer anderen technischen Umsetzung (Puffer-Handling, Helligkeits-/Power-Limits, RTC-Status/Initialisierung etc.). Allein dadurch ist ein 1:1 Kopieren praktisch nicht gegeben.

---

### E) Zeitverhalten / Animation (RTC-synchron statt freilaufend)
Viele Inspirationscodes blinken Trenner per `millis()` mit frei definierbarer Blinkfrequenz (freilaufend).

Unser Code:
- Funkfeuer blinkt **sekundensynchron** (1 Hz, gekoppelt an RTC-Sekunden)
- Trenner-„Akzent“ orientiert sich an **Zeitereignissen**:
  - Grundglimmen immer
  - volle Minute: alle Trenner kurz hell
  - volle Stunde: Stunden-Trenner eine Minute lang hell

Ergebnis: Animation wirkt „uhrtypisch“ und an Zeitpunkte gebunden statt rein periodisch.

---

### F) Farb- und Stabilitäts-Tuning
Viele Inspirationscodes nutzen ein neutrales Grau/Weiß und setzen ansonsten nur „rot“ für Trenner.

Unsere Version nutzt eine gezielte Farb- und Leistungsstrategie:
- Warmweiß (RGB-Mix) für die Zeit
- gedimmtes Gelb für Dummy-Bullaugen
- orange Trenner in zwei Stufen (dim + Akzent)
- globale Helligkeitsbegrenzung (~30%)
- optionales Stromlimit (`setMaxPowerInVoltsAndMilliamps`) für 5V/3A-Setups

Ergebnis: stabileres Verhalten und klarere optische Hierarchie.

---

## 4) Was ausdrücklich NICHT gemacht wurde

- Keine Übernahme fremder Quelltexte in dieses Repository
- Keine Übernahme fremder Layout-Maskenarrays (Indexmasken) „as is“
- Keine Übernahme fremder Dokumentationstexte
- Mapping, Segmentlogik, Farben und Animationen wurden eigenständig für die konkrete Hardware (UNO + 60 LEDs + RTC) umgesetzt

---

## 5) Audit-/Vergleichshinweise (Nachweis durch Unterschiede)

Beim Vergleich mit typischen Inspirationscodes sind die Unterschiede sofort sichtbar:
- andere LED-Anzahl und Segment-Indizes
- andere Layout-Strategie (Ranges statt Masken)
- andere Bibliotheken (FastLED + RTClib statt NeoPixel + Makuna)
- andere Animationslogik (RTC-synchron, Akzent-Logik)
- andere Farb-/Helligkeits- und Power-Strategie
- andere Struktur und Wartbarkeit des Codes

Diese Unterschiede spiegeln eine eigenständige Implementierung wider, die auf ein turm-treues 60-LED-Modell optimiert wurde.

---

## 6) Credits / Referenzen

- Öffentliche Maker-Projekte zum Rheinturm-Lichtzeitpegel dienten als Inspiration auf Ideenebene.
- Der Rheinturm-Lichtzeitpegel ist eine bekannte öffentliche Installation; dieses Repository ist ein Hobby-/Lern-Nachbau zu Bildungszwecken.
