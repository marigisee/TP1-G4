#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// MAC del receptor
uint8_t receiverAddress[] = {0xF0,0x24,0xF9,0x0C,0x4F,0x34};

Adafruit_MPU6050 mpu;

// Definición del struct
typedef struct {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

IMUMessage mensaje;

// Pines de botones
const uint8_t buttonPins[4] = {14, 27, 26, 25};

// Timestamp del último envío
uint32_t t_send;
bool ack_received = false;

// Callback de envío
void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  if(status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Mensaje enviado ");
  } else {
    Serial.println("Error al enviar ");
  }
}

// Callback de recepción (ACK)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if(len > 0) {
    ack_received = true;
    uint32_t t_recv_ack = millis();
    uint32_t round_trip = t_recv_ack - t_send;
    Serial.print("Round-trip delay: ");
    Serial.print(round_trip);
    Serial.println(" ms");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Configurar botones
  for(int i=0;i<4;i++) pinMode(buttonPins[i], INPUT_PULLUP);

  // Inicializar ESP-NOW
  if(esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Agregar peer del receptor
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0; // usar mismo canal
  peerInfo.encrypt = false;
  if(esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Error agregando peer");
  }

  // Inicializar MPU6050
  if(mpu.begin()) {
    Serial.println("No se encontró MPU6050!");
    while(1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Emisor listo ");
}

void loop() {
  // Leer MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  mensaje.ax = a.acceleration.x;
  mensaje.ay = a.acceleration.y;
  mensaje.az = a.acceleration.z;
  mensaje.gx = g.gyro.x;
  mensaje.gy = g.gyro.y;
  mensaje.gz = g.gyro.z;

  // Leer botones
  for(int i=0;i<4;i++)
    mensaje.botones[i] = digitalRead(buttonPins[i]) == LOW ? 1 : 0;

  // Enviar struct
  t_send = millis();
  ack_received = false;
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&mensaje, sizeof(mensaje));

  if(result != ESP_OK){
    Serial.println("Error iniciando envío ");
  }

  // Esperar hasta recibir ACK o timeout 100 ms
  uint32_t start = millis();
  while(!ack_received && millis() - start < 100){
    delay(1);
  }

  if(!ack_received){
    Serial.println("ACK no recibido ");
  }

  delay(200); // enviar cada 200 ms
}