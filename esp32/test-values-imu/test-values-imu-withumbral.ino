#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

#define SDA_PIN 21
#define SCL_PIN 22

// --- CONFIGURACIÓN DE UMBRALES ---
const float UMBRAL_NORMAL = 11.0; // Mínimo para considerar rasgueo
const float UMBRAL_FUERTE = 16.0; // Mínimo para considerar rasgueo fuerte
const int TIEMPO_ESPERA = 300;    // Milisegundos entre detecciones (anti-rebote)

unsigned long ultimo_tiempo_deteccion = 0; 

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (mpu.begin()) {
    Serial.println("Error: No se pudo encontrar el chip MPU6050");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Encontrado!");

  // Configuración de rango a 8G para aguantar los rasgueos fuertes sin saturar
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Sistema listo. Esperando rasgueos...");
  delay(100);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // 1. Cálculo de Magnitud
  float magnitud = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

  // 2. Salida para el Serial Plotter (para ver la curva)
  Serial.print("Magnitud:");
  Serial.print(magnitud);
  
  // Dibujamos las líneas de referencia en el gráfico
  Serial.print(",Ref_Normal:");
  Serial.print(UMBRAL_NORMAL);
  Serial.print(",Ref_Fuerte:");
  Serial.println(UMBRAL_FUERTE);

  // 3. Lógica de Detección
  // Solo entramos si ha pasado el tiempo de espera desde el último rasgueo
  if (millis() - ultimo_tiempo_deteccion > TIEMPO_ESPERA) {
    
    // CASO 1: Rasgueo Fuerte (>= 16)
    if (magnitud >= UMBRAL_FUERTE) {
      Serial.println("!!! RASGUEO FUERTE DETECTADO !!!");
      ultimo_tiempo_deteccion = millis(); // Reseteamos el reloj
    }
    // CASO 2: Rasgueo Normal (> 11 y < 16)
    // Usamos 'else if' para que si ya entró en fuerte, no entre aquí
    else if (magnitud > UMBRAL_NORMAL) {
      Serial.println("!!! RASGUEO NORMAL DETECTADO !!!");
      ultimo_tiempo_deteccion = millis(); // Reseteamos el reloj
    }
  }

  delay(200); 
}