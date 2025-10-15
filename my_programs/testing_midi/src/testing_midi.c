#include <stdio.h>
#include "sapi.h"
#include "lowlevel.h"
#include "loader_tables.h"
#include "midi_sdi.h"

static void playChord(void)
{               // C4 (60), E4 (64), G4 (67)
   delay(5000); // espera 5 s antes de tocar (como en tu ejemplo)
   midiNoteOn(0, 60, 100);
   midiNoteOn(0, 64, 100);
   midiNoteOn(0, 67, 100);
   delay(2000); // suena 2 s
   midiNoteOff(0, 60, 64);
   midiNoteOff(0, 64, 64);
   midiNoteOff(0, 67, 64);
}


void setup(void) {
   uartConfig(UART_USB, 115200);
   setvbuf(stdout, NULL, _IONBF, 0);
   printf("\r\n[VS1053 MIDI Test] Setup\r\n");
   vs_pinsInit();
   vs_spiInit();
   hardResetVS();
   softInitVS();
   loadUserCodeFromTables();
   startRTMIDI();
   midiProgramChange(0, 1); // canal 0, instrumento 1
}

int main(void)
{
   boardInit();


   /* Activar interfaz nueva y (opcional) clock interno más alto */
   // sciWrite(SCI_CLOCKF, 0x9800); delay(5);

   // sciWrite(SCI_VOL, 0x2020);  // más bajo = más fuerte; 0x2020 ~ moderado

   setup();
   gpioInit(LED1, GPIO_OUTPUT);

   printf("Setup finalizado.\r\n");
   while (true)
   {
      playChord();
      /* Blink suave entre acordes para ver que el programa sigue vivo */
      for (int i = 0; i < 4; i++)
      {
         gpioToggle(LED1);
         delay(250);
      }
   }
   return 0;
}
