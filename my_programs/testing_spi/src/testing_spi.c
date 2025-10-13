// vs1053_smoketest.c (sAPI con spiInit/spiWrite/spiRead)
#include "sapi.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ======= Pines EDU-CIAA (sAPI) ======= */
#define PIN_XCS    GPIO2   // VS1053 XCS (SCI CS)
#define PIN_XDCS   GPIO4   // VS1053 XDCS (SDI CS)
#define PIN_DREQ   GPIO3   // VS1053 DREQ (IN)
#define PIN_XRESET GPIO6   // VS1053 XRESET

/* ======= Registros/constantes VS1053 ======= */
#define SCI_MODE    0x00
#define SCI_STATUS  0x01
#define SCI_BASS    0x02
#define SCI_CLOCKF  0x03
#define SCI_VOL     0x0B
#define SM_SDINEW   (1U<<11)

#define VS_SPI SPI0

/* ======= Helpers de SPI (sAPI v0.6.x) ======= */
static inline void spi_write_bytes(const uint8_t* buf, uint32_t len){
   // spiWrite no acepta const*, casteamos de forma segura
   spiWrite(VS_SPI, (uint8_t*)buf, len);
}
static inline void spi_read_bytes(uint8_t* buf, uint32_t len){
   spiRead(VS_SPI, buf, len); // sAPI envía dummies (típicamente 0xFF) para clockear
}

/* ======= VS1053 helpers ======= */
static void vs_gpioInit(void){
   gpioInit(PIN_XCS,    GPIO_OUTPUT);
   gpioInit(PIN_XDCS,   GPIO_OUTPUT);
   gpioInit(PIN_XRESET, GPIO_OUTPUT);
   gpioInit(PIN_DREQ,   GPIO_INPUT);

   gpioWrite(PIN_XCS, TRUE);
   gpioWrite(PIN_XDCS, TRUE);
   gpioWrite(PIN_XRESET, TRUE);
}

static void vs_waitDREQ(void){
   while(!gpioRead(PIN_DREQ)){ delay(1); }
}

static void sciWrite(uint8_t addr, uint16_t data){
   uint8_t pkt[4] = { 0x02, addr, (uint8_t)(data>>8), (uint8_t)(data&0xFF) };
   vs_waitDREQ();
   gpioWrite(PIN_XCS, FALSE);
   spi_write_bytes(pkt, sizeof(pkt));
   gpioWrite(PIN_XCS, TRUE);
}

static uint16_t sciRead(uint8_t addr){
   uint8_t cmd[2] = { 0x03, addr };
   uint8_t rx[2]  = { 0xFF, 0xFF };

   vs_waitDREQ();
   gpioWrite(PIN_XCS, FALSE);

   // 1) Enviar opcode+addr
   spi_write_bytes(cmd, sizeof(cmd));
   // 2) Leer 2 bytes de dato
   spi_read_bytes(rx, sizeof(rx));

   gpioWrite(PIN_XCS, TRUE);

   return ((uint16_t)rx[0]<<8) | rx[1];
}

static void hardResetVS(void){
   gpioWrite(PIN_XRESET, FALSE);
   delay(2);     // >= 1 ms
   gpioWrite(PIN_XRESET, TRUE);
   delay(10);    // estabiliza
}

/* ======= Prueba ======= */
int main(void){
   boardInit();
   uartConfig(UART_USB, 115200);
   printf("\r\n[VS1053 Smoke Test] Inicio\r\n");

   /* SPI0: modo 0, MSB first (por defecto). */
   spiInit(VS_SPI); // (spiConfig es alias)

   vs_gpioInit();
   hardResetVS();

   if(!gpioRead(PIN_DREQ)){
      printf("ATENCION: DREQ no alto tras reset, esperando...\r\n");
      vs_waitDREQ();
      printf("DREQ alto.\r\n");
   } else {
      printf("DREQ OK.\r\n");
   }

   /* Asegurar interfaz nueva y setear CLOCKF */
   uint16_t mode0 = sciRead(SCI_MODE);
   sciWrite(SCI_MODE, mode0 | SM_SDINEW);
   sciWrite(SCI_CLOCKF, 0x9800);
   delay(5);

   uint16_t mode  = sciRead(SCI_MODE);
   uint16_t stat  = sciRead(SCI_STATUS);
   uint16_t clock = sciRead(SCI_CLOCKF);
   printf("SCI_MODE   = 0x%04X\r\n", mode);
   printf("SCI_STATUS = 0x%04X\r\n", stat);
   printf("SCI_CLOCKF = 0x%04X\r\n", clock);

   sciWrite(SCI_VOL, 0x1818);
   uint16_t vol = sciRead(SCI_VOL);
   printf("SCI_VOL    = 0x%04X\r\n", vol);

   gpioInit(LED1, GPIO_OUTPUT);
   printf("Test terminado. Si ves estos prints, SPI/SCI OK.\r\n");

   while(TRUE){
      gpioToggle(LED1);
      delay(500);
   }
   return 0;
}
