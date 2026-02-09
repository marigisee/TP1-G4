#include "sapi.h"
#include <string.h>
#include <stdio.h>

typedef struct __attribute__((packed)) {
   float   ax, ay, az;
   float   gx, gy, gz;
   uint8_t botones[4];
} IMUMessage;

int main(void) {
   boardConfig();

   uartConfig(UART_USB, 115200);
   uartWriteString(UART_USB, "EDU-CIAA lista para recibir datos IMU (UART_232)...\r\n");

   uartConfig(UART_232, 115200);   

   IMUMessage msg;
   uint8_t* p = (uint8_t*)&msg;
   uint32_t idx = 0;

   uint32_t rxCount = 0;
   tick_t lastTick = 0;

   while (true) {
      uint8_t byte;

      if (uartReadByte(UART_232, &byte)) {
         rxCount++;
      }

      if (tickRead() - lastTick >= 1000) { // cada 1 segundo
         lastTick = tickRead();
         char buf[64];
         snprintf(buf, sizeof(buf),
                  "Bytes recibidos UART_232: %lu\r\n",
                  (unsigned long)rxCount);
         uartWriteString(UART_USB, buf);
      }
   }
}
