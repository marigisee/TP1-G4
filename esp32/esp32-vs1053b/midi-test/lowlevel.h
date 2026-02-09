#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

/* ================== Pines ESP32 (ajustá si hace falta) ================== */
#ifndef PIN_XCS
  #define PIN_XCS     5    // SCI CS
#endif
#ifndef PIN_XDCS
  #define PIN_XDCS    16   // SDI CS
#endif
#ifndef PIN_DREQ
  #define PIN_DREQ    4    // DREQ (IN)
#endif
#ifndef PIN_XRESET
  #define PIN_XRESET  17   // XRESET (OUT)
#endif

#ifndef PIN_SCK
  #define PIN_SCK     18
#endif
#ifndef PIN_MISO
  #define PIN_MISO    19
#endif
#ifndef PIN_MOSI
  #define PIN_MOSI    23
#endif

/* ================== Registros SCI ================== */
#define SCI_MODE        0x00
#define SCI_STATUS      0x01
#define SCI_BASS        0x02
#define SCI_CLOCKF      0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA      0x05
#define SCI_WRAM        0x06
#define SCI_WRAMADDR    0x07
#define SCI_AIADDR      0x0A
#define SCI_VOL         0x0B

/* ================== Flags ================== */
#define SM_SDINEW      (1U << 11)
// (opcional) #define SM_TEST    (1U << 5)

/* ================== SPI instance ================== */
static SPIClass& VS_SPI = SPI;   // usamos el SPI global (VSPI típicamente)

/* Velocidades: SCI suele ser más lento; SDI puede ser más rápido.
   Para pruebas, mantené ambos conservadores. */
static SPISettings VS_SCI_SETTINGS(1000000, MSBFIRST, SPI_MODE0); // 1 MHz
static SPISettings VS_SDI_SETTINGS(2000000, MSBFIRST, SPI_MODE0); // 2 MHz

/* ================== Init de pines/SPI ================== */
static inline void vs_pinsInit(void){
  pinMode(PIN_XCS, OUTPUT);
  pinMode(PIN_XDCS, OUTPUT);
  pinMode(PIN_XRESET, OUTPUT);
  pinMode(PIN_DREQ, INPUT);

  digitalWrite(PIN_XCS, HIGH);
  digitalWrite(PIN_XDCS, HIGH);
  digitalWrite(PIN_XRESET, HIGH);
}

static inline void vs_spiInit(void){
  VS_SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI); // SS no hace falta pasarlo
}

/* ================== Helpers ================== */
static inline void waitDREQ(void){
  while(!digitalRead(PIN_DREQ)) { delayMicroseconds(50); }
}

/* ================== SCI ================== */
static inline void sciWrite(uint8_t reg, uint16_t val){
  waitDREQ();
  VS_SPI.beginTransaction(VS_SCI_SETTINGS);
  digitalWrite(PIN_XCS, LOW);
  VS_SPI.transfer(0x02);      // write opcode
  VS_SPI.transfer(reg);
  VS_SPI.transfer((uint8_t)(val >> 8));
  VS_SPI.transfer((uint8_t)(val & 0xFF));
  digitalWrite(PIN_XCS, HIGH);
  VS_SPI.endTransaction();
  waitDREQ(); // recomendado
}

static inline uint16_t sciRead(uint8_t reg){
  waitDREQ();
  VS_SPI.beginTransaction(VS_SCI_SETTINGS);
  digitalWrite(PIN_XCS, LOW);
  VS_SPI.transfer(0x03); // read opcode
  VS_SPI.transfer(reg);
  uint8_t hi = VS_SPI.transfer(0xFF);
  uint8_t lo = VS_SPI.transfer(0xFF);
  digitalWrite(PIN_XCS, HIGH);
  VS_SPI.endTransaction();
  return ((uint16_t)hi << 8) | lo;
}

/* ================== SDI ================== */
/* Enviar 2 bytes por SDI */
static inline void sdiWrite16(uint8_t hi, uint8_t lo){
  waitDREQ();
  VS_SPI.beginTransaction(VS_SDI_SETTINGS);
  digitalWrite(PIN_XDCS, LOW);
  VS_SPI.transfer(hi);
  VS_SPI.transfer(lo);
  digitalWrite(PIN_XDCS, HIGH);
  VS_SPI.endTransaction();
}

/* Enviar N bytes por SDI (útil para MIDI “crudo” y sine test) */
static inline void sdiWrite(const uint8_t* data, size_t len){
  size_t i = 0;
  while (i < len){
    waitDREQ();
    VS_SPI.beginTransaction(VS_SDI_SETTINGS);
    digitalWrite(PIN_XDCS, LOW);

    // chunk chico para no pasarnos: 32 bytes suele ser seguro
    size_t chunk = min((size_t)32, len - i);
    for(size_t k = 0; k < chunk; k++){
      VS_SPI.transfer(data[i + k]);
    }

    digitalWrite(PIN_XDCS, HIGH);
    VS_SPI.endTransaction();
    i += chunk;
  }
}

/* ================== Reset / init ================== */
static inline void hardResetVS(void){
  digitalWrite(PIN_XRESET, LOW);
  delay(2);
  digitalWrite(PIN_XRESET, HIGH);
  delay(2);
  waitDREQ();
}

static inline void softInitVS(void){
  uint16_t m = sciRead(SCI_MODE);
  sciWrite(SCI_MODE, m | SM_SDINEW);

  // (opcional) subir clock interno si después querés subir SPI:
  // sciWrite(SCI_CLOCKF, 0x9800);
  // delay(5);
}
