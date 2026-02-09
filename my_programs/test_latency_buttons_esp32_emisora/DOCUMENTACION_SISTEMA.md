# Sistema de Comunicación IMU/Botones → MIDI

Este documento explica el funcionamiento completo del sistema que permite enviar comandos desde una ESP32 emisora hacia una EDU-CIAA para generar sonidos MIDI, pasando por una ESP32 receptora como puente.

---

## Índice

1. [Arquitectura General](#arquitectura-general)
2. [Flujo de Datos](#flujo-de-datos)
3. [Estructura del Paquete](#estructura-del-paquete)
4. [Por qué se necesita Framing en UART](#por-qué-se-necesita-framing-en-uart)
5. [Problemas Encontrados y Soluciones](#problemas-encontrados-y-soluciones)
6. [Lógica del MIDI Scheduler](#lógica-del-midi-scheduler)
7. [Resumen de Archivos](#resumen-de-archivos)

---

## Arquitectura General

```
┌─────────────────┐      ESP-NOW       ┌─────────────────┐      UART 115200     ┌─────────────────┐
│  ESP32 EMISORA  │ ─────────────────► │ ESP32 RECEPTORA │ ───────────────────► │    EDU-CIAA     │
│                 │                    │                 │                      │                 │
│  - Botones B1-B3│                    │  - Recibe       │                      │  - Procesa msg  │
│  - Botón STRUM  │                    │  - Valida       │                      │  - Detecta strum│
│  - (IMU mock)   │                    │  - Reenvía UART │                      │  - Genera MIDI  │
└─────────────────┘                    └─────────────────┘                      └─────────────────┘
```

### Componentes

| Componente | Función |
|------------|---------|
| **ESP32 Emisora** | Lee botones de acorde (B1, B2, B3) y botón de rasgueo (STRUM). Envía paquetes por ESP-NOW. |
| **ESP32 Receptora** | Recibe paquetes ESP-NOW, valida integridad, reenvía por UART a la EDU-CIAA. |
| **EDU-CIAA** | Recibe paquetes UART, sincroniza, valida checksum, detecta evento de strum y genera comandos MIDI. |

---

## Flujo de Datos

### 1. Detección de STRUM (Emisora)

```
Usuario presiona botón STRUM (GPIO 14)
         │
         ▼
┌─────────────────────────────────┐
│ Detectar flanco HIGH → LOW     │
│ (con debounce de 30ms)         │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Leer estado ACTUAL de B1,B2,B3 │
│ (digitalRead instantáneo)      │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Armar máscara de acorde        │
│ maskNow = bits según botones   │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Enviar 2 paquetes:             │
│   1) gz=500 (pico de strum)    │
│   2) gz=0   (vuelta a reposo)  │
└─────────────────────────────────┘
```

### 2. Recepción y Reenvío (Receptora)

```
Callback ESP-NOW recibe paquete
         │
         ▼
┌─────────────────────────────────┐
│ Validar tamaño == sizeof(msg)  │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Validar header1=0xAA, header2=0x55 │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Validar checksum (XOR)         │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ CIAA.write() → enviar por UART │
│ delayMicroseconds(1000)        │
└─────────────────────────────────┘
```

### 3. Procesamiento (EDU-CIAA)

```
while(uartReadByte(...))  ← Drenar todos los bytes disponibles
         │
         ▼
┌─────────────────────────────────┐
│ Buscar secuencia 0xAA 0x55     │
│ (sincronización)               │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Llenar buffer hasta completar  │
│ sizeof(IMUMessage) bytes       │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Validar checksum               │
│ Si falla → descartar, re-sync  │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Extraer acorde de botones[]    │
│ Detectar strum por umbral gz   │
└─────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────┐
│ Si strum detectado:            │
│   → strumChord() genera MIDI   │
└─────────────────────────────────┘
```

---

## Estructura del Paquete

### Definición del struct `IMUMessage`

```c
typedef struct __attribute__((packed))
{
   uint8_t header1;    // 0xAA (sync byte 1)
   uint8_t header2;    // 0x55 (sync byte 2)
   float ax, ay, az;   // Acelerómetro (mock: 0, 0, 9.81)
   float gx, gy, gz;   // Giroscopio (gz usado para detectar strum)
   uint8_t botones[4]; // Estado de botones [b0, b1, b2, b3]
   uint8_t checksum;   // XOR de bytes [2..N-2]
} IMUMessage;
```

### Layout en memoria (31 bytes total)

```
Offset  Tamaño  Campo
──────  ──────  ─────────────
0       1       header1 (0xAA)
1       1       header2 (0x55)
2       4       ax (float)
6       4       ay (float)
10      4       az (float)
14      4       gx (float)
18      4       gy (float)
22      4       gz (float)
26      1       botones[0]
27      1       botones[1]
28      1       botones[2]
29      1       botones[3]
30      1       checksum
──────  ──────  ─────────────
Total: 31 bytes
```

### Cálculo del Checksum

```c
uint8_t chk = 0;
for (int i = 2; i < sizeof(msg) - 1; i++) {
    chk ^= p[i];  // XOR de bytes desde offset 2 hasta N-2
}
msg.checksum = chk;
```

Se excluyen:
- `header1` y `header2` (bytes 0 y 1)
- `checksum` (último byte)

---

## Por qué se necesita Framing en UART

### El problema fundamental

UART es un protocolo de **stream de bytes**. No tiene concepto de "paquetes" ni delimitadores inherentes. Si enviás un struct binario crudo:

```
[ax][ay][az][gx][gy][gz][b0][b1][b2][b3]
```

El receptor **no sabe dónde empieza cada paquete**. Si se pierde un byte o el receptor arranca "a mitad" de un paquete, la alineación se pierde y todo se interpreta mal.

### Ejemplo de desalineación

```
Emisor envía:     [AA][55][ax...][az...][gz...][b0][b1][b2][b3][CK]
                   ↑ inicio real

Receptor arranca aquí (perdió 3 bytes):
                         [ax...][az...][gz...][b0][b1][b2][b3][CK][AA][55]...
                         ↑ cree que esto es header

Resultado: interpreta bytes de float como botones → datos basura
```

### Solución: Framing con Preámbulo + Checksum

1. **Preámbulo de 2 bytes** (`0xAA 0x55`): Secuencia única que marca inicio de paquete.
2. **Tamaño fijo**: El receptor sabe exactamente cuántos bytes leer después del preámbulo.
3. **Checksum**: Permite detectar si el paquete está corrupto o desalineado.

### Por qué 2 bytes de preámbulo (no 1)

Con un solo byte (`0xAA`), ese valor puede aparecer **dentro del payload** (por ejemplo, como parte de un `float`). Esto causa "falsos sincronismos": el receptor cree que encontró el inicio de un paquete pero en realidad está en el medio de otro.

Con 2 bytes (`0xAA 0x55`), la probabilidad de que esa secuencia exacta aparezca accidentalmente en los datos es mucho menor (1/65536 vs 1/256).

---

## Problemas Encontrados y Soluciones

### Problema 1: Botones siempre en `[0 0 0 0]`

**Síntoma**: Al rasguear, el acorde siempre llegaba como 0.

**Causa**: El código usaba `lastStable[]` (estado con debounce) para armar la máscara. Si el usuario apretaba acorde y rasgueaba rápido, el debounce no había terminado y `lastStable[]` seguía en 0.

**Solución**: En el instante del STRUM, leer los pines **directamente** con `digitalRead()`:

```c
int r1 = digitalRead(PIN_B1);
int r2 = digitalRead(PIN_B2);
int r3 = digitalRead(PIN_B3);
// Armar maskNow con estos valores
```

---

### Problema 2: Checksum inválido intermitente

**Síntoma**: Aparecían errores de checksum aunque el protocolo parecía correcto.

**Causa 1 - Falsos sync**: Con header de 1 byte, el receptor se "enganchaba" en un `0xAA` dentro del payload.

**Solución**: Usar preámbulo de 2 bytes (`0xAA 0x55`).

**Causa 2 - Overflow de buffer**: La EDU-CIAA leía 1 byte por iteración (`if(uartReadByte...)`). Mientras procesaba (prints, MIDI), el UART seguía recibiendo y el buffer se llenaba → pérdida de bytes.

**Solución**: Cambiar a `while(uartReadByte...)` para drenar todos los bytes disponibles en cada iteración.

---

### Problema 3: Solo sonaba el primer strum

**Síntoma**: El primer rasgueo generaba sonido, los siguientes no.

**Causa**: La función `detectStrum()` usa detección por **flanco** (edge detection). Detecta cuando `gz` cruza el umbral de abajo hacia arriba. Si solo se enviaba `gz=500` sin volver a 0, el estado interno `above` quedaba en `true` y no había más flancos.

**Solución**: Enviar un "pulso" de strum:

```c
sendPacket(maskNow, 500.0f);  // Pico
delay(5);
sendPacket(maskNow, 0.0f);    // Vuelta a reposo
```

---

### Problema 4: Envío innecesario al cambiar acorde

**Síntoma**: Se enviaban mensajes cada vez que se presionaba un botón de acorde, aunque no se rasgueara.

**Causa**: El código original enviaba en dos situaciones: cambio de acorde Y strum.

**Solución**: Eliminar el envío al cambiar acorde. Solo enviar cuando se detecta el flanco del botón STRUM.

---

## Lógica del MIDI Scheduler

El archivo `midi_scheduler.c` implementa una **cola de eventos MIDI diferidos** (scheduler). Esto permite agendar notas para que suenen o se apaguen en un momento futuro.

### Estructura de un evento

```c
typedef enum { EV_ON, EV_OFF } EvType;

typedef struct {
  uint32_t t;       // Timestamp (ms) en que debe ejecutarse
  EvType type;      // NOTE_ON o NOTE_OFF
  uint8_t ch;       // Canal MIDI (0-15)
  uint8_t note;     // Nota MIDI (0-127)
  uint8_t vel;      // Velocity (0-127)
  uint8_t used;     // 1 = slot ocupado, 0 = libre
} MidiEv;
```

### Cola circular de eventos

```c
#define QSIZE 32
static MidiEv q[QSIZE];
```

Se usa un array estático de 32 slots. Cada slot puede estar libre (`used=0`) u ocupado (`used=1`).

### Funciones principales

#### `midiSchedInit()`
Inicializa la cola marcando todos los slots como libres.

```c
void midiSchedInit(void) {
  for(int i=0; i<QSIZE; i++) q[i].used = 0;
}
```

#### `midiSchedNoteOn(t_ms, ch, note, vel)`
Agenda un NOTE_ON para el tiempo `t_ms`.

```c
bool midiSchedNoteOn(uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel) {
  int k = allocSlot();
  if(k < 0) return false;  // Cola llena
  q[k] = (MidiEv){ .t=t_ms, .type=EV_ON, .ch=ch, .note=note, .vel=vel, .used=1 };
  return true;
}
```

#### `midiSchedNoteOff(t_ms, ch, note, vel)`
Agenda un NOTE_OFF para el tiempo `t_ms`.

```c
bool midiSchedNoteOff(uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel) {
  int k = allocSlot();
  if(k < 0) return false;
  q[k] = (MidiEv){ .t=t_ms, .type=EV_OFF, .ch=ch, .note=note, .vel=vel, .used=1 };
  return true;
}
```

#### `midiSchedProcess(now_ms)`
**Debe llamarse en cada iteración del loop principal.** Revisa todos los eventos pendientes y ejecuta los que ya "vencieron".

```c
void midiSchedProcess(uint32_t now_ms) {
  for(int i=0; i<QSIZE; i++) {
    if(!q[i].used) continue;

    // Comparación wrap-safe
    if((int32_t)(now_ms - q[i].t) >= 0) {
      if(q[i].type == EV_ON)
        midiNoteOn(q[i].ch, q[i].note, q[i].vel);
      else
        midiNoteOff(q[i].ch, q[i].note, q[i].vel);
      
      q[i].used = 0;  // Liberar slot
    }
  }
}
```

### Diagrama de flujo del scheduler

```
strumChord() llamado
       │
       ▼
┌──────────────────────────────┐
│ Para cada nota del acorde:   │
│   midiSchedNoteOn(now, ...)  │  ← Suena inmediatamente
│   midiSchedNoteOff(now+300)  │  ← Se apagará en 300ms
└──────────────────────────────┘
       │
       ▼
   (loop principal)
       │
       ▼
┌──────────────────────────────┐
│ midiSchedProcess(now)        │
│   - Revisa cola              │
│   - Ejecuta eventos vencidos │
│   - Libera slots             │
└──────────────────────────────┘
```

### Por qué usar un scheduler

Sin scheduler, tendrías que hacer `delay()` para esperar antes de apagar las notas. Eso **bloquea** todo el programa y no podrías:
- Leer nuevos paquetes UART
- Detectar nuevos strums
- Procesar otros eventos

Con el scheduler, el programa sigue corriendo normalmente y las notas se apagan "solas" cuando llega su momento.

---

## Resumen de Archivos

| Archivo | Ubicación | Función |
|---------|-----------|---------|
| `emisor.ino` | `esp32/test_latency_buttons_esp32_emisora/` | Lee botones, detecta STRUM, envía por ESP-NOW |
| `receptor.ino` | `esp32/test_latency_buttons_esp32_emisora/` | Recibe ESP-NOW, valida, reenvía por UART |
| `main.c` | `my_programs/test_latency_buttons_esp32_emisora/src/` | Recibe UART, sincroniza, detecta strum, genera MIDI |
| `midi_scheduler.c` | `my_programs/test_latency_buttons_esp32_emisora/src/` | Cola de eventos MIDI diferidos |
| `chords_map.c` | `my_programs/test_latency_buttons_esp32_emisora/src/` | Mapeo botones → acordes |
| `playing_chords.c` | `my_programs/test_latency_buttons_esp32_emisora/src/` | Lógica de strumChord() |

---

## Checklist para Reproducir

1. **Hardware**
   - [ ] ESP32 emisora con botones en GPIO 14 (STRUM), 27 (B1), 26 (B2), 25 (B3)
   - [ ] ESP32 receptora conectada por UART a EDU-CIAA
   - [ ] GND común entre ESP32 receptora y EDU-CIAA
   - [ ] Verificar si UART_232 es TTL o RS-232 (si es RS-232, usar MAX3232)

2. **Configuración**
   - [ ] Cambiar `receiverAddress[]` en emisor.ino por la MAC de tu receptora
   - [ ] UART a 115200 baud en ambos extremos

3. **Compilar y cargar**
   - [ ] Flashear emisor.ino en ESP32 emisora
   - [ ] Flashear receptor.ino en ESP32 receptora
   - [ ] Compilar y cargar programa en EDU-CIAA

4. **Probar**
   - [ ] Mantener botón de acorde presionado
   - [ ] Presionar botón STRUM
   - [ ] Verificar que suena el acorde correspondiente
   - [ ] Verificar que se puede rasguear múltiples veces

---

*Documento generado para el proyecto de guitarra MIDI con EDU-CIAA*
