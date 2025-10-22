#include <WiFi.h>
#include <esp_now.h>

// Definición del struct igual que en el emisor
typedef struct {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

// Pines de LEDs
const uint8_t ledPins[4] = {2, 4, 16, 17}; // ajustar según tu placa

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if(len != sizeof(IMUMessage)) {
    Serial.println("Error: tamaño de mensaje incorrecto");
    return;
  }

  IMUMessage mensajeRecibido;
  memcpy(&mensajeRecibido, data, sizeof(mensajeRecibido));

  // Mostrar datos en Serial
  Serial.printf("De: %02X:%02X:%02X:%02X:%02X:%02X\n",
                info->src_addr[0], info->src_addr[1], info->src_addr[2],
                info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.print("Aceleración: ");
  Serial.print(mensajeRecibido.ax); Serial.print(", ");
  Serial.print(mensajeRecibido.ay); Serial.print(", ");
  Serial.println(mensajeRecibido.az);

  Serial.print("Giroscopio: ");
  Serial.print(mensajeRecibido.gx); Serial.print(", ");
  Serial.print(mensajeRecibido.gy); Serial.print(", ");
  Serial.println(mensajeRecibido.gz);

  Serial.print("Botones: ");
  for(int i=0;i<4;i++) {
    Serial.print(mensajeRecibido.botones[i]);
    // Encender LED según botón
    digitalWrite(ledPins[i], mensajeRecibido.botones[i] ? HIGH : LOW);
  }
  Serial.println("\n");

  // Enviar ACK (1 byte) de vuelta al emisor
  uint8_t ack = 1;
  esp_now_send(info->src_addr, &ack, sizeof(ack));
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Configurar LEDs
  for(int i=0;i<4;i++) pinMode(ledPins[i], OUTPUT);

  if(esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Registrar callback de recepción
  esp_now_register_recv_cb(OnDataRecv);

  // Agregar peer del emisor (reemplaza con la MAC del emisor)
  uint8_t senderAddress[] = {0x94,0x54,0xC5,0xAE,0xF0,0xA4};
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 0;  // mismo canal que el emisor
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Error agregando peer");
  }

  Serial.println("Receptor listo");
}

void loop() {
  // Todo se maneja por callback
}