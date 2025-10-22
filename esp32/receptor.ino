//Receptor actualizado. La firma del callback estaba mal. Este compilo en el IDE:

#include <WiFi.h>
#include <esp_now.h>

// Estructura de datos recibidos
typedef struct {
    uint8_t buttons;
} struct_message;

struct_message receivedData;

// Callback actualizado según ESP32 core 3.x
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
    memcpy(&receivedData, data, sizeof(receivedData));

    // Si querés seguir usando la MAC, se obtiene así:
    const uint8_t *mac_addr = info->src_addr;

    Serial.print("Datos recibidos de: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac_addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();

    Serial.print("Botones: ");
    Serial.println(receivedData.buttons, BIN);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error al inicializar ESP-NOW");
        return;
    }

    // Registrar el callback con la nueva firma
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}