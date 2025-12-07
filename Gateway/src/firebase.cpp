#include "firebase.h"
#include <HTTPClient.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>

extern bool purifierOn;

const String FIREBASE_URL =
  "https://embedded-system-e1d18-default-rtdb.asia-southeast1.firebasedatabase.app";

// ---- epoch ms helper (Thailand local after NTP) ----
uint64_t epochThaiMillis() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return ((uint64_t)tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL);
}

void sendToFirebase(float pm25, float humidity, float temperature, float gas) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = FIREBASE_URL + "/airPurifier.json";

  String statusStr = purifierOn ? "ON" : "OFF";

  String payload = "{";
  payload += "\"pm25\":" + String(pm25, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"gas\":" + String(gas, 2) + ",";
  payload += "\"status\":\"" + statusStr + "\"";
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PATCH(payload);
  Serial.print("Firebase live code: ");
  Serial.println(code);

  http.end();
}

void pushHistory(float pm25, float humidity, float temperature, float gas) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  uint64_t ts = epochThaiMillis();  
  String url = FIREBASE_URL + "/airPurifier/history/" + String(ts) + ".json";

  String statusStr = purifierOn ? "ON" : "OFF";

  String payload = "{";
  payload += "\"pm25\":" + String(pm25, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"gas\":" + String(gas, 2) + ",";
  payload += "\"status\":\"" + statusStr + "\",";
  payload += "\"timestamp\":" + String(ts);
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT(payload);
  Serial.print("Firebase history code: ");
  Serial.println(code);

  http.end();
}
