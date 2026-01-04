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
   float ax, ay, az;
   float gx, gy, gz;
   uint8_t botones[4];
   uint8_t checksum;   // XOR de todos los bytes (excepto headers y checksum)
} IMUMessage;

/* Detecta strum por pico en gz (siempre arriba->abajo: usamos gz > TH) */
static bool detectStrum(float gz, uint32_t now_ms)
{
   const float TH = 350.0f;         // AJUSTAR segun tu mock/IMU real
   const uint32_t COOLDOWN = 100;   // ms (anti doble disparo)

   static uint32_t lastStrumMs = 0;
   static bool above = false;

   bool nowAbove = (gz > TH);
   bool event = false;

   if (!above && nowAbove && (now_ms - lastStrumMs) > COOLDOWN) {
      event = true;
      lastStrumMs = now_ms;
   }

   above = nowAbove;
   return event;
}

/* Mapea fuerza (gz) a velocity 0..127 (simple, ajustable) */
static uint8_t velocityFromGz(float gz)
{
   float a = fabsf(gz);
   // Ganancia simple. Ajustala si tu gz esta en rad/s o en otra escala.
   int v = (int)(40.0f + a * 0.25f);
   if (v < 20) v = 20;
   if (v > 127) v = 127;
   return (uint8_t)v;
}

static void setupVS1053(void)
{
   printf("\r\n[VS1053 MIDI Test] Setup\r\n");
   vs_pinsInit();
   vs_spiInit();
   hardResetVS();
   softInitVS();
   loadUserCodeFromTables();
   startRTMIDI();

   midiProgramChange(0, 26); // 25 = Acoustic Guitar (GM) (si queres otro, cambialo)
   midiSchedInit();
}

int main(void)
{
   boardConfig();

   uartConfig(UART_USB, 115200);
   uartConfig(UART_232, 115200);
   setvbuf(stdout, NULL, _IONBF, 0);

   uartWriteString(UART_USB, "EDU-CIAA lista (UART_232) - modo STRUM\r\n");

   setupVS1053();

   IMUMessage msg;
   uint8_t *p = (uint8_t *)&msg;
   uint32_t idx = 0;
   bool synced = false;
   bool gotAA = false;
   uint32_t lastCkErrMs = 0;

   const ButtonChord *currentChord = NULL;

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

         if (idx >= sizeof(IMUMessage))
         {
            // Validar checksum
            uint8_t chk = 0;
            for (uint32_t i = 2; i < sizeof(IMUMessage) - 1; i++)
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

            // 1) Seleccionar acorde por botones (no suena)
            uint8_t b = obtenerValorBotones(msg.botones);
            currentChord = getChordFromButtons(b);

            // Debug opcional
            char dbg[96];
            snprintf(dbg, sizeof(dbg),
                     "b=%u -> %s (root=%u tipo=%d) gz=%.2f\r\n",
                     b, currentChord->nombre, currentChord->root, currentChord->tipo, msg.gz);
            uartWriteString(UART_USB, dbg);

            // 2) Detectar evento strum por pico en gz
            if (currentChord && currentChord->tipo != CHORD_NONE)
            {
               if (detectStrum(msg.gz, now))
               {
                  uint8_t vel = velocityFromGz(msg.gz);
                  debugPrintChord(currentChord);  // opcional
                  strumChord(currentChord, vel, now);
               }
            }
         }
      }
   }
}
