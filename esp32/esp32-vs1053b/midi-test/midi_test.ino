#include <Arduino.h>
#include <SPI.h>

#include "lowlevel.h"
#include "rtmidi1053b_tables.h"

// Pines (según tu setup)
#define PIN_XCS     5
#define PIN_XDCS    16
#define PIN_DREQ    4
#define PIN_XRESET  17

#define PIN_SCK     18
#define PIN_MISO    19
#define PIN_MOSI    23



static inline void loadUserCodeFromTables() {
  const size_t N = sizeof(dtab) / sizeof(dtab[0]);
  for (size_t i = 0; i < N; ++i) {
    uint8_t  reg  = atab[i];
    uint16_t data = dtab[i];
    waitDREQ();
    sciWrite(reg, data);
  }
}

// Entry point RT-MIDI (vos lo tenés como 0x0050)
static inline void startRTMIDI() {
  waitDREQ();
  sciWrite(SCI_AIADDR, 0x0050);
  waitDREQ();
}

// Enviar bytes MIDI crudos por SDI (XDCS) respetando DREQ
// Recomendación: enviar en paquetes chicos (ej 32 bytes o menos)
static inline void midiSendBytes(const uint8_t* data, size_t len) {
  size_t i = 0;
  while (i < len) {
    waitDREQ();
    // mandamos de a pocos para no pasarnos del buffer interno
    size_t chunk = min((size_t)32, len - i);
    sdiWrite((uint8_t*)(data + i), chunk);
    i += chunk;
  }
}

// Helpers MIDI mínimos (canal 0 = 0)
static inline void midiProgramChange(uint8_t channel, uint8_t program) {
  uint8_t msg[2] = { (uint8_t)(0xC0 | (channel & 0x0F)), program };
  midiSendBytes(msg, sizeof(msg));
}

static inline void midiNoteOn(uint8_t channel, uint8_t note, uint8_t vel) {
  uint8_t msg[3] = { (uint8_t)(0x90 | (channel & 0x0F)), note, vel };
  midiSendBytes(msg, sizeof(msg));
}

static inline void midiNoteOff(uint8_t channel, uint8_t note, uint8_t vel=0) {
  uint8_t msg[3] = { (uint8_t)(0x80 | (channel & 0x0F)), note, vel };
  midiSendBytes(msg, sizeof(msg));
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // SPI del ESP32
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

  // Si tu lowlevel usa #defines internos, ok.
  // Si tu lowlevel tiene init de pines, llamalo acá (si existe).
  // vs1053_begin_pins(PIN_XCS, PIN_XDCS, PIN_DREQ, PIN_XRESET);

  Serial.println("Reset + init VS1053...");
  hardResetVS();
  softInitVS();

  // Volumen (0x0000 fuerte, 0xFEFE mute aprox). Ajustá a gusto.
  sciWrite(SCI_VOL, 0x2020);

  Serial.println("Cargando plugin RT-MIDI...");
  loadUserCodeFromTables();

  Serial.println("Saltando a entry point RT-MIDI (SCI_AIADDR=0x0050)...");
  startRTMIDI();

  Serial.println("Probando MIDI: Program Change + escala...");

  // Piano acústico (GM: 0 = Acoustic Grand Piano, en muchos casos)
  midiProgramChange(0, 0);

  // Do mayor: C4(60) D(62) E(64) F(65) G(67) A(69) B(71) C5(72)
  uint8_t notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
  for (uint8_t i = 0; i < sizeof(notes); i++) {
    midiNoteOn(0, notes[i], 100);
    delay(250);
    midiNoteOff(0, notes[i], 0);
    delay(40);
  }

  Serial.println("Listo. Si escuchaste notas, RT-MIDI OK.");
}

void loop() {}
