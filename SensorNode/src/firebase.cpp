#include "firebase.h"
#include <HTTPClient.h>

extern bool purifierOn;

const String FIREBASE_URL =
  "https://embedded-system-e1d18-default-rtdb.asia-southeast1.firebasedatabase.app";

void sendToFirebase(float pm25, float humidity, float temperature) {
  HTTPClient http;

  // target path
  String url = FIREBASE_URL + "/airPurifier.json";

  String statusStr = purifierOn ? "ON" : "OFF";

  String payload = "{";
  payload += "\"pm25\":" + String(pm25, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"status\":\"" + statusStr + "\"";
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.PATCH(payload); 
  Serial.print("Firebase Response Code: ");
  Serial.println(code);
  Serial.println(http.getString());

  http.end();
}
void pushHistory(float pm25, float humidity, float temperature) {
  HTTPClient http;

  String url = FIREBASE_URL + "/airPurifier/history.json";

  String statusStr = purifierOn ? "ON" : "OFF";

  String payload = "{";
  payload += "\"pm25\":" + String(pm25, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"status\":\"" + statusStr + "\",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(payload);   
  Serial.print("History push code: ");
  Serial.println(code);

  http.end();
}
