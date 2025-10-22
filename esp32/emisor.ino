// Emisor actualizado por lo mismo de antes:

#include <WiFi.h>
#include <string.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ========== CONFIGURACIÓN ==========

// Dirección MAC del receptor (CAMBIAR)
uint8_t receptorMAC[] = {0x24, 0x6F, 0x28, 0x1A, 0xBC, 0xDE};

// Pines de pulsadores principales
#define NUM_BUTTONS 4
int buttonPins[NUM_BUTTONS] = {14, 27, 26, 25};
int buttonStates[NUM_BUTTONS];

// --- (Opcional) 5° pulsador de habilitación tipo “hold” ---
// Cuando está presionado, el sistema lee y envía datos.
// Cuando se suelta, no hace lecturas ni envíos.
////#define ENABLE_PIN 33    // Descomentar y ajustar pin si se usa el pulsador de habilitación

// Rango de aceleración permitido (en m/s²)
float minAccel = -3.0;
float maxAccel = 3.0;

// ===================================

typedef struct
{
    uint8_t buttons;
} struct_message;

struct_message data;
Adafruit_MPU6050 mpu;

// ---------------- CALLBACK ENVÍO ----------------
// Compatible con ESP32 core 3.0.5 y posteriores
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status)
{
    Serial.print("Estado del envío: ");
    if (status == ESP_NOW_SEND_SUCCESS)
        Serial.println("Éxito");
    else
        Serial.println("Fallo");
}

// ---------------- CONFIGURACIÓN ----------------
void setup()
{
    Serial.begin(115200);

    // Inicializar pulsadores
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }

    // --- (Opcional) Inicializar pulsador de habilitación tipo “hold” ---
    ////pinMode(ENABLE_PIN, INPUT_PULLUP);

    // Inicializar I2C primero
    Wire.begin(21, 22, 400000);

    // Inicializar IMU
    if (!mpu.begin())
    {
        Serial.println("No se encontró el MPU6050, revisa las conexiones.");
        while (1)
            delay(10);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 listo!");

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error al inicializar ESP-NOW");
        return;
    }

    // Registrar callback actualizado
    esp_now_register_send_cb(OnDataSent);

    // Configurar el peer (receptor)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receptorMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Error agregando peer");
        return;
    }

    Serial.println("Emisor listo.");
}

// ---------------- FUNCIÓN BOTONES ----------------
byte buttonsToByte()
{
    byte val = 0;
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        if (buttonStates[i])
            val |= (1 << i);
    }
    return val;
}

// ---------------- LOOP PRINCIPAL ----------------
void loop()
{
    // --- (Opcional) Comprobar pulsador de habilitación tipo “hold” ---
    /*
    if (digitalRead(ENABLE_PIN) == HIGH) {  // HIGH = suelto (por INPUT_PULLUP)
        Serial.println("Pulsador de habilitación suelto. En espera...");
        delay(200);
        return;
    }
    */

    // Leer botones principales
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        buttonStates[i] = (digitalRead(buttonPins[i]) == LOW) ? 1 : 0;
    }

    // Leer IMU
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    // Comprobar rango de aceleración
    if (accel.acceleration.x > minAccel && accel.acceleration.x < maxAccel)
    {
        data.buttons = buttonsToByte();
        esp_err_t result = esp_now_send(receptorMAC, (uint8_t *)&data, sizeof(data));

        Serial.print("Enviado: ");
        Serial.print(data.buttons, BIN);
        Serial.print(" (acel X=");
        Serial.print(accel.acceleration.x);
        Serial.println(" m/s²)");
    }
    else
    {
        Serial.print("Aceleración fuera de rango (X=");
        Serial.print(accel.acceleration.x);
        Serial.println(")");
    }

    delay(200);
}