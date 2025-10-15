#pragma once
#include "sapi.h"
#include <stdint.h>
#include <stdbool.h>

/* ================== Pines EDU-CIAA ================== */
#define PIN_XCS     GPIO2   // SCI CS
#define PIN_XDCS    GPIO4   // SDI CS
#define PIN_DREQ    GPIO3   // DREQ (IN)
#define PIN_XRESET  GPIO6   // XRESET (OUT)

/* ================== Registros SCI ================== */
#define SCI_MODE       0x00
#define SCI_STATUS     0x01
#define SCI_BASS       0x02
#define SCI_CLOCKF     0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA     0x05
#define SCI_WRAM       0x06
#define SCI_WRAMADDR   0x07
#define SCI_AIADDR     0x0A
#define SCI_VOL        0x0B

/* ================== Flags ================== */
#define SM_SDINEW      (1U << 11)

/* ================== Bus SPI usado ================== */
#define VS_SPI SPI0

/* ================== Init de pines/SPI ================== */
static inline void vs_pinsInit(void){
   gpioInit(PIN_XCS,    GPIO_OUTPUT);
   gpioInit(PIN_XDCS,   GPIO_OUTPUT);
   gpioInit(PIN_XRESET, GPIO_OUTPUT);
   gpioInit(PIN_DREQ,   GPIO_INPUT);

   gpioWrite(PIN_XCS, TRUE);
   gpioWrite(PIN_XDCS, TRUE);
   gpioWrite(PIN_XRESET, TRUE);
}

static inline void vs_spiInit(void){
   /* sAPI v0.6+: spiInit/spiConfig. Por defecto: modo 0, MSB, rate seguro. */
   spiInit(VS_SPI);
}

/* ================== Helpers ================== */
static inline void waitDREQ(void){
   while(!gpioRead(PIN_DREQ)){ delay(1); }
}

static inline void sciWrite(uint8_t reg, uint16_t val){
   uint8_t pkt[4] = { 0x02, reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
   waitDREQ();
   gpioWrite(PIN_XCS, FALSE);
   spiWrite(VS_SPI, pkt, sizeof(pkt));
   gpioWrite(PIN_XCS, TRUE);
   /* En algunos flows conviene esperar DREQ luego de cada SCI */
   waitDREQ();
}

static inline uint16_t sciRead(uint8_t reg){
   uint8_t cmd[2] = { 0x03, reg };
   uint8_t rx[2]  = { 0xFF, 0xFF };

   waitDREQ();
   gpioWrite(PIN_XCS, FALSE);
   spiWrite(VS_SPI, cmd, sizeof(cmd));   // opcode+addr
   spiRead (VS_SPI, rx,  sizeof(rx));    // lee 16 bits
   gpioWrite(PIN_XCS, TRUE);

   return ( ((uint16_t)rx[0]) << 8 ) | rx[1];
}

/* Enviar 2 bytes por SDI (XDCS). Tu flujo usa 0x00 + byte MIDI */
static inline void sdiWrite16(uint8_t hi, uint8_t lo){
   uint8_t b[2] = { hi, lo };
   waitDREQ();
   gpioWrite(PIN_XDCS, FALSE);
   spiWrite(VS_SPI, b, 2);
   gpioWrite(PIN_XDCS, TRUE);
}

/* Reset y soft-init como en tu flujo */
static inline void hardResetVS(void){
   gpioWrite(PIN_XRESET, FALSE); delay(2);
   gpioWrite(PIN_XRESET, TRUE);  delay(2);
   waitDREQ();
}

static inline void softInitVS(void){
   uint16_t m = sciRead(SCI_MODE);
   sciWrite(SCI_MODE, m | SM_SDINEW);
   /* Opcional: subir clock interno si luego acelerás SPI:
      sciWrite(SCI_CLOCKF, 0x9800);  // ~3× cristal típico
      delay(5);
   */
}
