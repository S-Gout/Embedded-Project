#include "wifi_config.h"
#include <WiFi.h>

const char* WIFI_SSID = "AingAing"; // Edit
const char* WIFI_PASSWORD = "AingAing"; // Edit

void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("\nWiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}