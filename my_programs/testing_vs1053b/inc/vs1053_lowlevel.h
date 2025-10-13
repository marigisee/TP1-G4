#ifndef VS1053_LOWLEVEL_H
#define VS1053_LOWLEVEL_H

#pragma once
#include "board.h"
#include "chip.h"

#include <stdint.h>

void delay(uint32_t ms);

// Pines para la EDU-CIAA 
#define PIN_XCS_PORT     5
#define PIN_XCS_PIN      1
#define PIN_XDCS_PORT    5
#define PIN_XDCS_PIN     2
#define PIN_DREQ_PORT    3
#define PIN_DREQ_PIN     0
#define PIN_XRESET_PORT  3
#define PIN_XRESET_PIN   1

#define VS1053_SPI_SSP LPC_SSP0 

#define SCI_MODE       0x00
#define SCI_STATUS     0x01
#define SCI_CLOCKF     0x03
#define SCI_WRAM       0x06
#define SCI_WRAMADDR   0x07
#define SCI_AIADDR     0x0A
#define SM_SDINEW      0x0800

// SPI velocidad y configuración
#define VS1053_SPI_ID      LPC_SPI
#define VS1053_SPI_CLOCK   1000000  // 1 MHz inicial

static inline void gpioWrite(uint8_t port, uint8_t pin, bool state) {
    Chip_GPIO_SetPinState(LPC_GPIO_PORT, port, pin, state);
}

static inline bool gpioRead(uint8_t port, uint8_t pin) {
    return Chip_GPIO_GetPinState(LPC_GPIO_PORT, port, pin);
}

static inline void waitDREQ(void) {
    while (!gpioRead(PIN_DREQ_PORT, PIN_DREQ_PIN));
}

static inline void spiSend(uint16_t data) {
    Chip_SPI_SendFrame(VS1053_SPI_ID, data);
}

static inline uint16_t spiTransfer(uint16_t data) {
    Chip_SPI_SendFrame(VS1053_SPI_ID, data);
    return Chip_SPI_ReceiveFrame(VS1053_SPI_ID);
}


// --- SCI: comandos de control ---
static inline void sciWrite(uint8_t reg, uint16_t val) {
    waitDREQ();
    gpioWrite(PIN_XCS_PORT, PIN_XCS_PIN, false);

    spiSend(0x02); // Write op
    spiSend(reg);
    spiSend(val >> 8);
    spiSend(val & 0xFF);

    gpioWrite(PIN_XCS_PORT, PIN_XCS_PIN, true);
    waitDREQ();
}

static inline uint16_t sciRead(uint8_t reg) {
    waitDREQ();
    gpioWrite(PIN_XCS_PORT, PIN_XCS_PIN, false);

    spiSend(0x03); // Read op
    spiSend(reg);
    uint8_t hi = spiTransfer(0xFF);
    uint8_t lo = spiTransfer(0xFF);

    gpioWrite(PIN_XCS_PORT, PIN_XCS_PIN, true);
    return (hi << 8) | lo;
}

// --- SDI: datos de audio/MIDI ---
static inline void sdiWrite16(uint8_t hi, uint8_t lo) {
    waitDREQ();
    gpioWrite(PIN_XDCS_PORT, PIN_XDCS_PIN, false);
    spiSend(hi);
    spiSend(lo);
    gpioWrite(PIN_XDCS_PORT, PIN_XDCS_PIN, true);
}

static inline void hardResetVS(void) {
    gpioWrite(PIN_XRESET_PORT, PIN_XRESET_PIN, false);
    delay(2);
    gpioWrite(PIN_XRESET_PORT, PIN_XRESET_PIN, true);
    delay(2);
    waitDREQ();
}

static inline void softInitVS(void) {
    uint16_t mode = sciRead(SCI_MODE);
    sciWrite(SCI_MODE, mode | SM_SDINEW);
    // opcional: sciWrite(SCI_CLOCKF, 0xC000); // para subir velocidad
}

static inline void initVS1053GPIO(void) {
    // Configurar pines como salida/entrada
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PIN_XCS_PORT, PIN_XCS_PIN);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PIN_XDCS_PORT, PIN_XDCS_PIN);
    Chip_GPIO_SetPinDIRInput(LPC_GPIO_PORT, PIN_DREQ_PORT, PIN_DREQ_PIN);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, PIN_XRESET_PORT, PIN_XRESET_PIN);

    gpioWrite(PIN_XCS_PORT, PIN_XCS_PIN, true);
    gpioWrite(PIN_XDCS_PORT, PIN_XDCS_PIN, true);
    gpioWrite(PIN_XRESET_PORT, PIN_XRESET_PIN, true);
}

static inline void initVS1053SPI(void) {
    Chip_SPI_Init(VS1053_SPI_ID);
    Chip_SPI_SetBitRate(VS1053_SPI_ID, VS1053_SPI_CLOCK);
    Chip_SSP_Enable(VS1053_SPI_SSP);
}

#endif 
