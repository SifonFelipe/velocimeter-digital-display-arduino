// I'm using a HSN-5643AS-H as a display

#include <SevSeg.h>  // lib to control 7 segments displays
SevSeg sevseg;       // SevSeg obj to control display

// Data from SimHub
int speed = 0;
int rpm = 0;
int shiftLight1 = 0;
int shiftLight2 = 0;

// Control the connection with SimHub
unsigned long lastValidUpdate = 0;
const unsigned long TIMEOUT = 3000; // 3s without data = blink

// Shift Lights
const int ledPins[] = {A5, A4, A3};
const int NUM_LEDS = 3;

void setup() {
  Serial.begin(115200);
  while (!Serial) {;} // waits Serial Port to be ready

  // config of 7 segments display HSN-5643AS-H
  byte numDigits = 4;
  byte digitPins[] = {12, 9, 8, 6}; // Controlling digits pins (with resistors)
  byte segmentPins[] = {11, 7, 4, 2, 13, 10, 5, 3}; // Controlling segments

  // COMMON_CATHODE = GND in common
  // false = no decimal places
  sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins, false);
  sevseg.setBrightness(90);
  sevseg.blank();

  // RPM leds config
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
}

void loop() {
  static String buf = ""; // buffer to build message received
  unsigned long now = millis(); // actual time in milliseconds

  while (Serial.available()) {
    char c = Serial.read();  // reads 1 character from the serial port

    // If line break, the message ends
    if (c == '\n' || c == '\r') {
      buf.trim(); // deletes spaces or `\n`

      // Only processes message if at least contains 6 characters (ex.: "0|0|0|0")
      if (buf.length() > 5) {
        int p1 = buf.indexOf('|');
        int p2 = buf.indexOf('|', p1 + 1);
        int p3 = buf.indexOf('|', p2 + 1);

        // Ensures that all the data is available
        if (p1 > 0 && p2 > p1 && p3 > p2) {
          int newSpeed = buf.substring(0, p1).toInt();
          int newL1    = buf.substring(p1 + 1, p2).toInt();
          int newL2    = buf.substring(p2 + 1, p3).toInt();
          int newRpm   = buf.substring(p3 + 1).toInt();

          // Data validation
          if (newSpeed >= 0 && newSpeed <= 9999 &&  // valid speed
              newRpm >= 0 && newRpm <= 20000 &&     // realistic rpm
              newL1 > 1000 && newL2 > newL1) {      // logical limits
            speed = newSpeed;
            rpm = newRpm;
            shiftLight1 = newL1;
            shiftLight2 = newL2;
            lastValidUpdate = now; // resets timeout
          }
        }
      }
      buf = "";  // cleans buffer
    }
    // accumulates characters (if not line break)
    else if (buf.length() < 50) {
      buf += c;
    }
  }

  // show display and leds
  if (now - lastValidUpdate < TIMEOUT) {
    if (speed > 0) {
      sevseg.setNumber(speed, 0);
    } else {
      sevseg.setNumber(0, 0);
    }
    updateRPMLEDs(rpm, shiftLight1, shiftLight2);
  } 
  else {
    // If not data for 3 seconds, blinking display
    if ((now / 500) % 2 == 0) {
      sevseg.setNumber(speed, 0);
      rpm = 1;  // forces a led to indicate that it is alive
      updateRPMLEDs(rpm, shiftLight1, shiftLight2);

    } else {
      sevseg.blank();
      clearLEDs();
    }
  }
  
  // refresh display
  sevseg.refreshDisplay();
}

// Function to control RPM leds
void updateRPMLEDs(int currentRPM, int gearRedline, int maxRPM) {
  clearLEDs(); // shuts down leds first

  // if rpm is less than 80%, nothing shows
  if (currentRPM < gearRedline * 0.8) return;

  float start = gearRedline * 0.8;
  float range = maxRPM - start;

  // progress normalized, 0.0 to 1
  float progress = (currentRPM - start) / range;
  progress = constrain(progress, 0, 1);

  // how many leds turn on
  int ledsOn = (int)(progress * NUM_LEDS + 0.5);

  // turns on leds progressively
  for (int i = 0; i < ledsOn && i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], HIGH);
  }

  // red line blinking
  if (currentRPM >= maxRPM && NUM_LEDS >= 2) {
    digitalWrite(ledPins[NUM_LEDS - 1], (millis() / 100) % 2);
  }
}

// Function to turn off leds
void clearLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}
