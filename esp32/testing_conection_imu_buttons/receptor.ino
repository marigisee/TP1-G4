#include <WiFi.h>
#include <esp_now.h>

// Definición del struct igual que en el emisor
typedef struct {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

// Pines de LEDs
const uint8_t ledPins[4] = {2, 4, 16, 17}; // ajusta según tu placa

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

  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Receptor listo ✅");
}

void loop() {
  // Todo se maneja por callback
}