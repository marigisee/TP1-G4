#include "sapi.h"
#include "vs1053b_sapi.h"
#include "vs1053b_midi.h"

int main(void)
{
   boardInit();
   uartConfig(UART_USB, 115200); // consola (opcional)

   vs1053b_init_all();             // GPIO/SPI + reset + setup
   vs1053b_set_volume(0x18, 0x18); // volumen cómodo

   // Program Change: trompeta (56)
   midiProgramChange(0, 56);

   // Beep: C4 (60) por 500 ms
   midiNoteOn(0, 60, 100);
   delay(500);
   midiNoteOff(0, 60, 64);

   // Loop inactivo
   while (TRUE)
   {
      delay(1000);
   }
   return 0;
}
