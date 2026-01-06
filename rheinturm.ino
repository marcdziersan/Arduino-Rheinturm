/*
  ==============================================================================
  Rheinturm-Uhr (Original-Logik) – 60 LEDs (WS2812B) + Arduino UNO + RTC
  Version: 1.0.1
  ==============================================================================
  GitHub-ready Single-File Sketch (Arduino IDE)

  Projekt:
    Rheinturm-Lichtzeituhr (Lichtzeitpegel) als NeoPixel/WS2812B-Umsetzung.
    Anzeige basiert auf unärer Darstellung der Uhrzeit (Stunden/Minuten/Sekunden),
    ergänzt durch Trenner-Animation und Funkfeuer-Blinken.

  Autor:
    Marcus Dziersan

  Lizenz:
    MIT License

    Copyright (c) 2026 Marcus Dziersan

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

  ------------------------------------------------------------------------------
  1) Features (v1.0.0)
  ------------------------------------------------------------------------------
  - 60-LED Layout nach Rheinturm-Logik (62 Bullaugen adaptiert auf 60 LEDs)
  - Unäre Zeitanzeige:
      * Sekunden: 9 Einer + 5 Zehner
      * Minuten : 9 Einer + 5 Zehner
      * Stunden : 9 Einer + 2 Zehner
  - Trenner-Animation (orange):
      * Grundglimmen immer
      * Akzent bei voller Minute (s == 0): alle Trenner kurz hell
      * Akzent bei voller Stunde (m == 0): Stunden-Trenner 1 Minute hell
  - Funkfeuer (rot blinkend, 1 Hz) mit je 1 LED (gesparte LEDs -> oben wieder 5 Bullaugen)
  - Warmweiß-Farbton für die Zeit
  - Helligkeitslimit (~30%) + optionales Power-Limit (FastLED)

  ------------------------------------------------------------------------------
  2) Hardware & Verdrahtung
  ------------------------------------------------------------------------------
  Benötigt:
    - Arduino UNO
    - WS2812B/NeoPixel LED Strip mit 60 LEDs
    - RTC-Modul (DS3231 empfohlen; DS1307 möglich)
    - 5V Netzteil (empfohlen: 5V / 3A)
    - 330 Ohm Widerstand (Datenleitung, in Reihe)
    - Elko z.B. 1000 µF (zwischen +5V und GND nahe Strip-Eingang; empfohlen)

  Anschlüsse (WS2812B / NeoPixel):
    - Netzteil +5V  -> Strip 5V (bei dir: weiß)
    - Netzteil GND  -> Strip GND (bei dir: braun)
    - Arduino GND   -> Strip GND (gemeinsame Masse ist Pflicht!)
    - Arduino D6    -> 330 Ohm -> Strip DIN (bei dir: grün)

  RTC am Arduino UNO (I²C):
    - RTC VCC -> UNO 5V
    - RTC GND -> UNO GND
    - RTC SDA -> UNO A4
    - RTC SCL -> UNO A5

  Hinweise:
    - Achte auf die Pfeilrichtung des Strips: Daten müssen an DIN rein.
    - Bei Flackern/Reset: zusätzliches Einspeisen (Power-Injection) am Strip-Ende kann helfen.
    - Power-Limit ist konservativ gesetzt (2500 mA) und kann angepasst werden.

  ------------------------------------------------------------------------------
  3) 60-LED Mapping (LED0 unten -> LED59 oben)
  ------------------------------------------------------------------------------
  Farben:
    - Zeit (C_TIME)      : warmweiß (RGB-Mix)
    - Dummies (C_DUMMY)  : gedimmt gelb (Bullaugen ohne Funktion)
    - Trenner (C_SEP_*)  : orange (dim + Akzent)
    - Funkfeuer (C_BEACON): rot blinkend (1 Hz)

  Segmentaufteilung:
    0-10   unten "ohne Funktion" (11)   -> C_DUMMY

    11-19  Sekunden Einer (9)           -> unär, C_TIME
    20     Trenner Sekunden (1)         -> C_SEP_DIM / C_SEP_HI
    21-25  Sekunden Zehner (5)          -> unär (0..5), C_TIME
    26     Funkfeuer 1 (1)              -> blink rot (1 Hz)

    27-35  Minuten Einer (9)            -> unär, C_TIME
    36     Trenner Minuten (1)          -> C_SEP_DIM / C_SEP_HI
    37-41  Minuten Zehner (5)           -> unär (0..5), C_TIME
    42     Funkfeuer 2 (1)              -> blink rot (1 Hz)

    43-51  Stunden Einer (9)            -> unär, C_TIME
    52     Trenner Stunden (1)          -> C_SEP_DIM / C_SEP_HI
    53-54  Stunden Zehner (2)           -> unär (0..2), C_TIME

    55-59  oben "ohne Funktion" (5)     -> C_DUMMY

  ------------------------------------------------------------------------------
  4) Unär-Funktion (Lichtzeitpegel)
  ------------------------------------------------------------------------------
  setUnary(range, value):
    - value = Anzahl LEDs, die von unten innerhalb der Range leuchten.
    - Der Rest bleibt aus.
    - value wird auf Range-Länge begrenzt.

  Beispiel: Minuten 34
    - Zehner (3) -> 3/5 LEDs an
    - Einer  (4) -> 4/9 LEDs an

  ------------------------------------------------------------------------------
  5) Build / Dependencies
  ------------------------------------------------------------------------------
  Arduino IDE:
    - Installiere Bibliotheken:
        * FastLED (Daniel Garcia u.a.)
        * RTClib (Adafruit)

  RTC-Typ:
    - Standard: RTC_DS3231
    - Für DS1307:
        * RTC_DS1307 rtc; verwenden
        * ggf. rtc.lostPower() Verhalten je Modul prüfen

  ------------------------------------------------------------------------------
  6) Konfiguration (häufige Anpassungen)
  ------------------------------------------------------------------------------
  - BRIGHTNESS: globale Helligkeit (0..255)
  - C_TIME: Warmweiß-Ton nach Geschmack
  - Power-Limit: setMaxPowerInVoltsAndMilliamps(5, 2500) -> je Netzteil/Strip anpassen

  ==============================================================================
*/

#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>

#define LED_PIN     6
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

/*
  RTC-Auswahl:
    - DS3231: RTC_DS3231 rtc;
    - DS1307: RTC_DS1307 rtc;
*/
RTC_DS3231 rtc;

// ~30% Helligkeit (255 * 0.30 ≈ 76.5)
const uint8_t BRIGHTNESS = 77;

// Farben
const CRGB C_TIME    = CRGB(255, 150, 70); // warmweiß (deutlich warm)
const CRGB C_BEACON  = CRGB::Red;          // Funkfeuer
const CRGB C_DUMMY   = CRGB(40, 18, 0);    // Dummies (Bullaugen ohne Funktion)
const CRGB C_SEP_DIM = CRGB(18, 6, 0);     // Trenner Grundglimmen
const CRGB C_SEP_HI  = CRGB(255, 90, 0);   // Trenner Akzent

// Range-Definition
struct Range { uint8_t start; uint8_t len; };

// Dummies
const Range R_DUMMY_BOTTOM = {0, 11};
const Range R_DUMMY_TOP    = {55, 5};

// Sekunden
const Range  R_S_U      = {11, 9};
const uint8_t I_S_SEP   = 20;
const Range  R_S_T      = {21, 5};
const uint8_t I_BEACON1 = 26;

// Minuten
const Range  R_M_U      = {27, 9};
const uint8_t I_M_SEP   = 36;
const Range  R_M_T      = {37, 5};
const uint8_t I_BEACON2 = 42;

// Stunden
const Range  R_H_U    = {43, 9};
const uint8_t I_H_SEP = 52;
const Range  R_H_T    = {53, 2};

// Hilfsfunktion: Range komplett färben
static inline void fillRange(const Range& r, const CRGB& c) {
  for (uint8_t i = 0; i < r.len; i++) leds[r.start + i] = c;
}

// Hilfsfunktion: unäre Darstellung
static inline void setUnary(const Range& r, uint8_t value, const CRGB& onColor) {
  if (value > r.len) value = r.len;
  for (uint8_t i = 0; i < r.len; i++) {
    leds[r.start + i] = (i < value) ? onColor : CRGB::Black;
  }
}

void setup() {
  // I²C für RTC
  Wire.begin();

  // LEDs
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // Optional: konservatives Stromlimit (hilft bei 5V/3A)
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2500);

  FastLED.clear(true);

  // RTC initialisieren
  if (!rtc.begin()) {
    // Wenn RTC nicht gefunden wird, stoppt der Sketch absichtlich,
    // damit die Fehlersuche eindeutig bleibt (Verkabelung/Modultyp).
    while (true) { delay(1000); }
  }

  // Optional: Uhrzeit setzen, falls RTC Strom verloren hat.
  // Setzt auf Compile-Zeitpunkt. (PC-Uhr sollte korrekt sein.)
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  // Zeit lesen
  DateTime now = rtc.now();
  const uint8_t h = now.hour();
  const uint8_t m = now.minute();
  const uint8_t s = now.second();

  // Funkfeuer: 1 Hz (sekundengenau)
  const bool beaconOn = (s % 2 == 0);

  // Trenner-Akzentlogik:
  // - volle Minute: alle Trenner kurz hell
  // - volle Stunde: Stunden-Trenner eine Minute lang hell
  const bool fullMinute = (s == 0);
  const bool fullHour   = (m == 0);

  // Frame löschen
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // Dummies (unten/oben) setzen
  fillRange(R_DUMMY_BOTTOM, C_DUMMY);
  fillRange(R_DUMMY_TOP,    C_DUMMY);

  // Zeit zerlegen
  const uint8_t sU = s % 10;
  const uint8_t sT = s / 10; // 0..5

  const uint8_t mU = m % 10;
  const uint8_t mT = m / 10; // 0..5

  const uint8_t hU = h % 10;
  const uint8_t hT = h / 10; // 0..2

  // Unäranzeige (warmweiß)
  setUnary(R_S_U, sU, C_TIME);
  setUnary(R_S_T, sT, C_TIME);

  setUnary(R_M_U, mU, C_TIME);
  setUnary(R_M_T, mT, C_TIME);

  setUnary(R_H_U, hU, C_TIME);
  setUnary(R_H_T, hT, C_TIME);

  // Trenner: Grundglimmen
  leds[I_S_SEP] = C_SEP_DIM;
  leds[I_M_SEP] = C_SEP_DIM;
  leds[I_H_SEP] = C_SEP_DIM;

  // Akzent bei voller Minute: alle Trenner kurz hell
  if (fullMinute) {
    leds[I_S_SEP] = C_SEP_HI;
    leds[I_M_SEP] = C_SEP_HI;
    leds[I_H_SEP] = C_SEP_HI;
  }

  // Akzent bei voller Stunde: Stunden-Trenner eine Minute hell
  if (fullHour) {
    leds[I_H_SEP] = C_SEP_HI;
  }

  // Funkfeuer (rot blinkend) – je Funkfeuer nur 1 LED
  const CRGB b = beaconOn ? C_BEACON : CRGB::Black;
  leds[I_BEACON1] = b;
  leds[I_BEACON2] = b;

  // Ausgabe
  FastLED.show();

  // Kurze Pause: reicht für sauberes Blinken, hält CPU-Last klein
  delay(20);
}
