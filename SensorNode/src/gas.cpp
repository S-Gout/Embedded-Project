#include <Arduino.h>
#include "gas.h"

const int anInput = 34;
const int co2Zero = 55;

void gasInit() {
    pinMode(anInput, INPUT);
} // Might not need this

int gasRead() {
    float co2now[10];     // array
    int co2raw = 0;     // raw
    int co2ppm = 0;     // calculated ppm
    int zzz = 0;        // int for avg

    for(int i = 0 ; i < 10 ; i++) {
        co2now[i] = analogRead(anInput) * (1024.0 / 4096.0);
        delay(200);
    }

    for(int i = 0 ; i < 10 ; i++) {
        zzz = zzz + co2now[i];
    }

    co2raw = zzz/10;
    co2ppm = co2raw - co2Zero;

    // Serial.print("AirQuality: ");
    // Serial.print(co2ppm);
    // Serial.println(" PPM");
    return co2ppm;
}