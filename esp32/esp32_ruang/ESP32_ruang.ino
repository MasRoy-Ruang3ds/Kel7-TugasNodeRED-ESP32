/*
  Tugas MQTT-NodeRED - Kelompok 7
  ESP32 + potensiometer (simulasi input suhu dan kelembapan).
  Satu board mewakili satu ruangan; ganti ROOM untuk board lainnya.
  Broker lokal laptop: 172.20.10.2:1883

  Potensiometer: kaki luar ke 3V3 dan GND, kaki tengah ke GPIO34.
  LED1: GPIO25 -> resistor 220-330 ohm -> anoda LED; katoda ke GND.
  LED2: GPIO26 -> resistor 220-330 ohm -> anoda LED; katoda ke GND.
*/
#include <WiFi.h>
#include <EspMQTTClient.h>

// ===== CUKUP GANTI ROOM DI BAWAH INI UNTUK TIAP BOARD =====
#define KELOMPOK "klp7"
#define ROOM     "ruang2" // ubah menjadi "ruang1", "ruang2", "ruang3", atau "ruang4"

const char* WIFI_SSID = "SSID_LAB";
const char* WIFI_PASSWORD = "PASSWORD_LAB";
const char* MQTT_SERVER = "172.20.10.2";
const uint16_t MQTT_PORT = 1883;

const int POT_PIN = 34;
const int LED1_PIN = 25;
const int LED2_PIN = 26;

String baseTopic, topicTemperature, topicHumidity, topicStatus, topicLamp1, topicLamp2;
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;

// Client ID otomatis menjadi "esp32-klp7-ruang4" (mengikuti #define ROOM di atas)
EspMQTTClient client(
  WIFI_SSID,
  WIFI_PASSWORD,
  MQTT_SERVER,
  "", "",
  "esp32-" KELOMPOK "-" ROOM,
  MQTT_PORT
);

void onConnectionEstablished() {
  client.subscribe(topicLamp1, [](const String &payload) {
    if (payload == "1") { digitalWrite(LED1_PIN, HIGH); Serial.println("MQTT: Lampu 1 NYALA"); }
    else if (payload == "0") { digitalWrite(LED1_PIN, LOW); Serial.println("MQTT: Lampu 1 MATI"); }
  }, 1);

  client.subscribe(topicLamp2, [](const String &payload) {
    if (payload == "1") { digitalWrite(LED2_PIN, HIGH); Serial.println("MQTT: Lampu 2 NYALA"); }
    else if (payload == "0") { digitalWrite(LED2_PIN, LOW); Serial.println("MQTT: Lampu 2 MATI"); }
  }, 1);

  client.publish(topicStatus, "online", true);
  Serial.println("MQTT tersambung; status ONLINE dikirim.");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  analogReadResolution(12); // ADC ESP32: 0..4095

  baseTopic = String("pens/") + KELOMPOK + "/" + ROOM;
  topicTemperature = baseTopic + "/temperature";
  topicHumidity = baseTopic + "/humidity";
  topicStatus = baseTopic + "/status";
  topicLamp1 = baseTopic + "/lamp1";
  topicLamp2 = baseTopic + "/lamp2";

  client.setKeepAlive(15);
  client.enableLastWillMessage(topicStatus.c_str(), "offline", true);

  Serial.println("=== ESP32 MQTT Kelompok 7 ===");
  Serial.print("Ruangan: "); Serial.println(ROOM);
  Serial.print("Client ID: esp32-"); Serial.print(KELOMPOK); Serial.print("-"); Serial.println(ROOM);
  Serial.print("Broker: "); Serial.print(MQTT_SERVER); Serial.print(":"); Serial.println(MQTT_PORT);
  Serial.print("Topic suhu: "); Serial.println(topicTemperature);
  Serial.print("Topic kelembapan: "); Serial.println(topicHumidity);
  Serial.println("Menunggu koneksi Wi-Fi/MQTT...");
}

void loop() {
  client.loop();
  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;
    int raw = analogRead(POT_PIN);
    
    // Simulasi: pot minimum -> 15 C dan 20%; maksimum -> 40 C dan 90%.
    float temperature = 15.0f + (raw / 4095.0f) * 25.0f;
    float humidity = 20.0f + (raw / 4095.0f) * 70.0f;
    
    char tempPayload[12], humPayload[12];
    snprintf(tempPayload, sizeof(tempPayload), "%.1f", temperature);
    snprintf(humPayload, sizeof(humPayload), "%.1f", humidity);

    Serial.print("["); Serial.print(ROOM); Serial.print("] ADC=");
    Serial.print(raw); Serial.print(" | Suhu =");
    Serial.print(tempPayload); Serial.print(" C | Kelembapan =");
    Serial.print(humPayload); Serial.println(" %");

    if (client.isConnected()) {
      client.publish(topicTemperature, tempPayload, false);
      client.publish(topicHumidity, humPayload, false);
    } else {
      Serial.println("MQTT belum terhubung; data belum dikirim.");
    }
  }
}