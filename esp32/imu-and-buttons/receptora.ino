/*
 * ESP32 RECEPTORA - Puente ESP-NOW -> UART EDU-CIAA
 * DEBUG:
 * Imprime todo paquete recibido, validaciones y reenvío UART
 */

#include <WiFi.h>
#include <esp_now.h>

// ===== UART hacia EDU-CIAA =====
HardwareSerial CIAA(2);           // UART2 (ESP32 clásico)
static const int CIAA_TX = 17;    // ESP32 TX -> RX EDU-CIAA
static const int CIAA_RX = 16;    // ESP32 RX <- TX EDU-CIAA (opcional)
static const uint32_t CIAA_BAUD = 115200;

// ===== Estructura del mensaje =====
typedef struct __attribute__((packed)) {
  uint8_t header1;    // 0xAA
  uint8_t header2;    // 0x55
  uint8_t botones[4]; // b0..b3
  uint8_t velocity;
  uint8_t checksum;
} ChordMessage;

// ===== Buffer compartido =====
volatile bool hasMsg = false;
ChordMessage lastMsg;

// ===== Callback ESP-NOW =====
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data,
                int len)
{
  Serial.print("[RX RAW] len=");
  Serial.print(len);
  Serial.print(" bytes: ");

  for (int i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  if (len != (int)sizeof(ChordMessage)) {
    Serial.println("[RX ERROR] Tamaño incorrecto, paquete descartado");
    return;
  }

  memcpy((void*)&lastMsg, data, sizeof(lastMsg));
  hasMsg = true;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  Serial.print("MAC Receptora: ");
  Serial.println(WiFi.macAddress());

  // UART a EDU-CIAA
  CIAA.begin(CIAA_BAUD, SERIAL_8N1, CIAA_RX, CIAA_TX);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ERROR] Inicializando ESP-NOW");
    while (true) delay(100);
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP32 Receptora lista (ESP-NOW -> UART CIAA)");
}

void loop() {
  if (!hasMsg) return;

  // Copia segura
  noInterrupts();
  ChordMessage msg = lastMsg;
  hasMsg = false;
  interrupts();

  Serial.println("----- PAQUETE RECIBIDO -----");
  Serial.print("Header: ");
  Serial.print(msg.header1, HEX);
  Serial.print(" ");
  Serial.println(msg.header2, HEX);

  Serial.print("Botones: ");
  Serial.print(msg.botones[3]); Serial.print(" ");
  Serial.print(msg.botones[2]); Serial.print(" ");
  Serial.print(msg.botones[1]); Serial.print(" ");
  Serial.println(msg.botones[0]);

  Serial.print("Velocity: ");
  Serial.println(msg.velocity);

  Serial.print("Checksum recibido: 0x");
  Serial.println(msg.checksum, HEX);

  // Validar header
  if (msg.header1 != 0xAA || msg.header2 != 0x55) {
    Serial.println("[ERROR] Header inválido, paquete descartado");
    return;
  }

  // Validar checksum
  uint8_t *p = (uint8_t*)&msg;
  uint8_t chk = 0;
  for (int i = 2; i < (int)sizeof(msg) - 1; i++) {
    chk ^= p[i];
  }

  Serial.print("Checksum calculado: 0x");
  Serial.println(chk, HEX);

  if (chk != msg.checksum) {
    Serial.println("[ERROR] Checksum inválido, paquete descartado");
    return;
  }

  Serial.println("[OK] Paquete válido");

  // Envío binario crudo a la EDU-CIAA
  CIAA.write((uint8_t*)&msg, sizeof(msg));
  delayMicroseconds(1000);

  Serial.println("[TX UART] Paquete reenviado a EDU-CIAA");
  Serial.println("-----------------------------");
}
