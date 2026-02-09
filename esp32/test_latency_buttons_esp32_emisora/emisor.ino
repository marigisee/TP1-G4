#include <WiFi.h>
#include <esp_now.h>

// ====== CAMBIAR POR MAC REAL DE TU RECEPTORA ======
uint8_t receiverAddress[] = {0xF0, 0x24, 0xF9, 0x0C, 0x4F, 0x34};

// Estructura EXACTA a la que espera la CIAA
typedef struct __attribute__((packed)) {
  uint8_t header1;    // 0xAA (sync)
  uint8_t header2;    // 0x55 (sync)
  float ax, ay, az;
  float gx, gy, gz;
  uint8_t botones[4]; // b0..b3
  uint8_t checksum;   // XOR de todos los bytes (excepto headers y checksum)
} IMUMessage;

IMUMessage msg;

// Pines físicos
const uint8_t PIN_STRUM = 14;  // B0
const uint8_t PIN_B1    = 27;
const uint8_t PIN_B2    = 26;
const uint8_t PIN_B3    = 25;

static const bool BUTTONS_ACTIVE_LOW = true;

static const uint32_t DEBOUNCE_MS = 30;

// Estados
bool lastStrumStable = true;
bool lastStrumRead   = true;
uint32_t lastStrumChange = 0;

bool lastStable[3] = {true, true, true}; // B1,B2,B3
bool lastRead[3]   = {true, true, true};
uint32_t lastChange[3] = {0,0,0};

static inline uint8_t buildChordMask() {
  uint8_t mask = 0;
  if (!lastStable[0]) mask |= (1 << 1); // B1
  if (!lastStable[1]) mask |= (1 << 2); // B2
  if (!lastStable[2]) mask |= (1 << 3); // B3
  return mask; // b0 siempre 0
}

void sendPacket(uint8_t mask, float gz) {
  msg.header1 = 0xAA;                       // Preambulo
  msg.header2 = 0x55;                       // Preambulo
  msg.botones[0] = 0;                       // B0 NO participa del acorde
  msg.botones[1] = (mask & (1<<1)) ? 1 : 0;
  msg.botones[2] = (mask & (1<<2)) ? 1 : 0;
  msg.botones[3] = (mask & (1<<3)) ? 1 : 0;

  msg.ax = 0.0f;
  msg.ay = 0.0f;
  msg.az = 9.81f;
  msg.gx = 0.0f;
  msg.gy = 0.0f;
  msg.gz = gz;

  // Calcular checksum (XOR de todos los bytes excepto header y checksum)
  uint8_t *p = (uint8_t*)&msg;
  uint8_t chk = 0;
  for (int i = 2; i < (int)sizeof(msg) - 1; i++) {
    chk ^= p[i];
  }
  msg.checksum = chk;

  esp_err_t res = esp_now_send(receiverAddress, (uint8_t*)&msg, sizeof(msg));

 // ===== PRINT DEBUG =====
  Serial.print("[SEND] mask=");
  Serial.print(mask, BIN);

  Serial.print(" botones=[");
  Serial.print(msg.botones[3]); Serial.print(" ");
  Serial.print(msg.botones[2]); Serial.print(" ");
  Serial.print(msg.botones[1]); Serial.print(" ");
  Serial.print(msg.botones[0]); Serial.print("]");

  Serial.print(" | IMU: ");
  Serial.print("ax="); Serial.print(msg.ax, 2);
  Serial.print(" ay="); Serial.print(msg.ay, 2);
  Serial.print(" az="); Serial.print(msg.az, 2);

  Serial.print(" gx="); Serial.print(msg.gx, 2);
  Serial.print(" gy="); Serial.print(msg.gy, 2);
  Serial.print(" gz="); Serial.print(msg.gz, 2);

  Serial.print(" | esp_now=");
  Serial.println(res == ESP_OK ? "OK" : "ERR");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  pinMode(PIN_STRUM, INPUT_PULLUP);
  pinMode(PIN_B1, INPUT_PULLUP);
  pinMode(PIN_B2, INPUT_PULLUP);
  pinMode(PIN_B3, INPUT_PULLUP);

  lastStrumStable = digitalRead(PIN_STRUM);
  lastStrumRead   = lastStrumStable;
  lastStrumChange = millis();

  int pins[3] = {PIN_B1, PIN_B2, PIN_B3};
  for (int i=0;i<3;i++) {
    lastStable[i] = digitalRead(pins[i]);
    lastRead[i]   = lastStable[i];
    lastChange[i] = millis();
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init error");
    while(true);
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("ESP32 Emisora lista");
  Serial.println("B1–B3 = acorde | B0 = STRUM");
}

void loop() {
  uint32_t now = millis();

  // -------- B1–B3 (acorde) --------
  int pins[3] = {PIN_B1, PIN_B2, PIN_B3};
  bool chordChanged = false;

  for (int i=0;i<3;i++) {
    bool r = digitalRead(pins[i]);
    if (r != lastRead[i]) {
      lastRead[i] = r;
      lastChange[i] = now;
    }
    if ((now - lastChange[i]) >= DEBOUNCE_MS && lastStable[i] != lastRead[i]) {
      lastStable[i] = lastRead[i];
      chordChanged = true;
    }
  }

  uint8_t mask = buildChordMask();

  // NO enviar cuando cambia acorde, solo actualizar estado interno
  // El envío se hace únicamente cuando se presiona STRUM

  // -------- B0 (STRUM) --------
  bool r = digitalRead(PIN_STRUM);
  if (r != lastStrumRead) {
    lastStrumRead = r;
    lastStrumChange = now;
  }
  if ((now - lastStrumChange) >= DEBOUNCE_MS && lastStrumStable != lastStrumRead) {
    bool prev = lastStrumStable;
    lastStrumStable = lastStrumRead;

    // flanco HIGH->LOW = STRUM
    if (prev == true && lastStrumStable == false) {
      uint8_t maskNow = 0;
      int r1 = digitalRead(PIN_B1);
      int r2 = digitalRead(PIN_B2);
      int r3 = digitalRead(PIN_B3);

      bool p1 = BUTTONS_ACTIVE_LOW ? (r1 == LOW) : (r1 == HIGH);
      bool p2 = BUTTONS_ACTIVE_LOW ? (r2 == LOW) : (r2 == HIGH);
      bool p3 = BUTTONS_ACTIVE_LOW ? (r3 == LOW) : (r3 == HIGH);

      if (p1) maskNow |= (1 << 1);
      if (p2) maskNow |= (1 << 2);
      if (p3) maskNow |= (1 << 3);

      Serial.print("[STRUM] raw B1,B2,B3=");
      Serial.print(r1);
      Serial.print(",");
      Serial.print(r2);
      Serial.print(",");
      Serial.print(r3);
      Serial.print(" maskNow=");
      Serial.println(maskNow, BIN);

      // Un solo paquete por rasgueo (la EDU-CIAA detecta strum por umbral en gz)
      sendPacket(maskNow, 500.0f);
      delay(5);
      sendPacket(maskNow, 0.0f);
    }
  }

  delay(1);
}
