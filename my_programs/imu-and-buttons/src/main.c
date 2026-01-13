#include <stdio.h>
#include <string.h>
#include <math.h>

#include "sapi.h"
#include "lowlevel.h"
#include "loader_tables.h"
#include "midi_sdi.h"
#include "chords_map.h"
#include "playing_chords.h"
#include "midi_scheduler.h"

typedef struct __attribute__((packed))
{
   uint8_t header1;    // 0xAA (sync)
   uint8_t header2;    // 0x55 (sync)
   uint8_t botones[4]; // b0..b3 (acordes)
   uint8_t velocity;   // velocity MIDI
   uint8_t checksum;   // XOR de bytes [2..N-2]
} ChordMessage;

static void setupVS1053(void)
{
   printf("\r\n[VS1053 MIDI Test] Setup\r\n");
   vs_pinsInit();
   vs_spiInit();
   hardResetVS();
   softInitVS();
   loadUserCodeFromTables();
   startRTMIDI();

   midiProgramChange(0, 26); // 25 = Acoustic Guitar (GM)
   midiSchedInit();
}

int main(void)
{
   boardConfig();

   uartConfig(UART_USB, 115200);
   uartConfig(UART_232, 115200);
   setvbuf(stdout, NULL, _IONBF, 0);

   uartWriteString(UART_USB, "EDU-CIAA lista (UART_232) - IMU + Botones\r\n");

   setupVS1053();

   ChordMessage msg;
   uint8_t *p = (uint8_t *)&msg;
   uint32_t idx = 0;
   bool synced = false;
   bool gotAA = false;
   uint32_t lastCkErrMs = 0;

   while (true)
   {
      uint32_t now = tickRead();
      midiSchedProcess(now); // SIEMPRE (para que salgan NOTE OFF a tiempo)

      uint8_t byte;
      while (uartReadByte(UART_232, &byte))
      {
         // Buscar secuencia de sincronizacion 0xAA 0x55
         if (!synced)
         {
            if (!gotAA)
            {
               gotAA = (byte == 0xAA);
               continue;
            }

            // gotAA == true
            if (byte == 0x55)
            {
               synced = true;
               gotAA = false;
               idx = 0;
               p[idx++] = 0xAA;
               p[idx++] = 0x55;
            }
            else
            {
               gotAA = (byte == 0xAA);
            }
            continue;
         }

         // Recibir resto del paquete
         p[idx++] = byte;

         if (idx >= sizeof(ChordMessage))
         {
            // Validar checksum
            uint8_t chk = 0;
            for (uint32_t i = 2; i < sizeof(ChordMessage) - 1; i++)
            {
               chk ^= p[i];
            }

            if (chk != msg.checksum)
            {
               if ((now - lastCkErrMs) > 200)
               {
                  uartWriteString(UART_USB, "[ERROR] Checksum invalido\r\n");
                  lastCkErrMs = now;
               }
               synced = false;
               gotAA = false;
               idx = 0;
               continue;
            }

            // Paquete valido, procesar
            idx = 0;
            synced = false; // Buscar proximo header
            gotAA = false;

            // 1) Obtener acorde de los botones
            uint8_t b = obtenerValorBotones(msg.botones);
            const ButtonChord *chord = getChordFromButtons(b);

            // Debug
            char dbg[96];
            snprintf(dbg, sizeof(dbg),
                     "b=%u -> %s (root=%u tipo=%d) vel=%u\r\n",
                     b, chord->nombre, chord->root, chord->tipo, msg.velocity);
            uartWriteString(UART_USB, dbg);

            // 2) Tocar acorde si es valido
            if (chord && chord->tipo != CHORD_NONE)
            {
               debugPrintChord(chord);
               strumChord(chord, msg.velocity, now);
            }
         }
      }
   }
}
