[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) [![Build Status](https://travis-ci.com/epernia/firmware_v3.svg?branch=master)](https://travis-ci.com/epernia/firmware_v3) (Travis CI status)

# DIPESIM - Dispositivo inalámbrico para ejecución y simulación de instrumentos MIDI 

### Descripción

Se desea diseñar un dispositivo electrónico que permita la ejecución y simulación de instrumentos musicales MIDI de forma inalámbrica. El dispositivo a partir de los pulsadores y sensores correspondientes debe ser capaz de reproducir los acordes o notas que el usuario desee. El dispositivo estará divido en dos bloques: bloque portátil y bloque fijo.

El bloque portátil estará compuesto por un microcontrolador ESP32, una serie de pulsadores y un MPU6050 (acelerómetro y giroscopio) que permitirán al usuario ejecutar las notas o acordes deseados realizando diferentes gestos o movimientos con la mano.

El bloque fijo estará compuesto por un microcontrolador ESP32, la placa EDU-CIAA y el módulo sintetizador MIDI VS1053B. Este bloque recibirá las notas o acordes ejecutados por el bloque portátil y los reproducirá a través del módulo sintetizador MIDI.

---

## Estructura del repositorio (código final del proyecto)

Para esta versión del proyecto, **el código final e integrado** se encuentra en los siguientes directorios:

- **ESP32 (bloque portátil + puente)**
  - `esp32/imu-and-buttons/`
    - `emisora.ino`: lee MPU6050 + botones, detecta rasgueo y envía por ESP-NOW.
    - `receptora.ino`: recibe por ESP-NOW, valida y reenvía por UART a la EDU-CIAA.
    - `README.md`: documentación del flujo completo del sistema.

- **EDU-CIAA + VS1053B (bloque fijo)**
  - `my_programs/imu-and-buttons/`
    - `src/main.c`: recibe UART, sincroniza paquete, valida checksum y dispara el rasgueo MIDI.
    - `src/playing_chords.c`: define cómo se tocan/rasguean los acordes (timing y duración).
    - `src/midi_scheduler.c`: scheduler no bloqueante para NOTE ON/OFF.
    - `src/chords_map.c`: mapeo de botones (4 bits) a acordes.
    - `PROTOCOLOS_Y_MAQUINA_DE_ESTADOS.md`: documentación de FSM + UART/SPI.

Los demás directorios (`test_latency_*`, `testing_*`, `examples/`) contienen **pruebas, prototipos o ejemplos** y no son el camino principal del sistema final.

---

## Componentes y flujo (resumen)

1. **ESP32 Emisora**
   - Lee el IMU (MPU6050) y calcula magnitud.
   - Si detecta rasgueo y el usuario está tocando (STRUM + acorde), arma un `ChordMessage` y lo envía por ESP-NOW.

2. **ESP32 Receptora**
   - Recibe el `ChordMessage` por ESP-NOW.
   - Valida header y checksum.
   - Reenvía el paquete binario por `UART_232` hacia EDU-CIAA.

3. **EDU-CIAA**
   - Reconstruye el paquete en un stream UART usando preámbulo `0xAA 0x55`.
   - Valida checksum.
   - Mapea botones → acorde y usa la `velocity` recibida.
   - Envía mensajes MIDI al VS1053B por SPI/SDI (RT-MIDI).

---

## Cómo usar (alto nivel)

- **Paso 1: ESP32 Receptora**
  - Flashear `esp32/imu-and-buttons/receptora.ino`.
  - Leer por Serial la **MAC** que imprime (para configurarla en la Emisora).

- **Paso 2: ESP32 Emisora**
  - Configurar `receiverAddress[]` con la MAC real de la Receptora.
  - Flashear `esp32/imu-and-buttons/emisora.ino`.

- **Paso 3: EDU-CIAA**
  - Compilar y programar `my_programs/imu-and-buttons/`.
  - Conectar UART entre Receptora ↔ EDU-CIAA (115200) y el VS1053B por SPI.

---

### Miembros del equipo

- Amadori, Franco
- Giacoia, Pedro
- Forden Jones, Ian
- Chavez Sánchez, Máximo Nicolás
- Cuello, Marina Giselle