#ifndef VS1053B_SAPI_H
#define VS1053B_SAPI_H
#pragma once

#include "sapi.h"
#include <stdint.h>
#include <stdbool.h>

/* === Pines sAPI ===
   XCS→GPIO2 (CON2_31)
   XDCS→GPIO4 (CON2_33)
   DREQ→GPIO3 (CON2_34)
   XRESET→GPIO6 (CON2_35) 
*/
#ifndef VS1053B_PIN_XCS
#define VS1053B_PIN_XCS GPIO2
#endif
#ifndef VS1053B_PIN_XDCS
#define VS1053B_PIN_XDCS GPIO4
#endif
#ifndef VS1053B_PIN_DREQ
#define VS1053B_PIN_DREQ GPIO3
#endif
#ifndef VS1053B_PIN_XRESET
#define VS1053B_PIN_XRESET GPIO6
#endif

#ifndef VS1053B_SPI
#define VS1053B_SPI SPI0 // SCK/MISO/MOSI del header de la EDU-CIAA
#endif

/* === Registros / bits (VS1053b) === */
#define VS1053B_SCI_MODE 0x00
#define VS1053B_SCI_STATUS 0x01
#define VS1053B_SCI_BASS 0x02
#define VS1053B_SCI_CLOCKF 0x03
#define VS1053B_SCI_VOL 0x0B
#define VS1053B_SM_SDINEW (1U << 11)

/* === SPI y sincronización con DREQ === */
static inline uint8_t vs1053b_spi_trx(uint8_t b)
{
    uint8_t r = 0;
    spiWriteRead(VS_SPI0, &b, &r, 1);
    return r; 
}
static inline uint8_t vs1053b_spi_trx_safe(uint8_t b)
{
    uint8_t r = 0;
    spiWriteRead(VS1053B_SPI, &b, &r, 1);
    return r;
}
static inline void vs1053b_wait_dreq(void)
{
    while (!gpioRead(VS1053B_PIN_DREQ))
    {
        delay(1);
    }
}

/* === SCI (registros) === */
static inline void vs1053b_sci_write(uint8_t reg, uint16_t val)
{
    vs1053b_wait_dreq();
    gpioWrite(VS1053B_PIN_XCS, FALSE);
    vs1053b_spi_trx_safe(0x02); // WRITE
    vs1053b_spi_trx_safe(reg);
    vs1053b_spi_trx_safe((uint8_t)(val >> 8));
    vs1053b_spi_trx_safe((uint8_t)(val & 0xFF));
    gpioWrite(VS1053B_PIN_XCS, TRUE);
}
static inline uint16_t vs1053b_sci_read(uint8_t reg)
{
    uint16_t v = 0;
    vs1053b_wait_dreq();
    gpioWrite(VS1053B_PIN_XCS, FALSE);
    vs1053b_spi_trx_safe(0x03); // READ
    vs1053b_spi_trx_safe(reg);
    v = ((uint16_t)vs1053b_spi_trx_safe(0xFF)) << 8;
    v |= (uint16_t)vs1053b_spi_trx_safe(0xFF);
    gpioWrite(VS1053B_PIN_XCS, TRUE);
    return v;
}

/* === SDI (datos crudos / plugin / MIDI con prefijo 0x00) === */
static inline void vs1053b_sdi_write(const uint8_t *buf, uint16_t len)
{
    while (len)
    {
        vs1053b_wait_dreq();
        gpioWrite(VS1053B_PIN_XDCS, FALSE);
        uint16_t n = (len > 32) ? 32 : len; // ráfagas cortas
        for (uint16_t i = 0; i < n; i++)
            vs1053b_spi_trx_safe(*buf++);
        gpioWrite(VS1053B_PIN_XDCS, TRUE);
        len -= n;
    }
}

/* === Init: GPIO/SPI/Reset/Setup básico === */
static inline void vs1053b_gpio_spi_init(void)
{
    gpioInit(VS1053B_PIN_XCS, GPIO_OUTPUT);
    gpioInit(VS1053B_PIN_XDCS, GPIO_OUTPUT);
    gpioInit(VS1053B_PIN_XRESET, GPIO_OUTPUT);
    gpioInit(VS1053B_PIN_DREQ, GPIO_INPUT);
    gpioWrite(VS1053B_PIN_XCS, TRUE);
    gpioWrite(VS1053B_PIN_XDCS, TRUE);
    gpioWrite(VS1053B_PIN_XRESET, TRUE);
    spiConfig(VS1053B_SPI); // modo 0, ~1 MHz inicial
}
static inline void vs1053b_hard_reset(void)
{
    gpioWrite(VS1053B_PIN_XRESET, FALSE);
    delay(2);
    gpioWrite(VS1053B_PIN_XRESET, TRUE);
    delay(10);
}
static inline void vs1053b_soft_init(void)
{
    uint16_t m = vs1053b_sci_read(VS1053B_SCI_MODE);
    vs1053b_sci_write(VS1053B_SCI_MODE, m | VS1053B_SM_SDINEW);
    vs1053b_sci_write(VS1053B_SCI_CLOCKF, 0x9800); // clock interno típico
    vs1053b_sci_write(VS1053B_SCI_BASS, 0x0000);   // EQ plano
    delay(5);
}
static inline void vs1053b_init_all(void)
{
    vs1053b_gpio_spi_init();
    vs1053b_hard_reset();
    vs1053b_soft_init();
}
static inline void vs1053b_set_volume(uint8_t left, uint8_t right)
{
    // 0x00 = máximo, 0xFE = muy atenuado (más grande = más bajo)
    uint16_t v = ((uint16_t)left << 8) | right;
    vs1053b_sci_write(VS1053B_SCI_VOL, v);
}

#endif /* VS1053B_SAPI_H */
