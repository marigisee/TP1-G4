/*=============================================================================
 * Copyright (c) 2025, Marina Cuello <marina.cuello@alu.ing.unlp.edu.ar>
 * All rights reserved.
 * License:  (see LICENSE.txt)
 * Date: 2025/10/09
 * Version: 1
 *===========================================================================*/

/*=====[Inclusions of function dependencies]=================================*/
#include "sapi.h"
#include "board.h"
#include "vs1053_lowlevel.h"
#include "vs1053_midi.h"
#include "loader_tables.h"      // para atab, dtab
#include "rtmidi1053b_tables.h" // o el archivo que define dtab/atab
#include <stdio.h>

#define TICKRATE_HZ (1000)
extern volatile uint32_t tick_ct;

void delay(uint32_t ms)
{
   uint32_t end = tick_ct + ms;
   while (tick_ct < end)
      __WFI();
}

void SysTick_Handler(void)
{
   tick_ct++;
}

int main(void)
{
   SystemCoreClockUpdate();
   Board_Init();
   SysTick_Config(SystemCoreClock / TICKRATE_HZ);

   // Inicializar VS1053
   initVS1053GPIO();
   initVS1053SPI();
   hardResetVS();
   softInitVS();

   // Cargar plugin RT?MIDI en el VS1053
   loadUserCodeFromTables();
   startRTMIDI();

   // Seleccionar instrumento: guitarra acústica (programa 24)
   midiProgramChange(0, 24);

   while (1)
   {
      // Secuencia arpegiada C4 ? E4 ? G4

      midiNoteOn(0, 60, 100);
      delay(200);
      midiNoteOff(0, 60, 64);

      midiNoteOn(0, 64, 100);
      delay(200);
      midiNoteOff(0, 64, 64);

      midiNoteOn(0, 67, 100);
      delay(200);
      midiNoteOff(0, 67, 64);

      delay(400); // pausa antes de volver a empezar
   }
}
