#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Este programa envia el valor leido por los pulsadores ( en formato array / numero binario ) + lectura del IMU
// En formato registro struct

// MAC del receptor
uint8_t receiverAddress[] = {0xF0,0x24,0xF9,0x0C,0x4F,0x34};

// Instancia MPU6050
Adafruit_MPU6050 mpu;

// Definición del struct de datos a enviar
typedef struct {
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4];
} IMUMessage;

IMUMessage mensaje;

// Pines de botones
const uint8_t buttonPins[4] = {14, 27, 26, 25};

void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Enviado" : "Error");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Configurar botones como entrada
  for(int i=0;i<4;i++) pinMode(buttonPins[i], INPUT_PULLUP);

  // Inicializar ESP-NOW
  if(esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  // Agregar peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

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
  // Leer datos del MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  mensaje.ax = a.acceleration.x;
  mensaje.ay = a.acceleration.y;
  mensaje.az = a.acceleration.z;

  mensaje.gx = g.gyro.x;
  mensaje.gy = g.gyro.y;
  mensaje.gz = g.gyro.z;

  // Leer botones
  for(int i=0;i<4;i++){
    mensaje.botones[i] = digitalRead(buttonPins[i]) == LOW ? 1 : 0;
  }

  // Enviar struct
  esp_now_send(receiverAddress, (uint8_t *)&mensaje, sizeof(mensaje));

  // Debug en serial
  Serial.print("Botones: ");
  for(int i=0;i<4;i++) Serial.print(mensaje.botones[i]);
  Serial.println();

  delay(200); // enviar cada 200ms
}