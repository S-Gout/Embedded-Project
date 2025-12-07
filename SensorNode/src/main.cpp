#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <dust_sensor.h>
#include "humid.h"
#include "gas.h"

// ======== GATEWAY MAC (copy from your gateway serial monitor) ========
uint8_t gatewayMac[] = { 0x28, 0x56, 0x2F, 0x49, 0xCD, 0xE4 };

// ======== WiFi/ESP-NOW must be same channel as gateway ========
#define GATEWAY_CHANNEL 6   // <--- SET THIS TO YOUR GATEWAY'S WIFI CHANNEL

// ======== DATA STRUCT (MATCH Gateway) ========
typedef struct {
  char cmd[8];
  float pm25;
  float humid;
  float gas;
} EspNowPacket;

EspNowPacket pkt;

// ======== CALLBACK ========
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ======== ESP-NOW INIT ========
void setupEspNow() {
  WiFi.mode(WIFI_STA);

  // Lock sender to WiFi channel used by gateway
  esp_wifi_set_channel(GATEWAY_CHANNEL, WIFI_SECOND_CHAN_NONE);

  Serial.print("Sensor Node MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, gatewayMac, 6);
  peer.channel = GATEWAY_CHANNEL;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) == ESP_OK) {
    Serial.println("Gateway peer added!");
  } else {
    Serial.println("Failed to add peer.");
  }
}

// Mic Output
const int micOut = 18;

// Motor
bool motorState = false;
const int motorPin = 27;

void checkMotor(float pm25) {
  bool on = ( digitalRead(micOut) == HIGH );
  Serial.print(pm25 >= 50); Serial.print(" , "); Serial.print(on); Serial.print(" , "); Serial.println(motorState);
  if(pm25 >= 50 && on && !motorState) {
    digitalWrite(motorPin, HIGH);
    motorState = true;
    Serial.println("Motor On");
  } else if(pm25 <= 40 && on && motorState) {
    Serial.println(pm25);
    digitalWrite(motorPin, LOW);
    motorState = false;
    Serial.println("Motor Off");
  } else if(!on && motorState) {
    digitalWrite(motorPin, LOW);
    motorState = false;
    Serial.println("Motor Off");
  }
}

// ======== SETUP ========
void setup() {
  Serial.begin(115200);

  pinMode(micOut, INPUT);
  pinMode(motorPin, OUTPUT);
  setupEspNow();
  dustSetup();
  humidInit();
  gasInit();
}

// ======== LOOP ========
void loop() {
  float humid = humidRead();
  float gas = gasRead();
  float pm25 = dustRead();

  // Fill packet
  strcpy(pkt.cmd, "none");
  pkt.pm25  = pm25;
  pkt.humid = humid;
  pkt.gas   = gas;

  // Log for debugging
  Serial.print("Humid: "); Serial.print(humid);
  Serial.print("   Gas: "); Serial.println(gas);
  Serial.print("   PM2.5: "); Serial.println(pm25);

  checkMotor(pm25);

  // Send
  esp_err_t result = esp_now_send(gatewayMac, (uint8_t*)&pkt, sizeof(pkt));

  if (result != ESP_OK) {
    Serial.print("Send error: ");
    Serial.println(result);
  }

  delay(3000);
}
