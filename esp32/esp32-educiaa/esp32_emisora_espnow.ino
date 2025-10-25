// ============ ESP32 - Emisora ============

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <math.h>

// ==================== Configuración ====================
// MAC del receptor (reemplazá por la real)
uint8_t receiverAddress[] = {0xF0,0x24,0xF9,0x0C,0x4F,0x34};

// Canal WiFi fijo (debe coincidir con la receptora)
#define WIFI_CHANNEL 1

// ==================== Estructura mensaje ====================
typedef struct __attribute__((packed)) {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

// ==================== Callbacks ====================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Enviado OK" : "Error al enviar");
}

// ==================== Setup ====================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Fijar canal WiFi
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  Serial.print("Emisora MAC: "); Serial.println(WiFi.macAddress());

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    while (true);
  }

  esp_now_register_send_cb(OnDataSent);

  // Registrar peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error agregando peer");
    while (true);
  }

  Serial.println("Emisora lista para enviar datos IMU");
}

// ==================== Loop ====================
void loop() {
  static float t = 0;
  IMUMessage msg;

  // Construcción de los datos mockeados
  msg.ax = sinf(t);
  msg.ay = cosf(t);
  msg.az = 1.0f;
  msg.gx = 0.1f * sinf(2 * t);
  msg.gy = 0.1f * cosf(2 * t);
  msg.gz = 0.05f;

  msg.botones[0] = (int(t) % 2);
  msg.botones[1] = (int(t / 2) % 2);
  msg.botones[2] = (int(t / 3) % 2);
  msg.botones[3] = (int(t / 4) % 2);

  // Enviar struct
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&msg, sizeof(msg));

  // Si se realizó correctamente el envío
  if (result == ESP_OK) {
    Serial.printf("TX IMU: ax=%.2f ay=%.2f az=%.2f | gx=%.2f gy=%.2f gz=%.2f | B=[%u %u %u %u]\n",
                  msg.ax, msg.ay, msg.az, msg.gx, msg.gy, msg.gz,
                  msg.botones[0], msg.botones[1], msg.botones[2], msg.botones[3]);
  } else {
    Serial.println("Error enviando mensaje!");
  }

  delay(200);
  t += 0.1f;
}
