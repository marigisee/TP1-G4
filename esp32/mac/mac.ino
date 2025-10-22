#include <WiFi.h>

void printMacBytes(const uint8_t *mac) {
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 16) Serial.print('0');
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();
}

void printMacFromUint64(uint64_t mac64) {
  uint8_t mac[6];
  for (int i = 0; i < 6; ++i) {
    mac[5 - i] = (mac64 >> (8 * i)) & 0xFF;
  }
  printMacBytes(mac);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Diagnóstico MAC (métodos compatibles Arduino Core) ===");

  // 1) WiFi modo estación + WiFi.macAddress()
  WiFi.mode(WIFI_STA);
  delay(200);
  Serial.print("WiFi.macAddress(): ");
  Serial.println(WiFi.macAddress()); // si el WiFi se inicializó bien

  // 2) Forzar inicialización ligera
  WiFi.disconnect(true);
  WiFi.begin(); // no necesita SSID para inicializar el hw
  delay(500);
  Serial.print("Después de WiFi.begin(), WiFi.macAddress(): ");
  Serial.println(WiFi.macAddress());

  // 3) MAC "factory" desde efuse (muy fiable)
  uint64_t mac64 = ESP.getEfuseMac();
  Serial.print("ESP.getEfuseMac() (uint64): 0x");
  Serial.println((unsigned long long)mac64, HEX);
  Serial.print("ESP.getEfuseMac() como MAC: ");
  printMacFromUint64(mac64);

  // 4) MAC del modo AP (si te interesa)
  Serial.print("WiFi.softAPmacAddress(): ");
  Serial.println(WiFi.softAPmacAddress());

  Serial.println("=== fin diagnóstico ===");
}

void loop() {
  // nada
}