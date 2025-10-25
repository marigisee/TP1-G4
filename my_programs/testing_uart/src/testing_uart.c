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

   while (true) {
      uint8_t byte;
      if (uartReadByte(UART_232, &byte)) {     
         p[idx++] = byte;
         if (idx >= sizeof(IMUMessage)) {
            char buffer[200];
            snprintf(buffer, sizeof(buffer),
                     "Recibido: ax=%.2f ay=%.2f az=%.2f | gx=%.2f gy=%.2f gz=%.2f | B=[%d %d %d %d]\r\n",
                     msg.ax, msg.ay, msg.az,
                     msg.gx, msg.gy, msg.gz,
                     msg.botones[0], msg.botones[1], msg.botones[2], msg.botones[3]);
            uartWriteString(UART_USB, buffer);
            idx = 0;
         }
      }
   }
}
