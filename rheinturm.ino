#include <Adafruit_NeoPixel.h>   // Bibliothek für NeoPixel-LEDs
#include <Wire.h>                // I2C-Bibliothek für die RTC
#include <RtcDS1307.h>           // RTC-Bibliothek (Makuna)

RtcDS1307<TwoWire> Rtc(Wire);    // RTC-Objekt, das den I2C-Bus verwendet

// -------------------------
// LED-Konfiguration
// -------------------------
#define LED_PIN    6             // Datenpin, an den der NeoPixel-Streifen angeschlossen ist
#define LED_COUNT  51            // Anzahl der LEDs im Streifen

// NeoPixel-Objekt: Anzahl LEDs, Pin, Farbordnung + Protokollfrequenz
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Helligkeit der LEDs (0–255)
const uint8_t BRIGHTNESS = 50;   // 50 ist etwa 1/5 der maximalen Helligkeit

// Blinkgeschwindigkeit für die roten Trenner-LEDs in Millisekunden
// Kleinere Werte → schnelleres Blinken, 0 oder 1 → faktisch immer an
int blinkSpeedMs = 1000;

// -------------------------
// Layout des LED-Streifens
// -------------------------
//
// Wir teilen die 51 LEDs in Segmente ein:
//
// -  0 bis  9  : Sekunden-Einer (10 LEDs)
// - 10 bis 15  : Sekunden-Zehner (6 LEDs)
// - 16         : Trenner Sekunden/Minuten (rote LED)
// - 17 bis 26  : Minuten-Einer (10 LEDs)
// - 27 bis 32  : Minuten-Zehner (6 LEDs)
// - 33         : Trenner Minuten/Stunden (rote LED)
// - 34 bis 43  : Stunden-Einer (10 LEDs)
// - 44 bis 50  : Stunden-Zehner (7 LEDs, gebraucht werden max. 3)
//
// Das ist kein 1:1-Layout des Originalprojekts, aber funktional sehr ähnlich.

const uint8_t SEC_ONES_START      = 0;
const uint8_t SEC_ONES_HEIGHT     = 10;

const uint8_t SEC_TENS_START      = 10;
const uint8_t SEC_TENS_HEIGHT     = 6;

const uint8_t SEP_SEC_MIN_IDX     = 16;   // rote Trenner-LED zwischen Sekunden und Minuten

const uint8_t MIN_ONES_START      = 17;
const uint8_t MIN_ONES_HEIGHT     = 10;

const uint8_t MIN_TENS_START      = 27;
const uint8_t MIN_TENS_HEIGHT     = 6;

const uint8_t SEP_MIN_HOUR_IDX    = 33;   // rote Trenner-LED zwischen Minuten und Stunden

const uint8_t HOUR_ONES_START     = 34;
const uint8_t HOUR_ONES_HEIGHT    = 10;

const uint8_t HOUR_TENS_START     = 44;
const uint8_t HOUR_TENS_HEIGHT    = 7;    // reicht für 0–2

// -------------------------
// Hilfsfunktionen: LEDs
// -------------------------

// Setzt alle LEDs erstmal auf "aus"
void clearStrip()
{
    // interne Puffer der NeoPixel-Bibliothek werden auf schwarz gesetzt
    pixels.clear();
}

// Zeichnet eine "Spalte" für eine Dezimalstelle (0–9)
// startIndex: unterste LED der Spalte
// maxHeight : maximale Anzahl LEDs, die diese Spalte besitzt
// value     : Wert der Ziffer (z.B. 7 → 7 LEDs leuchten)
void drawDigitColumn(uint8_t startIndex, uint8_t maxHeight, uint8_t value)
{
    // Begrenzen des Wertes auf die maximal mögliche Höhe
    if (value > maxHeight) {
        value = maxHeight;
    }

    // Alle LEDs der Spalte zunächst ausschalten
    for (uint8_t i = 0; i < maxHeight; i++) {
        uint8_t idx = startIndex + i;
        if (idx < LED_COUNT) {
            pixels.setPixelColor(idx, pixels.Color(0, 0, 0));
        }
    }

    // Von unten nach oben "value" LEDs weiß einschalten
    for (uint8_t i = 0; i < value; i++) {
        uint8_t idx = startIndex + i;
        if (idx < LED_COUNT) {
            // Weißes Licht (RGB: 100, 100, 100)
            pixels.setPixelColor(idx, pixels.Color(100, 100, 100));
        }
    }
}

// Zeichnet eine rote Trenner-LED, optional blinkend
// index     : Position der Trenner-LED
// blinkOnMs : Blinkperiode, z.B. 1000ms → 0,5s an, 0,5s aus
void drawSeparator(uint8_t index, int blinkOnMs)
{
    if (index >= LED_COUNT) return; // Sicherheitsabfrage

    if (blinkOnMs <= 1) {
        // Blinken deaktiviert: LED dauerhaft rot
        pixels.setPixelColor(index, pixels.Color(100, 0, 0));
        return;
    }

    // Aktuelle Zeit seit Start (Millisekunden)
    unsigned long now = millis();

    // Einfache Rechteck-Blinklogik:
    // In der ersten Hälfte des Intervalls → aus,
    // in der zweiten Hälfte → rot an
    if ((now % blinkOnMs) < (unsigned long)(blinkOnMs / 2)) {
        // Phase 1: LED aus
        pixels.setPixelColor(index, pixels.Color(0, 0, 0));
    } else {
        // Phase 2: LED rot
        pixels.setPixelColor(index, pixels.Color(100, 0, 0));
    }
}

// -------------------------
// Hilfsfunktionen: RTC
// -------------------------

// Gibt Datum und Uhrzeit auf der seriellen Schnittstelle aus
void printDateTime(const RtcDateTime& dt)
{
    char buf[20]; // Puffer für Datum/Zeit-String

    // Formatiert als "MM/TT/JJJJ hh:mm:ss"
    snprintf_P(
        buf,
        sizeof(buf),
        PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
        dt.Month(),
        dt.Day(),
        dt.Year(),
        dt.Hour(),
        dt.Minute(),
        dt.Second()
    );

    Serial.print(buf);
}

// Initialisiert die RTC und stellt sicher, dass sie eine gültige Zeit hat
void initRtc()
{
    Serial.begin(115200);  // Serielle Schnittstelle zum Debuggen
    Wire.begin();          // I2C starten

    Serial.print("Sketch compiled: ");
    Serial.print(__DATE__);   // Kompilierdatum (Makro von der Toolchain)
    Serial.print(" ");
    Serial.println(__TIME__); // Kompilierzeit (Makro von der Toolchain)

    Rtc.Begin();              // RTC-Bibliothek starten

    // Zeitobjekt mit Kompilierzeit erstellen
    RtcDateTime compiled(__DATE__, __TIME__);

    // Prüfen, ob die RTC eine gültige Zeit kennt
    if (!Rtc.IsDateTimeValid()) {
        if (Rtc.LastError() != 0) {
            // Wenn LastError != 0 ist, liegt ein I2C-Kommunikationsproblem vor
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
        } else {
            // Häufige Ursache: Batterie fehlt oder war leer
            Serial.println("RTC has invalid DateTime, setting to compile time.");
            Rtc.SetDateTime(compiled); // RTC auf Kompilierzeit setzen
        }
    }

    // Falls die RTC noch nicht "läuft", jetzt starten
    if (!Rtc.GetIsRunning()) {
        Serial.println("RTC was not running, starting now.");
        Rtc.SetIsRunning(true);
    }

    // Aktuelle Zeit aus der RTC lesen
    RtcDateTime now = Rtc.GetDateTime();

    // Falls die RTC-Zeit vor der Kompilierzeit liegt, zur Sicherheit aktualisieren
    if (now < compiled) {
        Serial.println("RTC is older than compile time, updating RTC.");
        Rtc.SetDateTime(compiled);
    } else if (now > compiled) {
        Serial.println("RTC is newer than compile time (this is fine).");
    } else {
        Serial.println("RTC time equals compile time (unüblich, aber OK).");
    }

    // Square-Wave-Pin der DS1307 deaktivieren (Low-Pegel)
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);
}

// -------------------------
// setup() – wird einmal beim Start ausgeführt
// -------------------------
void setup()
{
    // NeoPixel initialisieren
    pixels.begin();                  // Interne Initialisierung des LED-Streifens
    pixels.setBrightness(BRIGHTNESS); // Standardhelligkeit setzen
    pixels.show();                   // Alle LEDs ausschalten (Buffer auf die Hardware übertragen)

    // RTC initialisieren
    initRtc();
}

// -------------------------
// loop() – wird in Endlosschleife ausgeführt
// -------------------------
void loop()
{
    // Prüfen, ob die RTC-Zeit gültig ist
    if (!Rtc.IsDateTimeValid()) {
        if (Rtc.LastError() != 0) {
            // Kommunikationsproblem über I2C
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
        } else {
            // Zeit ist ungültig (z.B. Batterieproblem)
            Serial.println("RTC lost confidence in the DateTime!");
        }
    }

    // Aktuelle Zeit aus der RTC holen
    RtcDateTime now = Rtc.GetDateTime();

    // Datum/Zeit im seriellen Monitor anzeigen
    printDateTime(now);
    Serial.print("  ");

    // LED-Streifen entsprechend der aktuellen Uhrzeit aktualisieren
    clearStrip();  // alle LEDs löschen

    // Stunden, Minuten, Sekunden als Zahlen holen
    uint8_t hour   = now.Hour();   // 0–23
    uint8_t minute = now.Minute(); // 0–59
    uint8_t second = now.Second(); // 0–59

    // Ziffern in Einer- und Zehnerstellen zerlegen
    uint8_t secOnes  = second % 10;
    uint8_t secTens  = second / 10;

    uint8_t minOnes  = minute % 10;
    uint8_t minTens  = minute / 10;

    uint8_t hourOnes = hour % 10;
    uint8_t hourTens = hour / 10;

    // Debug-Ausgabe der Ziffern
    Serial.print("H=");
    Serial.print(hourTens);
    Serial.print(hourOnes);
    Serial.print(" M=");
    Serial.print(minTens);
    Serial.print(minOnes);
    Serial.print(" S=");
    Serial.print(secTens);
    Serial.print(secOnes);
    Serial.println();

    // Sekunden-Säulen zeichnen
    drawDigitColumn(SEC_ONES_START, SEC_ONES_HEIGHT, secOnes);
    drawDigitColumn(SEC_TENS_START, SEC_TENS_HEIGHT, secTens);

    // Minuten-Säulen zeichnen
    drawDigitColumn(MIN_ONES_START, MIN_ONES_HEIGHT, minOnes);
    drawDigitColumn(MIN_TENS_START, MIN_TENS_HEIGHT, minTens);

    // Stunden-Säulen zeichnen
    drawDigitColumn(HOUR_ONES_START, HOUR_ONES_HEIGHT, hourOnes);
    drawDigitColumn(HOUR_TENS_START, HOUR_TENS_HEIGHT, hourTens);

    // Rote Trenner-LEDs zeichnen (blinken)
    drawSeparator(SEP_SEC_MIN_IDX,    blinkSpeedMs);
    drawSeparator(SEP_MIN_HOUR_IDX,   blinkSpeedMs);

    // Neue LED-Daten an den Streifen senden
    pixels.show();

    // Kurze Pause, um die CPU etwas zu entlasten
    delay(10);
}
