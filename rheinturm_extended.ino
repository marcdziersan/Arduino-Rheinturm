/*
  ==============================================================================
  Rheinturm-Uhr (turm-treu) – 60 LEDs + Arduino UNO + RTC + IR-Umschaltung
  Version: 1.2.0 (Mode-Switch über IR-Empfänger an D3, "egal welches Signal")
  ==============================================================================
  Ziel:
    - Anzeige wie zuvor (RTC + FastLED)
    - Mode-Umschaltung über IR-Empfänger (OUT an D3)
      -> sobald IR-Aktivität erkannt wird, Mode = (Mode+1) % 6
      -> kein Decoding, keine Codes, "egal welches Signal"
      -> Cooldown, damit ein Tastendruck nicht 1000x weiterzählt

  Anschlüsse:
    Strip (WS2812B):
      - Netzteil +5V  -> Strip 5V
      - Netzteil GND  -> Strip GND
      - UNO GND       -> Strip GND (gemeinsame Masse!)
      - UNO D6        -> 330–470 Ohm -> Strip DIN
      - (empfohlen) 1000µF Elko zwischen +5V und GND am Strip-Eingang

    RTC (I²C):
      - SDA -> A4
      - SCL -> A5
      - VCC -> 5V
      - GND -> GND

    IR-Empfänger (3-adrig GRY):
      - G (Green)  -> GND
      - R (Red)    -> 5V
      - Y (Yellow) -> D3  (IR_OUT)
      - (empfohlen) 100nF direkt am IR-Empfänger zwischen 5V und GND

  Bibliotheken:
    - FastLED
    - RTClib (Adafruit)
    - Wire (Standard)
*/

struct Theme; // wichtig gegen Arduino-Auto-Prototypen

#include <Wire.h>
#include <RTClib.h>
#include <FastLED.h>

// ============================ LED / HW CONFIG ===============================

#define LED_PIN     6
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define IR_PIN      3   // IR OUT an D3 (INT1)

CRGB leds[NUM_LEDS];

// RTC-Typ umschalten, falls nötig:
// RTC_DS1307 rtc;
RTC_DS3231 rtc;

// ~30% Helligkeit
const uint8_t BRIGHTNESS = 77;

// Optional: konservatives Stromlimit (an Netzteil anpassen)
const uint16_t MAX_MA = 2500;

// Modes
const uint8_t MODE_COUNT = 6;
uint8_t mode = 0;

// IR-Cooldown: verhindert Mehrfach-Umschalten pro IR-Tastendruck
const uint32_t IR_COOLDOWN_US = 300000; // 300ms

volatile bool     irEvent = false;
volatile uint32_t irLastUs = 0;

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

// ============================ COLOR HELPERS =================================

// Liefert garantiert CRGB (keine CHSV/namespace-Probleme im ?:)
static inline CRGB hsv(uint8_t h, uint8_t s, uint8_t v) {
  CRGB out;
  hsv2rgb_rainbow(CHSV(h, s, v), out);
  return out;
}

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

// ============================ IR "ANY SIGNAL" SWITCH =========================
//
// IR-Receiver liefern bei Aktivität Burst-Pakete (viele Flanken).
// Wir werten "erste Flanke nach Cooldown" als 1 Umschalt-Event.

void onIrEdge() {
  uint32_t nowUs = micros();

  // Cooldown-Gate: nur alle IR_COOLDOWN_US ein Event zulassen
  if ((uint32_t)(nowUs - irLastUs) >= IR_COOLDOWN_US) {
    irLastUs = nowUs;
    irEvent = true;
  }
}

static inline void processIrEvent() {
  if (!irEvent) return;

  noInterrupts();
  irEvent = false;
  interrupts();

  mode = (uint8_t)((mode + 1) % MODE_COUNT);
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

Theme themeForMode(uint8_t m) {
  Theme t;

  switch (m) {
    case 0: // Original (warmweiß + orange + rot)
      t.timeColor   = CRGB(255, 150, 70);
      t.dummyColor  = CRGB(40, 18, 0);
      t.sepDim      = CRGB(18, 6, 0);
      t.sepHi       = CRGB(255, 90, 0);
      t.beaconColor = CRGB::Red;
      t.sepAlt      = CRGB::White;
      t.twoTone     = false;
      break;

    case 1: // F95 rot/weiß
      t.timeColor   = CRGB::Red;
      t.dummyColor  = CRGB(18, 0, 0);
      t.sepDim      = CRGB(10, 10, 10);
      t.sepHi       = CRGB::White;
      t.beaconColor = CRGB::White;
      t.sepAlt      = CRGB::White;
      t.twoTone     = true;
      break;

    case 2: // grün/weiß
      t.timeColor   = CRGB::Green;
      t.dummyColor  = CRGB(0, 12, 0);
      t.sepDim      = CRGB(10, 10, 10);
      t.sepHi       = CRGB::White;
      t.beaconColor = CRGB::White;
      t.sepAlt      = CRGB::White;
      t.twoTone     = true;
      break;

    case 3: // blau/weiß
      t.timeColor   = CRGB(0, 120, 255);
      t.dummyColor  = CRGB(0, 0, 12);
      t.sepDim      = CRGB(10, 10, 10);
      t.sepHi       = CRGB::White;
      t.beaconColor = CRGB::White;
      t.sepAlt      = CRGB::White;
      t.twoTone     = true;
      break;

    default:
      // placeholder for modes 4/5 (handled separately)
      t.timeColor   = CRGB::White;
      t.dummyColor  = CRGB::Black;
      t.sepDim      = CRGB::Black;
      t.sepHi       = CRGB::White;
      t.beaconColor = CRGB::Red;
      t.sepAlt      = CRGB::White;
      t.twoTone     = false;
      break;
  }
  return t;
}

static inline CRGB rainbowColor(uint8_t ledIndex, uint8_t baseHue) {
  uint8_t hue = baseHue + (uint8_t)(ledIndex * 4);
  return hsv(hue, 255, 255);
}

void applyDiscoOverlay(uint8_t intensity /*0..255*/, uint8_t sparkleCount) {
  uint8_t baseHue = (uint8_t)(millis() / 6);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (leds[i].r + leds[i].g + leds[i].b < 60) {
      CRGB c = rainbowColor(i, baseHue);
      leds[i] = blend(leds[i], c, intensity);
    }
  }

  for (uint8_t k = 0; k < sparkleCount; k++) {
    uint8_t idx = random8(NUM_LEDS);
    CRGB s = hsv((uint8_t)(baseHue + random8()), 255, 255);
    leds[idx] = s;
  }
}

// ============================ SETUP ==========================================

void setup() {
  pinMode(IR_PIN, INPUT_PULLUP); // viele IR-Module haben Pullup, schadet nicht
  attachInterrupt(digitalPinToInterrupt(IR_PIN), onIrEdge, FALLING);

  random16_set_seed(analogRead(A0) + micros());

  Wire.begin();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MA);
  FastLED.clear(true);

  if (!rtc.begin()) {
    while (true) { delay(1000); }
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

// ============================ LOOP ===========================================

void loop() {
  // IR event => Mode umschalten (egal welches Signal)
  processIrEvent();

  DateTime now = rtc.now();
  const uint8_t h = now.hour();
  const uint8_t m = now.minute();
  const uint8_t s = now.second();

  const bool beaconOn   = (s % 2 == 0);  // 1 Hz (sekundensynchron)
  const bool fullMinute = (s == 0);
  const bool fullHour   = (m == 0);

  const uint8_t sU = s % 10;
  const uint8_t sT = s / 10; // 0..5
  const uint8_t mU = m % 10;
  const uint8_t mT = m / 10; // 0..5
  const uint8_t hU = h % 10;
  const uint8_t hT = h / 10; // 0..2

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // -------------------- MODE 0..3: statische Themes -------------------------
  if (mode <= 3) {
    Theme t = themeForMode(mode);

    fillRange(R_DUMMY_BOTTOM, t.dummyColor);
    fillRange(R_DUMMY_TOP,    t.dummyColor);

    setUnary(R_S_U, sU, t.timeColor);
    setUnary(R_S_T, sT, t.timeColor);
    setUnary(R_M_U, mU, t.timeColor);
    setUnary(R_M_T, mT, t.timeColor);
    setUnary(R_H_U, hU, t.timeColor);
    setUnary(R_H_T, hT, t.timeColor);

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

    CRGB b = beaconOn ? t.beaconColor : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;
  }

  // -------------------- MODE 4: Regenbogen ----------------------------------
  else if (mode == 4) {
    uint8_t baseHue = (uint8_t)(millis() / 20);

    fillRange(R_DUMMY_BOTTOM, CRGB::Black);
    fillRange(R_DUMMY_TOP,    CRGB::Black);

    // alles aus (Basis)
    for (uint8_t i = 0; i < R_S_U.len; i++) leds[R_S_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_S_T.len; i++) leds[R_S_T.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_M_U.len; i++) leds[R_M_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_M_T.len; i++) leds[R_M_T.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_H_U.len; i++) leds[R_H_U.start + i] = CRGB::Black;
    for (uint8_t i = 0; i < R_H_T.len; i++) leds[R_H_T.start + i] = CRGB::Black;

    // unär mit Regenbogenfarbe pro LED
    for (uint8_t i = 0; i < sU && i < R_S_U.len; i++) leds[R_S_U.start + i] = rainbowColor(R_S_U.start + i, baseHue);
    for (uint8_t i = 0; i < sT && i < R_S_T.len; i++) leds[R_S_T.start + i] = rainbowColor(R_S_T.start + i, baseHue);

    for (uint8_t i = 0; i < mU && i < R_M_U.len; i++) leds[R_M_U.start + i] = rainbowColor(R_M_U.start + i, baseHue);
    for (uint8_t i = 0; i < mT && i < R_M_T.len; i++) leds[R_M_T.start + i] = rainbowColor(R_M_T.start + i, baseHue);

    for (uint8_t i = 0; i < hU && i < R_H_U.len; i++) leds[R_H_U.start + i] = rainbowColor(R_H_U.start + i, baseHue);
    for (uint8_t i = 0; i < hT && i < R_H_T.len; i++) leds[R_H_T.start + i] = rainbowColor(R_H_T.start + i, baseHue);

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

    CRGB b = beaconOn ? CRGB::Red : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;
  }

  // -------------------- MODE 5: Disco Light ---------------------------------
  else { // mode == 5
    uint8_t baseHue = (uint8_t)(millis() / 6);

    fillRange(R_DUMMY_BOTTOM, hsv(baseHue, 255, 20));
    fillRange(R_DUMMY_TOP,    hsv((uint8_t)(baseHue + 80), 255, 20));

    uint8_t secHue = (uint8_t)(s * 17);

    for (uint8_t i = 0; i < R_S_U.len; i++) leds[R_S_U.start + i] = (i < sU) ? hsv((uint8_t)(secHue + 0),   255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_S_T.len; i++) leds[R_S_T.start + i] = (i < sT) ? hsv((uint8_t)(secHue + 20),  255, 255) : CRGB::Black;

    for (uint8_t i = 0; i < R_M_U.len; i++) leds[R_M_U.start + i] = (i < mU) ? hsv((uint8_t)(secHue + 60),  255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_M_T.len; i++) leds[R_M_T.start + i] = (i < mT) ? hsv((uint8_t)(secHue + 80),  255, 255) : CRGB::Black;

    for (uint8_t i = 0; i < R_H_U.len; i++) leds[R_H_U.start + i] = (i < hU) ? hsv((uint8_t)(secHue + 120), 255, 255) : CRGB::Black;
    for (uint8_t i = 0; i < R_H_T.len; i++) leds[R_H_T.start + i] = (i < hT) ? hsv((uint8_t)(secHue + 140), 255, 255) : CRGB::Black;

    bool fastFlash = ((millis() / 120) % 2) == 0; // ca. 4 Hz
    CRGB sep = fastFlash ? hsv(baseHue, 255, 255) : CRGB::Black;

    CRGB sepBase = hsv((uint8_t)(baseHue + 40), 255, 60);

    leds[I_S_SEP] = sepBase;
    leds[I_M_SEP] = sepBase;
    leds[I_H_SEP] = sepBase;

    if (fullMinute) {
      leds[I_S_SEP] = hsv(baseHue, 255, 255);
      leds[I_M_SEP] = hsv((uint8_t)(baseHue + 85), 255, 255);
      leds[I_H_SEP] = hsv((uint8_t)(baseHue + 170), 255, 255);
    } else {
      leds[I_S_SEP] = blend(leds[I_S_SEP], sep, 140);
      leds[I_M_SEP] = blend(leds[I_M_SEP], sep, 140);
      leds[I_H_SEP] = blend(leds[I_H_SEP], sep, 140);
    }

    if (fullHour) {
      leds[I_H_SEP] = hsv((uint8_t)(baseHue + 170), 255, 255);
    }

    CRGB b = beaconOn ? hsv((uint8_t)(baseHue + 200), 255, 255) : CRGB::Black;
    leds[I_BEACON1] = b;
    leds[I_BEACON2] = b;

    applyDiscoOverlay(/*intensity*/120, /*sparkleCount*/3);
  }

  FastLED.show();
  delay(20);
}
