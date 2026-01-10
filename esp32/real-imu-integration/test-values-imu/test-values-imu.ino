#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// Definición de pines I2C (aunque son los default del ESP32, es buena práctica definirlos)
#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  
  // Inicializamos I2C con los pines específicos
  Wire.begin(SDA_PIN, SCL_PIN);

  // Intentar inicializar el MPU6050
  if (mpu.begin()) {
    Serial.println("Error: No se pudo encontrar el chip MPU6050");
    while (1) {
      delay(10); // Bucle infinito si falla
    }
  }

  Serial.println("MPU6050 Encontrado!");

  // --- CONFIGURACIÓN DE SENSIBILIDAD ---
  // Rango del acelerómetro:
  // MPU6050_RANGE_2_G  -> Movimientos muy sutiles
  // MPU6050_RANGE_4_G  -> Movimientos normales
  // MPU6050_RANGE_8_G  -> Movimientos rápidos/bruscos (Recomendado para tu caso)
  // MPU6050_RANGE_16_G -> Impactos o movimientos extremos
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // Configuración del filtro (Bandwidth)
  // Ayuda a eliminar ruido eléctrico y vibraciones muy finas
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Sistema listo. Abre el Serial Plotter (Ctrl+Shift+L) para ver la grafica.");
  delay(100);
}

void loop() {
  /* Obtener nuevos eventos del sensor con las lecturas actuales */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // a.acceleration.x, .y, .z nos dan los valores en m/s^2
  
  // --- CÁLCULO MATEMÁTICO ---
  // Magnitud del vector aceleración = raiz_cuadrada(x^2 + y^2 + z^2)
  float magnitud = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

  // --- SALIDA POR SERIAL ---
  // Imprimimos solo el valor para que puedas usar el Serial Plotter
  // Formato: "Etiqueta:Valor" (opcional) o solo el valor crudo.
  
  Serial.print("Magnitud:");
  Serial.println(magnitud);

  // Pequeño delay para no saturar el puerto serie, ajusta según necesites más resolución temporal
  delay(50); 
}