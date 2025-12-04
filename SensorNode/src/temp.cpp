#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "temp.h"

// --- CONFIG ---
#define TEMP_PIN  5   // <-- change to the GPIO you actually use!

// --- DRIVERS ---
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// Initialize the sensor
void tempInit() {
    sensors.begin();
}

// Read temperature in Celsius
float tempRead() {
    sensors.requestTemperatures();           // Ask sensor to measure
    float celsius = sensors.getTempCByIndex(0);

    // Handle error case
    if (celsius == DEVICE_DISCONNECTED_C) {
        return NAN;  // return "Not A Number" if sensor not found
    }

    return celsius;
}
