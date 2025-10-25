// ============ ESP32 - Receptora ============

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>


// ==================== Estructura mensaje ====================
typedef struct __attribute__((packed)) {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

// ==================== Configuración ====================

HardwareSerial LINK(2);
const int PIN_TX = 17;
const int PIN_RX = 16;
const uint32_t BAUD = 115200;

#define WIFI_CHANNEL 1

// ==================== Callbacks ====================

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len != sizeof(IMUMessage)) {
    Serial.printf("Tamaño incorrecto: %d bytes\n", len);
    return;
  }

  IMUMessage msg;
  memcpy(&msg, data, sizeof(msg));

  // Envio del mensaje a la EDU-CIAA
  LINK.write((uint8_t*)&msg, sizeof(IMUMessage));

  // Print de los datos enviados
  Serial.printf("Rx IMU: ax=%.2f ay=%.2f az=%.2f | gx=%.2f gy=%.2f gz=%.2f | B=[%u %u %u %u]\n",
                msg.ax, msg.ay, msg.az, msg.gx, msg.gy, msg.gz,
                msg.botones[0], msg.botones[1], msg.botones[2], msg.botones[3]);
}

// ==================== Setup ====================
void setup() {

  // Inicializar Serial Monitor
  Serial.begin(115200);

  // Inicializar conexión UART
  LINK.begin(BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  // Inicializar WIFI
  WiFi.mode(WIFI_STA);

  // Fijar canal Wi-Fi
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);


  Serial.print("MAC receptora: ");
  Serial.println(WiFi.macAddress());

  // Inicializar ESP_NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    while (true) delay(1000);
  }

  // Registrar callback 
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receptora lista (ESP-NOW -> UART2)");
}

// ==================== Loop ====================
void loop() {
  // nada, todo sucede en OnDataRecv
}
