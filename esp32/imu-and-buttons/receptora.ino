/*
 * ESP32 RECEPTORA - Puente ESP-NOW -> UART EDU-CIAA
 * 
 * Recibe mensajes de la emisora por ESP-NOW y los reenvía
 * por UART a la EDU-CIAA manteniendo el framing (preámbulo + checksum).
 */

#include <WiFi.h>
#include <esp_now.h>

// ===== UART hacia EDU-CIAA =====
HardwareSerial CIAA(2);        // UART2
static const int CIAA_TX = 17; // ESP32 TX -> RX EDU-CIAA
static const int CIAA_RX = 16; // ESP32 RX <- TX EDU-CIAA (opcional)
static const uint32_t CIAA_BAUD = 115200;

// ===== Estructura del mensaje (igual que emisora) =====
typedef struct __attribute__((packed)) {
  uint8_t header1;    // 0xAA (sync)
  uint8_t header2;    // 0x55 (sync)
  uint8_t botones[4]; // b0..b3 (acordes)
  uint8_t velocity;   // velocity MIDI
  uint8_t checksum;   // XOR de bytes [2..N-2]
} ChordMessage;

// Buffer compartido con callback
volatile bool hasMsg = false;
ChordMessage lastMsg;

// ===== Callback ESP-NOW =====
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data,
                int len)
{
  if (len != (int)sizeof(ChordMessage)) {
    return; // descarta basura
  }

  memcpy((void*)&lastMsg, data, sizeof(lastMsg));
  hasMsg = true;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Imprimir MAC para configurar en emisora
  Serial.print("MAC Receptora: ");
  Serial.println(WiFi.macAddress());

  // UART a EDU-CIAA
  CIAA.begin(CIAA_BAUD, SERIAL_8N1, CIAA_RX, CIAA_TX);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    while (true) delay(100);
  }

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP32 Receptora lista (ESP-NOW -> UART CIAA)");
}

void loop() {
  if (!hasMsg) return;

  // Copia segura (evita race con callback)
  noInterrupts();
  ChordMessage msg = lastMsg;
  hasMsg = false;
  interrupts();

  // Validar header
  if (msg.header1 != 0xAA || msg.header2 != 0x55) {
    Serial.println("[ERROR] Header invalido, paquete descartado");
    return;
  }

  // Validar checksum
  uint8_t *p = (uint8_t*)&msg;
  uint8_t chk = 0;
  for (int i = 2; i < (int)sizeof(msg) - 1; i++) {
    chk ^= p[i];
  }

  if (chk != msg.checksum) {
    Serial.println("[ERROR] Checksum invalido, paquete descartado");
    return;
  }

  // Envío binario crudo a la EDU-CIAA
  CIAA.write((uint8_t*)&msg, sizeof(msg));
  delayMicroseconds(1000); // Pequeño delay para evitar overflow en CIAA

  // Debug
  Serial.print("TX->CIAA botones=");
  Serial.print(msg.botones[3]);
  Serial.print(msg.botones[2]);
  Serial.print(msg.botones[1]);
  Serial.print(msg.botones[0]);
  Serial.print(" vel=");
  Serial.println(msg.velocity);
}
