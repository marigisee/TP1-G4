#include <stdio.h>
#include "sapi.h"
#include "lowlevel.h"
#include "loader_tables.h"
#include "midi_sdi.h"
#include "chords_map.h"
#include <string.h>

typedef struct __attribute__((packed))
{
   float ax, ay, az;
   float gx, gy, gz;
   uint8_t botones[4];
} IMUMessage;


void setup(void)
{
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

/**
 * @brief Programa principal: recibe IMU por UART_232 y toca acordes según botones.
 */
int main(void)
{

   // --> inicialización de placa y UARTs
   boardConfig();
   uartConfig(UART_USB, 115200);
   uartConfig(UART_232, 115200);
   uartWriteString(UART_USB, "EDU-CIAA lista para recibir datos IMU (UART_232)...\r\n");

   // --> variables
   IMUMessage msg;
   uint8_t lastB = 0xFF; 
   const ButtonChord *lastChord = NULL;
   uint8_t *p = (uint8_t *)&msg;
   uint32_t idx = 0;

   // --> bucle principal

   while (true)
   {
      uint8_t byte;
      if (uartReadByte(UART_232, &byte)) // --> si llego un byte nuevo por UART_232
      {
         p[idx++] = byte; // --> almaceno el byte en el buffer

         // C
         if (idx >= sizeof(IMUMessage)) // --> si se obtuvieron todos los bytes del struct
         {
            idx = 0; // reinicia el buffer

            uint8_t b = obtenerValorBotones(msg.botones); // --> se arma el valor binario de los 4 botones

            const ButtonChord *chord = getChordFromButtons(b); // --> se obtiene el acorde (nombre)

            // Si cambió el acorde, detener el anterior y tocar el nuevo
            if (chord != lastChord)
            {
               if (lastChord && lastChord->tipo != CHORD_NONE)
                  stopChord(lastChord); // --> NOTE OFFs de las notas del acorde anterior

               if (chord->tipo != CHORD_NONE)
                  playChord(chord); // NOTE ONs de las notas del nuevo acorde

               lastChord = chord;

               // Log opcional por puerto USB
               char buffer[128];
               snprintf(buffer, sizeof(buffer),
                        "Acorde: %s (root=%d, tipo=%d)\r\n",
                        chord->nombre, chord->root, chord->tipo);
               uartWriteString(UART_USB, buffer);
            }
         }
      }
   }
}
