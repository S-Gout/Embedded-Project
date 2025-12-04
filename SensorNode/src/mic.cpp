#include "mic.h"
// Change from Led to Motor

// === Pin configuration ===
const int micAnalogPin = 36;   // VP → KY-038 A0 
const int actuatorPin  = 4;    // D4 → LED / Motor / Relay

// === Clap detection settings ===
const int THRESHOLD = 200;                 // adjust after printing raw values
const unsigned long CLAP_COOLDOWN = 300;   // ms to avoid double-trigger

static bool purifierOn = false;
static unsigned long lastClap = 0;
static bool actuatorState = false;

void micInit() {
    Serial.println("KY-038 Clap Detection Ready");

    pinMode(actuatorPin, OUTPUT);
    digitalWrite(actuatorPin, LOW);
}

void micUpdate() {
    int raw = analogRead(micAnalogPin);

    // Print raw values to tune threshold (you can comment out later)
    Serial.print("Mic raw: ");
    Serial.println(raw);

    unsigned long now = millis();

    // Detect clap: sound spike > threshold
    if (raw > THRESHOLD && (now - lastClap > CLAP_COOLDOWN)) {
        lastClap = now;

        // Toggle actuator (LED / Motor / Relay)
        actuatorState = !actuatorState;
        digitalWrite(actuatorPin, actuatorState);

        // Toggle purifier state flag
        purifierOn = !purifierOn;

        Serial.print("CLAP DETECTED → Purifier: ");
        Serial.println(purifierOn ? "ON" : "OFF");
    }
}