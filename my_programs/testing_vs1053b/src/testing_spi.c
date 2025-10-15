// vs1053_smoketest.c
#include "sapi.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ======= Pines EDU-CIAA (sAPI) ======= */
#define PIN_XCS GPIO2    // VS1053 XCS (SCI CS)
#define PIN_XDCS GPIO4   // VS1053 XDCS (SDI CS)
#define PIN_DREQ GPIO3   // VS1053 DREQ (IN)
#define PIN_XRESET GPIO6 // VS1053 XRESET

/* ======= Registros/constantes VS1053 ======= */
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_VOL 0x0B
#define SM_SDINEW (1U << 11)

#define VS_SPI SPI0

/* ======= SPI helpers (sAPI) ======= */
static inline uint8_t spi_trx(uint8_t b)
{
    uint8_t r = 0;
    spiWrite(SPI0, &b, 1);   // transmite 1 byte
    spiRead (SPI0, &r, 1);   // lee 1 byte (sAPI clockea con dummies 0xFF)
    return r;
}

/* ======= VS1053 helpers ======= */
static void vs_gpioInit(void)
{
    gpioInit(PIN_XCS, GPIO_OUTPUT);
    gpioInit(PIN_XDCS, GPIO_OUTPUT);
    gpioInit(PIN_XRESET, GPIO_OUTPUT);
    gpioInit(PIN_DREQ, GPIO_INPUT);

    gpioWrite(PIN_XCS, TRUE);    // reposo alto
    gpioWrite(PIN_XDCS, TRUE);   // reposo alto
    gpioWrite(PIN_XRESET, TRUE); // reposo alto
}

static void vs_waitDREQ(void)
{
    while (!gpioRead(PIN_DREQ))
    {
        delay(1);
    }
}

static void sciWrite(uint8_t addr, uint16_t data)
{
    vs_waitDREQ();
    gpioWrite(PIN_XCS, FALSE);
    spi_trx(0x02); // WRITE opcode
    spi_trx(addr);
    spi_trx((uint8_t)(data >> 8));
    spi_trx((uint8_t)(data & 0xFF));
    gpioWrite(PIN_XCS, TRUE);
}

static uint16_t sciRead(uint8_t addr)
{
    uint16_t v;
    vs_waitDREQ();
    gpioWrite(PIN_XCS, FALSE);
    spi_trx(0x03); // READ opcode
    spi_trx(addr);
    v = ((uint16_t)spi_trx(0xFF)) << 8;
    v |= (uint16_t)spi_trx(0xFF);
    gpioWrite(PIN_XCS, TRUE);
    return v;
}

static void hardResetVS(void)
{
    gpioWrite(PIN_XRESET, FALSE);
    delay(2); // ≥1 ms
    gpioWrite(PIN_XRESET, TRUE);
    delay(10); // espera estabilizar
}

/* ======= Prueba ======= */
int main(void)
{
    boardInit();
    uartConfig(UART_USB, 115200);
    printf("\r\n[VS1053 Smoke Test] Inicio\r\n");

    /* SPI0: modo 0, MSB first (por defecto). Arrancamos ~1 MHz. */
    spiConfig(VS_SPI);

    vs_gpioInit();
    hardResetVS();

    /* Verifico que DREQ sube */
    if (!gpioRead(PIN_DREQ))
    {
        printf("ERROR: DREQ no está alto tras reset.\r\n");
        // igual seguimos unos ms más por si tarda:
        vs_waitDREQ();
        printf("DREQ alto luego de espera.\r\n");
    }
    else
    {
        printf("DREQ OK.\r\n");
    }

    /* Asegurar interfaz nueva y setear CLOCKF (comun en VS1053) */
    uint16_t mode = sciRead(SCI_MODE);
    sciWrite(SCI_MODE, mode | SM_SDINEW);
    sciWrite(SCI_CLOCKF, 0x9800); // sube clock interno (ajustable)
    delay(5);

    /* Leer registros base para confirmar bus */
    uint16_t mode2 = sciRead(SCI_MODE);
    uint16_t status = sciRead(SCI_STATUS);
    uint16_t clockf = sciRead(SCI_CLOCKF);

    printf("SCI_MODE   = 0x%04X\r\n", mode2);
    printf("SCI_STATUS = 0x%04X\r\n", status);
    printf("SCI_CLOCKF = 0x%04X\r\n", clockf);

    /* Seteo un volumen y lo leo */
    sciWrite(SCI_VOL, 0x1818); // más grande = más atenuado
    uint16_t vol = sciRead(SCI_VOL);
    printf("SCI_VOL    = 0x%04X\r\n", vol);

    /* Señal visual: blink si todo respondió */
    gpioInit(LED1, GPIO_OUTPUT);
    printf("Test terminado. Si ves estos prints, SPI/SCI están OK.\r\n");

    while (TRUE)
    {
        gpioToggle(LED1);
        delay(500);
    }
    return 0;
}
