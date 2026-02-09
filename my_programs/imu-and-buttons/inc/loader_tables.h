#include "rtmidi1053b_tables.h"   

// ---> En este header se definen las funciones necesarias para cargar el plugin MIDI y para iniciarlizar el mismo.

// --> Carga del plugin
static inline void loadUserCodeFromTables(void){
   const size_t N = sizeof(dtab)/sizeof(dtab[0]);
   for(size_t i = 0; i < N; ++i){
      uint8_t  reg  = atab[i];      // 7 = WRAMADDR, 6 = WRAM (según tabla)
      uint16_t data = dtab[i];
      waitDREQ();
      sciWrite(reg, data);
   }
}

// --> Inicializar el plugin
static inline void startRTMIDI(void){
   waitDREQ();
   sciWrite(SCI_AIADDR, 0x0050); // entry point RT-MIDI en VS1053B
   waitDREQ();
}
