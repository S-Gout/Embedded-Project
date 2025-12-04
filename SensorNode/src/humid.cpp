#include "humid.h"
#include <DHT.h>

const int kyPin = 26;
DHT dht(kyPin, DHT11);

float lastT = 0;
float lastH = 0;

void humidInit() {
    dht.begin();
}

float humidRead() {
    float humidity = dht.readHumidity();

    if (isnan(humidity)) {
        Serial.println("Failed to read from DHT — using last known values.");
        humidity = lastH;
    } else {
        lastH = humidity;
    }

    // Serial.print("Humidity: ");
    // Serial.println(humidity);

    // delay(2000); // KY-015 refresh rate 
    return humidity;
}
