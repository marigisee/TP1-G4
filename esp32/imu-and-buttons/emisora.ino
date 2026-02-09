/*
 * ESP32 EMISORA - IMU + Botones de Acordes con LEDs
 * 
 * DEBUG:
 * Muestra en Serial:
 * STRUM B3 B2 B1 B0 | MAGNITUD | EVENTO
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// ===== MAC de la ESP32 Receptora =====
uint8_t receiverAddress[] = {0xF0,0x24,0xF9,0x0C,0x4F,0x34}; // CAMBIAR por MAC real

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

// ===== Pines LEDs =====
#define LED_ROJO 12
#define LED_VERDE 13

// ===== Configuración =====
#define BUTTONS_ACTIVE_LOW true
#define DEBOUNCE_MS 30

// ===== Umbrales IMU =====
const float UMBRAL_NORMAL = 11.0f;
const float UMBRAL_FUERTE = 16.0f;
const uint32_t TIEMPO_ESPERA = 300;

// ===== LEDs =====
const uint32_t LED_VERDE_TIEMPO = 100;

// ===== Velocities =====
const uint8_t VEL_NORMAL = 80;
const uint8_t VEL_FUERTE = 120;

// ===== Estructura =====
typedef struct __attribute__((packed)) {
  uint8_t header1;
  uint8_t header2;
  uint8_t botones[4];
  uint8_t velocity;
  uint8_t checksum;
} ChordMessage;

ChordMessage msg;
Adafruit_MPU6050 mpu;

// ===== Estado botones =====
bool lastStableStrum = true;
bool lastReadStrum = true;
uint32_t lastChangeStrum = 0;

// ===== IMU =====
uint32_t ultimoRasgueoMs = 0;

// ===== LEDs =====
uint32_t ledVerdeApagarMs = 0;
bool ledVerdeEncendido = false;

// ===== ESP-NOW =====
void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  // opcional
}

bool isButtonPressed(int pin) {
  int raw = digitalRead(pin);
  return BUTTONS_ACTIVE_LOW ? (raw == LOW) : (raw == HIGH);
}

uint8_t buildChordMask() {
  uint8_t mask = 0;
  if (isButtonPressed(PIN_B0)) mask |= (1 << 0);
  if (isButtonPressed(PIN_B1)) mask |= (1 << 1);
  if (isButtonPressed(PIN_B2)) mask |= (1 << 2);
  if (isButtonPressed(PIN_B3)) mask |= (1 << 3);
  return mask;
}

void encenderLedVerde() {
  digitalWrite(LED_VERDE, HIGH);
  ledVerdeEncendido = true;
  ledVerdeApagarMs = millis() + LED_VERDE_TIEMPO;
}

void sendPacket(uint8_t chordMask, uint8_t velocity) {
  msg.header1 = 0xAA;
  msg.header2 = 0x55;
  msg.botones[0] = (chordMask & (1 << 0)) ? 1 : 0;
  msg.botones[1] = (chordMask & (1 << 1)) ? 1 : 0;
  msg.botones[2] = (chordMask & (1 << 2)) ? 1 : 0;
  msg.botones[3] = (chordMask & (1 << 3)) ? 1 : 0;
  msg.velocity = velocity;

  uint8_t *p = (uint8_t*)&msg;
  uint8_t chk = 0;
  for (int i = 2; i < (int)sizeof(msg) - 1; i++) chk ^= p[i];
  msg.checksum = chk;

  esp_err_t res = esp_now_send(receiverAddress, (uint8_t*)&msg, sizeof(msg));

  Serial.print("[SEND] ");
  Serial.print("STRUM ");
  Serial.print(BUTTONS_ACTIVE_LOW ? (digitalRead(PIN_STRUM) == LOW) : (digitalRead(PIN_STRUM) == HIGH));
  Serial.print(" | ");
  Serial.print(msg.botones[3]); Serial.print(" ");
  Serial.print(msg.botones[2]); Serial.print(" ");
  Serial.print(msg.botones[1]); Serial.print(" ");
  Serial.print(msg.botones[0]);
  Serial.print(" | vel=");
  Serial.print(velocity);
  Serial.print(" | esp_now=");
  Serial.println(res == ESP_OK ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_B0, INPUT_PULLUP);
  pinMode(PIN_B1, INPUT_PULLUP);
  pinMode(PIN_B2, INPUT_PULLUP);
  pinMode(PIN_B3, INPUT_PULLUP);
  pinMode(PIN_STRUM, INPUT_PULLUP);

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);

  digitalWrite(LED_ROJO, HIGH);
  digitalWrite(LED_VERDE, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (mpu.begin()) {
    Serial.println("❌ MPU6050 no detectado");
    while (1) {
      digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
      delay(200);
    }
  }
  Serial.println("✅ MPU6050 OK");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Error inicializando ESP-NOW");
    while (1) {
      digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
      delay(100);
    }
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Error agregando peer");
    while (1) {
      digitalWrite(LED_ROJO, !digitalRead(LED_ROJO));
      delay(100);
    }
  }

  Serial.println("ESP32 Emisora lista");
  Serial.println("Formato debug:");
  Serial.println("STRUM B3 B2 B1 B0 | MAG | EVENTO");
  delay(100);
}

void loop() {
  uint32_t now = millis();

  // ===== LEDs =====
  if (ledVerdeEncendido && now >= ledVerdeApagarMs) {
    digitalWrite(LED_VERDE, LOW);
    ledVerdeEncendido = false;
  }

  // ===== IMU =====
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float magnitud = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

  // ===== Detectar rasgueo =====
  uint8_t velocityDetectada = 0;
  bool rasgueoDetectado = false;

  if ((now - ultimoRasgueoMs) > TIEMPO_ESPERA) {
    if (magnitud >= UMBRAL_FUERTE) {
      velocityDetectada = VEL_FUERTE;
      rasgueoDetectado = true;
      ultimoRasgueoMs = now;
      encenderLedVerde();
      Serial.print("[IMU] FUERTE mag=");
      Serial.println(magnitud);
    }
    else if (magnitud >= UMBRAL_NORMAL) {
      velocityDetectada = VEL_NORMAL;
      rasgueoDetectado = true;
      ultimoRasgueoMs = now;
      encenderLedVerde();
      Serial.print("[IMU] NORMAL mag=");
      Serial.println(magnitud);
    }
  }

  // ===== STRUM con debounce =====
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

  // ===== Botones =====
  uint8_t chordMask = buildChordMask();
  bool hayAcorde = (chordMask != 0);

  // ===== DEBUG continuo =====
  static uint32_t lastPrint = 0;
  if (now - lastPrint > 100) {   // cada 100 ms
    lastPrint = now;
    Serial.print(strumPressed); Serial.print(" ");
    Serial.print((chordMask >> 3) & 1); Serial.print(" ");
    Serial.print((chordMask >> 2) & 1); Serial.print(" ");
    Serial.print((chordMask >> 1) & 1); Serial.print(" ");
    Serial.print((chordMask >> 0) & 1);
    Serial.print(" | mag=");
    Serial.print(magnitud, 2);
    if (rasgueoDetectado) Serial.print(" | RASGUEO!");
    Serial.println();
  }

  // ===== Envío =====
  if (strumPressed && hayAcorde && rasgueoDetectado) {
    sendPacket(chordMask, velocityDetectada);
    Serial.print("enviandoooo");
  }

  delay(5);
}
