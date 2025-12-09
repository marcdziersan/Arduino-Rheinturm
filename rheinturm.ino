/* Rheinturmuhr mit NeoPixel WS2812 LED-Streifen */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);

#define PIN        6            // Pin, an dem der Datenpin der Neopixel angeschlossen ist
#define NUMPIXELS 51            // Anzahl der LEDs 

int blinkgeschwindigkeit = 1000;    // hier kann man die roten Trenn-LEDs blinken lassen – wenn man das nicht will, kann man die Variable auf 1 setzen 

int theLEDsAus[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0};
int theLEDs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0};

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);   // Anlegen des Neopixel-Objektes

/* ***** ***** ***** ***** SETUP ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** */
void setup() {
  pixels.begin();               // Initialisierung des Neopixel Objektes
  pixels.setBrightness(50);     // Set BRIGHTNESS to about 1/5 (max = 255)
  rtcInit();                    // Real Time Clock Initialisierung
}

/* ***** ***** ***** ***** LOOP ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** ***** */
void loop() {
  if (!Rtc.IsDateTimeValid())  {
    if (Rtc.LastError() != 0)    {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    } else {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  RtcDateTime now = Rtc.GetDateTime();

  printDateTime(now); Serial.print(" ");
  updateTime(now);
  showTime();
  Serial.println();

  delay(10); 
}

void updateTime(const RtcDateTime& dt)
{
  /* Reset aller LED Einträge */
  for (int i = 0; i < NUMPIXELS; i++) {
    theLEDs[i] = theLEDsAus[i];
  }

  /* Sekunden Einer */
  for (int i = 0; i < dt.Second() % 10; i++) {
    theLEDs[i] = 1;
  }
  /* Sekunden Zehner */
  for (int i = 0; i < dt.Second() / 10; i++) {
    theLEDs[15 - i] = 1;
  }

  /* Minuten Einer */
  for (int i = 0; i < dt.Minute() % 10; i++) {
    theLEDs[19 + i] = 1;
  }
  /* Minuten Zehner */
  for (int i = 0; i < dt.Minute() / 10; i++) {
    theLEDs[34 - i] = 1;
  }

  /* Stunden Einer */
  for (int i = 0; i < dt.Hour() % 10; i++) {
    theLEDs[38 + i] = 1;
  }

  /* Stunden Zehner */
  for (int i = 0; i < dt.Hour() / 10; i++) {
    theLEDs[50 - i] = 1;
  }
}

void showTime() {
  
  pixels.clear();               // Schalte alle LEDs aus
  
  for (int i = 0; i < NUMPIXELS; i++) {
    switch (theLEDs[i]) {
      case 0:
        Serial.print("-");
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        break;
      case 1:
        Serial.print("*");
        pixels.setPixelColor(i, pixels.Color(100, 100, 100));
        break;
      case 2:
        Serial.print(" ");
        break;
      case 3:
        if (millis() % blinkgeschwindigkeit < blinkgeschwindigkeit/2) {
          Serial.print("|");
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        } else {
          Serial.print("–");
          pixels.setPixelColor(i, pixels.Color(100, 0, 0));
        }
        break;
    }
  }
  pixels.show();   // Sende die Farbinformationen an den LED-Streifen
}

/* ***** ***** ***** ***** Methoden für die Real Time Clock ***** ***** ***** ***** ***** ***** */

void rtcInit() {
  Serial.begin(115200);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  //--------RTC SETUP ------------
  // if you are using ESP-01 then uncomment the line below to reset the pins to
  // the available pins for SDA, SCL
  // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");
      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");

  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
    countof(datestring),
    PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
    dt.Month(),
    dt.Day(),
    dt.Year(),
    dt.Hour(),
    dt.Minute(),
    dt.Second() );
  Serial.print(datestring);
}
