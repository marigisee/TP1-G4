/*
 * ESP32 EMISORA - IMU + Botones de Acordes
 * 
 * Detecta rasgueos con IMU (MPU6050) y envía comando cuando:
 * 1. Se presiona el botón STRUM
 * 2. Se presiona al menos un botón de acorde
 * 3. Se detecta un rasgueo válido por magnitud del IMU
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// ===== MAC de la ESP32 Receptora =====
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // CAMBIAR por MAC real

// ===== Pines IMU =====
#define SDA_PIN 21
#define SCL_PIN 22

// ===== Pines Botones de Acordes =====
#define PIN_B0 32  // bit0
#define PIN_B1 33  // bit1
#define PIN_B2 25  // bit2
#define PIN_B3 26  // bit3

// ===== Pin Botón STRUM =====
#define PIN_STRUM 27

// ===== Configuración =====
#define BUTTONS_ACTIVE_LOW true  // true si botones conectados a GND
#define DEBOUNCE_MS 30

// ===== Umbrales IMU (de test-values-imu-withumbral.ino) =====
const float UMBRAL_NORMAL = 11.0f;  // Mínimo para rasgueo normal
const float UMBRAL_FUERTE = 16.0f;  // Mínimo para rasgueo fuerte
const uint32_t TIEMPO_ESPERA = 300; // ms entre detecciones (anti-rebote)

// ===== Velocities MIDI =====
const uint8_t VEL_NORMAL = 80;
const uint8_t VEL_FUERTE = 120;

// ===== Estructura del mensaje simplificada =====
typedef struct __attribute__((packed)) {
  uint8_t header1;    // 0xAA (sync)
  uint8_t header2;    // 0x55 (sync)
  uint8_t botones[4]; // b0..b3 (acordes)
  uint8_t velocity;   // velocity MIDI según fuerza del rasgueo
  uint8_t checksum;   // XOR de bytes [2..N-2]
} ChordMessage;

ChordMessage msg;
Adafruit_MPU6050 mpu;

// ===== Estado de botones =====
bool lastStableStrum = true;  // Estado estable del STRUM (HIGH = no presionado)
bool lastReadStrum = true;
uint32_t lastChangeStrum = 0;

// ===== Estado del IMU =====
uint32_t ultimoRasgueoMs = 0;

// ===== ESP-NOW callback =====
void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  // Opcional: debug
}

// ===== Leer estado de un botón con polaridad configurable =====
bool isButtonPressed(int pin) {
  int raw = digitalRead(pin);
  return BUTTONS_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

// ===== Construir máscara de acordes (4 bits) =====
uint8_t buildChordMask() {
  uint8_t mask = 0;
  if (isButtonPressed(PIN_B0)) mask |= (1 << 0);
  if (isButtonPressed(PIN_B1)) mask |= (1 << 1);
  if (isButtonPressed(PIN_B2)) mask |= (1 << 2);
  if (isButtonPressed(PIN_B3)) mask |= (1 << 3);
  return mask;
}

// ===== Enviar paquete =====
void sendPacket(uint8_t chordMask, uint8_t velocity) {
  msg.header1 = 0xAA;
  msg.header2 = 0x55;
  msg.botones[0] = (chordMask & (1 << 0)) ? 1 : 0;
  msg.botones[1] = (chordMask & (1 << 1)) ? 1 : 0;
  msg.botones[2] = (chordMask & (1 << 2)) ? 1 : 0;
  msg.botones[3] = (chordMask & (1 << 3)) ? 1 : 0;
  msg.velocity = velocity;

  // Calcular checksum (XOR de bytes excepto headers y checksum)
  uint8_t *p = (uint8_t*)&msg;
  uint8_t chk = 0;
  for (int i = 2; i < (int)sizeof(msg) - 1; i++) {
    chk ^= p[i];
  }
  msg.checksum = chk;

  esp_err_t res = esp_now_send(receiverAddress, (uint8_t*)&msg, sizeof(msg));

  // Debug
  Serial.print("[SEND] botones=");
  Serial.print(msg.botones[3]);
  Serial.print(msg.botones[2]);
  Serial.print(msg.botones[1]);
  Serial.print(msg.botones[0]);
  Serial.print(" vel=");
  Serial.print(velocity);
  Serial.print(" esp_now=");
  Serial.println(res == ESP_OK ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  
  // Configurar pines de botones
  pinMode(PIN_B0, INPUT_PULLUP);
  pinMode(PIN_B1, INPUT_PULLUP);
  pinMode(PIN_B2, INPUT_PULLUP);
  pinMode(PIN_B3, INPUT_PULLUP);
  pinMode(PIN_STRUM, INPUT_PULLUP);

  // Inicializar I2C para IMU
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializar MPU6050
  if (mpu.begin()) {
    Serial.println("Error: No se encontró MPU6050");
    while (1) delay(10);
  }
  Serial.println("MPU6050 OK");

  // Configurar IMU (rango 8G para rasgueos fuertes)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Inicializar WiFi y ESP-NOW
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    while (1) delay(100);
  }

  esp_now_register_send_cb(OnDataSent);

  // Registrar peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error agregando peer");
    while (1) delay(100);
  }

  Serial.println("ESP32 Emisora lista (IMU + Botones)");
  delay(100);
}

void loop() {
  uint32_t now = millis();

  // ========== 1. Leer IMU ==========
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calcular magnitud de aceleración
  float magnitud = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

  // ========== 2. Detectar rasgueo válido ==========
  uint8_t velocityDetectada = 0;
  bool rasgueoDetectado = false;

  if ((now - ultimoRasgueoMs) > TIEMPO_ESPERA) {
    if (magnitud >= UMBRAL_FUERTE) {
      velocityDetectada = VEL_FUERTE;
      rasgueoDetectado = true;
      ultimoRasgueoMs = now;
      Serial.println("!!! RASGUEO FUERTE !!!");
    }
    else if (magnitud > UMBRAL_NORMAL) {
      velocityDetectada = VEL_NORMAL;
      rasgueoDetectado = true;
      ultimoRasgueoMs = now;
      Serial.println("!!! RASGUEO NORMAL !!!");
    }
  }

  // ========== 3. Leer botón STRUM con debounce ==========
  bool rawStrum = digitalRead(PIN_STRUM);
  if (rawStrum != lastReadStrum) {
    lastReadStrum = rawStrum;
    lastChangeStrum = now;
  }
  
  bool strumEstable = lastStableStrum;
  if ((now - lastChangeStrum) >= DEBOUNCE_MS && lastStableStrum != lastReadStrum) {
    strumEstable = lastReadStrum;
    lastStableStrum = strumEstable;
  }

  bool strumPressed = BUTTONS_ACTIVE_LOW ? (strumEstable == LOW) : (strumEstable == HIGH);

  // ========== 4. Leer botones de acordes ==========
  uint8_t chordMask = buildChordMask();
  bool hayAcorde = (chordMask != 0);

  // ========== 5. Enviar si se cumplen las 3 condiciones ==========
  // - STRUM presionado
  // - Al menos un botón de acorde presionado
  // - Rasgueo válido detectado por IMU
  if (strumPressed && hayAcorde && rasgueoDetectado) {
    sendPacket(chordMask, velocityDetectada);
  }

  // Delay pequeño para estabilidad del loop (sin bloquear demasiado)
  delay(5);
}
