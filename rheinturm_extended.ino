/*
  ==============================================================================
  Rheinturm-Uhr (turm-treu) – 60 LEDs + Arduino UNO + RTC + Taster-Modi
  Version: 1.1.0
  ==============================================================================
  Features:
    - Turm-treues 60-LED Mapping (Dummy unten/oben, Segmente, Trenner, Funkfeuer)
    - RTC-Zeit (DS3231 oder DS1307 via RTClib)
    - Trenner-Logik: Grundglimmen + Akzent bei voller Minute/Stunde
    - Funkfeuer: 2 Beacons (je 1 LED), 1 Hz sekunden-synchron
    - 6 Modi per Taster:
        Mode 0: Original
        Mode 1: F95 rot/weiß
        Mode 2: grün/weiß
        Mode 3: blau/weiß
        Mode 4: Regenbogen
        Mode 5: Disco Light

  Anschlüsse:
    Strip:
      - Netzteil +5V  -> Strip 5V
      - Netzteil GND  -> Strip GND
      - UNO GND       -> Strip GND (gemeinsame Masse!)
      - UNO D6        -> 330–470 Ohm -> Strip DIN
    RTC (I²C):
      - SDA -> A4
      - SCL -> A5
      - VCC -> 5V
      - GND -> GND
    Taster:
      - D2  -> Taster -> GND (INPUT_PULLUP)

  Bibliotheken:
    - FastLED
    - RTClib (Adafruit)
    - Wire (Standard)

  Lizenz:
    MIT (siehe LICENSE im Repo; optional zusätzlich in Header/Repo-Datei)

*/

#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>

// ============================ LED / HW CONFIG ===============================

#define LED_PIN     6
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define BTN_PIN     2            // Taster an D2 nach GND (INPUT_PULLUP)

CRGB leds[NUM_LEDS];

// RTC-Typ umschalten, falls nötig:
// RTC_DS1307 rtc;
RTC_DS3231 rtc;

// ~30% Helligkeit
const uint8_t BRIGHTNESS = 77;

// Optional: konservatives Stromlimit (an Netzteil anpassen)
const uint16_t MAX_MA = 2500;

// Button-Handling
const uint16_t DEBOUNCE_MS  = 35;
const uint16_t LONGPRESS_MS = 1200;

// Modes
const uint8_t MODE_COUNT = 6;
uint8_t mode = 0;

// ============================ MAPPING (turm-treu) ============================

struct Range { uint8_t start; uint8_t len; };

// Dummy-Zonen
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

// ============================ RENDER HELPERS =================================

static inline void fillRange(const Range& r, const CRGB& c) {
  for (uint8_t i = 0; i < r.len; i++) leds[r.start + i] = c;
}

// Standard unär: füllt von start nach oben
static inline void setUnary(const Range& r, uint8_t value, const CRGB& onColor) {
  if (value > r.len) value = r.len;
  for (uint8_t i = 0; i < r.len; i++) {
    leds[r.start + i] = (i < value) ? onColor : CRGB::Black;
  }
}

// ============================ BUTTON (debounced) =============================

struct ButtonState {
  bool stableLevel = HIGH;          // INPUT_PULLUP: HIGH = released, LOW = pressed
  bool lastReading = HIGH;
  uint32_t lastChangeMs = 0;

  bool pressed = false;
  uint32_t pressStartMs = 0;
};

ButtonState btn;

void updateButton() {
  bool reading = digitalRead(BTN_PIN);

  if (reading != btn.lastReading) {
    btn.lastChangeMs = millis();
    btn.lastReading = reading;
  }

  if ((millis() - btn.lastChangeMs) > DEBOUNCE_MS && reading != btn.stableLevel) {
    // stable edge detected
    btn.stableLevel = reading;

    if (btn.stableLevel == LOW) {
      // pressed
      btn.pressed = true;
      btn.pressStartMs = millis();
    } else {
      // released
      if (btn.pressed) {
        uint32_t dur = millis() - btn.pressStartMs;
        btn.pressed = false;

        if (dur >= LONGPRESS_MS) {
          mode = 0; // long-press => back to original
        } else {
          mode = (mode + 1) % MODE_COUNT; // short-press => next mode
        }
      }
    }
  }
}

// ============================ THEMES / MODES =================================

struct Theme {
  CRGB timeColor;     // used in modes 0..3 (static time)
  CRGB dummyColor;
  CRGB sepDim;
  CRGB sepHi;
  CRGB beaconColor;   // base beacon color (blinking)
  CRGB sepAlt;        // optional alternate (used for 2-tone styles)
  bool twoTone = false;
};

// Base themes for modes 0..3
Theme themeForMode(uint8_t m) {
  Theme t;

  switch (m) {
    case 0: // Original (warmweiß + orange + rot)
      t.timeColor  = CRGB(255, 150, 70);
      t.dummyColor = CRGB(40, 18, 0);
      t.sepDim     = CRGB(18, 6, 0);
      t.sepHi      = CRGB(255, 90, 0);
      t.beaconColor= CRGB::Red;
      t.sepAlt     = CRGB::White;
      t.twoTone    = false;
      break;

    case 1: // F95 rot/weiß
      t.timeColor  = CRGB::Red;              // Zeit rot
      t.dummyColor = CRGB(18, 0, 0);         // Dummies sehr dunkel rot (optional)
      t.sepDim     = CRGB(10, 10, 10);       // Trenner schwach weiß/grau
      t.sepHi      = CRGB::White;            // Trenner hell weiß
      t.beaconColor= CRGB::White;            // Beacons weiß blinkend
      t.sepAlt     = CRGB::White;
      t.twoTone    = true;
      break;

    case 2: // grün/weiß
      t.timeColor  = CRGB::Green;            // Zeit grün
      t.dummyColor = CRGB(0, 12, 0);         // Dummies dunkelgrün
      t.sepDim     = CRGB(10, 10, 10);       // Trenner schwach weiß
      t.sepHi      = CRGB::White;            // Trenner hell weiß
      t.beaconColor= CRGB::White;            // Beacons weiß blinkend
      t.sepAlt     = CRGB::White;
      t.twoTone    = true;
      break;

    case 3: // blau/weiß
      t.timeColor  = CRGB(0, 120, 255);      // Zeit blau
      t.dummyColor = CRGB(0, 0, 12);         // Dummies dunkelblau
      t.sepDim     = CRGB(10, 10, 10);       // Trenner schwach weiß
      t.sepHi      = CRGB::White;            // Trenner hell weiß
      t.beaconColor= CRGB::White;            // Beacons weiß blinkend
      t.sepAlt     = CRGB::White;
      t.twoTone    = true;
      break;

    default:
      // placeholder for modes 4/5 (handled separately)
      t.timeColor  = CRGB::White;
      t.dummyColor = CRGB::Black;
      t.sepDim     = CRGB::Black;
      t.sepHi      = CRGB::White;
      t.beaconColor= CRGB::Red;
      t.sepAlt     = CRGB::White;
      t.twoTone    = false;
      break;
  }
  return t;
}

// Regenbogen: erzeugt eine Farbe abhängig vom LED-Index + Basis-Hue
static inline CRGB rainbowColor(uint8_t ledIndex, uint8_t baseHue) {
  // leichte Streckung über 60 LEDs
  uint8_t hue = baseHue + (uint8_t)(ledIndex * 4);
  return CHSV(hue, 255, 255);
}

// Disco: Sparkle/Flash Overlay, ohne die Zeitlesbarkeit komplett zu zerstören
void applyDiscoOverlay(uint8_t intensity /*0..255*/, uint8_t sparkleCount) {
  // globaler „Beat“ über millis
  uint8_t baseHue = (uint8_t)(millis() / 6);

  // 1) leichte Farbverschiebung über alles (sehr subtil, sonst unlesbar)
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // nur dunkle LEDs stärker färben, helle eher erhalten
    if (leds[i].r + leds[i].g + leds[i].b < 60) {
      CRGB c = rainbowColor(i, baseHue);
      leds[i] = blend(leds[i], c, intensity);
    }
  }

  // 2) Sparkles
  for (uint8_t k = 0; k < sparkleCount; k++) {
    uint8_t idx = random8(NUM_LEDS);
    CRGB s = CHSV(baseHue + random8(), 255, 255);
    leds[idx] = s;
  }
}

// ============================ SETUP ==========================================

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  random16_set_seed(analogRead(A0) + micros()); // für Disco-Sparkles

  Wire.begin();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MA);
  FastLED.clear(true);

  if (!rtc.begin()) {
    // RTC fehlt -> hard stop (bewusst, damit Fehler eindeutig ist)
    while (true) { delay(1000); }
  }

  // Wenn RTC Strom verloren hat: auf Compile-Zeit setzen
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// ============================ LOOP ===========================================

void loop() {
  updateButton();

  DateTime now = rtc.now();
  const uint8_t h = now.hour();
  const uint8_t m = now.minute();
  const uint8_t s = now.second();

  // Ereignisse
  const bool beaconOn   = (s % 2 == 0);  // 1 Hz (sekundensynchron)
  const bool fullMinute = (s == 0);
  const bool fullHour   = (m == 0);

  // Zeitwerte
  const uint8_t sU = s % 10;
  const uint8_t sT = s / 10; // 0..5
  const uint8_t mU = m % 10;
  const uint8_t mT = m / 10; // 0..5
  const uint8_t hU = h % 10;
  const uint8_t hT = h / 10; // 0..2

  // Frame löschen
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // -------------------- MODE 0..3: statische Themes -------------------------
  if (mode <= 3) {
    Theme t = themeForMode(mode);

    // Dummies
    fillRange(R_DUMMY_BOTTOM, t.dummyColor);
    fillRange(R_DUMMY_TOP,    t.dummyColor);

    // Zeit (unär)
    setUnary(R_S_U, sU, t.timeColor);
    setUnary(R_S_T, sT, t.timeColor);
    setUnary(R_M_U, mU, t.timeColor);
    setUnary(R_M_T, mT, t.timeColor);
    setUnary(R_H_U, hU, t.timeColor);
    setUnary(R_H_T, hT, t.timeColor);

    // Trenner: dim + Akzent
    leds[I_S_SEP] = t.sepDim;
    leds[I_M_SEP] = t.sepDim;
    leds[I_H_SEP] = t.sepDim;

    if (fullMinute) {
      leds[I_S_SEP] = t.sepHi;
      leds[I_M_SEP] = t.sepHi;
      leds[I_H_SEP] = t.sepHi;
    }
    if (fullHour) {
      leds[I_H_SEP] = t.sepHi;
    }

    // Funkfeuer: blink
    CRGB b = beaconOn ? t.beaconColor : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;
  }

  // -------------------- MODE 4: Regenbogen ----------------------------------
  else if (mode == 4) {
    // Basishue driftet langsam
    uint8_t baseHue = (uint8_t)(millis() / 20);

    // Dummies: sehr dunkel, leicht eingefärbt (optional)
    fillRange(R_DUMMY_BOTTOM, CRGB(0, 0, 0));
    fillRange(R_DUMMY_TOP,    CRGB(0, 0, 0));

    // Zeitsegmente: Regenbogen abhängig von Index
    // (turm-treu: Segmente bleiben gleich, nur Farbe variiert)
    for (uint8_t i = 0; i < R_S_U.len; i++) leds[R_S_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_S_T.len; i++) leds[R_S_T.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_M_U.len; i++) leds[R_M_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_M_T.len; i++) leds[R_M_T.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_H_U.len; i++) leds[R_H_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_H_T.len; i++) leds[R_H_T.start + i] = CRGB::Black;

    // unär setzen, aber Farbe pro LED
    for (uint8_t i = 0; i < sU && i < R_S_U.len; i++) leds[R_S_U.start + i] = rainbowColor(R_S_U.start + i, baseHue);
    for (uint8_t i = 0; i < sT && i < R_S_T.len; i++) leds[R_S_T.start + i] = rainbowColor(R_S_T.start + i, baseHue);

    for (uint8_t i = 0; i < mU && i < R_M_U.len; i++) leds[R_M_U.start + i] = rainbowColor(R_M_U.start + i, baseHue);
    for (uint8_t i = 0; i < mT && i < R_M_T.len; i++) leds[R_M_T.start + i] = rainbowColor(R_M_T.start + i, baseHue);

    for (uint8_t i = 0; i < hU && i < R_H_U.len; i++) leds[R_H_U.start + i] = rainbowColor(R_H_U.start + i, baseHue);
    for (uint8_t i = 0; i < hT && i < R_H_T.len; i++) leds[R_H_T.start + i] = rainbowColor(R_H_T.start + i, baseHue);

    // Trenner: weiß (dim) + Akzent
    CRGB sepDim = CRGB(10, 10, 10);
    CRGB sepHi  = CRGB::White;
    leds[I_S_SEP] = sepDim;
    leds[I_M_SEP] = sepDim;
    leds[I_H_SEP] = sepDim;

    if (fullMinute) {
      leds[I_S_SEP] = sepHi;
      leds[I_M_SEP] = sepHi;
      leds[I_H_SEP] = sepHi;
    }
    if (fullHour) {
      leds[I_H_SEP] = sepHi;
    }

    // Funkfeuer: rot blinkend (klassisch)
    CRGB b = beaconOn ? CRGB::Red : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;
  }

  // -------------------- MODE 5: Disco Light ---------------------------------
  else { // mode == 5
    // Grundidee:
    // - Uhr bleibt sichtbar, aber Farben und Sparkles bewegen sich deutlich.
    // - Trenner/Beacons dürfen „party“ sein, aber die Zeitsegmente bleiben strukturiert.

    uint8_t baseHue = (uint8_t)(millis() / 6);

    // Dummies: sehr dunkel, aber lebendig
    fillRange(R_DUMMY_BOTTOM, CHSV(baseHue, 255, 20));
    fillRange(R_DUMMY_TOP,    CHSV(baseHue + 80, 255, 20));

    // Zeitsegmente: wechselnde Farben, aber stabil pro Sekunde lesbar
    // (Hue springt pro Sekunde, nicht pro Frame)
    uint8_t secHue = (uint8_t)(s * 17);

    // unär, aber farbig
    for (uint8_t i = 0; i < R_S_U.len; i++) leds[R_S_U.start + i] = (i < sU) ? CHSV(secHue + 0, 255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_S_T.len; i++) leds[R_S_T.start + i] = (i < sT) ? CHSV(secHue + 20, 255, 255) : CRGB::Black;

    for (uint8_t i = 0; i < R_M_U.len; i++) leds[R_M_U.start + i] = (i < mU) ? CHSV(secHue + 60, 255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_M_T.len; i++) leds[R_M_T.start + i] = (i < mT) ? CHSV(secHue + 80, 255, 255) : CRGB::Black;

    for (uint8_t i = 0; i < R_H_U.len; i++) leds[R_H_U.start + i] = (i < hU) ? CHSV(secHue + 120, 255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_H_T.len; i++) leds[R_H_T.start + i] = (i < hT) ? CHSV(secHue + 140, 255, 255) : CRGB::Black;

    // Trenner: strobiger, aber mit Minute/Stunde-„Peak“
    bool fastFlash = ((millis() / 120) % 2) == 0; // ca. 4 Hz
    CRGB sep = fastFlash ? CHSV(baseHue, 255, 255) : CRGB::Black;

    // Basis: leicht sichtbar statt komplett aus
    CRGB sepBase = CHSV(baseHue + 40, 255, 60);

    leds[I_S_SEP] = sepBase;
    leds[I_M_SEP] = sepBase;
    leds[I_H_SEP] = sepBase;

    // Minute/Hour Peaks
    if (fullMinute) {
      leds[I_S_SEP] = CHSV(baseHue, 255, 255);
      leds[I_M_SEP] = CHSV(baseHue + 85, 255, 255);
      leds[I_H_SEP] = CHSV(baseHue + 170, 255, 255);
    } else {
      // disco flash overlay
      leds[I_S_SEP] = blend(leds[I_S_SEP], sep, 140);
      leds[I_M_SEP] = blend(leds[I_M_SEP], sep, 140);
      leds[I_H_SEP] = blend(leds[I_H_SEP], sep, 140);
    }
    if (fullHour) {
      leds[I_H_SEP] = CHSV(baseHue + 170, 255, 255);
    }

    // Beacons: nicht nur rot, sondern „party-blink“
    CRGB b = beaconOn ? CHSV(baseHue + 200, 255, 255) : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;

    // Sparkle overlay (Parameter feinjustierbar)
    applyDiscoOverlay(/*intensity*/120, /*sparkleCount*/3);
  }

  FastLED.show();
  delay(20);
}
