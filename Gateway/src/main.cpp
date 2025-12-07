#define BLYNK_TEMPLATE_ID "TMPL6gFQBbAGg"
#define BLYNK_TEMPLATE_NAME "Airpurifier"
#define BLYNK_AUTH_TOKEN "ig3G9wmK_zpKLAgOwoXGoqFZr4ITRQmo"

#include <BlynkSimpleEsp32.h>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <time.h>
#include <temp.h>
#include "firebase.h"

// ---------- Blynk -----------
void sendToBlynk(int pm25, float temp, float hum, float gas) {
    Blynk.virtualWrite(V0, pm25);
    Blynk.virtualWrite(V2, temp);
    Blynk.virtualWrite(V1, hum);  
    Blynk.virtualWrite(V3, gas);
    if (pm25 >= 50) {
        Blynk.logEvent("pm25_alert", String("PM2.5 high: ") + pm25);
    }
    Serial.print("Blynk 0-0");
}


// ---------- WiFi ----------
const char* WIFI_SSID = "R_R";
const char* WIFI_PASS = "Rin12345";

// ---------- Server ----------
const char* SERVER_URL = "http://172.20.10.9:5000/audio";

// ---------- I2S + Mic config ----------
#define I2S_WS   25
#define I2S_SCK  32
#define I2S_SD   33

#define SAMPLE_RATE       16000
#define SAMPLES_PER_READ  256
#define CHUNK_SAMPLES     32000

int32_t i2sReadBuf[SAMPLES_PER_READ];
int16_t audioChunk[CHUNK_SAMPLES];

// ---------- LED / Relay ----------
#define LED_PIN     26
#define REC_LED_PIN 27

bool purifierOn = false;

// ---------- ESP-NOW packet ----------
typedef struct {
  char cmd[8];
  float pm25;
  float humid;
  float gas;
} EspNowPacket;

EspNowPacket lastPkt;    
volatile bool hasPkt = false;
volatile bool hasSensorData = false;

// ---------- ESP-NOW receive callback ----------
void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  Serial.print("[ESPNOW] Received packet, length: ");
  Serial.println(len);

  if (len != sizeof(EspNowPacket)) {
    Serial.println("[ESPNOW] Error: Packet size mismatch");
    return;
  }

  memcpy(&lastPkt, data, sizeof(EspNowPacket));
  hasPkt = true;
  hasSensorData = true;

  Serial.print("[ESPNOW] Received data: cmd=");
  Serial.print(lastPkt.cmd);
  Serial.print(", pm25=");
  Serial.print(lastPkt.pm25);
  Serial.print(", humid=");
  Serial.print(lastPkt.humid);
  Serial.print(", gas=");
  Serial.println(lastPkt.gas);

}

// ---------- WiFi connect ----------
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("...");
  }
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("WiFi channel: ");
  Serial.println(WiFi.channel());

  Serial.print("Gateway MAC: ");
  Serial.println(WiFi.macAddress());
}

// ---------- ✅ NTP time init for Thailand (UTC+7) ----------
void initThaiTimeNTP() {
  const long gmtOffset_sec = 7 * 3600;  // Thailand UTC+7
  const int daylightOffset_sec = 0;    // Thailand has no DST

  configTime(gmtOffset_sec, daylightOffset_sec,
             "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time (Thailand)");
  time_t now = time(nullptr);

  while (now < 1700000000) {  // wait until valid time (2023+)
    delay(500);
    Serial.print("...");
    now = time(nullptr);
  }

  Serial.println("\nNTP time synced (Thailand)!");

  // (debug) print local Thailand time once
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("TH time now: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  }
}

// ---------- ESP-NOW init ----------
void initEspNowReceiver() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(onEspNowRecv);
  Serial.println("ESP-NOW receiver ready.");
}

// ---------- I2S setup ----------
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("I2S initialized.");
}

// ---------- record audio ----------
void recordChunk() {
  int index = 0;
  Serial.println("=== START RECORDING ===");

  while (index < CHUNK_SAMPLES) {
    size_t bytesRead = 0;
    esp_err_t result = i2s_read(
      I2S_NUM_0,
      (void*)i2sReadBuf,
      sizeof(i2sReadBuf),
      &bytesRead,
      portMAX_DELAY
    );
    if (result != ESP_OK || bytesRead == 0) continue;

    int samplesRead = bytesRead / sizeof(int32_t);
    for (int i = 0; i < samplesRead && index < CHUNK_SAMPLES; i++, index++) {
      int32_t s = i2sReadBuf[i];
      s >>= 8;
      audioChunk[index] = (int16_t)(s >> 8);
    }
  }

  Serial.println("=== STOP RECORDING ===");
}

// ---------- send to server ----------
String sendChunkToServer(int16_t* buf, size_t sampleCount) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/octet-stream");

  int byteCount = sampleCount * sizeof(int16_t);
  int httpCode = http.POST((uint8_t*)buf, byteCount);

  String response = "none";
  if (httpCode > 0) {
    response = http.getString();
    response.trim();
  } else {
    digitalWrite(LED_PIN, LOW);
    purifierOn = false;
  }
  http.end();
  return response;
}

// ---------- setup ----------
unsigned long lastHistory = 0;
const unsigned long historyInterval = 5000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // init lastPkt so no garbage before first ESP-NOW receive
  strcpy(lastPkt.cmd, "none");
  lastPkt.pm25 = 0.0f;
  lastPkt.humid = 0.0f;
  lastPkt.gas = 0.0f;

  tempInit();

  pinMode(LED_PIN, OUTPUT);
  pinMode(REC_LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(REC_LED_PIN, LOW);
  purifierOn = false;

  connectWiFi();
  initThaiTimeNTP(); 

  setupI2S();
  initEspNowReceiver();

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS); 
}

// ---------- loop ----------
void loop() {
  // Handle Blynk events
  Blynk.run(); 

  // 1) Voice record + STT
  digitalWrite(REC_LED_PIN, HIGH);
  recordChunk();
  digitalWrite(REC_LED_PIN, LOW);

  String cmd = sendChunkToServer(audioChunk, CHUNK_SAMPLES);

  if (cmd == "on") {
    digitalWrite(LED_PIN, HIGH);
    purifierOn = true;
  } else if (cmd == "off") {
    digitalWrite(LED_PIN, LOW);
    purifierOn = false;
  }

  // 2) If ESP-NOW packet arrived
  if (hasPkt) {
    hasPkt = false;

    EspNowPacket p = lastPkt;  // safe copy
    Serial.printf("[ESPNOW] cmd=%s pm=%.2f humid=%.2f gas=%.0f\n",
                  p.cmd, p.pm25, p.humid, p.gas);

    if (strcmp(p.cmd, "on") == 0) {
      digitalWrite(LED_PIN, HIGH);
      purifierOn = true;
    } else if (strcmp(p.cmd, "off") == 0) {
      digitalWrite(LED_PIN, LOW);
      purifierOn = false;
    }
  }

  // 3) temp read
  float temp = tempRead();
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");

  // 4) Firebase upload only after first ESP-NOW data
  if (hasSensorData) {
    sendToFirebase(lastPkt.pm25, lastPkt.humid, temp, lastPkt.gas);

    if (millis() - lastHistory > historyInterval) {
      lastHistory = millis();
      pushHistory(lastPkt.pm25, lastPkt.humid, temp, lastPkt.gas);
    }
  }

  // Send data to Blynk app
  if (hasSensorData) {
    sendToBlynk(lastPkt.pm25, temp,  lastPkt.humid, lastPkt.gas);
  }

  delay(300);  // Delay before next loop iteration
}
