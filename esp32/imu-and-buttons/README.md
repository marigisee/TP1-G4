# Sistema de Guitarra MIDI con IMU y Botones

Este documento explica el flujo completo del sistema, desde que se detecta un rasgueo con el IMU hasta que se genera el sonido MIDI en el módulo VS1053B.

---

## Índice

1. [Arquitectura del Sistema](#arquitectura-del-sistema)
2. [Flujo Completo Paso a Paso](#flujo-completo-paso-a-paso)
3. [Detección de Rasgueo con IMU](#detección-de-rasgueo-con-imu)
4. [Lógica de Envío en la Emisora](#lógica-de-envío-en-la-emisora)
5. [Estructura del Mensaje](#estructura-del-mensaje)
6. [Recepción y Reenvío (Receptora)](#recepción-y-reenvío-receptora)
7. [Procesamiento en EDU-CIAA](#procesamiento-en-edu-ciaa)
8. [Generación de Comandos MIDI](#generación-de-comandos-midi)
9. [Configuración Inicial](#configuración-inicial)

---

## Arquitectura del Sistema

```
┌──────────────────────────────────────┐
│       ESP32 EMISORA                  │
│  ┌────────────┐    ┌──────────────┐ │
│  │ MPU6050    │    │ Botones      │ │
│  │ (IMU)      │    │ B0,B1,B2,B3  │ │
│  │            │    │ + STRUM      │ │
│  └─────┬──────┘    └──────┬───────┘ │
│        │                  │         │
│        └──────┬───────────┘         │
│               ▼                     │
│      ┌─────────────────┐            │
│      │ Lógica de Envío │            │
│      │ (3 condiciones) │            │
│      └────────┬────────┘            │
│               │ ESP-NOW             │
└───────────────┼─────────────────────┘
                │
                ▼
┌───────────────┼─────────────────────┐
│       ESP32 RECEPTORA               │
│               │                     │
│      ┌────────▼────────┐            │
│      │ Validar Header  │            │
│      │ + Checksum      │            │
│      └────────┬────────┘            │
│               │ UART                │
└───────────────┼─────────────────────┘
                │
                ▼
┌───────────────┼─────────────────────┐
│       EDU-CIAA                      │
│               │                     │
│      ┌────────▼────────┐            │
│      │ Sincronización  │            │
│      │ UART (0xAA 0x55)│            │
│      └────────┬────────┘            │
│               │                     │
│      ┌────────▼────────┐            │
│      │ Mapeo Acorde    │            │
│      │ (botones→notas) │            │
│      └────────┬────────┘            │
│               │                     │
│      ┌────────▼────────┐            │
│      │ MIDI Scheduler  │            │
│      │ (rasgueo)       │            │
│      └────────┬────────┘            │
│               │ SPI                 │
└───────────────┼─────────────────────┘
                │
                ▼
         ┌──────────────┐
         │   VS1053B    │
         │ (Sintetizador│
         │    MIDI)     │
         └──────┬───────┘
                │
                ▼
              🔊 SONIDO
```

---

## Flujo Completo Paso a Paso

### 1. Usuario Rasguea la Guitarra

El usuario realiza un movimiento de rasgueo mientras mantiene presionados:
- **Botón STRUM** (GPIO27)
- **Al menos un botón de acorde** (GPIO32, GPIO33, GPIO25, GPIO26)

### 2. IMU Detecta el Movimiento

El MPU6050 mide la aceleración en los 3 ejes (X, Y, Z) y calcula la **magnitud**:

```
magnitud = √(ax² + ay² + az²)
```

### 3. Clasificación del Rasgueo

La emisora compara la magnitud contra umbrales:

| Magnitud | Tipo de Rasgueo | Velocity MIDI |
|----------|-----------------|---------------|
| < 11.0   | No válido       | -             |
| 11.0 - 15.9 | Normal       | 80            |
| ≥ 16.0   | Fuerte          | 120           |

### 4. Verificación de Condiciones

La emisora verifica que se cumplan **las 3 condiciones simultáneamente**:

```
✓ STRUM presionado
✓ Al menos un botón de acorde presionado
✓ Rasgueo válido detectado (magnitud > umbral)
```

Si **todas** se cumplen → envía mensaje por ESP-NOW.

### 5. Transmisión ESP-NOW

La emisora arma un paquete de 8 bytes y lo envía inalámbricamente a la receptora.

### 6. Recepción y Validación

La receptora:
1. Valida el header (`0xAA 0x55`)
2. Valida el checksum (XOR)
3. Reenvía por UART a la EDU-CIAA

### 7. Sincronización UART

La EDU-CIAA busca la secuencia `0xAA 0x55` en el stream de bytes para saber dónde empieza cada paquete.

### 8. Mapeo de Acorde

La EDU-CIAA convierte los 4 botones en un número de 0 a 15 y busca el acorde correspondiente:

```
Ejemplo: botones = [0, 0, 0, 0] → b = 0000 (binario) = 0 → "Nada"
         botones = [1, 0, 0, 0] → b = 0001 (binario) = 1 → "DO mayor"
         botones = [0, 1, 0, 0] → b = 0010 (binario) = 2 → "DO menor"
         botones = [1, 1, 0, 0] → b = 0011 (binario) = 3 → "RE mayor"
```

### 9. Generación de Notas MIDI

La EDU-CIAA genera las 3 notas del acorde (tríada):
- **Mayor**: raíz + 4 semitonos + 7 semitonos
- **Menor**: raíz + 3 semitonos + 7 semitonos

Ejemplo: **DO mayor** (raíz = 60)
- Nota 1: 60 (DO)
- Nota 2: 64 (MI)
- Nota 3: 67 (SOL)

### 10. Rasgueo Escalonado

El MIDI Scheduler agenda las notas con un pequeño delay entre ellas (10ms) para simular el rasgueo:

```
t=0ms   → NOTE ON nota 1 (velocity recibida)
t=10ms  → NOTE ON nota 2 (velocity recibida)
t=20ms  → NOTE ON nota 3 (velocity recibida)
t=180ms → NOTE OFF nota 1
t=190ms → NOTE OFF nota 2
t=200ms → NOTE OFF nota 3
```

### 11. Envío al VS1053B

La EDU-CIAA envía comandos MIDI por SPI al VS1053B:
- `NOTE ON` (canal, nota, velocity)
- `NOTE OFF` (canal, nota, 64)

### 12. Síntesis de Audio

El VS1053B sintetiza el sonido del acorde con el instrumento configurado (Acoustic Guitar, GM #26).

---

## Detección de Rasgueo con IMU

### Cálculo de Magnitud

El IMU MPU6050 entrega aceleración en m/s² en 3 ejes. La magnitud vectorial es:

```cpp
float magnitud = sqrt(sq(a.acceleration.x) + 
                      sq(a.acceleration.y) + 
                      sq(a.acceleration.z));
```

En reposo, la magnitud es ~9.8 m/s² (gravedad). Durante un rasgueo, puede superar 15-20 m/s².

### Anti-Rebote Temporal

Para evitar múltiples detecciones por un solo rasgueo, se usa un **cooldown de 300ms**:

```cpp
if ((now - ultimoRasgueoMs) > TIEMPO_ESPERA) {
    if (magnitud >= UMBRAL_FUERTE) {
        velocityDetectada = VEL_FUERTE;
        rasgueoDetectado = true;
        ultimoRasgueoMs = now;  // Resetear timer
    }
}
```

Esto garantiza que solo se envíe **un mensaje por rasgueo**, aunque el IMU detecte múltiples picos durante el movimiento.

---

## Lógica de Envío en la Emisora

### Las 3 Condiciones

```cpp
// Condición 1: STRUM presionado
bool strumPressed = (digitalRead(PIN_STRUM) == LOW);

// Condición 2: Al menos un botón de acorde presionado
uint8_t chordMask = buildChordMask();
bool hayAcorde = (chordMask != 0);

// Condición 3: Rasgueo válido detectado
bool rasgueoDetectado = (magnitud > umbral && cooldown OK);

// Solo enviar si las 3 se cumplen
if (strumPressed && hayAcorde && rasgueoDetectado) {
    sendPacket(chordMask, velocityDetectada);
}
```

### ¿Por qué estas 3 condiciones?

1. **STRUM presionado**: Evita envíos accidentales por movimientos del IMU cuando no se quiere tocar.
2. **Acorde presionado**: Evita enviar "Nada" (acorde vacío).
3. **Rasgueo válido**: Asegura que el movimiento fue intencional y con suficiente fuerza.

---

## Estructura del Mensaje

### Definición del Struct

```cpp
typedef struct __attribute__((packed)) {
  uint8_t header1;    // 0xAA (sync byte 1)
  uint8_t header2;    // 0x55 (sync byte 2)
  uint8_t botones[4]; // Estado de botones [b0, b1, b2, b3]
  uint8_t velocity;   // Velocity MIDI (80 o 120)
  uint8_t checksum;   // XOR de bytes [2..6]
} ChordMessage;
```

### Layout en Memoria (8 bytes)

```
Offset  Campo       Valor
──────  ──────────  ─────────────
0       header1     0xAA (fijo)
1       header2     0x55 (fijo)
2       botones[0]  0 o 1
3       botones[1]  0 o 1
4       botones[2]  0 o 1
5       botones[3]  0 o 1
6       velocity    80 o 120
7       checksum    XOR(bytes 2-6)
──────  ──────────  ─────────────
Total: 8 bytes
```

### Cálculo del Checksum

```cpp
uint8_t chk = 0;
for (int i = 2; i < 7; i++) {  // Bytes 2-6 (excluye headers y checksum)
    chk ^= p[i];
}
msg.checksum = chk;
```

El checksum permite detectar si el paquete se corrompió durante la transmisión.

---

## Recepción y Reenvío (Receptora)

### Callback ESP-NOW

```cpp
void OnDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *data, int len)
{
    if (len != sizeof(ChordMessage)) return;  // Tamaño incorrecto
    memcpy((void*)&lastMsg, data, sizeof(lastMsg));
    hasMsg = true;
}
```

### Validación y Reenvío

```cpp
// 1. Validar header
if (msg.header1 != 0xAA || msg.header2 != 0x55) {
    return;  // Paquete inválido
}

// 2. Validar checksum
uint8_t chk = 0;
for (int i = 2; i < 7; i++) chk ^= p[i];
if (chk != msg.checksum) {
    return;  // Paquete corrupto
}

// 3. Reenviar por UART
CIAA.write((uint8_t*)&msg, sizeof(msg));
delayMicroseconds(1000);  // Pequeño delay para evitar overflow
```

---

## Procesamiento en EDU-CIAA

### Sincronización UART

El problema: UART es un **stream de bytes** sin delimitadores. Si la EDU-CIAA arranca a mitad de un paquete, todo se desalinea.

**Solución**: Buscar la secuencia única `0xAA 0x55` antes de leer el resto del paquete.

```cpp
// Estado: buscando sincronización
if (!synced) {
    if (!gotAA) {
        gotAA = (byte == 0xAA);
        continue;
    }
    // Ya tenemos 0xAA, esperamos 0x55
    if (byte == 0x55) {
        synced = true;  // ¡Sincronizado!
        idx = 0;
        p[idx++] = 0xAA;
        p[idx++] = 0x55;
    } else {
        gotAA = (byte == 0xAA);  // Reintentar
    }
    continue;
}

// Estado: sincronizado, leer resto del paquete
p[idx++] = byte;
if (idx >= sizeof(ChordMessage)) {
    // Paquete completo, validar checksum y procesar
}
```

### Drenado del Buffer UART

Para evitar que el buffer se llene y se pierdan bytes:

```cpp
while (uartReadByte(UART_232, &byte)) {
    // Procesar todos los bytes disponibles en cada iteración
}
```

---

## Generación de Comandos MIDI

### Mapeo Botones → Acorde

```cpp
uint8_t b = obtenerValorBotones(msg.botones);
// b es un número de 0 a 15 (4 bits)

const ButtonChord *chord = getChordFromButtons(b);
// Busca en la tabla buttonChordMap[16]
```

### Construcción de Tríada

```cpp
if (chord->tipo == CHORD_MAJOR) {
    notes[0] = chord->root + 0;  // Raíz
    notes[1] = chord->root + 4;  // Tercera mayor
    notes[2] = chord->root + 7;  // Quinta justa
}
else if (chord->tipo == CHORD_MINOR) {
    notes[0] = chord->root + 0;  // Raíz
    notes[1] = chord->root + 3;  // Tercera menor
    notes[2] = chord->root + 7;  // Quinta justa
}
```

### Rasgueo con MIDI Scheduler

```cpp
void strumChord(const ButtonChord *chord, uint8_t velocity, uint32_t now_ms)
{
    uint8_t notes[3];
    buildTriad(chord, notes, &n);

    for (uint8_t i = 0; i < 3; i++) {
        uint32_t t_on  = now_ms + i * 10;      // 0ms, 10ms, 20ms
        uint32_t t_off = t_on + 180;           // Duración 180ms

        midiSchedNoteOn (t_on,  MIDI_CH, notes[i], velocity);
        midiSchedNoteOff(t_off, MIDI_CH, notes[i], 64);
    }
}
```

### Procesamiento del Scheduler

En cada iteración del loop principal:

```cpp
void midiSchedProcess(uint32_t now_ms) {
    for (int i = 0; i < QSIZE; i++) {
        if (!q[i].used) continue;

        // ¿Ya es hora de ejecutar este evento?
        if ((int32_t)(now_ms - q[i].t) >= 0) {
            if (q[i].type == EV_ON)
                midiNoteOn(q[i].ch, q[i].note, q[i].vel);
            else
                midiNoteOff(q[i].ch, q[i].note, 64);
            
            q[i].used = 0;  // Liberar slot
        }
    }
}
```

### Envío SPI al VS1053B

```cpp
void midiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Comando MIDI: 0x90 + canal, nota, velocity
    vs_write_sdi(0x90 | channel);
    vs_write_sdi(note);
    vs_write_sdi(velocity);
}
```

---

## Configuración Inicial

### 1. Hardware

- **ESP32 Emisora**: Conectar MPU6050 en I2C (SDA=21, SCL=22)
- **Botones**: GPIO32, GPIO33, GPIO25, GPIO26 (acordes) + GPIO27 (STRUM)
- **ESP32 Receptora**: UART TX=17 → RX EDU-CIAA, GND común
- **EDU-CIAA**: UART_232 a 115200 baud, SPI al VS1053B

### 2. Software

1. **Obtener MAC de la receptora**: Al iniciar, imprime su MAC en Serial
2. **Configurar emisora**: Cambiar `receiverAddress[]` con la MAC real
3. **Compilar y cargar**:
   - `emisora.ino` → ESP32 Emisora
   - `receptora.ino` → ESP32 Receptora
   - `my_programs/imu-and-buttons` → EDU-CIAA

### 3. Prueba

1. Mantener presionado un botón de acorde (ej: GPIO32)
2. Mantener presionado STRUM (GPIO27)
3. Rasguear (mover el IMU con fuerza)
4. Verificar que suena el acorde correspondiente

---

## Diagrama de Tiempo

```
t=0ms    Usuario rasguea
         │
         ▼
t=5ms    IMU detecta magnitud > 16.0 → RASGUEO FUERTE
         │
         ▼
t=6ms    Emisora verifica 3 condiciones → OK
         │
         ▼
t=7ms    Envío ESP-NOW → Receptora
         │
         ▼
t=10ms   Receptora valida y reenvía UART → EDU-CIAA
         │
         ▼
t=12ms   EDU-CIAA sincroniza, valida checksum, mapea acorde
         │
         ▼
t=13ms   Scheduler agenda:
         - NOTE ON nota1 @ t=13ms
         - NOTE ON nota2 @ t=23ms
         - NOTE ON nota3 @ t=33ms
         - NOTE OFF nota1 @ t=193ms
         - NOTE OFF nota2 @ t=203ms
         - NOTE OFF nota3 @ t=213ms
         │
         ▼
t=13ms   VS1053B recibe NOTE ON nota1 → 🔊 Empieza sonido
t=23ms   VS1053B recibe NOTE ON nota2 → 🔊
t=33ms   VS1053B recibe NOTE ON nota3 → 🔊
         │
         ▼
t=193ms  VS1053B recibe NOTE OFF nota1 → 🔇
t=203ms  VS1053B recibe NOTE OFF nota2 → 🔇
t=213ms  VS1053B recibe NOTE OFF nota3 → 🔇
```

**Latencia total**: ~13ms desde el rasgueo hasta el primer sonido.

---

## Resumen de Conceptos Clave

| Concepto | Explicación |
|----------|-------------|
| **Magnitud vectorial** | √(x²+y²+z²) - Intensidad del movimiento sin importar dirección |
| **Cooldown** | Tiempo mínimo entre detecciones para evitar duplicados |
| **Framing UART** | Preámbulo 0xAA 0x55 para saber dónde empieza cada paquete |
| **Checksum** | XOR de bytes para detectar corrupción |
| **Tríada** | Acorde de 3 notas (raíz + tercera + quinta) |
| **Scheduler** | Cola de eventos diferidos para ejecutar NOTE ON/OFF a tiempo |
| **Rasgueo escalonado** | Delay de 10ms entre notas para simular rasgueo real |

---

*Documento generado para el proyecto de guitarra MIDI con IMU MPU6050*
